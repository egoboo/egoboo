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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void release_module( void )
{
    // ZZ> This function frees up memory used by the module
    release_all_textures();
    release_all_icons();
    release_map();

    // Close and then reopen SDL_mixer; it's easier than manually unloading each sound
    if ( musicvalid || soundvalid )
    {
        Mix_CloseAudio();
        songplaying = -1;
        Mix_OpenAudio( MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize );
        Mix_AllocateChannels( maxsoundchannel );
    }
}

//--------------------------------------------------------------------------------------------
int module_reference_matches( char *szLoadName, Uint32 idsz )
{
    // ZZ> This function returns btrue if the named module has the required IDSZ
    FILE *fileread;
    char newloadname[256];
    Uint32 newidsz;
    int foundidsz;
    int cnt;

    if ( idsz == IDSZNONE )
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
void add_module_idsz( char *szLoadName, Uint32 idsz )
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
int find_module( char *smallname )
{
    // ZZ> This function returns -1 if the module does not exist locally, the module
    //     index otherwise

    int cnt, index;
    cnt = 0;
    index = -1;

    while ( cnt < globalnummodule )
    {
        if ( strcmp( smallname, modloadname[cnt] ) == 0 )
        {
            index = cnt;
            cnt = globalnummodule;
        }

        cnt++;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
void load_module( char *smallname )
{
    // ZZ> This function loads a module
    STRING modname;

    // Load all the global icons
    if ( !load_all_global_icons() ) log_warning( "Could not load all global icons!\n" );

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
    reset_ai_script();
    load_ai_script( "basicdat" SLASH_STR "script.txt" );
    release_all_models();
    free_all_enchants();

    //Load all objects
    load_all_global_objects(load_all_objects(modname));   //First load global ones from the basicdat folder
    //Then override any using local objects found in the module folder

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
    load_font( "basicdat" SLASH_STR "font", "basicdat" SLASH_STR "font.txt" );
    load_bars( "basicdat" SLASH_STR "bars" );
    load_map( modname );

    log_madused( "slotused.txt" );

    // Start playing the damage tile sound silently...
    /*PORT
        play_sound_pvf_looped(damagetilesound, PANMID, VOLMIN, FRQDEFAULT);
    */
}

//--------------------------------------------------------------------------------------------
int load_all_objects( char *modname )
{
    // ZZ> This function loads a module's local objects and overrides the global ones already loaded
    const char *filehandle;
    bool_t keeplooking;
    char newloadname[256];
    char filename[256];
    FILE* fileread;
    int cnt;
    int skin;
    int importplayer;

    // Log all of the script errors
    parseerror = bfalse;

    //This overwrites existing loaded slots that are loaded globally
    overrideslots = btrue;

    // Clear the import slots...
    for ( cnt = 0; cnt < MAXMODEL; cnt++ )
        capimportslot[cnt] = 10000;

    // Load the import directory
    importplayer = -1;
    importobject = -100;
    skin = 8;  // Character skins start at 8...  Trust me

    if ( importvalid )
    {
        for ( cnt = 0; cnt < MAXIMPORT; cnt++ )
        {
            sprintf( filename, "import" SLASH_STR "temp%04d.obj", cnt );
            // Make sure the object exists...
            sprintf( newloadname, "%s" SLASH_STR "data.txt", filename );
            fileread = fopen( newloadname, "r" );

            if ( fileread )
            {

                fclose( fileread );

                // Load it...
                if ( ( cnt % 9 ) == 0 )
                {
                    importplayer++;
                }

                importobject = ( ( importplayer ) * 9 ) + ( cnt % 9 );
                capimportslot[importobject] = cnt;
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
    char newloadname[256];
    char filename[256];

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
int get_module_data( int modnumber, char *szLoadName )
{
    // ZZ> This function loads the module data file
    FILE *fileread;
    char reference[128];
    IDSZ idsz;
    Sint16 questlevel;
    char cTmp;
    int iTmp;
    bool_t playerhasquest = bfalse;

    fileread = fopen( szLoadName, "r" );

    if ( fileread )
    {
        // Read basic data
        parse_filename = szLoadName;
        goto_colon( fileread );  get_name( fileread, modlongname[modnumber] );
        goto_colon( fileread );  fscanf( fileread, "%s", reference );
        goto_colon( fileread );  idsz = get_idsz( fileread ); fgetc(fileread); questlevel = fget_int( fileread );

        //Check all selected players directories !!TODO!!
        playerhasquest = bfalse;
        iTmp = 0;

        while ( !playerhasquest && iTmp < numloadplayer )
        {
            if ( questlevel <= check_player_quest( loadplayername[selectedPlayer], idsz )) playerhasquest = btrue;

            iTmp++;
        }

        //So, do we load the module or not?
        if ( gDevMode || playerhasquest || module_reference_matches( reference, idsz ) )
        {
            parse_filename = szLoadName;
            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
            modimportamount[modnumber] = iTmp;
            goto_colon( fileread );  cTmp = get_first_letter( fileread );
            modallowexport[modnumber] = bfalse;

            if ( cTmp == 'T' || cTmp == 't' )  modallowexport[modnumber] = btrue;

            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  modminplayers[modnumber] = iTmp;
            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  modmaxplayers[modnumber] = iTmp;
            goto_colon( fileread );  cTmp = get_first_letter( fileread );
            modrespawnvalid[modnumber] = bfalse;

            if ( cTmp == 'T' || cTmp == 't' )  modrespawnvalid[modnumber] = btrue;
            if ( cTmp == 'A' || cTmp == 'a' )  modrespawnvalid[modnumber] = ANYTIME;

            goto_colon( fileread );   //BAD: Skip line
            modrtscontrol[modnumber] = bfalse;
            goto_colon( fileread );  fscanf( fileread, "%s", generictext );
            iTmp = 0;

            while ( iTmp < RANKSIZE - 1 )
            {
                modrank[modnumber][iTmp] = generictext[iTmp];
                iTmp++;
            }

            modrank[modnumber][iTmp] = 0;

            // Read the expansions
            return btrue;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
int get_module_summary( char *szLoadName )
{
    // ZZ> This function gets the quest description out of the module's menu file
    FILE *fileread;
    char cTmp;
    char szLine[160];
    int cnt;
    int tnc;
    bool_t result = bfalse;

    fileread = fopen( szLoadName, "r" );

    if ( fileread )
    {
        // Skip over basic data
        parse_filename = szLoadName;
        goto_colon( fileread );  // Name...
        goto_colon( fileread );  // Reference...
        goto_colon( fileread );  // IDSZ...
        goto_colon( fileread );  // Import...
        goto_colon( fileread );  // Export...
        goto_colon( fileread );  // Min players...
        goto_colon( fileread );  // Max players...
        goto_colon( fileread );  // Respawn...
        goto_colon( fileread );  // BAD! NOT USED
        goto_colon( fileread );  // Rank...

        // Read the summary
        cnt = 0;

        while ( cnt < SUMMARYLINES )
        {
            goto_colon( fileread );  fscanf( fileread, "%s", szLine );
            tnc = 0;

            cTmp = szLine[tnc];  if ( cTmp == '_' )  cTmp = ' ';

            while ( tnc < SUMMARYSIZE - 1 && cTmp != 0 )
            {
                modsummary[cnt][tnc] = cTmp;
                tnc++;

                cTmp = szLine[tnc];  if ( cTmp == '_' )  cTmp = ' ';
            }

            modsummary[cnt][tnc] = 0;
            cnt++;
        }

        result = btrue;
    }

    fclose( fileread );
    return result;
}
