#ifndef TARGET_H
#define TARGET_H

#include <robot.h>
#include <stdcmd.h>

struct target {
	char name[MAX_VAL_LEN];
	struct transform_info {
		struct cy_tag cy;
		struct transform_record {
			unsigned short start_len;
			unsigned short stop_len;
			int meg_val;
			struct transform_record *last;
		} *record;
	} trans[NUM_CYLINDERS];
	struct target *next;
	struct target *prev;
};

int alloc_target(char *name);
struct target *find_target(char *name);
int free_target(char *name);
int try_transform_once(struct target *tag);

#endif
