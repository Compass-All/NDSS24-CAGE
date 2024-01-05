#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/ccaext_realm.h>

#define DEVICE_NAME "CCAEXT_REALM"

MODULE_AUTHOR("COMPASS");
MODULE_DESCRIPTION("CCAEXT_REALM");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");

static int ccaext_realm_driver_major;
static struct class*  ccaext_realm_driver_class   = NULL;
static struct device* ccaext_realm_driver_device  = NULL;
struct ccaext_realm total_realm[CCAEXT_SHADOWTASK_MAX];
bool ccaext_gpu_memory_bitmap[CCAEXT_GPU_MEMORY_BITMAP_NUM];


static int ccaext_realm_driver_open(struct inode * inode, struct file * filp)
{
	return 0;
}


static int ccaext_realm_driver_release(struct inode * inode, struct file *filp)
{
  return 0;
}


ssize_t ccaext_realm_driver_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    return 0;
}

ssize_t ccaext_realm_driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    cmd = _IOC_NR(cmd);
    uint64_t *cmd_args=(uint64_t*)vmalloc(0x100);
    memset(cmd_args,0x0,0x100);
    copy_from_user(cmd_args,(void *)arg,0x100);

    if (cmd==0x1){//Initialization
        uint64_t realm_id=cmd_args[0];
        uint64_t total_datainfo=cmd_args[1];

        printk("Start establishing realm region %llx\n",realm_id);

		bool hasit=false;
		int idx=0;
		while (idx<CCAEXT_SHADOWTASK_MAX){
			if(!total_realm[idx].is_available){
				if(total_realm[idx].realm_id==realm_id){
					printk("Error: realm region %llx exist\n", realm_id);
					break;
				}
			}
			idx+=1;
		}

		idx=0;
		while (idx<CCAEXT_SHADOWTASK_MAX){
			if(total_realm[idx].is_available){
				total_realm[idx].is_available=false;
				total_realm[idx].realm_id=realm_id;
				break;
			}
			idx+=1;
		}
		if(idx==CCAEXT_SHADOWTASK_MAX){
			printk("Error: No available space for realm region\n");
			return 0;
		}

		// Ask the Monitor to create a realm region
		// [input] x1: realm_id

		asm volatile(
	        "ldr x0,=0xc7000001\n"
	        "mov x1,%0\n"
	        "smc #0\n"
	        :
	        :"r"(realm_id)
	        :"x0","x1"
	    );	



		// 2. transfer data based on the datainfo
		uint64_t cur_datainfo_idx=0;
		int tmp_idx=2;
		void __iomem *real_task_data;
		while (cur_datainfo_idx<total_datainfo){
			uint64_t source_info=cmd_args[tmp_idx];
			tmp_idx+=1;
			uint64_t data_size=cmd_args[tmp_idx];
			tmp_idx+=1;

	    	uint64_t set_size = 0;
	    	if ((data_size&0xfff)!=0) set_size = 0x1000+((data_size>>12)<<12); else set_size = data_size;

	    	//Call the service to transfer the data
			//[input] x1: realm_id, x2: source_info, x3: set_size
			//[output] x1: dest_info
	    	uint64_t dest_info=0;
	    	asm volatile(
	        	"ldr x0,=0xc7000002\n"
	        	"mov x1,%1\n"
				"mov x2,%2\n"
				"mov x3,%3\n"
	        	"smc #0\n"
	        	"mov %0,x1\n"
	        	:"=r"(dest_info)
	        	:"r"(realm_id),"r"(source_info),"r"(set_size)
	        	:"x0","x1","x2","x3"
	    	);
	    	printk("Get real dataset addr %llx for datainfo id %llx\n",dest_info,cur_datainfo_idx);
	    	
			//copy data from the source_info to the dest_info
	    	real_task_data = ioremap(dest_info, set_size);
			printk("Start copy, dest_info %llx, set_size %llx\n",dest_info, set_size);
	    	copy_from_user(real_task_data,(void *)source_info,set_size);

			cur_datainfo_idx+=1;
		}

	    // 3. Terminate the transmission, let the EL3 to protect the data
		// [input] x1: realm_id

	    asm volatile(
	        "ldr x0,=0xc7000006\n"
	        "mov x1,%0\n"
	        "smc #0\n"
	        :
	        :"r"(realm_id)
	        :"x0","x1"
	    );
	    printk("Realm %llx established successfully, transfer total %llx dataset\n",realm_id,total_datainfo);
    }

    else if(cmd==0x2){
        uint64_t realm_id=cmd_args[0];
        printk("Start destroying realm region %llx\n",realm_id);
		
		int idx=0;
		while (idx<CCAEXT_SHADOWTASK_MAX){
			if(!total_realm[idx].is_available){
				if(total_realm[idx].realm_id==realm_id){
					total_realm[idx].is_available=true;
					total_realm[idx].realm_id=-1;
				}
				break;
			}
			idx+=1;
		}
		if(idx==CCAEXT_SHADOWTASK_MAX){
			printk("Error: Cannot find the realm region.\n");
			return 0;
		}

	    asm volatile(
	        "ldr x0,=0xc7000009\n"
	        "mov x1,%0\n"
	        "smc #0\n"
	        :
	        :"r"(realm_id)
	        :"x0","x1"
	    );
	    printk("Realm region %llx destroyed successfully\n",realm_id);
    }

	else if(cmd==0x3){
        uint64_t realm_id=cmd_args[0];
        printk("Start creating stub task for realm region %llx\n",realm_id);
		uint64_t scenario=cmd_args[1];
		printk("Test scenario: %lx\n",scenario);
		uint64_t total_databuffer=cmd_args[2];
		uint64_t total_codebuffer=cmd_args[3];


		//Finish the TTBR part
		// +0x0: realm_id
		// +0x8: number of mapping requirements
		// for each +0x20:
		//	+0x0: start_ipa_addr
		//	+0x8: size
		//	+0x10: start_pa_addr
		//	+0x18: attr
		uint64_t ttbrreq_addr=find_available_gpu_memory_region(0x1000);
		if(ttbrreq_addr==0){
			printk("Cannot allocate ttbr req region for realm %llx\n",realm_id);
			return 0;
		}
		void __iomem* ttbrreq_region=ioremap(ttbrreq_addr,0x1000);
		*(uint64_t*) (ttbrreq_region) = realm_id;

		uint64_t ttbrreq_cnt = 0; 
		uint64_t cur_req_addr=ttbrreq_region+0x10;


		printk("Prepare metadata for the stub task\n");
		uint64_t metadata_size=0x1000;
		uint64_t metadata_addr=find_available_gpu_memory_region(metadata_size);
		if(metadata_addr==0){
			printk("Cannot allocate memory for metadata\n");
			return 0;
		}
		printk("metadata addr in %llx, size %llx\n",metadata_addr,metadata_size);

		void __iomem* metadata_region=ioremap(metadata_addr,metadata_size);
		uint64_t current_addr = 0 ;
		while(current_addr<metadata_size){
			*(uint64_t*) (metadata_region+current_addr)=0;
			current_addr+=8;
		}

		*(uint64_t*) (cur_req_addr+0x0)= metadata_addr;
		*(uint64_t*) (cur_req_addr+0x8)= 0x1000;
		*(uint64_t*) (cur_req_addr+0x10)= metadata_addr;
		*(uint64_t*) (cur_req_addr+0x18)= 0x01c000000000077f;
		cur_req_addr+=0x20;
		ttbrreq_cnt+=1;


		printk("Prepare data buffers for the stub task\n");
		uint64_t cur_idx=4;
		uint64_t cur_databuffer_id=0;

		uint64_t metadata_cur_databuffer_addr=0;

		while(cur_databuffer_id<total_databuffer){
			uint64_t curtask_curdatabuffer_size=cmd_args[(cur_idx+4)];

			printk("Start allocate memory for databuffer %llx, size %llx\n",cur_databuffer_id,curtask_curdatabuffer_size);

			uint64_t curdatabuffer_addr=find_available_gpu_memory_region(curtask_curdatabuffer_size);
			if(curdatabuffer_addr==0){
				printk("Cannot allocate memory for databuffer %llx\n",cur_databuffer_id);
				return 0;
			}
			printk("Allocate databuffer in addr %llx\n",curdatabuffer_addr);

			void __iomem* curdatabuffer_region=ioremap(curdatabuffer_addr,curtask_curdatabuffer_size);

			current_addr = 0 ;
			while(current_addr<curtask_curdatabuffer_size){
				*(uint64_t*) (curdatabuffer_region+current_addr)=0;
				current_addr+=8;
			}

			printk("Fill this buffer addr into metadata\n");

			*(uint64_t*) (metadata_region+metadata_cur_databuffer_addr)=curdatabuffer_addr; //Since we configure S-2 translation, here we use IPA, not VA
			metadata_cur_databuffer_addr+=0x8;

			printk("Start filling databuffer description\n");

			//Fill buffer description into the stub data buffer
			*(uint64_t*) (curdatabuffer_region+0x0)=cmd_args[(cur_idx+0)];
			*(uint64_t*) (curdatabuffer_region+0x8)=cmd_args[(cur_idx+1)];
			*(uint64_t*) (curdatabuffer_region+0x10)=cmd_args[(cur_idx+2)];
			*(uint64_t*) (curdatabuffer_region+0x18)=cmd_args[(cur_idx+3)];
			*(uint64_t*) (curdatabuffer_region+0x20)=cmd_args[(cur_idx+4)];
			*(uint64_t*) (curdatabuffer_region+0x28)=cmd_args[(cur_idx+5)];
			*(uint64_t*) (curdatabuffer_region+0x30)=cmd_args[(cur_idx+6)];
			*(uint64_t*) (curdatabuffer_region+0x38)=cmd_args[(cur_idx+7)];
			*(uint64_t*) (curdatabuffer_region+0x40)=cmd_args[(cur_idx+8)];


			*(uint64_t*) (cur_req_addr+0x0)= curdatabuffer_addr;
			*(uint64_t*) (cur_req_addr+0x8)= curtask_curdatabuffer_size;
			*(uint64_t*) (cur_req_addr+0x10)= curdatabuffer_addr;
			*(uint64_t*) (cur_req_addr+0x18)= 0x01c00000000007ff;
			cur_req_addr+=0x20;
			ttbrreq_cnt+=1;


			cur_databuffer_id+=1;
			cur_idx+=9;
		}
		
		printk("Prepare a simple code buffer\n");
		uint64_t cur_codebuffer_id=0;
		uint64_t metadata_cur_codebuffer_addr=0;

		while(cur_codebuffer_id<total_codebuffer){
			uint64_t curtask_curcodebuffer_size=0x1000;

			printk("Start allocate memory for codebuffer %llx, size %llx\n",cur_codebuffer_id,curtask_curcodebuffer_size);

			uint64_t curcodebuffer_addr=find_available_gpu_memory_region(curtask_curcodebuffer_size);
			if(curcodebuffer_addr==0){
				printk("Cannot allocate memory for codebuffer %llx\n",cur_codebuffer_id);
				return 0;
			}
			printk("Got codebuffer addr %llx\n",curcodebuffer_addr);

			void __iomem* curcodebuffer_region=ioremap(curcodebuffer_addr,curtask_curcodebuffer_size);

			current_addr = 0 ;
			while(current_addr<curtask_curcodebuffer_size){
				*(uint64_t*) (curcodebuffer_region+current_addr)=0;
				current_addr+=8;
			}



			printk("Fill this codebuffer addr into metadata\n");
			*(uint64_t*) (metadata_region+metadata_cur_codebuffer_addr+0x100)=curcodebuffer_addr; //Since we configure S-2 translation, here we use IPA, not VA
			metadata_cur_codebuffer_addr+=0x8;

			printk("Fill ramdom values as codes\n");

			*(uint64_t*) (curcodebuffer_region+0x0)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x8)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x10)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x18)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x20)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x28)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x30)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x38)=0xdeadc0de;
			*(uint64_t*) (curcodebuffer_region+0x40)=0xdeadc0de;


			*(uint64_t*) (cur_req_addr+0x0)= curcodebuffer_addr;
			*(uint64_t*) (cur_req_addr+0x8)= curtask_curcodebuffer_size;
			*(uint64_t*) (cur_req_addr+0x10)= curcodebuffer_addr;
			*(uint64_t*) (cur_req_addr+0x18)= 0x01c000000000077f;
			cur_req_addr+=0x20;
			ttbrreq_cnt+=1;

			curcodebuffer_addr+=1;
			cur_codebuffer_id+=1;
		}

		*(uint64_t*) (ttbrreq_region+0x8) = ttbrreq_cnt;

		printk("Submit the task with realm region %llx, metadata addr %llx, PTE requirement addr %llx\n",realm_id,metadata_addr,ttbrreq_addr);

		//Finally, send it to EL3
	    asm volatile(
	        "ldr x0,=0xc7000008\n"
	        "mov x1,%0\n"
	        "mov x2,%1\n"
			"mov x3,%2\n"
			"mov x4,%3\n"
	        "smc #0\n"
	        :
	        :"r"(realm_id),"r"(metadata_addr),"r"(ttbrreq_addr),"r"(scenario)
	        :"x0","x1","x2","x3","x4"
	    );
		printk("Send the task to the Monitor successfully\n");
    }

	return 0;	
}


