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

#include "chatserver.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>
#include <assert.h>
#include "commons.h"

int user_count = 1;
int nick_count = 0;
int poll_count = 1;
struct pollfd* fds;
struct User* users;

struct User* currentUser;


char* newlineCut (char* str)
{
	char* newline = strchr(str, '\n');

	char* newstr = (newline == NULL) ? strdup(str) : strndup(str, newline - str);

	if (newstr == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	return newstr;
}

void deleteUser (struct User* user)
{
	assert(user_count > 1);
	assert(poll_count > 1);

	if (user == NULL)
	{
		return;
	}

	int i;
	for (i = 1; i < user_count; i++)
	{
		if (&(users[i]) == user)
		{
			break;
		}
	}

	if (i >= user_count)
	{
		fprintf(stderr, "user does not exists (while deleting)\n");
		exit(1);
	}

	free(user->buffer);

	if (user_count > 2 && i != user_count - 1)
	{
		int olduserfdpos = user->pollfd;

		memcpy(&(users[i]), &(users[user_count - 1]), sizeof(struct User));
		memcpy(&(fds[olduserfdpos]), &(fds[poll_count - 1]), sizeof(struct pollfd));
		findUserBySocketNumber(poll_count - 1)->pollfd = olduserfdpos;
	}

	users = (struct User*) realloc(users, sizeof(struct User) * --user_count);
	if (users == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	fds = (struct pollfd*) realloc(fds, sizeof(struct pollfd) * --poll_count);
	if (fds == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
}

void sendToUser(struct User* fromUser, struct User* toUser, char* msg)
{
	msg = newlineCut(msg);

	char* msg2send;
	int msg2sendsize;
	if (fromUser != NULL)
		msg2sendsize = asprintf(&msg2send, "%s: %s\n", fromUser->nick, msg);
	else
		msg2sendsize = asprintf(&msg2send, "* %s\n", msg);

	if (msg2sendsize < 0)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	if (toUser != NULL)
	{
		if (write(fds[toUser->pollfd].fd, msg2send, msg2sendsize) < 1)
		{
			fprintf(stderr, "Socket Write to User %s failed\n", toUser->nick);
			handleDisconnect(fds[toUser->pollfd].fd);
		}
		else
		{
			printf("(to %s): %s", toUser->nick, msg2send);
		}
	}
	else
	{
		int i;
		for (i = 1; i < poll_count; i++)
		{
			if (fds[i].fd != 0)
			{
				if (write(fds[i].fd, msg2send, msg2sendsize) < 1)
				{
					fprintf(stderr, "Socket Write to User %s failed\n", findUserBySocketNumber(i)->nick);
					handleDisconnect(fds[i].fd);
				}
			}
		}

		printf(msg2send);
	}

	free(msg);
	free(msg2send);
}

struct User* findUserBySocketNumber (int socknum)
{
	int i;
	for (i = 1; i < user_count; i++)
	{
		if (users[i].pollfd == socknum)
		{
			return &(users[i]);
		}
	}

	return NULL;
}

struct User* findUserByName (char* name)
{
	int i;
	for (i = 1; i < user_count; i++)
	{
		if (strcmp(users[i].nick, name) == 0)
		{
			return &(users[i]);
		}
	}

	return NULL;
}

char* getValidatedNick (char* nick)
{
	int i, j = 0, len = strlen(nick);

	char* newnick = (char*) malloc(sizeof(char) * len + 1);
	if (newnick == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	for (i = 0; i < len; i++)
	{
		if (isalnum(nick[i]))
			newnick[j++] = nick[i];
	}

	newnick[j] = '\0';

	return newnick;
}

void setNick (struct User* user, char* newnick)
{
	if (user == NULL)
	{
		return;
	}

	if (newnick == NULL)
	{
		snprintf(user->nick, 20, "User%d", ++nick_count);
	}
	else
	{
		newnick = getValidatedNick(newnick);

		if (findUserByName(newnick) != NULL)
		{
			sendToUser(NULL, user, "This nick already exists.");
			return;
		}
		else if (strncmp(newnick, "User", 5) == 0)
		{
			sendToUser(NULL, user, "This nick is not allowed.");
			return;
		}

		char oldnick[strlen(user->nick)];
		strcpy(oldnick, user->nick);

		int nicklen = strnlen(newnick, 19);
		strncpy(user->nick, newnick, nicklen);
		
		if (user->nick[nicklen - 1] != '\0')
			user->nick[nicklen] = '\0';

		char* msg2send;
		int msg2sendsize = asprintf(&msg2send, "%s changed its nick to %s.", oldnick, user->nick);
		if (msg2sendsize < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		sendToUser(NULL, NULL, msg2send);
		free(msg2send);
		free(newnick);
	}
}

void handleNewConnection ()
{
	int peer_addr = accept(fds[0].fd, NULL, NULL);

	if (peer_addr < 0)
	{
		fprintf(stderr, "Client Socket Error: %s\n", strerror(errno));
		exit(1);
	}
	else
	{
		fds = (struct pollfd*) realloc(fds, sizeof(struct pollfd) * ++poll_count);
		if (fds == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		fds[poll_count-1].fd = peer_addr;
		fds[poll_count-1].events = POLLIN;
		fds[poll_count-1].revents = 0;

		users = (struct User*) realloc(users, sizeof(struct User) * ++user_count);
		if (users == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		users[user_count-1].pollfd = poll_count-1;
		users[user_count-1].nick[0] = '\0';
		users[user_count-1].bufferlen = 1;
		users[user_count-1].buffer = (char*) malloc(sizeof(char) * users[user_count-1].bufferlen);
		if (users[user_count-1].buffer == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		users[user_count-1].buffer[0] = '\0';

		setNick(&(users[user_count-1]), NULL);

		char* msg2send;
		int msg2sendsize = asprintf(&msg2send, "%s joined this chat.", users[user_count-1].nick);
		if (msg2sendsize < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		sendToUser(NULL, NULL, msg2send);
		free(msg2send);
	}
}

void handleDisconnect (int socket)
{
	int socknum = -1;

	int i;
	for (i = 0; i < poll_count; i++)
	{
		if (fds[i].fd == socket)
		{
			socknum = i;
			break;
		}
	}

	if (socknum == -1)
	{
		fprintf(stderr, "Could not find given socket in pollfd array.\n");
		exit(1);
	}

	close(socket);

	struct User* user = findUserBySocketNumber(socknum);
	if (user == NULL)
	{
		fprintf(stderr, "Could not find user for given socket.\n");
		exit(1);
	}
	
	char* tmpusername = strdup(user->nick);
	
	deleteUser(user);

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "%s leaved this chat.", tmpusername);
	if (msg2sendsize < 0)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
	sendToUser(NULL, NULL, msg2send);
	free(msg2send);
	free(tmpusername);
}

void handleMessage (char* content)
{
	if (currentUser == NULL)
	{
		return;
	}

	struct User* user = currentUser;

	if (strncmp(content, "/nick ", 6) == 0)
	{
		char* newnick;
		int newnicksize = asprintf(&newnick, "%s", content + 6);
		if (newnicksize < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		char* newnick_wo_newline = newlineCut(newnick);

		setNick(user, newnick_wo_newline);

		free(newnick);
		free(newnick_wo_newline);
	}
	else if (strncmp(content, "/list", 5) == 0)
	{
		sendToUser(NULL, user, "User list:");

		char* userlistmsg;

		int i;
		for (i = 1; i < user_count; i++)
		{
			if (asprintf(&userlistmsg, "-> %s", users[i].nick) < 0)
			{
				fprintf(stderr, "Speicherallokationsfehler\n");
				exit(1);
			}

			sendToUser(NULL, user, userlistmsg);
			
			free(userlistmsg);
		}
	}
	else if (strncmp(content, "/msg ", 5) == 0)
	{
		char* endOfNick = strchr(content + 5, ' ');

		if (endOfNick == NULL)
		{
			sendToUser(NULL, user, "wrong usage of /msg.");
			return;
		}

		int nickLen = endOfNick - (content + 5) + 1;
		char* nick = (char*) malloc(sizeof(char) * nickLen);
		if (nick == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		strncpy(nick, content + 5, nickLen);
		nick[nickLen - 1] = '\0';

		struct User* touser = findUserByName(nick);
		free(nick);

		if (touser == NULL)
		{
			sendToUser(NULL, user, "this user does not exists.");
			return;
		}

		char* msg;

		if (asprintf(&msg, "%s send to you: %s", user->nick, content + 5 + nickLen) < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}

		sendToUser(NULL, touser, msg);

		free(msg);

		if (asprintf(&msg, "you send to %s: %s", touser->nick, content + 5 + nickLen) < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}

		sendToUser(NULL, user, msg);

		free(msg);
	}
	else
	{
		sendToUser(user, NULL, content);
	}
}

/**
 * The beginning of all.
 */
int main (int argc, char *argv[])
{
	long port = 4444;

	if (argc > 2)
	{
		fprintf(stderr, "usage: %s [appnum]\n", argv[0]);
		exit(1);
	}
	else if (argc == 2)
	{
		port = strtol(argv[1], NULL, 10);

		if (errno == EINVAL || errno == ERANGE || port <= 0)
		{
			fprintf(stderr, "The given Port is not a valid integer.\n");
			exit(1);
		}

		if (port <= 1024 && geteuid() != 0)
		{
			fprintf(stderr, "You have to run as root or choose a port over 1024.\n");
			exit(1);
		}
	}

	fds = (struct pollfd*) malloc(sizeof(struct pollfd) * poll_count);
	if (fds == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
	users = (struct User*) malloc(sizeof(struct User) * user_count);
	if (users == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	// Dummy Server User
	users[0].pollfd = 0;
	strcpy(users[0].nick, "Server");
	users[0].buffer = NULL;
	users[0].bufferlen = 0;

	int sfd;
	struct sockaddr_in my_addr;

	sfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sfd == -1)
	{
		fprintf(stderr, "cannot create socket\n");
		exit(1);
	}

	int sockOpt = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int));

	memset(&my_addr, 0, sizeof(struct sockaddr_in));

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(port);

	if (bind(sfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in)) < 0)
	{
		fprintf(stderr, "bind error: %s\n", strerror(errno));
		exit(1);
	}
	if (listen(sfd, 5) < 0)
	{
		fprintf(stderr, "bind error: %s\n", strerror(errno));
		exit(1);
	}

	fds[0].fd = sfd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	while (1)
	{
		int pollr = poll(fds, poll_count, 500);

		if(pollr < 0)
		{
			fprintf(stderr, "poll error: %s\n", strerror(errno));
			exit(1);
		}
		else if (pollr > 0)
		{
			int i;
			for (i = 0; i < poll_count; i++)
			{
				if (fds[i].revents & POLLHUP || fds[i].revents & POLLERR)
				{
					handleDisconnect(fds[i].fd);
				}
				else if (fds[i].revents & POLLIN)
				{
					if (i == 0)
					{
						handleNewConnection();
					}
					else
					{
						currentUser = findUserBySocketNumber(i);
						if (currentUser == NULL)
						{
							fprintf(stderr, "User does not exists?!?!");
							exit(1);
						}
						handleSocket(fds[i].fd, &(currentUser->buffer), &(currentUser->bufferlen), handleMessage, handleDisconnect);
					}
				}
			}
		}
	}

	return 0;
}
