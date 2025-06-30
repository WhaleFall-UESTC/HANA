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

/**
 * Create a new socket
 * @param domain: Address family (only AF_INET supported)
 * @param type: Socket type (only SOCK_DGRAM supported)
 * @param protocol: Protocol (0 for default, or specific protocol)
 * @return Pointer to new socket structure, error pointer on failure
 */
struct socket* socket_socket(int domain, int type, int protocol);

/**
 * Register socket protocol operations
 * @param ops: Pointer to sockops structure to register
 */
void socket_register_proto(struct sockops *ops);

/**
 * Destroy a socket and free resources
 * @param sock: Socket to destroy
 */
void socket_destroy(struct socket *sock);

extern struct file_operations socket_fops;

/**
 * Initialize a socket file
 * @param file: File structure to initialize
 * @return 0 on success, negative error code on failure
 */
int socket_init(struct file* file);

/**
 * Finalize and close a socket file
 * @param file: File structure to finalize
 * @return 0 on success, negative error code on failure
 */
int socket_final(struct file* file);

#endif // __SOCKET_H__