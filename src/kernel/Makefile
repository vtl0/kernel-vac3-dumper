obj-m := kvacd.o
kvacd-objs = entry.o hooks.o memory.o
CC = gcc
KERNEL_HEADERS := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_HEADERS) KCFLAGS="${KCFLAGS}" M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_HEADERS) KCFLAGS="${KCFLAGS}" M=$(PWD) clean
