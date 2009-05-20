# note if you change the prefix also update egoboo.sh
PREFIX	:= ${HOME}/.local
EGO_SRC  := camera.c char.c client.c clock.c configfile.c egoboo_endian.c \
	egoboo_fileutil.c egoboo_math.c egoboo_setup.c egoboo_strutil.c \
	ego_wrap.c enchant.c file_common.c file_linux.c font.c \
	game.c gltexture.c graphic.c graphic_fan.c graphic_mad.c graphic_prt.c \
	input.c link.c log.c lua_console.c Md2.c menu.c module.c network.c particle.c \
	passage.c script.c script_compile.c server.c sound.c sys_linux.c ui.c \
	egoboo_console.c lua_console.c script_functions.c

EGO_OBJ  := ${EGO_SRC:.c=.o}
ENET_SRC := ../enet/host.c ../enet/list.c ../enet/memory.c \
                  ../enet/packet.c ../enet/peer.c ../enet/protocol.c \
                  ../enet/unix.c
ENET_OBJ := ${ENET_SRC:.c=.o}

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

CC      := gcc
OPT     := -Os -Wall
INC     := -I. -I../enet/include -I/usr/include/lua5.1 ${SDLCONF_I}
CFLAGS  := ${OPT} ${INC} -DUSE_LUA_CONSOLE
LDFLAGS := ${SDLCONF_L} -lSDL_ttf -lSDL_mixer -lGL -lGLU -lSDL_image -llua5.1

EGO_BIN := egoboo_lua

all: ${EGO_BIN}
   

${EGO_BIN}: ${EGO_OBJ} ${ENET_OBJ}
	${CC} -o $@ $^ ${LDFLAGS}

install: ${EGO_BIN}
	mkdir -p ${PREFIX}/bin
	mkdir -p ${PREFIX}/libexec
	install -m 755 ${EGO_BIN} ${PREFIX}/libexec
	install -p -m 755 ${EGO_BIN}.sh ${PREFIX}/bin/${EGO_BIN}

clean:
	rm -f ${ENET_OBJ} ${EGO_OBJ} ${EGO_BIN}
