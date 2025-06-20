/*
 * SPDX-License-Identifier: GPL-2.0
 * 
 * This file contains code ported from the Linux kernel.
 * Original source: https://elixir.bootlin.com/linux/v2.6.31.7/source/include/linux/kfifo.h
 * 
 * Original copyright and license:
 *   A simple kernel FIFO implementation.
 *
 *   Copyright (C) 2004 Stelian Pop <stelian@popies.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <tools/kfifo.h>
#include <debug.h>
#include <mm/mm.h>
#include <klib.h>
#include <arch.h>

#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define min(a, b) (((a) < (b)) ? (a) : (b))

static unsigned int roundup_pow_of_two(unsigned int n)
{
	if (n == 0)
		return 1;

	if (n == (n & (-n)))
		return n;

	unsigned int bit_length = 0;
	unsigned int temp = n;
	while (temp > 0)
	{
		bit_length++;
		temp >>= 1;
	}
	return 1U << bit_length;
}

/**
 * kfifo_init - allocates a new FIFO using a preallocated buffer
 * @buffer: the preallocated buffer to be used.
 * @size: the size of the internal buffer, this have to be a power of 2.
 * @lock: the lock to be used to protect the fifo buffer
 *
 * Do NOT pass the kfifo to kfifo_free() after use! Simply free the
 * &struct kfifo with kfree().
 */
struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size, spinlock_t *lock)
{
	struct kfifo *fifo;
	/* size must be a power of 2 */
	warn_on(!is_power_of_2(size), "size not power of 2");
	fifo = kalloc(sizeof(struct kfifo));
	if (!fifo)
		return NULL;
	fifo->buffer = buffer;
	fifo->size = size;
	fifo->in = fifo->out = 0;
	fifo->lock = lock;

	return fifo;
}

/**
 * kfifo_alloc - allocates a new FIFO and its internal buffer
 * @size: the size of the internal buffer to be allocated.
 * @lock: the lock to be used to protect the fifo buffer
 *
 * The size will be rounded-up to a power of 2.
 */
struct kfifo *kfifo_alloc(unsigned int size, spinlock_t *lock)
{
	unsigned char *buffer;
	struct kfifo *ret;
	if (!is_power_of_2(size))
	{
		warn_on(size > 0x80000000, "size too large");
		size = roundup_pow_of_two(size);
	}
	buffer = kalloc(size);
	if (!buffer)
		return NULL;
	ret = kfifo_init(buffer, size, lock);

	if (ret == NULL)
		kfree(buffer);

	return ret;
}

/**
 * kfifo_free - frees the FIFO
 * @fifo: the fifo to be freed.
 */
void kfifo_free(struct kfifo *fifo)
{
	if (fifo)
	{
		spinlock_t *lock = fifo->lock;

		spinlock_acquire(lock);
		if (fifo->buffer)
		{
			kfree(fifo->buffer);
			fifo->buffer = NULL;
		}
		kfree(fifo);
		spinlock_release(lock);
	}
}
/**
 * __kfifo_put - puts some data into the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int __kfifo_put(struct kfifo *fifo,
						 unsigned char *buffer, unsigned int len)
{
	unsigned int l;

	len = min(len, fifo->size - fifo->in + fifo->out);

	/*
	 * Ensure that we sample the fifo->out index -before- we
	 * start putting bytes into the kfifo.
	 */

	smp_mb();

	/* first put the data starting from fifo->in to buffer end */
	l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));
	memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);

	/* then put the rest (if any) at the beginning of the buffer */
	memcpy(fifo->buffer, buffer + l, len - l);

	/*
	 * Ensure that we add the bytes to the kfifo -before-
	 * we update the fifo->in index.
	 */

	smp_wmb();

	fifo->in += len;

	return len;
}

/**
 * __kfifo_get - gets some data from the FIFO, no locking version
 * @fifo: the fifo to be used.
 * @buffer: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @buffer and returns the number of bytes.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int __kfifo_get(struct kfifo *fifo,
						 unsigned char *buffer, unsigned int len)
{
	unsigned int l;

	len = min(len, fifo->in - fifo->out);

	/*
	 * Ensure that we sample the fifo->in index -before- we
	 * start removing bytes from the kfifo.
	 */

	smp_rmb();

	/* first get the data from fifo->out until the end of the buffer */
	l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
	memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);

	/* then get the rest (if any) from the beginning of the buffer */
	memcpy(buffer + l, fifo->buffer, len - l);

	/*
	 * Ensure that we remove the bytes from the kfifo -before-
	 * we update the fifo->out index.
	 */

	smp_mb();

	fifo->out += len;

	return len;
}