#src := $(wildcard *.c)
src := $(shell find -name "*.c")
objs := $(patsubst %.c,%.o,$(src))
dps := $(patsubst %.c,%.d,$(src))
code := $(shell find -name "*.h") $(src)
CC := gcc
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
	@rm -f *.elf $(objs) $(dps) build-all.o log_project3.txt
	@wc -l  $(code)

-include $(src:.c=.d)
