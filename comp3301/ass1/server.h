#ifndef SERVER_H
#define SERVER_H

/*
 * return -1 on failure
 * otherwise returns the binded and connected socket
 * parameters are
 * family,
 * local address and port
 * server address and port
 */
int 
create_server_socket(int,
		     const char *, const char *,
		     const char *, const char *);

#endif
