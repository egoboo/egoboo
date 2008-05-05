# Top level Makefile for Egoboo 2.x
# For use in MSYS + MinGW

all:
	make -C enet
	make -C game
	cp game/egoboo2x.exe .

clean:
	make -C enet clean
	make -C game clean
