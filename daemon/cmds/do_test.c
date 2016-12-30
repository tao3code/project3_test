#include <stdlib.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>

static char *seq[] = {
	"run example/0_start.txt",
	"run example/1_load_r.txt",
	"run example/2_load_l.txt",
};

#define NTEST	(sizeof(seq)/sizeof(char *))

static int do_test(int argc, char *argv[])
{
	int i, err;
	int index;

	if (argc == 1) { 
		for (i = 0; i < NTEST; i++) {
			err = run_cmd(seq[i]);
			if (err)
				return err;
		}
		return 0;
	}
		
	for (i = 1; i < argc; i++) {
		index = atoi(argv[i]);
		if (index < 0 || index >= NTEST) {
			log_info("test err: %d outof range[0->%d]\n",
				index, NTEST);
			return -1;
		}
		err = run_cmd(seq[index]);
		if (err)
			return err;
	}

	return 0;
}

static struct input_cmd cmd = {
	.str = "test",
	.func = do_test,
};

static int reg_cmd(void)
{
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
