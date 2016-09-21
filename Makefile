# lsocket Makefile, works on Linux and Mac OS X, everywhere else roll your own.

ifdef DEBUG
	DBG=-g
	OPT=
else
	DBG=
	OPT=-O2
endif

OS = $(shell uname -s)

# if this does not work, just set it to your version number
LUA_VERSION=$(shell lua -e "print((string.gsub(_VERSION, '^.+ ', '')))")

LUA_DIR = /usr/local
LUA_LIBDIR=$(LUA_DIR)/lib/lua/$(LUA_VERSION)
LUA_SHAREDIR=$(LUA_DIR)/share/lua/$(LUA_VERSION)
LUA_INCLUDE=$(LUA_DIR)/include

ifndef LIBFLAG
	ifeq ($(OS),Darwin)
		LIBFLAG=-bundle -undefined dynamic_lookup -all_load
	else
		LIBFLAG=-shared
	endif
endif

ifndef PTHRFLAG
	PTHRFLAG=-pthread
endif

ifndef CC
	CC=gcc
endif

CFLAGS=-Wall -fPIC $(OPT) $(DBG)
INCDIRS=-I$(LUA_INCLUDE)
LDFLAGS=$(LIBFLAG) $(DBG)

all: lsocket.so

debug:; make DEBUG=1

install:	all
	mkdir -p $(LUA_LIBDIR)
	cp lsocket.so $(LUA_LIBDIR)

install-aresolver:	async_resolver.so
	mkdir -p $(LUA_LIBDIR)
	cp async_resolver.so $(LUA_LIBDIR)

lsocket.so: lsocket.o
	$(CC) $(LDFLAGS) -o $@ $<

async_resolver.so: async_resolver.o gai_async.o
	$(CC) $(LDFLAGS) -o $@ $^ $(PTHRFLAG)

lsocket.o: lsocket.c
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

async_resolver.o: async_resolver.c gai_async.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

gai_async.o: gai_async.c gai_async.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@ $(PTHRFLAG)

clean:
	find . -name "*~" -exec rm {} \;
	find . -name .DS_Store -exec rm {} \;
	find . -name ._* -exec rm {} \;
	rm -f *.o *.so core samples/testsocket

mingw: lsocket.dll

lsocket.dll : lsocket.c win_compat.c
	$(CC) -o $@ -Wall $(OPT) $(DBG) $(INCDIRS) $^ $(LDFLAGS) -lws2_32 -L/usr/local/bin -llua53
