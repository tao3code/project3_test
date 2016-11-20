#include <string.h>
#include <init.h>
#include <input_cmd.h>
#include <robot.h>
#include <serial.h>

static int do_meg(int argc, char *argv[])
{
	int ret;

	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on")) {
		ret =  meg12v_on('1');
		update_meg12v();
		return ret;
	}

	if (!strcmp(argv[1], "off")) {
		ret =  meg12v_on('0');
		update_meg12v();
		return ret;
	}

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

init_func(reg_cmd);
