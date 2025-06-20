/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/inet.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 * 
 * inet.c: IP related utilities to share with userspace
 */

#include <common.h>
#include <net/inet.h>

uint32 ntohl(uint32 orig)
{
	return ((orig & 0xFF) << 24) | ((orig & 0xFF00) << 8) |
	       ((orig & 0xFF0000) >> 8) | ((orig & 0xFF000000) >> 24);
}

uint16 ntohs(uint16 orig)
{
	return ((orig & 0xFF) << 8 | (orig & 0xFF00) >> 8);
}

uint32 htonl(uint32 orig)
{
	return ntohl(orig);
}

uint32 htons(uint16 orig)
{
	return ntohs(orig);
}

/*
 * Unlike traditional inet_aton, we only support 4-component addresses.
 */
int inet_aton(const char *cp, uint32 *addr)
{
	uint32 myaddr = 0;
	uint32 component = 0;
	int group = 0;
	bool seen_in_group = false;
	int i = 0;

	for (i = 0; cp[i]; i++) {
		if (cp[i] >= '0' && cp[i] <= '9') {
			component = component * 10 + (cp[i] - '0');
			seen_in_group = true;
		} else if (cp[i] == '.') {
			if (component > 255)
				return 0;
			if (group >= 3)
				return 0;
			if (!seen_in_group)
				return 0;
			myaddr |= component << (8 * group);
			group++;
			component = 0;
			seen_in_group = false;
		} else {
			return 0;
		}
	}
	if (component > 255)
		return 0;
	if (group != 3)
		return 0;
	if (!seen_in_group)
		return 0;
	myaddr |= component << (8 * group);
	*addr = myaddr;
	return 1;
}
