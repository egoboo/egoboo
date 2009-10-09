//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file font_ttf.c
/// @brief TTF management
/// @details True-type font drawing functionality.  Uses Freetype 2 & OpenGL
/// to do it's business.

#include "font_ttf.h"
#include "log.h"

#include "egoboo_typedef.h"
#include "egoboo_strutil.h"

#include "ogl_include.h"
#include "ogl_debug.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct Font
{
    TTF_Font *ttfFont;

    GLuint texture;
    GLfloat texCoords[4];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int fnt_atexit_registered = 0;

int fnt_init()
{
    /// @details BB@> Make sure the TTF library was initialized

    int initialized;

    initialized = TTF_WasInit();
    if ( !initialized )
    {
        log_info( "Initializing the SDL_ttf font handler version %i.%i.%i... ", SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION, SDL_TTF_PATCHLEVEL );
        if ( TTF_Init() < 0 )
        {
            log_message( "Failed!\n" );
        }
        else
        {
            log_message( "Success!\n" );

            if ( !fnt_atexit_registered )
            {
                fnt_atexit_registered  = 1;
                atexit( TTF_Quit );
            }

            initialized = 1;
        }
    }

    return initialized;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------
int copySurfaceToTexture( SDL_Surface *surface, GLuint texture, GLfloat *texCoords )
{
    int w, h;
    SDL_Surface *image;
    SDL_Rect area;
    Uint32   saved_flags;
    Uint8    saved_alpha;

    // Use the surface width & height expanded to the next powers of two
    w = powerOfTwo( surface->w );
    h = powerOfTwo( surface->h );
    texCoords[0] = 0.0f;
    texCoords[1] = 0.0f;
    texCoords[2] = ( GLfloat )surface->w / w;
    texCoords[3] = ( GLfloat )surface->h / h;

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
    if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
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
    if ( ( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
    {
        SDL_SetAlpha( surface, saved_flags, saved_alpha );
    }

    // Send the texture to OpenGL
    GL_DEBUG(glBindTexture)(GL_TEXTURE_2D, texture );
    GL_DEBUG(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    GL_DEBUG(glTexParameteri)(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    GL_DEBUG(glTexImage2D)(GL_TEXTURE_2D,  0, GL_RGBA,  w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,  image->pixels );

    // Don't need the extra image anymore
    SDL_FreeSurface( image );

    return 1;
}

//--------------------------------------------------------------------------------------------
Font* fnt_loadFont( const char *fileName, int pointSize )
{
    Font *newFont;
    TTF_Font *ttfFont;
    if ( !fnt_init() )
    {
        printf( "fnt_loadFont: Could not initialize SDL_TTF!\n" );
        return NULL;
    }

    // Try and open the font
    ttfFont = TTF_OpenFont( fileName, pointSize );
    if ( NULL == ttfFont )
    {
        // couldn't open it, for one reason or another
        return NULL;
    }

    // Everything looks good
    newFont = ( Font* )malloc( sizeof( Font ) );
    newFont->ttfFont = ttfFont;
    GL_DEBUG(glGenTextures)(1, &newFont->texture );

    return newFont;
}

//--------------------------------------------------------------------------------------------
void fnt_freeFont( Font *font )
{
    if ( font )
    {
        TTF_CloseFont( font->ttfFont );
        GL_DEBUG(glDeleteTextures)(1, &font->texture );
        free( font );
    }
}

//--------------------------------------------------------------------------------------------
int fnt_print_raw( Font *font, SDL_Color color, SDL_Surface ** psurf, GLuint itex, float texCoords[], const char * szText )
{
    int rv;
    SDL_Surface *textSurf, **pptmp;
    bool_t sdl_surf_external;

    if ( NULL == font || NULL == texCoords ) return -1;

    if ( NULL != psurf )
    {
        sdl_surf_external = btrue;
        pptmp = psurf;
    }
    else
    {
        sdl_surf_external = bfalse;
        pptmp = &textSurf;
    }

    if ( INVALID_TX_ID == itex ) return -1;

   // create the text
    (*pptmp) = TTF_RenderText_Blended( font->ttfFont, szText, color );
    if (NULL == (*pptmp)) return -1;

    // upload the texture
    rv = 0;
    if ( !copySurfaceToTexture( (*pptmp), itex, texCoords ) )
    {
        rv = -1;
    }

    if ( !sdl_surf_external )
    {
        // Done with the surface
        SDL_FreeSurface( *pptmp );
        *pptmp = NULL;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
int fnt_vprintf( Font *font, SDL_Color color, SDL_Surface ** psurf, GLuint itex, float texCoords[], const char *format, va_list args )
{
    int rv;
    STRING szText = EMPTY_CSTR;

    // evaluate the variable args
    rv = vsnprintf( szText, SDL_arraysize(szText) - 1, format, args );
    if ( rv < 0 )
    {
        return rv;
    }

    return fnt_print_raw( font, color, psurf, itex, texCoords, szText );
}

//--------------------------------------------------------------------------------------------
void fnt_drawText_raw( Font *font, int x, int y, const char *text )
{
    int rv;
    SDL_Surface *textSurf;
    SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };

    rv = fnt_print_raw( font, color, &textSurf, font->texture, font->texCoords, text );
    if ( rv < 0 ) return;

    // And draw the darn thing
    GL_DEBUG(glBegin)( GL_QUADS );
    {
        GL_DEBUG(glTexCoord2f)(font->texCoords[0], font->texCoords[1] );
        GL_DEBUG(glVertex2f)( x, y );

        GL_DEBUG(glTexCoord2f)(font->texCoords[2], font->texCoords[1] );
        GL_DEBUG(glVertex2f)( x + textSurf->w, y );

        GL_DEBUG(glTexCoord2f)(font->texCoords[2], font->texCoords[3] );
        GL_DEBUG(glVertex2f)( x + textSurf->w, y + textSurf->h );

        GL_DEBUG(glTexCoord2f)(font->texCoords[0], font->texCoords[3] );
        GL_DEBUG(glVertex2f)( x, y + textSurf->h );
    }
    GL_DEBUG_END();

    if ( NULL != textSurf )
    {
        // Done with the surface
        SDL_FreeSurface( textSurf );
        textSurf = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void fnt_drawText( Font *font, int x, int y, const char *format, ...   )
{
    va_list args;
    int rv;
    SDL_Surface *textSurf;
    SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };

    va_start( args, format );
    rv = fnt_vprintf( font, color, &textSurf, font->texture, font->texCoords, format, args );
    va_end( args );

    if ( rv < 0 ) return;

    // And draw the darn thing
    GL_DEBUG(glBegin)( GL_QUADS );
    {
        GL_DEBUG(glTexCoord2f)(font->texCoords[0], font->texCoords[1] );
        GL_DEBUG(glVertex2f)( x, y );

        GL_DEBUG(glTexCoord2f)(font->texCoords[2], font->texCoords[1] );
        GL_DEBUG(glVertex2f)( x + textSurf->w, y );

        GL_DEBUG(glTexCoord2f)(font->texCoords[2], font->texCoords[3] );
        GL_DEBUG(glVertex2f)( x + textSurf->w, y + textSurf->h );

        GL_DEBUG(glTexCoord2f)(font->texCoords[0], font->texCoords[3] );
        GL_DEBUG(glVertex2f)( x, y + textSurf->h );
    }
    GL_DEBUG_END();

    if ( NULL != textSurf )
    {
        // Done with the surface
        SDL_FreeSurface( textSurf );
        textSurf = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void fnt_getTextSize( Font *font, const char *text, int *width, int *height )
{
    if ( font )
    {
        TTF_SizeText( font->ttfFont, text, width, height );
    }
}

//--------------------------------------------------------------------------------------------
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
void fnt_drawTextBox( Font *font, int x, int y, int width, int height, int spacing, const char *format, ...   )
{
    va_list args;
    int rv;
    int len;
    char *buffer, *line;
    char text[4096] = EMPTY_CSTR;

    va_start( args, format );
    rv = vsnprintf( text, SDL_arraysize(text), format, args );
    va_end( args );

    // some problem printing the text
    if ( rv < 0 ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = (char *)calloc( 1, len + 1 );
    strncpy( buffer, text, len );

    line = strtok( buffer, "\n" );

    while ( line != NULL )
    {
        fnt_drawText_raw( font, x, y, line );
        y += spacing;
        line = strtok( NULL, "\n" );
    }

    free( buffer );
}

//--------------------------------------------------------------------------------------------
void fnt_getTextBoxSize( Font *font, const char *text, int spacing, int *width, int *height )
{
    char *buffer, *line;
    int len;
    int tmp_w, tmp_h;
    if ( !font ) return;
    if ( !text || !text[0] ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = (char *)calloc( 1, len + 1 );
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
