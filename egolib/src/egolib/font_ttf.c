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

/// @file egolib/font_ttf.c
/// @brief TTF management
/// @details True-type font drawing functionality.  Uses Freetype 2 & OpenGL
/// to do it's business.

#include "egolib/font_ttf.h"
#include "egolib/log.h"

#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/SDL_GL_extensions.h"

#include "egolib/typedef.h"
#include "egolib/strutil.h"
#include "egolib/platform.h"
#include "egolib/vfs.h"

// this include must be the absolute last include
#include "egolib/mem.h"

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

static int  fnt_print_raw_SDL( TTF_Font *font, SDL_Color fnt_color, const char * szText, SDL_Surface ** ppTmpSurface );
static int  fnt_drawText_raw_SDL( TTF_Font *font, SDL_Color fnt_color, int x, int y, const char *text, SDL_Surface * pDstSurface, SDL_Surface ** ppTmpSurface );
static void fnt_streamText_SDL( TTF_Font * font, SDL_Color fnt_color, int x, int y, int spacing, const char *text );

static int  fnt_print_raw_OGL( Font *font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, const char * szText, SDL_Surface ** ppTmpSurface );
static void fnt_drawText_raw_OGL( Font *font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, int x, int y, const char *text, SDL_Surface ** ppTmpSurface );
static void fnt_streamText_OGL( Font * font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, int x, int y, int spacing, const char *text, SDL_Surface ** ppTmpSurface );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int fnt_atexit_registered = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int fnt_init( void )
{
    /// @author BB
    /// @details Make sure the TTF library was initialized

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
    SDL_RWops *rwops = vfs_openRWopsRead(fileName);
    if (NULL == rwops)
    {
        // SDL_ttf crashes when passed a NULL RWops pointer
        // it's fixed in SDL2_ttf
        return NULL;
    }
    ttfFont = TTF_OpenFontRW(rwops, 1, pointSize);
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
        GL_DEBUG( glDeleteTextures )( 1, &( font->texture ) );
        font->texture = INVALID_GL_ID;

        EGOBOO_DELETE( font );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fnt_getTextSize( Font *font, const char *text, int *width, int *height )
{
    if ( font )
    {
        TTF_SizeText( font->ttfFont, text, width, height );
    }
}

//--------------------------------------------------------------------------------------------
void fnt_getTextBoxSize( Font *font, int spacing, const char *text, int *width, int *height )
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

    *width = *height = 0;
    for ( line = strtok( buffer, "\n" ); NULL != line; line = strtok( NULL, "\n" ) )
    {
        TTF_SizeText( font->ttfFont, line, &tmp_w, &tmp_h );

        *width = ( *width > tmp_w ) ? *width : tmp_w;
        *height += spacing;
    }

    EGOBOO_DELETE_ARY( buffer );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int fnt_print_raw_SDL( TTF_Font *font, SDL_Color fnt_color, const char * szText, SDL_Surface ** ppTmpSurface )
{
    // if there is no external surface, there is no point
    if ( NULL == ppTmpSurface ) return -1;

    // make sure to delete the original surface
    if ( NULL != *ppTmpSurface )
    {
        SDL_FreeSurface( *ppTmpSurface );
        *ppTmpSurface = NULL;
    }

    // create the text
    if ( NULL != font && NULL != szText && 0 != szText[0] )
    {
        *ppTmpSurface = TTF_RenderText_Blended( font, szText, fnt_color );
    }

    return ( NULL == *ppTmpSurface ) ? -1 : 0;
}

//--------------------------------------------------------------------------------------------
int fnt_vprintf_SDL( TTF_Font *font, SDL_Color fnt_color, const char *format, va_list args, SDL_Surface ** ppTmpSurface )
{
    int vsnprintf_rv, print_rv;
    STRING szText = EMPTY_CSTR;

    // evaluate the variable args
    vsnprintf_rv = vsnprintf( szText, SDL_arraysize( szText ) - 1, format, args );
    if ( vsnprintf_rv < 0 ) return -1;

    print_rv = fnt_print_raw_SDL( font, fnt_color, szText, ppTmpSurface );
    if ( print_rv < 0 ) return -1;

    return 0;
}

//--------------------------------------------------------------------------------------------
int fnt_drawText_raw_SDL( TTF_Font *font, SDL_Color fnt_color, int x, int y, const char *text, SDL_Surface * pDstSurface, SDL_Surface ** ppSrcSurface )
{
    int rv;

    bool       sdl_surf_external;
    SDL_Surface * loc_pSrcSurface = NULL;
    SDL_Surface **loc_ppSrcSurface = NULL;

    // handle optional ppTmpSurface
    if ( NULL != ppSrcSurface )
    {
        sdl_surf_external = true;
        loc_ppSrcSurface = ppSrcSurface;
    }
    else
    {
        sdl_surf_external = false;
        loc_ppSrcSurface = &loc_pSrcSurface;
    }

    rv = fnt_print_raw_SDL( font, fnt_color, text, loc_ppSrcSurface );
    if ( rv < 0 ) return -1;

    rv = -1;
    if ( NULL != loc_ppSrcSurface && NULL != *loc_ppSrcSurface )
    {
        SDL_Rect rtmp;

        rtmp.x = x;
        rtmp.y = y;
        rtmp.w = ( *loc_ppSrcSurface )->w;
        rtmp.h = ( *loc_ppSrcSurface )->h;

        SDL_BlitSurface( *loc_ppSrcSurface, NULL, pDstSurface, &rtmp );

        rv = 0;
    }

    // unless something went wrong, there is a surface for us to delete
    if ( !sdl_surf_external && NULL != *loc_ppSrcSurface )
    {
        // Done with the surface
        SDL_FreeSurface( *loc_ppSrcSurface );
        *loc_ppSrcSurface = NULL;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
void fnt_drawText_SDL( TTF_Font * font, SDL_Color fnt_color, int x, int y, SDL_Surface * pDstSurface, SDL_Surface ** ppTmpSurface, const char *format, ... )
{
    va_list args;
    int rv;

    bool       sdl_surf_external;
    SDL_Surface *loc_pSurface = NULL;
    SDL_Surface **loc_ppTmpSurface = NULL;

    if ( NULL == pDstSurface ) return;

    // handle optional ppTmpSurface
    if ( NULL != ppTmpSurface )
    {
        sdl_surf_external = true;
        loc_ppTmpSurface = ppTmpSurface;
    }
    else
    {
        sdl_surf_external = false;
        loc_ppTmpSurface = &loc_pSurface;
    }

    va_start( args, format );
    rv = fnt_vprintf_SDL( font, fnt_color, format, args, loc_ppTmpSurface );
    va_end( args );

    if (( rv <= 0 ) )
    {
        if (( NULL != loc_ppTmpSurface ) && ( NULL != *loc_ppTmpSurface ) )
        {
            SDL_Rect rtmp;

            rtmp.x = x;
            rtmp.y = y;
            rtmp.w = ( *loc_ppTmpSurface )->w;
            rtmp.h = ( *loc_ppTmpSurface )->h;

            SDL_BlitSurface( *loc_ppTmpSurface, NULL, pDstSurface, &rtmp );
        }
    }

    // unless something went wrong, there is a surface for us to delete
    if ( !sdl_surf_external )
    {
        if ( NULL != loc_ppTmpSurface && NULL != *loc_ppTmpSurface )
        {
            // Done with the surface
            SDL_FreeSurface( *loc_ppTmpSurface );
            *loc_ppTmpSurface = NULL;
        }
    }
};

//--------------------------------------------------------------------------------------------
/** font_drawTextBox
 * Draws a text string into a box, splitting it into lines according to newlines in the string.
 * @warning Doesn't pay attention to the width/height arguments yet.
 *
 * @var font    - The font to draw with
 * @var text    - The text to draw
 * @var x       - The x position to start drawing at
 * @var y       - The y position to start drawing at
 * @var width   - Maximum width of the box (not implemented)
 * @var height  - Maximum height of the box (not implemented)
 * @var spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void fnt_drawTextBox_SDL( TTF_Font *font, SDL_Color fnt_color, int x, int y, int width, int height, int spacing, const char *format, ... )
{
    va_list args;
    int rv;
    char text[4096] = EMPTY_CSTR;

    va_start( args, format );
    rv = vsnprintf( text, SDL_arraysize( text ), format, args );
    va_end( args );

    // some problem printing the text
    if ( rv >= 0 )
    {
        fnt_streamText_SDL( font, fnt_color, x, y, spacing, text );
    }
}

//--------------------------------------------------------------------------------------------
void fnt_streamText_SDL( TTF_Font * font, SDL_Color fnt_color, int x, int y, int spacing, const char *text )
{
    size_t len;
    char *buffer, *line;

    // a temporary surface that font is going to be drawn into
    SDL_Surface * loc_pDstSurface = NULL;
    SDL_Surface **loc_ppDstSurface = &loc_pDstSurface;

    if ( NULL == font ) return;

    // If text is empty, there's nothing to draw
    if ( NULL == text || '\0' == text[0] ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = EGOBOO_NEW_ARY( char, len + 1 );
    strncpy( buffer, text, len );

    for ( line = strtok( buffer, "\n" ); NULL != line; line = strtok( NULL, "\n" ) )
    {
        fnt_drawText_raw_SDL( font, fnt_color, x, y, line, *loc_ppDstSurface, NULL );
        y += spacing;
    }

    EGOBOO_DELETE_ARY( buffer );

    // unless something went wrong, there is a surface for us to delete
    if ( NULL != loc_ppDstSurface && NULL != *loc_ppDstSurface )
    {
        // Done with the surface
        SDL_FreeSurface( *loc_ppDstSurface );
        *loc_ppDstSurface = NULL;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int fnt_print_raw_OGL( Font *font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, const char * szText, SDL_Surface ** ppTmpSurface )
{
    int rv, print_rv, upload_rv;

    bool  gl_id_external;
    GLfloat loc_tex_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    bool        sdl_surf_external;
    SDL_Surface  *loc_pSurface      = NULL;
    SDL_Surface **loc_ppTmpSurface  = NULL;

    // handle the tex_coords
    if ( NULL == tex_coords )
    {
        tex_coords = loc_tex_coords;
    }

    // handle optional tex_id
    gl_id_external = true;
    if ( INVALID_GL_ID == tex_id )
    {
        gl_id_external = false;
        glGenTextures( 1, &tex_id );
    }

    // handle optional ppTmpSurface
    if ( NULL != ppTmpSurface )
    {
        sdl_surf_external = true;
        loc_ppTmpSurface = ppTmpSurface;
    }
    else
    {
        sdl_surf_external = false;
        loc_ppTmpSurface = &loc_pSurface;
    }

    // assume the best
    rv = 0;

    // does the font exist?
    if ( NULL == font )
    {
        rv = -1;
        goto fnt_print_raw_OGL_exit;
    }

    // did we create the SDL surface?
    print_rv = fnt_print_raw_SDL( font->ttfFont, fnt_color, szText, loc_ppTmpSurface );
    if ( print_rv < 0 )
    {
        rv = -1;
        goto fnt_print_raw_OGL_exit;
    }

    // upload the surface
    if ( NULL != loc_ppTmpSurface && NULL != *loc_ppTmpSurface )
    {
        upload_rv = SDL_GL_uploadSurface( *loc_ppTmpSurface, tex_id, tex_coords );

        // did we succeed?
        rv = upload_rv ? 0 : -1;
    }

fnt_print_raw_OGL_exit:

    if ( !gl_id_external && INVALID_GL_ID != tex_id )
    {
        glDeleteTextures( 1, &tex_id );
        tex_id = INVALID_GL_ID;
    }

    // if the surface is not external, there is a surface for us to delete (unless something went wrong)
    if ( !sdl_surf_external && NULL != *loc_ppTmpSurface )
    {
        // Done with the surface
        SDL_FreeSurface( *loc_ppTmpSurface );
        *loc_ppTmpSurface = NULL;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
int fnt_vprintf_OGL( Font *font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, const char *format, va_list args, SDL_Surface ** ppTmpSurface )
{
    int rv, vsnprintf_rv;
    STRING szText = EMPTY_CSTR;

    bool  gl_id_external;
    GLfloat loc_tex_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // handle the tex_coords
    if ( NULL == tex_coords )
    {
        tex_coords = loc_tex_coords;
    }

    // handle optional tex_id
    gl_id_external = true;
    if ( INVALID_GL_ID == tex_id )
    {
        gl_id_external = false;
        glGenTextures( 1, &tex_id );
    }

    // evaluate the variable args
    vsnprintf_rv = vsnprintf( szText, SDL_arraysize( szText ) - 1, format, args );
    if ( vsnprintf_rv < 0 ) return -1;

    rv = fnt_print_raw_OGL( font, fnt_color, tex_id, tex_coords, szText, ppTmpSurface );
#if 0
fnt_vprintf_OGL_exit:
#endif
    if ( !gl_id_external && INVALID_GL_ID != tex_id )
    {
        glDeleteTextures( 1, &tex_id );
        tex_id = INVALID_GL_ID;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
void fnt_drawText_raw_OGL( Font *font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, int x, int y, const char *text, SDL_Surface ** ppTmpSurface )
{
    int rv;

    bool  gl_id_external;
    GLfloat loc_tex_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    bool sdl_surf_external;
    SDL_Surface * loc_pTmpSurface = NULL;
    SDL_Surface  **loc_ppTmpSurface = NULL;

    // handle the tex_coords
    if ( NULL == tex_coords )
    {
        tex_coords = loc_tex_coords;
    }

    // handle optional tex_id
    gl_id_external = true;
    if ( INVALID_GL_ID == tex_id )
    {
        gl_id_external = false;
        glGenTextures( 1, &tex_id );
    }

    // handle optional ppTmpSurface
    if ( NULL != ppTmpSurface )
    {
        sdl_surf_external = true;
        loc_ppTmpSurface = ppTmpSurface;
    }
    else
    {
        sdl_surf_external = false;
        loc_ppTmpSurface = &loc_pTmpSurface;
    }

    // alocate a texture, if necessary
    gl_id_external = true;
    if ( INVALID_GL_ID == tex_id )
    {
        gl_id_external = false;
        glGenTextures( 1, &tex_id );
    }

    rv = fnt_print_raw_OGL( font, fnt_color, tex_id, tex_coords, text, loc_ppTmpSurface );
    if ( rv < 0 ) goto fnt_drawText_raw_finish;

    // And draw the darn thing
    GL_DEBUG( glBegin )( GL_QUADS );
    {
        GL_DEBUG( glTexCoord2f )( tex_coords[0], tex_coords[1] );
        GL_DEBUG( glVertex2f )( x, y );

        GL_DEBUG( glTexCoord2f )( tex_coords[2], tex_coords[1] );
        GL_DEBUG( glVertex2f )( x + ( *loc_ppTmpSurface )->w, y );

        GL_DEBUG( glTexCoord2f )( tex_coords[2], tex_coords[3] );
        GL_DEBUG( glVertex2f )( x + ( *loc_ppTmpSurface )->w, y + ( *loc_ppTmpSurface )->h );

        GL_DEBUG( glTexCoord2f )( tex_coords[0], tex_coords[3] );
        GL_DEBUG( glVertex2f )( x, y + ( *loc_ppTmpSurface )->h );
    }
    GL_DEBUG_END();

fnt_drawText_raw_finish:

    if ( !gl_id_external && INVALID_GL_ID != tex_id )
    {
        glDeleteTextures( 1, &tex_id );
        tex_id = INVALID_GL_ID;
    }

    if ( !sdl_surf_external && NULL != *loc_ppTmpSurface )
    {
        // Done with the surface
        SDL_FreeSurface( *loc_ppTmpSurface );
        *loc_ppTmpSurface = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void fnt_drawText_OGL_va( Font *font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, int x, int y, SDL_Surface ** ppTmpSurface, const char *format, va_list args )
{
    int rv;

    bool  gl_id_external;
    GLfloat loc_tex_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    bool sdl_surf_external;
    SDL_Surface *loc_pTmpSurface = NULL;
    SDL_Surface **loc_ppTmpSurface = NULL;

    // handle the tex_coords
    if ( NULL == tex_coords )
    {
        tex_coords = loc_tex_coords;
    }

    // handle optional tex_id
    gl_id_external = true;
    if ( INVALID_GL_ID == tex_id )
    {
        gl_id_external = false;
        glGenTextures( 1, &tex_id );
    }

    // handle optional ppTmpSurface
    if ( NULL != ppTmpSurface )
    {
        sdl_surf_external = true;
        loc_ppTmpSurface = ppTmpSurface;
    }
    else
    {
        sdl_surf_external = false;
        loc_ppTmpSurface = &loc_pTmpSurface;
    }

    rv = fnt_vprintf_OGL( font, fnt_color, tex_id, tex_coords, format, args, loc_ppTmpSurface );
    
    EGOBOO_ASSERT(loc_ppTmpSurface != NULL && *loc_ppTmpSurface != NULL);

    if ( rv <= 0 )
    {
        // And draw the darn thing
        GL_DEBUG( glBegin )( GL_QUADS );
        {
            GL_DEBUG( glTexCoord2f )( tex_coords[0], tex_coords[1] );
            GL_DEBUG( glVertex2f )( x, y );

            GL_DEBUG( glTexCoord2f )( tex_coords[2], tex_coords[1] );
            GL_DEBUG( glVertex2f )( x + ( *loc_ppTmpSurface )->w, y );

            GL_DEBUG( glTexCoord2f )( tex_coords[2], tex_coords[3] );
            GL_DEBUG( glVertex2f )( x + ( *loc_ppTmpSurface )->w, y + ( *loc_ppTmpSurface )->h );

            GL_DEBUG( glTexCoord2f )( tex_coords[0], tex_coords[3] );
            GL_DEBUG( glVertex2f )( x, y + ( *loc_ppTmpSurface )->h );
        }
        GL_DEBUG_END();
    }
#if 0
fnt_drawText_OGL_va_exit:
#endif
    if ( !gl_id_external && INVALID_GL_ID != tex_id )
    {
        glDeleteTextures( 1, &tex_id );
        tex_id = INVALID_GL_ID;
    }

    if ( !sdl_surf_external && NULL != *loc_ppTmpSurface )
    {
        // Done with the surface
        SDL_FreeSurface( *loc_ppTmpSurface );
        *loc_ppTmpSurface = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void fnt_drawText_OGL_immediate( Font *font, SDL_Color fnt_color, int x, int y, const char *format, ... )
{
    va_list args;

    va_start( args, format );
    fnt_drawText_OGL_va( font, fnt_color, INVALID_GL_ID, NULL, x, y, NULL, format, args );
    va_end( args );
}

//--------------------------------------------------------------------------------------------
void fnt_drawText_OGL( Font *font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, int x, int y, SDL_Surface ** ppTmpSurface, const char *format, ... )
{
    va_list args;

    va_start( args, format );
    fnt_drawText_OGL_va( font, fnt_color, tex_id, tex_coords, x, y, ppTmpSurface, format, args );
    va_end( args );
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
 * @var width   - Maximum width of the box (not implemented)
 * @var height  - Maximum height of the box (not implemented)
 * @var spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void fnt_drawTextBox_OGL( Font *font, SDL_Color fnt_color, int x, int y, int width, int height, int spacing, SDL_Surface ** ppTmpSurface, const char *format, ... )
{
    va_list args;
    int rv;
    char text[4096] = EMPTY_CSTR;

    GLuint  tex_id = INVALID_GL_ID;
    GLfloat tex_coords[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    va_start( args, format );
    rv = vsnprintf( text, SDL_arraysize( text ), format, args );
    va_end( args );

    // some problem printing the text
    if ( rv >= 0 )
    {
        fnt_streamText_OGL( font, fnt_color, tex_id, tex_coords, x, y, spacing, text, ppTmpSurface );
    }
}

//--------------------------------------------------------------------------------------------
void fnt_streamText_OGL( Font * font, SDL_Color fnt_color, GLuint tex_id, GLfloat * tex_coords, int x, int y, int spacing, const char *text, SDL_Surface ** ppTmpSurface )
{
    size_t len;
    char *line;

    bool gl_id_external;
    GLfloat loc_tex_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLfloat tmp_tex_coords[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    bool sdl_surf_external;
    SDL_Surface * loc_pTmpSurface = NULL;
    SDL_Surface **loc_ppTmpSurface = NULL;

    if ( NULL == font ) return;

    // If text is empty, there's nothing to draw
    if ( NULL == text || '\0' == text[0] ) return;

    // handle the tex_coords
    if ( NULL == tex_coords )
    {
        tex_coords = loc_tex_coords;
    }

    // handle optional tex_id
    gl_id_external = true;
    if ( INVALID_GL_ID == tex_id )
    {
        gl_id_external = false;
        glGenTextures( 1, &tex_id );
    }

    // handle optional ppTmpSurface
    if ( NULL != ppTmpSurface )
    {
        sdl_surf_external = true;
        loc_ppTmpSurface = ppTmpSurface;
    }
    else
    {
        sdl_surf_external = false;
        loc_ppTmpSurface = &loc_pTmpSurface;
    }

    // Split the passed in text into separate lines
    len = strlen( text );
	std::unique_ptr<char[]> buffer = std::unique_ptr<char[]>(new char[len + 1]);
    strncpy(buffer.get(), text, len);

    //Always null terminate strings
    buffer[len] = '\0';

    for (line = strtok(buffer.get(), "\n"); nullptr != line; line = strtok(nullptr, "\n"))
    {
        fnt_drawText_raw_OGL( font, fnt_color, tex_id, tmp_tex_coords, x, y, line, loc_ppTmpSurface );

        if ( NULL != tex_coords )
        {
            if ( tex_coords[0] > tmp_tex_coords[0] ) tex_coords[0] = tmp_tex_coords[0];
            if ( tex_coords[1] > tmp_tex_coords[1] ) tex_coords[1] = tmp_tex_coords[1];
            if ( tex_coords[2] < tmp_tex_coords[2] ) tex_coords[2] = tmp_tex_coords[2];
            if ( tex_coords[3] < tmp_tex_coords[3] ) tex_coords[3] = tmp_tex_coords[3];
        }

        y += spacing;
    }

    if ( !gl_id_external && INVALID_GL_ID != tex_id )
    {
        glDeleteTextures( 1, &tex_id );
        tex_id = INVALID_GL_ID;
    }

    if ( !sdl_surf_external && NULL != *loc_ppTmpSurface )
    {
        // Done with the surface
        SDL_FreeSurface( *loc_ppTmpSurface );
        *loc_ppTmpSurface = NULL;
    }
}
