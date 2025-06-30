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

#define VIRTIO_PCI_ENABLE_MSI_X

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
/**
 * Add a virtqueue to a virtio device.
 * @param header Pointer to virtio PCI registers.
 * @param queue_sel Queue selector (queue index).
 * @return Pointer to the created virtq_info structure, or NULL on failure.
 */
struct virtq_info *virtq_add_to_device(volatile virtio_pci_header *header, uint32 queue_sel);

/**
 * Check and negotiate virtio device capabilities.
 * @param header Pointer to virtio PCI registers.
 * @param caps Array of virtio_cap structures describing capabilities.
 * @param n Number of capabilities in the array.
 */
void virtio_check_capabilities(virtio_pci_header *header, struct virtio_cap *caps, uint32 n);

/**
 * Initialize a virtio block device.
 * @param header Pointer to virtio PCI registers.
 * @param pci_dev PCI device struct for the device.
 * @return 0 on success, negative value on failure.
 */
int virtio_blk_init(volatile virtio_pci_header *header, pci_device_t *pci_dev);
/**
 * Initialize a virtio network device.
 * @param header Pointer to virtio PCI registers.
 * @param intid PCI device struct for the device.
 * @return 0 on success, negative value on failure.
 */
int virtio_net_init(virtio_pci_header *header, pci_device_t *pci_dev);

#ifdef VIRTIO_PCI_ENABLE_MSI_X
/**
 * Initialize the virtio device with MSI-X interrupts.
 * @param header Pointer to the virtio PCI header.
 * @param pci_dev Pointer to the PCI device structure.
 * @return The allocated interrupt ID, or 0 on failure.
 */
uint32 virtio_init_irq_msix(volatile virtio_pci_header *header, pci_device_t *pci_dev);
#else
/**
 * Initialize the virtio device with INTx interrupts.
 * @param pci_dev Pointer to the PCI device structure.
 * @return The allocated interrupt ID.
 */
uint32 virtio_init_irq_intx(pci_device_t *pci_dev);
#endif

#endif // __VIRTIO_PCI_H__