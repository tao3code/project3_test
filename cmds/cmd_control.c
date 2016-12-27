#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>
#include <stdcmd.h>
#include <serial.h>

static struct interface_info *info;

static int set_detect = 0;
static int set_m12 = 0;
static int set_serial = 0;

static struct func_arg set_args[] = {
	{.name = "serial",
	 .var = &set_serial,
	 .type = "%d"},
	{.name = "detect",
	 .var = &set_detect,
	 .type = "%d",},
	{.name = "m12",
	 .var = &set_m12,
	 .type = "%d"},
	{0},
};

static int detect_interface_board(void)
{
	test_device(&info->dev);
	if (!info->dev.id) {
		log_info("Can't detect device for control!\n");
		return -1;
	}
	return 0;
}

static int do_set(struct func_arg *args)
{
	if (set_serial) {
		if (serial_init())
			return -1;
	} else {
		log_info("Serial close, skip...\n");
		serial_close();
		return 0;
	}

	if (set_detect) {
		set_detect = 0;
		if (detect_interface_board())
			return -1;
	}

	meg12v_on(set_m12);

	return 0;
}

static int update_vol = 1;
static int update_air = 1;
static int update_gyr = 0;
static int update_m12 = 1;
static int update_eng = 1;

static struct func_arg update_args[] = {
	{.name = "vol",
	 .var = &update_vol,
	 .type = "%d",},
	{.name = "air",
	 .var = &update_air,
	 .type = "%d",},
	{.name = "gyr",
	 .var = &update_gyr,
	 .type = "%d",},
	{.name = "m12",
	 .var = &update_m12,
	 .type = "%d",},
	{.name = "engine",
	 .var = &update_eng,
	 .type = "%d",},
	{0},
};

static int do_update(struct func_arg *args)
{
	if (update_gyr) {
		update_gyr = 0;
		update_gyroscope();
	}

	if (update_vol)
		update_voltage();
	if (update_air)
		update_presure();
	if (update_m12)
		update_meg12v();
	if (update_eng)
		update_engine();
	return 0;
}

static pthread_t control_thread = 0;

static int thread_step = 1000000;
static int thread_autoair = 0;
static int thread_autoupdate = 1;
static int thread_display = 1;
static int thread_motion = 1;

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
	wprintw(ctrl_win, "air(auto:%d,eng:%d): %u\n",
		thread_autoair, info->engine, info->air);
	wprintw(ctrl_win, "gyr: %hd %hd %hd\n", info->gx, info->gy, info->gz);
	wprintw(ctrl_win, "thm: %hd\n", info->thermal);
	wprintw(ctrl_win, "acc: %hd %hd %hd\n", info->ax, info->ay, info->az);
	wprintw(ctrl_win, "m12: %d\n", info->m12v);
	lock_scr();
	wrefresh(ctrl_win);
	unlock_scr();
}

static int hvol_count = 0;
static int lvol_count = 0;

static int run_engine_onece(void)
{
	int can_run = 1;
	int need_run = 1;
	int run_count;

	if (lvol_count > 10) {
		log_info("voltage always too low!\n");
		lvol_count = 0;
		return -1;
	}

	if (hvol_count > 10) {
		log_info("voltage always too high!\n");
		hvol_count = 0;
		return -1;
	}

	if (info->vol < VOLTAGE_LOW) {
		lvol_count++;
		return 0;
	}
	lvol_count = 0;

	if (info->vol > VOLTAGE_OVE) {
		hvol_count++;
		return 0;
	}
	hvol_count = 0;

	can_run &= thread_autoupdate;
	can_run &= update_vol;
	can_run &= update_air;
	can_run &= update_eng;

	if (!can_run) {
		log_info("%s, not ready to run!\n", __FUNCTION__);
		return -1;
	}

	need_run &= info->air < AIR_THRESHOLD_H;
	need_run &= !info->engine;

	if (!need_run)
		return 0;

	run_count = (AIR_THRESHOLD_H - info->air) * 255 / AIR_THRESHOLD_H;
	if (run_count < 24)
		run_count = 24;

	log_info("engine_on(%d)\n", run_count);
	return engine_on(run_count);
}

static void *control_thread_func(void *arg)
{
	log_info("%s start\n", __FUNCTION__);

	while (!strcmp(thread_state, "on")) {
		if (thread_autoupdate)
			do_update(update_args);
		if (thread_display)
			refresh_window();
		if (thread_autoair)
			if (run_engine_onece())
				thread_autoair = 0;
		if (thread_motion) {
			if (just_run_cmd("motion update,id=all,display=1"))
				thread_motion = 0;
		}

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
	{.name = "autoair",
	 .var = &thread_autoair,
	 .type = "%d"},
	{.name = "autoupdate",
	 .var = &thread_autoupdate,
	 .type = "%d"},
	{.name = "display",
	 .var = &thread_display,
	 .type = "%d"},
	{.name = "motion",
	 .var = &thread_motion,
	 .type = "%d"},
	{0},
};

static int do_thread(struct func_arg *args)
{
	if (!strcmp(thread_state, "on")) {
		if (control_thread)
			return 0;
		return pthread_create(&control_thread, 0,
				      control_thread_func, NULL);
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
	{.name = "set",
	 .func = do_set,
	 .args = set_args},
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
