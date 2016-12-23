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

static void update_control_state(void)
{
	if (!info->dev.id)
		return; 
        update_voltage();
        update_presure();
        update_gyroscope();
        update_meg12v();
}

static void update_control_window(void)
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

static pthread_t ctrlshow_thread = 0;
static int ctrlshow_on = 0;

static int step = 1000000;

static int check_step(void)
{
	if (step < 1000 || step > 1000000)
		return -1;
	return 0;
}

static void *ctrlshow_thread_func(void *arg)
{
	log_info("%s start\n", __FUNCTION__);

	while (ctrlshow_on) {
		update_control_state();
		update_control_window();
		usleep(step);
	}

	log_info("%s stop\n", __FUNCTION__);

	return 0;
}

static char state[MAX_VAL_LEN] = "off";

static int check_state(void)
{
        if (!strcmp(state, "on"))
                return 0;
        if (!strcmp(state, "off"))
                return 0;
        return -1;
}

static struct func_arg thread_args[] = {
	{.name = "state", .var = state, .type = "%s", .check = check_state},
	{.name = "step", .var = &step, .type = "%d", .check = check_step},
	{0},
};

static int thread(struct func_arg *args)
{
	if (!strcmp(state, "on")) {
		if (ctrlshow_on)
			return 0;
		ctrlshow_on = 1;
		return pthread_create(&ctrlshow_thread, 0,
				      ctrlshow_thread_func, NULL);
	}

	if (!strcmp(state, "off")) {
		if (!ctrlshow_on)
			return 0;
		ctrlshow_on = 0;
		pthread_join(ctrlshow_thread, 0);
		return 0;
	}

	return -1;
}

static struct cmd_func control_funcs[] = {
	{.name = "thread", .func = thread, .args = thread_args},
	{0},
};

static int do_control(int argc, char *argv[])
{
        int i;
        int ret;

        for (i = 1; i < argc; i++) {
                ret = cmd_run_funcs(argv[i], control_funcs);
                if (ret)
                        return -1;
        }
        return 0;
}

static struct input_cmd cmd = {
	.str = "control",
	.func = do_control,
	.info = "Use 'control thread,start=on'",
};

static int reg_cmd(void)
{
	info = get_interface_info();
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
