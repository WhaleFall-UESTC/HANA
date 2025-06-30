/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/inet.h
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#ifndef __INET_H__
#define __INET_H__

#include <common.h>


/**
 * Convert 32-bit value from network byte order to host byte order
 * @param orig: Value in network byte order
 * @return Value in host byte order
 */
uint32 ntohl(uint32 orig);

/**
 * Convert 16-bit value from network byte order to host byte order
 * @param orig: Value in network byte order
 * @return Value in host byte order
 */
uint16 ntohs(uint16 orig);

/**
 * Convert 32-bit value from host byte order to network byte order
 * @param orig: Value in host byte order
 * @return Value in network byte order
 */
uint32 htonl(uint32 orig);

/**
 * Convert 16-bit value from host byte order to network byte order
 * @param orig: Value in host byte order
 * @return Value in network byte order
 */
uint32 htons(uint16 orig);

/**
 * Convert IPv4 address string to binary form
 * @param cp: Null-terminated string containing IPv4 address
 * @param addr: Pointer to store the resulting 32-bit IP address
 * @return 1 on success, 0 on failure
 */
int    inet_aton(const char *cp, uint32 *addr);

#endif // __INET_H__