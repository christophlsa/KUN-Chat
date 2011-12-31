#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "commons.h"

void handleSocket (int socket, char** buffer, int* bufferlen, void(*callback)(char*))
{
	char buf[BUFSIZE];
	int readcount = read(socket, buf, BUFSIZE - 1);

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

	buf[readcount] = '\0';

	*bufferlen += readcount;
	*buffer = (char*) realloc(*buffer, sizeof(char) * *bufferlen);
	strcat(*buffer, buf);

	char* newline;
	char* newcontent;
	char* content = *buffer;

	while ((newline = strchr(content, '\n')) && newline != NULL)
	{
		newcontent = strndup(content, newline - content + 1);
		if (newcontent == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		callback(newcontent);
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
	*bufferlen = strlen(newcontent) + 1;
	free(*buffer);
	*buffer = newcontent;
}
