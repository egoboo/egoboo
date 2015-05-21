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

/// @file  game/renderer_2d.c
/// @brief Implementation of the 2d renderer functions
/// @details

#include "game/egoboo.h"
#include "game/renderer_2d.h"

//--------------------------------------------------------------------------------------------
// EXTERNAL VARIABLES
//--------------------------------------------------------------------------------------------

StaticArray<msg_t, EGO_MESSAGE_MAX> DisplayMsg;

int DisplayMsg_timechange = 0;
int DisplayMsg_count = EGO_MESSAGE_MAX;
bool DisplayMsg_on = true;

//--------------------------------------------------------------------------------------------
// PRIVATE FUNCTIONS
//--------------------------------------------------------------------------------------------

static int  _va_draw_string( float x, float y, const char *format, va_list args );

//--------------------------------------------------------------------------------------------
// MODULE "PRIVATE" FUNCTIONS
//--------------------------------------------------------------------------------------------
int _va_draw_string( float x, float y, const char *format, va_list args )
{
    int cnt = 1;
    int x_stt;
    STRING szText;
    Uint8 cTmp;

    oglx_texture_t * tx_ptr = TextureManager::get().get_valid_ptr(( TX_REF )TX_FONT_BMP );
    if ( NULL == tx_ptr ) return y;

    if ( vsnprintf( szText, SDL_arraysize( szText ) - 1, format, args ) <= 0 )
    {
        return y;
    }

    gfx_begin_text();
    {
        x_stt = x;
        cnt = 0;
        cTmp = szText[cnt];
        while ( CSTR_END != cTmp )
        {
            Uint8 iTmp;

            // Convert ASCII to our own little font
            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = ( std::floor(( float )x / ( float )TABADD ) + 1.0f ) * TABADD;
            }
            else if ( C_LINEFEED_CHAR == cTmp )
            {
                x  = x_stt;
                y += fontyspacing;
            }
            else if ( isspace( cTmp ) )
            {
                // other whitespace
                iTmp = asciitofont[cTmp];
                x += fontxspacing[iTmp] / 2;
            }
            else
            {
                // Normal letter
                iTmp = asciitofont[cTmp];
                draw_one_font( tx_ptr, iTmp, x, y );
                x += fontxspacing[iTmp];
            }

            cnt++;
            cTmp = szText[cnt];
        }
    }
    gfx_end_text();

    y += fontyspacing;

    return y;
}

//--------------------------------------------------------------------------------------------
int draw_string_raw( float x, float y, const char *format, ... )
{
    /// @author BB
    /// @details the same as draw string, but it does not use the gfx_begin_2d() ... gfx_end_2d()
    ///    bookends.

    va_list args;

    va_start( args, format );
    y = _va_draw_string( x, y, format, args );
    va_end( args );

    return y;
}

//--------------------------------------------------------------------------------------------
// DisplayMsg IMPLEMENTATION
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
void DisplayMsg_clear()
{
    /// @author ZZ
    /// @details This function empties the message buffer
    int cnt;

    for ( cnt = 0; cnt < EGO_MESSAGE_MAX; cnt++ )
    {
        DisplayMsg.ary[cnt].time = 0;
    }
}

//--------------------------------------------------------------------------------------------
void DisplayMsg_reset()
{
    /// @author ZZ
    /// @details This makes messages safe to use

    int cnt;

    DisplayMsg_timechange = 0;
    DisplayMsg.count = 0;

    for ( cnt = 0; cnt < EGO_MESSAGE_MAX; cnt++ )
    {
        DisplayMsg.ary[cnt].time = 0;
    }
}

//--------------------------------------------------------------------------------------------
int DisplayMsg_get_free()
{
    /// @author ZZ
    /// @details This function finds the best message to use
    /// Pick the first one

    int tnc = DisplayMsg.count;

    DisplayMsg.count++;
    DisplayMsg.count %= DisplayMsg_count;

    return tnc;
}

//--------------------------------------------------------------------------------------------
int DisplayMsg_printf( const char *format, ... )
{
    va_list args;
    int retval;

    va_start( args, format );
    retval = DisplayMsg_vprintf( format, args );
    va_end( args );

    return retval;
}

