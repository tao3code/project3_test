#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <serial.h>
#include <log_project3.h>

static int serial_fd = -1;
static struct termios options;
static pthread_mutex_t mtx_s;
static pthread_mutexattr_t mat_s;

int send_cmd(const char *cmd)
{
	int ret;
	
	if (serial_fd < 0) {
		log_info("Serial port not open!\n");
		return -1;
	}

	pthread_mutex_lock(&mtx_s);
	ret = tcflush(serial_fd, TCIOFLUSH);
	if (ret < 0) {
		log_err();
		pthread_mutex_unlock(&mtx_s);
		return -1;
	}
	ret = write(serial_fd, cmd, strlen(cmd));
	pthread_mutex_unlock(&mtx_s);

	return ret;
}

static char sbuf[256];
 
int sent_cmd_alloc_response(const char *in, char **out)
{
	int ret;
	int readed;
	fd_set fs_read;
	struct timeval timeout;

	*out = 0;

	if (serial_fd < 0) {
		log_info("Serial port not open!\n");
		return -1;
	}

	pthread_mutex_lock(&mtx_s);
	
	ret = tcflush(serial_fd, TCIOFLUSH);
	if (ret < 0) {
		log_err();
		pthread_mutex_unlock(&mtx_s);
		return -1;
	}

	ret = write(serial_fd, in, strlen(in));
	if (ret < 0) {
		log_err();	
		pthread_mutex_unlock(&mtx_s);
		return ret;
	}
	tcdrain(serial_fd);

	FD_ZERO(&fs_read);
	FD_SET(serial_fd, &fs_read);
	timeout.tv_sec = RES_TIMEOUT_SEC;
	timeout.tv_usec = RES_TIMEOUT_MS;
	readed = 0;

	ret = select(serial_fd + 1, &fs_read, NULL, NULL, &timeout);

	if (ret < 0) {
		log_err();
		pthread_mutex_unlock(&mtx_s);
		return ret;
	}

	if (ret == 0) {
		log_info("Time out:%s\n", in);
		pthread_mutex_unlock(&mtx_s);
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
		if (ret < 0) {
			log_err();	
			pthread_mutex_unlock(&mtx_s);
			return ret;
		}
	}
	while (ret > 0);

	if (readed == 0) {
		pthread_mutex_unlock(&mtx_s);
		return 0;
	}

	*out = malloc(readed);
	if (!*out) {
		log_err();
		return -1;
	}
	memset(*out, 0, readed);
	memcpy(*out, sbuf, readed); 
	pthread_mutex_unlock(&mtx_s);

	return  readed;
}

int serial_init(void)
{
	int err;

	if (serial_fd >= 0) {
		log_info("Serial port already open, stop!\n");
		return -1;
	} 

	serial_fd = open(TTYDEV, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial_fd < 0) {
		err = -1;
		log_err();
		goto open_serial;
	}
	log_info("%s is open\n", TTYDEV);	
	tcgetattr(serial_fd, &options);
		
	options.c_cflag = 0;
	options.c_iflag = 0;
	options.c_oflag = 0;
	options.c_lflag = 0;
	cfsetspeed(&options, B115200);
	tcsetattr(serial_fd, TCSANOW, &options);
	tcflush(serial_fd, TCIOFLUSH);

	err = pthread_mutexattr_init(&mat_s);
	if (err) {
		log_err();
		goto init_mattr;
	}

	err = pthread_mutex_init(&mtx_s, &mat_s);
	if (err) {
		log_err();
		goto init_mutex;
	}

	return 0;

	pthread_mutex_destroy(&mtx_s);
 init_mutex:
	pthread_mutexattr_destroy(&mat_s);
 init_mattr:
	close(serial_fd);
	serial_fd = -1;
	log_info("%s closed\n", TTYDEV);	
 open_serial:
	return err;
}

void serial_close(void)
{
	pthread_mutex_destroy(&mtx_s);
	pthread_mutexattr_destroy(&mat_s);
	close(serial_fd);
	serial_fd = -1;
	log_info("%s closed\n", TTYDEV);	
}

int is_serial_on(void)
{
	return (serial_fd >= 0)? 1:0;
}
