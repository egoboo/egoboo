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

/// @file game/game.c
/// @brief The code for controlling the game
/// @details

#include "game/game.h"

#include "game/mad.h"
#include "game/player.h"
#include "game/link.h"
#include "game/ui.h"
//#include "game/sound.h"
#include "game/graphic.h"
#include "game/graphic_fan.h"
#include "game/graphic_texture.h"
#include "game/graphic_billboard.h"
#include "game/renderer_2d.h"
#include "game/input.h"
#include "game/menu.h"
#include "game/network_client.h"
#include "game/network_server.h"
#include "game/collision.h"
#include "game/bsp.h"
#include "game/script.h"
#include "game/script_compile.h"
#include "game/egoboo.h"

#include "game/module/PassageHandler.hpp"
#include "game/graphics/CameraSystem.hpp"

#include "game/char.h"
#include "game/particle.h"
#include "game/enchant.h"
#include "game/profile.h"
#include "game/mesh.h"
#include "game/physics.h"

#include "game/ChrList.h"
#include "game/EncList.h"
#include "game/PrtList.h"

//--------------------------------------------------------------------------------------------

static ego_mesh_t         _mesh[2];

static game_process_t    _gproc;
static game_module_t     _gmod;

static egolib_throttle_t     game_throttle = EGOLIB_THROTTLE_INIT;

PROFILE_DECLARE( game_update_loop );
PROFILE_DECLARE( gfx_loop );
PROFILE_DECLARE( game_single_update );

PROFILE_DECLARE( talk_to_remotes );
PROFILE_DECLARE( egonet_listen_for_packets );
PROFILE_DECLARE( check_stats );
PROFILE_DECLARE( set_local_latches );
PROFILE_DECLARE( cl_talkToHost );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool  overrideslots      = false;

// End text
char   endtext[MAXENDTEXT] = EMPTY_CSTR;
size_t endtext_carat = 0;

// Status displays
status_list_t StatusList = STATUS_LIST_INIT;

ego_mesh_t         * PMesh   = _mesh + 0;
game_module_t     * PMod    = &_gmod;
game_process_t    * GProc   = &_gproc;
CameraSystem      _cameraSystem;

pit_info_t pits = PIT_INFO_INIT;

FACING_T glouseangle = 0;                                        // actually still used

animtile_instance_t   animtile[2];
damagetile_instance_t damagetile;
weather_instance_t    weather;
water_instance_t      water;
fog_instance_t        fog;

bool activate_spawn_file_active = false;

import_list_t ImportList  = IMPORT_LIST_INIT;

Sint32          clock_wld        = 0;
Uint32          clock_enc_stat   = 0;
Uint32          clock_chr_stat   = 0;
Uint32          clock_pit        = 0;
Uint32          update_wld       = 0;
Uint32          true_update      = 0;
Uint32          true_frame       = 0;
int             update_lag       = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void game_reset_timers();

// looping - stuff called every loop - not accessible by scripts
static void check_stats();
static void tilt_characters_to_terrain();
static void update_pits();
static int  update_game();
static void game_update_ups();
static void do_damage_tiles();
static void set_local_latches();
static void let_all_characters_think();
static void do_weather_spawn_particles();

// module initialization / deinitialization - not accessible by scripts
static bool game_load_module_data( const char *smallname );
static void   game_release_module_data();
static void   game_load_all_profiles( const char *modname );
static void   game_load_profile_ai();

static void   activate_spawn_file_vfs();
static void   activate_alliance_file_vfs();

static bool chr_setup_apply( const CHR_REF ichr, spawn_file_info_t *pinfo );

static void   game_reset_players();

// Model stuff
static void log_madused_vfs( const char *savename );

// "process" management
static int game_process_do_begin( game_process_t * gproc );
static int game_process_do_running( game_process_t * gproc );
static int game_process_do_leaving( game_process_t * gproc );

// misc
static bool game_begin_menu( menu_process_t * mproc, which_menu_t which );
static void   game_end_menu( menu_process_t * mproc );

static void   do_game_hud();

// place the object lists in the initial state
void reset_all_object_lists();

// implementing wawalite data
static bool upload_light_data( const wawalite_data_t * pdata );
static bool upload_phys_data( const wawalite_physics_t * pdata );
static bool upload_graphics_data( const wawalite_graphics_t * pdata );
static bool upload_camera_data( const wawalite_camera_t * pdata );

// implementing water layer data
bool upload_water_layer_data( water_instance_layer_t inst[], const wawalite_water_layer_t data[], const int layer_count );

// misc
static float get_mesh_max_vertex_0( ego_mesh_t * pmesh, int grid_x, int grid_y, bool waterwalk );
static float get_mesh_max_vertex_1( ego_mesh_t * pmesh, int grid_x, int grid_y, oct_bb_t * pbump, bool waterwalk );
static float get_mesh_max_vertex_2( ego_mesh_t * pmesh, chr_t * pchr );

static bool activate_spawn_file_spawn( spawn_file_info_t * psp_info );
static bool activate_spawn_file_load_object( spawn_file_info_t * psp_info );
static void convert_spawn_file_load_name( spawn_file_info_t * psp_info );

static void game_setup_module( const char *smallname );
static void game_reset_module_data();

static void      game_load_all_assets( const char *modname );
static egolib_rv game_load_global_assets();
static void      game_load_module_assets( const char *modname );

static void load_all_profiles_import();
static void import_dir_profiles_vfs( const char * dirname );
static void game_load_global_profiles();
static void game_load_module_profiles( const char *modname );

static void initialize_all_objects();
static void finalize_all_objects();

static void update_used_lists();
static void update_all_objects();
static void move_all_objects();
static void cleanup_all_objects();
static void bump_all_update_counters();

