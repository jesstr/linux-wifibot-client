#include <stdio.h>
//#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include "graphic.h"

#include "linux-wifibot-client.h"

volatile unsigned char input_mode = 0; 	/* 1 - joystick, 0 - keyboard */

#define IS_ANALOG_INPUT		input_mode == 1
#define IS_DIGITAL_INPUT	input_mode == 0

#define SET_ANALOG_INPUT	input_mode = 1
#define SET_DIGITAL_INPUT	input_mode = 0

#define ABS(x)	( (x) < 0 ? -(x) : (x) )

char single_command[MAX_COMMAND_LENGTH];	/* Command buffer for using in non-thread functions */

unsigned char run_speed = JOYSTICK_RUN_DEADZONE_VALUE;	/* Default forward and backward run speed value, percent */
unsigned char run_time = 20; 	/* Default forward and backward run time value, 1=100ms */
volatile char *run_direction = "F";		/* Default forward or backward run direction, "F" or "B" */
volatile unsigned char chassis_state = NONE;		/* Marker of current chassis state: NONE, FORWARD or BACKWARD */
volatile unsigned char turret_state = STOP;
volatile unsigned short steer_pos = CENTER_STEER_POS;	/* Default steering servo position, us */

pthread_t thread, joystick_thread, telemetry_thread;

unsigned char turn_speed = TURRET_HOR_DC_DEADZONE_VALUE;	/* Default forward and backward run speed value, percent */
unsigned char turn_time = 5; 	/* Default forward and backward run time value, 1=100ms */
volatile char *turn_direction = "R";		/* Default forward or backward run direction, "F" or "B" */

unsigned char thread_id;

int sock;

SDL_Event event;	/* The event structure */


void * thread_command_send(void *arg)
{
	char command[MAX_COMMAND_LENGTH];
	unsigned char id = * (unsigned char *) arg;
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);

	while (1) {
		switch (id) {
		case 1:
			sprintf(command, "run=F,%d,%d\n", run_speed, run_time);
			break;
		case 2:
			sprintf(command, "run=B,%d,%d\n", run_speed, run_time);
			break;
		}

		puts("sent"); //debug
		printf("id: %d",id); //debug

		send(sock, command, strlen(command), 0);
		usleep(COMMAND_SEND_DELAY_US);

		/* Exit thread if key was released*/
		if ( ( !(keystate[SDL_SCANCODE_UP]) && id == 1) ||
			 ( !(keystate[SDL_SCANCODE_DOWN]) && id == 2) )
			pthread_exit(NULL);
	}
}

