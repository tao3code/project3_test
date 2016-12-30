#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>

#define MAX_FILE_SIE	(1024*128)
static int do_run(int argc, char *argv[])
{
	int fd;
	char *buf;
	char *cmd;
	int p = 0;
	int ret = 0;

	if (argc != 2) {
		log_err();
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		ret = log_system_err(argv[1]);
		goto open_file;
	}

	buf = malloc(MAX_FILE_SIE);
	if (!buf) {
		ret = log_system_err(__FUNCTION__);
		goto alloc_mem;
	}

	memset(buf, 0, MAX_FILE_SIE);

	ret = read(fd, buf, MAX_FILE_SIE);
	if (ret < 0) {
		log_system_err(__FUNCTION__);
		goto read_file;
	}

	cmd = buf;
	while (buf[p]) {
		if (buf[p] != '\n') {
			p++;
			continue;
		}

		buf[p] = 0;

		ret = run_cmd(cmd);
		if (ret)
			break;
		p++;
		cmd = buf + p;
	}

 read_file:
	free(buf);
 alloc_mem:
	close(fd);
 open_file:
	return ret;
}

static struct input_cmd cmd = {
	.str = "run",
	.func = do_run,
	.info = "run script."
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
