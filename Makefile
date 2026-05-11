obj-m += spi-shift-register.o

KDIR ?= ../linux
PWD := $(shell pwd)

BUILD_DIR := $(PWD)/build

ARCH ?= arm64
CROSS_COMPILE ?= aarch64-rpi3-linux-musl-

all: modules

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) MO=$(BUILD_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) MO=$(BUILD_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean

