#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <import/firmware.h>
#include <serial.h>
#include <robot.h>
#include <log_project3.h>

char testbuf[256];

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

	test_robot();
	update_voltage();
	update_presure();
	update_gyroscope();
	update_motion_state();
	
	serial_close();
	log_close();
 open_serial:
	return 0;
}
