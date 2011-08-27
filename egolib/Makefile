# Do not run this file. Run the Makefile in the parent directory, instead

#---------------------
# the source files

EGOLIB_SRC			:= $(wildcard ./*.c)
EGOLIB_PLATFORM		:= $(wildcard ./file_formats/*_linux.c)
EGOLIB_FILE_FORMAT	:= $(wildcard ./file_formats/*.c)
EGOLIB_EXTENSIONS		:= $(wildcard ./extensions/*.c)

EGOLIB_OBJ := ${EGOLIB_SRC:.c=.lo} ${EGOLIB_FILE_FORMAT:.c=.lo} ${EGOLIB_EXTENSIONS:.c=.lo} ${EGOLIB_PLATFORM:.c=.lo}

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
SDLCONF_C := $(shell ${SDL_CONF} --cflags)

#---------------------
# the compiler options

CC	:= gcc-3
INC	:= -I. -I./extensions -I./file_formats -I./platform

# use different options if the environmental variable PREFIX is defined
ifdef ($(PREFIX),"")
	OPT := -D_NO_PREFIX
else
	OPT := -DPREFIX=\"$(PREFIX)\" -D_NIX_PREFIX
endif

CFLAGS  := ${SDLCONF_C} ${OPT} ${INC}

#------------------------------------
# definitions of the libtool commands

define compile_rule
	libtool --mode=compile --tag=CC \
	$(CC) $(CFLAGS) -c $<
endef

define link_rule
	libtool --mode=link \
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
endef

define install_rule
	libtool --mode=install \
	install -c $(notdir $@) $(libdir)/$(notdir $@)
endef

#------------------------------------
# definitions of the target projects

.PHONY: all clean

%.lo: %.c
	$(call compile_rule)

$(EGOLIB_TARGET): ${EGOLIB_OBJ}
	$(call link_rule)

install/%.la: %.la
	$(call install_rule)

install: $(addprefix install/,${EGOLIB_OBJ})
	libtool --mode=finish $(libdir)

all: $(EGOLIB_TARGET)

clean:
	rm -f ${EGOLIB_OBJ} $(EGOLIB_TARGET)
