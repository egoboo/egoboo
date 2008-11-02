# Basic Makefile for Egoboo 2.x under MSYS + MinGW

TARGET=egoboo2x.exe
SOURCES=camera.c\
	char.c\
	Client.c\
	Clock.c\
	common-file.c\
	configfile.c\
	egoboostrutil.c\
	enchant.c\
	Font.c\
	game.c\
	gltexture.c\
	graphic.c\
	graphicfan.c\
	graphicmad.c\
	graphicprt.c\
	id_normals.c\
	input.c\
	Log.c\
	MainLoop.c\
	mathstuff.c\
	Md2.c\
	menu.c\
	module.c\
	network.c\
	particle.c\
	passage.c\
	script.c\
	Server.c\
	sound.c\
	sys_win32.c\
	Task.c\
	Ticker.c\
	Timer.c\
	Ui.c\
	win-file.c

OBJECTS=$(SOURCES:%.c=%.o)

CFLAGS=`sdl-config --cflags` -I../enet/include
LDFLAGS=`sdl-config --libs` -L../enet -lopengl32 -lglu32 -lenet -lSDL_ttf -lws2_32 -lshlwapi -lshfolder -lshell32

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(CFLAGS) $(LDFLAGS)

clean:
	-rm -f $(TARGET) $(OBJECTS)
