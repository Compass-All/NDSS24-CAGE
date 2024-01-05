#ifndef __ASSEMBLER__

#ifndef OTHER_UTIL_H
#define OTHER_UTIL_H
#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/plat_arm.h>

#define get_bit_range_value(number, start, end) (( (number) >> (end) ) & ( (1L << ( (start) - (end) + 1) ) - 1) )

#define GPU_MMIO_BASE (void*)0x2d000000
#define JOB_SLOT0 0x800
#define JS_COMMAND_START 0x01
#define JS_STATUS 0x24	/* (RO) Status register for job slot n */
#define JS_COMMAND_NEXT 0x60
#define JS_HEAD_NEXT_LO 0x40 /* (RW) Next job queue head pointer for job slot n, low word */
#define JS_HEAD_NEXT_HI 0x44 /* (RW) Next job queue head pointer for job slot n, high word */
#define JS_CONFIG_NEXT 0x58 /* (RW) Next configuration settings for job slot n */
#define JOB_CONTROL_BASE  0x1000
#define JOB_CONTROL_REG(r) (JOB_CONTROL_BASE + (r))
#define JOB_SLOT_REG(n, r) (JOB_CONTROL_REG(JOB_SLOT0 + ((n) << 7)) + (r))


#define MEMORY_MANAGEMENT_BASE  0x2000
#define MMU_REG(r)              (MEMORY_MANAGEMENT_BASE + (r))
#define MMU_AS0                 0x400	/* Configuration registers for address space 0 */
#define MMU_AS_REG(n, r)        (MMU_REG(MMU_AS0 + ((n) << 6)) + (r))
#define AS_TRANSTAB_LO         0x00	/* (RW) Translation Table Base Address for address space n, low word */
#define AS_TRANSTAB_HI         0x04	/* (RW) Translation Table Base Address for address space n, high word */
#define AS_COMMAND             0x18	/* (WO) MMU command register for address space n */

#define AS_COMMAND_UPDATE      0x01	/* Broadcasts the values in AS_TRANSTAB and ASn_MEMATTR to all MMUs */
#define AS_COMMAND_FLUSH_PT    0x04	/* Flush all L2 caches then issue a flush region command to all MMUs */
#define AS_COMMAND_FLUSH_MEM   0x05	/* Wait for memory accesses to complete, flush all the L1s cache then
					   flush all L2 caches then issue a flush region command to all MMUs */


void fast_memset(void *dst, unsigned long val, size_t size);
void fast_memcpy(void *dst, void *src, size_t size);
void fast_memcpy_addr(uint64_t dst, uint64_t src, size_t size);
void datafiller(uint64_t startaddr, uint64_t size, uint64_t val);
uint64_t read_ttbr_core(uint64_t IA, uint64_t this_mmu_pgd);

void handle_tlb_ipa(unsigned long ipa);

void change_gpu_interrupt_to_secure(void);
void change_gpu_interrupt_to_non_secure(void);
void submit_atom(uint64_t jc_head);
void juno_irq_mali_job_handler(void);

#endif
#endif
