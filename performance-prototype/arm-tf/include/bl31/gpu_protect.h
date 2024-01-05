#ifndef __ASSEMBLER__

#ifndef GPU_PROTECT_H
#define GPU_PROTECT_H

#define CCAEXT_ALL U(0xf)
#define CCAEXT_ROOT U(0xa)
#define CCAEXT_REALM U(0xb)
#define CCAEXT_INVAL U(0x0)

#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/plat_arm.h>

int init_ccaext_periph_usb_gpt(void);
int init_ccaext_periph_hdlcd0_gpt(void);
int init_ccaext_periph_hdlcd1_gpt(void);
int init_ccaext_periph_dma_gpt(void);
int init_ccaext_periph_pcie_gpt(void);
int init_ccaext_periph_etr_gpt(void);
int init_ccaext_gpu_gpt(void);
int init_ccaext_cpu_gpt(void);
int init_ccaext_gpu_gpt_mempart(uint64_t startaddr, uint64_t gpupteaddr, uint64_t gpuptesize);

void gptconfig_enable_gpummio_protect(void);
void gptconfig_disable_gpummio_protect(void);

int find_mask_idx(int leftidx, int rightidx);
void gptconfig_page(uint64_t root_addr, uint64_t physaddr, uint32_t attr);
void gptconfig_sublevel(uint64_t subgpt_addr, uint64_t start_addr, uint64_t start_size, uint32_t target_attr);
void gptconfig_enable_metadata_protect(void);
void gptconfig_disable_metadata_protect(void);

void gptconfig_enable_gpu_gpt_protection(uint64_t gpu_gpt_mempart_startaddr);
void gptconfig_disable_gpu_gpt_protection(void);

void gptconfig_enable_fpgammio_protect(void);
void gptconfig_disable_fpgammio_protect(void);

#endif

#endif
