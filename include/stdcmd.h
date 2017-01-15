#ifndef STDCMD_H
#define STDCMD_H

#define MAX_VAL_LEN	16

struct func_arg {
	const char *name;
	void *var;
	const char *type;
	int (*check) (void *var);
};

struct cmd_func {
	const char *name;
	int (*func) (struct func_arg * args);
	struct func_arg *args;
};

int stdcmd_run_funcs(const char *in, struct cmd_func *funcs);
int stdcmd_update_args(const char *in, struct func_arg *args);
int stdcmd_help(char *buf, struct cmd_func *funcs, int argc, char *argv[]);

#endif
