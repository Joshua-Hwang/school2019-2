#include "server.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXSOCK 256

int
create_server_socket(int family, const char *bindaddress, const char *bindport,
		     const char *connaddress, const char *connport)
{
	struct addrinfo hints, *bindres, *bindres0, *connres, *connres0;
	int 		error;
	int 		save_errno;
	int 		s;
	const char     *cause = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	error = getaddrinfo(bindaddress, bindport, &hints, &bindres0);
	if (error) {
		err(1, "%s", gai_strerror(error));
	}
	error = getaddrinfo(connaddress, connport, &hints, &connres0);
	if (error) {
		err(1, "%s", gai_strerror(error));
	}
	bool 		stop = false;
	for (connres = connres0; connres; connres = connres->ai_next) {
		for (bindres = bindres0; bindres; bindres = bindres->ai_next) {
			s = socket(connres->ai_family, connres->ai_socktype,
				   connres->ai_protocol);
			if (s == -1) {
				cause = "socket";
				break;
			}
			/*
	                s = socket(bindres->ai_family, bindres->ai_socktype,
	                    bindres->ai_protocol);
	                if (s == -1) {
	                    cause = "socket";
	                    continue;
	                }
	                */

			if (bind(s, bindres->ai_addr, bindres->ai_addrlen) == -1) {
				cause = "bind";
				save_errno = errno;
				close(s);
				s = -1;
				errno = save_errno;
				fprintf(stderr, "bind failed\n");
				continue;
			}
			fprintf(stderr, "bind succeeded\n");

			if (connect(s, connres->ai_addr, connres->ai_addrlen) == -1) {
				cause = "connect";
				save_errno = errno;
				close(s);
				errno = save_errno;
				s = -1;
				fprintf(stderr, "connect failed\n");
				break;
			}
			fprintf(stderr, "connect succeeded\n");

			if (s != -1) {
				/* we got a valid one */
				stop = true;
				break;
			}
		}
		if (stop)
			break;
	}
	if (s == -1)
		err(1, "%s", cause);

	freeaddrinfo(bindres0);
	freeaddrinfo(connres0);

	return s;
}
