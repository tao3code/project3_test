#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>

static struct cylinder_info *motion_target;
static int ntarget;
static struct cylinder_info *motion_current;

static int target_set(int argc, char *argv[])
{
	int i;
	unsigned short len;
	struct cylinder_info *info;	

	if (argc != 3 && argc != 4) {
		log_err();
		return -1;
	}

	i = atoi(argv[1]);
        if(i < 0 || i >= ntarget) {
                log_err();
                return -1;
        }

	info = &motion_target[i];
	
	len = atoi(argv[2]);
	if (len < ENCODER_OFFSET)
		len = ENCODER_OFFSET;
	if (len > info->fix.count)
		len = info->fix.count;

	info->len = len;
	
	if (argc == 3) {
		info->force = 0;
		return 0;
	}

	if (!strcmp(argv[3], "push")) {
		info->force = '+';
		return 0;
	}
	
	if (!strcmp(argv[3], "pull")) {
		info->force = '-';
		return 0;
	}

	log_err();
	return -1;
}

static int target_sync(int argc, char *argv[])
{
	int i;

	if (argc != 2) {
		log_err();
		return -1;
	}

	i = atoi(argv[1]);
	if(i < 0 || i >= ntarget) {
		log_err();
		return -1;
	}

	update_cylinder_len(&motion_current[i]);		
	memcpy(&motion_target[i], &motion_current[i], sizeof(struct cylinder_info));	

	return 0;
}

static struct input_cmd sub_cmd[] = {
	{
        .str = "sync",
        .func = target_sync,
	.info = "example: 'target sync 0'"
	},
	{
        .str = "set",
        .func = target_set,
	.info = "example: 'target set 0 128', or 'target set 0 128 push/pull'"
	},
};

#define NSUB_CMD	(sizeof(sub_cmd)/sizeof(struct input_cmd))

static int do_target(int argc, char *argv[])
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
	.str = "target",
	.func = do_target,
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

	motion_current = get_motion_info(&ntarget);

	motion_target = malloc(sizeof(struct cylinder_info) * ntarget);
	if (!motion_target) {
		log_system_err(__FUNCTION__);
		return errno;
	}

	memcpy(motion_target, motion_current, sizeof(struct cylinder_info) * ntarget);	 

	register_cmd(&cmd);
	return 0;
}

static void exit_cmd(void)
{
	if(motion_target)
		free(motion_target);
} 

init_func(reg_cmd);
exit_func(exit_cmd);
