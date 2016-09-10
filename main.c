#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <import/firmware.h>
#include <serial.h>
#include <robot.h>

char testbuf[256];

int main(int argc, const char *argv[])
{
	int err;
	char *msg;
	int len;
	char ch;

	err = serial_init();
	if (err) 
		goto open_serial;
	do {
	len = sent_cmd_read_response(">0 report;", &msg);
	printf("read %d\n", len);
	if (msg) {	
		printf("%s", msg);
		free(msg);
	}
	ch = getchar();
	} while(ch != 'x');

	serial_close();
 open_serial:
	return 0;
}
