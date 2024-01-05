/*
 * Copyright (c) 2014-2022, ARM Limited and Contributors. All rights reserved.
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
#include <services/drtm_svc.h>
#include <services/pci_svc.h>
#include <services/rmmd_svc.h>
#include <services/sdei.h>
#include <services/spm_mm_svc.h>
#include <services/spmc_svc.h>
#include <services/spmd_svc.h>
#include <services/std_svc.h>
#include <services/trng_svc.h>
#include <smccc_helpers.h>
#include <tools_share/uuid.h>

#include <bl31/gpu_protect.h>
#include <bl31/shadow_task.h>
#include <bl31/other_util.h>
#include <lib/mmio.h>
#include <bl31/smmuv3_test_engine.h>
#include <drivers/arm/smmu_v3.h>

#define LOOP_COUNT (5000U)
#define USR_BASE_FRAME	ULL(0x2BFE0000)
#define PRIV_BASE_FRAME			ULL(0x2BFF0000)

/* The test engine supports numerous frames but we only use a few */
#define FRAME_COUNT	(2U)
#define FRAME_SIZE	(0x80U) /* 128 bytes */
#define F_IDX(n)	(n * FRAME_SIZE)

/* Commands supported by SMMUv3TestEngine built into the AEM */
#define ENGINE_NO_FRAME	(0U)
#define ENGINE_HALTED	(1U)

/*
 * ENGINE_MEMCPY: Read and Write transactions
 * ENGINE_RAND48: Only Write transactions: Source address not required
 * ENGINE_SUM64: Only read transactions: Target address not required
 */
#define ENGINE_MEMCPY	(2U)
#define ENGINE_RAND48	(3U)
#define ENGINE_SUM64	(4U)
#define ENGINE_ERROR	(0xFFFFFFFFU)
#define ENGINE_MIS_CFG	(ENGINE_ERROR - 1)

/*
 * Refer to:
 * https://developer.arm.com/documentation/100964/1111-00/Trace-components/SMMUv3TestEngine---trace
 */

/* Offset of various control fields belonging to User Frame */
#define CMD_OFF		(0x0U)
#define UCTRL_OFF	(0x4U)
#define SEED_OFF	(0x24U)
#define BEGIN_OFF	(0x28U)
#define END_CTRL_OFF	(0x30U)
#define STRIDE_OFF	(0x38U)
#define UDATA_OFF	(0x40U)

/* Offset of various control fields belonging to PRIV Frame */
#define PCTRL_OFF		(0x0U)
#define DOWNSTREAM_PORT_OFF	(0x4U)
#define STREAM_ID_OFF		(0x8U)
#define SUBSTREAM_ID_OFF	(0xCU)





/* Standard Service UUID */
static uuid_t arm_svc_uid = {
	{0x5b, 0x90, 0x8d, 0x10},
	{0x63, 0xf8},
	{0xe8, 0x47},
	0xae, 0x2d,
	{0xc0, 0xfb, 0x56, 0x41, 0xf6, 0xe2}
};

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
	if (psci_setup((const psci_lib_args_t *)svc_arg) != PSCI_E_SUCCESS) {
		ret = 1;
	}

#if SPM_MM
	if (spm_mm_setup() != 0) {
		ret = 1;
	}
#endif

#if defined(SPD_spmd)
	if (spmd_setup() != 0) {
		ret = 1;
	}
#endif

#if ENABLE_RME
	if (rmmd_setup() != 0) {
		ret = 1;
	}
#endif

#if SDEI_SUPPORT
	/* SDEI initialisation */
	sdei_init();
#endif

#if TRNG_SUPPORT
	/* TRNG initialisation */
	trng_setup();
#endif /* TRNG_SUPPORT */

#if DRTM_SUPPORT
	if (drtm_setup() != 0) {
		ret = 1;
	}
