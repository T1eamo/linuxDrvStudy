#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>	
#include <linux/gfp.h>
//1.确定主设备号         0代表系统随机分配
static int major = 0;
static char kernel_buf[1024];
#define MIN(a,b) (a < b ? a : b)

static struct class *hello_class;


//3.实现相应的write等函数
static ssize_t hello_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
		int err;
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		err = copy_to_user(buf, kernel_buf, MIN(size,1024));
		return MIN(size,1024);

}

static ssize_t hello_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
		int err;
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		err = copy_from_user(kernel_buf, buf, MIN(size,1024));
		return MIN(size,1024);
}
static int hello_drv_open (struct inode *node, struct file *file)
{
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return 0;
}

static int hello_drv_release (struct inode *node, struct file *file)
{
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return 0;
}




//2.定义自己的file_operations结构体
static struct file_operations hello_drv = {
		.owner   = THIS_MODULE,
		.open    = hello_drv_open,
		.read    = hello_drv_read,
		.write   = hello_drv_write,
		.release = hello_drv_release,

};
//4.定义入口函数，用来注册驱动程序
static int __init hello_init(void)
{
		int err;
		major = register_chrdev(0,"hello",&hello_drv);
		
		hello_class = class_create(THIS_MODULE, "hello_class");
		err = PTR_ERR(hello_class);
		if (IS_ERR(hello_class)){
			unregister_chrdev(major,"hello");
		 	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
			return -1;
			}
		device_create(hello_class,NULL,MKDEV(major, 0),NULL,"hello");
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return 0;
}

static void __exit hello_exit(void)

{
		device_destroy(hello_class,MKDEV(major, 0));
		class_destroy(hello_class);
		unregister_chrdev(major,"hello");
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		
		
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");





