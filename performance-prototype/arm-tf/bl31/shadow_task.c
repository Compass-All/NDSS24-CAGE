#include <bl31/other_util.h>
#include <bl31/shadow_task.h>
#include <bl31/gpu_protect.h>


#include <lib/mmio.h>
#include <arch.h>
#include <common/debug.h>

struct shadowtask shadowtask_total[CCAEXT_SHADOWTASK_MAX];
uint64_t *bitmap_realtask_page = (uint64_t*)0xa0f00000; 
uint32_t init_H[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

void init_gpu_realtable_region(void){
    datafiller(CCAEXT_REALTASK_TABLE_STARTADDR,(CCAEXT_REALTASK_TABLE_STARTADDR-CCAEXT_REALTASK_MEMORY_STARTADDR),0x0);
}


void init_bitmap_realtask_page(void){
    uint64_t curaddr_realtask=CCAEXT_REALTASK_MEMORY_STARTADDR;
    uint64_t endaddr_realtask=CCAEXT_REALTASK_TABLE_STARTADDR;
    while(curaddr_realtask<endaddr_realtask){
        bitmap_realtask_config(curaddr_realtask,0x1);
        curaddr_realtask+=0x1000;
    }
    NOTICE("CCA Extension Bitmap for Real Task Region Initialized\n");
}

bool check_and_tag_bitmap_realtask(uint64_t startaddr, uint64_t required_size, uint64_t bit){
    uint64_t cursize=0;
    while (cursize<required_size){
        if (bitmap_realtask_check((startaddr+cursize),bit)){
            return false;
        }
        bitmap_realtask_config((startaddr+cursize),bit);
        cursize+=0x1000;
    }
    return true;
}

uint64_t find_available_realtask_region(uint64_t required_size){
    uint64_t failedaddr=0x0;
    uint64_t startaddr=CCAEXT_REALTASK_MEMORY_STARTADDR;
    uint64_t endaddr=CCAEXT_REALTASK_TABLE_STARTADDR;
    while ((startaddr+required_size)<=endaddr){

        
        if(bitmap_realtask_check(startaddr,0x1)){
            bool is_satisfied=true;
            uint64_t cursize=0;
            while(cursize<required_size){
                if(bitmap_realtask_check((startaddr+cursize),0x0)){
                    is_satisfied=false;
                    break;
                }
                cursize+=0x1000;
            }
            if (is_satisfied){
                return startaddr;
            }
            else{
                startaddr+=cursize;
            }
        }
        else{
            startaddr+=0x1000;
        }

    }
    return failedaddr;
}

void init_ccaext_shadowtask(void){    
    uint32_t cur_index=0;
    uint64_t cur_real_gpupte_startaddr=CCAEXT_REALTASK_TABLE_STARTADDR;
    
    uint64_t cur_real_gpupte_size=0x1000000;
    uint64_t cur_gpu_gpt_mempart_startaddr=0xa0900000;
    uint64_t cur_gpu_gpt_mempart_size=0x20000;


	while(cur_index<CCAEXT_SHADOWTASK_MAX){
        memset(&shadowtask_total[cur_index],0,sizeof(struct shadowtask));
        shadowtask_total[cur_index].is_available=true;
        shadowtask_total[cur_index].stub_app_id=0;
        int tmp_idx=0;
        while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATASET){
            shadowtask_total[cur_index].total_dataset[tmp_idx].isused=false;
            shadowtask_total[cur_index].total_dataset[tmp_idx].stub_addr=0;
            shadowtask_total[cur_index].total_dataset[tmp_idx].real_addr=0;
            shadowtask_total[cur_index].total_dataset[tmp_idx].set_size=0;
            tmp_idx+=1;
        }
        tmp_idx=0;
        while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATABUFFER){
            shadowtask_total[cur_index].total_databuffer[tmp_idx].isused=false;
            shadowtask_total[cur_index].total_databuffer[tmp_idx].stub_addr=0;
            shadowtask_total[cur_index].total_databuffer[tmp_idx].real_addr=0;
            shadowtask_total[cur_index].total_databuffer[tmp_idx].set_size=0;
            tmp_idx+=1;
        }

        shadowtask_total[cur_index].real_gpupte_startaddr=cur_real_gpupte_startaddr;
        shadowtask_total[cur_index].real_mmu_pgd=0;
        shadowtask_total[cur_index].stub_mmu_pgd=0;
        shadowtask_total[cur_index].real_gpupte_size=cur_real_gpupte_size;
        shadowtask_total[cur_index].gpu_gpt_mempart_startaddr=cur_gpu_gpt_mempart_startaddr;        
        init_ccaext_gpu_gpt_mempart(shadowtask_total[cur_index].gpu_gpt_mempart_startaddr,
            shadowtask_total[cur_index].real_gpupte_startaddr,
            shadowtask_total[cur_index].real_gpupte_size);
        cur_real_gpupte_startaddr+=cur_real_gpupte_size;
        cur_gpu_gpt_mempart_startaddr+=cur_gpu_gpt_mempart_size;

        tmp_idx=0;
        while(tmp_idx<CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX){
            shadowtask_total[cur_index].bitmap_real_gpupte_page[tmp_idx]=false;
            tmp_idx+=1;
        }
        cur_index+=1;
	}    
    NOTICE("CCA Extension Task Binder Initialized\n");
}

