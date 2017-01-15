#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <socket_project3.h>
#include <input_cmd.h>

static struct sockaddr_in addr;
static int s;

static char buf[CMDBUF_LEN];

int main(int argc, char *argv[])
{
	int i;
	int ret;
	int len = 0;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = SOCKET_PORT;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		perror("socket:");
		ret = -1;
		goto socket_err;
	}

	ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
	if (ret) {
		perror("connect:");
		ret = -1;
		goto connect_err;
	}

	ret = write(s, CLIENT_ASK, sizeof(CLIENT_ASK));
	if (ret <= 0) {
		perror("ask:");
		ret = -1;
		goto ask_err;
	}

	ret = read(s, buf, sizeof(buf));
	if (ret <= 0) {
		perror("ack:");
		ret = -1;
		goto ask_err;
	}

	if (strcmp(buf, SERVER_ACK)) {
		printf("Is not server:%s\n", buf);
		ret = -1;
		goto ask_err;
	}

	memset(buf, 0, sizeof(buf));

	for (i = 1; i < argc; i++) {
		if (strlen(argv[i]) > sizeof(buf) - len) {
			printf("Exceed, skip: %s\n", argv[i]);
			break;
		}
		len += sprintf(&buf[len], "%s ", argv[i]);
	}

	if (len <= 2) {
		ret = 0;
		goto no_cmd;
	}

	buf[len - 1] = 0;

	ret = write(s, buf, len);
	if (ret <= 0) {
		perror("write cmd:");
		ret = -1;
		goto ask_err;
	}

	memset(buf, 0, sizeof(buf));
	ret = read(s, buf, sizeof(buf));
	if (ret > 0) {
		printf("%s", buf);
		ret = -1;
	} else
		ret = 0;

 no_cmd:
 ask_err:
 connect_err:
	close(s);
 socket_err:
	return ret;
}
