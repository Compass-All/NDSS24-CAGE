#include <bl31/gpu_protect.h>
#include <bl31/other_util.h>
#include <bl31/shadow_task.h>
#include <drivers/arm/smmu_v3.h>

#include <lib/mmio.h>
#include <arch.h>
#include <common/debug.h>




static uint64_t ccaext_gpt_noaccess_filler[136]=
{0xfffffffffffffff0,0xffffffffffffff00,0xfffffffffffff000,0xffffffffffff0000,0xfffffffffff00000,0xffffffffff000000,0xfffffffff0000000,0xffffffff00000000,0xfffffff000000000,0xffffff0000000000,0xfffff00000000000,0xffff000000000000,0xfff0000000000000,0xff00000000000000,0xf000000000000000,0x0000000000000000,0xffffffffffffff0f,0xfffffffffffff00f,0xffffffffffff000f,0xfffffffffff0000f,0xffffffffff00000f,0xfffffffff000000f,0xffffffff0000000f,0xfffffff00000000f,0xffffff000000000f,0xfffff0000000000f,0xffff00000000000f,0xfff000000000000f,0xff0000000000000f,0xf00000000000000f,0x000000000000000f,0xfffffffffffff0ff,0xffffffffffff00ff,0xfffffffffff000ff,0xffffffffff0000ff,0xfffffffff00000ff,0xffffffff000000ff,0xfffffff0000000ff,0xffffff00000000ff,0xfffff000000000ff,0xffff0000000000ff,0xfff00000000000ff,0xff000000000000ff,0xf0000000000000ff,0x00000000000000ff,0xffffffffffff0fff,0xfffffffffff00fff,0xffffffffff000fff,0xfffffffff0000fff,0xffffffff00000fff,0xfffffff000000fff,0xffffff0000000fff,0xfffff00000000fff,0xffff000000000fff,0xfff0000000000fff,0xff00000000000fff,0xf000000000000fff,0x0000000000000fff,0xfffffffffff0ffff,0xffffffffff00ffff,0xfffffffff000ffff,0xffffffff0000ffff,0xfffffff00000ffff,0xffffff000000ffff,0xfffff0000000ffff,0xffff00000000ffff,0xfff000000000ffff,0xff0000000000ffff,0xf00000000000ffff,0x000000000000ffff,0xffffffffff0fffff,0xfffffffff00fffff,0xffffffff000fffff,0xfffffff0000fffff,0xffffff00000fffff,0xfffff000000fffff,0xffff0000000fffff,0xfff00000000fffff,0xff000000000fffff,0xf0000000000fffff,0x00000000000fffff,0xfffffffff0ffffff,0xffffffff00ffffff,0xfffffff000ffffff,0xffffff0000ffffff,0xfffff00000ffffff,0xffff000000ffffff,0xfff0000000ffffff,0xff00000000ffffff,0xf000000000ffffff,0x0000000000ffffff,0xffffffff0fffffff,0xfffffff00fffffff,0xffffff000fffffff,0xfffff0000fffffff,0xffff00000fffffff,0xfff000000fffffff,0xff0000000fffffff,0xf00000000fffffff,0x000000000fffffff,0xfffffff0ffffffff,0xffffff00ffffffff,0xfffff000ffffffff,0xffff0000ffffffff,0xfff00000ffffffff,0xff000000ffffffff,0xf0000000ffffffff,0x00000000ffffffff,0xffffff0fffffffff,0xfffff00fffffffff,0xffff000fffffffff,0xfff0000fffffffff,0xff00000fffffffff,0xf000000fffffffff,0x0000000fffffffff,0xfffff0ffffffffff,0xffff00ffffffffff,0xfff000ffffffffff,0xff0000ffffffffff,0xf00000ffffffffff,0x000000ffffffffff,0xffff0fffffffffff,0xfff00fffffffffff,0xff000fffffffffff,0xf0000fffffffffff,0x00000fffffffffff,0xfff0ffffffffffff,0xff00ffffffffffff,0xf000ffffffffffff,0x0000ffffffffffff,0xff0fffffffffffff,0xf00fffffffffffff,0x000fffffffffffff,0xf0ffffffffffffff,0x00ffffffffffffff,0x0fffffffffffffff};