struct shadowtask *add_ccaext_shadowtask(uint64_t stub_app_id){
    uint32_t cur_index=0;
    while(cur_index<CCAEXT_SHADOWTASK_MAX){
        if (shadowtask_total[cur_index].is_available==true){
            shadowtask_total[cur_index].is_available=false;
            shadowtask_total[cur_index].stub_app_id=stub_app_id;
            return &shadowtask_total[cur_index];
        }
        cur_index+=1;
	}
    return NULL;
}

struct shadowtask *find_ccaext_shadowtask(uint64_t stub_app_id){
    uint32_t cur_index=0;
    while(cur_index<CCAEXT_SHADOWTASK_MAX){
        if (shadowtask_total[cur_index].stub_app_id==stub_app_id){
            return &shadowtask_total[cur_index];
        }
        cur_index+=1;
    }
    return NULL;
}

void destroy_ccaext_shadowtask(struct shadowtask *cur_shadowtask){

    uint64_t app_id=cur_shadowtask->stub_app_id;
    int tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX){
        cur_shadowtask->bitmap_real_gpupte_page[tmp_idx]=false;
        tmp_idx+=1;
    }
    datafiller(cur_shadowtask->real_gpupte_startaddr,cur_shadowtask->real_gpupte_size,0x2);

    tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATASET){
        if(cur_shadowtask->total_dataset[tmp_idx].isused==true){
            
            uint64_t cur_start_addr=cur_shadowtask->total_dataset[tmp_idx].real_addr;
            uint64_t cur_size=cur_shadowtask->total_dataset[tmp_idx].set_size;           
            bool is_shadowtask_satisfied=check_and_tag_bitmap_realtask(cur_start_addr,cur_size,0x1);
            if (!is_shadowtask_satisfied){
                ERROR("In app %llx, cannot clear region %llx with size %llx\n",app_id,cur_start_addr,cur_size);
                panic();
            }
            datafiller(cur_shadowtask->total_dataset[tmp_idx].real_addr,cur_shadowtask->total_dataset[tmp_idx].set_size,0);

            cur_shadowtask->total_dataset[tmp_idx].isused=false;
            cur_shadowtask->total_dataset[tmp_idx].stub_addr=0;
            cur_shadowtask->total_dataset[tmp_idx].real_addr=0;
            cur_shadowtask->total_dataset[tmp_idx].set_size=0;
            gptconfig_sublevel(0xa00a0000,cur_start_addr,cur_size,CCAEXT_ALL);
        }

        if(cur_shadowtask->total_databuffer[tmp_idx].isused==true){
            
            uint64_t cur_start_addr=cur_shadowtask->total_databuffer[tmp_idx].real_addr;
            uint64_t cur_size=cur_shadowtask->total_databuffer[tmp_idx].set_size;           
            bool is_shadowtask_satisfied=check_and_tag_bitmap_realtask(cur_start_addr,cur_size,0x1);
            if (!is_shadowtask_satisfied){
                ERROR("In app %llx, cannot clear region %llx with size %llx\n",app_id,cur_start_addr,cur_size);
                panic();
            }
            datafiller(cur_shadowtask->total_databuffer[tmp_idx].real_addr,cur_shadowtask->total_databuffer[tmp_idx].set_size,0);
            
            gptconfig_sublevel(0xa00a0000,cur_start_addr,cur_size,CCAEXT_ALL);
            gptconfig_sublevel(cur_shadowtask->gpu_gpt_mempart_startaddr,cur_start_addr,cur_size,CCAEXT_INVAL);

            cur_shadowtask->total_databuffer[tmp_idx].isused=false;
            cur_shadowtask->total_databuffer[tmp_idx].stub_addr=0;
            cur_shadowtask->total_databuffer[tmp_idx].real_addr=0;
            cur_shadowtask->total_databuffer[tmp_idx].set_size=0;
        }


        tmp_idx+=1;
    }

    cur_shadowtask->is_available=true;
    cur_shadowtask->stub_app_id=0;
    cur_shadowtask->stub_mmu_pgd=0;
    cur_shadowtask->real_mmu_pgd=0;
    NOTICE("Shadow app id: %llx destroyed\n",app_id);
}


