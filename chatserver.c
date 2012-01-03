/* chatserver.c */

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
#include "commons.h"

struct User
{
	int pollfd;
	char nick[20];
	char* buffer;
	int bufferlen;
	int active;
};

int user_count = 0;
int nick_count = 0;
int poll_count = 1;
struct pollfd* fds;
struct User* users;

struct User* currentUser;

/**
 * Prints all connected users to console (for debugging).
 */
void debugUserPrint ()
{
	printf("# user count: %d\n", user_count);

	int i;
	for (i = 0; i < user_count; i++)
	{
		printf("# -> [%d]: %s\n", i, users[i].nick);
	}
}

/**
 * Cut all characters from beginning of newline.
 *
 * Memory for the new string is obtained with malloc(3), and can be freed with free(3).
 */
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

/**
 * Send message written by user to all users.
 */
void sendToAllFromUser (struct User* user, char* msg)
{
	msg = newlineCut(msg);

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "%s: %s\n", user->nick, msg);
	if (msg2sendsize < 0)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	printf(msg2send);

	int i;
	for (i = 1; i < poll_count; i++)
	{
		if (fds[i].fd != 0)
			write(fds[i].fd, msg2send, msg2sendsize);
	}

	free(msg);
	free(msg2send);
}

/**
 * Send status message to all users.
 */
void sendToAll (char* msg)
{
	msg = newlineCut(msg);

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "* %s\n", msg);
	if (msg2sendsize < 0)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	printf(msg2send);

	int i;
	for (i = 1; i < poll_count; i++)
	{
		if (fds[i].fd != 0)
			write(fds[i].fd, msg2send, msg2sendsize);
	}

	free(msg);
	free(msg2send);
}

/**
 * Send status message to only one user.
 */
void sendToUser (struct User* user, char* msg)
{
	if (fds[user->pollfd].fd == 0)
		return;

	msg = newlineCut(msg);

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "* %s\n", msg);
	if (msg2sendsize < 0)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

	printf("(to %s): %s", user->nick, msg2send);

	write(fds[user->pollfd].fd, msg2send, msg2sendsize);

	free(msg);
	free(msg2send);
}

/**
 * Returns user with given number of socket.
 */
struct User* findUserBySocketNumber (int socknum)
{
	int i;
	for (i = 0; i < user_count; i++)
	{
		if (users[i].pollfd == socknum)
		{
			return &(users[i]);
		}
	}

	return NULL;
}

/**
 * Returns user with given nick.
 */
struct User* findUserByName (char* name)
{
	int i;
	for (i = 0; i < user_count; i++)
	{
		if (users[i].active == 1 && strcmp(users[i].nick, name) == 0)
		{
			return &(users[i]);
		}
	}

	return NULL;
}

/**
 * Set new nick of user. For new user a new nick will be generated.
 */
void setNick (struct User* user, char* newnick)
{
	if (newnick == NULL)
	{
		snprintf(user->nick, 20, "User%d", ++nick_count);
	}
	else
	{
		if (findUserByName(newnick) != NULL)
		{
			sendToUser(user, "This nick already exists.");
			return;
		}
		else if (strncmp(newnick, "User", 5) == 0)
		{
			sendToUser(user, "This nick is not allowed.");
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
		sendToAll(msg2send);
		free(msg2send);
	}
}

/**
 * Accepts new connection and creates new user and in poll array.
 */
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
		fds = realloc(fds, sizeof(struct pollfd) * ++poll_count);
		if (fds == NULL)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		fds[poll_count-1].fd = peer_addr;
		fds[poll_count-1].events = POLLIN;
		fds[poll_count-1].revents = 0;

		users = realloc(users, sizeof(struct User) * ++user_count);
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
		users[user_count-1].active = 1;

		setNick(&(users[user_count-1]), NULL);

		char* msg2send;
		int msg2sendsize = asprintf(&msg2send, "%s joined this chat.", users[user_count-1].nick);
		if (msg2sendsize < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		sendToAll(msg2send);
		free(msg2send);
	}
}

/**
 * Set user and socket as non active.
 */
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

	fds[socknum].fd = 0;
	fds[socknum].events = 0;

	struct User* user = findUserBySocketNumber(socknum);
	if (user == NULL)
	{
		fprintf(stderr, "Could not find user for given socket.\n");
		exit(1);
	}
	user->active = 0;

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "%s leaved this chat.", user->nick);
	if (msg2sendsize < 0)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}
	sendToAll(msg2send);
	free(msg2send);
}

/**
 * Handles with message.
 */
void handleMessage (char* content)
{
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
		sendToUser(user, "User list:");

		char* userlistmsg;

		int i;
		for (i = 0; i < user_count; i++)
		{
			if (users[i].active == 0)
				continue;

			if (asprintf(&userlistmsg, "-> %s", users[i].nick) < 0)
			{
				fprintf(stderr, "Speicherallokationsfehler\n");
				exit(1);
			}

			sendToUser(user, userlistmsg);
			
			free(userlistmsg);
		}
	}
	else if (strncmp(content, "/msg ", 5) == 0)
	{
		char* endOfNick = strchr(content + 5, ' ');

		if (endOfNick == NULL)
		{
			sendToUser(user, "wrong usage of /msg.");
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
			sendToUser(user, "this user does not exists.");
			return;
		}

		char* msg;
		int msgsize = asprintf(&msg, "%s send to you: %s", user->nick, content + 5 + nickLen);
		if (msgsize < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		char* msg_wo_newline = newlineCut(msg);

		sendToUser(touser, msg_wo_newline);

		free(msg);
		free(msg_wo_newline);

		char* msg2;
		int msg2size = asprintf(&msg2, "you send to %s: %s", touser->nick, content + 5 + nickLen);
		if (msg2size < 0)
		{
			fprintf(stderr, "Speicherallokationsfehler\n");
			exit(1);
		}
		char* msg2_wo_newline = newlineCut(msg2);

		sendToUser(user, msg2_wo_newline);

		free(msg2);
		free(msg2_wo_newline);
	}
	else
	{
		sendToAllFromUser(user, content);
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
	users = (struct User*) malloc(sizeof(struct User) * 1);
	if (users == NULL)
	{
		fprintf(stderr, "Speicherallokationsfehler\n");
		exit(1);
	}

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

	bind(sfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in));
	listen(sfd, 5);

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
				if (fds[i].revents & POLLHUP)
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
						handleSocket(fds[i].fd, &(currentUser->buffer), &(currentUser->bufferlen), handleMessage, handleDisconnect);
					}
				}
			}
		}
	}

	return 0;
}