static struct file_operations ccaext_realm_driver_fops = {
	.owner   =    THIS_MODULE,
	.open    =    ccaext_realm_driver_open,
	.release =    ccaext_realm_driver_release,
	.read    =    ccaext_realm_driver_read,
	.unlocked_ioctl = ccaext_realm_driver_ioctl,
};


static int __init ccaext_realm_driver_init(void)
{
	ccaext_realm_driver_major = register_chrdev(0, DEVICE_NAME, &ccaext_realm_driver_fops);
	if(ccaext_realm_driver_major < 0){
		printk("failed to register ccaext_realm_driver device.\n");
		return -1;
	}
	
	
	ccaext_realm_driver_class = class_create(THIS_MODULE, "ccaext_realm_driver");
    if (IS_ERR(ccaext_realm_driver_class)){
        printk("failed to create ccaext_realm_driver class.\n");
        unregister_chrdev(ccaext_realm_driver_major, DEVICE_NAME);
        return -1;
    }
	
	
    ccaext_realm_driver_device = device_create(ccaext_realm_driver_class, NULL, 
		MKDEV(ccaext_realm_driver_major, 0), NULL, "ccaext_realm_driver_device");
    if (IS_ERR(ccaext_realm_driver_device)){
        printk("failed to create ccaext_realm_driver device.\n");
        unregister_chrdev(ccaext_realm_driver_major, DEVICE_NAME);
        return -1;
    }	

	//Initialize secure pids
	int idx=0;
	while (idx<CCAEXT_SHADOWTASK_MAX){
		total_realm[idx].is_available=true;	
		total_realm[idx].realm_id=-1;
		idx+=1;
	}

	int tmp_bit=0;
	while(tmp_bit<CCAEXT_GPU_MEMORY_BITMAP_NUM){
		ccaext_gpu_memory_bitmap[tmp_bit]=true;
		tmp_bit+=1;
	}

	printk("Initialized ccaext_realm_driver successfully!\n");
	
    return 0;	
}


