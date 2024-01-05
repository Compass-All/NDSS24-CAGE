/*
 * Copyright (c) 2017-2022, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <cdefs.h>
#include <drivers/arm/smmu_v3.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <arch_features.h>

/* SMMU poll number of retries */
#define SMMU_POLL_TIMEOUT_US	U(1000)

static int smmuv3_poll(uintptr_t smmu_reg, uint32_t mask,
				uint32_t value)
{
	uint32_t reg_val;
	uint64_t timeout;

	/* Set 1ms timeout value */
	timeout = timeout_init_us(SMMU_POLL_TIMEOUT_US);
	do {
		reg_val = mmio_read_32(smmu_reg);
		if ((reg_val & mask) == (value&mask))
			return 0;
	} while (!timeout_elapsed(timeout));

	ERROR("Timeout polling SMMUv3 register @%p\n", (void *)smmu_reg);
	ERROR("Read value 0x%x, expected 0x%x\n", reg_val,
		value == 0U ? reg_val & ~mask : reg_val | mask);
	return -1;
}

/*
 * Abort all incoming transactions in order to implement a default
 * deny policy on reset.
 */
int __init smmuv3_security_init(uintptr_t smmu_base)
{
	/* Attribute update has completed when SMMU_(S)_GBPA.Update bit is 0 */
	if (smmuv3_poll(smmu_base + SMMU_GBPA, SMMU_GBPA_UPDATE, 0U) != 0U)
		return -1;

	/*
	 * SMMU_(S)_CR0 resets to zero with all streams bypassing the SMMU,
	 * so just abort all incoming transactions.
	 */
	mmio_setbits_32(smmu_base + SMMU_GBPA,
			SMMU_GBPA_UPDATE | SMMU_GBPA_ABORT);

	if (smmuv3_poll(smmu_base + SMMU_GBPA, SMMU_GBPA_UPDATE, 0U) != 0U)
		return -1;

	/* Check if the SMMU supports secure state */
	if ((mmio_read_32(smmu_base + SMMU_S_IDR1) &
				SMMU_S_IDR1_SECURE_IMPL) == 0U)
		return 0;

	/* Abort all incoming secure transactions */
	if (smmuv3_poll(smmu_base + SMMU_S_GBPA, SMMU_S_GBPA_UPDATE, 0U) != 0U)
		return -1;

	mmio_setbits_32(smmu_base + SMMU_S_GBPA, SMMU_S_GBPA_UPDATE | SMMU_S_GBPA_ABORT);

	return smmuv3_poll(smmu_base + SMMU_S_GBPA, SMMU_S_GBPA_UPDATE, 0U);
}

/*
 * Initialize the SMMU by invalidating all secure caches and TLBs.
 * Abort all incoming transactions in order to implement a default
 * deny policy on reset
 */
int __init smmuv3_init(uintptr_t smmu_base,uint64_t gptbr)
{
	/* Abort all incoming transactions */
	if (smmuv3_security_init(smmu_base) != 0)
		return -1;

#if ENABLE_RME

	if (get_armv9_2_feat_rme_support() != 0U) {
		if ((mmio_read_32(smmu_base + SMMU_ROOT_IDR0) &
				  SMMU_ROOT_IDR0_ROOT_IMPL) == 0U) {
			WARN("Skip SMMU GPC configuration.\n");
		} else {
		
		
			

			//CCAEXT: Config SMMU_ROOT_GPT_BASE_CFG, 
			
			uint64_t gpccr_el3 = 0x13501;
			gpccr_el3 &= ~(1UL << 16);
			mmio_write_32(smmu_base + SMMU_ROOT_GPT_BASE_CFG,gpccr_el3);

			mmio_write_64(smmu_base + SMMU_ROOT_GPT_BASE,gptbr);

			/*
			 * ACCESSEN=1: SMMU- and client-originated accesses are
			 *             not terminated by this mechanism.
			 * GPCEN=1: All clients and SMMU-originated accesses,
			 *          except GPT-walks, are subject to GPC.
			 */
			mmio_setbits_32(smmu_base + SMMU_ROOT_CR0, SMMU_ROOT_CR0_GPCEN | SMMU_ROOT_CR0_ACCESSEN);

			/* Poll for ACCESSEN and GPCEN ack bits. */
			if (smmuv3_poll(smmu_base + SMMU_ROOT_CR0ACK,
					SMMU_ROOT_CR0_GPCEN |
					SMMU_ROOT_CR0_ACCESSEN,
					SMMU_ROOT_CR0_GPCEN |
					SMMU_ROOT_CR0_ACCESSEN) != 0) {
				WARN("Failed enabling SMMU GPC.\n");

				/*
				 * Do not return in error, but fall back to
				 * invalidating all entries through the secure
				 * register file.
				 */
			}
		}
	}

#endif /* ENABLE_RME */

	/*
	 * Initiate invalidation of secure caches and TLBs if the SMMU
	 * supports secure state. If not, it's implementation defined
	 * as to how SMMU_S_INIT register is accessed.
	 * Arm SMMU Arch RME supplement, section 3.4: all SMMU registers
	 * specified to be accessible only in secure physical address space are
	 * additionally accessible in root physical address space in an SMMU
	 * with RME.
	 * Section 3.3: as GPT information is permitted to be cached in a TLB,
	 * the SMMU_S_INIT.INV_ALL mechanism also invalidates GPT information
	 * cached in TLBs.
	 */
	mmio_write_32(smmu_base + SMMU_S_INIT, SMMU_S_INIT_INV_ALL);

	/* Wait for global invalidation operation to finish */
	return smmuv3_poll(smmu_base + SMMU_S_INIT,
				SMMU_S_INIT_INV_ALL, 0U);
}

