/*
 * graphic.c
 *
 *  Created on: 22.05.2014
 *      Author: embox
 */

#include <SDL2/SDL.h>
#include "graphic.h"
#include "linux-wifibot-client.h"

SDL_Surface *sdlCarSurface = NULL;
SDL_Surface *sdlWheelSurface = NULL;
SDL_Surface *sdlArrowSurface = NULL;
SDL_Surface *sdlSpeedometerSurface = NULL;
SDL_Surface *sdlPwmSurface = NULL;
SDL_Surface *sdlMeterArrowSurface = NULL;
SDL_Window *sdlWindow = NULL;
SDL_Renderer *sdlRenderer = NULL;
SDL_Texture *sdlCarTexture = NULL;
SDL_Texture *sdlWheelTexture = NULL;
SDL_Texture *sdlArrowTexture = NULL;
SDL_Texture *sdlSpeedometerTexture = NULL;
SDL_Texture *sdlPwmTexture = NULL;
SDL_Texture *sdlMeterArrowTexture = NULL;
SDL_Texture *sdlPwmArrowTexture = NULL;
SDL_Rect sdlCarDstrect;
SDL_Rect sdlLeftWheelDstrect, sdlRightWheelDstrect;
SDL_Rect sdlLeftArrowDstrect, sdlRightArrowDstrect;
SDL_Rect sdlSpeedometerDstrect, sdlMeterArrowDstrect;
SDL_Rect sdlPwmDstrect, sdlPwmArrowDstrect;;
SDL_Point sdlMeterArrowCenterPoint, sdlPwmArrowCenterPoint;


inline void Graphic_Init(void)
{
	SDL_CreateWindowAndRenderer(640, 480, 0, &sdlWindow, &sdlRenderer);

	SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderPresent(sdlRenderer);

	/* TODO move icons to /usr/share/linux-wifibot-client */

	sdlCarSurface = SDL_LoadBMP( "./img/car_img.bmp" );
	sdlWheelSurface = SDL_LoadBMP( "./img/wheel_img.bmp" );
	sdlArrowSurface = SDL_LoadBMP( "./img/arrow_img.bmp" );
	sdlSpeedometerSurface = SDL_LoadBMP( "./img/speedometer_small_img.bmp" );
	sdlPwmSurface = SDL_LoadBMP( "./img/pwm_small_img.bmp" );
	sdlMeterArrowSurface = SDL_LoadBMP( "./img/meterarrow_small_img.bmp" );

	sdlCarTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlCarSurface);
	sdlWheelTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlWheelSurface);
	sdlArrowTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlArrowSurface);
	sdlSpeedometerTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlSpeedometerSurface);
	sdlPwmTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlPwmSurface);
	sdlMeterArrowTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlMeterArrowSurface);
	sdlPwmArrowTexture = sdlMeterArrowTexture;

	SDL_GetClipRect(sdlCarSurface, &sdlCarDstrect);
	SDL_GetClipRect(sdlWheelSurface, &sdlLeftWheelDstrect);
	SDL_GetClipRect(sdlWheelSurface, &sdlRightWheelDstrect);
	SDL_GetClipRect(sdlArrowSurface, &sdlLeftArrowDstrect);
	SDL_GetClipRect(sdlArrowSurface, &sdlRightArrowDstrect);
	SDL_GetClipRect(sdlSpeedometerSurface, &sdlSpeedometerDstrect);
	SDL_GetClipRect(sdlPwmSurface, &sdlPwmDstrect);
	SDL_GetClipRect(sdlMeterArrowSurface, &sdlMeterArrowDstrect);
	sdlPwmArrowDstrect = sdlMeterArrowDstrect;

	sdlCarDstrect.y = 20;
	sdlCarDstrect.x = 0;

	sdlLeftWheelDstrect.x = sdlCarDstrect.x + 29;
	sdlLeftWheelDstrect.y = sdlCarDstrect.y + 0;
	sdlRightWheelDstrect.x = sdlCarDstrect.x + 152;
	sdlRightWheelDstrect.y = sdlLeftWheelDstrect.y;

	sdlLeftArrowDstrect.x = sdlCarDstrect.x + 19;
	sdlRightArrowDstrect.x = sdlCarDstrect.x + 142;
	sdlLeftArrowDstrect.y = sdlCarDstrect.y + 85;
	sdlRightArrowDstrect.y = sdlLeftArrowDstrect.y;

	sdlPwmDstrect.x = 450;
	sdlPwmDstrect.y = 15;
	sdlSpeedometerDstrect.x = 270;
	sdlSpeedometerDstrect.y = 15;

	sdlMeterArrowDstrect.x = sdlSpeedometerDstrect.x + 74;
	sdlMeterArrowDstrect.y = sdlSpeedometerDstrect.y + 26;
	sdlMeterArrowCenterPoint.x = sdlMeterArrowDstrect.w - 11;
	sdlMeterArrowCenterPoint.y = sdlMeterArrowDstrect.h - 14;

	sdlPwmArrowDstrect.x = sdlPwmDstrect.x + 74;
	sdlPwmArrowDstrect.y = sdlPwmDstrect.y + 26;
	sdlPwmArrowCenterPoint.x = sdlPwmArrowDstrect.w - 11;
	sdlPwmArrowCenterPoint.y = sdlPwmArrowDstrect.h - 14;

	UpdateScreen(0.0, NONE, JOYSTICK_RUN_DEADZONE_VALUE, 0);
}

