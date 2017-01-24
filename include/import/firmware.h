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
#define ID_IF_BOARD			'0'
#define ID_R_WAIST_A			'1'
#define ID_R_WAIST_B			'2'
#define ID_R_TIGHT			'3'
#define ID_R_CALF			'4'
#define ID_R_FOOT_A			'5'
#define ID_R_FOOT_B			'6'
#define ID_L_WAIST_A			'7'
#define ID_L_WAIST_B			'8'
#define ID_L_TIGHT			'9'
#define ID_L_CALF			'A'
#define ID_L_FOOT_A			'B'
#define ID_L_FOOT_B			'C'

/* message for CMD_REPORT */
#ifdef BUILD_FRMWARE
#define DATA16	"\x8\x8"
#define DATA8	"\x8"
#define NAME(X) #X
#define ID(X)   #X
#else
#define DATA16	"%hx"
#define DATA8	"%hhx"
#define NAME(X)	"%[^,]"
#define ID(X)   "%c"
#endif

#define MESSAGE(name, id)	NAME(name)	\
				", ID:" ID(id)	\
				" FAIL:" DATA16	\
				" SUM:" DATA16 "\r\n"

#define MESSAGE_0	MESSAGE(IF board, 0)

#define MESSAGE_1	MESSAGE(R wrist back, 1)
#define MESSAGE_2	MESSAGE(R wrist front, 2)
#define MESSAGE_3	MESSAGE(R thigh, 3)
#define MESSAGE_4	MESSAGE(R calf, 4)
#define MESSAGE_5	MESSAGE(R foot OUTER, 5)
#define MESSAGE_6	MESSAGE(R foot INNER, 6)

#define MESSAGE_7	MESSAGE(L wrist back, 7)
#define MESSAGE_8	MESSAGE(L wrist front, 8)
#define MESSAGE_9	MESSAGE(L thigh, 9)
#define MESSAGE_A	MESSAGE(L calf, A)
#define MESSAGE_B	MESSAGE(L foot OUTER, B)
#define MESSAGE_C	MESSAGE(L foot INNER, C)

#define MESSAGE_ERR		"ERR:" DATA8 "\r\n"
#define MESSAGE_VOL		"VOL:" DATA8 "\r\n"
#define MESSAGE_AIR		"AIR:" DATA8 "\r\n"
#define MESSAGE_GYRO	"ACC_XYZ:" DATA16 " " DATA16 " " DATA16 ", " \
			"THERMAL:" DATA16 ", "	\
		  	"GYR_XYZ:" DATA16 " " DATA16 " " DATA16 "\r\n"
#define MESSAGE_ENC	"ENC_CSP(" DATA8 "):"	\
			DATA16 " " DATA16 " " DATA8 "\r\n"
#define MESSAGE_OK	"DONE\n\r"
#define MESSAGE_MEG	"MEG12V:" DATA8 "\r\n"
#define MESSAGE_ENGINE	"ENGINE:" DATA8 "\r\n"

/* return walue for CMD_GETERR */
#define ENOERR	0
#define ENOCMD	1
#define ETIMER	2
#define EUART	3
#define EBUSY	4		/* last command is still running */

#endif
