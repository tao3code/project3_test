#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>

static struct interface_info *info;

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

static int wait_air(int argc, char *argv[])
{
	int time_out;
	time_t start, end;

	if (argc != 2) {
		log_err();
		return -1;
	}

	time_out = atoi(argv[1]);
	if (time_out <= 0 || time_out > 30) {
		log_info("%s, time_out:%d out of range 1~30\n",
			__FUNCTION__, time_out);
		return -1;
	}
 
	start = time(NULL);
        end = time(NULL);
        while ((end - start) < time_out) {
		update_presure();
                if (info->air > AIR_THRESHOLD_OFF) {
                        return 0;
                }
                usleep(500);
                end = time(NULL);
        }

	log_info("%s, time out:%d\n", __FUNCTION__, time_out);
        return -1;
}

static int wait_stand(int argc, char *argv[])
{
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
	{
        .str = "air",
        .func = wait_air,
	.info = "example: wait air 10"
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

	info = get_interface_info();
	
	for (i = 0; i < NSUB_CMD; i ++) {
		if (! sub_cmd[i].info)
			continue;
		len += sprintf(&help_info[len], " %s\n", sub_cmd[i].info);
	}

	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
