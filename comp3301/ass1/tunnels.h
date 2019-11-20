#ifndef TUNNELS_H
#define TUNNELS_H

#include <stdio.h>
#include <stdbool.h>

#include <event.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <net/if_tun.h>

#define IPV4TYPE 0x0800
#define IPV6TYPE 0X86DD
#define ETHTYPE 0x6558

/*
 * Tunnels is a tail queue implemented with the sys/queue.h headers.
 * They contain a (FILE *) and a int32_t
 * Don't forget to init and also free afterwards
 */
struct tunnelinfo {
	uint16_t 	isip;	/* is IP(tunnel) */
	int 		fd;
	bool 		haskey;
	uint32_t 	key;	/* key guaranteed 32 bits */
	char           *filepath;	/* not necessary just useful to have
					 * around */
};

/*
 * put whatever in the flag if you want if haskey if false also NULL for the
 * filepath because it's not necessary
 */
struct tunnelinfo *
create_tunnelinfo(uint16_t, const char *, bool,
		  const uint32_t);

void 		free_tunnelinfo(struct tunnelinfo *);

/*
 * A higher level version of create struct that takes a single string
 * as input. Also mallocs space for the filepath
 */
struct tunnelinfo *parse_device(int16_t, const char *);

/*
 * Likewise for this function here
 */
void 		free_device(struct tunnelinfo * item);

/*
 * assuming there's some kind of ordering to the tunnelinfos
 * then returns tunnelinfo_left - tunnelinfo_right
 */
int 		tunnelinfo_cmp(struct tunnelinfo *, struct tunnelinfo *);

/*
 * returns the index of the tunnel in the array
 * keep in mind all values in the struct excluding FILE * are compared
 * returns -1 if it isn't found
 */
int 		find_tunnel(struct tunnelinfo **, int, uint16_t, bool, uint32_t);

#endif
