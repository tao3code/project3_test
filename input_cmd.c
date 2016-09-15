#include <input_cmd.h>
#include <curses.h>
#include <pthread.h>
#include <log_project3.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <robot.h>
#include <serial.h>

static int do_serial(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on")) 
		return serial_init();

	if (!strcmp(argv[1], "off")) { 
		serial_close();
		return 0;
	}

	return -1;
} 

static WINDOW *input_win;
static WINDOW *ctrl_win;
static WINDOW *motion_win;

#define LINES_INPUT	8	
#define LINE_CTRL	9
#define ROW_CTRL	24	
#define LINE_MOTION	6
#define ROW_MOTION	48	

static void show_motion(void)
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
	
static pthread_t motionshow_thread = 0;
static int motionshow_on = 0;

static void *motionshow_thread_func(void *arg)
{
	int *on = arg;

	while (*on) {
		usleep(100);
		update_motion_state();
		show_motion();
	}
	return 0;
}

static int do_motionshow(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on")) {
		if (motionshow_on)
			return 0;
		motionshow_on = 1;
		return pthread_create(&motionshow_thread, 0,
			motionshow_thread_func, &motionshow_on);
	}

	if(!strcmp(argv[1], "off")) {
		if (!motionshow_on)
			return 0;
		motionshow_on = 0;
		pthread_join(motionshow_thread, 0);
	}

	return 0;
}


static void show_control(void)
{
	const struct interface_info *info;

	info = get_interface_info();

	werase(ctrl_win);
	if (!info->id) {
		wprintw(ctrl_win, "Interface board does not exise!\n");
		lock_scr();
		wrefresh(ctrl_win);
		unlock_scr();

		return;
	}

	wprintw(ctrl_win, "vol: %u\n", info->vol);
	wprintw(ctrl_win, "air: %u\n", info->air);
	wprintw(ctrl_win, "gyr: %hd %hd %hd\n",
		 info->gx, info->gy, info->gz);
	wprintw(ctrl_win, "thm: %hd\n", info->thermal);	
	wprintw(ctrl_win, "acc: %hd %hd %hd\n",
		 info->ax, info->ay, info->az);
	lock_scr();
	wrefresh(ctrl_win);
	unlock_scr();
}

static pthread_t ctrlshow_thread = 0;
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

	if(!strcmp(argv[1], "off")) {
		if (!ctrlshow_on)
			return 0;
		ctrlshow_on = 0;
		pthread_join(ctrlshow_thread, 0);
	}

	return 0;
}

static int do_detect(int argc, char *argv[])
{
	int ret;

	ret = test_robot();
	if (ret < 0)
		return ret;

	mvwprintw(input_win, LINES_INPUT - 1, 0, "find:%d devices", ret);
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

static int do_test(int argc, char *argv[])
{
	show_motion();
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
	 .info = "Use 'raw' switch to raw mode, any input "
		 "sring expect 'noraw', will be "
	 	 "directly sent to serial port.",
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
	{
	 .str = "motionshow",
	 .func = do_motionshow,
	 .info = "Use 'motionshow on' or 'motionshow off'",
	},
	{
	 .str = "serial",
	 .func = do_serial,
	 .info = "Use 'serial on' or 'serial off'",
	},
};

#define NCMDS	(sizeof(cmds)/sizeof(struct input_cmd))

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

#define INPUT_MODE	0
#define RAW_MODE	1

static int cmd_mod = INPUT_MODE;

static int switch_to_raw(int argc, char *argv[])
{
	cmd_mod = RAW_MODE;
	return 0;
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
		modes[cmd_mod].func(cmd);
	}


	if (ctrlshow_on) {
		ctrlshow_on = 0;
		pthread_join(ctrlshow_thread, 0);
	}

	if (motionshow_on) {
		motionshow_on = 0;
		pthread_join(motionshow_thread, 0);
	}

}

static pthread_mutex_t mtx_scr;
static pthread_mutexattr_t mat_scr;

inline void lock_scr(void) {
	pthread_mutex_lock(&mtx_scr);
}

inline void unlock_scr(void) {
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
