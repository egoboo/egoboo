# Top level Makefile for Egoboo 2.x

#!!!! do not specify the prefix in this file
#!!!! if you want to specify a prefix, do it on the command line
#For instance: "make PREFIX=$HOME/.local"

ifndef ($(PREFIX),"")
	# define a value for prefix assuming that the program will be installed in the root directory
	PREFIX := /usr
endif


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

INSTALL_DIR		:= ../install

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
	# Thank you for installing egoboo! 
	#
	# The default install of egoboo will require the commandline 
	#     "sudo make install"
	# and the required password
	#
	# If you do not have root access on this machine, 
	# you can specify a prefix on the command line: 
	#     "make install PREFIX=$$HOME/.local"
	# where the environment variable PREFIX specifies a
	# virtual root for your installation. In this example,
	# it is a local installation for this username only.
	#

#	copy the binary to the games folder
	mkdir -p $(PREFIX)/games
	install -m 755 $(EGO_DIR)/$(PROJ_NAME) $(PREFIX)/games

#	copy the data to the games folder
	mkdir -p $(PREFIX)/share/games/$(PROJ_NAME)
	cp -rdf ./basicdat $(PREFIX)/share/games/$(PROJ_NAME)
	cp -rdf ./modules $(PREFIX)/share/games/$(PROJ_NAME)

#	copy the players to the user's data folder
	mkdir -p ${HOME}/.$(PROJ_NAME)
	mkdir -p ${HOME}/.$(PROJ_NAME)/players

#	copy the basic configuration files to the config directory
	mkdir -p $(PREFIX)/etc/$(PROJ_NAME)
	cp -rdf setup.txt $(PREFIX)/etc/$(PROJ_NAME)/setup.txt
	cp -rdf controls.txt $(PREFIX)/etc/$(PROJ_NAME)/controls.txt

	#####################################
	# Egoboo installation is finished
	#####################################

install_svn: egoboo

	######################################
	# This command will install egoboo using the
	# directory structure currently used in svn repository
	#

#	copy the binary to the games folder
	mkdir -p $(PREFIX)/games
	install -m 755 $(EGO_DIR)/$(PROJ_NAME) $(PREFIX)/games

#	copy the data to the games folder
	mkdir -p $(PREFIX)/share/games/$(PROJ_NAME)
	cp -rdf ../install/basicdat $(PREFIX)/share/games/$(PROJ_NAME)
	cp -rdf ../install/modules $(PREFIX)/share/games/$(PROJ_NAME)

#	copy the players to the user's data folder
	mkdir -p ${HOME}/.$(PROJ_NAME)
	mkdir -p ${HOME}/.$(PROJ_NAME)/players

#	copy the basic configuration files to the config directory
	mkdir -p $(PREFIX)/etc/$(PROJ_NAME)
	cp -rdf ../install/setup.txt $(PREFIX)/etc/$(PROJ_NAME)/setup.txt
	cp -rdf ../install/controls.txt $(PREFIX)/etc/$(PROJ_NAME)/controls.txt

	#####################################
	# Egoboo installation is finished
	#####################################
