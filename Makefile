obj-m += keypad.o
 
KDIR = /lib/modules/$(shell uname -r)/build
MY_KERNEL_DIR = /home/dat/Downloads/Pi_compile/linux
 
all:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(shell pwd) -C $(MY_KERNEL_DIR) modules
 
clean:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(shell pwd) -C $(MY_KERNEL_DIR) clean