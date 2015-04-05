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

/// @file egolib/console.c
/// @brief A quake-style console that can be used for anything.
/// @details

#include "egolib/console.h"

#include "egolib/file_common.h"
#include "egolib/scancode.h"
#include "egolib/strutil.h"
#include "egolib/vfs.h"

#include "egolib/Graphics/FontManager.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Extensions/SDL_extensions.h"

#if defined(USE_LUA_CONSOLE)
#include "egolib/Lua/lua_console.h"
#endif

#include "egolib/_math.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_console_t *egolib_console_top = nullptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void *_egolib_console_top = nullptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if 0
static egolib_console_t *egolib_console_ctor(egolib_console_t *self, SDL_Rect con_rect, egolib_console_callback_t callback, void *data);
static egolib_console_t *egolib_console_dtor(egolib_console_t *self);
static SDL_bool egolib_console_run(egolib_console_t *self);
static void egolib_console_write(egolib_console_t *self, const char *format, va_list args);
static const char * egolib_console_get_saved(egolib_console_t *self);
static void egolib_console_add_saved(egolib_console_t *self, char *str);
static void egolib_console_add_output(egolib_console_t *self, char *szNew);
static SDL_bool egolib_console_draw(egolib_console_t *self);
#endif

/**
 * @brief
 *  This class is to wrap the font's std::shared_ptr for use in calloc and friends
 * @todo
 *  Remove this when egolib_console_t is a proper C++ class.
 */
class egolib_console_FontWrapper final
{
public:
    egolib_console_FontWrapper(const std::shared_ptr<Ego::Font> &font) :
    _font(font)
    {
        // ctor
    }
    ~egolib_console_FontWrapper()
    {
        // dtor
    }
    std::shared_ptr<Ego::Font> _font;
};

void egolib_console_handler_t::begin()
{
    /// @author BB
    /// @details initialize the console. This must happen after the screen has been defines,
    ///     otherwise sdl_scr.x == sdl_scr.y == 0 and the screen will be defined to
    ///     have no area...

    SDL_Rect blah;

    // autimatically shut down
    atexit(egolib_console_handler_t::end);

    blah.x = 0;
    blah.y = 0;
    blah.w = sdl_scr.x;
    blah.h = sdl_scr.y * 0.25f;

    scancode_begin();

#if defined(USE_LUA_CONSOLE)
    _egolib_console_top = lua_console_create(nullptr, blah);
#else
    // without a callback, this console just dumps the input and generates no output
    _egolib_console_top = egolib_console_create(nullptr, blah, nullptr, nullptr);
#endif
}

void egolib_console_handler_t::end()
{
    /// @author BB
    /// @details de-initialize the top console

#if defined(USE_LUA_CONSOLE)
    {
        lua_console_t *ptmp = (lua_console_t *)_egolib_console_top;
        lua_console_destroy(&ptmp);
    }
#else
    // without a callback, this console just dumps the input and generates no output
    {
        egolib_console_t *ptmp = (egolib_console_t *)_egolib_console_top;
        egolib_console_destroy(&ptmp, SDL_TRUE);
    }
#endif

    _egolib_console_top = nullptr;
}

bool egolib_console_handler_t::unlink(egolib_console_t *console)
{
    bool retval = false;

    if (!console)
    {
        return false;
    }

    if (console == egolib_console_top)
    {
        egolib_console_top = console->pnext;
        retval = true;
    }
    else
    {
        // find the console that points to this one
        egolib_console_t *tmp = egolib_console_top;
        while (nullptr != tmp && nullptr != tmp->pnext)
        {
            if (tmp->pnext == console)
            {
                retval = true;
                tmp->pnext = console->pnext;
                break;
            }
            tmp = tmp->pnext;
        }
    }

    return retval;
}

bool egolib_console_handler_t::push_front(egolib_console_t *console)
{
    if (!console)
    {
        return false;
    }

    console->pnext = egolib_console_top;
    egolib_console_top = console;

    return true;
}