//--------------------------------------------------------------------------------------------
// Random Things
//--------------------------------------------------------------------------------------------
egolib_rv export_one_character( const CHR_REF character, const CHR_REF owner, int chr_obj_index, bool is_local )
{
    /// @author ZZ
    /// @details This function exports a character

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
    if ( NULL == pobj ) return rv_error;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return rv_error;

    if ( !PMod->exportvalid || ( pcap->isitem && !pcap->cancarrytonextmodule ) )
    {
        return rv_fail;
    }

    // TWINK_BO.OBJ
    snprintf( todirname, SDL_arraysize( todirname ), "%s", str_encode_path( ChrList.lst[owner].Name ) );

    // Is it a character or an item?
    if ( chr_obj_index < 0 )
    {
        // Character directory
        snprintf( todirfullname, SDL_arraysize( todirfullname ), "%s", todirname );
    }
    else
    {
        // Item is a subdirectory of the owner directory...
        snprintf( todirfullname, SDL_arraysize( todirfullname ), "%s/%d.obj", todirname, chr_obj_index );
    }

    // players/twink.obj or players/twink.obj/0.obj
    if ( is_local )
    {
        snprintf( todir, SDL_arraysize( todir ), "/players/%s", todirfullname );
    }
    else
    {
        snprintf( todir, SDL_arraysize( todir ), "/remote/%s", todirfullname );
    }

    // Remove all the original info
    if ( chr_obj_index < 0 )
    {
        vfs_removeDirectoryAndContents( todir, VFS_TRUE );
        if ( !vfs_mkdir( todir ) )
        {
            log_warning( "export_one_character() - cannot create object directory \"%s\"\n", todir );
            return rv_error;
        }
    }

    // modules/advent.mod/objects/advent.obj
    snprintf( fromdir, SDL_arraysize( fromdir ), "%s", pobj->name );

    // Build the DATA.TXT file
    snprintf( tofile, SDL_arraysize( tofile ), "%s/data.txt", todir ); /*DATA.TXT*/
    export_one_character_profile_vfs( tofile, character );

    // Build the SKIN.TXT file
    // this is now handled by the [SKIN] expansion in data.txt
    //snprintf( tofile, SDL_arraysize( tofile ), "%s/skin.txt", todir ); /*SKIN.TXT*/
    //export_one_character_skin_vfs( tofile, character );

    // Build the NAMING.TXT file
    snprintf( tofile, SDL_arraysize( tofile ), "%s/naming.txt", todir ); /*NAMING.TXT*/
    export_one_character_name_vfs( tofile, character );

    // Build the QUEST.TXT file
    snprintf( tofile, SDL_arraysize( tofile ), "%s/quest.txt", todir );
    export_one_character_quest_vfs( tofile, character );

    // copy every file that does not already exist in the todir
    {
        vfs_search_context_t * ctxt;
        const char * searchResult;

        ctxt = vfs_findFirst( fromdir, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
        searchResult = vfs_search_context_get_current( ctxt );

        while ( NULL != ctxt && VALID_CSTR( searchResult ) )
        {
            snprintf( fromfile, SDL_arraysize( fromfile ), "%s/%s", fromdir, searchResult );
            snprintf( tofile, SDL_arraysize( tofile ), "%s/%s", todir, searchResult );

            if ( !vfs_exists( tofile ) )
            {
                vfs_copyFile( fromfile, tofile );
            }

            vfs_findNext( &ctxt );
            searchResult = vfs_search_context_get_current( ctxt );
        }

        vfs_findClose( &ctxt );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv export_all_players( bool require_local )
{
    /// @author ZZ
    /// @details This function saves all the local players in the
    ///    PLAYERS directory

    egolib_rv export_chr_rv;
    egolib_rv retval;
    bool is_local;
    PLA_REF ipla;
    int number;
    CHR_REF character;

    // Don't export if the module isn't running
    if ( !process_running( PROC_PBASE( GProc ) ) ) return rv_fail;

    // Stop if export isnt valid
    if ( !PMod->exportvalid ) return rv_fail;

    // assume the best
    retval = rv_success;

    // Check each player
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF item;
        player_t * ppla;
        chr_t    * pchr;

        if ( !VALID_PLA( ipla ) ) continue;
        ppla = PlaStack_get_ptr( ipla );

        is_local = ( NULL != ppla->pdevice );
        if ( require_local && !is_local ) continue;

        // Is it alive?
        if ( !INGAME_CHR( ppla->index ) ) continue;
        character = ppla->index;
        pchr      = ChrList_get_ptr( character );

        // don't export dead characters
        if ( !pchr->alive ) continue;

        // Export the character
        export_chr_rv = export_one_character( character, character, -1, is_local );
        if ( rv_error == export_chr_rv )
        {
            retval = rv_error;
        }

        // Export the left hand item
        item = pchr->holdingwhich[SLOT_LEFT];
        if ( INGAME_CHR( item ) )
        {
            export_chr_rv = export_one_character( item, character, SLOT_LEFT, is_local );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
        }

        // Export the right hand item
        item = pchr->holdingwhich[SLOT_RIGHT];
        if ( INGAME_CHR( item ) )
        {
            export_chr_rv = export_one_character( item, character, SLOT_RIGHT, is_local );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
        }

        // Export the inventory
        number = 0;
        PACK_BEGIN_LOOP( pchr->inventory, pitem, iitem )
        {
            if ( number >= MAXINVENTORY ) break;

            export_chr_rv = export_one_character( iitem, character, number + SLOT_COUNT, is_local );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
            else if ( rv_success == export_chr_rv )
            {
                number++;
            }
        }
        PACK_END_LOOP();
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void log_madused_vfs( const char *savename )
{
    /// @author ZZ
    /// @details This is a debug function for checking model loads

    vfs_FILE* hFileWrite;
    PRO_REF cnt;

    hFileWrite = vfs_openWrite( savename );
    if ( hFileWrite )
    {
        vfs_printf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        //vfs_printf( hFileWrite, "%d of %d frames used...\n", Md2FrameList_index, MAXFRAME );

        for ( cnt = 0; cnt < MAX_PROFILE; cnt++ )
        {
            if ( LOADED_PRO( cnt ) )
            {
                CAP_REF icap = pro_get_icap( cnt );
                MAD_REF imad = pro_get_imad( cnt );

                vfs_printf( hFileWrite, "%3d %32s %s\n", REF_TO_INT( cnt ), CapStack.lst[icap].classname, MadStack.lst[imad].name );
            }
            else if ( cnt <= 36 )    vfs_printf( hFileWrite, "%3d  %32s.\n", REF_TO_INT( cnt ), "Slot reserved for import players" );
            else                    vfs_printf( hFileWrite, "%3d  %32s.\n", REF_TO_INT( cnt ), "Slot Unused" );
        }

        vfs_close( hFileWrite );
    }
}

//--------------------------------------------------------------------------------------------
void statlist_add( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function adds a status display to the do list

    chr_t * pchr;

    if ( StatusList.count >= MAX_STATUS ) return;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList_get_ptr( character );

    if ( pchr->show_stats ) return;

    StatusList.lst[StatusList.count].who = character;
    pchr->show_stats = true;
    StatusList.count++;
}

//--------------------------------------------------------------------------------------------
void statlist_move_to_top( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function puts the character on top of the StatusList

    int cnt, oldloc;
    status_list_element_t tmp;

    // Find where it is
    oldloc = StatusList.count;

    for ( cnt = 0; cnt < StatusList.count; cnt++ )
    {
        if ( character == StatusList.lst[cnt].who )
        {
            memmove( &tmp, StatusList.lst + cnt, sizeof( status_list_element_t ) );
            oldloc = cnt;
            break;
        }
    }

    // Change position
    if ( oldloc < StatusList.count )
    {
        // Move all the lower ones up
        while ( oldloc > 0 )
        {
            oldloc--;
            memmove( StatusList.lst + oldloc + 1, StatusList.lst + oldloc, sizeof( status_list_element_t ) );
        }

        // Put the character in the top slot
        memmove( StatusList.lst + 0, &tmp, sizeof( status_list_element_t ) );
    }
}

//--------------------------------------------------------------------------------------------
void statlist_sort()
{
    /// @author ZZ
    /// @details This function puts all of the local players on top of the StatusList

    PLA_REF ipla;

    for ( ipla = 0; ipla < PlaStack.count; ipla++ )
    {
        if ( PlaStack.lst[ipla].valid && PlaStack.lst[ipla].pdevice != NULL )
        {
            statlist_move_to_top( PlaStack.lst[ipla].index );
        }
    }
}

//--------------------------------------------------------------------------------------------
egolib_rv chr_set_frame( const CHR_REF character, int req_action, int frame_along, int ilip )
{
    /// @author ZZ
    /// @details This function sets the frame for a character explicitly...  This is used to
    ///    rotate Tank turrets

    chr_t * pchr;
    MAD_REF imad;
    egolib_rv retval;
    int action;

    if ( !INGAME_CHR( character ) ) return rv_error;
    pchr = ChrList_get_ptr( character );

    imad = chr_get_imad( character );
    if ( !LOADED_MAD( imad ) ) return rv_fail;

    // resolve the requested action to a action that is valid for this model (if possible)
    action = mad_get_action_ref( imad, req_action );

    // set the action
    retval = chr_set_action( pchr, action, true, true );
    if ( rv_success == retval )
    {
        // the action is set. now set the frame info.
        // pass along the imad in case the pchr->inst is not using this same mad
        // (corrupted data?)
        retval = ( egolib_rv )chr_instance_set_frame_full( &( pchr->inst ), frame_along, ilip, imad );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void activate_alliance_file_vfs()
{
    /// @author ZZ
    /// @details This function reads the alliance file
    STRING szTemp;
    TEAM_REF teama, teamb;
    vfs_FILE *fileread;

    // Load the file
    fileread = vfs_openRead( "mp_data/alliance.txt" );
    if ( fileread )
    {
        while ( goto_colon_vfs( NULL, fileread, true ) )
        {
            vfs_get_string( fileread, szTemp, SDL_arraysize( szTemp ) );
            teama = ( szTemp[0] - 'A' ) % TEAM_MAX;

            vfs_get_string( fileread, szTemp, SDL_arraysize( szTemp ) );
            teamb = ( szTemp[0] - 'A' ) % TEAM_MAX;
            TeamStack.lst[teama].hatesteam[REF_TO_INT( teamb )] = false;
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
    chr_stoppedby_tests = prt_stoppedby_tests = 0;
    chr_pressure_tests  = prt_pressure_tests  = 0;

    update_all_characters();
    update_all_particles();
    update_all_enchants();
}

//--------------------------------------------------------------------------------------------
void move_all_objects()
{
    mesh_mpdfx_tests = 0;

    move_all_particles();
    move_all_characters();
}

//--------------------------------------------------------------------------------------------
void cleanup_all_objects()
{
    cleanup_all_characters();
    cleanup_all_particles();
    cleanup_all_enchants();
}

//--------------------------------------------------------------------------------------------
void bump_all_update_counters()
{
    bump_all_characters_update_counters();
    //bump_all_particles_update_counters();
    bump_all_enchants_update_counters();
}

//--------------------------------------------------------------------------------------------
void initialize_all_objects()
{
    /// @author BB
    /// @details begin the code for updating in-game objects

    // update all object timers etc.
    update_all_objects();

    // fix the list optimization, in case update_all_objects() turned some objects off.
    update_used_lists();
}

//--------------------------------------------------------------------------------------------
void finalize_all_objects()
{
    /// @author BB
    /// @details end the code for updating in-game objects

    // update the object's update counter for every active object
    bump_all_update_counters();

    // do end-of-life care for all objects
    cleanup_all_objects();
}

//--------------------------------------------------------------------------------------------
void blah_billboard()
{
    const SDL_Color color_blu = {0x7F, 0x7F, 0xFF, 0xFF};
    const GLXvector4f default_tint = { 1.00f, 1.00f, 1.00f, 1.00f };

    bool needs_new;
    Uint32 current_time;

    current_time = egoboo_get_ticks();

    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        if ( INVALID_CHR_REF != ChrList.lst[ichr].attachedto ) continue;

        needs_new = false;

        if ( !VALID_BILLBOARD_RANGE( pchr->ibillboard ) )
        {
            needs_new = true;
        }
        //else
        //{
        //    pbb = BillboardList_get_ptr(pchr->ibillboard);
        //    if( NULL == pbb )
        //    {
        //        needs_new = true;
        //    }
        //    else if ( current_time >= pbb->time )
        //    {
        //        needs_new = true;
        //    }

        //    BillboardList_free_one( pchr->ibillboard );
        //    pchr->ibillboard = BILLBOARD_COUNT;
        //}

        if ( needs_new )
        {
            const char * pname = chr_get_name( ichr, 0, NULL, 0 );

            chr_make_text_billboard( ichr, pname, color_blu, default_tint, 50, bb_opt_fade );

            pname = chr_get_name( ichr, 0, NULL, 0 );
        }
    }
    CHR_END_LOOP()
}

//--------------------------------------------------------------------------------------------
int update_game()
{
    /// @author ZZ
    /// @details This function does several iterations of character movements and such
    ///    to keep the game in sync.

    int tnc, numdead, numalive;
    int update_loop_cnt;
    int max_iterations;
    bool need_updates;
    bool free_running;
    Uint32 lotrue_update;

    PLA_REF ipla;

    // is the update counter free running?
    free_running = GProc->ups_timer.free_running && !egonet_on();

    // Check for all local players being dead
    local_stats.allpladead      = false;
    local_stats.seeinvis_level  = 0.0f;
    local_stats.seekurse_level  = 0.0f;
    local_stats.seedark_level   = 0.0f;
    local_stats.grog_level      = 0.0f;
    local_stats.daze_level      = 0.0f;

    // count the total number of players
    net_count_players();

    numdead = numalive = 0;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        chr_t * pchr;

        if ( !PlaStack.lst[ipla].valid ) continue;

        // fix bad players
        ichr = PlaStack.lst[ipla].index;
        if ( !INGAME_CHR( ichr ) )
        {
            PlaStack.lst[ipla].index = INVALID_CHR_REF;
            PlaStack.lst[ipla].valid = false;
            continue;
        }
        pchr = ChrList_get_ptr( ichr );

        // only interested in local players
        if ( NULL == PlaStack.lst[ipla].pdevice ) continue;

        if ( pchr->alive )
        {
            numalive++;

            local_stats.seeinvis_level += pchr->see_invisible_level;
            local_stats.seekurse_level += pchr->see_kurse_level;
            local_stats.seedark_level  += pchr->darkvision_level;
            local_stats.grog_level     += pchr->grog_timer;
            local_stats.daze_level     += pchr->daze_timer;
        }
        else
        {
            numdead++;
        }
    }

    if ( numalive > 0 )
    {
        local_stats.seeinvis_level /= ( float )numalive;
        local_stats.seekurse_level /= ( float )numalive;
        local_stats.seedark_level  /= ( float )numalive;
        local_stats.grog_level     /= ( float )numalive;
        local_stats.daze_level     /= ( float )numalive;
    }

    // this allows for kurses, which might make negative values to do something reasonable
    local_stats.seeinvis_mag = exp( 0.32f * local_stats.seeinvis_level );
    local_stats.seedark_mag  = exp( 0.32f * local_stats.seedark_level );

    // Did everyone die?
    if ( numdead >= local_stats.player_count )
    {
        local_stats.allpladead = true;
    }

    // check for autorespawn
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        chr_t * pchr;

        if ( !PlaStack.lst[ipla].valid ) continue;

        ichr = PlaStack.lst[ipla].index;
        if ( !INGAME_CHR( ichr ) ) continue;
        pchr = ChrList_get_ptr( ichr );

        if ( !pchr->alive )
        {
            if ( cfg.difficulty < GAME_HARD && local_stats.allpladead && SDL_KEYDOWN( keyb, SDLK_SPACE ) && PMod->respawnvalid && 0 == local_stats.revivetimer )
            {
                respawn_character( ichr );
                pchr->experience *= EXPKEEP;        // Apply xp Penality

                if ( cfg.difficulty > GAME_EASY )
                {
                    pchr->money *= EXPKEEP;        //Apply money loss
                }
            }
        }
    }

    // don't use a value of true_update that could change during the
    // iteration of the inner loop
    game_update_timers();
    lotrue_update = true_update;

    max_iterations = 1;
    need_updates    = false;
    if ( free_running )
    {
        // if the ups_timer is free-running then there is only one update
        // per each unregulated pass through the loop

        need_updates = true;
        max_iterations = 1;
    }
    else if ( single_frame_mode )
    {
        // for single frame mode, only one update per request
        need_updates = true;
        max_iterations = 1;
    }
    else if ( update_wld < lotrue_update )
    {
        // if the ups_timer is throttled, do not exceed the given rate
        need_updates    = true;
        max_iterations = 2 * TARGET_UPS;
    }

    update_lag = 0;
    update_loop_cnt = 0;
    if ( need_updates )
    {
        /// @todo claforte@> Put that back in place once networking is functional (Jan 6th 2001)
        for ( tnc = 0; tnc < max_iterations; tnc++ )
        {
            if ( !free_running && ( update_wld >= lotrue_update ) ) break;

            PROFILE_BEGIN( game_single_update );
            {
                // do important stuff to keep in sync inside this loop

                srand( PMod->randsave );
                PMod->randsave = rand();

                // keep the mpdfx lists up-to-date. No calculation is done unless one
                // of the mpdfx values was changed during the last update
                mpdfx_lists_synch( &( PMesh->fxlists ), &( PMesh->gmem ), false );

                // read the input values
                input_read_all_devices();

                // NETWORK PORT
                PROFILE_BEGIN( egonet_listen_for_packets );
                {
                    egonet_listen_for_packets();
                }
                PROFILE_END2( egonet_listen_for_packets );

                PROFILE_BEGIN( set_local_latches );
                {
                    set_local_latches();
                }
                PROFILE_END2( set_local_latches );

                PROFILE_BEGIN( cl_talkToHost );
                {
                    cl_talkToHost();
                }
                PROFILE_END2( cl_talkToHost );

                PROFILE_BEGIN( talk_to_remotes );
                {
                    // get all player latches from the "remotes"
                    sv_talkToRemotes();
                }
                PROFILE_END2( talk_to_remotes );

                //---- begin the code for updating misc. game stuff
                {
                    BillboardList_update_all();
                    animate_tiles();
                    water_instance_move( &water );
                    looped_update_all_sound();
                    do_damage_tiles();
                    update_pits();
                    do_weather_spawn_particles();
                }
                //---- end the code for updating misc. game stuff

                //---- begin the code object I/O
                {
                    let_all_characters_think();           // sets the non-player latches
                    net_unbuffer_player_latches();            // sets the player latches
                    blah_billboard();
                }
                //---- end the code object I/O

                //---- begin the code for updating in-game objects
                initialize_all_objects();
                {
                    move_all_objects();                   // clears some latches
                    bump_all_objects();                   // do the actual object interaction
                }
                finalize_all_objects();
                //---- end the code for updating in-game objects

                // put the camera movement inside here
                _cameraSystem.updateAll(PMesh);

                // Timers
                clock_wld += UPDATE_SKIP;
                clock_enc_stat++;
                clock_chr_stat++;

                // Reset the respawn timer
                if ( local_stats.revivetimer > 0 )
                {
                    local_stats.revivetimer--;
                }

                update_wld++;
                update_loop_cnt++;

                // calculate the updates per second
                game_ups_loops++;
                game_update_ups();

                // get the time values for the next iteration
                game_update_timers();
            }
            PROFILE_END2( game_single_update );

            // estimate how much time the main loop is taking per second
            est_single_update_time = 0.9F * est_single_update_time + 0.1F * PROFILE_QUERY( game_single_update );
            est_single_ups         = 0.9F * est_single_ups         + 0.1F * ( 1.0F / PROFILE_QUERY( game_single_update ) );
        }
        update_lag = tnc;
    }

    est_update_game_time = 0.9F * est_update_game_time + 0.1F * est_single_update_time * update_loop_cnt;
    est_max_game_ups     = 0.9F * est_max_game_ups     + 0.1F * ( 1.0F / est_update_game_time );

    if ( egonet_on() )
    {
        if ( 0 == numplatimes )
        {
            // The remote ran out of messages, and is now twiddling its thumbs...
            // Make it go slower so it doesn't happen again
            clock_wld += 25;
        }
        if ( numplatimes > 3 && !egonet_get_hostactive() )
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
    /// @author ZZ
    /// @details This function updates the game timers

    int    clock_diff;
    bool free_running = false;
    bool is_paused    = false;

    static bool was_paused = false;

    // is the game/module paused?
    is_paused = false;
    if ( !egonet_on() )
    {
        is_paused  = !process_running( PROC_PBASE( GProc ) ) || GProc->mod_paused;
    }

    // check to make sure that the game is running
    if ( is_paused )
    {
        // for a local game, force the function to ignore the accumulation of time
        // until you re-join the game
        egolib_throttle_update_diff( &game_throttle, 0 );
        was_paused = true;
        return;
    }

    // are the game updates free running?
    free_running = false;
    if ( !egonet_on() )
    {
        free_running = GProc->ups_timer.free_running;
    }

    clock_diff = 0;
    if ( egonet_on() )
    {
        // if the network game is on, there really is no real "pause"
        // so we can always measure the game time from the first clock reading
        egolib_throttle_update( &game_throttle );
    }
    else
    {
        // if the net is not on, the game clock will pause when the local game is paused.
        // if we use the other calculation, the game will freeze while it handles the updates
        // for all the time that the game was paused... not so good

        // calculate the time since the from the last update
        // if the game was paused, assume that only one update time elapsed since the last time through this function
        if ( was_paused || single_frame_mode )
        {
            egolib_throttle_update_diff( &game_throttle, UPDATE_SKIP );
        }
        else
        {
            egolib_throttle_update( &game_throttle );
        }
    }

    // calculate the difference
    clock_diff = game_throttle.time_now - game_throttle.time_lst;

    // return if there is no reason to continue
    if ( !free_running && 0 == clock_diff ) return;

    // update the game_ups clock
    game_ups_clock += clock_diff;

    // Use the number of updates that should have been performed up to this point (true_update)
    // to try to regulate the update speed of the game
    true_update      = game_throttle.time_now / UPDATE_SKIP;

    // get the number of frames that should have happened so far in a similar way
    true_frame      = ( game_throttle.time_now  / TICKS_PER_SEC ) * cfg.framelimit;

    // figure out the update rate
    game_ups_clock += clock_diff;

    // if it got this far and the funciton had been paused, it is time to unpause it
    was_paused = false;
}

//--------------------------------------------------------------------------------------------
void game_update_ups()
{
    /// @author ZZ
    /// @details This function updates the game timers

    // at fold = 0.60f, it will take approximately 9 updates for the
    // weight of the first value to be reduced to 1%
    const float fold = 0.60f;
    const float fnew = 1.0f - fold;

    if ( game_ups_loops > 0 && game_ups_clock > 0 )
    {
        stabilized_game_ups_sum    = fold * stabilized_game_ups_sum    + fnew * ( float ) game_ups_loops / (( float ) game_ups_clock / TICKS_PER_SEC );
        stabilized_game_ups_weight = fold * stabilized_game_ups_weight + fnew;

        // Don't allow the counters to overflow. 0x15555555 is 1/3 of the maximum Sint32 value
        if ( game_ups_loops > 0x15555555 || game_ups_clock  > 0x15555555 )
        {
            game_ups_loops = 0;
            game_ups_clock = 0;
        }
    }

    if ( stabilized_game_ups_weight > 0.5f )
    {
        stabilized_game_ups = stabilized_game_ups_sum / stabilized_game_ups_weight;
    }
}

//--------------------------------------------------------------------------------------------
void game_reset_timers()
{
    /// @author ZZ
    /// @details This function resets the timers...

    // reset the synchronization
    clock_wld = 0;
    outofsync = false;

    // reset the pits
    pits.kill = pits.teleport = false;
    clock_pit = 0;

    // reset some counters
    game_frame_all = 0;
    update_wld = 0;

    // reset some special clocks
    clock_enc_stat = 0;
    clock_chr_stat = 0;

    // reset the game clock
    egolib_throttle_reset( &game_throttle );

    // reset the ups counter(s)
    game_ups_clock        = 0;
    game_ups_loops        = 0;

    stabilized_game_ups        = TARGET_UPS;
    stabilized_game_ups_sum    = STABILIZED_COVER * TARGET_UPS;
    stabilized_game_ups_weight = STABILIZED_COVER;

    // reset the fps counter(s)
    game_fps_clock        = 0;
    game_fps_loops        = 0;

    stabilized_game_fps        = cfg.framelimit;
    stabilized_game_fps_sum    = STABILIZED_COVER * cfg.framelimit;
    stabilized_game_fps_weight = STABILIZED_COVER;

    stabilized_game_ups        = TARGET_UPS;
    stabilized_game_ups_sum    = STABILIZED_COVER * TARGET_UPS;
    stabilized_game_ups_weight = STABILIZED_COVER;
}

//--------------------------------------------------------------------------------------------
int game_do_menu( menu_process_t * mproc )
{
    /// @author BB
    /// @details do menus

    static double loc_frameDuration = 0.0f;

    int menuResult;
    bool need_menu = false;

    if ( process_running( PROC_PBASE( mproc ) ) )
    {
        loc_frameDuration += mproc->base.frameDuration;

        if ( gfx_flip_pages_requested() )
        {
            // someone else (and that means the game) has drawn a frame
            // so we just need to draw the menu over that frame
            need_menu = true;

            // force the menu to be displayed immediately when the game stops
            egolib_timer__reset( &( mproc->gui_timer ), -1, cfg.framelimit );
        }
        else if ( egolib_timer__throttle( &( mproc->gui_timer ), cfg.framelimit ) )
        {
            need_menu = true;
        }
    }

    menuResult = 0;
    if ( need_menu )
    {
        ui_beginFrame( loc_frameDuration );
        {
            menuResult = doMenu( loc_frameDuration );
            draw_mouse_cursor();
            gfx_request_flip_pages();
        }
        ui_endFrame();

        // reset the elapsed frame counter
        loc_frameDuration = 0.0F;
    }

    return menuResult;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int game_process_do_begin( game_process_t * gproc )
{
    BillboardList_init_all();

    gproc->escape_latch = false;

    // initialize math objects
    make_randie();
    make_turntosin();

    // Linking system
    log_info( "Initializing module linking... " );
    if ( link_build_vfs( "mp_data/link.txt", LinkList ) ) log_message( "Success!\n" );
    else log_message( "Failure!\n" );

    // initialize the collision system
    collision_system_begin();

    // intialize the "profile system"
    profile_system_begin();

    // do some graphics initialization
    //make_lightdirectionlookup();
    gfx_system_make_enviro();

    // try to start a new module
    if ( !game_begin_module( pickedmodule_path, ( Uint32 )~0 ) )
    {
        // failure - kill the game process
        process_kill( PROC_PBASE( gproc ) );
        process_resume( PROC_PBASE( MProc ) );
    }

    // set up the cameras *after* game_begin_module() or the player devices will not be initialized
    // and camera_system_begin() will not set up thte correct view
    _cameraSystem.begin(local_stats.player_count);

    // make sure the cameras are centered on something or there will be a graphics error
    _cameraSystem.resetAllTargets(PMesh);

    // Initialize the process
    gproc->base.valid = true;

    // initialize all the profile variables
    PROFILE_RESET( game_update_loop );
    PROFILE_RESET( game_single_update );
    PROFILE_RESET( gfx_loop );

    PROFILE_RESET( talk_to_remotes );
    PROFILE_RESET( egonet_listen_for_packets );
    PROFILE_RESET( check_stats );
    PROFILE_RESET( set_local_latches );
    PROFILE_RESET( cl_talkToHost );

    // reset the ups counter
    game_ups_clock        = 0;
    game_ups_loops        = 0;

    stabilized_game_ups        = TARGET_UPS;
    stabilized_game_ups_sum    = STABILIZED_COVER * TARGET_UPS;
    stabilized_game_ups_weight = STABILIZED_COVER;

    // reset the fps counter
    game_fps_clock        = 0;
    game_fps_loops        = 0;

    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;

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

    obj_BSP_system_begin(getMeshBSP());

    return 1;
}

//--------------------------------------------------------------------------------------------
int game_process_do_running( game_process_t * gproc )
{
    int update_loops = 0;

    bool need_ups_update  = false;
    bool need_fps_update  = false;
    bool ups_free_running = false;
    bool fps_free_running = false;

    if ( !process_validate( PROC_PBASE( gproc ) ) ) return -1;

    gproc->was_active  = gproc->base.valid;

    if ( !process_running( PROC_PBASE( gproc ) ) ) return 0;

    // are the updates free running?
    ups_free_running = gproc->ups_timer.free_running && !egonet_on();

    // update all the timers
    game_update_timers();

    need_ups_update = false;
    if ( single_frame_mode )
    {
        if ( single_update_requested )
        {
            need_ups_update = true;
            single_update_requested = false;
            egolib_timer__reset( &( gproc->ups_timer ), -1, TARGET_UPS );
        }
    }
    else if ( ups_free_running )
    {
        need_ups_update = true;
    }
    else if ( egolib_timer__throttle( &( gproc->ups_timer ), TARGET_UPS ) )
    {
        need_ups_update = true;
    }

    if ( need_ups_update )
    {
        PROFILE_BEGIN( game_update_loop );
        {
            // do the updates
            if ( gproc->mod_paused && !egonet_on() )
            {
                clock_wld = game_throttle.time_now;
            }
            else
            {
                input_device_t * pdevice;
                bool msg_pressed;
                int cnt;

                // check to see if anyone has pressed the message button
                msg_pressed = false;
                for ( cnt = 0; cnt < MAX_LOCAL_PLAYERS; cnt++ )
                {
                    pdevice = InputDevices.lst + cnt;

                    if ( input_device_control_active( pdevice, CONTROL_MESSAGE ) )
                    {
                        msg_pressed = true;
                        break;
                    }
                }

                // start the console mode?
                if ( msg_pressed )
                {
                    // reset the keyboard buffer
                    SDL_EnableKeyRepeat( 20, SDL_DEFAULT_REPEAT_DELAY );
                    keyb.chat_mode = true;
                    keyb.chat_done = false;
                    net_chat.buffer_count = 0;
                    net_chat.buffer[0] = CSTR_END;
                }

                // This is the control loop
                if ( egonet_on() && keyb.chat_done )
                {
                    net_send_message();
                }

                PROFILE_BEGIN( check_stats );
                {
                    check_stats();
                }
                PROFILE_END2( check_stats );

                Passages::checkPassageMusic();

                if ( egonet_get_waitingforclients() )
                {
                    clock_wld = game_throttle.time_now;
                }
                else
                {
                    update_loops = update_game();
                }
            }
        }
        PROFILE_END2( game_update_loop );

        // estimate the main-loop update time is taking per inner-loop iteration
        // do a kludge to average out the effects of functions like check_stats()
        // even when the inner loop does not execute
        if ( update_loops > 0 )
        {
            est_update_time = 0.9F * est_update_time + 0.1F * PROFILE_QUERY( game_update_loop ) / update_loops;
            est_max_ups     = 0.9F * est_max_ups     + 0.1F * ( update_loops / PROFILE_QUERY( game_update_loop ) );
        }
        else
        {
            est_update_time = 0.9F * est_update_time + 0.1F * PROFILE_QUERY( game_update_loop );
            est_max_ups     = 0.9F * est_max_ups     + 0.1F * ( 1.0F / PROFILE_QUERY( game_update_loop ) );
        }

        // just to be complete
        need_ups_update = false;
    }

    // Do the display stuff

    // are the frames free running?
    fps_free_running = GProc->fps_timer.free_running;

    need_fps_update = false;
    if ( single_frame_mode )
    {
        if ( single_frame_requested )
        {
            need_fps_update = true;
            single_frame_requested = false;
            egolib_timer__reset( &( gproc->fps_timer ), -1, cfg.framelimit );
        }
    }
    else if ( fps_free_running )
    {
        need_fps_update = true;
    }
    else if ( egolib_timer__throttle( &( gproc->fps_timer ), cfg.framelimit ) )
    {
        need_fps_update = true;
    }

    // Do the display stuff
    if ( need_fps_update )
    {
        PROFILE_BEGIN( gfx_loop );
        {
            gfx_system_main();

            DisplayMsg_timechange++;
        }
        PROFILE_END2( gfx_loop );

        // estimate how much time the main loop is taking per second
        est_gfx_time = 0.9F * est_gfx_time + 0.1F * PROFILE_QUERY( gfx_loop );
        est_max_gfx  = 0.9F * est_max_gfx  + 0.1F * ( 1.0F / PROFILE_QUERY( gfx_loop ) );

        // estimate how much time the main loop is taking per second
        est_render_time = est_gfx_time * TARGET_FPS;
        est_max_fps  = 0.9F * est_max_fps + 0.1F * ( 1.0F - est_update_time * TARGET_UPS ) / PROFILE_QUERY( gfx_loop );

        // just to be complete
        need_fps_update = false;
    }

    if ( gproc->escape_requested )
    {
        gproc->escape_requested = false;

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

            gproc->escape_latch = true;
            gproc->mod_paused   = true;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
int game_process_do_leaving( game_process_t * gproc )
{
    if ( !process_validate( PROC_PBASE( gproc ) ) ) return -1;

    // get rid of all module data
    game_quit_module();

    // resume the menu
    process_resume( PROC_PBASE( MProc ) );

    // deallocate any dynamically allocated collision memory
    collision_system_end();

    // deallocate any data used by the profile system
    profile_system_end();

    // deallocate the obj_BSP
    obj_BSP_system_end();

    // deallocate any dynamically allocated scripting memory
    scripting_system_end();

    // clean up any remaining models that might have dynamic data
    MadStack_release_all();

    // free the cameras
    _cameraSystem.end();

    // reset the fps counter
    game_fps_clock             = 0;
    game_fps_loops             = 0;

    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;

    PROFILE_FREE( game_update_loop );
    PROFILE_FREE( game_single_update );
    PROFILE_FREE( gfx_loop );

    PROFILE_FREE( talk_to_remotes );
    PROFILE_FREE( egonet_listen_for_packets );
    PROFILE_FREE( check_stats );
    PROFILE_FREE( set_local_latches );
    PROFILE_FREE( cl_talkToHost );

    return 1;
}

//--------------------------------------------------------------------------------------------
int game_process_run( game_process_t * gproc, double frameDuration )
{
    int result = 0, proc_result = 0;

    if ( !process_validate( PROC_PBASE( gproc ) ) ) return -1;
    gproc->base.frameDuration = frameDuration;

    if ( gproc->base.paused ) return 0;

    if ( gproc->base.killme )
    {
        gproc->base.state = proc_leaving;
    }

    switch ( gproc->base.state )
    {
        case proc_begin:
            proc_result = game_process_do_begin( gproc );

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
            proc_result = game_process_do_running( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state = proc_leaving;
            }
            break;

        case proc_leaving:
            proc_result = game_process_do_leaving( gproc );

            if ( 1 == proc_result )
            {
                gproc->base.state  = proc_finish;
                gproc->base.killme = false;
            }
            break;

        case proc_finish:
            process_terminate( PROC_PBASE( gproc ) );
            process_resume( PROC_PBASE( MProc ) );
            break;

        default:
            /* do noithing */
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF prt_find_target( fvec3_base_t pos, FACING_T facing,
                         const PIP_REF particletype, const TEAM_REF team, const CHR_REF donttarget, const CHR_REF oldtarget )
{
    /// @author ZF
    /// @details This is the new improved targeting system for particles. Also includes distance in the Z direction.

    const float max_dist2 = WIDE * WIDE;

    pip_t * ppip;

    CHR_REF besttarget = INVALID_CHR_REF;
    float  longdist2 = max_dist2;

    if ( !LOADED_PIP( particletype ) ) return INVALID_CHR_REF;
    ppip = PipStack_get_ptr( particletype );

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        bool target_friend, target_enemy;

        if ( !pchr->alive || pchr->isitem || INGAME_CHR( pchr->inwhich_inventory ) ) continue;

        // prefer targeting riders over the mount itself
        if ( pchr->ismount && ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) || INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) ) ) continue;

        // ignore invictus
        if ( pchr->invictus ) continue;

        // we are going to give the player a break and not target things that
        // can't be damaged, unless the particle is homing. If it homes in,
        // the he damage_timer could drop off en route.
        if ( !ppip->homing && ( 0 != pchr->damage_timer ) ) continue;

        // Don't retarget someone we already had or not supposed to target
        if ( cnt == oldtarget || cnt == donttarget ) continue;

        target_friend = ppip->onlydamagefriendly && team == chr_get_iteam( cnt );
        target_enemy  = !ppip->onlydamagefriendly && team_hates_team( team, chr_get_iteam( cnt ) );

        if ( target_friend || target_enemy )
        {
            FACING_T angle = - facing + vec_to_facing( pchr->pos.x - pos[kX] , pchr->pos.y - pos[kY] );

            // Only proceed if we are facing the target
            if ( angle < ppip->targetangle || angle > ( 0xFFFF - ppip->targetangle ) )
            {
                float dist2 = fvec3_dist_2( pchr->pos.v, pos );

                if ( dist2 < longdist2 && dist2 <= max_dist2 )
                {
                    glouseangle = angle;
                    besttarget = cnt;
                    longdist2 = dist2;
                }
            }
        }
    }
    CHR_END_LOOP();

    // All done
    return besttarget;
}

//--------------------------------------------------------------------------------------------
bool chr_check_target( chr_t * psrc, const CHR_REF ichr_test, IDSZ idsz, const BIT_FIELD targeting_bits )
{
    bool retval = false;

    bool is_hated, hates_me;
    bool is_friend, is_prey, is_predator, is_mutual;
    chr_t * ptst;

    // Skip non-existing objects
    if ( !ACTIVE_PCHR( psrc ) ) return false;

    if ( !INGAME_CHR( ichr_test ) ) return false;
    ptst = ChrList_get_ptr( ichr_test );

    // Skip hidden characters
    if ( ptst->is_hidden ) return false;

    // Players only?
    if (( HAS_SOME_BITS( targeting_bits, TARGET_PLAYERS ) || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) ) && !VALID_PLA( ptst->is_which_player ) ) return false;

    // Skip held objects
    if ( IS_ATTACHED_CHR( ichr_test ) ) return false;

    // Allow to target ourselves?
    if ( psrc == ptst && HAS_NO_BITS( targeting_bits, TARGET_SELF ) ) return false;

    // Don't target our holder if we are an item and being held
    if ( psrc->isitem && psrc->attachedto == GET_REF_PCHR( ptst ) ) return false;

    // Allow to target dead stuff?
    if ( ptst->alive == HAS_SOME_BITS( targeting_bits, TARGET_DEAD ) ) return false;

    // Don't target invisible stuff, unless we can actually see them
    if ( !chr_can_see_object( psrc, ptst ) ) return false;

    //Need specific skill? ([NONE] always passes)
    if ( HAS_SOME_BITS( targeting_bits, TARGET_SKILL ) && 0 == chr_get_skill( ptst, idsz ) ) return false;

    // Require player to have specific quest?
    if ( HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
    {
        int quest_level = QUEST_NONE;
        player_t * ppla = PlaStack_get_ptr( ptst->is_which_player );

        quest_level = quest_log_get_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), idsz );

        // find only active quests?
        // this makes it backward-compatible with zefz's version
        if ( quest_level < 0 ) return false;
    }

    is_hated = team_hates_team( psrc->team, ptst->team );
    hates_me = team_hates_team( ptst->team, psrc->team );

    // Target neutral items? (still target evil items, could be pets)
    if (( ptst->isitem || ptst->invictus ) && !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) ) return false;

    // Only target those of proper team. Skip this part if it's a item
    if ( !ptst->isitem )
    {
        if (( HAS_NO_BITS( targeting_bits, TARGET_ENEMIES ) && is_hated ) ) return false;
        if (( HAS_NO_BITS( targeting_bits, TARGET_FRIENDS ) && !is_hated ) ) return false;
    }

    // these options are here for ideas of ways to mod this function
    is_friend    = !is_hated && !hates_me;
    is_prey      =  is_hated && !hates_me;
    is_predator  = !is_hated &&  hates_me;
    is_mutual    =  is_hated &&  hates_me;

    //This is the last and final step! Check for specific IDSZ too? (not needed if we are looking for a quest)
    if ( IDSZ_NONE == idsz || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
    {
        retval = true;
    }
    else
    {
        bool match_idsz = ( idsz == pro_get_idsz( ptst->profile_ref, IDSZ_PARENT ) ) ||
                            ( idsz == pro_get_idsz( ptst->profile_ref, IDSZ_TYPE ) );

        if ( match_idsz )
        {
            if ( !HAS_SOME_BITS( targeting_bits, TARGET_INVERTID ) ) retval = true;
        }
        else
        {
            if ( HAS_SOME_BITS( targeting_bits, TARGET_INVERTID ) ) retval = true;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
CHR_REF chr_find_target( chr_t * psrc, float max_dist, IDSZ idsz, const BIT_FIELD targeting_bits )
{
    /// @author ZF
    /// @details This is the new improved AI targeting system. Also includes distance in the Z direction.
    ///     If max_dist is 0 then it searches without a max limit.

    line_of_sight_info_t los_info;

    Uint16 cnt;
    CHR_REF best_target = INVALID_CHR_REF;
    float  best_dist2, max_dist2;

    size_t search_list_size = 0;
    CHR_REF search_list[MAX_CHR];

    if ( !ACTIVE_PCHR( psrc ) ) return INVALID_CHR_REF;

    max_dist2 = max_dist * max_dist;

    //Only loop through the players
    if ( HAS_SOME_BITS( targeting_bits, TARGET_PLAYERS ) || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
    {
        PLA_REF ipla;

        for ( ipla = 0; ipla < MAX_PLAYER; ipla ++ )
        {
            if ( !PlaStack.lst[ipla].valid || !INGAME_CHR( PlaStack.lst[ipla].index ) ) continue;

            search_list[search_list_size] = PlaStack.lst[ipla].index;
            search_list_size++;
        }
    }

    //Loop through every active object
    else
    {
        CHR_BEGIN_LOOP_ACTIVE( tnc, pchr )
        {
            search_list[search_list_size] = tnc;
            search_list_size++;
        }
        CHR_END_LOOP();
    }

    // set the line-of-sight source
    los_info.x0         = psrc->pos.x;
    los_info.y0         = psrc->pos.y;
    los_info.z0         = psrc->pos.z + psrc->bump.height;
    los_info.stopped_by = psrc->stoppedby;

    best_target = INVALID_CHR_REF;
    best_dist2  = max_dist2;
    for ( cnt = 0; cnt < search_list_size; cnt++ )
    {
        float  dist2;
        fvec3_t   diff;
        chr_t * ptst;
        CHR_REF ichr_test = search_list[cnt];

        if ( !INGAME_CHR( ichr_test ) ) continue;
        ptst = ChrList_get_ptr( ichr_test );

        if ( !chr_check_target( psrc, ichr_test, idsz, targeting_bits ) ) continue;

        fvec3_sub( diff.v, psrc->pos.v, ptst->pos.v );
        dist2 = fvec3_dot_product( diff.v, diff.v );

        if (( 0 == max_dist2 || dist2 <= max_dist2 ) && ( INVALID_CHR_REF == best_target || dist2 < best_dist2 ) )
        {
            //Invictus chars do not need a line of sight
            if ( !psrc->invictus )
            {
                // set the line-of-sight source
                los_info.x1 = ptst->pos.x;
                los_info.y1 = ptst->pos.y;
                los_info.z1 = ptst->pos.z + std::max( 1.0f, ptst->bump.height );

                if ( line_of_sight_blocked( &los_info ) ) continue;
            }

            //Set the new best target found
            best_target = ichr_test;
            best_dist2  = dist2;
        }
    }

    // make sure the target is valid
    if ( !INGAME_CHR( best_target ) ) best_target = INVALID_CHR_REF;

    return best_target;
}

//--------------------------------------------------------------------------------------------
void do_damage_tiles()
{
    // do the damage tile stuff

    CHR_BEGIN_LOOP_ACTIVE( character, pchr )
    {
        cap_t * pcap;

        pcap = pro_get_pcap( pchr->profile_ref );
        if ( NULL == pcap ) continue;

        // if the object is not really in the game, do nothing
        if ( pchr->is_hidden || !pchr->alive ) continue;

        // if you are being held by something, you are protected
        if ( INGAME_CHR( pchr->inwhich_inventory ) ) continue;

        // are we on a damage tile?
        if ( !ego_mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) continue;
        if ( 0 == ego_mesh_test_fx( PMesh, pchr->onwhichgrid, MAPFX_DAMAGE ) ) continue;

        // are we low enough?
        if ( pchr->pos.z > pchr->enviro.floor_level + DAMAGERAISE ) continue;

        // allow reaffirming damage to things like torches, even if they are being held,
        // but make the tolerance closer so that books won't burn so easily
        if ( !INGAME_CHR( pchr->attachedto ) || pchr->pos.z < pchr->enviro.floor_level + DAMAGERAISE )
        {
            if ( pchr->reaffirm_damagetype == damagetile.damagetype )
            {
                if ( 0 == ( update_wld & TILE_REAFFIRM_AND ) )
                {
                    reaffirm_attached_particles( character );
                }
            }
        }

        // do not do direct damage to items that are being held
        if ( INGAME_CHR( pchr->attachedto ) ) continue;

        // don't do direct damage to invulnerable objects
        if ( pchr->invictus ) continue;

        if ( 0 == pchr->damage_timer )
        {
            int actual_damage;
            actual_damage = damage_character( character, ATK_BEHIND, damagetile.amount, damagetile.damagetype, ( TEAM_REF )TEAM_DAMAGE, INVALID_CHR_REF, DAMFX_NBLOC | DAMFX_ARMO, false );
            pchr->damage_timer = DAMAGETILETIME;

            if (( actual_damage > 0 ) && ( -1 != damagetile.part_gpip ) && 0 == ( update_wld & damagetile.partand ) )
            {
                spawn_one_particle_global( pchr->pos.v, ATK_FRONT, damagetile.part_gpip, 0 );
            }
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void update_pits()
{
    /// @author ZZ
    /// @details This function kills any character in a deep pit...

    if ( pits.kill || pits.teleport )
    {
        //Decrease the timer
        if ( clock_pit > 0 ) clock_pit--;

        if ( 0 == clock_pit )
        {
            //Reset timer
            clock_pit = 20;

            // Kill any particles that fell in a pit, if they die in water...
            PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
            {
                if ( prt_bdl.prt_ptr->pos.z < PITDEPTH && prt_bdl.pip_ptr->end_water )
                {
                    end_one_particle_now( prt_bdl.prt_ref );
                }
            }
            PRT_END_LOOP();

            // Kill or teleport any characters that fell in a pit...
            CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
            {
                // Is it a valid character?
                if ( pchr->invictus || !pchr->alive ) continue;
                if ( IS_ATTACHED_CHR( ichr ) ) continue;

                // Do we kill it?
                if ( pits.kill && pchr->pos.z < PITDEPTH )
                {
                    // Got one!
                    kill_character( ichr, INVALID_CHR_REF, false );
                    pchr->vel.x = 0;
                    pchr->vel.y = 0;

                    /// @note ZF@> Disabled, the pitfall sound was intended for pits.teleport only
                    // Play sound effect
                    // sound_play_chunk( pchr->pos, g_wavelist[GSND_PITFALL] );
                }

                // Do we teleport it?
                if ( pits.teleport && pchr->pos.z < PITDEPTH * 4 )
                {
                    bool teleported;

                    // Teleport them back to a "safe" spot
                    teleported = chr_teleport( ichr, pits.teleport_pos.x, pits.teleport_pos.y, pits.teleport_pos.z, pchr->ori.facing_z );

                    if ( !teleported )
                    {
                        // Kill it instead
                        kill_character( ichr, INVALID_CHR_REF, false );
                    }
                    else
                    {
                        // Stop movement
                        pchr->vel.z = 0;
                        pchr->vel.x = 0;
                        pchr->vel.y = 0;

                        // Play sound effect
                        if ( VALID_PLA( pchr->is_which_player ) )
                        {
                            sound_play_chunk_full( g_wavelist[GSND_PITFALL] );
                        }
                        else
                        {
                            sound_play_chunk( pchr->pos.v, g_wavelist[GSND_PITFALL] );
                        }

                        // Do some damage (same as damage tile)
                        damage_character( ichr, ATK_BEHIND, damagetile.amount, damagetile.damagetype, ( TEAM_REF )TEAM_DAMAGE, chr_get_pai( ichr )->bumplast, DAMFX_NBLOC | DAMFX_ARMO, false );
                    }
                }
            }
            CHR_END_LOOP();
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn_particles()
{
    /// @author ZZ
    /// @details This function drops snowflakes or rain or whatever, also swings the camera

    int    cnt;
    bool foundone;

    if ( weather.time > 0 && weather.part_gpip != -1 )
    {
        weather.time--;
        if ( 0 == weather.time )
        {
            weather.time = weather.timer_reset;

            // Find a valid player
            foundone = false;
            for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
            {
                weather.iplayer = ( PLA_REF )(( REF_TO_INT( weather.iplayer ) + 1 ) % MAX_PLAYER );
                if ( PlaStack.lst[weather.iplayer].valid )
                {
                    foundone = true;
                    break;
                }
            }

            // Did we find one?
            if ( foundone )
            {
                // Yes, but is the character valid?
                CHR_REF ichr = PlaStack.lst[weather.iplayer].index;
                if ( INGAME_CHR( ichr ) && !INGAME_CHR( ChrList.lst[ichr].inwhich_inventory ) )
                {
                    chr_t * pchr = ChrList_get_ptr( ichr );

                    // Yes, so spawn over that character
                    PRT_REF particle = spawn_one_particle_global( pchr->pos.v, ATK_FRONT, weather.part_gpip, 0 );
                    if ( DEFINED_PRT( particle ) )
                    {
                        prt_t * pprt = PrtList_get_ptr( particle );

                        bool destroy_particle = false;

                        if ( weather.over_water && !prt_is_over_water( particle ) )
                        {
                            destroy_particle = true;
                        }
                        else if ( EMPTY_BIT_FIELD != prt_test_wall( pprt, NULL, NULL ) )
                        {
                            destroy_particle = true;
                        }
                        else
                        {
                            // Weather particles spawned at the edge of the map look ugly, so don't spawn them there
                            if ( pprt->pos.x < EDGE || pprt->pos.x > PMesh->gmem.edge_x - EDGE )
                            {
                                destroy_particle = true;
                            }
                            else if ( pprt->pos.y < EDGE || pprt->pos.y > PMesh->gmem.edge_y - EDGE )
                            {
                                destroy_particle = true;
                            }
                        }

                        if ( destroy_particle )
                        {
                            PrtList_free_one( particle );
                        }
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch( const PLA_REF ipla )
{
    /// @author ZZ
    /// @details This function converts input readings to latch settings, so players can
    ///    move around

    TURN_T turnsin;
    float dist, scale;
    float fsin, fcos;
    latch_t sum;
    bool fast_camera_turn;
    fvec2_t joy_pos, joy_new;

    player_t       * ppla;
    input_device_t * pdevice;

    // skip invalid players
    if ( INVALID_PLA( ipla ) ) return;
    ppla = PlaStack_get_ptr( ipla );

    // is the device a local device or an internet device?
    pdevice = ppla->pdevice;
    if ( NULL == pdevice ) return;

    //No need to continue if device is not enabled
    if ( !input_device_is_enabled( pdevice ) ) return;

    // find the camera that is pointing at this character
    std::shared_ptr<Camera> pcam = _cameraSystem.getCameraByChrID(ppla->index);
    if ( nullptr == pcam ) return;

    // fast camera turn if it is enabled and there is only 1 local player
    fast_camera_turn = ( 1 == local_stats.player_count ) && ( CAM_TURN_GOOD == pcam->getTurnMode() );

    // Clear the player's latch buffers
    latch_init( &( sum ) );
    fvec2_self_clear( joy_new.v );
    fvec2_self_clear( joy_pos.v );

    // generate the transforms relative to the camera
    // this needs to be changed for multicamera
    turnsin = TO_TURN( pcam->getOrientation().facing_z );
    fsin    = turntosin[ turnsin ];
    fcos    = turntocos[ turnsin ];

    if ( INPUT_DEVICE_MOUSE == pdevice->device_type )
    {
        // Mouse routines

        if ( fast_camera_turn || !input_device_control_active( pdevice,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
        {
            dist = std::sqrt( mous.x * mous.x + mous.y * mous.y );
            if ( dist > 0 )
            {
                scale = mous.sense / dist;
                if ( dist < mous.sense )
                {
                    scale = dist / mous.sense;
                }

                if ( mous.sense != 0 )
                {
                    scale /= mous.sense;
                }

                joy_pos.x = mous.x * scale;
                joy_pos.y = mous.y * scale;

                //if ( fast_camera_turn && !input_device_control_active( pdevice,  CONTROL_CAMERA ) )  joy_pos.x = 0;

                joy_new.x = ( joy_pos.x * fcos + joy_pos.y * fsin );
                joy_new.y = ( -joy_pos.x * fsin + joy_pos.y * fcos );
            }
        }
    }

    else if ( INPUT_DEVICE_KEYBOARD == pdevice->device_type )
    {
        // Keyboard routines

        if ( fast_camera_turn || !input_device_control_active( pdevice, CONTROL_CAMERA ) )
        {
            if ( input_device_control_active( pdevice,  CONTROL_RIGHT ) ) joy_pos.x++;
            if ( input_device_control_active( pdevice,  CONTROL_LEFT ) ) joy_pos.x--;
            if ( input_device_control_active( pdevice,  CONTROL_DOWN ) ) joy_pos.y++;
            if ( input_device_control_active( pdevice,  CONTROL_UP ) ) joy_pos.y--;

            if ( fast_camera_turn )  joy_pos.x = 0;

            joy_new.x = ( joy_pos.x * fcos + joy_pos.y * fsin );
            joy_new.y = ( -joy_pos.x * fsin + joy_pos.y * fcos );
        }
    }
    else if ( IS_VALID_JOYSTICK( pdevice->device_type ) )
    {
        // Joystick routines

        //Figure out which joystick we are using
        joystick_data_t *joystick;
        joystick = joy_lst + ( pdevice->device_type - MAX_JOYSTICK );

        if ( fast_camera_turn || !input_device_control_active( pdevice, CONTROL_CAMERA ) )
        {
            joy_pos.x = joystick->x;
            joy_pos.y = joystick->y;

            dist = joy_pos.x * joy_pos.x + joy_pos.y * joy_pos.y;
            if ( dist > 1.0f )
            {
                scale = 1.0f / std::sqrt( dist );
                joy_pos.x *= scale;
                joy_pos.y *= scale;
            }

            if ( fast_camera_turn && !input_device_control_active( pdevice, CONTROL_CAMERA ) )  joy_pos.x = 0;

            joy_new.x = ( joy_pos.x * fcos + joy_pos.y * fsin );
            joy_new.y = ( -joy_pos.x * fsin + joy_pos.y * fcos );
        }
    }

    else
    {
        // unknown device type.
        pdevice = NULL;
    }

    // Update movement (if any)
    sum.x += joy_new.x;
    sum.y += joy_new.y;

    // Read control buttons
    if ( !ppla->draw_inventory )
    {
        if ( input_device_control_active( pdevice, CONTROL_JUMP ) )
            SET_BIT( sum.b, LATCHBUTTON_JUMP );
        if ( input_device_control_active( pdevice, CONTROL_LEFT_USE ) )
            SET_BIT( sum.b, LATCHBUTTON_LEFT );
        if ( input_device_control_active( pdevice, CONTROL_LEFT_GET ) )
            SET_BIT( sum.b, LATCHBUTTON_ALTLEFT );
        if ( input_device_control_active( pdevice, CONTROL_RIGHT_USE ) )
            SET_BIT( sum.b, LATCHBUTTON_RIGHT );
        if ( input_device_control_active( pdevice, CONTROL_RIGHT_GET ) )
            SET_BIT( sum.b, LATCHBUTTON_ALTRIGHT );

        // Now update movement and input
        input_device_add_latch( pdevice, sum.x, sum.y );

        ppla->local_latch.x = pdevice->latch.x;
        ppla->local_latch.y = pdevice->latch.y;
        ppla->local_latch.b = sum.b;
    }

    //inventory mode
    else if ( ppla->inventory_cooldown < update_wld )
    {
        int new_selected = ppla->inventory_slot;
        chr_t *pchr = ChrList_get_ptr( ppla->index );

        //dirty hack here... mouse seems to be inverted in inventory mode?
        if ( pdevice->device_type == INPUT_DEVICE_MOUSE )
        {
            joy_pos.x = - joy_pos.x;
            joy_pos.y = - joy_pos.y;
        }

        //handle inventory movement
        if ( joy_pos.x < 0 )       new_selected--;
        else if ( joy_pos.x > 0 )  new_selected++;
        if ( joy_pos.y < 0 )       new_selected -= MAXINVENTORY / 2;
        else if ( joy_pos.y > 0 )  new_selected += MAXINVENTORY / 2;

        //clip to a valid value
        if ( ppla->inventory_slot != new_selected )
        {
            ppla->inventory_cooldown = update_wld + 10;
            ppla->inventory_slot = CLIP( new_selected, 0, MAXINVENTORY - 1 );
        }

        //handle item control
        if ( pchr->inst.action_ready && 0 == pchr->reload_timer )
        {
            //handle LEFT hand control
            if ( input_device_control_active( pdevice, CONTROL_LEFT_GET ) )
            {
                //put it away and swap with any existing item
                inventory_swap_item( ppla->index, ppla->inventory_slot, SLOT_LEFT, false );

                // Make it take a little time
                chr_play_action( pchr, ACTION_MG, false );
                pchr->reload_timer = PACKDELAY;
            }

            //handle RIGHT hand control
            if ( input_device_control_active( pdevice, CONTROL_RIGHT_GET ) )
            {
                // put it away and swap with any existing item
                inventory_swap_item( ppla->index, ppla->inventory_slot, SLOT_RIGHT, false );

                // Make it take a little time
                chr_play_action( pchr, ACTION_MG, false );
                pchr->reload_timer = PACKDELAY;
            }
        }

        //empty any movement
        ppla->local_latch.x = 0;
        ppla->local_latch.y = 0;
    }

    //enable inventory mode?
    if ( update_wld > ppla->inventory_cooldown && input_device_control_active( pdevice, CONTROL_INVENTORY ) )
    {
        ppla->draw_inventory = !ppla->draw_inventory;
        ppla->inventory_cooldown = update_wld + ( ONESECOND / 4 );
        ppla->inventory_lerp = 0xFFFF;
    }
}

//--------------------------------------------------------------------------------------------
void set_local_latches()
{
    PLA_REF cnt;

    for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        set_one_player_latch( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void check_stats()
{
    /// @author ZZ
    /// @details This function lets the players check character stats

    static int stat_check_timer = 0;
    static int stat_check_delay = 0;

    int ticks;
    if ( keyb.chat_mode ) return;

    ticks = egoboo_get_ticks();
    if ( ticks > stat_check_timer + 20 )
    {
        stat_check_timer = ticks;
    }

    stat_check_delay -= 20;
    if ( stat_check_delay > 0 )
        return;

    // Show map cheat
    if ( cfg.dev_mode && SDL_KEYDOWN( keyb, SDLK_m ) && SDL_KEYDOWN( keyb, SDLK_LSHIFT ) && mapvalid )
    {
        mapon = !mapon;
        youarehereon = true;
        stat_check_delay = 150;
    }

    // XP CHEAT
    if ( cfg.dev_mode && SDL_KEYDOWN( keyb, SDLK_x ) )
    {
        PLA_REF docheat = ( PLA_REF )MAX_PLAYER;
        if ( SDL_KEYDOWN( keyb, SDLK_1 ) )  docheat = 0;
        else if ( SDL_KEYDOWN( keyb, SDLK_2 ) )  docheat = 1;
        else if ( SDL_KEYDOWN( keyb, SDLK_3 ) )  docheat = 2;
        else if ( SDL_KEYDOWN( keyb, SDLK_4 ) )  docheat = 3;

        //Apply the cheat if valid
        if ( INGAME_CHR( PlaStack.lst[docheat].index ) )
        {
            Uint32 xpgain;
            chr_t * pchr = ChrList_get_ptr( PlaStack.lst[docheat].index );
            cap_t * pcap = pro_get_pcap( pchr->profile_ref );

            //Give 10% of XP needed for next level
            xpgain = 0.1f * ( pcap->experience_forlevel[std::min( pchr->experiencelevel+1, MAXLEVEL )] - pcap->experience_forlevel[pchr->experiencelevel] );
            give_experience( pchr->ai.index, xpgain, XP_DIRECT, true );
            stat_check_delay = 1;
        }
    }

    // LIFE CHEAT
    if ( cfg.dev_mode && SDL_KEYDOWN( keyb, SDLK_z ) )
    {
        PLA_REF docheat = ( PLA_REF )MAX_PLAYER;

        if ( SDL_KEYDOWN( keyb, SDLK_1 ) )  docheat = 0;
        else if ( SDL_KEYDOWN( keyb, SDLK_2 ) )  docheat = 1;
        else if ( SDL_KEYDOWN( keyb, SDLK_3 ) )  docheat = 2;
        else if ( SDL_KEYDOWN( keyb, SDLK_4 ) )  docheat = 3;

        //Apply the cheat if valid
        if ( INGAME_CHR( PlaStack.lst[docheat].index ) )
        {
            cap_t * pcap;
            chr_t * pchr = ChrList_get_ptr( PlaStack.lst[docheat].index );
            pcap = pro_get_pcap( pchr->profile_ref );

            //Heal 1 life
            heal_character( pchr->ai.index, pchr->ai.index, 256, true );
            stat_check_delay = 1;
        }
    }

    // Display armor stats?
    if ( SDL_KEYDOWN( keyb, SDLK_LSHIFT ) )
    {
        if ( SDL_KEYDOWN( keyb, SDLK_1 ) )  { show_armor( 0 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_2 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_3 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_4 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_5 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_6 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_7 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_8 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
    }

    // Display enchantment stats?
    else if ( SDL_KEYDOWN( keyb, SDLK_LCTRL ) )
    {
        if ( SDL_KEYDOWN( keyb, SDLK_1 ) )  { show_full_status( 0 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_2 ) )  { show_full_status( 1 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_3 ) )  { show_full_status( 2 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_4 ) )  { show_full_status( 3 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_5 ) )  { show_full_status( 4 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_6 ) )  { show_full_status( 5 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_7 ) )  { show_full_status( 6 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_8 ) )  { show_full_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character special powers?
    else if ( SDL_KEYDOWN( keyb, SDLK_LALT ) )
    {
        if ( SDL_KEYDOWN( keyb, SDLK_1 ) )  { show_magic_status( 0 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_2 ) )  { show_magic_status( 1 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_3 ) )  { show_magic_status( 2 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_4 ) )  { show_magic_status( 3 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_5 ) )  { show_magic_status( 4 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_6 ) )  { show_magic_status( 5 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_7 ) )  { show_magic_status( 6 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_8 ) )  { show_magic_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character stats?
    else
    {
        if ( SDL_KEYDOWN( keyb, SDLK_1 ) )  { show_stat( 0 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_2 ) )  { show_stat( 1 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_3 ) )  { show_stat( 2 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_4 ) )  { show_stat( 3 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_5 ) )  { show_stat( 4 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_6 ) )  { show_stat( 5 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_7 ) )  { show_stat( 6 ); stat_check_delay = 1000; }
        if ( SDL_KEYDOWN( keyb, SDLK_8 ) )  { show_stat( 7 ); stat_check_delay = 1000; }
    }
}

//--------------------------------------------------------------------------------------------
void show_stat( int statindex )
{
    /// @author ZZ
    /// @details This function shows the more specific stats for a character

    CHR_REF character;
    int     level;
    char    gender[8] = EMPTY_CSTR;

    if ( statindex < StatusList.count )
    {
        character = StatusList.lst[statindex].who;

        if ( INGAME_CHR( character ) )
        {
            cap_t * pcap;
            chr_t * pchr = ChrList_get_ptr( character );

            pcap = pro_get_pcap( pchr->profile_ref );

            // Name
            DisplayMsg_printf( "=%s=", chr_get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_CAPITAL, NULL, 0 ) );

            // Level and gender and class
            gender[0] = 0;
            if ( pchr->alive )
            {
                int itmp;
                const char * gender_str;

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
                    DisplayMsg_printf( "~%dst level %s%s", level, gender_str, pcap->classname );
                }
                else if ( 2 == itmp )
                {
                    DisplayMsg_printf( "~%dnd level %s%s", level, gender_str, pcap->classname );
                }
                else if ( 3 == itmp )
                {
                    DisplayMsg_printf( "~%drd level %s%s", level, gender_str, pcap->classname );
                }
                else
                {
                    DisplayMsg_printf( "~%dth level %s%s", level, gender_str, pcap->classname );
                }
            }
            else
            {
                DisplayMsg_printf( "~Dead %s", pcap->classname );
            }

            // Stats
            DisplayMsg_printf( "~STR:~%2d~WIS:~%2d~DEF:~%d", SFP8_TO_SINT( pchr->strength ), SFP8_TO_SINT( pchr->wisdom ), 255 - pchr->defense );
            DisplayMsg_printf( "~INT:~%2d~DEX:~%2d~EXP:~%u", SFP8_TO_SINT( pchr->intelligence ), SFP8_TO_SINT( pchr->dexterity ), pchr->experience );
        }
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( int statindex )
{
    /// @author ZF
    /// @details This function shows detailed armor information for the character

    STRING tmps;
    CHR_REF ichr;

    SKIN_T  skinlevel;

    cap_t * pcap;
    chr_t * pchr;

    if ( statindex < 0 || ( size_t )statindex >= StatusList.count ) return;

    ichr = StatusList.lst[statindex].who;
    if ( !INGAME_CHR( ichr ) ) return;

    pchr = ChrList_get_ptr( ichr );
    skinlevel = pchr->skin;

    pcap = chr_get_pcap( ichr );
    if ( NULL == pcap ) return;

    // Armor Name
    DisplayMsg_printf( "=%s=", pcap->skin_info.name[skinlevel] );

    // Armor Stats
    DisplayMsg_printf( "~DEF: %d  SLASH:%3.0f%%~CRUSH:%3.0f%% POKE:%3.0f%%", 255 - pcap->defense[skinlevel],
                       pcap->damage_resistance[DAMAGE_SLASH][skinlevel]*100,
                       pcap->damage_resistance[DAMAGE_CRUSH][skinlevel]*100,
                       pcap->damage_resistance[DAMAGE_POKE ][skinlevel]*100 );

    DisplayMsg_printf( "~HOLY:%3.0f%%~EVIL:%3.0f%%~FIRE:%3.0f%%~ICE:%3.0f%%~ZAP:%3.0f%%",
                       pcap->damage_resistance[DAMAGE_HOLY][skinlevel]*100,
                       pcap->damage_resistance[DAMAGE_EVIL][skinlevel]*100,
                       pcap->damage_resistance[DAMAGE_FIRE][skinlevel]*100,
                       pcap->damage_resistance[DAMAGE_ICE ][skinlevel]*100,
                       pcap->damage_resistance[DAMAGE_ZAP ][skinlevel]*100 );

    DisplayMsg_printf( "~Type: %s", ( pcap->skin_info.dressy & ( 1 << skinlevel ) ) ? "Light Armor" : "Heavy Armor" );

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

    DisplayMsg_printf( "~Speed:~%3.0f~Jump Skill:~%s", pchr->maxaccel_reset*80, tmps );
}

//--------------------------------------------------------------------------------------------
bool get_chr_regeneration( chr_t * pchr, int * pliferegen, int * pmanaregen )
{
    /// @author ZF
    /// @details Get a character's life and mana regeneration, considering all sources

    int local_liferegen, local_manaregen;
    CHR_REF ichr;

    if ( !ACTIVE_PCHR( pchr ) ) return false;
    ichr = GET_REF_PCHR( pchr );

    if ( NULL == pliferegen ) pliferegen = &local_liferegen;
    if ( NULL == pmanaregen ) pmanaregen = &local_manaregen;

    // set the base values
    ( *pmanaregen ) = pchr->mana_return / MANARETURNSHIFT;
    ( *pliferegen ) = pchr->life_return;

    // Don't forget to add gains and costs from enchants
    ENC_BEGIN_LOOP_ACTIVE( enchant, penc )
    {
        if ( penc->target_ref == ichr )
        {
            ( *pliferegen ) += penc->target_life;
            ( *pmanaregen ) += penc->target_mana;
        }

        if ( penc->owner_ref == ichr )
        {
            ( *pliferegen ) += penc->owner_life;
            ( *pmanaregen ) += penc->owner_mana;
        }
    }
    ENC_END_LOOP();

    return true;
}

//--------------------------------------------------------------------------------------------
void show_full_status( int statindex )
{
    /// @author ZF
    /// @details This function shows detailed armor information for the character including magic

    CHR_REF character;
    int manaregen, liferegen;
    cap_t * pcap;
    chr_t * pchr;
    SKIN_T  skinlevel;

    if ( statindex < 0 || ( size_t )statindex >= StatusList.count ) return;
    character = StatusList.lst[statindex].who;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList_get_ptr( character );
    skinlevel = pchr->skin;

    pcap = chr_get_pcap( character );
    if ( NULL == pcap ) return;

    // clean up the enchant list
    cleanup_character_enchants( pchr );

    // Enchanted?
    DisplayMsg_printf( "=%s is %s=", chr_get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ), INGAME_ENC( pchr->firstenchant ) ? "enchanted" : "unenchanted" );

    // Armor Stats
    DisplayMsg_printf( "~DEF: %d  SLASH:%3.0f%%~CRUSH:%3.0f%% POKE:%3.0f%%", 255 - pcap->defense[skinlevel],
                       pchr->damage_resistance[DAMAGE_SLASH]*100,
                       pchr->damage_resistance[DAMAGE_CRUSH]*100,
                       pchr->damage_resistance[DAMAGE_POKE ]*100 );

    DisplayMsg_printf( "~HOLY:%3.0f%%~EVIL:%3.0f%%~FIRE:%3.0f%%~ICE:%3.0f%%~ZAP:%3.0f%%",
                       pchr->damage_resistance[DAMAGE_HOLY]*100,
                       pchr->damage_resistance[DAMAGE_EVIL]*100,
                       pchr->damage_resistance[DAMAGE_FIRE]*100,
                       pchr->damage_resistance[DAMAGE_ICE ]*100,
                       pchr->damage_resistance[DAMAGE_ZAP ]*100 );

    get_chr_regeneration( pchr, &liferegen, &manaregen );

    DisplayMsg_printf( "Mana Regen:~%4.2f Life Regen:~%4.2f", FP8_TO_FLOAT( manaregen ), FP8_TO_FLOAT( liferegen ) );
}

//--------------------------------------------------------------------------------------------
void show_magic_status( int statindex )
{
    /// @author ZF
    /// @details Displays special enchantment effects for the character

    CHR_REF character;
    const char * missile_str;
    chr_t * pchr;

    if ( statindex < 0 || ( size_t )statindex >= StatusList.count ) return;

    character = StatusList.lst[statindex].who;

    if ( !INGAME_CHR( character ) ) return;
    pchr = ChrList_get_ptr( character );

    // clean up the enchant list
    cleanup_character_enchants( pchr );

    // Enchanted?
    DisplayMsg_printf( "=%s is %s=", chr_get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ), INGAME_ENC( pchr->firstenchant ) ? "enchanted" : "unenchanted" );

    // Enchantment status
    DisplayMsg_printf( "~See Invisible: %s~~See Kurses: %s",
                       pchr->see_invisible_level ? "Yes" : "No",
                       pchr->see_kurse_level ? "Yes" : "No" );

    DisplayMsg_printf( "~Channel Life: %s~~Waterwalking: %s",
                       pchr->canchannel ? "Yes" : "No",
                       pchr->waterwalk ? "Yes" : "No" );

    switch ( pchr->missiletreatment )
    {
        case MISSILE_REFLECT: missile_str = "Reflect"; break;
        case MISSILE_DEFLECT: missile_str = "Deflect"; break;

        default:
        case MISSILE_NORMAL : missile_str = "None";    break;
    }

    DisplayMsg_printf( "~Flying: %s~~Missile Protection: %s", ( pchr->flyheight > 0 ) ? "Yes" : "No", missile_str );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    /// @author ZZ
    /// @details This function sets all of the character's starting tilt values

    Uint8 twist;

    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        if ( !INGAME_CHR( cnt ) ) continue;

        if ( pchr->stickybutt )
        {
            twist = ego_mesh_get_twist( PMesh, pchr->onwhichgrid );
            pchr->ori.map_twist_facing_y = map_twist_facing_y[twist];
            pchr->ori.map_twist_facing_x = map_twist_facing_x[twist];
        }
        else
        {
            pchr->ori.map_twist_facing_y = MAP_TURN_OFFSET;
            pchr->ori.map_twist_facing_x = MAP_TURN_OFFSET;
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void import_dir_profiles_vfs( const char * dirname )
{
    STRING newloadname;
    STRING filename;
    int cnt;

    if ( NULL == PMod || INVALID_CSTR( dirname ) ) return;

    if ( !PMod->importvalid || 0 == PMod->importamount ) return;

    for ( cnt = 0; cnt < PMod->importamount*MAX_IMPORT_PER_PLAYER; cnt++ )
    {
        // Make sure the object exists...
        snprintf( filename, SDL_arraysize( filename ), "%s/temp%04d.obj", dirname, cnt );
        snprintf( newloadname, SDL_arraysize( newloadname ), "%s/data.txt", filename );

        if ( vfs_exists( newloadname ) )
        {
            // new player found
            if ( 0 == ( cnt % MAX_IMPORT_PER_PLAYER ) ) import_data.player++;

            // store the slot info
            import_data.slot = cnt;

            // load it
            import_data.slot_lst[cnt] = load_one_profile_vfs( filename, MAX_PROFILE );
            import_data.max_slot      = std::max( import_data.max_slot, cnt );
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
    overrideslots = true;
    import_data.player = -1;
    import_data.slot   = -100;

    import_dir_profiles_vfs( "mp_import" );
    import_dir_profiles_vfs( "mp_remote" );

    // return this to the normal value
    overrideslots = false;
}

//--------------------------------------------------------------------------------------------
void game_load_profile_ai()
{
    /// @author ZF
    /// @details load the AI for each profile, done last so that all reserved slot numbers are already set
    /// since AI scripts can dynamically load new objects if they require it
    PRO_REF ipro;
    STRING loadname;

    // ensure that the script parser exists
    parser_state_t * ps = script_compiler_get_state();

    for ( ipro = 0; ipro < MAX_PROFILE; ipro++ )
    {
        pro_t *ppro;

        if ( !LOADED_PRO( ipro ) ) continue;
        ppro = ProList_get_ptr( ipro );

        // Load the AI script for this iobj
        make_newloadname( ppro->name, "/script.txt", loadname );

        load_ai_script_vfs( ps, loadname, ppro, &ppro->ai_script );
    }
}

//--------------------------------------------------------------------------------------------
void game_load_module_profiles( const char *modname )
{
    /// @author BB
    /// @details Search for .obj directories in the module directory and load them

    vfs_search_context_t * ctxt;
    const char *filehandle;
    STRING newloadname;

    import_data.slot = -100;
    make_newloadname( modname, "objects", newloadname );

    ctxt = vfs_findFirst( newloadname, "obj", VFS_SEARCH_DIR );
    filehandle = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( filehandle ) )
    {
        load_one_profile_vfs( filehandle, MAX_PROFILE );

        ctxt = vfs_findNext( &ctxt );
        filehandle = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void game_load_global_profiles()
{
    // load all special objects
    load_one_profile_vfs( "mp_data/globalobjects/book.obj", SPELLBOOK );

    // load the objects from various import directories
    load_all_profiles_import();
}

//--------------------------------------------------------------------------------------------
void game_load_all_profiles( const char *modname )
{
    /// @author ZZ
    /// @details This function loads a module's local objects and overrides the global ones already loaded

    // ensure that the script parser exists
    parser_state_t * ps = script_compiler_get_state();

    // Log all of the script errors
    script_compiler_clear_error( ps );

    // clear out any old object definitions
    release_all_pro();

    // load the global objects
    game_load_global_profiles();

    // load the objects from the module's directory
    game_load_module_profiles( modname );
}

//--------------------------------------------------------------------------------------------
bool chr_setup_apply( const CHR_REF ichr, spawn_file_info_t *pinfo )
{
    chr_t * pchr, *pparent;
    cap_t *pcap;

    // trap bad pointers
    if ( NULL == pinfo ) return false;

    if ( !INGAME_CHR( ichr ) ) return false;
    pchr = ChrList_get_ptr( ichr );
    pcap = chr_get_pcap( ichr );

    pparent = NULL;
    if ( INGAME_CHR( pinfo->parent ) ) pparent = ChrList_get_ptr( pinfo->parent );

    pchr->money = pchr->money + pinfo->money;
    if ( pchr->money > MAXMONEY )  pchr->money = MAXMONEY;
    if ( pchr->money < 0 )  pchr->money = 0;

    pchr->ai.content = pinfo->content;
    pchr->ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        inventory_add_item( pinfo->parent, ichr, MAXINVENTORY, true );

        SET_BIT( pchr->ai.alert, ALERTIF_GRABBED );     // Make spellbooks change
    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        grip_offset_t grip_off = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;

        if ( rv_success == attach_character_to_mount( ichr, pinfo->parent, grip_off ) )
        {
            // Handle the "grabbed" messages
            //scr_run_chr_script( ichr );
        }
    }

    // Set the starting pinfo->level
    if ( pinfo->level > 0 )
    {
        if ( pchr->experiencelevel < pinfo->level )
        {
            pchr->experience = pcap->experience_forlevel[pinfo->level];
            do_level_up( ichr );
        }
    }

    // automatically identify and unkurse all player starting equipment? I think yes.
    if ( start_new_player && NULL != pparent && VALID_PLA( pparent->is_which_player ) )
    {
        chr_t *pitem;
        pchr->nameknown = true;

        //Unkurse both inhand items
        if ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
        {
            pitem = ChrList_get_ptr( ichr );
            pitem->iskursed = false;
        }
        if ( INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) )
        {
            pitem = ChrList_get_ptr( ichr );
            pitem->iskursed = false;
        }

    }

    return true;
}

void convert_spawn_file_load_name( spawn_file_info_t * psp_info )
{
    /// @author ZF
    /// @details This turns a spawn comment line into an actual folder name we can use to load something with

    if ( NULL == psp_info ) return;

    // trim any excess spaces off the psp_info->spawn_coment
    str_trim( psp_info->spawn_coment );

    //If it is a reference to a random treasure table then get a random object from that table
    if ( '%' == psp_info->spawn_coment[0] )
    {
        get_random_treasure( psp_info->spawn_coment, SDL_arraysize( psp_info->spawn_coment ) );
    }

    // make sure it ends with a .obj extension
    if ( NULL == strstr( psp_info->spawn_coment, ".obj" ) )
    {
        strcat( psp_info->spawn_coment, ".obj" );
    }

    // no capital letters
    strlwr( psp_info->spawn_coment );
}

//--------------------------------------------------------------------------------------------
bool activate_spawn_file_load_object( spawn_file_info_t * psp_info )
{
    /// @author BB
    /// @details Try to load a global object named int psp_info->spawn_coment into
    ///               slot psp_info->slot

    STRING filename;
    PRO_REF ipro;

    if ( NULL == psp_info || psp_info->slot < 0 ) return false;

    //Is it already loaded?
    ipro = ( PRO_REF )psp_info->slot;
    if ( LOADED_PRO( ipro ) ) return false;

    // do the loading
    if ( CSTR_END != psp_info->spawn_coment[0] )
    {
        // we are relying on the virtual mount point "mp_objects", so use
        // the vfs/PHYSFS file naming conventions
        snprintf( filename, SDL_arraysize( filename ), "mp_objects/%s", psp_info->spawn_coment );

        psp_info->slot = load_one_profile_vfs( filename, psp_info->slot );
    }

    return LOADED_PRO(( PRO_REF ) psp_info->slot );
}

//--------------------------------------------------------------------------------------------
bool activate_spawn_file_spawn( spawn_file_info_t * psp_info )
{
    int     local_index = 0;
    CHR_REF new_object;
    chr_t * pobject;
    PRO_REF iprofile;

    if ( NULL == psp_info || !psp_info->do_spawn || psp_info->slot < 0 ) return false;

    iprofile = ( PRO_REF )psp_info->slot;

    // Spawn the character
    new_object = spawn_one_character( psp_info->pos.v, iprofile, psp_info->team, psp_info->skin, psp_info->facing, psp_info->pname, INVALID_CHR_REF );
    if ( !DEFINED_CHR( new_object ) ) return false;

    pobject = ChrList_get_ptr( new_object );

    // determine the attachment
    if ( psp_info->attach == ATTACH_NONE )
    {
        // Free character
        psp_info->parent = new_object;
        make_one_character_matrix( new_object );
    }

    chr_setup_apply( new_object, psp_info );

    // Turn on PlaStack.count input devices
    if ( psp_info->stat )
    {
        // what we do depends on what kind of module we're loading
        if ( 0 == PMod->importamount && PlaStack.count < PMod->playeramount )
        {
            // a single player module

            bool player_added;

            player_added = add_player( new_object, ( PLA_REF )PlaStack.count, InputDevices.lst + local_stats.player_count );

            if ( start_new_player && player_added )
            {
                // !!!! make sure the player is identified !!!!
                pobject->nameknown = true;
            }
        }
        else if ( PlaStack.count < PMod->importamount && PlaStack.count < PMod->playeramount && PlaStack.count < ImportList.count )
        {
            // A multiplayer module

            bool player_added;

            local_index = -1;
            for ( size_t tnc = 0; tnc < ImportList.count; tnc++ )
            {
                if ( pobject->profile_ref <= import_data.max_slot && VALID_PRO_RANGE( pobject->profile_ref ) )
                {
                    int islot = REF_TO_INT( pobject->profile_ref );

                    if ( import_data.slot_lst[islot] == ImportList.lst[tnc].slot )
                    {
                        local_index = tnc;
                        break;
                    }
                }
            }

            player_added = false;
            if ( -1 != local_index )
            {
                // It's a local PlaStack.count
                player_added = add_player( new_object, ( PLA_REF )PlaStack.count, InputDevices.lst + ImportList.lst[local_index].local_player_num );
            }
            else
            {
                // It's a remote PlaStack.count
                player_added = add_player( new_object, ( PLA_REF )PlaStack.count, NULL );
            }
        }

        // Turn on the stat display
        statlist_add( new_object );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void activate_spawn_file_vfs()
{
    /// @author ZZ
    /// @details This function sets up character data, loaded from "SPAWN.TXT"

    const char          *newloadname;
    vfs_FILE            *fileread;

    spawn_file_info_t   spawn_list[MAX_CHR];                //The full list of objects to be spawned
    STRING              reserved_slot[MAX_PROFILE];         //Keep track of which slot numbers are reserved by their load name
    int                 dynamic_list[MAX_CHR];              //references to slots that need to be dynamically loaded later
    size_t              spawn_count = 0;
    size_t              dynamic_count = 0;

    // tell everyone we are spawning a module
    activate_spawn_file_active = true;

    // Turn some back on
    newloadname = "mp_data/spawn.txt";
    fileread = vfs_openRead( newloadname );

    PlaStack.count = 0;
    if ( NULL == fileread )
    {
        log_error( "Cannot read file: %s\n", newloadname );
    }
    else
    {
        CHR_REF parent = INVALID_CHR_REF;
        size_t i;

        // Mark each slot as free with the string termination character
        memset( reserved_slot, CSTR_END, sizeof( reserved_slot ) );

        //First load spawn data of every object
        while ( spawn_file_scan( fileread, &spawn_list[spawn_count] ) )
        {
            spawn_file_info_t * pspawn;

            //Spit out a warning if they break the limit
            if ( spawn_count >= MAX_CHR )
            {
                log_warning( "Too many objects in spawn.txt! Maximum number of objects is %d\n", MAX_CHR );
                break;
            }
            pspawn = spawn_list + spawn_count;

            // check to see if the slot is valid
            if ( pspawn->slot >= MAX_PROFILE )
            {
                log_warning( "Invalid slot %d for \"%s\" in file \"%s\"\n", pspawn->slot, pspawn->spawn_coment, newloadname );
                continue;
            }

            //convert the spawn name into a format we like
            convert_spawn_file_load_name( pspawn );

            // If it is a dynamic slot, remember to dynamically allocate it for later
            if ( pspawn->slot <= -1 )
            {
                dynamic_list[dynamic_count] = spawn_count;
                dynamic_count++;
            }

            //its a static slot number, mark it as reserved if it isnt already
            else if ( reserved_slot[pspawn->slot][0] == CSTR_END )
            {
                strncpy( reserved_slot[pspawn->slot], pspawn->spawn_coment, SDL_arraysize( reserved_slot[pspawn->slot] ) );
            }

            //Finished with this object for now
            spawn_count++;
        }

        //Next we dynamically find slot numbers for each of the objects in the dynamic list
        for ( i = 0; i < dynamic_count; i++ )
        {
            spawn_file_info_t * sp_dynamic = spawn_list + dynamic_list[i];

            //Find first free slot that is not the spellbook slot
            for ( sp_dynamic->slot = MAX_IMPORT_PER_PLAYER * MAX_PLAYER; sp_dynamic->slot < MAX_PROFILE; sp_dynamic->slot++ )
            {
                //dont try to grab the spellbook slot
                if ( sp_dynamic->slot == SPELLBOOK ) continue;

                //the slot already dynamically loaded by a different spawn object of the same type that we are, no need to reload in a new slot
                if ( 0 == strcmp( reserved_slot[sp_dynamic->slot], sp_dynamic->spawn_coment ) ) break;

                //found a completely free slot
                if ( reserved_slot[sp_dynamic->slot][0] == CSTR_END )
                {
                    //Reserve this one for us
                    strncpy( reserved_slot[sp_dynamic->slot], sp_dynamic->spawn_coment, SDL_arraysize( reserved_slot[sp_dynamic->slot] ) );
                    break;
                }
            }

            //If all slots are reserved, spit out a warning
            if ( sp_dynamic->slot == INVALID_PRO_REF ) log_warning( "Could not allocate free dynamic slot for object (%s). All %d slots in use?\n", sp_dynamic->spawn_coment, INVALID_PRO_REF );
        }

        //Now spawn each object in order
        for ( i = 0; i < spawn_count; i++ )
        {
            spawn_file_info_t *sp_info = spawn_list + i;

            //Do we have a parent?
            if ( sp_info->attach != ATTACH_NONE ) sp_info->parent = parent;

            // If nothing is already in that slot, try to load it.
            if ( !LOADED_PRO(( PRO_REF ) sp_info->slot ) )
            {
                bool import_object = sp_info->slot > PMod->importamount * MAX_IMPORT_PER_PLAYER;

                if ( !activate_spawn_file_load_object( sp_info ) )
                {
                    // no, give a warning if it is useful
                    if ( import_object )
                    {
                        log_warning( "The object \"%s\"(slot %d) in file \"%s\" does not exist on this machine\n", sp_info->spawn_coment, sp_info->slot, newloadname );
                    }
                    continue;
                }
            }

            // we only reach this if everything was loaded properly
            activate_spawn_file_spawn( sp_info );

            //We might become the new parent
            if ( sp_info->attach == ATTACH_NONE ) parent = sp_info->parent;
        }

        vfs_close( fileread );
    }

    DisplayMsg_clear();

    // Make sure local players are displayed first
    statlist_sort();

    // Fix tilting trees problem
    tilt_characters_to_terrain();

    // done spawning
    activate_spawn_file_active = false;
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
    DisplayMsg_reset();
    chop_data_init( &chop_mem );
    game_reset_players();

    reset_end_text();
}

//--------------------------------------------------------------------------------------------
egolib_rv game_load_global_assets()
{
    // load a bunch of assets that are used in the module

    egolib_rv retval = rv_success;
    gfx_rv load_rv = gfx_success;

    load_rv = gfx_load_blips();
    switch ( load_rv )
    {
        case gfx_fail:   if ( rv_error != retval ) retval = rv_fail; break;
        case gfx_error:  retval = rv_error; break;
    }

    load_rv = gfx_load_bars();
    switch ( load_rv )
    {
        case gfx_fail:   if ( rv_error != retval ) retval = rv_fail; break;
        case gfx_error:  retval = rv_error; break;
    }

    load_rv = gfx_load_icons();
    switch ( load_rv )
    {
        case gfx_fail:   if ( rv_error != retval ) retval = rv_fail; break;
        case gfx_error:  retval = rv_error; break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void game_load_module_assets( const char *modname )
{
    // load a bunch of assets that are used in the module
    sound_load_global_waves_vfs();
    PrtList_reset_all();

    if ( NULL == read_wawalite_vfs() )
    {
        log_warning( "wawalite.txt not loaded for %s.\n", modname );
    }

    gfx_system_load_basic_textures();
    gfx_load_map();

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
    /// @author ZZ
    /// @details This runst the setup functions for a module

    STRING modname;

    // make sure the object lists are empty
    free_all_objects();

    // generate the module directory
    strncpy( modname, smallname, SDL_arraysize( modname ) );
    str_append_slash_net( modname, SDL_arraysize( modname ) );

    // just the information in these files to load the module
    Passages::loadAllPassages();         // read and implement the "passage script" passages.txt
    activate_spawn_file_vfs();           // read and implement the "spawn script" spawn.txt
    activate_alliance_file_vfs();        // set up the non-default team interactions

    // now load the profile AI, do last so that all reserved slot numbers are initialized
    game_load_profile_ai();
}

//--------------------------------------------------------------------------------------------
bool game_load_module_data( const char *smallname )
{
    /// @author ZZ
    /// @details This function loads a module

    egolib_rv mesh_BSP_retval;
    STRING modname;
    ego_mesh_t * pmesh_rv;

    // ensure that the script parser exists
    parser_state_t * ps = script_compiler_get_state();

    log_info( "Loading module \"%s\"\n", smallname );

    if ( load_ai_script_vfs( ps, "mp_data/script.txt", NULL, NULL ) != rv_success )
    {
        log_warning( "game_load_module_data() - cannot load the default script\n" );
        goto game_load_module_data_fail;
    }

    // generate the module directory
    strncpy( modname, smallname, SDL_arraysize( modname ) );
    str_append_slash( modname, SDL_arraysize( modname ) );

    // load all module assets
    game_load_all_assets( modname );

    // load all module objects
    game_load_all_profiles( modname );

    pmesh_rv = ego_mesh_load( modname, PMesh );
    if ( NULL == pmesh_rv )
    {
        // do not cause the program to fail, in case we are using a script function to load a module
        // just return a failure value and log a warning message for debugging purposes
        log_warning( "game_load_module_data() - Uh oh! Problems loading the mesh! (%s)\n", modname );

        goto game_load_module_data_fail;
    }

    // start the mesh_BSP_system
    if (!mesh_BSP_system_begin(pmesh_rv))
    {
        goto game_load_module_data_fail;
    }
    // if it is started, populate the map_BSP
    mesh_BSP_fill(getMeshBSP(), pmesh_rv);

    return true;

game_load_module_data_fail:

    // release any data that might have been allocated
    game_release_module_data();

    return false;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function makes sure a character has no attached particles

    PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
    {
        if ( prt_bdl.prt_ptr->attachedto_ref == character )
        {
            end_one_particle_in_game( prt_bdl.prt_ref );
        }
    }
    PRT_END_LOOP();

    if ( INGAME_CHR( character ) )
    {
        // Set the alert for disaffirmation ( wet torch )
        SET_BIT( ChrList.lst[character].ai.alert, ALERTIF_DISAFFIRMED );
    }
}

//--------------------------------------------------------------------------------------------
int number_of_attached_particles( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function returns the number of particles attached to the given character

    int     cnt = 0;

    PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
    {
        if ( prt_bdl.prt_ptr->attachedto_ref == character )
        {
            cnt++;
        }
    }
    PRT_END_LOOP();

    return cnt;
}

//--------------------------------------------------------------------------------------------
int reaffirm_attached_particles( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function makes sure a character has all of it's particles

    int     number_added, number_attached;
    int     amount, attempts;
    PRT_REF particle;
    chr_t * pchr;
    cap_t * pcap;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList_get_ptr( character );

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return 0;

    amount = pcap->attachedprt_amount;
    if ( 0 == amount ) return 0;

    number_attached = number_of_attached_particles( character );
    if ( number_attached >= amount ) return 0;

    number_added = 0;
    for ( attempts = 0; attempts < amount && number_attached < amount; attempts++ )
    {
        particle = spawn_one_particle( pchr->pos.v, pchr->ori.facing_z, pchr->profile_ref, pcap->attachedprt_lpip, character, GRIP_LAST + number_attached, chr_get_iteam( character ), character, INVALID_PRT_REF, number_attached, INVALID_CHR_REF );
        if ( DEFINED_PRT( particle ) )
        {
            prt_t * pprt = PrtList_get_ptr( particle );

            pprt = place_particle_at_vertex( pprt, character, pprt->attachedto_vrt_off );
            if ( NULL == pprt ) continue;

            number_added++;
            number_attached++;
        }
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    SET_BIT( pchr->ai.alert, ALERTIF_REAFFIRMED );

    return number_added;
}

//--------------------------------------------------------------------------------------------
void game_quit_module()
{
    /// @author BB
    /// @details all of the de-initialization code after the module actually ends

    // stop the module
    game_module_stop( PMod );

    // get rid of the game/module data
    game_release_module_data();

    // turn off networking
    egonet_close_session();

    // reset the "ui" mouse state
    input_cursor_reset();

    // re-initialize all game/module data
    game_reset_module_data();

    // finish whatever in-game song is playing
    sound_finish_sound();

    // remove the module-dependent mount points from the vfs
    setup_clear_module_vfs_paths();

    // turn off networking
    net_end();
}

//--------------------------------------------------------------------------------------------
bool game_begin_module( const char * modname, Uint32 seed )
{
    /// @author BB
    /// @details all of the initialization code before the module actually starts

    if ((( Uint32 )( ~0 ) ) == seed ) seed = ( Uint32 )time( NULL );

    // make sure the old game has been quit
    game_quit_module();

    // make sure that the object lists are in a good state
    reset_all_object_lists();

    // set up the virtual file system for the module
    if ( !setup_init_module_vfs_paths( modname ) ) return false;

    // load all the in-game module data
    srand( seed );
    if ( !game_load_module_data( modname ) )
    {
        game_module_stop( PMod );
        return false;
    };

    game_setup_module( modname );

    // make sure the per-module configuration settings are correct
    config_synch( &cfg, true );

    // initialize the game objects
    initialize_all_objects();
    input_cursor_reset();
    game_module_reset( PMod, seed );
    _cameraSystem.resetAll(PMesh);
    update_all_character_matrices();
    attach_all_particles();

    // log debug info for every object loaded into the module
    if ( cfg.dev_mode ) log_madused_vfs( "/debug/slotused.txt" );

    // initialize the network
    net_begin();
    net_sayHello();

    // start the module
    game_module_start( PMod );

    // initialize the timers as the very last thing
    timeron = false;
    game_reset_timers();

    return true;
}

//--------------------------------------------------------------------------------------------
bool game_update_imports()
{
    /// @author BB
    /// @details This function saves all the players to the players dir
    ///    and also copies them into the imports dir to prepare for the next module

    // save the players and their inventories to their original directory
    if ( PMod->exportvalid )
    {
        // export the players
        export_all_players( false );

        // update the import list
        import_list_from_players( &ImportList );
    }

    // erase the data in the import folder
    vfs_removeDirectoryAndContents( "import", VFS_TRUE );

    // copy the import data back into the import folder
    game_copy_imports( &ImportList );

    return true;
}

//--------------------------------------------------------------------------------------------
void game_release_module_data()
{
    /// @author ZZ
    /// @details This function frees up memory used by the module

    ego_mesh_t * ptmp;

    // Disable ESP
    local_stats.sense_enemies_idsz = IDSZ_NONE;
    local_stats.sense_enemies_team = ( TEAM_REF ) TEAM_MAX;

    // make sure that the object lists are cleared out
    free_all_objects();

    // deal with dynamically allocated game assets
    gfx_system_release_all_graphics();
    release_all_profiles();

    // free passages
    Passages::clearPassages();

    // delete the mesh data
    ptmp = PMesh;
    ego_mesh_destroy( &ptmp );

    // delete the mesh BSP data
    mesh_BSP_system_end();

    // restore the original statically allocated ego_mesh_t header
    PMesh = _mesh + 0;
}

//--------------------------------------------------------------------------------------------
bool attach_one_particle( prt_bundle_t * pbdl_prt )
{
    prt_t * pprt;
    chr_t * pchr;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;
    pprt = pbdl_prt->prt_ptr;

    if ( !INGAME_CHR( pbdl_prt->prt_ptr->attachedto_ref ) ) return false;
    pchr = ChrList_get_ptr( pbdl_prt->prt_ptr->attachedto_ref );

    pprt = place_particle_at_vertex( pprt, pprt->attachedto_ref, pprt->attachedto_vrt_off );
    if ( NULL == pprt ) return false;

    // the previous function can inactivate a particle
    if ( ACTIVE_PPRT( pprt ) )
    {
        // Correct facing so swords knock characters in the right direction...
        if ( NULL != pbdl_prt->pip_ptr && HAS_SOME_BITS( pbdl_prt->pip_ptr->damfx, DAMFX_TURN ) )
        {
            pprt->facing = pchr->ori.facing_z;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void attach_all_particles()
{
    /// @author ZZ
    /// @details This function attaches particles to their characters so everything gets
    ///    drawn right

    PRT_BEGIN_LOOP_DISPLAY( cnt, prt_bdl )
    {
        attach_one_particle( &prt_bdl );
    }
    PRT_END_LOOP()
}

//--------------------------------------------------------------------------------------------
bool add_player( const CHR_REF character, const PLA_REF player, input_device_t *pdevice )
{
    /// @author ZZ
    /// @details This function adds a player, returning false if it fails, true otherwise

    player_t * ppla = NULL;
    chr_t    * pchr = NULL;

    if ( !VALID_PLA_RANGE( player ) ) return false;
    ppla = PlaStack_get_ptr( player );

    // does the player already exist?
    if ( ppla->valid ) return false;

    // re-construct the players
    pla_reinit( ppla );

    if ( !DEFINED_CHR( character ) ) return false;
    pchr = ChrList_get_ptr( character );

    // set the reference to the player
    pchr->is_which_player = player;

    // download the quest info
    quest_log_download_vfs( ppla->quest_log, SDL_arraysize( ppla->quest_log ), chr_get_dir_name( character ) );

    //---- skeleton for using a ConfigFile to save quests
    // ppla->quest_file = quest_file_open( chr_get_dir_name(character) );

    ppla->index              = character;
    ppla->valid              = true;
    ppla->pdevice            = pdevice;

    if ( pdevice != NULL )
    {
        local_stats.noplayers = false;
        pchr->islocalplayer = true;
        local_stats.player_count++;
    }

    PlaStack.count++;

    return true;
}

//--------------------------------------------------------------------------------------------
void let_all_characters_think()
{
    /// @author ZZ
    /// @details This function funst the ai scripts for all eligible objects

    static Uint32 last_update = ( Uint32 )( ~0 );

    // make sure there is only one script update per game update
    if ( update_wld == last_update ) return;
    last_update = update_wld;

    blip_count = 0;

    CHR_BEGIN_LOOP_ACTIVE( character, pchr )
    {
        cap_t * pcap;

        bool is_crushed, is_cleanedup, can_think;

        pcap = chr_get_pcap( character );
        if ( NULL == pcap ) continue;

        // check for actions that must always be handled
        is_cleanedup = HAS_SOME_BITS( pchr->ai.alert, ALERTIF_CLEANEDUP );
        is_crushed   = HAS_SOME_BITS( pchr->ai.alert, ALERTIF_CRUSHED );

        // let the script run sometimes even if the item is in your backpack
        can_think = !INGAME_CHR( pchr->inwhich_inventory ) || pcap->isequipment;

        // only let dead/destroyed things think if they have beem crushed/cleanedup
        if (( pchr->alive && can_think ) || is_crushed || is_cleanedup )
        {
            // Figure out alerts that weren't already set
            set_alerts( character );

            // Cleaned up characters shouldn't be alert to anything else
            if ( is_cleanedup )  { pchr->ai.alert = ALERTIF_CLEANEDUP; /*pchr->ai.timer = update_wld + 1;*/ }

            // Crushed characters shouldn't be alert to anything else
            if ( is_crushed )  { pchr->ai.alert = ALERTIF_CRUSHED; pchr->ai.timer = update_wld + 1; }

            scr_run_chr_script( character );
        }
    }
    CHR_END_LOOP();
}

//--------------------------------------------------------------------------------------------
bool game_begin_menu( menu_process_t * mproc, which_menu_t which )
{
    if ( NULL == mproc ) return false;

    if ( !process_running( PROC_PBASE( mproc ) ) )
    {
        GProc->menu_depth = mnu_get_menu_depth();
    }

    if ( mnu_begin_menu( which ) )
    {
        process_start( PROC_PBASE( mproc ) );
    }

    return true;
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

    // restart the menu song
    sound_play_song( MENU_SONG, 0, -1 );

    // turn off networking
    net_end();
}

//--------------------------------------------------------------------------------------------
void free_all_objects()
{
    /// @author BB
    /// @details free every instance of the three object types used in the game.

    PrtList_free_all();
    EncList_free_all();
    free_all_chraracters();
}

//--------------------------------------------------------------------------------------------
void reset_all_object_lists()
{
    PrtList_reinit();
    ChrList_reinit();
    EncList_reinit();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mesh_t * set_PMesh( ego_mesh_t * pmpd )
{
    ego_mesh_t * pmpd_old = PMesh;

    PMesh = pmpd;

    return pmpd_old;
}

//--------------------------------------------------------------------------------------------
float get_mesh_level( ego_mesh_t * pmesh, float x, float y, bool waterwalk )
{
    /// @author ZZ
    /// @details This function returns the height of a point within a mesh fan, precise
    ///    If waterwalk is nonzero and the fan is watery, then the level returned is the
    ///    level of the water.

    float zdone;

    zdone = ego_mesh_get_level( pmesh, x, y );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = ego_mesh_get_grid( pmesh, x, y );

        if ( 0 != ego_mesh_test_fx( pmesh, tile, MAPFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
void reset_end_text()
{
    /// @author ZZ
    /// @details This function resets the end-module text

    endtext_carat = snprintf( endtext, SDL_arraysize( endtext ), "The game has ended..." );

    /*
    if ( PlaStack.count > 1 )
    {
        endtext_carat = snprintf( endtext, SDL_arraysize( endtext), "Sadly, they were never heard from again..." );
    }
    else
    {
        if ( 0 == PlaStack.count )
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
void expand_escape_codes( const CHR_REF ichr, script_state_t * pstate, char * src, char * src_end, char * dst, char * dst_end )
{
    int    cnt;
    STRING szTmp;

    chr_t      * pchr, *ptarget, *powner;
    ai_state_t * pai;

    pchr    = !INGAME_CHR( ichr ) ? NULL : ChrList_get_ptr( ichr );
    pai     = ( NULL == pchr )    ? NULL : &( pchr->ai );

    ptarget = (( NULL == pai ) || !INGAME_CHR( pai->target ) ) ? pchr : ChrList_get_ptr( pai->target );
    powner  = (( NULL == pai ) || !INGAME_CHR( pai->owner ) ) ? pchr : ChrList_get_ptr( pai->owner );

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
                        chr_get_name( ichr, CHRNAME_ARTICLE, szTmp, SDL_arraysize( szTmp ) );
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
                            chr_get_name( pai->target, CHRNAME_ARTICLE, szTmp, SDL_arraysize( szTmp ) );
                        }
                    }
                    break;

                case 'o':  // Owner name
                    {
                        if ( NULL != pai )
                        {
                            chr_get_name( pai->owner, CHRNAME_ARTICLE, szTmp, SDL_arraysize( szTmp ) );
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
                            ebuffer = chr_get_pcap( pai->target )->skin_info.name[( *src )-'0'];
                            ebuffer_end = ebuffer + SDL_arraysize( chr_get_pcap( pai->target )->skin_info.name[( *src )-'0'] );
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
            if ( 0 == cnt && NULL != ebuffer )  *ebuffer = char_toupper(( unsigned )( *ebuffer ) );

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
bool game_choose_module( int imod, int seed )
{
    bool retval;

    if ( seed < 0 ) seed = ( int )time( NULL );

    if ( NULL == PMod ) PMod = &_gmod;

    retval = game_module_setup( PMod, mnu_ModList_get_base( imod ), mnu_ModList_get_vfs_path( imod ), seed );

    if ( retval )
    {
        // give everyone virtual access to the game directories
        setup_init_module_vfs_paths( pickedmodule_path );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
game_process_t * game_process_init( game_process_t * gproc )
{
    if ( NULL == gproc ) return NULL;

    BLANK_STRUCT_PTR( gproc )

    process_init( PROC_PBASE( gproc ) );

    gproc->menu_depth = -1;
    gproc->pause_key_ready = true;

    // initialize all the profile variables
    PROFILE_INIT( game_update_loop );
    PROFILE_INIT( game_single_update );
    PROFILE_INIT( gfx_loop );

    PROFILE_INIT( talk_to_remotes );
    PROFILE_INIT( egonet_listen_for_packets );
    PROFILE_INIT( check_stats );
    PROFILE_INIT( set_local_latches );
    PROFILE_INIT( cl_talkToHost );

    return gproc;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void do_game_hud()
{
    int y = 0;

    if ( gfx_flip_pages_requested() && cfg.dev_mode )
    {
        GL_DEBUG( glColor4fv )( white_vec );

        if ( fpson )
        {
            y = draw_string( 0, y, "%2.3f FPS, %2.3f UPS", stabilized_fps, stabilized_game_ups );
            y = draw_string( 0, y, "estimated max FPS %2.3f", est_max_fps ); \
        }

        y = draw_string( 0, y, "Menu time %f", MProc->base.frameDuration );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void game_reset_players()
{
    /// @author ZZ
    /// @details This function clears the player list data

    // Reset the local data stuff
    local_stats.allpladead = false;

    local_stats.seeinvis_level = 0.0f;
    local_stats.seeinvis_level = 0.0f;
    local_stats.seekurse_level = 0.0f;
    local_stats.seedark_level  = 0.0f;
    local_stats.grog_level     = 0.0f;
    local_stats.daze_level     = 0.0f;

    local_stats.sense_enemies_team = ( TEAM_REF ) TEAM_MAX;
    local_stats.sense_enemies_idsz = IDSZ_NONE;

    PlaStack_reset_all();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool upload_water_layer_data( water_instance_layer_t inst[], const wawalite_water_layer_t data[], const int layer_count )
{
    int layer;

    if ( NULL == inst || 0 == layer_count ) return false;

    for ( layer = 0; layer < layer_count; layer++ )
    {
        BLANK_STRUCT_PTR( inst + layer );
    }

    // set the frame
    for ( layer = 0; layer < layer_count; layer++ )
    {
        inst[layer].frame = ( Uint16 )generate_randmask( 0 , WATERFRAMEAND );
    }

    if ( NULL != data )
    {
        for ( layer = 0; layer < layer_count; layer++ )
        {
            const wawalite_water_layer_t * pwawa  = data + layer;
            water_instance_layer_t       * player = inst + layer;

            player->z         = pwawa->z;
            player->amp       = pwawa->amp;

            player->dist      = pwawa->dist;

            player->light_dir = pwawa->light_dir / 63.0f;
            player->light_add = pwawa->light_add / 63.0f;

            player->tx_add    = pwawa->tx_add;

            player->alpha     = pwawa->alpha;

            player->frame_add = pwawa->frame_add;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_weather_data( weather_instance_t * pinst, const wawalite_weather_t * pdata )
{
    if ( NULL == pinst ) return false;

    BLANK_STRUCT_PTR( pinst )

    // set a default value
    pinst->timer_reset = 10;

    if ( NULL != pdata )
    {
        // copy the data
        pinst->timer_reset = pdata->timer_reset;
        pinst->over_water  = pdata->over_water;
        pinst->part_gpip   = pdata->part_gpip;
    }

    // set the new data
    pinst->time = pinst->timer_reset;

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_fog_data( fog_instance_t * pinst, const wawalite_fog_t * pdata )
{
    if ( NULL == pinst ) return false;

    BLANK_STRUCT_PTR( pinst )

    if ( NULL != pdata )
    {
        pinst->on     = pdata->found && pinst->on && cfg.fog_allowed;
        pinst->top    = pdata->top;
        pinst->bottom = pdata->bottom;

        pinst->red    = pdata->red * 0xFF;
        pinst->grn    = pdata->grn * 0xFF;
        pinst->blu    = pdata->blu * 0xFF;
    }

    pinst->distance = ( pdata->top - pdata->bottom );
    pinst->on       = ( pinst->distance < 1.0f ) && pinst->on;

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_damagetile_data( damagetile_instance_t * pinst, const wawalite_damagetile_t * pdata )
{
    if ( NULL == pinst ) return false;

    BLANK_STRUCT_PTR( pinst )

    //pinst->sound_time   = TILESOUNDTIME;
    //pinst->min_distance = 9999;

    if ( NULL != pdata )
    {
        pinst->amount.base  = pdata->amount;
        pinst->amount.rand  = 1;
        pinst->damagetype   = pdata->damagetype;

        pinst->part_gpip    = pdata->part_gpip;
        pinst->partand      = pdata->partand;
        pinst->sound_index  = CLIP( pdata->sound_index, INVALID_SOUND, MAX_WAVE );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_animtile_data( animtile_instance_t inst[], const wawalite_animtile_t * pdata, const size_t animtile_count )
{
    Uint32 cnt;

    if ( NULL == inst || 0 == animtile_count ) return false;

    BLANK_STRUCT_PTR( inst )

    for ( cnt = 0; cnt < animtile_count; cnt++ )
    {
        inst[cnt].frame_and  = ( 1 << ( cnt + 2 ) ) - 1;
        inst[cnt].base_and   = ~inst[cnt].frame_and;
        inst[cnt].frame_add  = 0;
    }

    if ( NULL != pdata )
    {
        inst[0].update_and = pdata->update_and;
        inst[0].frame_and  = pdata->frame_and;
        inst[0].base_and   = ~inst[0].frame_and;

        for ( cnt = 1; cnt < animtile_count; cnt++ )
        {
            inst[cnt].update_and = pdata->update_and;
            inst[cnt].frame_and  = ( inst[cnt-1].frame_and << 1 ) | 1;
            inst[cnt].base_and   = ~inst[cnt].frame_and;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_light_data( const wawalite_data_t * pdata )
{
    if ( NULL == pdata ) return false;

    // upload the lighting data
    light_nrm[kX] = pdata->light_x;
    light_nrm[kY] = pdata->light_y;
    light_nrm[kZ] = pdata->light_z;
    light_a = pdata->light_a;

    if ( ABS( light_nrm[kX] ) + ABS( light_nrm[kY] ) + ABS( light_nrm[kZ] ) > 0.0f )
    {
        float fTmp = std::sqrt( light_nrm[kX] * light_nrm[kX] + light_nrm[kY] * light_nrm[kY] + light_nrm[kZ] * light_nrm[kZ] );

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

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_phys_data( const wawalite_physics_t * pdata )
{
    if ( NULL == pdata ) return false;

    // upload the physics data
    hillslide      = pdata->hillslide;
    slippyfriction = pdata->slippyfriction;
    noslipfriction = pdata->noslipfriction;
    airfriction    = pdata->airfriction;
    waterfriction  = pdata->waterfriction;
    gravity        = pdata->gravity;

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_graphics_data( const wawalite_graphics_t * pdata )
{
    if ( NULL == pdata ) return false;

    // Read extra data
    gfx.exploremode = pdata->exploremode;
    gfx.usefaredge  = pdata->usefaredge;

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_camera_data( const wawalite_camera_t * pdata )
{
    if ( NULL == pdata ) return false;

    _cameraSystem.getCameraOptions().swing     = pdata->swing;
    _cameraSystem.getCameraOptions().swingRate = pdata->swing_rate;
    _cameraSystem.getCameraOptions().swingAmp  = pdata->swing_amp;

    return true;
}

//--------------------------------------------------------------------------------------------
void upload_wawalite()
{
    /// @author ZZ
    /// @details This function sets up water and lighting for the module

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
bool game_module_setup( game_module_t * pinst, const mod_file_t * pdata, const char * loadname, const Uint32 seed )
{
    //Prepeares a module to be played
    if ( NULL == pdata ) return false;

    if ( !game_module_init( pinst ) ) return false;

    pinst->importamount   = pdata->importamount;
    pinst->exportvalid    = pdata->allowexport;
    pinst->exportreset    = pdata->allowexport;
    pinst->playeramount   = pdata->maxplayers;
    pinst->importvalid    = ( pinst->importamount > 0 );
    pinst->respawnvalid   = ( false != pdata->respawnvalid );
    pinst->respawnanytime = ( RESPAWN_ANYTIME == pdata->respawnvalid );

    strncpy( pinst->loadname, loadname, SDL_arraysize( pinst->loadname ) );

    pinst->active = false;
    pinst->beat   = false;
    pinst->seed   = seed;

    return true;
}

//--------------------------------------------------------------------------------------------
bool game_module_init( game_module_t * pinst )
{
    if ( NULL == pinst ) return false;

    BLANK_STRUCT_PTR( pinst )

    pinst->seed = ( Uint32 )~0;

    return true;
}

//--------------------------------------------------------------------------------------------
bool game_module_reset( game_module_t * pinst, const Uint32 seed )
{
    if ( NULL == pinst ) return false;

    pinst->beat        = false;
    pinst->exportvalid = pinst->exportreset;
    pinst->seed        = seed;

    return true;
}

//--------------------------------------------------------------------------------------------
bool game_module_start( game_module_t * pinst )
{
    /// @author BB
    /// @details Let the module go

    if ( NULL == pinst ) return false;

    pinst->active = true;

    srand( pinst->seed );
    pinst->randsave = rand();
    randindex = rand() % RANDIE_COUNT;

    egonet_set_hostactive( true ); // very important or the input will not work

    return true;
}

//--------------------------------------------------------------------------------------------
bool game_module_stop( game_module_t * pinst )
{
    /// @author BB
    /// @details stop the module

    if ( NULL == pinst ) return false;

    pinst->active      = false;

    // network stuff
    egonet_set_hostactive( false );

    return true;
}

//--------------------------------------------------------------------------------------------
wawalite_data_t * read_wawalite_vfs( void /* const char *modname */ )
{
    wawalite_data_t * pdata;

    // if( INVALID_CSTR(modname) ) return NULL;

    pdata = read_wawalite_file_vfs( "mp_data/wawalite.txt", NULL );
    if ( NULL == pdata ) return NULL;

    memcpy( &wawalite_data, pdata, sizeof( wawalite_data_t ) );

    // fix any out-of-bounds data
    wawalite_limit( &wawalite_data );

    // finish up any data that has to be calculated
    wawalite_finalize( &wawalite_data );

    return &wawalite_data;
}

//--------------------------------------------------------------------------------------------
bool wawalite_finalize( wawalite_data_t * pdata )
{
    /// @author BB
    /// @details coerce all parameters to in-bounds values

    int waterspeed_count, windspeed_count;

    wawalite_water_layer_t * ilayer;

    if ( NULL == pdata ) return false;

    //No weather?
    if ( 0 == strcmp( pdata->weather.weather_name, "NONE" ) )
    {
        pdata->weather.part_gpip = -1;
    }
    else
    {
        STRING prt_file, prt_end_file, line;
        bool success;

        strncpy( line, pdata->weather.weather_name, SDL_arraysize( line ) );

        //prepeare the load paths
        snprintf( prt_file, SDL_arraysize( prt_file ), "mp_data/weather_%s.txt", strlwr( line ) );
        snprintf( prt_end_file, SDL_arraysize( prt_end_file ), "mp_data/weather_%s_finish.txt", strlwr( line ) );

        //try to load the particle files, we need at least the first particle for weather to work
        success = INVALID_PIP_REF != PipStack_load_one( prt_file, ( PIP_REF )PIP_WEATHER );
        PipStack_load_one( prt_end_file, ( PIP_REF )PIP_WEATHER_FINISH );

        //Unknown weather parsed
        if ( !success )
        {
            log_warning( "Failed to load weather type from wawalite.txt: %s - (%s)\n", line, prt_file );
            pdata->weather.part_gpip = -1;
            strncpy( pdata->weather.weather_name, "NONE", SDL_arraysize( pdata->weather.weather_name ) );
        }
    }

    windspeed_count = 0;
    fvec3_self_clear( windspeed.v );

    waterspeed_count = 0;
    fvec3_self_clear( waterspeed.v );

    ilayer = wawalite_data.water.layer + 0;
    if ( wawalite_data.water.background_req )
    {
        // this is a bit complicated.
        // it is the best I can do at reverse engineering what I did in render_world_background()

        const float cam_height = 1500.0f;
        const float default_bg_repeat = 4.0f;

        windspeed_count++;

        windspeed.x += -ilayer->tx_add.x * GRID_FSIZE / ( wawalite_data.water.backgroundrepeat / default_bg_repeat ) * ( cam_height + 1.0f / ilayer->dist.x ) / cam_height;
        windspeed.y += -ilayer->tx_add.y * GRID_FSIZE / ( wawalite_data.water.backgroundrepeat / default_bg_repeat ) * ( cam_height + 1.0f / ilayer->dist.y ) / cam_height;
        windspeed.z += -0;
    }
    else
    {
        waterspeed_count++;

        waterspeed.x += -ilayer->tx_add.x * GRID_FSIZE;
        waterspeed.y += -ilayer->tx_add.y * GRID_FSIZE;
        waterspeed.z += -0;
    }

    ilayer = wawalite_data.water.layer + 1;
    if ( wawalite_data.water.overlay_req )
    {
        windspeed_count++;

        windspeed.x += -600 * ilayer->tx_add.x * GRID_FSIZE / wawalite_data.water.foregroundrepeat * 0.04f;
        windspeed.y += -600 * ilayer->tx_add.y * GRID_FSIZE / wawalite_data.water.foregroundrepeat * 0.04f;
        windspeed.z += -0;
    }
    else
    {
        waterspeed_count++;

        waterspeed.x += -ilayer->tx_add.x * GRID_FSIZE;
        waterspeed.y += -ilayer->tx_add.y * GRID_FSIZE;
        waterspeed.z += -0;
    }

    if ( waterspeed_count > 1 )
    {
        waterspeed.x /= ( float )waterspeed_count;
        waterspeed.y /= ( float )waterspeed_count;
        waterspeed.z /= ( float )waterspeed_count;
    }

    if ( windspeed_count > 1 )
    {
        windspeed.x /= ( float )windspeed_count;
        windspeed.y /= ( float )windspeed_count;
        windspeed.z /= ( float )windspeed_count;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_vfs( const wawalite_data_t * pdata )
{
    /// @author BB
    /// @details Prepare and write the wawalite file

    if ( NULL == pdata ) return false;

    return write_wawalite_file_vfs( pdata );
}

//--------------------------------------------------------------------------------------------
Uint8 get_alpha( int alpha, float seeinvis_mag )
{
    // This is a bit of a kludge, but it should allow the characters to see
    // completely invisible objects as SEEINVISIBLE if their level is high enough.
    // AND it should make mostly invisible objects approach SEEINVISIBLE
    // BUT objects that are already more visible than SEEINVISIBLE can be made fully visible
    // with a large enough level

    if ( 1.0f != seeinvis_mag )
    {
        if ( 0 == alpha )
        {
            if ( seeinvis_mag > 1.0f )
            {
                alpha = SEEINVISIBLE * ( 1.0f - 1.0f / seeinvis_mag );
            }
        }
        else if ( alpha < SEEINVISIBLE )
        {
            alpha *= seeinvis_mag;
            alpha = std::max( alpha, SEEINVISIBLE );
        }
        else
        {
            alpha *= seeinvis_mag;
        }
    }

    return CLIP( alpha, 0, 255 );
}

//--------------------------------------------------------------------------------------------
Uint8 get_light( int light, float seedark_mag )
{
    // ZF> Why should Darkvision reveal invisible?
    // BB> This modification makes the character's light (i.e how much it glows)
    //     more visible to characters with darkvision. This does not affect an object's
    //     alpha, which is what makes somethig invisible. If the object is glowing AND invisible,
    //     darkvision should make it more visible

    // is this object NOT glowing?
    if ( light >= 0xFF ) return 0xFF;

    if ( seedark_mag != 1.0f )
    {
        light *= seedark_mag;
    }

    return CLIP( light, 0, 0xFE );
}

//--------------------------------------------------------------------------------------------
bool do_shop_drop( const CHR_REF idropper, const CHR_REF iitem )
{
    chr_t * pdropper, * pitem;
    bool inshop;

    if ( !INGAME_CHR( iitem ) ) return false;
    pitem = ChrList_get_ptr( iitem );

    if ( !INGAME_CHR( idropper ) ) return false;
    pdropper = ChrList_get_ptr( idropper );

    inshop = false;
    if ( pitem->isitem )
    {
        CHR_REF iowner;

        iowner = Passages::getShopOwner(pitem->pos.x, pitem->pos.y);
        if ( INGAME_CHR( iowner ) )
        {
            int price;
            chr_t * powner = ChrList_get_ptr( iowner );

            inshop = true;

            price = chr_get_price( iitem );

            // Are they are trying to sell junk or quest items?
            if ( 0 == price )
            {
                ai_add_order( &( powner->ai ), ( Uint32 ) price, Passage::SHOP_BUY );
            }
            else
            {
                pdropper->money  = pdropper->money + price;
                pdropper->money  = CLIP( pdropper->money, (Sint16)0, (Sint16)MAXMONEY );

                powner->money  = powner->money - price;
                powner->money  = CLIP( powner->money, (Sint16)0, (Sint16)MAXMONEY );

                ai_add_order( &( powner->ai ), ( Uint32 ) price, Passage::SHOP_BUY );
            }
        }
    }

    return inshop;
}

//--------------------------------------------------------------------------------------------
bool do_shop_buy( const CHR_REF ipicker, const CHR_REF iitem )
{
    bool can_grab, can_pay, in_shop;
    int price;

    chr_t * ppicker, * pitem;

    if ( !INGAME_CHR( iitem ) ) return false;
    pitem = ChrList_get_ptr( iitem );

    if ( !INGAME_CHR( ipicker ) ) return false;
    ppicker = ChrList_get_ptr( ipicker );

    can_grab = true;
    can_pay  = true;
    in_shop  = false;

    if ( pitem->isitem )
    {
        CHR_REF iowner;

        iowner = Passages::getShopOwner( pitem->pos.x, pitem->pos.y );
        if ( INGAME_CHR( iowner ) )
        {
            chr_t * powner = ChrList_get_ptr( iowner );

            in_shop = true;
            price   = chr_get_price( iitem );

            if ( ppicker->money >= price )
            {
                // Okay to sell
                ai_add_order( &( powner->ai ), ( Uint32 ) price, Passage::SHOP_SELL );

                ppicker->money  = ppicker->money - price;
                ppicker->money  = CLIP( (int)ppicker->money, 0, MAXMONEY );

                powner->money   = powner->money + price;
                powner->money   = CLIP( (int)powner->money, 0, MAXMONEY );

                can_grab = true;
                can_pay  = true;
            }
            else
            {
                // Don't allow purchase
                ai_add_order( &( powner->ai ), price, Passage::SHOP_NOAFFORD );
                can_grab = false;
                can_pay  = false;
            }
        }
    }

    /// @note some of these are handled in scripts, so they could be disabled
    // print some feedback messages
    /*if( can_grab )
    {
        if( in_shop )
        {
            if( can_pay )
            {
                DisplayMsg_printf( "%s bought %s", chr_get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name( iitem, CHRNAME_ARTICLE ) );
            }
            else
            {
                DisplayMsg_printf( "%s can't afford %s", chr_get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name( iitem, CHRNAME_ARTICLE ) );
            }
        }
        else
        {
            DisplayMsg_printf( "%s picked up %s", chr_get_name( ipicker, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL), chr_get_name( iitem, CHRNAME_ARTICLE ) );
        }
    }*/

    return can_grab;
}

//--------------------------------------------------------------------------------------------
bool do_shop_steal( const CHR_REF ithief, const CHR_REF iitem )
{
    // Pets can try to steal in addition to invisible characters

    bool can_steal;

    chr_t * pthief, * pitem;

    if ( !INGAME_CHR( iitem ) ) return false;
    pitem = ChrList_get_ptr( iitem );

    if ( !INGAME_CHR( ithief ) ) return false;
    pthief = ChrList_get_ptr( ithief );

    can_steal = true;
    if ( pitem->isitem )
    {
        CHR_REF iowner;

        iowner = Passages::getShopOwner( pitem->pos.x, pitem->pos.y );
        if ( INGAME_CHR( iowner ) )
        {
            IPair  tmp_rand = {1, 100};
            int  detection;
            chr_t * powner = ChrList_get_ptr( iowner );

            detection = generate_irand_pair( tmp_rand );

            can_steal = true;
            if ( chr_can_see_object( powner, pthief ) || detection <= 5 || ( detection - ( pthief->dexterity >> 7 ) + ( powner->wisdom >> 7 ) ) > 50 )
            {
                ai_add_order( &( powner->ai ), Passage::SHOP_STOLEN, Passage::SHOP_THEFT );
                powner->ai.target = ithief;
                can_steal = false;
            }
        }
    }

    return can_steal;
}

//--------------------------------------------------------------------------------------------
bool can_grab_item_in_shop( const CHR_REF ichr, const CHR_REF iitem )
{
    bool can_grab;
    bool is_invis, can_steal;
    chr_t * pchr, * pitem, *pkeeper;
    CHR_REF shop_keeper;

    if ( !INGAME_CHR( ichr ) ) return false;
    pchr = ChrList_get_ptr( ichr );

    if ( !INGAME_CHR( iitem ) ) return false;
    pitem = ChrList_get_ptr( iitem );

    // assume that there is no shop so that the character can grab anything
    can_grab = true;

    // check if we are doing this inside a shop
    shop_keeper = Passages::getShopOwner(pitem->pos.x, pitem->pos.y);
    pkeeper = ChrList_get_ptr( shop_keeper );
    if ( INGAME_PCHR( pkeeper ) )
    {

        // check for a stealthy pickup
        is_invis  = !chr_can_see_object( pkeeper, pchr );

        // pets are automatically stealthy
        can_steal = is_invis || pchr->isitem;

        if ( can_steal )
        {
            can_grab = do_shop_steal( ichr, iitem );

            if ( !can_grab )
            {
                DisplayMsg_printf( "%s was detected!!", chr_get_name( ichr, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
            }
            else
            {
                DisplayMsg_printf( "%s stole %s", chr_get_name( ichr, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ), chr_get_name( iitem, CHRNAME_ARTICLE, NULL, 0 ) );
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
float get_mesh_max_vertex_0( ego_mesh_t * pmesh, int grid_x, int grid_y, bool waterwalk )
{
    float zdone = ego_mesh_get_max_vertex_0( pmesh, grid_x, grid_y );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = ego_mesh_get_grid( pmesh, grid_x, grid_y );

        if ( 0 != ego_mesh_test_fx( pmesh, tile, MAPFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_1( ego_mesh_t * pmesh, int grid_x, int grid_y, oct_bb_t * pbump, bool waterwalk )
{
    float zdone = ego_mesh_get_max_vertex_1( pmesh, grid_x, grid_y, pbump->mins[OCT_X], pbump->mins[OCT_Y], pbump->maxs[OCT_X], pbump->maxs[OCT_Y] );

    if ( waterwalk && water.surface_level > zdone && water.is_water )
    {
        int tile = ego_mesh_get_grid( pmesh, grid_x, grid_y );

        if ( 0 != ego_mesh_test_fx( pmesh, tile, MAPFX_WATER ) )
        {
            zdone = water.surface_level;
        }
    }

    return zdone;
}

//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_2( ego_mesh_t * pmesh, chr_t * pchr )
{
    /// @author BB
    /// @details the object does not overlap a single grid corner. Check the 4 corners of the collision volume

    int corner;
    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    float pos_x[4];
    float pos_y[4];
    float zmax;

    for ( corner = 0; corner < 4; corner++ )
    {
        pos_x[corner] = pchr->pos.x + (( 0 == ix_off[corner] ) ? pchr->chr_min_cv.mins[OCT_X] : pchr->chr_min_cv.maxs[OCT_X] );
        pos_y[corner] = pchr->pos.y + (( 0 == iy_off[corner] ) ? pchr->chr_min_cv.mins[OCT_Y] : pchr->chr_min_cv.maxs[OCT_Y] );
    }

    zmax = get_mesh_level( pmesh, pos_x[0], pos_y[0], pchr->waterwalk );
    for ( corner = 1; corner < 4; corner++ )
    {
        float fval = get_mesh_level( pmesh, pos_x[corner], pos_y[corner], pchr->waterwalk );
        zmax = std::max( zmax, fval );
    }

    return zmax;
}

//--------------------------------------------------------------------------------------------
float get_chr_level( ego_mesh_t * pmesh, chr_t * pchr )
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
    if ( 0.0f == pchr->bump_stt.size )
    {
        return get_mesh_level( pmesh, pchr->pos.x, pchr->pos.y, pchr->waterwalk );
    }

    // otherwise, use the small collision volume to determine which tiles the object overlaps
    // move the collision volume so that it surrounds the object
    oct_bb_add_fvec3( &( pchr->chr_min_cv ), pchr->pos.v, &bump );

    // determine the size of this object in tiles
    ixmin = bump.mins[OCT_X] / GRID_FSIZE; ixmin = CLIP( ixmin, 0, pmesh->info.tiles_x - 1 );
    ixmax = bump.maxs[OCT_X] / GRID_FSIZE; ixmax = CLIP( ixmax, 0, pmesh->info.tiles_x - 1 );

    iymin = bump.mins[OCT_Y] / GRID_FSIZE; iymin = CLIP( iymin, 0, pmesh->info.tiles_y - 1 );
    iymax = bump.maxs[OCT_Y] / GRID_FSIZE; iymax = CLIP( iymax, 0, pmesh->info.tiles_y - 1 );

    // do the simplest thing if the object is just on one tile
    if ( ixmax == ixmin && iymax == iymin )
    {
        return get_mesh_max_vertex_2( pmesh, pchr );
    }

    // otherwise, make up a list of tiles that the object might overlap
    for ( iy = iymin; iy <= iymax; iy++ )
    {
        float grid_y = iy * GRID_ISIZE;

        for ( ix = ixmin; ix <= ixmax; ix++ )
        {
            float ftmp;
            int   itile;
            float grid_x = ix * GRID_ISIZE;

            ftmp = grid_x + grid_y;
            if ( ftmp < bump.mins[OCT_XY] || ftmp > bump.maxs[OCT_XY] ) continue;

            ftmp = -grid_x + grid_y;
            if ( ftmp < bump.mins[OCT_YX] || ftmp > bump.maxs[OCT_YX] ) continue;

            itile = ego_mesh_get_tile_int( pmesh, ix, iy );
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
            zmax = std::max( zmax, fval );
        }
    }

    if ( zmax == -1e6 ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
void disenchant_character( const CHR_REF cnt )
{
    /// @author ZZ
    /// @details This function removes all enchantments from a character

    chr_t * pchr;
    size_t ienc_count;

    if ( !ALLOCATED_CHR( cnt ) ) return;
    pchr = ChrList_get_ptr( cnt );

    ienc_count = 0;
    while ( VALID_ENC_RANGE( pchr->firstenchant ) && ( ienc_count < MAX_ENC ) )
    {
        // do not let disenchant_character() get stuck in an infinite loop if there is an error
        if ( !remove_enchant( pchr->firstenchant, &( pchr->firstenchant ) ) )
        {
            break;
        }
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

}

//--------------------------------------------------------------------------------------------
void cleanup_character_enchants( chr_t * pchr )
{
    if ( NULL == pchr ) return;

    // clean up the enchant list
    pchr->firstenchant = cleanup_enchant_list( pchr->firstenchant, &( pchr->firstenchant ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool attach_chr_to_platform( chr_t * pchr, chr_t * pplat )
{
    /// @author BB
    /// @details attach a character to a platform
    ///
    /// @note the function move_one_character_get_environment() has already been called from within the
    ///  move_one_character() function, so the environment has already been determined this round

    cap_t * pchr_cap;
    fvec3_t   platform_up = VECT3( 0, 0, 1 );

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PCHR( pchr ) ) return false;
    if ( !ACTIVE_PCHR( pplat ) ) return false;

    pchr_cap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pchr_cap ) return false;

    // check if they can be connected
    if ( !pchr_cap->canuseplatforms || ( 0 != pchr->flyheight ) ) return false;
    if ( !pplat->platform ) return false;

    // do the attachment
    pchr->onwhichplatform_ref    = GET_REF_PCHR( pplat );
    pchr->onwhichplatform_update = update_wld;
    pchr->targetplatform_ref     = INVALID_CHR_REF;

    // update the character's relationship to the ground
    pchr->enviro.level     = std::max( pchr->enviro.floor_level, pplat->pos.z + pplat->chr_min_cv.maxs[OCT_Z] );
    pchr->enviro.zlerp     = ( pchr->pos.z - pchr->enviro.level ) / PLATTOLERANCE;
    pchr->enviro.zlerp     = CLIP( pchr->enviro.zlerp, 0.0f, 1.0f );
    pchr->enviro.grounded  = ( 0 == pchr->flyheight ) && ( pchr->enviro.zlerp < 0.25f );

    pchr->enviro.fly_level = std::max( pchr->enviro.fly_level, pchr->enviro.level );
    if ( 0 != pchr->flyheight )
    {
        if ( pchr->enviro.fly_level < 0 ) pchr->enviro.fly_level = 0;  // fly above pits...
    }

    // add the weight to the platform based on the new zlerp
    pplat->holdingweight += pchr->phys.weight * ( 1.0f - pchr->enviro.zlerp );

    // update the character jumping
    pchr->jumpready = pchr->enviro.grounded;
    if ( pchr->jumpready )
    {
        pchr->jumpnumber = pchr->jumpnumberreset;
    }

    // what to do about traction if the platform is tilted... hmmm?
    chr_getMatUp( pplat, platform_up.v );
    fvec3_self_normalize( platform_up.v );

    pchr->enviro.traction = ABS( platform_up.z ) * ( 1.0f - pchr->enviro.zlerp ) + 0.25f * pchr->enviro.zlerp;

    // tell the platform that we bumped into it
    // this is necessary for key buttons to work properly, for instance
    ai_state_set_bumplast( &( pplat->ai ), GET_REF_PCHR( pchr ) );

    return true;
}

//--------------------------------------------------------------------------------------------
bool detach_character_from_platform( chr_t * pchr )
{
    /// @author BB
    /// @details attach a character to a platform
    ///
    /// @note the function move_one_character_get_environment() has already been called from within the
    ///  move_one_character() function, so the environment has already been determined this round

    cap_t * pchr_cap;
    CHR_REF old_platform_ref;
    chr_t * old_platform_ptr;
    float   old_level, old_zlerp;

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PCHR( pchr ) ) return false;

    pchr_cap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pchr_cap ) return false;

    // save some values
    old_platform_ref = pchr->onwhichplatform_ref;
    old_level        = pchr->enviro.level;
    old_platform_ptr = NULL;
    old_zlerp        = pchr->enviro.zlerp;
    if ( INGAME_CHR( old_platform_ref ) )
    {
        old_platform_ptr = ChrList_get_ptr( old_platform_ref );
    }

    // undo the attachment
    pchr->onwhichplatform_ref    = INVALID_CHR_REF;
    pchr->onwhichplatform_update = 0;
    pchr->targetplatform_ref     = INVALID_CHR_REF;
    pchr->targetplatform_level   = -1e32;

    // adjust the platform weight, if necessary
    if ( NULL != old_platform_ptr )
    {
        old_platform_ptr->holdingweight -= pchr->phys.weight * ( 1.0f - old_zlerp );
    }

    // update the character-platform properties
    move_one_character_get_environment( pchr );

    // update the character jumping
    pchr->jumpready = pchr->enviro.grounded;
    if ( pchr->jumpready )
    {
        pchr->jumpnumber = pchr->jumpnumberreset;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool attach_prt_to_platform( prt_t * pprt, chr_t * pplat )
{
    /// @author BB
    /// @details attach a particle to a platform

    pip_t   * pprt_pip;

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PPRT( pprt ) ) return false;
    if ( !ACTIVE_PCHR( pplat ) ) return false;

    pprt_pip = prt_get_ppip( GET_REF_PPRT( pprt ) );
    if ( NULL == pprt_pip ) return false;

    // check if they can be connected
    if ( !pplat->platform ) return false;

    // do the attachment
    pprt->onwhichplatform_ref    = GET_REF_PCHR( pplat );
    pprt->onwhichplatform_update = update_wld;
    pprt->targetplatform_ref     = INVALID_CHR_REF;

    // update the character's relationship to the ground
    prt_set_level( pprt, std::max( pprt->enviro.level, pplat->pos.z + pplat->chr_min_cv.maxs[OCT_Z] ) );

    return true;
}

//--------------------------------------------------------------------------------------------
bool detach_particle_from_platform( prt_t * pprt )
{
    /// @author BB
    /// @details attach a particle to a platform

    prt_bundle_t bdl_prt;

    // verify that we do not have two dud pointers
    if ( !DEFINED_PPRT( pprt ) ) return false;

    // grab all of the particle info
    prt_bundle_set( &bdl_prt, pprt );

    // check if they can be connected
    if ( INGAME_CHR( pprt->onwhichplatform_ref ) ) return false;

    // undo the attachment
    pprt->onwhichplatform_ref    = INVALID_CHR_REF;
    pprt->onwhichplatform_update = 0;
    pprt->targetplatform_ref     = INVALID_CHR_REF;
    pprt->targetplatform_level   = -1e32;

    // get the correct particle environment
    move_one_particle_get_environment( &bdl_prt );

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool import_element_init( import_element_t * ptr )
{
    if ( NULL == ptr ) return false;

    BLANK_STRUCT_PTR( ptr )

    // all non-zero, non-null values
    ptr->player = INVALID_PLAYER_REF;
    ptr->slot   = -1;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv game_copy_imports( import_list_t * imp_lst )
{
    int       tnc;
    egolib_rv retval;
    STRING    tmp_src_dir, tmp_dst_dir;

    int                import_idx = 0;
    import_element_t * import_ptr = NULL;

    if ( NULL == imp_lst ) return rv_error;

    if ( 0 == imp_lst->count ) return rv_success;

    // assume the best
    retval = rv_success;

    // delete the data in the directory
    vfs_removeDirectoryAndContents( "import", VFS_TRUE );
    vfs_remove_mount_point("import");

    // make sure the directory exists
    if ( !vfs_mkdir( "/import" ) )
    {
        log_warning( "mnu_copy_local_imports() - Could not create the import folder. (%s)\n", vfs_getError() );
        return rv_error;
    }
    vfs_add_mount_point( fs_getUserDirectory(), "import", "mp_import", 1 );

    // copy all of the imports over
    for ( import_idx = 0; import_idx < imp_lst->count; import_idx++ )
    {
        // grab the loadplayer info
        import_ptr = imp_lst->lst + import_idx;

        snprintf( import_ptr->dstDir, SDL_arraysize( import_ptr->dstDir ), "/import/temp%04d.obj", import_ptr->slot );

        if ( !vfs_copyDirectory( import_ptr->srcDir, import_ptr->dstDir ) )
        {
            retval = rv_error;
            log_warning( "mnu_copy_local_imports() - Failed to copy an import character \"%s\" to \"%s\" (%s)\n", import_ptr->srcDir, import_ptr->dstDir, vfs_getError() );
        }

        // Copy all of the character's items to the import directory
        for ( tnc = 0; tnc < MAX_IMPORT_OBJECTS; tnc++ )
        {
            snprintf( tmp_src_dir, SDL_arraysize( tmp_src_dir ), "%s/%d.obj", import_ptr->srcDir, tnc );

            // make sure the source directory exists
            if ( vfs_isDirectory( tmp_src_dir ) )
            {
                snprintf( tmp_dst_dir, SDL_arraysize( tmp_dst_dir ), "/import/temp%04d.obj", import_ptr->slot + tnc + 1 );
                if ( !vfs_copyDirectory( tmp_src_dir, tmp_dst_dir ) )
                {
                    retval = rv_error;
                    log_warning( "mnu_copy_local_imports() - Failed to copy an import inventory item \"%s\" to \"%s\" (%s)\n", tmp_src_dir, tmp_dst_dir, vfs_getError() );
                }
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool import_list_init( import_list_t * imp_lst )
{
    int cnt;

    if ( NULL == imp_lst ) return false;

    for ( cnt = 0; cnt < MAX_IMPORTS; cnt++ )
    {
        import_element_init( imp_lst->lst + cnt );
    }
    imp_lst->count = 0;

    return true;
}

//--------------------------------------------------------------------------------------------
egolib_rv import_list_from_players( import_list_t * imp_lst )
{
    bool is_local;
    PLA_REF player;

    PLA_REF                 player_idx;
    player_t              * player_ptr = NULL;

    import_element_t      * import_ptr = NULL;

    CHR_REF                 ichr;
    chr_t                 * pchr;

    if ( NULL == imp_lst ) return rv_error;

    // blank out the ImportList list
    import_list_init( &ImportList );

    // generate the ImportList list from the player info
    for ( player_idx = 0, player = 0; player_idx < MAX_PLAYER; player_idx++ )
    {
        if ( !VALID_PLA( player_idx ) ) continue;
        player_ptr = PlaStack_get_ptr( player_idx );

        ichr = player_ptr->index;
        if ( !DEFINED_CHR( ichr ) ) continue;
        pchr = ChrList_get_ptr( ichr );

        is_local = ( NULL != player_ptr->pdevice );

        // grab a pointer
        import_ptr = imp_lst->lst + imp_lst->count;
        imp_lst->count++;

        import_ptr->player          = player_idx;
        import_ptr->slot            = REF_TO_INT( player ) * MAX_IMPORT_PER_PLAYER;
        import_ptr->srcDir[0]       = CSTR_END;
        import_ptr->dstDir[0]       = CSTR_END;
        strncpy( import_ptr->name, pchr->Name, SDL_arraysize( import_ptr->name ) );

        // only copy the "source" directory if the player is local
        if ( is_local )
        {
            snprintf( import_ptr->srcDir, SDL_arraysize( import_ptr->srcDir ), "mp_players/%s", str_encode_path( pchr->Name ) );
        }
        else
        {
            snprintf( import_ptr->srcDir, SDL_arraysize( import_ptr->srcDir ), "mp_remote/%s", str_encode_path( pchr->Name ) );
        }

        player++;
    }

    return ( imp_lst->count > 0 ) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
bool check_time( Uint32 check )
{
    /// @author ZF
    /// @details Returns true if and only if all time and date specifications determined by the e_time parameter is true. This
    ///    could indicate time of the day, a specific holiday season etc.

    switch ( check )
    {
            //Halloween between 31th october and the 1st of november
        case SEASON_HALLOWEEN: return (( 10 == getCurrentTime()->tm_mon + 1 && getCurrentTime()->tm_mday >= 31 ) ||
                                           ( 11 == getCurrentTime()->tm_mon + 1 && getCurrentTime()->tm_mday <= 1 ) );

            //Xmas from december 16th until newyear
        case SEASON_CHRISTMAS: return ( 12 == getCurrentTime()->tm_mon + 1 && getCurrentTime()->tm_mday >= 16 );

            //From 0:00 to 6:00 (spooky time!)
        case TIME_NIGHT: return getCurrentTime()->tm_hour <= 6;

            //Its day whenever it's not night
        case TIME_DAY: return !check_time( TIME_NIGHT );

            //Unhandled check
        default: log_warning( "Unhandled time enum in check_time()\n" ); return false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool water_instance_make( water_instance_t * pinst, const wawalite_water_t * pdata )
{
    /// @author ZZ
    /// @details This function sets up water movements

    int layer, frame, point, cnt;
    float temp;
    Uint8 spek;

    if ( NULL == pinst || NULL == pdata ) return false;

    for ( layer = 0; layer < pdata->layer_count; layer++ )
    {
        pinst->layer[layer].tx.x = 0;
        pinst->layer[layer].tx.y = 0;

        for ( frame = 0; frame < MAXWATERFRAME; frame++ )
        {
            // Do first mode
            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = SIN(( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( PI_OVER_TWO * layer / MAXWATERLAYER ) );
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

        /// @note claforte@> Probably need to replace this with a
        ///           GL_DEBUG(glColor4f)(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if ( GL_FLAT == gfx.shading )
            pinst->spek[cnt] = 0;
        else
            pinst->spek[cnt] = spek;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool upload_water_data( water_instance_t * pinst, const wawalite_water_t * pdata )
{
    //int layer;

    if ( NULL == pinst ) return false;

    BLANK_STRUCT_PTR( pinst )

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

    water_instance_make( pinst, pdata );

    // Allow slow machines to ignore the fancy stuff
    if ( !cfg.twolayerwater_allowed && pinst->layer_count > 1 )
    {
        int iTmp = pdata->layer[0].light_add;
        iTmp = ( pdata->layer[1].light_add * iTmp * INV_FF ) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        pinst->layer_count        = 1;
        pinst->layer[0].light_add = iTmp * INV_FF;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
egolib_rv water_instance_move( water_instance_t * pwater )
{
    /// @author ZZ
    /// @details This function animates the water overlays

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

//--------------------------------------------------------------------------------------------
bool water_instance_set_douse_level( water_instance_t * pinst, float level )
{
    int   ilayer;
    float dlevel;

    if ( NULL == pinst ) return false;

    // get the level difference
    dlevel = level - pinst->douse_level;

    // update all special values
    pinst->surface_level += dlevel;
    pinst->douse_level += dlevel;

    // update the gfx height of the water
    for ( ilayer = 0; ilayer < MAXWATERLAYER; ilayer++ )
    {
        pinst->layer[ilayer].z += dlevel;
    }

    ego_mesh_update_water_level( PMesh );

    return true;
}

//--------------------------------------------------------------------------------------------
float water_instance_get_water_level( water_instance_t * ptr )
{
    int cnt;
    float level;

    if ( NULL == ptr ) return 0.0f;

    level = water_instance_layer_get_level( ptr->layer + 0 );

    if ( cfg.twolayerwater_allowed )
    {
        for ( cnt = 1; cnt < MAXWATERLAYER; cnt++ )
        {
            // do it this way so the macro does not evaluate water_instance_layer_get_level() twice
            float tmpval = water_instance_layer_get_level( ptr->layer + cnt );

            level = std::max( level, tmpval );
        }
    }

    return level;
}

//--------------------------------------------------------------------------------------------

float water_instance_layer_get_level( water_instance_layer_t * ptr )
{
    if ( NULL == ptr ) return 0.0f;

    return ptr->z + ptr->amp;
}

//--------------------------------------------------------------------------------------------
bool status_list_update_cameras( status_list_t * plst )
{
    if ( NULL == plst ) return false;

    if ( !plst->on || 0 == plst->count ) return true;

    for ( size_t cnt = 0; cnt < plst->count; cnt++ )
    {
        status_list_element_t * pelem = StatusList.lst + cnt;
        pelem->camera_index = _cameraSystem.getCameraIndexByID(pelem->who);
    }

    return true;
}
