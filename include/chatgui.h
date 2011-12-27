/*
 * =====================================================================================
 *
 *       Filename:  chatgui.h
 *
 *    Description:  API of the GUI for the KuN-Chat
 *
 *         Author:  Timo Lindhorst <lindhors@ivs.cs.ovgu.de>
 *
 * =====================================================================================
 */

#include <unistd.h>

/**
 * Start the chat GUI.
 *
 * Start the GUI for the chat client.
 *
 * @param infd is the input file descriptor. Everything typed within the GUI can
 * 		be read from \c infd. It can be considered to replace \c stdin.
 * @param outfd is the output file descriptor. Everything written to \c outfd is
 * 		printed in the GUI. It can be considered to replace \c stdout.
 * @return The process id of the GUI process is returned.
 */
pid_t gui_start(int *infd, int *outfd);