void egolib_console_t::printv(egolib_console_t *self, const char *format, va_list args)
{
    char buffer[EGOBOO_CONSOLE_WRITE_LEN] = EMPTY_CSTR;

    if (nullptr != self)
    {
        vsnprintf(buffer, EGOBOO_CONSOLE_WRITE_LEN - 1, format, args);

        egolib_console_t::add_output(self, buffer);
    }
}

void egolib_console_t::print(egolib_console_t *self, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    egolib_console_t::printv(self, format, args);
    va_end(args);
}

void egolib_console_t::add_output(egolib_console_t *self, char *line)
{
    if (!self || line)
    {
        return;
    }

    // How many characters are we adding?
    size_t lineLength = strlen(line);

    // initialize the pointers for the copy operation
    char *src = line;
    char *dst = self->output_buffer + self->output_carat;
    //copy_len = out_len;

    // check to make sure that the ranges are valid
    if (lineLength > EGOBOO_CONSOLE_OUTPUT)
    {
        // we need to replace the entire output buffer with
        // a portion of szNew

        size_t offset = lineLength - EGOBOO_CONSOLE_OUTPUT - 1;

        // update the copy parameters
        src      = line + offset;
        //copy_len = out_len - offset;
    }
    else if (self->output_carat + lineLength > EGOBOO_CONSOLE_OUTPUT)
    {
        // the length of the buffer after adding szNew would be too large
        // get rid of some of the input buffer and then add szNew

        size_t offset = (self->output_carat + lineLength) - EGOBOO_CONSOLE_OUTPUT - 1;

        // move the memory so that we create some space
        memmove(self->output_buffer, self->output_buffer + offset, self->output_carat - offset);

        // update the copy parameters
        self->output_carat -= offset;
        dst = self->output_buffer - self->output_carat;
    }

    self->output_carat += snprintf(dst, EGOBOO_CONSOLE_OUTPUT - self->output_carat, "%s", src);
    self->output_buffer[EGOBOO_CONSOLE_OUTPUT-1] = CSTR_END;
}

egolib_console_t *egolib_console_t::ctor(egolib_console_t *self, SDL_Rect con_rect, egolib_console_callback_t callback, void *data)
{
    if (!self)
    {
        return nullptr;
    }

    // reset all the console data
    BLANK_STRUCT_PTR(self);

    // set the console's font
    self->pfont = new egolib_console_FontWrapper(Ego::FontManager::loadFont("mp_data/pc8x8.fon", 12));

    // set the console's rectangle
    self->rect = con_rect;

    // register the "run" callback
    self->run_func = callback;
    self->run_data = data;

    // insert the new console as the top console
    egolib_console_handler_t::push_front(self);

    return self;
}

egolib_console_t *egolib_console_create(egolib_console_t *console, SDL_Rect rect, egolib_console_callback_t callback, void *data)
{
    if (!console )
    {
        console = EGOBOO_NEW(egolib_console_t);
    }

    return egolib_console_t::ctor(console, rect, callback, data);
}

bool egolib_console_t::run(egolib_console_t *self)
{
    bool retval = false;

    if (!self)
    {
        return false;
    }

    if (nullptr != self->run_func)
    {
        retval = (SDL_TRUE == self->run_func(self, self->run_data)) ? true : false;
    }

    return retval;
}

egolib_console_t *egolib_console_t::dtor(egolib_console_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    // Remove the console from the stack.
    egolib_console_handler_t::unlink(self);

    // Delete its font.
    delete self->pfont;
    self->pfont = nullptr;

    return self;
}

SDL_bool egolib_console_destroy( egolib_console_t ** pcon, SDL_bool do_free )
{
    if ( NULL == pcon ) return SDL_FALSE;

    if ( NULL == egolib_console_t::dtor( *pcon ) ) return SDL_FALSE;

    if ( do_free ) EGOBOO_DELETE( *pcon );

    return SDL_TRUE;
}

