#include "packet.h"

#include <string.h>
#include <ctype.h>
#include <sys/socket.h>

#define KMASK 32

/*
 * copies the from the second argument into the first and returns
 * the size in dest
 */
ssize_t
encapsulate_packet(uint8_t * dest, uint8_t * src, size_t msgsize,
		   struct tunnelinfo * tinfo)
{
	/*
         * add protocol type
         * This is done by reading the first 4 bytes of the buffer
         * This process could've been done much better but left for legacy reasons
         */
	if (tinfo->isip != ETHTYPE) {
		uint32_t 	ai_family = 0;
		ai_family = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
		switch (ai_family) {
		case AF_INET:
			tinfo->isip = IPV4TYPE;
			break;
		case AF_INET6:
			tinfo->isip = IPV6TYPE;
			break;
		default:
			return -1;
		}
		src = src + 4;
		msgsize = msgsize - 4;
	}
	ssize_t 	ret = msgsize + 4;
	uint8_t        *cursor = dest + 4;
	memset(cursor, 0, 64);

	dest[2] = (0xFF00 & tinfo->isip) >> 8;
	dest[3] = (0x00FF & tinfo->isip) >> 0;

	/* extract from tunnelinfo then OR them together */
	int 		haskey = tinfo->haskey;
	if (haskey) {
		dest[0] |= KMASK;
		dest[4] = (0xFF000000 & tinfo->key) >> 24;
		dest[5] = (0x00FF0000 & tinfo->key) >> 16;
		dest[6] = (0x0000FF00 & tinfo->key) >> 8;
		dest[7] = (0x000000FF & tinfo->key) >> 0;
		cursor = dest + 8;
		ret = msgsize + 8;
	}
	memcpy(cursor, src, msgsize);

	return ret;
}

struct parsedgre *
unpack_packet(struct parsedgre * ret, uint8_t * packet)
{
	ret->kflag = KMASK & packet[0];
	/* ensure no other flags are present */
	if ((packet[0] | KMASK) != KMASK)
		return NULL;

	/* ensure version is 0 */
	ret->ver = packet[1];
	if (ret->ver != 0)
		return NULL;

	ret->protocol = (packet[2] << 8) | packet[3];
	/* ensure the protocol is right */
	if (ret->protocol != IPV4TYPE && ret->protocol != IPV6TYPE
	    && ret->protocol != ETHTYPE)
		return NULL;

	if (ret->kflag) {
		ret->key
			= (packet[4] << 24) | (packet[5] << 16) | (packet[6] << 8) | packet[7];
		ret->payload = packet + 8;
	} else {
		ret->key = 0;
		ret->payload = packet + 4;
	}
	/* add the tunnel header to the payload */
	/* hacky method would be to go back 4 bytes and override that */
	if (ret->protocol == IPV4TYPE) {
		ret->payload = ret->payload - 4;
		ret->payload[0] = (0xFF000000 & AF_INET) >> 24;
		ret->payload[1] = (0x00FF0000 & AF_INET) >> 16;
		ret->payload[2] = (0x0000FF00 & AF_INET) >> 8;
		ret->payload[3] = (0x000000FF & AF_INET) >> 0;
	}
	if (ret->protocol == IPV6TYPE) {
		ret->payload = ret->payload - 4;
		ret->payload[0] = (0xFF000000 & AF_INET6) >> 24;
		ret->payload[1] = (0x00FF0000 & AF_INET6) >> 16;
		ret->payload[2] = (0x0000FF00 & AF_INET6) >> 8;
		ret->payload[3] = (0x000000FF & AF_INET6) >> 0;
	}
	return ret;
}

/*
 * possibly useful functions
 */
void
hexdump(const void *d, size_t datalen)
{
	const uint8_t  *data = d;
	size_t 		i     , j = 0;

	for (i = 0; i < datalen; i += j) {
		printf("%4zu: ", i);
		for (j = 0; j < 16 && i + j < datalen; j++)
			printf("%02x ", data[i + j]);

		while (j++ < 16)
			printf("   ");

		printf("|");
		for (j = 0; j < 16 && i + j < datalen; j++)
			putchar(isprint(data[i + j]) ? data[i + j] : '.');

		printf("|\n");
	}
}
