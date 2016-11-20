#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>

int do_len(int argc, char *argv[])
{
	struct interface_info *ifo;
	struct cylinder_info *mfo;
	int ncylinder;
	int index;
	int val;

	if (argc != 3)
		return -1;

	ifo = get_interface_info();
	mfo = get_motion_info(&ncylinder);

	if (!ifo->dev.id) {
		log_info("%s, no interface board!\n", __FUNCTION__);
		return -1;
	}

	index = atoi(argv[1]);

	if (index < 0 || index >= ncylinder) {
		log_err();
		return -1;
	}

	if (!mfo[index].dev.id) {
		log_info("%s, no cylinder %d\n", __FUNCTION__, index);
		return -1;
	}

	if (!ifo->m12v) {
		log_info("%s, megnet power is not on, stop!\n", __FUNCTION__);
		return -1;
	}

	val = atoi(argv[2]);

	if (run_cmd("wait air 2")) {
		log_info("%s, pressure is low, stop!\n", __FUNCTION__);
		return -1;
	}

	return megnet(&mfo[index], val);
}

static struct input_cmd cmd = {
	.str = "len",
	.func = do_len,
	.info = "example 'len 1 -128'",
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
