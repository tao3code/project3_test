#ifndef SERIAL_H
#define SERIAL_H

void send_cmd(const char *cmd);
int sent_cmd_read_response(const char *cmd, char *buf, int len);
int serial_init(void);
void serial_close(void);

#endif
