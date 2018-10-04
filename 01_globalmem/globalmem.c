#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>


#define GLOBALMEM_SIZE 0x1000  /* 4k */
#define MEM_CLEAR 0x1

//TODO: change the method by allocate automatically
#define GLOBALMEM_MAJOR 230 
static struct class *simple_class;
static int globalmem_major = GLOBALMEM_MAJOR;

/* module param is used for passing info from outside
 * such as, "insmod hello.ko num=1"
 * cat /sys/module/<module_name>/parameters/
 * to see the params
 */
module_param(globalmem_major,int, S_IRUGO);

/* define the globalmem self-defined struct 
* this is the dev entity
* includes
* 1. cdev struct- will include file ops
* 2. data structure mem
*/
struct globalmem_dev {
	struct cdev cdev;
	unsigned char mem[GLOBALMEM_SIZE];
};

/* global variable*/
struct globalmem_dev *globalmem_devp;


//TODO: how to set the function definition? 
static int globalmem_open(struct inode *inode, struct file *filp)
{
	filp->private_data = globalmem_devp;
	printk (KERN_INFO "globalmem is opened\n");
	return 0;
}

static int globalmem_release(struct inode *inode, struct file *filp)
{
	printk (KERN_INFO "globalmem is released\n");
	return 0;
}

static long globalmem_ioctl(struct file *filp,unsigned int cmd,
			unsigned long arg)
{


}

/*read n(size) bytes from file start from ppos to buf(user space)*/
static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size,
loff_t *ppos)
{
	unsigned long p = *ppos; /*start pos*/
	unsigned int count = size; /*read size*/
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	/*if start ppos is larger than data size*/
	if (p >= GLOBALMEM_SIZE)
		return 0;

	/*if count to read is larger than data left*/
	if (count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	/*copy start from globalmem->mem+p, and n(count) bytes
	 * to buf in user space
	 */
	if (copy_to_user(buf,dev->mem + p , count)) {
		ret = -EFAULT;	
	}
	else {/*return 0 means successful*/
		*ppos += count;/*return current ppos*/
		ret = count;/* number of bytes has been read*/
		printk (KERN_INFO "read %u bytes(s) from %lu\n",count, p);
	}
	return ret;

}
	
static ssize_t globalmem_write(struct file *filp, const char __user *buf,
size_t size, loff_t *ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev *dev = filp->private_data;

	if (p >= GLOBALMEM_SIZE)
		return 0;
	if (count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	if (copy_from_user(dev->mem + p, buf, count))
		ret = -EFAULT;
	else {
		*ppos += count;
		ret = count;

		printk(KERN_INFO "written %u bytes(s) from %lu\n", count, p);
	}

	return ret;
} 

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
}


/* file operation fuctions
 * you can use sys call like
 *	write
 *	read
 *	open
 *	close
 * to control the device
 */
static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
	.open = globalmem_open,
	.release = globalmem_release,	
};


/* setup cdev(character device) object 
 * including:
 * 	init cdev
 * 	pass the file opsi
 * 	report cdev object to the system
 */
static void globalmem_setup_cdev(struct globalmem_dev *dev, int index)
{
	/* MKDEV is a program to create a device
	 * set a major number  
	 * the func can be used when you need a devno
	 */
	int err,devno = MKDEV(globalmem_major,index);
	
	/* pass file operation struct to the device 
	 * so that we can use syscall to control the device later
	 */	
	cdev_init(&dev->cdev,&globalmem_fops); /*pass the file ops 
						*struct to cdev
						*/
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev,devno,1);/* pass the cdev object to  
					    * kernel system
					    */	
	if(err)
		printk(KERN_NOTICE "Error %d adding globalmem%d", err, index);

}
static int __init globalmem_init(void)
{
	/*register for device number*/
	int ret;
	dev_t devno = MKDEV(globalmem_major,0);

	//step1: register a dev number	
	if(globalmem_major)
		ret = register_chrdev_region(devno,1,"globalmem");
	else {
		ret = alloc_chrdev_region(&devno,0,1,"globalmem");
	}

	if(ret <0)
		return ret;

	//step2: alloc memory
	/* allocate space for the device struct globalmem_dev
	 * using kzalloc	 
	 * and get a pointer for the struct
	 */
	//TODO: why alloc here rather than use cdev_alloc
	//maybe it's becasue cdev_alloc cannot alloc mem meanwhile
	globalmem_devp = kzalloc(sizeof(struct globalmem_dev),GFP_KERNEL);

	if(!globalmem_devp){
		ret = -ENOMEM;
		goto fail_malloc;	

	}
	
	//step3: setup cdev object
	/*
	 * init cdev
	 * pass the file opsi
	 * report cdev object to the system
	 */
	globalmem_setup_cdev(globalmem_devp,0);	

	//TODO: add some error protection here
	simple_class = class_create(THIS_MODULE,"globalmem");
	
	device_create(simple_class,NULL,devno,0,"globalmem");

	
	printk (KERN_INFO "globalmem initialized\n");
	return 0;

fail_malloc:
	unregister_chrdev_region(devno,1);
	return ret;

}


module_init(globalmem_init);

static void __exit globalmem_exit(void)
{
	device_destroy(simple_class,MKDEV(globalmem_major,0));
	class_destroy(simple_class);

	cdev_del(&globalmem_devp->cdev);
	kfree(globalmem_devp);
	unregister_chrdev_region(MKDEV(globalmem_major,0),1);
	
	printk(KERN_INFO "globalmem exit\n");
}

module_exit(globalmem_exit);



MODULE_AUTHOR("Barry Song <baohua@kernel.org>");
MODULE_LICENSE("GPL v2");
