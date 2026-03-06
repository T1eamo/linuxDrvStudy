#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>


#define DEVICE_NODE "/dev/mpu6050"  // 请确认您的设备节点名称是否为此

// 定义数学常量
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- 转换系数 ---
// Accel: 16g / 32767
const float ACCEL_SCALE = 16.0f / 32767.0f;
// Gyro: 2000dps / 32767 * (PI / 180) -> 转为 rad/s
const float GYRO_SCALE  = (2000.0f / 32767.0f) * (M_PI / 180.0f);
// Mag: 800uT / 32767
const float MAG_SCALE   = 800.0f / 32767.0f;
// 弧度转角度
const float RAD_TO_DEG  = 180.0f / M_PI;

// --- 数据结构定义 ---
// 必须与内核驱动中的结构体完全一致
struct ybimu_data {
    uint8_t accel[6];      // 3 * int16
    uint8_t gyro[6];       // 3 * int16
    uint8_t mag[6];        // 3 * int16
    uint8_t quat_raw[16];  // 4 * float
    uint8_t euler_raw[12]; // 3 * float
    uint8_t baro_raw[16];  // 4 * float
};


// --- 辅助函数：将2字节转为 int16 (小端模式 LSB First) ---
int16_t buffer_to_int16(uint8_t *buf) {
    // 默认小端: buf[0] 是低8位, buf[1] 是高8位
    return (int16_t)((buf[1] << 8) | buf[0]);
}

// --- 辅助函数：将4字节转为 float ---
float buffer_to_float(uint8_t *buf) {
    float val;
    // 使用 memcpy 避免字节对齐和类型转换带来的 strict-aliasing 问题
    memcpy(&val, buf, 4);
    return val;
}

int main(int argc, char **argv)
{
    int fd;
    struct ybimu_data raw_data;
    int ret;

    // 1. 打开设备文件
    fd = open(DEVICE_NODE, O_RDONLY);
    if (fd < 0) {
        perror("无法打开设备文件");
        return -1;
    }

    printf("YbImu Reader Started (Press Ctrl+C to exit)\n");

    while (1) {
        // 2. 读取原始数据 (62字节)
        ret = read(fd, &raw_data, sizeof(struct ybimu_data));
        if (ret != sizeof(struct ybimu_data)) {
            printf("读取数据不完整，ret=%d\n", ret);
            usleep(100000); // 100ms
            continue;
        }

        // --- 3. 数据处理 ---

        // A. 加速度计 (Accel) -> g
        float ax = buffer_to_int16(&raw_data.accel[0]) * ACCEL_SCALE;
        float ay = buffer_to_int16(&raw_data.accel[2]) * ACCEL_SCALE;
        float az = buffer_to_int16(&raw_data.accel[4]) * ACCEL_SCALE;

        // B. 陀螺仪 (Gyro) -> rad/s
        float gx = buffer_to_int16(&raw_data.gyro[0]) * GYRO_SCALE;
        float gy = buffer_to_int16(&raw_data.gyro[2]) * GYRO_SCALE;
        float gz = buffer_to_int16(&raw_data.gyro[4]) * GYRO_SCALE;

        // C. 磁力计 (Mag) -> uT
        float mx = buffer_to_int16(&raw_data.mag[0]) * MAG_SCALE;
        float my = buffer_to_int16(&raw_data.mag[2]) * MAG_SCALE;
        float mz = buffer_to_int16(&raw_data.mag[4]) * MAG_SCALE;

        // D. 四元数 (Quaternion)
        float q0 = buffer_to_float(&raw_data.quat_raw[0]);
        float q1 = buffer_to_float(&raw_data.quat_raw[4]);
        float q2 = buffer_to_float(&raw_data.quat_raw[8]);
        float q3 = buffer_to_float(&raw_data.quat_raw[12]);

        // E. 欧拉角 (Euler) -> 默认转为角度 (Degrees)
        // Python: get_imu_attitude_data(ToAngle=True)
        float roll  = buffer_to_float(&raw_data.euler_raw[0]) * RAD_TO_DEG;
        float pitch = buffer_to_float(&raw_data.euler_raw[4]) * RAD_TO_DEG;
        float yaw   = buffer_to_float(&raw_data.euler_raw[8]) * RAD_TO_DEG;

        // F. 气压计 (Barometer)
        float height = buffer_to_float(&raw_data.baro_raw[0]);
        float temp   = buffer_to_float(&raw_data.baro_raw[4]);
        float press  = buffer_to_float(&raw_data.baro_raw[8]);
        float contrast = buffer_to_float(&raw_data.baro_raw[12]);

        // --- 4. 打印结果  ---
        // 清屏 (可选)
        // printf("\033[H\033[J"); 

        printf("raw_accel: [%.4f, %.4f, %.4f] g\n", ax, ay, az);
        printf("raw_gyro:  [%.4f, %.4f, %.4f] rad/s\n", gx, gy, gz);
        printf("raw_mag:   [%.4f, %.4f, %.4f] uT\n", mx, my, mz);
        printf("rpy:       [%.4f, %.4f, %.4f] deg\n", roll, pitch, yaw);
        printf("quat:      [%.4f, %.4f, %.4f, %.4f]\n", q0, q1, q2, q3);
        printf("baro:      [H:%.2f, T:%.2f, P:%.5f, C:%.5f]\n", height, temp, press, contrast);
        printf("--------------------------------------------------\n");

        // 延时 0.1s
        usleep(100000);
    }

    close(fd);
    return 0;
}