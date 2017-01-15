#include <pthread.h>
#include <log_project3.h>
#include <socket_project3.h>
#include <string.h>
#include <stdlib.h>
#include <input_cmd.h>
#include <robot.h>
#include <serial.h>

WINDOW *ctrl_win;
WINDOW *motion_win;

static pthread_mutex_t mtx_scr;
static pthread_mutexattr_t mat_scr;

inline void lock_scr(void)
{
	pthread_mutex_lock(&mtx_scr);
}

inline void unlock_scr(void)
{
	pthread_mutex_unlock(&mtx_scr);
}

int open_scr(void)
{
	int err;
	err = pthread_mutexattr_init(&mat_scr);
	if (err) {
		log_system_err(__FUNCTION__);
		goto init_mattr;
	}

	err = pthread_mutex_init(&mtx_scr, &mat_scr);
	if (err) {
		log_system_err(__FUNCTION__);
		goto init_mutex;
	}

	initscr();
	cbreak();
	noecho();
	ctrl_win = newwin(LINE_CTRL, ROW_CTRL, 0, 0);
	motion_win = newwin(LINE_MOTION, ROW_MOTION, 0, ROW_CTRL + 4);

	return 0;

	pthread_mutex_destroy(&mtx_scr);
 init_mutex:
	pthread_mutexattr_destroy(&mat_scr);
 init_mattr:
	return err;

}

void close_scr(void)
{
	pthread_mutex_destroy(&mtx_scr);
	pthread_mutexattr_destroy(&mat_scr);
	delwin(ctrl_win);
	delwin(motion_win);
	endwin();
}

static unsigned char game_over = 0;

static int do_kill(int argc, char *argv[])
{
	game_over = 1;
	return 0;
}

static int help_kill(char *buf, int argc, char *argv[])
{
	return sprintf(buf, "kill server\n");
}

static char help_msg[256];

static int do_help(int argc, char *argv[]);

static int serial_send_cmd(int argc, char *argv[])
{
	char *res;
	int ret;
	int len = 0;
	int i;
	char buf[256];

	memset(buf, 0, sizeof(buf));

	for (i = 1; i < argc; i++)
		len += sprintf(&buf[len], "%s ", argv[i]);

	ret = sent_cmd_alloc_response(buf, &res);
	if (ret < 0) {
		len = sprintf(help_msg, "operation fail: %s\n", buf);
		socket_write_buf(help_msg, len);
		return 0;
	}

	if (ret == 0)
		return 0;

	socket_write_buf(res, ret);
	free(res);

	return 0;
}

static int help_serial(char *buf, int argc, char *argv[])
{
	return sprintf(buf, "sent string to serial port\n");
}

static struct input_cmd buildin_cmd[] = {
	{
	 .str = "help",
	 .func = do_help,
	 .next = &buildin_cmd[1],
	 },
	{
	 .str = "kill",
	 .func = do_kill,
	 .help = help_kill,
	 .next = &buildin_cmd[2],
	 },
	{
	 .str = "serial",
	 .func = serial_send_cmd,
	 .help = help_serial,
	 .next = 0,
	 },
};

static struct input_cmd *cmd_head = &buildin_cmd[0];

static int list_all_cmds(char *buf)
{
	int len = 0;
	struct input_cmd *cmd_list = cmd_head;

	while (cmd_list) {
		len += sprintf(buf + len, " %s", cmd_list->str);
		cmd_list = cmd_list->next;
	}

	buf[len] = '\n';

	socket_write_buf(buf, len + 1);
	return 0;
}

static int do_help(int argc, char *argv[])
{
	int err;
	int len;
	struct input_cmd *cmd_list = cmd_head;

	if (argc == 1)
		return list_all_cmds(help_msg);
	err = -1;
	while (cmd_list) {
		if (strcmp(argv[1], cmd_list->str)) {
			cmd_list = cmd_list->next;
			continue;
		}
		if (cmd_list->help) {
			len = cmd_list->help(help_msg, argc - 1, &argv[1]);
			socket_write_buf(help_msg, len);
		}
		return 0;
	}
	return err;
}

#define MAX_ARGS	16

static int parse_cmd(char *cmd, char *args[])
{
	int argc = 0;
	int blank_on = 1;

	while (*cmd) {
		if (argc > MAX_ARGS) {
			log_err();
			return -1;
		}

		if (*cmd != ' ' && blank_on) {
			args[argc] = cmd;
			argc++;
			blank_on = 0;
			cmd++;
			continue;
		}
		if (*cmd == ' ') {
			blank_on = 1;
			*cmd = 0;
		}
		cmd++;
	}

	return argc;
}

int just_run_cmd(char *cmd_in)
{
	int err;
	int argc;
	char *args[MAX_ARGS];
	char cmd[256];
	struct input_cmd *cmd_list = cmd_head;
	int len;

	memcpy(cmd, cmd_in, 256);

	argc = parse_cmd(cmd, args);

	while (cmd_list) {
		if (strcmp(cmd, cmd_list->str)) {
			cmd_list = cmd_list->next;
			continue;
		}
		err = cmd_list->func(argc, args);
		if (err && !socket_write_buf(0, 0)) {
			len =
			    sprintf(help_msg,
				    "err: '%s', try 'help %s'\n", cmd, cmd);
			socket_write_buf(help_msg, len);
		}
		return err;
	}

	len = sprintf(help_msg, "unknow: '%s', try 'help'\n", cmd);
	socket_write_buf(help_msg, len);

	return -1;
}

int inline run_cmd(char *cmd_in)
{
	log_info("input: %s\n", cmd_in);

	return just_run_cmd(cmd_in);
}

static char cmd_buf[CMDBUF_LEN];

void cmd_loop(void)
{
	int ret;
	while (!game_over) {
		ret = socket_wait_client();
		if (ret)
			continue;
		ret = socket_read_client(cmd_buf, CMDBUF_LEN);
		if (ret <= 0) {
			socket_end_client();
			continue;
		}

		ret = run_cmd(cmd_buf);
		socket_end_client();
	}
}

static pthread_mutex_t mtx_cmd;
static pthread_mutexattr_t mat_cmd;

int input_cmd_init(void)
{
	int err;

	err = pthread_mutexattr_init(&mat_cmd);
	if (err) {
		log_err();
		goto init_mattr;
	}

	err = pthread_mutex_init(&mtx_cmd, &mat_cmd);
	if (err) {
		log_err();
		goto init_mutex;
	}

	return 0;

	pthread_mutex_destroy(&mtx_cmd);
 init_mutex:
	pthread_mutexattr_destroy(&mat_cmd);
 init_mattr:
	return err;

}

void input_cmd_exit(void)
{
	pthread_mutex_destroy(&mtx_cmd);
	pthread_mutexattr_destroy(&mat_cmd);
}

void register_cmd(struct input_cmd *cmd)
{
	pthread_mutex_lock(&mtx_cmd);
	cmd->next = cmd_head;
	cmd_head = cmd;
	pthread_mutex_unlock(&mtx_cmd);
}
