#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>

static void update_motion_window(void)
{
	const struct cylinder_info *info;
	int count;
	int i;

	info = get_motion_info(&count);
	werase(motion_win);
	for (i = 0; i < count; i++) {
		if (!info[i].dev.id)
			wprintw(motion_win, "len[%d]: NULL", i);
		else
			wprintw(motion_win, "len[%d]: %hu", i, info[i].len);

		if (i & 0x1)
			waddch(motion_win, '\n');
		else
			waddch(motion_win, '\t');
	}
	lock_scr();
	wrefresh(motion_win);
	unlock_scr();
}

static pthread_t motionshow_thread = 0;
static int motionshow_on = 0;

static void *motionshow_thread_func(void *arg)
{
	int *on = arg;

	log_info("%s start\n", __FUNCTION__);

	while (*on) {
		usleep(100);
		update_motion_state();
		update_motion_window();
	}

	log_info("%s stop\n", __FUNCTION__);
	return 0;
}

static int do_motionshow(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on")) {
		if (motionshow_on)
			return 0;
		motionshow_on = 1;
		return pthread_create(&motionshow_thread, 0,
				      motionshow_thread_func, &motionshow_on);
	}

	if (!strcmp(argv[1], "off")) {
		if (!motionshow_on)
			return 0;
		motionshow_on = 0;
		pthread_join(motionshow_thread, 0);
		return 0;
	}

	return -1;
}

static struct input_cmd cmd = {
	.str = "motionshow",
	.func = do_motionshow,
	.info = "Use 'motionshow on' or 'motionshow off'",
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

static void clean_cmd(void)
{
	if (motionshow_on) {
		motionshow_on = 0;
		pthread_join(motionshow_thread, 0);
	}
}

init_func(reg_cmd);
exit_func(clean_cmd);
