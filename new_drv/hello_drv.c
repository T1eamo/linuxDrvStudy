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
#include <linux/cdev.h>

#define NEWCHRLED_CNT 1 /* 设备号个数 */
#define NEWCHRLED_NAME "newchrled" /* 名字 */
 #define LEDOFF 0 /* 关灯 */
#define LEDON 1 /* 开灯 */


/*设备结构体*/
struct newchrled_dev{
 dev_t devid; /* 设备号 */
 struct cdev cdev; /* cdev */
 struct class *class; /* 类 */
 struct device *device; /* 设备 */

 int major; /* 主设备号 */
 int minor; /* 次设备号 */
 };                                  
 struct newchrled_dev newchrled; /* led 设备 */


//3.实现相应的write等函数
static ssize_t led_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{

		return 0;

}

static ssize_t led_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{

		return 0;
}
static int led_open (struct inode *node, struct file *file)
{
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return 0;
}

static int led_release (struct inode *node, struct file *file)
{
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return 0;
}




//2.定义自己的file_operations结构体
static struct file_operations led_drv_fops = {
		.owner   = THIS_MODULE,
		.open    = led_open,
		.read    = led_read,
		.write   = led_write,
		.release = led_release,

};

//4.定义入口函数，用来注册驱动程序
static int __init led_init(void)
{
	int ret;
	/* 注册字符设备驱动 */
	/* 1、创建设备号 */
	if (newchrled.major) { /* 定义了设备号 */
		newchrled.devid = MKDEV(newchrled.major, 0);
		ret = register_chrdev_region(newchrled.devid, NEWCHRLED_CNT,NEWCHRLED_NAME);
		if (ret < 0) {
			pr_err("cannot register %s char driver [ret=%d]\n",
			       NEWCHRLED_NAME, NEWCHRLED_CNT);
			goto fail_map;
		}
	} else { /* 没有定义设备号 */
		ret = alloc_chrdev_region(&newchrled.devid, 0,NEWCHRLED_CNT,NEWCHRLED_NAME); /* 申请设备号 */
				//申请多个连续的设备号
		if (ret < 0) {
			pr_err("%s Couldn't alloc_chrdev_region, ret=%d\r\n",
			       NEWCHRLED_NAME, ret);
			goto fail_map;
		}
		newchrled.major = MAJOR(newchrled.devid);/* 获取主设备号 */
		newchrled.minor = MINOR(newchrled.devid);/* 获取次设备号 */
	}
	printk("newcheled major=%d,minor=%d\r\n", newchrled.major,
	       newchrled.minor);

	/* 2、初始化 cdev */
	newchrled.cdev.owner = THIS_MODULE;
	cdev_init(&newchrled.cdev, &led_drv_fops);

	/* 3、添加一个 cdev */
	ret = cdev_add(&newchrled.cdev, newchrled.devid,
		       NEWCHRLED_CNT);
	if (ret < 0)
		goto del_unregister;
	/* 4、创建类 */
	newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.class)) {
		goto del_cdev;
	}
	/* 5、创建设备 */
	newchrled.device = device_create(newchrled.class, NULL,
				 newchrled.devid, NULL, NEWCHRLED_NAME);
	if (IS_ERR(newchrled.device)) {
		goto destroy_class;
	}


	return 0;

destroy_class:
	class_destroy(newchrled.class);
del_cdev:
	cdev_del(&newchrled.cdev);
del_unregister:
	unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
fail_map:
	return -EIO;
}

static void __exit led_exit(void)

{
		/* 清理：按注册顺序逆序释放 */
		device_destroy(newchrled.class, newchrled.devid);
		class_destroy(newchrled.class);
		cdev_del(&newchrled.cdev);
		unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		
		
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");