static uint64_t ccaext_gpt_all_filler[136]=
{0x000000000000000f,0x00000000000000ff,0x0000000000000fff,0x000000000000ffff,0x00000000000fffff,0x0000000000ffffff,0x000000000fffffff,0x00000000ffffffff,0x0000000fffffffff,0x000000ffffffffff,0x00000fffffffffff,0x0000ffffffffffff,0x000fffffffffffff,0x00ffffffffffffff,0x0fffffffffffffff,0xffffffffffffffff,0x00000000000000f0,0x0000000000000ff0,0x000000000000fff0,0x00000000000ffff0,0x0000000000fffff0,0x000000000ffffff0,0x00000000fffffff0,0x0000000ffffffff0,0x000000fffffffff0,0x00000ffffffffff0,0x0000fffffffffff0,0x000ffffffffffff0,0x00fffffffffffff0,0x0ffffffffffffff0,0xfffffffffffffff0,0x0000000000000f00,0x000000000000ff00,0x00000000000fff00,0x0000000000ffff00,0x000000000fffff00,0x00000000ffffff00,0x0000000fffffff00,0x000000ffffffff00,0x00000fffffffff00,0x0000ffffffffff00,0x000fffffffffff00,0x00ffffffffffff00,0x0fffffffffffff00,0xffffffffffffff00,0x000000000000f000,0x00000000000ff000,0x0000000000fff000,0x000000000ffff000,0x00000000fffff000,0x0000000ffffff000,0x000000fffffff000,0x00000ffffffff000,0x0000fffffffff000,0x000ffffffffff000,0x00fffffffffff000,0x0ffffffffffff000,0xfffffffffffff000,0x00000000000f0000,0x0000000000ff0000,0x000000000fff0000,0x00000000ffff0000,0x0000000fffff0000,0x000000ffffff0000,0x00000fffffff0000,0x0000ffffffff0000,0x000fffffffff0000,0x00ffffffffff0000,0x0fffffffffff0000,0xffffffffffff0000,0x0000000000f00000,0x000000000ff00000,0x00000000fff00000,0x0000000ffff00000,0x000000fffff00000,0x00000ffffff00000,0x0000fffffff00000,0x000ffffffff00000,0x00fffffffff00000,0x0ffffffffff00000,0xfffffffffff00000,0x000000000f000000,0x00000000ff000000,0x0000000fff000000,0x000000ffff000000,0x00000fffff000000,0x0000ffffff000000,0x000fffffff000000,0x00ffffffff000000,0x0fffffffff000000,0xffffffffff000000,0x00000000f0000000,0x0000000ff0000000,0x000000fff0000000,0x00000ffff0000000,0x0000fffff0000000,0x000ffffff0000000,0x00fffffff0000000,0x0ffffffff0000000,0xfffffffff0000000,0x0000000f00000000,0x000000ff00000000,0x00000fff00000000,0x0000ffff00000000,0x000fffff00000000,0x00ffffff00000000,0x0fffffff00000000,0xffffffff00000000,0x000000f000000000,0x00000ff000000000,0x0000fff000000000,0x000ffff000000000,0x00fffff000000000,0x0ffffff000000000,0xfffffff000000000,0x00000f0000000000,0x0000ff0000000000,0x000fff0000000000,0x00ffff0000000000,0x0fffff0000000000,0xffffff0000000000,0x0000f00000000000,0x000ff00000000000,0x00fff00000000000,0x0ffff00000000000,0xfffff00000000000,0x000f000000000000,0x00ff000000000000,0x0fff000000000000,0xffff000000000000,0x00f0000000000000,0x0ff0000000000000,0xfff0000000000000,0x0f00000000000000,0xff00000000000000,0xf000000000000000};

