#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>
#include <stdcmd.h>

static struct cylinder_info *info;
static int count;

static char update_id[MAX_VAL_LEN] = "all";
static int update_detect = 0;
static int update_display = 0;

static int check_update_id(void *var)
{
	int id;

	if (!strcmp(update_id, "all"))
		return 0;

	id = atoi(var);
	if (id < 0 || id >= count)
		return -1;
	return 0;
}

static struct func_arg update_args[] = {
	{.name = "id",
	 .var = &update_id,
	 .type = "%s",
	 .check = check_update_id},
	{.name = "detect",
	 .var = &update_detect,
	 .type = "%d"},
	{.name = "display",
	 .var = &update_display,
	 .type = "%d"},
	{0},
};

static void refresh_window(void)
{
	int i;

	werase(motion_win);
	for (i = 0; i < count; i++) {
		if (!info[i].dev.id)
			wprintw(motion_win, "len[%d]: NULL", i);
		else
			wprintw(motion_win, "len[%d]: %hu,%hd,%x",
				i, info[i].var.len, info[i].var.speed,
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
	
	if (update_detect) {
		update_detect = 0;
		for (i = 0; i < count; i++) {
			test_device(&info[i].dev);
                	if (!info[i].dev.id)
				return -1;
		}
	}
		
	if (!strcmp(update_id, "all")) {
		for (i = 0; i < count; i++) {
			if (!info[i].dev.id)
				continue;
			update_cylinder_state(&info[i]);
		}
	} else {
	i = atoi(update_id);
	update_cylinder_state(&info[i]);
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
	info = get_motion_info(&count);
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
