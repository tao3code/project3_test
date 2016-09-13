src := $(wildcard *.c)
objs := $(patsubst %.c,%.o,$(src))
code := $(shell find -name "*.h") $(src)
CC := gcc
LDFLAGS := -lpthread -lncurses
CFLAGS := -Iinclude -Wall -g

project3.elf: $(objs) 
	@$(CC) $(objs) $(LDFLAGS) -o project3.elf
	@echo $@

%.d: %.c
	@$(CC) -M $(CFLAGS) $< >$@
%.o: %.c
	@gcc $(CFLAGS) -c $<
	@echo $@
clean:
	@rm -f *.elf *.o *.d *.txt
	@wc -l  $(code)

-include $(src:.c=.d)
