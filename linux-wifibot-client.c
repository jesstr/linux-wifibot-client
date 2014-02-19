#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL.h>
#include <pthread.h>

#define MAX_COMMAND_LENGTH	16
#define CENTER_STEER_POS	850

char command1[MAX_COMMAND_LENGTH];	/* Independent command buffers for safe using in threads */
char command2[MAX_COMMAND_LENGTH];

char command3[MAX_COMMAND_LENGTH];	/* Command buffer for using in non-thread functions */

unsigned char run_speed = 200;	/* Default forward and backward run speed value, percent */
unsigned char run_time = 20; 	/* Default forward and backward run time value, 1=100ms */
unsigned short steer_pos = CENTER_STEER_POS;	/* Default steering servo position, us */

SDL_Event event;	/* The event structure */
int sock;


void * thread_func(void *arg)
{
	char *command;
	unsigned char id = * (unsigned char *) arg;
	Uint8 *keystate = SDL_GetKeyState( NULL );

	switch ( id ) {
		case 1: command = command1; break;
		case 2: command = command2; break;
	}
	do {
		puts("sent");
		send(sock, command, strlen(command), 0);
		usleep(500000);
		/* Exit thread if key was released*/
		if ( ( !(keystate[SDLK_UP]) && id == 1) ||
			 ( !(keystate[SDLK_DOWN]) && id == 2) )
			pthread_exit(NULL);
	}
	while ( keystate[SDLK_UP] || keystate[SDLK_DOWN] );
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	pthread_t thread1;
	unsigned char thread_id;
	struct sockaddr_in addr;
	int result;

	//Инициализировать SDL
	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}
	SDL_SetVideoMode (320, 200, 8, 0);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
       perror("socket");
       return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425); // или любой другой порт...
    //addr.sin_addr.s_addr = inet_addr("192.168.1.121");
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    printf("Connecting...\n");
    
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 2;
    }

	printf("Connected.\n");

	puts("press ENTER to exit");

	while(1) {
	   	if( SDL_PollEvent( &event ) ) {
	        //Если была нажата клавиша
	        if( event.type == SDL_KEYDOWN ) {
	   			//Выбрать правильное сообщение
	            switch( event.key.keysym.sym ) {
	                case SDLK_UP:
	                	sprintf(command1, "run=%s,%d,%d\n", "F", run_speed, run_time);
	                	puts("UP");
	                	thread_id = 1;
	                	result = pthread_create(&thread1, NULL, thread_func, &thread_id);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	break;
	                case SDLK_DOWN:
	                	sprintf(command2, "run=%s,%d,%d\n", "B", run_speed, run_time);
	                	puts("DOWN");
	                	thread_id = 2;
	                	result = pthread_create(&thread1, NULL, thread_func, &thread_id);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}
	                	break;
	                case SDLK_LEFT:
	                	steer_pos = 600;
	                	sprintf(command3, "steer=%d\n", steer_pos);
	                	send(sock, command3, strlen(command3), 0);
	                	puts("LEFT");
	                	break;
	                case SDLK_RIGHT:
	                	steer_pos = 1100;
	                	sprintf(command3, "steer=%d\n", steer_pos);
	                	//command3 = strbuf;
	                	send(sock, command3, strlen(command3), 0);
	                	puts("RIGHT");
	                	break;
	                case SDLK_PAGEUP:
	                	break;
	                case SDLK_PAGEDOWN:
	                	break;
	                case SDLK_RETURN:
	                	//Выходим из программы
	                	//Освободить ресурсы занятые SDL
	                	SDL_Quit();
	                	return 0;
	                	break;
	                default:
	                	break;
	            }
	        }
			else if( event.type == SDL_KEYUP ) {
	   			//Выбрать правильное сообщение
	            switch( event.key.keysym.sym ) {
	                case SDLK_UP:
	                	puts("STOP");
	                	break;
	                case SDLK_DOWN:
	                	puts("STOP");
	                	break;
	                case SDLK_LEFT:
	                case SDLK_RIGHT:
	                	steer_pos = CENTER_STEER_POS;
	                	sprintf(command3, "steer=%d\n", steer_pos);
	                	send(sock, command3, strlen(command3), 0);
	                	puts("CENTER");
	                	break;
	                default:
	                	break;
	            }
	        }
	        //Если пользователь хочет выйти
	        else if( event.type == SDL_QUIT ) {
	            //Выходим из программы
	           	//Освободить ресурсы занятые SDL
	   			SDL_Quit();
	  			return 0;
	        }
	    }
	}
return EXIT_SUCCESS;
}
