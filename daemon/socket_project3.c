#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <log_project3.h>
#include <socket_project3.h>

static struct sockaddr_in addr;
static struct sockaddr_in client_addr;

static int server_fd, ncon;
static int client_fd = -1;

char socket_buf[256];

#define LISTEN_BACKLOG 4

int client_len = sizeof(client_addr);

int socket_init(void)
{
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = SOCKET_PORT;
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

int socket_wait_client(void)
{
	int ret;

	client_fd = -1;
	client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
			   (socklen_t *) & client_len);
	if (client_fd < 0) {
		log_system_err("Socket accept");
		ret = -1;
		goto end_wait;
	}

	memset(socket_buf, 0, sizeof(socket_buf));
	ret = read(client_fd, socket_buf, sizeof(socket_buf));
	if (ret <= 0) {
		log_system_err("Socket hand shake read");
		goto end_wait;
	}

	if (strcmp(socket_buf, CLIENT_ASK)) {
		log_err();
		ret = -1;
		goto end_wait;
	}

	ret = write(client_fd, SERVER_ACK, sizeof(SERVER_ACK));
	if (ret < 0) {
		log_system_err("Socket hand shake write");
		goto end_wait;
	}

	memset(socket_buf, 0, sizeof(socket_buf));
	return 0;
 end_wait:
	close(client_fd);
	client_fd = -1;
	memset(socket_buf, 0, sizeof(socket_buf));
	return ret;
}

void socket_end_client(void)
{
	int ret;
	if (strlen(socket_buf)) {
		ret = write(client_fd, socket_buf, sizeof(socket_buf));
		if (ret < 0)
			log_system_err("Socket close msg");
	}

	memset(socket_buf, 0, sizeof(socket_buf));
	close(client_fd);
	client_fd = -1;
}

int socket_read_client(char *buf, int len)
{
	int ret;

	memset(buf, 0, len);

	memset(buf, 0, len);

	ret = read(client_fd, buf, len);
	if (ret <= 0) {
		log_system_err("socket read");
		return -1;
	}

	return ret;
}

int socket_write_buf(char *buf, int len)
{
	int buf_len;

	buf_len = strlen(socket_buf);
	if (!buf)
		return buf_len;

	if (buf_len + len >= sizeof(socket_buf))
		return buf_len;
	memcpy(&socket_buf[buf_len], buf, len);
	return buf_len + len;
}
