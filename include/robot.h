#ifndef ROBOT_H
#define ROBOT_H

struct device {
	char id;
	const char *ack;
};

struct cylinder_info {
	struct device dev;

	const struct fix_info {
		const char type;
		unsigned short range;
		short int area;
	} fix;

	const struct measured_info {
		unsigned short c;
		unsigned char pa;
		unsigned char na;
		short int pv;
		short int nv;
	} mea;

	struct cy_tag {
		volatile unsigned char id;
		char force;
		char inactive;
		volatile unsigned short len;
	} enc;

	volatile unsigned char port;
	volatile short int speed;

	unsigned long meg_delay;
	int meg_dir;

	volatile unsigned char raw[6];
};

struct interface_info {
	struct device dev;
	volatile unsigned char m12v;
	volatile unsigned char engine;
	volatile unsigned char vol;
	volatile unsigned char air;
	union {
		struct {
			volatile int short gz;
			volatile int short gy;
			volatile int short gx;
			volatile int short thermal;
			volatile int short az;
			volatile int short ay;
			volatile int short ax;
		};
		struct {
			volatile unsigned char raw[14];
		};
	};
};

inline struct interface_info *get_interface_info(void);
inline struct cylinder_info *get_motion_info(void);
int test_device(struct device *dev);

int update_voltage(void);
int update_meg12v(void);
int update_engine(void);
int update_presure(void);
int update_gyroscope(void);
int meg12v_on(int state);
int engine_on(int count);

int update_cylinder_state(struct cylinder_info *cy);
int set_encoder(struct cylinder_info *cy, int val);
int megnet(struct cylinder_info *cy, int count);

#define AIR_THRESHOLD_H		80
#define AIR_THRESHOLD_L		70
#define AIR_THRESHOLD_OFF	60

#define VOLTAGE_LOW		170
#define VOLTAGE_OVE		200

#define ENCODER_OFFSET		128

#define NUM_CYLINDERS		12

#endif
