# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGO_SRC := ${wildcard ./*.c}
EGO_OBJ := ${EGO_SRC:.c=.o}

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_C := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

#---------------------
# the egolib configuration

EGOLIB_L      := -L$../$(EGOLIB_DIR) -l$(EGOLIB_TARGET)

#---------------------
# the compiler options

CC      := gcc-3
INC     := -I.
LDFLAGS := ${SDLCONF_L} ${EGOLIB_L} -lSDL_ttf -lSDL_mixer -lSDL_image -lphysfs -lenet -lGL -lGLU

# use different options if the environmental variable PREFIX is defined
ifdef ($(PREFIX),"")
	OPT := -D_NO_PREFIX
else
	OPT := -DPREFIX=\"$(PREFIX)\" -D_NIX_PREFIX
endif

CFLAGS  := ${OPT} ${SDLCONF_C}

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: $(EGO_TARGET)

$(EGO_TARGET): ${EGO_OBJ} 
	${CC} -o $@ $^ ${LDFLAGS} ${CFLAGS}

clean:
	rm -f ${EGO_OBJ} $(EGO_TARGET)
