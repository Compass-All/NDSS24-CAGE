#include <bl31/shadow_task.h>
#include <bl31/gpu_protect.h>
#include <bl31/other_util.h>


#include <lib/mmio.h>
#include <arch.h>
#include <common/debug.h>

struct shadowtask shadowtask_total[CCAEXT_SHADOWTASK_MAX];
uint64_t curtask_realm_id=0;
uint64_t curtask_metadata_addr=0;
uint64_t curtask_codebuffer_addr[10]={0,0,0,0,0,0,0,0,0,0};
uint64_t curtask_codebuffer_size[10]={0,0,0,0,0,0,0,0,0,0};


bool check_and_tag_bitmap_realm(uint64_t realm_id, uint64_t startaddr, uint64_t required_size, bool bit){
    
    struct shadowtask *cur_shadowtask=find_realm_with_realmid(realm_id);
    uint64_t start_bit= (startaddr - cur_shadowtask->realm_startaddr) >> 12;
    uint64_t required_bits= required_size>>12;

    uint64_t cur_bit = 0;
    while (cur_bit < required_bits){
        if (cur_shadowtask->realm_bitmap_page[(cur_bit+start_bit)]==bit){
            return false;
        }
        cur_shadowtask->realm_bitmap_page[(cur_bit+start_bit)]=bit;
        cur_bit +=1;
    }
    return true;
}

uint64_t find_available_gpupte_page(struct shadowtask *cur_shadowtask){

    uint64_t failedaddr=0x0;

    uint64_t cur_bit = 0;
    while(cur_bit < ((cur_shadowtask->real_gpupte_size)>>12)){
        
        if(cur_shadowtask->real_gpupte_bitmap_page[cur_bit] == true){
            cur_shadowtask->real_gpupte_bitmap_page[cur_bit]=false;
            return (cur_shadowtask->real_gpupte_ttbr + (cur_bit << 12));
        }
        else{
            cur_bit+=1;
        }
    }
    return failedaddr;
}



uint64_t find_available_realm_region(uint64_t realm_id, uint64_t required_size){


    uint64_t failedaddr=0x0;

    struct shadowtask *cur_shadowtask=find_realm_with_realmid(realm_id);
    uint64_t required_bits= required_size>>12;

    uint64_t cur_bit = 0;
    while(cur_bit < ((cur_shadowtask->realm_size)>>12)){
        
        if(cur_shadowtask->realm_bitmap_page[cur_bit] == true){
            bool is_satisfied=true;
            uint64_t tmp_bit=0;
            while(tmp_bit<required_bits){
                if(cur_shadowtask->realm_bitmap_page[cur_bit+tmp_bit]==false){
                    is_satisfied=false;
                    break;
                }
                tmp_bit+=1;
            }
            if (is_satisfied){
                return (cur_shadowtask->realm_startaddr + (cur_bit << 12));
            }
            else{
                cur_bit+=tmp_bit;
            }



        }
        else{
            cur_bit+=1;
        }
    }
    return failedaddr;
}

void init_ccaext_shadowtask(void){    
    uint32_t cur_index=0;

	while(cur_index<CCAEXT_SHADOWTASK_MAX){
        memset(&shadowtask_total[cur_index],0,sizeof(struct shadowtask));
        shadowtask_total[cur_index].is_available=true;
        shadowtask_total[cur_index].realm_id=0;

        shadowtask_total[cur_index].realm_size=CCAEXT_REALM_SIZE;
        shadowtask_total[cur_index].realm_startaddr=0x8a0000000+cur_index*CCAEXT_REALM_SIZE;

        shadowtask_total[cur_index].real_gpupte_size=CCAEXT_REALM_SIZE;
        shadowtask_total[cur_index].real_gpupte_ttbr=0x8b0000000+cur_index*CCAEXT_REALM_SIZE;        

        int tmp_idx=0;
        while(tmp_idx<10){
            shadowtask_total[cur_index].total_dataset[tmp_idx].isused=false;
            shadowtask_total[cur_index].total_dataset[tmp_idx].stub_addr=0;
            shadowtask_total[cur_index].total_dataset[tmp_idx].real_addr=0;
            shadowtask_total[cur_index].total_dataset[tmp_idx].set_size=0;

            shadowtask_total[cur_index].total_databuffer[tmp_idx].isused=false;
            shadowtask_total[cur_index].total_databuffer[tmp_idx].stub_addr=0;
            shadowtask_total[cur_index].total_databuffer[tmp_idx].real_addr=0;
            shadowtask_total[cur_index].total_databuffer[tmp_idx].set_size=0;

            tmp_idx+=1;
        }

        int cur_bit=0;
        while(cur_bit<(shadowtask_total[cur_index].realm_size>>12)){
            shadowtask_total[cur_index].realm_bitmap_page[cur_bit]=true;
            shadowtask_total[cur_index].real_gpupte_bitmap_page[cur_bit]=true;
            cur_bit+=1;
        }
        shadowtask_total[cur_index].real_gpupte_bitmap_page[0]=false; //First page must be used

        uint64_t cur_addr=shadowtask_total[cur_index].real_gpupte_ttbr;
        while (cur_addr<(shadowtask_total[cur_index].real_gpupte_ttbr+shadowtask_total[cur_index].real_gpupte_size)){
            *(uint64_t *)cur_addr=0;
            cur_addr+=0x8;
        }
        flush_dcache_range(shadowtask_total[cur_index].real_gpupte_ttbr,shadowtask_total[cur_index].real_gpupte_size);


        shadowtask_total[cur_index].gpu_gpt_realm=cur_index*0x200000 + 0xa1000000;
        init_ccaext_gpu_gpt_realm(shadowtask_total[cur_index].gpu_gpt_realm,shadowtask_total[cur_index].realm_startaddr,shadowtask_total[cur_index].realm_size);

        cur_index+=1;
	}    
}