static uint64_t ccaext_gpt_root_filler[136]=
{0x000000000000000a,0x00000000000000aa,0x0000000000000aaa,0x000000000000aaaa,0x00000000000aaaaa,0x0000000000aaaaaa,0x000000000aaaaaaa,0x00000000aaaaaaaa,0x0000000aaaaaaaaa,0x000000aaaaaaaaaa,0x00000aaaaaaaaaaa,0x0000aaaaaaaaaaaa,0x000aaaaaaaaaaaaa,0x00aaaaaaaaaaaaaa,0x0aaaaaaaaaaaaaaa,0xaaaaaaaaaaaaaaaa,0x00000000000000a0,0x0000000000000aa0,0x000000000000aaa0,0x00000000000aaaa0,0x0000000000aaaaa0,0x000000000aaaaaa0,0x00000000aaaaaaa0,0x0000000aaaaaaaa0,0x000000aaaaaaaaa0,0x00000aaaaaaaaaa0,0x0000aaaaaaaaaaa0,0x000aaaaaaaaaaaa0,0x00aaaaaaaaaaaaa0,0x0aaaaaaaaaaaaaa0,0xaaaaaaaaaaaaaaa0,0x0000000000000a00,0x000000000000aa00,0x00000000000aaa00,0x0000000000aaaa00,0x000000000aaaaa00,0x00000000aaaaaa00,0x0000000aaaaaaa00,0x000000aaaaaaaa00,0x00000aaaaaaaaa00,0x0000aaaaaaaaaa00,0x000aaaaaaaaaaa00,0x00aaaaaaaaaaaa00,0x0aaaaaaaaaaaaa00,0xaaaaaaaaaaaaaa00,0x000000000000a000,0x00000000000aa000,0x0000000000aaa000,0x000000000aaaa000,0x00000000aaaaa000,0x0000000aaaaaa000,0x000000aaaaaaa000,0x00000aaaaaaaa000,0x0000aaaaaaaaa000,0x000aaaaaaaaaa000,0x00aaaaaaaaaaa000,0x0aaaaaaaaaaaa000,0xaaaaaaaaaaaaa000,0x00000000000a0000,0x0000000000aa0000,0x000000000aaa0000,0x00000000aaaa0000,0x0000000aaaaa0000,0x000000aaaaaa0000,0x00000aaaaaaa0000,0x0000aaaaaaaa0000,0x000aaaaaaaaa0000,0x00aaaaaaaaaa0000,0x0aaaaaaaaaaa0000,0xaaaaaaaaaaaa0000,0x0000000000a00000,0x000000000aa00000,0x00000000aaa00000,0x0000000aaaa00000,0x000000aaaaa00000,0x00000aaaaaa00000,0x0000aaaaaaa00000,0x000aaaaaaaa00000,0x00aaaaaaaaa00000,0x0aaaaaaaaaa00000,0xaaaaaaaaaaa00000,0x000000000a000000,0x00000000aa000000,0x0000000aaa000000,0x000000aaaa000000,0x00000aaaaa000000,0x0000aaaaaa000000,0x000aaaaaaa000000,0x00aaaaaaaa000000,0x0aaaaaaaaa000000,0xaaaaaaaaaa000000,0x00000000a0000000,0x0000000aa0000000,0x000000aaa0000000,0x00000aaaa0000000,0x0000aaaaa0000000,0x000aaaaaa0000000,0x00aaaaaaa0000000,0x0aaaaaaaa0000000,0xaaaaaaaaa0000000,0x0000000a00000000,0x000000aa00000000,0x00000aaa00000000,0x0000aaaa00000000,0x000aaaaa00000000,0x00aaaaaa00000000,0x0aaaaaaa00000000,0xaaaaaaaa00000000,0x000000a000000000,0x00000aa000000000,0x0000aaa000000000,0x000aaaa000000000,0x00aaaaa000000000,0x0aaaaaa000000000,0xaaaaaaa000000000,0x00000a0000000000,0x0000aa0000000000,0x000aaa0000000000,0x00aaaa0000000000,0x0aaaaa0000000000,0xaaaaaa0000000000,0x0000a00000000000,0x000aa00000000000,0x00aaa00000000000,0x0aaaa00000000000,0xaaaaa00000000000,0x000a000000000000,0x00aa000000000000,0x0aaa000000000000,0xaaaa000000000000,0x00a0000000000000,0x0aa0000000000000,0xaaa0000000000000,0x0a00000000000000,0xaa00000000000000,0xa000000000000000};

