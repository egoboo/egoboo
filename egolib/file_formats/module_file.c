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

/// @file file_formats/module_file.c
/// @brief Functions for reading and writing Egoboo's menu.txt file ( /modules/*.mod/gamedat/menu.txt )
/// @details

#include "egolib/file_formats/module_file.h"
#include "egolib/file_formats/quest_file.h"

#include "egolib/log.h"

#include "egolib/vfs.h"
#include "egolib/strutil.h"
#include "egolib/fileutil.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mod_file_t * mod_file__init( mod_file_t * ptr )
{
    int cnt;

    if ( NULL == ptr ) return ptr;

    strncpy( ptr->rank, "*", sizeof( ptr->rank ) );
    strncpy( ptr->longname, "*UNKNOWN*", sizeof( ptr->longname ) );
    ptr->reference[0] = CSTR_END;
    ptr->importamount = 1;
    ptr->allowexport = C_FALSE;
    ptr->minplayers = 0;
    ptr->maxplayers = 0;
    ptr->monstersonly = C_FALSE;
    ptr->respawnvalid = C_FALSE;

    ptr->numlines = 0;
    for ( cnt = 0; cnt < SUMMARYLINES; cnt++ )
    {
        ptr->summary[SUMMARYLINES][0] = '_';
        ptr->summary[SUMMARYLINES][1] = CSTR_END;
    }

    IDSZ_node__init( &( ptr->unlockquest ) );
    ptr->moduletype = FILTER_OFF;
    ptr->beaten = C_FALSE;

    return ptr;
}

