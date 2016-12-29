deamon_src := $(shell find ./ -path "./client" -prune -o -print | grep "\.c")
client_src := $(shell find ./client/ -name "*.c")
src := $(deamon_src) $(client_src)

deamon_objs := $(patsubst %.c,%.o,$(deamon_src))
client_objs := $(patsubst %.c,%.o,$(client_src))
objs := $(patsubst %.c,%.o,$(src))

header := $(shell find -name "*.h")
CC := gcc
LD := ld
LDFLAGS := -lpthread -lncurses
CFLAGS := -Iinclude -Wall -Werror -g

all: pj3d pj3c

pj3d: $(deamon_objs) link.lsd 
	@$(LD) $(deamon_objs) -r -Tlink.lsd -o build-all.o  
	@$(CC) build-all.o $(LDFLAGS) -o $@ 
	@echo $@

pj3c: $(client_objs)
	@$(CC) $(client_objs) -o $@ 
	@echo $@

%.d: %.c
	@$(CC) -M $(CFLAGS) $< >$@
%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo $@
clean:
	@rm -f pj3d pj3c $(objs) $(src:.c=.d)
	@rm -f $(src:.c=.c~) $(header:.h=.h~)
	@rm -f build-all.o log_project3.txt
report:
	@wc -l $(src) $(header) Makefile
test:
	echo $(client_objs)

-include $(src:.c=.d)