static uint64_t ccaext_gpt_realm_filler[136]=
{0x000000000000000b,0x00000000000000bb,0x0000000000000bbb,0x000000000000bbbb,0x00000000000bbbbb,0x0000000000bbbbbb,0x000000000bbbbbbb,0x00000000bbbbbbbb,0x0000000bbbbbbbbb,0x000000bbbbbbbbbb,0x00000bbbbbbbbbbb,0x0000bbbbbbbbbbbb,0x000bbbbbbbbbbbbb,0x00bbbbbbbbbbbbbb,0x0bbbbbbbbbbbbbbb,0xbbbbbbbbbbbbbbbb,0x00000000000000b0,0x0000000000000bb0,0x000000000000bbb0,0x00000000000bbbb0,0x0000000000bbbbb0,0x000000000bbbbbb0,0x00000000bbbbbbb0,0x0000000bbbbbbbb0,0x000000bbbbbbbbb0,0x00000bbbbbbbbbb0,0x0000bbbbbbbbbbb0,0x000bbbbbbbbbbbb0,0x00bbbbbbbbbbbbb0,0x0bbbbbbbbbbbbbb0,0xbbbbbbbbbbbbbbb0,0x0000000000000b00,0x000000000000bb00,0x00000000000bbb00,0x0000000000bbbb00,0x000000000bbbbb00,0x00000000bbbbbb00,0x0000000bbbbbbb00,0x000000bbbbbbbb00,0x00000bbbbbbbbb00,0x0000bbbbbbbbbb00,0x000bbbbbbbbbbb00,0x00bbbbbbbbbbbb00,0x0bbbbbbbbbbbbb00,0xbbbbbbbbbbbbbb00,0x000000000000b000,0x00000000000bb000,0x0000000000bbb000,0x000000000bbbb000,0x00000000bbbbb000,0x0000000bbbbbb000,0x000000bbbbbbb000,0x00000bbbbbbbb000,0x0000bbbbbbbbb000,0x000bbbbbbbbbb000,0x00bbbbbbbbbbb000,0x0bbbbbbbbbbbb000,0xbbbbbbbbbbbbb000,0x00000000000b0000,0x0000000000bb0000,0x000000000bbb0000,0x00000000bbbb0000,0x0000000bbbbb0000,0x000000bbbbbb0000,0x00000bbbbbbb0000,0x0000bbbbbbbb0000,0x000bbbbbbbbb0000,0x00bbbbbbbbbb0000,0x0bbbbbbbbbbb0000,0xbbbbbbbbbbbb0000,0x0000000000b00000,0x000000000bb00000,0x00000000bbb00000,0x0000000bbbb00000,0x000000bbbbb00000,0x00000bbbbbb00000,0x0000bbbbbbb00000,0x000bbbbbbbb00000,0x00bbbbbbbbb00000,0x0bbbbbbbbbb00000,0xbbbbbbbbbbb00000,0x000000000b000000,0x00000000bb000000,0x0000000bbb000000,0x000000bbbb000000,0x00000bbbbb000000,0x0000bbbbbb000000,0x000bbbbbbb000000,0x00bbbbbbbb000000,0x0bbbbbbbbb000000,0xbbbbbbbbbb000000,0x00000000b0000000,0x0000000bb0000000,0x000000bbb0000000,0x00000bbbb0000000,0x0000bbbbb0000000,0x000bbbbbb0000000,0x00bbbbbbb0000000,0x0bbbbbbbb0000000,0xbbbbbbbbb0000000,0x0000000b00000000,0x000000bb00000000,0x00000bbb00000000,0x0000bbbb00000000,0x000bbbbb00000000,0x00bbbbbb00000000,0x0bbbbbbb00000000,0xbbbbbbbb00000000,0x000000b000000000,0x00000bb000000000,0x0000bbb000000000,0x000bbbb000000000,0x00bbbbb000000000,0x0bbbbbb000000000,0xbbbbbbb000000000,0x00000b0000000000,0x0000bb0000000000,0x000bbb0000000000,0x00bbbb0000000000,0x0bbbbb0000000000,0xbbbbbb0000000000,0x0000b00000000000,0x000bb00000000000,0x00bbb00000000000,0x0bbbb00000000000,0xbbbbb00000000000,0x000b000000000000,0x00bb000000000000,0x0bbb000000000000,0xbbbb000000000000,0x00b0000000000000,0x0bb0000000000000,0xbbb0000000000000,0x0b00000000000000,0xbb00000000000000,0xb000000000000000};



