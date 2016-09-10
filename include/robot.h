#ifndef ROBOT_H
#define ROBOT_H

struct cylinder_info {
	char *name;
	char id;
	char force;
	union {
		unsigned short len;
		struct {
			unsigned char raw[2];
		};
	};
};

struct interface_info {
	unsigned char vol;
	unsigned char pre;
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

int test_robot(void);
int update_voltage(void);
unsigned char get_voltage(void);

#endif
