#include <sys/socket.h>
//#include <linux/in.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

struct sockaddr_in addr;

int s;

#define PORT    5039

int main(int argc, char *argv[])
{
	int i;
	int ret;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = PORT;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;
	ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));

	for (i = 1; i < argc; i++)
		write(s, argv[i], strlen(argv[i]));

	close(s);
	return ret;
}
