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

EGO_DIR           := ./game
EGO_TARGET        := $(PROJ_NAME)-$(PROJ_VERSION)

EGOLIB_DIR        := ./egolib
EGOLIB_TARGET     := lib$(PROJ_NAME).a

ifneq ($(USE_LUA),)
    EGOLIB_TARGET := lib$(PROJ_NAME)-lua.a
endif

#------------------------------------
# user defined macros

ifeq ($(PREFIX),)
	# define a value for prefix assuming that the program will be installed in the root directory
	PREFIX := /usr
endif

ifeq ($(INSTALL_DIR),)
	# the user can specify a non-standard location for "install"
	INSTALL_DIR := game/data
endif

export PREFIX EGOLIB_TARGET EGO_TARGET USE_LUA

#------------------------------------
# definitions of the target projects

.PHONY: all clean egolib egoboo

all: egoboo

$(EGOLIB_TARGET):
	make -C $(EGOLIB_DIR)

egolib: $(EGOLIB_TARGET)

$(EGO_TARGET): $(EGOLIB_TARGET)
	make -C $(EGO_DIR)

egoboo: $(EGO_TARGET)

clean:
	make -C $(EGOLIB_DIR) clean
	make -C $(EGO_DIR) clean

install: egoboo

	######################################
	# This command will install egoboo using the
	# directory structure currently used in svn repository
	#

#	copy the binary to the games folder
	mkdir -p $(PREFIX)/games
	install -m 755 $(EGO_DIR)/$(EGO_TARGET) $(PREFIX)/games

#	call the installer in the required install directory
	make -C $(INSTALL_DIR) install PROJ_NAME=$(EGO_TARGET)

	#####################################
	# Egoboo installation is finished
	#####################################
