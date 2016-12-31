#ifndef SOCKET_PROJECT3
#define SOCKET_PROJECT3

int socket_init(void);
void socket_close(void);
int socket_wait_client(void);
int socket_read_client(char *buf, int len);
int socket_write_buf(char *buf, int len);
void socket_end_client(void);

#define SOCKET_PORT	5039
#define CLIENT_ASK	"this is roject3 client, who are you?"
#define SERVER_ACK	"I am server"
#endif
