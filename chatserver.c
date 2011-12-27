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
	char* nick;
};

int user_count = 0;
int nick_count = 0;
int poll_count = 1;
struct pollfd* fds;
struct User* user;

void sendToAll (struct User* user, char* msg)
{
	int msg2sendsize = (strlen(user->nick) + strlen(msg) + 3);
	char* msg2send = (char*) malloc(sizeof(char) * msg2sendsize);
	snprintf(msg2send, msg2sendsize, "%s: %s", user->nick, msg);

	printf(msg2send);

	int i;
	for (i = 1; i < poll_count; i++)
	{
		write(fds[i].fd, msg2send, msg2sendsize-1);
	}
}

void setNick (struct User* user, char* newnick)
{
	if (newnick == NULL)
	{
		user->nick = (char*) malloc(sizeof(char) * 10); // TODO
		snprintf(user->nick, 10, "User %04d", ++nick_count);
	}
	else
	{
		char oldnick[strlen(user->nick)];
		strcpy(oldnick, user->nick);
	
		int nicklen = strnlen(newnick, 20);
		user->nick = (char*) realloc(user->nick, sizeof(char) * (nicklen + 1));
		strncpy(user->nick, newnick, nicklen);
		
		if (user->nick[nicklen - 1] != '\0')
			user->nick[nicklen] = '\0';
		
		sendToAll(user, "***changed its nick***\n");
	}

}

struct User* findUserBySocketNumber (int socknum)
{
	int i;
	for (i = 0; i < user_count; i++)
	{
		if (user[i].pollfd == socknum)
		{
			return &(user[i]);
		}
	}
	
	return NULL;
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
		fds = (struct pollfd*) realloc(fds, sizeof(struct pollfd) * ++poll_count);
		fds[poll_count-1].fd = peer_addr;
		fds[poll_count-1].events = POLLIN;
		
		user = (struct User*) realloc(user, sizeof(struct User) * ++user_count);
		user[user_count-1].pollfd = poll_count-1;
		
		setNick(&(user[user_count-1]), NULL);
		
		sendToAll(&(user[user_count-1]), "***connected to chat***\n");
	}
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
	else if (readcount > 0)
	{
		if (strncmp(buffer, "/nick ", 6) == 0)
		{
			
		}
		else
		{
			sendToAll(user, buffer);
		}
	}
}

int main (int argc, char *argv[])
{
	fds = (struct pollfd*) malloc(sizeof(struct pollfd) * poll_count);
	user = (struct User*) malloc(sizeof(struct User) * 0);

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
				if (fds[i].revents & POLLIN)
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