#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>

static int do_test(int argc, char *argv[])
{
	return run_cmd("run example/test.txt");
}

static struct input_cmd cmd = {
	.str = "test",
	.func = do_test,
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
