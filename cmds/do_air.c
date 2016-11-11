#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>

static pthread_t air_thread = 0;
static int air_on = 0;

static void *air_thread_func(void *arg)
{
	const struct interface_info *info;
	char air;
	int need_run;
	int run_count;
	int wait_count;

	info = get_interface_info();
	log_info("%s start\n", __FUNCTION__);

	while (air_on) {
		update_presure();
		if (info->vol < VOLTAGE_LOW) {
			update_voltage();
			if (info->vol < VOLTAGE_LOW) {
				log_info("low voltage!\n");
				break;
			}
		}
		air = info->air;
		if (air < AIR_THRESHOLD_L)
			need_run = 1;
		if (air > AIR_THRESHOLD_H)
			need_run = 0;
		if (!need_run) {
			usleep(500);
			continue;
		}

		run_count =
		    (AIR_THRESHOLD_H - air) * 255 / AIR_THRESHOLD_H;
		if (run_count < 24)
			run_count = 24;
		engine_on(run_count);
		log_info("engine_on(%d)\n", run_count);
		wait_count = run_count * 4 / 255;
		if (wait_count < 1)
			wait_count = 1;
		sleep(wait_count);
		update_presure();
	}

	air_on = 0;
	log_info("%s stop\n", __FUNCTION__);

	return 0;
}

static int do_air(int argc, char *argv[])
{
	if (argc != 2)
		return -1;
	if (!strcmp(argv[1], "on")) {
		if (air_on)
			return 0;
		air_on = 1;
		return pthread_create(&air_thread, 0, air_thread_func, NULL);
	}

	if (!strcmp(argv[1], "off")) {
		if (!air_on)
			return 0;
		air_on = 0;
		pthread_join(air_thread, 0);
		return 0;
	}

	return engine_on(atoi(argv[1]));
}

static struct input_cmd cmd = {
	.str = "air",
	.func = do_air,
	.info = "Use 'air on' or 'air off'",
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

static void clean_cmd(void)
{
	if (air_on) {
		air_on = 0;
		pthread_join(air_thread, 0);
	}

}

init_func(reg_cmd);
exit_func(clean_cmd);
