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

/// @file game.c
/// @brief The code for controlling the game
/// @details

#include "game.h"

#include "char.h"
#include "particle.h"
#include "mad.h"
#include "enchant.h"
#include "profile.h"

#include "controls_file.h"
#include "scancode_file.h"

#include "physics.h"
#include "clock.h"
#include "link.h"
#include "ui.h"
#include "font_bmp.h"
#include "font_ttf.h"
#include "log.h"
#include "system.h"
#include "script.h"
#include "sound.h"
#include "graphic.h"
#include "passage.h"
#include "input.h"
#include "menu.h"
#include "network.h"
#include "mesh.h"
#include "texture.h"
#include "wawalite_file.h"
#include "clock.h"
#include "spawn_file.h"
#include "camera.h"
#include "id_md2.h"
#include "collision.h"
#include "graphic_fan.h"

#include "script_compile.h"
#include "script.h"

#include "egoboo_vfs.h"
#include "egoboo_endian.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_vfs.h"
#include "egoboo.h"

#include "SDL_extensions.h"

#include "egoboo_console.h"
#if defined(USE_LUA_CONSOLE)
#include "lua_console.h"
#endif

#include <SDL_image.h>

#include <time.h>
#include <assert.h>
#include <float.h>
#include <string.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Data needed to specify a line-of-sight test
struct s_line_of_sight_info
{
    float x0, y0, z0;
    float x1, y1, z1;
    Uint32 stopped_by;

    Uint16 collide_chr;
    Uint32 collide_fx;
    int    collide_x;
    int    collide_y;
};

typedef struct s_line_of_sight_info line_of_sight_info_t;

static bool_t collide_ray_with_mesh( line_of_sight_info_t * plos );
static bool_t collide_ray_with_characters( line_of_sight_info_t * plos );
static bool_t do_line_of_sight( line_of_sight_info_t * plos );

//--------------------------------------------------------------------------------------------
void do_weather_spawn_particles();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static ego_mpd_t         _mesh[2];
static camera_t          _camera[2];

static game_process_t    _gproc;
static game_module_t      gmod;

PROFILE_DECLARE( game_update_loop );
PROFILE_DECLARE( gfx_loop );
PROFILE_DECLARE( game_single_update );

PROFILE_DECLARE( talk_to_remotes );
PROFILE_DECLARE( listen_for_packets );
PROFILE_DECLARE( check_stats );
PROFILE_DECLARE( set_local_latches );
PROFILE_DECLARE( check_passage_music );
PROFILE_DECLARE( cl_talkToHost );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t  overrideslots      = bfalse;

// End text
char   endtext[MAXENDTEXT] = EMPTY_CSTR;
size_t endtext_carat = 0;

// Status displays
bool_t StatusList_on     = btrue;
int    StatusList_count    = 0;
Uint16 StatusList[MAXSTAT];

ego_mpd_t         * PMesh   = _mesh + 0;
camera_t          * PCamera = _camera + 0;
game_module_t     * PMod    = &gmod;
game_process_t    * GProc   = &_gproc;

pit_info_t pits = { bfalse, bfalse, ZERO_VECT3 };

Uint16  glouseangle = 0;                                        // actually still used

Uint32                animtile_update_and = 0;
animtile_instance_t   animtile[2];
damagetile_instance_t damagetile;
weather_instance_t    weather;
water_instance_t      water;
fog_instance_t        fog;

Uint8  local_senseenemiesTeam = TEAM_GOOD; // TEAM_MAX;
IDSZ   local_senseenemiesID   = IDSZ_NONE;

// declare the variables to do profiling

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void reset_timers();

// looping - stuff called every loop - not accessible by scripts
static void check_stats();
static void tilt_characters_to_terrain();
//static void stat_return();
static void update_pits();
static int  update_game();
static void game_update_timers();
static void do_damage_tiles( void );
static void set_local_latches( void );
static void let_all_characters_think();

// module initialization / deinitialization - not accessible by scripts
static bool_t game_load_module_data( const char *smallname );
static void   game_release_module_data();
static void   game_load_all_profiles( const char *modname );

static void   activate_spawn_file();
static void   activate_alliance_file();
static void   load_all_global_objects();

static bool_t chr_setup_apply( Uint16 ichr, spawn_file_info_t *pinfo );

static void   game_reset_players();

// Model stuff
static void log_madused( const char *savename );

// "process" management
static int do_game_proc_begin( game_process_t * gproc );
static int do_game_proc_running( game_process_t * gproc );
static int do_game_proc_leaving( game_process_t * gproc );

// misc
static bool_t game_begin_menu( menu_process_t * mproc, which_menu_t which );
static void   game_end_menu( menu_process_t * mproc );

static void   do_game_hud();

static void init_all_objects( void );

//--------------------------------------------------------------------------------------------
// Random Things-----------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local )
{
    /// @details ZZ@> This function exports a character

    int tnc = 0;
    cap_t * pcap;
    pro_t * pobj;

    STRING fromdir;
    STRING todir;
    STRING fromfile;
    STRING tofile;
    STRING todirname;
    STRING todirfullname;

    // Don't export enchants
    disenchant_character( character );

    pobj = chr_get_ppro( character );
    if ( NULL == pobj ) return;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    if (( pcap->cancarrytonextmodule || !pcap->isitem ) && PMod->exportvalid )
    {
        // TWINK_BO.OBJ
        snprintf( todirname, SDL_arraysize( todirname ), "%s", str_encode_path( ChrList.lst[owner].Name ) );

        // Is it a character or an item?
        if ( owner != character )
        {
            // Item is a subdirectory of the owner directory...
            snprintf( todirfullname, SDL_arraysize( todirfullname ), "%s" SLASH_STR "%d.obj", todirname, number );
        }
        else
        {
            // Character directory
            snprintf( todirfullname, SDL_arraysize( todirfullname ), "%s", todirname );
        }

        // players/twink.obj or players/twink.obj/sword.obj
        if ( is_local )
        {
            snprintf( todir, SDL_arraysize( todir ), "players" SLASH_STR "%s", todirfullname );
        }
        else
        {
            snprintf( todir, SDL_arraysize( todir ), "remote" SLASH_STR "%s", todirfullname );
        }

        // modules/advent.mod/objects/advent.obj
        snprintf( fromdir, SDL_arraysize( fromdir ), "%s", pobj->name );

        // Delete all the old items
        if ( owner == character )
        {
            for ( tnc = 0; tnc < MAXIMPORTOBJECTS; tnc++ )
            {
                snprintf( tofile, SDL_arraysize( tofile ), "%s" SLASH_STR "%d.obj", todir, tnc ); /*.OBJ*/
                vfs_removeDirectoryAndContents( tofile, btrue );
            }
        }

        // Make the directory
        vfs_mkdir( todir );

        // Build the DATA.TXT file
        snprintf( tofile, SDL_arraysize( tofile ), "%s" SLASH_STR "data.txt", todir ); /*DATA.TXT*/
        export_one_character_profile( tofile, character );

        // Build the SKIN.TXT file
        snprintf( tofile, SDL_arraysize( tofile ), "%s" SLASH_STR "skin.txt", todir ); /*SKIN.TXT*/
        export_one_character_skin( tofile, character );

        // Build the NAMING.TXT file
        snprintf( tofile, SDL_arraysize( tofile ), "%s" SLASH_STR "naming.txt", todir ); /*NAMING.TXT*/
        export_one_character_name( tofile, character );

        // Copy all of the misc. data files
        snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "message.txt", fromdir ); /*MESSAGE.TXT*/
        snprintf( tofile, SDL_arraysize( tofile ), "%s" SLASH_STR "message.txt", todir ); /*MESSAGE.TXT*/
        vfs_copyFile( fromfile, tofile );

        snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "tris.md2", fromdir ); /*TRIS.MD2*/
        snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "tris.md2", todir ); /*TRIS.MD2*/
        vfs_copyFile( fromfile, tofile );

        snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "copy.txt", fromdir ); /*COPY.TXT*/
        snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "copy.txt", todir ); /*COPY.TXT*/
        vfs_copyFile( fromfile, tofile );

        snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "script.txt", fromdir );
        snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "script.txt", todir );
        vfs_copyFile( fromfile, tofile );

        snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "enchant.txt", fromdir );
        snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "enchant.txt", todir );
        vfs_copyFile( fromfile, tofile );

        snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "credits.txt", fromdir );
        snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "credits.txt", todir );
        vfs_copyFile( fromfile, tofile );

        snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "quest.txt", fromdir );
        snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "quest.txt", todir );
        vfs_copyFile( fromfile, tofile );

        // Copy all of the particle files
        for ( tnc = 0; tnc < MAX_PIP_PER_PROFILE; tnc++ )
        {
            snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "part%d.txt", fromdir, tnc );
            snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "part%d.txt", todir,   tnc );
            vfs_copyFile( fromfile, tofile );
        }

        // Copy all of the sound files
        for ( tnc = 0; tnc < MAX_WAVE; tnc++ )
        {
            snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "sound%d.wav", fromdir, tnc );
            snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "sound%d.wav", todir,   tnc );
            vfs_copyFile( fromfile, tofile );

            snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "sound%d.ogg", fromdir, tnc );
            snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "sound%d.ogg", todir,   tnc );
            vfs_copyFile( fromfile, tofile );
        }

        // Copy all of the image files (try to copy all supported formats too)
        for ( tnc = 0; tnc < MAX_SKIN; tnc++ )
        {
            Uint8 type;

            for ( type = 0; type < maxformattypes; type++ )
            {
                snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "tris%d%s", fromdir, tnc, TxFormatSupported[type] );
                snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "tris%d%s", todir,   tnc, TxFormatSupported[type] );
                vfs_copyFile( fromfile, tofile );

                snprintf( fromfile, SDL_arraysize( fromfile ), "%s" SLASH_STR "icon%d%s", fromdir, tnc, TxFormatSupported[type] );
                snprintf( tofile, SDL_arraysize( tofile ),   "%s" SLASH_STR "icon%d%s", todir,   tnc, TxFormatSupported[type] );
                vfs_copyFile( fromfile, tofile );
            }
        }
    }
}

