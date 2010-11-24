# Do not run this file. Run the Makefile in the parent directory, instead

EGO_BIN := $(PROJ_NAME)

#---------------------
# the source files

EGO_SRC  := \
	bbox.c bsp.c camera.c char.c client.c clock.c collision.c \
	egoboo_console.c egoboo_endian.c egoboo_fileutil.c \
	egoboo_math.c egoboo_setup.c egoboo_strutil.c egoboo_typedef.c \
	egoboo_vfs.c enchant.c file_common.c font_bmp.c font_ttf.c game.c \
	graphic.c graphic_fan.c graphic_mad.c graphic_prt.c hash.c input.c \
	lighting.c IDSZ_map.c link.c log.c mad.c md2.c menu.c mesh.c mpd_BSP.c network.c \
	obj_BSP.c particle.c passage.c profile.c quest.c script.c script_compile.c \
	script_functions.c server.c sound.c texture.c ui.c egoboo.c physics.c\
	egoboo_process.c ChrList.c EncList.c PrtList.c egoboo_object.c

EGO_PLATFORM    := platform/file_linux.c platform/sys_linux.c
EGO_FILE_FORMAT := $(wildcard ./file_formats/*.c)
EGO_EXTENSIONS  := $(wildcard ./extensions/*.c)

EGO_OBJ := ${EGO_SRC:.c=.o} ${EGO_FILE_FORMAT:.c=.o} ${EGO_EXTENSIONS:.c=.o} ${EGO_PLATFORM:.c=.o}

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

#---------------------
# the compiler options

CC      := gcc
INC     := -I. -I.. -I../enet/include ${SDLCONF_I} -I./extensions -I./file_formats -I./platform
LDFLAGS := ${SDLCONF_L} -L../enet/lib -lSDL_ttf -lSDL_mixer -lGL -lGLU -lSDL_image -lphysfs -lenet

# use different options if the environmental variable PREFIX is defined
ifdef ($(PREFIX),"")
	OPT := -Os -Wall
else
	OPT := -Os -Wall -DPREFIX=\"${PREFIX}\" -D_NIX_PREFIX
endif

CFLAGS  := ${OPT} ${INC}

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: ${EGO_BIN}

${EGO_BIN}: ${EGO_OBJ} 
	${CC} -o $@ $^ ${LDFLAGS} ${CFLAGS}

clean:
	rm -f ${ENET_OBJ} ${EGO_OBJ} ${EGO_BIN}

