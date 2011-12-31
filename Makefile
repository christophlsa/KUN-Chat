# Makefile for echoclient

ARCH64=''
ifeq (64,$(findstring 64,$(shell uname -m)))
	ARCH64=64
endif

CC	= gcc
CFLAGS  += -Wall -g -Iinclude -D_GNU_SOURCE
LDFLAGS += -Llib
LDLIBS  += -lchatgui$(ARCH64) -lpthread

GTKCFLAGS=$(shell pkg-config --cflags gtk+-2.0)
GTKLDLIBS=$(shell pkg-config --libs gtk+-2.0)

APPS = chatclient chatserver

apps: $(APPS)

chatserver: chatserver.o commons.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

chatclient: chatclient.o commons.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(GTKLDLIBS) -o $@

chatgui.o: chatgui.c
	$(CC) -c $(CFLAGS) $(GTKCFLAGS) -o $@ $^

commons.o: commons.c
	$(CC) -c $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: clean

clean:
	rm -rf *.o $(APPS) core
