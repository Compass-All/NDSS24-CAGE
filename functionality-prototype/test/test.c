#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h> 
#include <stdint.h> 
#include <asm-generic/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <assert.h>
#include <fcntl.h>
#include <vector>

#define CMD(x) _IO(77,x)

#define CCAEXT_DATABUFFER_INPUT 0x1
#define CCAEXT_DATABUFFER_OUTPUT 0x2
#define CCAEXT_DATABUFFER_BOTHIO (CCAEXT_DATABUFFER_INPUT|CCAEXT_DATABUFFER_OUTPUT)
#define CCAEXT_DATABUFFER_NULL 0x0


uint64_t realm1_id=0xdead1111;
uint64_t realm2_id=0xdead2222;

void *my_backup_array_1;
void *my_backup_array_2;
void *my_backup_array_3;
void *my_backup_array_4;
uint64_t total_dataset_num_realm1=2;
uint64_t total_dataset_num_realm2=2;

uint64_t total_databuffer_num_realm1=2;
uint64_t total_codebuffer_num_realm1=1;

uint64_t ccaext_magic_number=0xaaaabbbbccccdddd;
int fd=open("/dev/ccaext_realm_driver_device",O_RDWR);


		// We can modify the test condition here for four different tests
		// Condition 0: Correct computing scenario
		// Condition 1: Test the correctness of GPU GPT
		// Condition 2: Test the correctness of CPU GPT
		// Condition 3: Test the GPU PTE mapping
		
		uint64_t test_condition=0;
int main() {

   		printf("CAGE: Prepare Test\n");

   		uint64_t dataset1_realm1_size=0x1000;
   		uint64_t dataset2_realm1_size=0x2000;
 		uint64_t dataset1_realm2_size=0x3000;
   		uint64_t dataset2_realm2_size=0x4000;


		uint64_t *dataset1_realm1 = (uint64_t*) malloc(dataset1_realm1_size);
		uint64_t *dataset2_realm1 = (uint64_t*) malloc(dataset2_realm1_size);
		uint64_t *dataset1_realm2 = (uint64_t*) malloc(dataset1_realm2_size);
		uint64_t *dataset2_realm2 = (uint64_t*) malloc(dataset2_realm2_size);
		memset(dataset1_realm1,0x11111111,dataset1_realm1_size);
		memset(dataset2_realm1,0x22222222,dataset2_realm1_size);
		memset(dataset1_realm2,0x33333333,dataset1_realm2_size);
		memset(dataset2_realm2,0x44444444,dataset2_realm2_size);

		my_backup_array_1=(void*)malloc(dataset1_realm1_size);
		memcpy(my_backup_array_1,(void*)dataset1_realm1,dataset1_realm1_size);
		my_backup_array_2=(void*)malloc(dataset2_realm1_size);
		memcpy(my_backup_array_2,(void*)dataset2_realm1,dataset2_realm1_size);
		my_backup_array_3=(void*)malloc(dataset1_realm2_size);
		memcpy(my_backup_array_3,(void*)dataset1_realm2,dataset1_realm2_size);
		my_backup_array_4=(void*)malloc(dataset2_realm2_size);
		memcpy(my_backup_array_4,(void*)dataset2_realm2,dataset2_realm2_size);



   		printf("Transfer sensitive data to realm 1\n");   

		unsigned int cmd_create_realm1=CMD(0x1);
		uint64_t *cmd_create_realm1_args=(uint64_t*)malloc(0x100);
		memset(cmd_create_realm1_args,0x0,0x100);
		cmd_create_realm1_args[0]=realm1_id;
		cmd_create_realm1_args[1]=total_dataset_num_realm1;

		cmd_create_realm1_args[2]=(uint64_t)my_backup_array_1;
		cmd_create_realm1_args[3]=dataset1_realm1_size;
		cmd_create_realm1_args[4]=(uint64_t)my_backup_array_2;
		cmd_create_realm1_args[5]=dataset2_realm1_size;
				
		int ioctlret_realm1=ioctl(fd,cmd_create_realm1,cmd_create_realm1_args);
		assert(ioctlret_realm1==0);


   		printf("Transfer sensitive data to realm 2\n");   

		unsigned int cmd_create_realm2=CMD(0x1);
		uint64_t *cmd_create_realm2_args=(uint64_t*)malloc(0x100);
		memset(cmd_create_realm2_args,0x0,0x100);
		cmd_create_realm2_args[0]=realm2_id;
		cmd_create_realm2_args[1]=total_dataset_num_realm2;

		cmd_create_realm2_args[2]=(uint64_t)my_backup_array_3;
		cmd_create_realm2_args[3]=dataset1_realm2_size;
		cmd_create_realm2_args[4]=(uint64_t)my_backup_array_4;
		cmd_create_realm2_args[5]=dataset2_realm2_size;
				
		int ioctlret_realm2=ioctl(fd,cmd_create_realm2,cmd_create_realm2_args);
		assert(ioctlret_realm2==0);

		printf("Prepare Test\n");
   		printf("Create stub GPU app for realm 1\n");
   		printf("Let user-level driver provide data buffer descriptions");
   		
		unsigned int cmd_create_realm1_gpuapp=CMD(0x3);
		uint64_t *cmd_create_realm1_gpuapp_args=(uint64_t*)malloc(0x100);
		memset(cmd_create_realm1_gpuapp_args,0x0,0x100);
		cmd_create_realm1_gpuapp_args[0]=realm1_id;
		cmd_create_realm1_gpuapp_args[2]=total_databuffer_num_realm1;
		cmd_create_realm1_gpuapp_args[3]=total_codebuffer_num_realm1;
		
		uint64_t cur_arg_idx=4;
		uint64_t cur_idx=0;

			cmd_create_realm1_gpuapp_args[(cur_arg_idx+0)]=ccaext_magic_number;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+1)]=realm1_id;			
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+2)]=total_databuffer_num_realm1;			
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+3)]=0;			
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+4)]=dataset2_realm1_size;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+5)]=CCAEXT_DATABUFFER_INPUT;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+6)]=(uint64_t)my_backup_array_2;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+7)]=dataset2_realm1_size;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+8)]=0;
			cur_arg_idx+=9;

			cmd_create_realm1_gpuapp_args[(cur_arg_idx+0)]=ccaext_magic_number;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+1)]=realm1_id;			
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+2)]=total_databuffer_num_realm1;			
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+3)]=1;			
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+4)]=dataset2_realm1_size;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+5)]=CCAEXT_DATABUFFER_OUTPUT;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+6)]=0;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+7)]=0;
			cmd_create_realm1_gpuapp_args[(cur_arg_idx+8)]=0;
		
		printf("We select to test condition %lx\n", test_condition);
		printf("\t Condition 0: Correct computing scenario\n");
		printf("\t Condition 1: Test the correctness of GPU GPT\n");
		printf("\t Condition 2: Test the correctness of CPU GPT\n");
		printf("\t Condition 3: Test the GPU PTE mapping\n");

		cmd_create_realm1_gpuapp_args[1]=test_condition;		

		printf("Next we collaborate with kernel-level driver to create a stub application\n");
		printf("This includes creating metadata and buffers\n");
		int ioctlret_realm1_gpuapp=ioctl(fd,cmd_create_realm1_gpuapp,cmd_create_realm1_gpuapp_args);
		assert(ioctlret_realm1_gpuapp==0);   		
		
		printf("End of test, please kill FVP process if we want to start a new test\n");
		
   		return 0;
}



