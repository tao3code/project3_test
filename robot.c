#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <serial.h>
#include <robot.h>
#include <stdio.h>
#include <import/firmware.h>
#include <log_project3.h>

#define TYPE_P	0
#define TYPE_N	1

static struct cylinder_info motion_state[] = {
	[0] {.id = ID_R_WAIST_A, .type = TYPE_N},
	[1] {.id = ID_L_WAIST_A, .type = TYPE_P},
	[2] {.id = ID_R_WAIST_B, .type = TYPE_P},
	[3] {.id = ID_L_WAIST_B, .type = TYPE_N},
	[4] {.id = ID_R_TIGHT, .type = TYPE_N},
	[5] {.id = ID_L_TIGHT, .type = TYPE_P},
	[6] {.id = ID_R_CALF, .type = TYPE_N},
	[7] {.id = ID_L_CALF, .type = TYPE_P},
	[8] {.id = ID_R_FOOT_A, .type = TYPE_P},
	[9] {.id = ID_L_FOOT_A, .type = TYPE_N},
	[10] {.id = ID_R_FOOT_B, .type = TYPE_N},
	[11] {.id = ID_L_FOOT_B, .type = TYPE_P},
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
	char *msg;

	for (i = 0; i < NCYLINDER; i++) {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, ">%c report;\n", motion_state[i].id);
		msg = sent_cmd_alloc_response(cmd, &len);
		if (!msg) {
			motion_state[i].id = 0;
			continue;
		}
		trunc_name(msg);
		log_info("detect: %s\n", msg);
		free(msg);
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c report;", ID_IF_BOARD);
	msg = sent_cmd_alloc_response(cmd, &len);
	if (!msg) {
		control_state.id = 0;
		return;
	}
	trunc_name(msg);
	log_info("detect: %s\n", msg);
	free(msg);
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
			data++;	
			break;
		default:
			if (fmt[pfmt] != msg[pmsg]) {
				log_info("%s mismatch: "
					"msg[%d](%c) fmt[%d](%c)\n",
					__FUNCTION__,
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

static int get_byte(char *cmd, char *fmt, unsigned char *byte, int n)
{
	int ret;
	unsigned char b[32];
	char *msg;

	if (n > sizeof(b)) {
		log_info("exceed max bytes(%d):%d\n",
			sizeof(b), n);
		return -1;
	}

	msg = sent_cmd_alloc_response(cmd, &ret);

	if (!msg) {
		log_info("%s read nothing\n", __FUNCTION__);
		return -1;
	}

	if (ret < strlen(fmt)) {
                log_info("%s Message(%d) read %d\n",
                        __FUNCTION__, strlen(fmt), ret);
                return -1;
        }

	ret = msg_scan_data(msg, fmt, b, sizeof(b));

        if (ret != n) {
                log_info("%s request %d but get %d\n", __FUNCTION__,  n, ret);
                return -1;
        }

	memcpy(byte, b, ret);	
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
	ret = get_byte(cmd, MESSAGE_VOL, &control_state.vol, 1);
	
	if (ret) {
		log_err();
		return -1;
	}
	return 0;
}

int update_presure(void)
{
        char cmd[16];
        int ret;

        if (!control_state.id)
                return -1;

        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, ">%c presure;", control_state.id);
        ret = get_byte(cmd, MESSAGE_AIR, &control_state.air, 1);

        if (ret) {
                log_err();
                return -1;
        }
	
        return 0;
}

int update_gyroscope(void)
{
	char cmd[16];
        int ret;
	int i;
	unsigned char res[sizeof(control_state.raw)];

        if (!control_state.id)
                return -1;

        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, ">%c gyroscope;", control_state.id);
        ret = get_byte(cmd, MESSAGE_GYRO, res, sizeof(res));

        if (ret) {
                log_err();
                return -1;
        }
	
	for (i = 0; i < sizeof(res); i++)
		control_state.raw[i] = res[sizeof(res) -i -1];

        return 0;

}

void update_control_state(void)
{
	update_voltage();
	update_presure();
	update_gyroscope();
}

int update_cylinder_len(int index)
{
	char cmd[16];
        int ret;
	unsigned char res[sizeof(motion_state[0].raw)];

	if (index > NCYLINDER -1) {
		log_info("%s index %d > NCYLIDDER\n",
			__FUNCTION__, index);
		return -1;
	}

	if (!motion_state[index].id) {
		log_err();
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, ">%c count;", motion_state[index].id);
        ret = get_byte(cmd, MESSAGE_ENC, res, sizeof(res));

        if (ret) {
                log_err();
                return -1;
        }

	motion_state[index].raw[0] = res[2];
	motion_state[index].raw[1] = res[1];

	if (motion_state[index].type == TYPE_N)
		motion_state[index].len = 0xffff -  motion_state[index].len;

	return 0;
}

void update_motion_state(void) 
{
	int i;
	
	for (i = 0; i < NCYLINDER; i++) 
                 update_cylinder_len(i);	
}

inline const struct interface_info *get_interface_info(void)
{
	return &control_state;
}
