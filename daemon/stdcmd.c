#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdcmd.h>
#include <log_project3.h>
#include <socket_project3.h>

static char err_msg[256];

static void gen_err(const char *str, const char *in, const char *ep)
{
	int len;
	unsigned long pos;

	len = sprintf(err_msg, "error:");
	pos = (unsigned long)ep - (unsigned long)in;

	memcpy(&err_msg[len], in, pos + 1);
	len += pos;
	len += sprintf(&err_msg[len], "(%s)%s\n", str, ep);
	socket_write_buf(err_msg, len);
}

static void pop_len(const char *start, int *key_len, char *key_out)
{
	const char *str = start;
	int p = 0;

	while (str[p] && str[p] != ',' && str[p] != '=')
		p++;
	if (key_out)
		*key_out = str[p];
	if (key_len)
		*key_len = p;
}

static int if_equ(const char *str, const char *name)
{
	int p = 0;

	while (str[p] && str[p] != ',' && str[p] != '=') {
		if (str[p] != name[p])
			return 0;
		p++;
	}

	if (name[p])
		return 0;
	return 1;
}

int stdcmd_update_args(const char *in, struct func_arg *args)
{
	struct func_arg *arg;
	const char *str = in;
	int w_len;
	char w_end;
	int ret;
	char buf[MAX_VAL_LEN];

	pop_len(str, &w_len, &w_end);
	if (!args) {
		gen_err("miss args", in, str);
		return -1;
	}

	while (*str) {
		arg = args;
		pop_len(str, &w_len, &w_end);

		if (w_end != '=') {
			gen_err("<-miss '='", in, str + w_len);
			return -1;
		}

		while (arg->name) {
			if (if_equ(str, arg->name))
				break;
			arg++;
		}

		if (!arg->name) {
			gen_err("unknow->", in, str);
			return -1;
		}

		str += w_len + 1;

		if (*str == '=' || *str == ',' || !*str) {
			gen_err("<-illegal", in, str);
			return -1;
		}

		pop_len(str, &w_len, &w_end);
		if (w_len >= MAX_VAL_LEN) {
			gen_err("exceed->", in, str);
			return -1;
		}

		memcpy(buf, str, w_len);
		buf[w_len] = 0;

		if (!arg->type)
			arg->type = "%d";
		ret = sscanf(buf, arg->type, arg->var);
		if (ret != 1) {
			gen_err("<-failure", in, str + w_len);
			return -1;
		}

		if (arg->check) {
			if (arg->check(arg->var)) {
				gen_err("<-wrong", in, str + w_len);
				return -1;
			}
		}

		str += w_len;
		while (*str == ',')
			str++;
	}

	return 0;
}

int stdcmd_run_funcs(const char *in, struct cmd_func *funcs)
{
	int i = 0;
	int ret;
	const char *str = in;
	int w_len;

	pop_len(str, &w_len, 0);

	while (funcs[i].name) {
		ret = if_equ(str, funcs[i].name);
		if (!ret) {
			i++;
			continue;
		}
		str += w_len;

		while (*str == ',')
			str++;
		if (*str) {
			ret = stdcmd_update_args(str, funcs[i].args);
			if (ret)
				return -1;
		}
		return funcs[i].func(funcs[i].args);
	}
	gen_err("unsupport->", in, str);
	return -1;
}

static unsigned long arg_value(const struct func_arg *arg)
{
	unsigned long *var_u64 = arg->var;

	if (!strcmp(arg->type, "%s"))
		return (unsigned long)arg->var;
	return *var_u64;
}

static int help_func(char *buf, char *name, struct cmd_func *funcs)
{
	const struct cmd_func *func = funcs;
	const struct func_arg *arg;
	int len = 0;
	char fmt[8];

	if (!name) {
		while (func->name) {
			len += sprintf(&buf[len], "%s ", func->name);
			func++;
		}

		return len;
	}

	while (func->name) {
		if (strcmp(name, func->name)) {
			func++;
			continue;
		}
		len += sprintf(&buf[len], "%s,", func->name);
		arg = func->args;
		if (!arg)
			break;
		while (arg->name) {
			if (!arg->type)
				sprintf(fmt, "%%s=%%d,");
			else
				sprintf(fmt, "%%s=%s,", arg->type);
			len +=
			    sprintf(&buf[len], fmt, arg->name, arg_value(arg));
			arg++;
		}
		return len;
	}

	len += sprintf(&buf[len], "unsupport %s", name);
	return len;
}

int stdcmd_help(char *buf, struct cmd_func *funcs, int argc, char *argv[])
{
	int len = 0;
	int i;

	if (argc == 0) {
		len += help_func(&buf[len], 0, funcs);
		buf[len] = '\n';
		len++;
	} else {
		for (i = 0; i < argc; i++) {
			len += help_func(&buf[len], argv[i], funcs);
			buf[len] = '\n';
			len++;
		}
	}

	return len;
}
