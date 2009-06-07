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

/* Egoboo - game.c
*/

#define DECLARE_GLOBALS

#include "egoboo.h"

#include "clock.h"
#include "link.h"
#include "ui.h"
#include "font.h"
#include "log.h"
#include "System.h"
#include "script.h"
#include "sound.h"
#include "graphic.h"
#include "passage.h"
#include "enchant.h"
#include "input.h"
#include "menu.h"
#include "file_common.h"
#include "network.h"
#include "mad.h"
#include "mpd.h"

#include "char.h"
#include "particle.h"
#include "camera.h"
#include "id_md2.h"

#include "script_compile.h"
#include "script.h"

#include "egoboo_endian.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"

#include "SDL_extensions.h"

#include <SDL_image.h>

#include <time.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define ATTACH_NONE                      0
#define ATTACH_INVENTORY                 1
#define ATTACH_LEFT                      2
#define ATTACH_RIGHT                     3

#define CHR_MAX_COLLISIONS    512*16
#define COLLISION_HASH_NODES (CHR_MAX_COLLISIONS*2)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_chr_setup_info
{
    STRING spawn_name;
    char   *pname;
    Sint32 slot;
    float  x, y, z;
    int    passage, content, money, level, skin;
    bool_t stat;
    Uint8  team;
    Uint16 facing;
    Uint16 attach;
    Uint16 parent;
};
typedef struct s_chr_setup_info chr_setup_info_t;

//--------------------------------------------------------------------------------------------
// Bump List
struct s_bumplist
{
    Uint16  chr;                     // For character collisions
    Uint16  chrnum;                  // Number on the block
    Uint16  prt;                     // For particle collisions
    Uint16  prtnum;                  // Number on the block
};
typedef struct s_bumplist bumplist_t;

//--------------------------------------------------------------------------------------------
// pair-wise collision data

struct s_collision_data
{
  Uint16 chra, chrb;
  Uint16 prtb;
};

typedef struct s_collision_data co_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void make_randie();
static void reset_teams();
static void reset_messages();
static void reset_timers();
static void quit_game( void );

// looping - stuff called every loop - not accessible by scripts
static void do_enchant_spawn();
static void attach_particles();
static void check_stats();
static void tilt_characters_to_terrain();
static void bump_characters( void );
static void do_weather_spawn();
static void stat_return();
static void update_pits();
static void update_game();
static void update_timers();
static void make_onwhichfan( void );
static void set_local_latches( void );
static void make_onwhichfan( void );
static void let_all_characters_think();

// module initialization / deinitialization - not accessible by scripts
static bool_t load_module( const char *smallname );
static void   release_module();

static void   setup_characters( const char *modname );
static void   setup_alliances( const char *modname );
static int    load_one_object( const char* tmploadname , int skin );
static int    load_all_objects( const char *modname );
static void   load_all_global_objects(int skin);

static bool_t chr_setup_read( FILE * fileread, chr_setup_info_t *pinfo );
static bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo );
static void setup_characters( const char *modname );


// Model stuff
static void log_madused( const char *savename );

static bool_t game_begin_menu( int which );
static void   game_end_menu();

static void   memory_cleanUp(void);

// Collision stuff
static bool_t add_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );
static bool_t add_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );

static bool_t detect_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b );
static bool_t detect_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b );
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

char idsz_string[5] = { '\0' };

static int    gamemenu_depth = -1;
static bool_t game_escape_latch = bfalse;

static bumplist_t bumplist[MAXMESHFAN/16];

static int           chr_co_count = 0;
static hash_list_t * chr_co_list;

//--------------------------------------------------------------------------------------------
// Random Things-----------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local )
{
    // ZZ> This function exports a character
    int tnc = 0, profile;
    char fromdir[128];
    char todir[128];
    char fromfile[128];
    char tofile[128];
    char todirname[16];
    char todirfullname[64];

    // Don't export enchants
    disenchant_character( character );

    profile = ChrList[character].model;
    if ( ( CapList[profile].cancarrytonextmodule || !CapList[profile].isitem ) && exportvalid )
    {
        // TWINK_BO.OBJ
        sprintf(todirname, "%s", str_encode_path(ChrList[owner].name) );

        // Is it a character or an item?
        if ( owner != character )
        {
            // Item is a subdirectory of the owner directory...
            sprintf( todirfullname, "%s" SLASH_STR "%d.obj", todirname, number );
        }
        else
        {
            // Character directory
            sprintf( todirfullname, "%s", todirname );
        }

        // players/twink.obj or players/twink.obj/sword.obj
        if ( is_local )
        {
            sprintf( todir, "players" SLASH_STR "%s", todirfullname );
        }
        else
        {
            sprintf( todir, "remote" SLASH_STR "%s", todirfullname );
        }

        // modules/advent.mod/objects/advent.obj
        sprintf( fromdir, "%s", MadList[profile].name );

        // Delete all the old items
        if ( owner == character )
        {
            for ( tnc = 0; tnc < 8; tnc++ )
            {
                sprintf( tofile, "%s" SLASH_STR "%d.obj", todir, tnc );  /*.OBJ*/
                fs_removeDirectoryAndContents( tofile );
            }
        }

        // Make the directory
        fs_createDirectory( todir );

        // Build the DATA.TXT file
        sprintf( tofile, "%s" SLASH_STR "data.txt", todir );  /*DATA.TXT*/
        export_one_character_profile( tofile, character );

        // Build the SKIN.TXT file
        sprintf( tofile, "%s" SLASH_STR "skin.txt", todir );  /*SKIN.TXT*/
        export_one_character_skin( tofile, character );

        // Build the NAMING.TXT file
        sprintf( tofile, "%s" SLASH_STR "naming.txt", todir );  /*NAMING.TXT*/
        export_one_character_name( tofile, character );

        // Copy all of the misc. data files
        sprintf( fromfile, "%s" SLASH_STR "message.txt", fromdir );  /*MESSAGE.TXT*/
        sprintf( tofile, "%s" SLASH_STR "message.txt", todir );  /*MESSAGE.TXT*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "tris.md2", fromdir );  /*TRIS.MD2*/
        sprintf( tofile,   "%s" SLASH_STR "tris.md2", todir );  /*TRIS.MD2*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "copy.txt", fromdir );  /*COPY.TXT*/
        sprintf( tofile,   "%s" SLASH_STR "copy.txt", todir );  /*COPY.TXT*/
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "script.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "script.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "enchant.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "enchant.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "credits.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "credits.txt", todir );
        fs_copyFile( fromfile, tofile );
        sprintf( fromfile, "%s" SLASH_STR "quest.txt", fromdir );
        sprintf( tofile,   "%s" SLASH_STR "quest.txt", todir );
        fs_copyFile( fromfile, tofile );

        // Copy all of the particle files
        for ( tnc = 0; tnc < MAX_PIP_PER_PROFILE; tnc++ )
        {
            sprintf( fromfile, "%s" SLASH_STR "part%d.txt", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "part%d.txt", todir,   tnc );
            fs_copyFile( fromfile, tofile );
        }

        // Copy all of the sound files
        for ( tnc = 0; tnc < MAX_WAVE; tnc++ )
        {
            sprintf( fromfile, "%s" SLASH_STR "sound%d.wav", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "sound%d.wav", todir,   tnc );
            fs_copyFile( fromfile, tofile );

            sprintf( fromfile, "%s" SLASH_STR "sound%d.ogg", fromdir, tnc );
            sprintf( tofile,   "%s" SLASH_STR "sound%d.ogg", todir,   tnc );
            fs_copyFile( fromfile, tofile );
        }

        // Copy all of the image files (try to copy all supported formats too)
        
        for ( tnc = 0; tnc < MAXSKIN; tnc++ )
        {
            Uint8 type;

            for (type = 0; type < maxformattypes; type++)
            {
                sprintf( fromfile, "%s" SLASH_STR "tris%d%s", fromdir, tnc, TxFormatSupported[type] );
                sprintf( tofile,   "%s" SLASH_STR "tris%d%s", todir,   tnc, TxFormatSupported[type] );
                fs_copyFile( fromfile, tofile );
                sprintf( fromfile, "%s" SLASH_STR "icon%d%s", fromdir, tnc, TxFormatSupported[type] );
                sprintf( tofile,   "%s" SLASH_STR "icon%d%s", todir,   tnc, TxFormatSupported[type] );
                fs_copyFile( fromfile, tofile );
            }
        }
    }
}

//---------------------------------------------------------------------------------------------
void export_all_players( bool_t require_local )
{
    // ZZ> This function saves all the local players in the
    //     PLAYERS directory
    bool_t is_local;
    int cnt, character, item, number;
	
	//Stop if export isnt valid
	if(!exportvalid) return;

    // Check each player
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        is_local = ( 0 != PlaList[cnt].device );
        if ( require_local && !is_local ) continue;
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ChrList[character].on || !ChrList[character].alive ) continue;

        // Export the character
        number = 0;
        export_one_character( character, character, number, is_local );

        // Export the left hand item
        number = SLOT_LEFT;
        item = ChrList[character].holdingwhich[number];
        if ( item != MAX_CHR && ChrList[item].isitem )  export_one_character( item, character, number, is_local );

        // Export the right hand item
        number = SLOT_RIGHT;
        item = ChrList[character].holdingwhich[number];
        if ( item != MAX_CHR && ChrList[item].isitem )  export_one_character( item, character, number, is_local );

        // Export the inventory
        for ( number = 2, item = ChrList[character].nextinpack; 
              number < 8 && item != MAX_CHR; 
              item = ChrList[item].nextinpack )
        {
            if ( ChrList[item].isitem )
            {
                export_one_character( item, character, number++, is_local );
            }
        }
    }

}

//---------------------------------------------------------------------------------------------
void quit_module()
{
    // ZZ> This function forces a return to the menu
    moduleactive = bfalse;
    hostactive   = bfalse;

    game_update_imports();

    release_module();

    sound_fade_all();
}

//--------------------------------------------------------------------------------------------
void quit_game()
{
    // ZZ> This function exits the game entirely

    if ( gameactive )
    {
        gameactive = bfalse;
    }

    if ( moduleactive )
    {
        quit_module();
    }

    empty_import_directory();
}

