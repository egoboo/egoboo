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

/// @file game/input.c
/// @brief Keyboard, mouse, and joystick handling code.
/// @details

#include "game/input.h"

#include "game/ui.h"
#include "game/network.h"
#include "game/menu.h"
#include "game/graphic.h"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Raw input devices
mouse_data_t    mous = MOUSE_INIT;
keyboard_data_t keyb = KEYBOARD_INIT;
joystick_data_t joy_lst[MAX_JOYSTICK];
input_cursor_t  input_cursor = {0, 0, false, false, false, false};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void input_system_init_keyboard();
static void input_system_init_mouse();
static void input_system_init_joysticks();
static void input_system_init_devices();

static void input_read_mouse();
static void input_read_keyboard();
static void input_read_joysticks();
static void input_read_joystick(int which);

static bool input_handle_SDL_Event( SDL_Event * pevt );
static bool input_handle_SDL_KEYDOWN( SDL_Event * pevt );
static bool input_handle_chat( SDL_Event * pevt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void input_system_init_keyboard()
{
    // set up the keyboard
    keyboard_data__init( &keyb );

    // keyboard data
    scancode_begin();

    // turn the keyboard on
    keyb.on         = true;
}

//--------------------------------------------------------------------------------------------
void input_system_init_mouse()
{
    /// @author BB
    /// @details set up the mouse

    mouse_data__init( &mous );

    mous.on = true;
}

//--------------------------------------------------------------------------------------------
void input_system_init_joysticks()
{
    /// @author BB
    /// @details init the joysticks

    int i;

    for ( i = 0; i < MAX_JOYSTICK; i++ )
    {
        joystick_data__init( joy_lst + i );

        if ( i < SDL_NumJoysticks() )
        {
            joy_lst[i].sdl_ptr = SDL_JoystickOpen( i );
            joy_lst[i].on      = ( NULL != joy_lst[i].sdl_ptr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void input_system_init_devices()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_LOCAL_PLAYERS; cnt++ )
    {
        input_device_ctor( InputDevices.lst + cnt );
    }
    InputDevices.count = 0;
}

//--------------------------------------------------------------------------------------------
void input_system_init()
{
    /// @author BB
    /// @details initialize the inputs

    log_info( "Intializing SDL Joystick... " );
    if ( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) < 0 )
    {
        log_message( "Failed!\n" );
    }
    else
    {
        log_message( "Success!\n" );
    }

    input_system_init_keyboard();
    input_system_init_mouse();
    input_system_init_joysticks();
    input_system_init_devices();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void input_read_mouse()
{
    int x, y, b;

    if ( process_t::running( PROC_PBASE( MProc ) ) )
    {
        b = SDL_GetMouseState( &x, &y );
    }
    else
    {
        b = SDL_GetRelativeMouseState( &x, &y );

        //Move mouse to the center of the screen since SDL does not detect motion outside the window
        if ( !cfg.dev_mode ) SDL_WarpMouse( GFX_WIDTH >> 1, GFX_HEIGHT >> 1 );
    }

    mous.x = -x; // mous.x and mous.y are the wrong type to use in above call
    mous.y = -y;
    mous.button[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
    mous.button[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
    mous.button[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0; // Middle is 2 on SDL
    mous.button[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;

    // Mouse mask
    mous.b = ( mous.button[3] << 3 ) | ( mous.button[2] << 2 ) | ( mous.button[1] << 1 ) | ( mous.button[0] << 0 );
}

//--------------------------------------------------------------------------------------------
void input_read_keyboard()
{
    keyb.state_ptr = SDL_GetKeyState( &( keyb.state_size ) );
}

//--------------------------------------------------------------------------------------------
void input_read_joystick( int which )
{
    int dead_zone = 0x8000 / 10;
    int i, button_count, x, y;
    joystick_data_t * pjoy;

    if ( !joy_lst[which].on ) return;

    pjoy = joy_lst + which;

    // get the raw values
    x = SDL_JoystickGetAxis( pjoy->sdl_ptr, 0 );
    y = SDL_JoystickGetAxis( pjoy->sdl_ptr, 1 );

    // make a dead zone
    if ( x > dead_zone ) x -= dead_zone;
    else if ( x < -dead_zone ) x += dead_zone;
    else x = 0;

    if ( y > dead_zone ) y -= dead_zone;
    else if ( y < -dead_zone ) y += dead_zone;
    else y = 0;

    // store the values
    pjoy->x = x / ( float )( 0x8000 - dead_zone );
    pjoy->y = y / ( float )( 0x8000 - dead_zone );

    // get buttons
    button_count = SDL_JoystickNumButtons( pjoy->sdl_ptr );
    button_count = std::min( JOYBUTTON, button_count );
    for ( i = 0; i < button_count; i++ )
    {
        pjoy->button[i] = SDL_JoystickGetButton( pjoy->sdl_ptr, i );
    }

    // buttonmask mask
    pjoy->b = 0;
    for ( i = 0; i < button_count; i++ )
    {
        SET_BIT( pjoy->b, pjoy->button[i] << i );
    }

    return;
}

//--------------------------------------------------------------------------------------------
void input_read_joysticks()
{
    int cnt;

    SDL_JoystickUpdate();
    for ( cnt = 0; cnt < MAX_JOYSTICK; cnt++ )
    {
        input_read_joystick( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool input_handle_chat( SDL_Event * pevt )
{
    Uint32 kmod;
    bool is_alt, is_shift;

    if ( NULL == pevt || SDL_KEYDOWN != pevt->type ) return false;

    kmod = SDL_GetModState();

    is_alt   = HAS_SOME_BITS( kmod, KMOD_ALT | KMOD_CTRL );
    is_shift = HAS_SOME_BITS( kmod, KMOD_SHIFT );

    if ( is_alt ) return false;

    if ( SDLK_RETURN == pevt->key.keysym.sym || SDLK_KP_ENTER == pevt->key.keysym.sym )
    {
        net_chat.buffer[net_chat.buffer_count] = CSTR_END;
        keyb.chat_mode = false;
        keyb.chat_done = true;
        SDL_EnableKeyRepeat( 0, SDL_DEFAULT_REPEAT_DELAY );
    }
    else if ( SDLK_ESCAPE == pevt->key.keysym.sym )
    {
        // reset the keyboard buffer
        keyb.chat_mode = false;
        keyb.chat_done = false;
        net_chat.buffer_count = 0;
        net_chat.buffer[0] = CSTR_END;
        SDL_EnableKeyRepeat( 0, SDL_DEFAULT_REPEAT_DELAY );
    }
    else if ( SDLK_BACKSPACE == pevt->key.keysym.sym )
    {
        if ( net_chat.buffer_count > 0 )
        {
            net_chat.buffer_count--;
        }
        net_chat.buffer[net_chat.buffer_count] = CSTR_END;
    }
    else if ( net_chat.buffer_count < CHAT_BUFFER_SIZE )
    {
        if ( is_shift )
        {
            if (( unsigned )scancode_to_ascii_shift[pevt->key.keysym.sym] < 0xFF )
            {
                net_chat.buffer[net_chat.buffer_count++] = ( char )scancode_to_ascii_shift[pevt->key.keysym.sym];
            }
        }
        else
        {
            if (( unsigned )scancode_to_ascii[pevt->key.keysym.sym] < 0xFF )
            {
                net_chat.buffer[net_chat.buffer_count++] = ( char )scancode_to_ascii[pevt->key.keysym.sym];
            }
        }
        net_chat.buffer[net_chat.buffer_count] = CSTR_END;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool input_handle_SDL_KEYDOWN( SDL_Event * pevt )
{
    bool handled = false;

    if ( NULL == pevt || SDL_KEYDOWN != pevt->type ) return false;

    if ( keyb.chat_mode )
    {
        handled = input_handle_chat( pevt );
    }
    else
    {
        if ( SDLK_ESCAPE == pevt->key.keysym.sym )
        {
            // tell the main process about the escape request
            EProc->escape_requested = true;
            handled = true;
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool input_handle_SDL_Event( SDL_Event * pevt )
{
    bool handled = false;

    if ( NULL == pevt ) return false;

    handled = true;
    switch ( pevt->type )
    {
        case SDL_ACTIVEEVENT:
            // the application has gained or lost some form of focus
            if ( SDL_APPACTIVE == pevt->active.type && 1 == pevt->active.gain )
            {
                // the application has recovered from being minimized
                // the textures need to be reloaded into OpenGL memory

                gfx_system_reload_all_textures();
            }
            else if ( SDL_APPMOUSEFOCUS == pevt->active.type )
            {
                if ( 1 == pevt->active.gain )
                {
                    // gained mouse focus
                    mous.on = true;
                }
                else
                {
                    // lost mouse focus
                    mous.on = false;
                }
            }
            else if ( SDL_APPINPUTFOCUS == pevt->active.type )
            {
                if ( 1 == pevt->active.gain )
                {
                    // gained mouse focus
                    keyb.on = true;
                }
                else
                {
                    // lost mouse focus
                    keyb.on = false;
                }
            }
            break;

        case SDL_VIDEORESIZE:
            if ( SDL_VIDEORESIZE == pevt->resize.type )
            {
                // The video has been resized.
                // If the game is active, some camera info mught need to be recalculated
                // and possibly the auto-formatting for the menu system and the ui system
                // The ui will handle its own issues.

                // grab all the new SDL screen info
                SDLX_Get_Screen_Info( &sdl_scr, SDL_FALSE );
            }
            break;

        case SDL_VIDEOEXPOSE:
            // something has been done to the screen and it needs to be re-drawn.
            // For instance, a window above the app window was moved. This has no
            // effect on the game at the moment.
            break;

        case SDL_MOUSEBUTTONDOWN:
            if ( pevt->button.button == SDL_BUTTON_WHEELUP )
            {
                input_cursor.z++;
                input_cursor.wheel_event = true;
            }
            else if ( pevt->button.button == SDL_BUTTON_WHEELDOWN )
            {
                input_cursor.z--;
                input_cursor.wheel_event = true;
            }
            else
            {
                input_cursor.pending_click = true;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            input_cursor.pending_click = false;
            break;

        case SDL_MOUSEMOTION:
            input_cursor.x = pevt->motion.x;
            input_cursor.y = pevt->motion.y;
            break;

        case SDL_QUIT:
            //Someone pressed the little X in the corner while running windowed mode
            exit( 0 );
            break;

            // use this loop to grab any console-mode entry from the keyboard
        case SDL_KEYDOWN:
            input_handle_SDL_KEYDOWN( pevt );
            break;

        default:
            handled = true;
            break;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
void input_read_all_devices()
{
    /// @author ZZ
    /// @details This function gets all the current player input states

    SDL_Event evt;

    if ( EMPTY_BIT_FIELD == SDL_WasInit( SDL_INIT_EVERYTHING ) ) return;

    // Run through SDL's event loop to get info in the way that we want
    // it for the Gui code
    while ( SDL_PollEvent( &evt ) )
    {
        if ( cfg.dev_mode )
        {
            if ( NULL == egolib_console_handle_events( &evt ) )
            {
                continue;
            }
        }

        ui_handle_SDL_Event( &evt );

        input_handle_SDL_Event( &evt );
    }

    // Get immediate mode state for the rest of the game
    input_read_keyboard();
    input_read_mouse();
    input_read_joysticks();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void input_cursor_reset()
{
    input_cursor.pressed       = false;
    input_cursor.clicked       = false;
    input_cursor.pending_click = false;
    input_cursor.wheel_event   = false;
    input_cursor.z             = 0;
}

//--------------------------------------------------------------------------------------------
void input_cursor_finish_wheel_event()
{
    input_cursor.wheel_event   = false;
    input_cursor.z             = 0;
}

//--------------------------------------------------------------------------------------------
bool input_cursor_wheel_event_pending()
{
    if ( input_cursor.wheel_event && 0 == input_cursor.z )
    {
        input_cursor.wheel_event = false;
    }

    return input_cursor.wheel_event;
}

//--------------------------------------------------------------------------------------------
// implementation of specialized input_device_commands
//--------------------------------------------------------------------------------------------
BIT_FIELD input_device_get_buttonmask( input_device_t *pdevice )
{
    // assume the worst
    BIT_FIELD buttonmask = EMPTY_BIT_FIELD;

    // make sure the idevice is valid
    if ( NULL == pdevice ) return EMPTY_BIT_FIELD;

    // scan for devices that use buttons
    if ( INPUT_DEVICE_MOUSE == pdevice->device_type )
    {
        buttonmask = mous.b;
    }
    else if ( IS_VALID_JOYSTICK( pdevice->device_type ) )
    {
        int ijoy = pdevice->device_type - INPUT_DEVICE_JOY;

        buttonmask = joy_lst[ijoy].b;
    }

    return buttonmask;
}

//--------------------------------------------------------------------------------------------
bool input_device_is_enabled(input_device_t *self)
{
    // assume the worst
    bool retval = false;

    // make sure the idevice is valid
    if (!self) return false;

    if ( INPUT_DEVICE_KEYBOARD == self->device_type )
    {
        retval = TO_C_BOOL( keyb.on );
    }
    else if ( INPUT_DEVICE_MOUSE == self->device_type )
    {
        retval = TO_C_BOOL( mous.on );
    }
    else if ( IS_VALID_JOYSTICK( self->device_type ) )
    {
        int ijoy = self->device_type - INPUT_DEVICE_JOY;

        retval = TO_C_BOOL( joy_lst[ijoy].on );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool input_device_control_active( input_device_t *pdevice, CONTROL_BUTTON icontrol )
{
    /// @author ZZ
    /// @details This function returns true if the given icontrol is pressed...

	// make sure the idevice is valid
    if ( NULL == pdevice ) return false;
    const control_t &pcontrol = pdevice->keyMap[icontrol];

    // if no control information was loaded, it can't be pressed
    if ( !pcontrol.loaded ) return false;

    // test for bits
    if ( pcontrol.tag_bits.any() )
    {
        BIT_FIELD bmask = input_device_get_buttonmask( pdevice );

        if ( !HAS_ALL_BITS( bmask, pcontrol.tag_bits.to_ulong() ) )
        {
            return false;
        }
    }

    // how many tags does this control have?
    for(uint32_t keycode : pcontrol.mappedKeys)
    {
        if ( !SDL_KEYDOWN( keyb, keycode ) )
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mouse_data_t * mouse_data__init( mouse_data_t * ptr )
{
    int cnt;

    if ( NULL == ptr ) return ptr;

    ptr->on = true;
    ptr->sense = 12;
    ptr->x = ptr->y = 0.0f;
    ptr->b = 0;

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        ptr->button[cnt] = 0;
    }

    return ptr;

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
keyboard_data_t * keyboard_data__init( keyboard_data_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    // defaults
    ptr->on         = false;
    ptr->chat_mode  = false;
    ptr->chat_done  = false;
    ptr->state_size = 0;
    ptr->state_ptr  = NULL;

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
joystick_data_t * joystick_data__init( joystick_data_t * ptr )
{
    int cnt;

    if ( NULL == ptr ) return NULL;

    ptr->on = false;
    ptr->x = ptr->y = 0.0f;
    ptr->b = 0;
    ptr->sdl_ptr = NULL;

    for ( cnt = 0; cnt < JOYBUTTON; cnt++ )
    {
        ptr->button[cnt] = 0;
    }

    return ptr;
}
