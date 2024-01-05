/*
 * Copyright (c) 2014-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/el3_runtime/cpu_data.h>
#include <lib/pmf/pmf.h>
#include <lib/psci/psci.h>
#include <lib/runtime_instr.h>
#include <services/sdei.h>
#include <services/spm_svc.h>
#include <services/std_svc.h>
#include <smccc_helpers.h>
#include <tools_share/uuid.h>

#include <bl31/other_util.h>
#include <bl31/shadow_task.h>
#include <bl31/gpu_protect.h>
#include <lib/mmio.h>

uint32_t init_FPGA_H[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

/* Standard Service UUID */
static uuid_t arm_svc_uid = {
	{0x5b, 0x90, 0x8d, 0x10},
	{0x63, 0xf8},
	{0xe8, 0x47},
	0xae,
	0x2d,
	{0xc0, 0xfb, 0x56, 0x41, 0xf6, 0xe2}};

/* Setup Standard Services */
static int32_t std_svc_setup(void)
{
	uintptr_t svc_arg;
	int ret = 0;

	svc_arg = get_arm_std_svc_args(PSCI_FID_MASK);
	assert(svc_arg);

	/*
	 * PSCI is one of the specifications implemented as a Standard Service.
	 * The `psci_setup()` also does EL3 architectural setup.
	 */
	if (psci_setup((const psci_lib_args_t *)svc_arg) != PSCI_E_SUCCESS)
	{
		ret = 1;
	}

#if ENABLE_SPM
	if (spm_setup() != 0)
	{
		ret = 1;
	}
#endif

#if SDEI_SUPPORT
	/* SDEI initialisation */
	sdei_init();
#endif

	return ret;
}

/*
 * Top-level Standard Service SMC handler. This handler will in turn dispatch
 * calls to PSCI SMC handler
 */
static uintptr_t std_svc_smc_handler(uint32_t smc_fid,
									 u_register_t x1,
									 u_register_t x2,
									 u_register_t x3,
									 u_register_t x4,
									 void *cookie,
									 void *handle,
									 u_register_t flags)
{
	/*
	 * Dispatch PSCI calls to PSCI SMC handler and return its return
	 * value
	 */
	if (is_psci_fid(smc_fid))
	{
		uint64_t ret;

#if ENABLE_RUNTIME_INSTRUMENTATION

		/*
		 * Flush cache line so that even if CPU power down happens
		 * the timestamp update is reflected in memory.
		 */
		PMF_WRITE_TIMESTAMP(rt_instr_svc,
							RT_INSTR_ENTER_PSCI,
							PMF_CACHE_MAINT,
							get_cpu_data(cpu_data_pmf_ts[CPU_DATA_PMF_TS0_IDX]));
#endif

		ret = psci_smc_handler(smc_fid, x1, x2, x3, x4,
							   cookie, handle, flags);

#if ENABLE_RUNTIME_INSTRUMENTATION
		PMF_CAPTURE_TIMESTAMP(rt_instr_svc,
							  RT_INSTR_EXIT_PSCI,
							  PMF_NO_CACHE_MAINT);
#endif

		SMC_RET1(handle, ret);
	}

#if ENABLE_SPM && SPM_MM
	/*
	 * Dispatch SPM calls to SPM SMC handler and return its return
	 * value
	 */
	if (is_spm_fid(smc_fid))
	{
		return spm_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
							   handle, flags);
	}
#endif

#if SDEI_SUPPORT
	if (is_sdei_fid(smc_fid))
	{
		return sdei_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle,
								flags);
	}
#endif

	switch (smc_fid)
	{
	case ARM_STD_SVC_CALL_COUNT:
		/*
		 * Return the number of Standard Service Calls. PSCI is the only
		 * standard service implemented; so return number of PSCI calls
		 */
		SMC_RET1(handle, PSCI_NUM_CALLS);

	case ARM_STD_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, arm_svc_uid);

	case ARM_STD_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, STD_SVC_VERSION_MAJOR, STD_SVC_VERSION_MINOR);

	default:
		WARN("Unimplemented Standard Service Call: 0x%x \n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(
	std_svc,

	OEN_STD_START,
	OEN_STD_END,
	SMC_TYPE_FAST,
	std_svc_setup,
	std_svc_smc_handler);

static int32_t arm_arch_ccaext_init(void)
{
	NOTICE("CCA Extension is registered.\n");
	return 0;
}

