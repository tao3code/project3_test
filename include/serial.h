#ifndef SERIAL_H
#define SERIAL_H

#define TTYDEV  "/dev/ttyUSB0"
#define RES_TIMEOUT_SEC 1
#define RES_TIMEOUT_MS 0
#define BUF_TIMEOUT_SEC 0
#define BUF_TIMEOUT_MS  5000

int send_cmd(const char *in);
int sent_cmd_alloc_response(const char *in, char **out);
int serial_init(void);
void serial_close(void);
int is_serial_on(void);

#endif