//---------------------------------------------------------------------------------------------
void export_all_players( bool_t require_local )
{
    /// @details ZZ@> This function saves all the local players in the
    ///    PLAYERS directory

    bool_t is_local;
    int cnt, character, item, number;

    // Don't export if the module isn't running
    if ( !process_running( PROC_PBASE( GProc ) ) ) return;

    // Stop if export isnt valid
    if ( !PMod->exportvalid ) return;

    // Check each player
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        is_local = ( 0 != PlaList[cnt].device.bits );
        if ( require_local && !is_local ) continue;
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ACTIVE_CHR( character ) || !ChrList.lst[character].alive ) continue;

        // Export the character
        export_one_character( character, character, 0, is_local );

        // Export the left hand item
        item = ChrList.lst[character].holdingwhich[SLOT_LEFT];
        if ( ACTIVE_CHR( item ) && ChrList.lst[item].isitem )
        {
            export_one_character( item, character, SLOT_LEFT, is_local );
        }

        // Export the right hand item
        item = ChrList.lst[character].holdingwhich[SLOT_RIGHT];
        if ( ACTIVE_CHR( item ) && ChrList.lst[item].isitem )
        {
            export_one_character( item, character, SLOT_RIGHT, is_local );
        }

        // Export the inventory
        number = 0;
        for ( item = ChrList.lst[character].pack_next;
              number < MAXINVENTORY && item != MAX_CHR;
              item = ChrList.lst[item].pack_next )
        {
            if ( ChrList.lst[item].isitem )
            {
                export_one_character( item, character, number + 2, is_local );
                number++;
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void getadd( int min, int value, int max, int* valuetoadd )
{
    /// @details ZZ@> This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

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
    /// @details ZZ@> This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

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
    /// @details ZZ@> This is a debug function for checking model loads

    vfs_FILE* hFileWrite;
    int cnt;

    hFileWrite = vfs_openWrite( savename );
    if ( hFileWrite )
    {
        vfs_printf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        //vfs_printf( hFileWrite, "%d of %d frames used...\n", Md2FrameList_index, MAXFRAME );
        cnt = 0;

        while ( cnt < MAX_PROFILE )
        {
            vfs_printf( hFileWrite, "%3d %32s %s\n", cnt, CapList[cnt].classname, MadList[cnt].name );
            cnt++;
        }

        vfs_close( hFileWrite );
    }
}

//--------------------------------------------------------------------------------------------
void statlist_add( Uint16 character )
{
    /// @details ZZ@> This function adds a status display to the do list

    chr_t * pchr;

    if ( StatusList_count >= MAXSTAT ) return;

    if ( !ACTIVE_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    if ( pchr->StatusList_on ) return;

    StatusList[StatusList_count] = character;
    pchr->StatusList_on = btrue;
    StatusList_count++;
}

//--------------------------------------------------------------------------------------------
void statlist_move_to_top( Uint16 character )
{
    /// @details ZZ@> This function puts the character on top of the StatusList

    int cnt, oldloc;

    // Find where it is
    oldloc = StatusList_count;

    for ( cnt = 0; cnt < StatusList_count; cnt++ )
    {
        if ( StatusList[cnt] == character )
        {
            oldloc = cnt;
            cnt = StatusList_count;
        }
    }

    // Change position
    if ( oldloc < StatusList_count )
    {
        // Move all the lower ones up
        while ( oldloc > 0 )
        {
            oldloc--;
            StatusList[oldloc+1] = StatusList[oldloc];
        }

        // Put the character in the top slot
        StatusList[0] = character;
    }
}

//--------------------------------------------------------------------------------------------
void statlist_sort()
{
    /// @details ZZ@> This function puts all of the local players on top of the StatusList

    int cnt;

    for ( cnt = 0; cnt < PlaList_count; cnt++ )
    {
        if ( PlaList[cnt].valid && PlaList[cnt].device.bits != INPUT_BITS_NONE )
        {
            statlist_move_to_top( PlaList[cnt].index );
        }
    }
}

//--------------------------------------------------------------------------------------------
void chr_set_frame( Uint16 character, Uint16 action, int frame, Uint16 lip )
{
    /// @details ZZ@> This function sets the frame for a character explicitly...  This is used to
    ///    rotate Tank turrets

    chr_t * pchr;
    mad_t * pmad;
    int frame_stt, frame_nxt;

    if ( !ACTIVE_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return;

    action = mad_get_action( chr_get_imad( character ), action );

    if ( rv_success == chr_set_action( pchr, action, btrue, btrue ) )
    {
        frame_stt = pmad->action_stt[action] + frame;
        frame_stt = MIN( frame_stt, pmad->action_end[action] - 1 );

        frame_nxt = frame_stt + 1;
        frame_nxt = MIN( frame_nxt, pmad->action_end[action] - 1 );

        pchr->inst.ilip  = lip;
        pchr->inst.flip  = lip / 4.0f;

        // force the vlst_cache to be invalid if the initial frame changes
        // this should be picked up, by the automated routinr, but take no chances
        if ( pchr->inst.frame_lst != frame_stt )
        {
            pchr->inst.frame_lst  = frame_stt;
            pchr->inst.save.valid = bfalse;
        }

        // force the vlst_cache to be invalid if the initial frame changes
        // this should be picked up, by the automated routinr, but take no chances
        if ( pchr->inst.frame_nxt != frame_nxt )
        {
            pchr->inst.frame_nxt  = frame_nxt;
            pchr->inst.save.valid = bfalse;
        }

    }
}

//--------------------------------------------------------------------------------------------
void activate_alliance_file( /*const char *modname*/ )
{
    /// @details ZZ@> This function reads the alliance file
    STRING szTemp;
    Uint8 teama, teamb;
    vfs_FILE *fileread;

    // Load the file
    fileread = vfs_openRead( "data/alliance.txt" );
    if ( fileread )
    {
        while ( goto_colon( NULL, fileread, btrue ) )
        {
            fget_string( fileread, szTemp, SDL_arraysize( szTemp ) );
            teama = ( szTemp[0] - 'A' ) % TEAM_MAX;

            fget_string( fileread, szTemp, SDL_arraysize( szTemp ) );
            teamb = ( szTemp[0] - 'A' ) % TEAM_MAX;
            TeamList[teama].hatesteam[teamb] = bfalse;
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void update_used_lists()
{
    ChrList_update_used();
    PrtList_update_used();
    EncList_update_used();
}

//--------------------------------------------------------------------------------------------
void update_all_objects()
{
    update_all_characters();
    update_all_particles();
    update_all_enchants();
}

//--------------------------------------------------------------------------------------------
void cleanup_all_objects()
{
    cleanup_all_characters();
    cleanup_all_particles();
    cleanup_all_enchants();
};

//--------------------------------------------------------------------------------------------
void move_all_objects()
{
    move_all_particles();
    move_all_characters();
}

//--------------------------------------------------------------------------------------------
int update_game()
{
    /// @details ZZ@> This function does several iterations of character movements and such
    ///    to keep the game in sync.

    int cnt, tnc, numdead, numalive;
    int update_loop_cnt;

    // Check for all local players being dead
    local_allpladead     = bfalse;
    local_seeinvis_level = 0;
    local_seekurse       = bfalse;
    local_seedark_level  = 0;

    numplayer = 0;
    numdead = numalive = 0;
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        Uint16 ichr;
        chr_t * pchr;

        if ( !PlaList[cnt].valid ) continue;

        // fix bad players
        ichr = PlaList[cnt].index;
        if ( !ACTIVE_CHR( ichr ) )
        {
            PlaList[cnt].index = MAX_CHR;
            PlaList[cnt].valid = bfalse;
            continue;
        }
        pchr = ChrList.lst + ichr;

        // count the total number of players
        numplayer++;

        // only interested in local players
        if ( INPUT_BITS_NONE == PlaList[cnt].device.bits ) continue;

        if ( pchr->alive )
        {
            numalive++;

            if ( pchr->see_invisible_level )
            {
                local_seeinvis_level = MAX( local_seeinvis_level, pchr->see_invisible_level );
            }

            if ( pchr->canseekurse )
            {
                local_seekurse = btrue;
            }

            if ( pchr->darkvision_level )
            {
                local_seedark_level = MAX( local_seedark_level, pchr->darkvision_level );
            }
        }
        else
        {
            numdead++;
        }
    }

    // Did everyone die?
    if ( numdead >= local_numlpla )
    {
        local_allpladead = btrue;
    }

    // check for autorespawn
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        Uint16 ichr;
        chr_t * pchr;

        if ( !PlaList[cnt].valid ) continue;

        ichr = PlaList[cnt].index;
        if ( !ACTIVE_CHR( ichr ) ) continue;
        pchr = ChrList.lst + ichr;

        if ( !pchr->alive )
        {
            if ( cfg.difficulty < GAME_HARD && local_allpladead && SDLKEYDOWN( SDLK_SPACE ) && PMod->respawnvalid && 0 == revivetimer )
            {
                respawn_character( ichr );
                pchr->experience *= EXPKEEP;  // Apply xp Penality

                if ( cfg.difficulty > GAME_EASY )
                {
                    pchr->money *= EXPKEEP;
                }
            }
        }
    }

    PROFILE_BEGIN( talk_to_remotes );
    {
        // get all player latches from the "remotes"
        sv_talkToRemotes();
    }
    PROFILE_END2( talk_to_remotes );

    update_lag = 0;
    update_loop_cnt = 0;
    if ( update_wld < true_update )
    {
        // [claforte Jan 6th 2001]
        /// @todo Put that back in place once networking is functional.
        for ( tnc = 0; update_wld < true_update && tnc < TARGET_UPS ; tnc++ )
        {
            PROFILE_BEGIN( game_single_update );
            {
                // do important stuff to keep in sync inside this loop

                srand( PMod->randsave );
                PMod->randsave = rand();

                //---- begin the code for updating misc. game stuff
                {
                    BillboardList_update_all();
                    animate_tiles();
                    move_water( &water );
                    looped_update_all_sound();
                    do_damage_tiles();
                    update_pits();
                    do_weather_spawn_particles();
                }
                //---- end the code for updating misc. game stuff

                //---- begin the code for updating in-game objects
                {
                    update_all_objects();
                    update_used_lists();

                    let_all_characters_think();           // sets the non-player latches
                    unbuffer_player_latches();            // sets the player latches

                    move_all_objects();                   // clears some latches
                    bump_all_objects();

                    cleanup_all_objects();
                }
                //---- end the code for updating in-game objects

                game_update_timers();

                // put the camera movement inside here
                camera_move( PCamera, PMesh );

                // Timers
                clock_wld += UPDATE_SKIP;
                clock_enc_stat++;
                clock_chr_stat++;

                // Reset the respawn timer
                if ( revivetimer > 0 ) revivetimer--;

                update_wld++;
                ups_loops++;
                update_loop_cnt++;
            }
            PROFILE_END2( game_single_update );

            // estimate how much time the main loop is taking per second
            est_single_update_time = 0.9 * est_single_update_time + 0.1 * PROFILE_QUERY( game_single_update );
            est_single_ups         = 0.9 * est_single_ups         + 0.1 * ( 1.0f / PROFILE_QUERY( game_single_update ) );
        }
        update_lag = tnc;
    }

    //printf( "---- update_loop_cnt %d\n", update_loop_cnt );

    est_update_game_time = 0.9 * est_update_game_time + 0.1 * est_single_update_time * update_loop_cnt;
    est_max_game_ups     = 0.9 * est_max_game_ups     + 0.1 * 1.0 / est_update_game_time;

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

    return update_loop_cnt;
}

//--------------------------------------------------------------------------------------------
void game_update_timers()
{
    /// @details ZZ@> This function updates the game timers

    static bool_t update_was_paused = bfalse;

    int ticks_diff;
    int clock_diff;

    const float fold = 0.77f;
    const float fnew = 1.0f - fold;

    ticks_last = ticks_now;
    ticks_now  = SDL_GetTicks();

    // check to make sure that the game is running
    if ( !process_running( PROC_PBASE( GProc ) ) || GProc->mod_paused )
    {
        // for a local game, force the function to ignore the accumulation of time
        // until you re-join the game
        if ( !PNet->on )
        {
            ticks_last = ticks_now;
            update_was_paused = btrue;
            return;
        }
    }

    // make sure some amount of time has passed
    ticks_diff = ticks_now - ticks_last;
    if ( 0 == ticks_diff ) return;

    // measure the time since the last call
    clock_lst  = clock_all;

    // calculate the time since the from the last update
    // if the game was paused, assume that only one update time elapsed since the last time through this function
    clock_diff = UPDATE_SKIP;
    if ( !update_was_paused )
    {
        clock_diff = ticks_diff;
        clock_diff = MIN( clock_diff, 10 * UPDATE_SKIP );
    }

    if ( PNet->on )
    {
        // if the network game is on, there really is no real "pause"
        // so we can always measure the game time from the first clock reading
        clock_all = ticks_now - clock_stt;
    }
    else
    {
        // if the net is not on, the game clock will pause when the local game is paused.
        // if we use the other calculation, the game will freeze while it handles the updates
        // for all the time that the game was paused... not so good
        clock_all  += clock_diff;
    }

    // Use the number of updates that should have been performed up to this point (true_update)
    // to try to regulate the update speed of the game
    // By limiting this loop to 10, you are essentially saying that the update loop
    // can go 10 times as fast as normal to help update_wld catch up to true_update,
    // but it can't completely bog doen the game
    true_update = clock_all / UPDATE_SKIP;

    // get the number of frames that should have happened so far in a similar way
    true_frame  = clock_all / FRAME_SKIP;

    // figure out the update rate
    ups_clock += clock_diff;

    if ( ups_loops > 0 && ups_clock > 0 )
    {
        stabilized_ups_sum    = stabilized_ups_sum * fold + fnew * ( float ) ups_loops / (( float ) ups_clock / TICKS_PER_SEC );
        stabilized_ups_weight = stabilized_ups_weight * fold + fnew;

        // blank these every so often so that the numbers don't overflow
        if ( ups_loops > 10 * TARGET_UPS )
        {
            ups_loops = 0;
            ups_clock = 0;
        }
    }

    if ( stabilized_ups_weight > 0.5f )
    {
        stabilized_ups = stabilized_ups_sum / stabilized_ups_weight;
    }

    // if it got this far and the funciton had been paused, it is time to unpause it
    update_was_paused = bfalse;
}

//--------------------------------------------------------------------------------------------
void reset_timers()
{
    /// @details ZZ@> This function resets the timers...

    clock_stt = ticks_now = ticks_last = SDL_GetTicks();

    clock_all = 0;
    clock_lst = 0;
    clock_wld = 0;
    clock_enc_stat = 0;
    clock_chr_stat = 0;
    clock_pit = 0;

    update_wld = 0;
    frame_all = 0;
    frame_fps = 0;
    outofsync = bfalse;

    pits.kill = pits.teleport = bfalse;
}

//--------------------------------------------------------------------------------------------
int game_do_menu( menu_process_t * mproc )
{
    /// @details BB@> do menus

    int menuResult;
    bool_t need_menu = bfalse;

    need_menu = bfalse;
    if ( flip_pages_requested() )
    {
        // someone else (and that means the game) has drawn a frame
        // so we just need to draw the menu over that frame
        need_menu = btrue;

        // force the menu to be displayed immediately when the game stops
        mproc->base.dtime = 1.0f / ( float )cfg.framelimit;
    }
    else if ( !process_running( PROC_PBASE( GProc ) ) )
    {
        // the menu's frame rate is controlled by a timer
        mproc->ticks_now = SDL_GetTicks();
        if ( mproc->ticks_now > mproc->ticks_next )
        {
            // FPS limit
            float  frameskip = ( float )TICKS_PER_SEC / ( float )cfg.framelimit;
            mproc->ticks_next = mproc->ticks_now + frameskip;

            need_menu = btrue;
            mproc->base.dtime = 1.0f / ( float )cfg.framelimit;
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
int do_game_proc_begin( game_process_t * gproc )
{
    BillboardList_init_all();

    gproc->escape_latch = bfalse;

    // initialize math objects
    make_randie();
    make_turntosin();

    // reset the fps counter
    fps_clock             = 0;
    fps_loops             = 0;
    stabilized_fps_sum    = 0.1f * TARGET_FPS;
    stabilized_fps_weight = 0.1f;

    // Linking system
    log_info( "Initializing module linking... " );
    if ( link_build( "basicdat" SLASH_STR "link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failure!\n" );

    // intialize the "profile system"
    init_profile_system();

    // do some graphics initialization
    //make_lightdirectionlookup();
    make_enviro();
    camera_new( PCamera );

    // try to start a new module
    if ( !game_begin_module( pickedmodule_name, ( Uint32 )~0 ) )
    {
        // failure - kill the game process
        process_kill( PROC_PBASE( gproc ) );
        process_resume( PROC_PBASE( MProc ) );
    }

    // Initialize the process
    gproc->base.valid = btrue;

    // initialize all the profile variables
    PROFILE_RESET( game_update_loop );
    PROFILE_RESET( game_single_update );
    PROFILE_RESET( gfx_loop );

    PROFILE_RESET( talk_to_remotes );
    PROFILE_RESET( listen_for_packets );
    PROFILE_RESET( check_stats );
    PROFILE_RESET( set_local_latches );
    PROFILE_RESET( check_passage_music );
    PROFILE_RESET( cl_talkToHost );

    stabilized_ups_sum    = 0.0f;
    stabilized_ups_weight = 0.0f;

    stabilized_fps_sum    = 0.0f;
    stabilized_fps_weight = 0.0f;

    // re-initialize these variables
    est_max_fps          =  TARGET_FPS;
    est_render_time      =  1.0f / TARGET_FPS;

    est_update_time      =  1.0f / TARGET_UPS;
    est_max_ups          =  TARGET_UPS;

    est_gfx_time         =  1.0f / TARGET_FPS;
    est_max_gfx          =  TARGET_FPS;

    est_single_update_time  = 1.0f / TARGET_UPS;
    est_single_ups          = TARGET_UPS;

    est_update_game_time  = 1.0f / TARGET_UPS;
    est_max_game_ups      = TARGET_UPS;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_running( game_process_t * gproc )
{
    int update_loops = 0;

    if ( !process_validate( PROC_PBASE( gproc ) ) ) return -1;

    gproc->was_active  = gproc->base.valid;

    if ( gproc->base.paused ) return 0;

    gproc->ups_ticks_now = SDL_GetTicks();
    if ( gproc->ups_ticks_now > gproc->ups_ticks_next )
    {
        // UPS limit
        gproc->ups_ticks_next = gproc->ups_ticks_now + UPDATE_SKIP / 4;

        PROFILE_BEGIN( game_update_loop );
        {
            // This is the control loop
            if ( PNet->on && console_done )
            {
                net_send_message();
            }

            // update all the timers
            game_update_timers();

            // do the updates
            if ( gproc->mod_paused && !PNet->on )
            {
                clock_wld = clock_all;
            }
            else
            {
                // start the console mode?
                if ( control_is_pressed( INPUT_DEVICE_KEYBOARD, CONTROL_MESSAGE ) )
                {
                    // reset the keyboard buffer
                    SDL_EnableKeyRepeat( 20, SDL_DEFAULT_REPEAT_DELAY );
                    console_mode = btrue;
                    console_done = bfalse;
                    keyb.buffer_count = 0;
                    keyb.buffer[0] = CSTR_END;
                }

                // NETWORK PORT
                PROFILE_BEGIN( listen_for_packets );
                {
                    listen_for_packets();
                }
                PROFILE_END2( listen_for_packets );

                PROFILE_BEGIN( check_stats );
                {
                    check_stats();
                }
                PROFILE_END2( check_stats );

                PROFILE_BEGIN( set_local_latches );
                {
                    set_local_latches();
                }
                PROFILE_END2( set_local_latches );

                PROFILE_BEGIN( check_passage_music );
                {
                    check_passage_music();
                }
                PROFILE_END2( check_passage_music );

                if ( PNet->waitingforplayers )
                {
                    clock_wld = clock_all;
                }
                else
                {
                    PROFILE_BEGIN( cl_talkToHost );
                    {
                        cl_talkToHost();
                    }
                    PROFILE_END2( cl_talkToHost );

                    update_loops = update_game();
                }
            }
        }
        PROFILE_END2( game_update_loop );

        // estimate the main-loop update time is taking per inner-loop iteration
        // do a kludge to average out the effects of functions like check_passage_music()
        // even when the inner loop does not execute
        if ( update_loops > 0 )
        {
            est_update_time = 0.9 * est_update_time + 0.1 * PROFILE_QUERY( game_update_loop ) / update_loops;
            est_max_ups     = 0.9 * est_max_ups     + 0.1 * ( update_loops / PROFILE_QUERY( game_update_loop ) );
        }
        else
        {
            est_update_time = 0.9 * est_update_time + 0.1 * PROFILE_QUERY( game_update_loop );
            est_max_ups     = 0.9 * est_max_ups     + 0.1 * ( 1.0f / PROFILE_QUERY( game_update_loop ) );
        }
    }

    // Do the display stuff
    gproc->fps_ticks_now = SDL_GetTicks();
    if ( gproc->fps_ticks_now > gproc->fps_ticks_next )
    {
        // FPS limit
        float  frameskip = ( float )TICKS_PER_SEC / ( float )cfg.framelimit;
        gproc->fps_ticks_next = gproc->fps_ticks_now + frameskip;

        PROFILE_BEGIN( gfx_loop );
        {
            gfx_main();

            msgtimechange++;
        }
        PROFILE_END2( gfx_loop );

        // estimate how much time the main loop is taking per second
        est_gfx_time = 0.9 * est_gfx_time + 0.1 * PROFILE_QUERY( gfx_loop );
        est_max_gfx  = 0.9 * est_max_gfx  + 0.1 * ( 1.0f / PROFILE_QUERY( gfx_loop ) );

        // estimate how much time the main loop is taking per second
        est_render_time = est_gfx_time * TARGET_FPS;
        est_max_fps  = 0.9 * est_max_fps + 0.1 * ( 1.0f - est_update_time * TARGET_UPS ) / PROFILE_QUERY( gfx_loop );
    }

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
    if ( !process_validate( PROC_PBASE( gproc ) ) ) return -1;

    // get rid of all module data
    game_quit_module();

    // resume the menu
    process_resume( PROC_PBASE( MProc ) );

    // reset the fps counter
    fps_clock             = 0;
    fps_loops             = 0;
    stabilized_fps_sum    = 0.1f * stabilized_fps_sum / stabilized_fps_weight;
    stabilized_fps_weight = 0.1f;

    return 1;
}

//--------------------------------------------------------------------------------------------
int do_game_proc_run( game_process_t * gproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_validate( PROC_PBASE( gproc ) ) ) return -1;
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
            // proc_result = do_game_proc_entering( gproc );

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
            process_terminate( PROC_PBASE( gproc ) );
            process_resume( PROC_PBASE( MProc ) );
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16 prt_find_target( float pos_x, float pos_y, float pos_z, Uint16 facing,
                        Uint16 particletype, Uint8 team, Uint16 donttarget,
                        Uint16 oldtarget )
{
    /// @details ZF@> This is the new improved targeting system for particles. Also includes distance in the Z direction.

    const float max_dist2 = WIDE * WIDE;

    pip_t * ppip;

    Uint16 besttarget = MAX_CHR, cnt;
    float  longdist2 = max_dist2;

    if ( !LOADED_PIP( particletype ) ) return MAX_CHR;
    ppip = PipStack.lst + particletype;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t * pchr;
        bool_t target_friend, target_enemy;

        if ( !ACTIVE_CHR( cnt ) ) continue;
        pchr = ChrList.lst + cnt;

        if ( !pchr->alive || pchr->isitem || ACTIVE_CHR( pchr->attachedto ) ) continue;

        // ignore invictus
        if ( pchr->invictus ) continue;

        // we are going to give the player a break and not target things that
        // can't be damaged, unless the particle is homing. If it homes in,
        // the he damagetime could drop off en route.
        if ( !ppip->homing && ( 0 != pchr->damagetime ) ) continue;

        // Don't retarget someone we already had or not supposed to target
        if ( cnt == oldtarget || cnt == donttarget ) continue;

        target_friend = ppip->onlydamagefriendly && team == chr_get_iteam( cnt );
        target_enemy  = !ppip->onlydamagefriendly && TeamList[team].hatesteam[chr_get_iteam( cnt )];

        if ( target_friend || target_enemy )
        {
            Uint16 angle = - facing + vec_to_facing( pchr->pos.x - pos_x , pchr->pos.y - pos_y );

            // Only proceed if we are facing the target
            if ( angle < ppip->targetangle || angle > ( 0xFFFF - ppip->targetangle ) )
            {
                float dist2 =
                    POW( ABS( pchr->pos.x - pos_x ), 2 ) +
                    POW( ABS( pchr->pos.y - pos_y ), 2 ) +
                    POW( ABS( pchr->pos.z - pos_z ), 2 );

                if ( dist2 < longdist2 && dist2 <= max_dist2 )
                {
                    glouseangle = angle;
                    besttarget = cnt;
                    longdist2 = dist2;
                }
            }
        }
    }

    // All done
    return besttarget;
}

//--------------------------------------------------------------------------------------------
bool_t check_target( chr_t * psrc, Uint16 ichr_test, TARGET_TYPE target_type, bool_t target_items, bool_t target_dead, IDSZ target_idsz, bool_t exclude_idsz, bool_t target_players )
{
    bool_t retval;

    bool_t is_hated, hates_me;
    bool_t is_friend, is_prey, is_predator, is_mutual;
    chr_t * ptst;

    // Skip non-existing objects
    if ( !ACTIVE_PCHR( psrc ) ) return bfalse;

    if ( !ACTIVE_CHR( ichr_test ) ) return bfalse;
    ptst = ChrList.lst + ichr_test;

    // Players only?
    if ( target_players && !ptst->isplayer ) return bfalse;

    // Skip held objects and self
    if ( psrc == ptst || ACTIVE_CHR( ptst->attachedto ) || ptst->pack_ispacked ) return bfalse;

    // Either only target dead stuff or alive stuff
    if ( target_dead == ptst->alive ) return bfalse;

    // Dont target invisible stuff, unless we can actually see them
    if ( !chr_can_see_object( GET_INDEX_PCHR( psrc ), ichr_test ) ) return bfalse;

    is_hated = TeamList[psrc->team].hatesteam[ptst->team];
    hates_me = TeamList[ptst->team].hatesteam[psrc->team];

    // Target neutral items? (still target evil items, could be pets)
    if ( !target_items && (( ptst->isitem && is_hated ) || ptst->invictus ) ) return bfalse;

    // these options are here for ideas of ways to mod this function
    is_friend    = !is_hated && !hates_me;
    is_prey      =  is_hated && !hates_me;
    is_predator  = !is_hated &&  hates_me;
    is_mutual    =  is_hated &&  hates_me;

    // Which target_type to target
    retval = bfalse;
    if ( target_type == TARGET_ALL || ( target_type == TARGET_ENEMY && is_hated ) || ( target_type == TARGET_FRIEND && !is_hated ) )
    {
        // Check for specific IDSZ too?
        if ( IDSZ_NONE == target_idsz )
        {
            retval = btrue;
        }
        else
        {
            bool_t match_idsz = ( target_idsz == pro_get_idsz( ptst->iprofile, IDSZ_PARENT ) ) ||
                                ( target_idsz == pro_get_idsz( ptst->iprofile, IDSZ_TYPE ) );

            if ( match_idsz )
            {
                if ( !exclude_idsz ) retval = btrue;
            }
            else
            {
                if ( exclude_idsz ) retval = btrue;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
Uint16 chr_find_target( chr_t * psrc, float max_dist2, TARGET_TYPE target_type, bool_t target_items, bool_t target_dead, IDSZ target_idsz, bool_t exclude_idsz, bool_t target_players )
{
    /// @details BB@> this is the raw character targeting code, this is not throttled at all. You should call
    ///     scr_get_chr_target() if you are calling this function from the scripting system.

    line_of_sight_info_t los_info;

    Uint16 cnt;
    Uint16 best_target = MAX_CHR;
    float  best_dist2;
    Uint16 search_list = target_players ? MAXPLAYER : MAX_CHR;

    if ( TARGET_NONE == target_type ) return MAX_CHR;

    if ( !ACTIVE_PCHR( psrc ) ) return MAX_CHR;

    // set the line-of-sight source
    los_info.x0         = psrc->pos.x;
    los_info.y0         = psrc->pos.y;
    los_info.z0         = psrc->pos.z + psrc->bump.height;
    los_info.stopped_by = psrc->stoppedby;

    best_target = MAX_CHR;
    best_dist2  = max_dist2;
    for ( cnt = 0; cnt < search_list; cnt++ )
    {
        float  dist2;
        fvec3_t   diff;
        chr_t * ptst;
        Uint16 ichr_test = target_players ? PlaList[cnt].index : cnt;

        if ( !ACTIVE_CHR( ichr_test ) ) continue;
        ptst = ChrList.lst + ichr_test;

        if ( !check_target( psrc, ichr_test, target_type, target_items, target_dead, target_idsz, exclude_idsz, target_players ) )
        {
            continue;
        }

        diff  = fvec3_sub( psrc->pos.v, ptst->pos.v );
        dist2 = fvec3_dot_product( diff.v, diff.v );

        if (( 0 == max_dist2 || dist2 <= max_dist2 ) && ( MAX_CHR == best_target || dist2 < best_dist2 ) )
        {
            // set the line-of-sight source
            los_info.x1 = ptst->pos.x;
            los_info.y1 = ptst->pos.y;
            los_info.z1 = ptst->pos.z + MAX( 1, ptst->bump.height );

            //Invictus chars do not need a line of sight
            if ( psrc->invictus || !do_line_of_sight( &los_info ) )
            {
                best_target = ichr_test;
                best_dist2  = dist2;
            }
        }
    }

    // make sure the target is valid
    if ( !ACTIVE_CHR( best_target ) ) best_target = MAX_CHR;

    return best_target;
}

//--------------------------------------------------------------------------------------------
void do_damage_tiles()
{
    Uint16 character;

    // do the damage tile stuff
    for ( character = 0; character < MAX_CHR; character++ )
    {
        cap_t * pcap;
        chr_t * pchr;

        if ( !ACTIVE_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

        pcap = pro_get_pcap( pchr->iprofile );
        if ( NULL == pcap ) continue;

        // if the object is not really in the game, do nothing
        if ( pchr->is_hidden || !pchr->alive ) continue;

        // if you are being held by something, you are protected
        if ( pchr->pack_ispacked ) continue;

        // are we on a damage tile?
        if ( !VALID_TILE( PMesh, pchr->onwhichfan ) ) continue;
        if ( 0 == mesh_test_fx( PMesh, pchr->onwhichfan, MPDFX_DAMAGE ) ) continue;

        // are we low enough?
        if ( pchr->pos.z > pchr->enviro.floor_level + DAMAGERAISE ) continue;

        // allow reaffirming damage to things like torches, even if they are being held,
        // but make the tolerance closer so that books won't burn so easily
        if ( !ACTIVE_CHR( pchr->attachedto ) || pchr->pos.z < pchr->enviro.floor_level + DAMAGERAISE )
        {
            if ( pchr->reaffirmdamagetype == damagetile.type )
            {
                if ( 0 == ( update_wld & TILEREAFFIRMAND ) )
                {
                    reaffirm_attached_particles( character );
                }
            }
        }

        // do not do direct damage to items that are being held
        if ( ACTIVE_CHR( pchr->attachedto ) ) continue;

        // don't do direct damage to invulnerable objects
        if ( pchr->invictus ) continue;

		//@todo: sound of lava sizzling and such
        //distance = ABS( PCamera->track_pos.x - pchr->pos.x ) + ABS( PCamera->track_pos.y - pchr->pos.y );

        //if ( distance < damagetile.min_distance )
        //{
        //    damagetile.min_distance = distance;
        //}

        //if ( distance < damagetile.min_distance + 256 )
        //{
        //    damagetile.sound_time = 0;
        //}

        if ( 0 == pchr->damagetime )
        {
		    int actual_damage;
            actual_damage = damage_character( character, ATK_BEHIND, damagetile.amount, damagetile.type, TEAM_DAMAGE, MAX_CHR, DAMFX_NBLOC | DAMFX_ARMO, bfalse );
            pchr->damagetime = DAMAGETILETIME;
			
			if (( actual_damage > 0 ) && ( -1 != damagetile.parttype ) && 0 == ( update_wld & damagetile.partand ) )
			{
				spawn_one_particle_global( pchr->pos, ATK_FRONT, damagetile.parttype, 0);
			}
        }
	}
};

//--------------------------------------------------------------------------------------------
Uint16 terp_dir( Uint16 majordir, Uint16 minordir )
{
    /// @details ZZ@> This function returns a direction between the major and minor ones, closer
    ///    to the major.

    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;

    if ( minordir > 0x8000 )
    {
        temp = 0xFFFF;
    }
    else
    {
        temp = 0;
    }

    minordir  = ( minordir + ( temp << 3 ) - temp ) >> 3;
    minordir += majordir;

    return minordir;
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir )
{
    /// @details ZZ@> This function returns a direction between the major and minor ones, closer
    ///    to the major, but not by much.  Makes turning faster.

    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;

    if ( minordir > 0x8000 )
    {
        temp = 0xFFFF;
    }
    else
    {
        temp = 0;
    }

    minordir = ( minordir + ( temp << 1 ) - temp ) >> 1;
    minordir += majordir;

    return minordir;
}

//--------------------------------------------------------------------------------------------
void update_pits()
{
    /// @details ZZ@> This function kills any character in a deep pit...

    Uint16 cnt;

    if ( pits.kill || pits.teleport )
    {
        //Decrease the timer
        if ( clock_pit > 0 ) clock_pit--;

        if ( clock_pit == 0 )
        {
            //Reset timer
            clock_pit = 20;

            // Kill any particles that fell in a pit, if they die in water...
            for ( cnt = 0; cnt < maxparticles; cnt++ )
            {
                if ( !ACTIVE_PRT( cnt ) || !LOADED_PIP( PrtList.lst[cnt].pip_ref ) ) continue;

                if ( PrtList.lst[cnt].pos.z < PITDEPTH && prt_get_ppip( cnt )->endwater )
                {
                    prt_request_terminate( cnt );
                }
            }

            // Kill or teleport any characters that fell in a pit...
            for ( cnt = 0; cnt < MAX_CHR; cnt++ )
            {
                // Is it a valid character?
                if ( !ACTIVE_CHR( cnt ) || ChrList.lst[cnt].invictus || !ChrList.lst[cnt].alive ) continue;
                if ( ACTIVE_CHR( ChrList.lst[cnt].attachedto ) || ChrList.lst[cnt].pack_ispacked ) continue;

                // Do we kill it?
                if ( pits.kill && ChrList.lst[cnt].pos.z < PITDEPTH )
                {
                    // Got one!
                    kill_character( cnt, MAX_CHR, bfalse );
                    ChrList.lst[cnt].vel.x = 0;
                    ChrList.lst[cnt].vel.y = 0;

                    //ZF> Disabled, the pitfall sound was intended for pits.teleport only
                    // Play sound effect
                    // sound_play_chunk( ChrList.lst[cnt].pos, g_wavelist[GSND_PITFALL] );
                }

                // Do we teleport it?
                if ( pits.teleport && ChrList.lst[cnt].pos.z < PITDEPTH << 3 )
                {
                    bool_t teleported;

                    // Teleport them back to a "safe" spot
                    teleported = chr_teleport( cnt, pits.teleport_pos.x, pits.teleport_pos.y, pits.teleport_pos.z, ChrList.lst[cnt].turn_z );

                    if ( !teleported )
                    {
                        // Kill it instead
                        kill_character( cnt, MAX_CHR, bfalse );
                    }
                    else
                    {
                        // Stop movement
                        ChrList.lst[cnt].vel.z = 0;
                        ChrList.lst[cnt].vel.x = 0;
                        ChrList.lst[cnt].vel.y = 0;

                        // Play sound effect
                        if ( ChrList.lst[cnt].isplayer )
                        {
                            sound_play_chunk( PCamera->track_pos, g_wavelist[GSND_PITFALL] );
                        }
                        else
                        {
                            sound_play_chunk( ChrList.lst[cnt].pos, g_wavelist[GSND_PITFALL] );
                        }

                        // Do some damage (same as damage tile)
                        damage_character( cnt, ATK_BEHIND, damagetile.amount, damagetile.type, TEAM_DAMAGE, chr_get_pai( cnt )->bumplast, DAMFX_NBLOC | DAMFX_ARMO, btrue );
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn_particles()
{
    /// @details ZZ@> This function drops snowflakes or rain or whatever, also swings the camera

    int particle, cnt;
    bool_t foundone;

    if ( weather.time > 0 )
    {
        weather.time--;
        if ( weather.time == 0 )
        {
            weather.time = weather.timer_reset;

            // Find a valid player
            foundone = bfalse;
            for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
            {
                weather.iplayer = ( weather.iplayer + 1 ) & ( MAXPLAYER - 1 );
                if ( PlaList[weather.iplayer].valid )
                {
                    foundone = btrue;
                    break;
                }
            }

            // Did we find one?
            if ( foundone )
            {
                // Yes, but is the character valid?
                cnt = PlaList[weather.iplayer].index;
                if ( ACTIVE_CHR( cnt ) && !ChrList.lst[cnt].pack_ispacked )
                {
                    // Yes, so spawn over that character
					particle = spawn_one_particle_global( ChrList.lst[cnt].pos, ATK_FRONT, weather.particle, 0 );
                    if ( ACTIVE_PRT( particle ) )
                    {
                        prt_t * pprt = PrtList.lst + particle;

                        bool_t destroy_particle = bfalse;

                        if ( __prthitawall( pprt, NULL, NULL ) )
                        {
                            destroy_particle = btrue;
                        }
                        else if ( weather.over_water && !prt_is_over_water( particle ) )
                        {
                            destroy_particle = btrue;
                        }
                        else
                        {
                            //Weather particles spawned at the edge of the map look ugly, so don't spawn them there
                            if ( pprt->pos.x < EDGE || pprt->pos.x > PMesh->gmem.edge_x - EDGE )
                            {
                                destroy_particle = btrue;
                            }
                            if ( pprt->pos.y < EDGE || pprt->pos.y > PMesh->gmem.edge_y - EDGE )
                            {
                                destroy_particle = btrue;
                            }
                        }

                        if ( destroy_particle )
                        {
                            PrtList_free_one( particle );
                        };
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
    /// @details ZZ@> This function converts input readings to latch settings, so players can
    ///    move around

    float newx, newy;
    Uint16 turnsin;
    float dist, scale;
    float inputx, inputy;
    float fsin, fcos;
    latch_t sum;

    chr_t          * pchr;
    player_t       * ppla;
    input_device_t * pdevice;

    if ( INVALID_PLA( player ) ) return;
    ppla = PlaList + player;

    pdevice = &( ppla->device );

    if ( !ACTIVE_CHR( ppla->index ) ) return;
    pchr = ChrList.lst + ppla->index;

    // is the device a local device or an internet device?
    if ( pdevice->bits == INPUT_BITS_NONE ) return;

    // Clear the player's latch buffers
    latch_init( &( sum ) );

    // generate the transforms relative to the camera
    turnsin = PCamera->turn_z >> 2;
    fsin    = turntosin[turnsin & TRIG_TABLE_MASK ];
    fcos    = turntocos[turnsin & TRIG_TABLE_MASK ];

    // Mouse routines
    if ( HAS_SOME_BITS( pdevice->bits , INPUT_BITS_MOUSE ) && mous.on )
    {
        // Movement
        newx = 0;
        newy = 0;
        if (( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
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

                if ( PCamera->turn_mode == CAMTURN_GOOD &&
                     local_numlpla == 1 &&
                     control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) == 0 )  inputx = 0;

                newx = ( inputx * fcos + inputy * fsin );
                newy = ( -inputx * fsin + inputy * fcos );
            }
        }

        sum.x += newx;
        sum.y += newy;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_JUMP ) )
            sum.b |= LATCHBUTTON_JUMP;
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_USE ) )
            sum.b |= LATCHBUTTON_LEFT;
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_GET ) )
            sum.b |= LATCHBUTTON_ALTLEFT;
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_PACK ) )
            sum.b |= LATCHBUTTON_PACKLEFT;
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_USE ) )
            sum.b |= LATCHBUTTON_RIGHT;
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_GET ) )
            sum.b |= LATCHBUTTON_ALTRIGHT;
        if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_PACK ) )
            sum.b |= LATCHBUTTON_PACKRIGHT;
    }

    // Joystick A routines
    if ( HAS_SOME_BITS( pdevice->bits , INPUT_BITS_JOYA ) && joy[0].on )
    {
        newx = 0;
        newy = 0;
        // Movement
        if (( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
            !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
        {
            inputx = joy[0].x;
            inputy = joy[0].y;

            dist = inputx * inputx + inputy * inputy;
            if ( dist > 1.0f )
            {
                scale = 1.0f / SQRT( dist );
                inputx *= scale;
                inputy *= scale;
            }

            if ( PCamera->turn_mode == CAMTURN_GOOD &&
                 local_numlpla == 1 &&
                 !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )  inputx = 0;

            newx = ( inputx * fcos + inputy * fsin );
            newy = ( -inputx * fsin + inputy * fcos );
        }

        sum.x += newx;
        sum.y += newy;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_JUMP ) )
            sum.b |= LATCHBUTTON_JUMP;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_USE ) )
            sum.b |= LATCHBUTTON_LEFT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_GET ) )
            sum.b |= LATCHBUTTON_ALTLEFT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_PACK ) )
            sum.b |= LATCHBUTTON_PACKLEFT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_USE ) )
            sum.b |= LATCHBUTTON_RIGHT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_GET ) )
            sum.b |= LATCHBUTTON_ALTRIGHT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_PACK ) )
            sum.b |= LATCHBUTTON_PACKRIGHT;
    }

    // Joystick B routines
    if ( HAS_SOME_BITS( pdevice->bits , INPUT_BITS_JOYB ) && joy[1].on )
    {
        newx = 0;
        newy = 0;

        // Movement
        if (( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 ) ||
            !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
        {
            inputx = joy[1].x;
            inputy = joy[1].y;

            dist = inputx * inputx + inputy * inputy;
            if ( dist > 1.0f )
            {
                scale = 1.0f / SQRT( dist );
                inputx *= scale;
                inputy *= scale;
            }

            if ( PCamera->turn_mode == CAMTURN_GOOD &&
                 local_numlpla == 1 &&
                 !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )  inputx = 0;

            newx = ( inputx * fcos + inputy * fsin );
            newy = ( -inputx * fsin + inputy * fcos );
        }

        sum.x += newx;
        sum.y += newy;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_JUMP ) )
            sum.b |= LATCHBUTTON_JUMP;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_USE ) )
            sum.b |= LATCHBUTTON_LEFT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_GET ) )
            sum.b |= LATCHBUTTON_ALTLEFT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_PACK ) )
            sum.b |= LATCHBUTTON_PACKLEFT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_USE ) )
            sum.b |= LATCHBUTTON_RIGHT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_GET ) )
            sum.b |= LATCHBUTTON_ALTRIGHT;
        if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_PACK ) )
            sum.b |= LATCHBUTTON_PACKRIGHT;
    }

    // Keyboard routines
    if ( HAS_SOME_BITS( pdevice->bits , INPUT_BITS_KEYBOARD ) && keyb.on )
    {
        // Movement

        // ???? is this if statement doing anything ????
        if ( ACTIVE_CHR( pchr->attachedto ) )
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

        if ( PCamera->turn_mode == CAMTURN_GOOD && local_numlpla == 1 )  inputx = 0;

        newx = ( inputx * fcos + inputy * fsin );
        newy = ( -inputx * fsin + inputy * fcos );

        sum.x += newx;
        sum.y += newy;

        // Read buttons
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_JUMP ) )
            sum.b |= LATCHBUTTON_JUMP;
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_USE ) )
            sum.b |= LATCHBUTTON_LEFT;
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_GET ) )
            sum.b |= LATCHBUTTON_ALTLEFT;
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_PACK ) )
            sum.b |= LATCHBUTTON_PACKLEFT;
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_USE ) )
            sum.b |= LATCHBUTTON_RIGHT;
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_GET ) )
            sum.b |= LATCHBUTTON_ALTRIGHT;
        if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_PACK ) )
            sum.b |= LATCHBUTTON_PACKRIGHT;
    }

    input_device_add_latch( pdevice, sum.x, sum.y );

    ppla->local_latch.x = pdevice->latch.x;
    ppla->local_latch.y = pdevice->latch.y;
    ppla->local_latch.b = sum.b;
}

