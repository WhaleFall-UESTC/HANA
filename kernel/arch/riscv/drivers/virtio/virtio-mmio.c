/**
 * This code is partly copied from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/virtio.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 * 
 * Implements virtio device drivers, particularly mmio ones.
 *
 * Reference:
 *
 * http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html
 */

#include <common.h>
#include <debug.h>
#include <mm/mm.h>
#include <drivers/virtio-mmio.h>
#include <drivers/virtio.h>
#include <platform.h>
#include <klib.h>

struct virtq_info* virtq_add_to_device(volatile virtio_regs *regs, uint32 queue_sel, uint32 queue_size)
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

	if(queue_size > max_queue_size) {
		panic("Default queue size too big, must be lower than %d", max_queue_size);
	}

	virtq_info->free_desc = virtq_info->seen_used = 0;
	virtq_info->queue_num = queue_sel;
	virtq_info->queue_size = queue_size;
	virtq_create(virtq_info);
	virtq_info->pfn = phys_page_number(virt_to_phys((uint64)virtq_info->virtq.base));

	// Step 5: Notify the device about the queue size
	WRITE32(regs->QueueNum, queue_size);

	// Step 6: Notify the device about the used alignment
	WRITE32(regs->QueueAlign, VIRTIO_DEFAULT_ALIGN);
	mb();
	
	// Step 7: Write the physical number of the first page of the queue
	WRITE32(regs->QueuePFN, virtq_info->pfn);

	return virtq_info;
}

void virtio_check_capabilities(virtio_regs *regs, struct virtio_cap *caps, uint32 n)
{
	uint32 i;
	uint32 sel = 0;
	uint32 driver = 0;
	uint32 device;

	WRITE32(regs->HostFeaturesSel, sel);
	mb();
	device = READ32(regs->HostFeatures);

	for (i = 0; i < n; i++)
	{
		if (caps[i].bit / 32 != sel)
		{
			/* Time to write our selected bits for this sel */
			WRITE32(regs->GuestFeaturesSel, sel);
			mb();
			WRITE32(regs->GuestFeatures, driver);
			if (device)
			{
				/*log("%s: device supports unknown bits"
					   " 0x%x in sel %u\n", whom, device,
				   sel);*/
			}
			/* Now we set these variables for next time. */
			sel = caps[i].bit / 32;
			WRITE32(regs->HostFeaturesSel, sel);
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
					   "(%s)\n",
					   caps[i].name, caps[i].help);*/
			}
			/* clear this from device now */
			device &= ~(1 << caps[i].bit);
		}
	}
	/* Time to write our selected bits for this sel */
	WRITE32(regs->GuestFeaturesSel, sel);
	mb();
	WRITE32(regs->GuestFeatures, driver);
	if (device)
	{
		/*log("%s: device supports unknown bits"
			   " 0x%x in sel %u\n", whom, device, sel);*/
	}
	mb();
}

static int virtio_dev_init(uint64 virt, uint32 intid)
{
	volatile virtio_regs *regs = (virtio_regs *)virt;
	int device_id;

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
	if ((device_id = (regs->DeviceID)) == 0)
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

    WRITE32(regs->GuestPageSize, PGSIZE);
    mb();

	switch (device_id)
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

void virtio_device_init(void)
{
	for (int i = 0; i < 1; i++)
		virtio_dev_init(VIRT_VIRTIO + VIRT_VIRTIO_SIZE * i, i + 1);
}
