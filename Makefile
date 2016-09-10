src=$(wildcard *.c)
objs:=$(patsubst %.c,%.o,$(src))

CC := gcc
LDFLAGS := -lpthread
CFLAGS := -Iinclude -Wall

project3.elf: $(objs) 
	$(CC) $(objs) $(LDFLAGS) -o project3.elf

%.d: %.c
	$(CC) -M $(CFLAGS) $< >$@ 
clean:
	rm -f *.elf *.o *.d

-include $(src:.c=.d)