struct shadowtask *allocate_realm_with_realmid(uint64_t realm_id){
    uint32_t cur_index=0;
    while(cur_index<CCAEXT_SHADOWTASK_MAX){
        if (shadowtask_total[cur_index].is_available==true){
            shadowtask_total[cur_index].is_available=false;
            shadowtask_total[cur_index].realm_id=realm_id;
            return &shadowtask_total[cur_index];
        }
        cur_index+=1;
	}
    return NULL;
}

struct shadowtask *find_realm_with_realmid(uint64_t realm_id){
    uint32_t cur_index=0;
    while(cur_index<CCAEXT_SHADOWTASK_MAX){
        if (shadowtask_total[cur_index].realm_id==realm_id){
            return &shadowtask_total[cur_index];
        }
        cur_index+=1;
    }
    return NULL;
}

void destroy_realm(struct shadowtask *cur_shadowtask){

    int tmp_idx=0;
    while(tmp_idx<10){
        if(cur_shadowtask->total_dataset[tmp_idx].isused==true){
            
            uint64_t cur_start_addr=cur_shadowtask->total_dataset[tmp_idx].real_addr;
            uint64_t cur_size=cur_shadowtask->total_dataset[tmp_idx].set_size;           

            datafiller(cur_shadowtask->total_dataset[tmp_idx].real_addr,cur_shadowtask->total_dataset[tmp_idx].set_size,0);

            cur_shadowtask->total_dataset[tmp_idx].isused=false;
            cur_shadowtask->total_dataset[tmp_idx].stub_addr=0;
            cur_shadowtask->total_dataset[tmp_idx].real_addr=0;
            cur_shadowtask->total_dataset[tmp_idx].set_size=0;
            gptconfig_sublevel(0xa00a0000,cur_start_addr,cur_size-1,CCAEXT_ALL);
        }

        if(cur_shadowtask->total_databuffer[tmp_idx].isused==true){
            
            uint64_t cur_start_addr=cur_shadowtask->total_databuffer[tmp_idx].real_addr;
            uint64_t cur_size=cur_shadowtask->total_databuffer[tmp_idx].set_size;           
   
            datafiller(cur_shadowtask->total_databuffer[tmp_idx].real_addr,cur_shadowtask->total_databuffer[tmp_idx].set_size,0);
            gptconfig_sublevel(0xa00a0000,cur_start_addr,cur_size-1,CCAEXT_ALL);
            cur_shadowtask->total_databuffer[tmp_idx].isused=false;
            cur_shadowtask->total_databuffer[tmp_idx].stub_addr=0;
            cur_shadowtask->total_databuffer[tmp_idx].real_addr=0;
            cur_shadowtask->total_databuffer[tmp_idx].set_size=0;
        }


        tmp_idx+=1;
    }
    tlbipaallos();


    cur_shadowtask->is_available=true;
    cur_shadowtask->realm_id=0;

    uint64_t cur_bit=0;
    while (cur_bit< ((cur_shadowtask->realm_size)>>12)){
        cur_shadowtask->realm_bitmap_page[cur_bit]=true;
        cur_bit+=1;
    }

    datafiller(cur_shadowtask->gpu_gpt_realm,0x200000,0x0);
    cur_shadowtask->gpu_gpt_realm=0;
}