void egolib_console_handler_t::draw_begin()
{
    // do not use the ATTRIB_PUSH macro, since the glPopAttrib() is in a different function
    GL_DEBUG( glPushAttrib )( GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT );

    // don't worry about hidden surfaces
	Ego::Renderer::get().setDepthTestEnabled(false);

    // draw draw front and back faces of polygons
    oglx_end_culling();                                         // GL_ENABLE_BIT

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );                                         // GL_ENABLE_BIT

    Ego::Renderer::get().setBlendingEnabled(true);
    GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // GL_COLOR_BUFFER_BIT

    Ego::Renderer::get().setViewportRectangle(0, 0, sdl_scr.x, sdl_scr.y);                          // GL_VIEWPORT_BIT

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame

    // store the GL_PROJECTION matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadIdentity )();
    GL_DEBUG( glOrtho )( 0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glLoadIdentity )();
}

void egolib_console_handler_t::draw_end()
{
    // Restore the GL_PROJECTION matrix
    GL_DEBUG( glMatrixMode )( GL_PROJECTION );
    GL_DEBUG( glPopMatrix )();

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // Re-enable any states disabled by gui_beginFrame
    // do not use the ATTRIB_POP macro, since the glPushAttrib() is in a different function
    GL_DEBUG( glPopAttrib )();
}

bool egolib_console_t::draw(egolib_console_t *self)
{
    char   buffer[EGOBOO_CONSOLE_WRITE_LEN] = EMPTY_CSTR;
    size_t console_line_count;
    size_t console_line_offsets[1024];
    size_t console_line_lengths[1024];
    char * pstr;

    SDL_Rect * pwin;
    SDL_Surface * surf = SDL_GetVideoSurface();

    if ( NULL == surf || NULL == self || !self->on ) return false;

    pwin = &(self->rect);

    GL_DEBUG( glDisable )( GL_TEXTURE_2D );

    GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
    GL_DEBUG( glLineWidth )( 5 );
    GL_DEBUG( glBegin )( GL_LINE_LOOP );
    {
        GL_DEBUG( glVertex2i )( pwin->x,           pwin->y );
        GL_DEBUG( glVertex2i )( pwin->x + pwin->w, pwin->y );
        GL_DEBUG( glVertex2i )( pwin->x + pwin->w, pwin->y + pwin->h );
        GL_DEBUG( glVertex2i )( pwin->x,           pwin->y + pwin->h );
    }
    GL_DEBUG_END();

    GL_DEBUG( glLineWidth )( 1 );

    GL_DEBUG( glColor4f )( 0, 0, 0, 1 );
    GL_DEBUG( glBegin )( GL_POLYGON );
    {
        GL_DEBUG( glVertex2i )( pwin->x,           pwin->y );
        GL_DEBUG( glVertex2i )( pwin->x + pwin->w, pwin->y );
        GL_DEBUG( glVertex2i )( pwin->x + pwin->w, pwin->y + pwin->h );
        GL_DEBUG( glVertex2i )( pwin->x,           pwin->y + pwin->h );
    }
    GL_DEBUG_END();

    GL_DEBUG( glEnable )( GL_TEXTURE_2D );

    Ego::Renderer::get().setColour(Ego::Math::Colour4f(1.0f, 1.0f, 1.0f, 1.0f));
    ATTRIB_PUSH( __FUNCTION__, GL_SCISSOR_BIT | GL_ENABLE_BIT );
    {
        int text_w, text_h, height;
        Ego::Math::Colour4f con_color = Ego::Math::Colour4f::white();

        // make the texture a "null" texture
        GL_DEBUG( glBindTexture )( GL_TEXTURE_2D, ( GLuint )( ~0 ) );

        // clip the viewport
		Ego::Renderer::get().setScissorTestEnabled(true);
        Ego::Renderer::get().setScissorRectangle(pwin->x, surf->h - ( pwin->y + pwin->h ), pwin->w, pwin->h);

        height = pwin->h;

        // draw the current command line
        buffer[0] = EGOBOO_CONSOLE_PROMPT;
        buffer[1] = ' ';
        buffer[2] = CSTR_END;

        strncat( buffer, self->buffer, 1022 );
        buffer[1022] = CSTR_END;

        self->pfont->_font->getTextSize(buffer, &text_w, &text_h);
        height -= text_h;
        self->pfont->_font->drawText(buffer, pwin->x, height - text_h, con_color);

        if ( CSTR_END != self->output_buffer[0] )
        {
            int i;

            // grab the line offsets
            console_line_count = 0;
            pstr = self->output_buffer;
            while ( NULL != pstr )
            {
                size_t len;

                len = strcspn( pstr, "\n" );

                console_line_offsets[console_line_count] = pstr - self->output_buffer;
                console_line_lengths[console_line_count] = len;

                if ( 0 == len ) break;

                pstr += len + 1;
                console_line_count++;
            }

            // draw the last output line and work backwards
            for ( i = (( int )console_line_count ) - 1; i >= 0 && height > 0 ; i-- )
            {
                size_t len = std::min( (size_t)1023, console_line_lengths[i] );

                strncpy( buffer, self->output_buffer + console_line_offsets[i], len );
                buffer[len] = CSTR_END;
                
                self->pfont->_font->getTextSize(buffer, &text_w, &text_h);
                height -= text_h;
                self->pfont->_font->drawText(buffer, pwin->x, height - text_h, con_color);
            }
        }

    };
    ATTRIB_POP( __FUNCTION__ );

    return true;
}

