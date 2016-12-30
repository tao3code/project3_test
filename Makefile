ROOT_DIR := $(shell pwd)

CFLAGS := -g -Wall -Werror -I$(ROOT_DIR)/include/

export ROOT_DIR 
export CFLAGS
export CC
export LD

all: project3_daemon project3_client

project3_daemon:
	@make -C daemon
	@$(LD) -r daemon/build_in.o -T$(ROOT_DIR)/script/link.lsd -o pj3d.o
	@$(CC) pj3d.o -lpthread -lncurses -o pj3d
	@echo generate pj3d
project3_client:
	@make -C client
	@$(CC) client/build_in.o -o pj3cmd
	@echo generate pj3cmd	
clean:
	@make -C daemon/ clean
	@make -C client/ clean
	@rm -f pj3d.o pj3d pj3cmd log_project3.txt include/*.h~
