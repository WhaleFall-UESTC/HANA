/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/include/sys/socket.h
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#ifndef __SOCKET_TYPE_H__
#define __SOCKET_TYPE_H__

#include <common.h>

typedef uint32 sa_family_t;
typedef uint32 socklen_t;
typedef uint16 in_port_t;

enum {
	/* noformat */
	AF_INET = 1,
};

enum {
	/* noformat */
	SOCK_STREAM = 1,
	SOCK_DGRAM,
};

#define IPPROTO_UDP 17

struct in_addr {
	uint32 s_addr;
};

struct sockaddr_in {
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
};

struct sockaddr {
	sa_family_t s_family;
};

#endif // __SOCKET_TYPE_H__