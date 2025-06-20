/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/inet.h
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#ifndef __INET_H__
#define __INET_H__

#include <common.h>

uint32 ntohl(uint32 orig);
uint16 ntohs(uint16 orig);
uint32 htonl(uint32 orig);
uint32 htons(uint16 orig);
int    inet_aton(const char *cp, uint32 *addr);

#endif // __INET_H__