// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

#include "egoboo_console.inl"

#include "egoboo_config.h"
#include "egoboo_math.h"

#include "ogl_debug.h"
#include "SDL_extensions.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_console_t * egoboo_console_top = NULL;

Uint8  scancode_to_ascii[SDLK_LAST];
Uint8  scancode_to_ascii_shift[SDLK_LAST];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void egoboo_console_add_output( egoboo_console_t * pcon, char * szNew );
static void egoboo_console_write( egoboo_console_t * pcon, const char *format, va_list args );

static SDL_bool egoboo_console_draw( egoboo_console_t * pcon );
static SDL_bool egoboo_console_run( egoboo_console_t * pcon );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static SDL_bool egoboo_console_stack_unlink( egoboo_console_t * pcon )
{
    SDL_bool retval = SDL_FALSE;

    if (NULL == pcon) return retval;

    if ( pcon == egoboo_console_top )
    {
        egoboo_console_top = pcon->pnext;
        retval = SDL_TRUE;
    }
    else
    {
        egoboo_console_t * ptmp;

        // find the console that points to this one
        ptmp = egoboo_console_top;
        while ( NULL != ptmp && NULL != ptmp->pnext )
        {
            if ( ptmp->pnext == pcon )
            {
                retval = SDL_TRUE;
                ptmp->pnext = pcon->pnext;
                break;
            }
            ptmp = ptmp->pnext;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
static SDL_bool egoboo_console_stack_push_front( egoboo_console_t * pcon )
{
    if ( NULL == pcon ) return SDL_FALSE;

    pcon->pnext = egoboo_console_top;
    egoboo_console_top = pcon;

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void egoboo_console_write( egoboo_console_t * pcon, const char *format, va_list args )
{
    char buffer[EGOBOO_CONSOLE_WRITE_LEN];

    if ( NULL != pcon )
    {
        vsnprintf( buffer, EGOBOO_CONSOLE_WRITE_LEN - 1, format, args );

        egoboo_console_add_output( pcon, buffer );
    }
}

//--------------------------------------------------------------------------------------------
void egoboo_console_fprint( egoboo_console_t * pcon, const char *format, ... )
{
    va_list args;

    va_start( args, format );
    egoboo_console_write( pcon, format, args );
    va_end( args );
}

//--------------------------------------------------------------------------------------------
void egoboo_console_add_output( egoboo_console_t * pcon, char * szNew )
{
    size_t out_len, copy_len;
    char * src, * dst;

    if ( NULL == pcon ) return;

    // how many characters are we adding?
    out_len = strlen(szNew);

    // initialize the pointers for the copy operation
    src      = szNew;
    dst      = pcon->output_buffer + pcon->output_carat;
    copy_len = out_len;

    // check to make sure that the ranges are valid
    if ( out_len > EGOBOO_CONSOLE_OUTPUT )
    {
        // we need to replace the entire output buffer with
        // a portion of szNew

        size_t offset = out_len - EGOBOO_CONSOLE_OUTPUT - 1;

        // update the copy parameters
        src      = szNew + offset;
        copy_len = out_len - offset;
    }
    else if ( pcon->output_carat + out_len > EGOBOO_CONSOLE_OUTPUT )
    {
        // the length of the buffer after adding szNew would be too large
        // get rid of some of the input buffer and then add szNew

        size_t offset = ( pcon->output_carat + out_len ) - EGOBOO_CONSOLE_OUTPUT - 1;

        // move the memory so that we create some space
        memmove( pcon->output_buffer, pcon->output_buffer + offset, pcon->output_carat - offset );

        // update the copy parameters
        pcon->output_carat -= offset;
        dst = pcon->output_buffer - pcon->output_carat;
    }

    pcon->output_carat += snprintf( dst, EGOBOO_CONSOLE_OUTPUT - pcon->output_carat, "%s", src );
    pcon->output_buffer[EGOBOO_CONSOLE_OUTPUT-1] = '\0';

}

//--------------------------------------------------------------------------------------------
egoboo_console_t * egoboo_console_new( egoboo_console_t * pcon, SDL_Rect Con_rect, egoboo_console_callback_t pcall, void * data  )
{
    SDL_bool local_allocation = SDL_FALSE;

    if ( NULL == pcon )
    {
        local_allocation = SDL_TRUE;
        pcon = (egoboo_console_t *) calloc(1, sizeof(egoboo_console_t));
    }

    if (NULL == pcon) return NULL;

    // reset all the console data
    memset( pcon, 0, sizeof(egoboo_console_t) );

    // set the console's font
    pcon->pfont = fnt_loadFont( "basicdat" SLASH_STR "pc8x8.fon", 12 );

    // set the console's rectangle
    pcon->rect = Con_rect;

    // register the "run" callback
    pcon->run_func = pcall;
    pcon->run_data = data;

    // insert the new console as the top console
    egoboo_console_stack_push_front( pcon );

    return pcon;
}

//--------------------------------------------------------------------------------------------
SDL_bool egoboo_console_run( egoboo_console_t * pcon )
{
    SDL_bool retval = SDL_FALSE;

    if (NULL == pcon) return retval;

    if (NULL != pcon->run_func)
    {
        retval = pcon->run_func(pcon, pcon->run_data);
    }

    return retval;
};

//--------------------------------------------------------------------------------------------
SDL_bool egoboo_console_delete( egoboo_console_t * pcon, SDL_bool do_free )
{
    SDL_bool retval = SDL_FALSE;

    if ( NULL == pcon ) return SDL_FALSE;

    fnt_freeFont( pcon->pfont );

    // remove the console from the stack
    egoboo_console_stack_unlink(pcon);

    if ( do_free ) free( pcon );

    return retval;
}

//--------------------------------------------------------------------------------------------
void egoboo_console_draw_begin()
{
    ATTRIB_PUSH( "egoboo_console_draw_begin", GL_ENABLE_BIT );

    GL_DEBUG(glDisable)( GL_DEPTH_TEST );
    GL_DEBUG(glDisable)( GL_CULL_FACE );
    GL_DEBUG(glEnable)( GL_TEXTURE_2D );

    GL_DEBUG(glEnable)( GL_BLEND );
    GL_DEBUG(glBlendFunc)( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    GL_DEBUG(glViewport)( 0, 0, sdl_scr.x, sdl_scr.y );

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame
    GL_DEBUG(glMatrixMode)( GL_PROJECTION );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glLoadIdentity)();
    GL_DEBUG(glOrtho)( 0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glLoadIdentity)();
}

//--------------------------------------------------------------------------------------------
void egoboo_console_draw_end()
{
    // Restore the OpenGL matrices to what they were
    GL_DEBUG(glMatrixMode)( GL_PROJECTION );
    GL_DEBUG(glPopMatrix)();

    GL_DEBUG(glMatrixMode)( GL_MODELVIEW );
    GL_DEBUG(glLoadIdentity)();

    // Re-enable any states disabled by gui_beginFrame
    ATTRIB_POP( "egoboo_console_draw_end" );
}

//--------------------------------------------------------------------------------------------
SDL_bool egoboo_console_draw( egoboo_console_t * pcon )
{
    char   buffer[EGOBOO_CONSOLE_WRITE_LEN];
    size_t console_line_count;
    size_t console_line_offsets[1024];
    size_t console_line_lengths[1024];
    char * pstr;

    SDL_Rect * pwin;
    SDL_Surface * surf = SDL_GetVideoSurface();

    if ( NULL == surf || NULL == pcon || !pcon->on ) return SDL_FALSE;

    pwin = &(pcon->rect);

    GL_DEBUG(glDisable)( GL_TEXTURE_2D );

    GL_DEBUG(glColor4f)(1, 1, 1, 1);
    GL_DEBUG(glLineWidth)( 5 );
    GL_DEBUG(glBegin)( GL_LINE_LOOP );
    {
        GL_DEBUG(glVertex2i)( pwin->x,           pwin->y );
        GL_DEBUG(glVertex2i)( pwin->x + pwin->w, pwin->y );
        GL_DEBUG(glVertex2i)( pwin->x + pwin->w, pwin->y + pwin->h );
        GL_DEBUG(glVertex2i)( pwin->x,           pwin->y + pwin->h );
    }
    GL_DEBUG_END();
    GL_DEBUG(glLineWidth)( 1 );

    GL_DEBUG(glColor4f)(0, 0, 0, 1);
    GL_DEBUG(glBegin)( GL_POLYGON );
    {
        GL_DEBUG(glVertex2i)( pwin->x,           pwin->y );
        GL_DEBUG(glVertex2i)( pwin->x + pwin->w, pwin->y );
        GL_DEBUG(glVertex2i)( pwin->x + pwin->w, pwin->y + pwin->h );
        GL_DEBUG(glVertex2i)( pwin->x,           pwin->y + pwin->h );
    }
    GL_DEBUG_END();

    GL_DEBUG(glEnable)( GL_TEXTURE_2D );

    GL_DEBUG(glColor4f)(1, 1, 1, 1);
    ATTRIB_PUSH( "egoboo_console_draw", GL_SCISSOR_BIT | GL_ENABLE_BIT );
    {
        int text_w, text_h, height;

        // make the texture a "null" texture
        GL_DEBUG(glBindTexture)( GL_TEXTURE_2D, (GLuint)(~0) );

        // clip the viewport
        GL_DEBUG(glEnable)( GL_SCISSOR_TEST );
        GL_DEBUG(glScissor)( pwin->x, surf->h - (pwin->y + pwin->h), pwin->w, pwin->h );

        height = pwin->h;

        // draw the current command line
        buffer[0] = EGOBOO_CONSOLE_PROMPT;
        buffer[1] = ' ';
        buffer[2] = '\0';

        strncat( buffer, pcon->buffer, 1022 );
        buffer[1022] = '\0';

        fnt_getTextSize( pcon->pfont, buffer, &text_w, &text_h );
        height -= text_h;
        fnt_drawText( pcon->pfont, pwin->x, height - text_h, buffer );

        if ( '\0' != pcon->output_buffer[0] )
        {
            int i;

            // grab the line offsets
            console_line_count = 0;
            pstr = pcon->output_buffer;
            while (NULL != pstr)
            {
                size_t len;

                len = strcspn(pstr, "\n" );

                console_line_offsets[console_line_count] = pstr - pcon->output_buffer;
                console_line_lengths[console_line_count] = len;

                if (0 == len) break;

                pstr += len + 1;
                console_line_count++;
            }

            // draw the last output line and work backwards
            for ( i = console_line_count - 1; i >= 0 && height > 0 ; i-- )
            {
                size_t len = MIN(1023, console_line_lengths[i]);

                strncpy( buffer, pcon->output_buffer + console_line_offsets[i], len );
                buffer[len] = '\0';

                fnt_getTextSize( pcon->pfont, buffer, &text_w, &text_h );
                height -= text_h;
                fnt_drawText( pcon->pfont, pwin->x, height - text_h, buffer );
            }
        }

    };

    ATTRIB_POP( "egoboo_console_draw" );
    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
void egoboo_console_draw_all()
{
    egoboo_console_t * pcon = egoboo_console_top;

    if ( NULL == pcon ) return;

    egoboo_console_draw_begin();
    {
        for ( pcon = egoboo_console_top; NULL != pcon; pcon = pcon->pnext )
        {
            egoboo_console_draw( pcon );
        }
    }
    egoboo_console_draw_end();

}

//--------------------------------------------------------------------------------------------
void egoboo_console_show( egoboo_console_t * pcon )
{
    if ( NULL != pcon )
    {
        // turn the console on
        pcon->on = SDL_TRUE;
    }

    // fix the keyrepeat
    if ( NULL == egoboo_console_top )
    {
        SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
    }
    else
    {
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    }
}

//--------------------------------------------------------------------------------------------
void egoboo_console_hide( egoboo_console_t * pcon )
{
    if ( NULL != pcon )
    {
        // turn the console on
        pcon->on = SDL_FALSE;
    }

    // fix the keyrepeat
    if ( NULL == egoboo_console_top )
    {
        SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
    }
    else
    {
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    }
}

//--------------------------------------------------------------------------------------------
char * egoboo_console_get_saved( egoboo_console_t * pcon )
{
    if (NULL == pcon) return "";

    pcon->save_count = CLIP( pcon->save_count, 0, EGOBOO_CONSOLE_LINES );
    pcon->save_index = CLIP( pcon->save_index, 0, pcon->save_count - 1 );

    return pcon->save_buffer[pcon->save_index];
}

//--------------------------------------------------------------------------------------------
void egoboo_console_add_saved( egoboo_console_t * pcon, char * str )
{
    if (NULL == pcon) return;

    pcon->save_count = CLIP( pcon->save_count, 0, EGOBOO_CONSOLE_LINES );

    if ( pcon->save_count >= EGOBOO_CONSOLE_LINES )
    {
        int i;

        // bump all of the saved lines so that we can insert a new one
        for (i = 0; i < EGOBOO_CONSOLE_LINES - 1; i++)
        {
            strncpy( pcon->save_buffer[i], pcon->save_buffer[i+1], EGOBOO_CONSOLE_LENGTH );
        }
        pcon->save_count--;
    }

    strncpy( pcon->save_buffer[pcon->save_count], str, EGOBOO_CONSOLE_LENGTH );
    pcon->save_count++;
    pcon->save_index = pcon->save_count;
}

//--------------------------------------------------------------------------------------------
SDL_Event * egoboo_console_handle_events( SDL_Event * pevt )
{
    egoboo_console_t     * pcon = egoboo_console_top;
    SDLKey              vkey;

    Uint32 kmod;
    SDL_bool is_alt, is_shift;

    if ( NULL == pcon || NULL == pevt ) return pevt;

    // only handle keyboard events
    if ( SDL_KEYDOWN != pevt->type ) return pevt;

    // grab the virtual key code
    vkey = pevt->key.keysym.sym;

    // get any keymods
    kmod = SDL_GetModState();

    is_alt   = (SDL_bool)HAS_SOME_BITS( kmod, KMOD_ALT | KMOD_CTRL );
    is_shift = (SDL_bool)HAS_SOME_BITS( kmod, KMOD_SHIFT );

    // start the top console
    if ( !is_alt && !is_shift && SDLK_BACKQUOTE == vkey )
    {
        if ( !pcon->on )
        {
            pcon->on = SDL_TRUE;
            pcon->buffer_carat = 0;
            pcon->buffer[0]    = '\0';

            SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_DELAY );
            return NULL;
        }
    };

    // quit the top console
    if ( !is_alt && !is_shift && SDLK_ESCAPE == vkey )
    {
        if ( pcon->on )
        {
            pcon->on           = SDL_FALSE;
            pcon->buffer_carat = 0;
            pcon->buffer[0]    = '\0';

            SDL_EnableKeyRepeat( 0, SDL_DEFAULT_REPEAT_DELAY );
            return NULL;
        }
    };

    // Only grab the keycodes if the terminal is on
    if ( !pcon->on ) return pevt;

    // handle any terminal commands
    if ( NULL != pevt && !is_alt && !is_shift )
    {
        if ( SDLK_BACKSPACE == vkey )
        {
            if (pcon->buffer_carat > 0)
            {
                pcon->buffer_carat--;
            }
            pcon->buffer[pcon->buffer_carat] = '\0';

            pevt = NULL;
        }
        else if ( SDLK_UP == vkey )
        {
            pcon->save_index--;

            if ( pcon->save_index < 0 )
            {
                // after the last command line. blank the line
                pcon->save_index   = 0;
                pcon->buffer[0]    = '\0';
                pcon->buffer_carat = 0;
            }
            else
            {
                pcon->save_index = CLIP( pcon->save_index, 0, pcon->save_count - 1 );

                if ( pcon->save_count > 0)
                {
                    strncpy(pcon->buffer, pcon->save_buffer[pcon->save_index], sizeof(pcon->buffer) );
                    pcon->buffer_carat = strlen(pcon->buffer) - 1;
                }
            }

            pevt = NULL;
        }
        else if ( SDLK_DOWN == vkey )
        {
            pcon->save_index++;

            if ( pcon->save_index >= pcon->save_count )
            {
                // before the first command line. blank the line
                pcon->save_index   = pcon->save_count;
                pcon->buffer[0]    = '\0';
                pcon->buffer_carat = 0;
            }
            else
            {
                pcon->save_index = CLIP( pcon->save_index, 0, pcon->save_count - 1 );

                if ( pcon->save_count > 0)
                {
                    strncpy(pcon->buffer, pcon->save_buffer[pcon->save_index], EGOBOO_CONSOLE_LENGTH - 1);
                    pcon->buffer_carat = strlen(pcon->buffer) - 1;
                }
            }

            pevt = NULL;
        }
        else if ( SDLK_LEFT == vkey )
        {
            pcon->buffer_carat--;
            pcon->buffer_carat = CLIP( pcon->buffer_carat, 0, EGOBOO_CONSOLE_LENGTH - 1 );

            pevt = NULL;
        }

        else if ( SDLK_RIGHT == vkey )
        {
            pcon->buffer_carat++;
            pcon->buffer_carat = CLIP( pcon->buffer_carat, 0, EGOBOO_CONSOLE_LENGTH - 1 );

            pevt = NULL;
        }

        else if ( SDLK_RETURN == vkey || SDLK_KP_ENTER == vkey )
        {
            pcon->buffer[pcon->buffer_carat] = '\0';

            // add this command to the "saved command list"
            egoboo_console_add_saved(pcon, pcon->buffer);

            // add the command to the output buffer
            egoboo_console_fprint( pcon, "%c %s\n", EGOBOO_CONSOLE_PROMPT, pcon->buffer );

            // actually execute the command
            egoboo_console_run( pcon );

            // blank the command line
            pcon->buffer_carat = 0;
            pcon->buffer[0] = '\0';

            pevt = NULL;
        }
    }

    // handle normal keystrokes
    if ( NULL != pevt && !is_alt && vkey < SDLK_NUMLOCK )
    {
        if ( pcon->buffer_carat < EGOBOO_CONSOLE_LENGTH )
        {
            if ( is_shift )
            {
                pcon->buffer[pcon->buffer_carat++] = scancode_to_ascii_shift[vkey];
            }
            else
            {
                pcon->buffer[pcon->buffer_carat++] = scancode_to_ascii[vkey];
            }
            pcon->buffer[pcon->buffer_carat] = '\0';

            pevt = NULL;
        }
    }

    return pevt;
}

//--------------------------------------------------------------------------------------------
void init_scancodes()
{
    // BB > initialize the scancode translation

    int i;

    // do the basic translation
    for ( i = 0; i < SDLK_LAST; i++ )
    {
        // SDL uses ascii values for it's virtual scancodes
        scancode_to_ascii[i] = i;
        if ( i < 255 )
        {
            scancode_to_ascii_shift[i] = toupper(i);
        }
        else
        {
            scancode_to_ascii_shift[i] = scancode_to_ascii[i];
        }
    }

    // fix the keymap
    scancode_to_ascii_shift[SDLK_1]  = '!';
    scancode_to_ascii_shift[SDLK_2]  = '@';
    scancode_to_ascii_shift[SDLK_3]  = '#';
    scancode_to_ascii_shift[SDLK_4]  = '$';
    scancode_to_ascii_shift[SDLK_5]  = '%';
    scancode_to_ascii_shift[SDLK_6]  = '^';
    scancode_to_ascii_shift[SDLK_7]  = '&';
    scancode_to_ascii_shift[SDLK_8]  = '*';
    scancode_to_ascii_shift[SDLK_9]  = '(';
    scancode_to_ascii_shift[SDLK_0]  = ')';

    scancode_to_ascii_shift[SDLK_QUOTE]        = '\"';
    scancode_to_ascii_shift[SDLK_SEMICOLON]    = ':';
    scancode_to_ascii_shift[SDLK_PERIOD]       = '>';
    scancode_to_ascii_shift[SDLK_COMMA]        = '<';
    scancode_to_ascii_shift[SDLK_BACKQUOTE]    = '~';
    scancode_to_ascii_shift[SDLK_MINUS]        = '_';
    scancode_to_ascii_shift[SDLK_EQUALS]       = '+';
    scancode_to_ascii_shift[SDLK_LEFTBRACKET]  = '{';
    scancode_to_ascii_shift[SDLK_RIGHTBRACKET] = '}';
    scancode_to_ascii_shift[SDLK_BACKSLASH]    = '|';
    scancode_to_ascii_shift[SDLK_SLASH]        = '?';
}
