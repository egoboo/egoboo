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

#include "extensions/ogl_include.h"
#include "extensions/ogl_debug.h"
#include "extensions/SDL_GL_extensions.h"

#include "egoboo_typedef.h"
#include "egoboo_strutil.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the trye-type font data for Egoboo
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
    newFont = EGOBOO_NEW( Font );
    newFont->ttfFont = ttfFont;
    GL_DEBUG( glGenTextures )( 1, &newFont->texture );

    return newFont;
}

//--------------------------------------------------------------------------------------------
void fnt_freeFont( Font *font )
{
    if ( font )
    {
        TTF_CloseFont( font->ttfFont );
        GL_DEBUG( glDeleteTextures )( 1, &font->texture );
        EGOBOO_DELETE( font );
    }
}

//--------------------------------------------------------------------------------------------
int fnt_print_raw( Font *font, SDL_Color color, SDL_Surface ** ppSurface, GLuint itex, float texCoords[], const char * szText )
{
    int rv;
    bool_t sdl_surf_external;
    SDL_Surface *textSurf = NULL, **pptmp = NULL, *ptmp;

    if ( NULL == font || NULL == texCoords ) return -1;

    if ( NULL != ppSurface )
    {
        sdl_surf_external = btrue;
        pptmp = ppSurface;
    }
    else
    {
        sdl_surf_external = bfalse;
        pptmp = &textSurf;
    }

    if ( INVALID_GL_ID == itex )
    {
        rv = -1;
        goto fnt_print_raw_finish;
    }

    // create the text
    ptmp = TTF_RenderText_Blended( font->ttfFont, szText, color );
    if ( NULL != ptmp )
    {
        SDL_FreeSurface( *pptmp );
        *pptmp = ptmp;
    }
    else
    {
        rv = -1;
        goto fnt_print_raw_finish;
    }

    // upload the texture
    rv = 0;
    if ( !SDL_GL_uploadSurface( *pptmp, itex, texCoords ) )
    {
        rv = -1;
    }

fnt_print_raw_finish:

    if ( !sdl_surf_external && NULL != *pptmp )
    {
        // Done with the surface
        SDL_FreeSurface( *pptmp );
        *pptmp = NULL;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
int fnt_vprintf( Font *font, SDL_Color color, SDL_Surface ** ppSurface, GLuint itex, float texCoords[], const char *format, va_list args )
{
    int rv;
    STRING szText = EMPTY_CSTR;

    // evaluate the variable args
    rv = vsnprintf( szText, SDL_arraysize( szText ) - 1, format, args );
    if ( rv < 0 ) return rv;

    return fnt_print_raw( font, color, ppSurface, itex, texCoords, szText );
}

//--------------------------------------------------------------------------------------------
void fnt_drawText_raw( Font *font, int x, int y, const char *text, SDL_Surface ** ppSurface )
{
    int rv;
    SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };

    bool_t sdl_surf_external;
    SDL_Surface *textSurf = NULL, **pptmp = NULL;

    if ( NULL != ppSurface )
    {
        sdl_surf_external = btrue;
        pptmp = ppSurface;
    }
    else
    {
        sdl_surf_external = bfalse;
        pptmp = &textSurf;
    }

    rv = fnt_print_raw( font, color, pptmp, font->texture, font->texCoords, text );
    if ( rv < 0 ) goto fnt_drawText_raw_finish;

    // And draw the darn thing
    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( font->texCoords[0], font->texCoords[1] );
        GL_DEBUG( glVertex2f )( x, y );

        GL_DEBUG( glTexCoord2f )( font->texCoords[2], font->texCoords[1] );
        GL_DEBUG( glVertex2f )( x + ( *pptmp )->w, y );

        GL_DEBUG( glTexCoord2f )( font->texCoords[2], font->texCoords[3] );
        GL_DEBUG( glVertex2f )( x + ( *pptmp )->w, y + ( *pptmp )->h );

        GL_DEBUG( glTexCoord2f )( font->texCoords[0], font->texCoords[3] );
        GL_DEBUG( glVertex2f )( x, y + ( *pptmp )->h );
    }
    GL_DEBUG_END();

fnt_drawText_raw_finish:

    if ( !sdl_surf_external && NULL != *pptmp )
    {
        // Done with the surface
        SDL_FreeSurface( *pptmp );
        *pptmp = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void fnt_drawText( Font *font, SDL_Surface ** ppSurface, int x, int y, const char *format, ... )
{
    va_list args;
    int rv;
    SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };

    bool_t sdl_surf_external;
    SDL_Surface *textSurf = NULL, **pptmp = NULL;

    if ( NULL != ppSurface )
    {
        sdl_surf_external = btrue;
        pptmp = ppSurface;
    }
    else
    {
        sdl_surf_external = bfalse;
        pptmp = &textSurf;
    }

    va_start( args, format );
    rv = fnt_vprintf( font, color, pptmp, font->texture, font->texCoords, format, args );
    va_end( args );

    if ( rv <= 0 )
    {
        // And draw the darn thing
        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( font->texCoords[0], font->texCoords[1] );
            GL_DEBUG( glVertex2f )( x, y );

            GL_DEBUG( glTexCoord2f )( font->texCoords[2], font->texCoords[1] );
            GL_DEBUG( glVertex2f )( x + ( *pptmp )->w, y );

            GL_DEBUG( glTexCoord2f )( font->texCoords[2], font->texCoords[3] );
            GL_DEBUG( glVertex2f )( x + ( *pptmp )->w, y + ( *pptmp )->h );

            GL_DEBUG( glTexCoord2f )( font->texCoords[0], font->texCoords[3] );
            GL_DEBUG( glVertex2f )( x, y + ( *pptmp )->h );
        }
        GL_DEBUG_END();
    }

    if ( !sdl_surf_external && NULL != *pptmp )
    {
        // Done with the surface
        SDL_FreeSurface( *pptmp );
        *pptmp = NULL;
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
 * @warning Doesn't pay attention to the width/height arguments yet.
 *
 * @var font    - The font to draw with
 * @var text    - The text to draw
 * @var x       - The x position to start drawing at
 * @var y       - The y position to start drawing at
 * width   - Maximum width of the box (not implemented)
 * @var height  - Maximum height of the box (not implemented)
 * @var spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void fnt_drawTextBox( Font *font, SDL_Surface ** ppSurface, int x, int y, int width, int height, int spacing, const char *format, ... )
{
    va_list args;
    int rv;
    size_t len;
    char *buffer, *line;
    char text[4096] = EMPTY_CSTR;

    bool_t       sdl_surf_external;
    SDL_Surface *textSurf = NULL, **pptmp = NULL;

    if ( NULL != ppSurface )
    {
        sdl_surf_external = btrue;
        pptmp = ppSurface;
    }
    else
    {
        sdl_surf_external = bfalse;
        pptmp = &textSurf;
    }

    va_start( args, format );
    rv = vsnprintf( text, SDL_arraysize( text ), format, args );
    va_end( args );

    // some problem printing the text
    if ( rv < 0 ) goto fnt_drawTextBox_finish;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = EGOBOO_NEW_ARY( char, len + 1 );
    strncpy( buffer, text, len );

    line = strtok( buffer, "\n" );

    while ( NULL != line )
    {
        fnt_drawText_raw( font, x, y, line, pptmp );
        y += spacing;
        line = strtok( NULL, "\n" );
    }

    EGOBOO_DELETE_ARY( buffer );

fnt_drawTextBox_finish:

    if ( !sdl_surf_external && NULL != *pptmp )
    {
        // Done with the surface
        SDL_FreeSurface( *pptmp );
        *pptmp = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void fnt_getTextBoxSize( Font *font, const char *text, int spacing, int *width, int *height )
{
    char *buffer, *line;
    size_t len;
    int tmp_w, tmp_h;
    if ( !font ) return;
    if ( !text || !text[0] ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = EGOBOO_NEW_ARY( char, len + 1 );
    strncpy( buffer, text, len );

    line = strtok( buffer, "\n" );
    *width = *height = 0;

    while ( NULL != line )
    {
        TTF_SizeText( font->ttfFont, line, &tmp_w, &tmp_h );
        *width = ( *width > tmp_w ) ? *width : tmp_w;
        *height += spacing;

        line = strtok( NULL, "\n" );
    }

    EGOBOO_DELETE_ARY( buffer );
}
