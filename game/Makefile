# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGO_SRC    := ${wildcard src/game/*.c} ${wildcard ./src/game/ai/*.c}
EGO_CPPSRC := ${wildcard src/game/*.cpp} ${wildcard src/game/*/*.cpp}
EGO_CPPSRC := $(filter-out src/game/PhysicalConstants.cpp, $(EGO_CPPSRC))
EGO_OBJ    := ${EGO_SRC:.c=.o} ${EGO_CPPSRC:.cpp=.o}

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_C := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

#---------------------
# the egolib configuration

EGOLIB_L      := ../egolib/$(EGOLIB_TARGET)

#---------------------
# the compiler options

INC     := -Isrc -I../egolib/src
LDFLAGS := ${SDLCONF_L} ${EGOLIB_L} -lSDL_ttf -lSDL_mixer -lSDL_image -lphysfs -lenet -lGL -lGLU

# use different options if the environmental variable PREFIX is defined
ifeq ($(PREFIX),)
	OPT := -D_NO_PREFIX
else
	OPT := -DPREFIX=\"$(PREFIX)\" -D_NIX_PREFIX
endif


define link_rule
	libtool --mode=link \
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endef

CFLAGS  := ${INC} ${OPT} ${SDLCONF_C} -x c++ -std=c++11
CXXFLAGS:= ${CFLAGS}

#------------------------------------
# definitions of the target projects

.PHONY: all clean

all: $(EGO_TARGET)

$(EGO_TARGET): ${EGO_OBJ} 
	$(call link_rule)

clean:
	rm -f ${EGO_OBJ} $(EGO_TARGET)
