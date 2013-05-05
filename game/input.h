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

#include <SDL.h>

#include "egoboo_typedef.h"

#include "../egolib/input_device.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_control;
struct s_input_device;
struct s_device_list;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_cursor;
typedef struct s_cursor input_cursor_t;

struct s_mouse_data;
typedef struct s_mouse_data mouse_data_t;

struct s_keyboard_data;
typedef struct s_keyboard_data keyboard_data_t;

typedef struct s_joystick_data joystick_data_t;
struct s_joystick_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// old user interface variables
struct s_cursor
{
    int     x;
    int     y;
    int     z;
    ego_bool  pressed;
    ego_bool  clicked;
    ego_bool  pending_click;
    ego_bool  wheel_event;
};

//--------------------------------------------------------------------------------------------
// DEVICE DATA
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// MOUSE

/// The internal representation of the mouse data
struct s_mouse_data
{
    ego_bool                  on;              ///< Is it alive?
    float                   sense;           ///< Sensitivity threshold

    Sint32                  x;               ///< Mouse X movement counter
    Sint32                  y;               ///< Mouse Y movement counter

    Uint8                   button[4];       ///< Mouse button states
    Uint32                  b;               ///< Button masks
};

mouse_data_t * mouse_data__init( mouse_data_t * ptr );

#define MOUSE_INIT \
    {\
        ego_false,    /* on */         \
        0.9f,      /* sense */      \
        0,         /* x */          \
        0,         /* y */          \
        {0,0,0,0}, /* button[4] */  \
        0,         /* b */          \
    }

//--------------------------------------------------------------------------------------------
// KEYBOARD

/// The internal representation of the keyboard data
struct s_keyboard_data
{
    ego_bool  on;                        ///< Is the keyboard alive?

    ego_bool  chat_mode;                 ///< Input text from keyboard?
    ego_bool  chat_done;                 ///< Input text from keyboard finished?

    int     state_size;
    Uint8  *state_ptr;
};

#define KEYBOARD_INIT \
    {\
        ego_false,   /* on */           \
        ego_false,   /* chat_mode */    \
        ego_false,   /* chat_done */    \
        0,        /* state_size */   \
        NULL,     /* state_ptr */    \
    }

keyboard_data_t * keyboard_data__init( keyboard_data_t * ptr );

#define SDL_KEYDOWN(KEYB,k) ( !KEYB.chat_mode &&  (NULL != KEYB.state_ptr) &&  ((k) < KEYB.state_size) && ( 0 != KEYB.state_ptr[k] ) )

//--------------------------------------------------------------------------------------------
// JOYSTICK
#define JOYBUTTON           32                      ///< Maximum number of joystick buttons

/// The internal representation of the joystick data
struct s_joystick_data
{
    ego_bool  on;                ///< Is the holy joystick alive?

    float   x;
    float   y;
    Uint8   button[JOYBUTTON];
    Uint32  b;                 ///< Button masks

    SDL_Joystick * sdl_ptr;
};

joystick_data_t * joystick_data__init( joystick_data_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern mouse_data_t    mous;
extern keyboard_data_t keyb;
extern joystick_data_t joy_lst[MAX_JOYSTICK];

extern input_cursor_t input_cursor;

//--------------------------------------------------------------------------------------------
// Function prototypes

void   input_system_init( void );
void   input_read_all_devices( void );

void      input_cursor_reset( void );
void      input_cursor_finish_wheel_event( void );
ego_bool    input_cursor_wheel_event_pending( void );
