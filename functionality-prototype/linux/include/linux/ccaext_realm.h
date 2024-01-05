#ifndef __CCAEXT_REALM_H__
#define __CCAEXT_REALM_H__

#define CCAEXT_SHADOWTASK_MAX 0x10
#define CCAEXT_GPU_MEMORY_STARTADDR 0x880000000
#define CCAEXT_GPU_MEMORY_SIZE 0x10000000
#define CCAEXT_GPU_MEMORY_BITMAP_NUM (CCAEXT_GPU_MEMORY_SIZE >> 12)

struct ccaext_realm {
    bool is_available;
    uint64_t realm_id;
};

extern bool ccaext_gpu_memory_bitmap[CCAEXT_GPU_MEMORY_BITMAP_NUM];


extern struct ccaext_realm total_realm[CCAEXT_SHADOWTASK_MAX];
uint64_t find_available_gpu_memory_region(uint64_t required_size);
#endif