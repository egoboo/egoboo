# Basic *nix Makefile

ENET_TARGET	:= libenet.a
ENET_SRCLST	:= host.c list.c memory.c packet.c peer.c protocol.c unix.c
ENET_OBJLST	:= $(ENET_SRCLST:.c=.o)

CC      := gcc
OPT     := -Os
INC     := -Iinclude
CFLAGS  := $(OPT) $(INC)

.PHONY: all clean

all: $(ENET_OBJLST)

	ar -rcs ./lib/$(ENET_TARGET) $(ENET_OBJLST)

clean:
	rm -f ./lib/$(ENET_TARGET) $(ENET_OBJLST)
