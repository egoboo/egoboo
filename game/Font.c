/* Egoboo - Font.c
 * True-type font drawing functionality.  Uses Freetype 2 & OpenGL
 * to do it's business.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gltexture.h"
#include "Font.h"
#include "Log.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>

#ifndef __unix__
#define vsnprintf _vsnprintf
#endif

struct font_t
{
  TTF_Font *ttfFont;

  GLuint texture;
  GLfloat texCoords[4];
};

// The next two functions are borrowed from the gl_font.c test program from SDL_ttf
static int powerOfTwo( int input )
{
  int value = 1;

  while ( value < input )
  {
    value <<= 1;
  }

  return value;
}

int copySurfaceToTexture( SDL_Surface *surface, GLuint texture, GLfloat *texCoords )
{
  int w, h;
  SDL_Surface *image;
  SDL_Rect area;
  Uint32 saved_flags;
  Uint8 saved_alpha;

  // Use the surface width & height expanded to the next powers of two
  w = powerOfTwo( surface->w );
  h = powerOfTwo( surface->h );
  texCoords[0] = 0.0f;
  texCoords[1] = 0.0f;
  texCoords[2] = ( GLfloat ) surface->w / w;
  texCoords[3] = ( GLfloat ) surface->h / h;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  image = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );
#else
  image = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
#endif

  if ( image == NULL )
  {
    return 0;
  }

  // Save the alpha blending attributes
  saved_flags = surface->flags & ( SDL_SRCALPHA | SDL_RLEACCELOK );
  saved_alpha = surface->format->alpha;
  if (( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
  {
    SDL_SetAlpha( surface, 0, 0 );
  }

  // Copy the surface into the texture image
  area.x = 0;
  area.y = 0;
  area.w = surface->w;
  area.h = surface->h;
  SDL_BlitSurface( surface, &area, image, &area );

  // Restore the blending attributes
  if (( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
  {
    SDL_SetAlpha( surface, saved_flags, saved_alpha );
  }

  // Send the texture to OpenGL
  glBindTexture( GL_TEXTURE_2D, texture );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );

  // Don't need the extra image anymore
  SDL_FreeSurface( image );

  return 1;
}

Font* fnt_loadFont( const char *fileName, int pointSize )
{
  Font *newFont;
  TTF_Font *ttfFont;

  // Make sure the TTF library was initialized
  if ( !TTF_WasInit() )
  {
    if ( TTF_Init() != -1 )
    {
      atexit( TTF_Quit );
    }
    else
    {
      log_warning( "fnt_loadFont() - Could not initialize SDL_TTF!\n" );
      return NULL;
    }
  }

  // Try and open the font
  ttfFont = TTF_OpenFont( fileName, pointSize );
  if ( NULL == ttfFont )
  {
    // couldn't open it, for one reason or another
    return NULL;
  }

  // Everything looks good
  newFont = ( Font* ) malloc( sizeof( Font ) );
  newFont->ttfFont = ttfFont;
  newFont->texture = 0;

  return newFont;
}

void fnt_freeFont( Font *font )
{
  if ( font )
  {
    TTF_CloseFont( font->ttfFont );
    glDeleteTextures( 1, &font->texture );
    free( font );
  }
}

void fnt_drawText( Font *font, int x, int y, const char *text )
{
  SDL_Surface *textSurf;
  SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };

  if ( font )
  {
    // Let TTF render the text
    textSurf = TTF_RenderText_Blended( font->ttfFont, text, color );

    // Does this font already have a texture?  If not, allocate it here
    if ( font->texture == 0 )
    {
      glGenTextures( 1, &font->texture );
    }

    // Copy the surface to the texture
    if ( copySurfaceToTexture( textSurf, font->texture, font->texCoords ) )
    {
      // And draw the darn thing
      glBegin( GL_TRIANGLE_STRIP );
      glTexCoord2f( font->texCoords[0], font->texCoords[1] );
      glVertex2i( x, y );
      glTexCoord2f( font->texCoords[2], font->texCoords[1] );
      glVertex2i( x + textSurf->w, y );
      glTexCoord2f( font->texCoords[0], font->texCoords[3] );
      glVertex2i( x, y + textSurf->h );
      glTexCoord2f( font->texCoords[2], font->texCoords[3] );
      glVertex2i( x + textSurf->w, y + textSurf->h );
      glEnd();
    }

    // Done with the surface
    SDL_FreeSurface( textSurf );
  }
}

void fnt_getTextSize( Font *font, const char *text, int *width, int *height )
{
  if ( font )
  {
    TTF_SizeText( font->ttfFont, text, width, height );
  }
}

/** font_drawTextBox
 * Draws a text string into a box, splitting it into lines according to newlines in the string.
 * NOTE: Doesn't pay attention to the width/height arguments yet.
 *
 * font    - The font to draw with
 * text    - The text to draw
 * x       - The x position to start drawing at
 * y       - The y position to start drawing at
 * width   - Maximum width of the box (not implemented)
 * height  - Maximum height of the box (not implemented)
 * spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void fnt_drawTextBox( Font *font, const char *text, int x, int y, int width, int height, int spacing )
{
  size_t len;
  char *buffer, *line;

  if ( NULL == font ) return;

  // If text is empty, there's nothing to draw
  if ( NULL == text || '\0' == text[0] ) return;

  // Split the passed in text into separate lines
  len = strlen( text );
  buffer = calloc( 1, len + 1 );
  strncpy( buffer, text, len );

  line = strtok( buffer, "\n" );
  while ( line != NULL )
  {
    fnt_drawText( font, x, y, line );
    y += spacing;
    line = strtok( NULL, "\n" );
  }
  free( buffer );
}

void fnt_getTextBoxSize( Font *font, const char *text, int spacing, int *width, int *height )
{
  char *buffer, *line;
  size_t len;
  int tmp_w, tmp_h;

  if ( NULL == font ) return;
  if ( NULL == text || '\0' == text[0] ) return;

  // Split the passed in text into separate lines
  len = strlen( text );
  buffer = calloc( 1, len + 1 );
  strncpy( buffer, text, len );

  line = strtok( buffer, "\n" );
  *width = *height = 0;
  while ( line != NULL )
  {
    TTF_SizeText( font->ttfFont, line, &tmp_w, &tmp_h );
    *width = ( *width > tmp_w ) ? *width : tmp_w;
    *height += spacing;

    line = strtok( NULL, "\n" );
  }
  free( buffer );
}

void fnt_drawTextFormatted( Font * fnt, int x, int y, const char *format, ... )
{
  va_list args;
  char buffer[256];

  va_start( args, format );
  vsnprintf( buffer, 256, format, args );
  va_end( args );

  fnt_drawText( fnt, x, y, buffer );
}