//--------------------------------------------------------------------------------------------
void set_local_latches( void )
{
    /// @details ZZ@> This function emulates AI thinkin' by setting latches from input devices

    int cnt;

    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        set_one_player_latch( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void check_stats()
{
    /// @details ZZ@> This function lets the players check character stats

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

    // XP CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_x ) )
    {
        Uint16 docheat = MAX_CHR;
        if ( SDLKEYDOWN( SDLK_1 ) )  docheat = 0;
        else if ( SDLKEYDOWN( SDLK_2 ) )  docheat = 1;
        else if ( SDLKEYDOWN( SDLK_3 ) )  docheat = 2;
        else if ( SDLKEYDOWN( SDLK_4 ) )  docheat = 3;

        //Apply the cheat if valid
        if ( ACTIVE_CHR( PlaList[docheat].index ) )
        {
            Uint32 xpgain;
            cap_t * pcap;
            chr_t * pchr = ChrList.lst + PlaList[docheat].index;
            pcap = pro_get_pcap( pchr->iprofile );

            //Give 10% of XP needed for next level
            xpgain = 0.1f * ( pcap->experience_forlevel[MIN( pchr->experiencelevel+1, MAXLEVEL )] - pcap->experience_forlevel[pchr->experiencelevel] );
            give_experience( pchr->ai.index, xpgain, XP_DIRECT, btrue );
            stat_check_delay = 1;
        }
    }

    // LIFE CHEAT
    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_z ) )
    {
        Uint16 docheat = MAX_CHR;
        if ( SDLKEYDOWN( SDLK_1 ) )  docheat = 0;
        else if ( SDLKEYDOWN( SDLK_2 ) )  docheat = 1;
        else if ( SDLKEYDOWN( SDLK_3 ) )  docheat = 2;
        else if ( SDLKEYDOWN( SDLK_4 ) )  docheat = 3;

        //Apply the cheat if valid
        if ( ACTIVE_CHR( PlaList[docheat].index ) )
        {
            cap_t * pcap;
            chr_t * pchr = ChrList.lst + PlaList[docheat].index;
            pcap = pro_get_pcap( pchr->iprofile );

            //Heal 1 life
            heal_character( pchr->ai.index, pchr->ai.index, 256, btrue );
            stat_check_delay = 1;
        }
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
    else if ( SDLKEYDOWN( SDLK_LCTRL ) )
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
    /// @details ZZ@> This function shows the more specific stats for a character

    int character, level;
    char gender[8] = EMPTY_CSTR;

    if ( statindex < StatusList_count )
    {
        character = StatusList[statindex];

        if ( ACTIVE_CHR( character ) )
        {
            cap_t * pcap;
            chr_t * pchr = ChrList.lst + character;

            pcap = pro_get_pcap( pchr->iprofile );

            // Name
            debug_printf( "=%s=", chr_get_name( GET_INDEX_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_CAPITAL ) );

            // Level and gender and class
            gender[0] = 0;
            if ( pchr->alive )
            {
                int itmp;
                char * gender_str;

                gender_str = "";
                switch ( pchr->gender )
                {
                    case GENDER_MALE: gender_str = "male "; break;
                    case GENDER_FEMALE: gender_str = "female "; break;
                }

                level = 1 + pchr->experiencelevel;
                itmp = level % 10;
                if ( 1 == itmp )
                {
                    debug_printf( "~%dst level %s%s", level, gender_str, pcap->classname );
                }
                else if ( 2 == itmp )
                {
                    debug_printf( "~%dnd level %s%s", level, gender_str, pcap->classname );
                }
                else if ( 3 == itmp )
                {
                    debug_printf( "~%drd level %s%s", level, gender_str, pcap->classname );
                }
                else
                {
                    debug_printf( "~%dth level %s%s", level, gender_str, pcap->classname );
                }
            }
            else
            {
                debug_printf( "~Dead %s", pcap->classname );
            }

            // Stats
            debug_printf( "~STR:~%2d~WIS:~%2d~DEF:~%d", FP8_TO_INT( pchr->strength ), FP8_TO_INT( pchr->wisdom ), 255 - pchr->defense );
            debug_printf( "~INT:~%2d~DEX:~%2d~EXP:~%d", FP8_TO_INT( pchr->intelligence ), FP8_TO_INT( pchr->dexterity ), pchr->experience );
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( Uint16 statindex )
{
    /// @details ZF@> This function shows detailed armor information for the character

    STRING tmps;
    Uint16 ichr;

    Uint8  skinlevel;

    cap_t * pcap;
    chr_t * pchr;

    if ( statindex >= StatusList_count ) return;

    ichr = StatusList[statindex];
    if ( !ACTIVE_CHR( ichr ) ) return;

    pchr = ChrList.lst + ichr;
    skinlevel = pchr->skin;

    pcap = chr_get_pcap( ichr );
    if ( NULL == pcap ) return;

    // Armor Name
    debug_printf( "=%s=", pcap->skinname[skinlevel] );

    // Armor Stats
    debug_printf( "~DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d", 255 - pcap->defense[skinlevel],
                  pcap->damagemodifier[DAMAGE_SLASH][skinlevel]&DAMAGESHIFT,
                  pcap->damagemodifier[DAMAGE_CRUSH][skinlevel]&DAMAGESHIFT,
                  pcap->damagemodifier[DAMAGE_POKE ][skinlevel]&DAMAGESHIFT );

    debug_printf( "~HOLY:~%i~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP:~%i",
                  pcap->damagemodifier[DAMAGE_HOLY][skinlevel]&DAMAGESHIFT,
                  pcap->damagemodifier[DAMAGE_EVIL][skinlevel]&DAMAGESHIFT,
                  pcap->damagemodifier[DAMAGE_FIRE][skinlevel]&DAMAGESHIFT,
                  pcap->damagemodifier[DAMAGE_ICE ][skinlevel]&DAMAGESHIFT,
                  pcap->damagemodifier[DAMAGE_ZAP ][skinlevel]&DAMAGESHIFT );

    debug_printf( "~Type: %s", ( pcap->skindressy & ( 1 << skinlevel ) ) ? "Light Armor" : "Heavy Armor" );

    // jumps
    tmps[0] = CSTR_END;
    switch ( pcap->jumpnumber )
    {
        case 0:  snprintf( tmps, SDL_arraysize( tmps ), "None    (%i)", pchr->jumpnumberreset ); break;
        case 1:  snprintf( tmps, SDL_arraysize( tmps ), "Novice  (%i)", pchr->jumpnumberreset ); break;
        case 2:  snprintf( tmps, SDL_arraysize( tmps ), "Skilled (%i)", pchr->jumpnumberreset ); break;
        case 3:  snprintf( tmps, SDL_arraysize( tmps ), "Adept   (%i)", pchr->jumpnumberreset ); break;
        default: snprintf( tmps, SDL_arraysize( tmps ), "Master  (%i)", pchr->jumpnumberreset ); break;
    };

    debug_printf( "~Speed:~%3.0f~Jump Skill:~%s", pchr->maxaccel*80, tmps );
}

//--------------------------------------------------------------------------------------------
bool_t get_chr_regeneration( chr_t * pchr, int * pliferegen, int * pmanaregen )
{
    /// @details ZF@> Get a character's life and mana regeneration, considering all sources

    int local_liferegen, local_manaregen;
    Uint16 ichr, enchant;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    ichr = GET_INDEX_PCHR( pchr );

    if ( NULL == pliferegen ) pliferegen = &local_liferegen;
    if ( NULL == pmanaregen ) pmanaregen = &local_manaregen;

    // set the base values
    ( *pmanaregen ) = pchr->manareturn / MANARETURNSHIFT;
    ( *pliferegen ) = pchr->life_return;

    // Don't forget to add gains and costs from enchants
    for ( enchant = 0; enchant < MAX_ENC; enchant++ )
    {
        enc_t * penc;

        if ( !ACTIVE_ENC( enchant ) ) continue;
        penc = EncList.lst + enchant;

        if ( penc->target_ref == ichr )
        {
            ( *pliferegen ) += penc->targetlife;
            ( *pmanaregen ) += penc->targetmana;
        }

        if ( penc->owner_ref == ichr )
        {
            ( *pliferegen ) += penc->ownerlife;
            ( *pmanaregen ) += penc->ownermana;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void show_full_status( Uint16 statindex )
{
    /// @details ZF@> This function shows detailed armor information for the character including magic

    Uint16 character;
    int manaregen, liferegen;
    chr_t * pchr;

    if ( statindex >= StatusList_count ) return;

    character = StatusList[statindex];
    if ( !ACTIVE_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // clean up the enchant list
    pchr->firstenchant = cleanup_enchant_list( pchr->firstenchant );

    // Enchanted?
    debug_printf( "=%s is %s=", chr_get_name( GET_INDEX_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ), ACTIVE_ENC( pchr->firstenchant ) ? "enchanted" : "unenchanted" );

    // Armor Stats
    debug_printf( "~DEF: %d  SLASH:%3d~CRUSH:%3d POKE:%3d",
                  255 - pchr->defense,
                  pchr->damagemodifier[DAMAGE_SLASH]&DAMAGESHIFT,
                  pchr->damagemodifier[DAMAGE_CRUSH]&DAMAGESHIFT,
                  pchr->damagemodifier[DAMAGE_POKE ]&DAMAGESHIFT );

    debug_printf( "~HOLY: %i~~EVIL:~%i~FIRE:~%i~ICE:~%i~ZAP: ~%i",
                  pchr->damagemodifier[DAMAGE_HOLY]&DAMAGESHIFT,
                  pchr->damagemodifier[DAMAGE_EVIL]&DAMAGESHIFT,
                  pchr->damagemodifier[DAMAGE_FIRE]&DAMAGESHIFT,
                  pchr->damagemodifier[DAMAGE_ICE ]&DAMAGESHIFT,
                  pchr->damagemodifier[DAMAGE_ZAP ]&DAMAGESHIFT );

    get_chr_regeneration( pchr, &liferegen, &manaregen );

    debug_printf( "Mana Regen:~%4.2f Life Regen:~%4.2f", FP8_TO_FLOAT( manaregen ), FP8_TO_FLOAT( liferegen ) );
}

//--------------------------------------------------------------------------------------------
void show_magic_status( Uint16 statindex )
{
    /// @details ZF@> Displays special enchantment effects for the character

    Uint16 character;
    char * missile_str;
    chr_t * pchr;

    if ( statindex >= StatusList_count ) return;

    character = StatusList[statindex];

    if ( !ACTIVE_CHR( character ) ) return;
    pchr = ChrList.lst + character;

    // clean up the enchant list
    pchr->firstenchant = cleanup_enchant_list( pchr->firstenchant );

    // Enchanted?
    debug_printf( "=%s is %s=", chr_get_name( GET_INDEX_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ), ACTIVE_ENC( pchr->firstenchant ) ? "enchanted" : "unenchanted" );

    // Enchantment status
    debug_printf( "~See Invisible: %s~~See Kurses: %s",
                  pchr->see_invisible_level ? "Yes" : "No",
                  pchr->canseekurse ? "Yes" : "No" );

    debug_printf( "~Channel Life: %s~~Waterwalking: %s",
                  pchr->canchannel ? "Yes" : "No",
                  pchr->waterwalk ? "Yes" : "No" );

    switch ( pchr->missiletreatment )
    {
        case MISSILE_REFLECT: missile_str = "Reflect"; break;
        case MISSILE_DEFLECT: missile_str = "Deflect"; break;

        default:
        case MISSILE_NORMAL : missile_str = "None";    break;
    }

    debug_printf( "~Flying: %s~~Missile Protection: %s", ( pchr->flyheight > 0 ) ? "Yes" : "No", missile_str );
}

//-------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    /// @details ZZ@> This function sets all of the character's starting tilt values

    int cnt;
    Uint8 twist;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !ACTIVE_CHR( cnt ) ) continue;

        if ( ChrList.lst[cnt].stickybutt && VALID_TILE( PMesh, ChrList.lst[cnt].onwhichfan ) )
        {
            twist = PMesh->gmem.grid_list[ChrList.lst[cnt].onwhichfan].twist;
            ChrList.lst[cnt].map_turn_y = map_twist_y[twist];
            ChrList.lst[cnt].map_turn_x = map_twist_x[twist];
        }
        else
        {
            ChrList.lst[cnt].map_turn_y = MAP_TURN_OFFSET;
            ChrList.lst[cnt].map_turn_x = MAP_TURN_OFFSET;
        }
    }

}

//--------------------------------------------------------------------------------------------
void import_dir_profiles( const char * dirname )
{
    STRING newloadname;
    STRING filename;
    int cnt;

    if ( NULL == PMod || INVALID_CSTR( dirname ) ) return;

    if ( !PMod->importvalid || 0 == PMod->importamount ) return;

    for ( cnt = 0; cnt < PMod->importamount*MAXIMPORTPERPLAYER; cnt++ )
    {
        // Make sure the object exists...
        snprintf( filename, SDL_arraysize( filename ), "%s" SLASH_STR "temp%04d.obj", dirname, cnt );
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s" SLASH_STR "data.txt", filename );

        if ( vfs_exists( newloadname ) )
        {
            // new player found
            if ( 0 == ( cnt % MAXIMPORTPERPLAYER ) ) import_data.player++;

            // store the slot info
            import_data.slot = cnt;

            // load it
            import_data.slot_lst[cnt] = load_one_profile( filename, MAX_PROFILE );
            import_data.max_slot      = MAX( import_data.max_slot, cnt );
        }
    }

}

//--------------------------------------------------------------------------------------------
void load_all_profiles_import()
{
    int cnt;

    // Clear the import slots...
    for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
    {
        import_data.slot_lst[cnt] = 10000;
    }
    import_data.max_slot = -1;

    // This overwrites existing loaded slots that are loaded globally
    overrideslots = btrue;
    import_data.player = -1;
    import_data.slot   = -100;

    import_dir_profiles( "import" );
    import_dir_profiles( "remote" );
}

//--------------------------------------------------------------------------------------------
void game_load_module_profiles( const char *modname )
{
    /// @details BB@> Search for .obj directories int the module directory and load them

    const char *filehandle;
    STRING newloadname;

    import_data.slot = -100;
    make_newloadname( modname, "objects", newloadname );

    filehandle = vfs_findFirst( newloadname, "obj", VFS_SEARCH_DIR );
    while ( filehandle != NULL )
    {
        load_one_profile( filehandle, MAX_PROFILE );
        filehandle = vfs_findNext();
    }
    vfs_findClose();
}

//--------------------------------------------------------------------------------------------
void game_load_global_profiles()
{
    // load all special objects
    load_one_profile( "basicdat" SLASH_STR "globalobjects" SLASH_STR "book.obj", SPELLBOOK );

    // load the objects from various import directories
    load_all_profiles_import();
}

//--------------------------------------------------------------------------------------------
void game_load_all_profiles( const char *modname )
{
    /// @details ZZ@> This function loads a module's local objects and overrides the global ones already loaded

    // Log all of the script errors
    parseerror = bfalse;

    // clear out any old object definitions
    release_all_pro();

    // load the global objects
    game_load_global_profiles();

    // load the objects from the module's directory
    game_load_module_profiles( modname );
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_apply( Uint16 ichr, spawn_file_info_t *pinfo )
{
    chr_t * pchr, *pparent;

    // trap bad pointers
    if ( NULL == pinfo ) return bfalse;

    if ( !ACTIVE_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;

    pparent = NULL;
    if ( ACTIVE_CHR( pinfo->parent ) ) pparent = ChrList.lst + pinfo->parent;

    pchr->money += pinfo->money;
    if ( pchr->money > MAXMONEY )  pchr->money = MAXMONEY;
    if ( pchr->money < 0 )  pchr->money = 0;

    pchr->ai.content = pinfo->content;
    pchr->ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        inventory_add_item( ichr, pinfo->parent );

        pchr->ai.alert |= ALERTIF_GRABBED;  // Make spellbooks change
        pchr->attachedto = pinfo->parent;  // Make grab work
        let_character_think( ichr );  // Empty the grabbed messages

        pchr->attachedto = MAX_CHR;  // Fix grab

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
        while ( pchr->experiencelevel < pinfo->level && pchr->experience < MAXXP )
        {
            give_experience( ichr, 25, XP_DIRECT, btrue );
            do_level_up( ichr );
        }
    }

    // automatically identify all player starting equipment? I think yes.
    if ( startNewPlayer && NULL != pparent && pparent->isplayer )
    {
        pchr->nameknown = btrue;
    }

    // adjust the price of items that are spawned in a shop

    return btrue;
}

//--------------------------------------------------------------------------------------------
// gcc does not define this function on linux (at least not Ubuntu),
// but it is defined under MinGW, which is yucky.
// I actually had to spend like 45 minutes looking up the compiler flags
// to catch this... good documentation, guys!
#if defined(__GNUC__) && !(defined (__MINGW) || defined(__MINGW32__))
int strlwr( char * str )
{
    if ( NULL == str ) return -1;

    while ( CSTR_END != *str )
    {
        *str = tolower( *str );
        str++;
    }

    return 0;
}
#endif

//--------------------------------------------------------------------------------------------
bool_t activate_spawn_file_load_object( spawn_file_info_t * psp_info )
{
    /// @details BB@> Try to load a global object named int psp_info->spawn_coment into
    ///               slot psp_info->slot

    STRING filename;

    if ( NULL == psp_info || LOADED_PRO( psp_info->slot ) ) return bfalse;

    // trim any excess spaces off the psp_info->spawn_coment
    str_trim( psp_info->spawn_coment );

    if ( NULL == strstr( psp_info->spawn_coment, ".obj" ) )
    {
        strcat( psp_info->spawn_coment, ".obj" );
    }

    strlwr( psp_info->spawn_coment );

    // do the loading
    if ( CSTR_END != psp_info->spawn_coment[0] )
    {
        // we are relying on the firtual mount point "/objects", so use
        // the vfs/PHYSFS file naming conventions
        snprintf( filename, SDL_arraysize( filename ), "objects/%s", psp_info->spawn_coment );

        psp_info->slot = load_one_profile( filename, psp_info->slot );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t activate_spawn_file_spawn( spawn_file_info_t * psp_info )
{
    int tnc;
    int new_object, local_index = 0;
    chr_t * pobject;

    if ( NULL == psp_info || !psp_info->do_spawn ) return bfalse;

    // Spawn the character
    new_object = spawn_one_character( psp_info->pos, psp_info->slot, psp_info->team, psp_info->skin, psp_info->facing, psp_info->pname, MAX_CHR );
    if ( !ACTIVE_CHR( new_object ) ) return bfalse;

    pobject = ChrList.lst + new_object;

    // determine the attachment
    if ( psp_info->attach == ATTACH_NONE )
    {
        // Free character
        psp_info->parent = new_object;
        make_one_character_matrix( new_object );
    }

    chr_setup_apply( new_object, psp_info );

    // Turn on PlaList_count input devices
    if ( psp_info->stat )
    {
        // what we do depends on what kind of module we're loading
        if ( 0 == PMod->importamount && PlaList_count < PMod->playeramount )
        {
            // a single player module

            bool_t player_added;

            player_added = bfalse;
            if ( 0 == local_numlpla )
            {
                // the first player gets everything
                player_added = add_player( new_object, PlaList_count, ( Uint32 )( ~0 ) );
            }
            else
            {
                Uint32 bits;

                // each new player steals an input device from the 1st player
                bits = 1 << local_numlpla;
                for ( tnc = 0; tnc < MAXPLAYER; tnc++ )
                {
                    PlaList[tnc].device.bits &= ~bits;
                }

                player_added = add_player( new_object, PlaList_count, bits );
            }

            if ( startNewPlayer && player_added )
            {
                // !!!! make sure the player is identified !!!!
                pobject->nameknown = btrue;
            }
        }
        else if ( PlaList_count < PMod->importamount && PlaList_count < PMod->playeramount && PlaList_count < local_import_count )
        {
            // A multiplayer module

            bool_t player_added;

            local_index = -1;
            for ( tnc = 0; tnc < local_import_count; tnc++ )
            {
                if ( pobject->iprofile <= import_data.max_slot && pobject->iprofile < MAX_PROFILE )
                {
                    if ( import_data.slot_lst[pobject->iprofile] == local_import_slot[tnc] )
                    {
                        local_index = tnc;
                        break;
                    }
                }
            }

            player_added = bfalse;
            if ( -1 != local_index )
            {
                // It's a local PlaList_count
                player_added = add_player( new_object, PlaList_count, local_import_control[local_index] );
            }
            else
            {
                // It's a remote PlaList_count
                player_added = add_player( new_object, PlaList_count, INPUT_BITS_NONE );
            }

            // if for SOME REASON your player is not identified, give him
            // about a 50% chance to get identified every time you enter a module
            if ( player_added && !pobject->nameknown )
            {
                float frand = rand() / ( float )RAND_MAX;

                if ( frand > 0.5f )
                {
                    pobject->nameknown = btrue;
                }
            }
        }

        // Turn on the stat display
        statlist_add( new_object );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void activate_spawn_file()
{
    /// @details ZZ@> This function sets up character data, loaded from "SPAWN.TXT"

    char             *newloadname;
    vfs_FILE         *fileread;
    spawn_file_info_t sp_info;

    // Turn some back on
    newloadname = "data/spawn.txt";
    fileread = vfs_openRead( newloadname );

    PlaList_count = 0;
    sp_info.parent = MAX_CHR;
    if ( NULL == fileread )
    {
        log_error( "Cannot read file: %s\n", newloadname );
    }
    else
    {
        sp_info.parent = MAX_CHR;
        while ( spawn_file_scan( fileread, &sp_info ) )
        {
            int save_slot = sp_info.slot;

            // check to see if the slot is valid
            if ( -1 == sp_info.slot || sp_info.slot >= MAX_PROFILE )
            {
                log_warning( "Invalid slot %d for \"%s\" in file \"%s\"\n", sp_info.slot, sp_info.spawn_coment, newloadname );
                continue;
            }

            // If nothing is in that slot, try to load it
            if ( !LOADED_PRO( sp_info.slot ) )
            {
                activate_spawn_file_load_object( &sp_info );
            }

            // do we have a valid profile, yet?
            if ( !LOADED_PRO( sp_info.slot ) )
            {
                // no, give a warning if it is useful
                if ( save_slot > PMod->importamount * MAXIMPORTPERPLAYER )
                {
                    log_warning( "The object \"%s\"(slot %d) in file \"%s\" does not exist on this machine\n", sp_info.spawn_coment, save_slot, newloadname );
                }
            }
            else
            {
                // yes, do the spawning if need be
                activate_spawn_file_spawn( &sp_info );
            }
        }

        vfs_close( fileread );
    }

    clear_messages();

    // Make sure local players are displayed first
    statlist_sort();

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

//--------------------------------------------------------------------------------------------
void load_all_global_objects()
{
    /// @details ZF@> This function loads all global objects found in the basicdat folder

    const char *filehandle;

    // Warn the user for any duplicate slots
    overrideslots = bfalse;

    // Search for .obj directories and load them
    filehandle = vfs_findFirst( "basicdat" SLASH_STR "globalobjects", "obj", VFS_SEARCH_DIR );
    while ( VALID_CSTR( filehandle ) )
    {
        load_one_profile( filehandle, MAX_PROFILE );
        filehandle = vfs_findNext();
    }

    vfs_findClose();
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
    chop_data_init( &chop_mem );
    game_reset_players();

    reset_end_text();
    renderlist_reset();
}

//--------------------------------------------------------------------------------------------
void game_load_global_assets()
{
    // load a bunch of assets that are used in the module

    // Load all the global icons
    if ( !load_all_global_icons() )
    {
        log_warning( "Could not load all global icons!\n" );
    }
    load_blips();
    load_bars();
    font_bmp_load( "data/font", "data/font.txt" );
}

//--------------------------------------------------------------------------------------------
void game_load_module_assets( const char *modname )
{
    // load a bunch of assets that are used in the module
    load_global_waves( /* modname */ );
    reset_particles( /* modname */ );
    if ( NULL == read_wawalite( /* modname */ ) )
    {
        log_warning( "wawalite.txt not loaded for %s.\n", modname );
    }
    load_basic_textures( /* modname */ );
    load_map( /* modname */ );

    upload_wawalite();
}

//--------------------------------------------------------------------------------------------
void game_load_all_assets( const char *modname )
{
    game_load_global_assets();

    game_load_module_assets( modname );
}

//--------------------------------------------------------------------------------------------
void game_setup_module( const char *smallname )
{
    /// @details ZZ@> This runst the setup functions for a module

    STRING modname;

    // make sure the object lists are empty
    free_all_objects();

    // generate the module directory
    strncpy( modname, smallname, SDL_arraysize( modname ) );
    str_append_slash( modname, SDL_arraysize( modname ) );

    // ust the information in these files to load the module
    activate_passages_file();        // read and implement the "passage script" passages.txt
    activate_spawn_file();           // read and implement the "spawn script" spawn.txt
    activate_alliance_file();        // set up the non-default team interactions
}

//--------------------------------------------------------------------------------------------
bool_t game_load_module_data( const char *smallname )
{
    /// @details ZZ@> This function loads a module

    STRING modname;

    log_info( "Loading module \"%s\"\n", smallname );

    load_ai_script( "data/script.txt" );

    // generate the module directory
    strncpy( modname, smallname, SDL_arraysize( modname ) );
    str_append_slash( modname, SDL_arraysize( modname ) );

    // load all module assets
    game_load_all_assets( modname );

    // load all module objects
    game_load_all_profiles( modname );

    if ( NULL == mesh_load( modname, PMesh ) )
    {
        // do not cause the program to fail, in case we are using a script function to load a module
        // just return a failure value and log a warning message for debugging purposes
        log_warning( "Uh oh! Problems loading the mesh! (%s)\n", modname );
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( Uint16 character )
{
    /// @details ZZ@> This function makes sure a character has no attached particles

    Uint16 particle;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( !ACTIVE_PRT( particle ) ) continue;

        if ( PrtList.lst[particle].attachedto_ref == character )
        {
            prt_request_terminate( particle );
        }
    }

    if ( ACTIVE_CHR( character ) )
    {
        // Set the alert for disaffirmation ( wet torch )
        ChrList.lst[character].ai.alert |= ALERTIF_DISAFFIRMED;
    }
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( Uint16 character )
{
    /// @details ZZ@> This function returns the number of particles attached to the given character

    Uint16 cnt = 0;
    Uint16 particle;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( ACTIVE_PRT( particle ) && PrtList.lst[particle].attachedto_ref == character )
        {
            cnt++;
        }
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
int reaffirm_attached_particles( Uint16 character )
{
    /// @details ZZ@> This function makes sure a character has all of it's particles

    int number_added, number_attached;
    int amount, attempts;
    Uint16 particle;
    chr_t * pchr;
    cap_t * pcap;

    if ( !ACTIVE_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

    pcap = pro_get_pcap( pchr->iprofile );
    if ( NULL == pcap ) return 0;
    amount = pcap->attachedprt_amount;

    if ( 0 == amount ) return 0;

    number_attached = number_of_attached_particles( character );
    if ( number_attached >= amount ) return 0;

    number_added = 0;
    for ( attempts = 0; attempts < amount && number_attached < amount; attempts++ )
    {
        particle = spawn_one_particle( pchr->pos, 0, pchr->iprofile, pcap->attachedprt_pip, character, GRIP_LAST + number_attached, chr_get_iteam( character ), character, TOTAL_MAX_PRT, number_attached, MAX_CHR );
        if ( ACTIVE_PRT( particle ) )
        {
            place_particle_at_vertex( particle, character, PrtList.lst[particle].vrt_off );
            number_added++;
            number_attached++;
        }
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    pchr->ai.alert |= ALERTIF_REAFFIRMED;

    return number_added;
}

//--------------------------------------------------------------------------------------------
void game_quit_module()
{
    /// @details BB@> all of the de-initialization code after the module actually ends

    // stop the module
    game_module_stop( PMod );

    // get rid of the game/module data
    game_release_module_data();

    // turn off networking
    close_session();

    // reset the "ui" mouse state
    cursor_reset();

    // re-initialize all game/module data
    game_reset_module_data();

    // finish whatever in-game song is playing
    sound_finish_sound();

    // remove "/objects" as a virtual mounting point
    vfs_remove_mount_point( "objects" );
}

//-----------------------------------------------------------------
bool_t game_setup_vfs( const char * modname )
{
    /// @details BB@> set up the virtual mount points for the module's data
    ///               and objects

    STRING tmpDir;

    if ( INVALID_CSTR( modname ) ) return bfalse;

    // set the mount point for the module's objects
    vfs_remove_mount_point( "objects" );
    snprintf( tmpDir, sizeof( tmpDir ), "%s" SLASH_STR "objects", modname );
    vfs_add_mount_point( tmpDir, "objects", 0 );

    // mount all of the default global objects directories
    vfs_add_mount_point( "basicdat/globalobjects/items",            "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/magic",            "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/magic_item" ,      "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/misc",             "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/monsters",         "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/players",          "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/potions",          "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/unique",           "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/weapons",          "objects", 1 );
    vfs_add_mount_point( "basicdat/globalobjects/work_in_progress", "objects", 1 );

    // set the mount point for the module's data
    vfs_remove_mount_point( "data" );
    snprintf( tmpDir, sizeof( tmpDir ), "%s" SLASH_STR "gamedat", modname );
    vfs_add_mount_point( tmpDir, "data", 0 );

    // mount all of the default global data directories
    vfs_add_mount_point( "basicdat",                 "data", 1 );
    vfs_add_mount_point( "basicdat/globalparticles", "data", 1 );

    return btrue;
}

//-----------------------------------------------------------------
bool_t game_begin_module( const char * modname, Uint32 seed )
{
    /// @details BB@> all of the initialization code before the module actually starts

    if ( ~0 == seed ) seed = time( NULL );

    // make sure the old game has been quit
    game_quit_module();

    reset_timers();

    // set up the birtual file system for the module
    if ( !game_setup_vfs( modname ) ) return bfalse;

    // load all the in-game module data
    srand( seed );
    if ( !game_load_module_data( modname ) )
    {
        game_module_stop( PMod );
        return bfalse;
    };

    game_setup_module( modname );

    // make sure the per-module configuration settings are correct
    setup_synch( &cfg );

    // initialize the game objects
    update_all_objects();
    update_used_lists();
    cursor_reset();
    game_module_reset( PMod, seed );
    camera_reset( PCamera, PMesh );
    make_all_character_matrices( update_wld != 0 );
    attach_particles();

    // log debug info for every object loaded into the module
    if ( cfg.dev_mode ) log_madused( "debug" SLASH_STR "slotused.txt" );

    // initialize the network
    net_initialize();
    net_sayHello();

    // start the module
    game_module_start( PMod );

    // initialize the timers as the very last thing
    timeron = bfalse;

    return btrue;
}

//-----------------------------------------------------------------
bool_t game_update_imports()
{
    /// @details BB@> This function saves all the players to the players dir
    ///    and also copies them into the imports dir to prepare for the next module

    bool_t is_local;
    int cnt, tnc, j, character, player;
    STRING srcPlayer, srcDir, destDir;

    // do the normal export to save all the player settings
    if ( PMod->exportvalid )
    {
        export_all_players( btrue );
    }

    // reload all of the available players
    check_player_import( "players", btrue );
    check_player_import( "remote",  bfalse );

    // build the import directory using the player info
    vfs_empty_import_directory();
    vfs_mkdir( "/import" );

    // export all of the players directly from memory straight to the "import" dir
    for ( player = 0, cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        if ( !PlaList[cnt].valid ) continue;

        // Is it alive?
        character = PlaList[cnt].index;
        if ( !ACTIVE_CHR( character ) ) continue;

        is_local = ( INPUT_BITS_NONE != PlaList[cnt].device.bits );

        // find the saved copy of the players that are in memory right now
        for ( tnc = 0; tnc < loadplayer_count; tnc++ )
        {
            if ( 0 == strcmp( loadplayer[tnc].name, ChrList.lst[character].Name ) )
            {
                break;
            }
        }

        if ( tnc == loadplayer_count )
        {
            log_warning( "game_update_imports() - cannot find exported file for \"%s\" (\"%s\") \n", ChrList.lst[character].obj_base._name, str_encode_path( ChrList.lst[character].Name ) ) ;
            continue;
        }

        // grab the controls from the currently loaded players
        // calculate the slot from the current player count
        local_import_control[player] = PlaList[cnt].device.bits;
        local_import_slot[player]    = player * MAXIMPORTPERPLAYER;
        player++;

        // Copy the character to the import directory
        if ( is_local )
        {
            snprintf( srcPlayer, SDL_arraysize( srcPlayer ), "%s", loadplayer[tnc].dir );
        }
        else
        {
            snprintf( srcPlayer, SDL_arraysize( srcPlayer ), "remote" SLASH_STR "%s", str_encode_path( loadplayer[tnc].name ) );
        }

        snprintf( destDir, SDL_arraysize( destDir ), "import" SLASH_STR "temp%04d.obj", local_import_slot[tnc] );
        vfs_copyDirectory( srcPlayer, destDir );

        // Copy all of the character's items to the import directory
        for ( j = 0; j < MAXIMPORTOBJECTS; j++ )
        {
            snprintf( srcDir, SDL_arraysize( srcDir ), "%s" SLASH_STR "%d.obj", srcPlayer, j );
            snprintf( destDir, SDL_arraysize( destDir ), "import" SLASH_STR "temp%04d.obj", local_import_slot[tnc] + j + 1 );

            vfs_copyDirectory( srcDir, destDir );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void game_release_module_data()
{
    /// @details ZZ@> This function frees up memory used by the module

    // Disable EMP
    local_senseenemiesID = IDSZ_NONE;
    local_senseenemiesTeam = TEAM_MAX;

    // make sure that the object lists are cleared out
    free_all_objects();

    // deal with dynamically allocated game assets
    release_all_graphics();
    release_all_profiles();
    release_all_ai_scripts();

    // deal with the mesh
    clear_all_passages();
    mesh_delete( PMesh );
}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
    /// @details ZZ@> This function attaches particles to their characters so everything gets
    ///    drawn right

    int cnt;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_t * pprt;

        if ( !ACTIVE_PRT( cnt ) ) continue;
        pprt = PrtList.lst + cnt;

        if ( ACTIVE_CHR( pprt->attachedto_ref ) )
        {
            place_particle_at_vertex( cnt, pprt->attachedto_ref, pprt->vrt_off );

            // the previous function can inactivate a particle
            if ( ACTIVE_PRT( cnt ) )
            {
                pip_t * ppip = prt_get_ppip( cnt );

                // Correct facing so swords knock characters in the right direction...
                if ( NULL != ppip && ppip->damfx & DAMFX_TURN )
                {
                    pprt->facing = ChrList.lst[pprt->attachedto_ref].turn_z;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t add_player( Uint16 character, Uint16 player, Uint32 device_bits )
{
    /// @details ZZ@> This function adds a player, returning bfalse if it fails, btrue otherwise

    bool_t retval = bfalse;

    if ( VALID_PLA_RANGE( player ) && !PlaList[player].valid )
    {
        player_t * ppla = PlaList + player;

        player_init( ppla );

        ChrList.lst[character].isplayer = btrue;
        ppla->index           = character;
        ppla->valid           = btrue;
        ppla->device.bits     = device_bits;

        if ( device_bits != INPUT_BITS_NONE )
        {
            local_noplayers = bfalse;
            ChrList.lst[character].islocalplayer = btrue;
            local_numlpla++;

            // reset the camera
            camera_reset_target( PCamera, PMesh );
        }

        PlaList_count++;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void let_all_characters_think()
{
    /// @details ZZ@> This function lets every computer controlled character do AI stuff

    int character;
    static Uint32 last_update = ( Uint32 )( ~0 );

    // make sure there is only one script update per game update;
    if ( update_wld == last_update ) return;
    last_update = update_wld;

    numblip = 0;
    for ( character = 0; character < MAX_CHR; character++ )
    {
        chr_t * pchr;
        cap_t * pcap;

        bool_t is_crushed, is_cleanedup, can_think;

        if ( !ACTIVE_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

        pcap = chr_get_pcap( character );
        if ( NULL == pcap ) continue;

        // check for actions that must always be handled
        is_cleanedup = HAS_SOME_BITS( pchr->ai.alert, ALERTIF_CLEANEDUP );
        is_crushed   = HAS_SOME_BITS( pchr->ai.alert, ALERTIF_CRUSHED );

        // let the script run sometimes even if the item is in your backpack
        can_think = !pchr->pack_ispacked || pcap->isequipment;

        // only let dead/destroyed things think if they have beem crushed/cleanedup
        if (( pchr->alive && can_think ) || is_crushed || is_cleanedup )
        {
            // Figure out alerts that weren't already set
            set_alerts( character );

            // Crushed characters shouldn't be alert to anything else
            if ( is_crushed )  { pchr->ai.alert = ALERTIF_CRUSHED; pchr->ai.timer = update_wld + 1; }

            // Cleaned up characters shouldn't be alert to anything else
            if ( is_cleanedup )  { pchr->ai.alert = ALERTIF_CLEANEDUP; pchr->ai.timer = update_wld + 1; }

            let_character_think( character );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t game_begin_menu( menu_process_t * mproc, which_menu_t which )
{
    if ( NULL == mproc ) return bfalse;

    if ( !process_running( PROC_PBASE( mproc ) ) )
    {
        GProc->menu_depth = mnu_get_menu_depth();
    }

    if ( mnu_begin_menu( which ) )
    {
        process_start( PROC_PBASE( mproc ) );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void game_end_menu( menu_process_t * mproc )
{
    mnu_end_menu();

    if ( mnu_get_menu_depth() <= GProc->menu_depth )
    {
        process_resume( PROC_PBASE( MProc ) );
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
    /// @details BB@> free every instance of the three object types used in the game.

    PrtList_free_all();
    EncList_free_all();
    free_all_chraracters();
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
    rotmeshtopside    = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHTOPSIDE / ( 1.33333f );
    rotmeshbottomside = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHBOTTOMSIDE / ( 1.33333f );
    rotmeshup         = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHUP / ( 1.33333f );
    rotmeshdown       = (( float )sdl_scr.x / sdl_scr.y ) * ROTMESHDOWN / ( 1.33333f );

    return pcam_old;
}

//---------------------------------------------------------------------------------------------
float get_mesh_level( ego_mpd_t * pmesh, float x, float y, bool_t waterwalk )
{
    /// @details ZZ@> This function returns the height of a point within a mesh fan, precise
    ///    If waterwalk is nonzero and the fan is watery, then the level returned is the
    ///    level of the water.

    float zdone;

    zdone = mesh_get_level( pmesh, x, y );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = mesh_get_tile( pmesh, x, y );

        if ( 0 != mesh_test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
bool_t make_water( water_instance_t * pinst, wawalite_water_t * pdata )
{
    /// @details ZZ@> This function sets up water movements

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
                temp = SIN(( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( PI / 2 * layer / MAXWATERLAYER ) );
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
        //           GL_DEBUG(glColor4f)(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
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
    /// @details ZZ@> This function resets the end-module text

    endtext_carat = snprintf( endtext, SDL_arraysize( endtext ), "The game has ended..." );

    /*
    if ( PlaList_count > 1 )
    {
        endtext_carat = snprintf( endtext, SDL_arraysize( endtext), "Sadly, they were never heard from again..." );
    }
    else
    {
        if ( PlaList_count == 0 )
        {
            // No players???
            endtext_carat = snprintf( endtext, SDL_arraysize( endtext), "The game has ended..." );
        }
        else
        {
            // One player
            endtext_carat = snprintf( endtext, SDL_arraysize( endtext), "Sadly, no trace was ever found..." );
        }
    }
    */

    str_add_linebreaks( endtext, endtext_carat, 20 );
}

//--------------------------------------------------------------------------------------------
void expand_escape_codes( Uint16 ichr, script_state_t * pstate, char * src, char * src_end, char * dst, char * dst_end )
{
    int    cnt;
    STRING szTmp;

    chr_t      * pchr, *ptarget, *powner;
    ai_state_t * pai;

    pchr    = !ACTIVE_CHR( ichr ) ? NULL : ChrList.lst + ichr;
    pai     = ( NULL == pchr )    ? NULL : &( pchr->ai );

    ptarget = (( NULL == pai ) || !ACTIVE_CHR( pai->target ) ) ? pchr : ChrList.lst + pai->target;
    powner  = (( NULL == pai ) || !ACTIVE_CHR( pai->owner ) ) ? pchr : ChrList.lst + pai->owner;

    cnt = 0;
    while ( CSTR_END != *src && src < src_end && dst < dst_end )
    {
        if ( '%' == *src )
        {
            char * ebuffer, * ebuffer_end;

            // go to the escape character
            src++;

            // set up the buffer to hold the escape data
            ebuffer     = szTmp;
            ebuffer_end = szTmp + SDL_arraysize( szTmp ) - 1;

            // make the excape buffer an empty string
            *ebuffer = CSTR_END;

            switch ( *src )
            {
                case '%' : // the % symbol
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp ), "%%" );
                    }
                    break;

                case 'n' : // Name
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp ), "%s", chr_get_name( ichr, CHRNAME_ARTICLE ) );
                    }
                    break;

                case 'c':  // Class name
                    {
                        if ( NULL != pchr )
                        {
                            ebuffer     = chr_get_pcap( ichr )->classname;
                            ebuffer_end = ebuffer + SDL_arraysize( chr_get_pcap( ichr )->classname );
                        }
                    }
                    break;

                case 't':  // Target name
                    {
                        if ( NULL != pai )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%s", chr_get_name( pai->target, CHRNAME_ARTICLE ) );
                        }
                    }
                    break;

                case 'o':  // Owner name
                    {
                        if ( NULL != pai )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%s", chr_get_name( pai->owner, CHRNAME_ARTICLE ) );
                        }
                    }
                    break;

                case 's':  // Target class name
                    {
                        if ( NULL != ptarget )
                        {
                            ebuffer     = chr_get_pcap( pai->target )->classname;
                            ebuffer_end = ebuffer + SDL_arraysize( chr_get_pcap( pai->target )->classname );
                        }
                    }
                    break;

                case '0':
                case '1':
                case '2':
                case '3': // Target's skin name
                    {
                        if ( NULL != ptarget )
                        {
                            ebuffer = chr_get_pcap( pai->target )->skinname[( *src )-'0'];
                            ebuffer_end = ebuffer + SDL_arraysize( chr_get_pcap( pai->target )->skinname[( *src )-'0'] );
                        }
                    }
                    break;

                case 'a':  // Character's ammo
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->ammoknown )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pchr->ammo );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "?" );
                            }
                        }
                    }
                    break;

                case 'k':  // Kurse state
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->iskursed )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "kursed" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "unkursed" );
                            }
                        }
                    }
                    break;

                case 'p':  // Character's possessive
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "her" );
                            }
                            else if ( pchr->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "his" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "its" );
                            }
                        }
                    }
                    break;

                case 'm':  // Character's gender
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "female " );
                            }
                            else if ( pchr->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "male " );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), " " );
                            }
                        }
                    }
                    break;

                case 'g':  // Target's possessive
                    {
                        if ( NULL != ptarget )
                        {
                            if ( ptarget->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "her" );
                            }
                            else if ( ptarget->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "his" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "its" );
                            }
                        }
                    }
                    break;

                case '#':  // New line (enter)
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp ), "\n" );
                    }
                    break;

                case 'd':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->distance );
                        }
                    }
                    break;

                case 'x':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->x );
                        }
                    }
                    break;

                case 'y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->y );
                        }
                    }
                    break;

                case 'D':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->distance );
                        }
                    }
                    break;

                case 'X':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->x );
                        }
                    }
                    break;

                case 'Y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->y );
                        }
                    }
                    break;

                default:
                    snprintf( szTmp, SDL_arraysize( szTmp ), "%%%c???", ( *src ) );
                    break;
            }

            if ( CSTR_END == *ebuffer )
            {
                ebuffer     = szTmp;
                ebuffer_end = szTmp + SDL_arraysize( szTmp );
                snprintf( szTmp, SDL_arraysize( szTmp ), "%%%c???", ( *src ) );
            }

            // make the line capitalized if necessary
            if ( 0 == cnt && NULL != ebuffer )  *ebuffer = toupper( *ebuffer );

            // Copy the generated text
            while ( CSTR_END != *ebuffer && ebuffer < ebuffer_end && dst < dst_end )
            {
                *dst++ = *ebuffer++;
            }
            *dst = CSTR_END;
        }
        else
        {
            // Copy the letter
            *dst = *src;
            dst++;
        }

        src++;
        cnt++;
    }

    // make sure the destination string is terminated
    if ( dst < dst_end )
    {
        *dst = CSTR_END;
    }
    *dst_end = CSTR_END;
}

