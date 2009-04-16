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

#include "egoboo.h"
#include "log.h"
#include "menu.h"
#include "sound.h"
#include "graphic.h"
#include "enchant.h"
#include "passage.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t load_module_info( FILE * fileread, mod_t * pmod );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void release_module()
{
    // ZZ> This function frees up memory used by the module

    release_all_icons();
    release_all_titleimages();
    release_bars();
    release_blip();
    release_map();
    release_all_textures();
    release_all_models();

    // Close and then reopen SDL_mixer; it's easier than manually unloading each sound
    sound_restart();
}

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
            newidsz = get_idsz( fileread );
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
void load_module(  const char *smallname )
{
    // ZZ> This function loads a module
    STRING modname;

    log_info( "Loading module \"%s\"\n", smallname );

    // Load all the global icons
    if ( !load_all_global_icons() )
    {
        log_warning( "Could not load all global icons!\n" );
    }

    beatmodule = bfalse;
    timeron = bfalse;
    snprintf( modname, sizeof(modname), "modules" SLASH_STR "%s" SLASH_STR, smallname );
    make_randie();
    reset_teams();
    load_one_icon( "basicdat" SLASH_STR "nullicon" );  // This works (without transparency)

    load_global_waves( modname );

    reset_particles( modname );
    read_wawalite( modname );
    make_twist();
    reset_messages();
    prime_names();
    load_basic_textures( modname );  // This should work (without colorkey stuff)
    release_all_ai_scripts();
    load_ai_script( "basicdat" SLASH_STR "script.txt" );
    release_all_models();
    free_all_enchants();

    //Load all objects
    {
        int skin;
        skin = load_all_objects(modname);
        load_all_global_objects(skin);
    }
    if ( !load_mesh( modname ) )
    {
        log_error( "Uh oh! Problems loading the mesh! (%s)\n", modname );
    }

    setup_particles();
    setup_passage( modname );
    reset_players();

    setup_characters( modname );

    reset_end_text();
    setup_alliances( modname );

    // Load fonts and bars after other images, as not to hog videomem
    font_load( "basicdat" SLASH_STR "font", "basicdat" SLASH_STR "font.txt" );
    load_bars( "basicdat" SLASH_STR "bars" );
    load_map( modname );

    log_madused( "slotused.txt" );

    // Start playing the damage tile sound silently...
    /*PORT
        play_sound_pvf_looped(damagetilesound, PANMID, VOLMIN, FRQDEFAULT);
    */
}

//--------------------------------------------------------------------------------------------
int load_all_objects(  const char *modname )
{
    // ZZ> This function loads a module's local objects and overrides the global ones already loaded
    const char *filehandle;
    bool_t keeplooking;
    char newloadname[256];
    char filename[256];
    int cnt;
    int skin;
    int importplayer;

    // Log all of the script errors
    parseerror = bfalse;

    //This overwrites existing loaded slots that are loaded globally
    overrideslots = btrue;

    // Clear the import slots...
    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
    {
        CapList[cnt].importslot = 10000;
    }

    // Load the import directory
    importplayer = -1;
    importobject = -100;
    skin = TX_LAST;  // Character skins start after the last special texture
    if ( importvalid )
    {
        for ( cnt = 0; cnt < MAXIMPORT; cnt++ )
        {
            // Make sure the object exists...
            sprintf( filename, "import" SLASH_STR "temp%04d.obj", cnt );
            sprintf( newloadname, "%s" SLASH_STR "data.txt", filename );
            if ( fs_fileExists(newloadname) )
            {
                // new player found
                if ( 0 == ( cnt % MAXIMPORTPERPLAYER ) ) importplayer++;

                // store the slot info
                importobject = ( importplayer * MAXIMPORTPERPLAYER ) + ( cnt % MAXIMPORTPERPLAYER );
                CapList[importobject].importslot = cnt;

                // load it
                skin += load_one_object( skin, filename );
            }
        }
    }

    // Search for .obj directories and load them
    importobject = -100;
    make_newloadname( modname, "objects" SLASH_STR, newloadname );
    filehandle = fs_findFirstFile( newloadname, "obj" );

    keeplooking = btrue;
    if ( filehandle != NULL )
    {
        while ( keeplooking )
        {
            sprintf( filename, "%s%s", newloadname, filehandle );
            skin += load_one_object( skin, filename );

            filehandle = fs_findNextFile();

            keeplooking = ( filehandle != NULL );
        }
    }

    fs_findClose();
    return skin;
}

//--------------------------------------------------------------------------------------------
void load_all_global_objects(int skin)
{
    //ZF> This function loads all global objects found in the basicdat folder
    const char *filehandle;
    bool_t keeplooking;
    STRING newloadname;
    STRING filename;

    //Warn the user for any duplicate slots
    overrideslots = bfalse;

    // Search for .obj directories and load them
    sprintf( newloadname, "basicdat" SLASH_STR "globalobjects" SLASH_STR );
    filehandle = fs_findFirstFile( newloadname, "obj" );

    keeplooking = btrue;
    if ( filehandle != NULL )
    {
        while ( keeplooking )
        {
            sprintf( filename, "%s%s", newloadname, filehandle );
            skin += load_one_object( skin, filename );

            filehandle = fs_findNextFile();

            keeplooking = ( filehandle != NULL );
        }
    }

    fs_findClose();
}

//--------------------------------------------------------------------------------------------
bool_t load_valid_module( int modnumber,  const char *szLoadName )
{
    // ZZ> This function loads the module data file
    FILE  *fileread;
    STRING reference;
    IDSZ   idsz;
    int    iTmp;

    bool_t playerhasquest;
    Sint16 questlevel;

    mod_t * pmod;

    if( modnumber >= MAXMODULE ) return bfalse;
    pmod = ModList + modnumber;

    fileread = fopen( szLoadName, "r" );
    if ( NULL == fileread ) return bfalse;
    parse_filename = szLoadName;

    // Read basic data
    goto_colon( fileread );  get_name( fileread, pmod->longname );
    goto_colon( fileread );  fscanf( fileread, "%s", reference );
    goto_colon( fileread );  idsz = get_idsz( fileread ); fgetc(fileread); questlevel = fget_int( fileread );

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
    if ( gDevMode || playerhasquest || module_reference_matches( reference, idsz ) )
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

    if( NULL == fileread || NULL == pmod ) return bfalse;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    pmod->importamount = iTmp;

    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    pmod->allowexport = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  pmod->allowexport = btrue;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  pmod->minplayers = iTmp;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  pmod->maxplayers = iTmp;

    goto_colon( fileread );  cTmp = get_first_letter( fileread );
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
        goto_colon( fileread );  fscanf( fileread, "%s", szLine );
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