uint64_t find_available_gpu_memory_region(uint64_t required_size){

    uint64_t failedaddr=0x0;
    uint64_t required_bits= required_size>>12;
    uint64_t cur_bit = 0;
    while(cur_bit < CCAEXT_GPU_MEMORY_BITMAP_NUM){
        if(ccaext_gpu_memory_bitmap[cur_bit] == true){
            bool is_satisfied=true;
            uint64_t tmp_bit=0;
            while(tmp_bit<required_bits){
                if(ccaext_gpu_memory_bitmap[cur_bit+tmp_bit]==false){
                    is_satisfied=false;
                    break;
                }
                tmp_bit+=1;
            }
            if (is_satisfied){

				uint64_t newtmp_bit=0;
				while(newtmp_bit<required_bits){
					ccaext_gpu_memory_bitmap[(cur_bit+newtmp_bit)]=false;
					newtmp_bit+=1;
				}
                return (CCAEXT_GPU_MEMORY_STARTADDR + (cur_bit << 12));
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

static void __exit ccaext_realm_driver_exit(void)
{
	printk("Exiting ccaext_realm_driver.\n");
    device_destroy(ccaext_realm_driver_class, MKDEV(ccaext_realm_driver_major, 0));
    class_unregister(ccaext_realm_driver_class);
	class_destroy(ccaext_realm_driver_class);
	unregister_chrdev(ccaext_realm_driver_major, DEVICE_NAME);
    printk("Succesfully exit ccaext_realm_driver.\n");	
}

module_init(ccaext_realm_driver_init);
module_exit(ccaext_realm_driver_exit);
MODULE_LICENSE("GPL");