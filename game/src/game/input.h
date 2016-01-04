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

/// @file game/input.h
/// @details Keyboard, mouse, and joystick handling code.

#pragma once

#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------

/// The internal representation of the mouse data
struct mouse_data_t
{
    bool    on;              ///< Is it alive?
    float   sense;           ///< Sensitivity threshold

    Sint32  x;               ///< Mouse X movement counter
    Sint32  y;               ///< Mouse Y movement counter

    Uint8   button[4];       ///< Mouse button states
    Uint32  b;               ///< Button masks

	mouse_data_t();
	void init();
};

//--------------------------------------------------------------------------------------------
// KEYBOARD

/// The internal representation of the keyboard data
struct keyboard_data_t
{
    bool  on;                        ///< Is the keyboard alive?

    bool  chat_mode;                 ///< Input text from keyboard?
    bool  chat_done;                 ///< Input text from keyboard finished?

    int     state_size;
    const Uint8 *state_ptr;

	keyboard_data_t();
	void init();
	bool is_key_down(int keycode) const;
};

//--------------------------------------------------------------------------------------------
// JOYSTICK
#define JOYBUTTON           32                      ///< Maximum number of joystick buttons

/// The internal representation of the joystick data
struct joystick_data_t
{
    bool  on;                ///< Is the holy joystick alive?

    float   x;
    float   y;
    Uint8   button[JOYBUTTON];
    Uint32  b;                 ///< Button masks

    SDL_Joystick * sdl_ptr;

	joystick_data_t();
	void init();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern mouse_data_t    mous;
extern keyboard_data_t keyb;
extern joystick_data_t joy_lst[MAX_JOYSTICK];

//--------------------------------------------------------------------------------------------
// Function prototypes

struct InputSystem {
	/// @details initialize the input system.
	static void initialize();
	static void uninitialize();
	static void read_mouse();
	static void read_keyboard();
	static void read_joysticks();
	static void read_joystick(int which);
protected:
	/// @details Initialize the devices.
	static void init_devices();
	/// @details Initialize the keyboard.
	static void init_keyboard();
	/// @details Initialize the mouse.
	static void init_mouse();
	/// @details Initialize the joysticks.
	static void init_joysticks();

};
