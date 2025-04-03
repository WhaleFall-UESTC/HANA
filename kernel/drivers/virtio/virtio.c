/**
 * Implements virtio device drivers, particularly mmio ones.
 *
 * Reference:
 *
 * http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html
 */

#include <common.h>
#include <debug.h>
#include <mm/mm.h>
#include <drivers/virtio.h>
#include <platform.h>

struct virtqueue *virtq_create(uint32 len)
{
	int i;
	uint64 page_virt;
	struct virtqueue *virtq;

	/* compute offsets, ensure virtq struct fit into single page */
	uint64 off_desc = ALIGN(sizeof(struct virtqueue), 16);
	uint64 off_avail =
		ALIGN(off_desc + len * sizeof(struct virtqueue_desc), 2);
	uint64 off_used_event = (off_avail + sizeof(struct virtqueue_avail) +
							   len * sizeof(uint16));
	uint64 off_used = ALIGN(off_used_event + sizeof(uint16), 4);
	uint64 off_avail_event = (off_used + sizeof(struct virtqueue_used) +
								len * sizeof(struct virtqueue_used_elem));
	uint64 off_desc_virt =
		ALIGN(off_avail_event + sizeof(uint16), sizeof(void *));
	uint64 memsize = off_desc_virt + len * sizeof(void *);

	if (memsize > PGSIZE)
	{
		log("error: too big for a page\n");
		return NULL;
	}
	page_virt = kalloc(PGSIZE);

	virtq = (struct virtqueue *)page_virt;
	virtq->phys = virt_to_phys(page_virt);
	virtq->len = len;

	virtq->desc = (struct virtqueue_desc *)(page_virt + off_desc);
	virtq->avail = (struct virtqueue_avail *)(page_virt + off_avail);
	virtq->used_event = (uint16 *)(page_virt + off_used_event);
	virtq->used = (struct virtqueue_used *)(page_virt + off_used);
	virtq->avail_event = (uint16 *)(page_virt + off_avail_event);
	virtq->desc_virt = (void **)(page_virt + off_desc_virt);

	virtq->avail->idx = 0;
	virtq->used->idx = 0;
	virtq->seen_used = virtq->used->idx;
	virtq->free_desc = 0;

	for (i = 0; i < len; i++)
	{
		virtq->desc[i].next = i + 1;
	}

	return virtq;
}

uint32 virtq_alloc_desc(struct virtqueue *virtq, void *addr)
{
	uint32 desc = virtq->free_desc;
	uint32 next = virtq->desc[desc].next;
	if (desc == virtq->len)
		error("ran out of virtqueue descriptors\n");
	virtq->free_desc = next;

	virtq->desc[desc].addr = virt_to_phys(addr);
	virtq->desc_virt[desc] = addr;
	return desc;
}

void virtq_free_desc(struct virtqueue *virtq, uint32 desc)
{
	virtq->desc[desc].next = virtq->free_desc;
	virtq->free_desc = desc;
	virtq->desc_virt[desc] = NULL;
}

void virtq_add_to_device(volatile virtio_regs *regs, struct virtqueue *virtq,
						 uint32 queue_sel)
{
	WRITE32(regs->QueueSel, queue_sel);
	mb();
	WRITE32(regs->QueueNum, virtq->len);
	WRITE32(regs->QueueDescLow,
			virtq->phys + ((void *)virtq->desc - (void *)virtq));
	WRITE32(regs->QueueDescHigh, 0);
	WRITE32(regs->QueueAvailLow,
			virtq->phys + ((void *)virtq->avail - (void *)virtq));
	WRITE32(regs->QueueAvailHigh, 0);
	WRITE32(regs->QueueUsedLow,
			virtq->phys + ((void *)virtq->used - (void *)virtq));
	WRITE32(regs->QueueUsedHigh, 0);
	mb();
	WRITE32(regs->QueueReady, 1);
}

