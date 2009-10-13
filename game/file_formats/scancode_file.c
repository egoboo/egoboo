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

/// @file file_formats/scancode_file.c
/// @brief Functions to read and write egoboo's basicdat/scantag.txt file
/// @details

#include "scancode_file.h"

#include "log.h"
#include "input.h"

#include "egoboo_vfs.h"
#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int       scantag_count = 0;
scantag_t scantag[MAXTAG];

static void   scantag_reset();
static bool_t scantag_read_one( vfs_FILE *fileread );

//--------------------------------------------------------------------------------------------
// Tag Reading
//--------------------------------------------------------------------------------------------
void scantag_reset()
{
    /// @details ZZ@> This function resets the tags
    scantag_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t scantag_read_one( vfs_FILE *fileread )
{
    /// @details ZZ@> This function finds the next tag, returning btrue if it found one

    bool_t retval;

    retval = goto_colon( NULL, fileread, btrue ) && (scantag_count < MAXTAG);
    if ( retval )
    {
        fget_string( fileread, scantag[scantag_count].name, SDL_arraysize(scantag[scantag_count].name) );
        scantag[scantag_count].value = fget_int( fileread );
        scantag_count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void scantag_read_all( const char *szFilename )
{
    /// @details ZZ@> This function reads the scancode.txt file
    vfs_FILE* fileread;

    scantag_reset();

    fileread = vfs_openRead( szFilename );
    if ( NULL == fileread )
    {
        log_error( "Cannot read %s.", szFilename );
    }

    while ( scantag_read_one( fileread ) );

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
int scantag_get_value( const char *string )
{
    /// @details ZZ@> This function matches the string with its tag, and returns the value...
    ///    It will return 255 if there are no matches.

    int cnt;

    cnt = 0;

    while ( cnt < scantag_count )
    {
        if ( 0 == strcmp( string, scantag[cnt].name ) )
        {
            // They match
            return scantag[cnt].value;
        }

        cnt++;
    }

    // No matches
    return 255;
}

//--------------------------------------------------------------------------------------------
char* scantag_get_string( Sint32 device, Sint32 tag, bool_t is_key )
{
    /// @details ZF@> This translates a input tag value to a string

    int cnt;

    if ( device >= INPUT_DEVICE_JOY ) device = INPUT_DEVICE_JOY;
    if ( device == INPUT_DEVICE_KEYBOARD ) is_key = btrue;

    for ( cnt = 0; cnt < scantag_count; cnt++ )
    {
        // do not search invalid keys
        if ( is_key )
        {
            if ( 'K' != scantag[cnt].name[0] ) continue;
        }
        else
        {
            switch ( device )
            {
                case INPUT_DEVICE_MOUSE:
                    if ( 'M' != scantag[cnt].name[0] ) continue;
                    break;

                case INPUT_DEVICE_JOY:
                    if ( 'J' != scantag[cnt].name[0] ) continue;
                    break;
            }
        };
        if ( tag == scantag[cnt].value)
        {
            return scantag[cnt].name;
        }
    }

    // No matches
    return "N/A";
}
