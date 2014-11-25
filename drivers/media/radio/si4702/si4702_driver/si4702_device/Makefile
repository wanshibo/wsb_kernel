# If KERNELRELEASE is defined, we've been invoked from the 
# kernel build system and can use its language. 
obj-m := si4702_device.o

ifneq ($(KERNELRELEASE),) 
#KERNELDIR ?= /lib/modules/$(shell uname -r)/build 
KERNELDIR = $(KERNELRELEASE)
else 
KERNELDIR = /home/zhangda/kernel/unsp210_linux-2.6.35.7
endif

$(PWD) := $(shell pwd)

default: 
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
	rm -rf *.o *.mod.c *.order *.symvers
clean:
	rm -rf *.ko