void egolib_console_handler_t::draw_all()
{
    egolib_console_t *console = egolib_console_top;

    if (!console)
    {
        return;
    }

    egolib_console_handler_t::draw_begin();

    for (; nullptr != console; console = console->pnext)
    {
        egolib_console_t::draw(console);
    }

    egolib_console_handler_t::draw_end();
}

void egolib_console_t::show(egolib_console_t *self)
{
    if (nullptr != self)
    {
        // Turn the console on.
        self->on = true;
    }

    // Fix the keyrepeat.
    if (nullptr == egolib_console_top)
    {
        SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
    }
    else
    {
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    }
}

void egolib_console_t::hide(egolib_console_t *self)
{
    if (nullptr != self)
    {
        // Turn the console off.
        self->on = SDL_FALSE;
    }

    // Fix the keyrepeat.
    if (nullptr == egolib_console_top)
    {
        SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);
    }
    else
    {
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    }
}

const char *egolib_console_t::get_saved(egolib_console_t *self)
{
    if (!self)
    {
        return "";
    }

    self->save_count = CLIP(self->save_count, 0, EGOBOO_CONSOLE_LINES);
    self->save_index = CLIP(self->save_index, 0, self->save_count - 1);

    return self->save_buffer[self->save_index];
}

void egolib_console_t::add_saved(egolib_console_t *self, char *line)
{
    if (!self)
    {
        return;
    }

    self->save_count = CLIP(self->save_count, 0, EGOBOO_CONSOLE_LINES);

    if (self->save_count == EGOBOO_CONSOLE_LINES)
    {
        // Bump all of the saved lines so that we can insert a new one.
        for (size_t i = 0; i < EGOBOO_CONSOLE_LINES - 1; ++i)
        {
            strncpy(self->save_buffer[i], self->save_buffer[i+1], EGOBOO_CONSOLE_LENGTH);
        }
        self->save_count--;
    }

    strncpy(self->save_buffer[self->save_count], line, EGOBOO_CONSOLE_LENGTH);
    self->save_count++;
    self->save_index = self->save_count;
}

