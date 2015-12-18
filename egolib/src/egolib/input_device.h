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

/// @file egolib/input_device.h
/// @details routines for reading and writing the file controls.txt and "scancode.txt"

#pragma once

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
/// The latch used by the input system
struct latch_t
{
public:
    latch_t() :
        x(0.0f),
        y(0.0f),
        b()
    {
        //ctor
    }

    void clear()
    {
        x = 0.0f;
        y = 0.0f;
        b.reset();
    }

public:
    float           x;         ///< the x input
    float           y;         ///< the y input
    std::bitset<32> b;         ///< the button bits
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#   define CURRENT_CONTROLS_FILE_VERSION 2

#   define IS_VALID_JOYSTICK(XX) ( ( (XX) >= INPUT_DEVICE_JOY ) && ( (XX) < INPUT_DEVICE_JOY + MAX_JOYSTICK ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

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

    // this typedef must be after the enum definition or gcc has a fit
    typedef enum e_input_device INPUT_DEVICE;

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

// this typedef must be after the enum definition or gcc has a fit
    typedef enum e_input_controls CONTROL_BUTTON;

#define MAXCONTROLKEYS 4

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the basic definition of a single control
struct control_t
{
public:
    control_t();

    void clear();

public:

    bool loaded;

    // device bits for non-keyboard entry
    std::bitset<32> tag_bits;

    // list of keystrokes mapped to this control
    std::forward_list<uint32_t> mappedKeys;
    uint32_t tag_key_mods;
};

//--------------------------------------------------------------------------------------------
/// The mapping between the hardware inputs and the in-game inputs

    struct input_device_t
    {
    public:
		input_device_t();

        void clear();

        /// @brief Reset this input device.
        /// @param type the new type of the input device.
        ///             If this is @a INPUT_DEVICE_UNKNOWN, then the original device type is retained.
        void initialize(e_input_device type);

    public:
        float sustain;                            ///< Falloff rate for old movement
        float cover;                              ///< For falloff

        latch_t latch;
        latch_t latch_old;                        ///< For sustain

		e_input_device device_type;               ///< Device type - mouse, keyboard, etc.
        std::array<control_t, CONTROL_COMMAND_COUNT> keyMap;        ///< Key mappings

    };

    void input_device_add_latch(input_device_t *self, float newx, float newy);

// special functions that must be implemented by the user
    extern BIT_FIELD input_device_get_buttonmask(input_device_t *self);
	/// @brief Get if this input device is enabled.
	extern bool input_device_is_enabled(input_device_t *self);
    extern bool input_device_control_active(input_device_t *self, CONTROL_BUTTON icontrol);

//--------------------------------------------------------------------------------------------

    struct device_list_t
    {
        device_list_t() :
            count(0),
            lst()
        {
            //ctor
        }

        size_t         count;
        std::array<input_device_t, MAX_LOCAL_PLAYERS> lst;      // up to MAX_LOCAL_PLAYERS input controllers
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern device_list_t     InputDevices;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    int         translate_string_to_input_type( const char *string );
    const char* translate_input_type_to_string( const int type );

    char        get_device_char_from_device_type( Uint32 device_type );
    Uint32      get_device_type_from_device_char( char tag_type );
