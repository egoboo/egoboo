This directory contains a converter from .bmps to premultiplied-alpha .pngs for Egoboo.

If the bitmaps are paletted, it will be converted into the RGB colorspace.

It requires SDL2 and SDL2_image.

## Compiling for Linux

```
g++ -o convertpalette convertpalette.cpp --std c++11 `sdl2-config --cflags --libs` -lSDL2_image
```

## Usage

`./convertpalette <files to convert or directories to search>`

Convertpalette takes files and directories as arguments, converting the files and searching 
the directories recursively for files to convert.