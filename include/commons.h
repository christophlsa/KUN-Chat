#ifndef __commons_include__
#define __commons_include__

#define BUFSIZE 1025

void handleSocket (int socket, char** buffer, int* bufferlen, void(*callback)(char*));

#endif
