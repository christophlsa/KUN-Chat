/* Copyright (C) 2011-2012 Christoph Giesel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __chatclient_include__
#define __chatclient_include__

/**
 * Writes all messages from server to GUI.
 */
void handleMessageFromServer (char* msg);

/**
 * Writes all messages from GUI to server.
 */
void handleMessageFromGUI (char* msg);

/**
 * Connects to server using given hostname/ip address and port.
 */
int connectToServer (char* host, char* port);

/**
 * Closes connections and GUI on disconnect from server or GUI.
 */
void handleDisconnect (int sock);

#endif
