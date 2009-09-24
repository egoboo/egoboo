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

/* Egoboo - input.c
 * Keyboard, mouse, and joystick handling code.
 */

#include "egoboo_typedef.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enum e_input_device
{
    INPUT_DEVICE_KEYBOARD = 0,
    INPUT_DEVICE_MOUSE,
    INPUT_DEVICE_JOY,
    INPUT_DEVICE_COUNT
};
typedef enum  e_input_device input_device_t;

#define MAXJOYSTICK          2     //the maximum number of supported joysticks

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// old user interface variables

extern int              cursor_x;              // Cursor position
extern int              cursor_y;
extern bool_t           cursor_pressed;
extern bool_t           cursor_clicked;
extern bool_t           cursor_pending_click;
extern bool_t           cursor_wheel_event;

//--------------------------------------------------------------------------------------------
// MOUSE
struct s_mouse
{
    bool_t                  on;              // Is the mouse alive?
    float                   sense;           // Sensitivity threshold
    float                   sustain;         // Falloff rate for old movement
    float                   cover;           // For falloff
    Sint32                  x;               // Mouse X movement counter
    Sint32                  y;               // Mouse Y movement counter
    Sint32                  z;               // Mouse wheel movement counter
    float                   latcholdx;       // For sustain
    float                   latcholdy;
    Uint8                   button[4];       // Mouse button states
    Uint32                  b;               // Button masks
};
typedef struct s_mouse mouse_t;

extern mouse_t mous;

//--------------------------------------------------------------------------------------------
// KEYBOARD
#define KEYB_BUFFER_SIZE 2048
struct s_keyboard
{
    bool_t  on;                // Is the keyboard alive?
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
#define JOYBUTTON           32                      // Maximum number of joystick buttons

struct s_device_joystick
{
    bool_t  on;                // Is the holy joystick alive?
    float   x;
    float   y;
    Uint8   button[JOYBUTTON];
    Uint32  b;                 // Button masks
    SDL_Joystick * sdl_ptr;
};
typedef struct s_device_joystick device_joystick_t;

extern device_joystick_t joy[MAXJOYSTICK];

//--------------------------------------------------------------------------------------------

enum e_input_bits
{
    INPUT_BITS_NONE      = 0,
    INPUT_BITS_MOUSE     = ( 1 << INPUT_DEVICE_MOUSE  ),        // Input devices
    INPUT_BITS_KEYBOARD  = ( 1 << INPUT_DEVICE_KEYBOARD     ),
    INPUT_BITS_JOYA      = ( 1 << (INPUT_DEVICE_JOY + 0) ),
    INPUT_BITS_JOYB      = ( 1 << (INPUT_DEVICE_JOY + 1) )
};

//--------------------------------------------------------------------------------------------
// Function prototypes

void   input_init();
void   input_read();

Uint32 input_get_buttonmask( Uint32 idevice );

bool_t control_is_pressed( Uint32 idevice, Uint8 icontrol );

void cursor_reset();
