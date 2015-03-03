#include <SDL_main.h>

#ifndef main

int SDL_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    return SDL_main(argc, argv);
}
#endif