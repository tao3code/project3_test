#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>

static int do_erase(int argc, char *argv[])
{
	werase(input_win);
	return 0;
}

static struct input_cmd cmd = {
	.str = "erase",
	.func = do_erase,
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
