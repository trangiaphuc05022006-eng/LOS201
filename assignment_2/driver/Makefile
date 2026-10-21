KDIR := $(HOME)/embedded_lab1/kernel/linux-5.15
obj-m += sms_sensor_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- clean
