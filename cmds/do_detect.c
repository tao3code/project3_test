#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>

static int do_detect(int argc, char *argv[])
{
	int i, ncy;
	int find = 0;
	struct cylinder_info *cy_info;
	struct interface_info *if_info;

	cy_info = get_motion_info(&ncy);
	if_info = get_interface_info();

	for (i = 0; i < ncy; i++) {
		test_device(&cy_info[i].dev);
		if (cy_info[i].dev.id)
			find++;
	}

	test_device(&if_info->dev);
	if(if_info->dev.id)
		find++;
	log_info("detect: find %d devices\n", find);

	if (find != 13) {
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
