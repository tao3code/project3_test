#ifndef TARGET_H
#define TARGET_H

struct target {
        char *name;
        struct cylinder_info *info;
        int num;
        struct target *next;
        struct target *prev;
};

int alloc_target(char *name);
struct target *find_target(char *name);
int free_target(char *name);

#endif
