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

/// @file file_formats/controls_file.h
/// @details routines for reading and writing the file controls.txt and "scancode.txt"

#include "egoboo_typedef.h"

#include "input.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/*
/// All the possible game actions that be assiciated with the keyboard
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

/// All the possible game actions that be assiciated with the mouse
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

/// All the possible game actions that be assiciated a joystick
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
*/
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    bool_t input_settings_load_vfs( const char *szFilename );
    bool_t input_settings_save_vfs( const char* szFilename );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _controls_file_h