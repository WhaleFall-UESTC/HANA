/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/net.h
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#ifndef __NET_H__
#define __NET_H__

#include <net/inet.h>
#include <net/packets.h>
#include <klib.h>

struct netif {
	uint32 ip;
	uint8 mac[6];

	uint32 gateway_ip;
	uint32 subnet_mask;
	uint32 dns;
};

void eth_recv(struct netif *netif, struct packet *pkt);
int eth_send(struct netif *netif, struct packet *pkt, uint16 ethertype,
             uint8 dst_mac[6]);

void ip_recv(struct netif *netif, struct packet *pkt);
/* TODO: maybe netif shouldn't be in ip_send() */
int ip_send(struct netif *netif, struct packet *pkt, uint8 proto,
            uint32 src_ip, uint32 dst_ip);
int ip_reserve(void);

void udp_recv(struct netif *netif, struct packet *pkt);
void udp_send(struct netif *netif, struct packet *pkt, uint32 src_ip,
              uint32 dst_ip, uint16 src_port, uint16 dst_port);
int udp_reserve(void);
void udp_init(void);

void csum_init(uint32 *csum);
void csum_add(uint32 *csum, uint16 *data, uint32 n);
void csum_add_value(uint32 *csum, uint16 data);
uint16 csum_finalize(uint32 *csum);

static inline char* ip_ntoa(uint32 ip)
{
	static char buf[16];
	sprintf(buf, "%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
	        (ip >> 8) & 0xFF, ip & 0xFF);
	return buf;
}

static inline char* mac_ntoa(uint8 mac[6])
{
	static char buf[18];
	sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
	        mac[3], mac[4], mac[5]);
	return buf;
}

#endif // __NET_H__