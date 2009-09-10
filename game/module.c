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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - module.c
 */

#include "module.h"

#include "log.h"
#include "menu.h"
#include "sound.h"
#include "graphic.h"
#include "char.h"
#include "enchant.h"
#include "passage.h"
#include "input.h"
#include "game.h"

#include "egoboo_vfs.h"
#include "egoboo_strutil.h"
#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

DECLARE_STACK( ACCESS_TYPE_NONE, mod_data_t, ModList );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t module_load_info( const char * szLoadName, mod_data_t * pmod );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int module_reference_matches( const char *szLoadName, IDSZ idsz )
{
    // ZZ> This function returns btrue if the named module has the required IDSZ
    vfs_FILE *fileread;
    STRING newloadname;
    Uint32 newidsz;
    int foundidsz;
    int cnt;

    if ( idsz == IDSZ_NONE ) return btrue;

    if ( 0 == strcmp( szLoadName, "NONE" )  ) return bfalse;

    snprintf( newloadname, SDL_arraysize( newloadname), "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", szLoadName );

    parse_filename = "";
    fileread = vfs_openRead( newloadname );
    if ( NULL == fileread ) return bfalse;
    parse_filename = newloadname;

    // Read basic data
    parse_filename = szLoadName;
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
    while ( goto_colon( NULL, fileread, btrue )  )
    {
        newidsz = fget_idsz( fileread );
        if ( newidsz == idsz )
        {
            foundidsz = btrue;
            break;
        }
    }

    vfs_close( fileread );
    parse_filename = "";

    return foundidsz;
}

//--------------------------------------------------------------------------------------------
void module_add_idsz( const char *szLoadName, IDSZ idsz )
{
    // ZZ> This function appends an IDSZ to the module's menu.txt file
    vfs_FILE *filewrite;
    STRING newloadname;
    char chara, charb, charc, chard;

    // Only add if there isn't one already
    if ( !module_reference_matches( szLoadName, idsz ) )
    {
        // Try to open the file in append mode
        snprintf( newloadname, SDL_arraysize( newloadname), "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", szLoadName );
        filewrite = vfs_openAppend( newloadname );
        if ( filewrite )
        {
            chara = ( ( idsz >> 15 ) & 31 ) + 'A';
            charb = ( ( idsz >> 10 ) & 31 ) + 'A';
            charc = ( ( idsz >> 5 ) & 31 ) + 'A';
            chard = ( ( idsz ) & 31 ) + 'A';
            vfs_printf( filewrite, "\n:[%c%c%c%c]\n", chara, charb, charc, chard );
            vfs_close( filewrite );
        }
    }
}

//--------------------------------------------------------------------------------------------
int modlist_get_mod_number( const char *szModName )
{
    // ZZ> This function returns -1 if the module does not exist locally, the module
    //    index otherwise

    int modnum, retval = -1;

    for ( modnum = 0; modnum < ModList.count; modnum++ )
    {
        if ( 0 == strcmp( ModList.lst[modnum].loadname, szModName ) )
        {
            retval = modnum;
            break;
        }
    }

    return modnum;
}

//--------------------------------------------------------------------------------------------
bool_t modlist_test_by_index( int modnumber )
{
    int     cnt;
    mod_data_t * pmod;
    bool_t  allowed;
    bool_t  playerhasquest;

    if ( modnumber < 0 || modnumber >= ModList.count || modnumber >= MAX_MODULE ) return bfalse;
    pmod = ModList.lst + modnumber;

    // if the module data was never loaded, then it is not valid
    if ( !pmod->loaded ) return bfalse;

    // Check all selected players directories
    playerhasquest = bfalse;
    for ( cnt = 0; cnt < mnu_selectedPlayerCount; cnt++ )
    {
        if ( pmod->quest_level <= check_player_quest( loadplayer[mnu_selectedPlayer[cnt]].name, pmod->quest_idsz ))
        {
            playerhasquest = btrue;
            break;
        }
    }

    // So, do we load the module or not?
    allowed = bfalse;
    if ( cfg.dev_mode || playerhasquest || module_reference_matches( pmod->reference, pmod->quest_idsz ) )
    {
        allowed = btrue;
    }

    return allowed;
}

//--------------------------------------------------------------------------------------------
bool_t modlist_test_by_name( const char *szModName )
{
    // ZZ> This function tests to see if a module can be entered by
    //    the players

    // find the module by name
    int modnumber = modlist_get_mod_number( szModName );

    return modlist_test_by_index( modnumber );
}

