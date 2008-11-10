# Basic *nix Makefile

ENET_TARGET	:= libenet.a
ENET_SRCLST	:= host.c list.c memory.c packet.c peer.c protocol.c unix.c win32.c
ENET_OBJLST	:= $(ENET_SRCLST:.c=.o)

CC      := gcc
OPT     := -Os
INC     := -Iinclude
CFLAGS  := $(OPT) $(INC)

all: $(ENET_OBJLST)

	ar rcs $(ENET_TARGET) $(ENET_OBJLST)

clean:
	rm -f $(ENET_TARGET) $(ENET_OBJLST)