void virtq_show(struct virtqueue *virtq)
{
	int count = 0;
	uint32 i = virtq->free_desc;
	log("Current free_desc: %u, len=%u\n", virtq->free_desc, virtq->len);
	while (i != virtq->len && count++ <= virtq->len)
	{
		log("  next: %u -> %u\n", i, virtq->desc[i].next);
		i = virtq->desc[i].next;
	}
	if (count > virtq->len)
	{
		log("Overflowed descriptors?\n");
	}
}

void virtio_check_capabilities(virtio_regs *regs, struct virtio_cap *caps,
							   uint32 n, char *whom)
{
	uint32 i;
	uint32 bank = 0;
	uint32 driver = 0;
	uint32 device;

	WRITE32(regs->DeviceFeaturesSel, bank);
	mb();
	device = READ32(regs->DeviceFeatures);

	for (i = 0; i < n; i++)
	{
		if (caps[i].bit / 32 != bank)
		{
			/* Time to write our selected bits for this bank */
			WRITE32(regs->DriverFeaturesSel, bank);
			mb();
			WRITE32(regs->DriverFeatures, driver);
			if (device)
			{
				/*log("%s: device supports unknown bits"
					   " 0x%x in bank %u\n", whom, device,
				   bank);*/
			}
			/* Now we set these variables for next time. */
			bank = caps[i].bit / 32;
			WRITE32(regs->DeviceFeaturesSel, bank);
			mb();
			device = READ32(regs->DeviceFeatures);
		}
		if (device & (1 << caps[i].bit))
		{
			if (caps[i].support)
			{
				driver |= (1 << caps[i].bit);
			}
			else
			{
				/*log("virtio supports unsupported option %s
				   "
					   "(%s)\n",
					   caps[i].name, caps[i].help);*/
			}
			/* clear this from device now */
			device &= ~(1 << caps[i].bit);
		}
	}
	/* Time to write our selected bits for this bank */
	WRITE32(regs->DriverFeaturesSel, bank);
	mb();
	WRITE32(regs->DriverFeatures, driver);
	if (device)
	{
		/*log("%s: device supports unknown bits"
			   " 0x%x in bank %u\n", whom, device, bank);*/
	}
}

static int virtio_dev_init(uint32 virt, uint32 intid)
{
	virtio_regs *regs = (virtio_regs *)virt;

	if (READ32(regs->MagicValue) != VIRTIO_MAGIC)
	{
		error("virtio at 0x%x had wrong magic value 0x%x, "
			   "expected 0x%x\n",
			   virt, regs->MagicValue, VIRTIO_MAGIC);
		return -1;
	}
	if (READ32(regs->Version) != VIRTIO_VERSION)
	{
		error("virtio at 0x%x had wrong version 0x%x, expected "
			   "0x%x\n",
			   virt, regs->Version, VIRTIO_VERSION);
		return -1;
	}
	if (READ32(regs->DeviceID) == 0)
	{
		/*On QEMU, this is pretty common, don't print a message */
		/*log("warn: virtio at 0x%x has DeviceID=0, skipping\n",
		 * virt);*/
		return -1;
	}

	/* First step of initialization: reset */
	WRITE32(regs->Status, 0);
	mb();
	/* Hello there, I see you */
	WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_ACKNOWLEDGE);
	mb();

	/* Hello, I am a driver for you */
	WRITE32(regs->Status, READ32(regs->Status) | VIRTIO_STATUS_DRIVER);
	mb();

	switch (READ32(regs->DeviceID))
	{
	case VIRTIO_DEV_BLK:
		return virtio_blk_init(regs, intid);
	case VIRTIO_DEV_NET:
		return virtio_net_init(regs, intid);
	default:
		error("unsupported virtio device ID 0x%x\n",
			   READ32(regs->DeviceID));
	}
	return 0;
}

void virtio_init(void)
{
	for (int i = 0; i < 32; i++)
		virtio_dev_init(VIRT_VIRTIO + VIRT_VIRTIO_SIZE * i, i);
}
