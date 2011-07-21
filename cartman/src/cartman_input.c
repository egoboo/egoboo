#include "cartman_input.h"

#include "cartman_gui.h"
#include "cartman_math.inl"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

mouse_t      mos   = { btrue };
keyboard_t   key = { btrue };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool_t check_input_mouse( SDL_Event * pevt );
static bool_t check_input_keyboard( SDL_Event * pevt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void update_mouse()
{
    if ( !mos.on ) return;

    mos.x_old = mos.x;
    mos.y_old = mos.y;
}

//--------------------------------------------------------------------------------------------
bool_t check_keys( Uint32 resolution )
{
    static int last_tick = -1;
    int tick;

    // 20 ticks per key dalay
    tick = SDL_GetTicks();
    if ( tick < last_tick + resolution ) return bfalse;
    last_tick = tick;

    if ( key.delay > 0 )
    {
        key.delay--;
        return bfalse;
    }

    if ( !key.on ) return bfalse;

    if ( key.needs_update )
    {
        key.sdlbuffer = SDL_GetKeyState( &( key.count ) );
        key.needs_update = bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void check_input()
{
    // ZZ> This function gets all the current player input states
    SDL_Event evt;

    while ( SDL_PollEvent( &evt ) )
    {
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
                    if ( key.on ) key.needs_update = btrue;
                }

                if ( HAS_BITS( evt.active.state, SDL_APPACTIVE ) )
                {
                    if ( 1 != evt.active.gain )
                    {
                        mos.on = bfalse;
                        key.on = bfalse;
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
bool_t check_input_mouse( SDL_Event * pevt )
{
    bool_t handled = bfalse;

    if ( NULL == pevt || !mos.on ) return bfalse;

    if ( 0 == mos.b )
    {
        mos.drag = bfalse;
        mos.drag_begin = bfalse;

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
            ui.pending_click = btrue;
            handled = btrue;
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
            ui.pending_click = bfalse;
            handled = btrue;
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
                    mos.drag = bfalse;
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
            mos.drag = btrue;
        }
        else if ( !mos.drag )
        {
            // set the dragging to begin the next mouse time the mouse moves
            mos.drag_begin = btrue;

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
bool_t check_input_keyboard( SDL_Event * pevt )
{
    bool_t handled = bfalse;

    if ( NULL == pevt || !key.on ) return bfalse;

    switch ( pevt->type )
    {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            key.state = pevt->key.state;
            key.mod   = pevt->key.keysym.mod;
            key.needs_update = btrue;
            handled = btrue;
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

    ptr->on = btrue;

    return ptr;
}

//--------------------------------------------------------------------------------------------
keyboard_t * keyboard_ctor( keyboard_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->on           = btrue;
    ptr->needs_update = btrue;

    return ptr;
}

