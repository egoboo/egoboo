#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

int main( int argc, char* argv[] )
{
  int f, c;
  Uint16 fmt;
  SDL_Init( 0 );
  Mix_QuerySpec( &f, &fmt, &c );
  TTF_Init();
  TTF_Quit();
  SDL_Quit();
  for ( f = 0; f < 70; ++f ) printf( "*" );printf( "\n" );
  printf( "If you can read this all is well -- compiling egoboo now\n" );
  for ( f = 0; f < 70; ++f ) printf( "*" );printf( "\n" );
  return 0;
}
