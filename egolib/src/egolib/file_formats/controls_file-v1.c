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

/// @file  egolib/file_formats/controls_file-v1.c
/// @brief Routines for reading and writing version 1 of <tt>"controls.txt"</tt>.

#include "egolib/file_formats/controls_file.h"
#include "egolib/file_formats/controls_file-v1.h"
#include "egolib/file_formats/scancode_file.h"

#include "egolib/log.h"
#include "egolib/input_device.h"

#include "egolib/fileutil.h"
#include "egolib/strutil.h"
#include "egolib/platform.h"
#include "egolib/vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool input_settings_load_vfs_1(const char* szFilename)
{
    /// @author ZZ
    /// @details This function reads the settings.txt file, version 3

    int i, cnt;
    INPUT_DEVICE idevice;

    input_device_t * pdevice = NULL;
    vfs_FILE* fileread = NULL;
    TAG_STRING currenttag = EMPTY_CSTR;

    // clear out all existing control data
    for ( cnt = 0; cnt < MAX_LOCAL_PLAYERS; cnt++ )
    {
        // clear the input control
        input_device_ctor( InputDevices.lst + cnt );
    }
    InputDevices.count = 0;

    // find the settings file
    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread )
    {
        log_error( "Could not load input settings (%s)!\n", szFilename );

        return false;
    }
    ReadContext ctxt(szFilename, fileread, true);

    // read the keyboard InputDevices
    idevice = INPUT_DEVICE_KEYBOARD;
    pdevice = InputDevices.lst + idevice;
    for ( i = KEY_CONTROL_BEGIN; i <= KEY_CONTROL_END; i++ )
    {
        if ( vfs_get_next_line( ctxt, currenttag, SDL_arraysize( currenttag ) ) )
        {
            scantag_parse_control( currenttag, pdevice->keyMap[i] );
        }
    };
    pdevice->device_type = idevice;
    InputDevices.count++;

    // read the mouse InputDevices
    idevice = INPUT_DEVICE_MOUSE;
    pdevice = InputDevices.lst + idevice;
    for ( i = MOS_CONTROL_BEGIN; i <= MOS_CONTROL_END; i++ )
    {
        if ( vfs_get_next_line(ctxt, currenttag, SDL_arraysize( currenttag ) ) )
        {
            scantag_parse_control( currenttag, pdevice->keyMap[i] );
        }
    };
    pdevice->device_type = idevice;
    InputDevices.count++;

    // read in however many joysticks there are...
    for ( cnt = 0; !vfs_eof(ctxt._file) && cnt < MAX_JOYSTICK; cnt++ )
    {
        idevice = ( INPUT_DEVICE )( INPUT_DEVICE_JOY + cnt );
        pdevice = InputDevices.lst + idevice;
        for ( i = JOY_CONTROL_BEGIN; i <= JOY_CONTROL_END; i++ )
        {
            if ( vfs_get_next_line( ctxt, currenttag, SDL_arraysize( currenttag ) ) )
            {
                scantag_parse_control( currenttag, pdevice->keyMap[i] );
            }
        };
        pdevice->device_type = idevice;

        InputDevices.count++;
    }

    return InputDevices.count > 0;
}

