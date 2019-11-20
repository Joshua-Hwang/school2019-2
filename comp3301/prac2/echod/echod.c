#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>

#include <event.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/queue.h>

#define MAXBUFFER 256
#define MAXSOCK 256

/*
 * maybe useful functions. they're not static so the linker doesnt get upset.
 */
void	hexdump(const void *, size_t);
void	msginfo(const struct sockaddr_storage *, socklen_t, size_t);

__dead static void usage(void);

struct echod {
	TAILQ_ENTRY(echod) entry;
	struct event	ev;
};
TAILQ_HEAD(echod_list, echod);

static void
echod_recv(int fd, short revents, void *conn)
{
    char buffer[MAXBUFFER];
    struct sockaddr_storage address;
    socklen_t addresslen = sizeof(address);

    ssize_t retlen = recvfrom(fd, buffer, MAXBUFFER, 0,
                (struct sockaddr *)&address, &addresslen);

    if (retlen == -1)
        err(1, "%s", "recv");

    printf("%s\n", buffer);

    ssize_t sendlen = sendto(fd, buffer, retlen, 0,
            (struct sockaddr *)&address, addresslen);
    if (sendlen == -1)
        err(1, "%s", "sendto");
}

__dead static void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s [-46] [-l address] [-p port]\n", __progname);
	exit(1);
}

static void
echod_bind(struct echod_list *echods, sa_family_t af,
    const char *host, const char *port)
{
	/* my code below */
	struct addrinfo hints, *res, *res0;
	int error;
	int save_errno = ENOTCONN;
	/* ZZZ screw this s[] bullshit. We gotta add these to the echod
	 * ... but as events */
	/* int s[MAXSOCK]; */
	int nsock;
	const char *cause = "missing code";

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = af;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	error = getaddrinfo(host, port, &hints, &res0);
	if (error)
		errx(1, "%s", gai_strerror(error));
	nsock = 0;
	for (res = res0; res && nsock < MAXSOCK; res = res->ai_next) {
		int s = socket(res->ai_family, res->ai_socktype,
				res->ai_protocol);
		if (s == -1) {
			cause = "socket";
			continue;
		}

		if (bind(s, res->ai_addr, res->ai_addrlen) == -1 ) {
			cause = "bind";
			save_errno = errno;
			close(s);
			errno = save_errno;
			continue;
		}
		/*
		 * 5 is the backlog. Not sure if it should be changed.
		 * Set the socket to listen for connections. Doesn't block.
		 */
		//(void) listen(tmp, 5);

		nsock++;

		/* ZZZ generate an event from tmp. We malloc. */
		struct echod *item = malloc(sizeof(struct echod));
		event_set(&(item->ev), s, 0, NULL, NULL);
		TAILQ_INSERT_TAIL(echods, item, entry);
	}
	if (nsock == 0)
		err(1, "%s", cause);
	freeaddrinfo(res0);
	/* my code above */

	if (TAILQ_EMPTY(echods))
		errc(1, save_errno, "host %s port %s %s", host, port, cause);
}

int
main(int argc, char *argv[])
{
	struct echod *e;
	struct echod_list echods = TAILQ_HEAD_INITIALIZER(echods);
	sa_family_t af = AF_UNSPEC;
	const char *host = "localhost";
	const char *port = "3301";
	int ch;

	while ((ch = getopt(argc, argv, "46l:p:")) != -1) {
		switch (ch) {
		case '4':
			af = AF_INET;
			break;
		case '6':
			af = AF_INET6;
			break;

		case 'l':
			host = (strcmp(optarg, "*") == 0) ? NULL : optarg;
			break;
		case 'p':
			port = optarg;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	echod_bind(&echods, af, host, port); /* this works or exits */

	event_init();

	TAILQ_FOREACH(e, &echods, entry) {
		event_set(&e->ev, EVENT_FD(&e->ev), EV_READ|EV_PERSIST,
		    echod_recv, e);
		event_add(&e->ev, NULL);
	}

	event_dispatch();

	return (0);
}

/*
 * possibly useful functions
 */
void
hexdump(const void *d, size_t datalen)
{
	const uint8_t *data = d;
	size_t i, j = 0;

	for (i = 0; i < datalen; i += j) {
		printf("%4zu: ", i);
		for (j = 0; j < 16 && i+j < datalen; j++)
			printf("%02x ", data[i + j]);
		while (j++ < 16)
			printf("   ");
		printf("|");
		for (j = 0; j < 16 && i+j < datalen; j++)
			putchar(isprint(data[i + j]) ? data[i + j] : '.');
		printf("|\n");
	}
}

void
msginfo(const struct sockaddr_storage *ss, socklen_t sslen, size_t len)
{
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	int error;

	error = getnameinfo((const struct sockaddr *)ss, sslen,
	    hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
	    NI_NUMERICHOST | NI_NUMERICSERV);
	if (error != 0) {
		warnx("msginfo: %s", gai_strerror(error));
		return;
	}

	printf("host %s port %s bytes %zu\n", hbuf, sbuf, len);
}