uint64_t ccaext_check_buffer(uint64_t num_update_pte_sections) {
    uint64_t data_field_virtaddr=*(uint64_t*)(curtask_jc_phys+0x130);
    uint64_t code_field_virtaddr=*(uint64_t*)(curtask_jc_phys+0x138);


	uint64_t data_field_addr = read_ttbr_core(data_field_virtaddr,curtask_stub_mmu_pgd); 
	uint64_t code_field_addr = read_ttbr_core(code_field_virtaddr,curtask_stub_mmu_pgd); 
    flush_dcache_range(data_field_addr,(code_field_addr-data_field_addr));

    uint64_t stub_app_id=0;
    struct shadowtask *cur_shadowtask=NULL;

    gptconfig_disable_gpu_gpt_protection();
    bool real_gpupte_need_update= num_update_pte_sections==0? false: true;

    for (uint64_t current_addr = data_field_addr ; current_addr < code_field_addr ; current_addr += 8){
        uint64_t buffer_virt_addr = *(uint64_t*)current_addr;
        if(buffer_virt_addr==0)continue;

        uint64_t cur_map_sec=0;
        uint64_t cur_sec_va=0x89f000000;
        while(cur_map_sec<num_update_pte_sections){
            if((*(uint64_t*)cur_sec_va)==buffer_virt_addr){
                *(uint64_t*)cur_sec_va=0;
                flush_dcache_range(cur_sec_va,0x8);
                break;
            }
            cur_map_sec+=1;
            cur_sec_va+=0x10;
        }
    }


	for (uint64_t current_addr = data_field_addr ; current_addr < code_field_addr ; current_addr += 8) {
		uint64_t buffer_virt_addr = *(uint64_t*)current_addr;
		uint64_t buffer_phys_addr = read_ttbr_core(buffer_virt_addr,curtask_stub_mmu_pgd);
        
        if(buffer_phys_addr==0)continue;
        flush_dcache_range(buffer_phys_addr,0x50);                
        uint64_t magic_number=*(uint64_t*)(buffer_phys_addr);
        if(magic_number!=0xaaaabbbbccccdddd){
            ERROR("Not a confidential task buffer\n");
            panic();
        }

        uint32_t H[8];
        void *current_ptr = (void*)buffer_phys_addr;
        fast_memcpy(H, init_H, sizeof(H));
        sha256_final(H, current_ptr, 0x80, 0x80);

        uint64_t cur_app_id=*(uint64_t*)(buffer_phys_addr+0x8);
        if(stub_app_id==0){
            stub_app_id=cur_app_id;
            cur_shadowtask=find_ccaext_shadowtask(stub_app_id);
        }
        else if(stub_app_id!=cur_app_id){
            ERROR("Find buffer from another task\n");
            panic();
        }

        if(cur_shadowtask->stub_mmu_pgd==0)cur_shadowtask->stub_mmu_pgd=curtask_stub_mmu_pgd;
        else if (cur_shadowtask->stub_mmu_pgd != curtask_stub_mmu_pgd){
            ERROR("Detecting the stub mmu pgd is changed\n");
            panic();
        }
        
        //if the real page table is not init or updated, consturct one
        if(cur_shadowtask->real_mmu_pgd==0) {
            reconstuct_gpupte(cur_shadowtask,num_update_pte_sections,true);
            real_gpupte_need_update=false;
        }
        else if(real_gpupte_need_update){
            reconstuct_gpupte(cur_shadowtask,num_update_pte_sections,false);
            real_gpupte_need_update=false;            
        }


        uint64_t buffer_size=*(uint64_t*)(buffer_phys_addr+0x20);
        uint64_t aligned_buffer_size=buffer_size&0xfffffffff000;
        if(buffer_size&0xfff)aligned_buffer_size+=0x1000;
        uint64_t databuffer_real_addr=get_databuffer_real_addr(cur_shadowtask,buffer_virt_addr);
        
        if(databuffer_real_addr==0){
            databuffer_real_addr=find_available_realtask_region(aligned_buffer_size);
            if(databuffer_real_addr==0){
				ERROR("No available region to allocate databuffer.\n");
				panic();
			}
			record_each_databuffer(stub_app_id,buffer_virt_addr,databuffer_real_addr,aligned_buffer_size);
            
            
            //Configure GPTs for the protected region.
            gptconfig_sublevel(0xa00a0000,databuffer_real_addr,aligned_buffer_size,CCAEXT_ROOT);
            gptconfig_sublevel(cur_shadowtask->gpu_gpt_mempart_startaddr,databuffer_real_addr,aligned_buffer_size,CCAEXT_ALL);

            //Update GPU PTE mapping for the protected region.
            insert_real_gpupte_entries(cur_shadowtask,databuffer_real_addr,(aligned_buffer_size>>12),0,true);
        }

        uint64_t buffer_attr=*(uint64_t*)(buffer_phys_addr+0x28);
            if ((buffer_attr&0x1)!=0){ 

            uint64_t dataset_stub_addr=*(uint64_t*)(buffer_phys_addr+0x30);
            uint64_t transferred_size=*(uint64_t*)(buffer_phys_addr+0x38);


            //Find the real_addr of dataset based on the dataset_stub_addr
            uint64_t dataset_real_addr=get_dataset_real_addr(cur_shadowtask,dataset_stub_addr);
            if(dataset_real_addr==0){
                ERROR("Cannot find the real_addr based on source %llx\n",dataset_stub_addr);
                panic();
            }

            //Transfer data, because it is a input buffer
            fast_memcpy_addr(databuffer_real_addr, dataset_real_addr, transferred_size); 
        }

        //Then process the buffer replacement
        datafiller(current_addr,0x8,databuffer_real_addr);
        
        uint64_t replacestartaddr= curtask_jc_phys & 0xffffff000;
		uint64_t replaceendaddr=replacestartaddr+0x1000;
		while (replacestartaddr<replaceendaddr){
			if(*(uint64_t*)replacestartaddr==buffer_virt_addr){
                datafiller(replacestartaddr,0x8,databuffer_real_addr);
            }
			replacestartaddr+=0x8;
		}
        
	}

    check_code_integrity();

    gptconfig_enable_gpu_gpt_protection(cur_shadowtask->gpu_gpt_mempart_startaddr);

    return stub_app_id;
}

