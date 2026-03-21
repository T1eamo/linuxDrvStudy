#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>

/* IIO 设备的标准节点 (如果有多个 IIO 设备，数字可能会变) */
#define DEVICE_NODE "/dev/iio:device0"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- 转换系数 ---
const float ACCEL_SCALE = 16.0f / 32767.0f;
const float GYRO_SCALE  = (2000.0f / 32767.0f) * (M_PI / 180.0f);
const float MAG_SCALE   = 800.0f / 32767.0f;
const float RAD_TO_DEG  = 180.0f / M_PI;

/* * --- 核心数据结构 ---
 * 警告：此结构体成员的物理顺序，必须严格按照你驱动头文件中 
 * YBIMU_SCAN_*** 宏的【整数值从小到大】的顺序排列！
 * 这里假设你的 scan_index 是按 Accel->Gyro->Mag->Quat->Euler->Baro 升序定义的。
 */
struct iio_ybimu_data {
    // 1-3. Accel, Gyro, Mag (可以直接用 int16_t，不需要再做底层字节移位了)
    int16_t accel[3];
    int16_t gyro[3];
    int16_t mag[3];
    
    // 4. 四元数
    float quat[4];
    
    // 5. 欧拉角
    float euler[3];
    
    // 6. 气压计/温度/高度 (注意：去掉了你旧代码里的 contrast，现在是 3 个 float)
    float baro[3];
    
    // 7. 内存对齐填充区 (58 字节 -> 64 字节)
    uint8_t padding[6];
    
    // 8. IIO 框架强行注入的 64 位时间戳
    uint64_t timestamp; 
} __attribute__((packed)); // 加上 packed 避免编译器乱加额外对齐，虽然这里我们已经手动对齐了

int main(int argc, char **argv)
{
    int fd;
    struct iio_ybimu_data data;
    ssize_t ret;

    fd = open(DEVICE_NODE, O_RDONLY);
    if (fd < 0) {
        perror("无法打开 IIO 设备，请确认节点名称且 Buffer 已使能");
        return -1;
    }

    printf("YbImu IIO Reader Started (Press Ctrl+C to exit)\n");
    printf("Expected packet size: %lu bytes\n", sizeof(struct iio_ybimu_data));

    while (1) {
        // IIO buffer 读取默认是阻塞的，有数据才会返回，省去了你以前 usleep 轮询的麻烦
        ret = read(fd, &data, sizeof(struct iio_ybimu_data));
        
        if (ret != sizeof(struct iio_ybimu_data)) {
            printf("读取数据不完整或读取错误，ret=%ld\n", ret);
            continue;
        }

        // --- 数据处理 (直接乘系数，告别 buffer_to_int16/float) ---
        float ax = data.accel[0] * ACCEL_SCALE;
        float ay = data.accel[1] * ACCEL_SCALE;
        float az = data.accel[2] * ACCEL_SCALE;

        float gx = data.gyro[0] * GYRO_SCALE;
        float gy = data.gyro[1] * GYRO_SCALE;
        float gz = data.gyro[2] * GYRO_SCALE;

        float mx = data.mag[0] * MAG_SCALE;
        float my = data.mag[1] * MAG_SCALE;
        float mz = data.mag[2] * MAG_SCALE;

        // IIO 里存的是 processed (直接是浮点)，直接打印
        float roll  = data.euler[0] * RAD_TO_DEG;
        float pitch = data.euler[1] * RAD_TO_DEG;
        float yaw   = data.euler[2] * RAD_TO_DEG;

        // 清屏并打印结果
        printf("\033[H\033[J"); 
        printf("--- IIO Data Frame [TS: %llu] ---\n", (unsigned long long)data.timestamp);
        printf("raw_accel: [%.4f, %.4f, %.4f] g\n", ax, ay, az);
        printf("raw_gyro:  [%.4f, %.4f, %.4f] rad/s\n", gx, gy, gz);
        printf("raw_mag:   [%.4f, %.4f, %.4f] uT\n", mx, my, mz);
        printf("rpy:       [%.4f, %.4f, %.4f] deg\n", roll, pitch, yaw);
        printf("quat:      [%.4f, %.4f, %.4f, %.4f]\n", 
               data.quat[0], data.quat[1], data.quat[2], data.quat[3]);
        printf("baro:      [H:%.2f, T:%.2f, P:%.5f]\n", 
               data.baro[0], data.baro[1], data.baro[2]);
        printf("--------------------------------------------------\n");

        // 因为 IIO read 根据 Trigger 的频率自动阻塞，所以这里不需要强行 usleep 了
        // 如果想降低打印频率，可以用计数器跳过几帧再打印
    }

    close(fd);
    return 0;
}