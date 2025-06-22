/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/virtio-net.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 * 
 * Virtio network driver
 */

#include <common.h>
#include <net/net.h>
#include <io/net.h>
#include <mm/mm.h>
#include <klib.h>
#include <drivers/virtio.h>
#include <drivers/virtio-pci.h>

#define VIRTIO_NET_DEV_NAME "virtio-net"

static uint32 virtio_net_devid = DEVID_VIRTIO_NET_BASE;

static uint32 virtio_net_get_devid(void)
{
    if(virtio_net_devid > DEVID_VIRTIO_NET_BASE + DEVID_VIRTIO_NET_RANGE)
    {
        panic("virtio-net: no more device id available");
    }
    return virtio_net_devid++;
}

struct virtio_cap net_caps[] = {
	{ "VIRTIO_NET_F_CSUM", 0, true,
	  "Device handles packets with partial checksum. This “checksum "
	  "offload” is a common feature on modern network cards." },
	{ "VIRTIO_NET_F_GUEST_CSUM", 1, false,
	  "Driver handles packets with partial checksum." },
	{ "VIRTIO_NET_F_CTRL_GUEST_OFFLOADS", 2, false,
	  "Control channel offloads reconfiguration support." },
	{ "VIRTIO_NET_F_MAC", 5, true, "Device has given MAC address." },
	{ "VIRTIO_NET_F_GUEST_TSO4", 7, false, "Driver can receive TSOv4." },
	{ "VIRTIO_NET_F_GUEST_TSO6", 8, false, "Driver can receive TSOv6." },
	{ "VIRTIO_NET_F_GUEST_ECN", 9, false,
	  "Driver can receive TSO with ECN." },
	{ "VIRTIO_NET_F_GUEST_UFO", 10, false, "Driver can receive UFO." },
	{ "VIRTIO_NET_F_HOST_TSO4", 11, false, "Device can receive TSOv4." },
	{ "VIRTIO_NET_F_HOST_TSO6", 12, false, "Device can receive TSOv6." },
	{ "VIRTIO_NET_F_HOST_ECN", 13, false,
	  "Device can receive TSO with ECN." },
	{ "VIRTIO_NET_F_HOST_UFO", 14, false, "Device can receive UFO." },
	{ "VIRTIO_NET_F_MRG_RXBUF", 15, false,
	  "Driver can merge receive buffers." },
	{ "VIRTIO_NET_F_STATUS", 16, true,
	  "Configuration status field is available." },
	{ "VIRTIO_NET_F_CTRL_VQ", 17, false, "Control channel is available." },
	{ "VIRTIO_NET_F_CTRL_RX", 18, false,
	  "Control channel RX mode support." },
	{ "VIRTIO_NET_F_CTRL_VLAN", 19, false,
	  "Control channel VLAN filtering." },
	{ "VIRTIO_NET_F_GUEST_ANNOUNCE", 21, false,
	  "Driver can send gratuitous packets." },
	{ "VIRTIO_NET_F_MQ", 22, false,
	  "Device supports multiqueue with automatic receive steering." },
	{ "VIRTIO_NET_F_CTRL_MAC_ADDR", 23, false,
	  "Set MAC address through control channel" },
	VIRTIO_INDP_CAPS
};

struct virtio_net {
	virtio_pci_header *header;
	volatile struct virtio_net_config *cfg;
	struct virtq_info *rx_info;
	struct virtq_info *tx_info;
    struct netdev netdev;
};

#define get_vnetdev(dev) container_of(dev, struct virtio_net, netdev)
#define get_vnetif(iface) container_of(iface, struct virtio_net, nif)

static void add_packets_to_virtqueue(int n, struct virtq_info *info)
{
    struct virtqueue *virtq = &info->virtq;
	int i;
	uint32 d1, d2;
	struct virtio_net_hdr *hdr;
	struct packet *pkt;
	for (i = 0; i < n; i++) {
		hdr = kalloc(sizeof(struct virtio_net_hdr));
		pkt = packet_alloc();
		hdr->packet = pkt;
		d1 = virtq_alloc_desc(info, hdr);
		d2 = virtq_alloc_desc(info, pkt->data);
		virtq->desc[d1].len = VIRTIO_NET_HDRLEN;
		virtq->desc[d1].flags = VIRTQ_DESC_F_WRITE | VIRTQ_DESC_F_NEXT;
		virtq->desc[d1].next = d2;
		virtq->desc[d2].len = PACKET_CAPACITY;
		virtq->desc[d2].flags = VIRTQ_DESC_F_WRITE;
		pkt->capacity = PACKET_CAPACITY;
		virtq->avail->ring[virtq->avail->idx + i] = d1;
	}
	mb();
	virtq->avail->idx += n;
}

