#include <string.h>
#include <init.h>
#include <target.h>
#include <stdcmd.h>
#include <input_cmd.h>
#include <socket_project3.h>

static struct cylinder_info *info;

#define MAX_TRANS_EXPIRE_MS	5000

static char trans_name[sizeof(((struct target *) 0)->name)];
static unsigned long trans_expire = MAX_TRANS_EXPIRE_MS;

static int check_expire(void *var)
{
	if (trans_expire > MAX_TRANS_EXPIRE_MS)
		trans_expire = MAX_TRANS_EXPIRE_MS;
	return 0;
}

static struct func_arg trans_args[] = {
	{.name = "name",
	 .var = trans_name,
	 .type = "%s"},
	{.name = "name",
	 .var = &trans_expire,
	 .type = "%lu",
	 .check = check_expire},
	{0},
};

static int do_trans(struct func_arg *args)
{
	int name_len;
	struct target *t;
	char msg[128];
	int msg_len = 0;
	int ret = 0;

	name_len = strlen(trans_name);
	if (!name_len) {
		msg_len = sprintf(msg, "miss 'name'\n");
		ret = -1;
		goto end_trans;
	}

	t = find_target(trans_name);
	if (!t) {
		msg_len = sprintf(msg, "target with name '%s' not exist!\n",
				  trans_name);
		ret = -1;
		goto end_trans;
	}

	ret = transform(t, trans_expire);

 end_trans:
	socket_write_buf(msg, msg_len);
	memset(trans_name, 0, sizeof(trans_name));
	trans_expire = MAX_TRANS_EXPIRE_MS;
	return ret;
}

static int check_id(void *var)
{
	int *id = var;

	if (*id < 0 || *id >= NUM_CYLINDERS)
		return -1;
	return 0;
}

static char set_name[sizeof(((struct target *) 0)->name)];
static int set_id = -1;
static char set_inactive = 0;
static unsigned short set_len = 0;

static struct func_arg set_args[] = {
	{.name = "name",
	 .var = set_name,
	 .type = "%s"},
	{.name = "id",
	 .var = &set_id,
	 .type = "%d",
	 .check = check_id},
	{.name = "inactive",
	 .var = &set_inactive,
	 .type = "%c"},
	{.name = "len",
	 .var = &set_len,
	 .type = "%hu"},
	{0},
};

static int do_set(struct func_arg *args)
{
	int name_len;
	struct target *t;
	char msg[128];
	int msg_len = 0;
	int ret = 0;

	name_len = strlen(set_name);
	if (!name_len) {
		msg_len = sprintf(msg, "miss 'name'\n");
		ret = -1;
		goto end_set;
	}

	t = find_target(set_name);
	if (!t) {
		msg_len = sprintf(msg, "target with name '%s' not exist!\n",
				  set_name);
		ret = -1;
		goto end_set;
	}

	if (set_id == -1) {
		msg_len = sprintf(msg, "miss 'id'\n");
		ret = -1;
		goto end_set;
	}

	t->cy[set_id].id = info[set_id].enc.id;

	if (set_inactive) {
		if (set_inactive == '0')
			set_inactive = 0;
		else
			set_inactive = 1;
		t->cy[set_id].inactive = set_inactive;
	}

	if (set_len) {
		if (set_len < ENCODER_OFFSET)
			set_len = ENCODER_OFFSET;
		if (set_len > ENCODER_OFFSET + info[set_id].mea.c)
			set_len = ENCODER_OFFSET + info[set_id].mea.c;
		t->cy[set_id].len = set_len;
	}

 end_set:
	socket_write_buf(msg, msg_len);
	memset(set_name, 0, sizeof(set_name));
	set_id = -1;
	set_inactive = 0;
	set_len = 0;
	return ret;
}

static char list_name[sizeof(((struct target *) 0)->name)];
static int list_id = -1;

static struct func_arg list_args[] = {
	{.name = "name",
	 .var = list_name,
	 .type = "%s"},
	{.name = "id",
	 .var = &list_id,
	 .type = "%d",
	 .check = check_id},
	{0},
};

static int do_list(struct func_arg *args)
{
	int name_len;
	struct target *t;
	char msg[256];
	int msg_len = 0;
	int ret = 0;
	int i;

	name_len = strlen(list_name);

	if (!name_len) {
		msg_len = 0;
		t = find_target(NULL);
		while (t) {
			msg_len += sprintf(&msg[msg_len], "%s ", t->name);
			t = t->next;
		}
		msg[msg_len] = '\n';
		msg_len++;
		goto end_list;
	}

	t = find_target(list_name);
	if (!t) {
		msg_len = sprintf(msg, "target with name '%s' not exist!\n",
				  list_name);
		ret = -1;
		goto end_list;
	}

	if (list_id != -1) {
		msg_len = sprintf(msg, "%s.cy[%d] = {", t->name, list_id);
		if (t->cy[list_id].force)
			msg_len += sprintf(&msg[msg_len], ".force = %c,",
					   t->cy[list_id].force);
		if (t->cy[list_id].inactive)
			msg_len += sprintf(&msg[msg_len], ".inactive = %d,",
					   t->cy[list_id].inactive);
		if (t->cy[list_id].len)
			msg_len += sprintf(&msg[msg_len], ".len = %hu}\n",
					   t->cy[list_id].len);
		goto end_list;
	}

	msg_len = sprintf(msg, "%s:\n", t->name);

	for (i = 0; i < NUM_CYLINDERS; i++) {
		msg_len += sprintf(&msg[msg_len],
				   "[%d]{%d, %d, %hu}\n",
				   i, t->cy[i].force,
				   t->cy[i].inactive, t->cy[i].len);
	}

 end_list:
	socket_write_buf(msg, msg_len);
	memset(list_name, 0, sizeof(list_name));
	list_id = -1;
	return ret;
}

static char free_name[sizeof(((struct target *) 0)->name)];

static struct func_arg free_args[] = {
	{.name = "name",
	 .var = free_name,
	 .type = "%s"},
	{0},
};

static int do_free(struct func_arg *args)
{
	int name_len;
	struct target *t;
	char msg[128];
	int msg_len = 0;
	int ret = 0;

	name_len = strlen(free_name);
	if (!name_len) {
		msg_len = sprintf(msg, "miss 'name'\n");
		ret = -1;
		goto end_free;
	}

	t = find_target(free_name);
	if (!t) {
		msg_len = sprintf(msg, "target with name '%s' not exist!\n",
				  free_name);
		ret = -1;
		goto end_free;
	}

	if (free_target(free_name)) {
		msg_len = sprintf(msg, "free target '%s' failue!\n", free_name);
		ret = -1;
		goto end_free;
	}

	msg_len = sprintf(msg, "target '%s' is freed!\n", free_name);

 end_free:
	socket_write_buf(msg, msg_len);
	memset(free_name, 0, sizeof(free_name));
	return ret;
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
	int msg_len = 0;
	int ret = 0;

	name_len = strlen(alloc_name);
	if (!name_len) {
		msg_len = sprintf(msg, "miss 'name'\n");
		ret = -1;
		goto end_alloc;
	}

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
	{.name = "trans",
	 .func = do_trans,
	 .args = trans_args},
	{.name = "alloc",
	 .func = do_alloc,
	 .args = alloc_args},
	{.name = "free",
	 .func = do_free,
	 .args = free_args},
	{.name = "list",
	 .func = do_list,
	 .args = list_args},
	{.name = "set",
	 .func = do_set,
	 .args = set_args},
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
	info = get_motion_info();
	register_cmd(&cmd);
	return 0;
}

init_func(reg_cmd);
