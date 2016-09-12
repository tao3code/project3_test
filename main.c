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

	err = serial_init();
	if (err) 
		goto open_serial;
	log_info("Open %s\n", TTYDEV);

	open_scr();

	cmd_loop();
	
	close_scr();
	serial_close();
	log_close();
 open_serial:
	return 0;
}
