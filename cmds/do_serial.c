#include <string.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <serial.h>

static int do_serial(int argc, char *argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "on"))
		return serial_init();

	if (!strcmp(argv[1], "off")) {
		serial_close();
		return 0;
	}

	return -1;
}

static struct input_cmd cmd = {
	.str = "serial",
	.func = do_serial,
	.info = "Use 'serial on' or 'serial off'",
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
