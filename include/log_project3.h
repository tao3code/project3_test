#ifndef LOG_PROJECT3_H
#define LOG_PROJECT3_H

#define LOGFILE  "./log_project3.txt"

void log_project3(const char *fmt, ...);

#define log_err(X) \
do {log_project3("err: %s at %d\n", __FUNCTION__, __LINE__);}while(0)

#define log_info( ...) \
do {log_project3( __VA_ARGS__);}while(0)

int log_system_err(const char *s);

int log_open(void);
void log_close(void);

#endif
