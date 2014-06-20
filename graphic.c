/*
 * graphic.c
 *
 *  Created on: 22.05.2014
 *      Author: embox
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "graphic.h"
#include "linux-wifibot-client.h"

SDL_Surface *sdlCarSurface = NULL;
SDL_Surface *sdlWheelSurface = NULL;
SDL_Surface *sdlArrowSurface = NULL;
SDL_Surface *sdlSpeedometerSurface = NULL;
SDL_Surface *sdlPwmSurface = NULL;
SDL_Surface *sdlMeterArrowSurface = NULL;
SDL_Surface *sdlTextSurface = NULL;
SDL_Window *sdlWindow = NULL;
SDL_Renderer *sdlRenderer = NULL;
SDL_Texture *sdlCarTexture = NULL;
SDL_Texture *sdlWheelTexture = NULL;
SDL_Texture *sdlArrowTexture = NULL;
SDL_Texture *sdlSpeedometerTexture = NULL;
SDL_Texture *sdlPwmTexture = NULL;
SDL_Texture *sdlMeterArrowTexture = NULL;
SDL_Texture *sdlPwmArrowTexture = NULL;
SDL_Texture *sdlTextTexture = NULL;
SDL_Rect sdlCarDstrect;
SDL_Rect sdlLeftWheelDstrect, sdlRightWheelDstrect;
SDL_Rect sdlLeftArrowDstrect, sdlRightArrowDstrect;
SDL_Rect sdlSpeedometerDstrect, sdlMeterArrowDstrect;
SDL_Rect sdlPwmDstrect, sdlPwmArrowDstrect;
SDL_Rect sdlTextDstrect;
SDL_Point sdlMeterArrowCenterPoint, sdlPwmArrowCenterPoint;

SDL_Color sdlBlack = {0, 0, 0};
SDL_Color sdlWhite = {255, 255, 255};
SDL_Color sdlRed = {255, 0, 0};
SDL_Color sdlGreen = {0, 255, 0};

/* Init graphic objects */
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

	WriteText(5, 250, "You win!", 26, sdlBlack); // debug
	WriteText(5, 300, "You win!", 26, sdlRed); // debug

	SDL_RenderCopy(sdlRenderer, sdlCarTexture, NULL, &sdlCarDstrect);

	UpdateScreen(0.0, NONE, JOYSTICK_RUN_DEADZONE_VALUE, 0);
}

/* Update (redraw) the screen*/
inline void UpdateScreen(double steer_angle, unsigned char direction, unsigned char pwm, unsigned char speed) {

	SDL_RenderClear(sdlRenderer); // trick: we don't need clear the render while updated textures are placed on updated background.

	/* Draw meters area */
	SDL_RenderCopy(sdlRenderer, sdlSpeedometerTexture, NULL, &sdlSpeedometerDstrect);
	SDL_RenderCopy(sdlRenderer, sdlPwmTexture, NULL, &sdlPwmDstrect);

	SDL_RenderCopyEx(sdlRenderer, sdlMeterArrowTexture, NULL, &sdlMeterArrowDstrect, ( (double) pwm ) - 135.0, &sdlMeterArrowCenterPoint, 0);
	SDL_RenderCopyEx(sdlRenderer, sdlPwmArrowTexture, NULL, &sdlPwmArrowDstrect, ( (double) speed ) - 135.0, &sdlPwmArrowCenterPoint, 0);

	/* Draw car scheme */
	SDL_RenderCopy(sdlRenderer, sdlCarTexture, NULL, &sdlCarDstrect);

	SDL_RenderCopyEx(sdlRenderer, sdlWheelTexture, NULL, &sdlLeftWheelDstrect, steer_angle, NULL, 0);
	SDL_RenderCopyEx(sdlRenderer, sdlWheelTexture, NULL, &sdlRightWheelDstrect, steer_angle, NULL, SDL_FLIP_HORIZONTAL);

	switch (direction) {
	case FORWARD:
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlRightArrowDstrect, 0, NULL, 0);
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlLeftArrowDstrect, 0, NULL, 0);
		//WriteText(5, 250, "FORWARD!", 26, sdlBlack); // debug
		break;
	case BACKWARD:
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlRightArrowDstrect, 0, NULL, SDL_FLIP_VERTICAL);
		SDL_RenderCopyEx(sdlRenderer, sdlArrowTexture, NULL, &sdlLeftArrowDstrect, 0, NULL, SDL_FLIP_VERTICAL);
		//WriteText(5, 250, "BACKWARD!", 26, sdlBlack); // debug
		break;
	case NONE:
		break;
	}

	SDL_RenderCopy(sdlRenderer, sdlTextTexture, NULL, &sdlTextDstrect);

	SDL_RenderPresent(sdlRenderer);
}


inline void RenderImages()
{

}


inline void RenderText()
{

}

/* Destroy graphic objects and free memory */
inline void Graphic_Destroy(void)
{
	SDL_DestroyTexture(sdlCarTexture);
	SDL_DestroyTexture(sdlWheelTexture);
	SDL_DestroyTexture(sdlArrowTexture);
	SDL_DestroyTexture(sdlSpeedometerTexture);
	SDL_DestroyTexture(sdlPwmTexture);
	SDL_DestroyTexture(sdlMeterArrowTexture);
	SDL_DestroyTexture(sdlPwmArrowTexture);
	SDL_DestroyTexture(sdlTextTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(sdlWindow);
	SDL_FreeSurface(sdlCarSurface);
	SDL_FreeSurface(sdlWheelSurface);
	SDL_FreeSurface(sdlArrowSurface);
	SDL_FreeSurface(sdlSpeedometerSurface);
	SDL_FreeSurface(sdlPwmSurface);
	SDL_FreeSurface(sdlMeterArrowSurface);
	SDL_FreeSurface(sdlTextSurface);
}

/* Render text on the screen */
inline void WriteText(int x, int y, char *text, int size, SDL_Color color)
{
	/*
	SDL_Surface surface;
	SDL_Texture texture;
	SDL_Rect rect;
	*/
    TTF_Font *font = TTF_OpenFont("./font/FreeSans.ttf", size); // Загружаем шрифт по заданному адресу размером sz
    if( font == NULL ) {
    	printf("TTF_OpenFont: %s \n", SDL_GetError());
    	return;
    }

    if ( (sdlTextSurface = TTF_RenderText_Blended(font, text, color)) == NULL ) { // Переносим на поверхность текст с заданным шрифтом и цветом
    	printf("TTF_RenderText_Blended: %s \n", SDL_GetError());
    	return;
    }

    if ( (sdlTextTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlTextSurface)) == NULL ) {
    	printf("SDL_CreateTextureFromSurface: %s \n", SDL_GetError());
    	return;
    }

    SDL_GetClipRect(sdlTextSurface, &sdlTextDstrect);
    sdlTextDstrect.x = x;
    sdlTextDstrect.y = y;

    if ( (SDL_RenderCopy(sdlRenderer, sdlTextTexture, NULL, &sdlTextDstrect)) != 0 ) {
    	printf("SDL_RenderCopy: %s \n", SDL_GetError());
    	return;
    }

    TTF_CloseFont(font);
}
