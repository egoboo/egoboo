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

/// @file egolib/file_formats/spawn_file.c
/// @brief Implementation of a scanner for Egoboo's spawn.txt file
/// @details

#include "egolib/file_formats/spawn_file.h"

#include "egolib/file_formats/cap_file.h"

#include "egolib/fileutil.h"
#include "egolib/strutil.h"

#include "egolib/_math.inl"

// includes for egoboo constants
#include "game/char.h"       // for TEAM_* constants

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
spawn_file_info_t * spawn_file_info_init( spawn_file_info_t *pinfo )
{
    /// @author BB
    /// @details safe values for all parameters

    if ( NULL == pinfo ) return pinfo;

    BLANK_STRUCT_PTR( pinfo )

    pinfo->attach = ATTACH_NONE;
    pinfo->team   = TEAM_NULL;

    return pinfo;
}

//--------------------------------------------------------------------------------------------
spawn_file_info_t * spawn_file_info_reinit( spawn_file_info_t *pinfo )
{
    CHR_REF old_parent;

    if ( NULL == pinfo ) return pinfo;

    // save the parent data just in case
    old_parent = pinfo->parent;

    // init the data
    spawn_file_info_init( pinfo );

    // restore the parent data
    pinfo->parent = old_parent;

    return pinfo;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN spawn_file_scan( vfs_FILE * fileread, spawn_file_info_t *pinfo )
{
    char cTmp, delim;
    C_BOOLEAN retval;

    // trap bad pointers
    if ( NULL == fileread || NULL == pinfo ) return C_FALSE;

    spawn_file_info_reinit( pinfo );

    // check for another entry, either the "#" or ":" delimiters
    delim = goto_delimiter_list_vfs( pinfo->spawn_coment, fileread, "#:", C_TRUE );
    if ( CSTR_END == delim ) return C_FALSE;

    retval = C_FALSE;
    if ( ':' == delim )
    {
        retval = C_TRUE;

        pinfo->do_spawn = C_TRUE;

        vfs_get_string( fileread, pinfo->spawn_name, SDL_arraysize( pinfo->spawn_name ) );
        str_decode( pinfo->spawn_name, SDL_arraysize( pinfo->spawn_name ), pinfo->spawn_name );

        pinfo->pname = pinfo->spawn_name;
        if ( 0 == strcmp( pinfo->spawn_name, "NONE" ) )
        {
            // Random pinfo->pname
            pinfo->pname = NULL;
        }

        pinfo->slot = vfs_get_int( fileread );

        pinfo->pos.x = vfs_get_float( fileread ) * GRID_FSIZE;
        pinfo->pos.y = vfs_get_float( fileread ) * GRID_FSIZE;
        pinfo->pos.z = vfs_get_float( fileread ) * GRID_FSIZE;

        pinfo->facing = FACE_NORTH;
        pinfo->attach = ATTACH_NONE;
        cTmp = vfs_get_first_letter( fileread );
        if ( 'S' == char_toupper(( unsigned )cTmp ) )       pinfo->facing = FACE_SOUTH;
        else if ( 'E' == char_toupper(( unsigned )cTmp ) )  pinfo->facing = FACE_EAST;
        else if ( 'W' == char_toupper(( unsigned )cTmp ) )  pinfo->facing = FACE_WEST;
        else if ( '?' == char_toupper(( unsigned )cTmp ) )  pinfo->facing = FACE_RANDOM;
        else if ( 'L' == char_toupper(( unsigned )cTmp ) )  pinfo->attach = ATTACH_LEFT;
        else if ( 'R' == char_toupper(( unsigned )cTmp ) )  pinfo->attach = ATTACH_RIGHT;
        else if ( 'I' == char_toupper(( unsigned )cTmp ) )  pinfo->attach = ATTACH_INVENTORY;

        pinfo->money   = vfs_get_int( fileread );
        pinfo->skin    = vfs_get_int( fileread );
        pinfo->passage = vfs_get_int( fileread );
        pinfo->content = vfs_get_int( fileread );
        pinfo->level   = vfs_get_int( fileread );

        if ( pinfo->skin >= MAX_SKIN )
        {
            int irand = RANDIE;
            pinfo->skin = irand % MAX_SKIN;     // Randomize skin?
        }

        pinfo->stat = vfs_get_bool( fileread );

        vfs_get_first_letter( fileread );   ///< BAD! Unused ghost value

        cTmp = vfs_get_first_letter( fileread );
        pinfo->team = ( cTmp - 'A' ) % TEAM_MAX;
    }
    else if ( '#' == delim )
    {
        STRING szTmp1, szTmp2;
        int    iTmp, fields;

        pinfo->do_spawn = C_FALSE;

        fields = vfs_scanf( fileread, "%255s%255s%d", szTmp1, szTmp2, &iTmp );
        if ( 3 == fields && 0 == strcmp( szTmp1, "dependency" ) )
        {
            retval = C_TRUE;

            // seed the info with the data
            strncpy( pinfo->spawn_coment, szTmp2, SDL_arraysize( pinfo->spawn_coment ) );
            pinfo->slot = iTmp;
        }
    }

    return retval;
}


