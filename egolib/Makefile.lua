# note if you change the prefix also update egoboo.sh
PREFIX	:= ${HOME}/.local

EGO_SRC  := \
	camera.c char.c client.c clock.c egoboo_console.c egoboo_endian.c \
	egoboo_fileutil.c egoboo_math.c egoboo_setup.c egoboo_strutil.c \
	egoboo_typedef.c egoboo_vfs.c enchant.c file_common.c \
	font_bmp.c font_ttf.c game.c graphic.c graphic_fan.c \
	graphic_mad.c graphic_prt.c input.c link.c log.c \
	mad.c md2.c menu.c mesh.c network.c particle.c \
	passage.c profile.c quest.c script.c script_compile.c \
	script_functions.c server.c sound.c texture.c ui.c 

EGO_LUA         := ego_wrap.c egoboo_console.c lua_console.c
EGO_PLATFORM    := platform/file_linux.c platform/sys_linux.c
EGO_FILE_FORMAT := $(wildcard ./file_formats/*.c)
EGO_EXTENSIONS  := $(wildcard ./extensions/*.c)

EGO_OBJ  := ${EGO_SRC:.c=.o} ${EGO_FILE_FORMAT:.c=.o} ${EGO_EXTENSIONS:.c=.o} ${EGO_PLATFORM:.c=.o} ${EGO_LUA:.c=.o}

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

CC      := gcc
OPT     := -Os -Wall -DPREFIX=\"${PREFIX}\"
INC     := -I. -I.. -I../enet/include -I/usr/include/lua5.1 ${SDLCONF_I} -I./extensions -I./file_formats -I./platform
CFLAGS  := ${OPT} ${INC} -DUSE_LUA_CONSOLE
LDFLAGS := ${SDLCONF_L} -lSDL_ttf -lSDL_mixer -lGL -lGLU -lSDL_image -lphysfs -llua5.1

.PHONY: all clean

EGO_BIN := egoboo-2.x

all: ${EGO_BIN}

${EGO_BIN}: ${EGO_OBJ} ${ENET_OBJ}
	${CC} -o $@ $^ ${LDFLAGS}

clean:
	rm -f ${ENET_OBJ} ${EGO_OBJ} ${EGO_BIN}