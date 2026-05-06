#ifndef MPU6050_DRV_H
#define MPU6050_DRV_H

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


#endif