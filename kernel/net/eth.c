/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/eth.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#include <common.h>
#include <net/net.h>
#include <klib.h>
#include <io/net.h>

uint8 broadcast_mac[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#define MAC_SIZE 6

void eth_recv(struct netif *netif, struct packet *pkt)
{
	/*log("eth_recv src=%M dst=%M ethertype=0x%x", &pkt->eth->src_mac,
	       &pkt->eth->dst_mac, ntohs(pkt->eth->ethertype));*/
	pkt->nl = pkt->ll + sizeof(struct etherframe);
	if (memcmp(pkt->eth->dst_mac, broadcast_mac, MAC_SIZE) == 0 ||
	    memcmp(pkt->eth->dst_mac, netif->mac, MAC_SIZE) == 0) {
		switch (ntohs(pkt->eth->ethertype)) {
		case ETHERTYPE_IP:
			ip_recv(netif, pkt);
			break;
		default:
			error("received ethernet packet of unknown ethertype "
			       "0x%x",
			       ntohs(pkt->eth->ethertype));
			packet_free(pkt);
		}
	} else {
		log("received ethernet packet not destined for us");
		packet_free(pkt);
	}
}

int eth_send(struct netif *netif, struct packet *pkt, uint16 ethertype,
             uint8 dst_mac[MAC_SIZE])
{
	pkt->ll = pkt->nl - sizeof(struct etherframe);
	if (pkt->ll < (void *)&pkt->data) {
		return -1;
	}

	memcpy(pkt->eth->src_mac, netif->mac, MAC_SIZE);
	memcpy(pkt->eth->dst_mac, dst_mac, MAC_SIZE);
	pkt->eth->ethertype = htons(ethertype);

	struct netdev *dev = container_of(netif, struct netdev, netif);
	dev->ops->send(dev, pkt);
	return 0;
}