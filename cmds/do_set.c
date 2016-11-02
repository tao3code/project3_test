#include <stdlib.h>
#include <init.h>
#include <log_project3.h>
#include <input_cmd.h>
#include <robot.h>

static int do_set(int argc, char *argv[])
{
        const struct cylinder_info *info;
        int ncylinder;
        int index;
        int val;

        if (argc != 3)
                return -1;

        info = get_motion_info(&ncylinder);
        index = atoi(argv[1]);

        if (index < 0 || index > ncylinder) {
                log_err();
                return -1;
        }

        if (!info[index].id) {
                log_info("%s, no cylinder %d\n", __FUNCTION__, index);
                return -1;
        }

        val = atoi(argv[2]);

        return set_encoder(index, val);
}

static struct input_cmd cmd = {
        .str = "set",
        .func = do_set,
        .info = "example 'set 1 0'",
};

static int reg_cmd(void)
{
        register_cmd(&cmd);
        return 0;
}

init_func(reg_cmd);
