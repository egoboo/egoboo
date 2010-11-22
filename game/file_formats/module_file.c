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

#include "module_file.h"

#include "char.inl"
#include "enchant.inl"
#include "log.h"
#include "menu.h"
#include "sound.h"
#include "graphic.h"
#include "passage.h"
#include "input.h"
#include "game.h"
#include "quest.h"

#include "egoboo_vfs.h"
#include "egoboo_strutil.h"
#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mod_file_t * module_load_info_vfs( const char * szLoadName, mod_file_t * pmod )
{
    /// @details BB@> this function actually reads in the module data

    vfs_FILE * fileread;
    int cnt;
    char cTmp;

    // clear all the module info
    if ( NULL == pmod ) return NULL;

    memset( pmod, 0, sizeof( *pmod ) );

    // see if we can open the file
    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return NULL;

    // Read basic data
    fget_next_name( fileread, pmod->longname, sizeof( pmod->longname ) );
    fget_next_string( fileread, pmod->reference, SDL_arraysize( pmod->reference ) );
    pmod->unlockquest.id    = fget_next_idsz( fileread );
    pmod->unlockquest.level = fget_int( fileread );

    pmod->importamount = fget_next_int( fileread );
    pmod->allowexport  = fget_next_bool( fileread );
    pmod->minplayers   = fget_next_int( fileread );
    pmod->maxplayers   = fget_next_int( fileread );

    cTmp = fget_next_char( fileread );
    if ( 'T' == toupper( cTmp ) )  pmod->respawnvalid = btrue;
    if ( 'A' == toupper( cTmp ) )  pmod->respawnvalid = RESPAWN_ANYTIME;

    fget_next_char( fileread );
    pmod->rtscontrol = bfalse;        //< depecrated, not in use

    fget_next_string( fileread, pmod->rank, SDL_arraysize( pmod->rank ) );
    str_trim( pmod->rank );

    // convert the special ranks of "unranked" or "-" ("rank 0") to an empty string
    if ( 0 == strncmp( pmod->rank, "-", RANKSIZE ) )
    {
        pmod->rank[0] = CSTR_END;
    }
    else if ( 'U' == toupper( pmod->rank[0] ) )
    {
        pmod->rank[0] = CSTR_END;
    }
    pmod->rank[RANKSIZE-1] = CSTR_END;

    // Read the summary
    for ( cnt = 0; cnt < SUMMARYLINES; cnt++ )
    {
        // load the string
        fget_next_string( fileread,  pmod->summary[cnt], SDL_arraysize( pmod->summary[cnt] ) );
        pmod->summary[cnt][SUMMARYSIZE-1] = CSTR_END;

        // remove the '_' characters
        str_decode( pmod->summary[cnt], SDL_arraysize( pmod->summary[cnt] ), pmod->summary[cnt] );
    }

    // Assume default module type as a sidequest
    pmod->moduletype = FILTER_SIDE;

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        IDSZ idsz = fget_idsz( fileread );

        // Read module type
        if ( idsz == MAKE_IDSZ( 'T', 'Y', 'P', 'E' ) )
        {
            // grab the expansion value
            cTmp = fget_first_letter( fileread );

            // parse the expansion value
            if ( 'M' == toupper( cTmp ) )  pmod->moduletype = FILTER_MAIN;
            else if ( 'S' == toupper( cTmp ) )  pmod->moduletype = FILTER_SIDE;
            else if ( 'T' == toupper( cTmp ) )  pmod->moduletype = FILTER_TOWN;
            else if ( 'F' == toupper( cTmp ) )  pmod->moduletype = FILTER_FUN;
            else if ( 'S' == toupper( cTmp ) )  pmod->moduletype = FILTER_STARTER;
        }
        else if ( idsz == MAKE_IDSZ( 'B', 'E', 'A', 'T' ) )
        {
            pmod->beaten = btrue;
        }
    }

    vfs_close( fileread );

    return pmod;
}

//--------------------------------------------------------------------------------------------
int module_has_idsz_vfs( const char *szModName, IDSZ idsz, size_t buffer_len, char * buffer )
{
    /// @details ZZ@> This function returns btrue if the named module has the required IDSZ

    vfs_FILE *fileread;
    STRING newloadname;
    Uint32 newidsz;
    int foundidsz;
    int cnt;

    if ( idsz == IDSZ_NONE ) return btrue;

    if ( 0 == strcmp( szModName, "NONE" ) ) return bfalse;

    snprintf( newloadname, SDL_arraysize( newloadname ), "mp_modules/%s/gamedat/menu.txt", szModName );

    fileread = vfs_openRead( newloadname );
    if ( NULL == fileread ) return bfalse;

    // Read basic data
    goto_colon( NULL, fileread, bfalse );  // Name of module...  Doesn't matter
    goto_colon( NULL, fileread, bfalse );  // Reference directory...
    goto_colon( NULL, fileread, bfalse );  // Reference IDSZ...
    goto_colon( NULL, fileread, bfalse );  // Import...
    goto_colon( NULL, fileread, bfalse );  // Export...
    goto_colon( NULL, fileread, bfalse );  // Min players...
    goto_colon( NULL, fileread, bfalse );  // Max players...
    goto_colon( NULL, fileread, bfalse );  // Respawn...
    goto_colon( NULL, fileread, bfalse );  // BAD! NOT USED
    goto_colon( NULL, fileread, bfalse );  // Rank...

    // Summary...
    for ( cnt = 0; cnt < SUMMARYLINES; cnt++ )
    {
        goto_colon( NULL, fileread, bfalse );
    }

    // Now check expansions
    foundidsz = bfalse;
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        newidsz = fget_idsz( fileread );
        if ( newidsz == idsz )
        {
            foundidsz = btrue;
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
void module_add_idsz_vfs( const char *szModName, IDSZ idsz, size_t buffer_len, const char * buffer )
{
    /// @details ZZ@> This function appends an IDSZ to the module's menu.txt file
    vfs_FILE *filewrite;

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
        if ( filewrite )
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

            // invalidate any module list so that we will reload them
            module_list_valid = bfalse;

            // close the file
            vfs_close( filewrite );
        }
    }
}
