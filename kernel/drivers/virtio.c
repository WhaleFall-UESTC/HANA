/**
 * This code is partly copied from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/virtio.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#include <common.h>
#include <debug.h>
#include <mm/mm.h>
#include <drivers/virtio.h>
#include <platform.h>
#include <klib.h>

struct virtqueue *virtq_create()
{
	int i;
	struct virtqueue *virtq;

	// log("virtq size: %u\n", virtq_size(VIRTIO_DEFAULT_QUEUE_SIZE));
	// log("struct virtqueue layout:");
	// log("  desc offset: %lu, size: %lu", offsetof(struct virtqueue, desc), sizeof(((struct virtqueue *)0)->desc));
	// log("  avail offset: %lu, size: %lu", offsetof(struct virtqueue, avail), sizeof(((struct virtqueue *)0)->avail));
	// log("  used offset: %lu, size: %lu", offsetof(struct virtqueue, used), sizeof(((struct virtqueue *)0)->used));

	virtq = (struct virtqueue *)kalloc(sizeof(struct virtqueue));
	assert(virtq != NULL);
	memset(virtq, 0, sizeof(struct virtqueue));

	debug("virtq_create: virtq=0x%lx", (uint64)virtq);

	virtq->avail.idx = 0;
	virtq->used.idx = 0;
	virtq->used.flags = 0;

	for (i = 0; i < VIRTIO_DEFAULT_QUEUE_SIZE; i++)
	{
		virtq->desc[i].next = i + 1;
	}

	return virtq;
}

uint32 virtq_alloc_desc(struct virtq_info *virtq_info, void *addr)
{
	uint32 desc = virtq_info->free_desc;
	uint32 next = virtq_info->virtq->desc[desc].next;
	if (desc == VIRTIO_DEFAULT_QUEUE_SIZE)
		error("ran out of virtqueue descriptors");
	virtq_info->free_desc = next;

	virtq_info->virtq->desc[desc].addr = virt_to_phys((uint64)addr);
	virtq_info->desc_virt[desc] = addr;

	// log("virtq_alloc_desc: %u, addr= 0x%p 0x%lx", desc, addr, virtq_info->virtq->desc[desc].addr);

	return desc;
}

void virtq_free_desc(struct virtq_info *virtq_info, uint32 desc)
{
	if(virtq_info->desc_virt[desc] == NULL)
		error("Trying to free a free desc");

	virtq_info->virtq->desc[desc].next = virtq_info->free_desc;
	virtq_info->free_desc = desc;
	virtq_info->desc_virt[desc] = NULL;
}

void virtq_show(struct virtq_info *virtq_info)
{
	int count = 0;
	uint32 i = virtq_info->free_desc;
	log("Current free_desc: %u, len=%u", virtq_info->free_desc, VIRTIO_DEFAULT_QUEUE_SIZE);
	while (i != VIRTIO_DEFAULT_QUEUE_SIZE && count++ <= VIRTIO_DEFAULT_QUEUE_SIZE)
	{
		log("  next: %u -> %u", i, virtq_info->virtq->desc[i].next);
		i = virtq_info->virtq->desc[i].next;
	}
	if (count > VIRTIO_DEFAULT_QUEUE_SIZE)
	{
		log("Overflowed descriptors?");
	}
}

