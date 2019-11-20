/* my submission :) */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/namei.h>
#include <sys/errno.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/filio.h>
#include <sys/kthread.h>
#include <sys/rwlock.h>
#include <sys/syscallargs.h>
#include <sys/poll.h>

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/smr.h>

#include "acct.h"

/*
 * fancy way to save space and make
 * the differences easier
 */
union acct_generic {
	struct acct_common all;
	struct acct_fork acctfork;
	struct acct_exec acctexec;
	struct acct_exit acctexit;
};

/*
 * for the simple queue
 */
struct acctq {
	SIMPLEQ_ENTRY(acctq) entry;
	union acct_generic data;
};

int 		filt_acct_read(struct knote *, long);
void 		filt_acct_rdetach(struct knote *);

/*
 * used by kqfilter
 */
const struct filterops acct_rd_filtops = {
	1,
	NULL,
	filt_acct_rdetach,
	filt_acct_read
};

/*
 * since we only have a single minor we only have one acct_softc thus we
 * should just dump everything into global
 */
struct selinfo 	acct_rsel;	/* read select */

/*
 * computing times
 */
typedef u_int16_t comp_t;
static comp_t 	encode_comp_t(u_long, u_long);

/*
 * Globals we use. Please ensure that your locking before playing with
 * these.
 */
SIMPLEQ_HEAD(acct_queue, acctq) acctqueue;
	int 		counter = 0;
	bool 		nonblock = false;
	bool 		isused = false;

/*
 * run when someone attempts to call open() on this
 * throws errors if someone else is using this device
 * or attempts to write to it.
 */
	int
			acctopen      (dev_t dev, int flag, int mode, struct proc * p)
{
	if (minor(dev) != 0)
		return (ENXIO);

	if (flag & FWRITE)
		return (EPERM);

	KERNEL_LOCK();
	nonblock = flag & O_NONBLOCK;
	KERNEL_UNLOCK();

	int 		error = 0;
	KERNEL_LOCK();
	if (isused) {
		error = EPERM;
	} else {
		counter = 0;
		isused = true;
	}
	KERNEL_UNLOCK();

	return error;
}

/*
 * run when someone attempts to close() on this
 * clears all the information we've generated.
 */
int
acctclose(dev_t dev, int flag, int mode, struct proc * p)
{
	KERNEL_LOCK();
	/* need to clear the simple queue */
	struct acctq   *n1;
	while (!SIMPLEQ_EMPTY(&acctqueue)) {
		n1 = SIMPLEQ_FIRST(&acctqueue);
		SIMPLEQ_REMOVE_HEAD(&acctqueue, entry);
		free(n1, M_DEVBUF, sizeof(n1));
	}
	isused = false;
	KERNEL_UNLOCK();

	return (0);
}

/*
 * run when someone attempts to read on this device
 * returns accounts of forks, execs and exits.
 */
int
acctread(dev_t dev, struct uio * uio, int flag)
{
	int 		error = 0;

	if (uio->uio_offset < 0)
		return (EINVAL);

	KERNEL_LOCK();
	while (SIMPLEQ_EMPTY(&acctqueue)) {
		if (nonblock) {
			KERNEL_UNLOCK();
			return (0);
		}
		/* wait for a message via tsleep */
		KERNEL_UNLOCK();
		int 		res = tsleep(&acctqueue, 0, "acct: nothing in queue", 0);
		/* a signal was sent if res != 0 */
		if (res != 0) {
			return (0);
		}
		KERNEL_LOCK();
	}
	/* dequeue a message then add the counter then send it through */
	struct acctq   *node = SIMPLEQ_FIRST(&acctqueue);
	union acct_generic *temp = &(node->data);
	KERNEL_UNLOCK();

	/* check if the residual is giving enough space to send the temp */
	if (uio->uio_resid >= temp->all.ac_len) {
		/* we now know we're sending this bad boy */
		KERNEL_LOCK();
		SIMPLEQ_REMOVE_HEAD(&acctqueue, entry);
		KERNEL_UNLOCK();

		temp->all.ac_seq = counter;

		size_t 		org_offset = uio->uio_offset;
		void           *v;	/* bad name I know */
		size_t 		len;
		while (uio->uio_resid > 0) {
			size_t 		actual_offset = uio->uio_offset - org_offset;
			v = temp + actual_offset;
			len = temp->all.ac_len - actual_offset;

			/* check uio_rw for reading ONLY */
			if (uio->uio_rw != UIO_READ)
				return (EPERM);

			/* it's now safe to cast */
			if ((error = uiomove((void *) v, len, uio)) != 0)
				return (error);
			/* jump out of this mess */
			break;
		}
		KERNEL_LOCK();
		counter++;
		free(node, M_DEVBUF, sizeof(struct acctq));
		KERNEL_UNLOCK();
	}
	return (error);
}

