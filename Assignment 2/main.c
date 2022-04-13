#include <linux/genhd.h> 
#include <linux/blkdev.h> 
#include <linux/hdreg.h> 
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <asm/segment.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))
#define SECTOR_SIZE 512
#define MBR_SIZE SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 
#define PARTITION_TABLE_SIZE 64 
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55
#define DOF_FIRST_MINOR 0
#define DOF_MINOR_CNT 16
#define DOF_SECTOR_SIZE 512
#define DOF_DEVICE_SIZE 1024 
static u8 *dev_data;
static u_int mjr_no_dof = 0;  
typedef struct
{
	unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
	unsigned char start_head;
	unsigned char start_sec:6;
	unsigned char start_cyl_hi:2;
	unsigned char start_cyl;
	unsigned char part_type;
	unsigned char end_head;
	unsigned char end_sec:6;
	unsigned char end_cyl_hi:2;
	unsigned char end_cyl;
	unsigned int abs_start_sec;
	unsigned int sec_in_part;
} PartEntry;


typedef PartEntry PartTable[4]; 

//Two partitions created here
static PartTable def_part_table =
{
	{
		boot_type: 0x00,
		start_head: 0x00,
		start_sec: 0x2,
		start_cyl: 0x00,
		part_type: 0x83,
		end_head: 0x00,
		end_sec: 0x20,
		end_cyl: 0x09,
		abs_start_sec: 0x00000001,
		sec_in_part: 0x0000013F
	},
	{
		boot_type: 0x00,
		start_head: 0x00,
		start_sec: 0x1,
		start_cyl: 0x14,
		part_type: 0x83,
		end_head: 0x00,
		end_sec: 0x20,
		end_cyl: 0x1F,
		abs_start_sec: 0x00000280,
		sec_in_part: 0x00000180
	},
         
};


static void copy_mbr(u8 *disk)
{
struct file *DoF;
loff_t offset1=510;
loff_t offset2=446;
char data='A';
	memset(disk, 0x0, MBR_SIZE);
	*(unsigned long *)(disk + MBR_DISK_SIGNATURE_OFFSET) = 0x36E5756D;
	memcpy(disk + PARTITION_TABLE_OFFSET, &def_part_table, PARTITION_TABLE_SIZE);
	*(unsigned short *)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
	
	DoF = filp_open("/etc/Sample.txt",  O_RDWR|O_CREAT, 0666);
       kernel_write(DoF,&data,MBR_SIGNATURE_SIZE, &offset1);
       kernel_write(DoF,&def_part_table,PARTITION_TABLE_SIZE, &offset2);
	filp_close(DoF,NULL); 
        
}


int dofdevice_init(void)
{
	dev_data = vmalloc(DOF_DEVICE_SIZE * DOF_SECTOR_SIZE);
	if (dev_data == NULL)
		return -ENOMEM;
	copy_mbr(dev_data);
	return DOF_DEVICE_SIZE;
}


void dofdevice_cleanup(void)
{
	vfree(dev_data);
}


void DoF_write(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
struct file *DoF;
loff_t offset;
		offset=(loff_t)sector_off*DOF_SECTOR_SIZE;
	memcpy(dev_data + sector_off * DOF_SECTOR_SIZE, buffer,
		sectors * DOF_SECTOR_SIZE);
		
	DoF = filp_open("/etc/Sample.txt",  O_WRONLY, 0666);
 	kernel_write(DoF,buffer,sectors * DOF_SECTOR_SIZE,&offset);
	filp_close(DoF,NULL); 
}


void DoF_read(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
	struct file *DoF;
loff_t offset;
		offset=(loff_t)sector_off*DOF_SECTOR_SIZE;
	memcpy(buffer, dev_data + sector_off * DOF_SECTOR_SIZE,
		sectors * DOF_SECTOR_SIZE);
		
        DoF = filp_open("/etc/Sample.txt",  O_RDONLY, 0666);
	kernel_read(DoF,buffer, sector_off*DOF_SECTOR_SIZE, &offset);
	filp_close(DoF,NULL);    
}


static struct dof_device
{
	unsigned int size;
	spinlock_t lock;
	struct request_queue *dof_queue;
	struct gendisk *dof_disk;
} dof_dev;

static int dof_open(struct block_device *bdev, fmode_t mode)
{
	unsigned unit = iminor(bdev->bd_inode);

	printk(KERN_INFO "dof: Device is opened\n");
	printk(KERN_INFO "dof: Inode number is %d\n", unit);

	if (unit > DOF_MINOR_CNT)
		return -ENODEV;
	return 0;
}


static void dof_close(struct gendisk *disk, fmode_t mode)
{
	printk(KERN_INFO "dof: Device is closed\n");
}

