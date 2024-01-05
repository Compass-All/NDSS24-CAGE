#ifndef __ASSEMBLER__

#ifndef SHADOW_TASK_H
#define SHADOW_TASK_H

#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/plat_arm.h>

#define CCAEXT_SHADOWTASK_MAX 0x2
#define CCAEXT_REALM_SIZE 0x00400000

extern uint64_t curtask_realm_id;
extern uint64_t curtask_metadata_addr;
extern uint64_t curtask_codebuffer_addr[10];
extern uint64_t curtask_codebuffer_size[10];

struct dataset{
    bool isused;
    uint64_t stub_addr;
    uint64_t real_addr;
    uint64_t set_size;
};

struct shadowtask{
    bool is_available;
    
    uint64_t realm_id;
    uint64_t realm_size;
    uint64_t realm_startaddr;


    struct dataset total_databuffer[10];
    struct dataset total_dataset[10];

    bool realm_bitmap_page[(CCAEXT_REALM_SIZE>>12)];
    uint64_t gpu_gpt_realm;

    uint64_t real_gpupte_ttbr;
    uint64_t real_gpupte_size;
    bool real_gpupte_bitmap_page[(CCAEXT_REALM_SIZE>>12)];





};

extern struct shadowtask shadowtask_total[CCAEXT_SHADOWTASK_MAX];

bool check_and_tag_bitmap_realm(uint64_t realm_id, uint64_t startaddr, uint64_t required_size, bool bit);
uint64_t find_available_realm_region(uint64_t realm_id, uint64_t required_size);
void init_ccaext_shadowtask(void);
struct shadowtask *allocate_realm_with_realmid(uint64_t realm_id);
struct shadowtask *find_realm_with_realmid(uint64_t realm_id);
void destroy_realm(struct shadowtask *cur_shadowtask);
uint64_t ccaext_check_buffer(uint64_t realm_id);
void record_each_dataset(uint64_t realm_id,uint64_t stub_addr,uint64_t real_addr,uint64_t set_size);
void record_each_databuffer(uint64_t realm_id,uint64_t stub_addr,uint64_t real_addr,uint64_t set_size);
void protect_total_dataset(uint64_t realm_id);
uint64_t get_dataset_real_addr(struct shadowtask *cur_shadowtask, uint64_t stub_addr);
uint64_t get_databuffer_real_addr(struct shadowtask *cur_shadowtask, uint64_t stub_addr);
void init_gpu_stubtable_structure(void);
uint64_t get_databuffer_stub_addr(struct shadowtask *cur_shadowtask, uint64_t real_addr);
void update_gpu_table(struct shadowtask *cur_shadowtask, uint64_t req_start_ipa,uint64_t req_size,uint64_t req_start_pa,uint64_t req_attr);
uint64_t find_available_gpupte_page(struct shadowtask *cur_shadowtask);
#endif
#endif
