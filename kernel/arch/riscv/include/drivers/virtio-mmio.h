/**
 * This code is partly from Stephen's OS (MIT License)
 * Original source: https://github.com/brenns10/sos/blob/master/kernel/virtio.h
 * Copyright (c) 2018-2022 Stephen Brennan
 * For full license text, see LICENSE-MIT-sos file in this repository
 */

#ifndef __VIRTIO_MMIO_H__
#define __VIRTIO_MMIO_H__

#include <common.h>
#include <io/blk.h>
#include <arch.h>
#include <drivers/virtio.h>

#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_VERSION 0x1

/**
 * Legacy register layout
 * See Section 4.2.4 of VIRTIO 1.0 Spec:
 * http://docs.oasis-open.org/virtio/virtio/v1.0/cs04/virtio-v1.0-cs04.html
 */
typedef volatile struct __attribute__((aligned(4)))
{
    /* 0x000 */ uint32 MagicValue;      // R
    /* 0x004 */ uint32 Version;         // R
    /* 0x008 */ uint32 DeviceID;        // R
    /* 0x00c */ uint32 VendorID;        // R
    /* 0x010 */ uint32 HostFeatures;    // R
    /* 0x014 */ uint32 HostFeaturesSel; // W
    /* 0x018 */ uint32 _reserved0[2];
    /* 0x020 */ uint32 GuestFeatures;    // W
    /* 0x024 */ uint32 GuestFeaturesSel; // W
    /* 0x028 */ uint32 GuestPageSize;    // W
    /* 0x02c */ uint32 _reserved1;
    /* 0x030 */ uint32 QueueSel;    // W
    /* 0x034 */ uint32 QueueNumMax; // R
    /* 0x038 */ uint32 QueueNum;    // W
    /* 0x03c */ uint32 QueueAlign;  // W
    /* 0x040 */ uint32 QueuePFN;    // RW
    /* 0x044 */ uint32 _reserved2[3];
    /* 0x050 */ uint32 QueueNotify; // W
    /* 0x054 */ uint32 _reserved3[3];
    /* 0x060 */ uint32 InterruptStatus; // R
    /* 0x064 */ uint32 InterruptACK;    // W
    /* 0x068 */ uint32 _reserved4[2];
    /* 0x070 */ uint32 Status; // RW
    /* 0x074 */ uint32 _reserved5[3];
    /* 0x080 */ uint32 _reserved6[0x20];
    /* 0x100 */ uint32 Config[]; // RW
} virtio_regs;

/*
 * virtqueue routines
 */
struct virtq_info* virtq_add_to_device(volatile virtio_regs *regs, uint32 queue_sel, uint32 queue_size);

void virtio_check_capabilities(virtio_regs *regs, struct virtio_cap *caps, uint32 n);

int virtio_blk_init(volatile virtio_regs *regs, uint32 intid);
int virtio_net_init(virtio_regs *regs, uint32 intid);

#define VIRTIO_MMIO_DEV_NUM 8

#endif // __VIRTIO_MMIO_H__
