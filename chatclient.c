/* chatclient.c */

#include <unistd.h>
#include <stdio.h>
#include <cnaiapi.h>
#include <chatgui.h>

#define BUFSIZE 1024

int main(int argc, char *argv[])
{
	// Pass these to gui_start() and use them instead of stdin/stdout.
	int infd, outfd;

	char buf[BUFSIZE];

	// start GUI
	if((gui_start(&infd, &outfd)) < 0) {
		fprintf(stderr, "Failed to start GUI -- exiting\n");
		return -1;
	}

	while (1) {
		int len = read(infd, buf, BUFSIZE);  // read input from GUI
		if (len <= 0) {
			printf("GUI closed\n");
			break;
		}
		write(outfd, buf, len);	 // write output to GUI
	}

	return 0;
}
