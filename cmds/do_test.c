#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>


static int do_test(int argc, char *argv[])
{
	int i;
        log_info("%s:\n", __FUNCTION__);
	for (i = 0; i < argc ; i++)
		log_info("    argv[%d]:%s\n", i, argv[i]);
        return 0;
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
