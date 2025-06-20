/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/socket.h
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <common.h>
#include <tools/list.h>
#include <net/net.h>
#include <net/packets.h>
#include <io/net.h>
#include <proc/proc.h>
#include <net/socket_type.h>
#include <fs/file.h>

struct socket;

struct sockops {
	int (*connect)(struct socket *socket, const struct sockaddr *address,
	               socklen_t address_len);
	int (*bind)(struct socket *socket, const struct sockaddr *address,
	            socklen_t address_len);
	int (*send)(struct socket *socket, const void *buffer, size_t length,
	            int flags);
	int (*recv)(struct socket *socket, void *buffer, size_t length,
	            int flags);
	int (*close)(struct socket *socket);

	struct list_head list;
	uint32 proto;
};

struct socket {
    struct {
        int sk_bound : 1;
		int sk_connected : 1;
		int sk_open : 1;
	} flags;
	struct sockaddr_in src;
	struct sockaddr_in dst;

    struct sockops *ops;

	struct list_head recvq;
    struct netdev *netdev; // Network device associated with this socket
};

struct socket* socket_socket(int domain, int type, int protocol);
void socket_register_proto(struct sockops *ops);
void socket_destroy(struct socket *sock);

extern struct file_operations socket_fops;

int socket_init(struct file* file);
int socket_final(struct file* file);

#endif // __SOCKET_H__