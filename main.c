#include <serial.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <init.h>

int main(int argc, const char *argv[])
{
	int err;

	err = log_open();
	if (err)
		goto log_open_err;
	log_info("Build virsion: %s,%s\n", __DATE__, __TIME__);

	err = open_scr();
	if (err) {
		log_err();
		goto scr_open_err;
	}

	err = input_cmd_init();
	if (err) {
		log_err();
		goto input_init_err;
	}

	do_init_funcs();
	cmd_loop();
	do_exit_funcs();

	input_cmd_exit();
 input_init_err:
	close_scr();
 scr_open_err:
	log_close();
 log_open_err:
	return err;
}
