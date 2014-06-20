/*
 * graphic.h
 *
 *  Created on: 22.05.2014
 *      Author: embox
 */

#ifndef GRAPHIC_H_
#define GRAPHIC_H_

#include <SDL2/SDL_ttf.h>

extern SDL_Color sdlBlack;
extern SDL_Color sdlWhite;
extern SDL_Color sdlRed;
extern SDL_Color sdlGreen;

/* Init graphic objects */
inline void Graphic_Init(void);
/* Destroy graphic objects and free memory */
inline void Graphic_Destroy(void);
/* Update (redraw) the screen*/
inline void UpdateScreen(double steer_angle, unsigned char direction, unsigned char pwm, unsigned char speed);
/* Render text on the screen */
inline void WriteText(int x, int y, char *text, int size, SDL_Color color);

#endif /* GRAPHIC_H_ */
