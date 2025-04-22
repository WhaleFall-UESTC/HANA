/*
 * Copyright (c) 2015 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup lwext4
 * @{
 */
/**
 * @file  ext4_mkfs.h
 * @brief
 */

#ifndef EXT4_MKFS_H_
#define EXT4_MKFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <fs/ext4/lwext4/ext4_config.h>
#include <fs/ext4/lwext4/ext4_types.h>

#include <fs/ext4/lwext4/ext4_blockdev.h>
#include <fs/ext4/lwext4/ext4_fs.h>

#include <common.h>

struct ext4_mkfs_info {
	uint64 len;
	uint32 block_size;
	uint32 blocks_per_group;
	uint32 inodes_per_group;
	uint32 inode_size;
	uint32 inodes;
	uint32 journal_blocks;
	uint32 feat_ro_compat;
	uint32 feat_compat;
	uint32 feat_incompat;
	uint32 bg_desc_reserve_blocks;
	uint16 dsc_size;
	uint8 uuid[UUID_SIZE];
	bool journal;
	const char *label;
};


int ext4_mkfs_read_info(struct ext4_blockdev *bd, struct ext4_mkfs_info *info);

int ext4_mkfs(struct ext4_fs *fs, struct ext4_blockdev *bd,
	      struct ext4_mkfs_info *info, int fs_type);

#ifdef __cplusplus
}
#endif

#endif /* EXT4_MKFS_H_ */

/**
 * @}
 */
