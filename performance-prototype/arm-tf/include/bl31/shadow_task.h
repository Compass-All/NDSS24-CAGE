#ifndef __ASSEMBLER__

#ifndef SHADOW_TASK_H
#define SHADOW_TASK_H

#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/plat_arm.h>

#define CCAEXT_SHADOWTASK_MAX 0x1
#define CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX 0x100
#define CCAEXT_SHADOWTASK_MAX_TOTALDATABUFFER 170
#define CCAEXT_SHADOWTASK_MAX_TOTALDATASET 170

#define CCAEXT_STUBTASK_MEMORY_STARTADDR ULL(0x880000000)
#define CCAEXT_STUBTASK_TABLE_STARTADDR ULL(0x890000000)
#define CCAEXT_REALTASK_MEMORY_STARTADDR ULL(0x8a0000000)
#define CCAEXT_REALTASK_TABLE_STARTADDR ULL(0x8b0000000)

#define BITMAP_REALTASK_ADDR(phys_addr) ((((phys_addr & 0xffffff000) - CCAEXT_REALTASK_MEMORY_STARTADDR) >> 9) + 0xa0f00000)
#define bitmap_realtask_config(phys_addr, bit) *(uint64_t *)BITMAP_REALTASK_ADDR(phys_addr) = bit
#define bitmap_realtask_check(phys_addr, bit) (*(uint64_t *)BITMAP_REALTASK_ADDR(phys_addr) == bit)

uint64_t curtask_app_id;
uint64_t curtask_jc_phys;
uint64_t curtask_stub_mmu_pgd;
int curtask_js;

struct dataset
{
    bool isused;
    uint64_t stub_addr;
    uint64_t real_addr;
    uint64_t set_size;
};

struct shadowtask
{
    bool is_available;
    uint64_t stub_app_id;
    uint64_t stub_mmu_pgd;
    uint64_t real_mmu_pgd;
    uint64_t real_gpupte_startaddr;
    uint64_t real_gpupte_size;
    struct dataset total_databuffer[CCAEXT_SHADOWTASK_MAX_TOTALDATABUFFER];
    struct dataset total_dataset[CCAEXT_SHADOWTASK_MAX_TOTALDATASET];
    bool bitmap_real_gpupte_page[CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX];
    uint64_t gpu_gpt_mempart_startaddr;
};

extern struct shadowtask shadowtask_total[CCAEXT_SHADOWTASK_MAX];

bool check_and_tag_bitmap_realtask(uint64_t startaddr, uint64_t required_size, uint64_t bit);
uint64_t find_available_realtask_region(uint64_t size);
void init_bitmap_realtask_page(void);
void init_ccaext_shadowtask(void);
struct shadowtask *add_ccaext_shadowtask(uint64_t stub_app_id);
struct shadowtask *find_ccaext_shadowtask(uint64_t stub_app_id);
void destroy_ccaext_shadowtask(struct shadowtask *cur_shadowtask);
uint64_t ccaext_check_buffer(uint64_t num_update_pte_sections);
void record_each_dataset(uint64_t stub_app_id, uint64_t stub_addr, uint64_t real_addr, uint64_t set_size);
void record_each_databuffer(uint64_t stub_app_id, uint64_t stub_addr, uint64_t real_addr, uint64_t set_size);
void protect_total_dataset(uint64_t stub_app_id);
uint64_t get_dataset_real_addr(struct shadowtask *cur_shadowtask, uint64_t stub_addr);
uint64_t get_databuffer_real_addr(struct shadowtask *cur_shadowtask, uint64_t stub_addr);
void datacopy_real_to_stubbuffer(uint64_t src_phys_addr, uint64_t dst_virt_addr, size_t total_size);
void init_gpu_stubtable_structure(void);
void configure_vapa_stub_databuffer(uint64_t va, uint64_t size, uint64_t this_mmu_pgd);
void restore_vapa_stub_databuffer(uint64_t va, uint64_t size, uint64_t this_mmu_pgd);
uint64_t get_databuffer_stub_addr(struct shadowtask *cur_shadowtask, uint64_t real_addr);

void reconstuct_gpupte(struct shadowtask *cur_shadowtask, uint64_t num_update_pte_sections, bool is_init);

void insert_real_gpupte_entries(struct shadowtask *cur_shadowtask, uint64_t start_va, uint64_t num_entries, uint64_t pa_desc_startaddr, bool is_real_databuffer);
void configure_real_gpupte_page_used(struct shadowtask *cur_shadowtask, uint64_t cur_addr);
uint32_t find_available_real_gpupte_page(struct shadowtask *cur_shadowtask);

void sha256(uint32_t *ctx, const void *in, size_t size);
void sha256_final(uint32_t *ctx, const void *in, size_t remain_size, size_t tot_size);
void sha256_block_data_order(uint32_t *ctx, const void *in, size_t num);
void hmac_sha256(void *out, const void *in, size_t size);
int check_code_integrity();
void init_gpu_realtable_region(void);

#endif
#endif
