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

/// @file egolib/FileFormats/controls_file.c
/// @brief Routines for reading and writing the file "controls.txt" and "scancode.txt"
/// @details

#include "egolib/FileFormats/controls_file.h"
#include "egolib/FileFormats/controls_file-v1.h"
#include "egolib/FileFormats/controls_file-v2.h"
#include "egolib/FileFormats/controls_file-v3.h"

#include "egolib/FileFormats/scancode_file.h"

#include "egolib/log.h"
#include "egolib/input_device.h"

#include "egolib/fileutil.h"
#include "egolib/strutil.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void export_control( vfs_FILE * filewrite, const char * text, int device, const control_t &pcontrol )
{
    STRING write;

    if ( !pcontrol.loaded )
    {
        snprintf( write, SDL_arraysize( write ), "%s : N/A\n", text );
    }
    else
    {
        snprintf( write, SDL_arraysize( write ), "%s : %s\n", text, scantag_get_string( device, pcontrol, NULL, 0 ) );
    }

    vfs_puts( write, filewrite );
}

//--------------------------------------------------------------------------------------------
bool input_settings_load_vfs(const char *szFilename, int required_version)
{
    /// @author ZZ
    /// @details This function reads the controls.txt file
    int file_version;
	bool retval = false;

    // Make sure the file exists, if not copy it from the default folder
    // should be unneeded, need to test on linux
#if 0
    if ( !fs_ensureUserFile( "controls.txt", true ) )
    {
        return false;
    }
#endif

    // get the file version
    file_version     = -1;

    ReadContext ctxt(szFilename);
    if (!ctxt.ensureOpen())
    {
        log_warning("unable to read input settings file `%s`\n",szFilename);
        return false;
    }
    file_version = vfs_get_version(ctxt);
    ctxt.close();

    // check for a version conflict
    if ( -1 != required_version && file_version != required_version )
    {
        log_warning( "File version (%d) did match the required version (%d)\n", file_version, required_version );

        return false;
    }

    switch ( file_version )
    {
        case 0:
        case 1:
            retval = input_settings_load_vfs_1( szFilename );
            break;

        case 2:
            retval = input_settings_load_vfs_2( szFilename );
            break;

        case 3:
            retval = input_settings_load_vfs_3( szFilename );
            break;

        default:
            log_warning( "Cannot load the given setting.txt file because required_version %d is not supported.\n", file_version );
            retval = false;
            break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool input_settings_save_vfs(const char* szFilename, int version)
{
    /// @author BB
    /// @details Write the controls.txt file using the correct version

	bool retval = false;

    if ( -1 == version )
    {
        version = CURRENT_CONTROLS_FILE_VERSION;
    }

    // dispatch the file to the correct writer
    switch ( version )
    {
        case 0:
        case 1:
            retval = input_settings_save_vfs_1( szFilename );
            break;

        case 2:
            retval = input_settings_save_vfs_2( szFilename );
            break;

        case 3:
            retval = input_settings_save_vfs_3( szFilename );
            break;

        default:
            log_warning( "Cannot save the given setting.txt file because version %d is not supported.\n", version );
            retval = false;
            break;
    }

    return retval;
}
