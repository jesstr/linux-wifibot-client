#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define COMMAND_LENGTH 16 	// 16 byte

char *command;


int main(int argc, char **argv)
{
    int sock;
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

  	struct termios newt, oldt;
  	tcgetattr(STDIN_FILENO, &oldt);
  	newt = oldt;
  	newt.c_lflag &= ~(ICANON | ECHO);
  	tcsetattr(STDIN_FILENO, TCSANOW, &newt); /* enable raw mode */

  int c = 0;
  puts("press ENTER to exit");
  for (;;)			/* read_symbols */
    {
      c = getchar();
      if (c == 27)		/* if ESC */
	{
	  c = getchar();	
	  if (c == '[')		/* ... [ */
	    {
	      c = getchar();
	      switch (c)
		{
		case 'A': /* Up */
		  //message = "U"; send(sock, message, sizeof(message), 0); puts("UP"); printf("%d\n",sizeof(message)); break;
		  command = "run=F,200,20\n"; send(sock, command, strlen(command), 0); puts("UP"); break;
		case 'B': /* Down */
		  command = "run=B,200,20\n"; send(sock, command, strlen(command), 0); puts("DOWN"); break;
		case 'C': /* Right */
		  command = "steer=600\n"; send(sock, command, strlen(command), 0); puts("RIGHT"); break;
		case 'D': /* Left */
		  command = "steer=1100\n"; send(sock, command, strlen(command), 0); puts("LEFT"); break;
		default:
		  puts("not arrow"); break;
	      }
	    }
	  else 
	    {
	      puts("not arrow");
	    }
	}
      else if (c == 10)
	{
	  puts("Good by");
	  break;
	}
      else
	{
	  puts("not arrow");
	}
    }
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /* return old config */
  return EXIT_SUCCESS;

}
