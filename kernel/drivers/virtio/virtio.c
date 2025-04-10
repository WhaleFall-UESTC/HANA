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
#include <klib.h>

struct virtqueue *virtq_create()
{
	int i;
	struct virtqueue *virtq;

	assert(virtq_size(VIRTIO_DEFAULT_QUEUE_SIZE) == sizeof(struct virtqueue));

	virtq = (struct virtqueue *)kalloc(sizeof(struct virtqueue));
	assert(virtq != NULL);
	memset(virtq, 0, sizeof(struct virtqueue));

	virtq->avail.idx = 0;
	virtq->used.idx = 0;

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
	return desc;
}

void virtq_free_desc(struct virtq_info *virtq_info, uint32 desc)
{
	virtq_info->virtq->desc[desc].next = virtq_info->free_desc;
	virtq_info->free_desc = desc;
	virtq_info->desc_virt[desc] = NULL;
}

struct virtq_info* virtq_add_to_device(volatile virtio_regs *regs, uint32 queue_sel)
{
	uint32 max_queue_size;
	KALLOC(struct virtq_info, virtq_info);
	assert(virtq_info != NULL);

	// Step 1: Select the queue
	WRITE32(regs->QueueSel, queue_sel);
	mb();

	// Step 2: Check if the queue is not already in use
	if (READ32(regs->QueuePFN) != 0)
	{
		error("Queue %u is already in use", queue_sel);
		return NULL;
	}

	// Step 3: Read maximum queue size
	max_queue_size = READ32(regs->QueueNumMax);
	if (max_queue_size == 0)
	{
		error("Queue %u is not available", queue_sel);
		return NULL;
	}

	// Step 4: Allocate and zero the queue pages

	if(VIRTIO_DEFAULT_QUEUE_SIZE > max_queue_size) {
		panic("Default queue size too big, must be lower than %d", max_queue_size);
	}

	virtq_info->free_desc = virtq_info->seen_used = 0;
	virtq_info->virtq = virtq_create();
	virtq_info->pfn = phys_page_number(virt_to_phys((uint64)virtq_info->virtq));
	memset(virtq_info->desc_virt, 0, sizeof(virtq_info->desc_virt));

	// Step 5: Notify the device about the queue size
	WRITE32(regs->QueueNum, VIRTIO_DEFAULT_QUEUE_SIZE);

	// Step 6: Notify the device about the used alignment
	WRITE32(regs->QueueAlign, PGSIZE);
	mb();
	
	// Step 7: Write the physical number of the first page of the queue
	WRITE32(regs->QueuePFN, virtq_info->pfn);

	return virtq_info;
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

void virtio_check_capabilities(virtio_regs *regs, struct virtio_cap *caps,
							   uint32 n, char *whom)
{
	uint32 i;
	uint32 bank = 0;
	uint32 driver = 0;
	uint32 device;

	WRITE32(regs->HostFeaturesSel, bank);
	mb();
	device = READ32(regs->HostFeatures);

	for (i = 0; i < n; i++)
	{
		if (caps[i].bit / 32 != bank)
		{
			/* Time to write our selected bits for this bank */
			WRITE32(regs->GuestFeaturesSel, bank);
			mb();
			WRITE32(regs->GuestFeatures, driver);
			if (device)
			{
				/*log("%s: device supports unknown bits"
					   " 0x%x in bank %u", whom, device,
				   bank);*/
			}
			/* Now we set these variables for next time. */
			bank = caps[i].bit / 32;
			WRITE32(regs->HostFeaturesSel, bank);
			mb();
			device = READ32(regs->HostFeatures);
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
					   "(%s)",
					   caps[i].name, caps[i].help);*/
			}
			/* clear this from device now */
			device &= ~(1 << caps[i].bit);
		}
	}
	/* Time to write our selected bits for this bank */
	WRITE32(regs->GuestFeaturesSel, bank);
	mb();
	WRITE32(regs->GuestFeatures, driver);
	if (device)
	{
		/*log("%s: device supports unknown bits"
			   " 0x%x in bank %u", whom, device, bank);*/
	}
}

static int virtio_dev_init(uint64 virt, uint32 intid)
{
	volatile virtio_regs *regs = (virtio_regs *)virt;

	if (READ32(regs->MagicValue) != VIRTIO_MAGIC)
	{
		error("virtio at 0x%lx had wrong magic value 0x%x, "
			   "expected 0x%x",
			   virt, regs->MagicValue, VIRTIO_MAGIC);
		return -1;
	}
	if (READ32(regs->Version) != VIRTIO_VERSION)
	{
		error("virtio at 0x%lx had wrong version 0x%x, expected "
			   "0x%x",
			   virt, regs->Version, VIRTIO_VERSION);
		return -1;
	}
	if (READ32(regs->DeviceID) == 0)
	{
		/*On QEMU, this is pretty common, don't print a message */
		/*log("warn: virtio at 0x%x has DeviceID=0, skipping",
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

	log("virtio regs at 0x%lx", (uint64)regs);
	log("virtio regs phycal addr: 0x%lx",
		virt_to_phys((uint64)regs));
	log("Magic: 0x%x", READ32(regs->MagicValue));

	switch (READ32(regs->DeviceID))
	{
	case VIRTIO_DEV_BLK:
		return virtio_blk_init(regs, intid);
	// case VIRTIO_DEV_NET:
		// return virtio_net_init(regs, intid);
	default:
		error("unsupported virtio device ID 0x%x",
			   READ32(regs->DeviceID));
	}
	
	return 0;
}

void virtio_init(void)
{
	for (int i = 0; i < 1; i++)
		virtio_dev_init(VIRT_VIRTIO + VIRT_VIRTIO_SIZE * i, i + 1);
}