void record_each_dataset(uint64_t stub_app_id,uint64_t stub_addr,uint64_t real_addr,uint64_t set_size){

	bool is_shadowtask_satisfied=check_and_tag_bitmap_realtask(real_addr, set_size, 0x0);
    if (!is_shadowtask_satisfied){
        ERROR("In app %llx, cannot record dataset real_addr %llx, size: %llx\n",stub_app_id,real_addr,set_size);
        panic();
    }
    struct shadowtask *cur_shadowtask=find_ccaext_shadowtask(stub_app_id);
    int tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATASET){
        if(cur_shadowtask->total_dataset[tmp_idx].isused==false){
            cur_shadowtask->total_dataset[tmp_idx].isused=true;
            cur_shadowtask->total_dataset[tmp_idx].stub_addr=stub_addr;
            cur_shadowtask->total_dataset[tmp_idx].real_addr=real_addr;
            cur_shadowtask->total_dataset[tmp_idx].set_size=set_size;
            break;
        }
        tmp_idx+=1;
    }
}

void record_each_databuffer(uint64_t stub_app_id,uint64_t stub_addr,uint64_t real_addr,uint64_t set_size){

	bool is_shadowtask_satisfied=check_and_tag_bitmap_realtask(real_addr, set_size, 0x0);
    if (!is_shadowtask_satisfied){
        ERROR("In app %llx, cannot record databuffer real_addr %llx, size: %llx\n",stub_app_id,real_addr,set_size);
        panic();
    }
    struct shadowtask *cur_shadowtask=find_ccaext_shadowtask(stub_app_id);
    int tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATABUFFER){
        if(cur_shadowtask->total_databuffer[tmp_idx].isused==false){
            cur_shadowtask->total_databuffer[tmp_idx].isused=true;
            cur_shadowtask->total_databuffer[tmp_idx].stub_addr=stub_addr;
            cur_shadowtask->total_databuffer[tmp_idx].real_addr=real_addr;
            cur_shadowtask->total_databuffer[tmp_idx].set_size=set_size;
            break;
        }
        tmp_idx+=1;
    }
}

