#include "tunnels.h"
#include "server.h"
#include "packet.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <errno.h>

/* networking */
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>

/* libevent */
#include <sys/time.h>
#include <event.h>

struct argstruct {
	int 		family;	/* AF_UNSPEC, AF_INET, AF_INET6 */
	bool 		daemonise;
	/* these are the local address and port */
	char           *address;/* NULL for no address */
	char           *port;	/* whatever the spec says is default */
	/* these are the ports to the server */
	int 		serverfd;
	char           *servername;
	char           *serverport;
	struct tunnelinfo **devices;
	int 		devcount;	/* number of devices */
};

__dead static void usage(void);
void 		parse_server(struct argstruct *);
void 		error_tunnelinfo_duplicates(struct argstruct *);
void 		send_to_server(struct argstruct *, uint8_t *, size_t);
void 		send_to_tunnels(struct parsedgre *, size_t, struct tunnelinfo **, int);
void 		listen_tunnel(int, short, void *);
void 		listen_server(int, short, void *);
void 		add_events(struct argstruct *);

/*
 * prints the usage statement for this program.
 */
__dead static void
usage()
{
	fprintf(stderr, "usage: %s [-46d] [-l address] [-p port]"
		" [-e /dev/tapX[@key]] [-i /dev/tunX[@key]]"
		" server [port]\n", getprogname());
	exit(1);
}

void
parse_server(struct argstruct * parsedargs)
{
	int 		res = create_server_socket(parsedargs->family,
				      parsedargs->address, parsedargs->port,
			    parsedargs->servername, parsedargs->serverport);

	if (res <= -1) {
		err(1, "%s\n", "failed to create socket");
	}
	parsedargs->serverfd = res;
}

/*
 * throws error on duplicates
 * algorithm implementation isn't considered i'm just scratching something
 * together
 */
void
error_tunnelinfo_duplicates(struct argstruct * parsedargs)
{
	struct tunnelinfo **arr = parsedargs->devices;
	int 		count = parsedargs->devcount;
	for (int i = 0; i < count; i++) {
		for (int j = i + 1; j < count; j++) {
			if (tunnelinfo_cmp(arr[i], arr[j]) == 0) {
				/* they are equal */
				err(1, "%s and %s share key:%d",
				     arr[i]->filepath, arr[j]->filepath,
				     arr[i]->key);
			}
		}
	}
}

/*
 * the packet must already have the GRE header attached
 */
void
send_to_server(struct argstruct * parsedargs, uint8_t * packet,
	       size_t packetsize)
{
	int 		fd = parsedargs->serverfd;

	ssize_t 	sendlen = send(fd, packet, packetsize, MSG_DONTWAIT);
	if (sendlen <= -1) {
		err(1, "Sending to server failed: %s", strerror(errno));
	}
	fprintf(stderr, "\n\nsend_to_server sent:------------------------------\n");
	hexdump(packet, packetsize);
}

/*
 * event for listening to the file descriptor of a tunnel or a tap
 * We listen (non-blocking so ensure error handling) and when a packet
 * arrives we wrap it in a GRE header and send it to the server.
 * This information is provided in the void *arg as the argstruct
 *
 * The GRE header we construct requires 64 bits if we're adding a key
 * and 32 bits otherwise. We need to correctly identify the protocol type
 * just from the void *arg.
 *
 * We do this by running across all args and finding the one that matches
 * our specific file descriptor.
 */
void
listen_tunnel(int fd, short flags, void *arg)
{
	struct argstruct *pargs = (struct argstruct *) arg;

	/* find the key associated with this fd */
	struct tunnelinfo **devices = pargs->devices;
	struct tunnelinfo *myinfo = NULL;
	for (int i = 0; i < pargs->devcount; i++) {
		if (devices[i]->fd == fd) {
			myinfo = devices[i];
			break;
		}
	}

	if (myinfo == NULL) {
		err(1, "Failed to find file descriptor: %d", fd);
	}
	/*
         * read from fd and then package and send to server
         * it's important we do this since the device may be a tap or tunnel
         */
	uint8_t 	buffer [MAXPACKET];
	ssize_t 	bytecount = read(fd, buffer, MAXPACKET);
	if (bytecount <= 0) {
		/*
	         * false positive on the read
	         * maybe next time boys
	         */
		return;
	}
	fprintf(stderr, "\n\nlisten_tunnel recv:-------------------------------\n");
	hexdump(buffer, bytecount);

	uint8_t 	packet [MAXPACKET];
	/* sanitize for safety */
	memset(packet, 0, MAXPACKET);
	bytecount = encapsulate_packet(packet, buffer, (size_t) bytecount, myinfo);

	send_to_server(pargs, packet, (size_t) bytecount);
}

