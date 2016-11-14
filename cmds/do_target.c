#include <string.h>
#include <stdlib.h>
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
				len += sprintf(&str[len], "[%d]%hu %x, ", 
					j, t->info[j].len, t->info[j].force);
			}
			len += sprintf(&str[len], ";\n");
		}
	}

	mvwprintw(input_win, LINES_INPUT - 1, 0, "%s\n", str);
	
	return 0;
}


static int infor_par_len(char *arg, struct cylinder_info *info)
{
	int len;
	len = atoi(arg);

	if (len < ENCODER_OFFSET || len > 0xffff -128) {
		log_err();
		return -1;
	}

	info->len = len;
	return 0;	
}

static int infor_par_force(char *arg, struct cylinder_info *info)
{
	if (!strcmp(arg, "push")) {
		info->force = '+'; 
		return 0;
	}

	if (!strcmp(arg, "pull")) {
		info->force = '-'; 
		return 0;
	}

	if (!strcmp(arg, "none")) {
		info->force = 0; 
		return 0;
	}

	log_info("%s err:%s, only 'push', 'pull', and 'none' is suported\n",
		 __FUNCTION__, arg);
	return -1;
}

static int infor_par_inactive(char *arg, struct cylinder_info *info)
{
	if (!strcmp(arg, "true")) {
		info->inactive = 1; 
		return 0;
	}

	if (!strcmp(arg, "false")) {
		info->inactive = 0; 
		return 0;
	}

	log_info("%s err:%s, only 'true', 'false' is suported\n",
		 __FUNCTION__, arg);
	return -1;
}

static struct info_par {
	const char *str;
	int (*func)(char *arg, struct cylinder_info *info);
} par_funcs[] = {
	{
	.str = "len",
	.func = infor_par_len,
	},
	{
	.str = "force",
	.func = infor_par_force,
	},
	{
	.str = "inactive",
	.func = infor_par_inactive,
	},
};

#define NINFO_PAR	(sizeof(par_funcs)/sizeof(struct info_par))

static char *match_val_return_arg(const char *str, char *in)
{
	int i = 0;	
	while (str[i] && in[i] && in[i] != '=') {
		if (str[i] != in[i])
			return 0;
		i++;
	}

	if (in[i] == '=')
		return &in[i+1];
	return 0;
}

static int target_set_par(char *arg, struct cylinder_info *info)
{
	int i;
	char *arg_val;
	char err_str[256];
	int len = 0;

	for (i = 0; i < NINFO_PAR; i++) {
		arg_val = match_val_return_arg(par_funcs[i].str, arg);
		if (!arg_val)
			continue;
		return par_funcs[i].func(arg_val, info);
	}

	for (i = 0; i < NINFO_PAR; i++) 
		len += sprintf(&err_str[len], "'%s', ", par_funcs[i].str);

	log_info("%s err:%s, only %s is suported\n",
                 __FUNCTION__, arg, err_str);
	return -1;
}

static int target_set(int argc, char *argv[])
{
	struct target *t;
	int i, err, index;

	if (argc < 3) {
		log_err();
		return -1;
	}

	t = find_target(argv[1]);
	if (!t) {
		log_info("%s, %s not find\n", __FUNCTION__, argv[1]);
		return -1;
	}

	index = atoi(argv[2]);
	if (index < 0 || index >= t->num) {
		log_info("%s, index:%d, no cylinder info\n", __FUNCTION__, index);
		return -1;
	}

	for (i = 3; i < argc; i++) {
		err = target_set_par(argv[i], &t->info[index]);
		if (err) { 
			log_info("%s, %s error\n", __FUNCTION__, argv[i]);
			return err;
		}
	}
	
	return 0;
}


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
	{
        .str = "set",
        .func = target_set,
	.info = "example: 'target set tarhet_name 1 len=128 force=+ inactive=0'"
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
