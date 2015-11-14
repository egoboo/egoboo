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

/// @file  egolib/FileFormats/controls_file-v2.c
/// @brief Routines for reading and writing version 2 of <tt>"controls.txt"</tt>.

#include "egolib/FileFormats/controls_file.h"
#include "egolib/FileFormats/controls_file-v2.h"
#include "egolib/FileFormats/scancode_file.h"

#include "egolib/log.h"
#include "egolib/input_device.h"

#include "egolib/typedef.h"
#include "egolib/fileutil.h"
#include "egolib/strutil.h"
#include "egolib/vfs.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------
bool input_settings_load_vfs_2( const char* szFilename )
{
    /// @author ZZ
    /// @details This function reads the controls.txt file, version 2

    std::string currenttag;
	int idevice;

    // clear out all existing control data
    for ( idevice = 0; idevice < MAX_LOCAL_PLAYERS; idevice++ )
    {
        // clear the input control
        InputDevices.lst[idevice].clear();
    }
    InputDevices.count = 0;

    ReadContext ctxt(szFilename);
    if (!ctxt.ensureOpen())
    {
		Log::get().warn("unable to read input settings file `%s`\n", szFilename);
        return false;
    }
    // Read input for each player
    for ( idevice = 0; idevice < MAX_LOCAL_PLAYERS; idevice++ )
    {
        input_device_t &pdevice = InputDevices.lst[idevice];

        // figure out how we move
        ctxt.skipToColon(false);
        currenttag = ctxt.readName();
        if (currenttag.empty())
        {
            continue;
        }

        // get the input device type from the tag
		int type;
		type = translate_string_to_input_type( currenttag.c_str() );

        // set the device type based on the control name
		pdevice.initialize(static_cast<e_input_device>(type));

        //Find out how many fields we are to read
		size_t count;
        if ( INPUT_DEVICE_KEYBOARD == pdevice.device_type ) count = CONTROL_COMMAND_COUNT;
        else                                                count = CONTROL_CAMERA + 1;

        //Read each input control button
        for ( size_t icontrol = CONTROL_BEGIN; icontrol < count; icontrol++ )
        {
            ctxt.skipToColon(false);
            currenttag = ctxt.readToEndOfLine();
            if (!currenttag.empty())
            {
                scantag_parse_control( currenttag.c_str(), pdevice.keyMap[icontrol] );
            }
        }

        InputDevices.count++;
    }
    return InputDevices.count > 0;
}

//--------------------------------------------------------------------------------------------
bool input_settings_save_vfs_2(const char* szFilename)
{
    /// @author ZF
    /// @details This function saves all current game settings to "controls.txt"
    vfs_FILE* filewrite;
    STRING write;
    Uint32 i;

    filewrite = vfs_openWrite( szFilename );
    if ( NULL == filewrite )
    {
		Log::get().warn( "Could not save input settings (%s)!\n", szFilename );
        return false;
    }

    //Add version number
    vfs_put_version( filewrite, 2 );

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
        input_device_t &pdevice = InputDevices.lst[i];
        snprintf( write, SDL_arraysize( write ), "\nPLAYER %i\n", i + 1 );

        //which player
        vfs_puts( write, filewrite );
        vfs_puts( "========\n", filewrite );

        //controller type
        snprintf( write, SDL_arraysize( write ), "CONTROLLER:         %s\n", translate_input_type_to_string( pdevice.device_type ) );
        vfs_puts( write, filewrite );

        //Default input InputDevices
        export_control( filewrite, "Jump                 ", pdevice.device_type, pdevice.keyMap[CONTROL_JUMP] );
        export_control( filewrite, "Left Hand Use        ", pdevice.device_type, pdevice.keyMap[CONTROL_LEFT_USE] );
        export_control( filewrite, "Left Hand Get/Drop   ", pdevice.device_type, pdevice.keyMap[CONTROL_LEFT_GET] );
        export_control( filewrite, "Left Hand Inventory  ", pdevice.device_type, pdevice.keyMap[CONTROL_LEFT_PACK] );
        export_control( filewrite, "Right Hand Use       ", pdevice.device_type, pdevice.keyMap[CONTROL_RIGHT_USE] );
        export_control( filewrite, "Right Hand Get/Drop  ", pdevice.device_type, pdevice.keyMap[CONTROL_RIGHT_GET] );
        export_control( filewrite, "Right Hand Inventory ", pdevice.device_type, pdevice.keyMap[CONTROL_RIGHT_PACK] );
        export_control( filewrite, "Sneak                ", pdevice.device_type, pdevice.keyMap[CONTROL_SNEAK] );

        //this is only needed for keyboard
        if ( INPUT_DEVICE_KEYBOARD == pdevice.device_type )
        {
            //Could be a global key?
            export_control( filewrite, "Send Message", pdevice.device_type, pdevice.keyMap[CONTROL_MESSAGE] );

            export_control( filewrite, "Camera Rotate Left  ", pdevice.device_type, pdevice.keyMap[CONTROL_CAMERA_LEFT] );
            export_control( filewrite, "Camera Rotate Right ", pdevice.device_type, pdevice.keyMap[CONTROL_CAMERA_RIGHT] );
            export_control( filewrite, "Camera Zoom In      ", pdevice.device_type, pdevice.keyMap[CONTROL_CAMERA_IN] );
            export_control( filewrite, "Camera Zoom Out     ", pdevice.device_type, pdevice.keyMap[CONTROL_CAMERA_OUT] );

            export_control( filewrite, "Up                  ", pdevice.device_type, pdevice.keyMap[CONTROL_UP] );
            export_control( filewrite, "Down                ", pdevice.device_type, pdevice.keyMap[CONTROL_DOWN] );
            export_control( filewrite, "Left                ", pdevice.device_type, pdevice.keyMap[CONTROL_LEFT] );
            export_control( filewrite, "Right               ", pdevice.device_type, pdevice.keyMap[CONTROL_RIGHT] );
        }

        //Mouse and Joystick specific
        else
        {
            export_control( filewrite, "Camera Control Mode	 ", pdevice.device_type, pdevice.keyMap[CONTROL_CAMERA] );
        }
    }

    // All done
    vfs_close( filewrite );

    return true;
}
