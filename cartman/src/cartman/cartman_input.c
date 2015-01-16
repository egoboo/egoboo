//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "cartman/cartman_input.h"

#include "cartman/cartman_gui.h"
#include "cartman/cartman_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

mouse_t      mos = { true };
keyboard_t   key = { true, false };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool check_input_mouse( SDL_Event * pevt );
static bool check_input_keyboard( SDL_Event * pevt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void update_mouse()
{
    if ( !mos.on ) return;

    mos.x_old = mos.x;
    mos.y_old = mos.y;
}

//--------------------------------------------------------------------------------------------
bool check_keys( Uint32 resolution )
{
    static int last_tick = -1;
    int tick;

    // 20 ticks per key dalay
    tick = SDL_GetTicks();
    if ( tick < last_tick + resolution ) return false;
    last_tick = tick;

    if ( key.delay > 0 )
    {
        key.delay--;
        return false;
    }

    if ( !key.on ) return false;

    if ( key.needs_update )
    {
        key.sdlbuffer = SDL_GetKeyState( &( key.count ) );
        key.needs_update = false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void check_input()
{
    // ZZ> This function gets all the current player input states
    SDL_Event evt;

    while ( SDL_PollEvent( &evt ) )
    {
        if ( NULL == egolib_console_handle_events( &evt ) )
        {
            key.override = true;
            continue;
        }
        else
        {
            key.override = false;
        }

        switch ( evt.type )
        {
            case SDL_ACTIVEEVENT:

                if ( HAS_BITS( evt.active.state, SDL_APPMOUSEFOCUS ) )
                {
                    mos.on = ( 1 == evt.active.gain );
                }

                if ( HAS_BITS( evt.active.state, SDL_APPINPUTFOCUS ) )
                {
                    key.on = ( 1 == evt.active.gain );
                    if ( key.on ) key.needs_update = true;
                }

                if ( HAS_BITS( evt.active.state, SDL_APPACTIVE ) )
                {
                    if ( 1 != evt.active.gain )
                    {
                        mos.on = false;
                        key.on = false;
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
                check_input_mouse( &evt );
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                check_input_keyboard( &evt );
                break;
        }
    }
};

//--------------------------------------------------------------------------------------------
bool check_input_mouse( SDL_Event * pevt )
{
    bool handled = false;

    if ( NULL == pevt || !mos.on ) return false;

    if ( 0 == mos.b )
    {
        mos.drag = false;
        mos.drag_begin = false;

        // set mdata??
    }

    switch ( pevt->type )
    {
        case SDL_MOUSEBUTTONDOWN:
            switch ( pevt->button.button )
            {
                case SDL_BUTTON_LEFT:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = true;
            handled = true;
            break;

        case SDL_MOUSEBUTTONUP:
            switch ( pevt->button.button )
            {
                case SDL_BUTTON_LEFT:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = false;
            handled = true;
            break;

        case SDL_MOUSEMOTION:
            mos.b = pevt->motion.state;
            if ( mos.drag )
            {
                if ( 0 != mos.b )
                {
                    mos.brx = pevt->motion.x;
                    mos.bry = pevt->motion.y;
                }
                else
                {
                    mos.drag = false;
                }
            }

            if ( mos.relative )
            {
                mos.cx = pevt->motion.xrel;
                mos.cy = pevt->motion.yrel;
            }
            else
            {
                mos.x = pevt->motion.x;
                mos.y = pevt->motion.y;
            }
            break;
    }

    if ( 0 != mos.b )
    {
        if ( mos.drag_begin )
        {
            // start dragging
            mos.drag = true;
        }
        else if ( !mos.drag )
        {
            // set the dragging to begin the next mouse time the mouse moves
            mos.drag_begin = true;

            // initialize the drag rect
            mos.tlx = mos.x;
            mos.tly = mos.y;

            mos.brx = mos.x;
            mos.bry = mos.y;

            // set the drag window
            mos.drag_window = find_window( mos.x, mos.y );
            mos.drag_mode   = ( NULL == mos.drag_window ) ? 0 : mos.drag_mode;
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool check_input_keyboard( SDL_Event * pevt )
{
    bool handled = false;

    if ( NULL == pevt || !key.on ) return false;

    switch ( pevt->type )
    {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            key.state = pevt->key.state;
            key.mod   = pevt->key.keysym.mod;
            key.needs_update = true;
            handled = true;
            break;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mouse_t * mouse_ctor( mouse_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->on = true;

    return ptr;
}

//--------------------------------------------------------------------------------------------
keyboard_t * keyboard_ctor( keyboard_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->on           = true;
    ptr->needs_update = true;

    return ptr;
}
