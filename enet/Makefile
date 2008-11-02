# Basic Makefile for Enet under MSYS + MinGW

TARGET=libenet.a
SOURCES=host.c list.c memory.c packet.c peer.c protocol.c unix.c win32.c
OBJECTS=$(SOURCES:%.c=%.o)

CFLAGS=-Iinclude

$(TARGET): $(OBJECTS)
	ar rcs $(TARGET) $(OBJECTS)

clean:
	-rm -f $(TARGET) $(OBJECTS)