static int dof_transfer(struct request *req)
{

	int dir = rq_data_dir(req);
	sector_t start_sector = blk_rq_pos(req);
	unsigned int sector_cnt = blk_rq_sectors(req);
	struct bio_vec bv;
	struct req_iterator iter;

	sector_t sector_offset;
	unsigned int sectors;
	u8 *buffer;

	int ret = 0;

	sector_offset = 0;
	rq_for_each_segment(bv, req, iter)
	{
		buffer = page_address((bv).bv_page) + (bv).bv_offset;
		if ((bv).bv_len % DOF_SECTOR_SIZE != 0)
		{
			printk(KERN_ERR "dof: Should never happen: "
				"bio size (%d) is not a multiple of DOF_SECTOR_SIZE (%d).\n"
				"This may lead to data truncation.\n",
				(bv).bv_len, DOF_SECTOR_SIZE);
			ret = -EIO;
		}
		sectors = (bv).bv_len / DOF_SECTOR_SIZE;
		printk(KERN_DEBUG "dof: Start Sector: %llu, Sector Offset: %llu; Buffer: %p; Length: %u sectors\n",
			(unsigned long long)(start_sector), (unsigned long long)(sector_offset), buffer, sectors);
		if (dir == WRITE)
		{
			DoF_write(start_sector + sector_offset, buffer, sectors);
		}
		else
		{
			DoF_read(start_sector + sector_offset, buffer, sectors);
		}
		sector_offset += sectors;
	}
	if (sector_offset != sector_cnt)
	{
		printk(KERN_ERR "dof: bio info doesn't match with the request info");
		ret = -EIO;
	}

	return ret;
}
	
static void dof_request(struct request_queue *q)
{
	struct request *req;
	int ret;
	while ((req = blk_fetch_request(q)) != NULL)
	{
		ret = dof_transfer(req);
		__blk_end_request_all(req, ret);
	}
}

static struct block_device_operations dof_fops =
{
	.owner = THIS_MODULE,
	.open = dof_open,
	.release = dof_close,
};
	
static int __init dof_init(void)
{

	int ret;

	if ((ret = dofdevice_init()) < 0)
	{
		return ret;
	}
	dof_dev.size = ret;

	mjr_no_dof = register_blkdev(mjr_no_dof, "dof");
	if (mjr_no_dof <= 0)
	{
		printk(KERN_ERR "dof: Unable to get Major Number\n");
		dofdevice_cleanup();
		return -EBUSY;
	}

	spin_lock_init(&dof_dev.lock);
	dof_dev.dof_queue = blk_init_queue(dof_request, &dof_dev.lock);
	if (dof_dev.dof_queue == NULL)
	{
		printk(KERN_ERR "dof: blk_init_queue failure\n");
		unregister_blkdev(mjr_no_dof, "dof");
		dofdevice_cleanup();
		return -ENOMEM;
	}
	
	dof_dev.dof_disk = alloc_disk(DOF_MINOR_CNT);
	if (!dof_dev.dof_disk)
	{
		printk(KERN_ERR "dof: alloc_disk failure\n");
		blk_cleanup_queue(dof_dev.dof_queue);
		unregister_blkdev(mjr_no_dof, "dof");
		dofdevice_cleanup();
		return -ENOMEM;
	}


	dof_dev.dof_disk->major = mjr_no_dof;
	dof_dev.dof_disk->first_minor = DOF_FIRST_MINOR;
	dof_dev.dof_disk->fops = &dof_fops;
	dof_dev.dof_disk->private_data = &dof_dev;
	dof_dev.dof_disk->queue = dof_dev.dof_queue;
	sprintf(dof_dev.dof_disk->disk_name, "dof");
	set_capacity(dof_dev.dof_disk, dof_dev.size);
	add_disk(dof_dev.dof_disk);
	printk(KERN_INFO "dof: DOF Block driver initialised (%d sectors; %d bytes)\n",
		dof_dev.size, dof_dev.size * DOF_SECTOR_SIZE);
	return 0;
}

static void __exit dof_cleanup(void)
{       


	del_gendisk(dof_dev.dof_disk);
	put_disk(dof_dev.dof_disk);
	blk_cleanup_queue(dof_dev.dof_queue);
	unregister_blkdev(mjr_no_dof, "dof");
	dofdevice_cleanup();
}

module_init(dof_init);
module_exit(dof_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mark Ronald P <h20210156@pilani.bits-pilani.ac.in>");
MODULE_DESCRIPTION("Disk_on_File");
MODULE_ALIAS_BLOCKDEV_MAJOR(mjr_no_dof);
