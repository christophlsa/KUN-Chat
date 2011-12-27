/* cnaiapi.h */

#ifndef _CNAIAPI_H_
#define _CNAIAPI_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>

/**
 * Application number.
 *
 * A type to describe a distinct application on a server. Note that
 * unprivileged users must use numbers > 1024.
 */
typedef short	appnum;

/**
 * Computer.
 *
 * A type to describe a distinct computer.
 */
typedef long	computer;

/**
 * Connection.
 *
 * A type to describe a connection.
 */
typedef int	connection;

/**
 * Get a computer by its name.
 *
 * Resolve the name of a computer to a value of type ::computer identifying it.
 *
 * @param cname is the name of the computer.
 * @return A value of type ::computer to identify the computer is returned.
 * 		If the name cannot be resolved, -1 is returned.
 */
computer	cname_to_comp(char *cname);

/**
 * Wait for a client to connect.
 *
 * Wait for a client to establish a new connection.
 *
 * @param a is the application number of the provided service. The connecting
 * 		client has to use the same number to connect to the server.
 * 		Remember to use numbers > 1024 as unprivileged user.
 * @return The function blocks until a client connects to the server.
 * 		A connection id is returned when a connection is established.
 * 		In case of error, -1 is returned.
 */
connection	await_contact(appnum a);

/**
 * Connect to a server.
 *
 * Try to establish a connection to a server.
 *
 * @param c is the computer id identifying the server.
 * @param a is the application number of the service.
 * @return A connection id is returned when a connection is established. In
 * 		case of error, -1 is returned.
 */
connection	make_contact(computer c, appnum a);

/**
 * End a connection.
 *
 * End a connection to another computer.
 *
 * @param c is the id of the connection to be ended.
 * @return 0 is returned on success, -1 if an error occurred.
 */
int		end_contact(connection c);

/**
 * Get the server socket.
 *
 * Get the socket the server listens on.
 *
 * @param a is the application number of the service.
 * @return The socket the server listens on is returned or -1 in case of error.
 */
int		get_server_socket(appnum a);

#endif /* !defined(_CNAIAPI_H_) */