//--------------------------------------------------------------------------------------------
void DisplayMsg_print( const char *text )
{
    /// @author ZZ
    /// @details This function sticks a message in the display queue and sets its timer

    int          slot;
    const char * src;
    char       * dst, * dst_end;
    msg_t      * pmsg;

    if ( INVALID_CSTR( text ) ) return;

    // Get a "free" message
    slot = DisplayMsg_get_free();
    pmsg = DisplayMsg.get_ptr(slot);

    // Copy the message
    for ( src = text, dst = pmsg->textdisplay, dst_end = dst + EGO_MESSAGE_SIZE;
          CSTR_END != *src && dst < dst_end;
          src++, dst++ )
    {
        *dst = *src;
    }
    if ( dst < dst_end ) *dst = CSTR_END;

    // Set the time
    pmsg->time = egoboo_config_t::get().hud_messageDuration.getValue();
}

//--------------------------------------------------------------------------------------------
int DisplayMsg_vprintf( const char *format, va_list args )
{
    int retval = 0;

    if ( VALID_CSTR( format ) )
    {
        STRING szTmp;

        retval = vsnprintf( szTmp, SDL_arraysize( szTmp ), format, args );
        DisplayMsg_print( szTmp );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
float DisplayMsg_draw_all( float y )
{
    int cnt, tnc;

    // Messages
    if ( DisplayMsg_on )
    {
        // Display the messages
        tnc = DisplayMsg.count;
        for ( cnt = 0; cnt < DisplayMsg_count; cnt++ )
        {
            if ( DisplayMsg.ary[tnc].time > 0 )
            {
                y = draw_wrap_string( DisplayMsg.ary[tnc].textdisplay, 0, y, sdl_scr.x - WRAP_TOLERANCE );
                if ( DisplayMsg.ary[tnc].time > DisplayMsg_timechange )
                {
                    DisplayMsg.ary[tnc].time -= DisplayMsg_timechange;
                }
                else
                {
                    DisplayMsg.ary[tnc].time = 0;
                }
            }

            tnc = ( tnc + 1 ) % DisplayMsg_count;
        }

        DisplayMsg_timechange = 0;
    }

    return y;
}

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------
void gfx_begin_2d()
{

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_SCISSOR_BIT );

    // Reset the Projection Matrix
    // Set up an orthogonal projection
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
	fmat_4x4_t projection = fmat_4x4_t::ortho(0.0, sdl_scr.x, sdl_scr.y, 0.0, -1.0f, +1.0f);
	Ego::Renderer::get().loadMatrix(projection);

    // Reset the Modelview Matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().loadMatrix(fmat_4x4_t::identity());

    // remove any scissor test
	Ego::Renderer::get().setScissorTestEnabled(false);

    // don't worry about hidden surfaces
	Ego::Renderer::get().setDepthTestEnabled(false);

    // stop culling backward facing polygons
    oglx_end_culling();                            // GL_ENABLE_BIT
}

//--------------------------------------------------------------------------------------------
void gfx_end_2d()
{
    // get the old modelview matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // get the old projection matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();

    // ATTRIB_POP()
    // - restores the culling mode
    // - restores the culling depth-testing mode
    // - restores the SCISSOR mode
    ATTRIB_POP( __FUNCTION__ );

    // leave the MatrixMode in GL_MODELVIEW
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
}

//--------------------------------------------------------------------------------------------
void gfx_begin_text()
{
    // do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG( glPushAttrib )( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );

    // do not display the completely transparent portion
    Ego::Renderer::get().setAlphaTestEnabled(true);
    GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                         // GL_COLOR_BUFFER_BIT

    Ego::Renderer::get().setBlendingEnabled(true);
    GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );     // GL_COLOR_BUFFER_BIT

    // don't worry about hidden surfaces
	Ego::Renderer::get().setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
    oglx_end_culling();                                                  // GL_ENABLE_BIT

	Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());// GL_CURRENT_BIT
}

//--------------------------------------------------------------------------------------------
void gfx_end_text()
{
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG( glPopAttrib )();
}

//--------------------------------------------------------------------------------------------
void gfx_reshape_viewport(int w, int h)
{
    Ego::Renderer::get().setViewportRectangle(0, 0, w, h);
}

