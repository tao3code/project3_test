#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <log_project3.h>

static int log_fd;

static pthread_mutex_t mtx_log;
static pthread_mutexattr_t mat_log;

static char logbuf[256];

void log_project3(const char *fmt, ...)
{
	int len;

	va_list args;
	va_start(args, fmt);

	pthread_mutex_lock(&mtx_log);
	len = vsprintf(logbuf, fmt, args);
	write(log_fd, logbuf, len);	
	pthread_mutex_unlock(&mtx_log);	
	va_end(args);
}

int log_open(void)
{
	int err;

	log_fd = open(LOGFILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (log_fd < 0) {
		perror(LOGFILE);
		return -1;
	}

	err = pthread_mutexattr_init(&mat_log);
        if (err) {
		write(log_fd, "mat_log err\n", 13);	
                goto init_mattr;
        }
	
	err = pthread_mutex_init(&mtx_log, &mat_log);
        if (err) {
		write(log_fd, "mtx_log err\n", 13);	
                goto init_mutex;
        }

	return 0;

	pthread_mutex_destroy(&mtx_log);
 init_mutex:
	pthread_mutexattr_destroy(&mat_log);	
 init_mattr:
	close(log_fd);	
	return err;
}

void log_close(void)
{
	pthread_mutex_destroy(&mtx_log);
        pthread_mutexattr_destroy(&mat_log);
        close(log_fd); 
}