void protect_total_dataset(uint64_t stub_app_id){
    struct shadowtask *cur_shadowtask=find_ccaext_shadowtask(stub_app_id);
    int tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATASET){
        if(cur_shadowtask->total_dataset[tmp_idx].isused==true){
            uint64_t cur_start_addr=cur_shadowtask->total_dataset[tmp_idx].real_addr;
            uint64_t cur_size=cur_shadowtask->total_dataset[tmp_idx].set_size;
            gptconfig_sublevel(0xa00a0000,cur_start_addr,cur_size,CCAEXT_ROOT);            
        }
        tmp_idx+=1;
    }
}

uint64_t get_dataset_real_addr(struct shadowtask *cur_shadowtask, uint64_t stub_addr){
    int tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATASET){
        if(cur_shadowtask->total_dataset[tmp_idx].isused==true){
            if (cur_shadowtask->total_dataset[tmp_idx].stub_addr==stub_addr){
                return cur_shadowtask->total_dataset[tmp_idx].real_addr;
            }
        }
        tmp_idx+=1;
    }
    return 0;
}


uint64_t get_databuffer_real_addr(struct shadowtask *cur_shadowtask, uint64_t stub_addr){
    int tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATABUFFER){
        if(cur_shadowtask->total_databuffer[tmp_idx].isused==true){
            if (cur_shadowtask->total_databuffer[tmp_idx].stub_addr==stub_addr){
                return cur_shadowtask->total_databuffer[tmp_idx].real_addr;
            }
        }
        tmp_idx+=1;
    }
    return 0;
}

uint64_t get_databuffer_stub_addr(struct shadowtask *cur_shadowtask, uint64_t real_addr){
    int tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_MAX_TOTALDATABUFFER){
        if(cur_shadowtask->total_databuffer[tmp_idx].isused==true){
            if (cur_shadowtask->total_databuffer[tmp_idx].real_addr==real_addr){
                return cur_shadowtask->total_databuffer[tmp_idx].stub_addr;
            }
        }
        tmp_idx+=1;
    }
    return 0;
}

void datacopy_real_to_stubbuffer(uint64_t src_phys_addr, uint64_t dst_virt_addr, size_t total_size){
    
    if(dst_virt_addr&0xfff){
        ERROR("Input dst_virt_addr addr not aligned\n");
        panic();
    }
    if(src_phys_addr&0xfff){
        ERROR("Input src_phys_addr addr not aligned\n");
        panic();
    }    
    uint64_t remain_size=total_size;
    uint64_t cur_src_addr=src_phys_addr;
    uint64_t cur_dst_addr=dst_virt_addr;
    while(remain_size>0){
        if(remain_size<0x1000){
            uint64_t cur_phys_dst_addr=read_ttbr_core(cur_dst_addr,curtask_stub_mmu_pgd);
            fast_memcpy_addr(cur_phys_dst_addr,cur_src_addr,remain_size);
            flush_dcache_range(cur_phys_dst_addr,remain_size);
            remain_size=0;
        }
        else{
            uint64_t cur_phys_dst_addr=read_ttbr_core(cur_dst_addr,curtask_stub_mmu_pgd);
            fast_memcpy_addr(cur_phys_dst_addr,cur_src_addr,0x1000);
            flush_dcache_range(cur_phys_dst_addr,0x1000);
            remain_size-=0x1000;
            cur_dst_addr+=0x1000;
            cur_src_addr+=0x1000;
        }
    }
}



void init_gpu_stubtable_structure(void){
    
    datafiller(CCAEXT_STUBTASK_TABLE_STARTADDR,0x400000,0x2);
    datafiller(CCAEXT_STUBTASK_TABLE_STARTADDR+0x400000,0x1000,0x2);
    uint64_t curaddr=CCAEXT_STUBTASK_TABLE_STARTADDR+0x400800;
    uint64_t endaddr=CCAEXT_STUBTASK_TABLE_STARTADDR+0x400c00;
    uint64_t subleveldesc=CCAEXT_STUBTASK_TABLE_STARTADDR+0x700003;
    while(curaddr<endaddr){
        datafiller(curaddr,0x8,subleveldesc);
        subleveldesc+=0x1000;
        curaddr+=0x8;
    }
    datafiller(CCAEXT_STUBTASK_TABLE_STARTADDR+0x600000,0x200000,0x2);
}

