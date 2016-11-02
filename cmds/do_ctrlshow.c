#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>

static pthread_t ctrlshow_thread = 0;
static int ctrlshow_on = 0;

static void *ctrlshow_thread_func(void *arg)
{
	int *on = arg;

	log_info("%s start\n", __FUNCTION__);

	while (*on) {
		usleep(100);
		update_control_state();
		update_control_window();
	}

	log_info("%s stop\n", __FUNCTION__);

	return 0;
}

static int do_ctrlshow(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on")) {
		if (ctrlshow_on)
			return 0;
		ctrlshow_on = 1;
		return pthread_create(&ctrlshow_thread, 0,
				      ctrlshow_thread_func, &ctrlshow_on);
	}

	if (!strcmp(argv[1], "off")) {
		if (!ctrlshow_on)
			return 0;
		ctrlshow_on = 0;
		pthread_join(ctrlshow_thread, 0);
		return 0;
	}

	return -1;
}

static struct input_cmd cmd = {
	.str = "ctrlshow",
	.func = do_ctrlshow,
	.info = "Use 'ctrlshow on' or 'ctrlshow off'",
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

static void clean_cmd(void)
{
	if (ctrlshow_on) {
		ctrlshow_on = 0;
		pthread_join(ctrlshow_thread, 0);
	}
}

init_func(reg_cmd);
exit_func(clean_cmd);
