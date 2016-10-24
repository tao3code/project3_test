#ifndef ROBOT_H
#define ROBOT_H

struct cylinder_info {
	char id;
	const char *msg;
	char force;
	char type;
	union {
		volatile unsigned short len;
		struct {
			volatile unsigned char raw[3];
		};
	};
};

struct interface_info {
	char id;
	const char *msg;
	char m12v;
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

int test_robot(void);

int update_voltage(void);
int update_presure(void);
int update_gyroscope(void);
void update_control_state(void);
int meg12v_on(char state);
int engine_on(int count);
int megnet(int index, int count);
int set_encoder(int index, int val);

int update_cylinder_len(int index);
void update_motion_state(void);

const struct cylinder_info *get_cylinder_info(int index);
inline const struct interface_info *get_interface_info(void); 
inline const struct cylinder_info *get_motion_info(int *count); 

struct cylinder_info *alloc_cylinder_by_id(char id);
struct interface_info *alloc_interface_board(void);

#define AIR_THRESHOLD_H		80
#define AIR_THRESHOLD_L		60

#define VOLTAGE_LOW		170	

#endif
