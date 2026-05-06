export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
KERN_DIR=/home/yu/Documents/linux_sdk/kernel
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules
	$(CROSS_COMPILE)gcc -o mpu6050_test mpu6050_test.c -lm  #lm是链接到数学库
clean:
	make -C $(KERN_DIR) M=$(shell pwd) modules clean
	rm -rf modules.order
obj-m += mpu6050_drv.o
