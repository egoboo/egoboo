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
mod_file_t * module_load_info( const char * szLoadName, mod_file_t * pmod )
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
    pmod->quest_idsz  = fget_next_idsz( fileread );
    pmod->quest_level = fget_int( fileread );

    pmod->importamount = fget_next_int( fileread );
    pmod->allowexport  = fget_next_bool( fileread );
    pmod->minplayers   = fget_next_int( fileread );
    pmod->maxplayers   = fget_next_int( fileread );

    cTmp = fget_next_char( fileread );
    pmod->respawnvalid = bfalse;
    if ( 'T' == toupper( cTmp ) )  pmod->respawnvalid = btrue;
    if ( 'A' == toupper( cTmp ) )  pmod->respawnvalid = RESPAWN_ANYTIME;

    pmod->rtscontrol = fget_next_char( fileread );

    fget_next_string( fileread, pmod->rank, SDL_arraysize( pmod->rank ) );
    pmod->rank[RANKSIZE-1] = CSTR_END;
    str_trim( pmod->rank );

    // Read the summary
    for ( cnt = 0; cnt < SUMMARYLINES; cnt++ )
    {
        // load hte string
        fget_next_string( fileread,  pmod->summary[cnt], SDL_arraysize( pmod->summary[cnt] ) );
        pmod->summary[cnt][SUMMARYSIZE-1] = CSTR_END;

        // remove the '_' characters
        str_decode( pmod->summary[cnt], SDL_arraysize( pmod->summary[cnt] ), pmod->summary[cnt] );
    }

    vfs_close( fileread );

    return pmod;
}

//--------------------------------------------------------------------------------------------
int module_has_idsz( const char *szLoadName, IDSZ idsz )
{
    /// @details ZZ@> This function returns btrue if the named module has the required IDSZ

    vfs_FILE *fileread;
    STRING newloadname;
    Uint32 newidsz;
    int foundidsz;
    int cnt;

    if ( idsz == IDSZ_NONE ) return btrue;

    if ( 0 == strcmp( szLoadName, "NONE" ) ) return bfalse;

    snprintf( newloadname, SDL_arraysize( newloadname ), "/modules/%s/gamedat/menu.txt", szLoadName );

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

    vfs_close( fileread );

    return foundidsz;
}

//--------------------------------------------------------------------------------------------
void module_add_idsz_vfs( const char *szLoadName, IDSZ idsz )
{
    /// @details ZZ@> This function appends an IDSZ to the module's menu.txt file
    vfs_FILE *filewrite;
    STRING newloadname;

    // Only add if there isn't one already
    if ( !module_has_idsz( szLoadName, idsz ) )
    {
        // Try to open the file in append mode
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/gamedat/menu.txt", szLoadName );

        filewrite = vfs_openAppend( newloadname );
        if ( filewrite )
        {
            vfs_printf( filewrite, "\n:[%s]\n", undo_idsz( idsz ) );
            vfs_close( filewrite );
        }
    }
}

