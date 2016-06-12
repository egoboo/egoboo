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

/// @file egolib/Input/input_device.h
/// @details routines for reading and writing the file controls.txt and "scancode.txt"

#pragma once

#include "egolib/Input/input.h"
#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
/// The latch used by the input system
struct latch_t {
public:
    latch_t()
        : input(Vector2f::zero()), b() {
    }

    void clear() {
        input = Vector2f();
        b.reset();
    }

public:
    /// The input.
    Vector2f input;
    /// The button bits.
    std::bitset<32> b;         ///< the button bits
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#   define IS_VALID_JOYSTICK(XX) ( ( (XX) >= INPUT_DEVICE_JOY ) && ( (XX) < INPUT_DEVICE_JOY + MAX_JOYSTICK ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

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