int init_ccaext_periph_gpt(void){
	uint64_t new_gpt_root=0xa0200000;

    // configure 4GB regions
    datafiller(new_gpt_root, 0x1000, 0xf1);

	datafiller(new_gpt_root, 0x8, new_gpt_root+0x20003);
	datafiller(new_gpt_root+0x8, 0x8, new_gpt_root+0x40003);
	datafiller(new_gpt_root+0x10, 0x8, new_gpt_root+0x60003);
	datafiller(new_gpt_root+0x18, 0x8, new_gpt_root+0x80003);

	datafiller(new_gpt_root+0x20000, 0x80000, 0xffffffffffffffff); 
	datafiller(new_gpt_root+0x9f800, 0x800, 0xbbbbbbbbbbbbbbbb); //Some Realm Regions in TF-A prototype
	datafiller(new_gpt_root+0x9ff60, 0xa0, 0xaaaaaaaaaaaaaaaa);	//Some Root Regions in TF-A prototype

	/*Challenge Implementation: 
	* To configure 0x8_8000_0000 - 0x8_c000_0000,
	* it INITIALLY follows the configuration in CPU GPT, but can be changed.
	*/
	datafiller(new_gpt_root+0x110,0x8,0xa0000000+0xa0003);

	//GPU MMIO, 0x2d00_0000 - 0x2d00_ffff, (configured as Accessible)
	datafiller(new_gpt_root+0x36800 ,0x8, 0xffffffffffffffff);

	//GPT region, 0xa000_0000 - 0xa200_0000 (configured as No access)
	datafiller(new_gpt_root+0x70000,0x1000,0x0);

	dsbishst();
	isb();
	NOTICE("CCAEXT Periph SMMU GPT Established\n");
	return 0;
}

int init_ccaext_cpu_gpt(void){
	uint64_t new_gpt_root=0xa0000000;    
	
	// configure 4GB regions
	datafiller(new_gpt_root, 0x1000, 0xf1);
	datafiller(new_gpt_root, 0x8, new_gpt_root+0x20003);
	datafiller(new_gpt_root+0x8, 0x8, new_gpt_root+0x40003);
	datafiller(new_gpt_root+0x10, 0x8, new_gpt_root+0x60003);
	datafiller(new_gpt_root+0x18, 0x8, new_gpt_root+0x80003);

	datafiller(new_gpt_root+0x20000, 0xa0000, 0xffffffffffffffff); 
	datafiller(new_gpt_root+0x9f800, 0x800, 0xbbbbbbbbbbbbbbbb); //Some Realm Regions in TF-A prototype
	datafiller(new_gpt_root+0x9ff60, 0xa0, 0xaaaaaaaaaaaaaaaa);	//Some Root Regions in TF-A prototype
	
	/*Challenge Implementation: 
	* To configure 0x8_8000_0000 - 0x8_c000_0000
	*/
	datafiller(new_gpt_root+0x110,0x8,new_gpt_root+0xa0003);
	datafiller(new_gpt_root+0xb0000,0x10000,0xbbbbbbbbbbbbbbbb);
	//datafiller(new_gpt_root+0xb8000,0x8000,0xffffffffffffffff);

	//GPT region, 0xa000_0000 - 0xa200_0000 (configured as Root)
	datafiller(new_gpt_root+0x70000,0x1000,0xaaaaaaaaaaaaaaaa);

	dsbishst();
	tlbipaallos();
	dsb();
	isb();
	NOTICE("CCAEXT GPT Memory Established\n");
	return 0;
}

