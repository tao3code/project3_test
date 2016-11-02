#ifndef INPUT_CMD_H
#define INPUT_CMD_H

struct input_cmd {
	struct input_cmd *next;
        const char *str;
        int (*func)(int argc, char *argv[]);
	const char *info;
};

int input_cmd_init(void);
void input_cmd_exit(void);
void register_cmd(struct input_cmd *cmd);

void cmd_loop(void);
int run_cmd(char *cmd);

int open_scr(void);
void close_scr(void);

void update_control_window(void);
void update_motion_window(void);

#endif
