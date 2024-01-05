/*
 * Copyright (c) 2015-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform_def.h>
#include <plat/arm/common/plat_arm.h>

/*
 * Table of memory regions for different BL stages to map using the MMU.
 * This doesn't include Trusted SRAM as setup_page_tables() already takes care
 * of mapping it.
 */
#ifdef IMAGE_BL1
const mmap_region_t plat_arm_mmap[] = {
	ARM_MAP_SHARED_RAM,
	V2M_MAP_FLASH0_RW,
	V2M_MAP_IOFPGA,
	CSS_MAP_DEVICE,
	SOC_CSS_MAP_DEVICE,
#if TRUSTED_BOARD_BOOT
	/* Map DRAM to authenticate NS_BL2U image. */
	ARM_MAP_NS_DRAM1,
#endif
	{0}
};
#endif
#ifdef IMAGE_BL2
const mmap_region_t plat_arm_mmap[] = {
	ARM_MAP_SHARED_RAM,
	V2M_MAP_FLASH0_RW,
#ifdef PLAT_ARM_MEM_PROT_ADDR
	ARM_V2M_MAP_MEM_PROTECT,
#endif
	V2M_MAP_IOFPGA,
	CSS_MAP_DEVICE,
	SOC_CSS_MAP_DEVICE,
	ARM_MAP_NS_DRAM1,
#ifdef __aarch64__
	ARM_MAP_DRAM2,
#endif
#ifdef SPD_tspd
	ARM_MAP_TSP_SEC_MEM,
#endif
#ifdef SPD_opteed
	ARM_MAP_OPTEE_CORE_MEM,
	ARM_OPTEE_PAGEABLE_LOAD_MEM,
#endif
#if TRUSTED_BOARD_BOOT && !BL2_AT_EL3
	ARM_MAP_BL1_RW,
#endif
	{0}
};
#endif
#ifdef IMAGE_BL2U
const mmap_region_t plat_arm_mmap[] = {
	ARM_MAP_SHARED_RAM,
	CSS_MAP_DEVICE,
	CSS_MAP_SCP_BL2U,
	V2M_MAP_IOFPGA,
	SOC_CSS_MAP_DEVICE,
	{0}
};
#endif
#ifdef IMAGE_BL31
const mmap_region_t plat_arm_mmap[] = {
	ARM_MAP_SHARED_RAM,
	V2M_MAP_IOFPGA,
	CSS_MAP_DEVICE,
#ifdef PLAT_ARM_MEM_PROT_ADDR
	ARM_V2M_MAP_MEM_PROTECT,
#endif
	SOC_CSS_MAP_DEVICE,
	MAP_REGION_FLAT(0xa0000000,(0xa2000000-0xa0000000),MT_MEMORY | MT_RW |MT_NS), //GPT
	MAP_REGION_FLAT(0xb0000000,(0xb4000000-0xb0000000),MT_MEMORY | MT_RW |MT_NS), 

	MAP_REGION_FLAT(0x900000000,0x100000000,MT_MEMORY | MT_RW |MT_NS), //Other DRAM	

	MAP_REGION_FLAT(0x2b400000,0x10000,MT_DEVICE | MT_RW |MT_SECURE), 
	MAP_REGION_FLAT(0xf8000000,0x6000000,MT_MEMORY | MT_RW|MT_NS), 
	MAP_REGION_FLAT(0x880000000,0x10000000,MT_MEMORY | MT_RW |MT_NS), //Stub Task
	MAP_REGION_FLAT(0x890000000,0x10000000,MT_MEMORY | MT_RW |MT_NS), //Stub GPU page table
	MAP_REGION_FLAT(0x8a0000000,0x10000000,MT_MEMORY | MT_RW |MT_NS), //Real Task
	MAP_REGION_FLAT(0x8b0000000,0x10000000,MT_MEMORY | MT_RW |MT_NS), //Real GPU page table
	MAP_REGION_FLAT(0x9b0000000,0x050000000,MT_MEMORY | MT_RW|MT_NS), 

	{0}
};
#endif
#ifdef IMAGE_BL32
const mmap_region_t plat_arm_mmap[] = {
#ifndef __aarch64__
	ARM_MAP_SHARED_RAM,
#ifdef PLAT_ARM_MEM_PROT_ADDR
	ARM_V2M_MAP_MEM_PROTECT,
#endif
#endif
	V2M_MAP_IOFPGA,
	CSS_MAP_DEVICE,
	SOC_CSS_MAP_DEVICE,
	{0}
};
#endif

ARM_CASSERT_MMAP
