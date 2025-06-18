/**
 * This code is partly copied from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/virtio.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 * 
 * Implements virtio device drivers in pci.
 *
 * Reference:
 *
 * https://ozlabs.org/~rusty/virtio-spec/virtio-0.9.5.pdf
 */

#include <common.h>
#include <debug.h>
#include <mm/mm.h>
#include <drivers/virtio-pci.h>
#include <drivers/pci.h>
#include <platform.h>
#include <klib.h>
#include <mm/memlayout.h>

struct virtq_info* virtq_add_to_device(volatile virtio_pci_header *header, uint32 queue_sel)
{
	KALLOC(struct virtq_info, virtq_info);
	assert(virtq_info != NULL);

	// Step 1: Select the queue
	WRITE32(header->QueueSelect, queue_sel);
	mb();

	// Step 2: Check if the queue is already in use
	uint32 queue_size = READ32(header->QueueSize);
	if (queue_size == 0)
	{
		error("Queue %u is already in use", queue_sel);
		return NULL;
	}

	// Step 3: Allocate and zero the queue pages

	virtq_info->free_desc = virtq_info->seen_used = 0;
	virtq_info->queue_num = queue_sel;
	virtq_info->queue_size = queue_size;
	virtq_create(virtq_info);
	virtq_info->pfn = virt_to_phys((uint64)virtq_info->virtq.base) / 4096;

	// Step 4: Write the physical page number of the first page of the queue
	WRITE32(header->QueueAddress, virtq_info->pfn);

	return virtq_info;
}

void virtio_check_capabilities(virtio_pci_header *header, struct virtio_cap *caps, uint32 n)
{
	uint32 i;
	uint32 sel = 0;
	uint32 driver = 0;
	uint32 device;

	device = READ32(header->DeviceFeature);

	for (i = 0; i < n; i++)
	{
		if (caps[i].bit / 32 != sel)
		{
			/* Time to write our selected bits for this sel */
			WRITE32(header->GuestFeature, driver);
			if (device)
			{
				/*log("%s: device supports unknown bits"
					   " 0x%x in sel %u\n", whom, device,
				   sel);*/
			}
			/* Now we set these variables for next time. */
			sel = caps[i].bit / 32;
			device = READ32(header->DeviceFeature);
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
	WRITE32(header->GuestFeature, driver);
	if (device)
	{
		/*log("%s: device supports unknown bits"
			   " 0x%x in sel %u\n", whom, device, sel);*/
	}
	mb();
}

static int virtio_dev_init(pci_device_t* pci_dev)
{
	if (pci_dev->vendor_id != VIRTIO_PCI_VENDORID)
	{
		error("virtio for pci device bus %d dev %d had wrong vendor id 0x%x, "
			   "expected 0x%x",
			   pci_dev->bus, pci_dev->dev, pci_dev->vendor_id, VIRTIO_PCI_VENDORID);
		return -1;
	}
	if (pci_dev->device_id < VIRTIO_PCI_DEVICEID_BASE
        || pci_dev->device_id >= VIRTIO_PCI_DEVICEID_BASE + VIRTIO_PCI_DEVICEID_RANGE)
	{
		error("virtio for pci device bus %d dev %d had wrong device id 0x%x, "
			   "expected 0x%x",
			   pci_dev->bus, pci_dev->dev, pci_dev->device_id, VIRTIO_PCI_VENDORID);
		return -1;
	}

    virtio_pci_header* header = (virtio_pci_header*)phys_to_virt(pci_device_get_io_addr(pci_dev));
	pci_enable_bus_mastering(pci_dev);

#ifdef VIRTIO_PCI_ENABLE_MSI_X
    if(pci_enable_msix(pci_dev) < 0) {
        error("virtio enable msi-x irq failed");
        return -1;
    }
#endif

	/* First step of initialization: reset */
	WRITE8(header->DeviceStatus, 0);
	mb();
	/* Hello there, I see you */
	WRITE8(header->DeviceStatus, READ8(header->DeviceStatus) | VIRTIO_STATUS_ACKNOWLEDGE);
	mb();

	/* Hello, I am a driver for you */
	WRITE8(header->DeviceStatus, READ8(header->DeviceStatus) | VIRTIO_STATUS_DRIVER);
	mb();

	switch (pci_dev->subsystem_device_id)
	{
	case VIRTIO_DEV_BLK:
        debug("virtio block type");
		return virtio_blk_init(header, pci_dev);
        break;
	case VIRTIO_DEV_NET:
        debug("virtio net type");
		// return virtio_net_init(header, intid);
        break;
	default:
		error("unsupported virtio device type 0x%x",
			   pci_dev->subsystem_device_id);
	}
	
	return 0;
}

void virtio_device_init(void)
{
	pci_device_t *device;

	pci_for_using_device(device) {
		if (device->vendor_id == VIRTIO_PCI_VENDORID) {
			virtio_dev_init(device);
		}
	}
}