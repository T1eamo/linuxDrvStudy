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
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include "mpu6050_drv.h"
#include <linux/cdev.h>


// static int16_t gyro_16_data[3];
// static int16_t accer_16_data[3];
// static int16_t mpu6050data[12];
struct ybimu_data {
    u8 	accel[6];  //加速度计
    u8 	gyro[6];   //陀螺仪
    u8 	mag[6];	   //磁力计  
	u8  quat_raw[16];  //  四元数  4个 float
    u8  euler_raw[12]; //  欧拉角   3个 float，每个4字节
	u8  baro_raw[16];   //  气压计  高度, 温度, 气压, 气压对比值 (每个 4 字节 float)
};

struct ybimu_data ybimuData;
struct mpu6050_dev{

	dev_t mpu6050_id;
	struct cdev mpu6050_dev;
	struct class *mpu6050_class;
	struct device *mpu6050_device;

    struct device_node *nd;
	struct i2c_client *mpu6050_client;

	int major;
	int minor;
};
struct mpu6050_dev mpu6050cdev;
//3.实现相应的write等函数

static int mpu6050_read_regs(struct mpu6050_dev *dev,u8 reg,void *val,int len)
{
	struct i2c_client *client = dev->mpu6050_client;
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;  //从机地址
	msg[0].flags = 0;           //写标志
	msg[0].buf = &reg;          //写数据地址，这里是要访问的寄存器地址
	msg[0].len = 1;				//写数据的长度

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;           //读标志
	msg[1].buf = val;
	msg[1].len = len;
	return i2c_transfer(client -> adapter, msg, 2);

}
// static int mpu6050_write_regs(struct mpu6050_dev *dev, u8 reg, u8 *buf, u8 len)
// {
// 	u8 b[256];
// 	struct i2c_msg msg;
// 	struct i2c_client *client = dev->mpu6050_client;
	
// 	b[0] = reg;					/* 寄存器首地址 */
// 	memcpy(&b[1],buf,len);		/* 将要写入的数据拷贝到数组b里面 */
		
// 	msg.addr = client->addr;	/* mpu6050i2c地址 */
// 	msg.flags = 0;				/* 标记为写数据 */
// 	msg.buf = b;				/* 要写入的数据缓冲区 */
// 	msg.len = len + 1;			/* 要写入的数据长度 */

// 	return i2c_transfer(client->adapter, &msg, 1);
// }

static ssize_t mpu6050_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
		
		return 0;
}


static ssize_t mpu6050_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int err;
	size_t data_len = sizeof(struct ybimu_data);

	if (size < data_len)
		return -EINVAL;

	err = mpu6050_read_regs(&mpu6050cdev, YBIMU_REG_RAW_ACCEL, ybimuData.accel, 6);
	if (err < 0) return err;
	err = mpu6050_read_regs(&mpu6050cdev, YBIMU_REG_RAW_GYRO, ybimuData.gyro, 6);
	if (err < 0) return err;
	err = mpu6050_read_regs(&mpu6050cdev, YBIMU_REG_RAW_MAG, ybimuData.mag, 6);
	if (err < 0) return err;
	err = mpu6050_read_regs(&mpu6050cdev, YBIMU_REG_QUAT, ybimuData.quat_raw, 16);
	if (err < 0) return err;
	err = mpu6050_read_regs(&mpu6050cdev, YBIMU_REG_EULER, ybimuData.euler_raw, 12);
	if (err < 0) return err;
	err = mpu6050_read_regs(&mpu6050cdev, YBIMU_REG_BARO, ybimuData.baro_raw, 16);
	if (err < 0) return err;

	if (copy_to_user(buf, &ybimuData, data_len))
		return -EFAULT;

	return data_len;
	return 0;

}
static int mpu6050_open (struct inode *node, struct file *file)
{
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return 0;
}

static int mpu6050_release (struct inode *node, struct file *file)
{
		printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		return 0;
}


