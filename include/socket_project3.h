#ifndef SOCKET_PROJECT3
#define SOCKET_PROJECT3

int socket_init(void);
void socket_close(void);
int socket_poll_read(char *buf, int len);

#endif
