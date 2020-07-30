
CROSS_COMPILE = arm-himix200-linux-

LD = $(CROSS_COMPILE)gcc
CC = $(CROSS_COMPILE)gcc
TARGET = ymodem

SRCS = $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

LIBS = -L. 

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $^ $(LIBS) -o $@
$(OBJS): %.o:%.c
	$(CC) -c  $<  $(INC) -o $@

.PHONY:clean
clean:
	-rm $(OBJS) $(TARGET)
