#ifndef __VIRTIO_PCI_H__
#define __VIRTIO_PCI_H__

#include <common.h>
#include <io/blk.h>
#include <arch.h>
#include <drivers/virtio.h>
#include <drivers/pci.h>

#define VIRTIO_PCI_VENDORID 0x1af4
#define VIRTIO_PCI_DEVICEID_BASE 0x1000
#define VIRTIO_PCI_DEVICEID_RANGE 0x3f

// #define VIRTIO_PCI_ENABLE_MSI_X

typedef volatile struct __attribute__((packed)) {
    uint32 DeviceFeature;
    uint32 GuestFeature;
    uint32 QueueAddress;
    uint16 QueueSize;
    uint16 QueueSelect;
    uint16 QueueNotify;
    uint8 DeviceStatus;
    uint8 ISRStatus;
#ifdef VIRTIO_PCI_ENABLE_MSI_X
#define VIRTIO_MSI_NO_VECTOR 0xffff
    uint16 ConfigurationVector;
    uint16 QueueVector;
#endif
    uint32 Config[];
} virtio_pci_header;

/*
 * virtqueue routines
 */
struct virtq_info *virtq_add_to_device(volatile virtio_pci_header *header, uint32 queue_sel);
void virtio_check_capabilities(virtio_pci_header *header, struct virtio_cap *caps, uint32 n);
int virtio_blk_init(volatile virtio_pci_header *header, pci_device_t *pci_dev);

#endif // __VIRTIO_PCI_H__