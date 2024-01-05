#ifndef __ASSEMBLER__

#ifndef OTHER_UTIL_H
#define OTHER_UTIL_H



#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/plat_arm.h>

#define get_bit_range_value(number, start, end) (( (number) >> (end) ) & ( (1L << ( (start) - (end) + 1) ) - 1) )

void fast_memset(void *dst, unsigned long val, size_t size);
void fast_memcpy(void *dst, void *src, size_t size);
void fast_memcpy_addr(uint64_t dst, uint64_t src, size_t size);
void datafiller(uint64_t startaddr, uint64_t size, uint64_t val);
uint64_t read_ttbr_core(uint64_t IA, uint64_t this_mmu_pgd) ;
#endif
#endif
