#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>

#define MAX_WAIT_TIME	5

static int wait_time(int argc, char *argv[])
{
	int val;
	
	if (argc != 2) {
		log_err();
		return -1;
	}

	val = atoi(argv[1]);
	if (val <= 0)
		return 0;
	if (val > MAX_WAIT_TIME)
		val = MAX_WAIT_TIME;
	sleep(val);
	return 0;
}

static int wait_stand(int argc, char *argv[])
{
	struct interface_info *info;

	info = get_interface_info();

	update_gyroscope();

	if (info->ax > -14000) {
		log_err();
		return -1;
	}

	if (abs(info->ay) > 4000) {
		log_err();
		return -1;
	}

	if (abs(info->az) > 8000) {
		log_err();
		return -1;
	}

	return 0;
}

static struct input_cmd sub_cmd[] = {
	{
        .str = "time",
        .func = wait_time,
	.info = "example: wait time 10"
	},
	{
        .str = "stand",
        .func = wait_stand,
	.info = "example: wait stand"
	},
};

#define NSUB_CMD	(sizeof(sub_cmd)/sizeof(struct input_cmd))

static int do_wait(int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		log_err();
		return -1;
	}
	
	for (i = 0; i < NSUB_CMD; i ++) {
		if(strcmp(argv[1], sub_cmd[i].str))
			continue;
		return  sub_cmd[i].func(argc - 1, &argv[1]);
	}

	return -1;
}


static char help_info[256];

static struct input_cmd cmd = {
	.str = "wait",
	.func = do_wait,
	.info = help_info,
};

static int reg_cmd(void)
{
	int i;
	int len = 0;
	
	for (i = 0; i < NSUB_CMD; i ++) {
		if (! sub_cmd[i].info)
			continue;
		len += sprintf(&help_info[len], " %s\n", sub_cmd[i].info);
	}

	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
