#include "serial.h"
#include "robot.h"
#include <stdio.h>

static struct cylinder_info motion_state[] = {
	[0]	{.name = "R wrist back"},
	[1]	{.name = "L wrist back"},
	[2]	{.name = "R wrist front"},
	[3]	{.name = "L wrist front"},
	[4]	{.name = "R thigh"},
	[5]	{.name = "L thigh"},
	[6]	{.name = "R calf"},
	[7]	{.name = "L calf"},
	[8]	{.name = "R foot OUTER"},
	[9]	{.name = "L foot OUTER"},
	[10]	{.name = "R foot INNER"},
	[11]	{.name = "L foot INNER"},
};

static struct interface_info control_state;

int test_robot(void)
{
	printf("%s, len:%hx\n", motion_state[3].name, motion_state[3].len);
	return 0;
}