static void virtio_net_send(struct netdev *dev, struct packet *pkt)
{
    struct virtio_net *netdev = get_vnetdev(dev);
	uint32 d1, d2;
	struct virtio_net_hdr *hdr =
	        (struct virtio_net_hdr *)kalloc(sizeof(struct virtio_net_hdr));

    struct virtqueue* tx = &netdev->tx_info->virtq;

	hdr->flags = 0;
	hdr->gso_type = VIRTIO_NET_HDR_GSO_NONE;
	hdr->hdr_len = 0;  /* not used unless we have segmentation offload */
	hdr->gso_size = 0; /* same */
	hdr->csum_start = 0;
	hdr->csum_offset = 0;
	hdr->num_buffers = 0xDEAD;
	hdr->packet = pkt;

	d1 = virtq_alloc_desc(netdev->tx_info, (void *)hdr);
	tx->desc[d1].len = VIRTIO_NET_HDRLEN;
	tx->desc[d1].flags = VIRTQ_DESC_F_NEXT;

	d2 = virtq_alloc_desc(netdev->tx_info, pkt->ll);
	tx->desc[d2].len = (pkt->end - pkt->ll);
	tx->desc[d2].flags = 0;

	tx->desc[d1].next = d2;

	tx->avail->ring[tx->avail->idx] = d1;
	mb();
	tx->avail->idx += 1;
	mb();
	WRITE32(netdev->header->QueueNotify, netdev->tx_info->queue_num);
}

static void virtio_net_status(struct netdev *dev)
{
    struct virtio_net *netdev = get_vnetdev(dev);
	log("virtio_net_dev at 0x%lx",
	       virt_to_phys((uint64)netdev->header));
	log("    Status=0x%x", READ32(netdev->header->DeviceStatus));
	// log("    DeviceID=0x%x", READ32(netdev->header->DeviceID));
	// log("    VendorID=0x%x", READ32(netdev->header->VendorID));
	// log("    InterruptStatus=0x%x",
	//        READ32(netdev->header->InterruptStatus));
	// log("    MagicValue=0x%x", READ32(netdev->header->MagicValue));
	log("  tx queue:");
	log("    avail.idx = %u", netdev->tx_info->virtq.avail->idx);
	log("    used.idx = %u", netdev->tx_info->virtq.used->idx);
	WRITE32(netdev->header->QueueSelect, netdev->tx_info->queue_num);
	mb();
	log("  rx queue:");
	log("    avail.idx = %u", netdev->rx_info->virtq.avail->idx);
	log("    used.idx = %u", netdev->rx_info->virtq.used->idx);
	WRITE32(netdev->header->QueueSelect, netdev->rx_info->queue_num);
}

static void virtio_handle_rxused(struct virtio_net *dev, uint32 idx)
{
    struct virtq_info *rx_info = dev->rx_info;
    struct virtqueue *rx = &rx_info->virtq;

	uint32 d1 = rx->used->ring[idx].id;
	uint32 d2 = rx->desc[d1].next;
	uint32 len = rx->used->ring[idx].len;
	struct virtio_net_hdr *hdr =
	        (struct virtio_net_hdr *)rx_info->desc_virt[d1];
	/* We can get this from d2, but hdr->packet is more foolproof, since we
	 * don't necessarily know the offset into the packet structure which d2
	 * will point at. */
	struct packet *pkt = hdr->packet;
	pkt->ll = rx_info->desc_virt[d2];
	pkt->end = pkt->ll + (len - VIRTIO_NET_HDRLEN);
	eth_recv(&dev->netdev.netif, pkt);

	/* eth_recv takes ownership of pkt, we will put a new packet in there
	 * and stick the descriptor back into the avail queue */
	pkt = packet_alloc();
	hdr->packet = pkt;
	rx->desc[d2].addr = virt_to_phys((uint64)&pkt->data);
	rx_info->desc_virt[d2] = &pkt->data;

	rx->avail->ring[rx->avail->idx] = d1;
	rx->avail->ring[wrap(rx->avail->idx + 1, rx_info->queue_size)] = d2;
	mb();
	rx->avail->idx = wrap(rx->avail->idx + 2, rx_info->queue_size);
}