int init_ccaext_gpu_gpt_realm(uint64_t new_gpt_root, uint64_t realm_startaddr, uint64_t realm_size){
		
	//Only allow the GPU GPT to access its own realm
	datafiller(new_gpt_root, 0x20000, 0xf1);
	
	datafiller(new_gpt_root+0x110,0x8, new_gpt_root+0x20003); //0x8_8000_0000 - 0x8_c000_0000
	datafiller(new_gpt_root+0x20000,0x20000, 0x0); //initially it is all non-access

	//Next configure realm area (assume it is physically continuous) as ALL
	gptconfig_sublevel((new_gpt_root+0x20000), realm_startaddr,realm_size-1,CCAEXT_ALL);
	gptconfig_sublevel((new_gpt_root+0x20000), 0x8b0000000,0x01000000-1,CCAEXT_ALL);

	// datafiller(new_gpt_root+0x0,0x8, new_gpt_root+0x40003); //0x0000_0000 - 0x4000_0000
	// gptconfig_sublevel((new_gpt_root+0x40000), 0x06041000,0x00020000,CCAEXT_ALL);
	// gptconfig_sublevel((new_gpt_root+0x40000), 0x07000000,0x01000000,CCAEXT_ALL);


	
	return 0;
}

//Configure 1GB region in GPT, and this region is continuous.
void gptconfig_sublevel(uint64_t subgpt_addr, uint64_t start_addr, uint64_t start_size, uint32_t target_attr){
	

	NOTICE("Change the sublevel %lx, region (%lx -- %lx) as %x\n",subgpt_addr, start_addr, (start_addr+start_size),target_attr);


	//where this is belongs to?
	uint64_t end_addr=start_addr+start_size;
	uint64_t filtered_start_addr=start_addr & 0x3fffffff;
	uint64_t filtered_end_addr= end_addr & 0x3fffffff;
	uint64_t filtered_end_addr_largepage=filtered_end_addr & 0x3fff0000;
	uint64_t cur_filtered_start_addr=filtered_start_addr;
	
	uint64_t leftidx=(cur_filtered_start_addr & 0xf000)>>12;//get initial start idx with its offs
	uint64_t rightidx=0;

	while (cur_filtered_start_addr<filtered_end_addr){
				
		uint64_t cur_filtered_start_addr_largepage=cur_filtered_start_addr & 0x3fff0000;
		uint64_t cur_subgpt_desc_addr= subgpt_addr + (cur_filtered_start_addr_largepage >>13);
		uint64_t cur_subgpt_desc_val= *(uint64_t*)cur_subgpt_desc_addr;


		if (cur_filtered_start_addr_largepage != filtered_end_addr_largepage){
			
			//not in the same largepage, fill it.
			rightidx=15;
			int cur_filter_idx=find_mask_idx(leftidx,rightidx);
			uint64_t cur_tmp_val=cur_subgpt_desc_val & ccaext_gpt_noaccess_filler[cur_filter_idx];
			uint64_t final_subgpt_desc_val=0;
			if (target_attr==CCAEXT_INVAL){
				final_subgpt_desc_val=cur_tmp_val;
			}
			else if(target_attr==CCAEXT_ALL){
				final_subgpt_desc_val = cur_tmp_val | ccaext_gpt_all_filler[cur_filter_idx];
			}
			else if(target_attr==CCAEXT_ROOT){
				final_subgpt_desc_val = cur_tmp_val | ccaext_gpt_root_filler[cur_filter_idx];
			}
			else if(target_attr==CCAEXT_REALM){
				final_subgpt_desc_val = cur_tmp_val | ccaext_gpt_realm_filler[cur_filter_idx];
			}
			datafiller(cur_subgpt_desc_addr,0x8,final_subgpt_desc_val);
			cur_filtered_start_addr+=0x10000;
			leftidx=0;
		}
		else{
			rightidx=(filtered_end_addr & 0xf000)>>12;
			int cur_filter_idx=find_mask_idx(leftidx,rightidx);
			uint64_t cur_tmp_val=cur_subgpt_desc_val & ccaext_gpt_noaccess_filler[cur_filter_idx];
			uint64_t final_subgpt_desc_val=0;
			if (target_attr==CCAEXT_INVAL){
				final_subgpt_desc_val=cur_tmp_val;
			}
			else if(target_attr==CCAEXT_ALL){
				final_subgpt_desc_val = cur_tmp_val | ccaext_gpt_all_filler[cur_filter_idx];
			}
			else if(target_attr==CCAEXT_ROOT){
				final_subgpt_desc_val = cur_tmp_val | ccaext_gpt_root_filler[cur_filter_idx];
			}
			else if(target_attr==CCAEXT_REALM){
				final_subgpt_desc_val = cur_tmp_val | ccaext_gpt_realm_filler[cur_filter_idx];
			}
			datafiller(cur_subgpt_desc_addr,0x8,final_subgpt_desc_val);

			cur_filtered_start_addr=filtered_end_addr;
		}
		cur_subgpt_desc_val= *(uint64_t*)cur_subgpt_desc_addr;
	}
}




