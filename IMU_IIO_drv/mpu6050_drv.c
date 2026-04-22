#include "mpu6050_drv.h"

#define MPU6050_NAME "mpu6050"

/* 针对 16-bit signed little-endian (小端有符号整数) 的传感器通道宏 */
#define YBIMU_INT16_CHANNEL(_type, _mod, _index) { \
    .type = _type, \
    .modified = 1, \
    .channel2 = _mod, \
    .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE), \
    .scan_index = _index, \
    .scan_type = { \
        .sign = 's', \
        .realbits = 16, \
        .storagebits = 16, \
        .endianness = IIO_LE, \
    }, \
}

/* 针对 32-bit float (小端浮点数) 的数据通道宏 */
#define YBIMU_FLOAT32_CHANNEL(_type, _mod, _index) { \
    .type = _type, \
    .modified = 1, \
    .channel2 = _mod, \
    .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED), \
    .scan_index = _index, \
    .scan_type = { \
        .sign = 's', \
        .realbits = 32, \
        .storagebits = 32, \
        .endianness = IIO_LE, \
    }, \
}

/* 针对 32-bit float 四元数的专属通道宏 */
#define YBIMU_QUAT_CHANNEL(_name, _index) { \
    .type = IIO_ROT, \
    .modified = 0, \
    .extend_name = _name, \
    .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED), \
    .scan_index = _index, \
    .scan_type = { \
        .sign = 's', \
        .realbits = 32, \
        .storagebits = 32, \
        .endianness = IIO_LE, \
    }, \
}


struct mpu6050_dev {
    struct i2c_client *client;
    struct mutex lock;
};

/* 修复了语法错误和格式的通道数组 */
static const struct iio_chan_spec mpu6050_channels[] = {
    /* --- 1. 加速度计 --- */
    YBIMU_INT16_CHANNEL(IIO_ACCEL, IIO_MOD_X, YBIMU_SCAN_ACCEL_X),
    YBIMU_INT16_CHANNEL(IIO_ACCEL, IIO_MOD_Y, YBIMU_SCAN_ACCEL_Y),
    YBIMU_INT16_CHANNEL(IIO_ACCEL, IIO_MOD_Z, YBIMU_SCAN_ACCEL_Z),

    /* --- 2. 陀螺仪 --- */
    YBIMU_INT16_CHANNEL(IIO_ANGL_VEL, IIO_MOD_X, YBIMU_SCAN_GYRO_X),
    YBIMU_INT16_CHANNEL(IIO_ANGL_VEL, IIO_MOD_Y, YBIMU_SCAN_GYRO_Y),
    YBIMU_INT16_CHANNEL(IIO_ANGL_VEL, IIO_MOD_Z, YBIMU_SCAN_GYRO_Z),

    /* --- 3. 磁力计 --- */
    YBIMU_INT16_CHANNEL(IIO_MAGN, IIO_MOD_X, YBIMU_SCAN_MAG_X),
    YBIMU_INT16_CHANNEL(IIO_MAGN, IIO_MOD_Y, YBIMU_SCAN_MAG_Y),
    YBIMU_INT16_CHANNEL(IIO_MAGN, IIO_MOD_Z, YBIMU_SCAN_MAG_Z),
	
    /* --- 6. 四元数 --- */
    YBIMU_QUAT_CHANNEL("quaternion_w", YBIMU_SCAN_QUAT_0),
    YBIMU_QUAT_CHANNEL("quaternion_x", YBIMU_SCAN_QUAT_1),
    YBIMU_QUAT_CHANNEL("quaternion_y", YBIMU_SCAN_QUAT_2),
    YBIMU_QUAT_CHANNEL("quaternion_z", YBIMU_SCAN_QUAT_3),

    /* --- 4. 欧拉角 --- */
    YBIMU_FLOAT32_CHANNEL(IIO_ANGL, IIO_MOD_X,  YBIMU_SCAN_EULER_ROLL),
    YBIMU_FLOAT32_CHANNEL(IIO_ANGL, IIO_MOD_Y,  YBIMU_SCAN_EULER_PITCH),
    YBIMU_FLOAT32_CHANNEL(IIO_ANGL, IIO_MOD_Z,  YBIMU_SCAN_EULER_YAW),

    /* --- 5. 气压/温度/高度 --- */
    {
        .type = IIO_DISTANCE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
        .scan_index = YBIMU_SCAN_BARO_HEIGHT,
        .scan_type = { .sign = 's', .realbits = 32, .storagebits = 32, .endianness = IIO_LE },
    },
    {
        .type = IIO_TEMP,
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
        .scan_index = YBIMU_SCAN_BARO_TEMP,
        .scan_type = { .sign = 's', .realbits = 32, .storagebits = 32, .endianness = IIO_LE },
    },
    {
        .type = IIO_PRESSURE,
        .info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
        .scan_index = YBIMU_SCAN_BARO_PRESS,
        .scan_type = { .sign = 's', .realbits = 32, .storagebits = 32, .endianness = IIO_LE },
    },

    /* --- 7. 软时间戳 (必须有，用于缓冲模式) --- */
    IIO_CHAN_SOFT_TIMESTAMP(19), 
};