/*
 * called when someone tries to write to the device
 * we don't support that operation
 */
int
acctwrite(dev_t dev, struct uio * uio, int flag)
{
	return (EOPNOTSUPP);
}

/*
 * ran when someone calls ioctl() on this device
 * supports FIONREAD and FIONBIO
 */
int
acctioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc * p)
{
	int 		error = 0;

	KERNEL_LOCK();
	struct acctq   *node = SIMPLEQ_EMPTY(&acctqueue) ? NULL : SIMPLEQ_FIRST(&acctqueue);
	union acct_generic *temp = node ? &(node->data) : NULL;
	KERNEL_UNLOCK();

	switch (cmd) {
	case FIONREAD:		/* get # bytes to read */
		*(int *) data = temp ? temp->all.ac_len : 0;
		break;
	case FIONBIO:		/* set/clear non-blocking i/o */
		nonblock = !nonblock;
		break;
	default:
		error = ENOTTY;
	}

	return (error);
}

/*
 * run when someone calls poll() on this device.
 */
int
acctpoll(dev_t dev, int events, struct proc * p)
{
	int 		revents = 0;
	if (events & (POLLIN | POLLRDNORM)) {
		KERNEL_LOCK();
		if (!SIMPLEQ_EMPTY(&acctqueue))
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &acct_rsel);
		KERNEL_UNLOCK();
	}
	return (revents);
}

/*
 * run when someone calls kqfilter on this device
 */
int
acctkqfilter(dev_t dev, struct knote * kn)
{
	struct klist   *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &acct_rsel.si_note;
		kn->kn_fop = &acct_rd_filtops;
		break;
	default:
		return (EINVAL);
	}

	SLIST_INSERT_HEAD(klist, kn, kn_selnext);

	return (0);
}

/*
 * run when the kernel first begins
 */
int
acctattach(int n)
{
	/* init queue */
	SIMPLEQ_INIT(&acctqueue);
	return (0);
}

/*
 * generates the acct struct from the proc struct
 */
void
acct_init(struct acct_common * ret, struct process * info)
{
	memcpy(ret->ac_comm, info->ps_comm, 16);	/* you gave us a magic
							 * size for comm */
	ret->ac_etime = (info->ps_tu).tu_runtime;
	ret->ac_btime = info->ps_start;
	ret->ac_pid = info->ps_pid;
	/* using the effective user id */
	ret->ac_uid = info->ps_ucred->cr_uid;
	ret->ac_gid = info->ps_ucred->cr_gid;

	if ((info->ps_flags & PS_CONTROLT) &&
	    info->ps_pgrp->pg_session->s_ttyp)
		ret->ac_tty = info->ps_pgrp->pg_session->s_ttyp->t_dev;
	else
		ret->ac_tty = NODEV;

	ret->ac_flag = info->ps_acflag;
}

/*
 * For the forking
 * Append information to the acctqueue
 */
void
acct_fork(struct process * info)
{
	KERNEL_LOCK();
	if (!isused) {
		KERNEL_UNLOCK();
		return;
	}
	KERNEL_UNLOCK();

	struct acctq   *item = malloc(sizeof(struct acctq), M_DEVBUF, M_WAITOK);
	acct_init(&(item->data.all), info->ps_pptr);

	(item->data).all.ac_type = ACCT_MSG_FORK;
	(item->data).all.ac_len = sizeof(struct acct_fork);
	/*
	 * ZZZ this isn't right but i'm pretty sure we're meant to change
	 * this in the read part just before sending out
	 */
	(item->data).all.ac_seq = counter;

	(item->data).acctfork.ac_cpid = info->ps_pid;

	KERNEL_LOCK();
	SIMPLEQ_INSERT_TAIL(&acctqueue, item, entry);
	KERNEL_UNLOCK();

	wakeup(&acctqueue);
	selwakeup(&acct_rsel);
	return;
}

/*
 * For the exec
 * Append information to the acctqueue
 */
