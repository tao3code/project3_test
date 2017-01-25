#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>
#include <stdcmd.h>
#include <serial.h>

static struct interface_info *ctrl;

unsigned long sys_ms;
static struct timeval sys_tv;

static int detect_interface_board(void)
{
	test_device(&ctrl->dev);
	return 0;
}

static int update_gyr = 0;
static int update_m12 = 1;
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
	int i;
	char cmd[32];

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
		for (i = 0; i < NUM_CYLINDERS; i++) {
			memset(cmd, 0, sizeof(cmd));
			sprintf(cmd, "motion set,id=%d,detect=1", i);
			just_run_cmd(cmd);
		}
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
static int thread_autoair = 1;
static unsigned thread_ah = AIR_THRESHOLD_H;
static int thread_autoupdate = 1;
static int thread_display = 1;
static int thread_motion = 1;

static int check_step(void *var)
{
	if (thread_step < 1000 || thread_step > 1000000)
		return -1;
	return 0;
}

static int check_ah(void *var)
{
	if (thread_ah > 100)
		thread_ah = AIR_THRESHOLD_H;
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

	struct tm *sys_time;

	sys_time = localtime(&sys_tv.tv_sec);
	werase(ctrl_win);
	wprintw(ctrl_win, "%d:%d:%d  %lu\n", sys_time->tm_hour,
		sys_time->tm_min, sys_time->tm_sec, sys_ms);

	if (ctrl->dev.state) {
		wprintw(ctrl_win, "Interface board does not exise!\n");
		lock_scr();
		wrefresh(ctrl_win);
		unlock_scr();

		return;
	}

	wprintw(ctrl_win, "vol: %u\n", ctrl->vol);
	wprintw(ctrl_win, "air(auto:%d,eng:%d): %u\n",
		thread_autoair, ctrl->engine, ctrl->air);
	wprintw(ctrl_win, "gyr: %hd %hd %hd\n", ctrl->gx, ctrl->gy, ctrl->gz);
	wprintw(ctrl_win, "thm: %hd\n", ctrl->thermal);
	wprintw(ctrl_win, "acc: %hd %hd %hd\n", ctrl->ax, ctrl->ay, ctrl->az);
	wprintw(ctrl_win, "m12: %d\n", ctrl->m12v);
	lock_scr();
	wrefresh(ctrl_win);
	unlock_scr();
}

static unsigned long hvol_expire = ~0x0;
static unsigned long lvol_expire = ~0x0;
#define VOL_EXPIRE	5000

static int run_engine_onece(void)
{
	int can_run = 1;
	int need_run = 1;
	int run_count;

	if (sys_ms > lvol_expire) {
		log_info("voltage always too low!\n");
		lvol_expire = ~0x0;
		return -1;
	}

	if (sys_ms > hvol_expire) {
		log_info("voltage always too high!\n");
		hvol_expire = ~0x0;
		return -1;
	}

	if (ctrl->vol < VOLTAGE_LOW) {
		if (lvol_expire == ~0x0)
			lvol_expire = sys_ms + VOL_EXPIRE;
		return 0;
	}
	lvol_expire = ~0x0;

	if (ctrl->vol > VOLTAGE_OVE) {
		if (hvol_expire == ~0x0)
			hvol_expire = sys_ms + VOL_EXPIRE;
		return 0;
	}
	hvol_expire = ~0x0;

	can_run &= thread_autoupdate;
	can_run &= !serial_err;
	can_run &= !dev_err;

	if (!can_run) {
		log_info("%s, not ready to run!\n", __FUNCTION__);
		return -1;
	}

	need_run &= ctrl->air < thread_ah;
	need_run &= !ctrl->engine;

	if (!need_run)
		return 0;

	run_count = (thread_ah - ctrl->air) * 255 / thread_ah;
	if (run_count < 24)
		run_count = 24;

	if (megs_on)
		return 0;
	ctrl->engine = 1;

	log_info("engine_on(%d)\n", run_count);
	if (engine_on(run_count)) {
		ctrl->engine = 0;
		return -1;
	}

	return 0;
}

static void *control_thread_func(void *arg)
{
	int ret;

	log_info("%s start\n", __FUNCTION__);

	while (!strcmp(thread_state, "on")) {
		gettimeofday(&sys_tv, NULL);
		sys_ms = sys_tv.tv_sec * 1000 + sys_tv.tv_usec / 1000;
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
			thread_step = 1000;

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
	{.name = "ah",
	 .var = &thread_ah,
	 .type = "%u",
	 .check = check_ah},
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
	ctrl = get_interface_info();
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
