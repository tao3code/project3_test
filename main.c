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

	cmd_loop();
	
	close_scr();
	log_close();

	return 0;
}
