#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <pthread.h>

#define MAX_COMMAND_LENGTH	16
#define LEFT_STEER_POS		600
#define RIGHT_STEER_POS		1100
#define CENTER_STEER_POS	(RIGHT_STEER_POS + LEFT_STEER_POS) / 2
#define SPEED_CHANGE_DELAY_US	10000
#define COMMAND_SEND_DELAY_US	500000

#define KEYBOARD_SPEED_CONROL_THREAD	1	/* Speed & direstion control with keyboard thread id, */
#define JOYSTICK_SPEED_CONROL_THREAD	3	/* Speed & direstion control with gamepad thread id */

#define FORWARD		1
#define BACKWARD	2
#define NONE 		0

volatile unsigned char input_mode = 0; 	/* 1 - joystick, 0 - keyboard */

#define IS_ANALOG_INPUT		input_mode == 1
#define IS_DIGITAL_INPUT	input_mode == 0

#define SET_ANALOG_INPUT	input_mode = 1
#define SET_DIGITAL_INPUT	input_mode = 0

char command1[MAX_COMMAND_LENGTH];	/* Independent command buffers for safe using in threads */
char command2[MAX_COMMAND_LENGTH];

char command3[MAX_COMMAND_LENGTH];	/* Command buffer for using in non-thread functions */

unsigned char run_speed = 0;	/* Default forward and backward run speed value, percent */
unsigned char run_time = 20; 	/* Default forward and backward run time value, 1=100ms */
volatile char *run_direction = "F";		/* Default forward or backward run direction, "F" or "B" */
volatile unsigned char chassis_state = NONE;		/* Marker of current chassis state: NONE, FORWARD or BACKWARD */
volatile unsigned short steer_pos = CENTER_STEER_POS;	/* Default steering servo position, us */

SDL_Event event;	/* The event structure */
SDL_Surface *sdlCarSurface = NULL;
SDL_Surface *sdlWheelSurface = NULL;
SDL_Surface *sdlArrowSurface = NULL;
SDL_Surface *sdlSpeedometerSurface = NULL;
SDL_Surface *sdlMeterarrowSurface = NULL;
SDL_Window *sdlWindow = NULL;
SDL_Renderer *sdlRenderer = NULL;
SDL_Texture *sdlCarTexture = NULL;
SDL_Texture *sdlWheelTexture = NULL;
SDL_Texture *sdlArrowTexture = NULL;
SDL_Texture *sdlSpeedometerTexture = NULL;
SDL_Texture *sdlMeterarrowTexture = NULL;
SDL_Rect sdlCarDstrect;
SDL_Rect sdlLeftWheelDstrect, sdlRightWheelDstrect;
SDL_Rect sdlLeftArrowDstrect, sdlRightArrowDstrect;
SDL_Rect sdlSpeedometerDstrect, sdlMeterarrowDstrect;
SDL_Point sdlMeterarrowCenterPoint;

pthread_t thread1, thread2, thread3, joystick_thread, telemetry_thread;
unsigned char thread_id;

int sock;


void UpdateScreen(double steer_angle, unsigned char direction, unsigned char speed);


