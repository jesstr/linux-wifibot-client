/*
 * linux-wifibot-client.h
 *
 *  Created on: 22.05.2014
 *      Author: embox
 */

#ifndef LINUX_WIFIBOT_CLIENT_H_
#define LINUX_WIFIBOT_CLIENT_H_

#define OPT_BUF_SIZE 	16

#define MAX_COMMAND_LENGTH	16
#define LEFT_STEER_POS		600
#define RIGHT_STEER_POS		1100
#define CENTER_STEER_POS	(RIGHT_STEER_POS + LEFT_STEER_POS) / 2
#define SPEED_CHANGE_DELAY_US	10000
#define COMMAND_SEND_DELAY_US	500000
#define JOYSTICK_STEER_TRESHOLD_VALUE	10
#define	JOYSTICK_RUN_TRESHOLD_VALUE 	5
#define	JOYSTICK_RUN_DEADZONE_VALUE 	100
#define	TURRET_HOR_DC_DEADZONE_VALUE 	70
#define	TURRET_HOR_DC_TRESHOLD_VALUE 	2


#define KEYBOARD_SPEED_CONROL_THREAD	1	/* Speed & direstion control with keyboard thread id, */
#define JOYSTICK_SPEED_CONROL_THREAD	3	/* Speed & direstion control with gamepad thread id */

#define FORWARD		1
#define BACKWARD	2
#define LEFT		3
#define RIGHT		4
#define NONE 		0
#define STOP 		0

extern unsigned char run_speed;


#endif /* LINUX_WIFIBOT_CLIENT_H_ */
