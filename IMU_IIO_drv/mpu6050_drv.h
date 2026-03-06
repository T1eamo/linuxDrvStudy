#ifndef MPU6050_DRV_H
#define MPU6050_DRV_H

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/buffer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/iio/trigger.h>
#include <asm/unaligned.h>
#define CDEV_NAME "mpu6050"

/* 寄存器定义 */
#define YBIMU_REG_VERSION       0x01
#define YBIMU_REG_RAW_ACCEL     0x04
#define YBIMU_REG_RAW_GYRO      0x0A
#define YBIMU_REG_RAW_MAG       0x10
#define YBIMU_REG_QUAT          0x16
#define YBIMU_REG_EULER         0x26
#define YBIMU_REG_BARO          0x32
#define YBIMU_REG_ALGO_TYPE     0x61
#define YBIMU_REG_CALIB_IMU     0x70
#define YBIMU_REG_CALIB_MAG     0x71
#define YBIMU_REG_CALIB_TEMP    0x73
#define YBIMU_REG_RESET_FLASH   0xA0

/* 常用常量 */
#define RETRY_TIMEOUT_MS        7000
#define DEFAULT_DELAY_MS        1



enum ybimu_scan_index {
    /* 加速度计 16-bit */
    YBIMU_SCAN_ACCEL_X,
    YBIMU_SCAN_ACCEL_Y,
    YBIMU_SCAN_ACCEL_Z,
    /* 陀螺仪 16-bit */
    YBIMU_SCAN_GYRO_X,
    YBIMU_SCAN_GYRO_Y,
    YBIMU_SCAN_GYRO_Z,
    /* 磁力计 16-bit */
    YBIMU_SCAN_MAG_X,
    YBIMU_SCAN_MAG_Y,
    YBIMU_SCAN_MAG_Z,
    
    /* 四元数 32-bit float */
    YBIMU_SCAN_QUAT_0,
    YBIMU_SCAN_QUAT_1,
    YBIMU_SCAN_QUAT_2,
    YBIMU_SCAN_QUAT_3,
    
    /* 欧拉角 32-bit float */
    YBIMU_SCAN_EULER_ROLL,
    YBIMU_SCAN_EULER_PITCH,
    YBIMU_SCAN_EULER_YAW,
    
    /* 气压计相关 32-bit float */
    YBIMU_SCAN_BARO_HEIGHT,
    YBIMU_SCAN_BARO_TEMP,
    YBIMU_SCAN_BARO_PRESS,
};
struct ybimu_scan {
    __le16 accel_x;
    __le16 accel_y;
    __le16 accel_z;
    __le16 gyro_x;
    __le16 gyro_y;
    __le16 gyro_z;
    __le16 mag_x;
    __le16 mag_y;
    __le16 mag_z;
    __le32 quat_w;
    __le32 quat_x;
    __le32 quat_y;
    __le32 quat_z;
    __le32 euler_roll;
    __le32 euler_pitch;
    __le32 euler_yaw;
    __le32 baro_height;
    __le32 baro_temp;
    __le32 baro_press;
} __packed;

#endif