//--------------------------------------------------------------------------------------------
mod_file_t * module_load_info_vfs( const char * szLoadName, mod_file_t * pmod )
{
    /// @author BB
    /// @details this function actually reads in the module data

    vfs_FILE * fileread;
    int cnt;
    char cTmp;

    // clear all the module info
    if ( NULL == pmod ) return NULL;

    BLANK_STRUCT_PTR( pmod )

    // see if we can open the file
    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return NULL;

    // Read basic data
    vfs_get_next_name( fileread, pmod->longname, sizeof( pmod->longname ) );
    vfs_get_next_string( fileread, pmod->reference, SDL_arraysize( pmod->reference ) );
    pmod->unlockquest.id    = vfs_get_next_idsz( fileread );
    pmod->unlockquest.level = vfs_get_int( fileread );

    pmod->importamount = vfs_get_next_int( fileread );
    pmod->allowexport  = vfs_get_next_bool( fileread );
    pmod->minplayers   = vfs_get_next_int( fileread );
    pmod->maxplayers   = vfs_get_next_int( fileread );

    cTmp = vfs_get_next_char( fileread );
    if ( 'T' == char_toupper(( unsigned )cTmp ) )  pmod->respawnvalid = C_TRUE;
    if ( 'A' == char_toupper(( unsigned )cTmp ) )  pmod->respawnvalid = RESPAWN_ANYTIME;

    vfs_get_next_char( fileread );

    vfs_get_next_string( fileread, pmod->rank, SDL_arraysize( pmod->rank ) );
    str_trim( pmod->rank );

    // convert the special ranks of "unranked" or "-" ("rank 0") to an empty string
    if ( 0 == strncmp( pmod->rank, "-", RANKSIZE ) )
    {
        pmod->rank[0] = CSTR_END;
    }
    else if ( 'U' == char_toupper(( unsigned )pmod->rank[0] ) )
    {
        pmod->rank[0] = CSTR_END;
    }
    pmod->rank[RANKSIZE-1] = CSTR_END;

    // Read the summary
    for ( cnt = 0; cnt < SUMMARYLINES; cnt++ )
    {
        // load the string
        vfs_get_next_string( fileread,  pmod->summary[cnt], SDL_arraysize( pmod->summary[cnt] ) );
        pmod->summary[cnt][SUMMARYSIZE-1] = CSTR_END;

        // remove the '_' characters
        str_decode( pmod->summary[cnt], SDL_arraysize( pmod->summary[cnt] ), pmod->summary[cnt] );
    }

    // Assume default module type as a sidequest
    pmod->moduletype = FILTER_SIDE;

    // Read expansions
    while ( goto_colon_vfs( NULL, fileread, C_TRUE ) )
    {
        IDSZ idsz = vfs_get_idsz( fileread );

        // Read module type
        if ( idsz == MAKE_IDSZ( 'T', 'Y', 'P', 'E' ) )
        {
            // grab the expansion value
            cTmp = vfs_get_first_letter( fileread );

            // parse the expansion value
            if ( 'M' == char_toupper(( unsigned )cTmp ) )  pmod->moduletype = FILTER_MAIN;
            else if ( 'S' == char_toupper(( unsigned )cTmp ) )  pmod->moduletype = FILTER_SIDE;
            else if ( 'T' == char_toupper(( unsigned )cTmp ) )  pmod->moduletype = FILTER_TOWN;
            else if ( 'F' == char_toupper(( unsigned )cTmp ) )  pmod->moduletype = FILTER_FUN;
            else if ( 'S' == char_toupper(( unsigned )cTmp ) )  pmod->moduletype = FILTER_STARTER;
        }
        else if ( idsz == MAKE_IDSZ( 'B', 'E', 'A', 'T' ) )
        {
            pmod->beaten = C_TRUE;
        }
    }

    vfs_close( fileread );

    return pmod;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN module_has_idsz_vfs( const char *szModName, IDSZ idsz, size_t buffer_len, char * buffer )
{
    /// @author ZZ
    /// @details This function returns C_TRUE if the named module has the required IDSZ

    vfs_FILE *fileread;
    STRING newloadname;
    Uint32 newidsz;
    C_BOOLEAN foundidsz;
    int cnt;

    if ( idsz == IDSZ_NONE ) return C_TRUE;

    if ( 0 == strcmp( szModName, "NONE" ) ) return C_FALSE;

    snprintf( newloadname, SDL_arraysize( newloadname ), "mp_modules/%s/gamedat/menu.txt", szModName );

    fileread = vfs_openRead( newloadname );
    if ( NULL == fileread ) return C_FALSE;

    // Read basic data
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Name of module...  Doesn't matter
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Reference directory...
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Reference IDSZ...
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Import...
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Export...
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Min players...
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Max players...
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Respawn...
    goto_colon_vfs( NULL, fileread, C_FALSE );  // BAD! NOT USED
    goto_colon_vfs( NULL, fileread, C_FALSE );  // Rank...

    // Summary...
    for ( cnt = 0; cnt < SUMMARYLINES; cnt++ )
    {
        goto_colon_vfs( NULL, fileread, C_FALSE );
    }

    // Now check expansions
    foundidsz = C_FALSE;
    while ( goto_colon_vfs( NULL, fileread, C_TRUE ) )
    {
        newidsz = vfs_get_idsz( fileread );
        if ( newidsz == idsz )
        {
            foundidsz = C_TRUE;
            break;
        }
    }

    if ( NULL != buffer )
    {
        if ( buffer_len < 1 )
        {
            /* nothing */
        }
        else if ( 1 == buffer_len )
        {
            buffer[0] = CSTR_END;
        }
        else
        {
            vfs_gets( buffer, buffer_len, fileread );
        }
    }

    vfs_close( fileread );

    return foundidsz;
}

//--------------------------------------------------------------------------------------------
C_BOOLEAN module_add_idsz_vfs( const char *szModName, IDSZ idsz, size_t buffer_len, const char * buffer )
{
    /// @author ZZ
    /// @details This function appends an IDSZ to the module's menu.txt file

    vfs_FILE *filewrite;
    C_BOOLEAN retval = C_FALSE;

    // Only add if there isn't one already
    if ( !module_has_idsz_vfs( szModName, idsz, 0, NULL ) )
    {
        STRING src_file, dst_file;

        // make sure that the file exists in the user data directory since we are WRITING to it
        snprintf( src_file, SDL_arraysize( src_file ), "mp_modules/%s/gamedat/menu.txt", szModName );
        snprintf( dst_file, SDL_arraysize( dst_file ), "/modules/%s/gamedat/menu.txt", szModName );
        vfs_copyFile( src_file, dst_file );

        // Try to open the file in append mode
        filewrite = vfs_openAppend( dst_file );
        if ( NULL != filewrite )
        {
            // output the expansion IDSZ
            vfs_printf( filewrite, "\n:[%s]", undo_idsz( idsz ) );

            // output an optional parameter
            if ( NULL != buffer && buffer_len > 1 )
            {
                vfs_printf( filewrite, " %s", undo_idsz( idsz ) );
            }

            // end the line
            vfs_printf( filewrite, "\n" );

            // success
            retval = C_TRUE;

            // close the file
            vfs_close( filewrite );
        }
    }

    return retval;
}
