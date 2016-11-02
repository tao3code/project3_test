#ifndef INIT_H
#define INITH

#define init_func(FUNC) \
static int __attribute__((unused, __section__(".init_sec"))) \
 (*p_##FUNC)(void) = FUNC

#define exit_func(FUNC) \
static void __attribute__((unused, __section__(".exit_sec"))) \
 (*p_##FUNC)(void) = FUNC

int do_init_funcs(void);
void do_exit_funcs(void);

#endif
