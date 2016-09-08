#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "./import/firmware.h"
#include "serial.h"
#include "robot.h"

char testbuf[256];

int main(int argc, const char *argv[])
{
	int err;
	char ch;
	int len;

	err = serial_init();
	if (err) 
		goto open_serial;
	do {
		ch = getchar();
		switch (ch) {
		case 'v':
			len = sent_cmd_read_response(">0 voltage;", testbuf, 256);
			break;
		case 'h':
			len = sent_cmd_read_response(">0 help;", testbuf, 256);
			break;
		case 'g':
			len =
			    sent_cmd_read_response(">0 gyroscope;", testbuf,
						   256);
			break;
		default:
			len =
			    sent_cmd_read_response(">0 report;", testbuf, 256);
		}
		if (len > 0)
			printf("(%d)%s", len, testbuf);
	} while (ch != 'x');

	serial_close();
 open_serial:
	return 0;
}