/* 底层 I2C 寄存器读取封装 */
static int mpu6050_read_regs(struct mpu6050_dev *dev, u8 reg, void *val, int len)
{
    struct i2c_client *client = dev->client;
    struct i2c_msg msg[2];

    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &reg;
    msg[0].len = 1;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = len;

    return i2c_transfer(client->adapter, msg, 2);
}

/* Sysfs 单次读取接口 (cat /sys/bus/iio/devices/iio:deviceX/in_accel_x_raw) */
static int mpu6050_read_raw(struct iio_dev *indio_dev,
                            struct iio_chan_spec const *chan,
                            int *val, int *val2, long mask)
{
    // 这里你需要根据具体的 mask 和 chan->type 实现单次寄存器读取
    // 为了聚焦于缓冲模式，这里暂不展开完整的实现
    return -EINVAL; 
}

/* IIO 框架必要的操作集合 */
static const struct iio_info mpu6050_info = {
    .read_raw = mpu6050_read_raw,
};

/* * ======== 核心：触发器处理函数 (Trigger Handler) ========
 * 当定时器触发时，内核会自动调用此函数。
 * 我们在这里把所有数据读出来，加上时间戳，塞进环形缓冲区。
 */
static irqreturn_t mpu6050_trigger_handler(int irq, void *p)
{
    struct iio_poll_func *pf = p;
    struct iio_dev *indio_dev = pf->indio_dev;
    struct mpu6050_dev *dev = iio_priv(indio_dev);
    struct ybimu_scan scan;
    int ret;

    mutex_lock(&dev->lock);
    ret = mpu6050_read_regs(dev, YBIMU_REG_RAW_ACCEL, &scan, sizeof(scan));
    mutex_unlock(&dev->lock);

    if (ret != 2) {
        iio_trigger_notify_done(indio_dev->trig);
        return IRQ_HANDLED;
    }

    iio_push_to_buffers_with_timestamp(indio_dev, &scan, pf->timestamp);
    iio_trigger_notify_done(indio_dev->trig);
    return IRQ_HANDLED;
}

static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct mpu6050_dev *dev;
    struct iio_dev *indio_dev;

    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*dev));
    if (!indio_dev)
        return -ENOMEM;

    dev = iio_priv(indio_dev); 
    dev->client = client;
    i2c_set_clientdata(client, indio_dev);
        
    mutex_init(&dev->lock); 

    indio_dev->dev.parent = &client->dev;
    indio_dev->info = &mpu6050_info;
    indio_dev->name = MPU6050_NAME; 
    indio_dev->modes = INDIO_DIRECT_MODE; // 既支持直接读取，也能挂载 Buffer
    indio_dev->channels = mpu6050_channels;
    indio_dev->num_channels = ARRAY_SIZE(mpu6050_channels);

    /* ======== 注册触发缓冲区 ======== */
    ret = devm_iio_triggered_buffer_setup(&client->dev, indio_dev,
                                          iio_pollfunc_store_time,   // 底层默认提供的时间获取函数
                                          mpu6050_trigger_handler,   // 我们的数据读取函数
                                          NULL);
    if (ret < 0) {
        dev_err(&client->dev, "iio triggered buffer setup failed\n");
        return ret;
    }

    ret = iio_device_register(indio_dev);
    if (ret < 0) {
        dev_err(&client->dev, "iio_device_register failed\n");
        return ret;
    }

    return 0;
}

static int mpu6050_remove(struct i2c_client *client)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    iio_device_unregister(indio_dev);
    return 0;
}

static const struct of_device_id mpu6050_of_match_table[] = {
    { .compatible = "topeet,imu"},
    {/* sentinel */},
};
MODULE_DEVICE_TABLE(of, mpu6050_of_match_table);

static const struct i2c_device_id mpu6050_ids[] = {
    { "topeet,imu", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, mpu6050_ids);

static struct i2c_driver mpu6050_driver = {    
    .driver = {
        .name = "topeet_imu", // 建议不要带逗号
        .of_match_table = mpu6050_of_match_table,
    },
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .id_table = mpu6050_ids,
};

module_i2c_driver(mpu6050_driver); // 使用宏简化 init 和 exit

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("MPU6050 IIO Driver with Buffered Timer Trigger");