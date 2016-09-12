#include <input_cmd.h>
#include <curses.h>
#include <pthread.h>
#include <log_project3.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static WINDOW *input_win;

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

static int do_list(int argc, char *argv[]);

static struct input_cmd cmds[] = {
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
	 },
};

#define NCMDS	(sizeof(cmds)/sizeof(struct input_cmd))
#define LINES_INPUT	5

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
	for( i = 0; i < NCMDS; i++)
		len += sprintf(buf + len, " %s", cmds[i].str);

	mvwprintw(input_win, LINES_INPUT -1, 0, 
		"support:%s", buf);
	free(buf);
	return 0;
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

static int do_cmds(char *cmd)
{
	int i;
	int err;
	int argc;
	char *args[MAX_ARGS];

	argc = parse_cmd(cmd, args); 

	for (i = 0; i < NCMDS; i++) {
		if (strcmp(cmd, cmds[i].str))
			continue;
		err = cmds[i].func(argc, args);
		return err;
	}
	mvwprintw(input_win, LINES_INPUT -1, 0, "unknow: '%s', try 'list'", cmd);
	return -1;
}

static char cmd_buf[256];

#define ENTER	0xd

void cmd_loop(void)
{
	chtype ch;
	int i;

	while (!game_over) {
		i = 0;
		scroll(input_win);
		mvwprintw(input_win, LINES_INPUT - 1, 0, "input:");
		wrefresh(input_win);

		do {
			ch = getchar();
			cmd_buf[i] = ch;
			i++;
			waddch(input_win, ch);
			wrefresh(input_win);
		} while (ch != ENTER);
		cmd_buf[i - 1] = 0;
		do_cmds(cmd_buf);
	}
}

int open_scr(void)
{
	initscr();
	cbreak();
	noecho();
	input_win = newwin(LINES_INPUT, COLS, LINES - LINES_INPUT, 0);
	scrollok(input_win, 1);

	return 0;
}

void close_scr(void)
{
	delwin(input_win);
	endwin();
}
