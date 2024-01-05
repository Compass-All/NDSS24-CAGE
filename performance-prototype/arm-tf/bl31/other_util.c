#include <lib/mmio.h>
#include <arch.h>
#include <common/debug.h>
#include <bl31/interrupt_mgmt.h>
#include <drivers/arm/gicv2.h>

#include <plat/arm/common/arm_def.h>
#include <plat/arm/common/plat_arm.h>

#include <bl31/other_util.h>
#include <bl31/shadow_task.h>
#include <bl31/gpu_protect.h>

void fast_memset(void *dst, unsigned long val, size_t size)
{
	size_t _size = size / 64 * 64;
	for (unsigned long addr = (unsigned long)dst; addr < (unsigned long)dst + _size; addr += 64)
	{
		((unsigned long *)addr)[0] = val;
		((unsigned long *)addr)[1] = val;
		((unsigned long *)addr)[2] = val;
		((unsigned long *)addr)[3] = val;
		((unsigned long *)addr)[4] = val;
		((unsigned long *)addr)[5] = val;
		((unsigned long *)addr)[6] = val;
		((unsigned long *)addr)[7] = val;
	}
	memset(dst + _size, val, size - _size);
}
void fast_memcpy(void *dst, void *src, size_t size)
{
	for (long long remain = size; remain - 32 >= 0; dst += 32, src += 32, remain -= 32)
	{
		((unsigned long *)dst)[0] = ((unsigned long *)src)[0];
		((unsigned long *)dst)[1] = ((unsigned long *)src)[1];
		((unsigned long *)dst)[2] = ((unsigned long *)src)[2];
		((unsigned long *)dst)[3] = ((unsigned long *)src)[3];
	}
}

void fast_memcpy_addr(uint64_t dst, uint64_t src, size_t size)
{

	uint64_t cntsize = 0;
	while (cntsize < size)
	{
		*(uint64_t *)(dst + cntsize) = *(uint64_t *)(src + cntsize);
		cntsize += 8;
	}
	flush_dcache_range(dst, size);
}

void datafiller(uint64_t startaddr, uint64_t size, uint64_t val)
{
	if ((size & 0x7) != 0)
	{
		ERROR("Input size unaligned\n");
		ERROR("startaddr %llx size %llx val %llx\n", startaddr, size, val);
		panic();
	}
	uint64_t curaddr = startaddr;
	uint64_t finaladdr = startaddr + size;
	while (curaddr < finaladdr)
	{
		asm volatile("str %0,[%1]\n" ::"r"(val), "r"(curaddr) :);
		curaddr += 0x8;
	}

	flush_dcache_range(startaddr, size);
}

uint64_t read_ttbr_core(uint64_t IA, uint64_t this_mmu_pgd)
{
	uint64_t offset, phys_DA, table_base, OA;

	table_base = this_mmu_pgd;
	offset = get_bit_range_value(IA, 47, 39) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;
	table_base = *(uint64_t *)phys_DA;

	if ((table_base & 0x1) == 0)
		return 0;
	offset = get_bit_range_value(IA, 38, 30) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;
	table_base = *(uint64_t *)phys_DA;

	if ((table_base & 0x1) == 0)
		return 0;
	offset = get_bit_range_value(IA, 29, 21) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;
	table_base = *(uint64_t *)phys_DA;

	if ((table_base & 0x1) == 0)
		return 0;
	offset = get_bit_range_value(IA, 20, 12) << 3;
	phys_DA = (table_base & 0xfffffffff000) | offset;
	table_base = *(uint64_t *)phys_DA;

	if ((table_base & 0x1) == 0)
		return 0;
	offset = IA & 0xfff;
	OA = (table_base & 0xfffffffff000) | offset;
	return OA;
}

void handle_tlb_ipa(unsigned long ipa)
{
	unsigned long chunkipa = ipa >> 12;
	asm volatile(
		"dsb ish\n"
		"tlbi ipas2e1is, %0\n"
		"dsb ish\n"
		"tlbi vmalle1is\n"
		"dsb ish\n"
		"isb\n"
		:
		: "r"(chunkipa)
		:);
}

void change_gpu_interrupt_to_secure()
{
	gicv2_set_interrupt_type(65, GICV2_INTR_GROUP0);
	return;
}

void change_gpu_interrupt_to_non_secure()
{
	gicv2_set_interrupt_type(65, GICV2_INTR_GROUP1);
	return;
}

