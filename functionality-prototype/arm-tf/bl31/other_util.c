#include <bl31/other_util.h>

#include <lib/mmio.h>
#include <arch.h>
#include <common/debug.h>


void fast_memset(void *dst, unsigned long val, size_t size) {
	size_t _size = size / 64 * 64;
	for (unsigned long addr = (unsigned long)dst ; addr < (unsigned long)dst + _size ; addr += 64) {
		((unsigned long*)addr)[0] = val; ((unsigned long*)addr)[1] = val; ((unsigned long*)addr)[2] = val; ((unsigned long*)addr)[3] = val;
		((unsigned long*)addr)[4] = val; ((unsigned long*)addr)[5] = val; ((unsigned long*)addr)[6] = val; ((unsigned long*)addr)[7] = val;
	}
	memset(dst+_size, val, size-_size);
}
void fast_memcpy(void *dst, void *src, size_t size) {
	for (long long remain = size ; remain-32 >= 0 ; dst += 32, src += 32, remain -= 32) {
		((unsigned long*)dst)[0] = ((unsigned long*)src)[0]; ((unsigned long*)dst)[1] = ((unsigned long*)src)[1];
		((unsigned long*)dst)[2] = ((unsigned long*)src)[2]; ((unsigned long*)dst)[3] = ((unsigned long*)src)[3];
	}
}

void fast_memcpy_addr(uint64_t dst, uint64_t src, size_t size) {

	uint64_t cntsize=0;
	while(cntsize<size){
		*(uint64_t*)(dst+cntsize) = *(uint64_t*)(src+cntsize);
		cntsize+=8;
	}
	flush_dcache_range(dst,size);
}

void datafiller(uint64_t startaddr, uint64_t size, uint64_t val){
	if((size&0x7)!=0){
        ERROR("Input size unaligned\n");
        panic();
    }
    uint64_t curaddr=startaddr;
    uint64_t finaladdr=startaddr+size;
    while(curaddr<finaladdr){
		asm volatile("str %0,[%1]\n"::"r"(val),"r"(curaddr):);
		curaddr+=0x8;
	}

    flush_dcache_range(startaddr,size);
}


uint64_t read_ttbr_core(uint64_t IA, uint64_t this_mmu_pgd) {
	uint64_t offset, phys_DA, table_base, OA;
	table_base = this_mmu_pgd;
	offset = get_bit_range_value(IA, 47, 39) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;

	table_base = *(uint64_t*)phys_DA;

	if ((table_base & 0x1) == 0) return 0;
	offset = get_bit_range_value(IA, 38, 30) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;

	table_base = *(uint64_t*)phys_DA;

	if ((table_base & 0x1) == 0) return 0;
	offset = get_bit_range_value(IA, 29, 21) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;

	table_base = *(uint64_t*)phys_DA;

	if ((table_base & 0x1) == 0) return 0;
	offset = get_bit_range_value(IA, 20, 12) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;

	table_base = *(uint64_t*)phys_DA;

	if ((table_base & 0x1) == 0) return 0;
	offset = IA & 0xfff;
	OA = (table_base & 0xfffffffff000) | offset;
	
	return OA;
}
