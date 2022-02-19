
 // main.c - Create an input/output character device
 

#include <linux/kernel.h>       // We're doing kernel work 
#include <linux/module.h>       // Specifically, a module
#include <linux/fs.h>
#include <asm/uaccess.h>        // for get_user and put_user
#include <linux/time.h>
#include<linux/init.h>
#include <linux/random.h>
#include<linux/moduleparam.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>


 // The major device number. We can't rely on dynamic
 // registration any more, because ioctls need to know
 // it.
 
#define MAJOR_NUM 100

 //Select the channel of the adc 

#define IOCTL_SELECT_CHANNEL _IOR(MAJOR_NUM, 0, int *)

 //Select the alignment of the adc

#define IOCTL_SELECT_ALIGNMENT _IOR(MAJOR_NUM, 1, char *)
 
 //Select the mode of operation of the adc

#define IOCTL_SELECT_MODE _IOR(MAJOR_NUM, 2, char *)

 // The name of the device file is adc-dev
 
#define DEVICE_FILE_NAME "/dev/adc-dev"

#define SUCCESS 0

#define DEVICE_NAME "adc-dev"

int temp_1;
char temp_2;
char temp_3;

 //random generator function

uint16_t random(void)
{
uint16_t number;
get_random_bytes(&number,sizeof(number));
number&=0x0fff;
return number;
}

 //alignment function
 
uint16_t align(uint16_t temp)
{
if(temp_2=='L')
temp=temp<<4;
return temp;
}


 // Is the device open right now? Used to prevent
 // concurent access into the same device

static int Device_Open = 0;

 static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
        printk(KERN_INFO "device_open(%p)\n", file);
#endif

    
 // We don't want to talk to two processes at the same time
     
    if (Device_Open)
        return -EBUSY;

    Device_Open++;
    
    try_module_get(THIS_MODULE);
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
    printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif

    
 // We're now ready for our next caller
     
    Device_Open--;

    module_put(THIS_MODULE);
    return SUCCESS;
}


 // This function is called whenever a process which has already opened the
 // device file attempts to read from it.

static ssize_t device_read(struct file *file,   // see include/linux/fs.h   
                           char __user * buffer,// buffer to be
                                                // filled with data 
                           size_t length,       // length of the buffer   
                           loff_t * offset)
{
   uint16_t adc_reg;
   adc_reg=random();
   printk(KERN_INFO "Random 12 bit is %x",adc_reg);
   adc_reg=align(adc_reg);
   printk(KERN_INFO "Aligned adc register is %x",adc_reg);
   copy_to_user(buffer,&adc_reg,sizeof(adc_reg));
   return sizeof(adc_reg);
}

 // This function is called whenever a process tries to do an ioctl on our
 // device file. We get two extra parameters (additional to the inode and file
 // structures, which all device functions get): the number of the ioctl called
 // and the parameter given to the ioctl function.
 
 // If the ioctl is write or read/write (meaning output is returned to the
 // calling process), the ioctl call returns the output of this function.
 
long device_ioctl(struct file *file,             // ditto
                  unsigned int ioctl_num,        // number and param for ioctl 
                  unsigned long ioctl_param)
{
    
 // Switch according to the ioctl called
     
    switch (ioctl_num) {
  
	case IOCTL_SELECT_CHANNEL:
	temp_1=ioctl_param;
	printk(KERN_INFO "The channel selected is %d",temp_1);
	break;
	
	case IOCTL_SELECT_MODE:
	temp_3=(char)ioctl_param;
	printk(KERN_INFO "The mode selected is %c", temp_3);
	break;

	case IOCTL_SELECT_ALIGNMENT:
	temp_2=(char)ioctl_param;
	printk(KERN_INFO "The alignment selected is %c",temp_2);
	break;
	
    }

    return SUCCESS;
}




 // Module Declarations


 // This structure will hold the functions to be called
 // when a process does something to the device we
 // created. Since a pointer to this structure is kept in
 // the devices table, it can't be local to
 // init_module. NULL is for unimplemented functions.
 
struct file_operations Fops = {
        .read = device_read,
        .unlocked_ioctl = device_ioctl,
        .open = device_open,
        .release = device_release,      // a.k.a. close 
};


 // Initialize the module - Register the character device
 
int init_module()
{
    int ret_val;
    
 // Register the character device
     
    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

    
 // Negative values signify an error
     
    if (ret_val < 0) 
    {
        printk(KERN_ALERT "%s failed with %d\n",
               "Sorry, registering the character device ", ret_val);
        return ret_val;
   }
   
    printk(KERN_INFO "%s The major device number is %d.\n","Registeration is a success", MAJOR_NUM);
    printk(KERN_INFO "If you want to talk to the device driver,\n");
    printk(KERN_INFO "you'll have to create a device file. \n");
    printk(KERN_INFO "We suggest you use:\n");
    printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
    printk(KERN_INFO "The device file name is important, because\n");
    printk(KERN_INFO "the ioctl program assumes that's the\n");
    printk(KERN_INFO "file you'll use.\n");
	return 0;	
}

 // Cleanup - unregister the appropriate file from /proc
 
void cleanup_module()
{
    
 // Unregister the device
     
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "Bye: adc driver unregistered\n\n");
}


MODULE_DESCRIPTION("OUr first ADC driver");
MODULE_AUTHOR("Mark Ronald <markronald42@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_INFO(ChipSupport, "PCA9687, CH340G");