//--------------------------------------------------------------------------------------------
void getadd( int min, int value, int max, int* valuetoadd )
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    int newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
void fgetadd( float min, float value, float max, float* valuetoadd )
{
    // ZZ> This function figures out what value to add should be in order
    //     to not overflow the min and max bounds
    float newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void log_madused( const char *savename )
{
    // ZZ> This is a debug function for checking model loads
    FILE* hFileWrite;
    int cnt;

    hFileWrite = fopen( savename, "w" );
    if ( hFileWrite )
    {
        fprintf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        fprintf( hFileWrite, "%d of %d frames used...\n", md2_loadframe, MAXFRAME );
        cnt = 0;

        while ( cnt < MAX_PROFILE )
        {
            fprintf( hFileWrite, "%3d %32s %s\n", cnt, CapList[cnt].classname, MadList[cnt].name );
            cnt++;
        }

        fclose( hFileWrite );
    }
}

//--------------------------------------------------------------------------------------------
void statlist_add( Uint16 character )
{
    // ZZ> This function adds a status display to the do list
    if ( numstat < MAXSTAT )
    {
        statlist[numstat] = character;
        ChrList[character].staton = btrue;
        numstat++;
    }
}

//--------------------------------------------------------------------------------------------
void statlist_move_to_top( Uint16 character )
{
    // ZZ> This function puts the character on top of the statlist
    int cnt, oldloc;

    // Find where it is
    oldloc = numstat;

    for ( cnt = 0; cnt < numstat; cnt++ )
    {
        if ( statlist[cnt] == character )
        {
            oldloc = cnt;
            cnt = numstat;
        }
    }

    // Change position
    if ( oldloc < numstat )
    {
        // Move all the lower ones up
        while ( oldloc > 0 )
        {
            oldloc--;
            statlist[oldloc+1] = statlist[oldloc];
        }

        // Put the character in the top slot
        statlist[0] = character;
    }
}

//--------------------------------------------------------------------------------------------
void statlist_sort()
{
    // ZZ> This function puts all of the local players on top of the statlist
    int cnt;

    for ( cnt = 0; cnt < numpla; cnt++ )
    {
        if ( PlaList[cnt].valid && PlaList[cnt].device != INPUT_BITS_NONE )
        {
            statlist_move_to_top( PlaList[cnt].index );
        }
    }
}
//--------------------------------------------------------------------------------------------
void chr_play_action( Uint16 character, Uint16 action, Uint8 actionready )
{
    // ZZ> This function starts a generic action for a character
    
    chr_t * pchr;
    mad_t * pmad;

    if( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    if( pchr->inst.imad > MAX_PROFILE || !MadList[pchr->inst.imad].used ) return;
    pmad = MadList + pchr->inst.imad;

    if ( pmad->actionvalid[action] )
    {
        pchr->nextaction = ACTION_DA;
        pchr->action = action;

        pchr->inst.lip = 0;
        pchr->inst.frame_lst = pchr->inst.frame_nxt;
        pchr->inst.frame_nxt = pmad->actionstart[pchr->action];
        pchr->actionready    = actionready;
    }
}

//--------------------------------------------------------------------------------------------
void chr_set_frame( Uint16 character, Uint16 action, int frame, Uint16 lip )
{
    // ZZ> This function sets the frame for a character explicitly...  This is used to
    //     rotate Tank turrets

    chr_t * pchr;
    mad_t * pmad;

    if( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    if( pchr->inst.imad > MAX_PROFILE || !MadList[pchr->inst.imad].used ) return;
    pmad = MadList + pchr->inst.imad;

    if ( pmad->actionvalid[action] )
    {
        int framesinaction, frame_stt, frame_end;

        pchr->nextaction = ACTION_DA;
        pchr->action = ACTION_DA;
        pchr->actionready = btrue;

        framesinaction = (pmad->actionend[action] - pmad->actionstart[action]) + 1;
        if( framesinaction <= 1 )
        {
            frame_stt = pmad->actionstart[action];
            frame_end = frame_stt;
        }
        else
        {
            frame = MIN(frame, framesinaction);
            frame_stt = pmad->actionstart[action] + frame;

			frame = MIN(frame+1, framesinaction);
            frame_end = frame_stt + 1;
        }

        pchr->inst.lip = ( lip << 6 );
        pchr->inst.frame_lst = frame_stt;
        pchr->inst.frame_nxt = frame_end;
    }
}

//--------------------------------------------------------------------------------------------
int generate_number( int numbase, int numrand )
{
    // ZZ> This function generates a random number
    int tmp = 0;

    tmp = numbase;
    if ( numrand > 0 )
    {
        tmp += ( rand() % numrand );
    }
    else
    {
        log_warning( "One of the data pairs is wrong! (%i and %i) Cannot be 0 or less.\n", numbase, numrand );
        numrand = numbase;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
void setup_alliances( const char *modname )
{
    // ZZ> This function reads the alliance file
    char newloadname[256];
    char szTemp[256];
    Uint8 teama, teamb;
    FILE *fileread;

    // Load the file
    make_newloadname( modname, "gamedat" SLASH_STR "alliance.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( fileread )
    {
        while ( goto_colon( NULL, fileread, btrue ) )
        {
            fscanf( fileread, "%s", szTemp );
            teama = ( szTemp[0] - 'A' ) % MAXTEAM;
            fscanf( fileread, "%s", szTemp );
            teamb = ( szTemp[0] - 'A' ) % MAXTEAM;
            TeamList[teama].hatesteam[teamb] = bfalse;
        }

        fclose( fileread );
    }
}


//--------------------------------------------------------------------------------------------
void update_game()
{
    // ZZ> This function does several iterations of character movements and such
    //     to keep the game in sync.
    //     This is the main game loop
    int cnt, numdead;

    // Check for all local players being dead
    local_allpladead = bfalse;
    local_seeinvisible = bfalse;
    local_seekurse = bfalse;

    cnt = 0;
    numdead = 0;

    while ( cnt < MAXPLAYER )
    {
        if ( PlaList[cnt].valid && PlaList[cnt].device != INPUT_BITS_NONE )
        {
            if ( !ChrList[PlaList[cnt].index].alive )
            {
                numdead++;
                if ( cfg.difficulty < GAME_HARD && local_allpladead && SDLKEYDOWN( SDLK_SPACE ) && respawnvalid && 0 == revivetimer )
                {
                    respawn_character( PlaList[cnt].index );
                    ChrList[cnt].experience *= EXPKEEP;  // Apply xp Penality
					if(cfg.difficulty > GAME_EASY) ChrList[cnt].money *= EXPKEEP;
                }
            }
            else
            {
                if ( ChrList[PlaList[cnt].index].canseeinvisible )
                {
                    local_seeinvisible = btrue;
                }
                if ( ChrList[PlaList[cnt].index].canseekurse )
                {
                    local_seekurse = btrue;
                }
            }
        }

        cnt++;
    }
    if ( numdead >= local_numlpla )
    {
        local_allpladead = btrue;
    }

    sv_talkToRemotes();

    // [claforte Jan 6th 2001]
    // TODO: Put that back in place once networking is functional.
    while ( clock_wld < clock_all && numplatimes > 0 )
    {
        // Important stuff to keep in sync
        srand( randsave );
        randsave = rand();

        resize_characters();
        keep_weapons_with_holders();
        let_all_characters_think();
        do_weather_spawn();
        do_enchant_spawn();
        unbuffer_player_latches();

        make_onwhichfan();
        move_characters();
        move_particles();
        make_character_matrices( update_wld != 0 );
        attach_particles();
        bump_characters();

        stat_return();
        update_pits();

        // Stuff for which sync doesn't matter
        animate_tiles();
        move_water();

        // Timers
        clock_wld  += UPDATE_SKIP;
        clock_stat += UPDATE_SKIP;

        //Reset the respawn timer
        if ( revivetimer > 0 )
        {
            revivetimer -= UPDATE_SKIP;
        }

        update_wld++;
    }

    if ( networkon && !rtscontrol )
    {
        if ( numplatimes == 0 )
        {
            // The remote ran out of messages, and is now twiddling its thumbs...
            // Make it go slower so it doesn't happen again
            clock_wld += 25;
        }
        if ( numplatimes > 3 && !hostactive )
        {
            // The host has too many messages, and is probably experiencing control
            // lag...  Speed it up so it gets closer to sync
            clock_wld -= 5;
        }
    }
}

//--------------------------------------------------------------------------------------------
void update_timers()
{
    // ZZ> This function updates the game timers
    clock_lst = clock_all;
    clock_all = SDL_GetTicks() - clock_stt;
    clock_fps += clock_all - clock_lst;
    if ( clock_fps >= TICKS_PER_SEC )
    {
        create_szfpstext( frame_fps );
        clock_fps = 0;
        frame_fps = 0;
    }
}

//--------------------------------------------------------------------------------------------
void reset_teams()
{
    // ZZ> This function makes everyone hate everyone else
    int teama, teamb;

    teama = 0;

    while ( teama < MAXTEAM )
    {
        // Make the team hate everyone
        teamb = 0;

        while ( teamb < MAXTEAM )
        {
            TeamList[teama].hatesteam[teamb] = btrue;
            teamb++;
        }

        // Make the team like itself
        TeamList[teama].hatesteam[teama] = bfalse;
        // Set defaults
        TeamList[teama].leader = NOLEADER;
        TeamList[teama].sissy = 0;
        TeamList[teama].morale = 0;
        teama++;
    }

    // Keep the null team neutral
    teama = 0;

    while ( teama < MAXTEAM )
    {
        TeamList[teama].hatesteam[NULLTEAM] = bfalse;
        TeamList[NULLTEAM].hatesteam[teama] = bfalse;
        teama++;
    }
}

//--------------------------------------------------------------------------------------------
void reset_messages()
{
    // ZZ> This makes messages safe to use
    int cnt;

    msgtotal = 0;
    msgtotalindex = 0;
    msgtimechange = 0;
    msgstart = 0;
    cnt = 0;

    while ( cnt < MAXMESSAGE )
    {
        msgtime[cnt] = 0;
        cnt++;
    }

    cnt = 0;

    while ( cnt < MAXTOTALMESSAGE )
    {
        msgindex[cnt] = 0;
        cnt++;
    }

    msgtext[0] = 0;
}

//--------------------------------------------------------------------------------------------
void make_randie()
{
    // ZZ> This function makes the random number table
    int tnc, cnt;

    // Fill in the basic values
    cnt = 0;

    while ( cnt < MAXRAND )
    {
        randie[cnt] = rand() << 1;
        cnt++;
    }

    // Keep adjusting those values
    tnc = 0;

    while ( tnc < 20 )
    {
        cnt = 0;

        while ( cnt < MAXRAND )
        {
            randie[cnt] += rand();
            cnt++;
        }

        tnc++;
    }

    // All done
    randindex = 0;
}

//--------------------------------------------------------------------------------------------
void reset_timers()
{
    // ZZ> This function resets the timers...
    clock_stt = SDL_GetTicks();
    clock_all = 0;
    clock_lst = 0;
    clock_wld = 0;
    clock_stat = 0;
    clock_pit = 0;  pitskill = pitsfall = bfalse;

    update_wld = 0;
    frame_all = 0;
    frame_fps = 0;
    outofsync = bfalse;
}


//--------------------------------------------------------------------------------------------
int game_do_menu( double frameDuration, bool_t needs_clear )
{
    int menuResult;

    // do menus
    if ( needs_clear )
    {
        glClear( GL_COLOR_BUFFER_BIT );
    };

    ui_beginFrame( frameDuration );
    {
        menuResult = doMenu( ( float )frameDuration );
        request_flip_pages();
    }
    ui_endFrame();

    return menuResult;
}

//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    // ZZ> This is where the program starts and all the high level stuff happens

    double frameDuration;
    int menuResult;
    int frame_next = 0, frame_now = 0;
    bool_t menu_was_active, menu_is_active;

    // Initialize logging first, so that we can use it everywhere.
    log_init();
    log_setLoggingLevel( 2 );

    // start initializing the various subsystems
    log_message( "Starting Egoboo " VERSION " ...\n" );

    sys_initialize();
    clk_init();
    fs_init();

    // read the "setup.txt" file
    if ( !setup_read( "setup.txt" ) )
    {
        log_error( "Could not find Setup.txt\n" );
    }

    // download the "setup.txt" values into the cfg struct
    setup_download( &cfg );

    // synchronoze the config values with the various game subsystems
    setup_synch( &cfg );

    log_info( "Initializing SDL_Image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL ); \
    GLSetup_SupportedFormats();

    scantag_read_all( "basicdat" SLASH_STR "scancode.txt" );
    input_settings_load( "controls.txt" );

    load_ai_codes( "basicdat" SLASH_STR "aicodes.txt" );
    load_action_names( "basicdat" SLASH_STR "actions.txt" );

    sdl_init();
    ogl_init();
    net_initialize();

    ui_initialize( "basicdat" SLASH_STR "Negatori.ttf", 24 );
    sound_initialize();

    // register the memory_cleanUp function to automatically run whenever the program exits
    atexit( memory_cleanUp );

    // Linking system
    log_info( "Initializing module linking... " );
    empty_import_directory();
    if ( link_build( "basicdat" SLASH_STR "link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failure!\n" );

    // Matrix init stuff (from remove.c)
    rotmeshtopside    = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHTOPSIDE / ( 1.33333f );
    rotmeshbottomside = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHBOTTOMSIDE / ( 1.33333f );
    rotmeshup         = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHUP / ( 1.33333f );
    rotmeshdown       = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHDOWN / ( 1.33333f );

    camera_new( &gCamera );

    // initialize all these structures
    init_all_icons();
    init_all_titleimages();
    init_bars();
    init_blip();
    init_map();
    init_all_textures();
    init_all_models();
    font_init();
    mesh_new( PMesh );

    // Load stuff into memory
    make_lightdirectionlookup();
    make_turntosin();
    make_enviro();
    tile_dictionary_load( tile_dict, MAXMESHTYPE );
    load_blip_bitmap();
    load_all_music_sounds();
    initMenus();        // Start the game menu

    // Let the normal OS mouse cursor work
    SDL_WM_GrabInput( SDL_GRAB_OFF );
    SDL_ShowCursor( btrue );

    // Network's temporarily disabled
    clk_frameStep();
    frameDuration = clk_getFrameDuration();
    gameactive = btrue;

    // load all module info at game initialization
    // this will not change unless a new module is downloaded for a network game?
    modlist_load_all_info();

    menuactive = btrue;
    gameactive = bfalse;
    menu_was_active = menu_is_active = (menuactive || gamemenuactive);
    while ( gameactive || menuactive )
    {
        menu_was_active = menu_is_active;
        menu_is_active = (menuactive || gamemenuactive);

        if ( menu_is_active )
        {
            // menu settings
            SDL_WM_GrabInput ( SDL_GRAB_OFF );
            SDL_ShowCursor( SDL_ENABLE  );
        }
        else
        {
            // in-game settings
            SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
			SDL_WM_GrabInput ( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
        }

        // Clock updates each frame
        clk_frameStep();
        frameDuration = clk_getFrameDuration();

        // do the panic button
        input_read();
        if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
        {
            menuactive = bfalse;
            gameactive = bfalse;
        }

        // Either run through the menus, or jump into the game
        if ( menuactive )
        {
            // Play the menu music
            sound_play_song( 0, 0, -1 );

            menuResult = game_do_menu( frameDuration, btrue);

            switch ( menuResult )
            {
                case MENU_SELECT:
                    // Go ahead and start the game
                    menuactive = bfalse;
                    gameactive = btrue;
                    networkon  = bfalse;
                    break;

                case MENU_QUIT:
                    // The user selected "Quit"
                    menuactive = bfalse;
                    gameactive = bfalse;
                    break;
            }
        }
        else if ( gameactive )
        {
            bool_t game_menu_is_active, game_menu_was_active;
            // Start a new module
            seed = time( NULL );

            moduleactive = game_begin_module( pickedmodule_name, seed );

            game_menu_was_active = game_menu_is_active = gamemenuactive;
            game_escape_latch = bfalse;
            while ( moduleactive )
            {
                game_menu_was_active = menu_is_active;
                game_menu_is_active = gamemenuactive;

                if ( game_menu_is_active )
                {
                    // menu settings
                    SDL_ShowCursor( SDL_ENABLE  );
					SDL_WM_GrabInput ( SDL_GRAB_OFF );
                }
                else
                {
                    // in-game settings
                    SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );
                    SDL_WM_GrabInput ( cfg.grab_mouse ? SDL_GRAB_ON : SDL_GRAB_OFF );
                }

                // This is the control loop
                input_read();
                if ( networkon && console_done )
                {
                    net_send_message();
                }

                //Check for screenshots
                if ( !SDLKEYDOWN( SDLK_F11 ) ) screenshotkeyready = btrue;
                if ( SDLKEYDOWN( SDLK_F11 ) && keyb.on && screenshotkeyready )
                {
                    if ( !dump_screenshot() )                // Take the shot, returns bfalse if failed
                    {
                        debug_message( "Error writing screenshot!" );
                        log_warning( "Error writing screenshot\n" );    // Log the error in log.txt
                    }

                    screenshotkeyready = bfalse;
                }

                // Do important things
                if ( !gamepaused || networkon )
                {
                    // start the console mode?
                    if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_MESSAGE ) )
                    {
                        // reset the keyboard buffer
                        SDL_EnableKeyRepeat(20, SDL_DEFAULT_REPEAT_DELAY);
                        console_mode = btrue;
                        console_done = bfalse;
                        keyb.buffer_count = 0;
                        keyb.buffer[0] = '\0';
                    }

                    check_stats();
                    set_local_latches();
                    update_timers();
                    check_passage_music();

                    // NETWORK PORT
                    listen_for_packets();
                    if ( !waitingforplayers )
                    {
                        cl_talkToHost();
                        update_game();
                    }
                    else
                    {
                        clock_wld = clock_all;
                    }
                }
                else
                {
                    update_timers();
                    clock_wld = clock_all;
                }

                // Do the display stuff
                frame_now = SDL_GetTicks();
                if (frame_now > frame_next)
                {
                    float  frameskip = (float)TICKS_PER_SEC / (float)cfg.framelimit;
                    frame_next = frame_now + frameskip; //FPS limit

                    camera_move(&gCamera);
                    draw_main();

                    if ( gamemenuactive )
                    {
                        game_do_menu( frameDuration, bfalse );

                        if ( mnu_get_menu_depth() <= gamemenu_depth )
                        {
                            mnu_draw_background = btrue;
                            gamemenuactive = bfalse;
                            game_escape_latch = bfalse;
                            gamemenu_depth = -1;
                        }
                    }

                    msgtimechange++;
                    if ( statdelay > 0 )  statdelay--;
                }

                // Check for quitters
                // :TODO: local_noplayers is not set correctly
                //if( local_noplayers  )
                //{
                //   game_escape_requested  = btrue;
                //}

                if( game_escape_requested )
                {
                    game_escape_requested = bfalse;

                    if ( !game_escape_latch )
                    {
                        if( beatmodule )
                        {
                            game_begin_menu( ShowEndgame );
                        }
                        else
                        {
                            game_begin_menu( GamePaused );
                        }

                        game_escape_latch  = btrue;
                    }
                }

                do_flip_pages();
            }

            game_quit_module();
        }


        do_flip_pages();
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void memory_cleanUp(void)
{
    //ZF> This function releases all loaded things in memory and cleans up everything properly

    log_info("memory_cleanUp() - Attempting to clean up loaded things in memory... ");

    // quit any existing game
    quit_game();

    // synchronoze the config values with the various game subsystems
    setup_synch( &cfg );

    // quit the setup system, making sure that the setup file is written
    setup_upload( &cfg );
    setup_write();
    setup_quit();

    // make sure that the current control configuration is written
    input_settings_save( "controls.txt" );

    // shut down the ui
    ui_shutdown();

    // shut down the network
    if (networkon)
    {
        net_shutDown();
    }

    // shut down the clock services
    clk_shutdown();

    log_message("Success!\n");
    log_info( "Exiting Egoboo " VERSION " the good way...\n" );

    // shut down the log services
    log_shutdown();
}

//--------------------------------------------------------------------------------------------
int load_one_object( const char* tmploadname, int skin )
{
    // ZZ> This function loads one object and returns the number of skins
    int object;
    int numskins;
    STRING newloadname;

    // Load the object data file and get the object number
    object = load_one_character_profile( tmploadname );

    if( !VALID_CAP(object) ) return 0;  // no skins for an invalid object

    // Load the model for this object
    numskins = load_one_model_profile( tmploadname, object, skin );

    // Load the enchantment for this object
    make_newloadname( tmploadname, SLASH_STR "enchant.txt", newloadname );
    load_one_enchant_profile( newloadname, object );

    // Fix lighting if need be
    if ( CapList[object].uniformlit )
    {
        mad_make_equally_lit( object );
    }

    return numskins;
}

//--------------------------------------------------------------------------------------------
Uint16 get_particle_target( float pos_x, float pos_y, float pos_z, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget )
{
    //ZF> This is the new improved targeting system for particles. Also includes distance in the Z direction.
    Uint16 besttarget = MAX_CHR, cnt;
    Uint16 longdist = WIDE;

    for (cnt = 0; cnt < MAX_CHR; cnt++)
    {
        if (ChrList[cnt].on && ChrList[cnt].alive && !ChrList[cnt].isitem && ChrList[cnt].attachedto == MAX_CHR
                && !ChrList[cnt].invictus)
        {
            if ((PipList[particletype].onlydamagefriendly && team == ChrList[cnt].team) || (!PipList[particletype].onlydamagefriendly && TeamList[team].hatesteam[ChrList[cnt].team]) )
            {
                //Don't retarget someone we already had or not supposed to target
                if (cnt != oldtarget && cnt != donttarget)
                {
                    Uint16 angle = (ATAN2( ChrList[cnt].pos.y - pos_y, ChrList[cnt].pos.x - pos_x ) * 0xFFFF / ( TWO_PI ))
                                   + BEHIND - facing;

                    //Only proceed if we are facing the target
                    if (angle < PipList[particletype].targetangle || angle > ( 0xFFFF - PipList[particletype].targetangle ) )
                    {
                        Uint32 dist = ( Uint32 ) SQRT(ABS( pow(ChrList[cnt].pos.x - pos_x, 2))
                                                      + ABS( pow(ChrList[cnt].pos.y - pos_y, 2))
                                                      + ABS( pow(ChrList[cnt].pos.z - pos_z, 2)) );
                        if (dist < longdist && dist <= WIDE )
                        {
                            glouseangle = angle;
                            besttarget = cnt;
                            longdist = dist;
                        }
                    }
                }
            }
        }
    }

    //All done
    return besttarget;
}
//--------------------------------------------------------------------------------------------
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz )
{
    //ZF> This is the new improved AI targeting system. Also includes distance in the Z direction.
    //If maxdistance is 0 then it searches without a max limit.
    Uint16 besttarget;
    int cnt;
    float longdist2, maxdistance2 = maxdistance * maxdistance;

    if (team == NONE) return MAX_CHR;

    besttarget = MAX_CHR;
    longdist2 = 0;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        bool_t is_hated, hates_me;
        bool_t is_friend, is_prey, is_predator, is_mutual;

        //Skip non-existing objects, held objects and self
		if ( !ChrList[cnt].on || VALID_CHR(ChrList[cnt].attachedto) || ChrList[cnt].inpack || cnt == character || ChrList[character].attachedto == cnt ) continue;

        //Target items
        if ( !targetitems && ( ChrList[cnt].isitem || ChrList[cnt].invictus ) ) continue;

        //Either only target dead stuff or alive stuff
        if ( targetdead == ChrList[cnt].alive ) continue;

        //Dont target hostile invisible stuff, unless we can actually see them
        if ( !ChrList[character].canseeinvisible && FF_MUL( ChrList[cnt].inst.alpha, ChrList[cnt].inst.max_light ) < INVISIBLE ) continue;

        is_hated = TeamList[ChrList[character].team].hatesteam[ChrList[cnt].team];
        hates_me = TeamList[ChrList[cnt].team].hatesteam[ChrList[character].team];

        is_friend    = !is_hated && !hates_me;
        is_prey      = is_hated && !hates_me;
        is_predator  = !is_hated && hates_me;
        is_mutual    = is_hated && hates_me;

        //Which team to target
        if ( team == ALL || (team == ENEMY && is_hated) || (team == FRIEND && !is_hated) )
        {
            bool_t match_idsz = (idsz == CapList[ChrList[cnt].model].idsz[IDSZ_PARENT] ) || ( idsz == CapList[ChrList[cnt].model].idsz[IDSZ_TYPE] ); 

            //Check for specific IDSZ too?
            if ( idsz == IDSZ_NONE || ( excludeidsz != match_idsz ) )
            {
                float dx, dy, dz;
                float dist2;

                dx = ChrList[cnt].pos.x - ChrList[character].pos.x;
                dy = ChrList[cnt].pos.y - ChrList[character].pos.y;
                dz = ChrList[cnt].pos.z - ChrList[character].pos.z;

                dist2 = dx * dx + dy * dy + dz * dz;
                if ( (MAX_CHR == besttarget || dist2 < longdist2) && (maxdistance == 0 || dist2 <= maxdistance2) )
                {
                    besttarget = cnt;
                    longdist2 = dist2;
                }
            }
        }
    }

	//Target the holder if there is nothing better to target
	if(besttarget == MAX_CHR && VALID_CHR(ChrList[character].attachedto) && (team == ALL || team == FRIEND) && ChrList[ChrList[character].attachedto].alive)
	{
		besttarget = ChrList[character].attachedto;
	}

    //Now set the target
    if (besttarget != MAX_CHR)
    {
        ChrList[character].ai.target = besttarget;
    }

    return besttarget;
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan( void )
{
    // ZZ> This function figures out which fan characters are on and sets their level
    Uint16 character, distance, particle;
    int ripand;
    // int volume;
    float level;

    // First figure out which fan each character is in
    for ( character = 0; character < MAX_CHR; character++ )
    {
        Uint8 hide;
        Uint16 icap;
        if ( !ChrList[character].on ) continue;

        ChrList[character].onwhichfan   = mesh_get_tile ( PMesh, ChrList[character].pos.x, ChrList[character].pos.y );
        ChrList[character].onwhichblock = mesh_get_block( PMesh, ChrList[character].pos.x, ChrList[character].pos.y );

        // reject characters that are hidden
        icap = ChrList[character].model;
        hide = CapList[ icap ].hidestate;
        ChrList[character].is_hidden = bfalse;
        if( hide != NOHIDE && hide == ChrList[character].ai.state )
        {
            ChrList[character].is_hidden = btrue;
        }
    }

    // Get levels every update
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList[character].on || ChrList[character].inpack ) continue;

        level = get_level( PMesh, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].waterwalk ) + RAISE;

        if ( ChrList[character].alive )
        {
            if ( VALID_TILE( PMesh, ChrList[character].onwhichfan ) && 
                 ( 0 != mesh_test_fx( PMesh, ChrList[character].onwhichfan, MPDFX_DAMAGE ) ) && 
                 ( ChrList[character].pos.z <= ChrList[character].floor_level + DAMAGERAISE ) && 
                 ( MAX_CHR == ChrList[character].attachedto ) )
            {
                if ( ( ChrList[character].damagemodifier[damagetiletype]&DAMAGESHIFT ) != 3 && !ChrList[character].invictus ) // 3 means they're pretty well immune
                {
                    distance = ABS( gCamera.track_pos.x - ChrList[character].pos.x ) + ABS( gCamera.track_pos.y - ChrList[character].pos.y );
                    if ( distance < damagetilemindistance )
                    {
                        damagetilemindistance = distance;
                    }
                    if ( distance < damagetilemindistance + 256 )
                    {
                        damagetilesoundtime = 0;
                    }
                    if ( ChrList[character].damagetime == 0 )
                    {
                        damage_character( character, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, ChrList[character].ai.bumplast, DAMFX_NBLOC | DAMFX_ARMO, bfalse );
                        ChrList[character].damagetime = DAMAGETILETIME;
                    }
                    if ( (damagetileparttype != ((Sint16)~0)) && ( update_wld&damagetilepartand ) == 0 )
                    {
                        spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z,
                                            0, MAX_PROFILE, damagetileparttype, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
                    }
                }
                if ( ChrList[character].reaffirmdamagetype == damagetiletype )
                {
                    if ( ( update_wld&TILEREAFFIRMAND ) == 0 )
                        reaffirm_attached_particles( character );
                }
            }
        }

        if ( ChrList[character].pos.z < watersurfacelevel && (0 != mesh_test_fx( PMesh, ChrList[character].onwhichfan, MPDFX_WATER )) )
        {
            if ( !ChrList[character].inwater )
            {
                // Splash
                if ( INVALID_CHR( ChrList[character].attachedto ) )
                {
                    spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, watersurfacelevel + RAISE,
                                        0, MAX_PROFILE, SPLASH, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
                }

                if ( wateriswater )
                {
                    ChrList[character].ai.alert |= ALERTIF_INWATER;
                }
            }
            else
            {
                // Ripple
                if ( ChrList[character].pos.z > watersurfacelevel - RIPPLETOLERANCE && CapList[ChrList[character].model].ripple )
                {
                    // Ripples
                    ripand = ( ( int )ChrList[character].vel.x != 0 ) | ( ( int )ChrList[character].vel.y != 0 );
                    ripand = RIPPLEAND >> ripand;
                    if ( ( update_wld&ripand ) == 0 && ChrList[character].pos.z < watersurfacelevel && ChrList[character].alive )
                    {
                        spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, watersurfacelevel,
                                            0, MAX_PROFILE, RIPPLE, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
                    }
                }
                if ( wateriswater && 0 == ( frame_all & 7 ) )
                {
                    ChrList[character].jumpready = btrue;
                    ChrList[character].jumpnumber = 1; // ChrList[character].jumpnumberreset;
                }
            }

            ChrList[character].inwater  = btrue;
        }
        else
        {
            ChrList[character].inwater = bfalse;
        }

        ChrList[character].floor_level = level;
    }



    for ( particle = 0; particle < maxparticles; particle++ )
    {
        Uint16 ichr;
        if ( !PrtList[particle].on ) continue;

        PrtList[particle].onwhichfan   = mesh_get_tile ( PMesh, PrtList[particle].pos.x, PrtList[particle].pos.y );
        PrtList[particle].onwhichblock = mesh_get_block( PMesh, PrtList[particle].pos.x, PrtList[particle].pos.y );
        PrtList[particle].floor_level  = get_level( PMesh, PrtList[character].pos.x, PrtList[character].pos.y, bfalse );

        // reject particles that are hidden
        PrtList[particle].is_hidden = bfalse;
        ichr = PrtList[particle].attachedtocharacter;
        if( VALID_CHR( ichr ) )
        {
            PrtList[particle].is_hidden = ChrList[ichr].is_hidden;
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir( Uint16 majordir, Uint16 minordir )
{
    // ZZ> This function returns a direction between the major and minor ones, closer
    //     to the major.
    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;
    if ( minordir > 32768 )
    {
        temp = 0xFFFF;
        minordir = ( minordir + ( temp << 3 ) - temp ) >> 3;
        minordir += majordir;
        return minordir;
    }

    temp = 0;
    minordir = ( minordir + ( temp << 3 ) - temp ) >> 3;
    minordir += majordir;
    return minordir;
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir )
{
    // ZZ> This function returns a direction between the major and minor ones, closer
    //     to the major, but not by much.  Makes turning faster.
    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;
    if ( minordir > 32768 )
    {
        temp = 0xFFFF;
        minordir = ( minordir + ( temp << 1 ) - temp ) >> 1;
        minordir += majordir;
        return minordir;
    }

    temp = 0;
    minordir = ( minordir + ( temp << 1 ) - temp ) >> 1;
    minordir += majordir;
    return minordir;
}

//--------------------------------------------------------------------------------------------
void do_enchant_spawn()
{
    // ZZ> This function lets enchantments spawn particles

    int cnt, tnc;
    Uint16 facing, eve, character;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        if ( !EncList[cnt].on ) continue;

        eve = EncList[cnt].eve;
        if ( EveList[eve].contspawnamount > 0 )
        {
            EncList[cnt].spawntime--;
            if ( EncList[cnt].spawntime == 0 )
            {
                character = EncList[cnt].target;
                EncList[cnt].spawntime = EveList[eve].contspawntime;
                facing = ChrList[character].turn_z;
                for ( tnc = 0; tnc < EveList[eve].contspawnamount; tnc++ )
                {
                    spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z,
                                        facing, eve, EveList[eve].contspawnpip,
                                        MAX_CHR, GRIP_LAST, ChrList[EncList[cnt].owner].team, EncList[cnt].owner, tnc, MAX_CHR );
                    facing += EveList[eve].contspawnfacingadd;
                }
            }
        }
    }
}
//--------------------------------------------------------------------------------------------
void update_pits()
{
    // ZZ> This function kills any character in a deep pit...
    int cnt;
    if ( pitskill || pitsfall )
    {
        if ( clock_pit > 19 )
        {
            clock_pit = 0;

            // Kill any particles that fell in a pit, if they die in water...
            cnt = 0;

            while ( cnt < maxparticles )
            {
                if ( PrtList[cnt].on )
                {
                    if ( PrtList[cnt].pos.z < PITDEPTH && PipList[PrtList[cnt].pip].endwater )
                    {
                        PrtList[cnt]._time  = frame_all + 1;
                        PrtList[cnt].poofme = btrue; 
                    }
                }

                cnt++;
            }

            // Kill or teleport any characters that fell in a pit...
            cnt = 0;

            while ( cnt < MAX_CHR )
            {
                if ( ChrList[cnt].on && ChrList[cnt].alive && !ChrList[cnt].inpack )
                {
                    if ( !ChrList[cnt].invictus && ChrList[cnt].pos.z < PITDEPTH && ChrList[cnt].attachedto == MAX_CHR )
                    {
                        //Do we kill it?
                        if (pitskill)
                        {
                            // Got one!
                            kill_character( cnt, MAX_CHR );
                            ChrList[cnt].vel.x = 0;
                            ChrList[cnt].vel.y = 0;

                            //Play sound effect
                            sound_play_chunk( ChrList[cnt].pos, g_wavelist[GSND_PITFALL] );
                        }

                        //Do we teleport it?
                        if (pitsfall && ChrList[cnt].pos.z < PITDEPTH*8)
                        {
                            float nrm[2];

                            // Yeah!  It worked!
                            detach_character_from_mount( cnt, btrue, bfalse );
                            ChrList[cnt].pos_old.x = ChrList[cnt].pos.x;
                            ChrList[cnt].pos_old.y = ChrList[cnt].pos.y;
                            ChrList[cnt].pos.x = pitx;
                            ChrList[cnt].pos.y = pity;
                            ChrList[cnt].pos.z = pitz;
                            if ( __chrhitawall( cnt, nrm ) )
                            {
                                // No it didn't...
                                ChrList[cnt].pos = ChrList[cnt].pos_safe;
                                ChrList[cnt].vel = ChrList[cnt].vel_old;

                                // Kill it instead
                                kill_character( cnt, MAX_CHR );
                                ChrList[cnt].vel.x = 0;
                                ChrList[cnt].vel.y = 0;
                            }
                            else
                            {
                                ChrList[cnt].pos_safe = ChrList[cnt].pos;
                                ChrList[cnt].pos_old  = ChrList[cnt].pos;
                                ChrList[cnt].vel_old  = ChrList[cnt].vel;

                                //Stop movement
                                ChrList[cnt].vel.z = 0;
                                ChrList[cnt].vel.x = 0;
                                ChrList[cnt].vel.y = 0;

                                //Play sound effect
                                if (ChrList[cnt].isplayer)
                                {
                                    sound_play_chunk( gCamera.track_pos, g_wavelist[GSND_PITFALL] );
                                }
                                else
                                {
                                    sound_play_chunk( ChrList[cnt].pos, g_wavelist[GSND_PITFALL] );
                                }

                                //Do some damage (same as damage tile)
                                damage_character( cnt, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, ChrList[cnt].ai.bumplast, DAMFX_NBLOC | DAMFX_ARMO, btrue );
                            }
                        }
                    }

                }

                cnt++;
            }
        }
        else
        {
            clock_pit++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn()
{
    // ZZ> This function drops snowflakes or rain or whatever, also swings the camera
    int particle, cnt;
    float x, y, z;
    bool_t foundone;
    if ( weathertime > 0 )
    {
        weathertime--;
        if ( weathertime == 0 )
        {
            weathertime = weathertimereset;

            // Find a valid player
            foundone = bfalse;
            cnt = 0;

            while ( cnt < MAXPLAYER )
            {
                weatherplayer = ( weatherplayer + 1 ) & ( MAXPLAYER - 1 );
                if ( PlaList[weatherplayer].valid )
                {
                    foundone = btrue;
                    break;
                }
                cnt++;
            }

            // Did we find one?
            if ( foundone )
            {
                // Yes, but is the character valid?
                cnt = PlaList[weatherplayer].index;
                if ( ChrList[cnt].on && !ChrList[cnt].inpack )
                {
                    // Yes, so spawn over that character
                    x = ChrList[cnt].pos.x;
                    y = ChrList[cnt].pos.y;
                    z = ChrList[cnt].pos.z;
                    particle = spawn_one_particle( x, y, z, 0, MAX_PROFILE, WEATHER4, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
                    
					if(particle != TOTAL_MAX_PRT)
					{
						if(__prthitawall( particle ) ) free_one_particle( particle );
						else if ( weatheroverwater )
						{
							if ( !prt_is_over_water( particle ) )
							{
								free_one_particle( particle );
							}
						}
					}
					
                }
            }
        }
    }

    gCamera.swing = ( gCamera.swing + gCamera.swingrate ) & 16383;
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch( Uint16 player )
{
    // ZZ> This function converts input readings to latch settings, so players can
    //     move around
    float newx, newy;
    Uint16 turnsin, character;
    Uint8 device;
    float dist, scale;
    float inputx, inputy;

    // Check to see if we need to bother
    if ( PlaList[player].valid && PlaList[player].device != INPUT_BITS_NONE )
    {
        // Make life easier
        character = PlaList[player].index;
        device = PlaList[player].device;

        // Clear the player's latch buffers
        PlaList[player].latchbutton = 0;
        PlaList[player].latchx = 0;
        PlaList[player].latchy = 0;

        // Mouse routines
        if ( ( device & INPUT_BITS_MOUSE ) && mous.on )
        {
            // Movement
            newx = 0;
            newy = 0;
            if ( ( gCamera.turn_mode == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
            {
                dist = SQRT( mous.x * mous.x + mous.y * mous.y );
                if ( dist > 0 )
                {
                    scale = mous.sense / dist;
                    if ( dist < mous.sense )
                    {
                        scale = dist / mous.sense;
                    }

                    scale = scale / mous.sense;
                    inputx = mous.x * scale;
                    inputy = mous.y * scale;

                    turnsin = gCamera.turn_z >> 2;
                    turnsin = turnsin & 16383;
                    if ( gCamera.turn_mode == 255 &&
                            local_numlpla == 1 &&
                            control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) == 0 )  inputx = 0;

                    newx = ( inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
                    newy = (-inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
                    //                    PlaList[player].latchx+=newx;
                    //                    PlaList[player].latchy+=newy;
                }
            }

            PlaList[player].latchx += newx * mous.cover + mous.latcholdx * mous.sustain;
            PlaList[player].latchy += newy * mous.cover + mous.latcholdy * mous.sustain;
            mous.latcholdx = PlaList[player].latchx;
            mous.latcholdy = PlaList[player].latchy;

            // Sustain old movements to ease mouse play
            //            PlaList[player].latchx+=mous.latcholdx*mous.sustain;
            //            PlaList[player].latchy+=mous.latcholdy*mous.sustain;
            //            mous.latcholdx = PlaList[player].latchx;
            //            mous.latcholdy = PlaList[player].latchy;
            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }

        // Joystick A routines
        if ( ( device & INPUT_BITS_JOYA ) && joy[0].on )
        {
            // Movement
            if ( ( gCamera.turn_mode == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[0].x * joy[0].x + joy[0].y * joy[0].y );
                if ( dist > 0 )
                {
                    scale = 1.0f / dist;
                    inputx = joy[0].x * scale;
                    inputy = joy[0].y * scale;
                }

                turnsin = gCamera.turn_z >> 2;
                turnsin = turnsin & 16383;
                if ( gCamera.turn_mode == 255 &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
                newy = ( -inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
                PlaList[player].latchx += newx;
                PlaList[player].latchy += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }

        // Joystick B routines
        if ( ( device & INPUT_BITS_JOYB ) && joy[1].on )
        {
            // Movement
            if ( ( gCamera.turn_mode == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[1].x * joy[1].x + joy[1].y * joy[1].y );
                if ( dist > 0 )
                {
                    scale = 1.0f / dist;
                    inputx = joy[1].x * scale;
                    inputy = joy[1].y * scale;
                }

                turnsin = gCamera.turn_z >> 2;
                turnsin = turnsin & 16383;
                if ( gCamera.turn_mode == 255 &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
                newy = ( -inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
                PlaList[player].latchx += newx;
                PlaList[player].latchy += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }

        // Keyboard routines
        if ( ( device & INPUT_BITS_KEYBOARD ) && keyb.on )
        {
            // Movement
            if ( ChrList[character].attachedto != MAX_CHR )
            {
                // Mounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) );
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) );
            }
            else
            {
                // Unmounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) );
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) );
            }

            turnsin = gCamera.turn_z >> 2;
            turnsin = turnsin & 16383;
            if ( gCamera.turn_mode == 255 && local_numlpla == 1 )  inputx = 0;

            newx = (  inputx * turntocos[turnsin & TRIG_TABLE_MASK ] + inputy * turntosin[turnsin & TRIG_TABLE_MASK ] );
            newy = ( -inputx * turntosin[turnsin & TRIG_TABLE_MASK ] + inputy * turntocos[turnsin & TRIG_TABLE_MASK ] );
            PlaList[player].latchx += newx;
            PlaList[player].latchy += newy;

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_JUMP ) )
                PlaList[player].latchbutton |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_USE ) )
                PlaList[player].latchbutton |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_GET ) )
                PlaList[player].latchbutton |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_PACK ) )
                PlaList[player].latchbutton |= LATCHBUTTON_PACKRIGHT;
        }
    }
}

//--------------------------------------------------------------------------------------------
void set_local_latches( void )
{
    // ZZ> This function emulates AI thinkin' by setting latches from input devices
    int cnt;

    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        set_one_player_latch( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void prime_names()
{
    // ZZ> This function prepares the name chopper for use
    int cnt, tnc;

    chop.count = 0;
    chop.carat = 0;
    cnt = 0;

    while ( cnt < MAX_PROFILE )
    {
        tnc = 0;

        while ( tnc < MAXSECTION )
        {
            CapList[cnt].chop_sectionstart[tnc] = MAXCHOP;
            CapList[cnt].chop_sectionsize[tnc] = 0;
            tnc++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void check_stats()
{
    // ZZ> This function lets the players check character stats

    static int stat_check_timer = 0;
    static int stat_check_delay = 0;

    int ticks;
    if ( console_mode ) return;

    ticks = SDL_GetTicks();
    if ( ticks > stat_check_timer + 20 )
    {
        stat_check_timer = ticks;
    }

    stat_check_delay -= 20;
    if ( stat_check_delay > 0 )
        return;

    // !!!BAD!!!  XP CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_x ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && VALID_CHR(PlaList[0].index) )  { ChrList[PlaList[0].index].experience++; stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_2 ) && VALID_CHR(PlaList[1].index) )  { ChrList[PlaList[1].index].experience++; stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_3 ) && VALID_CHR(PlaList[2].index) )  { ChrList[PlaList[2].index].experience++; stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_4 ) && VALID_CHR(PlaList[3].index) )  { ChrList[PlaList[3].index].experience++; stat_check_delay = 500; }

        statdelay = 0;
    }

    // !!!BAD!!!  LIFE CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_z ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && VALID_CHR(PlaList[0].index) )  { ChrList[PlaList[0].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[0].index].life, PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_2 ) && VALID_CHR(PlaList[1].index) )  { ChrList[PlaList[1].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[1].index].life, PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_3 ) && VALID_CHR(PlaList[2].index) )  { ChrList[PlaList[2].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[2].index].life, PERFECTBIG); stat_check_delay = 500; }
        if ( SDLKEYDOWN( SDLK_4 ) && VALID_CHR(PlaList[3].index) )  { ChrList[PlaList[3].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[3].index].life, PERFECTBIG); stat_check_delay = 500; }
    }

    // Display armor stats?
    if ( SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_armor( 8 ); stat_check_delay = 1000; }
    }

    // Display enchantment stats?
    else if (  SDLKEYDOWN( SDLK_LCTRL ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_full_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_full_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_full_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_full_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_full_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_full_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_full_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_full_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else if ( SDLKEYDOWN( SDLK_LALT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_magic_status( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_magic_status( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_magic_status( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_magic_status( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_magic_status( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_magic_status( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_magic_status( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_magic_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_stat( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_stat( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_stat( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_stat( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_stat( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_stat( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_stat( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_stat( 7 ); stat_check_delay = 1000; }
    }

    // Show map cheat
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        mapon = mapvalid;
        youarehereon = btrue;
        stat_check_delay = 1000;
    }
}

//--------------------------------------------------------------------------------------------
void show_stat( Uint16 statindex )
{
    // ZZ> This function shows the more specific stats for a character
    int character, level;
    char text[64];
    char gender[8];
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Name
            sprintf( text, "=%s=", ChrList[character].name );
            debug_message( text );

            // Level and gender and class
            gender[0] = 0;
            if ( ChrList[character].alive )
            {
                if ( ChrList[character].gender == GENMALE )
                {
                    sprintf( gender, "male " );
                }
                if ( ChrList[character].gender == GENFEMALE )
                {
                    sprintf( gender, "female " );
                }

                level = ChrList[character].experiencelevel;
                if ( level == 0 )
                    sprintf( text, " 1st level %s%s", gender, CapList[ChrList[character].model].classname );
                if ( level == 1 )
                    sprintf( text, " 2nd level %s%s", gender, CapList[ChrList[character].model].classname );
                if ( level == 2 )
                    sprintf( text, " 3rd level %s%s", gender, CapList[ChrList[character].model].classname );
                if ( level >  2 )
                    sprintf( text, " %dth level %s%s", level + 1, gender, CapList[ChrList[character].model].classname );
            }
            else
            {
                sprintf( text, " Dead %s", CapList[ChrList[character].model].classname );
            }

            // Stats
            debug_message( text );
            sprintf( text, " STR:~%2d~WIS:~%2d~DEF:~%d", FP8_TO_INT( ChrList[character].strength ), FP8_TO_INT( ChrList[character].wisdom ), 255 - ChrList[character].defense );
            debug_message( text );
            sprintf( text, " INT:~%2d~DEX:~%2d~EXP:~%d", FP8_TO_INT( ChrList[character].intelligence ), FP8_TO_INT( ChrList[character].dexterity ), ChrList[character].experience );
            debug_message( text );
            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character
    char text[64], tmps[64];
    short character, skinlevel;
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];
            skinlevel = ChrList[character].skin;

            // Armor Name
            sprintf( text, "=%s=", CapList[ChrList[character].model].skinname[skinlevel] );
            debug_message( text );

            // Armor Stats
            sprintf( text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255 - CapList[ChrList[character].model].defense[skinlevel],
                     CapList[ChrList[character].model].damagemodifier[0][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[1][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[2][skinlevel]&DAMAGESHIFT );
            debug_message( text );

            sprintf( text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                     CapList[ChrList[character].model].damagemodifier[3][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[4][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[5][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[6][skinlevel]&DAMAGESHIFT,
                     CapList[ChrList[character].model].damagemodifier[7][skinlevel]&DAMAGESHIFT );
            debug_message( text );
            if ( CapList[ChrList[character].model].skindressy ) sprintf( tmps, "Light Armor" );
            else                   sprintf( tmps, "Heavy Armor" );

            sprintf( text, " Type: %s", tmps );

            // Speed and jumps
            if ( ChrList[character].jumpnumberreset == 0 )  sprintf( text, "None (0)" );
            if ( ChrList[character].jumpnumberreset == 1 )  sprintf( text, "Novice (1)" );
            if ( ChrList[character].jumpnumberreset == 2 )  sprintf( text, "Skilled (2)" );
            if ( ChrList[character].jumpnumberreset == 3 )  sprintf( text, "Master (3)" );
            if ( ChrList[character].jumpnumberreset > 3 )   sprintf( text, "Inhuman (%i)", ChrList[character].jumpnumberreset );

            sprintf( tmps, "Jump Skill: %s", text );
            sprintf( text, " Speed:~%3.0f~~%s", CapList[ChrList[character].model].maxaccel[skinlevel]*80, tmps );
            debug_message( text );

            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_full_status( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character including magic
    char text[64], tmps[64];
    short character;
    int i = 0;
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Enchanted?
            while ( i != MAX_ENC )
            {
                // Found a active enchantment that is not a skill of the character
                if ( EncList[i].on && EncList[i].spawner != character && EncList[i].target == character ) break;

                i++;
            }
            if ( i != MAX_ENC ) sprintf( text, "=%s is enchanted!=", ChrList[character].name );
            else sprintf( text, "=%s is unenchanted=", ChrList[character].name );

            debug_message( text );

            // Armor Stats
            sprintf( text, " DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d",
                     255 - ChrList[character].defense,
                     ChrList[character].damagemodifier[0]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[1]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[2]&DAMAGESHIFT );
            debug_message( text );
            sprintf( text, " HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                     ChrList[character].damagemodifier[3]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[4]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[5]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[6]&DAMAGESHIFT,
                     ChrList[character].damagemodifier[7]&DAMAGESHIFT );
            debug_message( text );

            // Speed and jumps
            if ( ChrList[character].jumpnumberreset == 0 )  sprintf( text, "None (0)" );
            if ( ChrList[character].jumpnumberreset == 1 )  sprintf( text, "Novice (1)" );
            if ( ChrList[character].jumpnumberreset == 2 )  sprintf( text, "Skilled (2)" );
            if ( ChrList[character].jumpnumberreset == 3 )  sprintf( text, "Master (3)" );
            if ( ChrList[character].jumpnumberreset > 3 )   sprintf( text, "Inhuman (4+)" );

            sprintf( tmps, "Jump Skill: %s", text );
            sprintf( text, " Speed:~%3.0f~~%s", ChrList[character].maxaccel*80, tmps );
            debug_message( text );
            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_magic_status( Uint16 statindex )
{
    // ZF> Displays special enchantment effects for the character
    char text[64], tmpa[64], tmpb[64];
    short character;
    int i = 0;
    if ( statdelay == 0 )
    {
        if ( statindex < numstat )
        {
            character = statlist[statindex];

            // Enchanted?
            while ( i != MAX_ENC )
            {
                // Found a active enchantment that is not a skill of the character
                if ( EncList[i].on && EncList[i].spawner != character && EncList[i].target == character ) break;

                i++;
            }
            if ( i != MAX_ENC ) sprintf( text, "=%s is enchanted!=", ChrList[character].name );
            else sprintf( text, "=%s is unenchanted=", ChrList[character].name );

            debug_message( text );

            // Enchantment status
            if ( ChrList[character].canseeinvisible )  sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );
            if ( ChrList[character].canseekurse )      sprintf( tmpb, "Yes" );
            else                 sprintf( tmpb, "No" );

            sprintf( text, " See Invisible: %s~~See Kurses: %s", tmpa, tmpb );
            debug_message( text );
            if ( ChrList[character].canchannel )     sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );
            if ( ChrList[character].waterwalk )        sprintf( tmpb, "Yes" );
            else                 sprintf( tmpb, "No" );

            sprintf( text, " Channel Life: %s~~Waterwalking: %s", tmpa, tmpb );
            debug_message( text );
            if ( ChrList[character].flyheight > 0 )    sprintf( tmpa, "Yes" );
            else                 sprintf( tmpa, "No" );
            if ( ChrList[character].missiletreatment == MISREFLECT )       sprintf( tmpb, "Reflect" );
            else if ( ChrList[character].missiletreatment == MISREFLECT )  sprintf( tmpb, "Deflect" );
            else                           sprintf( tmpb, "None" );

            sprintf( text, " Flying: %s~~Missile Protection: %s", tmpa, tmpb );
            debug_message( text );

            statdelay = 10;
        }
    }
}

//--------------------------------------------------------------------------------------------
void fill_bumplists()
{
    Uint16 character, particle;
    Uint32 fanblock;

    // Clear the lists
    for ( fanblock = 0; fanblock < PMesh->info.blocks_count; fanblock++ )
    {
        bumplist[fanblock].chr    = MAX_CHR;
        bumplist[fanblock].chrnum = 0;

        bumplist[fanblock].prt    = TOTAL_MAX_PRT;
        bumplist[fanblock].prtnum = 0;
    }

    // Fill 'em back up
    for ( character = 0; character < MAX_CHR; character++ )
    {
        chr_t * pchr;

        if ( !ChrList[character].on ) continue;
        pchr = ChrList + character;

        // reset the holding weight each update
        pchr->holdingweight   = 0;
        pchr->onwhichplatform = MAX_CHR;
        pchr->phys.level      = pchr->floor_level;

        // reset the fan and block position
        pchr->onwhichfan   = mesh_get_tile ( PMesh, pchr->pos.x, pchr->pos.y );
        pchr->onwhichblock = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );

        // reject characters that are in packs, or are marked as non-colliding
        if ( pchr->inpack ) continue;

        // reject characters that are hidden
        if ( pchr->is_hidden ) continue;

        if ( INVALID_BLOCK != pchr->onwhichblock )
        {
            // Insert before any other characters on the block
            pchr->bumpnext = bumplist[pchr->onwhichblock].chr;
            bumplist[pchr->onwhichblock].chr = character;
            bumplist[pchr->onwhichblock].chrnum++;
        }
    }

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;

        // reject invalid particles
        if ( !PrtList[particle].on ) continue;
        pprt = PrtList + particle;

        // reject characters that are hidden
        if ( pprt->is_hidden ) continue;

        // reset the fan and block position
        pprt->onwhichfan   = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

        if ( INVALID_BLOCK != pprt->onwhichblock )
        {
            // Insert before any other particles on the block
            pprt->bumpnext = bumplist[pprt->onwhichblock].prt;
            bumplist[pprt->onwhichblock].prt = particle;
            bumplist[pprt->onwhichblock].prtnum++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void fill_collision_list( co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    Uint16 ichr_a, ichr_b, iprt_b;

    Uint32 fanblock;
    int tnc, chrinblock, prtinblock;
    int cnt;

    if(NULL == chr_co_list)
    {
        chr_co_list = hash_list_create(-1);
        assert(NULL != chr_co_list);
    }

    // renew the collision list. Since we are filling this list with pre-allocated hash_node_t's,
    // there is no need to delete any of the existing chr_co_list->sublist elements
    for(cnt=0; cnt<256; cnt++)
    {
        chr_co_list->subcount[cnt] = 0;
        chr_co_list->sublist[cnt]  = NULL;
    }
    (*cdata_count) = 0;
    (*hn_count)    = 0;

    for ( ichr_a = 0; ichr_a < MAX_CHR; ichr_a++ )
    {
        int ixmax, ixmin;
        int iymax, iymin;

        int ix_block, ixmax_block, ixmin_block;
        int iy_block, iymax_block, iymin_block;

        chr_t * pchr_a;

        // make sure that it is on
        if ( !ChrList[ichr_a].on ) continue;
        pchr_a = ChrList + ichr_a;

        // reject characters that are in packs, or are marked as non-colliding
        if ( pchr_a->inpack ) continue;

        // reject characters that are hidden
        if ( pchr_a->is_hidden ) continue;

        // determine the size of this object in blocks
        ixmin = pchr_a->pos.x - pchr_a->bumpsize; ixmin = CLIP(ixmin, 0, PMesh->info.edge_x);
        ixmax = pchr_a->pos.x + pchr_a->bumpsize; ixmax = CLIP(ixmax, 0, PMesh->info.edge_x);

        iymin = pchr_a->pos.y - pchr_a->bumpsize; iymin = CLIP(iymin, 0, PMesh->info.edge_y);
        iymax = pchr_a->pos.y + pchr_a->bumpsize; iymax = CLIP(iymax, 0, PMesh->info.edge_y);

        ixmin_block = ixmin >> BLOCK_BITS; ixmin_block = CLIP( ixmin_block, 0, MAXMESHBLOCKY );
        ixmax_block = ixmax >> BLOCK_BITS; ixmax_block = CLIP( ixmax_block, 0, MAXMESHBLOCKY );

        iymin_block = iymin >> BLOCK_BITS; iymin_block = CLIP( iymin_block, 0, MAXMESHBLOCKY );
        iymax_block = iymax >> BLOCK_BITS; iymax_block = CLIP( iymax_block, 0, MAXMESHBLOCKY );

        // handle all the interactions on this block
        for (ix_block = ixmin_block; ix_block <= ixmax_block; ix_block++)
        {
            for (iy_block = iymin_block; iy_block <= iymax_block; iy_block++)
            {
                // Allow raw access here because we were careful :)
                fanblock = mesh_get_block_int(PMesh, ix_block, iy_block);
                if ( INVALID_BLOCK != fanblock )
                {
                    chrinblock = bumplist[fanblock].chrnum;
                    prtinblock = bumplist[fanblock].prtnum;

                    // detect all the character-character interactions
                    for ( tnc = 0, ichr_b = bumplist[fanblock].chr;
                            tnc < chrinblock && ichr_b != MAX_CHR;
                            tnc++, ichr_b = ChrList[ichr_b].bumpnext)
                    {
                        if( detect_chr_chr_collision(ichr_a, ichr_b) )
                        {
                            add_chr_chr_collision( ichr_a, ichr_b, cdata, cdata_count, hnlst, hn_count ); 
                        }
                    }

                    if ( pchr_a->alive )
                    {
                        // detect all the character-particle interactions
                        for ( tnc = 0, iprt_b = bumplist[fanblock].prt;
                                tnc < prtinblock;
                                tnc++, iprt_b = PrtList[iprt_b].bumpnext )
                        {
                            if( detect_chr_prt_collision(ichr_a, iprt_b) )
                            {
                                add_chr_prt_collision( ichr_a, iprt_b, cdata, cdata_count, hnlst, hn_count ); 
                            }
                        }
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t can_mount( Uint16 ichr_a, Uint16 ichr_b )
{
    bool_t is_valid_rider_a, is_valid_mount_b;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    // make sure that A is valid
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    if( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    is_valid_rider_a = !pchr_a->isitem && pchr_a->alive && 0 == pchr_a->flyheight && 
        INVALID_CHR(pchr_a->attachedto) && MadList[pchr_a->inst.imad].actionvalid[ACTION_MI];

    is_valid_mount_b = pchr_b->ismount && pchr_b->alive &&
        pcap_b->slotvalid[SLOT_LEFT] && INVALID_CHR(pchr_b->holdingwhich[SLOT_LEFT]);

    return is_valid_rider_a && is_valid_mount_b;
}

//--------------------------------------------------------------------------------------------
bool_t do_platforms( Uint16 ichr_a, Uint16 ichr_b )
{
    float xa, ya, za;
    float xb, yb, zb;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    bool_t platform_a, platform_b;
    bool_t mount_a, mount_b;
    float  dx, dy, dist;
    float  depth_z, lerp_z, radius, radius_xy;

    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t chara_on_top;

    // make sure that A is valid
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    if( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    // if you are mounted, only your mount is affected by platforms
    if( VALID_CHR(pchr_a->attachedto) || VALID_CHR(pchr_b->attachedto) ) return bfalse;

    // only check possible object-platform interactions
    platform_a = pcap_b->canuseplatforms && pchr_a->platform;
    platform_b = pcap_a->canuseplatforms && pchr_b->platform;
    if( !platform_a && !platform_b ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // If we can mount this platform, skip it
    mount_a = can_mount(ichr_b, ichr_a);
    if( mount_a && pchr_a->phys.level < zb + pchr_b->bumpheight + PLATTOLERANCE ) 
        return bfalse;

    //// If we can mount this platform, skip it
    mount_b = can_mount(ichr_a, ichr_b);
    if( mount_b && pchr_b->phys.level < za + pchr_a->bumpheight + PLATTOLERANCE ) 
        return bfalse;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;
    depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

    if( depth_z > PLATTOLERANCE || depth_z < -PLATTOLERANCE ) return bfalse;

    // estimate the radius of interaction based on the z overlap
    lerp_z  = depth_z / PLATTOLERANCE;
    lerp_z  = CLIP( lerp_z, 0, 1 );

    radius    = MIN(pchr_a->bumpsize,     pchr_b->bumpsize  ); /* * (1.0f - lerp_z) + (pchr_a->bumpsize    + pchr_b->bumpsize   ) * lerp_z; */
    radius_xy = MIN(pchr_a->bumpsizebig, pchr_b->bumpsizebig); /* * (1.0f - lerp_z) + (pchr_a->bumpsizebig + pchr_b->bumpsizebig) * lerp_z; */

    // determine how the characters can be attached
    chara_on_top = btrue;
    depth_z = 2*PLATTOLERANCE;
    if( platform_a && platform_b )
    {
        float depth_a, depth_b;

        depth_a = zb + pchr_b->bumpheight - za;
        depth_b = za + pchr_a->bumpheight - zb;

        depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

        chara_on_top = ABS(depth_z - depth_a) < ABS(depth_z - depth_b);

        // the collision is determined by the platform size
        if( chara_on_top )
        {
            collide_x  = (dx <= pchr_b->bumpsize);
            collide_y  = (dy <= pchr_b->bumpsize);
            collide_xy = (dist <= pchr_b->bumpsizebig);
        }
        else
        {
            collide_x  = (dx <= pchr_a->bumpsize);
            collide_y  = (dy <= pchr_a->bumpsize);
            collide_xy = (dist <= pchr_a->bumpsizebig);
        }
    }
    else if( platform_a )
    {
        chara_on_top = bfalse;
        depth_z = za + pchr_a->bumpheight - zb; 

        // the collision is determined by the platform size
        collide_x  = (dx <= pchr_a->bumpsize);
        collide_y  = (dy <= pchr_a->bumpsize);
        collide_xy = (dist <= pchr_a->bumpsizebig);
    }
    else if( platform_b )
    {
        chara_on_top = btrue;
        depth_z = zb + pchr_b->bumpheight - za;

        // the collision is determined by the platform size
        collide_x  = (dx <= pchr_b->bumpsize);
        collide_y  = (dy <= pchr_b->bumpsize);
        collide_xy = (dist <= pchr_b->bumpsizebig);
    }


    if( collide_x && collide_y && collide_xy && depth_z > -PLATTOLERANCE && depth_z < PLATTOLERANCE )
    {
        // check for the best possible attachment
        if( chara_on_top )
        {
            if( zb + pchr_b->bumpheight > pchr_a->phys.level )
            {
                pchr_a->phys.level = zb + pchr_b->bumpheight;
                pchr_a->onwhichplatform = ichr_b;
            }
        }
        else
        {
            if( za + pchr_a->bumpheight > pchr_b->phys.level )
            {
                pchr_b->phys.level = za + pchr_a->bumpheight;
                pchr_b->onwhichplatform = ichr_a;
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_mounts( Uint16 ichr_a, Uint16 ichr_b )
{
    float xa, ya, za;
    float xb, yb, zb;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    bool_t mount_a, mount_b;
    float  dx, dy, dist;
    float  depth_z;

    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;

    bool_t mounted;

    // make sure that A is valid
    if ( INVALID_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    if( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // can either of these objects mount the other?
    mount_a = can_mount(ichr_b, ichr_a);
    mount_b = can_mount(ichr_a, ichr_b);

    if( !mount_a && !mount_b ) return bfalse;

    mounted = bfalse;
    if( !mounted && mount_b && (pchr_a->vel.z - pchr_b->vel.z) < 0 )
    {
        // A falling on B?
        GLvector4 point[1], nupoint[1];

        // determine the actual location of the mount point
        {
            int frame_nxt, frame_lst, lip, vertex;
            float flip;
            chr_instance_t * pinst = &(pchr_b->inst);

            frame_nxt = pinst->frame_nxt;
            frame_lst = pinst->frame_lst;
            lip = pinst->lip >> 6;
            flip = lip / 4.0f;

            vertex = MadList[pinst->imad].md2.vertices - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( &(ChrList[ichr_b].inst), vertex, vertex );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vlst[vertex].pos[XX];
            point[0].y = pinst->vlst[vertex].pos[YY];
            point[0].z = pinst->vlst[vertex].pos[ZZ];
            point[0].w = 1.0f;

            // Do the transform
            TransformVertices( &(pinst->matrix), point, nupoint, 1 );
        }

        dx = ABS( xa - nupoint[0].x );
        dy = ABS( ya - nupoint[0].y );
        dist = dx + dy;
        depth_z = za - nupoint[0].z;

        if( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = (dx <= pchr_a->bumpsize * 2);
            collide_y  = (dy <= pchr_a->bumpsize * 2);
            collide_xy = (dist <= pchr_a->bumpsizebig * 2);

            if( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY );
                mounted = VALID_CHR( pchr_a->attachedto );
            }
        }
    }

    if( !mounted && mount_a && (pchr_b->vel.z - pchr_a->vel.z) < 0 )
    {
        // B falling on A?

        GLvector4 point[1], nupoint[1];

        // determine the actual location of the mount point
        {
            int frame_nxt, frame_lst, lip, vertex;
            float flip;
            chr_instance_t * pinst = &(pchr_a->inst);

            frame_nxt = pinst->frame_nxt;
            frame_lst = pinst->frame_lst;
            lip = pinst->lip >> 6;
            flip = lip / 256.0f;

            vertex = MadList[pinst->imad].md2.vertices - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( &(ChrList[ichr_a].inst), vertex, vertex );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vlst[vertex].pos[XX];
            point[0].y = pinst->vlst[vertex].pos[YY];
            point[0].z = pinst->vlst[vertex].pos[ZZ];
            point[0].w = 1.0f;

            // Do the transform
            TransformVertices( &(pinst->matrix), point, nupoint, 1 );
        }

        dx = ABS( xb - nupoint[0].x );
        dy = ABS( yb - nupoint[0].y );
        dist = dx + dy;
        depth_z = zb - nupoint[0].z;

        if( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = (dx <= pchr_b->bumpsize * 2);
            collide_y  = (dy <= pchr_b->bumpsize * 2);
            collide_xy = (dist <= pchr_b->bumpsizebig * 2);

            if( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_b, ichr_a, GRIP_ONLY );
                mounted = VALID_CHR( pchr_a->attachedto );
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b )
{
    float xa,ya,za, xb,yb,zb;
    float was_xa,was_ya,was_za, was_xb,was_yb,was_zb;
    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    float dx, dy, dist;
    float was_dx, was_dy, was_dist;
    float depth_z, was_depth_z;
    float lerp_z, radius, radius_xy;
    float wta, wtb;

    bool_t collide_x  = bfalse, was_collide_x;
    bool_t collide_y  = bfalse, was_collide_y;
    bool_t collide_xy = bfalse, was_collide_xy;
    bool_t collide_z  = bfalse, was_collide_z;
    bool_t collision  = bfalse;

    float interaction_strength = 1.0f;

    // make sure that it is on
    if ( !VALID_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    if( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that it is on
    if ( !VALID_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;


    // don't do anything if there is no interaction strength
    interaction_strength = 1.0f;
    if( 0 == pchr_a->bumpsize || 0 == pchr_b->bumpsize ) return bfalse;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;
    interaction_strength *= pchr_b->inst.alpha * INV_FF;


    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    was_xa = xa - pchr_a->vel.x;
    was_ya = ya - pchr_a->vel.y;
    was_za = za - pchr_a->vel.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    was_xb = xb - pchr_b->vel.x;
    was_yb = yb - pchr_b->vel.y;
    was_zb = zb - pchr_b->vel.z;

    // platform interaction
    if( ichr_a == pchr_b->onwhichplatform )
    {
        // we know that ichr_a is a platform and ichr_b is on it
        Sint16 rot_a, rot_b;

        lerp_z = (pchr_b->pos.z - pchr_b->phys.level) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        rot_b = pchr_b->turn_z - pchr_b->turn_old_z;
        rot_a = pchr_a->turn_z - pchr_a->turn_old_z;

        if( lerp_z < 0 )
        {
            pchr_b->phys.apos_0.z += (pchr_b->phys.level - pchr_b->pos.z) * 0.25f * (-lerp_z);
            pchr_b->jumpnumber = pchr_b->jumpnumberreset;
        }
        else if( lerp_z < 1 )
        {
            pchr_b->phys.apos_0.z += (pchr_b->phys.level - pchr_b->pos.z) * 0.125f * lerp_z;
        }

        lerp_z = 1.0f - CLIP( lerp_z, 0, 1 );

        if( lerp_z > 0 )
        {
            pchr_a->holdingweight += pchr_b->weight * lerp_z;

            pchr_b->phys.avel.z += ( pchr_a->vel.z  - pchr_b->vel.z      ) * lerp_z * 0.25f;
            pchr_b->turn_z     += ( rot_a - rot_b ) * platstick * lerp_z;
        };

        if( lerp_z < 0.25f )
        {
            pchr_b->jumpready = btrue;
        };
        
        // this is handled
        return btrue;
    }

    // platform interaction
    if( ichr_b == pchr_a->onwhichplatform )
    {
        // we know that ichr_b is a platform and ichr_a is on it
        Sint16 rot_a, rot_b;

        lerp_z = (pchr_a->pos.z - pchr_a->phys.level) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        rot_b = pchr_b->turn_z - pchr_b->turn_old_z;
        rot_a = pchr_a->turn_z - pchr_a->turn_old_z;

        if( lerp_z < 0 )
        {
            pchr_a->phys.apos_0.z += (pchr_a->phys.level - pchr_a->pos.z) * 0.25f * (-lerp_z);
            pchr_a->jumpnumber = pchr_a->jumpnumberreset;
        }
        else if( lerp_z < 1 )
        {
            pchr_a->phys.apos_0.z += (pchr_a->phys.level - pchr_a->pos.z) * 0.125f * lerp_z;
        }

        lerp_z = 1.0f - CLIP( lerp_z, 0, 1 );

        if( lerp_z > 0 )
        {
            pchr_b->holdingweight += pchr_a->weight * lerp_z;

            pchr_a->phys.avel.z += ( pchr_b->vel.z  - pchr_a->vel.z      ) * lerp_z * 0.25f;
            pchr_a->turn_z     += ( rot_b - rot_a ) * platstick * lerp_z;
        }

        if( lerp_z < 0.25f )
        {
            pchr_a->jumpready = btrue;
        }

        // this is handled
        return btrue;
    }

    // items can interact with platforms but not with other characters/objects
    if( pchr_a->isitem || pchr_b->isitem ) return bfalse;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;

    was_dx = ABS( was_xa - was_xb );
    was_dy = ABS( was_ya - was_yb );
    was_dist = was_dx + was_dy;

    depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);
    was_depth_z = MIN( was_zb + pchr_b->bumpheight, was_za + pchr_a->bumpheight ) - MAX(was_za, was_zb);

    // estimate the radius of interaction based on the z overlap
    lerp_z  = depth_z / PLATTOLERANCE;
    lerp_z  = CLIP( lerp_z, 0, 1 );

    radius    = MIN(pchr_a->bumpsize, pchr_b->bumpsize) * (1.0f - lerp_z) + (pchr_a->bumpsize + pchr_b->bumpsize) * lerp_z;
    radius_xy = MIN(pchr_a->bumpsizebig, pchr_b->bumpsizebig) * (1.0f - lerp_z) + (pchr_a->bumpsizebig + pchr_b->bumpsizebig) * lerp_z;

    // estimate the collisions this frame
    collide_x  = (dx <= radius);
    collide_y  = (dy <= radius);
    collide_xy = (dist <= radius_xy);
    collide_z  = (depth_z > 0);

    // estimate the collisions last frame
    was_collide_x  = (was_dx <= radius);
    was_collide_y  = (was_dy <= radius);
    was_collide_xy = (was_dist <= radius_xy);
    was_collide_z  = (was_depth_z > 0);

    //------------------
    // do character-character interactions
    if ( !collide_x || !collide_y || !collide_xy || depth_z < -PLATTOLERANCE ) return bfalse;

    wta = (0xFFFFFFFF == pchr_a->weight) ? -(float)0xFFFFFFFF : pchr_a->weight;
    wtb = (0xFFFFFFFF == pchr_b->weight) ? -(float)0xFFFFFFFF : pchr_b->weight;

    if ( wta == 0 && wtb == 0 )
    {
        wta = wtb = 1;
    }
    else if ( wta == 0 )
    {
        wta = 1;
        wtb = -0xFFFF;
    }
    else if ( wtb == 0 )
    {
        wtb = 1;
        wta = -0xFFFF;
    }

    if ( 0.0f == pchr_a->bumpdampen && 0.0f == pchr_b->bumpdampen )
    {
        /* do nothing */
    }
    else if ( 0.0f == pchr_a->bumpdampen )
    {
        // make the weight infinite
        wta = -0xFFFF;
    }
    else if ( 0.0f == pchr_b->bumpdampen )
    {
        // make the weight infinite
        wtb = -0xFFFF;
    }
    else
    {
        // adjust the weights to respect bumpdampen
        wta /= pchr_a->bumpdampen;
        wtb /= pchr_b->bumpdampen;
    }

    if ( !collision && collide_z )
    {
        float depth_x, depth_y, depth_xy, depth_yx, depth_z;
        GLvector3 nrm;
        int exponent = 1;

        if( pcap_a->canuseplatforms && pchr_b->platform ) exponent += 2;
        if( pcap_b->canuseplatforms && pchr_a->platform ) exponent += 2;


        nrm.x = nrm.y = nrm.z = 0.0f;

        depth_x  = MIN(xa + pchr_a->bumpsize, xb + pchr_b->bumpsize) - MAX(xa - pchr_a->bumpsize, xb - pchr_b->bumpsize);
        if ( depth_x < 0.0f )
        {
            depth_x = 0.0f;
        }
        else
        {
            float sgn = xb - xa;
            sgn = sgn > 0 ? -1 : 1;

            nrm.x += sgn / POW(depth_x/PLATTOLERANCE, exponent);
        }

        depth_y  = MIN(ya + pchr_a->bumpsize, yb + pchr_b->bumpsize) - MAX(ya - pchr_a->bumpsize, yb - pchr_b->bumpsize);
        if ( depth_y < 0.0f )
        {
            depth_y = 0.0f;
        }
        else
        {
            float sgn = yb - ya;
            sgn = sgn > 0 ? -1 : 1;

            nrm.y += sgn / POW(depth_y/PLATTOLERANCE, exponent);
        }

        depth_xy = MIN(xa + ya + pchr_a->bumpsizebig, xb + yb + pchr_b->bumpsizebig) - MAX(xa + ya - pchr_a->bumpsizebig, xb + yb - pchr_b->bumpsizebig);
        if ( depth_xy < 0.0f )
        {
            depth_xy = 0.0f;
        }
        else
        {
            float sgn = (xb + yb) - (xa + ya);
            sgn = sgn > 0 ? -1 : 1;

            nrm.x += sgn / POW(depth_xy/PLATTOLERANCE, exponent);
            nrm.y += sgn / POW(depth_xy/PLATTOLERANCE, exponent);
        }

        depth_yx = MIN(-xa + ya + pchr_a->bumpsizebig, -xb + yb + pchr_b->bumpsizebig) - MAX(-xa + ya - pchr_a->bumpsizebig, -xb + yb - pchr_b->bumpsizebig);
        if ( depth_yx < 0.0f )
        {
            depth_yx = 0.0f;
        }
        else
        {
            float sgn = (-xb + yb) - (-xa + ya);
            sgn = sgn > 0 ? -1 : 1;
            nrm.x -= sgn / POW(depth_yx/PLATTOLERANCE, exponent);
            nrm.y += sgn / POW(depth_yx/PLATTOLERANCE, exponent);
        }

        depth_z  = MIN(za + pchr_a->bumpheight, zb + pchr_b->bumpheight) - MAX( za, zb );
        if ( depth_z < 0.0f )
        {
            depth_z = 0.0f;
        }
        else
        {
            float sgn = (zb + pchr_b->bumpheight/2) - (za + pchr_a->bumpheight / 2);
            sgn = sgn > 0 ? -1 : 1;

            nrm.z += sgn / POW(exponent * depth_z/PLATTOLERANCE, exponent);
        }


        if ( ABS(nrm.x) + ABS(nrm.y) + ABS(nrm.z) > 0.0f )
        {
            GLvector3 vel_a, vel_b;
            GLvector3 vpara_a, vperp_a;
            GLvector3 vpara_b, vperp_b;
            GLvector3 imp_a, imp_b;
            float     vdot;

            nrm = VNormalize( nrm );

            vel_a.x = pchr_a->vel.x;
            vel_a.y = pchr_a->vel.y;
            vel_a.z = pchr_a->vel.z;

            vel_b.x = pchr_b->vel.x;
            vel_b.y = pchr_b->vel.y;
            vel_b.z = pchr_b->vel.z;

            vdot = VDotProduct( nrm, vel_a );
            vperp_a.x = nrm.x * vdot;
            vperp_a.y = nrm.y * vdot;
            vperp_a.z = nrm.z * vdot;
            vpara_a = VSub( vel_a, vperp_a );

            vdot = VDotProduct( nrm, vel_b );
            vperp_b.x = nrm.x * vdot;
            vperp_b.y = nrm.y * vdot;
            vperp_b.z = nrm.z * vdot;
            vpara_b = VSub( vel_b, vperp_b );

            // clear the "impulses"
            imp_a.x = imp_a.y = imp_a.z = 0.0f;
            imp_b.x = imp_b.y = imp_b.z = 0.0f;

            if ( collide_xy != was_collide_xy || collide_x != was_collide_x || collide_y != was_collide_y )
            {
                // an actual collision

                // generic coefficient of restitution
                float cr = 0.5f;

                if( (wta < 0 && wtb < 0) || (wta == wtb) )
                {
                    float factor = 0.5f * (1.0f - cr);

                    imp_a.x = factor * (vperp_b.x - vperp_a.x);
                    imp_a.y = factor * (vperp_b.y - vperp_a.y);
                    imp_a.z = factor * (vperp_b.z - vperp_a.z);

                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }
                else if( (wta < 0) || (wtb == 0) )
                {
                    float factor = (1.0f - cr);

                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }
                else if( (wtb < 0) || (wta == 0) )
                {
                    float factor = (1.0f - cr);

                    imp_a.x = factor * (vperp_b.x - vperp_a.x);
                    imp_a.y = factor * (vperp_b.y - vperp_a.y);
                    imp_a.z = factor * (vperp_b.z - vperp_a.z);
                }
                else
                {
                    float factor;

                    factor = (1.0f - cr) * wtb / ( wta + wtb );
                    imp_a.x = factor * (vperp_b.x - vperp_a.x);
                    imp_a.y = factor * (vperp_b.y - vperp_a.y);
                    imp_a.z = factor * (vperp_b.z - vperp_a.z);

                    factor = (1.0f - cr) * wta / ( wta + wtb );
                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }

                // add in the collision impulses
                pchr_a->phys.avel.x += imp_a.x;
                pchr_a->phys.avel.y += imp_a.y;
                pchr_a->phys.avel.z += imp_a.z;

                pchr_b->phys.avel.x += imp_b.x;
                pchr_b->phys.avel.y += imp_b.y;
                pchr_b->phys.avel.z += imp_b.z;

                collision = btrue;
            }
            else
            {
                float tmin;

                tmin = 1e6;
                if ( nrm.x != 0 )
                {
                    tmin = MIN(tmin, depth_x / ABS(nrm.x) );
                }
                if ( nrm.y != 0 )
                {
                    tmin = MIN(tmin, depth_y / ABS(nrm.y) );
                }
                if ( nrm.z != 0 )
                {
                    tmin = MIN(tmin, depth_z / ABS(nrm.z) );
                }

                if ( nrm.x + nrm.y != 0 )
                {
                    tmin = MIN(tmin, depth_xy / ABS(nrm.x + nrm.y) );
                }

                if ( -nrm.x + nrm.y != 0 )
                {
                    tmin = MIN(tmin, depth_yx / ABS(-nrm.x + nrm.y) );
                }

                if ( tmin < 1e6 )
                {
                    if ( wta >= 0.0f )
                    {
                        float ratio = (float)ABS(wtb) / ((float)ABS(wta) + (float)ABS(wtb));

                        imp_a.x = tmin * nrm.x * 0.25f * ratio;
                        imp_a.y = tmin * nrm.y * 0.25f * ratio;
                        imp_a.z = tmin * nrm.z * 0.25f * ratio;
                    }

                    if ( wtb >= 0.0f )
                    {
                        float ratio = (float)ABS(wta) / ((float)ABS(wta) + (float)ABS(wtb));

                        imp_b.x = -tmin * nrm.x * 0.25f * ratio;
                        imp_b.y = -tmin * nrm.y * 0.25f * ratio;
                        imp_b.z = -tmin * nrm.z * 0.25f * ratio;
                    }
                }

                // add in the collision impulses
                pchr_a->phys.apos_1.x += imp_a.x;
                pchr_a->phys.apos_1.y += imp_a.y;
                pchr_a->phys.apos_1.z += imp_a.z;

                pchr_b->phys.apos_1.x += imp_b.x;
                pchr_b->phys.apos_1.y += imp_b.y;
                pchr_b->phys.apos_1.z += imp_b.z;

                // you could "bump" something if you changed your velocity, even if you were still touching
                collision = (VDotProduct( pchr_a->vel, nrm ) * VDotProduct( pchr_a->vel_old, nrm ) < 0 ) ||
                            (VDotProduct( pchr_b->vel, nrm ) * VDotProduct( pchr_b->vel_old, nrm ) < 0 );

            }

            // add in the friction due to the "collision"
            // assume coeff of friction of 0.5
            if( ABS(imp_a.x) + ABS(imp_a.y) + ABS(imp_a.z) > 0.0f && 
                ABS(vpara_a.x) + ABS(vpara_a.y) + ABS(vpara_a.z) > 0.0f && 
                pchr_a->phys.dismount_timer <= 0)
            {
                float imp, vel, factor;
                
                imp = 0.5f * SQRT( imp_a.x*imp_a.x + imp_a.y*imp_a.y + imp_a.z*imp_a.z );
                vel = SQRT( vpara_a.x*vpara_a.x + vpara_a.y*vpara_a.y + vpara_a.z*vpara_a.z );

                factor = imp/vel;
                factor = CLIP(factor,0.0f,1.0f);

                pchr_a->phys.avel.x -= factor * vpara_a.x;
                pchr_a->phys.avel.y -= factor * vpara_a.y;
                pchr_a->phys.avel.z -= factor * vpara_a.z;
            }

            if( ABS(imp_b.x) + ABS(imp_b.y) + ABS(imp_b.z) > 0.0f && 
                ABS(vpara_b.x) + ABS(vpara_b.y) + ABS(vpara_b.z) > 0.0f && 
                pchr_b->phys.dismount_timer <= 0)
            {
                float imp, vel, factor;
                
                imp = 0.5f * SQRT( imp_b.x*imp_b.x + imp_b.y*imp_b.y + imp_b.z*imp_b.z );
                vel = SQRT( vpara_b.x*vpara_b.x + vpara_b.y*vpara_b.y + vpara_b.z*vpara_b.z );

                factor = imp/vel;
                factor = CLIP(factor,0.0f,1.0f);

                pchr_b->phys.avel.x -= factor * vpara_b.x;
                pchr_b->phys.avel.y -= factor * vpara_b.y;
                pchr_b->phys.avel.z -= factor * vpara_b.z;
            }
        }
    }

    if ( collision )
    {
        pchr_a->ai.bumplast = ichr_b;
        pchr_b->ai.bumplast = ichr_a;

        pchr_a->ai.alert |= ALERTIF_BUMPED;
        pchr_b->ai.alert |= ALERTIF_BUMPED;			
    }

    return btrue;
}



//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b )
{
    Uint16 ipip_b, direction;
    Uint32 prtidparent, prtidtype, eveidremove;
    int enchant, temp;
    float ax, ay, nx, ny, scale;  // For deflection
    Uint16 facing;

    float xa,ya,za, xb,yb,zb;
    float was_xa,was_ya,was_za, was_xb,was_yb,was_zb;
    chr_t * pchr_a;
    cap_t * pcap_a;

    prt_t * pprt_b;
    pip_t * ppip_b;

    float dx, dy, dist;
    bool_t collide_x, collide_y, collide_xy, collide_z;
    float depth_z;

    float interaction_strength = 1.0f;

    // make sure that it is on
    if ( !ChrList[ichr_a].on ) return bfalse;
    pchr_a = ChrList + ichr_a;

    if ( !pchr_a->alive ) return bfalse;

    if( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    if( !VALID_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList + iprt_b;

    ipip_b = pprt_b->pip;
    if( INVALID_PIP( ipip_b ) ) return bfalse;
    ppip_b = PipList + ipip_b;

    // do not collide with the thing that you're attached to
    if ( ichr_a == pprt_b->attachedtocharacter ) return bfalse;

    // if there's no friendly fire, particles issued by ichr_a can't hit it
    if ( !ppip_b->friendlyfire && ichr_a == pprt_b->chr ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    was_xa = xa - pchr_a->vel.x;
    was_ya = ya - pchr_a->vel.y;
    was_za = za - pchr_a->vel.z;

    xb = pprt_b->pos.x;
    yb = pprt_b->pos.y;
    zb = pprt_b->pos.z;

    was_xb = xb - pprt_b->vel.x;
    was_yb = yb - pprt_b->vel.y;
    was_zb = zb - pprt_b->vel.z;

    if( 0 == pchr_a->bumpsize ) interaction_strength = 0;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;

    collide_x  = dx < pchr_a->bumpsize    || dx < pprt_b->bumpsize;
    collide_y  = dy < pchr_a->bumpsize    || dy < pprt_b->bumpsize;
    collide_xy = dist < pchr_a->bumpsizebig || dist < pprt_b->bumpsize * 1.41f;

    depth_z = MIN( za + pchr_a->bumpheight, zb + pprt_b->bumpheight ) - MAX( za, zb - pprt_b->bumpheight );
    collide_z = depth_z > 0;

    if( !collide_x || !collide_y || !collide_xy || !collide_z ) return bfalse;

    if ( pchr_a->platform && INVALID_CHR(pprt_b->attachedtocharacter) && 
        (zb > za + pchr_a->bumpheight + pprt_b->vel.z) && (pprt_b->vel.z - pchr_a->vel.z < 0) )
    {
        // Particle is falling on A
        pprt_b->pos.z = za + pchr_a->bumpheight;
        pprt_b->vel.z = -pprt_b->vel.z * ppip_b->dampen;
        pprt_b->vel.x += ( pchr_a->vel.x - pchr_a->vel_old.x ) * platstick;
        pprt_b->vel.y += ( pchr_a->vel.y - pchr_a->vel_old.y  ) * platstick;
    }

    // Check reaffirmation of particles
    if ( pchr_a->reloadtime == 0 && pchr_a->damagetime == 0 )
    {
        if ( ichr_a != pprt_b->attachedtocharacter && pchr_a->reaffirmdamagetype == pprt_b->damagetype )
        {
            reaffirm_attached_particles( ichr_a );
        }
    }

    // Check for missile treatment
    if (  pchr_a->missiletreatment == MISNORMAL ||
        /*pchr_a->damagemodifier[pprt_b->damagetype]&3 ) < 2 ||*/
        /*pprt_b->attachedtocharacter != MAX_CHR ||*/
        ( pprt_b->chr == ichr_a && !ppip_b->friendlyfire ) ||
        ( ChrList[pchr_a->missilehandler].mana < ( pchr_a->missilecost << 4 ) && !ChrList[pchr_a->missilehandler].canchannel ) )
    {
        if ( ( TeamList[pprt_b->team].hatesteam[pchr_a->team] || ( ppip_b->friendlyfire && ( ( ichr_a != pprt_b->chr && ichr_a != ChrList[pprt_b->chr].attachedto ) || ppip_b->onlydamagefriendly ) ) ) && !pchr_a->invictus )
        {
            spawn_bump_particles( ichr_a, iprt_b ); // Catch on fire
            if ( ( pprt_b->damagebase | pprt_b->damagerand ) > 1 )
            {
                prtidparent = CapList[pprt_b->model].idsz[IDSZ_PARENT];
                prtidtype = CapList[pprt_b->model].idsz[IDSZ_TYPE];
                if ( pchr_a->damagetime == 0 && pprt_b->attachedtocharacter != ichr_a && ( ppip_b->damfx&DAMFX_ARRO ) == 0 )
                {
                    // Normal iprt_b damage
                    if ( ppip_b->allowpush && pchr_a->weight != 0xFFFFFFFF )
                    {
                        if( 0 == pchr_a->weight )
                        {
                            pchr_a->phys.avel.x  += pprt_b->vel.x - pchr_a->vel.x;
                            pchr_a->phys.avel.y  += pprt_b->vel.y - pchr_a->vel.y;
                            pchr_a->phys.avel.z  += pprt_b->vel.z - pchr_a->vel.z;
                        }
                        else
                        {
                            float factor = MIN( 1.0f, 110 / pchr_a->weight );   // 110 is the "iconic" weight of the adventurer
                            factor = MIN( 1.0f, factor * pchr_a->bumpdampen );

                            pchr_a->phys.avel.x  += pprt_b->vel.x * factor;
                            pchr_a->phys.avel.y  += pprt_b->vel.y * factor;
                            pchr_a->phys.avel.z  += pprt_b->vel.z * factor;
                        }
                    }

                    direction = ( ATAN2( pprt_b->vel.y, pprt_b->vel.x ) + PI ) * 0xFFFF / ( TWO_PI );
                    direction = pchr_a->turn_z - direction + 32768;

                    // Check all enchants to see if they are removed
                    enchant = pchr_a->firstenchant;
                    while ( enchant != MAX_ENC )
                    {
                        eveidremove = EveList[EncList[enchant].eve].removedbyidsz;
                        temp = EncList[enchant].nextenchant;
                        if ( eveidremove != IDSZ_NONE && ( eveidremove == prtidtype || eveidremove == prtidparent ) )
                        {
                            remove_enchant( enchant );
                        }

                        enchant = temp;
                    }

                    // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                    //+2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                    if ( ppip_b->intdamagebonus )
                    {
                        float percent;
                        percent = ( (FP8_TO_INT(ChrList[pprt_b->chr].intelligence)) - 14 ) *2;
                        percent /= 100;
                        pprt_b->damagebase *= 1.00f + percent;
                    }

                    if ( ppip_b->wisdamagebonus )
                    {
                        float percent;
                        percent = ( FP8_TO_INT(ChrList[pprt_b->chr].wisdom) - 14 ) *2;
                        percent /= 100;
                        pprt_b->damagebase *= 1.00f + percent;
                    }

                    // Damage the character
                    if ( pcap_a->idsz[IDSZ_VULNERABILITY] != IDSZ_NONE && ( pcap_a->idsz[IDSZ_VULNERABILITY] == prtidtype || pcap_a->idsz[IDSZ_VULNERABILITY] == prtidparent ) )
                    {
                        damage_character( ichr_a, direction, pprt_b->damagebase << 1, pprt_b->damagerand << 1, pprt_b->damagetype, pprt_b->team, pprt_b->chr, ppip_b->damfx, bfalse );
                        pchr_a->ai.alert |= ALERTIF_HITVULNERABLE;
                    }
                    else
                    {
                        damage_character( ichr_a, direction, pprt_b->damagebase, pprt_b->damagerand, pprt_b->damagetype, pprt_b->team, pprt_b->chr, ppip_b->damfx, bfalse );
                    }

                    // Do confuse effects
                    if ( 0 == ( Md2FrameList[pchr_a->inst.frame_nxt].framefx&MADFX_INVICTUS ) || ppip_b->damfx&DAMFX_NBLOC )
                    {
                        if ( ppip_b->grogtime != 0 && pcap_a->canbegrogged )
                        {
                            pchr_a->grogtime += ppip_b->grogtime;
                            if ( pchr_a->grogtime < 0 )  pchr_a->grogtime = 32767;

                            pchr_a->ai.alert |= ALERTIF_GROGGED;
                        }
                        if ( ppip_b->dazetime != 0 && pcap_a->canbedazed )
                        {
                            pchr_a->dazetime += ppip_b->dazetime;
                            if ( pchr_a->dazetime < 0 )  pchr_a->dazetime = 32767;

                            pchr_a->ai.alert |= ALERTIF_DAZED;
                        }
                    }

                    // Notify the attacker of a scored hit
                    if ( pprt_b->chr != MAX_CHR )
                    {
                        ChrList[pprt_b->chr].ai.alert |= ALERTIF_SCOREDAHIT;
                        ChrList[pprt_b->chr].ai.hitlast = ichr_a;
                    }
                }

                if (  0 == ( frame_all & 31 ) && pprt_b->attachedtocharacter == ichr_a )
                {
                    // Attached iprt_b damage ( Burning )
                    if ( ppip_b->xyvelbase == 0 )
                    {
                        // Make character limp
                        pchr_a->phys.avel.x += -pchr_a->vel.x;
                        pchr_a->phys.avel.y += -pchr_a->vel.y;
                    }

                    damage_character( ichr_a, 32768, pprt_b->damagebase, pprt_b->damagerand, pprt_b->damagetype, pprt_b->team, pprt_b->chr, ppip_b->damfx, bfalse );
                }
            }

            if ( ppip_b->endbump )
            {
                if ( ppip_b->bumpmoney )
                {
                    if ( pchr_a->cangrabmoney && pchr_a->alive && pchr_a->damagetime == 0 && pchr_a->money != MAXMONEY )
                    {
                        if ( pchr_a->ismount )
                        {
                            // Let mounts collect money for their riders
                            if ( pchr_a->holdingwhich[SLOT_LEFT] != MAX_CHR )
                            {
                                ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money += ppip_b->bumpmoney;
                                if ( ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money > MAXMONEY ) ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money = MAXMONEY;
                                if ( ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money < 0 ) ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money = 0;

                                pprt_b->_time = frame_all + 1;
                                pprt_b->poofme = btrue; 
                            }
                        }
                        else
                        {
                            // Normal money collection
                            pchr_a->money += ppip_b->bumpmoney;
                            if ( pchr_a->money > MAXMONEY ) pchr_a->money = MAXMONEY;
                            if ( pchr_a->money < 0 ) pchr_a->money = 0;

                            pprt_b->_time = frame_all + 1;
                            pprt_b->poofme = btrue;
                        }
                    }
                }
                else
                {
                    pprt_b->_time  = frame_all + 1;
                    pprt_b->poofme = btrue;

                    // Only hit one character, not several
                    pprt_b->damagebase = 0;
                    pprt_b->damagerand = 1;
                }
            }
        }
    }
    else if ( pprt_b->chr != ichr_a )
    {
        cost_mana( pchr_a->missilehandler, ( pchr_a->missilecost << 4 ), pprt_b->chr );

        // Treat the missile
        if ( pchr_a->missiletreatment == MISDEFLECT )
        {
            // Use old position to find normal
            ax = pprt_b->pos.x - pprt_b->vel.x;
            ay = pprt_b->pos.y - pprt_b->vel.y;
            ax = pchr_a->pos.x - ax;
            ay = pchr_a->pos.y - ay;
            // Find size of normal
            scale = ax * ax + ay * ay;
            if ( scale > 0 )
            {
                // Make the normal a unit normal
                scale = SQRT( scale );
                nx = ax / scale;
                ny = ay / scale;
                // Deflect the incoming ray off the normal
                scale = ( pprt_b->vel.x * nx + pprt_b->vel.y * ny ) * 2;
                ax = scale * nx;
                ay = scale * ny;
                pprt_b->vel.x = pprt_b->vel.x - ax;
                pprt_b->vel.y = pprt_b->vel.y - ay;
            }
        }
        else
        {
            // Reflect it back in the direction it came
            pprt_b->vel.x = -pprt_b->vel.x;
            pprt_b->vel.y = -pprt_b->vel.y;
        }

        // Change the owner of the missile
        if ( !ppip_b->homing )
        {
            pprt_b->team = pchr_a->team;
            pprt_b->chr = ichr_a;
        }

        // Change the direction of the particle
        if ( ppip_b->rotatetoface )
        {
            // Turn to face new direction
            facing = ATAN2( pprt_b->vel.y, pprt_b->vel.x ) * 0xFFFF / ( TWO_PI );
            facing += 32768;
            pprt_b->facing = facing;
        }
    }


    return btrue;
}


//--------------------------------------------------------------------------------------------
// collision data
static int       cdata_count = 0;
static co_data_t cdata[CHR_MAX_COLLISIONS];

// collision data hash nodes
static int         hn_count = 0;
static hash_node_t hnlst[COLLISION_HASH_NODES];

void bump_characters( void )
{
    // ZZ> This function sets handles characters hitting other characters or particles

    int tnc, cnt;

    co_data_t   * d;

    // fill up the bumplists
    fill_bumplists();

    // blank the accumulators
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList[cnt].phys.apos_0.x = 0.0f;
        ChrList[cnt].phys.apos_0.y = 0.0f;
        ChrList[cnt].phys.apos_0.z = 0.0f;

        ChrList[cnt].phys.apos_1.x = 0.0f;
        ChrList[cnt].phys.apos_1.y = 0.0f;
        ChrList[cnt].phys.apos_1.z = 0.0f;

        ChrList[cnt].phys.avel.x = 0.0f;
        ChrList[cnt].phys.avel.y = 0.0f;
        ChrList[cnt].phys.avel.z = 0.0f;
    }

    // fill the collision list with all possible binary interactions
    fill_collision_list( cdata, &cdata_count, hnlst, &hn_count );

    // Do platforms
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for( tnc = 0; tnc<count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);
            if(TOTAL_MAX_PRT != d->prtb) continue;

            do_platforms( d->chra, d->chrb );
        }
    }


    // Do mounts
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for( tnc = 0; tnc<count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);
            if(TOTAL_MAX_PRT != d->prtb) continue;

            do_mounts( d->chra, d->chrb );
        }
    }

    // do all interactions
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for( tnc = 0; tnc<count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);

            if( TOTAL_MAX_PRT == d->prtb )
            {
                do_chr_chr_collision( d->chra, d->chrb );
            }
            else if ( MAX_CHR == d->chrb )
            {
                do_chr_prt_collision( d->chra, d->prtb );
            }
        }
    }

    // accumulate the accumulators
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        float tmpx, tmpy, tmpz;
        chr_t * pchr;
        float bump_str;
        float nrm[2];

        if( !ChrList[cnt].on ) continue;
        pchr = ChrList + cnt;

        bump_str = 1.0f;
        if( VALID_CHR( pchr->attachedto ) )
        {
            bump_str = 0;
        }
        else if( pchr->phys.dismount_timer > 0 )
        {
            bump_str = 1.0f - (float)pchr->phys.dismount_timer / PHYS_DISMOUNT_TIME;
            bump_str = bump_str * bump_str * 0.5f;
        };

        // decrement the dismount timer
        if ( pchr->phys.dismount_timer > 0 ) pchr->phys.dismount_timer--;

        // do the "integration" of the accumulated accelerations
        pchr->vel.x += pchr->phys.avel.x;
        pchr->vel.y += pchr->phys.avel.y;
        pchr->vel.z += pchr->phys.avel.z;

        // do the "integration" on the position
        if ( ABS(pchr->phys.apos_0.x + pchr->phys.apos_1.x) > 0 )
        {
            tmpx = pchr->pos.x;
            pchr->pos.x += pchr->phys.apos_0.x + pchr->phys.apos_1.x;
            if ( __chrhitawall(cnt, nrm) )
            {
                // restore the old values
                pchr->pos.x = tmpx;
            }
            else
            {
                pchr->vel.x += pchr->phys.apos_1.x * bump_str;
                pchr->pos_safe.x = tmpx;
            }
        }

        if ( ABS(pchr->phys.apos_0.y + pchr->phys.apos_1.y) > 0 )
        {
            tmpy = pchr->pos.y;
            pchr->pos.y += pchr->phys.apos_0.y + pchr->phys.apos_1.y;
            if ( __chrhitawall(cnt, nrm) )
            {
                // restore the old values
                pchr->pos.y = tmpy;
            }
            else
            {
                pchr->vel.y += pchr->phys.apos_1.y * bump_str;
                pchr->pos_safe.y = tmpy;
            }
        }

        if ( ABS(pchr->phys.apos_0.z + pchr->phys.apos_1.z) > 0 )
        {
            tmpz = pchr->pos.z;
            pchr->pos.z += pchr->phys.apos_0.z + pchr->phys.apos_1.z;
            if ( pchr->pos.z < pchr->floor_level )
            {
                // restore the old values
                pchr->pos.z = tmpz;
            }
            else
            {
                pchr->vel.z += pchr->phys.apos_1.z * bump_str;
                pchr->pos_safe.z = tmpz;
            }
        }

        pchr->safe_valid = ( 0 == __chrhitawall(cnt, nrm) );
    }
}

//--------------------------------------------------------------------------------------------
void stat_return()
{
    // ZZ> This function brings mana and life back
    int cnt, owner, target, eve;

    // Do reload time
    cnt = 0;

    while ( cnt < MAX_CHR )
    {
        if ( ChrList[cnt].reloadtime > 0 )
        {
            ChrList[cnt].reloadtime--;
        }

        cnt++;
    }

    // Do stats
    if ( clock_stat >= ONESECOND )
    {
        // Reset the clock
        clock_stat -= ONESECOND;

        // Do all the characters
        for ( cnt = 0; cnt < MAX_CHR; cnt++ )
        {
            if ( !ChrList[cnt].on ) continue;

            // check for a level up
            do_level_up( cnt );

            // do the mana and life regen for "living" characters
            if ( ChrList[cnt].alive )
            {
                ChrList[cnt].mana += ( ChrList[cnt].manareturn / MANARETURNSHIFT );
                ChrList[cnt].mana = MAX(0, MIN(ChrList[cnt].mana, ChrList[cnt].manamax));

                ChrList[cnt].life += ChrList[cnt].lifereturn;
                ChrList[cnt].life = MAX(1, MIN(ChrList[cnt].life, ChrList[cnt].life));
            }
            if ( ChrList[cnt].grogtime > 0 )
            {
                ChrList[cnt].grogtime--;
            }
            if ( ChrList[cnt].dazetime > 0 )
            {
                ChrList[cnt].dazetime--;
            }
        }

        // Run through all the enchants as well
        for ( cnt = 0; cnt < MAX_ENC; cnt++ )
        {
            if ( !EncList[cnt].on ) continue;

            if ( EncList[cnt].time != 0 )
            {
                if ( EncList[cnt].time > 0 )
                {
                    EncList[cnt].time--;
                }

                owner = EncList[cnt].owner;
                target = EncList[cnt].target;
                eve = EncList[cnt].eve;

                // Do drains
                if ( ChrList[owner].alive )
                {
                    // Change life
                    ChrList[owner].life += EncList[cnt].ownerlife;
                    if ( ChrList[owner].life < 1 )
                    {
                        ChrList[owner].life = 1;
                        kill_character( owner, target );
                    }
                    if ( ChrList[owner].life > ChrList[owner].lifemax )
                    {
                        ChrList[owner].life = ChrList[owner].lifemax;
                    }

                    // Change mana
                    if ( !cost_mana( owner, -EncList[cnt].ownermana, target ) && EveList[eve].endifcantpay )
                    {
                        remove_enchant( cnt );
                    }
                }
                else if ( !EveList[eve].stayifnoowner )
                {
                    remove_enchant( cnt );
                }

                if ( EncList[cnt].on )
                {
                    if ( ChrList[target].alive )
                    {
                        // Change life
                        ChrList[target].life += EncList[cnt].targetlife;
                        if ( ChrList[target].life < 1 )
                        {
                            ChrList[target].life = 1;
                            kill_character( target, owner );
                        }
                        if ( ChrList[target].life > ChrList[target].lifemax )
                        {
                            ChrList[target].life = ChrList[target].lifemax;
                        }

                        // Change mana
                        if ( !cost_mana( target, -EncList[cnt].targetmana, owner ) && EveList[eve].endifcantpay )
                        {
                            remove_enchant( cnt );
                        }
                    }
                    else
                    {
                        remove_enchant( cnt );
                    }
                }
            }
            else
            {
                remove_enchant( cnt );
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    // ZZ> This function sets all of the character's starting tilt values
    int cnt;
    Uint8 twist;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList[cnt].on ) continue;

        if ( ChrList[cnt].stickybutt && VALID_TILE(PMesh, ChrList[cnt].onwhichfan) )
        {
            twist = PMesh->mem.tile_list[ChrList[cnt].onwhichfan].twist;
            ChrList[cnt].map_turn_y = map_twist_y[twist];
            ChrList[cnt].map_turn_x = map_twist_x[twist];
        }
        else
        {
            ChrList[cnt].map_turn_y = 32768;
            ChrList[cnt].map_turn_x = 32768;
        }
    }

}

//--------------------------------------------------------------------------------------------
int load_all_objects( const char *modname )
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
    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
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

                // load it
                skin += load_one_object( filename, skin );

                CapList[importobject].importslot = cnt;
            }
        }
    }

    // Search for .obj directories and load them
    importobject = -100;
    make_newloadname( modname, "objects" SLASH_STR, newloadname );
    filehandle = fs_findFirstFile( newloadname, "obj" );

    keeplooking = btrue;
    while ( filehandle != NULL )
    {
        sprintf( filename, "%s%s", newloadname, filehandle );
        skin += load_one_object( filename, skin );

        filehandle = fs_findNextFile();
    }

    fs_findClose();
    return skin;
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_read( FILE * fileread, chr_setup_info_t *pinfo )
{
    int cnt;
    char cTmp;

    // trap bad pointers
    if ( NULL == fileread || NULL == pinfo ) return bfalse;

    // check for another entry
    if ( !goto_colon( NULL, fileread, btrue ) ) return bfalse;

    fscanf( fileread, "%s", pinfo->spawn_name );
    for ( cnt = 0; cnt < sizeof(pinfo->spawn_name); cnt++ )
    {
        if ( pinfo->spawn_name[cnt] == '_' )  pinfo->spawn_name[cnt] = ' ';
    }

    pinfo->pname = pinfo->spawn_name;
    if ( 0 == strcmp( pinfo->spawn_name, "NONE") )
    {
        // Random pinfo->pname
        pinfo->pname = NULL;
    }

    fscanf( fileread, "%d", &pinfo->slot );

    fscanf( fileread, "%f%f%f", &pinfo->x, &pinfo->y, &pinfo->z );
    pinfo->x *= 128;  pinfo->y *= 128;  pinfo->z *= 128;

    pinfo->facing = NORTH;
    pinfo->attach = ATTACH_NONE;
    cTmp = fget_first_letter( fileread );
    if ( 'S' == toupper(cTmp) )  pinfo->facing = SOUTH;
    else if ( 'E' == toupper(cTmp) )  pinfo->facing = EAST;
    else if ( 'W' == toupper(cTmp) )  pinfo->facing = WEST;
    else if ( '?' == toupper(cTmp) )  pinfo->facing = RANDOM;
    else if ( 'L' == toupper(cTmp) )  pinfo->attach = ATTACH_LEFT;
    else if ( 'R' == toupper(cTmp) )  pinfo->attach = ATTACH_RIGHT;
    else if ( 'I' == toupper(cTmp) )  pinfo->attach = ATTACH_INVENTORY;

    fscanf( fileread, "%d%d%d%d%d", &pinfo->money, &pinfo->skin, &pinfo->passage, &pinfo->content, &pinfo->level );
    if (pinfo->skin >= MAXSKIN) pinfo->skin = rand() % MAXSKIN;     //Randomize skin?
	
    cTmp = fget_first_letter( fileread );
    pinfo->stat = ( 'T' == toupper(cTmp) );

    cTmp = fget_first_letter( fileread );   //BAD! Unused ghost value

    cTmp = fget_first_letter( fileread );
    pinfo->team = ( cTmp - 'A' ) % MAXTEAM;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo )
{
    // trap bad pointers
    if ( NULL == pinfo ) return bfalse;

    if ( ichr >= MAX_CHR || !ChrList[ichr].on ) return bfalse;

    ChrList[ichr].money += pinfo->money;
    if ( ChrList[ichr].money > MAXMONEY )  ChrList[ichr].money = MAXMONEY;
    if ( ChrList[ichr].money < 0 )  ChrList[ichr].money = 0;

    ChrList[ichr].ai.content = pinfo->content;
    ChrList[ichr].ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        pack_add_item( ichr, pinfo->parent );

        ChrList[ichr].ai.alert |= ALERTIF_GRABBED;  // Make spellbooks change
        ChrList[ichr].attachedto = pinfo->parent;  // Make grab work
        let_character_think( ichr );  // Empty the grabbed messages

        ChrList[ichr].attachedto = MAX_CHR;  // Fix grab

    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        Uint16 grip = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;
        attach_character_to_mount( ichr, pinfo->parent, grip );

        // Handle the "grabbed" messages
        let_character_think( ichr );
    }

    // Set the starting pinfo->level
    if ( pinfo->level > 0 )
    {
        while ( ChrList[ichr].experiencelevel < pinfo->level && ChrList[ichr].experience < MAXXP )
        {
            give_experience( ichr, 25, XPDIRECT, btrue );
            do_level_up( ichr );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void setup_characters( const char *modname )
{
    // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
    int new_object, tnc, local_index = 0;
    STRING newloadname;
    FILE *fileread;

    chr_setup_info_t info;

    // Turn all objects off
    free_all_objects();

    // Turn some back on
    make_newloadname( modname, "gamedat" SLASH_STR "spawn.txt", newloadname );
    fileread = fopen( newloadname, "r" );

    numpla = 0;
    info.parent = MAX_CHR;
    if ( NULL == fileread )
    {
        log_error( "Cannot read file: %s", newloadname );
    }
    else
    {
        info.parent = 0;

        while ( chr_setup_read( fileread, &info ) )
        {
            // Spawn the character
            if ( info.team < numplayer || !rtscontrol || info.team >= MAXPLAYER )
            {
                new_object = spawn_one_character( info.x, info.y, info.z, info.slot, info.team, info.skin, info.facing, info.pname, MAX_CHR );

                if ( MAX_CHR != new_object )
                {
                    // determine the attachment
                    if ( info.attach == ATTACH_NONE )
                    {
                        // Free character
                        info.parent = new_object;
                        make_one_character_matrix( new_object );
                    }

                    chr_setup_apply( new_object, &info );

                    // Turn on numpla input devices
                    if ( info.stat )
                    {

                        if ( 0 == importamount && numpla < playeramount )
                        {
                            if ( 0 == local_numlpla )
                            {
                                // the first player gets everything
                                add_player( new_object, numpla, (Uint32)(~0) );
                            }
                            else
                            {
                                int i;
                                Uint32 bits;

                                // each new player steals an input device from the 1st player
                                bits = 1 << local_numlpla;
                                for ( i = 0; i < MAXPLAYER; i++ )
                                {
                                    PlaList[i].device &= ~bits;
                                }

                                add_player( new_object, numpla, bits );
                            }

                        }
                        else if ( numpla < numimport && numpla < importamount && numpla < playeramount )
                        {
                            // Multiplayer import module
                            local_index = -1;
                            for ( tnc = 0; tnc < numimport; tnc++ )
                            {
                                if ( CapList[ChrList[new_object].model].importslot == local_slot[tnc] )
                                {
                                    local_index = tnc;
                                    break;
                                }
                            }

                            if ( -1 != local_index )
                            {
                                // It's a local numpla
                                add_player( new_object, numpla, local_control[local_index] );
                            }
                            else
                            {
                                // It's a remote numpla
                                add_player( new_object, numpla, INPUT_BITS_NONE );
                            }
                        }

                        // Turn on the stat display
                        statlist_add( new_object );
                    }
                }
            }
        }

        fclose( fileread );
    }

    clear_messages();

    // Make sure local players are displayed first
    statlist_sort();

    // Fix tilting trees problem
    tilt_characters_to_terrain();
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
            skin += load_one_object( filename, skin );

            filehandle = fs_findNextFile();

            keeplooking = ( filehandle != NULL );
        }
    }

    fs_findClose();
}

//--------------------------------------------------------------------------------------------
bool_t load_module( const char *smallname )
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

    make_randie();
    reset_teams();
    release_all_models();
    free_all_objects();
    reset_messages();
    prime_names();

    load_one_icon( "basicdat" SLASH_STR "nullicon" );
    load_ai_script( "basicdat" SLASH_STR "script.txt" );

    // generate the module directory
    snprintf( modname, sizeof(modname), "modules" SLASH_STR "%s" SLASH_STR, smallname );

    // load a bunch of assets that are used in the module
    load_global_waves( modname );
    reset_particles( modname );
    read_wawalite( modname );
    load_basic_textures( modname );

    //Load all objects
    {
        int skin;
        skin = load_all_objects(modname);
        load_all_global_objects(skin);
    }

    if ( NULL == mesh_load( modname, PMesh ) )
    {
        // do not cause the program to fail, in case we are using a script function to load a module
        // just return a failure value and log a warning message for debugging purposes
        log_warning( "Uh oh! Problems loading the mesh! (%s)\n", modname );
        return bfalse;
    }
    else
    {
        renderlist.all_count = 0;
        renderlist.ref_count = 0;
        renderlist.sha_count = 0;
        renderlist.drf_count = 0;
        renderlist.ndr_count = 0;
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

    return btrue;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( Uint16 character )
{
    // ZZ> This function makes sure a character has no attached particles
    Uint16 particle;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( PrtList[particle].on && PrtList[particle].attachedtocharacter == character )
        {
            free_one_particle_in_game( particle );
        }
    }

    // Set the alert for disaffirmation ( wet torch )
    ChrList[character].ai.alert |= ALERTIF_DISAFFIRMED;
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( Uint16 character )
{
    // ZZ> This function returns the number of particles attached to the given character
    Uint16 cnt, particle;

    cnt = 0;
    particle = 0;

    while ( particle < maxparticles )
    {
        if ( PrtList[particle].on && PrtList[particle].attachedtocharacter == character )
        {
            cnt++;
        }

        particle++;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles( Uint16 character )
{
    // ZZ> This function makes sure a character has all of it's particles
    Uint16 numberattached;
    Uint16 particle;

    numberattached = number_of_attached_particles( character );

    while ( numberattached < CapList[ChrList[character].model].attachedprtamount )
    {
        particle = spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z, 0, ChrList[character].model, CapList[ChrList[character].model].attachedprttype, character, GRIP_LAST + numberattached, ChrList[character].team, character, numberattached, MAX_CHR );
        if ( particle != TOTAL_MAX_PRT )
        {
            attach_particle_to_character( particle, character, PrtList[particle].grip );
        }

        numberattached++;
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    ChrList[character].ai.alert |= ALERTIF_REAFFIRMED;
}

//--------------------------------------------------------------------------------------------
void game_quit_module()
{
    // BB > all of the de-initialization code after the module actually ends

	release_module();
    close_session();

    // reset the "ui" mouse state
    pressed           = bfalse;
    clicked           = bfalse;
    pending_click     = bfalse;
    mouse_wheel_event = bfalse;

    // reset some of the module state variables
    beatmodule  = bfalse;
    exportvalid = bfalse;
    moduleactive = bfalse;
    hostactive   = bfalse;
}


//-----------------------------------------------------------------
bool_t game_init_module( const char * modname, Uint32 seed )
{
    // BB> all of the initialization code before the module actually starts

    srand( seed );

    // start the new module
    if ( !load_module( modname ) )
    {
        moduleactive = bfalse;
        gameactive   = bfalse;
        return bfalse;
    };

    // make sure the per-module configuration settings are correct
    setup_synch( &cfg );

    pressed = bfalse;

    make_onwhichfan();
    camera_reset(&gCamera);
    reset_timers();
    make_character_matrices( update_wld != 0 );
    attach_particles();

    net_initialize();
    if ( networkon )
    {
        net_sayHello();
    }

    // Let the game go
    moduleactive = btrue;
    randsave = 0;
    srand( randsave );

    return moduleactive;
}

//-----------------------------------------------------------------
bool_t game_update_imports()
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
        if ( !ChrList[character].on ) continue;

        is_local = ( 0 != PlaList[cnt].device );

        // find the saved copy of the players that are in memory right now
        for ( tnc = 0; tnc < loadplayer_count; tnc++ )
        {
            if ( 0 == strcmp( loadplayer[tnc].dir, str_encode_path(ChrList[character].name) ) )
            {
                break;
            }
        }

        if ( tnc == loadplayer_count )
        {
            log_warning( "game_update_imports() - cannot find exported file for \"%s\" (\"%s\") \n", ChrList[character].name, str_encode_path(ChrList[character].name) ) ;
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

void release_module()
{
    // ZZ> This function frees up memory used by the module

	// Disable EMP
    local_senseenemiesID = IDSZ_NONE;
    local_senseenemies = MAX_CHR;

    release_all_icons();
    release_all_titleimages();
    release_bars();
    release_map();
    release_all_textures();
    release_all_models();
    reset_all_ai_scripts();

    mesh_delete( PMesh );

    // Close and then reopen SDL_mixer. it's easier than manually unloading each sound ;)
    sound_restart();

    // fade out any sound
    sound_fade_all();
}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
    // ZZ> This function attaches particles to their characters so everything gets
    //     drawn right
    int cnt;

    cnt = 0;

    while ( cnt < maxparticles )
    {
        if ( PrtList[cnt].on && PrtList[cnt].attachedtocharacter != MAX_CHR )
        {
            attach_particle_to_character( cnt, PrtList[cnt].attachedtocharacter, PrtList[cnt].grip );

            // Correct facing so swords knock characters in the right direction...
            if ( PipList[PrtList[cnt].pip].damfx&DAMFX_TURN )
            {
                PrtList[cnt].facing = ChrList[PrtList[cnt].attachedtocharacter].turn_z;
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int add_player( Uint16 character, Uint16 player, Uint32 device )
{
    // ZZ> This function adds a player, returning bfalse if it fails, btrue otherwise
    int cnt;

    if ( !PlaList[player].valid )
    {
        ChrList[character].isplayer = btrue;
        PlaList[player].index = character;
        PlaList[player].valid = btrue;
        PlaList[player].device = device;

        PlaList[player].latchx = 0;
        PlaList[player].latchy = 0;
        PlaList[player].latchbutton = 0;

        for ( cnt = 0; cnt < MAXLAG; cnt++ )
        {
            PlaList[player].tlatch[cnt].x      = 0;
            PlaList[player].tlatch[cnt].y      = 0;
            PlaList[player].tlatch[cnt].button = 0;
            PlaList[player].tlatch[cnt].time   = (Uint32)(~0);
        }

        if ( device != INPUT_BITS_NONE )
        {
            local_noplayers = bfalse;
            ChrList[character].islocalplayer = btrue;
            local_numlpla++;
        }

        numpla++;
        return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void let_all_characters_think()
{
    // ZZ> This function lets every computer controlled character do AI stuff
    int character;
    static Uint32 last_update = (Uint32)(~0);

    // make sure there is only one update per frame;
    if ( update_wld == last_update ) return;
    last_update = update_wld;

    numblip = 0;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        bool_t is_crushed, is_cleanedup, can_think;

        if ( !ChrList[character].on ) continue;

        // check for actions that must always be handled
        is_cleanedup = ( 0 != ( ChrList[character].ai.alert & ALERTIF_CLEANEDUP ) );
        is_crushed   = ( 0 != ( ChrList[character].ai.alert & ALERTIF_CRUSHED   ) );

        // let the script run sometimes even if the item is in your backpack
        can_think = !ChrList[character].inpack || CapList[ChrList[character].model].isequipment;

        // only let dead/destroyed things think if they have beem crushed/cleanedup
        if ( ( ChrList[character].alive && can_think ) || is_crushed || is_cleanedup )
        {
            // Figure out alerts that weren't already set
            set_alerts( character );

            // Crushed characters shouldn't be alert to anything else
            if ( is_crushed )  { ChrList[character].ai.alert = ALERTIF_CRUSHED; ChrList[character].ai.timer = update_wld + 1; }

            // Cleaned up characters shouldn't be alert to anything else
            if ( is_cleanedup )  { ChrList[character].ai.alert = ALERTIF_CLEANEDUP; ChrList[character].ai.timer = update_wld + 1; }

            let_character_think( character );
        }
    }
}


//struct s_line_of_sight_info
//{
//    float x0, y0;
//    float x1, y1;
//    Uint32 stopped_by;
//
//    Uint16 collide_chr;
//    Uint32 collide_fx;
//    int    collide_x;
//    int    collide_y;
//};
//
//typedef struct s_line_of_sight_info line_of_sight_info_t;
//
//bool_t do_line_of_sight( line_of_sight_info_t * plos )
//{
//    int Dx, Dy;
//
//    int xstep, ystep;
//    int TwoDy, TwoDyTwoDx, E;
//    int y;
//    int xDraw, yDraw;
//
//    int steep;
//
//    if(NULL == plos) return bfalse;
//
//    int steep = (ABS(Dy) >= ABS(Dx));
//
//    int Dx = plos->x1 - plos->x0;
//    int Dy = plos->y1 - plos->y0;
//
//    if (steep)
//    {
//        SWAP(int, plos->x0, plos->y0);
//        SWAP(int, plos->x1, plos->y1);
//
//        // recompute Dx, Dy after swap
//        Dx = plos->x1 - plos->x0;
//        Dy = plos->y1 - plos->y0;
//    }
//
//    xstep = 1;
//    if (Dx < 0)
//    {
//        xstep = -1;
//        Dx = -Dx;
//    }
//
//    ystep = 1;
//    if (Dy < 0)
//    {
//        ystep = -1;
//        Dy = -Dy;
//    }
//
//    TwoDy = 2*Dy;
//    TwoDyTwoDx = TwoDy - 2*Dx; // 2*Dy - 2*Dx
//    E = TwoDy - Dx; //2*Dy - Dx
//    y = plos->y0;
//    block_last = INVALID_BLOCK;
//    for (int x = plos->x0; x != plos->x1; x += xstep)
//    {
//        int fan;
//
//        if (steep)
//        {
//            xDraw = y;
//            yDraw = x;
//        }
//        else
//        {
//            xDraw = x;
//            yDraw = y;
//        }
//
//        // plot
//        fan = meshgetfan(xDraw, yDraw);
//        if( INVALID_FAN != fan && fan != last_fan )
//        {
//            // collide the ray with the mesh
//
//            if( 0!= (PMesh->mem.tile_list[fan].fx & plos->stopped_by) )
//            {
//                plos->collide_x  = xDraw;
//                plos->collide_y  = yDraw;
//                plos->collide_fx = PMesh->mem.tile_list[fan].fx & plos->stopped_by;
//
//                return btrue;
//            }
//
//            last_fan = fan;
//        }
//
//        block = meshgetblock(xDraw, yDraw);
//        if( INVALID_BLOCK != block && block != block_last )
//        {
//            // collide the ray with all items/characters in this block
//
//        };
//
//        // next
//        if (E > 0)
//        {
//            E += TwoDyTwoDx; //E += 2*Dy - 2*Dx;
//            y = y + ystep;
//        } else
//        {
//            E += TwoDy; //E += 2*Dy;
//        }
//    }
//}


//--------------------------------------------------------------------------------------------
bool_t game_begin_menu( int which )
{
    if ( !gamemenuactive )
    {
        gamemenu_depth = mnu_get_menu_depth();
    }

    if ( mnu_begin_menu( which ) )
    {
        mnu_draw_background = bfalse;
        gamemenuactive = btrue;
    }

    return gamemenuactive;
}

//--------------------------------------------------------------------------------------------
static void game_end_menu()
{
    mnu_end_menu();

    if ( mnu_get_menu_depth() <= gamemenu_depth )
    {
        gamemenuactive = bfalse;
        gamemenu_depth = -1;
    }
}

//--------------------------------------------------------------------------------------------
void game_finish_module()
{
    // export all the local and remote characters
    game_update_imports();

    // quit the old module
    game_quit_module();
}

//--------------------------------------------------------------------------------------------
bool_t game_begin_module( const char * modname, Uint32 seed )
{
    hostactive = btrue; // very important or the input will not work

    SDL_WM_GrabInput ( SDL_GRAB_ON );                            // grab the cursor
    SDL_ShowCursor( cfg.hide_mouse ? SDL_DISABLE : SDL_ENABLE );

    return game_init_module( modname, seed );
};

//--------------------------------------------------------------------------------------------
/*Uint8 find_target_in_block( int x, int y, float chrx, float chry, Uint16 facing,
Uint8 onlyfriends, Uint8 anyone, Uint8 team,
Uint16 donttarget, Uint16 oldtarget )
{
// ZZ> This function helps find a target, returning btrue if it found a decent target
int cnt;
Uint16 angle;
Uint16 charb;
Uint8 enemies, returncode;
Uint32 fanblock;
int distance;

returncode = bfalse;

// Current fanblock
if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
{
fanblock = mesh_get_block_int(PMesh, x,y);

enemies = bfalse;
if ( !onlyfriends ) enemies = btrue;

charb = bumplist[fanblock].chr;
cnt = 0;
while ( cnt < bumplist[fanblock].chrnum )
{
if ( ChrList[charb].alive && !ChrList[charb].invictus && charb != donttarget && charb != oldtarget )
{
if ( anyone || ( ChrList[charb].team == team && onlyfriends ) || ( TeamList[team].hatesteam[ChrList[charb].team] && enemies ) )
{
distance = ABS( ChrList[charb].pos.x - chrx ) + ABS( ChrList[charb].pos.y - chry );
if ( distance < globestdistance )
{
angle = ( ATAN2( ChrList[charb].pos.y - chry, ChrList[charb].pos.x - chrx ) + PI ) * 0xFFFF / ( TWO_PI );
angle = facing - angle;
if ( angle < globestangle || angle > ( 0xFFFF - globestangle ) )
{
returncode = btrue;
globesttarget = charb;
globestdistance = distance;
glouseangle = angle;
if ( angle  > 32767 )
globestangle = -angle;
else
globestangle = angle;
}
}
}
}
charb = ChrList[charb].bumpnext;
cnt++;
}
}
return returncode;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 find_target( float chrx, float chry, Uint16 facing,
Uint16 targetangle, Uint8 onlyfriends, Uint8 anyone,
Uint8 team, Uint16 donttarget, Uint16 oldtarget )
{
// This function finds the best target for the given parameters
Uint8 done;
int x, y;

x = chrx;
y = chry;
x = x >> BLOCK_BITS;
y = y >> BLOCK_BITS;
globestdistance = 9999;
globestangle = targetangle;
done = find_target_in_block( x, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x + 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x - 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
if ( done ) return globesttarget;

done = find_target_in_block( x + 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x + 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x - 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
done |= find_target_in_block( x - 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
if ( done ) return globesttarget;

return MAX_CHR;
}*/


//--------------------------------------------------------------------------------------------
void free_all_objects( void )
{
    // BB > every instance of the three object types used in the game.

    free_all_particles();
    free_all_enchants();
    free_all_characters();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t add_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    Uint32 hashval = 0;
    int count;
    bool_t found;

    hash_node_t * n;
    co_data_t   * d;

    // create a hash that is order-independent
    hashval = (ichr_a* 0x0111 + 0x006E) + (ichr_b* 0x0111 + 0x006E);
    hashval &= 0xFF;

    found = bfalse;
    count = chr_co_list->subcount[hashval];
    if( count > 0)
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for(i = 0; i<count; i++)
        {
            d = (co_data_t *)(n->data);
            if(d->chra == ichr_a && d->chrb == ichr_b)
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if(!found)
    {
        // pick a free collision data
        assert((*cdata_count) < CHR_MAX_COLLISIONS);
        d = cdata + (*cdata_count);
        (*cdata_count)++;

        // fill it in
        d->chra = ichr_a;
        d->chrb = ichr_b;
        d->prtb = TOTAL_MAX_PRT;

        // generate a new hash node
        assert((*hn_count) < COLLISION_HASH_NODES);
        n = hnlst + (*hn_count);
        (*hn_count)++;
        hash_node_ctor(n, (void*)d);

        // insert the node
        chr_co_list->subcount[hashval]++;
        chr_co_list->sublist[hashval] = hash_node_insert_before(chr_co_list->sublist[hashval], n);
    }

    return !found;
}


//--------------------------------------------------------------------------------------------
bool_t add_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    bool_t found;
    int    count;
    Uint32 hashval = 0;

    hash_node_t * n;
    co_data_t   * d;

    // create a hash that is order-independent
    hashval = (ichr_a * 0x0111 + 0x006E) + (iprt_b * 0x0111 + 0x006E);
    hashval &= 0xFF;

    found = bfalse;
    count = chr_co_list->subcount[hashval];
    if( count > 0)
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for(i = 0; i<count; i++)
        {
            d = (co_data_t *)(n->data);
            if(d->chra == ichr_a && d->prtb == iprt_b)
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if(!found)
    {
        // pick a free collision data
        assert((*cdata_count) < CHR_MAX_COLLISIONS);
        d = cdata + (*cdata_count);
        (*cdata_count)++;

        // fill it in
        d->chra = ichr_a;
        d->chrb = MAX_CHR;
        d->prtb = iprt_b;

        // generate a new hash node
        assert((*hn_count) < COLLISION_HASH_NODES);
        n = hnlst + (*hn_count);
        (*hn_count)++;
        hash_node_ctor(n, (void*)d);

        // insert the node
        chr_co_list->subcount[hashval]++;
        chr_co_list->sublist[hashval] = hash_node_insert_before(chr_co_list->sublist[hashval], n);
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b )
{
    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t collide_z  = bfalse;

    float xa,ya,za;
    float xb,yb,zb;
    float dxy, dx,dy,depth_z;

    chr_t *pchr_a, *pchr_b;
    cap_t *pcap_a, *pcap_b;

    // Don't collide with self
    if( ichr_a == ichr_b ) return bfalse;

    // Ignore invalid characters
    if( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    if( INVALID_CAP(pchr_a->model) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // Ignore invalid characters
    if( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if( INVALID_CAP(pchr_b->model) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // don't collide if there is no interaction
    if( 0 == pchr_a->bumpsize || 0 == pchr_b->bumpsize ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pchr_b->is_hidden ) return bfalse;

    // First check absolute value diamond
    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dxy = dx + dy;

    depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

    // estimate the collisions this frame
    collide_x  = (dx  <= pchr_a->bumpsize    + pchr_b->bumpsize   );
    collide_y  = (dy  <= pchr_a->bumpsize    + pchr_b->bumpsize   );
    collide_xy = (dxy <= pchr_a->bumpsizebig + pchr_b->bumpsizebig);

    if( (pchr_a->platform && pcap_b->canuseplatforms) ||
        (pchr_b->platform && pcap_a->canuseplatforms) )
    {
        collide_z  = (depth_z > -PLATTOLERANCE);
    }
    else
    {
        collide_z  = (depth_z > 0);
    }

    if( !collide_x || !collide_y || !collide_z || !collide_xy ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b )
{
    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t collide_z  = bfalse;

    float xa,ya,za;
    float xb,yb,zb;
    float dxy, dx,dy,depth_z;

    chr_t * pchr_a;
    prt_t * pprt_b;

    // Ignore invalid characters
    if( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    // Ignore invalid characters
    if( INVALID_PRT(iprt_b) ) return bfalse;
    pprt_b = PrtList + iprt_b;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pprt_b->is_hidden ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pprt_b->pos.x;
    yb = pprt_b->pos.y;
    zb = pprt_b->pos.z;

    // don't collide if there is no interaction
    if( 0 == pchr_a->bumpsize || 0 == pprt_b->bumpsize ) return bfalse;

    // First check absolute value diamond
    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dxy = dx + dy;

    depth_z = MIN( za + pchr_a->bumpheight, zb + pprt_b->bumpheight ) - MAX(za, zb - pprt_b->bumpheight);

    // estimate the collisions this frame
    collide_x  = (dx  <= pchr_a->bumpsize    + pprt_b->bumpsize   );
    collide_y  = (dy  <= pchr_a->bumpsize    + pprt_b->bumpsize   );
    collide_xy = (dxy <= pchr_a->bumpsizebig + pprt_b->bumpsizebig);
    collide_z  = (depth_z > 0);

    if( !collide_x || !collide_y || !collide_z || !collide_xy ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_instance_update_vertices( chr_instance_t * pinst, int vmin, int vmax )
{
    int    i;
    float  flip;
    bool_t vertices_match, frames_match;

    mad_t * pmad;

    if( NULL == pinst ) return bfalse;

    // get the model. try to heal a bad model.
    if( INVALID_MAD(pinst->imad) ) return bfalse;
    pmad = MadList + pinst->imad;

    // handle the default parameters
    if( vmin < 0 ) vmin = 0;
    if( vmax < 0 ) vmax = pmad->md2.vertices - 1;

    vmin = CLIP(vmin, 0, pmad->md2.vertices - 1);
    vmax = CLIP(vmax, 0, pmad->md2.vertices - 1);

    flip = pinst->lip / 256.0f;

    // test to see if we have already calculated this data
    vertices_match = (pinst->save_vmin <= vmin) && (pinst->save_vmax >= vmax);

    frames_match = ( pinst->save_frame_nxt == pinst->frame_nxt && flip == 1.0f ) ||
                   ( pinst->save_frame_lst == pinst->frame_lst && flip == 0.0f ) ||
                   ( pinst->save_frame_nxt == pinst->frame_nxt && pinst->save_frame_lst == pinst->frame_lst && pinst->save_flip == flip );

    if( frames_match && vertices_match ) return bfalse;

    if( pinst->frame_nxt == pinst->frame_lst || flip == 0.0f )
    {
        for( i=vmin; i<=vmax; i++)
        {
            Uint16 vrta_lst;

            pinst->vlst[i].pos[XX] = Md2FrameList[pinst->frame_lst].vrtx[i];
            pinst->vlst[i].pos[YY] = Md2FrameList[pinst->frame_lst].vrty[i];
            pinst->vlst[i].pos[ZZ] = Md2FrameList[pinst->frame_lst].vrtz[i];
            pinst->vlst[i].pos[WW] = 1.0f;

            vrta_lst = Md2FrameList[pinst->frame_lst].vrta[i];

            pinst->vlst[i].nrm[XX] = kMd2Normals[vrta_lst][XX];
            pinst->vlst[i].nrm[YY] = kMd2Normals[vrta_lst][YY];
            pinst->vlst[i].nrm[ZZ] = kMd2Normals[vrta_lst][ZZ];

            pinst->vlst[i].env[XX] = indextoenvirox[vrta_lst];
        }
    }
    else if( flip == 1.0f )
    {
        for( i=vmin; i<=vmax; i++)
        {
            Uint16 vrta_nxt;

            pinst->vlst[i].pos[XX] = Md2FrameList[pinst->frame_nxt].vrtx[i];
            pinst->vlst[i].pos[YY] = Md2FrameList[pinst->frame_nxt].vrty[i];
            pinst->vlst[i].pos[ZZ] = Md2FrameList[pinst->frame_nxt].vrtz[i];
            pinst->vlst[i].pos[WW] = 1.0f;

            vrta_nxt = Md2FrameList[pinst->frame_nxt].vrta[i];

            pinst->vlst[i].nrm[XX] = kMd2Normals[vrta_nxt][XX];
            pinst->vlst[i].nrm[YY] = kMd2Normals[vrta_nxt][YY];
            pinst->vlst[i].nrm[ZZ] = kMd2Normals[vrta_nxt][ZZ];

            pinst->vlst[i].env[XX] = indextoenvirox[vrta_nxt];
        }
    }
    else
    {
        for( i=vmin; i<=vmax; i++)
        {
            Uint16 vrta_lst, vrta_nxt;

            pinst->vlst[i].pos[XX] = Md2FrameList[pinst->frame_lst].vrtx[i] + (Md2FrameList[pinst->frame_nxt].vrtx[i] - Md2FrameList[pinst->frame_lst].vrtx[i]) * flip;
            pinst->vlst[i].pos[YY] = Md2FrameList[pinst->frame_lst].vrty[i] + (Md2FrameList[pinst->frame_nxt].vrty[i] - Md2FrameList[pinst->frame_lst].vrty[i]) * flip;
            pinst->vlst[i].pos[ZZ] = Md2FrameList[pinst->frame_lst].vrtz[i] + (Md2FrameList[pinst->frame_nxt].vrtz[i] - Md2FrameList[pinst->frame_lst].vrtz[i]) * flip;
            pinst->vlst[i].pos[WW] = 1.0f;

            vrta_lst = Md2FrameList[pinst->frame_lst].vrta[i];
            vrta_nxt = Md2FrameList[pinst->frame_nxt].vrta[i];

            pinst->vlst[i].nrm[XX] = kMd2Normals[vrta_lst][XX] + (kMd2Normals[vrta_nxt][XX] - kMd2Normals[vrta_lst][XX]) * flip;
            pinst->vlst[i].nrm[YY] = kMd2Normals[vrta_lst][YY] + (kMd2Normals[vrta_nxt][YY] - kMd2Normals[vrta_lst][YY]) * flip;
            pinst->vlst[i].nrm[ZZ] = kMd2Normals[vrta_lst][ZZ] + (kMd2Normals[vrta_nxt][ZZ] - kMd2Normals[vrta_lst][ZZ]) * flip;

            pinst->vlst[i].env[XX] = indextoenvirox[vrta_lst] + (indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst]) * flip;
        }
    }

    pinst->save_frame     = update_wld;
    pinst->save_vmin      = MIN(pinst->save_vmin, vmin);
    pinst->save_vmax      = MAX(pinst->save_vmax, vmax);
    pinst->save_frame_nxt = pinst->frame_nxt;
    pinst->save_frame_lst = pinst->frame_lst;

    return btrue;
}