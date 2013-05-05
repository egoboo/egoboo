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

EGO_DIR		:= ./game
EGO_TARGET		:= $(PROJ_NAME)-$(PROJ_VERSION)

EGOLIB_DIR		:= ./egolib
EGOLIB_TARGET	:= lib$(PROJ_NAME).la
EGOLIB_TARGET_LUA	:= lib$(PROJ_NAME)-lua.la

#------------------------------------
# user defined macros

ifndef ($(PREFIX),"")
	# define a value for prefix assuming that the program will be installed in the root directory
	PREFIX := /usr
endif

ifndef ($(INSTALL_DIR),"")
	# the user can specify a non-standard location for "install"
	INSTALL_DIR := ../install
endif

#------------------------------------
# definitions of the target projects

.PHONY: all clean

$(EGOLIB_DIR)/$(EGOLIB_TARGET):
	make -C $(EGOLIB_DIR) all PREFIX=$(PREFIX) EGOLIB_TARGET=$(EGOLIB_TARGET)

egolib:   $(EGOLIB_DIR)/$(EGOLIB_TARGET)

egoboo: egolib
	make -C $(EGO_DIR) all PREFIX=$(PREFIX) EGO_TARGET=$(EGO_TARGET) EGOLIB_DIR=$(EGOLIB_DIR)

$(EGOLIB_DIR)/$(EGOLIB_TARGET_LUA):
	make -C $(EGOLIB_DIR) -F Makefile.lua all PREFIX=$(PREFIX) EGOLIB_TARGET=$(EGOLIB_TARGET_LUA) 

egolib_lua:   $(EGOLIB_DIR)/$(EGOLIB_TARGET_LUA)
	
egoboo_lua: egolib_lua
	make -C $(EGO_DIR) -F Makefile.lua all PREFIX=$(PREFIX) EGO_TARGET=$(EGO_TARGET) EGOLIB_DIR=$(EGOLIB_DIR)

all: egolib egoboo

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
	install -m 755 $(EGO_DIR)/$(PROJ_NAME) $(PREFIX)/games

#	call the installer in the required install directory
	make -C INSTALL_DIR install

	#####################################
	# Egoboo installation is finished
	#####################################
