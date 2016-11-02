#include <string.h>
#include <init.h>
#include <input_cmd.h>
#include <robot.h>
#include <serial.h>

static int do_meg(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on"))
		return meg12v_on('1');

	if (!strcmp(argv[1], "off"))
		return meg12v_on('0');

	return -1;
}

static struct input_cmd cmd = {
	.str = "meg",
	.func = do_meg,
	.info = "Use 'meg on' or 'meg off'",
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

static void clean_cmd(void)
{
	if (is_serial_on()) {
		run_cmd("meg off");
		run_cmd("serial off");
	}

}

init_func(reg_cmd);
exit_func(clean_cmd);
