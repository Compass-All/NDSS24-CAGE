#ifndef __ASSEMBLER__


#ifndef GPU_PROTECT_H
#define GPU_PROTECT_H

#define CCAEXT_ALL U(0xf)
#define CCAEXT_ROOT U(0xa)
#define CCAEXT_REALM U(0xb)
#define CCAEXT_INVAL U(0x0)

#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/plat_arm.h>


int init_ccaext_periph_gpt(void);
int init_ccaext_cpu_gpt(void);
int init_ccaext_gpu_gpt_realm(uint64_t new_gpt_root, uint64_t realm_startaddr, uint64_t realm_size);

int find_mask_idx(int leftidx,int rightidx);
void gptconfig_page(uint64_t root_addr, uint64_t physaddr,uint32_t attr);
void gptconfig_sublevel(uint64_t subgpt_addr, uint64_t start_addr, uint64_t start_size, uint32_t target_attr);
int ccaext_gpt_enable(void);

void gptconfig_enable_gpu_gpt_protection(uint64_t gpu_gpt_realm);
void gptconfig_disable_gpu_gpt_protection(void);

void enable_gpummio_protect(void);
void disable_gpummio_protect(void);
void enable_code_metadata_protect(void);
void disable_code_metadata_protect(void);

void configure_smmuv3(void);
void tlb_wait(void);
#endif

#endif
