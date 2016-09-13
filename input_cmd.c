#include <input_cmd.h>
#include <curses.h>
#include <pthread.h>
#include <log_project3.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <robot.h>

static WINDOW *input_win;
static WINDOW *ctrl_win;

static void show_control(void)
{
	const struct interface_info *info;

	info = get_interface_info();

	werase(ctrl_win);
	if (!info->id) {
		wprintw(ctrl_win, "Interface board does not exise!\n");
		wrefresh(ctrl_win);

		return;
	}

	wprintw(ctrl_win, "vol: %u\n", info->vol);
	wprintw(ctrl_win, "air: %u\n", info->air);
	wprintw(ctrl_win, "gyr: %hd %hd %hd\n",
		 info->gx, info->gy, info->gz);
	wprintw(ctrl_win, "thm: %hd\n", info->thermal);	
	wprintw(ctrl_win, "acc: %hd %hd %hd\n",
		 info->ax, info->ay, info->az);
	wrefresh(ctrl_win);
}

static pthread_t ctrlshow_thread;
static int ctrlshow_on = 0;

static void *ctrlshow_thread_func(void *arg)
{
	int *on = arg;

	while (*on) {
		usleep(100);
		update_control_state();
		show_control();
	}
	return 0;
}

static int do_ctrlshow(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on")) {
		if (ctrlshow_on)
			return 0;
		ctrlshow_on = 1;
		return pthread_create(&ctrlshow_thread, 0,
			ctrlshow_thread_func, &ctrlshow_on);
	}

	if(!strcmp(argv[1], "off"))
		ctrlshow_on = 0;

	return 0;
}

static int do_detect(int argc, char *argv[])
{
	test_robot();
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
	sleep(1);
	return 0;
}

static int do_test(int argc, char *argv[])
{
	update_control_state();
	show_control();
	return 0;
}

static int do_list(int argc, char *argv[]);
static int switch_to_raw(int argc, char *argv[]);
static int do_help(int argc, char *argv[]);

static struct input_cmd cmds[] = {
	{
	 .str = "test",
	 .func = do_test,
	 },
	{
	 .str = "help",
	 .func = do_help,
	 .info = "'help COMMAND' display some informations, "
	 " COMMAND should be from 'list'",
	 },
	{
	 .str = "quit",
	 .func = do_quit,
	 },
	{
	 .str = "erase",
	 .func = do_erase,
	 },
	{
	 .str = "list",
	 .func = do_list,
	 .info = "list out all of the supported command",
	 },
	{
	 .str = "raw",
	 .func = switch_to_raw,
	 .info = "Use 'raw' switch to raw mode, and"
	 "directly send charactors to serial port",
	 },
	{
	 .str = "detect",
	 .func = do_detect,
	 .info = "Query interface board and all cylinders",
	},
	{
	 .str = "ctrlshow",
	 .func = do_ctrlshow,
	 .info = "Use 'ctrlshow on' or 'ctrlshow off'",
	},
};

#define NCMDS	(sizeof(cmds)/sizeof(struct input_cmd))
#define LINES_INPUT	16	

static int do_list(int argc, char *argv[])
{
	int i;
	char *buf;
	int len;

	buf = malloc(4096);
	if (!buf) {
		log_err();
		return -1;
	}

	len = 0;
	for (i = 0; i < NCMDS; i++)
		len += sprintf(buf + len, " %s", cmds[i].str);

	mvwprintw(input_win, LINES_INPUT - 1, 0, "support:%s", buf);
	free(buf);
	return 0;
}

static int do_help(int argc, char *argv[])
{
	int i;
	int j;
	int err;

	for (i = 1; i < argc; i++) {
		err = -1;
		for (j = 0; j < NCMDS; j++) {
			if (strcmp(argv[i], cmds[j].str))
				continue;
			mvwprintw(input_win, LINES_INPUT - 1, 0,
				  "%s:\n\t%s\n", argv[i], cmds[j].info);
			err = 0;
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
	int i;
	int err;
	int argc;
	char *args[MAX_ARGS];
	char cmd[256];

	memcpy(cmd, cmd_in, 256);

	argc = parse_cmd(cmd, args);

	for (i = 0; i < NCMDS; i++) {
		if (strcmp(cmd, cmds[i].str))
			continue;
		err = cmds[i].func(argc, args);
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
	return 0;
}

#define INPUT_MODE	0
#define RAW_MODE	1

static int cmd_mod = INPUT_MODE;

static int switch_to_raw(int argc, char *argv[])
{
	cmd_mod = RAW_MODE;
	return 0;
}

static struct mod_info {
	const char *name;
	int (*func) (char *cmd);
} modes[] = {
	[INPUT_MODE] {
	.name = "input:",.func = input_run_cmd,},[RAW_MODE] {
.name = "('Esc' to exit) raw:",.func = raw_run_cmd,},};

static char cmd_buf[256];

#define ENTER	0x0d
#define ESCAP	0x1b

static char *scan_cmd_buf(void)
{
	chtype ch;
	int i;

	i = 0;
	do {
		ch = getchar();
		if (ch == ESCAP && cmd_mod == RAW_MODE) {
			cmd_mod = INPUT_MODE;
			return 0;
		}
		cmd_buf[i] = ch;
		i++;
		waddch(input_win, ch);
		wrefresh(input_win);
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
		wrefresh(input_win);
		cmd = scan_cmd_buf();
		if (!cmd)
			continue;
		modes[cmd_mod].func(cmd);
	}
}

int open_scr(void)
{
	initscr();
	cbreak();
	noecho();
	input_win = newwin(LINES_INPUT, COLS, LINES - LINES_INPUT, 0);
	ctrl_win = newwin(9, 32, 0, 0);
	scrollok(input_win, 1);

	return 0;
}

void close_scr(void)
{
	delwin(input_win);
	delwin(ctrl_win);
	endwin();
}