inline void UpdateScreen(double steer_angle, unsigned char direction, unsigned char pwm, unsigned char speed) {
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlCarTexture, NULL, &sdlCarDstrect);
	SDL_RenderCopy(sdlRenderer, sdlSpeedometerTexture, NULL, &sdlSpeedometerDstrect);
	SDL_RenderCopy(sdlRenderer, sdlPwmTexture, NULL, &sdlPwmDstrect);

	SDL_RenderCopyEx(sdlRenderer, sdlMeterArrowTexture, NULL, &sdlMeterArrowDstrect, ( (double) pwm ) - 135.0, &sdlMeterArrowCenterPoint, 0);
	SDL_RenderCopyEx(sdlRenderer, sdlPwmArrowTexture, NULL, &sdlPwmArrowDstrect, ( (double) speed ) - 135.0, &sdlPwmArrowCenterPoint, 0);

	SDL_RenderCopyEx(sdlRenderer, sdlWheelTexture, NULL, &sdlLeftWheelDstrect, steer_angle, NULL, 0);
	SDL_RenderCopyEx(sdlRenderer, sdlWheelTexture, NULL, &sdlRightWheelDstrect, steer_angle, NULL, SDL_FLIP_HORIZONTAL);
	switch (direction) {
	case FORWARD:
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlRightArrowDstrect, 0, NULL, 0);
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlLeftArrowDstrect, 0, NULL, 0);
		break;
	case BACKWARD:
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlRightArrowDstrect, 0, NULL, SDL_FLIP_VERTICAL);
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlLeftArrowDstrect, 0, NULL, SDL_FLIP_VERTICAL);
		break;
	case NONE:
		break;
	}
	SDL_RenderPresent(sdlRenderer);
}

inline void Graphic_Destroy(void)
{
	SDL_DestroyTexture(sdlCarTexture);
	SDL_DestroyTexture(sdlWheelTexture);
	SDL_DestroyTexture(sdlArrowTexture);
	SDL_DestroyTexture(sdlSpeedometerTexture);
	SDL_DestroyTexture(sdlPwmTexture);
	SDL_DestroyTexture(sdlMeterArrowTexture);
	SDL_DestroyTexture(sdlPwmArrowTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(sdlWindow);
	SDL_FreeSurface(sdlCarSurface);
	SDL_FreeSurface(sdlWheelSurface);
	SDL_FreeSurface(sdlArrowSurface);
	SDL_FreeSurface(sdlSpeedometerSurface);
	SDL_FreeSurface(sdlPwmSurface);
	SDL_FreeSurface(sdlMeterArrowSurface);
}