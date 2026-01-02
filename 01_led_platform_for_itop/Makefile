export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
	KERN_DIR=/home/yu/Documents/linux_sdk/kernel
all:
	make -C $(KERN_DIR) M=$(shell pwd) modules
	$(CROSS_COMPILE)gcc -o hello_drv_test hello_drv_test.c
clean:
	make -C $(KERN_DIR) M=$(shell pwd) modules clean
	rm -rf modules.order
obj-m += hello_drv.o
