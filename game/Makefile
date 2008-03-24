# Basic Makefile for Egoboo 2.x under MSYS + MinGW

TARGET=egoboo2x.exe
SOURCES=camera.c\
	cartman.c\
	char.c\
	Client.c\
	Clock.c\
	configfile.c\
	egoboo_math.c\
	egoboo_strutil.c\
	enchant.c\
	file_common.c\
	file_win.c\
	Font.c\
	Frustum.c\
	game.c\
	graphic.c\
	graphic_fan.c\
	graphic_mad.c\
	graphic_prt.c\
	id_normals.c\
	input.c\
	Log.c\
	MainLoop.c\
	Md2.c\
	menu.c\
	mesh.c\
	module.c\
	network.c\
	ogl_include.c\
	ogl_texture.c\
	particle.c\
	passage.c\
	script.c\
	SDL_GetPixel.c\
	SDL_PutPixel.c\
	SDL_rotozoom.c\
	Server.c\
	sound.c\
	System_win32.c\
	Task.c\
	Ticker.c\
	Timer.c\
	Ui.c


OBJECTS=$(SOURCES:%.c=%.o)

CFLAGS=`sdl-config --cflags` -I../enet/include
LDFLAGS=`sdl-config --libs` -L../enet -lopengl32 -lglu32 -lenet -lSDL_ttf -lws2_32 -lshlwapi -lshfolder -lshell32

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(CFLAGS) $(LDFLAGS)

clean:
	-rm -f $(TARGET) $(OBJECTS)
