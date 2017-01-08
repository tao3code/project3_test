#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>
#include <stdcmd.h>

struct interface_info *if_info;
static struct cylinder_info *info;
static int count;

volatile int air_loading = 0;
volatile int megs_on = 0;

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

#define MAX_LOADING		800

static unsigned long air_expire = ~0x0;
static unsigned long vol_expire = ~0x0;
static unsigned long load_expire = ~0x0;

static int set_meg_once(struct cylinder_info *cy, int val)
{
	unsigned char air;

	if (!val)
		return 0;

	air = (val > 0) ? cy->mea.pa : cy->mea.na;

	if (!if_info->m12v) {
		log_info("%s, megnet power is not on, stop!\n", __FUNCTION__);
		goto err_meg;
	}

	if (if_info->air < air)
		air_expire = sys_ms + AIR_EXPIER;
	if (if_info->vol < VOLTAGE_LOW)
		vol_expire = sys_ms + VOL_EXPIER;
	if (air_loading > MAX_LOADING)
		load_expire = sys_ms + LOAD_EXPIER;

	while (if_info->air < air || if_info->vol < VOLTAGE_LOW
	       || air_loading > MAX_LOADING) {
		if (sys_ms > air_expire) {
			log_info("%s, wait air error, stop!\n", __FUNCTION__);
			goto err_meg;
		}
		if (sys_ms > vol_expire) {
			log_info("%s, wait voltage error, stop!\n",
				 __FUNCTION__);
			goto err_meg;
		}
		if (sys_ms > load_expire) {
			log_info("%s, wait engine error, stop!\n",
				 __FUNCTION__);
			goto err_meg;
		}
		usleep(200);
	}
	air_expire = ~0x0;
	vol_expire = ~0x0;
	load_expire = ~0x0;
	return megnet(cy, val);
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
	int i;

	if (set_detect) {
		for (i = 0; i < count; i++) {
			test_device(&info[i].dev);
			if (!info[i].dev.id)
				ret = -1;
		}
		if (ret)
			goto set_end;
	}

	if (set_id < 0 && set_id >= count) {
		ret = -1;
		goto set_end;
	}

	if (strlen(set_enc)) {
		val = enc_val(set_id, set_enc);
		ret = set_encoder(&info[set_id], val);
		if (ret)
			goto set_end;
	}

	if (set_meg != 0)
		ret = set_meg_once(&info[set_id], set_meg);

 set_end:
	set_meg = 0;
	memset(set_enc, 0, sizeof(set_enc));
	set_id = -1;
	set_detect = 0;
	return ret;
}

static unsigned short update_mask = 0;
static int update_display = 0;

static struct func_arg update_args[] = {
	{.name = "mask",
	 .var = &update_mask,
	 .type = "%hx"},
	{.name = "display",
	 .var = &update_display,
	 .type = "%d"},
	{0},
};

static void refresh_window(void)
{
	int i;
	char masked;

	werase(motion_win);
	for (i = 0; i < count; i++) {
		masked = update_mask & (1 << i) ? 'm' : ' ';
		if (!info[i].dev.id)
			wprintw(motion_win, "*[%d]: NULL", i);
		else
			wprintw(motion_win, "%c[%d]: %6hu,%hd,%x",
				masked, i, info[i].var.len, info[i].var.speed,
				info[i].var.port);

		if (i & 0x1)
			waddch(motion_win, '\n');
		else
			waddch(motion_win, '\t');
	}
	wprintw(motion_win, "\nloadng: %d  megs: %d\n", air_loading, megs_on);
	lock_scr();
	wrefresh(motion_win);
	unlock_scr();
}

static int do_update(struct func_arg *args)
{
	int i;

	air_loading = 0;
	megs_on = 0;
	for (i = 0; i < count; i++) {
		if ((1 << i) & update_mask)
			continue;
		if (!info[i].dev.id)
			continue;
		if (update_cylinder_state(&info[i]))
			update_mask |= 1 << i;
		air_loading +=
		    info[i].var.speed * info[i].meg_dir * info[i].fix.area;
		if (info[i].var.port & (0x8 | 0x4))
			megs_on++;
	}

	if (update_display) {
		update_display = 0;
		refresh_window();
	}

	return 0;
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
	if_info = get_interface_info();
	info = get_motion_info(&count);
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
