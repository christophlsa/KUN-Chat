/* chatserver.c */

#include <cnaiapi.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>

struct User
{
	int pollfd;
	char nick[10];
	int active;
};

int user_count = 0;
int nick_count = 0;
int poll_count = 1;
struct pollfd* fds;
struct User* users;

void debugUserPrint ()
{
	printf("# user count: %d\n", user_count);

	int i;
	for (i = 0; i < user_count; i++)
	{
		printf("# -> [%d]: %s\n", i, users[i].nick);
	}
}

char* newlineReplace (char* str)
{
	char* newline = strchr(str, '\n');

	if (newline == NULL)
		return str;

	return strndup(str, newline - str);
}

void sendToAllFromUser (struct User* user, char* msg)
{
	msg = newlineReplace(msg);

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "%s: %s\n", user->nick, msg);
	if (msg2sendsize < 0)
	{
		printf("Speicherallokationsfehler\n");
		exit(1);
	}

	printf(msg2send);

	int i;
	for (i = 1; i < poll_count; i++)
	{
		if (fds[i].fd != 0)
			write(fds[i].fd, msg2send, msg2sendsize);
	}

	free(msg2send);
}

void sendToAll (char* msg)
{
	msg = newlineReplace(msg);

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "* %s\n", msg);
	if (msg2sendsize < 0)
	{
		printf("Speicherallokationsfehler\n");
		exit(1);
	}

	printf(msg2send);

	int i;
	for (i = 1; i < poll_count; i++)
	{
		if (fds[i].fd != 0)
			write(fds[i].fd, msg2send, msg2sendsize);
	}

	free(msg2send);
}

void sendToUser (struct User* user, char* msg)
{
	if (fds[user->pollfd].fd == 0)
		return;

	msg = newlineReplace(msg);

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "* %s\n", msg);
	if (msg2sendsize < 0)
	{
		printf("Speicherallokationsfehler\n");
		exit(1);
	}

	printf("(to %s): %s", user->nick, msg2send);

	write(fds[user->pollfd].fd, msg2send, msg2sendsize);

	free(msg2send);
}

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

void setNick (struct User* user, char* newnick)
{
	if (newnick == NULL)
	{
		snprintf(user->nick, 10, "User%05d", ++nick_count);
	}
	else
	{
		if (findUserByName(newnick) != NULL)
		{
			sendToUser(user, "This nick already exists.\n");
			return;
		}
		else if (strncmp(newnick, "User", 5) == 0)
		{
			sendToUser(user, "This nick is not allowed.\n");
			return;
		}

		char oldnick[strlen(user->nick)];
		strcpy(oldnick, user->nick);

		int nicklen = strnlen(newnick, 9);
		strncpy(user->nick, newnick, nicklen);
		
		if (user->nick[nicklen - 1] != '\0')
			user->nick[nicklen] = '\0';

		char* msg2send;
		int msg2sendsize = asprintf(&msg2send, "%s changed its nick to %s\n", oldnick, user->nick);
		if (msg2sendsize < 0)
		{
			printf("Speicherallokationsfehler\n");
			exit(1);
		}
		sendToAll(msg2send);
		free(msg2send);
	}
}

void handleNewConnection ()
{
	int peer_addr = accept(fds[0].fd, NULL, NULL);

	if (peer_addr < 0)
	{
		printf("Client Socket Error: %s\n", strerror(errno));
		exit(1);
	}
	else
	{
		fds = realloc(fds, sizeof(struct pollfd) * ++poll_count);
		if (fds == NULL)
		{
			printf("Speicherallokationsfehler\n");
			exit(1);
		}
		fds[poll_count-1].fd = peer_addr;
		fds[poll_count-1].events = POLLIN;

		users = realloc(users, sizeof(struct User) * ++user_count);
		if (users == NULL)
		{
			printf("Speicherallokationsfehler\n");
			exit(1);
		}
		users[user_count-1].pollfd = poll_count-1;
		users[user_count-1].nick[0] = '\0';
		users[user_count-1].active = 1;

		setNick(&(users[user_count-1]), NULL);

		char* msg2send;
		int msg2sendsize = asprintf(&msg2send, "%s joined this chat\n", users[user_count-1].nick);
		if (msg2sendsize < 0)
		{
			printf("Speicherallokationsfehler\n");
			exit(1);
		}
		sendToAll(msg2send);
		free(msg2send);
	}
}

