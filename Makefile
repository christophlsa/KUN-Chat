# Makefile for echoclient

CC	= gcc
CFLAGS  += -Wall -O2 -g -Iinclude -D_GNU_SOURCE
LDLIBS  += -lpthread

GTKCFLAGS=$(shell pkg-config --cflags gtk+-3.0 gmodule-2.0)
GTKLDLIBS=$(shell pkg-config --libs gtk+-3.0 gmodule-2.0)

APPS = chatclient chatserver

apps: $(APPS)

chatserver: chatserver.o commons.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

chatclient: chatclient.o commons.o chatgui.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(GTKLDLIBS) -o $@

chatgui.o: chatgui.c
	$(CC) -c $(CFLAGS) $(GTKCFLAGS) -o $@ $^

commons.o: commons.c
	$(CC) -c $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean

clean:
	rm -rf *.o $(APPS) core
