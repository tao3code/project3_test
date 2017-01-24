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

static struct cylinder_info motion_state[NUM_CYLINDERS] = {
	[0] {.dev = {.id = '1',.ack = MESSAGE_1},
	     .fix = {.type = TYPE_N,.range = 25,.area = 1},
	     .mea = {.c = 237,.pa = 60,.na = 70,.pv = 110,.nv = -100}},
	[1] {.dev = {.id = '7',.ack = MESSAGE_7},
	     .fix = {.type = TYPE_P,.range = 25,.area = 1},
	     .mea = {.c = 236,.pa = 60,.na = 70,.pv = 110,.nv = -100}},
	[2] {.dev = {.id = '2',.ack = MESSAGE_2},
	     .fix = {.type = TYPE_P,.range = 25,.area = 1},
	     .mea = {.c = 240,.pa = 60,.na = 70,.pv = 50,.nv = -50}},
	[3] {.dev = {.id = '8',.ack = MESSAGE_8},
	     .fix = {.type = TYPE_N,.range = 25,.area = 1},
	     .mea = {.c = 243,.pa = 60,.na = 70,.pv = 50,.nv = -50}},
	[4] {.dev = {.id = '3',.ack = MESSAGE_3},
	     .fix = {.type = TYPE_N,.range = 140,.area = 4},
	     .mea = {.c = 1400,.pa = 60,.na = 60,.pv = 240,.nv = -140}},
	[5] {.dev = {.id = '9',.ack = MESSAGE_9},
	     .fix = {.type = TYPE_P,.range = 140,.area = 4},
	     .mea = {.c = 1427,.pa = 60,.na = 60,.pv = 240,.nv = -140}},
	[6] {.dev = {.id = '4',.ack = MESSAGE_4},
	     .fix = {.type = TYPE_N,.range = 125,.area = 4},
	     .mea = {.c = 1180,.pa = 60,.na = 70,.pv = 200,.nv = -110}},
	[7] {.dev = {.id = 'A',.ack = MESSAGE_A},
	     .fix = {.type = TYPE_P,.range = 125,.area = 4},
	     .mea = {.c = 1150,.pa = 60,.na = 70,.pv = 200,.nv = -110}},
	[8] {.dev = {.id = '5',.ack = MESSAGE_5},
	     .fix = {.type = TYPE_P,.range = 50,.area = 1},
	     .mea = {.c = 500,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
	[9] {.dev = {.id = 'B',.ack = MESSAGE_B},
	     .fix = {.type = TYPE_N,.range = 50,.area = 1},
	     .mea = {.c = 472,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
	[10] {.dev = {.id = '6',.ack = MESSAGE_6},
	      .fix = {.type = TYPE_N,.range = 50,.area = 1},
	      .mea = {.c = 494,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
	[11] {.dev = {.id = 'C',.ack = MESSAGE_C},
	      .fix = {.type = TYPE_P,.range = 50,.area = 1},
	      .mea = {.c = 436,.pa = 60,.na = 60,.pv = 160,.nv = -80}},
};

static struct interface_info control_state = {
	.dev = {.id = '0',.ack = MESSAGE_0},
};

#define CMD_VAR	\
	char cmd[16];	\
	char *msg;	\
	int ret

#define ASSERT_SEND_CMD_OK(cmd_fmt, ...)	\
do {	\
	memset(cmd, 0, sizeof(cmd));	\
	sprintf(cmd, cmd_fmt, __VA_ARGS__); \
	sent_cmd_alloc_response(cmd, &msg);	\
	if (!msg) {				\
		log_err();			\
		return -1;			\
	}	\
} while(0)

#define ASSERT_SCAN_CMD_OK(expected, ...)	\
do {	\
	if (!expected) {		\
		free(msg);	\
		break;		\
	}			\
	ret = sscanf(msg, __VA_ARGS__);	\
	if (ret != expected) {		\
		log_err();	\
		free(msg);	\
		return -1;	\
	}			\
	free(msg);	\
} while (0);

#define ASSERT_DEV_OK(dev)	\
do {	\
	if (!dev.detected) {	\
		log_info("%s, no shuch device \n", __FUNCTION__);	\
		return -1;	\
	}	\
} while(0)

int test_device(struct device *dev)
{
	CMD_VAR;
	char name[16];
	unsigned short h1, h2;

	if (!dev->ack) {
		log_err();
		return -1;
	}
	dev->detected = 0;

	ASSERT_SEND_CMD_OK(">%c report;", dev->id);
	ASSERT_SCAN_CMD_OK(4, dev->ack, name, &dev->detected, &h1, &h2);

	log_info("detect: %s, id: %c\n", name, dev->detected);
	return 0;
}

int update_voltage(void)
{
	CMD_VAR;

	ASSERT_DEV_OK(control_state.dev);
	ASSERT_SEND_CMD_OK(">%c voltage;", control_state.dev.id);
	ASSERT_SCAN_CMD_OK(1, MESSAGE_VOL, &control_state.vol);

	return 0;
}

int update_presure(void)
{
	CMD_VAR;

	ASSERT_DEV_OK(control_state.dev);
	ASSERT_SEND_CMD_OK(">%c presure;", control_state.dev.id);
	ASSERT_SCAN_CMD_OK(1, MESSAGE_AIR, &control_state.air);

	return 0;
}

int update_gyroscope(void)
{
	CMD_VAR;

	ASSERT_DEV_OK(control_state.dev);
	ASSERT_SEND_CMD_OK(">%c gyroscope;", control_state.dev.id);
	ASSERT_SCAN_CMD_OK(7, MESSAGE_GYRO, &control_state.ax,
			   &control_state.ay,
			   &control_state.az,
			   &control_state.thermal,
			   &control_state.gx,
			   &control_state.gy, &control_state.gz);

	return 0;
}

int update_meg12v(void)
{
	CMD_VAR;
	char m12v_n;

	ASSERT_DEV_OK(control_state.dev);
	ASSERT_SEND_CMD_OK(">%c meg12v 3;", control_state.dev.id);
	ASSERT_SCAN_CMD_OK(1, MESSAGE_MEG, &m12v_n);

	control_state.m12v = !m12v_n;
	return 0;
}

int update_engine(void)
{
	CMD_VAR;
	char engine_n;

	ASSERT_DEV_OK(control_state.dev);
	ASSERT_SEND_CMD_OK(">%c engine 0;", control_state.dev.id);
	ASSERT_SCAN_CMD_OK(1, MESSAGE_ENGINE, &engine_n);

	engine_n >>= 1;
	control_state.engine = !engine_n;
	return 0;
}

#define MEG_EXPIRE	500

int update_cylinder_state(struct cylinder_info *cy)
{
	CMD_VAR;
	unsigned short len;
	short int speed;

	ASSERT_DEV_OK(cy->dev);
	ASSERT_SEND_CMD_OK(">%c state;", cy->dev.id);
	ASSERT_SCAN_CMD_OK(4, MESSAGE_ENC, &cy->enc.id, &len, &speed,
			   &cy->port);

	if (cy->dev.id != cy->enc.id) {
		log_err();
		return -1;
	}

	if (cy->fix.type == TYPE_N) {
		cy->enc.len = 0xffff - len;
		cy->speed = -1 * speed;
	} else {
		cy->enc.len = len;
		cy->speed = speed;
	}

	if (sys_ms > cy->meg_delay) {
		cy->meg_delay = ~0x0;
		cy->meg_dir = 0;
	}

	if (cy->port & 0x4) {
		cy->meg_delay = sys_ms + MEG_EXPIRE;
		cy->meg_dir = -1;
	}
	if (cy->port & 0x8) {
		cy->meg_delay = sys_ms + MEG_EXPIRE;
		cy->meg_dir = 1;
	}

	return 0;
}

inline struct interface_info *get_interface_info(void)
{
	return &control_state;
}

inline struct cylinder_info *get_motion_info(void)
{
	return motion_state;
}

int meg12v_on(int state)
{
	CMD_VAR;

	ASSERT_DEV_OK(control_state.dev);

	switch (state) {
	case 0:
	case 1:
		ASSERT_SEND_CMD_OK(">%c meg12v %d;", control_state.dev.id,
				   state);
		ASSERT_SCAN_CMD_OK(0, MESSAGE_OK);
		break;
	default:
		log_err();
		return -1;
	}

	return 0;
}

int engine_on(int count)
{
	CMD_VAR;

	if (count <= 0 || count > 255) {
		log_err();
		return -1;
	}

	ASSERT_DEV_OK(control_state.dev);
	ASSERT_SEND_CMD_OK(">%c engine %x;", control_state.dev.id, count);
	ASSERT_SCAN_CMD_OK(0, MESSAGE_OK);

	return 0;
};

int megnet(struct cylinder_info *cy, int count)
{
	CMD_VAR;

	ASSERT_DEV_OK(cy->dev);

	if (count > 0 && count < 255) {
		ASSERT_SEND_CMD_OK(">%c inc %2x;", cy->dev.id, count);
		ASSERT_SCAN_CMD_OK(0, MESSAGE_OK);
		cy->enc.force = '+';
		return 0;
	}

	if (count < 0 && count > -255) {
		ASSERT_SEND_CMD_OK(">%c dec %2x;", cy->dev.id, abs(count));
		ASSERT_SCAN_CMD_OK(0, MESSAGE_OK);
		cy->enc.force = '-';
		return 0;
	}

	log_info("%s, set %d error\n", __FUNCTION__, count);
	return -1;
}

int set_encoder(struct cylinder_info *cy, int val)
{
	CMD_VAR;

	if (val > 65535 || val < 0) {
		log_err();
		return -1;
	}

	ASSERT_DEV_OK(cy->dev);

	if (cy->fix.type == TYPE_N)
		val = 65535 - val;

	ASSERT_SEND_CMD_OK(">%c set %4x;", cy->dev.id, val);
	ASSERT_SCAN_CMD_OK(0, MESSAGE_OK);

	return 0;
}
