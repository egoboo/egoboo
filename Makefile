# Top level Makefile for Egoboo 2.x

#!!!! do not specify the prefix in this file
#!!!! if you want to specify a prefix, do it on the command line
#make PREFIX=$HOME/.local
#PREFIX	:= ${HOME}/.local

PROJ_NAME	:= egoboo-2.x

all:
	make -C enet all
	make -C game all

clean:
	make -C enet clean
	make -C game clean

install: ${EGO_BIN}

	echo Thank you for installing egoboo! 
	echo The default install of egoboo will require the commandline "sudo make install", and
	echo the required password
	echo
	echo If you do not have root access on this machine, you can specify a prefix
	echo on the command line: "make install PREFIX=$HOME/.local", where the environment
	echo variable PREFIX specifies a virtual root for your installation. In this example,
	echo it is a local installation for this username, only.
	echo

	#copy the binary to the games folder
	mkdir -p ${PREFIX}/games/
	install -m 755 ${PROJ_NAME} ${PREFIX}/games/${PROJ_NAME}
	
	#copy the data to the games folder
	mkdir -p ${PREFIX}/share/games/${PROJ_NAME}
	cp -r ./basicdat ./modules ${HOME}/games/${PROJ_NAME}

	#copy the players to the user's data folder
	mkdir -p ${HOME}/.${PROJ_NAME}
	cp -r ./players ${HOME}/.${PROJ_NAME}

	#copy the basic configuration files to the config directory
	mkdir -p ${PREFIX}/etc/${PROJ_NAME}
	cp -r setup.txt ${PREFIX}/etc/${PROJ_NAME}/setup.txt
	cp -r controls.txt ${PREFIX}/etc/${PROJ_NAME}/controls.txt
