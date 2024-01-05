#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <asm-generic/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <vector>
#include <sys/time.h>

#define CCAEXT_DATABUFFER_INPUT 0x1
#define CCAEXT_DATABUFFER_OUTPUT 0x2
#define CCAEXT_DATABUFFER_BOTHIO (CCAEXT_DATABUFFER_INPUT|CCAEXT_DATABUFFER_OUTPUT)
#define CCAEXT_DATABUFFER_NULL 0x0

struct dataset{
    uint64_t set_addr;
    uint64_t set_size;
};


cl_mem clCreateNewBuffer(cl_context context,
 	cl_mem_flags flags,
 	size_t size,
 	void *host_ptr,
 	cl_int *errcode_ret,
   	int useless);

void init_databuffer_infoarray(
    uint64_t infoarray[],
    uint64_t curtask_app_uuid, uint64_t curtask_total_databuffer,
    uint64_t curtask_curdatabuffer_id, uint64_t curtask_curdatabuffer_size,
    uint64_t curtask_curdatabuffer_attr, uint64_t curtask_curdatabuffer_dataset_addr,
    uint64_t curtask_curdatabuffer_dataset_size,
    uint64_t curtask_index);

void init_ccaext_region_and_pass_data(uint64_t app_uuid, uint64_t total_dataset_num, struct dataset total_dataset[]);
void destroy_ccaext_region(uint64_t app_uuid);
extern uint64_t ccaext_magic_number;
void fill_description(cl_mem buffer_addr,uint64_t infoarray[]);