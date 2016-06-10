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
EGOLIB_TARGET     := libegolib.a

IDLIB_DIR         := idlib
IDLIB_TARGET      := libidlib.a

CARTMAN_DIR       := cartman
CARTMAN_TARGET    := cartman

EGOTOOL_DIR       := utilities/convertpaletted
EGOTOOL_TARGET    := egotool

INSTALL_DIR       := data

#---------------------
# the SDL configuration

SDL_CONF  := sdl2-config
TMPFLAGS  := $(shell ${SDL_CONF} --cflags)
SDLCONF_L := $(shell ${SDL_CONF} --libs)

EXTERNAL_LUA := $(shell pwd)/external/lua-5.2.3

# set LUA_LDFLAGS to not use the lua in external/
ifeq ($(LUA_LDFLAGS),)
LUA_CFLAGS := -I${EXTERNAL_LUA}/src
LUA_LDFLAGS := ${EXTERNAL_LUA}/src/liblua.a
USE_EXTERNAL_LUA := 1
endif

#---------------------
# the compiler options
TMPFLAGS += -std=c++14 $(LUA_CFLAGS)

# for now, find a better way to do this?
ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

# use different options if the environmental variable PREFIX is defined
ifeq ($(PREFIX),)
	TMPFLAGS += -D_NO_PREFIX
else
	TMPFLAGS += -DPREFIX=\"$(PREFIX)\" -D_NIX_PREFIX
endif

EGO_CXXFLAGS = $(TMPFLAGS)
EGO_LDFLAGS  = -pthread $(LUA_LDFLAGS) ${SDLCONF_L} -lSDL2_ttf -lSDL2_mixer -lSDL2_image -lphysfs -lGL

export PREFIX EGO_CXXFLAGS EGO_LDFLAGS IDLIB_TARGET EGOLIB_TARGET EGO_TARGET CARTMAN_TARGET EGOTOOL_TARGET

#------------------------------------
# definitions of the target projects

.PHONY: all clean idlib egolib egoboo cartman install doxygen external_lua test egotool

all: idlib egolib egoboo cartman egotool

idlib: external_lua
	${MAKE} -C $(IDLIB_DIR)

egolib: idlib
	${MAKE} -C $(EGOLIB_DIR)

egoboo: egolib
	${MAKE} -C $(EGO_DIR)

cartman: egolib
	${MAKE} -C $(CARTMAN_DIR)

egotool: egolib
	${MAKE} -C $(EGOTOOL_DIR)

test: all
	${MAKE} -C ${IDLIB_DIR} test
	${MAKE} -C ${EGOLIB_DIR} test

external_lua:
ifeq ($(USE_EXTERNAL_LUA), 1)
	${MAKE} -C $(EXTERNAL_LUA)/src liblua.a SYSCFLAGS="-DLUA_USE_POSIX"
endif

clean:
	${MAKE} -C $(IDLIB_DIR) clean
	${MAKE} -C $(EGOLIB_DIR) clean
	${MAKE} -C $(EGO_DIR) clean
	${MAKE} -C $(CARTMAN_DIR) clean
	${MAKE} -C $(EGOTOOL_DIR) clean
ifeq ($(USE_EXTERNAL_LUA), 1)
	${MAKE} -C $(EXTERNAL_LUA) clean
endif

doxygen:
	doxygen

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

uninstall:
	rm $(PREFIX)/games/$(EGO_TARGET)
	${MAKE} -C $(INSTALL_DIR) uninstall PROJ_NAME=$(EGO_TARGET)
