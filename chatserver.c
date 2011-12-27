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
struct pollfd* fds;
struct User* user;

void sendToAll (struct User* user, char* msg)
{
	int msg2sendsize = (strlen(user->nick) + strlen(msg) + 3);
	char* msg2send = (char*) malloc(sizeof(char) * msg2sendsize);
	snprintf(msg2send, msg2sendsize, "%s: %s", user->nick, msg);

	printf(msg2send);

	int i;
	for (i = 0; i < user_count; i++)
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

int main (int argc, char *argv[])
{
	signal(SIGCLD, SIG_IGN);

	fds = (struct pollfd*) malloc(sizeof(struct pollfd) * 0);
	user = (struct User*) malloc(sizeof(struct User) * 0);

	int sfd, peer_addr;
	struct sockaddr_in my_addr, cfd;
	socklen_t peer_addr_size = sizeof(peer_addr);

	sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

	if (sfd == -1)
	{
		printf("cannot create socket\n");
		exit(1);
	}
	
	memset(&my_addr, 0, sizeof(struct sockaddr_in));

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(4444);

	bind(sfd, (struct sockaddr*) &my_addr, sizeof(my_addr));
	listen(sfd, 5);

	while (1)
	{
		peer_addr = accept(sfd, (struct sockaddr*) &cfd, &peer_addr_size);
		
		if (peer_addr < 0)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				printf("Client Socket Error: %s\n", strerror(errno));
				exit(1);
			}
		}
		else
		{
			user_count++;
			fds = (struct pollfd*) malloc(sizeof(struct pollfd) * user_count);
			fds[user_count-1].fd = peer_addr;
			fds[user_count-1].events = POLLOUT;
			fds[user_count-1].revents = 0;
			
			user = (struct User*) realloc(user, sizeof(struct User) * user_count);
			user[user_count-1].pollfd = user_count-1;
			
			setNick(&(user[user_count-1]), NULL);
			
			sendToAll(&(user[user_count-1]), "***connected to chat***\n");
		}
		
		int pollr = poll(fds, user_count, 500);
		
		if(pollr < 0)
		{
			printf("poll error: %s\n", strerror(errno));
			exit(1);
		}
		else if (pollr > 0)
		{
			int i;
			for (i = 0; i < user_count; i++)
			{
				if (fds[i].revents & POLLOUT)
				{
					char buffer[1025];
					int readcount = read(fds[i].fd, buffer, 1024);
					buffer[readcount] = '\0';
					
					if (readcount < 0)
					{
						if (errno != EAGAIN && errno != EWOULDBLOCK)
						{
							printf("Client Socket Error: %s\n", strerror(errno));
							exit(1);
						}
					}
					else if (readcount > 0)
					{
						if (strncmp(buffer, "/nick ", 6) == 0)
						{
							
						}
						else
						{
							sendToAll(&(user[i]), buffer);
						}
					}
				}
			}
		}
	}

	return 0;
}
