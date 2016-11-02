#include <input_cmd.h>
#include <curses.h>
#include <pthread.h>
#include <log_project3.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <robot.h>
#include <serial.h>
#include <sys/time.h>

static WINDOW *input_win;
static WINDOW *ctrl_win;
static WINDOW *motion_win;

#define LINES_INPUT	8
#define LINE_CTRL	9
#define ROW_CTRL	24
#define LINE_MOTION	6
#define ROW_MOTION	48

static pthread_mutex_t mtx_scr;
static pthread_mutexattr_t mat_scr;

static inline void lock_scr(void)
{
	pthread_mutex_lock(&mtx_scr);
}

static inline void unlock_scr(void)
{
	pthread_mutex_unlock(&mtx_scr);
}

int open_scr(void)
{
	int err;
	err = pthread_mutexattr_init(&mat_scr);
	if (err) {
		log_err();
		goto init_mattr;
	}

	err = pthread_mutex_init(&mtx_scr, &mat_scr);
	if (err) {
		log_err();
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

void update_motion_window(void)
{
	const struct cylinder_info *info;
	int count;
	int i;

	info = get_motion_info(&count);
	werase(motion_win);
	for (i = 0; i < count; i++) {
		if (!info[i].id)
			wprintw(motion_win, "len[%d]: NULL", i);
		else
			wprintw(motion_win, "len[%d]: %hu", i, info[i].len);

		if (i & 0x1)
			waddch(motion_win, '\n');
		else
			waddch(motion_win, '\t');
	}
	lock_scr();
	wrefresh(motion_win);
	unlock_scr();
}

void update_control_window(void)
{
	const struct interface_info *info;

	info = get_interface_info();

	werase(ctrl_win);
	wprintw(ctrl_win, "utc: %lu\n", time(NULL));

	if (!info->id) {
		wprintw(ctrl_win, "Interface board does not exise!\n");
		lock_scr();
		wrefresh(ctrl_win);
		unlock_scr();

		return;
	}

	wprintw(ctrl_win, "vol: %u\n", info->vol);
	wprintw(ctrl_win, "air: %u\n", info->air);
	wprintw(ctrl_win, "gyr: %hd %hd %hd\n", info->gx, info->gy, info->gz);
	wprintw(ctrl_win, "thm: %hd\n", info->thermal);
	wprintw(ctrl_win, "acc: %hd %hd %hd\n", info->ax, info->ay, info->az);
	wprintw(ctrl_win, "m12: %c\n", info->m12v);
	lock_scr();
	wrefresh(ctrl_win);
	unlock_scr();
}

#define INPUT_MODE	0
#define RAW_MODE	1

static int cmd_mod = INPUT_MODE;

static int switch_to_raw(int argc, char *argv[])
{
	cmd_mod = RAW_MODE;
	return 0;
}

static int do_erase(int argc, char *argv[])
{
	werase(input_win);
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
	 .info = "'help COMMAND' display some informations, "
	 " COMMAND should be from 'list'",
	 .next = &buildin_cmd[1],
	 },
	{
	 .str = "quit",
	 .func = do_quit,
	 .next = &buildin_cmd[2],
	 },
	{
	 .str = "erase",
	 .func = do_erase,
	 .next = &buildin_cmd[3],
	 },
	{
	 .str = "raw",
	 .func = switch_to_raw,
	 .info = "Use 'raw' switch to raw mode, any input "
	 "sring expect 'noraw', will be " "directly sent to serial port.",
	 .next = 0,
	 },
};

static struct input_cmd *cmd_head = &buildin_cmd[0];

static int list_all_cmds(void)
{
	char *buf;
	int len;
	struct input_cmd *cmd_list = cmd_head;

	buf = malloc(4096);
	if (!buf) {
		log_err();
		return -1;
	}

	len = 0;
	while (cmd_list) {
		len += sprintf(buf + len, " %s", cmd_list->str);
		cmd_list = cmd_list->next;
	}

	mvwprintw(input_win, LINES_INPUT - 1, 0, "support:%s", buf);
	free(buf);
	return 0;
}

static int do_help(int argc, char *argv[])
{
	int i;
	int err;
	struct input_cmd *cmd_list = cmd_head;

	if (argc == 1)
		return list_all_cmds();

	for (i = 1; i < argc; i++) {
		err = -1;
		while (cmd_list) {
			if (strcmp(argv[i], cmd_list->str)) {
				cmd_list = cmd_list->next;
				continue;
			}
			mvwprintw(input_win, LINES_INPUT - 1, 0,
				  "%s:\n\t%s\n", argv[i], cmd_list->info);
			err = 0;
			cmd_list = cmd_head;
			break;
		}
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
		if (err)
			mvwprintw(input_win, LINES_INPUT - 1, 0,
				  "err: '%s', try 'help %s'", cmd, cmd);
		return err;
	}

	mvwprintw(input_win, LINES_INPUT - 1, 0,
		  "unknow: '%s', try 'list'", cmd);
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
		mvwprintw(input_win, LINES_INPUT - 1, 0,
			  "operation fail: %s", cmd);
		return 0;
	}

	if (ret == 0)
		return 0;

	mvwprintw(input_win, LINES_INPUT - 1, 0, "%s", res);
	free(res);

	return 0;
}

static struct mod_info {
	const char *name;
	int (*func) (char *cmd);
} modes[] = {
	[INPUT_MODE] {
	.name = "input:",.func = input_run_cmd,},[RAW_MODE] {
.name = "('noraw' to exit) raw:",.func = raw_run_cmd,},};

static char cmd_buf[256];

int inline run_cmd(char *cmd)
{
	return modes[cmd_mod].func(cmd);
}

#define ENTER	0x0d
#define BACKS	0x7f

static char *scan_cmd_buf(void)
{
	chtype ch;
	int i;
	int x, y;

	i = 0;
	do {
		ch = getchar();
		if (ch == BACKS) {
			if (i == 0)
				continue;
			i--;
			cmd_buf[i] = 0;
			getyx(input_win, y, x);
			wmove(input_win, y, x - 1);
			wdelch(input_win);
			lock_scr();
			wrefresh(input_win);
			unlock_scr();
			continue;
		}

		cmd_buf[i] = ch;
		i++;
		waddch(input_win, ch);
		lock_scr();
		wrefresh(input_win);
		unlock_scr();
	} while (ch != ENTER);

	if (i < 2)
		return 0;

	cmd_buf[i - 1] = 0;

	return cmd_buf;
}

void cmd_loop(void)
{
	char *cmd;

	while (!game_over) {
		scroll(input_win);
		mvwprintw(input_win, LINES_INPUT - 1, 0, modes[cmd_mod].name);
		lock_scr();
		wrefresh(input_win);
		unlock_scr();
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
