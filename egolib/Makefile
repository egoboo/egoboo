# Do not run this file. Run the Makefile in the parent directory, instead

EGOLIB_TARGET = lib$(PROJ_NAME).la

#---------------------
# the source files

EGOLIB_SRC         := $(wildcard ./*.c)
EGOLIB_PLATFORM    := $(wildcard ./file_formats/*_linux.c)
EGOLIB_FILE_FORMAT := $(wildcard ./file_formats/*.c)
EGOLIB_EXTENSIONS  := $(wildcard ./extensions/*.c)

EGOLIB_OBJ := ${EGOLIB_SRC:.c=.o} ${EGOLIB_FILE_FORMAT:.c=.o} ${EGOLIB_EXTENSIONS:.c=.o} ${EGOLIB_PLATFORM:.c=.o}

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_I := $(shell ${SDL_CONF} --cflags)

#---------------------
# the compiler options

CC      := gcc
INC     := -I. -I.. ${SDLCONF_I} -I./extensions -I./file_formats -I./platform

# use different options if the environmental variable PREFIX is defined
ifdef ($(PREFIX),"")
	OPT := -Os -Wall
else
	OPT := -Os -Wall -DPREFIX=\"${PREFIX}\" -D_NIX_PREFIX
endif

CFLAGS  := ${OPT} ${INC}

#------------------------------------
# definitions of the libtool commands

define compile_rule
        libtool --mode=compile \
        $(CC) $(CFLAGS) $(CPPFLAGS) -c $<
endef

define link_rule
        libtool --mode=link \
        $(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endef

#------------------------------------
# definitions of the target projects

.PHONY: all clean

%.lo: %.c
        $(call compile_rule)

${EGOLIB_TARGET}: $(EGOLIB_OBJ)
        $(call link_rule)

install/%.la: %.la
        libtool --mode=install \
        install -c $(notdir $@) $(libdir)/$(notdir $@)

install: $(addprefix install/,$(LIBS))
        libtool --mode=finish $(libdir)

all: ${EGOLIB_TARGET}

clean:
	rm -f ${ENET_OBJ} ${EGOLIB_OBJ} ${EGOLIB_TARGET}