void
send_to_tunnels(struct parsedgre * greinfo, size_t msgsize,
		struct tunnelinfo ** tinfos, int devcount)
{
	int 		res = find_tunnel(tinfos, devcount, greinfo->protocol,
				greinfo->kflag, greinfo->key);
	if (res == -1) {
		/* don't throw error, just drop it */
		return;
	}
	int 		fd = tinfos[res]->fd;

	write(fd, greinfo->payload, msgsize);
	/*
         * don't really care if it worked or not since there isn't anything
         * we can do about that
         */
	fprintf(stderr, "\n\nsend_to_tunnels sent:-----------------------------\n");
	hexdump(greinfo->payload, msgsize);
}

/*
 * event for listening to the file descriptor of the server
 * We listen to recv packets from the server. We then remove the GRE header,
 * recognise the key and send the packet to the right descriptor.
 * This can be determined by the void *arg which is the argstruct.
 */
void
listen_server(int fd, short flags, void *arg)
{
	struct argstruct *pargs = (struct argstruct *) arg;

	uint8_t 	packet [MAXPACKET];
	ssize_t 	bytecount = recv(fd, packet, MAXPACKET, MSG_DONTWAIT);
	if (bytecount <= 0) {
		return;
	}
	struct parsedgre greinfo;
	/* just to be safe sanitize greinfo */
	memset(&greinfo, 0, sizeof(greinfo));
	if (!unpack_packet(&greinfo, packet)) {
		/*
	         * failed to parse whatever came in.
	         * just drop it and keep going
	         */
		return;
	}
	fprintf(stderr, "\n\nlisten_server recv:-------------------------------\n");
	hexdump(packet, bytecount);

	send_to_tunnels(&greinfo, bytecount, pargs->devices, pargs->devcount);
}

/*
 * Requires you set event_init first
 * throws an error on failure
 * makes events for devices (event on read)
 * make events for server
 */
void
add_events(struct argstruct * parsedargs)
{
	struct event   *ev = calloc(1, sizeof(struct event));
	int 		fd = parsedargs->serverfd;
	event_set(ev, fd, EV_READ | EV_PERSIST, listen_server, parsedargs);
	event_add(ev, NULL);

	/* add each device in the same way */
	for (int i = 0; i < parsedargs->devcount; i++) {
		ev = calloc(1, sizeof(struct event));
		fd = parsedargs->devices[i]->fd;
		event_set(ev, fd, EV_READ | EV_PERSIST, listen_tunnel, parsedargs);
		event_add(ev, NULL);
	}
}

/*
 * parses arguments and then does stuff.
 */
int
main(int argc, char *argv[])
{
	if (argc < 2)
		usage();

	struct argstruct parsedargs;
	memset(&parsedargs, 0, sizeof(parsedargs));
	parsedargs.family = AF_UNSPEC;	/* AF_UNSPEC, AF_INET, AF_INET6 */
	parsedargs.daemonise = true;
	parsedargs.port = NULL;	/* if no args are present we set our own */

	/* safe upper bound for devices */
	int 		tunind = 0;
	/* is this okay in terms of style? */

	struct tunnelinfo *devices[argc];
	int 		ch;
	while ((ch = getopt(argc, argv, "46dl:p:e:i:")) != -1) {
		/* force defaults (isip default true) */
		switch (ch) {
		case '4':
			parsedargs.family = AF_INET;
			break;
		case '6':
			parsedargs.family = AF_INET6;
			break;
		case 'd':
			parsedargs.daemonise = false;
			break;
		case 'l':
			parsedargs.address = optarg;
			break;
		case 'p':
			parsedargs.port = optarg;
			break;
		case 'e':
			devices[tunind] = parse_device(ETHTYPE, optarg);
			tunind++;
			break;
		case 'i':
			/* need to figure out how to get IPv4 and IPv6 */
			devices[tunind] = parse_device(IPV4TYPE, optarg);
			tunind++;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;
	/* ensure we still have server [port] as arguments */
	/*
         * listen for server
         * The remaining args are server [port]
         * now we take the server and the port and create fd for them
         * (Also not argv is null terminated so we good)
         */
	if (argv[0] == NULL) {
		usage();
	}
	parsedargs.servername = argv[0];
	parsedargs.serverport = argv[1] ? argv[1] : "4754";
	parsedargs.port = parsedargs.port ? parsedargs.port : parsedargs.serverport;

	parse_server(&parsedargs);

	/* create tunnels_node */
	parsedargs.devcount = tunind;
	parsedargs.devices = devices;

	/* now we check for duplicates */
	error_tunnelinfo_duplicates(&parsedargs);

	/* check for daemonise */
	if (parsedargs.daemonise) {
		//logger_syslog(getprogname());
		if (daemon(1, 1) != 0) {
			err(1, "%s", "failed to daemonise");
		}
	}
	/*
         * now we create a bunch of events and add them to the queue
         * Strange why we have to do this AFTER declaring daemonise
         */
	event_init();
	add_events(&parsedargs);
	event_dispatch();

	return 0;
}
