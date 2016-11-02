#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>

static int do_detect(int argc, char *argv[])
{
	int ret;

	ret = test_robot();
	if (ret < 0)
		return ret;

	log_info("detect: find %d devices\n", ret);

	if (ret != 13) {
		log_err();
		return -1;
	}
	return 0;
}

static struct input_cmd cmd = {
	.str = "detect",
	.func = do_detect,
	.info = "Query interface board and all cylinders",
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
