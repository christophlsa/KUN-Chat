/* chatclient.c */

#include <unistd.h>
#include <stdio.h>
#include <chatgui.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include "commons.h"

#define PORT "4444"

int bufferlen = 1;
char* buffer;
int bufferlen2 = 1;
char* buffer2;

struct pollfd* fds;
int serverfd, inguifd, outguifd;

void handleMessageFromServer (char* msg)
{
	printf("%s", msg);
	write(outguifd, msg, strlen(msg));
}

void handleMessageFromGUI (char* msg)
{
	printf("%s", msg);
	write(serverfd, msg, strlen(msg));
}

int connectToServer (char* host, char* port)
{
	int sockfd, rv;
	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0)
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

	if (p == NULL)
	{
		fprintf(stderr, "Failed to connect to server.\n");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("Connected to server.\n");

	return sockfd;
}

void handleDisconnect (int sock)
{
	printf("Connection to %s closed.\n", sock == serverfd ? "Server" : "GUI");

	close(serverfd);
	close(inguifd);
	close(outguifd);

	free(fds);
	free(buffer);
	free(buffer2);

	exit(0);
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

	buffer2 = (char*) malloc(sizeof(char) * bufferlen2);
	if (buffer2 == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
	buffer2[0] = '\0';


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
					handleDisconnect(fds[i].fd);
				}
				else if (fds[i].revents & POLLIN)
				{
					if (i == 0)
					{
						handleSocket(serverfd, &buffer, &bufferlen, handleMessageFromServer, handleDisconnect);
					}
					else if (i == 1)
					{
						handleSocket(inguifd, &buffer2, &bufferlen2, handleMessageFromGUI, handleDisconnect);
					}
				}
			}
		}
	}

	return 0;
}