int find_mask_idx(int leftidx,int rightidx){
	int remainidx=rightidx-leftidx;
	if(leftidx!=0) {
		int prevleft=leftidx-1;
		int prevallidx= ((32 - prevleft) * leftidx) >>1;
		return prevallidx+remainidx;
	}
	else return remainidx;
}

void gptconfig_page(uint64_t root_addr,uint64_t physaddr,uint32_t attr){

	uint64_t aligned_physaddr= (physaddr >> 12)<<12;
	uint64_t lv0descaddr=root_addr+((aligned_physaddr>>30)*8);
	uint64_t lv0descval=0;
	asm volatile("ldr %0,[%1]\n":"=r"(lv0descval):"r"(lv0descaddr):);
	if ((lv0descval&0x3)==0x3){
		//it is a table
		uint64_t lv1tableaddr=(lv0descval&(uint64_t)(0xffffff000));
		uint64_t lv1tmpnum=((aligned_physaddr&(uint64_t)(0x3fffffff))>>12); //1GB
		uint64_t lv1descaddr= (lv1tmpnum>>4)*8+lv1tableaddr;
		uint64_t lv1pos=(lv1tmpnum&0xf);
		uint64_t lv1posmask=0xffffffffffffffff-((uint64_t)(0xf)<<(lv1pos*4));

		//replace the specific pos.
		uint64_t prevlv1descval=0;
		asm volatile("ldr %0,[%1]\n":"=r"(prevlv1descval):"r"(lv1descaddr):);
		uint64_t marked=prevlv1descval&lv1posmask;
		uint64_t finalattr=((uint64_t)attr)<<(lv1pos*4);	
		uint64_t newlv1descval=marked+finalattr;
		asm volatile("str %0,[%1]\n"::"r"(newlv1descval),"r"(lv1descaddr):);
		flush_dcache_range(lv1descaddr,0x8);
	}
	else {
        ERROR("Unexpected condition\n");
		panic();
	}
}

int ccaext_gpt_enable(void){
	
	//CPU
	write_gptbr_el3(0xa0000000>>12);
	write_gpccr_el3(0x13501);
	

	isb();
	tlbipaallos();
	dsb();
	isb();
	return 0;
}

void enable_code_metadata_protect(void){

	uint64_t cpugpt_root_addr=0xa0000000;
	uint64_t periphgpt_root_addr=0xa0200000;

	uint64_t cur_idx=0;
	while(cur_idx<10){
		uint64_t cur_codebuffer_addr=curtask_codebuffer_addr[cur_idx];
		uint64_t cur_codebuffer_size=curtask_codebuffer_size[cur_idx];
		if ((cur_codebuffer_addr!=0)&&(cur_codebuffer_size!=0)){
			uint64_t cur_size=0;
			while(cur_size<cur_codebuffer_size){
				gptconfig_page(cpugpt_root_addr,cur_codebuffer_addr+cur_size,CCAEXT_REALM);
				gptconfig_page(periphgpt_root_addr,cur_codebuffer_addr+cur_size,CCAEXT_REALM);
				cur_size+=0x1000;
			}
		}
		cur_idx+=1;
	}

	gptconfig_page(cpugpt_root_addr,curtask_metadata_addr,CCAEXT_REALM);
	gptconfig_page(periphgpt_root_addr,curtask_metadata_addr,CCAEXT_REALM);

	tlbipaallos();
	mmio_write_64(0x2b400000 + SMMU_ROOT_TLBI,0x1);

}

