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

/// @file file_formats/controls_file.c
/// @brief Routines for reading and writing the file "controls.txt" and "scancode.txt"
/// @details

#include "../egolib/input_device.h"

#include "../egolib/log.h"

#include "../egolib/strutil.h"
#include "../egolib/platform.h"

#include "../egolib/_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int translate_string_to_input_type( const char *string )
{
    /// @details ZF@> This function turns a string into a input type (mouse, keyboard, joystick, etc.)

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
    /// @details ZF@> This function turns a input type into a string

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
// input_device_t
//--------------------------------------------------------------------------------------------
input_device_t * input_device_ctor( input_device_t * pdevice )
{
    if ( NULL == pdevice ) return NULL;

    // clear out all the data, including all
    // control data
    BLANK_STRUCT_PTR( pdevice )

    pdevice->device_type = INPUT_DEVICE_UNKNOWN;

    return pdevice;
}

//--------------------------------------------------------------------------------------------
void input_device_init( input_device_t * pdevice, int req_type )
{
    int type;

    if ( NULL == pdevice ) return;

    // save the old type
    if ( INPUT_DEVICE_UNKNOWN == req_type )
    {
        type = pdevice->device_type;
    }
    else
    {
        type = req_type;
    }

    // clear out all the data
    BLANK_STRUCT_PTR( pdevice )

    // set everything that is not 0, bfalse, 0.0f, etc.
    pdevice->sustain     = 0.58f;
    pdevice->cover       = 1.0f - pdevice->sustain;
    pdevice->device_type = type;
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
        float scale = 1.0f / SQRT( dist );

        pdevice->latch.x *= scale;
        pdevice->latch.y *= scale;
    }
}
