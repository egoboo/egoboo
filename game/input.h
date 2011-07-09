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

#include "network.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_control;
typedef struct s_control control_t;

struct s_input_device;
typedef struct s_input_device input_device_t;

struct s_device_list;
typedef struct s_device_list device_list_t;

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

#define MAX_JOYSTICK         MAX_LOCAL_PLAYERS     ///< the maximum number of supported joysticks (up to one for each local player)

/// Which input control
/// @details Used by the InputDevices.lst[] structure and the input_device_control_active() function to query the state of various controls.
enum e_input_device
{
    INPUT_DEVICE_KEYBOARD = 0,
    INPUT_DEVICE_MOUSE,
    INPUT_DEVICE_JOY,
    INPUT_DEVICE_UNKNOWN = 0xFFFF,

    // aliases
    INPUT_DEVICE_BEGIN = INPUT_DEVICE_KEYBOARD,
    INPUT_DEVICE_END   = INPUT_DEVICE_MOUSE + MAX_JOYSTICK
};

// this typedef must be after the enum definition of gcc has a fit
typedef enum e_input_device INPUT_DEVICE;

#define IS_VALID_JOYSTICK(XX) ( ( (XX) >= INPUT_DEVICE_JOY ) && ( (XX) < INPUT_DEVICE_JOY + MAX_JOYSTICK ) )

//--------------------------------------------------------------------------------------------
/// All the possible game actions that can be triggered from an input device
enum e_input_controls
{
    CONTROL_JUMP = 0,

    CONTROL_LEFT_USE,
    CONTROL_LEFT_GET,
    CONTROL_LEFT_PACK,

    CONTROL_RIGHT_USE,
    CONTROL_RIGHT_GET,
    CONTROL_RIGHT_PACK,

    CONTROL_SNEAK,

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
    CONTROL_CAMERA    = CONTROL_MESSAGE,
    CONTROL_INVENTORY = CONTROL_LEFT_PACK,
    CONTROL_BEGIN     = CONTROL_JUMP,
    CONTROL_END       = CONTROL_RIGHT
};

// this typedef must be after the enum definition of gcc has a fit
typedef enum e_input_controls CONTROL_BUTTON;

//--------------------------------------------------------------------------------------------
/// the basic definition of a single control
struct s_control
{
    bool_t loaded;

    Uint32 tag;
    Uint32 tag_mods;
    bool_t is_key;
};

//--------------------------------------------------------------------------------------------
/// The mapping between the inputs detected by SDL and the device's in-game function
struct s_input_device
{
    float                   sustain;                            ///< Falloff rate for old movement
    float                   cover;                              ///< For falloff

    latch_t                 latch;
    latch_t                 latch_old;                          ///< For sustain

    int                     device_type;                        ///< Device type - mouse, keyboard, etc.
    control_t               control[CONTROL_COMMAND_COUNT];     ///< Key mappings
};

input_device_t * input_device_ctor( input_device_t * pdevice );
BIT_FIELD input_device_get_buttonmask( input_device_t *pdevice );
void      input_device_init( input_device_t * pdevice, int type );
bool_t    input_device_is_enabled( input_device_t *pdevice );
bool_t    input_device_control_active( input_device_t *pdevice, CONTROL_BUTTON icontrol );
void      input_device_add_latch( input_device_t * pdevice, float newx, float newy );

//--------------------------------------------------------------------------------------------
// GENERIC DEVICE LIST

struct s_device_list
{
    size_t         count;
    input_device_t lst[MAX_LOCAL_PLAYERS];      //up to 4 local players (input controllers)
};

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

//--------------------------------------------------------------------------------------------
// DEVICE DATA
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// MOUSE

/// The internal representation of the mouse data
struct s_mouse_data
{
    bool_t                  on;              ///< Is it alive?
    float                   sense;           ///< Sensitivity threshold

    Sint32                  x;               ///< Mouse X movement counter
    Sint32                  y;               ///< Mouse Y movement counter

    Uint8                   button[4];       ///< Mouse button states
    Uint32                  b;               ///< Button masks
};

#define MOUSE_INIT \
    {\
        bfalse,    /* on */         \
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
    bool_t  on;                        ///< Is the keyboard alive?

    bool_t  chat_mode;                 ///< Input text from keyboard?
    bool_t  chat_done;                 ///< Input text from keyboard finished?

    int     state_size;
    Uint8  *state_ptr;
};

#define KEYBOARD_INIT \
    {\
        bfalse,   /* on */           \
        bfalse,   /* chat_mode */    \
        bfalse,   /* chat_done */    \
        0,        /* state_size */   \
        NULL,     /* state_ptr */    \
    }

#define SDLKEYDOWN(k) ( !keyb.chat_mode &&  (NULL != keyb.state_ptr) &&  ((k) < keyb.state_size) && ( 0 != keyb.state_ptr[k] ) )

//--------------------------------------------------------------------------------------------
// JOYSTICK
#define JOYBUTTON           32                      ///< Maximum number of joystick buttons

/// The internal representation of the joystick data
struct s_joystick_data
{
    bool_t  on;                ///< Is the holy joystick alive?

    float   x;
    float   y;
    Uint8   button[JOYBUTTON];
    Uint32  b;                 ///< Button masks

    SDL_Joystick * sdl_ptr;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern mouse_data_t    mous;
extern keyboard_data_t keyb;
extern joystick_data_t JoyList[MAX_JOYSTICK];

extern device_list_t     InputDevices;

extern input_cursor_t input_cursor;

//--------------------------------------------------------------------------------------------
// Function prototypes

void   input_system_init( void );
void   input_read_all_devices( void );

void      input_cursor_reset( void );
void      input_cursor_finish_wheel_event( void );
bool_t    input_cursor_wheel_event_pending( void );

int         translate_string_to_input_type( const char *string );
const char* translate_input_type_to_string( const int type );