#endif /* DRTM_SUPPORT */

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
	if (((smc_fid >> FUNCID_CC_SHIFT) & FUNCID_CC_MASK) == SMC_32) {
		/* 32-bit SMC function, clear top parameter bits */

		x1 &= UINT32_MAX;
		x2 &= UINT32_MAX;
		x3 &= UINT32_MAX;
		x4 &= UINT32_MAX;
	}

	/*
	 * Dispatch PSCI calls to PSCI SMC handler and return its return
	 * value
	 */
	if (is_psci_fid(smc_fid)) {
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

#if SPM_MM
	/*
	 * Dispatch SPM calls to SPM SMC handler and return its return
	 * value
	 */
	if (is_spm_mm_fid(smc_fid)) {
		return spm_mm_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
					  handle, flags);
	}
#endif

#if defined(SPD_spmd)
	/*
	 * Dispatch FFA calls to the FFA SMC handler implemented by the SPM
	 * dispatcher and return its return value
	 */
	if (is_ffa_fid(smc_fid)) {
		return spmd_ffa_smc_handler(smc_fid, x1, x2, x3, x4, cookie,
					    handle, flags);
	}
#endif

#if SDEI_SUPPORT
	if (is_sdei_fid(smc_fid)) {
		return sdei_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle,
				flags);
	}
#endif

#if TRNG_SUPPORT
	if (is_trng_fid(smc_fid)) {
		return trng_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle,
				flags);
	}
#endif /* TRNG_SUPPORT */

#if ENABLE_RME

	if (is_rmmd_el3_fid(smc_fid)) {
		return rmmd_rmm_el3_handler(smc_fid, x1, x2, x3, x4, cookie,
					    handle, flags);
	}

	if (is_rmi_fid(smc_fid)) {
		return rmmd_rmi_handler(smc_fid, x1, x2, x3, x4, cookie,
					handle, flags);
	}
#endif

#if SMC_PCI_SUPPORT
	if (is_pci_fid(smc_fid)) {
		return pci_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle,
				       flags);
	}
#endif

#if DRTM_SUPPORT
	if (is_drtm_fid(smc_fid)) {
		return drtm_smc_handler(smc_fid, x1, x2, x3, x4, cookie, handle,
					flags);
	}
#endif /* DRTM_SUPPORT */

	switch (smc_fid) {
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
		VERBOSE("Unimplemented Standard Service Call: 0x%x \n", smc_fid);
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
		std_svc_smc_handler
);


