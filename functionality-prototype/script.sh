#!/bin/bash
export MODEL=<PATH/TO/FVP/MODEL>
export DISK=<PATH/TO/HOST/FILESYSTEM>
export DTB=<PATH/TO/DTB/IMG>
export IMAGE=<PATH/TO/KERNEL/IMAGE>
export INITRD=<PATH/TO/RAMDISK.IMG>

chmod a+w $DISK


cd <PATH/TO/TF-A-TESTSv2.8>
make -j8 all \
	PLAT=fvp \ 
	DEBUG=1 \ 
	CROSS_COMPILE=<PATH/TO/CROSSCOMPILER> \

#git clone -b tf-rmm-v0.2.0 https://git.trustedfirmware.org/TF-RMM/tf-rmm.git --recurse-submodules
cd  <PATH/TO/TF-RMM-v0.2.0>
cmake -DRMM_CONFIG=fvp_defcfg -S . -B build 
cmake --build build

#git clone -b v2.8 https://git.trustedfirmware.org/hafnium/hafnium.git --recurse-submodules
#remember to remove the "enable_mte = 1" in project/reference submodule
cd <PATH/TO/Hafniumv2.8>
PATH=$PWD/prebuilts/linux-x64/clang/bin:$PWD/prebuilts/linux-x64/dtc:$PATH
make PROJECT=reference \ 
	CROSS_COMPILE=<PATH/TO/CROSSCOMPILER>


cd <PATH/TO/TF-Av2.8>
make PLAT=fvp all fip \
	CROSS_COMPILE=<PATH/TO/CROSSCOMPILER> \
	ENABLE_RME=1 \
	DEBUG=1 \
	ARCH=aarch64 \
	FVP_HW_CONFIG_DTS=fdts/fvp-base-gicv3-psci-1t.dts \
	ARM_DISABLE_TRUSTED_WDOG=1 \
	SPD=spmd \
	SPMD_SPM_AT_SEL2=1 \
	BRANCH_PROTECTION=1 \
	CTX_INCLUDE_PAUTH_REGS=1 \
	SP_LAYOUT_FILE=<PATH/TO/TF-A-TEST/SP_LAYOUT.JSON> \
	BL32=<PATH/TO/HAFNIUM.BIN> \
	CTX_INCLUDE_EL2_REGS=1 
	BL33=<PATH/TO/UBOOT.BIN> \
	RMM=<PATH/TO/TF-RMM/RMM.IMG>

$MODEL \
	-C bp.secureflashloader.fname=<PATH/TO/TF-A/BL1.BIN> \
	-C bp.flashloader0.fname=<PATH/TO/TF-A/FIP.BIN>  \
	--data cluster0.cpu0=$IMAGE@0x80080000 \
	--data cluster0.cpu0=$DTB@0x82000000 \
	--data cluster0.cpu0=$INITRD@0x84000000 \
	-C bp.virtioblockdevice.image_path=$DISK \
	-C bp.refcounter.non_arch_start_at_default=1 \
	-C bp.refcounter.use_real_time=0 \
	-C bp.ve_sysregs.exit_on_shutdown=1 \
	-C cache_state_modelled=1 \
	-C bp.dram_size=4 \
	-C bp.secure_memory=1 \
	-C pci.pci_smmuv3.mmu.SMMU_ROOT_IDR0=3 \
	-C pci.pci_smmuv3.mmu.SMMU_ROOT_IIDR=0x43B \
	-C pci.pci_smmuv3.mmu.root_register_page_offset=0x20000 \
	-C pci.pci_smmuv3.mmu.SMMU_AIDR=2 \
	-C pci.pci_smmuv3.mmu.SMMU_IDR0=0x0046123B \
	-C pci.pci_smmuv3.mmu.SMMU_IDR1=0x00600002 \
	-C pci.pci_smmuv3.mmu.SMMU_IDR3=0x1714 \
	-C pci.pci_smmuv3.mmu.SMMU_IDR5=0xFFFF0475 \
	-C pci.pci_smmuv3.mmu.SMMU_S_IDR1=0xA0000002 \
	-C pci.pci_smmuv3.mmu.SMMU_S_IDR2=0 \
	-C pci.pci_smmuv3.mmu.SMMU_S_IDR3=0 \
	-C cluster0.NUM_CORES=1 \
	-C cluster0.PA_SIZE=48 \
	-C cluster0.ecv_support_level=2 \
	-C cluster0.gicv3.cpuintf-mmap-access-level=2 \
	-C cluster0.gicv3.without-DS-support=1 \
	-C cluster0.gicv4.mask-virtual-interrupt=1 \
	-C cluster0.has_arm_v8-6=1 \
	-C cluster0.has_amu=1 \
	-C cluster0.has_branch_target_exception=1 \
	-C cluster0.rme_support_level=2 \
	-C cluster0.has_rndr=1 \
	-C cluster0.has_v8_7_pmu_extension=2 \
	-C cluster0.max_32bit_el=-1 \
	-C cluster0.stage12_tlb_size=1024 \
	-C cluster0.check_memory_attributes=0 \
	-C cluster0.ish_is_osh=1 \
	-C cluster0.restriction_on_speculative_execution=2 \
	-C cluster0.restriction_on_speculative_execution_aarch32=2 \
	-C cluster1.NUM_CORES=0 \
	-C cluster1.PA_SIZE=48 \
	-C cluster1.ecv_support_level=2 \
	-C cluster1.gicv3.cpuintf-mmap-access-level=2 \
	-C cluster1.gicv3.without-DS-support=1 \
	-C cluster1.gicv4.mask-virtual-interrupt=1 \
	-C cluster1.has_arm_v8-6=1 \
	-C cluster1.has_amu=1 \
	-C cluster1.has_branch_target_exception=1 \
	-C cluster1.rme_support_level=2 \
	-C cluster1.has_rndr=1 \
	-C cluster1.has_v8_7_pmu_extension=2 \
	-C cluster1.max_32bit_el=-1 \
	-C cluster1.stage12_tlb_size=1024 \
	-C cluster1.check_memory_attributes=0 \
	-C cluster1.ish_is_osh=1 \
	-C cluster1.restriction_on_speculative_execution=2 \
	-C cluster1.restriction_on_speculative_execution_aarch32=2 \
	-C pctl.startup=0.0.0.0 \
	-C bp.smsc_91c111.enabled=1 \
	-C bp.hostbridge.userNetworking=1
