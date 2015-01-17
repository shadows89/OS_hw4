CC=gcc
TARGET=main
C_SOURCES=$(wildcard *.c)
C_OBJS=$(C_SOURCES:.c=.o)
CFLAGS=-I. -I. -Wall -Werror -g3 -O0
LDFLAGS=-L. -lpthread
MODULE_NAME=hw4
MODULE_MAJOR=$(shell cat /proc/devices | grep $(MODULE_NAME) | sed 's/ .*//')


all: $(TARGET)

dec:
	mknod $@ c $(MODULE_MAJOR) 0

enc:
	mknod $@ c $(MODULE_MAJOR) 1

inv:
	mknod $@ c $(MODULE_MAJOR) 2

check: $(TARGET) dec enc inv
	./$(TARGET)

%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $<

$(TARGET): $(C_OBJS)
	$(CC) $(C_OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf *.o $(TARGET) enc dec inv

.PHONY: all clean check