void handleDisconnect (int socknum)
{
	close(fds[socknum].fd);

	fds[socknum].fd = 0;
	fds[socknum].events = 0;

	struct User* user = findUserBySocketNumber(socknum);
	user->active = 0;

	char* msg2send;
	int msg2sendsize = asprintf(&msg2send, "%s leaved this chat\n", user->nick);
	if (msg2sendsize < 0)
	{
		printf("Speicherallokationsfehler\n");
		exit(1);
	}
	sendToAll(msg2send);
	free(msg2send);
}

void handleContent (struct User* user)
{
	if (user == NULL)
	{
		printf("user is NULL ?!?!\n");
		exit(1);
	}

	char buffer[1025];
	int readcount = read(fds[user->pollfd].fd, buffer, 1024);
	buffer[readcount] = '\0';

	if (readcount < 0)
	{
		printf("Client Socket Error: %s\n", strerror(errno));
		exit(1);
	}
	else if (readcount == 0)
	{
		handleDisconnect(user->pollfd);
	}
	else
	{
		if (strncmp(buffer, "/nick ", 6) == 0)
		{
			char* newnick = (char*) malloc(sizeof(char) * (readcount - 6));
			strncpy(newnick, buffer + 6, readcount - 7);
			newnick[readcount - 1] = '\0';

			setNick(user, newnick);

			free(newnick);
		}
		else if (strncmp(buffer, "/list", 5) == 0)
		{
			sendToUser(user, "User list:\n");

			char userlistmsg[14];

			int i;
			for (i = 0; i < user_count; i++)
			{
				if (users[i].active == 0)
					continue;

				sprintf(userlistmsg, "-> %s\n", users[i].nick);

				sendToUser(user, userlistmsg);
			}
		}
		else if (strncmp(buffer, "/msg ", 5) == 0)
		{
			char* endOfNick = strchr(buffer + 5, ' ');

			if (endOfNick == NULL)
			{
				sendToUser(user, "wrong usage of /msg\n");
				return;
			}

			int nickLen = endOfNick - (buffer + 5) + 1;
			char* nick = (char*) malloc(sizeof(char) * nickLen);
			if (nick == NULL)
			{
				printf("Speicherallokationsfehler\n");
				exit(1);
			}
			strncpy(nick, buffer + 5, nickLen);
			nick[nickLen - 1] = '\0';

			struct User* touser = findUserByName(nick);

			if (touser == NULL)
			{
				sendToUser(user, "this user does not exists.\n");
				return;
			}

			char* msg;
			int msgsize = asprintf(&msg, "%s send to you: %s", user->nick, buffer + 5 + nickLen);
			if (msgsize < 0)
			{
				printf("Speicherallokationsfehler\n");
				exit(1);
			}
			msg = newlineReplace(msg);

			char* msg2;
			int msg2size = asprintf(&msg2, "you send to %s: %s", touser->nick, buffer + 5 + nickLen);
			if (msg2size < 0)
			{
				printf("Speicherallokationsfehler\n");
				exit(1);
			}
			msg2 = newlineReplace(msg2);

			sendToUser(touser, msg);
			sendToUser(user, msg2);
		}
		else
		{
			sendToAllFromUser(user, buffer);
		}
	}
}

int main (int argc, char *argv[])
{
	fds = (struct pollfd*) malloc(sizeof(struct pollfd) * poll_count);
	users = (struct User*) malloc(sizeof(struct User) * 1);

	int sfd;
	struct sockaddr_in my_addr;

	sfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sfd == -1)
	{
		printf("cannot create socket\n");
		exit(1);
	}

	int sockOpt = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(int));

	memset(&my_addr, 0, sizeof(struct sockaddr_in));

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(4444);

	bind(sfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr_in));
	listen(sfd, 5);

	fds[0].fd = sfd;
	fds[0].events = POLLIN;

	while (1)
	{
		int pollr = poll(fds, poll_count, 500);

		if(pollr < 0)
		{
			printf("poll error: %s\n", strerror(errno));
			exit(1);
		}
		else if (pollr > 0)
		{
			int i;
			for (i = 0; i < poll_count; i++)
			{
				if (fds[i].revents & POLLHUP)
				{
					handleDisconnect(i);
				}
				else if (fds[i].revents & POLLIN)
				{
					if (i == 0)
					{
						handleNewConnection();
					}
					else
					{
						handleContent(findUserBySocketNumber(i));
					}
				}
			}
		}
	}

	return 0;
}
