# Top level Makefile for Egoboo 2.x

PREFIX	:= ${HOME}/.local
PROJ_NAME	:= egoboo

all:
	make -C enet all
	make -C game all

clean:
	make -C enet clean
	make -C game clean

install: 
	mkdir -p ${PREFIX}/bin
	install -p -m 755 game/${PROJ_NAME}.sh ${PREFIX}/bin/${PROJ_NAME}

	mkdir -p ${PREFIX}/libexec
	install -m 755 game/${PROJ_NAME} ${PREFIX}/libexec