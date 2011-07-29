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
#include "controls_file-v1.h"
#include "controls_file-v2.h"
#include "controls_file-v3.h"

#include "scancode_file.h"

#include "../log.h"
#include "../input_device.h"

#include "../fileutil.h"
#include "../strutil.h"
#include "../platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void export_control( vfs_FILE * filewrite, const char * text, int device, control_t * pcontrol )
{
    STRING write;

    if ( !pcontrol->loaded )
    {
        snprintf( write, SDL_arraysize( write ), "%s : N/A\n", text );
    }
    else
    {
        snprintf( write, SDL_arraysize( write ), "%s : %s\n", text, scantag_get_string( device, pcontrol->tag, pcontrol->is_key ) );
    }

    vfs_puts( write, filewrite );
}

//--------------------------------------------------------------------------------------------
bool_t input_settings_load_vfs( const char *szFilename, int required_version )
{
    /// @details ZZ@> This function reads the controls.txt file
    int file_version;
    vfs_FILE* fileread = NULL;
    bool_t retval = bfalse;

    // Make sure the file exists, if not copy it from the default folder
    if ( !fs_ensureUserFile( "controls.txt", btrue ) )
    {
        return bfalse;
    }

    // get the file version
    file_version     = -1;

    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread )
    {
        log_warning( "Could not load input settings (%s)!\n", szFilename );
        return bfalse;
    }
    else
    {
        file_version = vfs_get_version( fileread );

        vfs_close( fileread );
    }

    // check for a version conflict
    if ( -1 != required_version && file_version != required_version )
    {
        log_warning( "File version (%d) did match the required version (%d)\n", file_version, required_version );

        return bfalse;
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
            retval = bfalse;
            break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t input_settings_save_vfs( const char* szFilename, int version )
{
    /// @details BB@> Write the controls.txt file using the correct version

    bool_t retval = bfalse;

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
            retval = bfalse;
            break;
    }

    return retval;
}