uint64_t ccaext_check_buffer(uint64_t realm_id) {


    struct shadowtask *cur_shadowtask=find_realm_with_realmid(realm_id);
    if(cur_shadowtask==NULL){
        ERROR("Illegal realm region\n");
        panic();
    }

    gptconfig_sublevel(cur_shadowtask->gpu_gpt_realm+0x20000,curtask_metadata_addr,0x1000-1,CCAEXT_ALL);

    uint64_t data_field_addr=curtask_metadata_addr;
    uint64_t code_field_addr=curtask_metadata_addr+0x100;
    
    flush_dcache_range(data_field_addr,0x100);
    flush_dcache_range(code_field_addr,0x100);


    uint64_t tmp_codebuffer_idx=0;

    for (uint64_t current_addr = code_field_addr ; current_addr < (code_field_addr+0x100) ; current_addr += 8){
        uint64_t code_buffer_phys_addr = *(uint64_t *)current_addr;
        if (code_buffer_phys_addr==0)continue;
        
        //Add it to the codebuffer set
        curtask_codebuffer_addr[tmp_codebuffer_idx]=code_buffer_phys_addr;
        curtask_codebuffer_size[tmp_codebuffer_idx]=0x1000; //size
        tmp_codebuffer_idx+=1;

        //set accessible codebuffer in GPU GPT
        gptconfig_sublevel(cur_shadowtask->gpu_gpt_realm+0x20000,code_buffer_phys_addr,0x1000-1,CCAEXT_ALL);

    }

	for (uint64_t current_addr = data_field_addr ; current_addr < (data_field_addr+0x100)  ; current_addr += 8) {
		uint64_t databuffer_stub_addr = *(uint64_t *)current_addr;
        if(databuffer_stub_addr==0)continue;

        uint64_t magic_number=*(uint64_t *)(databuffer_stub_addr);
        if(magic_number!=0xaaaabbbbccccdddd){
            ERROR("Not a confidential task\n");
            panic();
        }

        uint64_t cur_realm_id=*(uint64_t *)(databuffer_stub_addr+0x8);
        if(cur_realm_id!=realm_id){
            ERROR("Not the expected buffer\n");
            panic();
        }

        uint64_t buffer_size=*(uint64_t *)(databuffer_stub_addr+0x20);
        uint64_t databuffer_real_addr=get_databuffer_real_addr(cur_shadowtask,databuffer_stub_addr);
        
        if(databuffer_real_addr==0){
            databuffer_real_addr=find_available_realm_region(realm_id, buffer_size);
            if(databuffer_real_addr==0){
				ERROR("No available region to allocate databuffer.\n");
				panic();
			}
			record_each_databuffer(realm_id,databuffer_stub_addr,databuffer_real_addr,buffer_size);
        }

        uint64_t buffer_attr=*(uint64_t *)(databuffer_stub_addr+0x28);
        if ((buffer_attr&0x1)!=0){ 
            uint64_t dataset_stub_addr=*(uint64_t *)(databuffer_stub_addr+0x30);
            uint64_t transferred_size=*(uint64_t *)(databuffer_stub_addr+0x38);

            uint64_t dataset_real_addr=get_dataset_real_addr(cur_shadowtask,dataset_stub_addr);
            if(dataset_real_addr==0){
                ERROR("Cannot find the real_addr based on source %lx\n",dataset_stub_addr);
                panic();
            }

            fast_memcpy_addr(databuffer_real_addr, dataset_real_addr, transferred_size);
            flush_dcache_range(databuffer_real_addr,transferred_size);
            dsbsy();
        }

        datafiller(current_addr,0x8,databuffer_real_addr);
	}

    gptconfig_enable_gpu_gpt_protection(cur_shadowtask->gpu_gpt_realm);

    return realm_id;
}

