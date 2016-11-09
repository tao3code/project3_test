/* This file is designed both for firmware and up-level-software,
   describing how to use the robot.
   Control computer can send command via serial port of robot.
   For example:
         >[ID] [cmd] [arg1] [arg2] ...
   It is totally ASCII strings, the question is how to update
   firmware and up-level-software
   
   Author: Yuhong tao */

#ifndef PROJECT3_H
#define PROJECT3_H

/*Valid	IDs */
#define ID_IF_BOARD				'0'
#define ID_R_WAIST_A			'1'
#define ID_R_WAIST_B			'2'
#define ID_R_TIGHT				'3'
#define ID_R_CALF				'4'
#define ID_R_FOOT_A				'5'
#define ID_R_FOOT_B				'6'
#define ID_L_WAIST_A			'7'
#define ID_L_WAIST_B			'8'
#define ID_L_TIGHT				'9'
#define ID_L_CALF				'A'
#define ID_L_FOOT_A				'B'
#define ID_L_FOOT_B				'C'	

/* message for CMD_REPORT */
#define MESSAGE_0	"IF board, ID:0 FAIL:\x8\x8 SUM:\x8\x8\n\r"

#define MESSAGE_1	"R wrist back, ID:1 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_2	"R wrist front, ID:2 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_3	"R thigh, ID:3 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_4	"R calf, ID:4 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_5	"R foot OUTER, ID:5 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_6	"R foot INNER, ID:6 FAIL:\x8\x8 SUM:\x8\x8\n\r"

#define MESSAGE_7	"L wrist back, ID:7 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_8	"L wrist front, ID:8 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_9	"L thigh, ID:9 FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_A	"L calf, ID:A FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_B	"L foot OUTER, ID:B FAIL:\x8\x8 SUM:\x8\x8\n\r"
#define MESSAGE_C	"L foot INNER, ID:C FAIL:\x8\x8 SUM:\x8\x8\n\r"

#define MESSAGE_ERR		"ERR:\x8\n\r"
#define MESSAGE_VOL		"VOL:\x8\n\r"	
#define MESSAGE_AIR		"AIR:\x8\n\r"
#define MESSAGE_GYRO	"ACC_XYZ:\x8\x8 \x8\x8 \x8\x8, " \
					  	"THERMAL:\x8\x8, "				\
					  	"GYR_XYZ:\x8\x8 \x8\x8 \x8\x8\n\r"
#define MESSAGE_ENC		"ENC(\x8):\x8\x8\n\r"
#define MESSAGE_OK		"DONE\n\r"
#define MESSAGE_MEG		"MEG12V:\x8\n\r"
/* return walue for CMD_GETERR */
#define ENOERR	0
#define ENOCMD	1
#define ETIMER	2
#define EUART	3
#define EBUSY	4		/* last command is still running */

#endif