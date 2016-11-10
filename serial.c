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

static char sbuf[256];
 
int sent_cmd_alloc_response(const char *in, char **out)
{
	int ret;
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
		log_system_err("flush serial");
		goto flush_serial;
	}

	ret = write(serial_fd, in, strlen(in));
	if (ret < 0) {
		log_system_err("write serial");
		goto write_serial;	
	}
	tcdrain(serial_fd);

	FD_ZERO(&fs_read);
	FD_SET(serial_fd, &fs_read);
	timeout.tv_sec = RES_TIMEOUT_SEC;
	timeout.tv_usec = RES_TIMEOUT_MS;

	ret = select(serial_fd + 1, &fs_read, NULL, NULL, &timeout);

	if (ret <= 0) {
		log_info("Error response(%d):%s\n", ret, in);
		goto wait_response;
	}

	memset(sbuf, 0, sizeof(sbuf));

	ret = read(serial_fd, sbuf, sizeof(sbuf));

	if (ret <= 0) {
		log_system_err("read serial");
		goto read_serial;
	}

	*out = malloc(ret);
	if (!*out) {
		ret = log_system_err(__FUNCTION__);
		goto alloc_buf;
	}

	memset(*out, 0, ret);

	if (sbuf[0] == '\r')
		memcpy(*out, &sbuf[1], ret -1);
	else
		memcpy(*out, sbuf, ret);

 alloc_buf:
 read_serial:
 wait_response:
 write_serial:
 flush_serial: 
	pthread_mutex_unlock(&mtx_s);
	return  ret;
}

int send_cmd(const char *cmd)
{
	int ret;
	char *msg_ingro;

	ret = sent_cmd_alloc_response(cmd, &msg_ingro);	

	if (ret < 0) 
		return ret;

	free(msg_ingro);
	return 0;
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
		err = log_system_err(TTYDEV);
		goto open_serial;
	}
	log_info("%s is open\n", TTYDEV);	
	tcgetattr(serial_fd, &options);
		
	options.c_cflag = 0;
	options.c_iflag = 0;
	options.c_oflag = ONOCR | ONLRET;
	options.c_lflag = ICANON;
	cfsetspeed(&options, B115200);
	tcsetattr(serial_fd, TCSANOW, &options);
	tcflush(serial_fd, TCIOFLUSH);

	err = pthread_mutexattr_init(&mat_s);
	if (err) {
		log_system_err(__FUNCTION__);
		goto init_mattr;
	}

	err = pthread_mutex_init(&mtx_s, &mat_s);
	if (err) {
		log_system_err(__FUNCTION__);
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