void configure_vapa_stub_databuffer(uint64_t va, uint64_t size, uint64_t this_mmu_pgd){
    uint64_t realsize=size&0xfffffffff000;
    if(size&0xfff)realsize+=0x1000;

    uint64_t curdescaddr = this_mmu_pgd;
    uint64_t curdescval=*(uint64_t*)curdescaddr;
    uint64_t curtable=curdescval & 0x0000fffffffff000;
    curdescaddr= curtable+((0x22)<<3);
    curdescval = *(uint64_t*)curdescaddr;
    if(curdescval==0x2){
        datafiller(curdescaddr,0x8,CCAEXT_STUBTASK_TABLE_STARTADDR +0x400003);
        curdescval = *(uint64_t*)curdescaddr;
    }
    uint64_t init_target_addr=CCAEXT_STUBTASK_TABLE_STARTADDR+((va-CCAEXT_STUBTASK_MEMORY_STARTADDR)>>9)+0x600000;
    uint64_t target_addr=init_target_addr;
    uint64_t target_desc=va+0x00400000000003c1;
    uint64_t cur_size=0;
    while(cur_size<realsize){
        *(uint64_t*)target_addr=target_desc;
        cur_size+=0x1000;
        target_addr+=0x8;
        target_desc+=0x1000;
    }
    flush_dcache_range(init_target_addr,(realsize>>9));
}

void restore_vapa_stub_databuffer(uint64_t va, uint64_t size, uint64_t this_mmu_pgd){
    uint64_t realsize=size&0xfffffffff000;
    if(size&0xfff)realsize+=0x1000;

    uint64_t curdescaddr = this_mmu_pgd;
    uint64_t curdescval=*(uint64_t*)curdescaddr;
    uint64_t curtable=curdescval & 0x0000fffffffff000;
    curdescaddr= curtable+((0x22)<<3);
    curdescval = *(uint64_t*)curdescaddr;
    datafiller(curdescaddr,0x8,0x2); 
    uint64_t init_target_addr=CCAEXT_STUBTASK_TABLE_STARTADDR+((va-CCAEXT_STUBTASK_MEMORY_STARTADDR)>>9)+0x600000;
    uint64_t target_addr=init_target_addr;
    uint64_t cur_size=0;
    while(cur_size<realsize){
        *(uint64_t*)target_addr=0x2;
        cur_size+=0x1000;
        target_addr+=0x8;
    }
    flush_dcache_range(init_target_addr,(realsize>>9));
}


void reconstuct_gpupte(struct shadowtask *cur_shadowtask, uint64_t num_update_pte_sections,bool is_init){
    if(num_update_pte_sections==0x0){
        ERROR("We do not need to update the gpu page table entry.\n");
        return;
    }

    if(is_init){
        cur_shadowtask->real_mmu_pgd=cur_shadowtask->real_gpupte_startaddr;
        configure_real_gpupte_page_used(cur_shadowtask,cur_shadowtask->real_mmu_pgd);
    }

    uint64_t cnt_pte_sec=0;
	uint64_t cur_pte_sec_addr=0x89f000000;
	uint64_t cur_pte_sec_entry_startaddr=0x89f001000;

	while (cnt_pte_sec<num_update_pte_sections){
        
        uint64_t cur_pte_start_va=*(uint64_t*)(cur_pte_sec_addr);
        uint64_t cur_num_pte_entries=*(uint64_t*)(cur_pte_sec_addr+0x8);

        if(cur_pte_start_va!=0x0){
            insert_real_gpupte_entries(cur_shadowtask,cur_pte_start_va,cur_num_pte_entries,cur_pte_sec_entry_startaddr,false);
        }

    	cur_pte_sec_addr+=0x10;
		cnt_pte_sec+=1;
        cur_pte_sec_entry_startaddr+=(0x8*cur_num_pte_entries);
    }
}


