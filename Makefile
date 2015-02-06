# Top level Makefile for Egoboo 2.x

#!!!! do not specify the prefix in this file
#!!!! if you want to specify a prefix, do it on the command line
#For instance: "make PREFIX=$HOME/.local"

#---------------------
# some project definitions

PROJ_NAME	:= egoboo
PROJ_VERSION	:= 2.x

#---------------------
# the target names

EGO_DIR           := game
EGO_TARGET        := $(PROJ_NAME)-$(PROJ_VERSION)

EGOLIB_DIR        := egolib
EGOLIB_TARGET     := lib$(PROJ_NAME).a

CARTMAN_DIR       := cartman
CARTMAN_TARGET    := $(PROJ_NAME)-cartman

INSTALL_DIR       := data

#---------------------
# the SDL configuration

SDL_CONF  := sdl-config
TMPFLAGS  := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

#---------------------
# the compiler options
# todo: use pkg-config for lua?
TMPFLAGS += -I/usr/include/lua5.2
TMPFLAGS += -x c++ -std=c++11

# for now, find a better way to do this?
ifeq ($(PREFIX),)
	PREFIX := /usr
endif

# use different options if the environmental variable PREFIX is defined
ifeq ($(PREFIX),)
	TMPFLAGS += -D_NO_PREFIX
else
	TMPFLAGS += -DPREFIX=\"$(PREFIX)\" -D_NIX_PREFIX
endif

CFLAGS   += $(TMPFLAGS)
CXXFLAGS += $(TMPFLAGS)
LDFLAGS  := ${SDLCONF_L} -lSDL_ttf -lSDL_mixer -lSDL_image -lphysfs -lenet -llua5.2 -lGL -lGLU

export PREFIX CFLAGS CXXFLAGS LDFLAGS EGOLIB_TARGET EGO_TARGET CARTMAN_TARGET

#------------------------------------
# definitions of the target projects

.PHONY: all clean egolib egoboo cartman

all: egoboo

$(EGOLIB_TARGET):
	${MAKE} -C $(EGOLIB_DIR)

egolib: $(EGOLIB_TARGET)

$(EGO_TARGET): $(EGOLIB_TARGET)
	${MAKE} -C $(EGO_DIR)

egoboo: $(EGO_TARGET)

$(CARTMAN_TARGET): $(EGOLIB_TARGET)
	${MAKE} -C $(CARTMAN_DIR)

cartman: $(CARTMAN_TARGET)

clean:
	${MAKE} -C $(EGOLIB_DIR) clean
	${MAKE} -C $(EGO_DIR) clean
	${MAKE} -C $(CARTMAN_DIR) clean

install: egoboo

	######################################
	# This command will install egoboo using the
	# directory structure currently used in svn repository
	#

#	copy the binary to the games folder
	mkdir -p $(PREFIX)/games
	install -m 755 $(EGO_DIR)/$(EGO_TARGET) $(PREFIX)/games

#	call the installer in the required install directory
	${MAKE} -C $(INSTALL_DIR) install PROJ_NAME=$(EGO_TARGET)

	#####################################
	# Egoboo installation is finished
	#####################################
