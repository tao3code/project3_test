#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>

#define TTYDEV	"/dev/ttyUSB0"
#define RES_TIMEOUT_SEC	1
#define RES_TIMEOUT_MS 0
#define BUF_TIMEOUT_SEC 0
#define BUF_TIMEOUT_MS  5000

static int serial_fd = -1;
static struct termios options;
static pthread_mutex_t mutex;
static pthread_mutexattr_t mattr;

void send_cmd(const char *cmd)
{
	pthread_mutex_lock(&mutex);
	write(serial_fd, cmd, strlen(cmd));
	pthread_mutex_unlock(&mutex);
}

static char sbuf[256];
 
int sent_cmd_read_response(const char *in, char **out)
{
	int ret;
	int readed;
	fd_set fs_read;
	struct timeval timeout;

	pthread_mutex_lock(&mutex);
	tcflush(serial_fd, TCIOFLUSH);
	write(serial_fd, in, strlen(in));
	tcdrain(serial_fd);

	FD_ZERO(&fs_read);
	FD_SET(serial_fd, &fs_read);
	timeout.tv_sec = RES_TIMEOUT_SEC;
	timeout.tv_usec = RES_TIMEOUT_MS;
	readed = 0;

	ret = select(serial_fd + 1, &fs_read, NULL, NULL, &timeout);
	if (ret < 0) {
		printf("select err:%d\n", ret);
		pthread_mutex_unlock(&mutex);
		return ret;
	}

	if (ret == 0) {
		printf("select timeout\n");
		pthread_mutex_unlock(&mutex);
		return ret;
	}

	readed = 0;
	memset(sbuf, 0, sizeof(sbuf));

	do {
		readed += read(serial_fd, &sbuf[readed], sizeof(sbuf));
		FD_ZERO(&fs_read);
		FD_SET(serial_fd, &fs_read);
		timeout.tv_sec = BUF_TIMEOUT_SEC;
		timeout.tv_usec = BUF_TIMEOUT_MS;
		ret = select(serial_fd + 1, &fs_read, NULL, NULL, &timeout);
	}
	while (ret > 0);

	pthread_mutex_unlock(&mutex);

	*out = malloc(readed);
	if (!*out) {
		printf("no memory\n");
		return -1;
	}

	memcpy(*out, sbuf, readed); 

	return readed;
}

int serial_init(void)
{
	int err;

	serial_fd = open(TTYDEV, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial_fd < 0) {
		err = -1;
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

	err = pthread_mutexattr_init(&mattr);
	if (err) {
		perror("init mutex attribute:");
		goto init_mattr;
	}

	err = pthread_mutex_init(&mutex, &mattr);
	if (err) {
		perror("init mutex:");
		goto init_mutex;
	}

	return 0;

	pthread_mutex_destroy(&mutex);
 init_mutex:
	pthread_mutexattr_destroy(&mattr);
 init_mattr:
	close(serial_fd);
 open_serial:
	return err;
}

void serial_close(void)
{
	pthread_mutex_destroy(&mutex);
	pthread_mutexattr_destroy(&mattr);
	close(serial_fd);
}