uint32_t find_available_real_gpupte_page(struct shadowtask *cur_shadowtask){
    uint32_t tmp_idx=0;
    while(tmp_idx<CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX){
        if(cur_shadowtask->bitmap_real_gpupte_page[tmp_idx]==false){
            cur_shadowtask->bitmap_real_gpupte_page[tmp_idx]=true;
            return tmp_idx;
        }
        tmp_idx+=1;
    }
    return tmp_idx; 
}

void configure_real_gpupte_page_used(struct shadowtask *cur_shadowtask,uint64_t cur_addr){
    int tmp_idx= (cur_addr - cur_shadowtask->real_gpupte_startaddr)>>12;
    cur_shadowtask->bitmap_real_gpupte_page[tmp_idx]=true;
}


void insert_real_gpupte_entries(struct shadowtask *cur_shadowtask, uint64_t start_va, uint64_t num_entries, uint64_t pa_desc_startaddr,bool is_real_databuffer){
    
    //Based on the GPU driver's implementation, we insert page entries

    uint64_t cur_start_va=start_va;
    uint64_t cur_pa_desc_startaddr=pa_desc_startaddr;

    uint64_t remain_entries=num_entries;
    while(remain_entries!=0){
        
        uint64_t cur_block_entries=0x200-((cur_start_va & 0x1FF000)>>12);
        if(cur_block_entries>remain_entries)cur_block_entries=remain_entries;

        //First get the target table, if no, construct it
        uint64_t table_base, offset, phys_DA;
        table_base = cur_shadowtask->real_mmu_pgd;
        offset = get_bit_range_value(cur_start_va, 47, 39) << 3;
	    phys_DA = (table_base & 0xfffffffff000) | offset;
        table_base = *(uint64_t*)phys_DA;
        if ((table_base & 0x3) != 0x3){
            int tmp_idx = find_available_real_gpupte_page(cur_shadowtask);
            if(tmp_idx==CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX){
                ERROR("NO Page can be used\n");
                return;
            }
            uint64_t tmp_page_addr=cur_shadowtask->real_gpupte_startaddr+(tmp_idx<<12);
            *(uint64_t*)phys_DA = tmp_page_addr + 0x3;
		    flush_dcache_range(phys_DA,0x8);
		    table_base = *(uint64_t*)phys_DA;
        }

        offset = get_bit_range_value(cur_start_va, 38, 30) << 3;
	    phys_DA = (table_base & 0xfffffffff000) | offset;
	    table_base = *(uint64_t*)phys_DA;
	    if ((table_base & 0x3) != 0x3){
            int tmp_idx = find_available_real_gpupte_page(cur_shadowtask);
            if(tmp_idx==CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX){
                ERROR("NO Page can be used\n");
                return;
            }
            uint64_t tmp_page_addr=cur_shadowtask->real_gpupte_startaddr+(tmp_idx<<12);
            *(uint64_t*)phys_DA = tmp_page_addr + 0x3;
		    flush_dcache_range(phys_DA,0x8);
		    table_base = *(uint64_t*)phys_DA;
	    }

	    offset = get_bit_range_value(cur_start_va, 29, 21) << 3;
	    phys_DA = (table_base & 0xfffffffff000) | offset;
	    table_base = *(uint64_t*)phys_DA;
	    if ((table_base & 0x3) != 0x3){
            int tmp_idx = find_available_real_gpupte_page(cur_shadowtask);
            if(tmp_idx==CCAEXT_SHADOWTASK_REAL_GPUPTE_MAX){
                ERROR("NO Page can be used\n");
                return;
            }
            uint64_t tmp_page_addr=cur_shadowtask->real_gpupte_startaddr+(tmp_idx<<12);
            *(uint64_t*)phys_DA = tmp_page_addr + 0x3;
		    flush_dcache_range(phys_DA,0x8);
		    table_base = *(uint64_t*)phys_DA;
	    }
	    offset = get_bit_range_value(cur_start_va, 20, 12) << 3;
	    uint64_t cur_phys_DA = (table_base & 0xfffffffff000) | offset; 

        uint64_t tmp_block_entries=0;
        uint64_t cur_insert_va=cur_start_va;

        //Find the index of the target PA

        while(tmp_block_entries<cur_block_entries){
            uint64_t cur_pa_desc=0;

            if(is_real_databuffer){
                cur_pa_desc= cur_insert_va+0x00400000000003c1;//VA==PA
            }
            else{
                cur_pa_desc=*(uint64_t*)cur_pa_desc_startaddr;
            }            
            *(uint64_t*)cur_phys_DA=cur_pa_desc;
            flush_dcache_range(cur_phys_DA,0x8);
            cur_pa_desc_startaddr+=0x8;
            tmp_block_entries+=1;
            cur_phys_DA+=0x8;
            cur_insert_va+=0x1000;
        }
        remain_entries-=cur_block_entries;
        cur_start_va += (cur_block_entries<<12);
    }
}