void record_each_dataset(uint64_t realm_id,uint64_t stub_addr,uint64_t real_addr,uint64_t set_size){

	bool is_shadowtask_satisfied=check_and_tag_bitmap_realm(realm_id,real_addr, set_size, false);
    if (!is_shadowtask_satisfied){
        ERROR("In realm %lx, cannot record dataset real_addr %lx, size: %lx\n",realm_id,real_addr,set_size);
        panic();
    }
    struct shadowtask *cur_shadowtask=find_realm_with_realmid(realm_id);
    int tmp_idx=0;
    while(tmp_idx<10){
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

void record_each_databuffer(uint64_t realm_id,uint64_t stub_addr,uint64_t real_addr,uint64_t set_size){

	bool is_shadowtask_satisfied=check_and_tag_bitmap_realm(realm_id,real_addr, set_size, false);
    if (!is_shadowtask_satisfied){
        ERROR("In realm %lx, cannot record databuffer real_addr %lx, size: %lx\n",realm_id,real_addr,set_size);
        panic();
    }
    struct shadowtask *cur_shadowtask=find_realm_with_realmid(realm_id);
    int tmp_idx=0;
    while(tmp_idx<10){
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

void protect_total_dataset(uint64_t realm_id){
    struct shadowtask *cur_shadowtask=find_realm_with_realmid(realm_id);
    int tmp_idx=0;
    while(tmp_idx<10){
        if(cur_shadowtask->total_dataset[tmp_idx].isused==true){
            uint64_t cur_start_addr=cur_shadowtask->total_dataset[tmp_idx].real_addr;
            uint64_t cur_size=cur_shadowtask->total_dataset[tmp_idx].set_size;
            gptconfig_sublevel(0xa00a0000,cur_start_addr,cur_size-1,CCAEXT_REALM);            
        }
        tmp_idx+=1;
    }
    tlbipaallos();
}

uint64_t get_dataset_real_addr(struct shadowtask *cur_shadowtask, uint64_t stub_addr){
    int tmp_idx=0;
    while(tmp_idx<10){
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
    while(tmp_idx<10){
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
    while(tmp_idx<10){
        if(cur_shadowtask->total_databuffer[tmp_idx].isused==true){
            if (cur_shadowtask->total_databuffer[tmp_idx].real_addr==real_addr){
                return cur_shadowtask->total_databuffer[tmp_idx].stub_addr;
            }
        }
        tmp_idx+=1;
    }
    return 0;
}

void init_gpu_stubtable_structure(void){
    
    datafiller(0x890000000,0x400000,0x2);
    datafiller(0x890000000+0x400000,0x1000,0x2);
    uint64_t curaddr=0x890000000+0x400800;
    uint64_t endaddr=0x890000000+0x400c00;
    uint64_t subleveldesc=0x890000000+0x700003;
    while(curaddr<endaddr){
        datafiller(curaddr,0x8,subleveldesc);
        subleveldesc+=0x1000;
        curaddr+=0x8;
    }
    datafiller(0x890000000+0x600000,0x200000,0x2);
}

void update_gpu_table(struct shadowtask *cur_shadowtask, uint64_t req_start_ipa,uint64_t req_size,uint64_t req_start_pa,uint64_t req_attr){
    uint64_t cur_ipa=req_start_ipa;
    uint64_t table_base,offset,phys_DA;
    
    while(cur_ipa<(req_start_ipa+req_size)){

        table_base=cur_shadowtask->real_gpupte_ttbr;

        offset = get_bit_range_value(cur_ipa, 47, 39) << 3;
	    phys_DA = (table_base & 0xfffffffff000) | offset;
        table_base = *(uint64_t *)phys_DA;

        if ((table_base & 0x3) != 0x3){
            uint64_t table_addr = find_available_gpupte_page(cur_shadowtask);
            if(table_addr==0x0){
                ERROR("NO Page can be used\n");
                panic();
            }
            *(uint64_t *)phys_DA = (uint64_t)(table_addr + 0x3);

		    flush_dcache_range(phys_DA,0x8);
		    table_base = *(uint64_t *)phys_DA;
        }

        offset = get_bit_range_value(cur_ipa, 38, 30) << 3;
	    phys_DA = (table_base & 0xfffffffff000) | offset;
	    table_base = *(uint64_t *)phys_DA;

        if ((table_base & 0x3) != 0x3){
            uint64_t table_addr = find_available_gpupte_page(cur_shadowtask);
            if(table_addr==0x0){
                ERROR("NO Page can be used\n");
                panic();
            }
            *(uint64_t *)phys_DA = table_addr + 0x3;
		    flush_dcache_range(phys_DA,0x8);
		    table_base = *(uint64_t *)phys_DA;
        }

	    offset = get_bit_range_value(cur_ipa, 29, 21) << 3;
	    phys_DA = (table_base & 0xfffffffff000) | offset;
	    table_base = *(uint64_t *)phys_DA;
        if ((table_base & 0x3) != 0x3){
            uint64_t table_addr = find_available_gpupte_page(cur_shadowtask);
            if(table_addr==0x0){
                ERROR("NO Page can be used\n");
                panic();
            }
            *(uint64_t *)phys_DA = table_addr + 0x3;
		    flush_dcache_range(phys_DA,0x8);
		    table_base = *(uint64_t *)phys_DA;
        }
	    offset = get_bit_range_value(cur_ipa, 20, 12) << 3;
	    uint64_t cur_phys_DA = (table_base & 0xfffffffff000) | offset; //It is the target
        *(uint64_t *)cur_phys_DA= req_attr | cur_ipa;
        flush_dcache_range(cur_phys_DA,0x8);
        cur_ipa+=0x1000;
    }
    return;
}