int smmuv3_ns_set_abort_all(uintptr_t smmu_base)
{
	/* Attribute update has completed when SMMU_GBPA.Update bit is 0 */
	if (smmuv3_poll(smmu_base + SMMU_GBPA, SMMU_GBPA_UPDATE, 0U) != 0U) {
		return -1;
	}

	/*
	 * Set GBPA's ABORT bit. Other GBPA fields are presumably ignored then,
	 * so simply preserve their value.
	 */
	mmio_setbits_32(smmu_base + SMMU_GBPA, SMMU_GBPA_UPDATE | SMMU_GBPA_ABORT);
	if (smmuv3_poll(smmu_base + SMMU_GBPA, SMMU_GBPA_UPDATE, 0U) != 0U) {
		return -1;
	}

	/* Disable the SMMU to engage the GBPA fields previously configured. */
	mmio_clrbits_32(smmu_base + SMMU_CR0, SMMU_CR0_SMMUEN);
	if (smmuv3_poll(smmu_base + SMMU_CR0ACK, SMMU_CR0_SMMUEN, 0U) != 0U) {
		return -1;
	}

	return 0;
}

bool smmuv3_reset_s(uintptr_t smmu_base){
	
	/* Configure the behaviour of SMMU when disabled */
	smmuv3_disabled_translation(smmu_base);

	/*
	 * Disable SMMU using SMMU_(S)_CR0.SMMUEN bit. This is necessary
	 * for driver to configure SMMU.
	 */
	uint32_t data_cr0;
	uint32_t offset_cr0 = S_CR0;
	uint32_t offset_cr0_ack = S_CR0_ACK;

	INFO("SMMUv3: write to (S_)CR0\n");
	INFO("prev offset_cr0: %x\n",mmio_read_32((uintptr_t)((uint8_t *)smmu_base + CR0)));

	data_cr0 = mmio_read_32((uintptr_t)((uint8_t *)smmu_base + offset_cr0));
	data_cr0 = data_cr0 & SMMUEN_CLR_MASK;
	mmio_write_32((uintptr_t)((uint8_t *)smmu_base + offset_cr0), data_cr0);
	
	INFO("offset_cr0: %x\n",mmio_read_32((uintptr_t)((uint8_t *)smmu_base + CR0)));
	if (!smmuv3_poll(smmu_base+offset_cr0_ack, data_cr0,
			 SMMUEN_MASK)) {
		ERROR("SMMUv3: Failed to disable SMMU\n");
		return false;
	}

	return true;
}

void smmuv3_disabled_translation(uintptr_t smmu_base)
{
	uint32_t gbpa_reg;
	uint32_t gbpa_update_set;
	uint32_t offset;

	gbpa_reg = COMPOSE(BYPASS_GBPA, ABORT_SHIFT, ABORT_MASK);
	gbpa_reg =
		gbpa_reg | COMPOSE(INCOMING_CFG, INSTCFG_SHIFT, INSTCFG_MASK);
	gbpa_reg =
		gbpa_reg | COMPOSE(INCOMING_CFG, PRIVCFG_SHIFT, PRIVCFG_MASK);
	gbpa_reg = gbpa_reg | COMPOSE(INCOMING_CFG, SHCFG_SHIFT, SHCFG_MASK);
	gbpa_reg =
		gbpa_reg | COMPOSE(INCOMING_CFG, ALLOCFG_SHIFT, ALLOCFG_MASK);
	gbpa_reg = gbpa_reg | COMPOSE(INCOMING_CFG, MTCFG_SHIFT, MTCFG_MASK);
	gbpa_update_set = (1U << UPDATE_SHIFT);

	offset = S_GBPA;
	INFO("SMMUv3: write to (S_)GBPA, value %x\n",(gbpa_reg | gbpa_update_set));
	mmio_write_32((uintptr_t)((uint8_t *)smmu_base + S_GBPA), (gbpa_reg | gbpa_update_set));

	// if (!smmuv3_poll(smmu_base + offset, SMMU_S_GBPA_UPDATE, 1U)) {
	// 	ERROR("SMMUv3: Failed to update Gloal Bypass Attribute\n");
	// 	panic();
	// }
	INFO("%x\n",mmio_read_32((uintptr_t)((uint8_t *)smmu_base + offset)));

}