/*
 * graphic.h
 *
 *  Created on: 22.05.2014
 *      Author: embox
 */

#ifndef GRAPHIC_H_
#define GRAPHIC_H_

inline void Graphic_Init(void);
inline void Graphic_Destroy(void);
inline void UpdateScreen(double steer_angle, unsigned char direction, unsigned char pwm, unsigned char speed);

#endif /* GRAPHIC_H_ */
