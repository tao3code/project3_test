#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>
#include <stdcmd.h>

struct interface_info *ctrl;
static struct cylinder_info *info;

static int update_display = 0;

static struct func_arg update_args[] = {
	{.name = "display",
	 .var = &update_display,
	 .type = "%d"},
	{0},
};

static void refresh_window(void)
{
	int i;

	werase(motion_win);
	for (i = 0; i < NUM_CYLINDERS; i++) {
		switch (info[i].dev.state) {
		case 0:
			wprintw(motion_win, "[%d]: %6hu,%hd,%x",
				i, info[i].enc.len, info[i].speed,
				info[i].port);
			break;
		case MASK_NODEV:
		case MASK_NODEV | MASK_ERR:
			wprintw(motion_win, "[%d]: none", i);
			break;
		case MASK_ERR:
			wprintw(motion_win, "[%d]: offline", i);
			break;
		default:
			wprintw(motion_win, "[%d]: err", i);
		}

		if (i & 0x1)
			waddch(motion_win, '\n');
		else
			waddch(motion_win, '\t');
	}
	wprintw(motion_win, "\nloadng: %d  megs: %x\n", air_loading, megs_on);
	lock_scr();
	wrefresh(motion_win);
	unlock_scr();
}

static int do_update(struct func_arg *args)
{
	int i;

	air_loading = 0;
	megs_on = 0;
	for (i = 0; i < NUM_CYLINDERS; i++) {
		if (info[i].dev.state)
			continue;
		if (update_cylinder_state(&info[i]))
			info[i].dev.state |= MASK_ERR;
		air_loading +=
		    info[i].speed * info[i].meg_dir * info[i].fix.area;
		if (!(info[i].port & (0x8 | 0x4)))
			megs_on &= ~(0x1 << i);
	}

	if (update_display) {
		update_display = 0;
		refresh_window();
	}

	return 0;
}

volatile int air_loading = 0;
volatile unsigned megs_on = 0;

static int set_detect = 0;
static int set_id = -1;
static int set_meg = 0;
static char set_enc[MAX_VAL_LEN];

static struct func_arg set_args[] = {
	{.name = "detect",
	 .var = &set_detect,
	 .type = "%d"},
	{.name = "id",
	 .var = &set_id,
	 .type = "%d"},
	{.name = "meg",
	 .var = &set_meg,
	 .type = "%d"},
	{.name = "enc",
	 .var = &set_enc,
	 .type = "%s"},
	{0},
};

static int enc_val(int id, char *enc)
{
	if (!strcmp(set_enc, "max"))
		return info[id].mea.c + ENCODER_OFFSET;
	if (!strcmp(set_enc, "min"))
		return ENCODER_OFFSET;
	return atoi(enc);
}

#define AIR_EXPIER		2000
#define VOL_EXPIER		2000
#define LOAD_EXPIER		2000

#define MAX_LOADING		300

static unsigned long air_expire = ~0x0;
static unsigned long vol_expire = ~0x0;
static unsigned long load_expire = ~0x0;

static int set_meg_once(int id, int val)
{
	unsigned char air;
	int ret;

	if (!val)
		return 0;

	air = (val > 0) ? info[id].mea.pa : info[id].mea.na;

	if (!ctrl->m12v) {
		log_info("%s, megnet power is not on, stop!\n", __FUNCTION__);
		goto err_meg;
	}

	while (1) {
		ret = do_update(update_args);
		if (ret) {
			log_info("%s, update cylinders err, stop!\n",
				 __FUNCTION__);
			goto err_meg;
		}

		if (air_loading < MAX_LOADING)
			break;
		if (sys_ms > load_expire) {
			log_info("%s, wait engine error, stop!\n",
				 __FUNCTION__);
			goto err_meg;
		}

		if (load_expire == ~0x0)
			load_expire = sys_ms + LOAD_EXPIER;
		usleep(50000);
	}

	load_expire = ~0x0;

	ret = update_presure();
	if (ret) {
		log_info("%s, update presure err, stop!\n", __FUNCTION__);
		goto err_meg;
	}
	ret = update_voltage();
	if (ret) {
		log_info("%s, update voltage err, stop!\n", __FUNCTION__);
		goto err_meg;
	}

	if (ctrl->air < air && air)
		air_expire = sys_ms + AIR_EXPIER;
	if (ctrl->vol < VOLTAGE_LOW)
		vol_expire = sys_ms + VOL_EXPIER;

	while (ctrl->air < air || ctrl->vol < VOLTAGE_LOW) {
		if (sys_ms > air_expire) {
			log_info("%s, wait air error, stop!\n", __FUNCTION__);
			goto err_meg;
		}
		if (sys_ms > vol_expire) {
			log_info("%s, wait voltage error, stop!\n",
				 __FUNCTION__);
			goto err_meg;
		}
		usleep(50000);
	}
	air_expire = ~0x0;
	vol_expire = ~0x0;

	while (ctrl->engine)
		usleep(50000);

	megs_on |= 0x1 << id;

	return megnet(&info[id], val);
 err_meg:
	air_expire = ~0x0;
	vol_expire = ~0x0;
	load_expire = ~0x0;
	return -1;
}

static int do_set(struct func_arg *args)
{
	int ret = 0;
	int val;

	if (set_id < 0 && set_id >= NUM_CYLINDERS) {
		ret = -1;
		goto set_end;
	}

	if (set_detect)
		test_device(&info[set_id].dev);

	if (strlen(set_enc)) {
		val = enc_val(set_id, set_enc);
		ret = set_encoder(&info[set_id], val);
		if (ret)
			goto set_end;
	}

	if (set_meg != 0) {
		ret = set_meg_once(set_id, set_meg);
		if (ret)
			goto set_end;
	}

 set_end:
	set_meg = 0;
	memset(set_enc, 0, sizeof(set_enc));
	set_id = -1;
	set_detect = 0;
	return ret;
}

static struct cmd_func motion_funcs[] = {
	{.name = "update",
	 .func = do_update,
	 .args = update_args},
	{.name = "set",
	 .func = do_set,
	 .args = set_args},
	{0},
};

static int cmd_motion(int argc, char *argv[])
{
	int i;
	int ret;

	for (i = 1; i < argc; i++) {
		ret = stdcmd_run_funcs(argv[i], motion_funcs);
		if (ret)
			return -1;
	}
	return 0;
}

static int help_motion(char *buf, int argc, char *argv[])
{
	return stdcmd_help(buf, motion_funcs, argc - 1, &argv[1]);
}

static struct input_cmd cmd = {
	.str = "motion",
	.func = cmd_motion,
	.help = help_motion,
};

static int reg_cmd(void)
{
	ctrl = get_interface_info();
	info = get_motion_info();
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
