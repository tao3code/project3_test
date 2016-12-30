#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <log_project3.h>

static struct sockaddr_in addr;
static struct sockaddr_in client_addr;

static int server_fd, ncon;
static int client_fd;

#define PORT    5039
#define LISTEN_BACKLOG 4

int client_len = sizeof(client_addr);

int socket_init(void)
{
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = PORT;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		log_system_err(__FUNCTION__);
		return -1;
	}

	ncon = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
		   (const char *)&ncon, sizeof(ncon));

	ret = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		log_system_err(__FUNCTION__);
		close(server_fd);
		return -1;
	}
	ret = listen(server_fd, LISTEN_BACKLOG);
	if (ret < 0) {
		log_system_err(__FUNCTION__);
		close(server_fd);
		return -1;
	}

	return 0;
}

void socket_close(void)
{
	close(server_fd);
}

int socket_poll_read(char *buf, int len)
{
	int ret;
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
			   (socklen_t *) & client_len);
	ret = read(client_fd, buf, len);
	printf("get[%d]:%s\n", ret, buf);
	close(client_fd);
	return ret;
}