//--------------------------------------------------------------------------------------------
bool_t module_load_info( const char * szLoadName, mod_data_t * pmod )
{
    // BB > this function actually reads in the module data

    vfs_FILE * fileread;
    int cnt;
    char cTmp;

    // clear all the module info
    if ( NULL == pmod ) return bfalse;
    memset( pmod, 0, sizeof(mod_data_t) );

    // see if we can open the file
    fileread = vfs_openRead( szLoadName );
    if ( NULL == fileread ) return bfalse;

    // the file is open
    parse_filename = szLoadName;

    // Read basic data
    fget_next_name( fileread, pmod->longname, sizeof(pmod->longname) );
    fget_next_string( fileread, pmod->reference, SDL_arraysize(pmod->reference) );
    pmod->quest_idsz = fget_next_idsz( fileread );
    pmod->quest_level = fget_int( fileread );

    pmod->importamount = fget_next_int( fileread );
    pmod->allowexport  = fget_next_bool( fileread );
    pmod->minplayers   = fget_next_int( fileread );
    pmod->maxplayers   = fget_next_int( fileread );

    cTmp = fget_next_char( fileread );
    pmod->respawnvalid = bfalse;
    if ( 'T' == toupper(cTmp) )  pmod->respawnvalid = btrue;
    if ( 'A' == toupper(cTmp) )  pmod->respawnvalid = RESPAWN_ANYTIME;

    goto_colon( NULL, fileread, bfalse );   // BAD: Skip line
    // pmod->rtscontrol = bfalse;

    fget_next_string( fileread, pmod->rank, SDL_arraysize(pmod->rank) );
    pmod->rank[RANKSIZE-1] = '\0';
    str_trim( pmod->rank );

    // Read the summary
    for ( cnt = 0; cnt < SUMMARYLINES; cnt++ )
    {
        // load hte string
        fget_next_string( fileread,  pmod->summary[cnt], SDL_arraysize(pmod->summary[cnt]) );
        pmod->summary[cnt][SUMMARYSIZE-1] = '\0';

        // remove the '_' characters
        str_decode( pmod->summary[cnt], SDL_arraysize(pmod->summary[cnt]), pmod->summary[cnt] );
    }

    vfs_close(fileread);

    pmod->loaded = btrue;

    return pmod->loaded;
}

//--------------------------------------------------------------------------------------------
void modlist_load_all_info()
{
    STRING loadname;
    const char *FileName;

    // Search for all .mod directories and load the module info
    ModList.count = 0;
    FileName = vfs_findFirst( "modules", "mod", VFS_SEARCH_DIR );
    while ( VALID_CSTR(FileName) && ModList.count < MAX_MODULE )
    {
        // save the filename
        snprintf( loadname, SDL_arraysize( loadname), "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", FileName );

        if ( module_load_info( loadname, ModList.lst + ModList.count ) )
        {
            strncpy( ModList.lst[ModList.count].loadname, FileName, MAXCAPNAMESIZE );
            ModList.count++;
        };

        FileName = vfs_findNext();
    }
    vfs_findClose();
    ModList.lst[ModList.count].longname[0] = '\0';
}

//--------------------------------------------------------------------------------------------
bool_t module_instance_init( module_instance_t * pinst )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof(module_instance_t) );

    pinst->seed = (Uint32)~0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t module_upload( module_instance_t * pinst, int imod, Uint32 seed )
{
    mod_data_t * pdata;

    if ( imod < 0 || imod >= ModList.count ) return bfalse;

    if ( !module_instance_init(pinst) ) return bfalse;

    pdata = ModList.lst + imod;

    pinst->importamount   = pdata->importamount;
    pinst->exportvalid    = pdata->allowexport;
    pinst->playeramount   = pdata->maxplayers;
    pinst->importvalid    = ( pinst->importamount > 0 );
    pinst->respawnvalid   = ( bfalse != pdata->respawnvalid );
    pinst->respawnanytime = ( RESPAWN_ANYTIME == pdata->respawnvalid );

    strncpy(pinst->loadname, pdata->loadname, SDL_arraysize(pinst->loadname));

    pinst->active = bfalse;
    pinst->beat   = bfalse;
    pinst->seed   = seed;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t module_reset( module_instance_t * pinst, Uint32 seed )
{
    if (NULL == pinst) return bfalse;

    pinst->beat        = bfalse;
    //pinst->exportvalid = bfalse;  //Zefz> we can't disable export here, some modules are supposed to allow export (towns)
    pinst->seed        = seed;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t module_start( module_instance_t * pinst )
{
    // BB> Let the module go

    if (NULL == pinst) return bfalse;

    pinst->active = btrue;

    srand( pinst->seed );
    pinst->randsave = rand();
    randindex = rand() % RANDIE_COUNT;

    PNet->hostactive = btrue; // very important or the input will not work

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t module_stop( module_instance_t * pinst )
{
    // BB> stop the module

    if (NULL == pinst) return bfalse;

    pinst->active      = bfalse;

    // ntework stuff
    PNet->hostactive  = bfalse;

    return btrue;
}