//--------------------------------------------------------------------------------------------
// PRIMITIVES
//--------------------------------------------------------------------------------------------
void draw_quad_2d(oglx_texture_t *tex, const ego_frect_t scr_rect, const ego_frect_t tx_rect, const bool use_alpha, const Ego::Colour4f& tint)
{
    ATTRIB_PUSH( __FUNCTION__, GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT )
    {
        oglx_texture_t::bind(tex);

		Ego::Renderer::get().setColour(tint);

        if ( use_alpha )
        {
            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // GL_COLOR_BUFFER_BIT

            Ego::Renderer::get().setAlphaTestEnabled(true);
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                      // GL_COLOR_BUFFER_BIT
        }
        else
        {
            Ego::Renderer::get().setBlendingEnabled(false);
            Ego::Renderer::get().setAlphaTestEnabled(false);
        }

        GL_DEBUG( glBegin )( GL_QUADS );
        {
			GL_DEBUG(glTexCoord2f)(tx_rect.xmin, tx_rect.ymax); GL_DEBUG(glVertex2f)(scr_rect.xmin, scr_rect.ymax);
			GL_DEBUG(glTexCoord2f)(tx_rect.xmax, tx_rect.ymax); GL_DEBUG(glVertex2f)(scr_rect.xmax, scr_rect.ymax);
			GL_DEBUG(glTexCoord2f)(tx_rect.xmax, tx_rect.ymin); GL_DEBUG(glVertex2f)(scr_rect.xmax, scr_rect.ymin);
			GL_DEBUG(glTexCoord2f)(tx_rect.xmin, tx_rect.ymin); GL_DEBUG(glVertex2f)(scr_rect.xmin, scr_rect.ymin);
        }
        GL_DEBUG_END();
    }
    ATTRIB_POP( __FUNCTION__ );
}

//--------------------------------------------------------------------------------------------
// BITMAP FONT FUNCTIONS
//--------------------------------------------------------------------------------------------
void draw_one_font( oglx_texture_t * ptex, int fonttype, float x_stt, float y_stt )
{
    /// @author GAC
    /// @details Very nasty version for starters.  Lots of room for improvement.
    /// @author ZZ
    /// @details This function draws a letter or number

    GLfloat dx, dy, border;

    ego_frect_t tx_rect, sc_rect;

    sc_rect.xmin  = x_stt;
    sc_rect.xmax  = x_stt + fontrect[fonttype].w;
    sc_rect.ymin  = y_stt + fontoffset - fontrect[fonttype].h;
    sc_rect.ymax  = y_stt + fontoffset;

    dx = 2.0f / 512.0f;
    dy = 1.0f / 256.0f;
    border = 1.0f / 512.0f;

    tx_rect.xmin = fontrect[fonttype].x * dx;
    tx_rect.xmax = tx_rect.xmin + fontrect[fonttype].w * dx;
    tx_rect.ymin = fontrect[fonttype].y * dy;
    tx_rect.ymax = tx_rect.ymin + fontrect[fonttype].h * dy;

    // shrink the texture size slightly
    tx_rect.xmin += border;
    tx_rect.xmax -= border;
    tx_rect.ymin += border;
    tx_rect.ymax -= border;

    draw_quad_2d(ptex, sc_rect, tx_rect, true, Ego::Colour4f::white());
}

