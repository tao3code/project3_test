#ifndef INPUT_CMD_H
#define INPUT_CMD_H

struct input_cmd {
        const char *str;
        int (*func)(int argc, char *argv[]);
	const char *info;
};

void cmd_loop(void);

int open_scr(void);
void close_scr(void);
void lock_scr(void);
void unlock_scr(void);

#endif
