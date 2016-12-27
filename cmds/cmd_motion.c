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
		return info[id].mea.count + ENCODER_OFFSET;
	if (!strcmp(set_enc, "min"))
		return ENCODER_OFFSET;
	return atoi(enc);
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

	if (set_meg != 0) {
		if (!if_info->m12v) {
			log_info("%s, megnet power is not on, stop!\n",
				 __FUNCTION__);
			ret = -1;
			goto set_end;
		}
		if (run_cmd("wait air 2")) {
			log_info("%s, pressure is low, stop!\n", __FUNCTION__);
			ret = -1;
			goto set_end;
		}

		ret = megnet(&info[set_id], set_meg);
	}

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
			wprintw(motion_win, "%c[%d]: %hu,%hd,%x",
				masked, i, info[i].var.len, info[i].var.speed,
				info[i].var.port);

		if (i & 0x1)
			waddch(motion_win, '\n');
		else
			waddch(motion_win, '\t');
	}
	lock_scr();
	wrefresh(motion_win);
	unlock_scr();
}

static int do_update(struct func_arg *args)
{
	int i;

	for (i = 0; i < count; i++) {
		if ((1 << i) & update_mask)
			continue;
		if (!info[i].dev.id)
			continue;
		if (update_cylinder_state(&info[i]))
			update_mask |= 1 << i;
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
