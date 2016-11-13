#include <string.h>
#include <init.h>
#include <input_cmd.h>
#include <log_project3.h>
#include <robot.h>
#include <target.h>

static int target_alloc(int argc, char *argv[])
{
	if (argc != 2) {
		log_err();
		return -1;
	}

	return alloc_target(argv[1]);
}

static int target_free(int argc, char *argv[])
{
	if (argc != 2) {
		log_err();
		return -1;
	}

	return free_target(argv[1]);
}

static int target_list(int argc, char *argv[])
{
	char str[256];
	int len = 0;
	struct target *t; 
	int i, j;

	memset(str, 0, sizeof(str));

	if (argc == 1) {	
		t = find_target(NULL);

		while(t) {
			len += sprintf(&str[len], "%s ", t->name);
			t = t->next;
		}
	}

	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			t = find_target(argv[i]);
			if (!t)
				continue;
			len += sprintf(&str[len], "%s:", t->name);
			for (j = 0; j < t->num; j++) {
				if (t->info[j].inactive)
					continue;
				len += sprintf(&str[len], "[%d] %hu %x,", 
					j, t->info[j].len, t->info[j].force);
			}
			len += sprintf(&str[len], ";\n");
		}
	}

	mvwprintw(input_win, LINES_INPUT - 1, 0, "%s\n", str);
	
	return 0;
}
/*
static int target_setlen(int argc, char *argv[])
{
//	struct target *t;

	if (argc != 3) {
		log_err();
		return -1;
	}
	
	return 0;
}
*/

static struct input_cmd sub_cmd[] = {
	{
        .str = "alloc",
        .func = target_alloc,
	.info = "example: 'target alloc stand_target'"
	},
	{
        .str = "free",
        .func = target_free,
	.info = "example: 'target free stand_target'"
	},
	{
        .str = "list",
        .func = target_list,
	.info = "example: 'target list'"
	},
};

#define NSUB_CMD	(sizeof(sub_cmd)/sizeof(struct input_cmd))

static int do_target(int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		log_err();
		return -1;
	}
	
	for (i = 0; i < NSUB_CMD; i ++) {
		if(strcmp(argv[1], sub_cmd[i].str))
			continue;
		return  sub_cmd[i].func(argc - 1, &argv[1]);
	}

	return -1;
}


static char help_info[256];

static struct input_cmd cmd = {
	.str = "target",
	.func = do_target,
	.info = help_info,
};

static int reg_cmd(void)
{
	int i;
	int len = 0;

	for (i = 0; i < NSUB_CMD; i ++) {
		if (! sub_cmd[i].info)
			continue;
		len += sprintf(&help_info[len], " %s\n", sub_cmd[i].info);
	}

	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
