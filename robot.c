#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <serial.h>
#include <robot.h>
#include <stdio.h>
#include <import/firmware.h>
#include <log_project3.h>

static struct cylinder_info motion_state[] = {
	[0] {.id = ID_R_WAIST_A},
	[1] {.id = ID_L_WAIST_A},
	[2] {.id = ID_R_WAIST_B},
	[3] {.id = ID_L_WAIST_B},
	[4] {.id = ID_R_TIGHT},
	[5] {.id = ID_L_TIGHT},
	[6] {.id = ID_R_CALF},
	[7] {.id = ID_L_CALF},
	[8] {.id = ID_R_FOOT_A},
	[9] {.id = ID_L_FOOT_A},
	[10] {.id = ID_R_FOOT_B},
	[11] {.id = ID_L_FOOT_B},
};

static struct interface_info control_state = {.id = ID_IF_BOARD };

#define NCYLINDER	(sizeof(motion_state)/sizeof(struct cylinder_info))

static void trunc_name(char *msg)
{
	while(*msg != ',')
		msg++;
	*msg = 0;
}

void test_robot(void)
{
	int i;
	char cmd[16];
	int len;

	for (i = 0; i < NCYLINDER; i++) {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, ">%c report;\n", motion_state[i].id);
		len = sent_cmd_alloc_response(cmd, &motion_state[i].msg);
		if (len <= 0) {
			motion_state[i].id = 0;
			continue;
		}
		trunc_name(motion_state[i].msg);
		log_info("detect: %s\n", motion_state[i].msg);
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c report;", ID_IF_BOARD);
	len = sent_cmd_alloc_response(cmd, &control_state.msg);
	if (len <= 0) {
		control_state.id = 0;
		return;
	}
	trunc_name(control_state.msg);
	log_info("detect: %s\n", control_state.msg);
}

static unsigned char c2x(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch -'A' + 0xa;
	if (ch >= 'a' && ch <= 'f')
		return ch -'a' + 0xa;
	return 0;
}

static int msg_scan_data(char *msg, char *fmt, unsigned char *data, int n)
{
	int pmsg = 0;
	int pfmt = 0;
	int find = 0;
	while (fmt[pfmt]) {
		switch(fmt[pfmt]) {
		case '\x8':
			find++;
			*data = c2x(msg[pmsg]);
			pmsg++;
			*data <<= 4;
			*data |= c2x(msg[pmsg]);
			printf("get %x\n", *data);
			data++;	
			break;
		default:
			if (fmt[pfmt] != msg[pmsg]) {
				printf("err: %d(%c) %d(%c)\n",
					pfmt, fmt[pfmt], pmsg, msg[pmsg]);
				return -1;
			}
		}
		pfmt++;
		pmsg++;
		if (find >= n)
			break;
	}

	return find;
}

static int get_byte(char *cmd, char *fmt, unsigned char *byte)
{
	int ret;
	unsigned char b[32];
	char *msg;

	ret = sent_cmd_alloc_response(cmd, &msg);
	if (ret < strlen(fmt)) {
                log_info("Message(%d) read %d\n",
                        strlen(fmt), ret);
                return -1;
        }

	ret = msg_scan_data(msg, fmt, b, 32);

        if (ret != 1) {
                log_err();
                return -1;
        }

	*byte = b[0];	
	free(msg);
	return 0;	
}

int update_voltage(void)
{
	char cmd[16];
	int ret;

	if (!control_state.id) 
		return -1;
	
	memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, ">%c voltage;", control_state.id);
	ret = get_byte(cmd, MESSAGE_VOL, &control_state.vol);
	
	if (ret) {
		log_err();
		return -1;
	}
	printf("voltage:%x\n", control_state.vol);
	return 0;
}
