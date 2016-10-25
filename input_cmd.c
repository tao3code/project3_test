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

int do_set(int argc, char *argv[])
{
	const struct cylinder_info *info;
	int ncylinder;
	int index;
	int val;

	if (argc != 3)
		return -1;

	info = get_motion_info(&ncylinder);
	index = atoi(argv[1]);

	if (index < 0 || index > ncylinder) {
		log_err();
		return -1;
	}

	if (!info[index].id) {
		log_info("%s, no cylinder %d\n", __FUNCTION__, index);
		return -1;
	}

	val = atoi(argv[2]);

	return set_encoder(index, val);
}

static int wait_air_ready(void)
{
	time_t start, end;
	const struct interface_info *info = get_interface_info();

	start = time(NULL);
	end = time(NULL);
	while((end -start) > 2) {
		if(info->air > AIR_THRESHOLD_L) {
			return 0;
		}
		usleep(500);
		end = time(NULL);
	}
	return -1;
}

int do_len(int argc, char *argv[])
{
	const struct interface_info *ifo;
	const struct cylinder_info *mfo;
	int ncylinder;
	int index;
	int val;

	if (argc != 3)
		return -1;

	ifo = get_interface_info();
	mfo = get_motion_info(&ncylinder);

	if (!ifo->id) {
		log_info("%s, no interface board!\n", __FUNCTION__);
		return -1;
	}

	index = atoi(argv[1]);

	if (index < 0 || index > ncylinder) {
		log_err();
		return -1;
	}

	if (!mfo[index].id) {
		log_info("%s, no cylinder %d\n", __FUNCTION__, index);
		return -1;
	}

	if (ifo->vol < VOLTAGE_LOW) {
		log_info("%s, voltage is low, stop!\n", __FUNCTION__);
		return -1;
	}

	val = atoi(argv[2]);

	if (wait_air_ready()) {
		log_info("%s, pressure is low, stop!\n", __FUNCTION__);
		return -1;
	}

	return megnet(index, val);
}

static int do_meg(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on"))
		return meg12v_on('1');

	if (!strcmp(argv[1], "off"))
		return meg12v_on('0');

	return -1;
}

static pthread_t air_thread = 0;
static int air_on = 0;

static void *air_thread_func(void *arg)
{
	int *on = arg;
	const struct interface_info *info;
	char air;
	int need_run;
	int run_count;
	int wait_count;

	info = get_interface_info();
	log_info("%s start\n", __FUNCTION__);

	while (*on) {
		air = info->air;
		if (air < AIR_THRESHOLD_L)
			need_run = 1;
		if (air > AIR_THRESHOLD_H)
			need_run = 0;
		if (!need_run) {
			usleep(200);
			continue;
		}

		run_count =
		    (AIR_THRESHOLD_H - air) * 255 / AIR_THRESHOLD_H + 16;
		engine_on(run_count);
		log_info("engine_on(%d)\n", run_count);
		wait_count = run_count * 4 / 255;
		if (wait_count < 1)
			wait_count = 1;
		sleep(wait_count);
		update_presure();
	}

	log_info("%s stop\n", __FUNCTION__);

	return 0;
}

static int do_air(int argc, char *argv[])
{
	if (argc != 2)
		return -1;
	if (!strcmp(argv[1], "on")) {
		if (air_on)
			return 0;
		air_on = 1;
		return pthread_create(&air_thread, 0, air_thread_func, &air_on);
	}

	if (!strcmp(argv[1], "off")) {
		if (!air_on)
			return 0;
		air_on = 0;
		pthread_join(air_thread, 0);
		return 0;
	}

	return engine_on(atoi(argv[1]));
}

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

	log_info("%s start\n", __FUNCTION__);

	while (*on) {
		usleep(100);
		update_motion_state();
		show_motion();
	}

	log_info("%s stop\n", __FUNCTION__);
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

	if (!strcmp(argv[1], "off")) {
		if (!motionshow_on)
			return 0;
		motionshow_on = 0;
		pthread_join(motionshow_thread, 0);
		return 0;
	}

	return -1;
}

static void show_control(void)
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

static pthread_t ctrlshow_thread = 0;
static int ctrlshow_on = 0;

static void *ctrlshow_thread_func(void *arg)
{
	int *on = arg;

	log_info("%s start\n", __FUNCTION__);

	while (*on) {
		usleep(100);
		update_control_state();
		show_control();
	}

	log_info("%s stop\n", __FUNCTION__);

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

	if (!strcmp(argv[1], "off")) {
		if (!ctrlshow_on)
			return 0;
		ctrlshow_on = 0;
		pthread_join(ctrlshow_thread, 0);
		return 0;
	}

	return -1;
}

static int do_detect(int argc, char *argv[])
{
	int ret;

	ret = test_robot();
	if (ret < 0)
		return ret;

	mvwprintw(input_win, LINES_INPUT - 1, 0, "find:%d devices", ret);

	if (ret != 13) {
		log_err();
		scroll(input_win);
		return -1;
	}

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
	 "sring expect 'noraw', will be " "directly sent to serial port.",
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
	{
	 .str = "air",
	 .func = do_air,
	 .info = "Use 'air on' or 'air off'",
	 },
	{
	 .str = "meg",
	 .func = do_meg,
	 .info = "Use 'meg on' or 'meg off'",
	 },
	{
	 .str = "len",
	 .func = do_len,
	 .info = "example 'len 1 -128'",
	 },
	{
	 .str = "set",
	 .func = do_set,
	 .info = "example 'set 1 0'",
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

	/* command loop stop, do clean up work */

	if (ctrlshow_on) {
		ctrlshow_on = 0;
		pthread_join(ctrlshow_thread, 0);
	}

	if (motionshow_on) {
		motionshow_on = 0;
		pthread_join(motionshow_thread, 0);
	}

	if (air_on) {
		air_on = 0;
		pthread_join(air_thread, 0);
	}

	if (is_serial_on()) {
		run_cmd("meg off");
		run_cmd("serial off");
	}

}

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
