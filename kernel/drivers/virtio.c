/**
 * This code is partly from Stephen's OS (MIT License)
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

void virtq_create(struct virtq_info *virtq_info)
{
	int i;
	uint32 queue_size;
	uint64 queue_mem_size;
	struct virtqueue* virtq = &virtq_info->virtq;

	// log("virtq size: %u\n", virtq_size(VIRTIO_DEFAULT_QUEUE_SIZE));
	// log("struct virtqueue layout:");
	// log("  desc offset: %lu, size: %lu", offsetof(struct virtqueue, desc), sizeof(((struct virtqueue *)0)->desc));
	// log("  avail offset: %lu, size: %lu", offsetof(struct virtqueue, avail), sizeof(((struct virtqueue *)0)->avail));
	// log("  used offset: %lu, size: %lu", offsetof(struct virtqueue, used), sizeof(((struct virtqueue *)0)->used));

	assert(virtq_info->queue_size != 0);
	queue_size = virtq_info->queue_size;
	queue_mem_size = virtq_size(queue_size);
	
	void* virtq_base = kalloc(queue_mem_size);
	Assert(((uint64)virtq_base & (VIRTIO_DEFAULT_ALIGN - 1)) == 0, "mem alloced not aligned");
	assert(virtq != NULL);
	assert(virtq_base != NULL);
	memset(virtq_base, 0, queue_mem_size);

	debug("virtq_create: virtq=0x%lx", (uint64)virtq_base);

	virtq->desc = virtq_base;
	virtq->avail = (volatile struct virtqueue_avail *)((uint64)virtq_base + virtq_desc_size(queue_size));
	virtq->used = (volatile struct virtqueue_used *)ALIGN((uint64)virtq->avail + virtq_avail_size(queue_size), VIRTIO_DEFAULT_ALIGN);

	Assert((uint64)virtq->avail + virtq_avail_size(queue_size) + virtq_pad(queue_size) == (uint64)virtq->used, "virtq pad err");

	virtq->avail->idx = 0;
	virtq->used->idx = 0;
	virtq->used->flags = 0;

	for (i = 0; i < queue_size; i++)
	{
		virtq->desc[i].next = i + 1;
	}

	virtq_info->desc_virt = kcalloc(queue_size, sizeof(void*));
}

uint32 virtq_alloc_desc(struct virtq_info *virtq_info, void *addr)
{
	uint32 desc = virtq_info->free_desc;
	uint32 next = virtq_info->virtq.desc[desc].next;
	if (desc == virtq_info->queue_size)
		error("ran out of virtqueue descriptors");
	virtq_info->free_desc = next;

	virtq_info->virtq.desc[desc].addr = virt_to_phys((uint64)addr);
	virtq_info->desc_virt[desc] = addr;

	// log("virtq_alloc_desc: %u, addr= 0x%p 0x%lx", desc, addr, virtq_info->virtq.desc[desc].addr);

	return desc;
}

void virtq_free_desc(struct virtq_info *virtq_info, uint32 desc)
{
	if(virtq_info->desc_virt[desc] == NULL)
		error("Trying to free a free desc");

	virtq_info->virtq.desc[desc].next = virtq_info->free_desc;
	virtq_info->free_desc = desc;
	virtq_info->desc_virt[desc] = NULL;
}

void virtq_show(struct virtq_info *virtq_info)
{
	int count = 0;
	uint32 i = virtq_info->free_desc;
	log("Current free_desc: %u, len=%u", virtq_info->free_desc, virtq_info->queue_size);
	while (i != virtq_info->queue_size && count++ <= virtq_info->queue_size)
	{
		log("  next: %u -> %u", i, virtq_info->virtq.desc[i].next);
		i = virtq_info->virtq.desc[i].next;
	}
	if (count > virtq_info->queue_size)
	{
		log("Overflowed descriptors?");
	}
}