void * thread_joystick_control(void *arg)
{
	char command[MAX_COMMAND_LENGTH];

	while (1) {
		if ( IS_ANALOG_INPUT ) {
			if ( run_speed > JOYSTICK_RUN_DEADZONE_VALUE + JOYSTICK_RUN_TRESHOLD_VALUE ) {
				sprintf(command, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
				puts("joystick_run_sent"); //debug
				send(sock, command, strlen(command), 0);
			}
			if ( turn_speed > TURRET_HOR_DC_DEADZONE_VALUE + TURRET_HOR_DC_TRESHOLD_VALUE ) {
				sprintf(command, "turhdc=%s,%d,%d\n", turn_direction, turn_speed, turn_time);
				puts("turret_turn_sent"); //debug
				send(sock, command, strlen(command), 0);
			}
			usleep(COMMAND_SEND_DELAY_US);
		}
	}
}

void * thread_speed_change(void *arg)
{
	unsigned char id = * (unsigned char *) arg;
	const Uint8 *keystate = SDL_GetKeyboardState( NULL );

	while (1) {
		if ( keystate[SDL_SCANCODE_PAGEUP] )
			if ( run_speed < 255 ) {
				run_speed++;
			}
		if ( keystate[SDL_SCANCODE_PAGEDOWN] )
			if ( run_speed > JOYSTICK_RUN_DEADZONE_VALUE ) {
				run_speed--;
			}
		usleep(SPEED_CHANGE_DELAY_US);

		/* Exit thread if key was released */
		if ( ( !(keystate[SDL_SCANCODE_PAGEUP]) && id == 1) ||
			 ( !(keystate[SDL_SCANCODE_PAGEDOWN]) && id == 2) ) {
			pthread_exit(NULL);
		}
	}
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


	SDL_Quit();
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	int opt, result;

	char dest_addr[OPT_BUF_SIZE + 1] = "127.0.0.1";
	char dest_port[OPT_BUF_SIZE + 1] = "3425";
	unsigned char joystickID = 0;	/* Default joystick Id */

	/* Init SDL */
	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}

	/* Init SDL_ttf */
	if( TTF_Init() != 0 )
	{
		return 1;
	}

	while (-1 != (opt = getopt(argc, argv, "hJj:i:p:"))) {
		switch (opt) {
		/* Show help */
		case '?':
		case 'h':
			puts("help: ");
			return 0;
		/* Show available joysticks */
		case 'J':
			/* Check for joysticks */
			if( SDL_NumJoysticks() < 1 ) {
				printf( "Joysticks discovering: No joysticks connected!\n" );
			}
			else {
				printf( "Joysticks discovering: %d joysticks found!\n", SDL_NumJoysticks());
				int i;
				printf("ID  DEVICE NAME\n");
				for (i = 0; i < SDL_NumJoysticks(); i++) {
					printf( "%d - %s\n", i, SDL_JoystickNameForIndex(i));
				}
			}
			return 0;
			break;
		/* Choise joystick by id */
		case 'j':
			joystickID = atoi(optarg);
			break;
		/* Set destination ip addres */
		case 'i':
			snprintf(dest_addr, OPT_BUF_SIZE, "%s", optarg);
			break;
		/* Set destination port */
		case 'p':
			snprintf(dest_port, OPT_BUF_SIZE, "%s", optarg);
			break;
		default:
			break;
		}
	}

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
       perror("Socket");
       return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(dest_addr);
    addr.sin_port = htons(atoi(dest_port));

    printf("Connecting %s:%s...\n", dest_addr, dest_port);

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect");
        return 2;
    }

    puts("Connected!");
	puts("press ENTER to exit");

	/* Telemetry thread creating */
	result = pthread_create(&telemetry_thread, NULL, thread_telemetry, NULL);
	if (result != 0) {
		perror("Creating the thread");
		return 1;
	}

	/* Check for joysticks */
	if( SDL_NumJoysticks() < 1 ) {
		printf( "Warning: No joysticks connected!\n" );
	}
	else {
		/* Open joystick */
		if( SDL_JoystickOpen( joystickID ) == NULL ) {
			printf( "Warning: Unable to open game controller!\n" );
			//SDL_JoystickEventState(SDL_ENABLE);
		}

		/* Joystick control thread creating */
		result = pthread_create(&joystick_thread, NULL, thread_joystick_control, NULL);
		if (result != 0) {
			perror("Creating the thread");
			return 1;
		}
	}

	Graphic_Init();

	while(1) {
	   	if( SDL_PollEvent( &event ) ) {
	        if( event.type == SDL_KEYDOWN ) {
	        	/* We don't use keybord repeat events */
	        	if ( !event.key.repeat ) {
	            switch( event.key.keysym.sym ) {
	                case SDLK_UP:
	                	chassis_state = FORWARD;
	                	thread_id = 1;
	                	//sprintf(thread_commands[thread_id], "run=F,%d,%d\n", run_speed, run_time);
	                	puts("UP");
	                	result = pthread_create(&thread, NULL, thread_command_send, &thread_id);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	SET_DIGITAL_INPUT;
	                	break;
	                case SDLK_DOWN:
	                	chassis_state = BACKWARD;
	                	thread_id = 2;
	                	//sprintf(thread_commands[thread_id], "run=B,%d,%d\n", run_speed, run_time);
	                	puts("DOWN");
	                	result = pthread_create(&thread, NULL, thread_command_send, &thread_id);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	SET_DIGITAL_INPUT;
	                	break;
	                case SDLK_LEFT:
	                	steer_pos = LEFT_STEER_POS;
	                	sprintf(single_command, "steer=%d\n", steer_pos);
	                	send(sock, single_command, strlen(single_command), 0);
	                	puts("LEFT");
	                	break;
	                case SDLK_RIGHT:
	                	steer_pos = RIGHT_STEER_POS;
	                	sprintf(single_command, "steer=%d\n", steer_pos);
	                	//single_command = strbuf;
	                	send(sock, single_command, strlen(single_command), 0);
	                	puts("RIGHT");
	                	break;
	                case SDLK_PAGEUP:
	                	thread_id = 1;
	                	result = pthread_create(&thread, NULL, thread_speed_change, &thread_id);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	break;
	                case SDLK_PAGEDOWN:
	                	thread_id = 2;
	                	result = pthread_create(&thread, NULL, thread_speed_change, &thread_id);
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
	                	sprintf(single_command, "steer=%d\n", steer_pos);
	                	send(sock, single_command, strlen(single_command), 0);
	                	//UpdateScreen(0.0, NONE, run_speed);
	                	puts("CENTER");
	                	break;
	                case SDLK_PAGEUP:
	                case SDLK_PAGEDOWN:
	                	/*
	                	result = pthread_cancel(thread3);
	                	if (result != 0) {
	                		perror("Cancelling the thread");
	                		return 1;
	                	}
	                	*/
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
	        	if( event.jaxis.which == joystickID ) {
	        		unsigned short value = 0;
	        		switch (event.jaxis.axis) {
	        		/* Steering axis */
	        		case 3:
	        			value = CENTER_STEER_POS + ((( RIGHT_STEER_POS - LEFT_STEER_POS ) / 2 ) * \
	        						event.jaxis.value ) / 32768;
	        			/* Joystick steer treshold protection */
	        			if ( ABS( steer_pos - value ) > JOYSTICK_STEER_TRESHOLD_VALUE ) {
	        				steer_pos = value;
						printf("steer_pos = %d\n", steer_pos);
	        				sprintf(single_command, "steer=%d\n", steer_pos);
	        				send(sock, single_command, strlen(single_command), 0);
	        			}
	        			break;
	        		/* Speed & direction axis */
	        		case 2:
	        			if ( event.jaxis.value < 0 ) {
	        				run_direction = "F";
	        				chassis_state = FORWARD;
	        				//run_speed = (( 0 - event.jaxis.value )  * 255 ) / 32768;
	        				//printf("%d\n",0 - event.jaxis.value ); // debug
	        			}
	        			else if ( event.jaxis.value > 0 ) {
	        				run_direction = "B";
	        				chassis_state = BACKWARD;
	        				//run_speed = ((event.jaxis.value )  * 255 ) / 32767;
	        				//printf("%d\n", event.jaxis.value ); // debug
	        			}
	        			else {
	        				chassis_state = NONE;
	        			}

	        			value = ( ABS(event.jaxis.value)  * ( 255 - JOYSTICK_RUN_DEADZONE_VALUE ) ) / 32768 + JOYSTICK_RUN_DEADZONE_VALUE;
	        			printf("%d\n", ABS(event.jaxis.value) ); // debug

	        			/* Joystick run treshold protection */
	        			if ( ABS( run_speed - value ) > JOYSTICK_RUN_TRESHOLD_VALUE ) {
	        				run_speed = value;
	        				sprintf(single_command, "run=%s,%d,%d\n", run_direction, run_speed, run_time);
	        				send(sock, single_command, strlen(single_command), 0);
	        				SET_ANALOG_INPUT;
	        			}
	        			break;
	        		/* Turret axis */
					case 0:
						if ( event.jaxis.value < 0 ) {
							turn_direction = "R";
							turret_state = RIGHT;
						}
						else if ( event.jaxis.value > 0 ) {
							turn_direction = "L";
							turret_state = LEFT;
						}
						else {
							turret_state = STOP;
						}

						value = ( ABS(event.jaxis.value)  * ( 200 - TURRET_HOR_DC_DEADZONE_VALUE ) ) / 32768 + TURRET_HOR_DC_DEADZONE_VALUE;
						printf("%d\n", ABS(event.jaxis.value) ); // debug
						turn_speed = value;

						/* Joystick run treshold protection */
						if ( ABS( turn_speed - value ) > TURRET_HOR_DC_TRESHOLD_VALUE ) {
							sprintf(single_command, "turhdc=%s,%d,%d\n", turn_direction, turn_speed, turn_time);
							send(sock, single_command, strlen(single_command), 0);
						}
						SET_ANALOG_INPUT;
						break;
					/* Turret axis */
	        		case 1:
	        			break;
	        		}
	        	}
	        }
	        else if( event.type ==  SDL_JOYBUTTONDOWN ) {
	        	//Motion on controller 0
	        	if( event.jaxis.which == joystickID ) {

	        	}
	        }
	    }
	Graphic_UpdateScreen((double)((0 - CENTER_STEER_POS + steer_pos) / 6), chassis_state, run_speed, 0);
	}
return 0;
}
