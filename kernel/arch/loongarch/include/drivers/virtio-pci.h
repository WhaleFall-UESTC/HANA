#ifndef __VIRTIO_PCI_H__
#define __VIRTIO_PCI_H__

#include <common.h>
#include <io/blk.h>
#include <arch.h>
#include <drivers/virtio.h>

#define VIRTIO_PCI_VENDORID 0x2af4
#define VIRTIO_PCI_DEVICEID_BASE 0x1000
#define VIRTIO_PCI_DEVICEID_RANGE 0x3f

typedef volatile struct __attribute__((packed)) {
    uint32 DeviceFeature;
    uint32 GuestFeature;
    uint32 QueueAddress;
    uint16 QueueSize;
    uint16 QueueSelect;
    uint16 QueueNotify;
    uint8 DeviceStatus;
    uint8 ISRStatus;
#ifdef MSI_X
    uint16 ConfigurationVector
    uint16 QueueVector;
#endif
} virtio_pci_header;



#endif // __VIRTIO_PCI_H__