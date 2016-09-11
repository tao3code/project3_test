#ifndef ROBOT_H
#define ROBOT_H

struct cylinder_info {
	char id;
	char force;
	char type;
	union {
		unsigned short len;
		struct {
			unsigned char raw[3];
		};
	};
};

struct interface_info {
	char id;
	unsigned char vol;
	unsigned char air;
	union {
		struct {
			int short gz;
			int short gy;
			int short gx;
			int short thermal;
			int short az;
			int short ay;
			int short ax;
		};
		struct {
			unsigned char raw[14];
		};
	};
};

void test_robot(void);
int update_voltage(void);
int update_presure(void);
int update_gyroscope(void);
int update_cylinder_len(int index);
int update_motion_state(void);

#endif
