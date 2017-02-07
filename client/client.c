#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <socket_project3.h>

static struct sockaddr_in addr;
static int s;

static struct pj3_socket_msg smsg;

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

	ret = read(s, smsg.buf, sizeof(smsg.buf));
	if (ret <= 0) {
		perror("ack:");
		ret = -1;
		goto ask_err;
	}

	if (strcmp(smsg.buf, SERVER_ACK)) {
		printf("Is not server:%s\n", smsg.buf);
		ret = -1;
		goto ask_err;
	}

	memset(&smsg, 0, sizeof(smsg));

	for (i = 1; i < argc; i++) {
		if (strlen(argv[i]) > sizeof(smsg.buf) - len) {
			printf("Exceed, skip: %s\n", argv[i]);
			break;
		}
		len += sprintf(&smsg.buf[len], "%s ", argv[i]);
	}

	if (len <= 2) {
		ret = 0;
		goto no_cmd;
	}

	smsg.buf[len - 1] = 0;

	ret = write(s, smsg.buf, sizeof(smsg.buf));
	if (ret <= 0) {
		perror("write cmd:");
		ret = -1;
		goto ask_err;
	}

	memset(&smsg, 0, sizeof(smsg));
	ret = read(s, &smsg, sizeof(smsg));
	if (ret > 0) {
		printf("%s", smsg.buf);
		ret = smsg.ser_ret;
	} else
		ret = 0;

 no_cmd:
 ask_err:
 connect_err:
	close(s);
 socket_err:
	return ret;
}
