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

#ifndef __commons_include__
#define __commons_include__

#define PORT "4444"
#define BUFSIZE 1024

/**
 * Read from given socket, write in given buffer and call callback for each message seperated with newline character.
 * If count of characters is zero then call callbackClose.
 */
void handleSocket (int socket, char** buffer, int* bufferlen, void(*callback)(char*), void(*callbackClose)(int));

#endif
