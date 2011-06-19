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
/// @brief Routines for reading and writing version 2 of "controls.txt"
/// @details

#include "controls_file.h"
#include "controls_file-v2.h"

#include "scancode_file.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_vfs.h"

//--------------------------------------------------------------------------------------------
bool_t input_settings_load_vfs_2( const char* szFilename )
{
    /// @details ZZ@> This function reads the controls.txt file, version 2

    char currenttag[TAGSIZE] = EMPTY_CSTR;
    int idevice, icontrol;
    input_device_t * pdevice;
    vfs_FILE* fileread = NULL;

    // clear out all existing control data
    for ( idevice = 0; idevice < MAX_LOCAL_PLAYERS; idevice++ )
    {
        // clear the input control
        input_device_ctor( InputDevices.lst + idevice );
    }
    InputDevices.count = 0;

    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread ) return bfalse;

    // Read input for each player
    for ( idevice = 0; idevice < MAX_LOCAL_PLAYERS; idevice++ )
    {
        size_t count;
        int type;

        pdevice = InputDevices.lst + idevice;

        // figure out how we move
        if ( !vfs_get_next_string( fileread, currenttag, SDL_arraysize( currenttag ) ) )
        {
            continue;
        }

        // get the input device type from the tag
        type = translate_string_to_input_type( currenttag );

        // set the device type based on the control name
        input_device_init( pdevice, type );

        //Find out how many fields we are to read
        if ( INPUT_DEVICE_KEYBOARD == pdevice->device_type ) count = CONTROL_COMMAND_COUNT;
        else                                                 count = CONTROL_CAMERA + 1;

        //Read each input control button
        for ( icontrol = CONTROL_BEGIN; icontrol < count; icontrol++ )
        {
            if ( vfs_get_next_string( fileread, currenttag, SDL_arraysize( currenttag ) ) )
            {
                pdevice->control[icontrol].loaded = btrue;
                pdevice->control[icontrol].tag    = scantag_get_value( currenttag );
                pdevice->control[icontrol].is_key = ( 'K' == currenttag[0] );
            }
        }

        InputDevices.count++;
    }

    return InputDevices.count > 0;
}

//--------------------------------------------------------------------------------------------
bool_t input_settings_save_vfs_2( const char* szFilename )
{
    /// @details ZF@> This function saves all current game settings to "controls.txt"

    input_device_t * pdevice;
    vfs_FILE* filewrite;
    STRING write;
    Uint32 i;

    filewrite = vfs_openWrite( szFilename );
    if ( NULL == filewrite )
    {
        log_warning( "Could not save input settings (%s)!\n", szFilename );
        return bfalse;
    }

    //Add version number
    fput_version( filewrite, 2 );

    // Just some information
    vfs_puts( "Controls\n", filewrite );
    vfs_puts( "========\n", filewrite );
    vfs_puts( "This file lets users modify the handling of input devices.\n", filewrite );
    vfs_puts( "See the game manual for a list of settings and more info.\n", filewrite );
    vfs_puts( "Note that you can mix KEY_ type settings with other \n", filewrite );
    vfs_puts( "devices... Write the input after the colons!\n\n", filewrite );

    vfs_puts( "General Controls\n", filewrite );
    vfs_puts( "========\n", filewrite );
    vfs_puts( "These are general control codes and cannot be changed\n", filewrite );
    vfs_puts( "ESC                       - Open ingame menu\n", filewrite );
    vfs_puts( "SPACE                     - Respawn character (if dead and possible)\n", filewrite );
    vfs_puts( "1 to 7                    - Show character detailed stats\n", filewrite );
    vfs_puts( "LEFT SHIFT   + 1 to 8     - Show selected character armor without magic enchants\n", filewrite );
    vfs_puts( "LEFT CONTROL + 1 to 8     - Show armor stats with magic enchants included\n", filewrite );
    vfs_puts( "LEFT ALT     + 1 to 8     - Show character magic enchants\n", filewrite );
    vfs_puts( "F11                       - Take screenshot\n", filewrite );
    vfs_puts( "\n", filewrite );

    // The actual settings
    for ( i = 0; i < MAX_LOCAL_PLAYERS; i++ )
    {
        pdevice = InputDevices.lst + i;
        snprintf( write, SDL_arraysize( write ), "\nPLAYER %i\n", i + 1 );

        //which player
        vfs_puts( write, filewrite );
        vfs_puts( "========\n", filewrite );

        //controller type
        snprintf( write, SDL_arraysize( write ), "CONTROLLER:         %s\n", translate_input_type_to_string( pdevice->device_type ) );
        vfs_puts( write, filewrite );

        //Default input InputDevices
        export_control( filewrite, "Jump                 ", pdevice->device_type, pdevice->control + CONTROL_JUMP );
        export_control( filewrite, "Left Hand Use        ", pdevice->device_type, pdevice->control + CONTROL_LEFT_USE );
        export_control( filewrite, "Left Hand Get/Drop   ", pdevice->device_type, pdevice->control + CONTROL_LEFT_GET );
        export_control( filewrite, "Left Hand Inventory  ", pdevice->device_type, pdevice->control + CONTROL_LEFT_PACK );
        export_control( filewrite, "Right Hand Use       ", pdevice->device_type, pdevice->control + CONTROL_RIGHT_USE );
        export_control( filewrite, "Right Hand Get/Drop  ", pdevice->device_type, pdevice->control + CONTROL_RIGHT_GET );
        export_control( filewrite, "Right Hand Inventory ", pdevice->device_type, pdevice->control + CONTROL_RIGHT_PACK );
        export_control( filewrite, "Sneak                ", pdevice->device_type, pdevice->control + CONTROL_SNEAK );

        //this is only needed for keyboard
        if ( INPUT_DEVICE_KEYBOARD == pdevice->device_type )
        {
            //Could be a global key?
            export_control( filewrite, "Send Message", pdevice->device_type, pdevice->control + CONTROL_MESSAGE );

            export_control( filewrite, "Camera Rotate Left  ", pdevice->device_type, pdevice->control + CONTROL_CAMERA_LEFT );
            export_control( filewrite, "Camera Rotate Right ", pdevice->device_type, pdevice->control + CONTROL_CAMERA_RIGHT );
            export_control( filewrite, "Camera Zoom In      ", pdevice->device_type, pdevice->control + CONTROL_CAMERA_IN );
            export_control( filewrite, "Camera Zoom Out     ", pdevice->device_type, pdevice->control + CONTROL_CAMERA_OUT );

            export_control( filewrite, "Up                  ", pdevice->device_type, pdevice->control + CONTROL_UP );
            export_control( filewrite, "Down                ", pdevice->device_type, pdevice->control + CONTROL_DOWN );
            export_control( filewrite, "Left                ", pdevice->device_type, pdevice->control + CONTROL_LEFT );
            export_control( filewrite, "Right               ", pdevice->device_type, pdevice->control + CONTROL_RIGHT );
        }

        //Mouse and Joystick specific
        else
        {
            export_control( filewrite, "Camera Control Mode	 ", pdevice->device_type, pdevice->control + CONTROL_CAMERA );
        }
    }

    // All done
    vfs_close( filewrite );

    return btrue;
}
