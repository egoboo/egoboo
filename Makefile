# Top level Makefile for Egoboo 2.x

#!!!! do not specify the prefix in this file
#!!!! if you want to specify a prefix, do it on the command line
#For instance: "make PREFIX=$HOME/.local"

#---------------------
# some project definitions

PROJ_NAME		:= egoboo
PROJ_VERSION	:= 2.x

#---------------------
# the target names

EGO_DIR           := ./game
EGO_TARGET        := $(PROJ_NAME)-$(PROJ_VERSION)

EGOLIB_DIR        := ./egolib
EGOLIB_TARGET     := lib$(PROJ_NAME).la
EGOLIB_TARGET_LUA := lib$(PROJ_NAME)-lua.la

#------------------------------------
# user defined macros

ifndef ($(PREFIX),"")
	# define a value for prefix assuming that the program will be installed in the root directory
	PREFIX := /usr
endif

ifndef ($(INSTALL_DIR),"")
	# the user can specify a non-standard location for "install"
	INSTALL_DIR := game/data
endif

#------------------------------------
# definitions of the target projects

.PHONY: all clean egolib egoboo egolib_lua egoboo_lua

egolib:
	make -C $(EGOLIB_DIR) PREFIX=$(PREFIX) EGOLIB_TARGET=$(EGOLIB_TARGET)

egoboo: egolib
	make -C $(EGO_DIR) PREFIX=$(PREFIX) EGO_TARGET=$(EGO_TARGET) EGOLIB_TARGET=$(EGOLIB_TARGET)

egolib_lua:
	make -C $(EGOLIB_DIR) -F Makefile.lua PREFIX=$(PREFIX) EGOLIB_TARGET=$(EGOLIB_TARGET_LUA)
	
egoboo_lua: egolib_lua
	make -C $(EGO_DIR) -F Makefile.lua PREFIX=$(PREFIX) EGO_TARGET=$(EGO_TARGET) EGOLIB_TARGET=$(EGOLIB_TARGET_LUA)

all: egolib egoboo

clean:
	make -C $(EGOLIB_DIR) clean EGOLIB_TARGET=$(EGOLIB_TARGET)
	make -C $(EGO_DIR) clean EGO_TARGET=$(EGOLIB_TARGET)

install: egoboo

	######################################
	# This command will install egoboo using the
	# directory structure currently used in svn repository
	#

#	copy the binary to the games folder
	mkdir -p $(PREFIX)/games
	install -m 755 $(EGO_DIR)/$(EGO_TARGET) $(PREFIX)/games

#	call the installer in the required install directory
	make -C $(INSTALL_DIR) install PREFIX=$(PREFIX) PROJ_NAME=$(EGO_TARGET)

	#####################################
	# Egoboo installation is finished
	#####################################