//--------------------------------------------------------------------------------------------
bool_t game_choose_module( int imod, int seed )
{
    if ( seed < 0 ) seed = time( NULL );

    if ( NULL == PMod ) PMod = &gmod;

    return game_module_setup( PMod, mnu_ModList_get_base( imod ), mnu_ModList_get_name( imod ), seed );
}

//--------------------------------------------------------------------------------------------
game_process_t * game_process_init( game_process_t * gproc )
{
    if ( NULL == gproc ) return NULL;

    memset( gproc, 0, sizeof( *gproc ) );

    process_init( PROC_PBASE( gproc ) );

    gproc->menu_depth = -1;
    gproc->pause_key_ready = btrue;

    // initialize all the profile variables
    PROFILE_INIT( game_update_loop );
    PROFILE_INIT( game_single_update );
    PROFILE_INIT( gfx_loop );

    PROFILE_INIT( talk_to_remotes );
    PROFILE_INIT( listen_for_packets );
    PROFILE_INIT( check_stats );
    PROFILE_INIT( set_local_latches );
    PROFILE_INIT( check_passage_music );
    PROFILE_INIT( cl_talkToHost );

    return gproc;
}

//---------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/*Uint8 find_target_in_block( int x, int y, float chrx, float chry, Uint16 facing,
Uint8 onlyfriends, Uint8 anyone, Uint8 team,
Uint16 donttarget, Uint16 oldtarget )
{
/// @details ZZ@> This function helps find a target, returning btrue if it found a decent target

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
if ( ChrList.lst[charb].alive && !ChrList.lst[charb].invictus && charb != donttarget && charb != oldtarget )
{
if ( anyone || ( chr_get_iteam(charb) == team && onlyfriends ) || ( TeamList[team].hatesteam[chr_get_iteam(charb)] && enemies ) )
{
distance = ABS( ChrList.lst[charb].pos.x - chrx ) + ABS( ChrList.lst[charb].pos.y - chry );
if ( distance < globestdistance )
{
angle = vec_to_facing( ChrList.lst[charb].pos.x - chrx , ChrList.lst[charb].pos.y - chry );
angle = facing - angle;
if ( angle < globestangle || angle > ( 0x00010000 - globestangle ) )
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
charb = ChrList.lst[charb].bumplist_next;
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
        GL_DEBUG( glColor4f )( 1, 1, 1, 1 );
        if ( fpson )
        {
            y = draw_string( 0, y, "%2.3f FPS, %2.3f UPS", stabilized_fps, stabilized_ups );
            y = draw_string( 0, y, "estimated max FPS %2.3f", est_max_fps );
        }

        y = draw_string( 0, y, "Menu time %f", MProc->base.dtime );
    }
}

//--------------------------------------------------------------------------------------------
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

    if ( NULL == plos ) return bfalse;

    if ( 0 == plos->stopped_by ) return bfalse;

    ix_stt = plos->x0 / TILE_SIZE;
    ix_end = plos->x1 / TILE_SIZE;

    iy_stt = plos->y0 / TILE_SIZE;
    iy_end = plos->y1 / TILE_SIZE;

    Dx = plos->x1 - plos->x0;
    Dy = plos->y1 - plos->y0;

    steep = ( ABS( Dy ) >= ABS( Dx ) );

    // determine which are the big and small values
    if ( steep )
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
    if ( Dbig < 0 )
    {
        dbig = -1;
        Dbig = -Dbig;
        ibig_end--;
    }
    else
    {
        ibig_end++;
    }

    // set up the small loop variables
    dsmall = 1;
    Dsmall = ismall_end - ismall_stt;
    if ( Dsmall < 0 )
    {
        dsmall = -1;
        Dsmall = -Dsmall;
    }

    // pre-compute some common values
    TwoDsmall             = 2 * Dsmall;
    TwoDsmallMinusTwoDbig = TwoDsmall - 2 * Dbig;
    TwoDsmallMinusDbig    = TwoDsmall - Dbig;

    fan_last = INVALID_TILE;
    for ( ibig = ibig_stt, ismall = ismall_stt;  ibig != ibig_end;  ibig += dbig )
    {
        Uint32 fan;

        if ( steep )
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
        fan = mesh_get_tile_int( PMesh, ix, iy );
        if ( INVALID_TILE != fan && fan != fan_last )
        {
            Uint32 collide_fx = mesh_test_fx( PMesh, fan, plos->stopped_by );
            // collide the ray with the mesh

            if ( 0 != collide_fx )
            {
                plos->collide_x  = ix;
                plos->collide_y  = iy;
                plos->collide_fx = collide_fx;

                return btrue;
            }

            fan_last = fan;
        }

        // go to the next step
        if ( TwoDsmallMinusDbig > 0 )
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

    if ( NULL == plos ) return bfalse;

    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( !ACTIVE_CHR( ichr ) ) continue;

        // do line/character intersection
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t do_line_of_sight( line_of_sight_info_t * plos )
{
    bool_t mesh_hit = bfalse, chr_hit = bfalse;
    mesh_hit = collide_ray_with_mesh( plos );

    /*if ( mesh_hit )
    {
        plos->x1 = (plos->collide_x + 0.5f) * TILE_SIZE;
        plos->y1 = (plos->collide_y + 0.5f) * TILE_SIZE;
    }

    chr_hit = collide_ray_with_characters( plos );
    */

    return mesh_hit || chr_hit;
}

//--------------------------------------------------------------------------------------------
void game_reset_players()
{
    /// @details ZZ@> This function clears the player list data

    // Reset the local data stuff
    local_seekurse         = bfalse;
    local_senseenemiesTeam = TEAM_MAX;
    local_seeinvis_level     = bfalse;
    local_allpladead       = bfalse;

    net_reset_players();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t upload_water_layer_data( water_instance_layer_t inst[], wawalite_water_layer_t data[], int layer_count )
{
    int layer;

    if ( NULL == inst || 0 == layer_count ) return bfalse;

    // clear all data
    memset( inst, 0, layer_count * sizeof( *inst ) );

    // set the frame
    for ( layer = 0; layer < layer_count; layer++ )
    {
        inst[layer].frame = generate_randmask( 0 , WATERFRAMEAND );
    }

    if ( NULL != data )
    {
        for ( layer = 0; layer < layer_count; layer++ )
        {
            inst[layer].z         = data[layer].z;
            inst[layer].amp       = data[layer].amp;

            inst[layer].dist.x    = data[layer].dist[XX];
            inst[layer].dist.y    = data[layer].dist[YY];

            inst[layer].light_dir = data[layer].light_dir / 63.0f;
            inst[layer].light_add = data[layer].light_add / 63.0f;

            inst[layer].tx_add.x    = data[layer].tx_add[XX];
            inst[layer].tx_add.y    = data[layer].tx_add[YY];

            inst[layer].alpha       = data[layer].alpha;

            inst[layer].frame_add = data[layer].frame_add;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_water_data( water_instance_t * pinst, wawalite_water_t * pdata )
{
    //int layer;

    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof( *pinst ) );

    if ( NULL != pdata )
    {
        // upload the data

        pinst->surface_level    = pdata->surface_level;
        pinst->douse_level      = pdata->douse_level;

        pinst->is_water         = pdata->is_water;
        pinst->overlay_req      = pdata->overlay_req;
        pinst->background_req   = pdata->background_req;

        pinst->light            = pdata->light;

        pinst->foregroundrepeat = pdata->foregroundrepeat;
        pinst->backgroundrepeat = pdata->backgroundrepeat;

        // upload the layer data
        pinst->layer_count   = pdata->layer_count;
        upload_water_layer_data( pinst->layer, pdata->layer, pdata->layer_count );
    }

    // fix the light in case of self-lit textures
    //if ( pdata->light )
    //{
    //    for ( layer = 0; layer < pinst->layer_count; layer++ )
    //    {
    //        pinst->layer[layer].light_add = 1.0f;  // Some cards don't support light lights...
    //    }
    //}

    make_water( pinst, pdata );

    // Allow slow machines to ignore the fancy stuff
    if ( !cfg.twolayerwater_allowed && pinst->layer_count > 1 )
    {
        int iTmp = pdata->layer[0].light_add;
        iTmp = ( pdata->layer[1].light_add * iTmp * INV_FF ) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        pinst->layer_count        = 1;
        pinst->layer[0].light_add = iTmp * INV_FF;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_weather_data( weather_instance_t * pinst, wawalite_weather_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof( *pinst ) );

    // set a default value
    pinst->timer_reset = 10;

    if ( NULL != pdata )
    {
        // copy the data
        pinst->timer_reset = pdata->timer_reset;
        pinst->over_water  = pdata->over_water;
		pinst->particle = pdata->particle;
    }

    // set the new data
    pinst->time = pinst->timer_reset;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_fog_data( fog_instance_t * pinst, wawalite_fog_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof( *pinst ) );

    pdata->top      = 0;
    pdata->bottom   = -100;
    pinst->on       = cfg.fog_allowed;

    if ( NULL != pdata )
    {
        pinst->on     = pdata->found && pinst->on;
        pinst->top    = pdata->top;
        pinst->bottom = pdata->bottom;

        pinst->red    = pdata->red * 0xFF;
        pinst->grn    = pdata->grn * 0xFF;
        pinst->blu    = pdata->blu * 0xFF;
    }

    pinst->distance = ( pdata->top - pdata->bottom );
    pinst->on       = ( pinst->distance < 1.0f ) && pinst->on;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_damagetile_data( damagetile_instance_t * pinst, wawalite_damagetile_t * pdata )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof( *pinst ) );

    //pinst->sound_time   = TILESOUNDTIME;
    //pinst->min_distance = 9999;

    if ( NULL != pdata )
    {
        pinst->amount.base  = pdata->amount;
        pinst->amount.rand  = 1;
        pinst->type         = pdata->type;

        pinst->parttype     = pdata->parttype;
        pinst->partand      = pdata->partand;
        pinst->sound_index  = CLIP( pdata->sound_index, INVALID_SOUND, MAX_WAVE );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_animtile_data( animtile_instance_t inst[], wawalite_animtile_t * pdata, size_t animtile_count )
{
    Uint32 size;

    if ( NULL == inst || 0 == animtile_count ) return bfalse;

    memset( inst, 0, sizeof( *inst ) );

    for ( size = 0; size < animtile_count; size++ )
    {
        inst[size].frame_and  = ( 1 << ( size + 2 ) ) - 1;
        inst[size].base_and   = ~inst[size].frame_and;
        inst[size].frame_add  = 0;
    }

    if ( NULL != pdata )
    {
        inst[0].update_and = pdata->update_and;
        inst[0].frame_and  = pdata->frame_and;
        inst[0].base_and   = ~inst[0].frame_and;

        for ( size = 1; size < animtile_count; size++ )
        {
            inst[size].update_and = pdata->update_and;
            inst[size].frame_and  = ( inst[size-1].frame_and << 1 ) | 1;
            inst[size].base_and   = ~inst[size].frame_and;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_light_data( wawalite_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    // upload the lighting data
    light_nrm[kX] = pdata->light_x;
    light_nrm[kY] = pdata->light_y;
    light_nrm[kZ] = pdata->light_z;
    light_a = pdata->light_a;

    if ( ABS( light_nrm[kX] ) + ABS( light_nrm[kY] ) + ABS( light_nrm[kZ] ) > 0.0f )
    {
        float fTmp = SQRT( light_nrm[kX] * light_nrm[kX] + light_nrm[kY] * light_nrm[kY] + light_nrm[kZ] * light_nrm[kZ] );

        // get the extra magnitude of the direct light
        if ( gfx.usefaredge )
        {
            // we are outside, do the direct light as sunlight
            light_d = 1.0f;
            light_a = light_a / fTmp;
            light_a = CLIP( light_a, 0.0f, 1.0f );
        }
        else
        {
            // we are inside. take the lighting values at face value.
            //light_d = (1.0f - light_a) * fTmp;
            //light_d = CLIP(light_d, 0.0f, 1.0f);
            light_d = CLIP( fTmp, 0.0f, 1.0f );
        }

        light_nrm[kX] /= fTmp;
        light_nrm[kY] /= fTmp;
        light_nrm[kZ] /= fTmp;
    }

    //make_lighttable( pdata->light_x, pdata->light_y, pdata->light_z, pdata->light_a );
    //make_lighttospek();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_phys_data( wawalite_physics_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    // upload the physics data
    hillslide      = pdata->hillslide;
    slippyfriction = pdata->slippyfriction;
    noslipfriction = pdata->noslipfriction;
    airfriction    = pdata->airfriction;
    waterfriction  = pdata->waterfriction;
    gravity        = pdata->gravity;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_graphics_data( wawalite_graphics_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    // Read extra data
    gfx.exploremode = pdata->exploremode;
    gfx.usefaredge  = pdata->usefaredge;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t upload_camera_data( wawalite_camera_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    PCamera->swing     = pdata->swing;
    PCamera->swingrate = pdata->swingrate;
    PCamera->swingamp  = pdata->swingamp;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void upload_wawalite()
{
    /// @details ZZ@> This function sets up water and lighting for the module

    wawalite_data_t * pdata = &wawalite_data;

    upload_phys_data( &( pdata->phys ) );
    upload_graphics_data( &( pdata->graphics ) );
    upload_light_data( pdata );                         // this statement depends on data from upload_graphics_data()
    upload_camera_data( &( pdata->camera ) );
    upload_fog_data( &fog, &( pdata->fog ) );
    upload_water_data( &water, &( pdata->water ) );
    upload_weather_data( &weather, &( pdata->weather ) );
    upload_damagetile_data( &damagetile, &( pdata->damagetile ) );
    upload_animtile_data( animtile, &( pdata->animtile ), SDL_arraysize( animtile ) );
}

//--------------------------------------------------------------------------------------------
bool_t game_module_setup( game_module_t * pinst, mod_file_t * pdata, const char * loadname, Uint32 seed )
{
    //Prepeares a module to be played
    if ( NULL == pdata ) return bfalse;

    if ( !game_module_init( pinst ) ) return bfalse;

    pinst->importamount   = pdata->importamount;
    pinst->exportvalid    = pdata->allowexport;
    pinst->playeramount   = pdata->maxplayers;
    pinst->importvalid    = ( pinst->importamount > 0 );
    pinst->respawnvalid   = ( bfalse != pdata->respawnvalid );
    pinst->respawnanytime = ( RESPAWN_ANYTIME == pdata->respawnvalid );

    strncpy( pinst->loadname, loadname, SDL_arraysize( pinst->loadname ) );

    pinst->active = bfalse;
    pinst->beat   = bfalse;
    pinst->seed   = seed;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_init( game_module_t * pinst )
{
    if ( NULL == pinst ) return bfalse;

    memset( pinst, 0, sizeof( *pinst ) );

    pinst->seed = ( Uint32 )~0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_reset( game_module_t * pinst, Uint32 seed )
{
    if ( NULL == pinst ) return bfalse;

    pinst->beat        = bfalse;
    //pinst->exportvalid = bfalse;  /// ZF@> we can't disable export here, some modules are supposed to allow export (towns)
    pinst->seed        = seed;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_start( game_module_t * pinst )
{
    /// @details BB@> Let the module go

    if ( NULL == pinst ) return bfalse;

    pinst->active = btrue;

    srand( pinst->seed );
    pinst->randsave = rand();
    randindex = rand() % RANDIE_COUNT;

    PNet->hostactive = btrue; // very important or the input will not work

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t game_module_stop( game_module_t * pinst )
{
    /// @details BB@> stop the module

    if ( NULL == pinst ) return bfalse;

    pinst->active      = bfalse;

    // network stuff
    PNet->hostactive  = bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite( /* const char *modname */ )
{
    int cnt;
    wawalite_data_t * pdata;

    // if( INVALID_CSTR(modname) ) return NULL;

    pdata = read_wawalite_file( "data/wawalite.txt", NULL );
    if ( NULL == pdata ) return NULL;

    memcpy( &wawalite_data, pdata, sizeof( wawalite_data_t ) );

    // limit some values
    wawalite_data.damagetile.sound_index = CLIP( wawalite_data.damagetile.sound_index, INVALID_SOUND, MAX_WAVE );

    for ( cnt = 0; cnt < MAXWATERLAYER; cnt++ )
    {
        wawalite_data.water.layer[cnt].light_dir = CLIP( wawalite_data.water.layer[cnt].light_dir, 0, 63 );
        wawalite_data.water.layer[cnt].light_add = CLIP( wawalite_data.water.layer[cnt].light_add, 0, 63 );
    }

    return &wawalite_data;
}

//--------------------------------------------------------------------------------------------
bool_t write_wawalite( const char *modname, wawalite_data_t * pdata )
{
    /// @details BB@> Prepare and write the wawalite file

    int cnt;
//    STRING filename;

    if ( !VALID_CSTR( modname ) || NULL == pdata ) return bfalse;

    // limit some values
    pdata->damagetile.sound_index = CLIP( pdata->damagetile.sound_index, INVALID_SOUND, MAX_WAVE );

    for ( cnt = 0; cnt < MAXWATERLAYER; cnt++ )
    {
        pdata->water.layer[cnt].light_dir = CLIP( pdata->water.layer[cnt].light_dir, 0, 63 );
        pdata->water.layer[cnt].light_add = CLIP( pdata->water.layer[cnt].light_add, 0, 63 );
    }

    //snprintf( filename, SDL_arraysize(filename), "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "menu.txt", modname );

    return write_wawalite_file( pdata );
}

//--------------------------------------------------------------------------------------------
Uint8 get_local_alpha( int light )
{
    if ( local_seeinvis_level > 0 )
    {
        light = MAX( light, INVISIBLE );
        light *= local_seeinvis_level + 1;
    }

    return CLIP( light, 0, 255 );
}

//--------------------------------------------------------------------------------------------
Uint8 get_local_light( int light )
{
    if ( 0xFF == light ) return light;

    if ( local_seedark_level > 0 )
    {
        light = MAX( light, INVISIBLE );
        light *= local_seedark_level + 1;
    }

    return CLIP( light, 0, 254 );
}

//--------------------------------------------------------------------------------------------
bool_t do_shop_drop( Uint16 idropper, Uint16 iitem )
{
    chr_t * pdropper, * pitem;
    bool_t inshop;

    // ?? lol what ??
    if ( idropper == iitem ) return bfalse;

    if ( !ACTIVE_CHR( iitem ) ) return bfalse;
    pitem = ChrList.lst + iitem;

    if ( !ACTIVE_CHR( idropper ) ) return bfalse;
    pdropper = ChrList.lst + idropper;

    inshop = bfalse;
    if ( pitem->isitem && ShopStack.count > 0 )
    {
        Uint16 iowner;

        int ix = pitem->pos.x / TILE_SIZE;
        int iy = pitem->pos.y / TILE_SIZE;

        // This is a hack that makes spellbooks in shops cost correctly
        if ( pdropper->isshopitem ) pitem->isshopitem = btrue;

        iowner = shop_get_owner( ix, iy );
        if ( ACTIVE_CHR( iowner ) )
        {
            int price;
            chr_t * powner = ChrList.lst + iowner;

            inshop = btrue;

            price = chr_get_price( iitem );

            // Are they are trying to sell junk or quest items?
            if ( 0 == price )
            {
                ai_add_order( &( powner->ai ), ( Uint32 ) price, SHOP_BUY );
            }
            else
            {
                pdropper->money += price;
                pdropper->money  = CLIP( pdropper->money, 0, MAXMONEY );

                powner->money -= price;
                powner->money  = CLIP( powner->money, 0, MAXMONEY );

                ai_add_order( &( powner->ai ), ( Uint32 ) price, SHOP_BUY );
            }
        }
    }

    return inshop;
}

//--------------------------------------------------------------------------------------------
bool_t do_shop_buy( Uint16 ipicker, Uint16 iitem )
{
    bool_t can_grab, can_pay, in_shop;
    int price;

    chr_t * ppicker, * pitem;

    // ?? lol what ??
    if ( ipicker == iitem ) return bfalse;

    if ( !ACTIVE_CHR( iitem ) ) return bfalse;
    pitem = ChrList.lst + iitem;

    if ( !ACTIVE_CHR( ipicker ) ) return bfalse;
    ppicker = ChrList.lst + ipicker;

    can_grab = btrue;
    can_pay  = btrue;
    in_shop  = bfalse;

    if ( pitem->isitem && ShopStack.count > 0 )
    {
        Uint16 iowner;

        int ix = pitem->pos.x / TILE_SIZE;
        int iy = pitem->pos.y / TILE_SIZE;

        iowner = shop_get_owner( ix, iy );
        if ( ACTIVE_CHR( iowner ) )
        {
            chr_t * powner = ChrList.lst + iowner;

            in_shop = btrue;
            price   = chr_get_price( iitem );

            if ( ppicker->money >= price )
            {
                // Okay to sell
                ai_add_order( &( powner->ai ), ( Uint32 ) price, SHOP_SELL );

                ppicker->money -= price;
                ppicker->money  = CLIP( ppicker->money, 0, MAXMONEY );

                powner->money  += price;
                powner->money   = CLIP( powner->money, 0, MAXMONEY );

                can_grab = btrue;
                can_pay  = btrue;
            }
            else
            {
                // Don't allow purchase
                ai_add_order( &( powner->ai ), price, SHOP_NOAFFORD );
                can_grab = bfalse;
                can_pay  = bfalse;
            }
        }
    }

    /// print some feedback messages
    /// @todo: some of these are handled in scripts, so they could be disabled
    /*if( can_grab )
    {
        if( in_shop )
        {
            if( can_pay )
            {
                debug_printf( "%s bought %s", chr_get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name( iitem, CHRNAME_ARTICLE ) );
            }
            else
            {
                debug_printf( "%s can't afford %s", chr_get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name( iitem, CHRNAME_ARTICLE ) );
            }
        }
        else
        {
            debug_printf( "%s picked up %s", chr_get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name( iitem, CHRNAME_ARTICLE ) );
        }
    }*/

    return can_grab;
}

//--------------------------------------------------------------------------------------------
bool_t do_shop_steal( Uint16 ithief, Uint16 iitem )
{
    // Pets can try to steal in addition to invisible characters

    bool_t can_steal;

    chr_t * pthief, * pitem;

    // ?? lol what ??
    if ( ithief == iitem ) return bfalse;

    if ( !ACTIVE_CHR( iitem ) ) return bfalse;
    pitem = ChrList.lst + iitem;

    if ( !ACTIVE_CHR( ithief ) ) return bfalse;
    pthief = ChrList.lst + ithief;

    can_steal = btrue;
    if ( pitem->isitem && ShopStack.count > 0 )
    {
        Uint16 iowner;

        int ix = pitem->pos.x / TILE_SIZE;
        int iy = pitem->pos.y / TILE_SIZE;

        iowner = shop_get_owner( ix, iy );
        if ( ACTIVE_CHR( iowner ) )
        {
            IPair  tmp_rand = {1, 100};
            Uint8  detection;
            chr_t * powner = ChrList.lst + iowner;

            detection = generate_irand_pair( tmp_rand );

            can_steal = btrue;
            if ( chr_can_see_object( iowner, ithief ) || detection <= 5 || ( detection - ( pthief->dexterity >> 7 ) + ( powner->wisdom >> 7 ) ) > 50 )
            {
                ai_add_order( &( powner->ai ), SHOP_STOLEN, SHOP_THEFT );
                powner->ai.target = ithief;
                can_steal = bfalse;
            }
        }
    }

    return can_steal;
}

//--------------------------------------------------------------------------------------------
bool_t do_item_pickup( Uint16 ichr, Uint16 iitem )
{
    bool_t can_grab;
    bool_t is_invis, can_steal, in_shop;
    chr_t * pchr, * pitem;
    int ix, iy;

    // ?? lol what ??
    if ( ichr == iitem ) return bfalse;

    if ( !ACTIVE_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;

    if ( !ACTIVE_CHR( iitem ) ) return bfalse;
    pitem = ChrList.lst + iitem;
    ix = pitem->pos.x / TILE_SIZE;
    iy = pitem->pos.y / TILE_SIZE;

    // assume that there is no shop so that the character can grab anything
    can_grab = btrue;
    in_shop = ACTIVE_CHR( shop_get_owner( ix, iy ) );

    if ( in_shop )
    {
        // check for a stealthy pickup
        is_invis  = !chr_can_see_object( ichr, iitem );

        // pets are automatically stealthy
        can_steal = is_invis || pchr->isitem;

        if ( can_steal )
        {
            can_grab = do_shop_steal( ichr, iitem );

            if ( !can_grab )
            {
                debug_printf( "%s was detected!!", chr_get_name( ichr, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ) );
            }
            else
            {
                debug_printf( "%s stole %s", chr_get_name( ichr, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL ), chr_get_name( iitem, CHRNAME_ARTICLE ) );
            }
        }
        else
        {
            can_grab = do_shop_buy( ichr, iitem );
        }
    }

    return can_grab;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_0( ego_mpd_t * pmesh, int tile_x, int tile_y, bool_t waterwalk )
{
    float zdone = mesh_get_max_vertex_0( pmesh, tile_x, tile_y );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = mesh_get_tile( pmesh, tile_x, tile_y );

        if ( 0 != mesh_test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_1( ego_mpd_t * pmesh, int tile_x, int tile_y, oct_bb_t * pbump, bool_t waterwalk )
{
    float zdone = mesh_get_max_vertex_1( pmesh, tile_x, tile_y, pbump->mins[OCT_X], pbump->mins[OCT_Y], pbump->maxs[OCT_X], pbump->maxs[OCT_Y] );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = mesh_get_tile( pmesh, tile_x, tile_y );

        if ( 0 != mesh_test_fx( pmesh, tile, MPDFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_2( ego_mpd_t * pmesh, chr_t * pchr )
{
    // BB> the object does not overlap a single grid corner. Check the 4 corners of the collision volume

    int corner;
    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    float pos_x[4];
    float pos_y[4];
    float zmax;

    for ( corner = 0; corner < 4; corner++ )
    {
        pos_x[corner] = pchr->pos.x + (( 0 == ix_off[corner] ) ? pchr->chr_chr_cv.mins[OCT_X] : pchr->chr_chr_cv.maxs[OCT_X] );
        pos_y[corner] = pchr->pos.y + (( 0 == iy_off[corner] ) ? pchr->chr_chr_cv.mins[OCT_Y] : pchr->chr_chr_cv.maxs[OCT_Y] );
    }

    zmax = get_mesh_level( pmesh, pos_x[0], pos_y[0], pchr->waterwalk );
    for ( corner = 1; corner < 4; corner++ )
    {
        float fval = get_mesh_level( pmesh, pos_x[corner], pos_y[corner], pchr->waterwalk );
        zmax = MAX( zmax, fval );
    }

    return zmax;
}

//--------------------------------------------------------------------------------------------
float get_chr_level( ego_mpd_t * pmesh, chr_t * pchr )
{
    float zmax;
    int ix, ixmax, ixmin;
    int iy, iymax, iymin;

    int grid_vert_count = 0;
    int grid_vert_x[1024];
    int grid_vert_y[1024];

    oct_bb_t bump;

    if ( NULL == pmesh || !ACTIVE_PCHR( pchr ) ) return 0;

    // certain scenery items like doors and such just need to be able to
    // collide with the mesh. They all have 0 == pchr->bump.size
    if ( 0 == pchr->bump.size )
    {
        return get_mesh_level( pmesh, pchr->pos.x, pchr->pos.y, pchr->waterwalk );
    }

    // otherwise, use the small collision volume to determine which tiles the object overlaps
    // move the collision volume so that it surrounds the object
    bump.mins[OCT_X]  = pchr->chr_chr_cv.mins[OCT_X]  + pchr->pos.x;
    bump.maxs[OCT_X]  = pchr->chr_chr_cv.maxs[OCT_X]  + pchr->pos.x;
    bump.mins[OCT_Y]  = pchr->chr_chr_cv.mins[OCT_Y]  + pchr->pos.y;
    bump.maxs[OCT_Y]  = pchr->chr_chr_cv.maxs[OCT_Y]  + pchr->pos.y;

    // determine the size of this object in tiles
    ixmin = bump.mins[OCT_X] / TILE_SIZE; ixmin = CLIP( ixmin, 0, pmesh->info.tiles_x - 1 );
    ixmax = bump.maxs[OCT_X] / TILE_SIZE; ixmax = CLIP( ixmax, 0, pmesh->info.tiles_x - 1 );

    iymin = bump.mins[OCT_Y] / TILE_SIZE; iymin = CLIP( iymin, 0, pmesh->info.tiles_y - 1 );
    iymax = bump.maxs[OCT_Y] / TILE_SIZE; iymax = CLIP( iymax, 0, pmesh->info.tiles_y - 1 );

    // do the simplest thing if the object is just on one tile
    if ( ixmax == ixmin && iymax == iymin )
    {
        return get_mesh_max_vertex_2( pmesh, pchr );
    }

    // hold off on these calculations in case they are not necessary
    bump.mins[OCT_Z]  = pchr->chr_chr_cv.mins[OCT_Z]  + pchr->pos.z;
    bump.maxs[OCT_Z]  = pchr->chr_chr_cv.maxs[OCT_Z]  + pchr->pos.z;
    bump.mins[OCT_XY] = pchr->chr_chr_cv.mins[OCT_XY] + ( pchr->pos.x + pchr->pos.y );
    bump.maxs[OCT_XY] = pchr->chr_chr_cv.maxs[OCT_XY] + ( pchr->pos.x + pchr->pos.y );
    bump.mins[OCT_YX] = pchr->chr_chr_cv.mins[OCT_YX] + ( -pchr->pos.x + pchr->pos.y );
    bump.maxs[OCT_YX] = pchr->chr_chr_cv.maxs[OCT_YX] + ( -pchr->pos.x + pchr->pos.y );

    // otherwise, make up a list of tiles that the object might overlap
    for ( iy = iymin; iy <= iymax; iy++ )
    {
        float grid_y = iy * TILE_ISIZE;

        for ( ix = ixmin; ix <= ixmax; ix++ )
        {
            float ftmp;
            int   itile;
            float grid_x = ix * TILE_ISIZE;

            ftmp = grid_x + grid_y;
            if ( ftmp < bump.mins[OCT_XY] || ftmp > bump.maxs[OCT_XY] ) continue;

            ftmp = -grid_x + grid_y;
            if ( ftmp < bump.mins[OCT_YX] || ftmp > bump.maxs[OCT_YX] ) continue;

            itile = mesh_get_tile_int( pmesh, ix, iy );
            if ( INVALID_TILE == itile ) continue;

            grid_vert_x[grid_vert_count] = ix;
            grid_vert_y[grid_vert_count] = iy;
            grid_vert_count++;
        }
    }

    // we did not intersect a single tile corner
    // this could happen for, say, a very long, but thin shape that fits between the tiles.
    // the current system would not work for that shape
    if ( 0 == grid_vert_count )
    {
        return get_mesh_max_vertex_2( pmesh, pchr );
    }
    else
    {
        int cnt;
        float fval;

        // scan through the vertices that we know will interact with the object
        zmax = get_mesh_max_vertex_1( pmesh, grid_vert_x[0], grid_vert_y[0], &bump, pchr->waterwalk );
        for ( cnt = 1; cnt < grid_vert_count; cnt ++ )
        {
            fval = get_mesh_max_vertex_1( pmesh, grid_vert_x[cnt], grid_vert_y[cnt], &bump, pchr->waterwalk );
            zmax = MAX( zmax, fval );
        }
    }

    if ( zmax == -1e6 ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
egoboo_rv move_water( water_instance_t * pwater )
{
    /// @details ZZ@> This function animates the water overlays

    int layer;

    if ( NULL == pwater ) return rv_error;

    for ( layer = 0; layer < MAXWATERLAYER; layer++ )
    {
        water_instance_layer_t * player = pwater->layer + layer;

        player->tx.x += player->tx_add.x;
        player->tx.y += player->tx_add.y;

        if ( player->tx.x >  1.0f )  player->tx.x -= 1.0f;
        if ( player->tx.y >  1.0f )  player->tx.y -= 1.0f;
        if ( player->tx.x < -1.0f )  player->tx.x += 1.0f;
        if ( player->tx.y < -1.0f )  player->tx.y += 1.0f;

        player->frame = ( player->frame + player->frame_add ) & WATERFRAMEAND;
    }

    return rv_success;
}

