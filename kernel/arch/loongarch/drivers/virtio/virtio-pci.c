/**
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
#include <platform.h>
#include <klib.h>

// static int virtio_dev_init(uint64 virt, uint32 intid)
// {
// 	volatile virtio_pci_header *header = (virtio_pci_header *)virt;
// 	int device_id;

// 	if (READ32(header->MagicValue) != VIRTIO_MAGIC)
// 	{
// 		error("virtio at 0x%lx had wrong magic value 0x%x, "
// 			   "expected 0x%x",
// 			   virt, header->MagicValue, VIRTIO_MAGIC);
// 		return -1;
// 	}
// 	if (READ32(header->Version) != VIRTIO_VERSION)
// 	{
// 		error("virtio at 0x%lx had wrong version 0x%x, expected "
// 			   "0x%x",
// 			   virt, header->Version, VIRTIO_VERSION);
// 		return -1;
// 	}
// 	if ((device_id = (header->DeviceID)) == 0)
// 	{
// 		/*On QEMU, this is pretty common, don't print a message */
// 		/*log("warn: virtio at 0x%x has DeviceID=0, skipping",
// 		 * virt);*/
// 		return -1;
// 	}

// 	/* First step of initialization: reset */
// 	WRITE32(header->Status, 0);
// 	mb();
// 	/* Hello there, I see you */
// 	WRITE32(header->Status, READ32(header->Status) | VIRTIO_STATUS_ACKNOWLEDGE);
// 	mb();

// 	/* Hello, I am a driver for you */
// 	WRITE32(header->Status, READ32(header->Status) | VIRTIO_STATUS_DRIVER);
// 	mb();

// 	switch (device_id)
// 	{
// 	case VIRTIO_DEV_BLK:
// 		return virtio_blk_init(header, intid);
// 	// case VIRTIO_DEV_NET:
// 		// return virtio_net_init(header, intid);
// 	default:
// 		error("unsupported virtio device ID 0x%x",
// 			   READ32(header->DeviceID));
// 	}
	
// 	return 0;
// }

void virtio_device_init(void)
{
	
}