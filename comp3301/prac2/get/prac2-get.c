/*
** CSSE2310/7231 - sample client - code to be commented in class
** Send a request for the top level web page (/) on some webserver and
** print out the response - including HTTP headers.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

__dead void
usage(void)
{
	fprintf(stderr, "usage: %s [-46] [-p port] hostname\n", getprogname());
	exit(1);
}

struct parsedargs {
	char *hostname;
	char *service;
	int ver; /* AF_UNSPEC AF_INET AF_INET6 */
};

/*
 * This function doesn't set defaults. This is a requirement of main
 */
int
parse_args(int argc, char *argv[], struct parsedargs *ret) {
	if (argc < 2) {
		return -1;
	}

	int ch;
	while ((ch = getopt(argc, argv, "46p:")) != -1) {
		switch (ch) {
		case '4':
			ret->ver = AF_INET;
			break;
		case '6':
			ret->ver = AF_INET6;
			break;
		case 'p':
			ret->service = optarg;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

int
connect_to(char *hostname, char *service, int ver)
{
	struct addrinfo hints, *res, *res0;
	int		error;
	int		save_errno;
	int		s;
	const char     *cause = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = ver;
	hints.ai_socktype = SOCK_STREAM;
	/* args are hostname, service or port, optional hints, results */
	error = getaddrinfo(hostname, "80", &hints, &res0);
	if (error) {
		errx(1, "%s", gai_strerror(error));
	}
	s = -1;
	for (res = res0; res; res = res->ai_next) {
		s = socket(res->ai_family, res->ai_socktype,
			   res->ai_protocol);
		if (s == -1) {
			cause = "socket";
			continue;
		}
		if (connect(s, res->ai_addr, res->ai_addrlen) == -1) {
			cause = "connect";
			save_errno = errno;
			close(s);
			errno = save_errno;
			s = -1;
			continue;
		}
		break;		/* Ladies and gentlemen... We got 'em */
	}
	if (s == -1) {
		err(1, "%s", cause);
	}
	freeaddrinfo(res0);
	return s;
}

void
send_HTTP_request(int fd, char *file, char *host)
{
	char           *requestString;

	/*
	 * Construct HTTP request: GET / HTTP/1.0 Host: hostname <blank line>
	 */
	int error = asprintf(&requestString, "GET %s HTTP/1.0\r\n"
			     "Host: %s\n\n", file, host);
	if (error == -1) {
		err(1, "memory");
	}

	/* Send our request to server */
	if (write(fd, requestString, strlen(requestString)) < 1) {
		err(1, "write");
	}

	free(requestString);
}

void
get_and_output_HTTP_response(int fd)
{
	char		buffer   [1024];
	int		numBytesRead;
	int		eof = 0;

	//Repeatedly read from network fd until nothing left - write
		// everything out to stdout
		while (!eof) {
		numBytesRead = read(fd, buffer, 1024);
		if (numBytesRead < 0) {
			err(1, "read");
		} else if (numBytesRead == 0) {
			eof = 1;
		} else {
			fwrite(buffer, sizeof(char), numBytesRead, stdout);
		}
	}
}

int
main(int argc, char *argv[])
{
	struct parsedargs args = { .service = "80", .ver = AF_UNSPEC };
	int		fd;

	int error = parse_args(argc, argv, &args);
	if (error != 0 || optind >= argc) {
		usage();
	}

	args.hostname = argv[optind];

	/* Connect to port 80 on that address */
	fd = connect_to(args.hostname, args.service, args.ver);
	send_HTTP_request(fd, "/", args.hostname);
	get_and_output_HTTP_response(fd);
	close(fd);
	return 0;
}
