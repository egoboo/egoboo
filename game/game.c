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

#include "game.h"

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
#include "mesh.h"

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
#include <float.h>

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
    STRING     spawn_name;
    char      *pname;
    Sint32     slot;
    GLvector3  pos;
    int        passage, content, money, level, skin;
    bool_t     stat;
    Uint8      team;
    Uint16     facing;
    Uint16     attach;
    Uint16     parent;
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

static ego_mpd_t            _mesh[2];
static camera_t          _camera[2];
static ego_process_t     _eproc;
static menu_process_t    _mproc;
static game_process_t    _gproc;
static module_instance_t gmod;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t  overrideslots      = bfalse;

bool_t    screenshotkeyready = btrue;

//End text
char   endtext[MAXENDTEXT] = { '\0' };
int    endtextwrite = 0;

// Status displays
bool_t staton     = btrue;
int    numstat    = 0;
Uint16 statlist[MAXSTAT];

ego_mpd_t         * PMesh   = _mesh + 0;
camera_t          * PCamera = _camera + 0;
module_instance_t * PMod    = &gmod;
ego_process_t     * EProc   = &_eproc;
menu_process_t    * MProc   = &_mproc;
game_process_t    * GProc   = &_gproc;

bool_t  pitskill  = bfalse;
bool_t  pitsfall  = bfalse;
Uint32  pitx;
Uint32  pity;
Uint32  pitz;

Uint16  glouseangle = 0;                                        // actually still used

animtile_data_t       animtile_data;
animtile_instance_t   animtile[2];
damagetile_data_t     damagetile_data;
damagetile_instance_t damagetile;
weather_data_t        weather_data;
weather_instance_t    weather;
water_data_t          water_data;
water_instance_t      water;
fog_data_t            fog_data;
fog_instance_t        fog;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void make_randie();
static void reset_teams();
static void reset_messages();
static void reset_timers();
static void _quit_game( ego_process_t * pgame );

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
static bool_t game_load_module_data( const char *smallname );
static void   game_release_module_data();

static void   setup_characters( const char *modname );
static void   setup_alliances( const char *modname );
static int    load_one_object( const char* tmploadname , int skin );
static int    load_all_objects( const char *modname );
static void   load_all_global_objects(int skin);

static bool_t chr_setup_read( FILE * fileread, chr_setup_info_t *pinfo );
static bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo );
static void   setup_characters( const char *modname );

// Model stuff
static void log_madused( const char *savename );

// Collision stuff
static bool_t add_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );
static bool_t add_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );

static bool_t detect_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b );
static bool_t detect_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b );

static bumplist_t bumplist[MAXMESHFAN/16];

static int           chr_co_count = 0;
static hash_list_t * chr_co_list;

// "process" management

static int do_ego_proc_begin( ego_process_t * eproc );
static int do_ego_proc_running( ego_process_t * eproc );
static int do_ego_proc_leaving( ego_process_t * eproc );
static int do_ego_proc_run( ego_process_t * eproc, double frameDuration );

static int do_menu_proc_begin( menu_process_t * mproc );
static int do_menu_proc_running( menu_process_t * mproc );
static int do_menu_proc_leaving( menu_process_t * mproc );
static int do_menu_proc_run( menu_process_t * mproc, double frameDuration );

static int do_game_proc_begin( game_process_t * gproc );
static int do_game_proc_running( game_process_t * gproc );
static int do_game_proc_leaving( game_process_t * gproc );
static int do_game_proc_run( game_process_t * gproc, double frameDuration );

// profile handling
static void init_all_pip();
static void init_all_eve();
static void init_all_cap();
static void init_all_mad();

static void release_all_pip();
static void release_all_eve();
static void release_all_cap();
static void release_all_mad();

// misc
static bool_t game_begin_menu( menu_process_t * mproc, which_menu_t which );
static void   game_end_menu( menu_process_t * mproc );

static void   memory_cleanUp(void);

