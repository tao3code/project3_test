#ifndef ROBOT_H
#define ROBOT_H

struct cylinder_info {
	char id;	
	unsigned short len;
	char force;
};
 
struct interface_info {
	unsigned char vol;
	unsigned char pre;

	int short gx;	
	int short gy;	
	int short gz;
	int short temp;
	int short ax;	
	int short ay;	
	int short az;
};	

extern struct cylinder_info motion_state[];	
extern struct interface_info control_state;	

#endif
