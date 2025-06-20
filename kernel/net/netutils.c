/**
 * This code is from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/netutils.c
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 * 
 * network utilities
 */

#include <common.h>
#include <net/net.h>
#include <mm/mm.h>

struct slab *pktslab = NULL;
#define PACKET_SIZE 2048

void csum_init(uint32 *csum)
{
	*csum = 0;
}

void csum_add(uint32 *csum, uint16 *data, uint32 n)
{
	uint32 i;
	for (i = 0; i < n; i++) {
		*csum += data[i];
	}
}

void csum_add_value(uint32 *csum, uint16 data)
{
	csum_add(csum, &data, 1);
}

uint16 csum_finalize(uint32 *csum)
{
	uint32 add;
	while (*csum & 0xFFFF0000) {
		add = (*csum & 0xFFFF0000) >> 16;
		*csum &= 0x0000FFFF;
		*csum += add;
	}
	return ~((uint16)*csum);
}

void packet_init(void)
{
	
}

struct packet *packet_alloc(void)
{
	struct packet *pkt = (struct packet *)kalloc(PACKET_SIZE);
	memset(pkt, 0, PACKET_SIZE);
	pkt->capacity = PACKET_CAPACITY;
	return pkt;
}

void packet_free(struct packet *pkt)
{
	kfree((void *)pkt);
}