SDL_Event *egolib_console_handler_t::handle_event(SDL_Event *event)
{
    egolib_console_t *console = egolib_console_top;

    if (!event)
    {
        return nullptr;
    }
    if (!console)
    {
        return event;
    }

    // Only handle keyboard events.
    if (SDL_KEYDOWN != event->type)
    {
        return event;
    }

    // Grab the virtual key code.
    SDLKey vkey = event->key.keysym.sym;

    // Get the key modifiers.
    Uint32 kmod = SDL_GetModState();

    // Is alt or shift down?
    bool is_alt   = HAS_SOME_BITS(kmod, KMOD_ALT | KMOD_CTRL);
    bool is_shift = HAS_SOME_BITS(kmod, KMOD_SHIFT);

    // If the virtual key code for the backquote is pressed,
    // toggle the console on the top of the console stack.
    if (!is_alt && !is_shift && SDLK_BACKQUOTE == vkey)
    {
        if (!console->on)
        {
            console->on = true;
            console->buffer_carat = 0;
            console->buffer[0] = CSTR_END;

            SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_DELAY);
            return nullptr;
        }
        else
        {
            console->on = false;
            console->buffer_carat = 0;
            console->buffer[0] = CSTR_END;

            SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_DELAY);
            return nullptr;
        }
    };

    // Only grab the keycodes if the console is on.
    if (!console->on)
    {
        return event;
    }

    // Handle any console commands.
    if (nullptr != event && !is_alt && !is_shift)
    {
        // backspace: delete character before the carat.
        if (SDLK_BACKSPACE == vkey)
        {
            if (console->buffer_carat > 0)
            {
                console->buffer_carat--;
            }
            console->buffer[console->buffer_carat] = CSTR_END;

            event = nullptr;
        }
        else if (SDLK_UP == vkey)
        {
            console->save_index--;

            if (console->save_index < 0)
            {
                // Behind the last saved line. Blank the line.
                console->save_index = 0;
                console->buffer[0] = CSTR_END;
                console->buffer_carat = 0;
            }
            else
            {
                console->save_index = CLIP(console->save_index, 0, console->save_count - 1);

                if (console->save_count > 0)
                {
                    strncpy(console->buffer, egolib_console_t::get_saved(console), EGOBOO_CONSOLE_LENGTH - 1);
                    console->buffer_carat = strlen(console->buffer);
                }
            }

            event = nullptr;
        }
        else if (SDLK_DOWN == vkey)
        {
            console->save_index++;

            if (console->save_index >= console->save_count)
            {
                // Before the first saved line. Blank the line.
                console->save_index = console->save_count;
                console->buffer[0] = CSTR_END;
                console->buffer_carat = 0;
            }
            else
            {
                console->save_index = CLIP(console->save_index, 0, console->save_count - 1);

                if (console->save_count > 0)
                {
                    strncpy(console->buffer, egolib_console_t::get_saved(console), EGOBOO_CONSOLE_LENGTH - 1);
                    console->buffer_carat = strlen(console->buffer);
                }
            }

            event = nullptr;
        }
        else if (SDLK_LEFT == vkey)
        {
            console->buffer_carat--;
            console->buffer_carat = CLIP(console->buffer_carat, (size_t)0, (size_t)(EGOBOO_CONSOLE_LENGTH - 1));

            event = nullptr;
        }

        else if (SDLK_RIGHT == vkey)
        {
            console->buffer_carat++;
            console->buffer_carat = CLIP(console->buffer_carat, (size_t)0, (size_t)(EGOBOO_CONSOLE_LENGTH - 1));

            event = nullptr;
        }
        else if (SDLK_RETURN == vkey || SDLK_KP_ENTER == vkey)
        {
            console->buffer[console->buffer_carat] = CSTR_END;

            // Add this command line to the list of saved command line.
            egolib_console_t::add_saved(console, console->buffer);

            // Add the command line to the output buffer.
            egolib_console_t::print(console, "%c %s\n", EGOBOO_CONSOLE_PROMPT, console->buffer);

            // Actually execute the command line.
            egolib_console_t::run(console);

            // Blank the command line.
            console->buffer_carat = 0;
            console->buffer[0] = CSTR_END;

            event = nullptr;
        }
    }

    // handle normal keystrokes
    if (nullptr != event && !is_alt && vkey < SDLK_NUMLOCK )
    {
        if (console->buffer_carat < EGOBOO_CONSOLE_LENGTH)
        {
            if (is_shift)
            {
                if ((unsigned)scancode_to_ascii_shift[vkey] <= 0xFF)
                {
                    console->buffer[console->buffer_carat++] = (char)scancode_to_ascii_shift[vkey];
                }
            }
            else
            {
                if ((unsigned)scancode_to_ascii[vkey] <= 0xFF)
                {
                    console->buffer[console->buffer_carat++] = (char)scancode_to_ascii[vkey];
                }
            }
            console->buffer[console->buffer_carat] = CSTR_END;

            event = nullptr;
        }
    }

    return event;
}
