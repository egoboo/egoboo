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

#include "log.h"
#include "menu.h"
#include "sound.h"
#include "graphic.h"
#include "char.h"
#include "enchant.h"
#include "passage.h"
#include "input.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t load_module_info( FILE * fileread, mod_t * pmod );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int module_reference_matches(  const char *szLoadName, Uint32 idsz )
{
    // ZZ> This function returns btrue if the named module has the required IDSZ
    FILE *fileread;
    char newloadname[256];
    Uint32 newidsz;
    int foundidsz;
    int cnt;
    if ( idsz == IDSZ_NONE )
        return btrue;
    if ( szLoadName[0] == 'N' && szLoadName[1] == 'O' && szLoadName[2] == 'N' && szLoadName[3] == 'E' && szLoadName[4] == 0 )
        return bfalse;

    foundidsz = bfalse;
    sprintf( newloadname, "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", szLoadName );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread )
    {
        log_warning("Cannot open file! (%s)\n", newloadname);
        return bfalse;
    }
    else
    {
        // Read basic data
        parse_filename = szLoadName;
        goto_colon( fileread );  // Name of module...  Doesn't matter
        goto_colon( fileread );  // Reference directory...
        goto_colon( fileread );  // Reference IDSZ...
        goto_colon( fileread );  // Import...
        goto_colon( fileread );  // Export...
        goto_colon( fileread );  // Min players...
        goto_colon( fileread );  // Max players...
        goto_colon( fileread );  // Respawn...
        goto_colon( fileread );  // BAD! NOT USED
        goto_colon( fileread );  // Rank...

        // Summary...
        cnt = 0;

        while ( cnt < SUMMARYLINES )
        {
            goto_colon( fileread );
            cnt++;
        }

        // Now check expansions
        while ( goto_colon_yesno( fileread ) && !foundidsz )
        {
            newidsz = fget_idsz( fileread );
            if ( newidsz == idsz )
            {
                foundidsz = btrue;
            }
        }

        fclose( fileread );
    }

    return foundidsz;
}

//--------------------------------------------------------------------------------------------
void add_module_idsz(  const char *szLoadName, Uint32 idsz )
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
int find_module(  const char *smallname )
{
    // ZZ> This function returns -1 if the module does not exist locally, the module
    //     index otherwise

    int cnt, index;
    cnt = 0;
    index = -1;

    while ( cnt < ModList_count )
    {
        if ( strcmp( smallname, ModList[cnt].loadname ) == 0 )
        {
            index = cnt;
            cnt = ModList_count;
        }

        cnt++;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
bool_t load_valid_module( int modnumber,  const char *szLoadName )
{
    // ZZ> This function loads the module data file
    FILE  *fileread;
    IDSZ   idsz;
    int    iTmp;

    bool_t playerhasquest;
    Sint16 questlevel;

    mod_t * pmod;

    if ( modnumber >= MAXMODULE ) return bfalse;
    pmod = ModList + modnumber;

    fileread = fopen( szLoadName, "r" );
    if ( NULL == fileread ) return bfalse;
    parse_filename = szLoadName;

    // Read basic data
    goto_colon( fileread );  fget_name( fileread, pmod->longname, sizeof(pmod->longname) );
    goto_colon( fileread );  fscanf( fileread, "%s", pmod->reference );
    goto_colon( fileread );  idsz = fget_idsz( fileread ); fgetc(fileread); questlevel = fget_int( fileread );

    //Check all selected players directories !!TODO!!
    playerhasquest = bfalse;
    for ( iTmp = 0; iTmp < mnu_selectedPlayerCount; iTmp++ )
    {
        if ( questlevel <= check_player_quest( loadplayer[mnu_selectedPlayer[iTmp]].name, idsz ))
        {
            playerhasquest = btrue;
            break;
        }
    }

    //So, do we load the module or not?
    pmod->loaded = bfalse;
    if ( gDevMode || playerhasquest || module_reference_matches( pmod->reference, idsz ) )
    {
        parse_filename = szLoadName;
        load_module_info( fileread, pmod );
    }

    return pmod->loaded;
}

//--------------------------------------------------------------------------------------------
bool_t load_module_info( FILE * fileread, mod_t * pmod )
{
    // BB > this function actually reads in the module data

    STRING readtext, szLine;
    int cnt, tnc, iTmp;
    char cTmp;

    if ( NULL == fileread || NULL == pmod ) return bfalse;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    pmod->importamount = iTmp;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    pmod->allowexport = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pmod->allowexport = btrue;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  pmod->minplayers = iTmp;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  pmod->maxplayers = iTmp;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    pmod->respawnvalid = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pmod->respawnvalid = btrue;
    if ( cTmp == 'A' || cTmp == 'a' )  pmod->respawnvalid = ANYTIME;

    goto_colon( fileread );   //BAD: Skip line
    pmod->rtscontrol = bfalse;

    goto_colon( fileread );  fscanf( fileread, "%s", readtext );
    for ( iTmp = 0; iTmp < RANKSIZE - 1; iTmp++ )
    {
        pmod->rank[iTmp] = readtext[iTmp];
    }
    pmod->rank[iTmp] = '\0';

    // Read the summary
    cnt = 0;
    while ( cnt < SUMMARYLINES )
    {
        goto_colon( fileread );  fscanf( fileread, "%255s", szLine );
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

    pmod->loaded = btrue;

    return pmod->loaded;
}

