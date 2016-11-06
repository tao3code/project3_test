src := $(shell find -name "*.c")
objs := $(patsubst %.c,%.o,$(src))
header := $(shell find -name "*.h")
CC := gcc
LD := ld
LDFLAGS := -lpthread -lncurses
CFLAGS := -Iinclude -Wall -Werror -g

project3.elf: $(objs) link.lsd 
	@$(LD) $(objs) -r -Tlink.lsd -o build-all.o  
	@$(CC) build-all.o $(LDFLAGS) -o project3.elf
	@echo $@

%.d: %.c
	@$(CC) -M $(CFLAGS) $< >$@
%.o: %.c
	@gcc $(CFLAGS) -c $< -o $@
	@echo $@
clean:
	@rm -f *.elf $(objs) $(src:.c=.d)
	@rm -f $(src:.c=.c~) $(header:.h=.h~)
	@rm -f build-all.o log_project3.txt
report:
	@wc -l $(src) $(header) Makefile

-include $(src:.c=.d)
