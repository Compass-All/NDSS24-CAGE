/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2016-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "xdma_cdev.h"
#include <linux/ioctl.h>

int stop_submit;

static int cage_open(struct inode * inode, struct file * filp)
{
	return 0;
}


static int cage_release(struct inode * inode, struct file *filp)
{
  return 0;
}


ssize_t cage_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    return 0;
}

ssize_t cage_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	cmd = _IOC_NR(cmd);
	uint64_t appid=arg;
	switch (cmd)
	{
		case 1:
			//enable protection
					asm volatile(
						"ldr x0,=0xc700000a\n"
						"ldr x1,=0x1\n"
						"smc #0\n"
						:::"x0","x1"
					);
			stop_submit=1;
			break;
		case 2:
			//disable protection
					asm volatile(
						"ldr x0,=0xc700000a\n"
						"ldr x1,=0x2\n"
						"smc #0\n"
						:::"x0","x1"
					);
			stop_submit=0;
			break;
	default:
		break;
	}
	return 0;	
}



static const struct file_operations cage_fops = {
	.owner = THIS_MODULE,
	.open = cage_open,
	.release = cage_release,
	.read = cage_read,
	.unlocked_ioctl = cage_ioctl,
};

void cdev_cage_init(struct xdma_cdev *xcdev)
{
	stop_submit=0;
	cdev_init(&xcdev->cdev, &cage_fops);
}
