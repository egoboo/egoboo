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

#include "egolib/input_device.h"

#include "egolib/log.h"

#include "egolib/strutil.h"
#include "egolib/platform.h"

#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
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
        log_warning( "Unknown device controller parsed (%s) - defaulted to Keyboard\n", string );
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
input_device_t::input_device_t()
{
	// Clear out all the data, including all control data.
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
input_device_t * input_device_ctor( input_device_t * pdevice )
{
    if ( NULL == pdevice ) return NULL;

    // clear out all the data, including all
    // control data
    pdevice->sustain = 0.0f;
    pdevice->cover = 0.0f;

    pdevice->latch.clear();
    pdevice->latch_old.clear();

    pdevice->device_type = INPUT_DEVICE_UNKNOWN;

    for (control_t &control : pdevice->keyMap)
    {
        control.clear();
    }

    return pdevice;
}

//--------------------------------------------------------------------------------------------
void input_device_init(input_device_t *self, e_input_device req_type)
{
	e_input_device type;

    if (!self) return;

    // save the old type
    if (INPUT_DEVICE_UNKNOWN == req_type)
    {
        type = self->device_type;
    }
    else
    {
        type = req_type;
    }

    // set everything that is not 0, false, 0.0f, etc.
	self->latch.clear();
	self->latch_old.clear();
    self->sustain = 0.58f;
    self->cover = 1.0f - self->sustain;
    self->device_type = type;
	for (control_t &control : self->keyMap)
	{
		control.clear();
	}

}

//--------------------------------------------------------------------------------------------
void input_device_add_latch( input_device_t * pdevice, float newx, float newy )
{
    // Sustain old movements to ease mouse/keyboard play

    float dist;

    if ( NULL == pdevice ) return;

    pdevice->latch_old = pdevice->latch;

    pdevice->latch.x = pdevice->latch.x * pdevice->sustain + newx * pdevice->cover;
    pdevice->latch.y = pdevice->latch.y * pdevice->sustain + newy * pdevice->cover;

    // make sure that the latch never overflows
    dist = pdevice->latch.x * pdevice->latch.x + pdevice->latch.y * pdevice->latch.y;
    if ( dist > 1.0f )
    {
        float scale = 1.0f / std::sqrt( dist );

        pdevice->latch.x *= scale;
        pdevice->latch.y *= scale;
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