void
acct_exec(struct process * info)
{
	KERNEL_LOCK();
	if (!isused) {
		KERNEL_UNLOCK();
		return;
	}
	KERNEL_UNLOCK();

	struct acctq   *item = malloc(sizeof(struct acctq), M_DEVBUF, M_WAITOK);
	acct_init(&(item->data.all), info);

	(item->data).all.ac_type = ACCT_MSG_EXEC;
	(item->data).all.ac_len = sizeof(struct acct_exec);
	/*
	 * ZZZ this isn't right but i'm pretty sure we're meant to change
	 * this in the read part just before sending out
	 */
	(item->data).all.ac_seq = counter;

	KERNEL_LOCK();
	SIMPLEQ_INSERT_TAIL(&acctqueue, item, entry);
	KERNEL_UNLOCK();

	wakeup(&acctqueue);
	selwakeup(&acct_rsel);
	return;
}

/*
 * For the exit
 * Append information to the acctqueue
 */
void
acct_exit(struct process * info)
{
	KERNEL_LOCK();
	if (!isused) {
		KERNEL_UNLOCK();
		return;
	}
	KERNEL_UNLOCK();

	struct acctq   *item = malloc(sizeof(struct acctq), M_DEVBUF, M_WAITOK);
	acct_init(&(item->data.all), info);

	(item->data).all.ac_type = ACCT_MSG_EXIT;
	(item->data).all.ac_len = sizeof(struct acct_exit);
	/*
	 * ZZZ this isn't right but i'm pretty sure we're meant to change
	 * this in the read part just before sending out
	 */
	(item->data).all.ac_seq = counter;

	/* ZZZ add ac_utime, ac_stime, ac_mem, ac_io */
	struct timespec ut, st, tmp;
	struct rusage  *r;
	int 		t;

	/* The amount of user and system time that was used */
	calctsru(&(info->ps_tu), &ut, &st, NULL);
	(item->data).acctexit.ac_utime = ut;
	(item->data).acctexit.ac_stime = st;

	/* The average amount of memory used */
	r = info->ps_ru;
	timespecadd(&ut, &st, &tmp);
	/* hz is defined in sys/time.h */
	t = tmp.tv_sec * hz + tmp.tv_nsec / (1000 * tick);
	if (t)
		(item->data).acctexit.ac_mem = (r->ru_ixrss + r->ru_idrss + r->ru_isrss) / t;
	else
		(item->data).acctexit.ac_mem = 0;

	/* The number of disk I/O operations done */
	(item->data).acctexit.ac_io = encode_comp_t(r->ru_inblock + r->ru_oublock, 0);

	KERNEL_LOCK();
	SIMPLEQ_INSERT_TAIL(&acctqueue, item, entry);
	KERNEL_UNLOCK();

	wakeup(&acctqueue);
	selwakeup(&acct_rsel);
	return;
}

/*
 * used for kqfilter
 */
void
filt_acct_rdetach(struct knote * kn)
{
	struct klist   *klist = &acct_rsel.si_note;

	SLIST_REMOVE(klist, kn, knote, kn_selnext);
}

/*
 * used for kqfilter
 */
int
filt_acct_read(struct knote * kn, long hint)
{
	int 		event = 0;

	KERNEL_LOCK();
	if (!SIMPLEQ_EMPTY(&acctqueue))
		event = 1;
	KERNEL_UNLOCK();

	return (event);
}

/*
 * Encode_comp_t converts from ticks in seconds and microseconds
 * to ticks in 1/AHZ seconds.  The encoding is described in
 * Leffler, et al., on page 63.
 */
#define	AHZ	64
#define	MANTSIZE	13	/* 13 bit mantissa. */
#define	EXPSIZE		3	/* Base 8 (3 bit) exponent. */
#define	MAXFRACT	((1 << MANTSIZE) - 1)	/* Maximum fractional value. */

static 		comp_t
encode_comp_t(u_long s, u_long ns)
{
	int 		exp      , rnd;

	exp = 0;
	rnd = 0;
	s *= AHZ;
	s += ns / (1000000000 / AHZ);	/* Maximize precision. */

	while (s > MAXFRACT) {
		rnd = s & (1 << (EXPSIZE - 1));	/* Round up? */
		s >>= EXPSIZE;	/* Base 8 exponent == 3 bit shift. */
		exp++;
	}

	/* If we need to round up, do it (and handle overflow correctly). */
	if (rnd && (++s > MAXFRACT)) {
		s >>= EXPSIZE;
		exp++;
	}
	/* Clean it up and polish it off. */
	exp <<= MANTSIZE;	/* Shift the exponent into place */
	exp += s;		/* and add on the mantissa. */
	return (exp);
}
