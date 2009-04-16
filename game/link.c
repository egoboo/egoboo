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

#include "link.h"
#include "proto.h"
#include "egoboo.h"
#include "menu.h"
#include "log.h"
#include "graphic.h"
#include "camera.h"

Link_t LinkList[LINK_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t link_export_all()
{
    // BB> This function saves all the players to the players dir
    //     and also copies them into the imports dir to prepare for the next module

    bool_t is_local;
    int cnt, tnc, j, character, player;
    STRING srcPlayer, srcDir, destDir;

    // do the normal export to save all the player settings
    export_all_players( btrue );

    // reload all of the available players
    check_player_import( "players", btrue  );
    check_player_import( "remote",  bfalse );

    // build the import directory using the player info
    empty_import_directory();
    fs_createDirectory( "import" );

    // export all of the players directly from memory straight to the "import" dir
    for ( player = 0, cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ChrList[character].on || !ChrList[character].alive ) continue;

        is_local = ( 0 != PlaList[cnt].device );

        // find the saved copy of the players that are in memory right now
        for ( tnc = 0; tnc < loadplayer_count; tnc++ )
        {
            if ( 0 == strcmp( loadplayer[tnc].dir, get_file_path(ChrList[character].name) ) )
            {
                break;
            }
        }

        if ( tnc == loadplayer_count )
        {
            log_warning( "link_export_all() - cannot find exported file for \"%s\" (\"%s\") \n", ChrList[character].name, get_file_path(ChrList[character].name) ) ;
            continue;
        }

        // grab the controls from the currently loaded players
        // calculate the slot from the current player count
        local_control[player] = PlaList[cnt].device;
        local_slot[player]    = player * MAXIMPORTPERPLAYER;
        player++;

        // Copy the character to the import directory
        if ( is_local )
        {
            sprintf( srcPlayer, "players" SLASH_STR "%s", loadplayer[tnc].dir );
        }
        else
        {
            sprintf( srcPlayer, "remote" SLASH_STR "%s", loadplayer[tnc].dir );
        }

        sprintf( destDir, "import" SLASH_STR "temp%04d.obj", local_slot[tnc] );
        fs_copyDirectory( srcPlayer, destDir );

        // Copy all of the character's items to the import directory
        for ( j = 0; j < 8; j++ )
        {
            sprintf( srcDir, "%s" SLASH_STR "%d.obj", srcPlayer, j );
            sprintf( destDir, "import" SLASH_STR "temp%04d.obj", local_slot[tnc] + j + 1 );

            fs_copyDirectory( srcDir, destDir );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t link_follow_modname( const char * modname )
{
    // BB> This causes the game to follow a link, given the module name

    if ( NULL == modname || '\0' == modname[0] ) return bfalse;

    //-----------------------------------------------------------------
    // export all the local and remote characters
    {
        link_export_all();
    }

    //-----------------------------------------------------------------
    // all of the de-initialization code after the module actually ends
    {
        release_module();
        close_session();
    }

    // reset the "ui" mouse state
    pressed           = bfalse;
    clicked           = bfalse;
    pending_click     = bfalse;
    mouse_wheel_event = bfalse;

    // reset some of the module state variables
    beatmodule  = bfalse;
    exportvalid = bfalse;

    //-----------------------------------------------------------------
    // all of the initialization code before the module actually starts
    {
        //seed = time( NULL );
        srand( seed );

        // start the new module
        load_module( modname );

        make_onwhichfan();
        reset_camera(&gCamera);
        reset_timers();
        figure_out_what_to_draw();
        make_character_matrices();
        attach_particles();

        if ( networkon )
        {
            log_info( "SDL_main: Loading module %s...\n", pickedmodule );
            net_sayHello();
        }

        // Let the game go
        moduleactive = btrue;
        randsave = 0;
        srand( randsave );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t link_follow( Link_t list[], int ilink )
{
    // BB> This causes the game to follow a link

    Link_t * plink;
    if ( ilink < 0 || ilink >= LINK_COUNT ) return bfalse;

    plink = list + ilink;
    if ( !plink->valid ) return bfalse;

    // finish with the old module
    link_export_all();
    release_module();

    // start the new module
    load_module( plink->modname );

    pressed = bfalse;
    make_onwhichfan();
    reset_camera(&gCamera);
    reset_timers();
    figure_out_what_to_draw();
    make_character_matrices();
    attach_particles();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t link_build( const char * fname, Link_t list[] )
{
    FILE * pfile;
    int i;
    if ( NULL == fname || '\0' == fname ) return bfalse;

    pfile = fopen( fname, "r" );
    if ( NULL == pfile ) return bfalse;

    i = 0;

    while ( goto_colon_yesno( pfile ) && i < LINK_COUNT )
    {
        fscanf( pfile, "%s", list[i].modname );
        list[i].valid = btrue;
        i++;
    }

    return i > 0;
}
