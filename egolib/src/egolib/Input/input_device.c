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

/// @file egolib/input_device.c
/// @brief Routines for reading and writing the file "controls.txt" and "scancode.txt"
/// @details

#include "egolib/Input/input_device.h"
#include "egolib/Log/_Include.hpp"
#include "egolib/strutil.h"

//--------------------------------------------------------------------------------------------
int translate_string_to_input_type( const char *string )
{
    /// @author ZF
    /// @details This function turns a string into a input type (mouse, keyboard, joystick, etc.)

    int retval = INPUT_DEVICE_UNKNOWN;

    if ( INVALID_CSTR( string ) ) return INPUT_DEVICE_UNKNOWN;

    if ( 0 == strcmp( string, "KEYBOARD" ) )
    {
        retval = INPUT_DEVICE_KEYBOARD;
    }
    else if ( 0 == strcmp( string, "MOUSE" ) )
    {
        retval = INPUT_DEVICE_MOUSE;
    }
    else if ( 0 == strncmp( string, "JOYSTICK", 8 ) && CSTR_END != string[9] )
    {
        int ijoy = ( int )string[9] - ( int )'A';

        if ( ijoy >= 0 && ijoy < MAX_JOYSTICK )
        {
            retval = INPUT_DEVICE_JOY + ijoy;
        }
    }

    // No matches
    if ( INPUT_DEVICE_UNKNOWN == retval )
    {
        retval = INPUT_DEVICE_KEYBOARD;
        Log::get().warn( "Unknown device controller parsed (%s) - defaulted to Keyboard\n", string );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
const char* translate_input_type_to_string( const int type )
{
    /// @author ZF
    /// @details This function turns a input type into a string

    static STRING retval;

    if ( type == INPUT_DEVICE_KEYBOARD )
    {
        strncpy( retval, "KEYBOARD", SDL_arraysize( retval ) );
    }
    else if ( type == INPUT_DEVICE_MOUSE )
    {
        strncpy( retval, "MOUSE", SDL_arraysize( retval ) );
    }
    else if ( IS_VALID_JOYSTICK( type ) )
    {
        snprintf( retval, SDL_arraysize( retval ), "JOYSTICK_%c", ( char )( 'A' + ( type - INPUT_DEVICE_JOY ) ) );
    }
    else
    {
        // No matches
        strncpy( retval, "UNKNOWN", SDL_arraysize( retval ) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// control_t
//--------------------------------------------------------------------------------------------
control_t::control_t() :
    loaded(false),
    tag_bits(),
    mappedKeys(),
    tag_key_mods(0)
{
    //ctor
}

void control_t::clear()
{
    loaded = false;
    tag_bits.reset();
    mappedKeys.clear();
    tag_key_mods = 0;
}


//--------------------------------------------------------------------------------------------
// input_device_t
//--------------------------------------------------------------------------------------------
input_device_t::input_device_t() :
    sustain(0.0f),
    cover(0.0f),
    latch(),
    latch_old(),
    device_type(INPUT_DEVICE_UNKNOWN),
    keyMap()
{
	// Clear out all the data, including all control data.
	latch.clear();
	latch_old.clear();

	for (control_t &control : keyMap)
	{
		control.clear();
	}
}

void input_device_t::clear()
{
    // clear out all the data, including all
    // control data
    sustain = 0.0f;
    cover = 0.0f;

    latch.clear();
    latch_old.clear();

    device_type = INPUT_DEVICE_UNKNOWN;

    for (control_t &control : keyMap)
    {
        control.clear();
    }
}

//--------------------------------------------------------------------------------------------
void input_device_t::initialize(e_input_device req_type)
{
	e_input_device type;

    // save the old type
    if (INPUT_DEVICE_UNKNOWN == req_type)
    {
        type = device_type;
    }
    else
    {
        type = req_type;
    }

    // set everything that is not 0, false, 0.0f, etc.
	latch.clear();
	latch_old.clear();
    sustain = 0.58f;
    cover = 1.0f - sustain;
    device_type = type;
	for (control_t &control : keyMap)
	{
		control.clear();
	}

}

//--------------------------------------------------------------------------------------------
void input_device_t::add_latch( input_device_t * pdevice, const Vector2f& newInput )
{
    float dist;

    if ( NULL == pdevice ) return;

    pdevice->latch_old = pdevice->latch;

    pdevice->latch.input = pdevice->latch.input * pdevice->sustain + newInput * pdevice->cover;

    // make sure that the latch never overflows
    dist = pdevice->latch.input.length_2();
    if (dist > 1.0f) {
        float scale = 1.0f / std::sqrt(dist);
        pdevice->latch.input *= scale;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char get_device_char_from_device_type( Uint32 device_type )
{
    char retval = '?';

    if ( INPUT_DEVICE_KEYBOARD == device_type )
    {
        retval = 'K';
    }
    else if ( INPUT_DEVICE_MOUSE == device_type )
    {
        retval = 'M';
    }
    else if ( device_type >= INPUT_DEVICE_JOY && device_type < INPUT_DEVICE_END )
    {
        retval = 'J';
    }
    else
    {
        retval = '?';
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
Uint32 get_device_type_from_device_char( char tag_type )
{
    Uint32 retval = INPUT_DEVICE_UNKNOWN;

    switch ( tag_type )
    {
        case 'K': retval = INPUT_DEVICE_KEYBOARD; break;
        case 'M': retval = INPUT_DEVICE_MOUSE;    break;
        case 'J': retval = INPUT_DEVICE_JOY;      break;

        default: retval = INPUT_DEVICE_UNKNOWN; break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------

static BIT_FIELD get_buttonmask(input_device_t *pdevice) {
    // assume the worst
    BIT_FIELD buttonmask = EMPTY_BIT_FIELD;

    // make sure the idevice is valid
    if (NULL == pdevice) return EMPTY_BIT_FIELD;

    // scan for devices that use buttons
    if (INPUT_DEVICE_MOUSE == pdevice->device_type) {
        buttonmask = InputSystem::get().mouse.b;
    } else if (IS_VALID_JOYSTICK(pdevice->device_type)) {
        int ijoy = pdevice->device_type - INPUT_DEVICE_JOY;

        buttonmask = InputSystem::get().joysticks[ijoy]->b;
    }

    return buttonmask;
}

bool input_device_t::is_enabled(input_device_t *self) {
    // assume the worst
    bool retval = false;

    // make sure the idevice is valid
    if (!self) return false;

    if (INPUT_DEVICE_KEYBOARD == self->device_type) {
        retval = InputSystem::get().keyboard.enabled
            &&   InputSystem::get().keyboard.getConnected();
    } else if (INPUT_DEVICE_MOUSE == self->device_type) {
        retval = InputSystem::get().mouse.enabled
            &&   InputSystem::get().mouse.getConnected();
    } else if (IS_VALID_JOYSTICK(self->device_type)) {
        int ijoy = self->device_type - INPUT_DEVICE_JOY;

        retval = InputSystem::get().joysticks[ijoy]->enabled
            &&   InputSystem::get().joysticks[ijoy]->getConnected();
    }

    return retval;
}

bool input_device_t::control_active(input_device_t *pdevice, CONTROL_BUTTON icontrol) {
    // make sure the idevice is valid
    if (NULL == pdevice) return false;
    const control_t &pcontrol = pdevice->keyMap[icontrol];

    // if no control information was loaded, it can't be pressed
    if (!pcontrol.loaded) return false;

    // test for bits
    if (pcontrol.tag_bits.any()) {
        BIT_FIELD bmask = get_buttonmask(pdevice);

        if (!HAS_ALL_BITS(bmask, pcontrol.tag_bits.to_ulong())) {
            return false;
        }
    }

    // how many tags does this control have?
    for (uint32_t keycode : pcontrol.mappedKeys) {
        if (!InputSystem::get().keyboard.isKeyDown(keycode)) {
            return false;
        }
    }

    return true;
}
