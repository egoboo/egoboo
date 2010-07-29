#pragma once

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

/// @file input.h
/// @details Keyboard, mouse, and joystick handling code.

#include "egoboo_typedef.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXJOYSTICK          2     ///<the maximum number of supported joysticks

/// Which input control
/// @details Used by the controls[] structure and the control_is_pressed() function to query the state of various controls.
enum e_input_device
{
    INPUT_DEVICE_KEYBOARD = 0,
    INPUT_DEVICE_MOUSE,
    INPUT_DEVICE_JOY,

    // aliases
    INPUT_DEVICE_BEGIN = INPUT_DEVICE_KEYBOARD,
    INPUT_DEVICE_END   = INPUT_DEVICE_JOY,
    INPUT_DEVICE_JOY_A = INPUT_DEVICE_JOY + 0,
    INPUT_DEVICE_JOY_B = INPUT_DEVICE_JOY + 1
};
typedef enum  e_input_device INPUT_DEVICE;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// old user interface variables
struct s_cursor
{
    int     x;
    int     y;
    int     z;
    bool_t  pressed;
    bool_t  clicked;
    bool_t  pending_click;
    bool_t  wheel_event;
};
typedef struct s_cursor cursor_t;

extern cursor_t cursor;

//--------------------------------------------------------------------------------------------
// MOUSE

/// The internal representation of the mouse data
struct s_mouse
{
    bool_t                  on;              ///< Is it alive?
    float                   sense;           ///< Sensitivity threshold

    Sint32                  x;               ///< Mouse X movement counter
    Sint32                  y;               ///< Mouse Y movement counter

    Uint8                   button[4];       ///< Mouse button states
    Uint32                  b;               ///< Button masks

};
typedef struct s_mouse mouse_t;

extern mouse_t mous;

//--------------------------------------------------------------------------------------------
// KEYBOARD

#define KEYB_BUFFER_SIZE 2048

/// The internal representation of the keyboard data
struct s_keyboard
{
    bool_t  on;                ///< Is the keyboard alive?
    int     count;
    Uint8  *state_ptr;

    int     buffer_count;
    char    buffer[KEYB_BUFFER_SIZE];
};
typedef struct s_keyboard keyboard_t;

extern keyboard_t keyb;

#define SDLKEYDOWN(k) ( !console_mode &&  (NULL != keyb.state_ptr) &&  ((k) < keyb.count) && ( 0 != keyb.state_ptr[k] ) )

//--------------------------------------------------------------------------------------------
// JOYSTICK
#define JOYBUTTON           32                      ///< Maximum number of joystick buttons

/// The internal representation of the joystick data
struct s_device_joystick
{
    bool_t  on;                ///< Is the holy joystick alive?
    float   x;
    float   y;
    Uint8   button[JOYBUTTON];
    Uint32  b;                 ///< Button masks
    SDL_Joystick * sdl_ptr;
};
typedef struct s_device_joystick device_joystick_t;

extern device_joystick_t joy[MAXJOYSTICK];

//--------------------------------------------------------------------------------------------

/// The bits representing the possible input devices
enum e_input_bits
{
    INPUT_BITS_NONE      = 0,
    INPUT_BITS_MOUSE     = ( 1 << INPUT_DEVICE_MOUSE ),         ///< Input devices
    INPUT_BITS_KEYBOARD  = ( 1 << INPUT_DEVICE_KEYBOARD ),
    INPUT_BITS_JOYA      = ( 1 << ( INPUT_DEVICE_JOY_A ) ),
    INPUT_BITS_JOYB      = ( 1 << ( INPUT_DEVICE_JOY_B ) )
};

//--------------------------------------------------------------------------------------------
// Function prototypes

void   input_init();
void   input_read();

Uint32 input_get_buttonmask( Uint32 idevice );

bool_t control_is_pressed( Uint32 idevice, Uint8 icontrol );

void   cursor_reset();
void   cursor_finish_wheel_event();
bool_t cursor_wheel_event_pending();
