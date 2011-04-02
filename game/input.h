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
#define MAX_LOCAL_PLAYERS    4

/// Which input control
/// @details Used by the controls[] structure and the control_is_pressed() function to query the state of various controls.
enum e_input_device
{
    INPUT_DEVICE_NONE = -1,
    INPUT_DEVICE_KEYBOARD = 0,
    INPUT_DEVICE_MOUSE,
    INPUT_DEVICE_JOY_A,
    INPUT_DEVICE_JOY_B,

    // aliases
    INPUT_DEVICE_BEGIN = INPUT_DEVICE_KEYBOARD,
    INPUT_DEVICE_END   = INPUT_DEVICE_JOY_B,

};



//--------------------------------------------------------------------------------------------
/// All the possible game actions that can be triggered from an input device
enum e_input_controls
{
    CONTROL_JUMP = 0,
    CONTROL_LEFT_USE,
    CONTROL_LEFT_GET,
    CONTROL_RIGHT_USE,
    CONTROL_RIGHT_GET,

    CONTROL_SNEAK,
    CONTROL_INVENTORY,

    CONTROL_MESSAGE,
    CONTROL_CAMERA_LEFT,
    CONTROL_CAMERA_RIGHT,
    CONTROL_CAMERA_IN,
    CONTROL_CAMERA_OUT,

    CONTROL_UP,
    CONTROL_DOWN,
    CONTROL_LEFT,
    CONTROL_RIGHT,
    CONTROL_COMMAND_COUNT,

    // Aliases
    CONTROL_CAMERA = CONTROL_MESSAGE,
    CONTROL_BEGIN  = CONTROL_JUMP,
    CONTROL_END    = CONTROL_RIGHT
};
typedef enum e_input_controls CONTROL_BUTTON;


//--------------------------------------------------------------------------------------------
/// the basic definition of a single control
struct s_control
{
    Uint32 tag;
    bool_t is_key;
};
typedef struct s_control control_t;


//--------------------------------------------------------------------------------------------
/// The mapping between the inputs detected by SDL and the device's in-game function
struct s_input_device
{
    float                   sustain;                            ///< Falloff rate for old movement
    float                   cover;                              ///< For falloff

    latch_t                 latch;
    latch_t                 latch_old;                          ///< For sustain

    INPUT_DEVICE            device_type;                        ///< Device type - mouse, keyboard, etc.
    control_t               control[CONTROL_COMMAND_COUNT];     ///< Key mappings
};
typedef struct s_input_device input_device_t;

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

extern input_device_t controls[MAX_LOCAL_PLAYERS];      //up to 4 local players (input controllers)
extern device_joystick_t joy[MAXJOYSTICK];

//--------------------------------------------------------------------------------------------
// Function prototypes

void   input_init();
void   input_read();

BIT_FIELD input_get_buttonmask( input_device_t *pdevice );
void input_device_init( input_device_t * pdevice );
bool_t input_is_enabled( input_device_t *pdevice );

bool_t control_is_pressed( input_device_t *pdevice, CONTROL_BUTTON icontrol );

void   cursor_reset();
void   cursor_finish_wheel_event();
bool_t cursor_wheel_event_pending();

INPUT_DEVICE    translate_string_to_input_type( const char *string );
const char*     translate_input_type_to_string( const INPUT_DEVICE type );
