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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - input.c
 * Keyboard, mouse, and joystick handling code.
 */

#include "egoboo.h"
#include "ui.h"
#include "log.h"

//--------------------------------------------------------------------------------------------
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


//--------------------------------------------------------------------------------------------
void input_init()
{
    // BB > initialize the inputs

    int i;

    log_info( "Intializing SDL Joystick... " );
    if ( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) < 0 )
    {
        log_message( "Failed!\n" );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

    // init the keyboard info
    init_scancodes();
    keyb.on        = btrue;
    keyb.count     = 0;
    keyb.state_ptr = NULL;

    // init the mouse info
    memset( &mous, 0, sizeof(mouse_t) );
    mous.on      = btrue;
    mous.sense   = 6;
    mous.sustain = 0.50f;
    mous.cover   = 0.50f;

    // init the joystick info
    for (i = 0; i < MAXJOYSTICK; i++)
    {
        memset( joy + i, 0, sizeof(device_joystick_t) );

        if (i < SDL_NumJoysticks() )
        {
            joy[i].sdl_ptr = SDL_JoystickOpen( i );
            joy[i].on      = (NULL != joy[i].sdl_ptr);
        }
    }
};

//--------------------------------------------------------------------------------------------
void input_read_mouse()
{
    int x, y, b;

    if ( menuactive )
        b = SDL_GetMouseState( &x, &y );
    else
        b = SDL_GetRelativeMouseState( &x, &y );

    mous.x = x; // mous.x and mous.y are the wrong type to use in above call
    mous.y = y;
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
    keyb.state_ptr = SDL_GetKeyState( &keyb.count );
}

//--------------------------------------------------------------------------------------------
void input_read_joystick(int which)
{
    int dead_zone = 0x8000 / 10;
    int i, button_count, x, y;
    device_joystick_t * pjoy;

    if ( which + INPUT_JOY > input_device_count ) return;
    if ( !joy[which].on ) return;

    pjoy = joy + which;

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
    pjoy->x = x / (float)(0x8000 - dead_zone);
    pjoy->y = y / (float)(0x8000 - dead_zone);

    // get buttons
    button_count = SDL_JoystickNumButtons( pjoy->sdl_ptr );
    button_count = MIN(JOYBUTTON, button_count);
    for ( i = 0; i < button_count; i++ )
    {
        pjoy->button[i] = SDL_JoystickGetButton( pjoy->sdl_ptr, i );
    }

    // buttonmask mask
    pjoy->b = 0;
    for ( i = 0; i < button_count; i++ )
    {
        pjoy->b |= ( pjoy->button[i] << i );
    }

    return;
}

//--------------------------------------------------------------------------------------------
void input_read()
{
    // ZZ> This function gets all the current player input states
    int cnt;
    SDL_Event evt;

    // Run through SDL's event loop to get info in the way that we want
    // it for the Gui code
    while ( SDL_PollEvent( &evt ) )
    {
        ui_handleSDLEvent( &evt );

        switch ( evt.type )
        {
            case SDL_MOUSEBUTTONDOWN:
                if (evt.button.button == SDL_BUTTON_WHEELUP)
                {
                    mous.z++;
                    mouse_wheel_event = btrue;
                }
                else if (evt.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    mous.z--;
                    mouse_wheel_event = btrue;
                }
                else
                {
                    pending_click = btrue;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                pending_click = bfalse;
                break;

                // use this loop to grab any console-mode entry from the keyboard
            case SDL_KEYDOWN:
                {
                    Uint32 kmod;
                    bool_t is_alt, is_shift;

                    kmod = SDL_GetModState();

                    is_alt   = ( 0 != (kmod & (KMOD_ALT | KMOD_CTRL) ) );
                    is_shift = ( 0 != (kmod & KMOD_SHIFT) );

                    if ( console_mode && !is_alt )
                    {
                        if ( SDLK_RETURN == evt.key.keysym.sym || SDLK_KP_ENTER == evt.key.keysym.sym )
                        {
                            keyb.buffer[keyb.buffer_count] = '\0';
                            console_mode = bfalse;
                            console_done = btrue;
                            SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_DELAY);
                        }
                        else if ( SDLK_ESCAPE == evt.key.keysym.sym )
                        {
                            // reset the keyboard buffer
                            console_mode = bfalse;
                            console_done = bfalse;
                            keyb.buffer_count = 0;
                            keyb.buffer[0] = '\0';
                            SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_DELAY);
                        }
                        else if ( SDLK_BACKSPACE == evt.key.keysym.sym )
                        {
                            if (keyb.buffer_count > 0)
                            {
                                keyb.buffer_count--;
                            }
                            keyb.buffer[keyb.buffer_count] = '\0';
                        }
                        else if ( keyb.buffer_count < KEYB_BUFFER_SIZE )
                        {
                            if ( is_shift )
                            {
                                keyb.buffer[keyb.buffer_count++] = scancode_to_ascii_shift[evt.key.keysym.sym];
                            }
                            else
                            {
                                keyb.buffer[keyb.buffer_count++] = scancode_to_ascii[evt.key.keysym.sym];
                            }
                            keyb.buffer[keyb.buffer_count] = '\0';
                        }
                    }
                }
                break;
        }
    }

    // Get immediate mode state for the rest of the game
    input_read_keyboard();
    input_read_mouse();

    SDL_JoystickUpdate();
    for ( cnt = 0; cnt < MAXJOYSTICK; cnt++ )
    {
        input_read_joystick(cnt);
    }
}


//--------------------------------------------------------------------------------------------
Uint32 input_get_buttonmask( Uint32 idevice )
{
    Uint32 buttonmask = 0;
    Uint32 which_device;

    // make sure the idevice is valid
    if ( idevice > input_device_count || idevice > INPUT_COUNT + MAXJOYSTICK ) return 0;
    which_device = controls[idevice].device;

    if ( which_device >= INPUT_JOY )
    {
        // joysticks
        buttonmask = joy[which_device - INPUT_JOY].b;
    }
    else
    {
        switch ( controls[idevice].device )
        {
            case INPUT_KEYBOARD: buttonmask = 0; break;
            case INPUT_MOUSE:    buttonmask = mous.b; break;
        }
    }

    return buttonmask;
};
