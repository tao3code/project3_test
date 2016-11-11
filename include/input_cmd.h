#ifndef INPUT_CMD_H
#define INPUT_CMD_H

#include <curses.h>

extern WINDOW *input_win;
extern WINDOW *ctrl_win;
extern WINDOW *motion_win;

#define LINES_INPUT     8
#define LINE_CTRL       9
#define ROW_CTRL        24
#define LINE_MOTION     6
#define ROW_MOTION      48

struct input_cmd {
	struct input_cmd *next;
	const char *str;
	int (*func) (int argc, char *argv[]);
	const char *info;
};

int input_cmd_init(void);
void input_cmd_exit(void);
void register_cmd(struct input_cmd *cmd);

void cmd_loop(void);
char *check_cmd(char *cmd_in);
int run_cmd(char *cmd);

int open_scr(void);
void close_scr(void);

void lock_scr(void);
void unlock_scr(void);

#endif
