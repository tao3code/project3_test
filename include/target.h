#ifndef TARGET_H
#define TARGET_H

#include <robot.h>

struct target {
	char *name;
	struct cy_tag cy[NUM_CYLINDERS];
	struct target *next;
	struct target *prev;
};

int alloc_target(char *name);
struct target *find_target(char *name);
int free_target(char *name);

#endif