void submit_atom(uint64_t jc_head)
{

	uint32_t as_nr = mmio_read_32((long unsigned int)(GPU_MMIO_BASE + JOB_SLOT_REG(curtask_js, JS_CONFIG_NEXT))) & 0xf;

	struct shadowtask *cur_shadowtask = find_ccaext_shadowtask(curtask_app_id);
	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_TRANSTAB_LO)), ((cur_shadowtask->real_mmu_pgd) & 0xFFFFFFFFUL) + 0x7);
	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_TRANSTAB_HI)), ((cur_shadowtask->real_mmu_pgd) >> 32) & 0xFFFFFFFFUL);
	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_COMMAND)), AS_COMMAND_UPDATE);

	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_COMMAND)), AS_COMMAND_FLUSH_PT);
	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_COMMAND)), AS_COMMAND_FLUSH_MEM);

	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + JOB_SLOT_REG(curtask_js, JS_HEAD_NEXT_LO)), jc_head & 0xFFFFFFFF);
	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + JOB_SLOT_REG(curtask_js, JS_HEAD_NEXT_HI)), jc_head >> 32);
	mmio_write_32((long unsigned int)(GPU_MMIO_BASE + JOB_SLOT_REG(curtask_js, JS_COMMAND_NEXT)), JS_COMMAND_START);
}

void juno_irq_mali_job_handler()
{

	unsigned int mali_job_group = gicv2_get_interrupt_group(65);

	if (mali_job_group == GICV2_INTR_GROUP0)
	{
		struct shadowtask *cur_shadowtask = find_ccaext_shadowtask(curtask_app_id);
		uint64_t data_field_virtaddr = *(uint64_t *)(curtask_jc_phys + 0x130);
		uint64_t code_field_virtaddr = *(uint64_t *)(curtask_jc_phys + 0x138);
		uint64_t data_field_addr = read_ttbr_core(data_field_virtaddr, cur_shadowtask->real_mmu_pgd);
		uint64_t code_field_addr = read_ttbr_core(code_field_virtaddr, cur_shadowtask->real_mmu_pgd);

		for (uint64_t current_addr = data_field_addr; current_addr < code_field_addr; current_addr += 8)
		{
			uint64_t buffer_virt_addr = *(uint64_t *)current_addr;
			uint64_t buffer_phys_addr = read_ttbr_core(buffer_virt_addr, cur_shadowtask->real_mmu_pgd);

			if (buffer_phys_addr == 0)
				continue;
			if (buffer_phys_addr < CCAEXT_REALTASK_MEMORY_STARTADDR)
			{
				ERROR("juno_irq_mali_job_handler: Find a buffer does not belong to realms\n");
				panic();
			}

			uint64_t databuffer_stub_addr = get_databuffer_stub_addr(cur_shadowtask, buffer_virt_addr);
			datafiller(current_addr, 0x8, databuffer_stub_addr);

			uint64_t replacestartaddr = curtask_jc_phys & 0xffffff000;
			uint64_t replaceendaddr = replacestartaddr + 0x1000;
			while (replacestartaddr < replaceendaddr)
			{
				if (*(uint64_t *)replacestartaddr == buffer_virt_addr)
				{
					datafiller(replacestartaddr, 0x8, databuffer_stub_addr);
				}
				replacestartaddr += 0x8;
			}
		}

		gptconfig_disable_metadata_protect();
		gptconfig_disable_gpummio_protect();
		gptconfig_disable_gpu_gpt_protection();
		change_gpu_interrupt_to_non_secure();

		// Restore the GPU status and current task records
		uint32_t as_nr = mmio_read_32((long unsigned int)(GPU_MMIO_BASE + JOB_SLOT_REG(curtask_js, JS_CONFIG_NEXT))) & 0xf;
		mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_TRANSTAB_LO)), ((curtask_stub_mmu_pgd) & 0xFFFFFFFFUL) + 0x7);
		mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_TRANSTAB_HI)), ((curtask_stub_mmu_pgd) >> 32) & 0xFFFFFFFFUL);
		mmio_write_32((long unsigned int)(GPU_MMIO_BASE + MMU_AS_REG(as_nr, AS_COMMAND)), AS_COMMAND_UPDATE);

		curtask_app_id = 0;
		curtask_jc_phys = 0;
		curtask_js = 0;
		curtask_stub_mmu_pgd = 0;
		mali_job_group = gicv2_get_interrupt_group(65);
	}
	else
		return;
}
