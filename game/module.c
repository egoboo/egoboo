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
#include "file_common.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int        ModList_count = 0;                            // Number of modules
mod_data_t ModList[MAXMODULE];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t module_load_info( const char * szLoadName, mod_data_t * pmod );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int module_reference_matches( const char *szLoadName, IDSZ idsz )
{
    // ZZ> This function returns btrue if the named module has the required IDSZ
    FILE *fileread;
    char newloadname[256];
    Uint32 newidsz;
    int foundidsz;
    int cnt;

    if ( idsz == IDSZ_NONE ) return btrue;

    if ( 0 == strcmp( szLoadName, "NONE" )  ) return bfalse;

    sprintf( newloadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", szLoadName );

    parse_filename = "";
    fileread = fopen( newloadname, "r" );
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

    fclose( fileread );
    parse_filename = "";

    return foundidsz;
}

//--------------------------------------------------------------------------------------------
void module_add_idsz( const char *szLoadName, IDSZ idsz )
{
    // ZZ> This function appends an IDSZ to the module's menu.txt file
    FILE *filewrite;
    char newloadname[256];
    char chara, charb, charc, chard;

    // Only add if there isn't one already
    if ( !module_reference_matches( szLoadName, idsz ) )
    {
        // Try to open the file in append mode
        sprintf( newloadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", szLoadName );
        filewrite = fopen( newloadname, "a" );
        if ( filewrite )
        {
            chara = ( ( idsz >> 15 ) & 31 ) + 'A';
            charb = ( ( idsz >> 10 ) & 31 ) + 'A';
            charc = ( ( idsz >> 5 ) & 31 ) + 'A';
            chard = ( ( idsz ) & 31 ) + 'A';
            fprintf( filewrite, "\n:[%c%c%c%c]\n", chara, charb, charc, chard );
            fclose( filewrite );
        }
    }
}

//--------------------------------------------------------------------------------------------
int modlist_get_mod_number( const char *szModName )
{
    // ZZ> This function returns -1 if the module does not exist locally, the module
    //     index otherwise

    int modnum, retval = -1;

    for ( modnum = 0; modnum < ModList_count; modnum++ )
    {
        if ( 0 == strcmp( ModList[modnum].loadname, szModName ) )
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

    if ( modnumber < 0 || modnumber >= ModList_count || modnumber >= MAXMODULE ) return bfalse;
    pmod = ModList + modnumber;

    // if the module data was never loaded, then it is not valid
    if ( !pmod->loaded ) return bfalse;

    //Check all selected players directories
    playerhasquest = bfalse;
    for ( cnt = 0; cnt < mnu_selectedPlayerCount; cnt++ )
    {
        if ( pmod->quest_level <= check_player_quest( loadplayer[mnu_selectedPlayer[cnt]].name, pmod->quest_idsz ))
        {
            playerhasquest = btrue;
            break;
        }
    }

    //So, do we load the module or not?
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
    //     the players

    // find the module by name
    int modnumber = modlist_get_mod_number( szModName );

    return modlist_test_by_index( modnumber );
}

//--------------------------------------------------------------------------------------------
bool_t module_load_info( const char * szLoadName, mod_data_t * pmod )
{
    // BB > this function actually reads in the module data

    FILE * fileread;
    STRING readtext, szLine;
    int cnt, tnc, iTmp;
    char cTmp;

    // clear all the module info
    if ( NULL == pmod ) return bfalse;
    memset( pmod, 0, sizeof(mod_data_t) );

    // see if we can open the file
    fileread = fopen( szLoadName, "r" );
    if ( NULL == fileread ) return bfalse;

    // the file is open
    parse_filename = szLoadName;

    // Read basic data
    goto_colon( NULL, fileread, bfalse );  fget_name( fileread, pmod->longname, sizeof(pmod->longname) );
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%s", pmod->reference );
    goto_colon( NULL, fileread, bfalse );  pmod->quest_idsz = fget_idsz( fileread ); pmod->quest_level = fget_int( fileread );

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );
    pmod->importamount = iTmp;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pmod->allowexport = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pmod->allowexport = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pmod->minplayers = iTmp;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  pmod->maxplayers = iTmp;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    pmod->respawnvalid = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pmod->respawnvalid = btrue;
    if ( cTmp == 'A' || cTmp == 'a' )  pmod->respawnvalid = ANYTIME;

    goto_colon( NULL, fileread, bfalse );   //BAD: Skip line
  //  pmod->rtscontrol = bfalse;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%s", readtext );
    for ( iTmp = 0; iTmp < RANKSIZE - 1; iTmp++ )
    {
        pmod->rank[iTmp] = readtext[iTmp];
    }
    pmod->rank[iTmp] = '\0';

    // Read the summary
    cnt = 0;
    while ( cnt < SUMMARYLINES )
    {
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%255s", szLine );
        tnc = 0;

        cTmp = szLine[tnc];  if ( cTmp == '_' )  cTmp = ' ';

        while ( tnc < SUMMARYSIZE - 1 && cTmp != 0 )
        {
            pmod->summary[cnt][tnc] = cTmp;
            tnc++;

            cTmp = szLine[tnc];  if ( cTmp == '_' )  cTmp = ' ';
        }

        pmod->summary[cnt][tnc] = '\0';
        cnt++;
    }

    fclose(fileread);

    pmod->loaded = btrue;

    return pmod->loaded;
}

//--------------------------------------------------------------------------------------------
void modlist_load_all_info()
{
    STRING loadname;
    const char *FileName;

    // Search for all .mod directories and load the module info
    ModList_count = 0;
    FileName = fs_findFirstFile( "modules", "mod" );
    while ( NULL != FileName && ModList_count < MAXMODULE )
    {
        // save the filename
        sprintf( loadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", FileName );

        if ( module_load_info( loadname, ModList + ModList_count ) )
        {
            strncpy( ModList[ModList_count].loadname, FileName, MAXCAPNAMESIZE );

            ModList_count++;
        };

        FileName = fs_findNextFile();
    }
    fs_findClose();
    ModList[ModList_count].longname[0] = '\0';
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

    if ( imod < 0 || imod >= ModList_count ) return bfalse;

    if ( !module_instance_init(pinst) ) return bfalse;

    pdata = ModList + imod;

    pinst->importamount   = pdata->importamount;
    pinst->exportvalid    = pdata->allowexport;
    pinst->playeramount   = pdata->maxplayers;
    pinst->importvalid    = ( pinst->importamount > 0 );
    pinst->respawnvalid   = ( bfalse != pdata->respawnvalid );
    pinst->respawnanytime = ( ANYTIME == pdata->respawnvalid );
  //  pinst->rtscontrol     = bfalse;

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
    pinst->exportvalid = bfalse;
    pinst->seed        = seed;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t module_start( module_instance_t * pinst )
{
    // BB> Let the module go

    if (NULL == pinst) return bfalse;

    pinst->active = btrue;

    pinst->randsave = 0;
    srand( pinst->randsave );

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
