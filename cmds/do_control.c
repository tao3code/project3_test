#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>
#include <stdcmd.h>

static const struct interface_info *info;

static int update_auto = 1;
static int update_vol = 1;
static int update_air = 1;
static int update_gyr = 1;
static int update_meg = 1;

static struct func_arg update_args[] = {
	{.name = "auto",
	 .var = &update_auto,
	 .type = "%d",},
	{.name = "vol",
	 .var = &update_vol,
	 .type = "%d",},
	{.name = "air",
	 .var = &update_air,
	 .type = "%d",},
	{.name = "gyr",
	 .var = &update_gyr,
	 .type = "%d",},
	{.name = "meg",
	 .var = &update_meg,
	 .type = "%d",},
	{0},
};

static int do_update(struct func_arg *args)
{
	if (update_vol)
		update_voltage();
	if (update_air)
		update_presure();
	if (update_gyr)
		update_gyroscope();
	if (update_meg)
		update_meg12v();
	return 0;
}

static void refresh_window(void)
{
	time_t uts;
	struct tm *t;

	uts = time(NULL);
	t = localtime(&uts);

	werase(ctrl_win);
	wprintw(ctrl_win, "%d:%d:%d\n", t->tm_hour, t->tm_min, t->tm_sec);

	if (!info->dev.id) {
		wprintw(ctrl_win, "Interface board does not exise!\n");
		lock_scr();
		wrefresh(ctrl_win);
		unlock_scr();

		return;
	}

	wprintw(ctrl_win, "vol: %u\n", info->vol);
	wprintw(ctrl_win, "air(%s): %u\n", air_state, info->air);
	wprintw(ctrl_win, "gyr: %hd %hd %hd\n", info->gx, info->gy, info->gz);
	wprintw(ctrl_win, "thm: %hd\n", info->thermal);
	wprintw(ctrl_win, "acc: %hd %hd %hd\n", info->ax, info->ay, info->az);
	wprintw(ctrl_win, "m12: %d\n", info->m12v);
	lock_scr();
	wrefresh(ctrl_win);
	unlock_scr();
}

static pthread_t control_thread = 0;

static int thread_step = 1000000;

static int check_step(void *var)
{
	if (thread_step < 1000 || thread_step > 1000000)
		return -1;
	return 0;
}

static char thread_state[MAX_VAL_LEN] = "on";

static int check_state(void *var)
{
	if (!strcmp(thread_state, "on"))
		return 0;
	if (!strcmp(thread_state, "off"))
		return 0;
	return -1;
}

static void *ctrlshow_thread_func(void *arg)
{
	log_info("%s start\n", __FUNCTION__);

	while (!strcmp(thread_state, "on")) {
		if (update_auto)
			do_update(update_args);
		refresh_window();
		usleep(thread_step);
	}

	log_info("%s stop\n", __FUNCTION__);

	return 0;
}

static struct func_arg thread_args[] = {
	{.name = "state",
	 .var = thread_state,
	 .type = "%s",
	 .check = check_state},
	{.name = "step",
	 .var = &thread_step,
	 .type = "%d",
	 .check = check_step},
	{0},
};

static int do_thread(struct func_arg *args)
{
	if (!strcmp(thread_state, "on")) {
		if (control_thread)
			return 0;
		return pthread_create(&control_thread, 0,
				      ctrlshow_thread_func, NULL);
	}

	if (!strcmp(thread_state, "off")) {
		if (!control_thread)
			return 0;
		pthread_join(control_thread, 0);
		control_thread = 0;
		return 0;
	}

	return -1;
}

static struct cmd_func control_funcs[] = {
	{.name = "thread",
	 .func = do_thread,
	 .args = thread_args},
	{.name = "update",
	 .func = do_update,
	 .args = update_args},
	{0},
};

static int cmd_control(int argc, char *argv[])
{
	int i;
	int ret;

	for (i = 1; i < argc; i++) {
		ret = stdcmd_run_funcs(argv[i], control_funcs);
		if (ret)
			return -1;
	}
	return 0;
}

static int help_control(char *buf, int argc, char *argv[])
{
	return stdcmd_help(buf, control_funcs, argc - 1, &argv[1]);
}

static struct input_cmd cmd = {
	.str = "control",
	.func = cmd_control,
	.help = help_control,
};

static int reg_cmd(void)
{
	info = get_interface_info();
	register_cmd(&cmd);
	return 0;
}

static void clean_cmd(void)
{
	if (!strcmp(thread_state, "on")) {
		memcpy(thread_state, "off", sizeof("off"));
		pthread_join(control_thread, 0);
		control_thread = 0;
	}
}

init_func(reg_cmd);
exit_func(clean_cmd);
