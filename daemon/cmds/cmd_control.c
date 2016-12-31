#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>
#include <stdcmd.h>
#include <serial.h>

static struct interface_info *info;

static int detect_interface_board(void)
{
	test_device(&info->dev);
	if (!info->dev.id) {
		log_info("Can't detect device for control!\n");
		return -1;
	}
	return 0;
}

static int update_gyr = 0;
static int update_m12 = 0;
static char update_serial[MAX_VAL_LEN] = "/dev/ttyUSB0";

static struct func_arg update_args[] = {
	{.name = "gyr",
	 .var = &update_gyr,
	 .type = "%d",},
	{.name = "m12",
	 .var = &update_m12,
	 .type = "%d",},
	{.name = "serial",
	 .var = update_serial,
	 .type = "%s",},
	{0},
};

static int serial_err = -1;
static int dev_err = -1;

static int do_update(struct func_arg *args)
{
	struct stat s;
	int ret = 0;

	ret = stat(update_serial, &s);
	if (ret) {
		if (!serial_err) {
			log_info("%s not exist!\n", update_serial);
			serial_close();
			serial_err = ret;
		}
		return -1;
	}

	if (serial_err) {
		serial_err = serial_init(update_serial);
		if (serial_err)
			return -1;
		dev_err = detect_interface_board();
		just_run_cmd("motion set,detect=1");
		just_run_cmd("motion update,mask=0");
	}

	if (dev_err)
		return -1;

	if (update_gyr) {
		ret |= update_gyroscope();
		update_gyr = 0;
	}

	if (update_m12) {
		ret |= meg12v_on(update_m12);
		update_m12 = 0;
	}

	ret |= update_presure();
	ret |= update_voltage();
	ret |= update_meg12v();
	ret |= update_engine();

	return ret;
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
	can_run &= !serial_err;
	can_run &= !dev_err;

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
	int ret;

	log_info("%s start\n", __FUNCTION__);

	while (!strcmp(thread_state, "on")) {
		ret = 0;
		if (thread_autoupdate)
			ret = do_update(update_args);
		if (thread_display)
			refresh_window();
		if (thread_autoair)
			if (run_engine_onece()) {
				ret = -1;
				thread_autoair = 0;
			}
		if (thread_motion)
			if (just_run_cmd("motion update,display=1")) {
				ret = -1;
				thread_motion = 0;
			}

		if (ret)
			thread_step = 1000000;
		else
			thread_step = 100000;

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
	do_thread(thread_args);
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