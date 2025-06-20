/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/packets.h
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 * 
 * packets.h: protocol header and body structs
 */
#ifndef __PACKETS_H__
#define __PACKETS_H__

#include <common.h>
#include <tools/list.h>

/*
 * Ethernet Frame Header. A CRC-32 usually goes after the data.
 * https://en.wikipedia.org/wiki/Ethernet_frame
 */
struct etherframe {
	uint8 dst_mac[6];
	uint8 src_mac[6];
	uint16 ethertype;
} __attribute__((packed));

#define ETHERTYPE_IP 0x0800

/*
 * IP Header
 * https://tools.ietf.org/html/rfc791#page-11
 */
struct iphdr {
	uint8 verihl;
	uint8 tos;
	uint16 len;
	uint16 id;
	uint16 flags_foffset;
	uint8 ttl;
	uint8 proto;
	uint16 csum;
	uint32 src;
	uint32 dst;
	uint8 options[0];
} __attribute__((packed));

#define ip_get_version(ip) (((ip)->verihl & 0xF0) >> 4)
#define ip_get_length(ip)  (((ip)->verihl & 0x0F) * 4)

#define IPPROTO_UDP 17

/*
 * UDP Header
 * https://tools.ietf.org/html/rfc768
 */
struct udphdr {
	uint16 src_port;
	uint16 dst_port;
	uint16 len;
	uint16 csum;
} __attribute__((aligned(2)));

#define UDPPORT_DHCP_CLIENT 68
#define UDPPORT_DHCP_SERVER 67

struct dhcp_option {
	uint8 tag;
	uint8 len;
	uint8 data[0];
} __attribute__((packed));

/*
 * DHCP Packet
 * https://tools.ietf.org/html/rfc2131#page-9
 */
struct dhcp {
	uint8 op;
	uint8 htype;
	uint8 hlen;
	uint8 hops;
	uint32 xid;
	uint16 secs;
	uint16 flags;
	uint32 ciaddr;
	uint32 yiaddr;
	uint32 siaddr;
	uint32 giaddr;
	uint32 chaddr[4];
	uint8 sname[64];
	uint8 file[128];
	uint32 cookie;
	uint8 options[0];
} __attribute__((packed));

#define BOOTREQUEST       1
#define BOOTREPLY         2
#define DHCP_MAGIC_COOKIE 0x63825363

#define DHCP_HTYPE_ETHERNET 1

enum {
	/* clang-format: please allow tab indentation here */
	DHCPOPT_PAD = 0,
	DHCPOPT_SUBNET_MASK,
	DHCPOPT_TIME_OFFSET,
	DHCPOPT_ROUTER,
	DHCPOPT_TIME_SERVER,
	DHCPOPT_NAME_SERVER,
	DHCPOPT_DOMAIN_NAME_SERVER,
	DHCPOPT_LOG_SERVER,
	DHCPOPT_COOKIE_SERVER,
	DHCPOPT_LPR_SERVER,
	DHCPOPT_IMPRESS_SERVER,
	DHCPOPT_RESOURCE_LOCATION_SERVER,
	DHCPOPT_HOST_NAME,
	DHCPOPT_BOOT_FILE_SIZE,
	DHCPOPT_MERIT_DUMP_FILE,
	DHCPOPT_DOMAIN_NAME,
	DHCPOPT_SWAP_SERVER,
	DHCPOPT_ROOT_PATH,
	DHCPOPT_EXTENSIONS_PATH,
	DHCPOPT_IP_FORWARDING_ENABLE,
	DHCPOPT_IP_NONLOCAL_SOURCE_ROUTING,

	DHCPOPT_LEASE_TIME = 51,
	/* lots of seemingly less relevant options, trimming here */
	DHCPOPT_MSG_TYPE = 53,
	DHCPOPT_SERVER_IDENTIFIER,
	DHCPOPT_END = 255,
};

#define DHCPMTYPE_DHCPDISCOVER 1
#define DHCPMTYPE_DHCPOFFER    2
#define DHCPMTYPE_DHCPREQUEST  3
#define DHCPMTYPE_DHCPDECLINE  4
#define DHCPMTYPE_DHCPACK      5
#define DHCPMTYPE_DHCPNAK      6
#define DHCPMTYPE_DHCPRELEASE  7

struct packet {
	/* Link-layer header pointer */
	union {
		void *ll;
		struct etherframe *eth;
	};

	/* Network-layer header pointer */
	union {
		void *nl;
		struct iphdr *ip;
	};

	/* Transport-layer header pointer */
	union {
		void *tl;
		struct udphdr *udp;
	};

	/* App-layer pointer */
	union {
		void *al;
		void *app;
	};

	void *end;

	/* Packets need to be queued in various places, we use this list */
	struct list_head list;
	uint32 capacity;
	uint8 data[0];
};

struct packet *packet_alloc(void);
void packet_free(struct packet *pkt);
#define PACKET_SIZE     2048
#define PACKET_CAPACITY (PACKET_SIZE - sizeof(struct packet))

#define MAX_ETH_PKT_SIZE 1514


#endif // __PACKETS_H__