#include <pthread.h>
#include <log_project3.h>
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

static char *cmd_buf;
unsigned cmdbuf_index = 0;
#define CMDBUF_NUM	4
#define CMDBUF_LEN	64

char *check_cmd(char *cmd_in)
{
	char *cmd = cmd_in;
	char *cmd_out = 0;

	while (!cmd_out) {
		switch (*cmd) {
		case ' ':
		case '\n':
		case '\t':
		case '\r':
			break;
		case '#':
			return 0;
		default:
			if (*cmd < 0x20)
				return 0;
			if (*cmd > 0x7e)
				return 0;
			if (*cmd)
				cmd_out = cmd;
		}
		cmd++;
	}

	return cmd_out;
}

int inline just_run_cmd(char *cmd_in)
{
	char *cmd;

	cmd = check_cmd(cmd_in);

	if (!cmd) {
		log_err();
		return -1;
	}

	return modes[cmd_mod].func(cmd);
}

int inline run_cmd(char *cmd_in)
{
	log_info("%s %s\n", modes[cmd_mod].name, cmd_in);

	return just_run_cmd(cmd_in);
}

static char show_buf[CMDBUF_LEN];
static char show_blank[CMDBUF_LEN];

static void input_refresh_cmdline(char *str, int pos)
{
	mvwprintw(input_win, LINES_INPUT - 1, 0, "%s%s",
		  modes[cmd_mod].name, show_blank);
	lock_scr();
	wrefresh(input_win);
	unlock_scr();
	memset(show_buf, 0, CMDBUF_LEN);
	memcpy(show_buf, str, strlen(str));
	if (show_buf[pos])
		show_buf[pos] = '*';
	mvwprintw(input_win, LINES_INPUT - 1, 0, "%s%s",
		  modes[cmd_mod].name, show_buf);
	lock_scr();
	wrefresh(input_win);
	unlock_scr();
}

#define ENTER	0x0d
#define BACKS	0x7f
#define DELET	0x08

#define ESC	0x1b

#define ARROW_UP	0x1b5b41
#define ARROW_DOWN	0x1b5b42
#define ARROW_RIGHT	0x1b5b43
#define ARROW_LEFT	0x1b5b44

static char *scan_cmd_buf(void)
{
	chtype ch;
	int i;
	unsigned key;
	char *cur_buf = cmd_buf + cmdbuf_index * CMDBUF_LEN;

	memset(cur_buf, 0, CMDBUF_LEN);
	i = 0;
	key = 0;

	do {
		ch = getchar();

		if (key) {
			key = (key << 8) + ch;
			if (key < 0x1b0000)
				continue;
			switch (key) {
			case ARROW_UP:
				if (cmdbuf_index > 0)
					cmdbuf_index--;
				else
					cmdbuf_index = CMDBUF_NUM - 1;
				cur_buf = cmd_buf + cmdbuf_index * CMDBUF_LEN;
				i = strlen(cur_buf);
				break;
			case ARROW_DOWN:
				if (cmdbuf_index < CMDBUF_NUM - 1)
					cmdbuf_index++;
				else
					cmdbuf_index = 0;
				cur_buf = cmd_buf + cmdbuf_index * CMDBUF_LEN;
				i = strlen(cur_buf);
				break;
			case ARROW_RIGHT:
				if (cur_buf[i])
					i++;
				break;
			case ARROW_LEFT:
				if (i > 0)
					i--;
				break;
			default:
				log_info("detect unknow:%x\n", key);
			}
			input_refresh_cmdline(cur_buf, i);
			key = 0;
			continue;
		}

		switch (ch) {
		case BACKS:
		case DELET:
			if (i == 0)
				break;
			i--;
			cur_buf[i] = 0;
			input_refresh_cmdline(cur_buf, i);
			break;
		case ESC:
			key = ESC;
			break;
		case ENTER:
			if (i > 0) {
				cmdbuf_index++;
				if (cmdbuf_index >= CMDBUF_NUM)
					cmdbuf_index = 0;
				return cur_buf;
			}
			return 0;
		default:
			cur_buf[i] = ch;
			i++;
			input_refresh_cmdline(cur_buf, i);
		}
	} while (i < CMDBUF_LEN);

	log_info("cmd buf overflow!\n");

	return 0;
}

void cmd_loop(void)
{
	char *cmd;

	while (!game_over) {
		print_inputwin(modes[cmd_mod].name);
		cmd = scan_cmd_buf();
		if (!cmd)
			continue;
		run_cmd(cmd);
	}
}

static pthread_mutex_t mtx_cmd;
static pthread_mutexattr_t mat_cmd;

int input_cmd_init(void)
{
	int err;

	cmd_buf = malloc(CMDBUF_LEN * CMDBUF_NUM);

	if (!cmd_buf) {
		log_system_err(__FUNCTION__);
		goto alloc_cmdbuf;
	}

	memset(cmd_buf, 0, CMDBUF_LEN * CMDBUF_NUM);

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

	memset(show_blank, ' ', CMDBUF_LEN);
	show_blank[CMDBUF_LEN - 1] = 0;

	return 0;

	pthread_mutex_destroy(&mtx_cmd);
 init_mutex:
	pthread_mutexattr_destroy(&mat_cmd);
 init_mattr:
	free(cmd_buf);
	cmd_buf = 0;
 alloc_cmdbuf:
	return err;

}

void input_cmd_exit(void)
{
	if (cmd_buf)
		free(cmd_buf);
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
