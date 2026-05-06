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
//**************************类IIO增加头文件
#include <linux/hrtimer.h>
#include <linux/workqueue.h>    
#include <linux/kfifo.h>
#include <linux/poll.h>
//**************************


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
	//**************************类IIO增加成员变量
    struct hrtimer timer;       //定义一个高精度定时器对象。后面用它来周期性触发采样
    ktime_t period;              //保存定时周期。ktime_t 是内核表示时间的类型

    struct work_struct work;     //定义一个 workqueue 工作项。定时器触发后，不直接读取 MPU6050，而是调度这个 work 去读取。

    struct kfifo fifo;
    spinlock_t lock;

    wait_queue_head_t wq;
//**************************
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
//**************************类iio采集函数
static void mpu6050_work_func(struct work_struct *work)
{
    struct mpu6050_dev *dev =
    container_of(work, struct mpu6050_dev, work);
    unsigned long flags;

    /* 读IMU */
    mpu6050_read_regs(dev, YBIMU_REG_RAW_ACCEL, ybimuData.accel, 6);
    mpu6050_read_regs(dev, YBIMU_REG_RAW_GYRO, ybimuData.gyro, 6);
    mpu6050_read_regs(dev, YBIMU_REG_RAW_MAG, ybimuData.mag, 6);
    mpu6050_read_regs(dev, YBIMU_REG_QUAT, ybimuData.quat_raw, 16);
    mpu6050_read_regs(dev, YBIMU_REG_EULER, ybimuData.euler_raw, 12);
    mpu6050_read_regs(dev, YBIMU_REG_BARO, ybimuData.baro_raw, 16);

    /* 写入fifo */
    spin_lock_irqsave(&dev->lock, flags);
    kfifo_in(&dev->fifo, &ybimuData, sizeof(ybimuData));
    spin_unlock_irqrestore(&dev->lock, flags);

    /* 唤醒用户态 */
    wake_up_interruptible(&dev->wq);
}
//**************************

//**************************类IIO定时器函数
static enum hrtimer_restart mpu6050_timer_func(struct hrtimer *t)   //定义 hrtimer 的回调函数。每次定时器到期时，内核会调用它
{
    struct mpu6050_dev *dev =
        container_of(t, struct mpu6050_dev, timer);

    hrtimer_forward_now(t, dev->period);   //把定时器的下一次到期时间向后推进一个周期。这样定时器会周期性地到期，触发回调函数。

    schedule_work(&dev->work);             //调度工作队列，让 mpu6050_work_func() 稍后在进程上下文中执行

    return HRTIMER_RESTART;                 //告诉内核：这个定时器还要继续运行
}
//**************************

static ssize_t mpu6050_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
		
		return 0;
}


static ssize_t mpu6050_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    unsigned int copied;
    int ret;

//**************************  类IIO读函数   从buffer
    if (kfifo_is_empty(&mpu6050cdev.fifo)) {

        if (file->f_flags & O_NONBLOCK)
            return -EAGAIN;

        wait_event_interruptible(mpu6050cdev.wq,
            !kfifo_is_empty(&mpu6050cdev.fifo));
    }

    ret = kfifo_to_user(&mpu6050cdev.fifo, buf, size, &copied);

    return ret ? ret : copied;
//**************************
}
static int mpu6050_open (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);

//**************************
    hrtimer_start(&mpu6050cdev.timer,    //打开设备时启动定时器
                  mpu6050cdev.period,
                  HRTIMER_MODE_REL);
//**************************

	return 0;
}

static int mpu6050_release (struct inode *node, struct file *file)
{
	printk("%s %s line %d\n",__FILE__,__FUNCTION__,__LINE__);

//**************************
    hrtimer_cancel(&mpu6050cdev.timer);
	flush_work(&mpu6050cdev.work);
//**************************

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
	//**************************
    /* hrtimer */
    hrtimer_init(&mpu6050cdev.timer,                              //初始化 hrtimer
                 CLOCK_MONOTONIC,                                 //使用单调递增时钟。它不会因为系统时间被用户修改而倒退，适合做周期定时
                 HRTIMER_MODE_REL);                               //使用相对时间模式。也就是说后面启动定时器时，时间表示“从现在开始多久后触发
    mpu6050cdev.timer.function = mpu6050_timer_func;              //设置定时器到期时调用的回调函数。
    mpu6050cdev.period = ktime_set(0, 10 * 1000 * 1000); // 100Hz

    /* workqueue */
    INIT_WORK(&mpu6050cdev.work, mpu6050_work_func);

    /* fifo */
    kfifo_alloc(&mpu6050cdev.fifo, 4096, GFP_KERNEL);

    spin_lock_init(&mpu6050cdev.lock);
    init_waitqueue_head(&mpu6050cdev.wq);
//**************************
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
//**************************
    kfifo_free(&mpu6050cdev.fifo);
    hrtimer_cancel(&mpu6050cdev.timer);
	flush_work(&mpu6050cdev.work);
//**************************
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





