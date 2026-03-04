#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h> // 引入错误号头文件

int main(int argc, char **argv)
{
    int fd;
    char buf[1024];
    int len;

    // 1. 打印参数帮助调试
    if (argc < 2) {
        printf("Usage: %s -w <0|1>\n", argv[0]);
        return -1;
    }

    // 2. 尝试打开设备
    // 注意：这里必须和 ls /dev/ 下的名字完全一致
    fd = open("/dev/newchrled", O_RDWR);
    
    // 3. 错误处理核心：使用 perror 打印真实错误
    if (fd == -1) {
        // 这行代码会打印出: "Open device failed: [具体的错误原因]"
        perror("Open device failed"); 
        return -1;
    }

    // 4. 写操作
    if ((0 == strcmp(argv[1], "-w")) && (argc == 3)) {
        // 将字符串转换为整数写入 (根据驱动逻辑调整，这里先按你的逻辑传字符串)
        len = strlen(argv[2]) + 1;
        int ret = write(fd, argv[2], len);
        if (ret < 0) {
            perror("Write failed");
        }
    } else {
        len = read(fd, buf, 1024);
        if (len > 0) {
            buf[len] = '\0';
            printf("APP read :%s\n", buf);
        } else {
            perror("Read failed");
        }
    }

    close(fd);
    return 0;
}