static void   do_game_hud();

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
    if ( ( CapList[profile].cancarrytonextmodule || !CapList[profile].isitem ) && PMod->exportvalid )
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

    // Don't export if the module isn't running
    if ( !process_instance_running( PROC_PBASE(GProc) ) ) return;

    //Stop if export isnt valid
    if (!PMod->exportvalid) return;

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
        for ( number = 2, item = ChrList[character].pack_next;
                number < 8 && item != MAX_CHR;
                item = ChrList[item].pack_next )
        {
            if ( ChrList[item].isitem )
            {
                export_one_character( item, character, number++, is_local );
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void _quit_game( ego_process_t * pgame )
{
    // ZZ> This function exits the game entirely

    if ( process_instance_running( PROC_PBASE(pgame) ) )
    {
        game_quit_module();
    }

    // tell the game to kill itself
    process_instance_kill( PROC_PBASE(pgame) );

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

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    if ( pchr->inst.imad > MAX_PROFILE || !MadList[pchr->inst.imad].loaded ) return;
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

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    if ( pchr->inst.imad > MAX_PROFILE || !MadList[pchr->inst.imad].loaded ) return;
    pmad = MadList + pchr->inst.imad;

    if ( pmad->actionvalid[action] )
    {
        int framesinaction, frame_stt, frame_end;

        pchr->nextaction = ACTION_DA;
        pchr->action = ACTION_DA;
        pchr->actionready = btrue;

        framesinaction = (pmad->actionend[action] - pmad->actionstart[action]) + 1;
        if ( framesinaction <= 1 )
        {
            frame_stt = pmad->actionstart[action];
            frame_end = frame_stt;
        }
        else
        {
            frame = MIN(frame, framesinaction);
            frame_stt = pmad->actionstart[action] + frame;

            frame = MIN(frame + 1, framesinaction);
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
    int cnt, numdead, numalive;

    // Check for all local players being dead
    local_allpladead   = bfalse;
    local_seeinvisible = bfalse;
    local_seekurse     = bfalse;

    numdead = numalive = 0;
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid) continue;

        // only interested in local players
        if ( INPUT_BITS_NONE == PlaList[cnt].device ) continue;

        // fix bad players
        if ( INVALID_CHR(PlaList[cnt].index) )
        {
            PlaList[cnt].valid = bfalse;
            continue;
        }

        if ( ChrList[PlaList[cnt].index].alive )
        {
            numalive++;

            if ( ChrList[PlaList[cnt].index].canseeinvisible )
            {
                local_seeinvisible = btrue;
            }

            if ( ChrList[PlaList[cnt].index].canseekurse )
            {
                local_seekurse = btrue;
            }
        }
        else
        {
            numdead++;
        }
    }

    if ( numdead >= local_numlpla )
    {
        local_allpladead = btrue;
    }

    // check for autorespawn
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid ) continue;

        if ( INVALID_CHR(PlaList[cnt].index) ) continue;

        if ( !ChrList[PlaList[cnt].index].alive )
        {
            if ( cfg.difficulty < GAME_HARD && local_allpladead && SDLKEYDOWN( SDLK_SPACE ) && PMod->respawnvalid && 0 == revivetimer )
            {
                respawn_character( PlaList[cnt].index );
                ChrList[cnt].experience *= EXPKEEP;  // Apply xp Penality
                if (cfg.difficulty > GAME_EASY) ChrList[cnt].money *= EXPKEEP;
            }
        }
    }

    // get all player latches from the "remotes"
    sv_talkToRemotes();

    // [claforte Jan 6th 2001]
    // TODO: Put that back in place once networking is functional.
    while ( clock_wld < clock_all && numplatimes > 0 )
    {
        // Important stuff to keep in sync
        srand( PMod->randsave );
        PMod->randsave = rand();

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
        looped_update_all_sound();

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

    if ( PNet->on )
    {
        if ( numplatimes == 0 )
        {
            // The remote ran out of messages, and is now twiddling its thumbs...
            // Make it go slower so it doesn't happen again
            clock_wld += 25;
        }
        if ( numplatimes > 3 && !PNet->hostactive )
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

    for ( teama = 0; teama < MAXTEAM; teama++ )
    {
        // Make the team hate everyone
        for ( teamb = 0; teamb < MAXTEAM; teamb++ )
        {
            TeamList[teama].hatesteam[teamb] = btrue;
        }

        // Make the team like itself
        TeamList[teama].hatesteam[teama] = bfalse;

        // Set defaults
        TeamList[teama].leader = NOLEADER;
        TeamList[teama].sissy = 0;
        TeamList[teama].morale = 0;
    }

    // Keep the null team neutral
    for ( teama = 0; teama < MAXTEAM; teama++ )
    {
        TeamList[teama].hatesteam[NULLTEAM] = bfalse;
        TeamList[NULLTEAM].hatesteam[teama] = bfalse;
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

    for ( cnt = 0; cnt < MAXMESSAGE; cnt++ )
    {
        msgtime[cnt] = 0;
    }

    for ( cnt = 0; cnt < MAXTOTALMESSAGE; cnt++ )
    {
        msgindex[cnt] = 0;
    }

    msgtext[0] = '\0';
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
int game_do_menu( menu_process_t * mproc )
{
    // BB> do menus

    int menuResult;
    bool_t need_menu = bfalse;

    need_menu = bfalse;
    if ( flip_pages_requested() )
    {
        // someone else (and that means the game) has drawn a frame
        // so we just need to draw the menu over that frame
        need_menu = btrue;

        // force the menu to be displayed immediately when the game stops
        mproc->base.dtime = 1.0f / (float)cfg.framelimit;
    }
    else if (  !process_instance_running( PROC_PBASE(GProc) ) )
    {
        // the menu's frame rate is controlled by a timer
        mproc->frame_now = SDL_GetTicks();
        if (mproc->frame_now > mproc->frame_next)
        {
            float  frameskip = (float)TICKS_PER_SEC / (float)cfg.framelimit;
            mproc->frame_next = mproc->frame_now + frameskip; //FPS limit

            need_menu = btrue;
            mproc->base.dtime = 1.0f / (float)cfg.framelimit;
        }
    }

    menuResult = 0;
    if ( need_menu )
    {
        ui_beginFrame( mproc->base.dtime );
        {
            menuResult = doMenu( mproc->base.dtime );
            request_flip_pages();
        }
        ui_endFrame();
    }

    return menuResult;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_ego_proc_begin( ego_process_t * eproc )
{
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

    // do basic system initialization
    sdl_init();
    ogl_init();
    net_initialize();

    log_info( "Initializing SDL_Image version %d.%d.%d... ", SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL ); \
    GLSetup_SupportedFormats();

    // read all the scantags
    scantag_read_all( "basicdat" SLASH_STR "scancode.txt" );
    input_settings_load( "controls.txt" );

    // synchronoze the config values with the various game subsystems
    // do this acter the sdl_init() and ogl_init() in case the config values are clamped
    // to valid values
    setup_synch( &cfg );

    // initialize the sound system
    sound_initialize();
    load_all_music_sounds();

    // make sure that a bunch of stuff gets initialized properly
    module_instance_init( &gmod );
    init_all_graphics();
    init_all_profiles();
    free_all_objects();
    mesh_new( PMesh );

    // setup the menu system's gui
    ui_initialize( "basicdat" SLASH_STR "Negatori.ttf", 24 );
    font_load( "basicdat" SLASH_STR "font", "basicdat" SLASH_STR "font.txt" );  // must be done after init_all_graphics()

    // clear out the import directory
    empty_import_directory();

    // register the memory_cleanUp function to automatically run whenever the program exits
    atexit( memory_cleanUp );

    // initialize the game process (not active)
    game_process_init( GProc );

    // initialize the menu process (active)
    menu_process_init( MProc );
    process_instance_start( PROC_PBASE(MProc) );

    // Initialize the process
    process_instance_start( PROC_PBASE(eproc) );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_running( ego_process_t * eproc )
{
    bool_t menu_valid, game_valid;

    if ( !process_instance_validate( PROC_PBASE(eproc) ) ) return -1;

    eproc->was_active  = eproc->base.valid;

    menu_valid = process_instance_validate( PROC_PBASE(MProc) );
    game_valid = process_instance_validate( PROC_PBASE(GProc) );
    if ( !menu_valid && !game_valid )
    {
        process_instance_kill( PROC_PBASE(eproc) );
        return 1;
    }

    if ( eproc->base.paused ) return 0;

    if ( process_instance_running( PROC_PBASE(MProc) ) )
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
    eproc->frameDuration = clk_getFrameDuration();

    // read the input values
    input_read();

    if ( pickedmodule_ready && !process_instance_running( PROC_PBASE(MProc) ) )
    {
        // a new module has been picked

        // reset the flag
        pickedmodule_ready = bfalse;

        // start the game process
        process_instance_start( PROC_PBASE(GProc) );
    }

    // Test the panic button
    if ( SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) )
    {
        // terminate the program
        process_instance_kill( PROC_PBASE(eproc) );
    }

    // Check for screenshots
    if ( !SDLKEYDOWN( SDLK_F11 ) )
    {
        screenshotkeyready = btrue;
    }
    else if ( screenshotkeyready && SDLKEYDOWN( SDLK_F11 ) && keyb.on )
    {
        screenshotkeyready = bfalse;

        if ( !dump_screenshot() )                // Take the shot, returns bfalse if failed
        {
            debug_message( "Error writing screenshot!" );
            log_warning( "Error writing screenshot\n" );    // Log the error in log.txt
        }
    }

    // handle an escape by passing it on to all active sub-processes
    if ( eproc->escape_requested )
    {
        eproc->escape_requested = bfalse;

        if ( process_instance_running( PROC_PBASE(GProc) ) )
        {
            GProc->escape_requested = btrue;
        }

        if ( process_instance_running( PROC_PBASE(MProc) ) )
        {
            MProc->escape_requested = btrue;
        }
    }

    // run the sub-processes
    do_game_proc_run( GProc, EProc->frameDuration );
    do_menu_proc_run( MProc, EProc->frameDuration );

    // a heads up display that can be used to debug values that are used by both the menu and the game
    //do_game_hud();

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_leaving( ego_process_t * eproc )
{
    if ( !process_instance_validate( PROC_PBASE(eproc) )  ) return -1;

    // make sure that the
    if ( !GProc->base.terminated )
    {
        do_game_proc_run( GProc, eproc->frameDuration );
    }

    if ( !MProc->base.terminated )
    {
        do_menu_proc_run( MProc, eproc->frameDuration );
    }

    if ( GProc->base.terminated && MProc->base.terminated )
    {
        process_instance_terminate( PROC_PBASE(eproc) );
    }

    return eproc->base.terminated ? 1 : 0;
}

//--------------------------------------------------------------------------------------------
int do_ego_proc_run( ego_process_t * eproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_instance_validate( PROC_PBASE(eproc) )  ) return -1;
    eproc->base.dtime = frameDuration;

    if ( !eproc->base.paused ) return 0;

    if ( eproc->base.killme )
    {
        eproc->base.state = proc_leaving;
    }

    switch ( eproc->base.state )
    {
        case proc_begin:
            proc_result = do_ego_proc_begin( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            //proc_result = do_ego_proc_entering( eproc );

            eproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_ego_proc_running( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_ego_proc_leaving( eproc );

            if ( 1 == proc_result )
            {
                eproc->base.state  = proc_finish;
                eproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_instance_terminate( PROC_PBASE(eproc) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_menu_proc_begin( menu_process_t * mproc )
{
    // play some music
    sound_play_song( 0, 0, -1 );

    // initialize all these structures
    init_all_titleimages();
    initMenus();        // start the menu menu

    // load all module info at menu initialization
    // this will not change unless a new module is downloaded for a network menu?
    modlist_load_all_info();

    // initialize the process state
    mproc->base.valid = btrue;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_running( menu_process_t * mproc )
{
    int menuResult;

    if ( !process_instance_validate( PROC_PBASE(mproc) ) ) return -1;

    mproc->was_active = mproc->base.valid;

    if ( mproc->base.paused ) return 0;

    // play the menu music
    mnu_draw_background = !process_instance_running( PROC_PBASE(GProc) );
    menuResult          = game_do_menu( mproc );

    switch ( menuResult )
    {
        case MENU_SELECT:
            // go ahead and start the game
            process_instance_pause( PROC_PBASE(mproc) );
            break;

        case MENU_QUIT:
            // the user selected "quit"
            process_instance_kill( PROC_PBASE(mproc) );
            break;
    }

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        GProc->menu_depth   = -1;
        GProc->escape_latch = bfalse;

        // We have exited the menu and restarted the game
        GProc->mod_paused = bfalse;
        process_instance_pause( PROC_PBASE(MProc) );
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_leaving( menu_process_t * mproc )
{
    if ( !process_instance_validate( PROC_PBASE(mproc) ) ) return -1;

    // finish the menu song
    sound_finish_song( 500 );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_menu_proc_run( menu_process_t * mproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_instance_validate( PROC_PBASE(mproc) ) ) return -1;
    mproc->base.dtime = frameDuration;

    if ( mproc->base.paused ) return 0;

    if ( mproc->base.killme )
    {
        mproc->base.state = proc_leaving;
    }

    switch ( mproc->base.state )
    {
        case proc_begin:
            proc_result = do_menu_proc_begin( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            //proc_result = do_menu_proc_entering( mproc );

            mproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_menu_proc_running( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_menu_proc_leaving( mproc );

            if ( 1 == proc_result )
            {
                mproc->base.state  = proc_finish;
                mproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_instance_terminate( PROC_PBASE(mproc) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int do_game_proc_begin( game_process_t * gproc )
{

    gproc->escape_latch = bfalse;

    // initialize math objects
    make_randie();
    make_turntosin();

    // Linking system
    log_info( "Initializing module linking... " );
    if ( link_build( "basicdat" SLASH_STR "link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failure!\n" );

    load_ai_codes( "basicdat" SLASH_STR "aicodes.txt" );
    load_action_names( "basicdat" SLASH_STR "actions.txt" );

    // initialize the IDSZ values for various slots
    init_slot_idsz();

    // do some graphics initialization
    make_lightdirectionlookup();
    make_enviro();
    camera_new( PCamera );

    // try to start a new module
    if ( !game_begin_module( pickedmodule_name, (Uint32)~0 ) )
    {
        // failure - kill the game process
        process_instance_kill( PROC_PBASE(gproc) );
        process_instance_resume( PROC_PBASE(MProc) );
    }

    // Initialize the process
    gproc->base.valid = btrue;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_running( game_process_t * gproc )
{
    if ( !process_instance_validate( PROC_PBASE(gproc) ) ) return -1;

    gproc->was_active  = gproc->base.valid;

    if ( gproc->base.paused ) return 0;

    // This is the control loop
    if ( PNet->on && console_done )
    {
        net_send_message();
    }

    // Do important things
    if ( !gproc->mod_paused || PNet->on )
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
        if ( !PNet->waitingforplayers )
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
    gproc->frame_now = SDL_GetTicks();
    if (gproc->frame_now > gproc->frame_next)
    {
        float  frameskip = (float)TICKS_PER_SEC / (float)cfg.framelimit;
        gproc->frame_next = gproc->frame_now + frameskip; //FPS limit

        camera_move(PCamera, PMesh);
        draw_main();

        msgtimechange++;
    }

    // Check for quitters
    // :TODO: local_noplayers is not set correctly
    //if( local_noplayers  )
    //{
    //   gproc->escape_requested  = btrue;
    //}

    if ( gproc->escape_requested )
    {
        gproc->escape_requested = bfalse;

        if ( !gproc->escape_latch )
        {
            if ( PMod->beat )
            {
                game_begin_menu( MProc, emnu_ShowEndgame );
            }
            else
            {
                game_begin_menu( MProc, emnu_GamePaused );
            }

            gproc->escape_latch = btrue;
            gproc->mod_paused   = btrue;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_leaving( game_process_t * gproc )
{
    if ( !process_instance_validate( PROC_PBASE(gproc) ) ) return -1;

    // get rid of all module data
    game_quit_module();

    // resume the menu
    process_instance_resume( PROC_PBASE(MProc) );

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_run( game_process_t * gproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_instance_validate( PROC_PBASE(gproc) ) ) return -1;
    gproc->base.dtime = frameDuration;

    if ( gproc->base.paused ) return 0;

    if ( gproc->base.killme )
    {
        gproc->base.state = proc_leaving;
    }

    switch ( gproc->base.state )
    {
        case proc_begin:
            proc_result = do_game_proc_begin( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state = proc_entering;
            }
            break;

        case proc_entering:
            //proc_result = do_game_proc_entering( gproc );

            gproc->base.state = proc_running;
            break;

        case proc_running:
            proc_result = do_game_proc_running( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = do_game_proc_leaving( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state  = proc_finish;
                gproc->base.killme = bfalse;
            }
            break;

        case proc_finish:
            process_instance_terminate( PROC_PBASE(gproc) );
            process_instance_resume   ( PROC_PBASE(MProc) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int SDL_main( int argc, char **argv )
{
    // ZZ> This is where the program starts and all the high level stuff happens

    int   max_rate;
    float frameskip;

    // initialize the process
    ego_process_init( EProc );

    // turn on all basic services
    do_ego_proc_begin( EProc );

    // run the processes
    request_clear_screen();
    max_rate  = MAX(cfg.framelimit, UPDATE_SKIP) * 10;
    frameskip = (float)TICKS_PER_SEC / (float)max_rate;
    while ( !EProc->base.killme && !EProc->base.terminated )
    {
        // put a throttle on the ego process
        EProc->frame_now = SDL_GetTicks();
        if (EProc->frame_now < EProc->frame_next) continue;

        // update the timer
        EProc->frame_next = EProc->frame_now + frameskip;

        // clear the screen if needed
        do_clear_screen();

        do_ego_proc_running( EProc );

        // flip the graphics page if need be
        do_flip_pages();

        // let the OS breathe. It may delay as long as 10ms
        //SDL_Delay(1);
    }

    // terminate the game and menu processes
    process_instance_kill( PROC_PBASE(GProc) );
    process_instance_kill( PROC_PBASE(MProc) );
    while ( !EProc->base.terminated )
    {
        do_ego_proc_leaving( EProc );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void memory_cleanUp(void)
{
    //ZF> This function releases all loaded things in memory and cleans up everything properly

    log_info("memory_cleanUp() - Attempting to clean up loaded things in memory... ");

    // quit any existing game
    _quit_game( EProc );

    // synchronoze the config values with the various game subsystems
    setup_synch( &cfg );

    // quit the setup system, making sure that the setup file is written
    setup_upload( &cfg );
    setup_write();
    setup_quit();

    // delete all the graphics allocated by SDL and OpenGL
    delete_all_graphics();

    // make sure that the current control configuration is written
    input_settings_save( "controls.txt" );

    // shut down the ui
    ui_shutdown();

    // shut down the network
    if (PNet->on)
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

    if ( !VALID_CAP(object) ) return 0; // no skins for an invalid object

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
        if ( !ChrList[cnt].on || VALID_CHR(ChrList[cnt].attachedto) || ChrList[cnt].pack_ispacked || cnt == character || ChrList[character].attachedto == cnt ) continue;

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
        if ( hide != NOHIDE && hide == ChrList[character].ai.state )
        {
            ChrList[character].is_hidden = btrue;
        }
    }

    // Get levels every update
    for ( character = 0; character < MAX_CHR; character++ )
    {
        if ( !ChrList[character].on || ChrList[character].pack_ispacked ) continue;

        level = get_mesh_level( PMesh, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].waterwalk ) + RAISE;

        if ( ChrList[character].alive )
        {
            if ( VALID_TILE( PMesh, ChrList[character].onwhichfan ) &&
                    ( 0 != mesh_test_fx( PMesh, ChrList[character].onwhichfan, MPDFX_DAMAGE ) ) &&
                    ( ChrList[character].pos.z <= ChrList[character].floor_level + DAMAGERAISE ) &&
                    ( MAX_CHR == ChrList[character].attachedto ) )
            {
                if ( ( ChrList[character].damagemodifier[damagetile_data.type]&DAMAGESHIFT ) != 3 && !ChrList[character].invictus ) // 3 means they're pretty well immune
                {
                    distance = ABS( PCamera->track_pos.x - ChrList[character].pos.x ) + ABS( PCamera->track_pos.y - ChrList[character].pos.y );
                    if ( distance < damagetile.min_distance )
                    {
                        damagetile.min_distance = distance;
                    }
                    if ( distance < damagetile.min_distance + 256 )
                    {
                        damagetile.sound_time = 0;
                    }
                    if ( ChrList[character].damagetime == 0 )
                    {
                        damage_character( character, 32768, damagetile_data.amount, 1, damagetile_data.type, DAMAGETEAM, ChrList[character].ai.bumplast, DAMFX_NBLOC | DAMFX_ARMO, bfalse );
                        ChrList[character].damagetime = DAMAGETILETIME;
                    }
                    if ( (damagetile_data.parttype != ((Sint16)~0)) && ( update_wld & damagetile_data.partand ) == 0 )
                    {
                        spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].pos.z,
                                            0, MAX_PROFILE, damagetile_data.parttype, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
                    }
                }
                if ( ChrList[character].reaffirmdamagetype == damagetile_data.type )
                {
                    if ( ( update_wld&TILEREAFFIRMAND ) == 0 )
                        reaffirm_attached_particles( character );
                }
            }
        }

        if ( ChrList[character].pos.z < water.surface_level && (0 != mesh_test_fx( PMesh, ChrList[character].onwhichfan, MPDFX_WATER )) )
        {
            if ( !ChrList[character].inwater )
            {
                // Splash
                if ( INVALID_CHR( ChrList[character].attachedto ) )
                {
                    spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, water.surface_level + RAISE,
                                        0, MAX_PROFILE, SPLASH, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
                }

                if ( water_data.is_water )
                {
                    ChrList[character].ai.alert |= ALERTIF_INWATER;
                }
            }
            else
            {
                // Ripple
                if ( ChrList[character].pos.z > water.surface_level - RIPPLETOLERANCE && CapList[ChrList[character].model].ripple )
                {
                    // Ripples
                    ripand = ( ( int )ChrList[character].vel.x != 0 ) | ( ( int )ChrList[character].vel.y != 0 );
                    ripand = RIPPLEAND >> ripand;
                    if ( ( update_wld&ripand ) == 0 && ChrList[character].pos.z < water.surface_level && ChrList[character].alive )
                    {
                        spawn_one_particle( ChrList[character].pos.x, ChrList[character].pos.y, water.surface_level,
                                            0, MAX_PROFILE, RIPPLE, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );
                    }
                }
                if ( water_data.is_water && 0 == ( frame_all & 7 ) )
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
        PrtList[particle].floor_level  = mesh_get_level( PMesh, PrtList[character].pos.x, PrtList[character].pos.y );

        // reject particles that are hidden
        PrtList[particle].is_hidden = bfalse;
        ichr = PrtList[particle].attachedtocharacter;
        if ( VALID_CHR( ichr ) )
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
			for( cnt = 0; cnt < maxparticles; cnt++ )
            {
                if ( INVALID_PRT( cnt ) || INVALID_PIP( PrtList[cnt].pip ) ) continue;

				if ( PrtList[cnt].pos.z < PITDEPTH && PipList[PrtList[cnt].pip].endwater )
                {
                    PrtList[cnt].time  = frame_all + 1;
                    PrtList[cnt].poofme = btrue;
                }
            }

            // Kill or teleport any characters that fell in a pit...
            for( cnt = 0; cnt < MAX_CHR; cnt++ )
			{
				//Is it a valid character?
                if ( INVALID_CHR( cnt ) || ChrList[cnt].invictus || !ChrList[cnt].alive  ) continue;
                if ( ChrList[cnt].attachedto != MAX_CHR || ChrList[cnt].pack_ispacked ) continue;

				//Do we kill it?
                if ( pitskill && ChrList[cnt].pos.z < PITDEPTH )
                {
                    // Got one!
                    kill_character( cnt, MAX_CHR );
                    ChrList[cnt].vel.x = 0;
                    ChrList[cnt].vel.y = 0;

                    //Play sound effect
                    sound_play_chunk( ChrList[cnt].pos, g_wavelist[GSND_PITFALL] );
				}
                    
				//Do we teleport it?
                if ( pitsfall && ChrList[cnt].pos.z < PITDEPTH << 3 )
                {
                    float nrm[2];

                    // Teleport them back to a "safe" spot
                    detach_character_from_mount( cnt, btrue, bfalse );
                    ChrList[cnt].pos_old.x = ChrList[cnt].pos.x;
                    ChrList[cnt].pos_old.y = ChrList[cnt].pos.y;
                    ChrList[cnt].pos.x = pitx;
                    ChrList[cnt].pos.y = pity;
                    ChrList[cnt].pos.z = pitz;
                    if ( __chrhitawall( cnt, nrm ) )
                    {
                        // It did not work...
                        ChrList[cnt].pos = ChrList[cnt].pos_safe;
                        ChrList[cnt].vel = ChrList[cnt].vel_old;

                        // Kill it instead
                        kill_character( cnt, MAX_CHR );
                        ChrList[cnt].vel.x = 0;
                        ChrList[cnt].vel.y = 0;
                    }
                    else
                    {
						//It worked!
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
                            sound_play_chunk( PCamera->track_pos, g_wavelist[GSND_PITFALL] );
                        }
                        else
                        {
                            sound_play_chunk( ChrList[cnt].pos, g_wavelist[GSND_PITFALL] );
                        }

                        //Do some damage (same as damage tile)
                        damage_character( cnt, 32768, damagetile_data.amount, 1, damagetile_data.type, DAMAGETEAM, ChrList[cnt].ai.bumplast, DAMFX_NBLOC | DAMFX_ARMO, btrue );
                    }
                }
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

    if ( weather.time > 0 )
    {
        weather.time--;
        if ( weather.time == 0 )
        {
            weather.time = weather_data.timer_reset;

            // Find a valid player
            foundone = bfalse;
            cnt = 0;

            while ( cnt < MAXPLAYER )
            {
                weather.iplayer = ( weather.iplayer + 1 ) & ( MAXPLAYER - 1 );
                if ( PlaList[weather.iplayer].valid )
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
                cnt = PlaList[weather.iplayer].index;
                if ( ChrList[cnt].on && !ChrList[cnt].pack_ispacked )
                {
                    // Yes, so spawn over that character
                    x = ChrList[cnt].pos.x;
                    y = ChrList[cnt].pos.y;
                    z = ChrList[cnt].pos.z;
                    particle = spawn_one_particle( x, y, z, 0, MAX_PROFILE, WEATHER4, MAX_CHR, GRIP_LAST, NULLTEAM, MAX_CHR, 0, MAX_CHR );

                    if (particle != TOTAL_MAX_PRT)
                    {
                        if (__prthitawall( particle ) ) free_one_particle( particle );
                        else if ( weather_data.over_water )
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

    PCamera->swing = ( PCamera->swing + PCamera->swingrate ) & 16383;
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
            if ( ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
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

                    turnsin = PCamera->turn_z >> 2;
                    turnsin = turnsin & 16383;
                    if ( PCamera->turn_mode == CAMTURN_GOOD &&
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
            if ( ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
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

                turnsin = PCamera->turn_z >> 2;
                turnsin = turnsin & 16383;
                if ( PCamera->turn_mode == CAMTURN_GOOD &&
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
            if ( ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
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

                turnsin = PCamera->turn_z >> 2;
                turnsin = turnsin & 16383;
                if ( PCamera->turn_mode == CAMTURN_GOOD &&
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

            turnsin = PCamera->turn_z >> 2;
            turnsin = turnsin & 16383;
            if ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 )  inputx = 0;

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

    chop.count = 0;
    chop.carat = 0;
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

    // Show map cheat
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) && mapvalid )
    {
        mapon = !mapon;
        youarehereon = btrue;
        stat_check_delay = 150;
    }

    // !!!BAD!!!  XP CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_x ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && VALID_CHR(PlaList[0].index) )  { ChrList[PlaList[0].index].experience += 25; stat_check_delay = 1; }
        if ( SDLKEYDOWN( SDLK_2 ) && VALID_CHR(PlaList[1].index) )  { ChrList[PlaList[1].index].experience += 25; stat_check_delay = 1; }
        if ( SDLKEYDOWN( SDLK_3 ) && VALID_CHR(PlaList[2].index) )  { ChrList[PlaList[2].index].experience += 25; stat_check_delay = 1; }
        if ( SDLKEYDOWN( SDLK_4 ) && VALID_CHR(PlaList[3].index) )  { ChrList[PlaList[3].index].experience += 25; stat_check_delay = 1; }

    }

    // !!!BAD!!!  LIFE CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_z ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) && VALID_CHR(PlaList[0].index) )  { ChrList[PlaList[0].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[0].index].life, ChrList[PlaList[0].index].lifemax); stat_check_delay = 12; }
        if ( SDLKEYDOWN( SDLK_2 ) && VALID_CHR(PlaList[1].index) )  { ChrList[PlaList[1].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[1].index].life, ChrList[PlaList[1].index].lifemax); stat_check_delay = 12; }
        if ( SDLKEYDOWN( SDLK_3 ) && VALID_CHR(PlaList[2].index) )  { ChrList[PlaList[2].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[2].index].life, ChrList[PlaList[2].index].lifemax); stat_check_delay = 12; }
        if ( SDLKEYDOWN( SDLK_4 ) && VALID_CHR(PlaList[3].index) )  { ChrList[PlaList[3].index].life += 32; ChrList[PlaList[0].index].life = MIN(ChrList[PlaList[3].index].life, ChrList[PlaList[3].index].lifemax); stat_check_delay = 12; }
    }

    // Display armor stats?
    if ( SDLKEYDOWN( SDLK_LSHIFT ) )
    {
        if ( SDLKEYDOWN( SDLK_1 ) )  { show_armor( 0 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_2 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_3 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_4 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_5 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_6 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_7 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if ( SDLKEYDOWN( SDLK_8 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
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

    // Display character special powers?
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

}

//--------------------------------------------------------------------------------------------
void show_stat( Uint16 statindex )
{
    // ZZ> This function shows the more specific stats for a character
    int character, level;
    char text[64];
    char gender[8];
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
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( Uint16 statindex )
{
    // ZF> This function shows detailed armor information for the character
    STRING text, tmps;

    if ( statindex < numstat )
    {
        Uint16 ichr = statlist[statindex];

        if ( VALID_CHR(ichr) )
        {
            Uint16 icap;
            Uint8  skinlevel;

            icap      = ChrList[ichr].model;
            skinlevel = ChrList[ichr].skin;

            // Armor Name
            sprintf( text, "=%s=", CapList[icap].skinname[skinlevel] );
            debug_message( text );

            // Armor Stats
            sprintf( text, "~DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255 - CapList[icap].defense[skinlevel],
                     CapList[icap].damagemodifier[0][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[1][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[2][skinlevel]&DAMAGESHIFT );
            debug_message( text );

            sprintf( text, "~HOLY:~%i~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP:~%i",
                     CapList[icap].damagemodifier[3][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[4][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[5][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[6][skinlevel]&DAMAGESHIFT,
                     CapList[icap].damagemodifier[7][skinlevel]&DAMAGESHIFT );
            debug_message( text );

            strcpy( text, "~Type: " );
            if ( CapList[icap].skindressy & (1 << skinlevel) ) strcat( text, CapList[icap].skindressy ? "Light Armor" : "Heavy Armor" );
            debug_message( text );

            // Base speed
            sprintf( text, "~Speed:~%3.0f~", CapList[icap].maxaccel[skinlevel]*80 );

            // jumps
            strcat( text, "Jump Skill:~" );
            switch ( CapList[icap].jumpnumber )
            {
                case 0:  sprintf( tmps, "None    (%i)", CapList[icap].jumpnumber ); break;
                case 1:  sprintf( tmps, "Novice  (%i)", CapList[icap].jumpnumber ); break;
                case 2:  sprintf( tmps, "Skilled (%i)", CapList[icap].jumpnumber ); break;
                case 3:  sprintf( tmps, "Adept   (%i)", CapList[icap].jumpnumber ); break;
                default: sprintf( tmps, "Master  (%i)", CapList[icap].jumpnumber ); break;
            };
            strcat( text, tmps );

            debug_message( text );
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
        if ( ChrList[character].jumpnumberreset == 0 )  sprintf( text, "None    (%i)", ChrList[character].jumpnumberreset );
        if ( ChrList[character].jumpnumberreset == 1 )  sprintf( text, "Novice  (%i)", ChrList[character].jumpnumberreset );
        if ( ChrList[character].jumpnumberreset == 2 )  sprintf( text, "Skilled (%i)", ChrList[character].jumpnumberreset );
        if ( ChrList[character].jumpnumberreset == 3 )  sprintf( text, "Adept   (%i)", ChrList[character].jumpnumberreset );
        if ( ChrList[character].jumpnumberreset > 3 )   sprintf( text, "Master  (%i)", ChrList[character].jumpnumberreset );

        sprintf( tmps, "Jump Skill: %s", text );
        sprintf( text, " Speed:~%3.0f~~%s", ChrList[character].maxaccel*80, tmps );
        debug_message( text );
    }
}

//--------------------------------------------------------------------------------------------
void show_magic_status( Uint16 statindex )
{
    // ZF> Displays special enchantment effects for the character
    char text[64], tmpa[64], tmpb[64];
    short character;
    int i = 0;
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
        if ( pchr->pack_ispacked ) continue;

        // reject characters that are hidden
        if ( pchr->is_hidden ) continue;

        if ( INVALID_BLOCK != pchr->onwhichblock )
        {
            // Insert before any other characters on the block
            pchr->fanblock_next = bumplist[pchr->onwhichblock].chr;
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
            pprt->fanblock_next = bumplist[pprt->onwhichblock].prt;
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

    if (NULL == chr_co_list)
    {
        chr_co_list = hash_list_create(-1);
        assert(NULL != chr_co_list);
    }

    // renew the collision list. Since we are filling this list with pre-allocated hash_node_t's,
    // there is no need to delete any of the existing chr_co_list->sublist elements
    for (cnt = 0; cnt < 256; cnt++)
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
        if ( pchr_a->pack_ispacked ) continue;

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
                            tnc++, ichr_b = ChrList[ichr_b].fanblock_next)
                    {
                        if ( detect_chr_chr_collision(ichr_a, ichr_b) )
                        {
                            add_chr_chr_collision( ichr_a, ichr_b, cdata, cdata_count, hnlst, hn_count );
                        }
                    }

                    if ( pchr_a->alive )
                    {
                        // detect all the character-particle interactions
                        for ( tnc = 0, iprt_b = bumplist[fanblock].prt;
                                tnc < prtinblock;
                                tnc++, iprt_b = PrtList[iprt_b].fanblock_next )
                        {
                            if ( detect_chr_prt_collision(ichr_a, iprt_b) )
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

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
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

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    // if you are mounted, only your mount is affected by platforms
    if ( VALID_CHR(pchr_a->attachedto) || VALID_CHR(pchr_b->attachedto) ) return bfalse;

    // only check possible object-platform interactions
    platform_a = pcap_b->canuseplatforms && pchr_a->platform;
    platform_b = pcap_a->canuseplatforms && pchr_b->platform;
    if ( !platform_a && !platform_b ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // If we can mount this platform, skip it
    mount_a = can_mount(ichr_b, ichr_a);
    if ( mount_a && pchr_a->phys.level < zb + pchr_b->bumpheight + PLATTOLERANCE )
        return bfalse;

    //// If we can mount this platform, skip it
    mount_b = can_mount(ichr_a, ichr_b);
    if ( mount_b && pchr_b->phys.level < za + pchr_a->bumpheight + PLATTOLERANCE )
        return bfalse;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;
    depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

    if ( depth_z > PLATTOLERANCE || depth_z < -PLATTOLERANCE ) return bfalse;

    // estimate the radius of interaction based on the z overlap
    lerp_z  = depth_z / PLATTOLERANCE;
    lerp_z  = CLIP( lerp_z, 0, 1 );

    radius    = MIN(pchr_a->bumpsize,     pchr_b->bumpsize  ); /* * (1.0f - lerp_z) + (pchr_a->bumpsize    + pchr_b->bumpsize   ) * lerp_z; */
    radius_xy = MIN(pchr_a->bumpsizebig, pchr_b->bumpsizebig); /* * (1.0f - lerp_z) + (pchr_a->bumpsizebig + pchr_b->bumpsizebig) * lerp_z; */

    // determine how the characters can be attached
    chara_on_top = btrue;
    depth_z = 2 * PLATTOLERANCE;
    if ( platform_a && platform_b )
    {
        float depth_a, depth_b;

        depth_a = zb + pchr_b->bumpheight - za;
        depth_b = za + pchr_a->bumpheight - zb;

        depth_z = MIN( zb + pchr_b->bumpheight, za + pchr_a->bumpheight ) - MAX(za, zb);

        chara_on_top = ABS(depth_z - depth_a) < ABS(depth_z - depth_b);

        // the collision is determined by the platform size
        if ( chara_on_top )
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
    else if ( platform_a )
    {
        chara_on_top = bfalse;
        depth_z = za + pchr_a->bumpheight - zb;

        // the collision is determined by the platform size
        collide_x  = (dx <= pchr_a->bumpsize);
        collide_y  = (dy <= pchr_a->bumpsize);
        collide_xy = (dist <= pchr_a->bumpsizebig);
    }
    else if ( platform_b )
    {
        chara_on_top = btrue;
        depth_z = zb + pchr_b->bumpheight - za;

        // the collision is determined by the platform size
        collide_x  = (dx <= pchr_b->bumpsize);
        collide_y  = (dy <= pchr_b->bumpsize);
        collide_xy = (dist <= pchr_b->bumpsizebig);
    }

    if ( collide_x && collide_y && collide_xy && depth_z > -PLATTOLERANCE && depth_z < PLATTOLERANCE )
    {
        // check for the best possible attachment
        if ( chara_on_top )
        {
            if ( zb + pchr_b->bumpheight > pchr_a->phys.level )
            {
                pchr_a->phys.level = zb + pchr_b->bumpheight;
                pchr_a->onwhichplatform = ichr_b;
            }
        }
        else
        {
            if ( za + pchr_a->bumpheight > pchr_b->phys.level )
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

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that B is valid
    if ( INVALID_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
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

    if ( !mount_a && !mount_b ) return bfalse;

    mounted = bfalse;
    if ( !mounted && mount_b && (pchr_a->vel.z - pchr_b->vel.z) < 0 )
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

        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = (dx <= pchr_a->bumpsize * 2);
            collide_y  = (dy <= pchr_a->bumpsize * 2);
            collide_xy = (dist <= pchr_a->bumpsizebig * 2);

            if ( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY );
                mounted = VALID_CHR( pchr_a->attachedto );
            }
        }
    }

    if ( !mounted && mount_a && (pchr_b->vel.z - pchr_a->vel.z) < 0 )
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

        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = (dx <= pchr_b->bumpsize * 2);
            collide_y  = (dy <= pchr_b->bumpsize * 2);
            collide_xy = (dist <= pchr_b->bumpsizebig * 2);

            if ( collide_x && collide_y && collide_xy )
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
    float xa, ya, za, xb, yb, zb;
    float was_xa, was_ya, was_za, was_xb, was_yb, was_zb;
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

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // make sure that it is on
    if ( !VALID_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if ( INVALID_CAP( pchr_b->model ) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // don't do anything if there is no interaction strength
    interaction_strength = 1.0f;
    if ( 0 == pchr_a->bumpsize || 0 == pchr_b->bumpsize ) return bfalse;
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
    if ( ichr_a == pchr_b->onwhichplatform )
    {
        // we know that ichr_a is a platform and ichr_b is on it
        Sint16 rot_a, rot_b;

        lerp_z = (pchr_b->pos.z - pchr_b->phys.level) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        rot_b = pchr_b->turn_z - pchr_b->turn_old_z;
        rot_a = pchr_a->turn_z - pchr_a->turn_old_z;

        if ( lerp_z < 0 )
        {
            pchr_b->phys.apos_0.z += (pchr_b->phys.level - pchr_b->pos.z) * 0.25f * (-lerp_z);
            pchr_b->jumpnumber = pchr_b->jumpnumberreset;
        }
        else if ( lerp_z < 1 )
        {
            pchr_b->phys.apos_0.z += (pchr_b->phys.level - pchr_b->pos.z) * 0.125f * lerp_z;
        }

        lerp_z = 1.0f - CLIP( lerp_z, 0, 1 );

        if ( lerp_z > 0 )
        {
            pchr_a->holdingweight += pchr_b->weight * lerp_z;

            pchr_b->phys.avel.z += ( pchr_a->vel.z  - pchr_b->vel.z      ) * lerp_z * 0.25f;
            pchr_b->turn_z     += ( rot_a - rot_b ) * platstick * lerp_z;
        };

        if ( lerp_z < 0.25f )
        {
            pchr_b->jumpready = btrue;
        };

        // this is handled
        return btrue;
    }

    // platform interaction
    if ( ichr_b == pchr_a->onwhichplatform )
    {
        // we know that ichr_b is a platform and ichr_a is on it
        Sint16 rot_a, rot_b;

        lerp_z = (pchr_a->pos.z - pchr_a->phys.level) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        rot_b = pchr_b->turn_z - pchr_b->turn_old_z;
        rot_a = pchr_a->turn_z - pchr_a->turn_old_z;

        if ( lerp_z < 0 )
        {
            pchr_a->phys.apos_0.z += (pchr_a->phys.level - pchr_a->pos.z) * 0.25f * (-lerp_z);
            pchr_a->jumpnumber = pchr_a->jumpnumberreset;
        }
        else if ( lerp_z < 1 )
        {
            pchr_a->phys.apos_0.z += (pchr_a->phys.level - pchr_a->pos.z) * 0.125f * lerp_z;
        }

        lerp_z = 1.0f - CLIP( lerp_z, 0, 1 );

        if ( lerp_z > 0 )
        {
            pchr_b->holdingweight += pchr_a->weight * lerp_z;

            pchr_a->phys.avel.z += ( pchr_b->vel.z  - pchr_a->vel.z      ) * lerp_z * 0.25f;
            pchr_a->turn_z     += ( rot_b - rot_a ) * platstick * lerp_z;
        }

        if ( lerp_z < 0.25f )
        {
            pchr_a->jumpready = btrue;
        }

        // this is handled
        return btrue;
    }

    // items can interact with platforms but not with other characters/objects
    if ( pchr_a->isitem || pchr_b->isitem ) return bfalse;

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
    collide_x  = (dx < radius);
    collide_y  = (dy < radius);
    collide_xy = (dist < radius_xy);
    collide_z  = (depth_z > 0);

    // estimate the collisions last frame
    was_collide_x  = (was_dx < radius);
    was_collide_y  = (was_dy < radius);
    was_collide_xy = (was_dist < radius_xy);
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

        if ( pcap_a->canuseplatforms && pchr_b->platform ) exponent += 2;
        if ( pcap_b->canuseplatforms && pchr_a->platform ) exponent += 2;

        nrm.x = nrm.y = nrm.z = 0.0f;

        depth_x  = MIN(xa + pchr_a->bumpsize, xb + pchr_b->bumpsize) - MAX(xa - pchr_a->bumpsize, xb - pchr_b->bumpsize);
        if ( depth_x <= 0.0f )
        {
            depth_x = 0.0f;
        }
        else
        {
            float sgn = xb - xa;
            sgn = sgn > 0 ? -1 : 1;

            nrm.x += sgn / POW(depth_x / PLATTOLERANCE, exponent);
        }

        depth_y  = MIN(ya + pchr_a->bumpsize, yb + pchr_b->bumpsize) - MAX(ya - pchr_a->bumpsize, yb - pchr_b->bumpsize);
        if ( depth_y <= 0.0f )
        {
            depth_y = 0.0f;
        }
        else
        {
            float sgn = yb - ya;
            sgn = sgn > 0 ? -1 : 1;

            nrm.y += sgn / POW(depth_y / PLATTOLERANCE, exponent);
        }

        depth_xy = MIN(xa + ya + pchr_a->bumpsizebig, xb + yb + pchr_b->bumpsizebig) - MAX(xa + ya - pchr_a->bumpsizebig, xb + yb - pchr_b->bumpsizebig);
        if ( depth_xy <= 0.0f )
        {
            depth_xy = 0.0f;
        }
        else
        {
            float sgn = (xb + yb) - (xa + ya);
            sgn = sgn > 0 ? -1 : 1;

            nrm.x += sgn / POW(depth_xy / PLATTOLERANCE, exponent);
            nrm.y += sgn / POW(depth_xy / PLATTOLERANCE, exponent);
        }

        depth_yx = MIN(-xa + ya + pchr_a->bumpsizebig, -xb + yb + pchr_b->bumpsizebig) - MAX(-xa + ya - pchr_a->bumpsizebig, -xb + yb - pchr_b->bumpsizebig);
        if ( depth_yx <= 0.0f )
        {
            depth_yx = 0.0f;
        }
        else
        {
            float sgn = (-xb + yb) - (-xa + ya);
            sgn = sgn > 0 ? -1 : 1;
            nrm.x -= sgn / POW(depth_yx / PLATTOLERANCE, exponent);
            nrm.y += sgn / POW(depth_yx / PLATTOLERANCE, exponent);
        }

        depth_z  = MIN(za + pchr_a->bumpheight, zb + pchr_b->bumpheight) - MAX( za, zb );
        if ( depth_z <= 0.0f )
        {
            depth_z = 0.0f;
        }
        else
        {
            float sgn = (zb + pchr_b->bumpheight / 2) - (za + pchr_a->bumpheight / 2);
            sgn = sgn > 0 ? -1 : 1;

            nrm.z += sgn / POW(exponent * depth_z / PLATTOLERANCE, exponent);
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

                if ( (wta < 0 && wtb < 0) || (wta == wtb) )
                {
                    float factor = 0.5f * (1.0f - cr);

                    imp_a.x = factor * (vperp_b.x - vperp_a.x);
                    imp_a.y = factor * (vperp_b.y - vperp_a.y);
                    imp_a.z = factor * (vperp_b.z - vperp_a.z);

                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }
                else if ( (wta < 0) || (wtb == 0) )
                {
                    float factor = (1.0f - cr);

                    imp_b.x = factor * (vperp_a.x - vperp_b.x);
                    imp_b.y = factor * (vperp_a.y - vperp_b.y);
                    imp_b.z = factor * (vperp_a.z - vperp_b.z);
                }
                else if ( (wtb < 0) || (wta == 0) )
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
                LOG_NAN(pchr_a->phys.avel.z);

                pchr_b->phys.avel.x += imp_b.x;
                pchr_b->phys.avel.y += imp_b.y;
                pchr_b->phys.avel.z += imp_b.z;
                LOG_NAN(pchr_b->phys.avel.z);

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
            if ( ABS(imp_a.x) + ABS(imp_a.y) + ABS(imp_a.z) > 0.0f &&
                    ABS(vpara_a.x) + ABS(vpara_a.y) + ABS(vpara_a.z) > 0.0f &&
                    pchr_a->phys.dismount_timer <= 0)
            {
                float imp, vel, factor;

                imp = 0.5f * SQRT( imp_a.x * imp_a.x + imp_a.y * imp_a.y + imp_a.z * imp_a.z );
                vel = SQRT( vpara_a.x * vpara_a.x + vpara_a.y * vpara_a.y + vpara_a.z * vpara_a.z );

                factor = imp / vel;
                factor = CLIP(factor, 0.0f, 1.0f);

                pchr_a->phys.avel.x -= factor * vpara_a.x;
                pchr_a->phys.avel.y -= factor * vpara_a.y;
                pchr_a->phys.avel.z -= factor * vpara_a.z;
                LOG_NAN(pchr_a->phys.avel.z);
            }

            if ( ABS(imp_b.x) + ABS(imp_b.y) + ABS(imp_b.z) > 0.0f &&
                    ABS(vpara_b.x) + ABS(vpara_b.y) + ABS(vpara_b.z) > 0.0f &&
                    pchr_b->phys.dismount_timer <= 0)
            {
                float imp, vel, factor;

                imp = 0.5f * SQRT( imp_b.x * imp_b.x + imp_b.y * imp_b.y + imp_b.z * imp_b.z );
                vel = SQRT( vpara_b.x * vpara_b.x + vpara_b.y * vpara_b.y + vpara_b.z * vpara_b.z );

                factor = imp / vel;
                factor = CLIP(factor, 0.0f, 1.0f);

                pchr_b->phys.avel.x -= factor * vpara_b.x;
                pchr_b->phys.avel.y -= factor * vpara_b.y;
                pchr_b->phys.avel.z -= factor * vpara_b.z;
                LOG_NAN(pchr_b->phys.avel.z);
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

    float xa, ya, za, xb, yb, zb;
    float was_xa, was_ya, was_za, was_xb, was_yb, was_zb;
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

    if ( INVALID_CAP( pchr_a->model ) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    if ( !VALID_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList + iprt_b;

    ipip_b = pprt_b->pip;
    if ( INVALID_PIP( ipip_b ) ) return bfalse;
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

    if ( 0 == pchr_a->bumpsize ) interaction_strength = 0;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;

    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dist = dx + dy;

    collide_x  = dx < pchr_a->bumpsize    || dx < pprt_b->bumpsize;
    collide_y  = dy < pchr_a->bumpsize    || dy < pprt_b->bumpsize;
    collide_xy = dist < pchr_a->bumpsizebig || dist < pprt_b->bumpsize * 1.41f;

    depth_z = MIN( za + pchr_a->bumpheight, zb + pprt_b->bumpheight ) - MAX( za, zb - pprt_b->bumpheight );
    collide_z = depth_z > 0;

    if ( !collide_x || !collide_y || !collide_xy || !collide_z ) return bfalse;

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
            pprt_b->attachedtocharacter != MAX_CHR ||
            ( pprt_b->chr == ichr_a && !ppip_b->friendlyfire ) ||
            ( ChrList[pchr_a->missilehandler].mana < ( pchr_a->missilecost << 8 ) && !ChrList[pchr_a->missilehandler].canchannel ) )
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
                        if ( 0 == pchr_a->weight )
                        {
                            pchr_a->phys.avel.x  += pprt_b->vel.x - pchr_a->vel.x;
                            pchr_a->phys.avel.y  += pprt_b->vel.y - pchr_a->vel.y;
                            pchr_a->phys.avel.z  += pprt_b->vel.z - pchr_a->vel.z;
                            LOG_NAN(pchr_a->phys.avel.z);
                        }
                        else
                        {
                            float factor = MIN( 1.0f, 110 / pchr_a->weight );   // 110 is the "iconic" weight of the adventurer
                            factor = MIN( 1.0f, factor * pchr_a->bumpdampen );

                            pchr_a->phys.avel.x  += pprt_b->vel.x * factor;
                            pchr_a->phys.avel.y  += pprt_b->vel.y * factor;
                            pchr_a->phys.avel.z  += pprt_b->vel.z * factor;
                            LOG_NAN(pchr_a->phys.avel.z);
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

                    // Do confuse effects
                    if ( 0 == ( Md2FrameList[pchr_a->inst.frame_nxt].framefx&MADFX_INVICTUS ) || ppip_b->damfx&DAMFX_NBLOC )
                    {
                        if ( ppip_b->grogtime >= pchr_a->grogtime && pcap_a->canbegrogged )
                        {
                            pchr_a->grogtime = MAX(0, pchr_a->grogtime + ppip_b->grogtime);
                            pchr_a->ai.alert |= ALERTIF_GROGGED;
                        }
                        if ( ppip_b->dazetime >= pchr_a->dazetime && pcap_a->canbedazed )
                        {
                            pchr_a->dazetime = MAX(0, pchr_a->dazetime + ppip_b->dazetime);
                            pchr_a->ai.alert |= ALERTIF_DAZED;
                        }
                    }

                    // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                    //+2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                    if ( ppip_b->intdamagebonus )
                    {
                        float percent;
                        percent = ( (FP8_TO_INT(ChrList[pprt_b->chr].intelligence)) - 14 ) * 2;
                        percent /= 100;
                        pprt_b->damagebase *= 1.00f + percent;
                    }

                    if ( ppip_b->wisdamagebonus )
                    {
                        float percent;
                        percent = ( FP8_TO_INT(ChrList[pprt_b->chr].wisdom) - 14 ) * 2;
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
                    if ( pchr_a->cangrabmoney && pchr_a->alive && 0 == pchr_a->damagetime && MAXMONEY != pchr_a->money )
                    {
                        if ( pchr_a->ismount )
                        {
                            // Let mounts collect money for their riders
                            if ( pchr_a->holdingwhich[SLOT_LEFT] != MAX_CHR )
                            {
                                ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money += ppip_b->bumpmoney;
                                if ( ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money > MAXMONEY ) ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money = MAXMONEY;
                                if ( ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money < 0 ) ChrList[pchr_a->holdingwhich[SLOT_LEFT]].money = 0;

                                pprt_b->time = frame_all + 1;
                                pprt_b->poofme = btrue;
                            }
                        }
                        else
                        {
                            // Normal money collection
                            pchr_a->money += ppip_b->bumpmoney;
                            if ( pchr_a->money > MAXMONEY ) pchr_a->money = MAXMONEY;
                            if ( pchr_a->money < 0 ) pchr_a->money = 0;

                            pprt_b->time = frame_all + 1;
                            pprt_b->poofme = btrue;
                        }
                    }
                }
                else
                {
                    pprt_b->time  = frame_all + 1;
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
        bool_t mana_paid = cost_mana( pchr_a->missilehandler, pchr_a->missilecost << 8, pprt_b->chr );

        if ( mana_paid )
        {
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
            pprt_b->team = pchr_a->team;
            pprt_b->chr = ichr_a;
            ppip_b->homing = bfalse;

            // Change the direction of the particle
            if ( ppip_b->rotatetoface )
            {
                // Turn to face new direction
                facing = ATAN2( pprt_b->vel.y, pprt_b->vel.x ) * 0xFFFF / ( TWO_PI );
                facing += 32768;
                pprt_b->facing = facing;
            }
        }
        else
        {
            // what happes if the misslehandler can't pay the mana?
            free_one_particle( iprt_b );
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
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);
            if (TOTAL_MAX_PRT != d->prtb) continue;

            do_platforms( d->chra, d->chrb );
        }
    }

    // Do mounts
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);
            if (TOTAL_MAX_PRT != d->prtb) continue;

            do_mounts( d->chra, d->chrb );
        }
    }

    // do all interactions
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = (co_data_t *)(n->data);

            if ( TOTAL_MAX_PRT == d->prtb )
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

        if ( !ChrList[cnt].on ) continue;
        pchr = ChrList + cnt;

        bump_str = 1.0f;
        if ( VALID_CHR( pchr->attachedto ) )
        {
            bump_str = 0;
        }
        else if ( pchr->phys.dismount_timer > 0 )
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
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ChrList[cnt].on ) continue;

        if ( ChrList[cnt].reloadtime > 0 )
        {
            ChrList[cnt].reloadtime--;
        }
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
                ChrList[cnt].life = MAX(1, MIN(ChrList[cnt].life, ChrList[cnt].lifemax));
            }

            //countdown cofuse effects
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

            if ( 0 == EncList[cnt].time )
            {
                remove_enchant( cnt );
            }
            else
            {
                //Do enchant timer
                if ( EncList[cnt].time > 0 )
                {
                    EncList[cnt].time--;
                }

                // To make life easier
                owner  = EncList[cnt].owner;
                target = EncList[cnt].target;
                eve    = EncList[cnt].eve;

                // Do drains
                if ( ChrList[owner].alive )
                {
                    bool_t mana_paid;

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
                    mana_paid = cost_mana(owner, -EncList[cnt].ownermana, target);
                    if ( EveList[eve].endifcantpay && !mana_paid )
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
                        bool_t mana_paid;

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
                        mana_paid = cost_mana( target, -EncList[cnt].targetmana, owner );
                        if ( EveList[eve].endifcantpay && !mana_paid )
                        {
                            remove_enchant( cnt );
                        }
                    }
                    else if( !EveList[eve].stayifdead )
                    {
                        remove_enchant( cnt );
                    }
                }
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
            twist = PMesh->mmem.tile_list[ChrList[cnt].onwhichfan].twist;
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
    if ( PMod->importvalid )
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

    fscanf( fileread, "%f%f%f", &(pinfo->pos.x), &(pinfo->pos.y), &(pinfo->pos.z) );
    pinfo->pos.x *= TILE_SIZE;  pinfo->pos.y *= TILE_SIZE;  pinfo->pos.z *= TILE_SIZE;

    pinfo->facing = NORTH;
    pinfo->attach = ATTACH_NONE;
    cTmp = fget_first_letter( fileread );
    if ( 'S' == toupper(cTmp) )       pinfo->facing = SOUTH;
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
        inventory_add_item( ichr, pinfo->parent );

        ChrList[ichr].ai.alert |= ALERTIF_GRABBED;  // Make spellbooks change
        ChrList[ichr].attachedto = pinfo->parent;  // Make grab work
        let_character_think( ichr );  // Empty the grabbed messages

        ChrList[ichr].attachedto = MAX_CHR;  // Fix grab

    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        grip_offset_t grip_off = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;
        attach_character_to_mount( ichr, pinfo->parent, grip_off );

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
//            if ( info.team < numplayer || !PMod->rtscontrol || info.team >= MAXPLAYER )
            {
                new_object = spawn_one_character( info.pos, info.slot, info.team, info.skin, info.facing, info.pname, MAX_CHR );

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

                        if ( 0 == PMod->importamount && numpla < PMod->playeramount )
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
                        else if ( numpla < numimport && numpla < PMod->importamount && numpla < PMod->playeramount )
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
void game_reset_module_data()
{
    // reset all
    log_info( "Resetting module data\n" );

    // unload a lot of data
    reset_teams();
    release_all_profiles();
    free_all_objects();
    reset_messages();
    prime_names();
    reset_players();
    reset_end_text();
    reset_renderlist();
}

//--------------------------------------------------------------------------------------------
bool_t game_load_module_data( const char *smallname )
{
    // ZZ> This function loads a module
    STRING modname;

    log_info( "Loading module \"%s\"\n", smallname );

    // Load all the global icons
    if ( !load_all_global_icons() )
    {
        log_warning( "Could not load all global icons!\n" );
    }

    load_one_icon( "basicdat" SLASH_STR "nullicon" );
    load_ai_script( "basicdat" SLASH_STR "script.txt" );

    // generate the module directory
    snprintf( modname, sizeof(modname), "modules" SLASH_STR "%s" SLASH_STR, smallname );

    // load a bunch of assets that are used in the module
    load_global_waves( modname );
    reset_particles( modname );
    read_wawalite( modname );
    load_basic_textures( modname );
    load_blip_bitmap();
    load_bars( "basicdat" SLASH_STR "bars" );

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

    setup_particles();
    setup_passage( modname );
    setup_characters( modname );
    setup_alliances( modname );

    // Load fonts and bars after other images, as not to hog videomem
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
            attach_particle_to_character( particle, character, PrtList[particle].vrt_off );
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

    // stop the module
    module_stop( PMod );

    // get rid of the game/module data
    game_release_module_data();

    // turn off networking
    close_session();

    // reset the "ui" mouse state
    cursor_reset();

    // finish whatever in-game song is playing
    sound_finish_sound();
}

//-----------------------------------------------------------------
bool_t game_begin_module( const char * modname, Uint32 seed )
{
    // BB> all of the initialization code before the module actually starts

    if ( ~0 == seed ) seed = time(NULL);

    // make sure the old game has been quit
    game_quit_module();

    // re-initialize all game/module data
    game_reset_module_data();

    // load all the in-game module data
    srand( seed );
    if ( !game_load_module_data( modname ) )
    {
        module_stop( PMod );
        return bfalse;
    };

    timeron = bfalse;
    reset_timers();

    // make sure the per-module configuration settings are correct
    setup_synch( &cfg );

    // initialize the game objects
    make_onwhichfan();
    cursor_reset();
    module_reset( PMod, seed );
    camera_reset( PCamera, PMesh );
    make_character_matrices( update_wld != 0 );
    attach_particles();

    // initialize the network
    net_initialize();
    net_sayHello();

    // start the module
    module_start( PMod );

    return btrue;
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

//--------------------------------------------------------------------------------------------
void game_release_module_data()
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
    release_all_profiles();
    release_all_ai_scripts();

    mesh_delete( PMesh );
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
            attach_particle_to_character( cnt, PrtList[cnt].attachedtocharacter, PrtList[cnt].vrt_off );

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
        can_think = !ChrList[character].pack_ispacked || CapList[ChrList[character].model].isequipment;

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


//--------------------------------------------------------------------------------------------
bool_t game_begin_menu( menu_process_t * mproc, which_menu_t which )
{
    if ( NULL == mproc ) return bfalse;

    if ( !process_instance_running( PROC_PBASE(mproc) ) )
    {
        GProc->menu_depth = mnu_get_menu_depth();
    }

    if ( mnu_begin_menu( which ) )
    {
        process_instance_start( PROC_PBASE(mproc) );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void game_end_menu( menu_process_t * mproc )
{
    mnu_end_menu();

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        process_instance_resume( PROC_PBASE(MProc) );
        GProc->menu_depth = -1;
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
void free_all_objects( void )
{
    // BB > free every instance of the three object types used in the game.

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
    hashval = (ichr_a * 0x0111 + 0x006E) + (ichr_b * 0x0111 + 0x006E);
    hashval &= 0xFF;

    found = bfalse;
    count = chr_co_list->subcount[hashval];
    if ( count > 0)
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for (i = 0; i < count; i++)
        {
            d = (co_data_t *)(n->data);
            if (d->chra == ichr_a && d->chrb == ichr_b)
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if (!found)
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
    if ( count > 0)
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for (i = 0; i < count; i++)
        {
            d = (co_data_t *)(n->data);
            if (d->chra == ichr_a && d->prtb == iprt_b)
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if (!found)
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

    float xa, ya, za;
    float xb, yb, zb;
    float dxy, dx, dy, depth_z;

    chr_t *pchr_a, *pchr_b;
    cap_t *pcap_a, *pcap_b;

    // Don't collide with self
    if ( ichr_a == ichr_b ) return bfalse;

    // Ignore invalid characters
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    if ( INVALID_CAP(pchr_a->model) ) return bfalse;
    pcap_a = CapList + pchr_a->model;

    // Ignore invalid characters
    if ( INVALID_CHR(ichr_b) ) return bfalse;
    pchr_b = ChrList + ichr_b;

    if ( INVALID_CAP(pchr_b->model) ) return bfalse;
    pcap_b = CapList + pchr_b->model;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // don't collide if there is no interaction
    if ( 0 == pchr_a->bumpsize || 0 == pchr_b->bumpsize ) return bfalse;

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

    if ( (pchr_a->platform && pcap_b->canuseplatforms) ||
            (pchr_b->platform && pcap_a->canuseplatforms) )
    {
        collide_z  = (depth_z > -PLATTOLERANCE);
    }
    else
    {
        collide_z  = (depth_z > 0);
    }

    if ( !collide_x || !collide_y || !collide_z || !collide_xy ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b )
{
    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t collide_z  = bfalse;

    float xa, ya, za;
    float xb, yb, zb;
    float dxy, dx, dy, depth_z;

    chr_t * pchr_a;
    prt_t * pprt_b;

    // Ignore invalid characters
    if ( INVALID_CHR(ichr_a) ) return bfalse;
    pchr_a = ChrList + ichr_a;

    // Ignore invalid characters
    if ( INVALID_PRT(iprt_b) ) return bfalse;
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
    if ( 0 == pchr_a->bumpsize || 0 == pprt_b->bumpsize ) return bfalse;

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

    if ( !collide_x || !collide_y || !collide_z || !collide_xy ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t * set_PMesh( ego_mpd_t * pmpd )
{
    ego_mpd_t * pmpd_old = PMesh;

    PMesh = pmpd;

    return pmpd_old;
}

//--------------------------------------------------------------------------------------------
camera_t * set_PCamera( camera_t * pcam )
{
    camera_t * pcam_old = PCamera;

    PCamera = pcam;

    // Matrix init stuff (from remove.c)
    rotmeshtopside    = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHTOPSIDE / ( 1.33333f );
    rotmeshbottomside = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHBOTTOMSIDE / ( 1.33333f );
    rotmeshup         = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHUP / ( 1.33333f );
    rotmeshdown       = ( ( float )sdl_scr.x / sdl_scr.y ) * ROTMESHDOWN / ( 1.33333f );

    return pcam_old;
}

//--------------------------------------------------------------------------------------------
bool_t water_data_init( water_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof(water_data_t) );

    pdata->spek_start =   128;
    pdata->spek_level =   128;
    pdata->is_water   = btrue;

    pdata->foregroundrepeat = 1;
    pdata->backgroundrepeat = 1;

    if ( pdata->light )
    {
        int layer;
        for ( layer = 0; layer < pdata->layer_count; layer++ )
        {
            pdata->layer[layer].alpha = 255;  // Some cards don't support alpha lights...
        }
    }

    return btrue;
}

bool_t water_instance_init( water_instance_t * pinst, water_data_t * pdata )
{
    int layer;

    if (NULL == pinst) return bfalse;

    memset(pinst, 0, sizeof(water_instance_t));

    for ( layer = 0; layer < MAXWATERLAYER; layer++)
    {
        pinst->layer[layer].frame = rand() & WATERFRAMEAND;
    }

    if ( NULL != pdata )
    {
        pinst->surface_level = pdata->surface_level;
        pinst->douse_level   = pdata->douse_level;

        for ( layer = 0; layer < MAXWATERLAYER; layer++)
        {
            pinst->layer[layer].z         = pdata->layer[layer].z;
            pinst->layer[layer].dist      = pdata->layer[layer].dist;

            pinst->layer[layer].light_dir = pdata->layer[layer].light_dir / 63.0f;
            pinst->layer[layer].light_add = pdata->layer[layer].light_add / 63.0f;
        }
    }

    make_water( pinst, pdata );

    return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t weather_data_init( weather_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof(weather_data_t) );

    pdata->timer_reset = 10;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t weather_instance_init( weather_instance_t * pinst, weather_data_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof(weather_instance_t) );

    if ( NULL == pdata ) return bfalse;

    pinst->time = pdata->timer_reset;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fog_data_init( fog_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    pdata->top           = 100;
    pdata->bottom        = 0.0f;
    pdata->red           = 255;
    pdata->grn           = 255;
    pdata->blu           = 255;
    pdata->affects_water = btrue;

    return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t fog_instance_init( fog_instance_t * pinst, fog_data_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof(fog_instance_t) );

    pinst->distance = 100;
    pinst->on       = cfg.fog_allowed;

    if ( NULL == pdata ) return bfalse;

    pinst->top    = pdata->top;
    pinst->bottom = pdata->bottom;

    pinst->red    = pdata->red;
    pinst->grn    = pdata->grn;
    pinst->blu    = pdata->blu;

    pinst->distance = ( pdata->top - pdata->bottom );
    if ( pinst->distance < 1.0f )  pinst->on = bfalse;

    return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t damagetile_data_init( damagetile_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    pdata->parttype = -1;
    pdata->partand  = 255;
    pdata->sound    = INVALID_SOUND;
    pdata->type     = DAMAGE_FIRE;
    pdata->amount   = 256;

    return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t damagetile_instance_init( damagetile_instance_t * pinst, damagetile_data_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof(damagetile_instance_t) );

    pinst->sound_time   = TILESOUNDTIME;
    pinst->min_distance = 9999;

    if ( NULL == pdata ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t animtile_data_init( animtile_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof(animtile_data_t) );

    pdata->update_and    = 7;                        // New tile every 7 frames
    pdata->frame_and     = 3;              // Only 4 frames

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t animtile_instance_init( animtile_instance_t pinst[], animtile_data_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    pinst[0].frame_and  = (1 << 3) - 1;
    pinst[0].base_and   = ~pinst[0].frame_and;

    pinst[1].frame_and  = (1 << 4) - 1;
    pinst[1].base_and   = ~pinst[1].frame_and;

    if ( NULL != pdata ) return bfalse;

    pinst[0].frame_and = pdata->frame_and;
    pinst[0].base_and  = ~pinst[0].frame_and;

    pinst[1].frame_and = ( pdata->frame_and << 1 ) | 1;
    pinst[1].frame_and = ~pinst[1].frame_and;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void read_wawalite( const char *modname )
{
    // ZZ> This function sets up water and lighting for the module
    char newloadname[256];
    FILE* fileread;
    float fTmp;
    char cTmp;
    int iTmp;

    water_data_init( &water_data );
    weather_data_init( &weather_data );
    fog_data_init( &fog_data );
    damagetile_data_init( &damagetile_data );
    animtile_data_init( &animtile_data );

    make_newloadname( modname, "gamedat" SLASH_STR "wawalite.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread )
    {
        log_error( "Could not read file! (wawalite.txt)\n" );
        return;
    }

    goto_colon( NULL, fileread, bfalse );
    //  !!!BAD!!!
    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    //  !!!BAD!!!

    // Read water data first
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer_count = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.spek_start = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.spek_level = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.douse_level = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.surface_level = iTmp;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'T' || cTmp == 't' )  water_data.light = btrue;
    else water_data.light = bfalse;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    water_data.is_water = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  water_data.is_water = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    water_data.overlay_req = ('T' == toupper(cTmp));

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    water_data.background_req = ('T' == toupper(cTmp));

	//General data info
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[0].dist.x = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[0].dist.y = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[1].dist.x = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[1].dist.y = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.foregroundrepeat = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.backgroundrepeat = iTmp;

	//Read data on first water layer
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[0].z = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[0].alpha = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[0].frame_add = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[0].light_dir = CLIP(iTmp, 0, 63);
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[0].light_add = CLIP(iTmp, 0, 63);
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[0].amp = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[0].tx_add.x = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[0].tx_add.y = fTmp;

	//Read data on second water layer
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[1].z = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[1].alpha = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[1].frame_add = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[1].light_dir = CLIP(iTmp, 0, 63);
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  water_data.layer[1].light_add = CLIP(iTmp, 0, 63);
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[1].amp = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[1].tx_add.x = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  water_data.layer[1].tx_add.y = fTmp;

    // Read light data second
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_x = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_y = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_z = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  light_a = fTmp*10.0f;

    light_d = 0.0f;
    if ( ABS(light_x) + ABS(light_y) + ABS(light_z) > 0 )
    {
        fTmp = SQRT( light_x * light_x + light_y * light_y + light_z * light_z );

        // get the extra magnitude of the direct light
        light_d = (1.0f - light_a) * fTmp;
        light_d = CLIP(light_d, 0.0f, 1.0f);

        light_x /= fTmp;
        light_y /= fTmp;
        light_z /= fTmp;
    }

    // Read tile data third
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  hillslide = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  slippyfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  airfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  waterfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  noslipfriction = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  gravity = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  animtile_data.update_and = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  animtile_data.frame_and = iTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  damagetile_data.amount = iTmp;
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'S' || cTmp == 's' )  damagetile_data.type = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  damagetile_data.type = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  damagetile_data.type = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  damagetile_data.type = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  damagetile_data.type = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  damagetile_data.type = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  damagetile_data.type = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  damagetile_data.type = DAMAGE_ZAP;

    // Read weather data fourth
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    weather_data.over_water = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  weather_data.over_water = btrue;

    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );  weather_data.timer_reset = iTmp;

    // Read extra data
    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    gfx.exploremode = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  gfx.exploremode = btrue;

    goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
    gfx.usefaredge = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) gfx.usefaredge = btrue;

    PCamera->swing = 0;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  PCamera->swingrate = fTmp;
    goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  PCamera->swingamp = fTmp;

    // Read unnecessary data...  Only read if it exists...
    if ( goto_colon( NULL, fileread, btrue ) )
    {
        fscanf( fileread, "%f", &fTmp );  fog_data.top = fTmp;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  fog_data.bottom = fTmp;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  fog_data.red = fTmp * 255;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  fog_data.grn = fTmp * 255;
        goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%f", &fTmp );  fog_data.blu = fTmp * 255;
        goto_colon( NULL, fileread, bfalse );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'F' || cTmp == 'f' )  fog_data.affects_water = bfalse;

        // Read extra stuff for damage tile particles...
        if ( goto_colon( NULL, fileread, btrue ) )
        {
            fscanf( fileread, "%d", &iTmp );  damagetile_data.parttype = iTmp;
            goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );
            damagetile_data.partand = iTmp;
            goto_colon( NULL, fileread, bfalse );  fscanf( fileread, "%d", &iTmp );
            damagetile_data.sound = CLIP(iTmp, -1, MAX_WAVE);
        }
    }

    fclose( fileread );

    fog_instance_init( &fog, &fog_data );
    water_instance_init( &water, &water_data );
    weather_instance_init( &weather, &weather_data );
    damagetile_instance_init( &damagetile, &damagetile_data );
    animtile_instance_init( animtile, &animtile_data );

    // Allow slow machines to ignore the fancy stuff
    if ( !cfg.twolayerwater_allowed && water_data.layer_count > 1 )
    {
        water_data.layer_count = 1;
        iTmp = water_data.layer[0].alpha;
        iTmp = FF_MUL( water_data.layer[1].alpha, iTmp ) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        water_data.layer[0].alpha = iTmp;
    }

    // Do it
    make_lighttable( light_x, light_y, light_z, light_a );
    make_lighttospek();
}

//---------------------------------------------------------------------------------------------
float get_mesh_level( ego_mpd_t * pmesh, float x, float y, bool_t waterwalk )
{
    // ZZ> This function returns the height of a point within a mesh fan, precise
    //     If waterwalk is nonzero and the fan is watery, then the level returned is the
    //     level of the water.

    float zdone;

    zdone = mesh_get_level( pmesh, x, y );

    if ( waterwalk && water.surface_level > zdone && water_data.is_water )
    {
        int tile = mesh_get_tile( pmesh, x, y );

        if (  0 != mesh_test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
bool_t make_water( water_instance_t * pinst, water_data_t * pdata )
{
    // ZZ> This function sets up water movements
    int layer, frame, point, cnt;
    float temp;
    Uint8 spek;

    if ( NULL == pinst || NULL == pdata ) return bfalse;

    for ( layer = 0; layer < pdata->layer_count; layer++ )
    {
        pinst->layer[layer].tx.x = 0;
        pinst->layer[layer].tx.y = 0;

        for ( frame = 0; frame < MAXWATERFRAME; frame++ )
        {
            // Do first mode
            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = SIN( ( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( PI / 2 * layer / MAXWATERLAYER ) );
                pinst->layer_z_add[layer][frame][point] = temp * pdata->layer[layer].amp;
            }
        }
    }

    // Calculate specular highlights
    spek = 0;
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        spek = 0;
        if ( cnt > pdata->spek_start )
        {
            temp = cnt - pdata->spek_start;
            temp = temp / ( 256 - pdata->spek_start );
            temp = temp * temp;
            spek = temp * pdata->spek_level;
        }

        // [claforte] Probably need to replace this with a
        //            GL_DEBUG(glColor4f)(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if ( gfx.shading == GL_FLAT )
            pinst->spek[cnt] = 0;
        else
            pinst->spek[cnt] = spek;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void reset_end_text()
{
    // ZZ> This function resets the end-module text
    endtextwrite = sprintf( endtext, "The game has ended..." );

    /*
    if ( numpla > 1 )
    {
        endtextwrite = sprintf( endtext, "Sadly, they were never heard from again..." );
    }
    else
    {
        if ( numpla == 0 )
        {
            // No players???
            endtextwrite = sprintf( endtext, "The game has ended..." );
        }
        else
        {
            // One player
            endtextwrite = sprintf( endtext, "Sadly, no trace was ever found..." );
        }
    }
    */

    str_add_linebreaks( endtext, endtextwrite, 20 );
}

//--------------------------------------------------------------------------------------------
void append_end_text( script_state_t * pstate, int message, Uint16 character )
{
    // ZZ> This function appends a message to the end-module text
    int read, cnt;
    char *eread;
    char szTmp[256];
    char cTmp, lTmp;
    Uint16 target, owner;

    target = ChrList[character].ai.target;
    owner = ChrList[character].ai.owner;
    if ( message < msgtotal )
    {
        // Copy the message
        read = msgindex[message];
        cnt = 0;
        cTmp = msgtext[read];  read++;

        while ( cTmp != 0 )
        {
            if ( cTmp == '%' )
            {
                // Escape sequence
                eread = szTmp;
                szTmp[0] = 0;
                cTmp = msgtext[read];  read++;
                if ( cTmp == 'n' )  // Name
                {
                    if ( ChrList[character].nameknown )
                        sprintf( szTmp, "%s", ChrList[character].name );
                    else
                    {
                        lTmp = CapList[ChrList[character].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[character].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[character].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 'c' )  // Class name
                {
                    eread = CapList[ChrList[character].model].classname;
                }
                if ( cTmp == 't' )  // Target name
                {
                    if ( ChrList[target].nameknown )
                        sprintf( szTmp, "%s", ChrList[target].name );
                    else
                    {
                        lTmp = CapList[ChrList[target].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[target].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[target].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 'o' )  // Owner name
                {
                    if ( ChrList[owner].nameknown )
                        sprintf( szTmp, "%s", ChrList[owner].name );
                    else
                    {
                        lTmp = CapList[ChrList[owner].model].classname[0];
                        if ( lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U' )
                            sprintf( szTmp, "an %s", CapList[ChrList[owner].model].classname );
                        else
                            sprintf( szTmp, "a %s", CapList[ChrList[owner].model].classname );
                    }
                    if ( cnt == 0 && szTmp[0] == 'a' )  szTmp[0] = 'A';
                }
                if ( cTmp == 's' )  // Target class name
                {
                    eread = CapList[ChrList[target].model].classname;
                }
                if ( cTmp >= '0' && cTmp <= '3' )  // Target's skin name
                {
                    eread = CapList[ChrList[target].model].skinname[cTmp-'0'];
                }
                if ( NULL == pstate )
                {
                    sprintf( szTmp, "%%%c???", cTmp );
                }
                else
                {
                    if ( cTmp == 'd' )  // tmpdistance value
                    {
                        sprintf( szTmp, "%d", pstate->distance );
                    }
                    if ( cTmp == 'x' )  // tmpx value
                    {
                        sprintf( szTmp, "%d", pstate->x );
                    }
                    if ( cTmp == 'y' )  // tmpy value
                    {
                        sprintf( szTmp, "%d", pstate->y );
                    }
                    if ( cTmp == 'D' )  // tmpdistance value
                    {
                        sprintf( szTmp, "%2d", pstate->distance );
                    }
                    if ( cTmp == 'X' )  // tmpx value
                    {
                        sprintf( szTmp, "%2d", pstate->x );
                    }
                    if ( cTmp == 'Y' )  // tmpy value
                    {
                        sprintf( szTmp, "%2d", pstate->y );
                    }

                }
                if ( cTmp == 'a' )  // Character's ammo
                {
                    if ( ChrList[character].ammoknown )
                        sprintf( szTmp, "%d", ChrList[character].ammo );
                    else
                        sprintf( szTmp, "?" );
                }
                if ( cTmp == 'k' )  // Kurse state
                {
                    if ( ChrList[character].iskursed )
                        sprintf( szTmp, "kursed" );
                    else
                        sprintf( szTmp, "unkursed" );
                }
                if ( cTmp == 'p' )  // Character's possessive
                {
                    if ( ChrList[character].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( ChrList[character].gender == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }
                if ( cTmp == 'm' )  // Character's gender
                {
                    if ( ChrList[character].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "female " );
                    }
                    else
                    {
                        if ( ChrList[character].gender == GENMALE )
                        {
                            sprintf( szTmp, "male " );
                        }
                        else
                        {
                            sprintf( szTmp, " " );
                        }
                    }
                }
                if ( cTmp == 'g' )  // Target's possessive
                {
                    if ( ChrList[target].gender == GENFEMALE )
                    {
                        sprintf( szTmp, "her" );
                    }
                    else
                    {
                        if ( ChrList[target].gender == GENMALE )
                        {
                            sprintf( szTmp, "his" );
                        }
                        else
                        {
                            sprintf( szTmp, "its" );
                        }
                    }
                }
                if ( cTmp == '#' )  // New line (enter)
                {
                    sprintf( szTmp, "\n" );
                }

                // Copy the generated text
                cTmp = *eread;  eread++;

                while ( cTmp != 0 && endtextwrite < MAXENDTEXT - 1 )
                {
                    endtext[endtextwrite] = cTmp;
                    cTmp = *eread;  eread++;
                    endtextwrite++;
                }
            }
            else
            {
                // Copy the letter
                if ( endtextwrite < MAXENDTEXT - 1 )
                {
                    endtext[endtextwrite] = cTmp;
                    endtextwrite++;
                }
            }

            cTmp = msgtext[read];  read++;
            cnt++;
        }
    }

    endtext[endtextwrite] = 0;

    str_add_linebreaks( endtext, endtextwrite, 20 );
}

//--------------------------------------------------------------------------------------------
bool_t game_choose_module( int imod, int seed )
{
    if ( seed < 0 ) seed = time(NULL);

    if ( NULL == PMod ) PMod = &gmod;

    return module_upload( PMod, imod, seed );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
process_instance_t * process_instance_init( process_instance_t * proc )
{
    if ( NULL == proc ) return proc;

    memset( proc, 0, sizeof(process_instance_t) );

    proc->terminated = btrue;

    return proc;
}

bool_t process_instance_start( process_instance_t * proc )
{
    if ( NULL == proc ) return bfalse;

    // choose the correct proc->state
    if ( proc->terminated || proc->state > proc_leaving )
    {
        // must re-initialize the process
        proc->state = proc_begin;
    }
    if ( proc->state > proc_entering )
    {
        // the process is already initialized, just put it back in
        // proc_entering mode
        proc->state = proc_entering;
    }

    // tell it to run
    proc->terminated = bfalse;
    proc->valid      = btrue;
    proc->paused     = bfalse;

    return btrue;
}

bool_t process_instance_kill( process_instance_t * proc )
{
    if ( NULL == proc ) return bfalse;
    if ( !process_instance_validate(proc) ) return btrue;

    // turn the process back on with an order to commit suicide
    proc->paused = bfalse;
    proc->killme = btrue;

    return btrue;
}

bool_t process_instance_validate( process_instance_t * proc )
{
    if (NULL == proc) return bfalse;

    if ( !proc->valid || proc->terminated )
    {
        process_instance_terminate( proc );
    }

    return proc->valid;
}

bool_t process_instance_terminate( process_instance_t * proc )
{
    if (NULL == proc) return bfalse;

    proc->valid      = bfalse;
    proc->terminated = btrue;
    proc->state      = proc_begin;

    return btrue;
}

bool_t process_instance_pause( process_instance_t * proc )
{
    bool_t old_value;

    if ( !process_instance_validate(proc) ) return bfalse;

    old_value    = proc->paused;
    proc->paused = btrue;

    return old_value != proc->paused;
}

bool_t process_instance_resume( process_instance_t * proc )
{
    bool_t old_value;

    if ( !process_instance_validate(proc) ) return bfalse;

    old_value    = proc->paused;
    proc->paused = bfalse;

    return old_value != proc->paused;
}

bool_t process_instance_running( process_instance_t * proc )
{
    if ( !process_instance_validate(proc) ) return bfalse;

    return !proc->paused;
}

//--------------------------------------------------------------------------------------------
ego_process_t * ego_process_init( ego_process_t * eproc )
{
    if ( NULL == eproc ) return NULL;

    memset( eproc, 0, sizeof(ego_process_t) );

    process_instance_init( PROC_PBASE(eproc) );

    return eproc;
}

//--------------------------------------------------------------------------------------------
menu_process_t * menu_process_init( menu_process_t * mproc )
{
    if ( NULL == mproc ) return NULL;

    memset( mproc, 0, sizeof(menu_process_t) );

    process_instance_init( PROC_PBASE(mproc) );

    return mproc;
}

//--------------------------------------------------------------------------------------------
game_process_t * game_process_init( game_process_t * gproc )
{
    if ( NULL == gproc ) return NULL;

    memset( gproc, 0, sizeof(game_process_t) );

    process_instance_init( PROC_PBASE(gproc) );

    gproc->menu_depth = -1;
    gproc->pause_key_ready = btrue;

    return gproc;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void init_all_pip()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        memset( PipList + cnt, 0, sizeof(pip_t) );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_eve()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAXEVE; cnt++ )
    {
        memset( EveList + cnt, 0, sizeof(pip_t) );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_cap()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( CapList + cnt, 0, sizeof(cap_t) );
    }
}

//---------------------------------------------------------------------------------------------
void init_all_mad()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( MadList + cnt, 0, sizeof(mad_t) );

        strncpy( MadList[cnt].name, "*NONE*", sizeof(MadList[cnt].name) );
        MadList[cnt].ai = 0;
    }

    md2_loadframe = 0;
}

//---------------------------------------------------------------------------------------------
void init_all_profiles()
{
    // ZZ> This function initializes all of the model profiles

    init_all_pip();
    init_all_eve();
    init_all_cap();
    init_all_mad();
    init_all_ai_scripts();
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void release_all_pip()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        memset( PipList + cnt, 0, sizeof(pip_t) );
    }
}

//---------------------------------------------------------------------------------------------
void release_all_eve()
{
    int cnt;

    for ( cnt = 0; cnt < MAXEVE; cnt++ )
    {
        memset( EveList + cnt, 0, sizeof(pip_t) );
    }
}

//---------------------------------------------------------------------------------------------
void release_all_cap()
{
    int cnt, tnc;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        cap_t * pcap = CapList + cnt;

        memset( pcap, 0, sizeof(cap_t) );

        for ( tnc = 0; tnc < MAXSECTION; tnc++ )
        {
            pcap->chop_sectionstart[tnc] = MAXCHOP;
            pcap->chop_sectionsize[tnc] = 0;
        }
    };
}

//---------------------------------------------------------------------------------------------
void release_all_mad()
{
    int cnt;

    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        memset( MadList + cnt, 0, sizeof(mad_t) );
        strncpy( MadList[cnt].name, "*NONE*", sizeof(MadList[cnt].name) );
        MadList[cnt].ai = 0;
    }

    md2_loadframe = 0;
}

//---------------------------------------------------------------------------------------------
void release_all_profiles()
{
    // ZZ> This function clears out all of the model data

    release_all_pip();
    release_all_eve();
    release_all_cap();
    release_all_mad();
    release_all_ai_scripts();
}

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
charb = ChrList[charb].fanblock_next;
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

void do_game_hud()
{
    int y = 0;

    if ( flip_pages_requested() && cfg.dev_mode )
    {
        GL_DEBUG(glColor4f)( 1, 1, 1, 1 );
        if ( fpson )
        {
            y = draw_string( 0, y, szfpstext );
        }

        y = draw_string( 0, y, "Menu time %f", MProc->base.dtime );
    }
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_line_of_sight_info
{
    float x0, y0;
    float x1, y1;
    Uint32 stopped_by;

    Uint16 collide_chr;
    Uint32 collide_fx;
    int    collide_x;
    int    collide_y;
};

typedef struct s_line_of_sight_info line_of_sight_info_t;

//--------------------------------------------------------------------------------------------
bool_t collide_ray_with_mesh( line_of_sight_info_t * plos )
{
    Uint32 fan_last;

    int Dx, Dy;
    int ix, ix_stt, ix_end;
    int iy, iy_stt, iy_end;

    int Dbig, Dsmall;
    int ibig, ibig_stt, ibig_end;
    int ismall, ismall_stt, ismall_end;
    int dbig, dsmall;
    int TwoDsmall, TwoDsmallMinusTwoDbig, TwoDsmallMinusDbig;

    bool_t steep;

    if (NULL == plos) return bfalse;

    if ( 0 == plos->stopped_by ) return bfalse;

    ix_stt = plos->x0 / TILE_SIZE;
    ix_end = plos->x1 / TILE_SIZE;

    iy_stt = plos->y0 / TILE_SIZE;
    iy_end = plos->y1 / TILE_SIZE;

    Dx = plos->x1 - plos->x0;
    Dy = plos->y1 - plos->y0;

    steep = (ABS(Dy) >= ABS(Dx));

    // determine which are the big and small values
    if (steep)
    {
        ibig_stt = iy_stt;
        ibig_end = iy_end;

        ismall_stt = ix_stt;
        ismall_end = ix_end;
    }
    else
    {
        ibig_stt = ix_stt;
        ibig_end = ix_end;

        ismall_stt = iy_stt;
        ismall_end = iy_end;
    }

    // set up the big loop variables
    dbig = 1;
    Dbig = ibig_end - ibig_stt;
    if (Dbig < 0)
    {
        dbig = -1;
        Dbig = -Dbig;
    }

    // set up the small loop variables
    dsmall = 1;
    Dsmall = ismall_end - ismall_stt;
    if (Dsmall < 0)
    {
        dsmall = -1;
        Dsmall = -Dsmall;
    }

    // pre-compute some common values
    TwoDsmall             = 2 * Dsmall;
    TwoDsmallMinusTwoDbig = TwoDsmall - 2 * Dbig;
    TwoDsmallMinusDbig    = TwoDsmall - Dbig;

    fan_last = INVALID_TILE;
    for (ibig = ibig_stt, ismall = ismall_stt;  ibig != ibig_end;  ibig += dbig )
    {
        Uint32 fan;

        if (steep)
        {
            ix = ismall;
            iy = ibig;
        }
        else
        {
            ix = ibig;
            iy = ismall;
        }

        // check to see if the "ray" collides with the mesh
        fan = mesh_get_tile_int(PMesh, ix, iy);
        if ( INVALID_TILE != fan && fan != fan_last )
        {
            // collide the ray with the mesh

            if ( 0 != (PMesh->mmem.tile_list[fan].fx & plos->stopped_by) )
            {
                plos->collide_x  = ix;
                plos->collide_y  = iy;
                plos->collide_fx = PMesh->mmem.tile_list[fan].fx & plos->stopped_by;

                return btrue;
            }

            fan_last = fan;
        }

        // go to the next step
        if (TwoDsmallMinusDbig > 0)
        {
            TwoDsmallMinusDbig += TwoDsmallMinusTwoDbig;
            ismall             += dsmall;
        }
        else
        {
            TwoDsmallMinusDbig += TwoDsmall;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t collide_ray_with_characters( line_of_sight_info_t * plos )
{
    Uint16 ichr;

    if (NULL == plos) return bfalse;

    for ( ichr = 0; ichr < MAX_CHR; ichr++)
    {
        if ( !ChrList[ichr].on ) continue;

        // do line/character intersection
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t do_line_of_sight( line_of_sight_info_t * plos )
{
    bool_t mesh_hit, chr_hit;

    mesh_hit = collide_ray_with_mesh( plos );

    if ( mesh_hit )
    {
        plos->x1 = plos->collide_x * TILE_SIZE;
        plos->y1 = plos->collide_y * TILE_SIZE;
    }

    chr_hit = collide_ray_with_characters( plos );

    return mesh_hit || chr_hit;
}
