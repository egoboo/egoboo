# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGOLIB_SOURCE := $(wildcard src/egolib/*.c)
EGOLIB_SOURCE += $(wildcard src/egolib/*/*.c)
EGOLIB_CPPSRC := $(wildcard src/egolib/*.cpp)
EGOLIB_CPPSRC += $(wildcard src/egolib/*/*.cpp)

# remove some files
EGOLIB_SOURCE := $(filter-out src/egolib/bsp_leaf.c, $(EGOLIB_SOURCE))
EGOLIB_SOURCE := $(filter-out src/egolib/platform/file_win.c, $(EGOLIB_SOURCE))
EGOLIB_SOURCE := $(filter-out src/egolib/platform/sys_win32.c, $(EGOLIB_SOURCE))

ifeq ($(USE_LUA),)
	EGOLIB_SOURCE := $(filter-out src/egolib/lua/lua_console.c, $(EGOLIB_SOURCE))
endif

EGOLIB_OBJ := ${EGOLIB_SOURCE:.c=.o} ${EGOLIB_CPPSRC:.cpp=.o}

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_C := $(shell ${SDL_CONF} --cflags)

#---------------------
# the compiler options

INC	:= -Isrc -I../game/src

ifneq ($(USE_LUA),)
# use pkg-config for this?
	INC += -I/usr/include/lua5.2
endif

# use different options if the environmental variable PREFIX is defined
ifeq ($(PREFIX),)
	OPT := -D_NO_PREFIX
else
	OPT := -DPREFIX=\"$(PREFIX)\" -D_NIX_PREFIX
endif

CFLAGS  := ${SDLCONF_C} ${OPT} ${INC} -x c++ -std=c++11
CXXFLAGS:= ${CFLAGS}

#------------------------------------
# definitions of the target projects

.PHONY: all clean

$(EGOLIB_TARGET): ${EGOLIB_OBJ}
	$(AR) -r $@ $^

all: $(EGOLIB_TARGET)

clean:
	rm -f ${EGOLIB_OBJ} $(EGOLIB_TARGET)
