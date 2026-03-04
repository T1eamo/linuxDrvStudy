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
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

//1.确定主设备号         0代表系统随机分配
#define LED_NAME "led"
#define LEDOFF 0/*关闭*/
#define LEDON  1/*打开*/


struct gpioled_dev{
	int major;
	struct class *led_class;
    struct device_node *nd;
	int gpio_led;
};

//static struct class *led_class;

//struct gpioled_dev gpio_stru;
struct gpioled_dev gpio_stru = {
    .major = 0,
    .led_class = NULL,
    .nd = NULL,
    .gpio_led = 0,
};
//3.实现相应的write等函数


static ssize_t led_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
		
		unsigned int ret;
		unsigned char databuf[1];
		ret =copy_from_user(databuf,buf,size);
		if(ret < 0){
			printk("open error");
			return -EFAULT;
		}
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		if(databuf[0] == LEDON){
			gpio_set_value(gpio_stru.gpio_led,1);
		}else if(databuf[0] == LEDOFF){
			gpio_set_value(gpio_stru.gpio_led,0);

		}
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
static struct file_operations led_drv = {
		.owner   = THIS_MODULE,
		.open    = led_open,
		.write   = led_write,
		.release = led_release,

};

struct of_device_id led_of_match[] = {//这是一个数组，里面可以有多个匹配函数，只要有一项匹配就可以
	{.compatible = "topeet,led"}, //匹配函数   
	{/* sentinel */},           //结尾要有这个标志
};
static int led_probe(struct platform_device *dev){
	int err;   
	int ret;
	printk("led init\r\n");
	gpio_stru.major = register_chrdev(0,LED_NAME,&led_drv);  //注册设备文件结构体   声明设备名称
                                 
        if (gpio_stru.major < 0||gpio_stru.major == 0){
				printk("register field");
				return -EIO;
		}
	
	gpio_stru.led_class = class_create(THIS_MODULE, "led_class");//创建设备类
	err = PTR_ERR(gpio_stru.led_class);
	if (IS_ERR(gpio_stru.led_class)){
			unregister_chrdev(gpio_stru.major,LED_NAME);
		 	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
			return -1;
			}
	device_create(gpio_stru.led_class,NULL,MKDEV(gpio_stru.major, 0),NULL,LED_NAME);
    //  从设备树获取节点   
	gpio_stru.nd = dev->dev.of_node;  //可以直接获取节点 匹配成功后可以将设备信息自动转成结构体

	if(gpio_stru.nd == NULL){
        printk("cant find node");
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return -1;
	}
	//  获取gpio编号
	gpio_stru.gpio_led = of_get_named_gpio(gpio_stru.nd,"gpios",0);
	if(gpio_stru.gpio_led< 0){
			printk("cant find led num\r\n");
			return -1;
	}
	printk("the gpio id is %d",gpio_stru.gpio_led);
	//申请gpio
	ret = gpio_request(gpio_stru.gpio_led,"gpio_led");//第二个参数是标签名字，自己定义
	if(ret){
		printk("fail to request\r\n");
		return -1;
	}
	//使用io
	ret = gpio_direction_output(gpio_stru.gpio_led,0);
	if(ret){
		printk("dirction set err\r\n");
		return -1;
	}
	gpio_set_value(gpio_stru.gpio_led,1);
	return 0;
}
static int led_remove(struct platform_device *dev){
	printk("led remove\r\n");
	gpio_free(gpio_stru.gpio_led);              //释放io
    unregister_chrdev(gpio_stru.major,LED_NAME);	//卸载已注册设备（解注册）
	device_destroy(gpio_stru.led_class,MKDEV(gpio_stru.major, 0));
	class_destroy(gpio_stru.led_class);
  												//销毁设备
                                				//销毁设备类
	return 0;
}
struct platform_driver led_driver = {   
	.driver = {
		.name = "topeet_led",     //无设备树时，和设备进行匹配，驱动名字
		.of_match_table = led_of_match, //设备树匹配表
	},
	.probe = led_probe,
	.remove = led_remove,

};

//4.定义入口函数，用来注册驱动程序
static int __init led_init(void)
{   
	return platform_driver_register(&led_driver);
}
//出口函数
static void __exit led_exit(void)
{	
	platform_driver_unregister(&led_driver);
	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");





