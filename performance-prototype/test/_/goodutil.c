#ifndef GOODUTIL_H
#define GOODUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "goodutil.h"
#define CMD(x) _IO(77, x)

uint64_t ccaext_magic_number=0xaaaabbbbccccdddd;
int fd = open("/dev/ccaext_realm_driver_device", O_RDWR);

cl_mem clCreateNewBuffer(cl_context context,
 	cl_mem_flags flags,
 	size_t size,
 	void *host_ptr,
 	cl_int *errcode_ret,
   	int useless){

    size_t actual_size = size+0x1000;
    cl_mem buffer_addr = clCreateBuffer(context, flags, actual_size, host_ptr, errcode_ret);
    unsigned long addr = *(unsigned long*)((char*)buffer_addr+368);
    
    unsigned long align_addr = 0;

    if (addr%0x1000) align_addr= ((addr>>12)+1)<<12;
    else align_addr=((addr>>12)+0)<<12;

        // For clEnqueueRead/Write
        *(unsigned long*)((char*)buffer_addr+368) = align_addr; 
        // For enqueueKernel
        unsigned long ptr;
        ptr = *(unsigned long*)((char*)buffer_addr+376);
        ptr = *(unsigned long*)(ptr+0x80);
        assert(
            *(unsigned long*)(ptr+0x10) == addr && 
            *(unsigned long*)(ptr+0x20) == addr && 
            *(unsigned long*)(ptr+0x30) == addr
        );
        *(unsigned long*)(ptr+0x10) = align_addr;
        *(unsigned long*)(ptr+0x20) = align_addr;
        *(unsigned long*)(ptr+0x30) = align_addr;
    return buffer_addr;
}

void fill_description(cl_mem buffer_addr,uint64_t infoarray[]){
    uint64_t align_addr= *(unsigned long*)((char*)buffer_addr+368);
    *(uint64_t*)align_addr=infoarray[0];
    *(uint64_t*)(align_addr+0x8)=infoarray[1];
    *(uint64_t*)(align_addr+0x20)=infoarray[4];
    *(uint64_t*)(align_addr+0x28)=infoarray[5];
    if ((infoarray[5]&0x1)!=0){
        *(uint64_t*)(align_addr+0x30)=infoarray[6];
        *(uint64_t*)(align_addr+0x38)=infoarray[7];        
    }
}

void init_databuffer_infoarray(
    uint64_t infoarray[],
    uint64_t curtask_app_uuid, uint64_t curtask_total_databuffer,
    uint64_t curtask_curdatabuffer_id, uint64_t curtask_curdatabuffer_size,
    uint64_t curtask_curdatabuffer_attr, uint64_t curtask_curdatabuffer_dataset_addr,
    uint64_t curtask_curdatabuffer_dataset_size,
    uint64_t curtask_index){
    infoarray[0]=ccaext_magic_number; // For GPU driver use, distinguish whether it is an confidential application
    infoarray[1]=curtask_app_uuid; 
    infoarray[2]=curtask_total_databuffer;
    infoarray[3]=curtask_curdatabuffer_id;
    infoarray[4]=curtask_curdatabuffer_size;
    infoarray[5]=curtask_curdatabuffer_attr;
    infoarray[6]=curtask_curdatabuffer_dataset_addr;
    infoarray[7]=curtask_curdatabuffer_dataset_size;
    infoarray[8]=curtask_index;
    return;
}

void init_ccaext_region_and_pass_data(uint64_t app_uuid, uint64_t total_dataset_num, struct dataset total_dataset[]){
    unsigned int cmd_createrealm=CMD(0x1);
    uint64_t *cmd_createrealm_args=(uint64_t*)malloc(0x100);
    memset(cmd_createrealm_args,0x0,0x100);
    cmd_createrealm_args[0]=app_uuid;
    cmd_createrealm_args[1]=total_dataset_num; //one section of data

    int cur_tmpidx=2;
    uint64_t cur_dataset_idx=0;
    while (cur_dataset_idx<total_dataset_num){
        cmd_createrealm_args[cur_tmpidx]=total_dataset[cur_dataset_idx].set_addr;
        cur_tmpidx+=1;
        cmd_createrealm_args[cur_tmpidx]=total_dataset[cur_dataset_idx].set_size;
        cur_tmpidx+=1;
        cur_dataset_idx+=1;
    }
    int ioctlret = ioctl(fd, cmd_createrealm, cmd_createrealm_args);
    assert(ioctlret == 0);
}

void destroy_ccaext_region(uint64_t app_uuid){
    unsigned int cmd_destroyrealm=CMD(0x2);
    uint64_t *cmd_destroyrealm_args=(uint64_t*)malloc(0x100);
    memset(cmd_destroyrealm_args,0x0,0x100);    
    cmd_destroyrealm_args[0]=app_uuid;
    int ioctlret = ioctl(fd, cmd_destroyrealm, cmd_destroyrealm_args);
    assert(ioctlret == 0);
}

#endif
