struct User
{
	int pollfd;
	char nick[20];
	char* buffer;
	int bufferlen;
	int active;
};

/**
 * Cut all characters from beginning of newline.
 *
 * Memory for the new string is obtained with malloc(3), and can be freed with free(3).
 */
char* newlineCut (char* str);

/**
 * Send message to one or all users.
 */
void sendToUser(struct User* fromUser, struct User* toUser, char* msg);

/**
 * Returns user with given number of socket.
 */
struct User* findUserBySocketNumber (int socknum);

/**
 * Returns user with given nick.
 */
struct User* findUserByName (char* name);

/**
 * Returns given nick with only alpha numeric characters.
 *
 * Memory for the new string is obtained with malloc(3), and can be freed with free(3).
 */
char* getValidatedNick (char* nick);

/**
 * Set new nick of user. For new user a new nick will be generated.
 */
void setNick (struct User* user, char* newnick);

/**
 * Accepts new connection and creates new user and in poll array.
 */
void handleNewConnection ();

/**
 * Set user and socket as non active.
 */
void handleDisconnect (int socket);

/**
 * Handles with message.
 */
void handleMessage (char* content);

