#ifndef STDCMD_H

#define MAX_VAL_LEN	16

struct func_arg {
	const char *name;
	void *var;
	const char *type;
	int (*check) (void);
};

struct cmd_func {
	const char *name;
	int (*func) (struct func_arg * args);
	struct func_arg *args;
};

int cmd_run_funcs(const char *in, struct cmd_func *funcs);

#endif