//2.定义自己的file_operations结构体
static struct file_operations mpu6050_fops = {
		.owner   = THIS_MODULE,
		.open    = mpu6050_open,
		.write   = mpu6050_write,
		.read    = mpu6050_read,
		.release = mpu6050_release,

};
static int cdevmpu_init(struct i2c_client *client){

	int err;   
	int ret;
	printk("the i2c dev init\r\n");
	//1.申请设备号
	//mpu6050cdev.major = register_chrdev(0,CDEV_NAME,&mpu6050_fops);  //注册设备文件结构体   声明设备名称
	ret = alloc_chrdev_region(&mpu6050cdev.mpu6050_id, 0, 1, CDEV_NAME); //动态分配设备号
	if (ret < 0) {
		printk("alloc_chrdev_region failed: %d\n", ret);
		return ret;
	}
	mpu6050cdev.major = MAJOR(mpu6050cdev.mpu6050_id);
	mpu6050cdev.minor = MINOR(mpu6050cdev.mpu6050_id);
	//2.初始化cdev结构体
	cdev_init(&mpu6050cdev.mpu6050_dev, &mpu6050_fops);
	mpu6050cdev.mpu6050_dev.owner = THIS_MODULE;
	//3.添加cdev到内核
	err = cdev_add(&mpu6050cdev.mpu6050_dev, mpu6050cdev.mpu6050_id, 1);
	if (err) {
		unregister_chrdev_region(mpu6050cdev.mpu6050_id, 1);
		return err;
	}
	//4.创建设备类
	mpu6050cdev.mpu6050_class = class_create(THIS_MODULE, "mpu6050_class");//创建设备类
	if (IS_ERR(mpu6050cdev.mpu6050_class)){
			err = PTR_ERR(mpu6050cdev.mpu6050_class);
			cdev_del(&mpu6050cdev.mpu6050_dev);
			unregister_chrdev_region(mpu6050cdev.mpu6050_id, 1);
			printk("class_create failed: %d\n", err);
			return err;
	}
	//5.创建设备
	mpu6050cdev.mpu6050_device = device_create(mpu6050cdev.mpu6050_class, NULL, mpu6050cdev.mpu6050_id, NULL, CDEV_NAME);
	if (IS_ERR(mpu6050cdev.mpu6050_device)) {
		err = PTR_ERR(mpu6050cdev.mpu6050_device);
		class_destroy(mpu6050cdev.mpu6050_class);
		cdev_del(&mpu6050cdev.mpu6050_dev);
		unregister_chrdev_region(mpu6050cdev.mpu6050_id, 1);
		return err;
	}
	printk("the i2c dev create\r\n");

	return 0;
}

static const struct of_device_id mpu6050_of_match_table[] = {
	{ .compatible = "topeet,imu"},
	{/* sentinel */},
};
static const struct i2c_device_id mpu6050_ids[] = {
	{ "topeet,imu",0},
	{},
};
static int mpu6050_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    //搭建字符设备驱动框架 使其生成字符设备驱动  /dev/xxx 便于用户操作
	int ret;
	mpu6050cdev.mpu6050_client = client;
	ret = cdevmpu_init(client);
	return ret;
}
static int mpu6050_remove(struct i2c_client *client)
{

    unregister_chrdev(mpu6050cdev.major,CDEV_NAME);	//卸载已注册设备（解注册）
	device_destroy(mpu6050cdev.mpu6050_class,MKDEV(mpu6050cdev.major, 0));
	class_destroy(mpu6050cdev.mpu6050_class);
  												//销毁设备
                                				//销毁设备类
	return 0;
}
static struct i2c_driver mpu6050_driver = {   

	.driver = {
		.name = "topeet,imu",
		.of_match_table = mpu6050_of_match_table,
		.owner = THIS_MODULE,
	},
	.probe = mpu6050_probe,
	.remove	= mpu6050_remove,
	.id_table = mpu6050_ids,
};


//4.定义入口函数，用来注册驱动程序
static int __init mpu6050_init(void)
{   
	return i2c_add_driver(&mpu6050_driver);  //1.注册i2c驱动
}
//出口函数
static void __exit mpu6050_exit(void)
{	
	i2c_del_driver(&mpu6050_driver);          //end -- 删除i2c驱动
	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);
		
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);

MODULE_LICENSE("GPL");





