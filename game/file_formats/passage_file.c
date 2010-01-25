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

/// @file file_formats/passage_file.c
/// @brief A scanner for the passage file for a given module ( /modules/*.mod/basicdat/passages.txt )
/// @details

#include "passage_file.h"

#include "mpd_file.h"

#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
passage_t * passage_init( passage_t * ppass )
{
    if ( NULL == ppass ) return ppass;

    memset( ppass, 0, sizeof( *ppass ) );

    ppass->music = NO_MUSIC;     // Set no song as default

    return ppass;
}

//--------------------------------------------------------------------------------------------
bool_t scan_passage_file( vfs_FILE * fileread, passage_t * ppass )
{
    /// @details ZZ@> This function reads the passage file

    bool_t found;

    if ( NULL == fileread || NULL == ppass ) return bfalse;

    passage_init( ppass );

    found = bfalse;
    if ( goto_colon( NULL, fileread, btrue ) )
    {
        ppass->area.left   = fget_int( fileread );
        ppass->area.top    = fget_int( fileread );
        ppass->area.right  = fget_int( fileread );
        ppass->area.bottom = fget_int( fileread );

        ppass->open = fget_bool( fileread );

        ppass->mask = MPDFX_IMPASS | MPDFX_WALL;
        if ( fget_bool( fileread ) ) ppass->mask = MPDFX_IMPASS;
        if ( fget_bool( fileread ) ) ppass->mask = MPDFX_SLIPPY;

        found = btrue;
    }

    return found;
}