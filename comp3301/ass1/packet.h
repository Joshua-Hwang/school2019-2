#ifndef PACKET_H
#define PACKET_H

#include "tunnels.h"

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#define MAXPACKET 2000 // since max mtu is 1500 and just to be safe

/*
 *  The GRE packet header has the form:
 *
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |C| |K|S| Reserved0       | Ver |         Protocol Type         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Key (optional)                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

/*
 * Each are read big endian
 */
struct parsedgre {
	bool 		kflag;
	uint8_t 	ver;
	uint16_t 	protocol;
	uint32_t 	key;
	uint8_t        *payload;
};

/*
 * copies the from the second argument into the first and returns
 * the size
 */
ssize_t 	encapsulate_packet(uint8_t *, uint8_t *, size_t, struct tunnelinfo *);

/*
 * puts all information in the parsedgre struct
 * uint8_t is the packet we'll unpack
 * size_t size of packet
 * (the payload is just a pointer to where the header ends)
 *
 * return NULL on failure otherwise returns parsedgre
 */
struct parsedgre *unpack_packet(struct parsedgre *, uint8_t *);

void 		hexdump  (const void *, size_t);


#endif
