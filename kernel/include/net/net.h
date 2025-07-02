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

/**
 * Process a received Ethernet frame
 * @param netif: Network interface structure
 * @param pkt: Incoming packet containing Ethernet frame
 */
void eth_recv(struct netif *netif, struct packet *pkt);

/**
 * Process a received Ethernet frame
 * @param netif: Network interface structure
 * @param pkt: Incoming packet containing Ethernet frame
 */
int eth_send(struct netif *netif, struct packet *pkt, uint16 ethertype, uint8 dst_mac[6]);


/**
 * Process an incoming IP packet
 * @param netif: Network interface structure
 * @param pkt: Incoming IP packet
 */
void ip_recv(struct netif *netif, struct packet *pkt);

/* TODO: maybe netif shouldn't be in ip_send() */
/**
 * Send an IP packet
 * @param netif: Network interface structure
 * @param pkt: Packet to send (with IP header to be populated)
 * @param proto: IP protocol number (e.g., IPPROTO_UDP)
 * @param src_ip: Source IP address
 * @param dst_ip: Destination IP address
 * @return 0 on success, -1 on error
 */
int ip_send(struct netif *netif, struct packet *pkt, uint8 proto, uint32 src_ip, uint32 dst_ip);

/**
 * Calculate space needed for IP headers
 * @return Size in bytes for IP header reservation
 */
int ip_reserve(void);

/**
 * Process an incoming UDP packet
 * @param netif: Network interface structure
 * @param pkt: Incoming UDP packet
 */
void udp_recv(struct netif *netif, struct packet *pkt);

/**
 * Send a UDP datagram
 * @param netif: Network interface structure
 * @param pkt: Packet to send (with UDP header to be populated)
 * @param src_ip: Source IP address
 * @param dst_ip: Destination IP address
 * @param src_port: Source port
 * @param dst_port: Destination port
 */
void udp_send(struct netif *netif, struct packet *pkt, uint32 src_ip, uint32 dst_ip, uint16 src_port, uint16 dst_port);

/**
 * Calculate space needed for UDP headers
 * @return Size in bytes for UDP header reservation
 */
int udp_reserve(void);

/**
 * Initialize UDP subsystem and register protocol
 */
void udp_init(void);

/**
 * Initialize checksum calculation
 * @param csum: Pointer to checksum accumulator
 */
void csum_init(uint32 *csum);
/**
 * Add data to checksum calculation
 * @param csum: Pointer to checksum accumulator
 * @param data: Pointer to data to add
 * @param n: Number of 16-bit words to add
 */
void csum_add(uint32 *csum, uint16 *data, uint32 n);

/**
 * Add single value to checksum calculation
 * @param csum: Pointer to checksum accumulator
 * @param data: 16-bit value to add
 */
void csum_add_value(uint32 *csum, uint16 data);

/**
 * Add single value to checksum calculation
 * @param csum: Pointer to checksum accumulator
 * @param data: 16-bit value to add
 */
uint16 csum_finalize(uint32 *csum);

/**
 * Convert a 32-bit IP address to a string in dotted-decimal format.
 * @param ip: The 32-bit IP address in network byte order.
 * @return A static buffer containing the string representation of the IP address.
 */
static inline char* ip_ntoa(uint32 ip)
{
	static char buf[16];
	sprintf(buf, "%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
	        (ip >> 8) & 0xFF, ip & 0xFF);
	return buf;
}

/**
 * Convert a MAC address to a string in colon-separated hexadecimal format.
 * @param mac: The MAC address as a 6-byte array.
 * @return A static buffer containing the string representation of the MAC address.
 */
static inline char* mac_ntoa(uint8 mac[6])
{
	static char buf[18];
	sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
	        mac[3], mac[4], mac[5]);
	return buf;
}

#endif // __NET_H__