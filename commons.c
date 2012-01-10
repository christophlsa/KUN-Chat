/* Copyright (C) 2011-2012 Christoph Giesel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "commons.h"

void handleSocket (int socket, char** buffer, int* bufferlen, void(*callback)(char*), void(*callbackClose)(int))
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
		callbackClose(socket);
		return;
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