//--------------------------------------------------------------------------------------------
float draw_string( float x, float y, const char *format, ... )
{
    va_list args;

    gfx_begin_2d();
    {
        va_start( args, format );
        y = _va_draw_string( x, y, format, args );
        va_end( args );
    }
    gfx_end_2d();

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_wrap_string( const char *szText, float x, float y, int maxx )
{
    /// @author ZZ
    /// @details This function spits a line of null terminated text onto the backbuffer,
    ///    wrapping over the right side and returning the new y value

    int stt_x = x;
    Uint8 cTmp = szText[0];
    int newy = y + fontyspacing;
    Uint8 newword = true;
    int cnt = 1;

    oglx_texture_t * tx_ptr = TextureManager::get().get_valid_ptr(( TX_REF )TX_FONT_BMP );
    if ( NULL == tx_ptr ) return y;

    gfx_begin_text();

    maxx = maxx + stt_x;

    while ( CSTR_END != cTmp )
    {
        // Check each new word for wrapping
        if ( newword )
        {
            int endx = x + font_bmp_length_of_word( szText + cnt - 1 );

            newword = false;
            if ( endx > maxx )
            {
                // Wrap the end and cut off spaces and tabs
                x = stt_x + fontyspacing;
                y += fontyspacing;
                newy += fontyspacing;

                while ( ' ' == cTmp || '~' == cTmp )
                {
                    cTmp = szText[cnt];
                    cnt++;
                }
            }
        }
        else
        {
            Uint8 iTmp;

            if ( '~' == cTmp )
            {
                // Use squiggle for tab
                x = ( std::floor(( float )x / ( float )TABADD ) + 1.0f ) * TABADD;
            }
            else if ( C_LINEFEED_CHAR == cTmp )
            {
                x = stt_x;
                y += fontyspacing;
                newy += fontyspacing;
            }
            else if ( isspace( cTmp ) )
            {
                // other whitespace
                iTmp = asciitofont[cTmp];
                x += fontxspacing[iTmp] / 2;
            }
            else
            {
                // Normal letter
                iTmp = asciitofont[cTmp];
                draw_one_font( tx_ptr, iTmp, x, y );
                x += fontxspacing[iTmp];
            }

            cTmp = szText[cnt];
            cnt++;

            if ( '~' == cTmp || C_LINEFEED_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp || isspace( cTmp ) )
            {
                newword = true;
            }
        }
    }

    gfx_end_text();
    return newy;
}

//--------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
//--------------------------------------------------------------------------------------------
bool dump_screenshot()
{
    /// @author BB
    /// @details dumps the current screen (GL context) to a new bitmap file
    /// right now it dumps it to whatever the current directory is

    // returns true if successful, false otherwise

    int i;
    bool savefound = false;
    bool saved     = false;
    STRING szFilename, szResolvedFilename;

    // find a valid file name
    savefound = false;
    i = 0;
    while ( !savefound && ( i < 100 ) )
    {
        snprintf( szFilename, SDL_arraysize( szFilename ), "ego%02d.bmp", i );

        // lame way of checking if the file already exists...
        savefound = !vfs_exists( szFilename );
        if ( !savefound )
        {
            i++;
        }
    }

    if ( !savefound ) return false;

    // convert the file path to the correct write path
    strncpy( szResolvedFilename, szFilename, SDL_arraysize( szFilename ) );

    // if we are not using OpenGL, use SDL to dump the screen
    if ( HAS_NO_BITS( sdl_scr.pscreen->flags, SDL_OPENGL ) )
    {
        SDL_SaveBMP_RW( sdl_scr.pscreen, vfs_openRWopsWrite(szResolvedFilename), 1 );
        return false;
    }

    // we ARE using OpenGL
    GL_DEBUG( glPushClientAttrib )( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        SDL_Surface *temp;

        // create a SDL surface
        const auto& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8>();
        temp = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_scr.x, sdl_scr.y,
                                    pixelFormatDescriptor.getBitsPerPixel(),
                                    pixelFormatDescriptor.getRedMask(),
                                    pixelFormatDescriptor.getGreenMask(),
                                    pixelFormatDescriptor.getBlueMask(),
                                    pixelFormatDescriptor.getAlphaMask());

        if ( NULL == temp )
        {
            //Something went wrong
            SDL_FreeSurface( temp );
            return false;
        }

        //Now lock the surface so that we can read it
        if ( -1 != SDL_LockSurface( temp ) )
        {
            SDL_Rect rect;

            memcpy( &rect, &( sdl_scr.pscreen->clip_rect ), sizeof( SDL_Rect ) );
            if ( 0 == rect.w && 0 == rect.h )
            {
                rect.w = sdl_scr.x;
                rect.h = sdl_scr.y;
            }
            if ( rect.w > 0 && rect.h > 0 )
            {
                int y;
                Uint8 * pixels;

                GL_DEBUG( glGetError )();

                //// use the allocated screen to tell OpenGL about the row length (including the lapse) in pixels
                //// stolen from SDL ;)
                // GL_DEBUG(glPixelStorei)(GL_UNPACK_ROW_LENGTH, temp->pitch / temp->format->BytesPerPixel );
                // EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                //// since we have specified the row actual length and will give a pointer to the actual pixel buffer,
                //// it is not necesssaty to mess with the alignment
                // GL_DEBUG(glPixelStorei)(GL_UNPACK_ALIGNMENT, 1 );
                // EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG(glGetError)() );

                // ARGH! Must copy the pixels row-by-row, since the OpenGL video memory is flipped vertically
                // relative to the SDL Screen memory

                // this is supposed to be a DirectX thing, so it needs to be tested out on glx
                // there should probably be [SCREENSHOT_INVERT] and [SCREENSHOT_VALID] keys in setup.txt
                pixels = ( Uint8 * )temp->pixels;
                for ( y = rect.y; y < rect.y + rect.h; y++ )
                {
                    GL_DEBUG( glReadPixels )( rect.x, ( rect.h - y ) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels );
                    pixels += temp->pitch;
                }
                EGOBOO_ASSERT( GL_NO_ERROR == GL_DEBUG( glGetError )() );
            }

            SDL_UnlockSurface( temp );

            // Save the file as a .bmp
            saved = ( -1 != SDL_SaveBMP_RW( temp, vfs_openRWopsWrite(szResolvedFilename), 1 ) );
        }

        // free the SDL surface
        SDL_FreeSurface( temp );
        if ( saved )
        {
            // tell the user what we did
            DisplayMsg_printf( "Saved to %s", szFilename );
        }
    }
    GL_DEBUG( glPopClientAttrib )();

    return savefound && saved;
}