void sha256_final(uint32_t *ctx, const void *in, size_t remain_size, size_t tot_size) {
	size_t block_num = remain_size / 64;
    
    sha256(ctx, in, block_num*64);

	size_t remainder = remain_size % 64;
	size_t tot_bits = tot_size * 8;
	char last_block[64]; 
	fast_memset(last_block, 0, sizeof(last_block));
	fast_memcpy(last_block, (void*)in+block_num*64, remainder);
	last_block[remainder] = 0x80;
	if (remainder < 56) {}
	else {
		sha256_block_data_order(ctx, last_block, 1);
		fast_memset(last_block, 0, sizeof(last_block));
	}
	for (int i = 0 ; i < 8 ; ++ i) last_block[63-i] = tot_bits >> (i * 8);
	sha256_block_data_order(ctx, last_block, 1);
}

void sha256(uint32_t *ctx, const void *in, size_t size) {
	size_t block_num = size / 64;
	if (block_num) sha256_block_data_order(ctx, in, block_num); 
}

int parse_mali_instruction(uint64_t *code) {
	// refer: https://gitlab.freedesktop.org/panfrost/mali-isa-docs/-/blob/master/Midgard.md
	// 3 - Texture (4 words)
	// 5 - Load/Store (4 words)
	// 8 - ALU (4 words)
	// 9 - ALU (8 words)
	// A - ALU (12 words)
	// B - ALU (16 words)
	uint64_t code_start = (uint64_t)code;
	// int code_length = 0;
	int current_type, next_type;
	while (1) {
		current_type = ((*code) & 0xf);
		next_type    = ((*code) & 0xf0) >> 4;
		switch (current_type) {
			case 3:
				code += 2;
				break;
			case 4:
				code += 2;
				break;
			case 5:
				code += 2;
				break;
			case 8:
				code += 2;
				break;
			case 9:
				code += 4;
				break;
			case 0xa:
				code += 6;
				break;
			case 0xb:
				code += 8;
				break;
			default:
				ERROR("[Unexcepted Behavior]: Instruction format [%d] error!\n", current_type);
				panic();
		}
		
		if (next_type == 1) break;
	}
	int code_length = (uint64_t)(code) - code_start;
	return code_length;
}

void hmac_sha256(void *out, const void *in, size_t size) {
	char k[64], k_ipad[64], k_opad[64];
	fast_memset(k, 0, 64);
	// fast_memcpy(k, aes_key, 32);
	for (int i = 0 ; i < 64 ; ++ i) {
		k_ipad[i] ^= k[i];
		k_opad[i] ^= k[i];
	}
	uint32_t ihash[8]; fast_memcpy(ihash, init_H, sizeof(ihash));
	sha256(ihash, k_ipad, 64); 
	sha256(ihash, in, size); 
	sha256_final(ihash, k_ipad, size%64, size+64);
	uint32_t ohash[8]; fast_memcpy(ohash, init_H, sizeof(ohash));
	sha256(ohash, k_opad, 64); 
	sha256(ohash, ihash, 64);
	sha256_final(ohash, k_opad, 0, 128);
	fast_memcpy(out, ohash, 64);
}

int check_code_integrity() {
    void *code_seg = (void*)(*(uint64_t*)(curtask_jc_phys+0x138));
	void *code_ptr = (void*)(*(uint64_t*)read_ttbr_core((uint64_t)code_seg,curtask_stub_mmu_pgd));
	void *code = (void *)(read_ttbr_core((uint64_t)code_ptr,curtask_stub_mmu_pgd) & (~0xf));
	int code_length = parse_mali_instruction(code);

	uint32_t HMAC[8]; 
	hmac_sha256(HMAC, code, code_length);

    // TODO: If we locate the entire GPU code buffers in closed-source OpenCL, then we can compare the HAMC value in code buffers

	uint32_t H0[8]={0,0,0,0,0,0,0,0};
	if (memcmp(HMAC, H0, 32) != 0) {
		// ERROR("HMAC value does not match!!!\n");
		// panic();
	}

    return 0;
}
