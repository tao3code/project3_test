#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <serial.h>
#include <robot.h>
#include <import/firmware.h>
#include <log_project3.h>
#include <input_cmd.h>

#define TYPE_P	0
#define TYPE_N	1

static struct cylinder_info motion_state[] = {
	[0] {.dev = {.ack = MESSAGE_1},
	     .type = TYPE_N,
	     .fix = {25, 1},
	     .mea = {.c = 237,.pa = 60,.na = 70,.pv = 110,.nv = -100}},
	[1] {.dev = {.ack = MESSAGE_7},
	     .type = TYPE_P,
	     .fix = {25, 1},
	     .mea = {.c = 236,.pa = 60,.na = 70,.pv = 110,.nv = -100}},
	[2] {.dev = {.ack = MESSAGE_2},
	     .type = TYPE_P,
	     .fix = {25, 1},
	     .mea = {.c = 240,.pa = 60,.na = 70,.pv = 50,.nv = -50}},
	[3] {.dev = {.ack = MESSAGE_8},
	     .type = TYPE_N,
	     .fix = {25, 1},
	     .mea = {.c = 243,.pa = 60,.na = 70,.pv = 50,.nv = -50}},
	[4] {.dev = {.ack = MESSAGE_3},
	     .type = TYPE_N,
	     .fix = {140, 4},
	     .mea = {.c = 1400,.pa = 60,.na = 60,.pv = 240,.nv = -140}},
	[5] {.dev = {.ack = MESSAGE_9},
	     .type = TYPE_P,
	     .fix = {140, 4},
	     .mea = {.c = 1427,.pa = 60,.na = 60,.pv = 240,.nv = -140}},
	[6] {.dev = {.ack = MESSAGE_4},
	     .type = TYPE_N,
	     .fix = {125, 4},
	     .mea = {.c = 1180,.pa = 60,.na = 70,.pv = 200,.nv = -110}},
	[7] {.dev = {.ack = MESSAGE_A},
	     .type = TYPE_P,
	     .fix = {125, 4},
	     .mea = {.c = 1150,.pa = 60,.na = 70,.pv = 200,.nv = -110}},
	[8] {.dev = {.ack = MESSAGE_5},
	     .type = TYPE_P,
	     .fix = {50, 1},
	     .mea = {.c = 500,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
	[9] {.dev = {.ack = MESSAGE_B},
	     .type = TYPE_N,
	     .fix = {50, 1},
	     .mea = {.c = 472,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
	[10] {.dev = {.ack = MESSAGE_6},
	      .type = TYPE_N,
	      .fix = {50, 1},
	      .mea = {.c = 494,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
	[11] {.dev = {.ack = MESSAGE_C},
	      .type = TYPE_P,
	      .fix = {50, 1},
	      .mea = {.c = 436,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
};

static struct interface_info control_state = {.dev = {.ack = MESSAGE_0} };

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

		if (*fmt == ':') {
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

int test_device(struct device *dev)
{
	char cmd[16];
	char *msg;
	char *name;
	char id;
	int ret;

	if (!dev->ack) {
		log_err();
		return -1;
	}

	dev->id = 0;
	memset(cmd, 0, sizeof(cmd));
	id = get_assigned_id(dev->ack);
	sprintf(cmd, ">%c report;", id);
	ret = sent_cmd_alloc_response(cmd, &msg);
	if (ret < 0)
		return ret;
	if (!msg)
		return 0;
	ret = trunc_name_get_id(msg, dev->ack, &dev->id, &name);
	if (ret) {
		log_err();
		free(msg);
		return ret;
	}

	log_info("detect: %s, id: %c\n", name, dev->id);

	return 0;
}

static unsigned char c2x(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 0xa;
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 0xa;
	return 0;
}

static int msg_scan_data(char *msg, char *fmt, unsigned char *data, int n)
{
	int pmsg = 0;
	int pfmt = 0;
	int find = 0;
	while (fmt[pfmt]) {
		switch (fmt[pfmt]) {
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
					 "fmt[%d](%x) msg[%d](%s)\n",
					 __FUNCTION__,
					 pfmt, fmt[pfmt], pmsg, msg);
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

static int get_byte(char *cmd, char *fmt, volatile unsigned char *byte, int n)
{
	int ret;
	unsigned char b[32];
	char *msg;

	if (n > sizeof(b)) {
		log_info("exceed max bytes(%d):%d\n", sizeof(b), n);
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

	ret = msg_scan_data(msg, fmt, b, n);

	if (ret != n) {
		log_info("%s request %d but get %d\n", __FUNCTION__, n, ret);
		return -1;
	}

	memcpy((char *)byte, b, ret);
	free(msg);
	return 0;
}

int update_voltage(void)
{
	char cmd[16];
	int ret;

	if (!control_state.dev.id) {
		log_info("%s, no interface board!\n", __FUNCTION__);
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c voltage;", control_state.dev.id);
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

	if (!control_state.dev.id) {
		log_info("%s, no interface board!\n", __FUNCTION__);
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c presure;", control_state.dev.id);
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

	if (!control_state.dev.id) {
		log_info("%s, no interface board!\n", __FUNCTION__);
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c gyroscope;", control_state.dev.id);
	ret = get_byte(cmd, MESSAGE_GYRO, res, sizeof(res));

	if (ret) {
		log_err();
		return -1;
	}

	for (i = 0; i < sizeof(res); i++)
		control_state.raw[i] = res[sizeof(res) - i - 1];

	return 0;

}

int update_meg12v(void)
{
	char cmd[16];
	int ret;

	if (!control_state.dev.id) {
		log_info("%s, no interface board!\n", __FUNCTION__);
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c meg12v 3;", control_state.dev.id);
	ret = get_byte(cmd, MESSAGE_MEG, &control_state.m12v, 1);
	control_state.m12v = !control_state.m12v;

	if (ret) {
		log_err();
		return -1;
	}

	return 0;
}

int update_engine(void)
{
	char cmd[16];
	int ret;

	if (!control_state.dev.id) {
		log_info("%s, no interface board!\n", __FUNCTION__);
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c engine 0;", control_state.dev.id);
	ret = get_byte(cmd, MESSAGE_ENGINE, &control_state.engine, 1);
	control_state.engine >>= 1;
	control_state.engine = !control_state.engine;

	if (ret) {
		log_err();
		return -1;
	}

	return 0;
}

#define MEG_EXPIRE	500

int update_cylinder_state(struct cylinder_info *cy)
{
	char cmd[16];
	int ret;
	unsigned char res[sizeof(cy->raw)];

	if (!cy->dev.id) {
		return -1;
	}

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, ">%c state;", cy->dev.id);
	ret = get_byte(cmd, MESSAGE_ENC, res, sizeof(res));

	if (ret) {
		log_err();
		return -1;
	}

	cy->var.id = res[0];

	if (cy->dev.id != cy->var.id) {
		log_err();
		log_info("size cy->var:%d\n", sizeof(cy->var));
		return -1;
	}

	if (cy->type == TYPE_N) {
		cy->var.len = 0xffff - (res[1] << 8) - res[2];
		cy->var.speed = -1 * (res[3] << 8) - res[4];
	} else {
		cy->var.len = (res[1] << 8) + res[2];
		cy->var.speed = (res[3] << 8) + res[4];
	}

	cy->var.port = res[5];

	if (sys_ms > cy->meg_delay) {
		cy->meg_delay = ~0x0;
		cy->meg_dir = 0;
	}

	if (cy->var.port & 0x4) {
		cy->meg_delay = sys_ms + MEG_EXPIRE;
		cy->meg_dir = -1;
	}
	if (cy->var.port & 0x8) {
		cy->meg_delay = sys_ms + MEG_EXPIRE;
		cy->meg_dir = 1;
	}

	return 0;
}

inline struct interface_info *get_interface_info(void)
{
	return &control_state;
}

inline struct cylinder_info *get_motion_info(int *arry_size)
{
	*arry_size = NCYLINDER;
	return motion_state;
}

int meg12v_on(int state)
{
	char str[16];

	if (!control_state.dev.id) {
		log_info("No interface board!\n");
		return -1;
	}

	switch (state) {
	case 0:
	case 1:
		sprintf(str, ">%c meg12v %d;", control_state.dev.id, state);
		send_cmd(str);
		break;
	default:
		log_err();
		return -1;
	}

	return 0;
}

int engine_on(int count)
{
	char str[16];

	if (!control_state.dev.id) {
		log_info("No interface board!\n");
		return -1;
	}

	if (count <= 0 || count > 255) {
		log_err();
		return -1;
	}

	sprintf(str, ">%c engine %x;", control_state.dev.id, count);
	send_cmd(str);

	return 0;
};

int megnet(struct cylinder_info *cy, int count)
{
	char str[16];

	if (!cy->dev.id) {
		log_err();
		return -1;
	}

	if (count > 0 && count < 255) {
		sprintf(str, ">%c inc %2x;", cy->dev.id, count);
		if (send_cmd(str)) {
			log_err();
			return -1;
		}
		cy->force = '+';
		return 0;
	}

	if (count < 0 && count > -255) {
		sprintf(str, ">%c dec %2x;", cy->dev.id, abs(count));
		if (send_cmd(str)) {
			log_err();
			return -1;
		}
		cy->force = '-';
		return 0;
	}

	log_info("%s, set %d error\n", __FUNCTION__, count);
	return -1;
}

int set_encoder(struct cylinder_info *cy, int val)
{
	char str[16];

	if (val > 65535 || val < 0) {
		log_err();
		return -1;
	}

	if (!cy->dev.id) {
		log_err();
		return -1;
	}

	if (cy->type == TYPE_N)
		val = 65535 - val;

	sprintf(str, ">%c set %4x;", cy->dev.id, val);
	send_cmd(str);		/*todo: */
	return 0;
}
