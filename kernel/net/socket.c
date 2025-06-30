/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/socket.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#include <common.h>
#include <net/socket.h>
#include <lib/errno.h>
#include <mm/mm.h>
#include <klib.h>
#include <debug.h>
#include <syscall.h>
#include <io/net.h>

DECLARE_LIST_HEAD(sockops_list);

/**
 * Look up socket protocol operations
 * @param protocol: Protocol number (e.g., IPPROTO_UDP)
 * @return Pointer to sockops structure or NULL if not found
 */
static struct sockops *lookup_proto(int protocol)
{
	struct sockops *ops;
	list_for_each_entry(ops, &sockops_list, list)
	{
		if (ops->proto == protocol)
			return ops;
	}
	return NULL;
}

struct socket* socket_socket(int domain, int type, int protocol)
{
	struct socket *sock;
	struct sockops *ops;

	if (domain != AF_INET)
		return ERR_PTR(-EAFNOSUPPORT);

	if (type != SOCK_DGRAM)
		return ERR_PTR(-EPROTOTYPE);

	if (!protocol)
		protocol = IPPROTO_UDP;

	ops = lookup_proto(protocol);
	if (!ops)
		return ERR_PTR(-EPROTONOSUPPORT);

	sock = kalloc(sizeof(struct socket));
	memset(sock, 0, sizeof(struct socket));
	sock->ops = ops;
	INIT_LIST_HEAD(sock->recvq);

    sock->netdev = netdev_get_default_dev();
    assert(sock->netdev != NULL);

	return sock;
}

void socket_register_proto(struct sockops *ops)
{
	list_insert_end(&sockops_list, &ops->list);
}

void socket_destroy(struct socket *sock)
{
	struct packet *pkt;
	list_for_each_entry(pkt, &sock->recvq, list)
	{
		packet_free(pkt);
	}

    kfree(sock);
}

/**
 * Read data from a socket
 * @param file: File structure representing the socket
 * @param buffer: Buffer to store read data
 * @param size: Number of bytes to read
 * @param offset: Pointer to current offset (updated after reading)
 * @return Number of bytes read, or negative error code
 */
static ssize_t socket_read(struct file *file, char *buffer, size_t size, off_t *offset) {
    struct socket *sock = (struct socket *)file->f_private;
    int recvlen;

    assert(sock != NULL);

    if(!sock->flags.sk_open) {
        error("Socket not open");
        return -ENOTCONN;
    }

    recvlen = sock->ops->recv(sock, buffer, size, 0);
    if(recvlen < 0) {
        error("Socket recv failed: %d", recvlen);
        return recvlen;
    }

    *offset += recvlen;
    return recvlen;
}

/**
 * Write data to a socket
 * @param file: File structure representing the socket
 * @param buffer: Buffer containing data to write
 * @param size: Number of bytes to write
 * @param offset: Pointer to current offset (updated after writing)
 * @return Number of bytes written, or negative error code
 */
static ssize_t socket_write(struct file *file, const char *buffer, size_t size, off_t *offset) {
    struct socket *sock = (struct socket *)file->f_private;
    int sendlen;

    assert(sock != NULL);

    if(!sock->flags.sk_open) {
        error("Socket not open");
        return -ENOTCONN;
    }

    sendlen = sock->ops->send(sock, buffer, size, 0);
    if(sendlen < 0) {
        error("Socket send failed: %d", sendlen);
        return sendlen;
    }

    *offset += sendlen;
    return sendlen;
}

struct file_operations socket_fops = {
    .read = socket_read,
    .write = socket_write,
};

int socket_init(struct file* file) {
    struct socket *sock;

    if (!file)
        return -EINVAL;

    sock = socket_socket(AF_INET, SOCK_DGRAM, 0);
    if (!sock)
        return -ENOMEM;

    file_init(file, &socket_fops, NULL, 0, (void*)sock);

    return 0;
}

int socket_final(struct file* file) {
    struct socket *sock;

    if (!file || !file->f_private)
        return -EINVAL;

    sock = (struct socket *)file->f_private;
    if (!sock->flags.sk_open) {
        error("Socket is not open");
        return -ENOTCONN;
    }

    sock->ops->close(sock);
    socket_destroy(sock);

    file->f_private = NULL;

    return 0;
}

/************************ Syscalls for sockets *************************/

SYSCALL_DEFINE3(socket, int, int, domain, int, type, int, protocol)
{
    struct file *file;
    int ret;

    file = kcalloc(sizeof(struct file), 1);
    if (!file) {
        error("Failed to allocate file structure");
        return -ENOMEM;
    }

    ret = socket_init(file);
    if (ret < 0) {
        kfree(file);
        return ret;
    }

    return fd_alloc(myproc()->fdt, file);
}

SYSCALL_DEFINE3(connect, int, int, fd, const struct sockaddr *, address, socklen_t, address_len)
{
    struct file *file;
    struct socket *sock;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -EBADF;

    file = fd_get(myproc()->fdt, fd);
    if (!file || !file->f_private)
        return -EBADF;

    sock = (struct socket *)file->f_private;
    if (!sock->ops->connect)
        return -ENOSYS;

    ret = call_interface(sock->ops, connect, int, sock, address, address_len);
    if (ret < 0)
        return ret;

    sock->flags.sk_open = 1;
    return 0;
}

SYSCALL_DEFINE3(bind, int, int, fd, const struct sockaddr *, address, socklen_t, address_len)
{
    struct file *file;
    struct socket *sock;
    int ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -EBADF;

    file = fd_get(myproc()->fdt, fd);
    if (!file || !file->f_private)
        return -EBADF;

    sock = (struct socket *)file->f_private;
    if (!sock->ops->bind)
        return -ENOSYS;

    ret = call_interface(sock->ops, bind, int, sock, address, address_len);
    if (ret < 0)
        return ret;

    sock->flags.sk_bound = 1;
    return 0;
}

SYSCALL_DEFINE4(send, ssize_t, int, fd, const void *, buffer, size_t, length, int, flags)
{
    struct file *file;
    struct socket *sock;
    ssize_t ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -EBADF;

    file = fd_get(myproc()->fdt, fd);
    if (!file || !file->f_private)
        return -EBADF;

    sock = (struct socket *)file->f_private;
    if (!sock->ops->send)
        return -ENOSYS;

    ret = call_interface(sock->ops, send, ssize_t, sock, buffer, length, flags);
    if (ret < 0)
        return ret;

    return ret;
}

SYSCALL_DEFINE4(recv, ssize_t, int, fd, void *, buffer, size_t, length, int, flags)
{
    struct file *file;
    struct socket *sock;
    ssize_t ret;

    if (fd < 0 || fd >= NR_OPEN)
        return -EBADF;

    file = fd_get(myproc()->fdt, fd);
    if (!file || !file->f_private)
        return -EBADF;

    sock = (struct socket *)file->f_private;
    if (!sock->ops->recv)
        return -ENOSYS;

    ret = call_interface(sock->ops, recv, ssize_t, sock, buffer, length, flags);
    if (ret < 0)
        return ret;

    return ret;
}