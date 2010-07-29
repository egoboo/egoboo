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

#include "controls_file.h"

#include "scancode_file.h"
#include "log.h"

#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint32            input_device_count = 0;
device_controls_t controls[INPUT_DEVICE_END + MAXJOYSTICK];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void export_control( vfs_FILE * filewrite, const char * text, Sint32 device, control_t * pcontrol )
{
    STRING write;

    snprintf( write, SDL_arraysize( write ), "%s : %s\n", text, scantag_get_string( device, pcontrol->tag, pcontrol->is_key ) );
    vfs_puts( write, filewrite );
}

//--------------------------------------------------------------------------------------------
bool_t input_settings_load_vfs( const char *szFilename )
{
    /// @details ZZ@> This function reads the controls.txt file
    vfs_FILE* fileread;
    char currenttag[TAGSIZE] = EMPTY_CSTR;
    int i, cnt;

    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread )
    {
        log_error( "Could not load input settings (%s)!\n", szFilename );
        return bfalse;
    }

    // set the number of valid controls to be 0
    input_device_count = 0;

    // read the keyboard controls
    for ( i = KEY_CONTROL_BEGIN; i <= KEY_CONTROL_END; i++ )
    {
        fget_next_string( fileread, currenttag, SDL_arraysize( currenttag ) );
        controls[INPUT_DEVICE_KEYBOARD].control[i].tag    = scantag_get_value( currenttag );
        controls[INPUT_DEVICE_KEYBOARD].control[i].is_key = ( 'K' == currenttag[0] );
    };
    controls[INPUT_DEVICE_KEYBOARD].device = INPUT_DEVICE_KEYBOARD;
    controls[INPUT_DEVICE_KEYBOARD].count = i;
    input_device_count++;

    // read the mouse controls
    for ( i = MOS_CONTROL_BEGIN; i <= MOS_CONTROL_END; i++ )
    {
        fget_next_string( fileread, currenttag, SDL_arraysize( currenttag ) );
        controls[INPUT_DEVICE_MOUSE].control[i].tag    = scantag_get_value( currenttag );
        controls[INPUT_DEVICE_MOUSE].control[i].is_key = ( 'K' == currenttag[0] );
    };
    controls[INPUT_DEVICE_MOUSE].device = INPUT_DEVICE_MOUSE;
    controls[INPUT_DEVICE_MOUSE].count = i;
    input_device_count++;

    // read in however many joysticks there are...
    for ( cnt = 0; !vfs_eof( fileread ) && cnt < MAXJOYSTICK; cnt++ )
    {
        for ( i = JOY_CONTROL_BEGIN; i <= JOY_CONTROL_END; i++ )
        {
            fget_next_string( fileread, currenttag, SDL_arraysize( currenttag ) );
            controls[INPUT_DEVICE_JOY + cnt].control[i].tag    = scantag_get_value( currenttag );
            controls[INPUT_DEVICE_JOY + cnt].control[i].is_key = ( 'K' == currenttag[0] );
        };
        controls[INPUT_DEVICE_JOY + cnt].device = INPUT_DEVICE_JOY + cnt;
        controls[INPUT_DEVICE_JOY + cnt].count = i;
        input_device_count++;
    }

    vfs_close( fileread );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t input_settings_save_vfs( const char* szFilename )
{
    /// @details ZF@> This function saves all current game settings to "controls.txt"

    device_controls_t * pdevice;
    vfs_FILE* filewrite;
    STRING write;
    Uint32 i;

    filewrite = vfs_openWrite( szFilename );
    if ( NULL == filewrite )
    {
        log_warning( "Could not save input settings (%s)!\n", szFilename );
        return bfalse;
    }

    // Just some information
    vfs_puts( "Controls\n", filewrite );
    vfs_puts( "========\n", filewrite );
    vfs_puts( "This file lets users modify the handling of input devices.\n", filewrite );
    vfs_puts( "See the game manual for a list of settings and more info.\n", filewrite );
    vfs_puts( "Note that you can mix KEY_ type settings with other \n", filewrite );
    vfs_puts( "devices... Write the input after the colons!\n\n", filewrite );

    vfs_puts( "General Controls\n", filewrite );
    vfs_puts( "========\n", filewrite );
    vfs_puts( "These are general controls and cannot be changed\n", filewrite );
    vfs_puts( "ESC                   - End module\n", filewrite );
    vfs_puts( "SPACE                 - Respawn character (if dead and possible)\n", filewrite );
    vfs_puts( "1 to 7                - Show character detailed stats\n", filewrite );
    vfs_puts( "ATK_LEFT SHIFT   + 1 to 7 - Show selected character armor without magic enchants\n", filewrite );
    vfs_puts( "ATK_LEFT CONTROL + 1 to 7 - Show armor stats with magic enchants included\n", filewrite );
    vfs_puts( "ATK_LEFT ALT     + 1 to 7 - Show character magic enchants\n", filewrite );
    vfs_puts( "F11                   - Take screenshot\n", filewrite );
    vfs_puts( "\n", filewrite );

    // The actual settings
    pdevice = controls + INPUT_DEVICE_KEYBOARD;
    vfs_puts( "Keyboard\n", filewrite );
    vfs_puts( "========\n", filewrite );
    export_control( filewrite, "Jump                ", pdevice->device, pdevice->control + CONTROL_JUMP );
    export_control( filewrite, "Left Hand Use        ", pdevice->device, pdevice->control + CONTROL_LEFT_USE );
    export_control( filewrite, "Left Hand Get/Drop    ", pdevice->device, pdevice->control + CONTROL_LEFT_GET );
    export_control( filewrite, "Left Hand Inventory ", pdevice->device, pdevice->control + CONTROL_LEFT_PACK );
    export_control( filewrite, "Right Hand Use        ", pdevice->device, pdevice->control + CONTROL_RIGHT_USE );
    export_control( filewrite, "Right Hand Get/Drop ", pdevice->device, pdevice->control + CONTROL_RIGHT_GET );
    export_control( filewrite, "Right Hand Inventory", pdevice->device, pdevice->control + CONTROL_RIGHT_PACK );
    export_control( filewrite, "Send Message        ", pdevice->device, pdevice->control + CONTROL_MESSAGE );
    export_control( filewrite, "Camera Rotate Left    ", pdevice->device, pdevice->control + CONTROL_CAMERA_LEFT );
    export_control( filewrite, "Camera Rotate Right ", pdevice->device, pdevice->control + CONTROL_CAMERA_RIGHT );
    export_control( filewrite, "Camera Zoom In        ", pdevice->device, pdevice->control + CONTROL_CAMERA_IN );
    export_control( filewrite, "Camera Zoom Out    ", pdevice->device, pdevice->control + CONTROL_CAMERA_OUT );
    export_control( filewrite, "Up                    ", pdevice->device, pdevice->control + CONTROL_UP );
    export_control( filewrite, "Down                ", pdevice->device, pdevice->control + CONTROL_DOWN );
    export_control( filewrite, "Left                ", pdevice->device, pdevice->control + CONTROL_LEFT );
    export_control( filewrite, "Right                ", pdevice->device, pdevice->control + CONTROL_RIGHT );

    pdevice = controls + INPUT_DEVICE_MOUSE;
    vfs_puts( "\n\nMouse\n", filewrite );
    vfs_puts( "========\n", filewrite );
    export_control( filewrite, "Jump                ", pdevice->device, pdevice->control + CONTROL_JUMP );
    export_control( filewrite, "Left Hand Use        ", pdevice->device, pdevice->control + CONTROL_LEFT_USE );
    export_control( filewrite, "Left Hand Get/Drop    ", pdevice->device, pdevice->control + CONTROL_LEFT_GET );
    export_control( filewrite, "Left Hand Inventory    ", pdevice->device, pdevice->control + CONTROL_LEFT_PACK );
    export_control( filewrite, "Right Hand Use        ", pdevice->device, pdevice->control + CONTROL_RIGHT_USE );
    export_control( filewrite, "Right Hand Get/Drop    ", pdevice->device, pdevice->control + CONTROL_RIGHT_GET );
    export_control( filewrite, "Right Hand Inventory", pdevice->device, pdevice->control + CONTROL_RIGHT_PACK );
    export_control( filewrite, "Camera Control Mode    ", pdevice->device, pdevice->control + CONTROL_CAMERA );

    // export all known joysticks
    for ( i = INPUT_DEVICE_JOY; i < input_device_count; i++ )
    {
        pdevice = controls + i;

        snprintf( write, SDL_arraysize( write ), "\n\nJoystick %d\n", i - INPUT_DEVICE_JOY );
        vfs_puts( write, filewrite );
        vfs_puts( "========\n", filewrite );
        export_control( filewrite, "Jump                ", pdevice->device, pdevice->control + CONTROL_JUMP );
        export_control( filewrite, "Left Hand Use        ", pdevice->device, pdevice->control + CONTROL_LEFT_USE );
        export_control( filewrite, "Left Hand Get/Drop    ", pdevice->device, pdevice->control + CONTROL_LEFT_GET );
        export_control( filewrite, "Left Hand Inventory    ", pdevice->device, pdevice->control + CONTROL_LEFT_PACK );
        export_control( filewrite, "Right Hand Use        ", pdevice->device, pdevice->control + CONTROL_RIGHT_USE );
        export_control( filewrite, "Right Hand Get/Drop    ", pdevice->device, pdevice->control + CONTROL_RIGHT_GET );
        export_control( filewrite, "Right Hand Inventory", pdevice->device, pdevice->control + CONTROL_RIGHT_PACK );
        export_control( filewrite, "Camera Control Mode    ", pdevice->device, pdevice->control + CONTROL_CAMERA );
    }

    // All done
    vfs_close( filewrite );

    return btrue;
}