void disable_code_metadata_protect(void){

	uint64_t cpugpt_root_addr=0xa0000000;
	uint64_t periphgpt_root_addr=0xa0200000;

	uint64_t cur_idx=0;
	while(cur_idx<10){
		uint64_t cur_codebuffer_addr=curtask_codebuffer_addr[cur_idx];
		uint64_t cur_codebuffer_size=curtask_codebuffer_size[cur_idx];
		if ((cur_codebuffer_addr!=0)&&(cur_codebuffer_size!=0)){
			uint64_t cur_size=0;
			while(cur_size<cur_codebuffer_size){
				gptconfig_page(cpugpt_root_addr,cur_codebuffer_addr+cur_size,CCAEXT_ALL);
				gptconfig_page(periphgpt_root_addr,cur_codebuffer_addr+cur_size,CCAEXT_ALL);
				cur_size+=0x1000;
			}
		}
		cur_idx+=1;
	}

	gptconfig_page(cpugpt_root_addr,curtask_metadata_addr,CCAEXT_REALM);
	gptconfig_page(periphgpt_root_addr,curtask_metadata_addr,CCAEXT_REALM);

	tlbipaallos();
	mmio_write_64(0x2b400000 + SMMU_ROOT_TLBI,0x1);

}


void disable_gpummio_protect(void){

	uint64_t cpugpt_root_addr=0xa0000000;
	gptconfig_page(cpugpt_root_addr,0x2d000000,CCAEXT_ALL);
	tlbipaallos();

	uint64_t periphgpt_root_addr=0xa0200000;
	gptconfig_page(periphgpt_root_addr,0x2d000000,CCAEXT_ALL);
	mmio_write_64(0x2b400000 + SMMU_ROOT_TLBI,0x1);


}

void enable_gpummio_protect(void){

	uint64_t cpugpt_root_addr=0xa0000000;
	gptconfig_page(cpugpt_root_addr,0x2d000000,CCAEXT_REALM);
	tlbipaallos();

	uint64_t periphgpt_root_addr=0xa0200000;
	gptconfig_page(periphgpt_root_addr,0x2d000000,CCAEXT_REALM);
	mmio_write_64(0x2b400000 + SMMU_ROOT_TLBI,0x1);

}



void gptconfig_enable_gpu_gpt_protection(uint64_t gpu_gpt_realm){
	
	//Load the realm GPU GPT
	mmio_write_64(0x2b400000 + SMMU_ROOT_GPT_BASE,gpu_gpt_realm);
	//flush ALL GPU GPC TLB
	mmio_write_64(0x2b400000 + SMMU_ROOT_TLBI,0x1);

	tlb_wait();
}

void tlb_wait(void){

	uint64_t attempts=0;
	uint32_t status;
	while (attempts++ < 5000) {
		status = mmio_read_32((uintptr_t)((uint8_t *)0x2b400000 + SMMU_ROOT_TLBI_CTRL));
		if (status == 0x0) {
			break;
		} 
	}

	if (attempts == 5000) {
		ERROR("TLB Flush: Test timeout\n");
		panic();
	}

}

void gptconfig_disable_gpu_gpt_protection(void){
	//Load the untrusted periph GPU GPT
	mmio_write_64(0x2b400000 + SMMU_ROOT_GPT_BASE,0xa0200000);
	//flush ALL GPU GPC TLB
	mmio_write_64(0x2b400000 + SMMU_ROOT_TLBI,0x1);

	tlb_wait();
}



void configure_smmuv3(void){


	// 	smmuv3_disabled_translation(smmuv3);

	// /*
	//  * Disable SMMU using SMMU_(S)_CR0.SMMUEN bit. This is necessary
	//  * for driver to configure SMMU.
	//  */
	// uint32_t data_cr0;
	// uint32_t offset_cr0 = find_offset(S_CR0, CR0);
	// uint32_t offset_cr0_ack = find_offset(S_CR0_ACK, CR0_ACK);

	// dlog_verbose("SMMUv3: write to (S_)CR0\n");

	// data_cr0 = mmio_read32_offset(smmuv3->base_addr, offset_cr0);
	// data_cr0 = data_cr0 & SMMUEN_CLR_MASK;
	// mmio_write32_offset(smmuv3->base_addr, offset_cr0, data_cr0);

	// if (!smmuv3_poll(smmuv3->base_addr, offset_cr0_ack, data_cr0,
	// 		 SMMUEN_MASK)) {
	// 	dlog_error("SMMUv3: Failed to disable SMMU\n");
	// 	return false;
	// }

	// return true;
}
