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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - input.c
 * Keyboard, mouse, and joystick handling code.
 */

#include "egoboo_typedef.h"

#include <SDL.h>

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
// Key/Control input defenitions
#define MAXTAG              128                     // Number of tags in scancode.txt
#define TAGSIZE             32                      // Size of each tag

struct s_scantag
{
    char   name[TAGSIZE];                      // Scancode names
    Sint32 value;                     // Scancode values
};
typedef struct s_scantag scantag_t;

extern int       scantag_count;
extern scantag_t scantag[MAXTAG];

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
#define MAXJOYSTICK          2
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
enum e_input_device
{
    INPUT_DEVICE_KEYBOARD = 0,
    INPUT_DEVICE_MOUSE,
    INPUT_DEVICE_JOY,
    INPUT_DEVICE_COUNT
};
typedef enum  e_input_device input_device_t;

enum e_input_bits
{
    INPUT_BITS_NONE      = 0,
    INPUT_BITS_MOUSE     = ( 1 << INPUT_DEVICE_MOUSE  ),        // Input devices
    INPUT_BITS_KEYBOARD  = ( 1 << INPUT_DEVICE_KEYBOARD     ),
    INPUT_BITS_JOYA      = ( 1 << (INPUT_DEVICE_JOY + 0) ),
    INPUT_BITS_JOYB      = ( 1 << (INPUT_DEVICE_JOY + 1) )
};

extern Uint32 input_device_count;

enum e_input_controls
{
    CONTROL_JUMP = 0,
    CONTROL_LEFT_USE,
    CONTROL_LEFT_GET,
    CONTROL_LEFT_PACK,
    CONTROL_RIGHT_USE,
    CONTROL_RIGHT_GET,
    CONTROL_RIGHT_PACK,
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

enum e_keyboard_controls
{
    KEY_JUMP = 0,
    KEY_LEFT_USE,
    KEY_LEFT_GET,
    KEY_LEFT_PACK,
    KEY_RIGHT_USE,
    KEY_RIGHT_GET,
    KEY_RIGHT_PACK,
    KEY_MESSAGE,
    KEY_CAMERA_LEFT,
    KEY_CAMERA_RIGHT,
    KEY_CAMERA_IN,
    KEY_CAMERA_OUT,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,

    // Aliases
    KEY_CONTROL_BEGIN = KEY_JUMP,
    KEY_CONTROL_END   = KEY_RIGHT
};

enum e_mouse_controls
{
    MOS_JUMP = 0,
    MOS_LEFT_USE,
    MOS_LEFT_GET,
    MOS_LEFT_PACK,
    MOS_RIGHT_USE,
    MOS_RIGHT_GET,
    MOS_RIGHT_PACK,
    MOS_CAMERA,

    // Aliases
    MOS_CONTROL_BEGIN = MOS_JUMP,
    MOS_CONTROL_END   = MOS_CAMERA
};

enum e_joystick_controls
{
    JOY_JUMP = 0,
    JOY_LEFT_USE,
    JOY_LEFT_GET,
    JOY_LEFT_PACK,
    JOY_RIGHT_USE,
    JOY_RIGHT_GET,
    JOY_RIGHT_PACK,
    JOY_CAMERA,

    // Aliases
    JOY_CONTROL_BEGIN = JOY_JUMP,
    JOY_CONTROL_END   = JOY_CAMERA
};

struct s_control
{
    Uint32 tag;
    bool_t is_key;
};
typedef struct s_control control_t;

struct s_device_controls
{
    size_t    count;
    Uint32    device;
    control_t control[CONTROL_COMMAND_COUNT];
};
typedef struct s_device_controls device_controls_t;

extern device_controls_t controls[INPUT_DEVICE_COUNT + MAXJOYSTICK];

//--------------------------------------------------------------------------------------------
// Function prototypes

void init_scancodes();

void   input_init();
void   input_read();

Uint32 input_get_buttonmask( Uint32 idevice );

void   scantag_read_all( const char *szFilename );
int    scantag_get_value( const char *string );
char*  scantag_get_string( Sint32 device, Sint32 tag, bool_t onlykeys );

bool_t control_is_pressed( Uint32 idevice, Uint8 icontrol );

void reset_players();
void cursor_reset();
