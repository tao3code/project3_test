#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>

#define TTYDEV	"/dev/ttyUSB0"
#define RES_TIMEOUT_SEC	10
#define RES_TIMEOUT_MS	0
#define BUF_TIMEOUT_SEC 0	
#define BUF_TIMEOUT_MS	5000

static int serial_fd = -1;
static struct termios options;

char testbuf[256];

int main(int argc, const char *argv[])
{
	int err;
	char ch;
	int ret;
	int readed;
	fd_set fs_read;
	struct timeval timeout;

	serial_fd = open(TTYDEV, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial_fd < 0) {
		perror("open:");
		goto open_serial;
	}

	tcgetattr(serial_fd, &options);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CSTOPB;
	options.c_iflag |= IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	cfsetspeed(&options, B115200);
	tcsetattr(serial_fd, TCSANOW, &options);
	tcflush(serial_fd, TCIOFLUSH);

	FD_ZERO(&fs_read);
	FD_SET(serial_fd, &fs_read);

	do {
		FD_ZERO(&fs_read);
		FD_SET(serial_fd, &fs_read);
		timeout.tv_sec = RES_TIMEOUT_SEC;
		timeout.tv_usec = RES_TIMEOUT_MS;
		ret = select(serial_fd + 1, &fs_read, NULL, NULL, &timeout);
		if (ret < 0) {
			printf("select err:%d\n", ret);
			break;
		}
		if (ret == 0) {
			printf("select timeout\n");
			continue;
		}

		readed = 0;
		memset(testbuf, 0, 256);

		do {			
			readed += read(serial_fd, testbuf+readed, 256);
			FD_ZERO(&fs_read);
			FD_SET(serial_fd, &fs_read);
			timeout.tv_sec = BUF_TIMEOUT_SEC;
			timeout.tv_usec = BUF_TIMEOUT_MS;
			ret = select
				(serial_fd + 1, &fs_read, NULL, NULL, &timeout);
		} while (ret > 0);

		printf("read:%d:%s\n", readed, testbuf);

	} while (1);

	close(serial_fd);
 open_serial:
	return 0;
}
