#include "tunnels.h"
#include "log.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

/*
 * don't forget to test multiple devices
 */
/*
 * the -4 and -6 are based on each device not for the whole greu
 */
/*
 * don't strip the tunnel header you need that for writing later
 */
struct tunnelinfo *
create_tunnelinfo(uint16_t isip, const char *filepath,
		  bool haskey, const uint32_t key)
{
	struct tunnelinfo *ret = malloc(sizeof(struct tunnelinfo));
	ret->isip = isip;

	int 		fd = open(filepath, O_RDWR | O_NONBLOCK);
	if (fd == -1) {
		/* error has occured */
		err(1, "%s failed to open", filepath);
	}
	ret->fd = fd;
	ret->haskey = haskey;
	ret->key = key;
	return ret;
}

void
free_tunnelinfo(struct tunnelinfo * item)
{
	close(item->fd);
	free(item);
}

/* break up the argument and make a tunnelinfo */
struct tunnelinfo *
parse_device(int16_t isip, const char *argument)
{
	bool 		haskey = false;
	uint32_t 	key = 0;

	const char     *res = strchr(argument, '@');
	if (res) {
		haskey = true;
		key = atoi(res);
	} else {
		/* set res to the null terminating character */
		res = argument;
		while (*(res++) != '\0');
	}
	/* this is important for off-by-one errors */
	res++;

	int 		length = res - argument;
	char           *filepath = calloc(length, sizeof(char));
	memcpy(filepath, argument, length - 1);

	return create_tunnelinfo(isip, filepath, haskey, key);
}

void
free_device(struct tunnelinfo * item)
{
	free(item->filepath);
	free_tunnelinfo(item);
}

/*
 * Could be expanded to do some kind of ordering.
 * For now either returns 0 for equal and 1 for not equal
 */
int
tunnelinfo_cmp(struct tunnelinfo * left, struct tunnelinfo * right)
{
	bool 		equal = true;
	equal &= left->isip == right->isip;
	equal &= left->haskey == right->haskey;
	if (equal && left->haskey) {
		/* check that the keys match */
		equal &= left->key == right->key;
	}
	return equal ? 0 : 1;
}

/*
 * returns the first occurrence of item in the struct that matches
 * the requirements
 * implemented is kinda hacky by creating a new tunnelinfo struct
 * and using the tunnelinfo_cmp
 */
int
find_tunnel(struct tunnelinfo ** arr, int count, uint16_t isip,
	    bool haskey, uint32_t key)
{
	struct tunnelinfo compare;
	compare.isip = isip;
	compare.fd = -1;
	compare.haskey = haskey;
	compare.key = key;
	compare.filepath = NULL;

	for (int i = 0; i < count; i++) {
		if (tunnelinfo_cmp(arr[i], &compare) == 0) {
			return i;
		}
	}

	return -1;
}
