/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/ip.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#include <common.h>
#include <net/net.h>
#include <klib.h>
#include <debug.h>

static uint32 ipid = 0;

struct ip_mac_mapping {
	uint32 ip;
	uint8 mac[6];
};

#define MAX_MAPPINGS 16
static struct ip_mac_mapping mappings[MAX_MAPPINGS] = {
	{ 0xFFFFFFFF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } },
};
int nmap = 1;

static uint8 *get_mapping(uint32 ip)
{
	int i;
	for (i = 0; i < nmap; i++)
		if (mappings[i].ip == ip)
			return mappings[i].mac;
	return NULL;
}

static void upsert_mapping(uint32 ip, uint8 *mac)
{
	int i;
	for (i = 0; i < nmap; i++)
		if (mappings[i].ip == ip)
			break;
	if (i == nmap) {
		if (i >= MAX_MAPPINGS) {
			log("oh no! ran out of ip-mac mapping space");
		}
		nmap++;
	}
	mappings[i].ip = ip;
	memcpy(mappings[i].mac, mac, 6);
}

int ip_cmd_show_arptable(int argc, char **argv)
{
	int i;
	log("IP <--> MAC");
	for (i = 0; i < nmap; i++)
		log("%s: %s", ip_ntoa(mappings[i].ip), mac_ntoa(mappings[i].mac));
	return 0;
}

void ip_recv(struct netif *netif, struct packet *pkt)
{
	/*log("ip_recv src=%I dst=%I", pkt->ip->src, pkt->ip->dst);*/
	if (pkt->ip->dst != netif->ip && pkt->ip->dst != 0xFFFFFFFF) {
		log("received IP packet not destined for us, dropping");
		goto cleanup;
	}
	pkt->tl = pkt->nl + ip_get_length(pkt->ip);
	if (pkt->tl > pkt->end) {
		log("received IP packet with bad IHL field, dropping");
		goto cleanup;
	}
	upsert_mapping(pkt->ip->src, pkt->eth->src_mac);
	switch (pkt->ip->proto) {
	case IPPROTO_UDP:
		udp_recv(netif, pkt);
		return;
	default:
		warn("unimplemented ipproto 0x%x, dropping",
		       pkt->ip->proto);
		goto cleanup;
	}
cleanup:
	packet_free(pkt);
}

uint32 ip_route(struct netif *netif, uint32 dst_ip)
{
	if ((dst_ip & netif->subnet_mask) ==
	    (netif->gateway_ip & netif->subnet_mask))
		return dst_ip;
	return netif->gateway_ip;
}

uint8 *ip_get_mac(struct netif *netif, uint32 nexthop)
{
	return get_mapping(nexthop);
}

int ip_send(struct netif *netif, struct packet *pkt, uint8 proto,
            uint32 src_ip, uint32 dst_ip)
{
	uint32 csum;
	uint32 nexthop = ip_route(netif, dst_ip);
	uint8 *mac = ip_get_mac(netif, nexthop);

	pkt->nl = pkt->tl - sizeof(struct iphdr);

	/* Set IHL to 5, Version to 4 */
	pkt->ip->verihl = 5 | (4 << 4);
	pkt->ip->len = htons(pkt->end - pkt->nl);
	pkt->ip->id = htons(ipid++);
	pkt->ip->ttl = 32; /* somewhat low so we don't break the internet */
	pkt->ip->proto = proto;
	pkt->ip->src = netif->ip;
	pkt->ip->dst = dst_ip;
	csum_init(&csum);
	csum_add(&csum, pkt->nl, 10);
	pkt->ip->csum = csum_finalize(&csum);

	eth_send(netif, pkt, ETHERTYPE_IP, mac);
	return 0;
}

int ip_reserve(void)
{
	/* currently we don't use ip options */
	return sizeof(struct etherframe) + sizeof(struct iphdr);
}
