#ifndef SERIAL_H
#define SERIAL_H

void send_cmd(const char *in);
int sent_cmd_read_response(const char *in, char **out);
int serial_init(void);
void serial_close(void);

#endif
