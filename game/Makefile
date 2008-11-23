# note if you change the prefix also update egoboo.sh
EGO_BIN	:= egoboo
EGO_SRC	:= camera.c char.c Client.c Clock.c common-file.c configfile.c \
                 egobooendian.c egoboostrutil.c enchant.c Font.c game.c gltexture.c \
                 graphicfan.c graphicmad.c graphic.c graphicprt.c input.c \
                 linux-file.c Log.c Link.c egoboo_math.c Md2.c menu.c module.c \
                 network.c particle.c passage.c script.c Server.c sound.c \
                 sys_linux.c Ui.c
EGO_OBJ	:= ${EGO_SRC:.c=.o}

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

CC      := gcc
OPT     := -Os
INC     := -I. -I../enet/include -I.. ${SDLCONF_I}
CFLAGS  := ${OPT} ${INC}
LDFLAGS := ${SDLCONF_L} -lSDL_ttf -lSDL_mixer -lGL -lGLU -l../enet/libenet.a

${EGO_BIN}: ${EGO_OBJ} ${CC} -o $@ $^ ${LDFLAGS}

all:
	${EGO_BIN}

clean:
	rm -f ${EGO_OBJ} ${EGO_BIN} test_sdl.o test_sdl