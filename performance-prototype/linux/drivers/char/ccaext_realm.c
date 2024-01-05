// CAGEWORK: This is a small driver for helping copy data into realms. We assume the remote user will transfer data to realms via a secure channel.



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
struct ccaext_kctx total_kctx[10];
int stop_submit;

uint64_t num_update_pte_sections;
uint64_t next_available_pte_id;

static void __iomem *updated_entries = NULL;

void add_update_pte_entries(uint64_t pte_id,uint64_t pa_desc){
	iowrite64(pa_desc,updated_entries+0x1000+(pte_id)*0x8);
	return;
}

void add_update_pte_section(uint64_t sec_id,uint64_t start_vpfn, uint64_t size){
	iowrite64(start_vpfn,updated_entries+(sec_id)*0x10);
	iowrite64(size,updated_entries+(sec_id)*0x10+0x8);
	return;
}


bool check_any_ccaext_katom(void){
	int curidx=0;
	while(curidx<10){
		if(total_kctx[curidx].num_apps!=0){
			return true;
		}
		curidx+=1;
	}
	return false;
}

int find_kctx_by_kctx_addr(uint64_t kctx_addr){
    int ret=-1;
    int curidx=0;
    while(curidx<10){
        if(total_kctx[curidx].kctx_addr==kctx_addr){
            return curidx;
        }
        curidx+=1;
    }
    return ret;
}

int find_kctx_by_pid(uint32_t kctx_pid){
    int ret=-1;
    int curidx=0;
    while(curidx<10){
        if(total_kctx[curidx].kctx_pid==kctx_pid){
            return curidx;
        }
        curidx+=1;
    }
    return ret;
}

void update_kctx_addr_by_pos(int kctx_pos,uint64_t kctx_addr){
	total_kctx[kctx_pos].kctx_addr=kctx_addr;
}

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
    uint64_t *cmd_args=(uint64_t*)vmalloc(0x1000);
    memset(cmd_args,0x0,0x1000);
    copy_from_user(cmd_args,(void *)arg,0x1000);

    if (cmd==0x1){//Initialization
        uint64_t app_id=cmd_args[0];
        uint64_t total_datainfo=cmd_args[1];

		//Check whether it belongs to the recorded secure pids. If not, add it.
		bool hasit=false;
		int idx=0;
		while (idx<10){
			if(total_kctx[idx].kctx_pid==current->pid){
				hasit=true;
				total_kctx[idx].num_apps+=1;
				break;
			}
			idx+=1;
		}
		idx=0;
		if (!hasit){
			while (idx<10){
				if(total_kctx[idx].num_apps==0){
					total_kctx[idx].kctx_pid=current->pid;
					total_kctx[idx].num_apps+=1;
					break;
				}
				idx+=1;
			}			
		}
		if(idx==10){
			printk("Error: Max number of ccaext pid\n");
		}
		// Communication with the Monitor
		// 1. Tell it to establish an app
		// [input] x1: app_id

		asm volatile(
	        "ldr x0,=0xc7000001\n"
	        "mov x1,%0\n"
	        "smc #0\n"
	        :
	        :"r"(app_id)
	        :"x0","x1"
	    );	

		// 2. transfer data based on the datainfo
		uint64_t cur_datainfo_idx=0;
		int tmp_idx=2;
		while (cur_datainfo_idx<total_datainfo){
			uint64_t source_info=cmd_args[tmp_idx];
			tmp_idx+=1;
			uint64_t data_size=cmd_args[tmp_idx];
			tmp_idx+=1;

	    	uint64_t set_size = 0;
	    	if ((data_size&0xfff)!=0) set_size = 0x1000+((data_size>>12)<<12); else set_size = data_size;

	    	//Call the service to transfer the data
			//[input] x1: app_id, x2: source_info, x3: set_size
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
	        	:"r"(app_id),"r"(source_info),"r"(set_size)
	        	:"x0","x1"
	    	);
	    	
			//copy data from the source_info to the dest_info
	    	void __iomem *real_task_data = ioremap(dest_info, set_size);
	    	copy_from_user(real_task_data,(void *)source_info,set_size);

			cur_datainfo_idx+=1;
		}

	    // 3. Terminate the transmission, let the EL3 to protect the data
		// [input] x1: app_id

	    asm volatile(
	        "ldr x0,=0xc7000006\n"
	        "mov x1,%0\n"
	        "smc #0\n"
	        :
	        :"r"(app_id)
	        :"x0","x1"
	    );
    }
    else if(cmd==0x2){
        uint64_t app_id=cmd_args[0];
		
		bool hasit=false;
		int idx=0;
		while (idx<10){
			if(total_kctx[idx].kctx_pid==current->pid){
				hasit=true;
				total_kctx[idx].num_apps-=1;
				if(total_kctx[idx].num_apps==0){
					total_kctx[idx].kctx_addr=0x0;	
					total_kctx[idx].kctx_pid=0xffffffff;
				}
				break;
			}
			idx+=1;
		}
		if(!hasit){
			printk("Error: Cannot find the pid.\n");
		}

	    asm volatile(
	        "ldr x0,=0xc7000009\n"
	        "mov x1,%0\n"
	        "smc #0\n"
	        :
	        :"r"(app_id)
	        :"x0","x1"
	    );
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
	while (idx<10){
		total_kctx[idx].kctx_addr=0x0;	
		total_kctx[idx].kctx_pid=0xffffffff;
		total_kctx[idx].num_apps=0;
		idx+=1;
	}
	//Initialize stop stubmit
	stop_submit=0;
	num_update_pte_sections=0;
	next_available_pte_id=0;


	updated_entries=ioremap(0x89f000000, 0x01000000);
	printk("Initialized ccaext_realm_driver successfully!\n");
    return 0;	
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