//--------------------------------------------------------------------------------------------
bool input_settings_save_vfs_1(const char* szFilename)
{
    /// @author ZF
    /// @details This function saves all current game settings to "controls.txt"

    STRING write;
    INPUT_DEVICE idevice;

    input_device_t * pdevice = NULL;
    vfs_FILE* filewrite = NULL;

    filewrite = vfs_openWrite( szFilename );
    if ( NULL == filewrite )
    {
        log_warning( "Could not save input settings (%s)!\n", szFilename );
        return false;
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
    idevice = INPUT_DEVICE_KEYBOARD;
    pdevice = InputDevices.lst + idevice;
    vfs_puts( "Keyboard\n", filewrite );
    vfs_puts( "========\n", filewrite );
    export_control( filewrite, "Jump                ", idevice, pdevice->keyMap[CONTROL_JUMP] );
    export_control( filewrite, "Left Hand Use        ", idevice, pdevice->keyMap[CONTROL_LEFT_USE] );
    export_control( filewrite, "Left Hand Get/Drop    ", idevice, pdevice->keyMap[CONTROL_LEFT_GET] );
    export_control( filewrite, "Left Hand Inventory ", idevice, pdevice->keyMap[CONTROL_LEFT_PACK] );
    export_control( filewrite, "Right Hand Use        ", idevice, pdevice->keyMap[CONTROL_RIGHT_USE] );
    export_control( filewrite, "Right Hand Get/Drop ", idevice, pdevice->keyMap[CONTROL_RIGHT_GET] );
    export_control( filewrite, "Right Hand Inventory", idevice, pdevice->keyMap[CONTROL_RIGHT_PACK] );
    export_control( filewrite, "Send Message        ", idevice, pdevice->keyMap[CONTROL_MESSAGE] );
    export_control( filewrite, "Camera Rotate Left    ", idevice, pdevice->keyMap[CONTROL_CAMERA_LEFT] );
    export_control( filewrite, "Camera Rotate Right ", idevice, pdevice->keyMap[CONTROL_CAMERA_RIGHT] );
    export_control( filewrite, "Camera Zoom In        ", idevice, pdevice->keyMap[CONTROL_CAMERA_IN] );
    export_control( filewrite, "Camera Zoom Out    ", idevice, pdevice->keyMap[CONTROL_CAMERA_OUT] );
    export_control( filewrite, "Up                    ", idevice, pdevice->keyMap[CONTROL_UP] );
    export_control( filewrite, "Down                ", idevice, pdevice->keyMap[CONTROL_DOWN] );
    export_control( filewrite, "Left                ", idevice, pdevice->keyMap[CONTROL_LEFT] );
    export_control( filewrite, "Right                ", idevice, pdevice->keyMap[CONTROL_RIGHT] );

    idevice = INPUT_DEVICE_MOUSE;
    pdevice = InputDevices.lst + idevice;
    vfs_puts( "\n\nMouse\n", filewrite );
    vfs_puts( "========\n", filewrite );
    export_control( filewrite, "Jump                ", idevice, pdevice->keyMap[CONTROL_JUMP] );
    export_control( filewrite, "Left Hand Use        ", idevice, pdevice->keyMap[CONTROL_LEFT_USE] );
    export_control( filewrite, "Left Hand Get/Drop    ", idevice, pdevice->keyMap[CONTROL_LEFT_GET] );
    export_control( filewrite, "Left Hand Inventory    ", idevice, pdevice->keyMap[CONTROL_LEFT_PACK] );
    export_control( filewrite, "Right Hand Use        ", idevice, pdevice->keyMap[CONTROL_RIGHT_USE] );
    export_control( filewrite, "Right Hand Get/Drop    ", idevice, pdevice->keyMap[CONTROL_RIGHT_GET] );
    export_control( filewrite, "Right Hand Inventory", idevice, pdevice->keyMap[CONTROL_RIGHT_PACK] );
    export_control( filewrite, "Camera Control Mode    ", idevice, pdevice->keyMap[CONTROL_CAMERA] );

    // export all known joysticks
    for ( idevice = INPUT_DEVICE_JOY; idevice < InputDevices.count; idevice = ( INPUT_DEVICE )( idevice + 1 ) )
    {
        pdevice = InputDevices.lst + idevice;

        snprintf( write, SDL_arraysize( write ), "\n\nJoystick %d\n", idevice - INPUT_DEVICE_JOY );
        vfs_puts( write, filewrite );
        vfs_puts( "========\n", filewrite );
        export_control( filewrite, "Jump                ", idevice, pdevice->keyMap[CONTROL_JUMP] );
        export_control( filewrite, "Left Hand Use        ", idevice, pdevice->keyMap[CONTROL_LEFT_USE] );
        export_control( filewrite, "Left Hand Get/Drop    ", idevice, pdevice->keyMap[CONTROL_LEFT_GET] );
        export_control( filewrite, "Left Hand Inventory    ", idevice, pdevice->keyMap[CONTROL_LEFT_PACK] );
        export_control( filewrite, "Right Hand Use        ", idevice, pdevice->keyMap[CONTROL_RIGHT_USE] );
        export_control( filewrite, "Right Hand Get/Drop    ", idevice, pdevice->keyMap[CONTROL_RIGHT_GET] );
        export_control( filewrite, "Right Hand Inventory", idevice, pdevice->keyMap[CONTROL_RIGHT_PACK] );
        export_control( filewrite, "Camera Control Mode    ", idevice, pdevice->keyMap[CONTROL_CAMERA] );
    }

    // All done
    vfs_close( filewrite );

    return true;
}