static uintptr_t arm_arch_ccaext_smc_handler(uint32_t smc_fid,
											 u_register_t x1,
											 u_register_t x2,
											 u_register_t x3,
											 u_register_t x4,
											 void *cookie,
											 void *handle,
											 u_register_t flags)
{
	uint32_t filter_smc_value = smc_fid & 0xf;
	switch (filter_smc_value)
	{
	case 0x1:
	{

		/*
		 * This is used for converting a region as realm region and create the bind
		 * [input] x1: app_id
		 *
		 */
		uint64_t stub_app_id = x1;
		struct shadowtask *new_shadowtask = add_ccaext_shadowtask(stub_app_id);
		if (new_shadowtask == NULL)
		{
			ERROR("No available shadow task reserved\n");
			panic();
		}
		NOTICE("Shadow app recorded\n");

		SMC_RET1(handle, SMC_OK);
	}
	case 0x2:
	{
		/*
		 * This is used for telling kernel which region is available, and add it to the data info ahead
		 * Note: This service can be called in many times.
		 * [input] x1: app_id, x2: stub_addr, x3: set_size
		 * [output] x1: real_addr
		 */
		uint64_t stub_app_id = x1;
		uint64_t stub_addr = x2;
		uint64_t set_size = x3;
		if (set_size == 0)
		{
			ERROR("This set size is 0\n");
			panic();
		}
		uint64_t real_addr = find_available_realtask_region(set_size);
		if (real_addr == 0)
		{
			ERROR("No available region to allocate dataset.\n");
			panic();
		}
		record_each_dataset(stub_app_id, stub_addr, real_addr, set_size);
		NOTICE("In stub_app_id %llx\n", stub_app_id);
		NOTICE("Record stub_addr %llx, real_addr %llx, set_size %llx \n", stub_addr, real_addr, set_size);
		SMC_RET2(handle, SMC_OK, real_addr);
	}
	case 0x5:
	{

		// Shadow task mechanism, for unified-memory GPU.
		gptconfig_enable_metadata_protect();
		gptconfig_enable_gpummio_protect();
		change_gpu_interrupt_to_secure();

		uint64_t jc = x1;
		curtask_stub_mmu_pgd = x3;
		curtask_js = x2;
		uint64_t num_update_pte_sections = x4;
		uint64_t jc_phys = read_ttbr_core(jc, curtask_stub_mmu_pgd);

		while (1)
		{
			if (*(unsigned long *)jc_phys == jc)
				break;
			jc -= 8;
			jc_phys -= 8;
		}

		curtask_jc_phys = jc_phys;
		curtask_app_id = ccaext_check_buffer(num_update_pte_sections);

		if (curtask_app_id == 0)
		{
			ERROR("Maybe some non-confidential task here\n");
			panic();
		}

		submit_atom(x1); // x1:jc

		SMC_RET1(handle, SMC_OK);
	}
	case 0x6:
	{
		/*
		 * Protect the total dataset based on app_id
		 * [input] x1: app_id
		 *
		 */
		uint64_t stub_app_id = x1;
		protect_total_dataset(stub_app_id);
		SMC_RET1(handle, SMC_OK);
	}

	case 0x7:
	{

		/*
		 * Shadow task mechanism, for FPGA
		 * [input] x1: addr of this data descriptor, x2:h2c or c2h (0 is h2c, 1 is c2h)
		 *
		 */

		// Emulation of Hash verification
		uint32_t H[8];
		void *current_ptr = (void *)0xb0000000;
		fast_memcpy(H, init_FPGA_H, sizeof(H));
		sha256_final(H, current_ptr, 0x80, 0x80);

		// Analyze the descriptor.
		uint64_t fpgadesc_phys_addr = x1;
		uint64_t h2c_or_c2h = x2;

		// The data buffer description is small enough to be presented by only one xdma transfer descriptor
		uint64_t fpgainfo_phys_addr = 0;
		if (h2c_or_c2h == 0)
		{
			fpgainfo_phys_addr = *(uint64_t *)(fpgadesc_phys_addr + 0x8); // h2c, in src_adr
		}
		else
		{
			fpgainfo_phys_addr = *(uint64_t *)(fpgadesc_phys_addr + 0x10); // c2h, in dst_adr
		}

		uint64_t ccaext_magic_number = *(uint64_t *)(fpgainfo_phys_addr + 0x0);
		if (ccaext_magic_number != 0xaaaabbbbccccdddd)
		{
			ERROR("Not a confidential FPGA task\n");
			SMC_RET1(handle, SMC_OK);
		}

		uint64_t stub_app_id = 0;
		struct shadowtask *cur_shadowtask = NULL;
		uint64_t cur_app_id = *(uint64_t *)(fpgainfo_phys_addr + 0x8);
		if (stub_app_id == 0)
		{
			stub_app_id = cur_app_id;
			cur_shadowtask = find_ccaext_shadowtask(stub_app_id);
		}

		uint64_t fpga_datatransfer_stubdatabuffer_addr = *(uint64_t *)(fpgainfo_phys_addr + 0x18);
		uint64_t fpga_datatransfer_stubdatabuffer_size = *(uint64_t *)(fpgainfo_phys_addr + 0x20);

		if (fpga_datatransfer_stubdatabuffer_size == 0)
		{
			ERROR("Cannot transfer zero size\n");
			SMC_RET1(handle, SMC_OK);
		}

		uint64_t aligned_buffer_size = fpga_datatransfer_stubdatabuffer_size & 0xfffffffff000;
		if (fpga_datatransfer_stubdatabuffer_size & 0xfff)
			aligned_buffer_size += 0x1000;
		uint64_t databuffer_real_addr = get_databuffer_real_addr(cur_shadowtask, fpga_datatransfer_stubdatabuffer_addr);
		if (databuffer_real_addr == 0)
		{
			databuffer_real_addr = find_available_realtask_region(aligned_buffer_size);
			if (databuffer_real_addr == 0)
			{
				ERROR("No available region to allocate databuffer.\n");
				panic();
			}
			record_each_databuffer(stub_app_id, fpga_datatransfer_stubdatabuffer_addr, databuffer_real_addr, aligned_buffer_size);

			gptconfig_sublevel(0xa00a0000, databuffer_real_addr, aligned_buffer_size, CCAEXT_ROOT);
			gptconfig_sublevel(cur_shadowtask->gpu_gpt_mempart_startaddr, databuffer_real_addr, aligned_buffer_size, CCAEXT_ALL);
		}
		if (h2c_or_c2h == 0)
		{

			uint64_t fpga_datatransfer_stubdataset_addr = *(uint64_t *)(fpgainfo_phys_addr + 0x30);
			uint64_t fpga_datatransfer_stubdataset_size = *(uint64_t *)(fpgainfo_phys_addr + 0x38);

			uint64_t dataset_real_addr = get_dataset_real_addr(cur_shadowtask, fpga_datatransfer_stubdataset_addr);
			if (dataset_real_addr == 0)
			{
				ERROR("Cannot find the real_addr based on source %llx\n", fpga_datatransfer_stubdataset_addr);
				panic();
			}
			fast_memcpy_addr(databuffer_real_addr, dataset_real_addr, fpga_datatransfer_stubdataset_size);
		}

		// Based on the provided information, we create a new xdma transfer descriptor
		uint32_t total_transfer_num = 0;
		uint64_t cur_transfer_size = fpga_datatransfer_stubdatabuffer_size;
		while (cur_transfer_size != 0)
		{
			if (cur_transfer_size > 0x1000)
			{
				cur_transfer_size -= 0x1000;
			}
			else
			{
				cur_transfer_size = 0;
			}
			total_transfer_num += 1;
		}

		uint64_t not_aligned_new_transfer_size = 0x20 * total_transfer_num;
		uint64_t aligned_new_transfer_size = not_aligned_new_transfer_size & 0xfffffffff000;
		if (not_aligned_new_transfer_size & 0xfff)
			aligned_new_transfer_size += 0x1000;
		uint64_t new_transfer_addr = find_available_realtask_region(aligned_new_transfer_size); // TODO

		gptconfig_sublevel(0xa00a0000, new_transfer_addr, aligned_new_transfer_size, CCAEXT_ROOT);
		gptconfig_sublevel(cur_shadowtask->gpu_gpt_mempart_startaddr, new_transfer_addr, aligned_new_transfer_size, CCAEXT_ALL);

		uint32_t remain_transfer_num = total_transfer_num;

		uint32_t start_transfer_id = (total_transfer_num - 1) % (0x40);

		uint64_t cur_transfer_addr = new_transfer_addr;
		uint64_t transfer_size_offs = 0;
		if ((fpga_datatransfer_stubdatabuffer_size & 0xfff) == 0)
			transfer_size_offs = 0x1000;
		else
			transfer_size_offs = fpga_datatransfer_stubdatabuffer_size & 0xfff;
		uint64_t cur_fpga_addr = 0;
		uint64_t cur_transfer_buffer_addr = databuffer_real_addr;
		while (remain_transfer_num > 0)
		{
			if (start_transfer_id == 0x0)
			{
				start_transfer_id = 0x3f;
			}
			else
			{
				start_transfer_id -= 1;
			}
			remain_transfer_num -= 1;
			if (remain_transfer_num == 0)
			{
				*(uint64_t *)(cur_transfer_addr + 0x0) = ((transfer_size_offs) << 32) + 0xad4b0013;
				*(uint64_t *)(cur_transfer_addr + 0x18) = 0x0;
			}
			else
			{
				*(uint64_t *)(cur_transfer_addr + 0x0) = (((uint64_t)0x1000) << 32) + 0xad4b0000 + ((start_transfer_id) << 8);
				*(uint64_t *)(cur_transfer_addr + 0x18) = cur_transfer_addr + 0x20;
			}

			if (h2c_or_c2h == 0)
			{
				*(uint64_t *)(cur_transfer_addr + 0x8) = cur_transfer_buffer_addr; // src
				*(uint64_t *)(cur_transfer_addr + 0x10) = cur_fpga_addr;		   // dst
			}
			else
			{
				*(uint64_t *)(cur_transfer_addr + 0x8) = cur_fpga_addr;				// src
				*(uint64_t *)(cur_transfer_addr + 0x10) = cur_transfer_buffer_addr; // dst
			}
			cur_transfer_addr += 0x20;
			cur_transfer_buffer_addr += 0x1000;
			cur_fpga_addr += 0x1000;
		}

		flush_dcache_range(new_transfer_addr, not_aligned_new_transfer_size);

		uint32_t new_transfer_addr_low = (uint32_t)(new_transfer_addr & 0xffffffff);
		uint32_t new_transfer_addr_hi = (uint32_t)(new_transfer_addr >> 32);
		uint64_t fpga_datatransfer_channel = *(uint64_t *)(fpgainfo_phys_addr + 0x48);

		mmio_write_32(0x50100000 + fpga_datatransfer_channel, new_transfer_addr_low);
		mmio_write_32(0x50100000 + fpga_datatransfer_channel + 0x4, new_transfer_addr_hi);
		mmio_write_32(0x50100000 + fpga_datatransfer_channel + 0x8, (total_transfer_num - 1));

		// Submit to the correct channel.
		uint64_t expected_channel = (fpga_datatransfer_channel & 0xff00) - 0x4000 + 0x50100004;
		mmio_write_32(expected_channel, 0x0cf83e19);

		SMC_RET3(handle, SMC_OK, new_transfer_addr, (total_transfer_num - 1));
	}
	case 0xa:
	{

		// Handle the FPGA MMIO control requirements from the Host.

		if (x1 == 0x1)
		{
			gptconfig_enable_fpgammio_protect();
		}
		else if (x1 == 0x2)
		{
			gptconfig_disable_fpgammio_protect();
		}
		SMC_RET1(handle, SMC_OK);
	}
	case 0xb:
	{

		// Handle the FPGA channel shutdown requirements from the Host. x1 is control value, x2 is channel offset
		uint64_t fpga_control_addr = x2 + 0x50100000;
		uint32_t inputval = (uint32_t)x1;

		mmio_write_32(fpga_control_addr, inputval);
		SMC_RET1(handle, SMC_OK);
	}
	case 0x9:
	{
		/*
		 * This is used to destroy an application
		 * x1: stub app id
		 */

		uint64_t stub_app_id = x1;
		struct shadowtask *cur_shadowtask = find_ccaext_shadowtask(stub_app_id);
		destroy_ccaext_shadowtask(cur_shadowtask);
		NOTICE("App destroyed\n");

		SMC_RET1(handle, SMC_OK);
	}
	default:
		WARN("Unimplemented CCA Extension Call: 0x%x \n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

DECLARE_RT_SVC(
	cca_extension,
	OEN_CCAEXT_START,
	OEN_CCAEXT_END,
	SMC_TYPE_FAST,
	arm_arch_ccaext_init,
	arm_arch_ccaext_smc_handler);
