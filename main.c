#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <import/firmware.h>
#include <serial.h>
#include <robot.h>
#include <log_project3.h>
#include <curses.h>
#include <input_cmd.h>
#include <init.h>

int main(int argc, const char *argv[])
{
	int err;
	
	err = log_open();
	if (err)
		return -1;	
	log_info("Build virsion: %s,%s\n", __DATE__, __TIME__);

	err = open_scr();
	if (err) {
		log_err();
		log_close();
		return -1;
	}

	err = input_cmd_init();
	if (err) {
		log_err();
		close_scr();
		log_close();
		return -1;
	}

	do_init_funcs();
	cmd_loop();
	do_exit_funcs();

	input_cmd_exit();
	close_scr();
	log_close();

	return 0;
}
