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
	[0] {.msg = MESSAGE_1, .type = TYPE_N},
	[1] {.msg = MESSAGE_7, .type = TYPE_P},
	[2] {.msg = MESSAGE_2, .type = TYPE_P},
	[3] {.msg = MESSAGE_8, .type = TYPE_N},
	[4] {.msg = MESSAGE_3, .type = TYPE_N},
	[5] {.msg = MESSAGE_9, .type = TYPE_P},
	[6] {.msg = MESSAGE_4, .type = TYPE_N},
	[7] {.msg = MESSAGE_A, .type = TYPE_P},
	[8] {.msg = MESSAGE_5, .type = TYPE_P},
	[9] {.msg = MESSAGE_B, .type = TYPE_N},
	[10] {.msg = MESSAGE_6, .type = TYPE_N},
	[11] {.msg = MESSAGE_C, .type = TYPE_P},
};

static struct interface_info control_state = {.msg = MESSAGE_0};
#define NCYLINDER	(sizeof(motion_state)/sizeof(struct cylinder_info))

static int trunc_name_get_id(char *msg, const char *fmt, 
				char *idout, char **nameout)
{
	char *p_name = msg;

	*idout = 0;
	*nameout = 0;

	while (*fmt && *msg) {
		if (*fmt != *msg)
			return -1;

		if (*fmt == ',') {
			*msg = 0;
			*nameout = p_name;
		}

		if (*fmt ==':') {
			if (!*nameout)
				return -1;
			*idout = *(fmt + 1);
			return 0;
		}

		fmt++;
		msg++;
	}
	
	return -1;
}

static char get_assigned_id(const char *fmt)
{
	while (*fmt) {
		if (*fmt == ':')
			return *(fmt + 1);
		fmt++;
	}
	return 0;
}

int test_robot(void)
{
	int i;
	int ret;
	char cmd[16];
	char *msg;
	char *name;
	char id;
	int find;

	find = 0;
	for (i = 0; i < NCYLINDER; i++) {
		memset(cmd, 0, sizeof(cmd));
		id = get_assigned_id(motion_state[i].msg);
		sprintf(cmd, ">%c report;\n", id);
		ret = sent_cmd_alloc_response(cmd, &msg);
		if (ret < 0) 
			return ret;	
		if (!msg) {
			motion_state[i].id = 0;
			continue;
		}
		ret = trunc_name_get_id(msg, motion_state[i].msg,
			&motion_state[i].id, &name);
		if (ret) {
			log_err();
			free(msg);
			continue;
		}
		log_info("detect: %s, id: %c\n", name, motion_state[i].id);
		free(msg);
		find++;
	}

	memset(cmd, 0, sizeof(cmd));
	id = get_assigned_id(control_state.msg);
	sprintf(cmd, ">%c report;", id);
	ret = sent_cmd_alloc_response(cmd, &msg);
	if (ret < 0)
		return ret;

	if (!msg) {
		control_state.id = 0;
		return find;
	}
	ret = trunc_name_get_id(msg, control_state.msg,
		&control_state.id, &name);
	if (ret) {
		log_err();
		free(msg);
		return find;
	}
	log_info("detect: %s, id: %c\n", name, control_state.id);
	free(msg);
	find++;

	return find;
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

	ret = sent_cmd_alloc_response(cmd, &msg);

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

inline const struct cylinder_info *get_motion_info(int *count)
{
	*count = NCYLINDER;
	return	motion_state; 
}