static int32_t arm_arch_ccaext_init(void){
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
	NOTICE("In CCA Extension\n");
	uint32_t filter_smc_value=smc_fid&0xf;
	switch (filter_smc_value){
		case 0x1:{

			/*
			* This is used for create a realm. User will request a realm.
			* [input] x1: realm_id
			* 
			*/
			uint64_t realm_id=x1;
			//Find an available shadowtask region, record this bind
			struct shadowtask *new_shadowtask=allocate_realm_with_realmid(realm_id);
			if (new_shadowtask==NULL){
				ERROR("No available realm reserved\n");
				panic();
			}
			NOTICE("Realm is created, id: %lx\n",realm_id);
			SMC_RET1(handle, SMC_OK);
		}
		case 0x2:{
			/*
			* This is used for transfer data to the realm. This API will confirm the start addr, so that the kernel can transfer data into this realm.
			* Since we initially configure the region as REALM, we should configure it as ALL to allow data transfer
			* Note: This service can be called in many times.
			* [input] x1: realm_id, x2: stub_addr, x3: set_size
			* [output] x1: real_addr
			*/
			uint64_t realm_id=x1;
			uint64_t stub_addr=x2;
			uint64_t set_size=x3;

			uint64_t real_addr=find_available_realm_region(realm_id, set_size);
			if(real_addr==0){
				ERROR("No available region to allocate dataset.\n");
				panic();
			}
			record_each_dataset(realm_id,stub_addr,real_addr,set_size);
			NOTICE("In realm_id %lx\n",realm_id);
			NOTICE("Record stub_addr %lx, real_addr %lx, set_size %lx \n",stub_addr,real_addr,set_size);

			//Change the CPU GPT as ALL
            gptconfig_sublevel(0xa00a0000,real_addr,set_size-1,CCAEXT_ALL);

			SMC_RET2(handle, SMC_OK,real_addr);
		}
		case 0x6:{
			/*
			* Invoke this API to protect the entire dataset.
			* [input] x1: realm_id
			* 
			*/
			uint64_t realm_id=x1;
			protect_total_dataset(realm_id);
			NOTICE("Protect all dataset for realm %lx\n",realm_id);
			SMC_RET1(handle, SMC_OK);
		}
		case 0x8:{
			/*
			* CAGE's functionality verification
			* [input] x1: realm_id, x2: metadata_addr, x3: ttbr, x4: test scenario (correct is 0x0, incorrect is 0x1 -- 0x3)
			* 
			*/

			NOTICE("Test scenario: %lx\n",x4);


			curtask_realm_id=x1;
			struct shadowtask *cur_shadowtask=find_realm_with_realmid(curtask_realm_id);
			if(cur_shadowtask==NULL){
				NOTICE("Cannot find realm region %lx\n",curtask_realm_id);
				panic();
			}
			NOTICE("Start processing stub task for realm region %lx\n",curtask_realm_id);
			
			curtask_metadata_addr=x2;

			ccaext_check_buffer(curtask_realm_id);

			NOTICE("Code, metadata, GPUMMIO Protection\n");

			enable_code_metadata_protect();
			enable_gpummio_protect();

			NOTICE("Ready to use test-engine to access data\n");

			uint64_t ttbr_addr=(mmio_read_64((uintptr_t)((uint8_t *)0x2b400000 + 0x8080)) & 0xfffffffff)+0x30;
			uint64_t stub_gpupte_ttbr=*(uint64_t *)ttbr_addr;
			uint64_t ttbrreq_addr=x3;
			uint64_t ttbrreq_realm_id=*(uint64_t *)(ttbrreq_addr+0x0);
			uint64_t ttbrreq_cnt=*(uint64_t *)(ttbrreq_addr+0x8);
			if(ttbrreq_realm_id!=curtask_realm_id){
				ERROR("Find a false TTBR request\n");
				panic();
			}
			uint64_t cur_req=0;
			while(cur_req<ttbrreq_cnt){
				uint64_t cur_req_addr=ttbrreq_addr+0x10+cur_req*0x20;

				uint64_t req_start_ipa=*(uint64_t*) (cur_req_addr+0x0);
				uint64_t req_size=*(uint64_t*) (cur_req_addr+0x8);
				uint64_t req_start_pa=*(uint64_t*) (cur_req_addr+0x10);
				uint64_t req_attr=*(uint64_t*) (cur_req_addr+0x18);

				if ((req_attr&0xfff)==0x7ff){
					uint64_t new_addr=get_databuffer_real_addr(cur_shadowtask,req_start_ipa);
					req_start_ipa=new_addr;
					req_start_pa=new_addr;
				}

				NOTICE("Map ipa %lx with size %lx to pa %lx with attribute %lx\n",req_start_ipa,req_size,req_start_pa,req_attr);
				update_gpu_table(cur_shadowtask,req_start_ipa,req_size,req_start_pa,req_attr);

				cur_req+=1;
			}

			if (mmio_read_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF)) == ENGINE_ERROR) {
				ERROR("SMMUv3TestEngine: Error\n");
				panic();
			}

			*(uint64_t *)ttbr_addr=cur_shadowtask->real_gpupte_ttbr;
			*(uint64_t *)(ttbr_addr+0x40)=cur_shadowtask->real_gpupte_ttbr;
			*(uint64_t *)(ttbr_addr-0x18)=cur_shadowtask->real_gpupte_ttbr;
			*(uint64_t *)(ttbr_addr-0x18+0x40)=cur_shadowtask->real_gpupte_ttbr;

			mmio_write_32((uintptr_t)((uint8_t *)0x2b400000 + 0x803c), 0x1);
			
			while(1){
				if (mmio_read_32((uintptr_t)((uint8_t *)0x2b400000 + 0x803c)) == 0x1) {
					ERROR("SMMUv3TestEngine: Still running\n");
				}
				else{
					break;
				}
			}

			isb();
			dsbsy();

			mmio_write_32((uintptr_t)((uint8_t *)PRIV_BASE_FRAME + PCTRL_OFF), 0);
			mmio_write_32((uintptr_t)((uint8_t *)PRIV_BASE_FRAME + DOWNSTREAM_PORT_OFF), 0);
			mmio_write_32((uintptr_t)((uint8_t *)PRIV_BASE_FRAME + STREAM_ID_OFF), 0);
			mmio_write_32((uintptr_t)((uint8_t *)PRIV_BASE_FRAME + SUBSTREAM_ID_OFF), NO_SUBSTREAMID);

			mmio_write_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + UCTRL_OFF), 0);
			mmio_write_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + SEED_OFF), 0);
			
			//Access metadata region
			
			if(x4==0x2){
				NOTICE("CPU send Non-secure access to the protected (currently configured as Realm) metadata region\n");
				uint64_t metadata_result=*(uint64_t*)curtask_metadata_addr;
				ERROR("Unreachable Result: %lx\n",metadata_result);
				panic();
			}

			if((x4!=0x1)&&(x4!=0x2)&&(x4!=0x3)){
				NOTICE("Test engine accesses the metadata region %lx\n",curtask_metadata_addr);

				// Since the test engine can only perform memcpy, and metadata region is read-only
				// we temporarily map another region to store the metadata read by the test engine
				update_gpu_table(cur_shadowtask,0x8a03ff000,0x1000,0x8a03ff000,0x01c00000000007ff);

				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + BEGIN_OFF), curtask_metadata_addr);
				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + END_CTRL_OFF), (curtask_metadata_addr+0x100-1));
				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + STRIDE_OFF), 1);
				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + UDATA_OFF), (0x8a03ff000));

				mmio_write_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF), ENGINE_MEMCPY);

				NOTICE("SMMUv3TestEngine: Waiting for MEMCPY completion\n");

				if (mmio_read_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF)) == ENGINE_MIS_CFG) {
					ERROR("SMMUv3TestEngine: Misconfigured\n");
					panic();
				}

				/* Wait for mem copy to be complete */
				uint64_t attempts=0;
				uint32_t status;
				while (attempts++ < LOOP_COUNT) {
					status = mmio_read_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF));
					if (status == ENGINE_HALTED) break;
					else if (status == ENGINE_ERROR) { 
						ERROR("SMMUv3: Test failed\n");
						panic();
					}
				}

				if (attempts == LOOP_COUNT) {
					ERROR("SMMUv3: Test timeout\n");
					panic();
				}

				// Next we copy data in input buffer to output buffer.
				// Note that the real-world GPU can distinguish the input and output buffers

				uint64_t curtask_inputbuffer=*(uint64_t*)(0x8a03ff000+0x0);
				uint64_t curtask_outputbuffer=*(uint64_t*)(0x8a03ff000+0x8);
				uint64_t curtask_buffer_size=curtask_outputbuffer-curtask_inputbuffer;

				NOTICE("Next we copy data in input buffer %lx to output buffer %lx, size %lx.\n",curtask_inputbuffer,curtask_outputbuffer,curtask_buffer_size);
				NOTICE("Note that the real-world GPU can distinguish the input and output buffers.\n");


				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + BEGIN_OFF), curtask_inputbuffer);
				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + END_CTRL_OFF), (curtask_inputbuffer+curtask_buffer_size-1));
				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + STRIDE_OFF), 1);
				mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + UDATA_OFF), (curtask_outputbuffer));

				mmio_write_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF), ENGINE_MEMCPY);

				NOTICE("SMMUv3TestEngine: Waiting for MEMCPY completion\n");

				if (mmio_read_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF)) == ENGINE_MIS_CFG) {
					ERROR("SMMUv3TestEngine: Misconfigured\n");
					panic();
				}

				/* Wait for mem copy to be complete */
				attempts=0;
				while (attempts++ < LOOP_COUNT) {
					status = mmio_read_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF));
					if (status == ENGINE_HALTED) break;
					else if (status == ENGINE_ERROR) { 
						ERROR("SMMUv3: Test failed\n");
						panic();
					}
				}

				if (attempts == LOOP_COUNT) {
					ERROR("SMMUv3: Test timeout\n");
					panic();
				}

				NOTICE("Verify the result\n");
				uint64_t tmp_cnt=0;
				while(tmp_cnt<curtask_buffer_size){
					uint64_t curtask_input_value=*(uint64_t*)(curtask_inputbuffer+tmp_cnt);
					uint64_t curtask_output_value=*(uint64_t*)(curtask_outputbuffer+tmp_cnt);
					if(curtask_input_value!=curtask_output_value){
						ERROR("Find value in addr %lx does not match\n", (curtask_inputbuffer+tmp_cnt));
						panic();
					}
					tmp_cnt+=0x8;
				}

			}
			else{
				if(x4==0x1){
					uint64_t unauthorized_metadata_addr=curtask_metadata_addr+0x10000;
					NOTICE("Test engine accesses unauthorized region %lx\n",unauthorized_metadata_addr);
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + BEGIN_OFF), unauthorized_metadata_addr);
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + END_CTRL_OFF), (unauthorized_metadata_addr+0x100-1));
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + STRIDE_OFF), 1);
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + UDATA_OFF), (unauthorized_metadata_addr+0x100));
				}
				if(x4==0x3){
					NOTICE("Test engine writes to authorized but read-only region %lx\n",curtask_metadata_addr);
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + BEGIN_OFF), curtask_metadata_addr);
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + END_CTRL_OFF), (curtask_metadata_addr+0x100-1));
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + STRIDE_OFF), 1);
					mmio_write_64((uintptr_t)((uint8_t *)USR_BASE_FRAME + UDATA_OFF), (curtask_metadata_addr+0x100));					
				}

				mmio_write_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF), ENGINE_MEMCPY);

				NOTICE("SMMUv3TestEngine: Waiting for MEMCPY completion\n");

				if (mmio_read_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF)) == ENGINE_MIS_CFG) {
					ERROR("SMMUv3TestEngine: Misconfigured\n");
					panic();
				}

				/* Wait for mem copy to be complete */
				uint64_t attempts=0;
				uint32_t status;
				while (attempts++ < LOOP_COUNT) {
					status = mmio_read_32((uintptr_t)((uint8_t *)USR_BASE_FRAME + CMD_OFF));
					if (status == ENGINE_HALTED) break;
					else if (status == ENGINE_ERROR) { 
						ERROR("SMMUv3: Test failed\n");
						panic();
					}
				}

				if (attempts == LOOP_COUNT) {
					ERROR("SMMUv3: Test timeout\n");
					panic();
				}

				ERROR("Unreachable\n");
				panic();
			}

			dsbsy();

			NOTICE("Processing finished, terminate the environment\n");
			
			disable_code_metadata_protect();
			disable_gpummio_protect();
			
			*(uint64_t *)ttbr_addr=stub_gpupte_ttbr;

			curtask_realm_id=0;
			curtask_metadata_addr=0;

			uint64_t tmp_idx=0;
			while(tmp_idx<10){
				curtask_codebuffer_addr[tmp_idx]=0;
				curtask_codebuffer_size[tmp_idx]=0;
				tmp_idx+=1;
			}
			NOTICE("Return to Host\n");


			SMC_RET1(handle, SMC_OK);
		}



		case 0x9:{
			/*
			* This is used to destroy a realm, including its (1) task and (2) table
			* x1: stub app id
			*/
			uint64_t realm_id=x1;
			struct shadowtask *cur_shadowtask=find_realm_with_realmid(realm_id);
			destroy_realm(cur_shadowtask);	
			NOTICE("Realm %lx Destroyed\n",realm_id);

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
		arm_arch_ccaext_smc_handler
);
