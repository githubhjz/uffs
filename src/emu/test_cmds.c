/*
  This file is part of UFFS, the Ultra-low-cost Flash File System.
  
  Copyright (C) 2005-2009 Ricky Zheng <ricky_gz_zheng@yahoo.co.nz>

  UFFS is free software; you can redistribute it and/or modify it under
  the GNU Library General Public License as published by the Free Software 
  Foundation; either version 2 of the License, or (at your option) any
  later version.

  UFFS is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  or GNU Library General Public License, as applicable, for more details.
 
  You should have received a copy of the GNU General Public License
  and GNU Library General Public License along with UFFS; if not, write
  to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA  02110-1301, USA.

  As a special exception, if other files instantiate templates or use
  macros or inline functions from this file, or you compile this file
  and link it with other works to produce a work based on this file,
  this file does not by itself cause the resulting work to be covered
  by the GNU General Public License. However the source code for this
  file must still be made available in accordance with section (3) of
  the GNU General Public License v2.
 
  This exception does not invalidate any other reasons why a work based
  on this file might be covered by the GNU General Public License.
*/

/**
 * \file test_cmds.c
 * \brief commands for test uffs
 * \author Ricky Zheng
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uffs/uffs_config.h"
#include "uffs/uffs_public.h"
#include "uffs/uffs_fs.h"
#include "uffs/uffs_utils.h"
#include "uffs/uffs_core.h"
#include "uffs/uffs_mtb.h"
#include "uffs/uffs_find.h"
#include "cmdline.h"

#define PFX "test:"


static BOOL cmdTestPageReadWrite(const char *tail)
{
	TreeNode *node;
	uffs_Device *dev;
	uffs_Tags local_tag;
	uffs_Tags *tag = &local_tag;
	int ret;
	u16 block;
	u16 page;

	u8 page_buf[UFFS_MAX_PAGE_SIZE];
	u32 i;

	dev = uffs_GetDeviceFromMountPoint("/");

	node = uffs_TreeGetErasedNode(dev);
	if (!node) {
		uffs_Perror(UFFS_ERR_SERIOUS, PFX"no free block ?\n");
		goto ext;
	}

	for (i = 0; i < dev->com.pg_data_size; i++) {
		page_buf[i] = i & 0xFF;
	}

	block = node->u.list.block;
	page = 1;

	TAG_DATA_LEN(tag) = dev->com.pg_data_size;
	TAG_TYPE(tag) = UFFS_TYPE_DATA;
	TAG_PAGE_ID(tag) = 3;
	TAG_PARENT(tag) = 100;
	TAG_SERIAL(tag) = 10;
	TAG_BLOCK_TS(tag) = 1;

	ret = uffs_FlashWritePageCombine(dev, block, page, page_buf, tag);
	if (UFFS_FLASH_HAVE_ERR(ret)) {
		uffs_Perror(UFFS_ERR_SERIOUS, PFX"Write page error: %d\n", ret);
		goto ext;
	}


	ret = uffs_FlashReadPage(dev, block, page, page_buf);
	if (UFFS_FLASH_HAVE_ERR(ret)) {
		uffs_Perror(UFFS_ERR_SERIOUS, PFX"Read page error: %d\n", ret);
		goto ext;
	}

	for (i = 0; i < dev->com.pg_data_size; i++) {
		if (page_buf[i] != (i & 0xFF)) {
			uffs_Perror(UFFS_ERR_SERIOUS, PFX"Data verify fail at: %d\n", i);
			goto ext;
		}
	}

	ret = uffs_FlashReadPageSpare(dev, block, page, tag, NULL);
	if (UFFS_FLASH_HAVE_ERR(ret)) {
		uffs_Perror(UFFS_ERR_SERIOUS, PFX"Read tag (page spare) error: %d\n", ret);
		goto ext;
	}
	
	// verify tag:
	if (!TAG_IS_DIRTY(tag)) {
		uffs_Perror(UFFS_ERR_SERIOUS, PFX"not dirty ? Tag verify fail!\n");
		goto ext;
	}

	if (!TAG_IS_VALID(tag)) {
		uffs_Perror(UFFS_ERR_SERIOUS, PFX"not valid ? Tag verify fail!\n");
		goto ext;
	}

	if (TAG_DATA_LEN(tag) != dev->com.pg_data_size ||
		TAG_TYPE(tag) != UFFS_TYPE_DATA ||
		TAG_PAGE_ID(tag) != 3 ||
		TAG_PARENT(tag) != 100 ||
		TAG_SERIAL(tag) != 10 ||
		TAG_BLOCK_TS(tag) != 1) {

		uffs_Perror(UFFS_ERR_SERIOUS, PFX"Tag verify fail!\n");
		goto ext;
	}

	uffs_Perror(UFFS_ERR_SERIOUS, PFX"Page read/write test succ.\n");

ext:
	if (node) {
		uffs_FlashEraseBlock(dev, node->u.list.block);
		uffs_InsertToErasedListHead(dev, node);
	}

	if (dev)
		uffs_PutDevice(dev);

	return TRUE;
}

static struct cli_commandset cmdset[] = 
{
    { cmdTestPageReadWrite,	"t_pgrw",		NULL,		"test page read/write" },
    { NULL, NULL, NULL, NULL }
};


struct cli_commandset * get_test_cmds()
{
	return cmdset;
};