# Introduction

This work contains two prototypes: The functionality prototype, which runs on RME-enabled Arm FVP, and the performance prototype, which runs on Arm Juno R2 development board.

## Performance Prototype

This prototype runs on Armv8 Juno R2 development board. We build this prototype to measure CAGE's performance overhead on Arm Mali-T624 GPU and Xilinx Virtex UltraScale+ VCU118 FPGA.

We provide the following items

- The ```linux``` folder is the source code of Host. It includes a Midgard GPU driver.
- The ```arm-tf``` folder is the source code of the Monitor. Note that we emulate CCA-related operations to measure the performance.
- The ```xdma-driver``` folder is the source code of FPGA's XDMA driver.
- The ```test``` folder includes several sample tests for our Juno R2 prototype. Currently, we only provide GPU Rodinia tests and we will release the FPGA applications in the future.


## Functionality Prototype

This prototype runs on RME-enabled Arm Fixed Virtual Platform (FVP). We build this prototype to measure CAGE's functionality by using FVP's test engine. Since the FVP lacks a real GPU hardware, we use its test engine module to generate DMA requests and verify our mechanisms. Note that the test engine is an embedded peripheral and is designed for testing SMMU's functionality.

We provide the following items

- The ```linux``` folder is the source code of Host.
- The ```arm-tf``` folder is the source code of the Monitor. 
- The ```test``` folder includes several sample tests for our FVP prototype. 
- The ```script.sh``` file is the script for running RME-enabled FVP, with RME support on SMMU. 

The test engine can perform DMA as a unified-memory GPU. However, it does not have a configurable MMU and cannot perform address translation. Thus, we instead use the Stage-2 translation (specifically, it is Stage-2 translation in Secure World) for our functionality verification. To enable the Stage-2 translation in Secure World, we need to addtionally run a Hafnium for SMMU initialization. 