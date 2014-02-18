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

#define COMMAND_LENGTH 16 	// 16 byte

char *command;
//The event structure
SDL_Event event;
int sock;


void * thread_func(void *arg)
{
	int i;
	char *command = (char *) arg;

	for (i = 0; i < 10; i++) {
		puts("sent");
		send(sock, command, strlen(command), 0);
		sleep(1);
	}
	return 0;
}

int main(int argc, char **argv)
{
	pthread_t thread1;
	int result;

	//Инициализировать SDL
	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )
	{
		return 1;
	}

	SDL_SetVideoMode (320, 200, 8, 0);

    struct sockaddr_in addr;

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
	                	command = "run=F,200,20\n";
	                	//send(sock, command, strlen(command), 0);
	                	puts("UP");

	                	result = pthread_create(&thread1, NULL, thread_func, command);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}

	                	break;
	                case SDLK_DOWN:
	                	command = "run=B,200,20\n";
	                	//send(sock, command, strlen(command), 0);
	                	puts("DOWN");

	                	result = pthread_create(&thread1, NULL, thread_func, command);
	                	if (result != 0) {
	                		perror("Creating the thread");
	                		return 1;
	                	}

	                	break;
	                case SDLK_LEFT:
	                	command = "steer=600\n";
	                	send(sock, command, strlen(command), 0);
	                	puts("LEFT");
	                	break;
	                case SDLK_RIGHT:
	                	command = "steer=1100\n";
	                	send(sock, command, strlen(command), 0);
	                	puts("RIGHT");
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
	                	pthread_cancel(thread1);
	                	break;
	                case SDLK_DOWN:
	                	puts("STOP");
	                	pthread_cancel(thread1);
	                	break;
	                case SDLK_LEFT:
	                case SDLK_RIGHT:
	                	command = "steer=850\n";
	                	send(sock, command, strlen(command), 0);
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