static void virtio_handle_txused(struct virtio_net *dev, uint32 idx)
{
	uint32 d1 = dev->tx_info->virtq.used->ring[idx].id;

	struct virtio_net_hdr *hdr =
	        (struct virtio_net_hdr *)dev->tx_info->desc_virt[d1];
	struct packet *pkt = hdr->packet;

	kfree(hdr);
	packet_free(pkt);
}

static irqret_t virtio_net_isr(struct netdev *netdev)
{
    struct virtio_net *dev = get_vnetdev(netdev);

	uint32 i;
    struct virtqueue *rx = &dev->rx_info->virtq;
    struct virtq_info *rx_info = dev->rx_info;
    struct virtqueue *tx = &dev->tx_info->virtq;
    struct virtq_info *tx_info = dev->tx_info;

    if (!dev)
    {
        error("virtio-net: received IRQ for unknown device!");
        return IRQ_ERR;
    }

#ifdef VIRTIO_PCI_ENABLE_MSI_X
    // debug("virtio blk receive msi-x isr");
#else
    if (!(READ8(dev->header->ISRStatus) & 1))
    {
        log("virtio-blk: IRQ not for this device");
        return IRQ_SKIP;
    }
#endif

	for (i = rx_info->seen_used; i != rx->used->idx;
	     i = wrap(i + 1, 32)) {
		virtio_handle_rxused(dev, i);
	}
	rx_info->seen_used = rx->used->idx;
	for (i = tx_info->seen_used; i != tx->used->idx;
	     i = wrap(i + 1, 32)) {
		virtio_handle_txused(dev, i);
	}
	tx_info->seen_used = tx->used->idx;

    return IRQ_HANDLED;
}

struct netdev_ops virtio_net_ops = {
    .send = virtio_net_send,
    .status = virtio_net_status,
    .irq_handle = virtio_net_isr,
};

int virtio_net_init(virtio_pci_header *header, pci_device_t *pci_dev)
{
    struct virtio_net *vdev;
	volatile struct virtio_net_config *cfg =
	        (struct virtio_net_config *)header->Config;
    int i;
    uint8 macbyte;
	uint32 intid;

    vdev = kalloc(sizeof(struct virtio_net));

#ifndef VIRTIO_PCI_ENABLE_MSI_X
    intid = virtio_init_irq_intx(pci_dev);
#else
    intid = virtio_init_irq_msix(header, pci_dev);
#endif

	virtio_check_capabilities(header, net_caps, nr_elem(net_caps));

	vdev->header = header;
	vdev->cfg = cfg;
	vdev->rx_info = virtq_add_to_device(header, 0);
    vdev->tx_info = virtq_add_to_device(header, 1);
    assert(vdev->rx_info != NULL);
    assert(vdev->tx_info != NULL);

	vdev->netdev.netif.ip = 0;
	
    // Read the MAC address from the device configuration.
    // Alert that read operations are not aligned, thus this may fail
    for(i = 0; i < 6; i++) {
        vdev->netdev.netif.mac[i] = READ8(cfg->mac[i]);
        do {
            macbyte = vdev->netdev.netif.mac[i];
            mb();
            vdev->netdev.netif.mac[i] = READ8(cfg->mac[i]);
        } while(vdev->netdev.netif.mac[i] != macbyte);
    }

	vdev->netdev.netif.gateway_ip = 0;
	vdev->netdev.netif.subnet_mask = 0;

	add_packets_to_virtqueue(64, vdev->rx_info);

	WRITE32(header->DeviceStatus, READ32(header->DeviceStatus) | VIRTIO_STATUS_DRIVER_OK);
	mb();

    netdev_init(&vdev->netdev, virtio_net_get_devid(), intid,
                VIRTIO_NET_DEV_NAME, &virtio_net_ops);

	debug("virtio-net 0x%lx (intid %u, MAC %s): ready!", virt_to_phys((uint64)header), intid, mac_ntoa(vdev->netdev.netif.mac));
	return 0;
}
