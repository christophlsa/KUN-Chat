/* chatclient.c */

#include <unistd.h>
#include <stdio.h>
#include <cnaiapi.h>
#include <chatgui.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

#define BUFSIZE 1025
#define PORT "4444"

int bufferlen = 1;
char* buffer;
struct pollfd* fds;
int serverfd, inguifd, outguifd;

void handleMessage (char* msg)
{
	printf("%s\n", msg);
	write(outguifd, msg, strlen(msg));
}

void handleServerSocket ()
{
	char buf[BUFSIZE];
	int readcount = read(serverfd, buf, BUFSIZE - 1);
	
	if (readcount < 0)
	{
		fprintf(stderr, "Client Socket Error: %s\n", strerror(errno));
		exit(1);
	}
	else if (readcount == 0)
	{
		printf("Connection to Server closed.\n");
		exit(0);
	}
	else
	{
		buf[readcount] = '\0';

		bufferlen += readcount;
		buffer = (char*) realloc(buffer, sizeof(char) * bufferlen);
		strcat(buffer, buf);
	}
	
	char* newline;
	char* newcontent;
	char* content = buffer;

	while ((newline = strchr(content, '\n')) && newline != NULL)
	{
		newcontent = strndup(content, newline - content + 1);
		if (newcontent == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		handleMessage(newcontent);
		free(newcontent);
		content = newline;
		content++;
	}

	newcontent = strdup(content);
	if (newcontent == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
	bufferlen = strlen(newcontent) + 1;
	free(buffer);
	buffer = newcontent;
}

void handleGUISocket ()
{

}

int connectToServer (char* host, char* port)
{
	int sockfd, rv;
	struct addrinfo *servinfo, *p;

	if ((rv = getaddrinfo(host, port, NULL, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			continue;
		}

		break;
	}
	
	freeaddrinfo(servinfo);
	
	if (p == NULL)
	{
		fprintf(stderr, "Failed to connect to server.\n");
		exit(1);
	}
	
	printf("Connected to server.\n");
	
	return sockfd;
}

int main (int argc, char *argv[])
{
	if (argc < 2 || argc > 3)
	{
		fprintf(stderr, "usage: %s <compname> [appnum]\n", argv[0]);
		exit(1);
	}
	
	// initialize buffer
	buffer = (char*) malloc(sizeof(char) * bufferlen);
	if (buffer == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
	buffer[0] = '\0';
	
	
	// connect
	serverfd = connectToServer(argv[1], argc == 3 ? argv[2] : PORT);


	// start GUI
	if ((gui_start(&inguifd, &outguifd)) < 0)
	{
		fprintf(stderr, "Failed to start GUI -- exiting\n");
		exit(1);
	}
	
	
	// initialize pollfd struct
	fds = (struct pollfd*) malloc(sizeof(struct pollfd) * 2);
	if (fds == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
	
	fds[0].fd = serverfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	
	fds[1].fd = inguifd;
	fds[1].events = POLLIN;
	fds[1].revents = 0;

	while (1) {
		int pollr = poll(fds, 2, 500);
		
		if(pollr < 0)
		{
			fprintf(stderr, "poll error: %s\n", strerror(errno));
			exit(1);
		}
		else if (pollr > 0)
		{
			int i;
			for (i = 0; i < 2; i++)
			{
				if (fds[i].revents & POLLHUP)
				{
					printf("Connection to %s closed.\n", i == 0 ? "Server" : "GUI");
					exit(0);
				}
				else if (fds[i].revents & POLLIN)
				{
					if (i == 0)
					{
						handleServerSocket();
					}
					else if (i == 1)
					{
						handleGUISocket();
					}
				}
			}
		}
	}

	return 0;
}
