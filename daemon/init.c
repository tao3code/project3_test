#include <stdio.h>

int (*init_func_start)(void);

int do_init_funcs(void)
{
	int err;
	int (**pfunc)(void) = &init_func_start;

	while(*pfunc) {
		err = (*pfunc)();
		if (err)
			return err;
		pfunc++;
	}
	return 0;
}

int (*exit_func_start)(void);

void do_exit_funcs(void)
{
	int (**pfunc)(void) = &exit_func_start;

	while(*pfunc) {
		(*pfunc)();
		pfunc++;
	}
}


