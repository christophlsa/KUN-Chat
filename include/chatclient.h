#define PORT "4444"

void handleMessageFromServer (char* msg);

void handleMessageFromGUI (char* msg);

int connectToServer (char* host, char* port);

void handleDisconnect (int sock);
