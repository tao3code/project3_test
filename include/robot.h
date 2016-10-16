#ifndef ROBOT_H
#define ROBOT_H

struct cylinder_info {
	char id;
	const char *msg;
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
	const char *msg;
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

int test_robot(void);

int update_voltage(void);
int update_presure(void);
int update_gyroscope(void);
void update_control_state(void);
int meg12v_on(char state);
int engine_on(int count);

int update_cylinder_len(int index);
void update_motion_state(void);

const struct cylinder_info *get_cylinder_info(int index);
inline const struct interface_info *get_interface_info(void); 
inline const struct cylinder_info *get_motion_info(int *count); 

struct cylinder_info *alloc_cylinder_by_id(char id);
struct interface_info *alloc_interface_board(void);

#endif
