KERNELDIR=/usr/src/linux-2.4.18-14custom
include $(KERNELDIR)/.config

CFLAGS = -I. -D__KERNEL__ -DMODULE -I$(KERNELDIR)/include -O -Wall

all: hw4.o
	