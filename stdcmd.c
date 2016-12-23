#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdcmd.h>
#include <log_project3.h>

static char err_msg[256];

static void gen_err(const char *str, const char *in, const char *ep)
{
	unsigned long pos = (unsigned long)ep - (unsigned long)in;

	memcpy(err_msg, in, pos + 1);
	sprintf(&err_msg[pos], "(%s)%s", str, ep);
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

static int update_args(const char *in, struct func_arg *args)
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
		log_info("%s\n", err_msg);
		return -1;
	}

	while (*str) {
		arg = args;
		pop_len(str, &w_len, &w_end);

		if (w_end != '=') {
			gen_err("<-miss '='", in, str + w_len);
			log_info("%s\n", err_msg);
			return -1;
		}

		while (arg->name) {
			if (if_equ(str, arg->name))
				break;
			arg++;
		}

		if (!arg->name) {
			gen_err("unknow->", in, str);
			log_info("%s\n", err_msg);
			return -1;
		}

		str += w_len + 1;

		if (*str == '=' || *str == ',' || !*str) {
			gen_err("<-illegal", in, str);
			log_info("%s\n", err_msg);
			return -1;
		}

		pop_len(str, &w_len, &w_end);
		if (w_len >= MAX_VAL_LEN) {
			gen_err("exceed->", in, str);
			log_info("%s\n", err_msg);
			return -1;
		}

		memcpy(buf, str, w_len);
		buf[w_len] = 0;

		if (!arg->type)
			arg->type = "%d";
		ret = sscanf(buf, arg->type, arg->var);
		if (ret != 1) {
			gen_err("<-failure", in, str + w_len);
			log_info("%s\n", err_msg);
			return -1;
		}

		if (arg->check) {
			if (arg->check()) {
				gen_err("<-wrong", in, str + w_len);
				log_info("%s\n", err_msg);
				return -1;
			}
		}

		str += w_len;
		while (*str == ',')
			str++;
	}

	return 0;
}

int cmd_run_funcs(const char *in, struct cmd_func *funcs)
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
			ret = update_args(str, funcs[i].args);
			if (ret)
				return -1;
		}
		return funcs[i].func(funcs[i].args);
	}
	gen_err("unsupport->", in, str);
	log_info("%s\n", err_msg);
	return -1;
}
