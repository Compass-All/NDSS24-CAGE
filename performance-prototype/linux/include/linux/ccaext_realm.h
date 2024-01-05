#ifndef __CCAEXT_REALM_H__
#define __CCAEXT_REALM_H__

struct ccaext_kctx{
    uint64_t kctx_addr;
    uint32_t kctx_pid;
    uint32_t num_apps;
};

extern struct ccaext_kctx total_kctx[10];
extern int stop_submit;

extern uint64_t num_update_pte_sections;
extern uint64_t next_available_pte_id;


int find_kctx_by_pid(uint32_t kctx_pid);
void update_kctx_addr_by_pos(int kctx_pos,uint64_t kctx_addr);
bool check_any_ccaext_katom(void);
int find_kctx_by_kctx_addr(uint64_t kctx_addr);
void add_update_pte_entries(uint64_t pte_id,uint64_t pa_desc);
void add_update_pte_section(uint64_t sec_id,uint64_t curva, uint64_t size);

#endif