void * thread_command_send(void *arg)
{
	char *command;
	unsigned char id = * (unsigned char *) arg;
	Uint8 *keystate = SDL_GetKeyboardState(NULL);

	do {
		switch ( id ) {
			case 1:
				sprintf(command1, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
				command = command1;
				break;
			case 2:
				sprintf(command2, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
				command = command2;
				break;
		}
		puts("sent");
		send(sock, command, strlen(command), 0);
		usleep(COMMAND_SEND_DELAY_US);
		/* Exit thread if key was released*/
		if ( ( !(keystate[SDL_SCANCODE_UP]) && id == 1) ||
			 ( !(keystate[SDL_SCANCODE_DOWN]) && id == 2) )
			pthread_exit(NULL);
	}
	while (( keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_DOWN] ));
	puts("exit 2");
	pthread_exit(NULL);
}

void * thread_joystick_control(void *arg)
{
	char *command;
	//unsigned char id = * (unsigned char *) arg;

	while (1) {
		if ( IS_ANALOG_INPUT && run_speed != 0 ) {
			sprintf(command3, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
			command = command3;
			puts("joystick_sent");
			send(sock, command, strlen(command), 0);
			usleep(COMMAND_SEND_DELAY_US);
		}
	}
}

void * thread_speed_change(void *arg)
{
	Uint8 *keystate = SDL_GetKeyboardState( NULL );

	do {
	if ( keystate[SDL_SCANCODE_PAGEUP] )
		if ( run_speed < 255 ) {
			run_speed++;
			usleep(SPEED_CHANGE_DELAY_US);
		}
	if ( keystate[SDL_SCANCODE_PAGEDOWN] )
		if ( run_speed > 0 ) {
			run_speed--;
			usleep(SPEED_CHANGE_DELAY_US);
		}
	}
	while ( keystate[SDL_SCANCODE_PAGEUP] || keystate[SDL_SCANCODE_PAGEDOWN] );
	pthread_exit(NULL);
}

void * thread_telemetry(void *arg)
{
	unsigned short bytes_read;
	char buf[16];

	while((bytes_read = recv(sock, buf, 16, 0)) > 0 ) {
     	if(bytes_read <= 0) break;
        printf("RX: %.*s", bytes_read, buf); // debug
    }
    close(sock);
    printf("disconnected\n");
	pthread_exit(NULL);
}

void Quit(void) {
	pthread_cancel(joystick_thread);
	SDL_DestroyTexture(sdlCarTexture);
	SDL_DestroyTexture(sdlWheelTexture);
	SDL_DestroyTexture(sdlArrowTexture);
	SDL_DestroyTexture(sdlSpeedometerTexture);
	SDL_DestroyTexture(sdlMeterarrowTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(sdlWindow);
	SDL_FreeSurface(sdlCarSurface);
	SDL_FreeSurface(sdlWheelSurface);
	SDL_FreeSurface(sdlArrowSurface);
	SDL_FreeSurface(sdlSpeedometerSurface);
	SDL_FreeSurface(sdlMeterarrowSurface);

	SDL_Quit();
}

void UpdateScreen(double steer_angle, unsigned char direction, unsigned char speed) {
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlCarTexture, NULL, &sdlCarDstrect);
	SDL_RenderCopy(sdlRenderer, sdlSpeedometerTexture, NULL, &sdlSpeedometerDstrect);

	SDL_RenderCopyEx(sdlRenderer, sdlMeterarrowTexture, NULL, &sdlMeterarrowDstrect, ( (double) speed ) - 135.0, &sdlMeterarrowCenterPoint, 0);

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

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	int result;

	//Инициализировать SDL
	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}

	SDL_CreateWindowAndRenderer(640, 480, NULL, &sdlWindow, &sdlRenderer);

	SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderPresent(sdlRenderer);

	sdlCarSurface = SDL_LoadBMP( "car_img.bmp" );
	sdlWheelSurface = SDL_LoadBMP( "wheel_img.bmp" );
	sdlArrowSurface = SDL_LoadBMP( "arrow_img.bmp" );
	sdlSpeedometerSurface = SDL_LoadBMP( "speedometer_small_img.bmp" );
	sdlMeterarrowSurface = SDL_LoadBMP( "meterarrow_small_img.bmp" );

	sdlCarTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlCarSurface);
	sdlWheelTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlWheelSurface);
	sdlArrowTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlArrowSurface);
	sdlSpeedometerTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlSpeedometerSurface);
	sdlMeterarrowTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlMeterarrowSurface);

	SDL_GetClipRect(sdlCarSurface, &sdlCarDstrect);
	SDL_GetClipRect(sdlWheelSurface, &sdlLeftWheelDstrect);
	SDL_GetClipRect(sdlWheelSurface, &sdlRightWheelDstrect);
	SDL_GetClipRect(sdlArrowSurface, &sdlLeftArrowDstrect);
	SDL_GetClipRect(sdlArrowSurface, &sdlRightArrowDstrect);
	SDL_GetClipRect(sdlSpeedometerSurface, &sdlSpeedometerDstrect);
	SDL_GetClipRect(sdlMeterarrowSurface, &sdlMeterarrowDstrect);

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

	sdlSpeedometerDstrect.x = 270;
	sdlSpeedometerDstrect.y = 15;

	sdlMeterarrowDstrect.x = sdlSpeedometerDstrect.x + 74;
	sdlMeterarrowDstrect.y = sdlSpeedometerDstrect.y + 26;
	sdlMeterarrowCenterPoint.x = sdlMeterarrowDstrect.w - 11;
	sdlMeterarrowCenterPoint.y = sdlMeterarrowDstrect.h - 14;

	UpdateScreen(0.0, NONE, run_speed);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
       perror("socket");
       return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425); // или любой другой порт...
    //addr.sin_addr.s_addr = inet_addr("192.168.1.121");
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    puts("Connecting...");

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 2;
    }

    puts("Connected.");
	puts("press ENTER to exit");

	/* Telemetry thread creating */
	result = pthread_create(&telemetry_thread, NULL, thread_telemetry, NULL);
	if (result != 0) {
		perror("Creating the thread");
		return 1;
	}

	//Check for joysticks
	if( SDL_NumJoysticks() < 1 ) {
		printf( "Warning: No joysticks connected!\n" );
	}
	else {
		//Load joystick
		if( SDL_JoystickOpen( 0 ) == NULL ) {
			printf( "Warning: Unable to open game controller!\n" );
			//SDL_JoystickEventState(SDL_ENABLE);
		}
	}

	/* Joystick control thread creating */
	result = pthread_create(&joystick_thread, NULL, thread_joystick_control, NULL);
	if (result != 0) {
		perror("Creating the thread");
		return 1;
	}

	while(1) {
	   	if( SDL_PollEvent( &event ) ) {
	        if( event.type == SDL_KEYDOWN ) {
	        	/* We don't use keybord repeat events */
	        	if ( !event.key.repeat ) {
	            switch( event.key.keysym.sym ) {
	                case SDLK_UP:
	                	run_direction = "F";
	                	chassis_state = FORWARD;
	                	sprintf(command1, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
	                	puts("UP");
	                	thread_id = 1;
	                	result = pthread_create(&thread1, NULL, thread_command_send, &thread_id);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	SET_DIGITAL_INPUT;
	                	//UpdateScreen(0.0, FORWARD, run_speed);
	                	break;
	                case SDLK_DOWN:
	                	run_direction = "B";
	                	chassis_state =BACKWARD;
	                	sprintf(command2, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
	                	puts("DOWN");
	                	thread_id = 2;
	                	result = pthread_create(&thread2, NULL, thread_command_send, &thread_id);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	SET_DIGITAL_INPUT;
	                	//UpdateScreen(0.0, BACKWARD, run_speed);
	                	break;
	                case SDLK_LEFT:
	                	steer_pos = LEFT_STEER_POS;
	                	sprintf(command3, "steer=%d\n", steer_pos);
	                	send(sock, command3, strlen(command3), 0);
	                	//DrawCarScheme(-30.0);
	                	//UpdateScreen(-30.0, NONE, run_speed);
	                	puts("LEFT");
	                	break;
	                case SDLK_RIGHT:
	                	steer_pos = RIGHT_STEER_POS;
	                	sprintf(command3, "steer=%d\n", steer_pos);
	                	//command3 = strbuf;
	                	send(sock, command3, strlen(command3), 0);
	                	//DrawCarScheme(30.0);
	                	//UpdateScreen(30.0, NONE, run_speed);
	                	puts("RIGHT");
	                	break;
	                case SDLK_PAGEUP:
	                case SDLK_PAGEDOWN:
	                	result = pthread_create(&thread3, NULL, thread_speed_change, NULL);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	break;
	                case SDLK_RETURN:
	                	Quit();
	                	return 0;
	                	break;
	                default:
	                	break;
	            	}
	        	}
	        }
			else if( event.type == SDL_KEYUP ) {
				/* We don't use keybord repeat events */
				if ( !event.key.repeat ) {
	            switch( event.key.keysym.sym ) {
	                case SDLK_UP:
	                	chassis_state = NONE;
	                	puts("STOP");
	                	//UpdateScreen(0.0, NONE, run_speed);
	                	break;
	                case SDLK_DOWN:
	                	chassis_state = NONE;
	                	puts("STOP");
	                	//UpdateScreen(0.0, NONE, run_speed);
	                	break;
	                case SDLK_LEFT:
	                case SDLK_RIGHT:
	                	steer_pos = CENTER_STEER_POS;
	                	sprintf(command3, "steer=%d\n", steer_pos);
	                	send(sock, command3, strlen(command3), 0);
	                	//DrawCarScheme(0.0);
	                	//UpdateScreen(0.0, NONE, run_speed);
	                	puts("CENTER");
	                	break;
	                case SDLK_PAGEUP:
	                case SDLK_PAGEDOWN:
	                	result = pthread_cancel(thread3);
	                	if (result != 0) {
	                		perror("Cancelling the thread");
	                		return 1;
	                	}
	                	break;
	                default:
	                	break;
	            	}
				}
	        }
	        else if( event.type == SDL_QUIT ) {
	        	Quit();
	  			return 0;
	        }
	        else if( event.type ==  SDL_JOYAXISMOTION ) {
	        	//Motion on controller 0
	        	if( event.jaxis.which == 0 ) {
	        		switch (event.jaxis.axis) {
	        		/* Steering axis */
	        		case 0:
	        			steer_pos = CENTER_STEER_POS + ((( RIGHT_STEER_POS - LEFT_STEER_POS ) / 2 ) * \
	        						event.jaxis.value ) / 32768;
	        			sprintf(command3, "steer=%d\n", steer_pos);
	        			send(sock, command3, strlen(command3), 0);
	        			break;
	        		/* Speed & direction axis */
	        		case 1:
	        			if ( event.jaxis.value < 0 ) {
	        				run_direction = "F";
	        				chassis_state = FORWARD;
	        				run_speed = (( 0 - event.jaxis.value )  * 255 ) / 32768;
	        				printf("%d\n",0 - event.jaxis.value );
	        			}
	        			else {
	        				run_direction = "B";
	        				chassis_state = BACKWARD;
	        				run_speed = ((event.jaxis.value )  * 255 ) / 32767;
	        				printf("%d\n", event.jaxis.value );
	        			}
	        			sprintf(command3, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
	        			send(sock, command3, strlen(command3), 0);
	        			SET_ANALOG_INPUT;
	        			break;
	        		/* Turret axis */
	        		case 2:
	        			break;
	        		/* Turret axis */
	        		case 3:
	        			break;
	        		}
	        	}
	        }
	        else if( event.type ==  SDL_JOYBUTTONDOWN ) {
	        	//Motion on controller 0
	        	if( event.jaxis.which == 0 ) {

	        	}
	        }
	    }
	UpdateScreen((double)((0 - CENTER_STEER_POS + steer_pos) / 8), chassis_state, run_speed);
	}
return 0;
}
