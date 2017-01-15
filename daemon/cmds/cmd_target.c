#include <string.h>
#include <init.h>
#include <target.h>
#include <stdcmd.h>
#include <input_cmd.h>
#include <socket_project3.h>

static char free_name[sizeof(((struct target *) 0)->name)];

static struct func_arg free_args[] = {
	{.name = "name",
	 .var = free_name,
	 .type = "%s"},
	{0},
};

static int do_free(struct func_arg *args)
{
	return 0;
}

static char alloc_name[sizeof(((struct target *) 0)->name)];

static struct func_arg alloc_args[] = {
	{.name = "name",
	 .var = alloc_name,
	 .type = "%s"},
	{0},
};

static int do_alloc(struct func_arg *args)
{
	int name_len;
	struct target *t;
	char msg[128];
	int msg_len;
	int ret;

	ret = 0;
	name_len = strlen(alloc_name);
	if (!name_len)
		return 0;

	t = find_target(alloc_name);
	if (t) {
		msg_len = sprintf(msg, "target with name '%s' already exist!\n",
				  alloc_name);
		ret = -1;
		goto end_alloc;
	}

	if (alloc_target(alloc_name)) {
		msg_len = sprintf(msg, "alloc target '%s' failue!\n",
				  alloc_name);
		ret = -1;
		goto end_alloc;
	}

	msg_len = sprintf(msg, "alloc target '%s' success!\n", alloc_name);

 end_alloc:
	socket_write_buf(msg, msg_len);
	memset(alloc_name, 0, sizeof(alloc_name));
	return ret;
}

static struct cmd_func target_funcs[] = {
	{.name = "alloc",
	 .func = do_alloc,
	 .args = alloc_args},
	{.name = "free",
	 .func = do_free,
	 .args = free_args},
	{0},
};

static int cmd_target(int argc, char *argv[])
{
	int i;
	int ret;

	for (i = 1; i < argc; i++) {
		ret = stdcmd_run_funcs(argv[i], target_funcs);
		if (ret)
			return -1;
	}
	return 0;
}

static int help_target(char *buf, int argc, char *argv[])
{
	return stdcmd_help(buf, target_funcs, argc - 1, &argv[1]);
}

static struct input_cmd cmd = {
	.str = "target",
	.func = cmd_target,
	.help = help_target,
};

static int reg_cmd(void)
{
	memset(free_name, 0, sizeof(free_name));
	memset(alloc_name, 0, sizeof(alloc_name));
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
