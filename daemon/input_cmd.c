#include <pthread.h>
#include <log_project3.h>
#include <socket_project3.h>
#include <string.h>
#include <stdlib.h>
#include <input_cmd.h>
#include <robot.h>
#include <serial.h>

WINDOW *input_win;
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
	input_win = newwin(LINES_INPUT, COLS, LINES - LINES_INPUT, 0);
	ctrl_win = newwin(LINE_CTRL, ROW_CTRL, 0, 0);
	motion_win = newwin(LINE_MOTION, ROW_MOTION, 0, ROW_CTRL + 4);
	scrollok(input_win, 1);

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
	delwin(input_win);
	delwin(ctrl_win);
	delwin(motion_win);
	endwin();
}

#define INPUT_MODE	0
#define RAW_MODE	1
#define SILENT_MODE	2

static int cmd_mod = INPUT_MODE;

static int switch_to_raw(int argc, char *argv[])
{
	cmd_mod = RAW_MODE;
	return 0;
}

static int switch_to_silent(int argc, char *argv[])
{
	cmd_mod = SILENT_MODE;
	return 0;
}

static unsigned char game_over = 0;

static int do_quit(int argc, char *argv[])
{
	game_over = 1;
	return 0;
}

static int do_help(int argc, char *argv[]);

static struct input_cmd buildin_cmd[] = {
	{
	 .str = "help",
	 .func = do_help,
	 .info = "'help COMMAND' display some informations",
	 .next = &buildin_cmd[1],
	 },
	{
	 .str = "quit",
	 .func = do_quit,
	 .next = &buildin_cmd[2],
	 },
	{
	 .str = "raw",
	 .func = switch_to_raw,
	 .info = "Use 'raw' switch to raw mode, any input "
	 "sring expect 'noraw', will be " "directly sent to serial port.",
	 .next = &buildin_cmd[3],
	 },
	{
	 .str = "silence",
	 .func = switch_to_silent,
	 .info = "Use 'silence' switch to silent mode, ingro error result.",
	 .next = 0,
	 },
};

void print_inputwin(const char *str)
{
	scroll(input_win);
	mvwprintw(input_win, LINES_INPUT - 1, 0, "%s", str);
	lock_scr();
	wrefresh(input_win);
	unlock_scr();
}

static struct input_cmd *cmd_head = &buildin_cmd[0];

static int list_all_cmds(char *buf)
{
	int len = 0;
	struct input_cmd *cmd_list = cmd_head;

	while (cmd_list) {
		len += sprintf(buf + len, " %s", cmd_list->str);
		cmd_list = cmd_list->next;
	}

	print_inputwin(buf);
	return 0;
}

static char help_msg[256];

static int do_help(int argc, char *argv[])
{
	int err;
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
			cmd_list->help(help_msg, argc - 1, &argv[1]);
			print_inputwin(help_msg);
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

static int input_run_cmd(char *cmd_in)
{
	int err;
	int argc;
	char *args[MAX_ARGS];
	char cmd[256];
	struct input_cmd *cmd_list = cmd_head;

	memcpy(cmd, cmd_in, 256);

	argc = parse_cmd(cmd, args);

	while (cmd_list) {
		if (strcmp(cmd, cmd_list->str)) {
			cmd_list = cmd_list->next;
			continue;
		}
		err = cmd_list->func(argc, args);
		if (err) {
			sprintf(help_msg, "err: '%s', try 'help %s'", cmd, cmd);
			print_inputwin(help_msg);
		}
		return err;
	}

	sprintf(help_msg, "unknow: '%s', try 'help'", cmd);
	print_inputwin(help_msg);

	return -1;
}

static int raw_run_cmd(char *cmd)
{
	char *res;
	int ret;

	if (!strcmp(cmd, "noraw")) {
		cmd_mod = INPUT_MODE;
		return 0;
	}

	ret = sent_cmd_alloc_response(cmd, &res);
	if (ret < 0) {
		sprintf(help_msg, "operation fail: %s", cmd);
		print_inputwin(help_msg);
		return 0;
	}

	if (ret == 0)
		return 0;

	print_inputwin(res);
	free(res);

	return 0;
}

static int silent_run_cmd(char *cmd_in)
{
	int argc;
	char *args[MAX_ARGS];
	char cmd[256];
	struct input_cmd *cmd_list = cmd_head;

	memcpy(cmd, cmd_in, 256);

	argc = parse_cmd(cmd, args);

	while (cmd_list) {
		if (strcmp(cmd, cmd_list->str)) {
			cmd_list = cmd_list->next;
			continue;
		}
		cmd_list->func(argc, args);
		break;
	}
	return 0;
}

static struct mod_info {
	const char *name;
	int (*func) (char *cmd);
} modes[] = {
	[INPUT_MODE] {
	.name = "input:",.func = input_run_cmd,},[RAW_MODE] {
	.name = "('noraw' to exit) raw:",.func = raw_run_cmd,},[SILENT_MODE] {
.name = "('nosilence' to exit) silent:",.func = silent_run_cmd,},};

int inline just_run_cmd(char *cmd_in)
{
	return modes[cmd_mod].func(cmd_in);
}

int inline run_cmd(char *cmd_in)
{
	log_info("%s %s\n", modes[cmd_mod].name, cmd_in);

	return just_run_cmd(cmd_in);
}

static char cmd_buf[CMDBUF_LEN];

void cmd_loop(void)
{
	int ret;
	while (!game_over) {
		print_inputwin(modes[cmd_mod].name);
		ret = socket_poll_read(cmd_buf, CMDBUF_LEN);

		if (ret <= 0)
			continue;
		run_cmd(cmd_buf);
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
