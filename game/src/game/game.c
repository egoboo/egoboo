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

#include "game/GUI/MiniMap.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/Inventory.hpp"
#include "game/player.h"
#include "game/link.h"
#include "game/graphic.h"
#include "game/graphic_fan.h"
#include "game/graphic_billboard.h"
#include "game/renderer_2d.h"
#include "game/input.h"
#include "game/script_compile.h"
#include "game/script_implementation.h"
#include "game/egoboo.h"
#include "game/Core/GameEngine.hpp"
#include "game/Module/Passage.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"
#include "game/char.h"
#include "game/Physics/CollisionSystem.hpp"
#include "game/physics.h"
#include "game/Physics/PhysicalConstants.hpp"
#include "game/Physics/ObjectPhysics.h"
#include "game/Entities/ObjectHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "egolib/FileFormats/Globals.hpp"
#include "egolib/egolib.h"

//--------------------------------------------------------------------------------------------

bool  overrideslots      = false;

// End text
char   endtext[MAXENDTEXT] = EMPTY_CSTR;
size_t endtext_carat = 0;

pit_info_t g_pits;

animtile_instance_t   animtile[2];
damagetile_instance_t damagetile;
weather_instance_t    weather;
water_instance_t      water;
fog_instance_t        fog;

import_list_t g_importList;

Sint32          clock_wld        = 0;
Uint32          clock_chr_stat   = 0;
Uint32          clock_pit        = 0;
Uint32          update_wld       = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void game_reset_timers();

// looping - stuff called every loop - not accessible by scripts
static void check_stats();
static void tilt_characters_to_terrain();
static void update_pits();
static void do_damage_tiles();
static void set_local_latches();
static void let_all_characters_think();
static void do_weather_spawn_particles();

// module initialization / deinitialization - not accessible by scripts
static bool game_load_module_data( const char *smallname );
static void   game_release_module_data();
static void   game_load_profile_ai();

static void   activate_spawn_file_vfs();
static void   activate_alliance_file_vfs();

static bool chr_setup_apply(std::shared_ptr<Object> pchr, spawn_file_info_t *pinfo );

static void   game_reset_players();

// Model stuff
static void log_madused_vfs( const char *savename );

// implementing wawalite data
static void upload_light_data(const wawalite_data_t& data);
static void upload_phys_data(const wawalite_physics_t& data);
static void upload_graphics_data(const wawalite_graphics_t& data);
static void upload_camera_data(const wawalite_camera_t& data);

// implementing water layer data
bool upload_water_layer_data( water_instance_layer_t inst[], const wawalite_water_layer_t data[], const int layer_count );

// misc

static bool activate_spawn_file_spawn( spawn_file_info_t * psp_info );
static bool activate_spawn_file_load_object( spawn_file_info_t * psp_info );
static void convert_spawn_file_load_name( spawn_file_info_t * psp_info );

static void game_reset_module_data();

static void load_all_profiles_import();
static void import_dir_profiles_vfs(const std::string& importDirectory);
static void game_load_global_profiles();
static void game_load_module_profiles( const std::string& modname );

static void update_all_objects();
static void move_all_objects();

//--------------------------------------------------------------------------------------------
// Random Things
//--------------------------------------------------------------------------------------------
egolib_rv export_one_character( ObjectRef character, ObjectRef owner, int chr_obj_index, bool is_local )
{
    /// @author ZZ
    /// @details This function exports a character
    STRING fromdir;
    STRING todir;
    STRING fromfile;
    STRING tofile;
    STRING todirname;
    STRING todirfullname;

    const std::shared_ptr<Object> &object = _currentModule->getObjectHandler()[character];
    if(!object) {
        return rv_error;
    }

    if ( !_currentModule->isExportValid() || ( object->getProfile()->isItem() && !object->getProfile()->canCarryToNextModule() ) )
    {
        return rv_fail;
    }

    // TWINK_BO.OBJ
    snprintf( todirname, SDL_arraysize( todirname ), "%s", str_encode_path( _currentModule->getObjectHandler()[owner]->getName() ).c_str() );

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
			Log::get().warn("%s:%d:%s: cannot create object directory `%s`\n", __FILE__, __LINE__, __FUNCTION__, todir);
            return rv_error;
        }
    }

    // modules/advent.mod/objects/advent.obj
    snprintf(fromdir, SDL_arraysize(fromdir), "%s", object->getProfile()->getPathname().c_str());

    // Build the DATA.TXT file
    if(!ObjectProfile::exportCharacterToFile(std::string(todir) + "/data.txt", object.get())) {
		Log::get().warn( "export_one_character() - unable to save data.txt \"%s/data.txt\"\n", todir );
        return rv_error;
    }

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
    ObjectRef character;

    // Stop if export isnt valid
    if ( !_currentModule->isExportValid() ) return rv_fail;

    // assume the best
    retval = rv_success;

    // Check each player
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ObjectRef item;
        player_t * ppla;
        Object    * pchr;

        if ( !VALID_PLA( ipla ) ) continue;
        ppla = PlaStack.get_ptr( ipla );

        is_local = ( NULL != ppla->pdevice );
        if ( require_local && !is_local ) continue;

        // Is it alive?
        if ( !_currentModule->getObjectHandler().exists( ppla->index ) ) continue;
        character = ObjectRef(ppla->index);
        pchr      = _currentModule->getObjectHandler().get( character );

        // don't export dead characters
        if ( !pchr->isAlive() ) continue;

        // Export the character
        export_chr_rv = export_one_character( character, character, -1, is_local );
        if ( rv_error == export_chr_rv )
        {
            retval = rv_error;
        }

        // Export the left hand item
        item = ObjectRef(pchr->holdingwhich[SLOT_LEFT]);
        if ( _currentModule->getObjectHandler().exists( item ) )
        {
            export_chr_rv = export_one_character( item, character, SLOT_LEFT, is_local );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
        }

        // Export the right hand item
        item = ObjectRef(pchr->holdingwhich[SLOT_RIGHT]);
        if ( _currentModule->getObjectHandler().exists( item ) )
        {
            export_chr_rv = export_one_character( item, character, SLOT_RIGHT, is_local );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
        }

        // Export the inventory
        number = 0;
        for(const std::shared_ptr<Object> pitem : pchr->getInventory().iterate())
        {
            if ( number >= pchr->getInventory().getMaxItems() ) break;

            export_chr_rv = export_one_character( pitem->getObjRef(), character, number + SLOT_COUNT, is_local );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
            else if ( rv_success == export_chr_rv )
            {
                number++;
            }
        }
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

    hFileWrite = vfs_openWrite( savename );
    if ( hFileWrite )
    {
        vfs_printf( hFileWrite, "Slot usage for objects in last module loaded...\n" );
        //vfs_printf( hFileWrite, "%d of %d frames used...\n", Md2FrameList_index, MAXFRAME );

        PRO_REF lastSlotNumber = 0;
        for (const auto &element : ProfileSystem::get().getLoadedProfiles())
        {
            const std::shared_ptr<ObjectProfile> &profile = element.second;

            //ZF> ugh, import objects are currently handled in a weird special way.
            for(size_t i = lastSlotNumber; i < profile->getSlotNumber() && i <= 36; ++i)
            {
                if (!ProfileSystem::get().isValidProfileID(i))
                {
                    vfs_printf( hFileWrite, "%3" PRIuZ " %32s.\n", i, "Slot reserved for import players" );
                }
            }
            lastSlotNumber = profile->getSlotNumber();

            vfs_printf(hFileWrite, "%3d %32s %s\n", profile->getSlotNumber(), profile->getClassName().c_str(), profile->getModel()->getName().c_str());
        }

        vfs_close( hFileWrite );
    }
}

//--------------------------------------------------------------------------------------------
void activate_alliance_file_vfs()
{
    /// @author ZZ
    /// @details This function reads the alliance file
    TEAM_REF teama, teamb;

    // Load the file
    ReadContext ctxt("mp_data/alliance.txt");
    if (!ctxt.ensureOpen())
    {
        return;
    }
    while (ctxt.skipToColon(true))
    {
        char buffer[1024 + 1];
        vfs_read_string_lit(ctxt, buffer, 1024);
        if (strlen(buffer) < 1)
        {
            throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt.getLoadName(), ctxt.getLineNumber()),
                                                "empty string literal");
        }
        teama = (buffer[0] - 'A') % Team::TEAM_MAX;

        vfs_read_string_lit(ctxt, buffer, 1024);
        if (strlen(buffer) < 1)
        {
            throw Id::SyntacticalErrorException(__FILE__, __LINE__, Id::Location(ctxt.getLoadName(), ctxt.getLineNumber()),
                                                "empty string literal");
        }
        teamb = (buffer[0] - 'A') % Team::TEAM_MAX;
        _currentModule->getTeamList()[teama].makeAlliance(_currentModule->getTeamList()[teamb]);
    }
}

//--------------------------------------------------------------------------------------------
void update_all_objects()
{
    chr_stoppedby_tests = 0;
    chr_pressure_tests  = 0;

    _currentModule->updateAllObjects();
    ParticleHandler::get().updateAllParticles();
}

//--------------------------------------------------------------------------------------------
void move_all_objects()
{
	g_meshStats.mpdfxTests = 0;

    move_all_particles();
    move_all_characters();
}

//--------------------------------------------------------------------------------------------
int update_game()
{
    /// @author ZZ
    /// @details This function does several iterations of character movements and such
    ///    to keep the game in sync.

    // Check for all local players being dead
    local_stats.allpladead      = false;
    local_stats.seeinvis_level  = 0.0f;
    local_stats.seekurse_level  = 0.0f;
    local_stats.seedark_level   = 0.0f;
    local_stats.grog_level      = 0.0f;
    local_stats.daze_level      = 0.0f;
    AudioSystem::get().setMaxHearingDistance(AudioSystem::DEFAULT_MAX_DISTANCE);

    //status text for player stats
    check_stats();

    //Passage music
    _currentModule->checkPassageMusic();

    int numdead = 0;
    int numalive = 0;
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        Object * pchr;

        if ( !PlaStack.lst[ipla].valid ) continue;

        // fix bad players
        ichr = PlaStack.lst[ipla].index;
        if ( !_currentModule->getObjectHandler().exists( ichr ) )
        {
            PlaStack.lst[ipla].index = INVALID_CHR_REF;
            PlaStack.lst[ipla].valid = false;
            continue;
        }
        pchr = _currentModule->getObjectHandler().get( ichr );

        // only interested in local players
        if ( NULL == PlaStack.lst[ipla].pdevice ) continue;

        if ( pchr->isAlive() )
        {
            numalive++;

            local_stats.seeinvis_level += pchr->getAttribute(Ego::Attribute::SEE_INVISIBLE);
            local_stats.seekurse_level += pchr->getAttribute(Ego::Attribute::SENSE_KURSES);
            local_stats.seedark_level  += pchr->getAttribute(Ego::Attribute::DARKVISION);
            local_stats.grog_level     += pchr->grog_timer;
            local_stats.daze_level     += pchr->daze_timer;

            //See invisble through perk
            if (pchr->hasPerk(Ego::Perks::SENSE_INVISIBLE)) {
                local_stats.seeinvis_level += 1;
            }

            //Do they have the listening perk? (+100% hearing distance)
            if (pchr->hasPerk(Ego::Perks::PERCEPTIVE)) {
                AudioSystem::get().setMaxHearingDistance(AudioSystem::DEFAULT_MAX_DISTANCE*2);
            }
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
    for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        if ( !PlaStack.lst[ipla].valid ) continue;

        CHR_REF ichr = PlaStack.lst[ipla].index;
        if ( !_currentModule->getObjectHandler().exists( ichr ) ) continue;
        Object *pchr = _currentModule->getObjectHandler().get( ichr );

        if ( !pchr->isAlive() )
        {
            if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard && local_stats.allpladead && keyb.is_key_down(SDLK_SPACE) && _currentModule->isRespawnValid() && 0 == local_stats.revivetimer)
            {
                pchr->respawn();
                pchr->experience *= EXPKEEP;        // Apply xp Penality

                if (egoboo_config_t::get().game_difficulty.getValue() > Ego::GameDifficulty::Easy)
                {
                    pchr->money *= EXPKEEP;        //Apply money loss
                }
            }
        }
    }

    // do important stuff to keep in sync inside this loop

    // keep the mpdfx lists up-to-date. No calculation is done unless one
    // of the mpdfx values was changed during the last update
    _currentModule->getMeshPointer()->_fxlists.synch( _currentModule->getMeshPointer()->_tmem, false );
    
    // Get immediate mode state for the rest of the game
    InputSystem::read_keyboard();
    InputSystem::read_mouse();
    InputSystem::read_joysticks();

    set_local_latches();

    //Rebuild the quadtree for fast object lookup
    _currentModule->getObjectHandler().updateQuadTree(0.0f, 0.0f, _currentModule->getMeshPointer()->_info.getTileCountX()*Info<float>::Grid::Size(),
		                                                          _currentModule->getMeshPointer()->_info.getTileCountY()*Info<float>::Grid::Size());

    //---- begin the code for updating misc. game stuff
    {
        BillboardSystem::get()._billboardList.update();
        animate_tiles();
        water.move();
        AudioSystem::get().updateLoopingSounds();
        do_damage_tiles();
        update_pits();
        do_weather_spawn_particles();
    }
    //---- end the code for updating misc. game stuff

    //---- Run AI (but not on first update frame)
    if(update_wld > 0)
    {
        let_all_characters_think();           // sets the non-player latches
        net_unbuffer_player_latches();            // sets the player latches
    }

    //---- begin the code for updating in-game objects
    update_all_objects();
    {
        move_all_objects();                            //movement
        Ego::Physics::CollisionSystem::get().update(); //collisions
    }
    //---- end the code for updating in-game objects

    // put the camera movement inside here
    CameraSystem::get()->updateAll(_currentModule->getMeshPointer().get());

    // Timers
    clock_wld += TICKS_PER_SEC / GameEngine::GAME_TARGET_UPS; ///< 1000 tics per sec / 50 UPS = 20 ticks
    clock_chr_stat++;

    // Reset the respawn timer
    if ( local_stats.revivetimer > 0 )
    {
        local_stats.revivetimer--;
    }

    update_wld++;

    return 1;
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
    g_pits.kill = g_pits.teleport = false;
    clock_pit = 0;

    // reset some counters
    game_frame_all = 0;
    update_wld = 0;

    // reset some special clocks
    clock_chr_stat = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ObjectRef prt_find_target( const Vector3f& pos, FACING_T facing,
                           const PIP_REF particletype, const TEAM_REF team, 
	                       ObjectRef donttarget, ObjectRef oldtarget, FACING_T *targetAngle )
{
    /// @author ZF
    /// @details This is the new improved targeting system for particles. Also includes distance in the Z direction.

    const float max_dist2 = WIDE * WIDE;

    std::shared_ptr<pip_t> ppip;

    ObjectRef besttarget = ObjectRef::Invalid;
    float  longdist2 = max_dist2;

    if ( !LOADED_PIP( particletype ) ) return ObjectRef::Invalid;
    ppip = PipStack.get_ptr( particletype );

    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        bool target_friend, target_enemy;

        if ( !pchr->isAlive() || pchr->isitem || _currentModule->getObjectHandler().exists( pchr->inwhich_inventory ) ) continue;

        // prefer targeting riders over the mount itself
        if ( pchr->isMount() && ( _currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_LEFT] ) || _currentModule->getObjectHandler().exists( pchr->holdingwhich[SLOT_RIGHT] ) ) ) continue;

        // ignore invictus
        if ( pchr->invictus ) continue;

        // we are going to give the player a break and not target things that
        // can't be damaged, unless the particle is homing. If it homes in,
        // the he damage_timer could drop off en route.
        if ( !ppip->homing && ( 0 != pchr->damage_timer ) ) continue;

        // Don't retarget someone we already had or not supposed to target
        if ( pchr->getObjRef() == oldtarget || pchr->getObjRef() == donttarget ) continue;

        Team &particleTeam = _currentModule->getTeamList()[team];

        target_friend = ppip->onlydamagefriendly && particleTeam == pchr->getTeam();
        target_enemy  = !ppip->onlydamagefriendly && particleTeam.hatesTeam(pchr->getTeam() );

        if ( target_friend || target_enemy )
        {
            FACING_T angle = - facing + vec_to_facing( pchr->getPosX() - pos[kX] , pchr->getPosY() - pos[kY] );

            // Only proceed if we are facing the target
            if ( angle < ppip->targetangle || angle > ( 0xFFFF - ppip->targetangle ) )
            {
                float dist2 = (pchr->getPosition() - pos).length_2();

                if ( dist2 < longdist2 && dist2 <= max_dist2 )
                {
                    (*targetAngle) = angle;
                    besttarget = pchr->getObjRef();
                    longdist2 = dist2;
                }
            }
        }
    }

    // All done
    return besttarget;
}

//--------------------------------------------------------------------------------------------
bool chr_check_target( Object * psrc, const ObjectRef iObjectTest, IDSZ idsz, const BIT_FIELD targeting_bits )
{
    bool retval = false;

    bool is_hated;

    // Skip non-existing objects
    if (!psrc || psrc->isTerminated()) return false;

    const std::shared_ptr<Object> &ptst = _currentModule->getObjectHandler()[iObjectTest];
    if(!ptst) {
        return false;
    }

    // Skip hidden characters
    if ( ptst->isHidden() ) return false;

    // Players only?
    if (( HAS_SOME_BITS( targeting_bits, TARGET_PLAYERS ) || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) ) && !VALID_PLA( ptst->is_which_player ) ) return false;

    // Skip held objects
    if ( ptst->isBeingHeld() ) return false;

    // Allow to target ourselves?
    if ( psrc == ptst.get() && HAS_NO_BITS( targeting_bits, TARGET_SELF ) ) return false;

    // Don't target our holder if we are an item and being held
    if ( psrc->isitem && psrc->attachedto == ptst->getObjRef() ) return false;

    // Allow to target dead stuff?
    if ( ptst->isAlive() == HAS_SOME_BITS( targeting_bits, TARGET_DEAD ) ) return false;

    // Don't target invisible stuff, unless we can actually see them
    if ( !psrc->canSeeObject(ptst) ) return false;

    //Need specific skill? ([NONE] always passes)
    if ( HAS_SOME_BITS( targeting_bits, TARGET_SKILL ) && !ptst->hasSkillIDSZ(idsz) ) return false;

    // Require player to have specific quest?
    if ( HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
    {
        player_t * ppla = PlaStack.get_ptr( ptst->is_which_player );

        // find only active quests?
        // this makes it backward-compatible with zefz's version
        if ( quest_log_get_level( ppla->quest_log, idsz ) < 0 ) {
            return false;
        }
    }

    is_hated = psrc->getTeam().hatesTeam(ptst->getTeam());

    // Target neutral items? (still target evil items, could be pets)
    if (( ptst->isItem() || ptst->isInvincible() ) && !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) ) return false;

    // Only target those of proper team. Skip this part if it's a item
    if ( !ptst->isItem() )
    {
        if (( HAS_NO_BITS( targeting_bits, TARGET_ENEMIES ) && is_hated ) ) return false;
        if (( HAS_NO_BITS( targeting_bits, TARGET_FRIENDS ) && !is_hated ) ) return false;
    }

    //This is the last and final step! Check for specific IDSZ too? (not needed if we are looking for a quest)
    if ( IDSZ_NONE == idsz || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
    {
        retval = true;
    }
    else
    {
        bool match_idsz = ( idsz == ptst->getProfile()->getIDSZ(IDSZ_PARENT) ) ||
                            ( idsz == ptst->getProfile()->getIDSZ(IDSZ_TYPE) );

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
ObjectRef chr_find_target( Object * psrc, float max_dist, IDSZ idsz, const BIT_FIELD targeting_bits )
{
    /// @author ZF
    /// @details This is the new improved AI targeting algorithm. Also includes distance in the Z direction.
    ///     If max_dist is 0 then it searches without a max limit.

    line_of_sight_info_t los_info;

    if (!psrc || psrc->isTerminated()) return ObjectRef::Invalid;

    std::vector<std::shared_ptr<Object>> searchList;

    //Only loop through the players
    if ( HAS_SOME_BITS( targeting_bits, TARGET_PLAYERS ) || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
    {
        for (PLA_REF ipla = 0; ipla < MAX_PLAYER; ipla++)
        {
            if (!PlaStack.lst[ipla].valid) continue;

            const std::shared_ptr<Object> &player = _currentModule->getObjectHandler()[PlaStack.lst[ipla].index];
            if(player) {

                //Within range?
                float distance = (player->getPosition() - psrc->getPosition()).length();
                if(max_dist == NEAREST || distance < max_dist) {
                    searchList.push_back(player);
                }

            }
        }
    }

    //All objects in level
    else if(max_dist == NEAREST)
    {
        searchList = _currentModule->getObjectHandler().getAllObjects();
    }

    //All objects within range
    else
    {
        searchList = _currentModule->getObjectHandler().findObjects(psrc->getPosX(), psrc->getPosY(), max_dist, true);
    }


    // set the line-of-sight source
    los_info.x0         = psrc->getPosX();
    los_info.y0         = psrc->getPosY();
    los_info.z0         = psrc->getPosZ() + psrc->bump.height;
    los_info.stopped_by = psrc->stoppedby;

    ObjectRef best_target = ObjectRef::Invalid;
    float best_dist2  = (max_dist == NEAREST) ? std::numeric_limits<float>::max() : max_dist*max_dist + 1.0f;
    for(const std::shared_ptr<Object> &ptst : searchList)
    {
        if(ptst->isTerminated()) continue;

        //Skip held items
        if(ptst->isBeingHeld()) continue;

        if ( !chr_check_target( psrc, ptst->getObjRef(), idsz, targeting_bits ) ) continue;

		float dist2 = (psrc->getPosition() - ptst->getPosition()).length_2();
        if (dist2 < best_dist2)
        {
            //Invictus chars do not need a line of sight
            if ( !psrc->isInvincible() )
            {
                // set the line-of-sight source
                los_info.x1 = ptst->getPosition()[kX];
                los_info.y1 = ptst->getPosition()[kY];
                los_info.z1 = ptst->getPosition()[kZ] + std::max( 1.0f, ptst->bump.height );

                if ( line_of_sight_info_t::blocked( &los_info ) ) continue;
            }

            //Set the new best target found
            best_target = ptst->getObjRef();
            best_dist2  = dist2;
        }
    }

    return best_target;
}

//--------------------------------------------------------------------------------------------
void do_damage_tiles()
{
    // do the damage tile stuff

    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
        // if the object is not really in the game, do nothing
        if ( pchr->isHidden() || !pchr->isAlive() ) continue;

        // if you are being held by something, you are protected
        if ( _currentModule->getObjectHandler().exists( pchr->inwhich_inventory ) ) continue;

        // are we on a damage tile?
		auto mesh = _currentModule->getMeshPointer();
        if ( !mesh->grid_is_valid( pchr->getTile() ) ) continue;
        if ( 0 == mesh->test_fx( pchr->getTile(), MAPFX_DAMAGE ) ) continue;

        // are we low enough?
        if ( pchr->getPosZ() > pchr->enviro.floor_level + DAMAGERAISE ) continue;

        // allow reaffirming damage to things like torches, even if they are being held,
        // but make the tolerance closer so that books won't burn so easily
        if ( !_currentModule->getObjectHandler().exists( pchr->attachedto ) || pchr->getPosZ() < pchr->enviro.floor_level + DAMAGERAISE )
        {
            if ( pchr->reaffirm_damagetype == damagetile.damagetype )
            {
                if ( 0 == ( update_wld & TILE_REAFFIRM_AND ) )
                {
                    reaffirm_attached_particles(pchr->getObjRef());
                }
            }
        }

        // do not do direct damage to items that are being held
        if ( _currentModule->getObjectHandler().exists( pchr->attachedto ) ) continue;

        // don't do direct damage to invulnerable objects
        if ( pchr->invictus ) continue;

        if ( 0 == pchr->damage_timer )
        {
            int actual_damage = pchr->damage(ATK_BEHIND, damagetile.amount, static_cast<DamageType>(damagetile.damagetype), 
                Team::TEAM_DAMAGE, nullptr, DAMFX_NBLOC | DAMFX_ARMO, false);

            pchr->damage_timer = DAMAGETILETIME;

            if (( actual_damage > 0 ) && (LocalParticleProfileRef::Invalid != damagetile.part_gpip ) && 0 == ( update_wld & damagetile.partand ) )
            {
                ParticleHandler::get().spawnGlobalParticle( pchr->getPosition(), ATK_FRONT, damagetile.part_gpip, 0 );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void update_pits()
{
    /// @author ZZ
    /// @details This function kills any character in a deep pit...

    if ( g_pits.kill || g_pits.teleport )
    {
        //Decrease the timer
        if ( clock_pit > 0 ) clock_pit--;

        if ( 0 == clock_pit )
        {
            //Reset timer
            clock_pit = 20;

            // Kill any particles that fell in a pit, if they die in water...
            for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
            {
                if ( particle->getPosZ() < PITDEPTH && particle->getProfile()->end_water )
                {
                    particle->requestTerminate();
                }
            }

            // Kill or teleport any characters that fell in a pit...
            for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
            {
                // Is it a valid character?
                if ( pchr->isInvincible() || !pchr->isAlive() ) continue;
                if ( pchr->isBeingHeld() ) continue;

                // Do we kill it?
                if ( g_pits.kill && pchr->getPosZ() < PITDEPTH )
                {
                    // Got one!
                    pchr->kill(Object::INVALID_OBJECT, false);
                    pchr->vel[kX] = 0;
                    pchr->vel[kY] = 0;

                    /// @note ZF@> Disabled, the pitfall sound was intended for pits.teleport only
                    // Play sound effect
                    // sound_play_chunk( pchr->pos, g_wavelist[GSND_PITFALL] );
                }

                // Do we teleport it?
                if ( g_pits.teleport && pchr->getPosZ() < PITDEPTH * 4 )
                {
                    bool teleported;

                    // Teleport them back to a "safe" spot
                    teleported = pchr->teleport(g_pits.teleport_pos, pchr->ori.facing_z);

                    if ( !teleported )
                    {
                        // Kill it instead
                        pchr->kill(Object::INVALID_OBJECT, false);
                    }
                    else
                    {
                        // Stop movement
                        pchr->vel = Vector3f::zero();

                        // Play sound effect
                        if ( VALID_PLA( pchr->is_which_player ) )
                        {
                            AudioSystem::get().playSoundFull(AudioSystem::get().getGlobalSound(GSND_PITFALL));
                        }
                        else
                        {
                            AudioSystem::get().playSound(pchr->getPosition(), AudioSystem::get().getGlobalSound(GSND_PITFALL));
                        }

                        // Do some damage (same as damage tile)
                        pchr->damage(ATK_BEHIND, damagetile.amount, static_cast<DamageType>(damagetile.damagetype), Team::TEAM_DAMAGE, 
                            _currentModule->getObjectHandler()[pchr->ai.bumplast], DAMFX_NBLOC | DAMFX_ARMO, false);
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn_particles()
{
    /// @author ZZ
    /// @details This function drops snowflakes or rain or whatever

    //Does this module have valid weather?
    if(weather.time < 0 || weather.part_gpip == LocalParticleProfileRef::Invalid) {
        return;
    }

    weather.time--;
    if ( 0 == weather.time )
    {
        weather.time = weather.timer_reset;

        // Find a valid player
        bool foundone = false;
        for ( int cnt = 0; cnt < MAX_PLAYER; cnt++ )
        {
            // Yes, but is the character valid?
            weather.iplayer = ( PLA_REF )(( REF_TO_INT( weather.iplayer ) + 1 ) % MAX_PLAYER );
            if ( PlaStack.lst[weather.iplayer].valid && _currentModule->getObjectHandler().exists(PlaStack.lst[weather.iplayer].index) )
            {
                foundone = true;
                break;
            }
        }

        // Did we find one?
        if ( foundone )
        {
            CHR_REF ichr = PlaStack.lst[weather.iplayer].index;
            if ( _currentModule->getObjectHandler().exists( ichr ) && !_currentModule->getObjectHandler().exists( _currentModule->getObjectHandler().get(ichr)->inwhich_inventory ) )
            {
                const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[PlaStack.lst[weather.iplayer].index];

                // Yes, so spawn nearby that character
                std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnGlobalParticle(pchr->getPosition(), ATK_FRONT, weather.part_gpip, 0, weather.over_water);
                if ( particle )
                {
                    // Weather particles spawned at the edge of the map look ugly, so don't spawn them there
                    if ( particle->getPosX() < EDGE || particle->getPosX() > _currentModule->getMeshPointer()->_tmem._edge_x - EDGE )
                    {
                        particle->requestTerminate();
                    }
                    else if ( particle->getPosY() < EDGE || particle->getPosY() > _currentModule->getMeshPointer()->_tmem._edge_y - EDGE )
                    {
                        particle->requestTerminate();
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

    // skip invalid players
    if ( INVALID_PLA( ipla ) ) return;
	player_t       *ppla = PlaStack.get_ptr( ipla );

    // is the device a local device or an internet device?
	input_device_t *pdevice = ppla->pdevice;
    if ( NULL == pdevice ) return;

    //No need to continue if device is not enabled
    if ( !input_device_is_enabled( pdevice ) ) return;

    // find the camera that is pointing at this character
    auto pcam = CameraSystem::get()->getCamera(ObjectRef(ppla->index));
    if (!pcam) return;

    // fast camera turn if it is enabled and there is only 1 local player
	bool fast_camera_turn = ( 1 == local_stats.player_count ) && ( CameraTurnMode::Good == pcam->getTurnMode() );

	latch_t sum;
    // Clear the player's latch buffers
    sum.clear();
	Vector2f joy_new = Vector2f::zero(),
             joy_pos = Vector2f::zero();

    // generate the transforms relative to the camera
    // this needs to be changed for multicamera
	TURN_T turnsin = TO_TURN( pcam->getOrientation().facing_z );
    float fsin    = turntosin[ turnsin ];
    float fcos    = turntocos[ turnsin ];

	float scale;
    if ( INPUT_DEVICE_MOUSE == pdevice->device_type )
    {
        // Mouse routines

        if ( fast_camera_turn || !input_device_control_active( pdevice,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
        {
            float dist = std::sqrt( mous.x * mous.x + mous.y * mous.y );
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

                joy_pos[XX] = mous.x * scale;
                joy_pos[YY] = mous.y * scale;

                //if ( fast_camera_turn && !input_device_control_active( pdevice,  CONTROL_CAMERA ) )  joy_pos.x = 0;

                joy_new[XX] = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
                joy_new[YY] = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
            }
        }
    }

    else if ( INPUT_DEVICE_KEYBOARD == pdevice->device_type )
    {
        // Keyboard routines

        if ( fast_camera_turn || !input_device_control_active( pdevice, CONTROL_CAMERA ) )
        {
            if ( input_device_control_active( pdevice,  CONTROL_RIGHT ) )   joy_pos[XX]++;
            if ( input_device_control_active( pdevice,  CONTROL_LEFT ) )    joy_pos[XX]--;
            if ( input_device_control_active( pdevice,  CONTROL_DOWN ) )    joy_pos[YY]++;
            if ( input_device_control_active( pdevice,  CONTROL_UP ) )      joy_pos[YY]--;

            if ( fast_camera_turn )  joy_pos[XX] = 0;

            joy_new[XX] = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
            joy_new[YY] = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
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
            joy_pos[XX] = joystick->x;
            joy_pos[YY] = joystick->y;

            float dist = joy_pos.length_2();
            if ( dist > 1.0f )
            {
                scale = 1.0f / std::sqrt( dist );
                joy_pos *= scale;
            }

            if ( fast_camera_turn && !input_device_control_active( pdevice, CONTROL_CAMERA ) )  joy_pos[XX] = 0;

            joy_new[XX] = ( joy_pos[XX] * fcos + joy_pos[YY] * fsin );
            joy_new[YY] = ( -joy_pos[XX] * fsin + joy_pos[YY] * fcos );
        }
    }

    else
    {
        // unknown device type.
        pdevice = NULL;
    }

    // Update movement (if any)
    sum.x += joy_new[XX];
    sum.y += joy_new[YY];

    // Read control buttons
    if ( !ppla->inventoryMode )
    {
        if ( input_device_control_active( pdevice, CONTROL_JUMP ) ) 
            sum.b[LATCHBUTTON_JUMP] = true;
        if ( input_device_control_active( pdevice, CONTROL_LEFT_USE ) )
            sum.b[LATCHBUTTON_LEFT] = true;
        if ( input_device_control_active( pdevice, CONTROL_LEFT_GET ) )
            sum.b[LATCHBUTTON_ALTLEFT] = true;
        if ( input_device_control_active( pdevice, CONTROL_RIGHT_USE ) )
            sum.b[LATCHBUTTON_RIGHT] = true;
        if ( input_device_control_active( pdevice, CONTROL_RIGHT_GET ) )
            sum.b[LATCHBUTTON_ALTRIGHT] = true;

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
        Object *pchr = _currentModule->getObjectHandler().get( ppla->index );

        //dirty hack here... mouse seems to be inverted in inventory mode?
        if ( pdevice->device_type == INPUT_DEVICE_MOUSE )
        {
            joy_pos[XX] = - joy_pos[XX];
            joy_pos[YY] = - joy_pos[YY];
        }

        //handle inventory movement
        if ( joy_pos[XX] < 0 )       new_selected--;
        else if ( joy_pos[XX] > 0 )  new_selected++;

        //clip to a valid value
        if ( ppla->inventory_slot != new_selected )
        {
            ppla->inventory_cooldown = update_wld + 5;

            //Make inventory movement wrap around
            if(new_selected < 0) {
                new_selected = pchr->getInventory().getMaxItems() - 1;
            }
            else if(new_selected >= pchr->getInventory().getMaxItems()) {
                ppla->inventory_slot = 0;
            }
            else {
                ppla->inventory_slot = new_selected;
            }
        }

        //handle item control
        if ( pchr->inst.action_ready && 0 == pchr->reload_timer )
        {
            //handle LEFT hand control
            if ( input_device_control_active( pdevice, CONTROL_LEFT_USE ) || input_device_control_active(pdevice, CONTROL_LEFT_GET) )
            {
                //put it away and swap with any existing item
                Inventory::swap_item(ObjectRef(ppla->index), ppla->inventory_slot, SLOT_LEFT, false );

                // Make it take a little time
                chr_play_action( pchr, ACTION_MG, false );
                pchr->reload_timer = PACKDELAY;
            }

            //handle RIGHT hand control
            if ( input_device_control_active( pdevice, CONTROL_RIGHT_USE) || input_device_control_active( pdevice, CONTROL_RIGHT_GET) )
            {
                // put it away and swap with any existing item
                Inventory::swap_item(ObjectRef(ppla->index), ppla->inventory_slot, SLOT_RIGHT, false);

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
        _gameEngine->getActivePlayingState()->displayCharacterWindow(ipla);
         ppla->inventory_cooldown = update_wld + ( ONESECOND / 4 );
    }

    //Enter or exit stealth mode?
    if(input_device_control_active(pdevice, CONTROL_SNEAK) && update_wld > ppla->inventory_cooldown) {
        if(!_currentModule->getObjectHandler()[ppla->index]->isStealthed()) {
            _currentModule->getObjectHandler()[ppla->index]->activateStealth();
        }
        else {
            _currentModule->getObjectHandler()[ppla->index]->deactivateStealth();
        }
        ppla->inventory_cooldown = update_wld + ONESECOND;
    }
}

//--------------------------------------------------------------------------------------------
void set_local_latches()
{
    for (PLA_REF cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        set_one_player_latch( cnt );
    }

    // Let the players respawn
    if (keyb.is_key_down(SDLK_SPACE)
        && (local_stats.allpladead || _currentModule->canRespawnAnyTime())
        && _currentModule->isRespawnValid()
        && egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard
        && !keyb.chat_mode )
    {
        for (PLA_REF player = 0; player < MAX_PLAYER; player++)
        {
            if ( PlaStack.lst[player].valid && PlaStack.lst[player].pdevice != nullptr )
            {
                // Press the respawn button...
                PlaStack.lst[player].local_latch.b[LATCHBUTTON_RESPAWN] = true;
            }
        }
    }

    // update the local timed latches with the same info
    for (PLA_REF player = 0; player < MAX_PLAYER; player++)
    {
        player_t * ppla;

        if ( !PlaStack.lst[player].valid ) continue;
        ppla = PlaStack.get_ptr( player );

        int index = ppla->tlatch_count;
        if ( index < MAXLAG )
        {
            time_latch_t * ptlatch = ppla->tlatch + index;

            ptlatch->button = ppla->local_latch.b.to_ulong();

            // reduce the resolution of the motion to match the network packets
            ptlatch->x = std::floor( ppla->local_latch.x * SHORTLATCH ) / SHORTLATCH;
            ptlatch->y = std::floor( ppla->local_latch.y * SHORTLATCH ) / SHORTLATCH;

            ptlatch->time = update_wld;

            ppla->tlatch_count++;
        }

        // determine the max amount of lag
        for ( uint32_t cnt = 0; cnt < ppla->tlatch_count; cnt++ )
        {
            int loc_lag = update_wld - ppla->tlatch[index].time + 1;

            if ( loc_lag > 0 && ( size_t )loc_lag > numplatimes )
            {
                numplatimes = loc_lag;
            }
        }
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

    ticks = SDL_GetTicks();
    if ( ticks > stat_check_timer + 20 )
    {
        stat_check_timer = ticks;
    }

    stat_check_delay -= 20;
    if ( stat_check_delay > 0 )
        return;

    // Show map cheat
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && keyb.is_key_down(SDLK_m) && keyb.is_key_down(SDLK_LSHIFT))
    {
        _gameEngine->getActivePlayingState()->getMiniMap()->setVisible(true);
        _gameEngine->getActivePlayingState()->getMiniMap()->setShowPlayerPosition(true);
        stat_check_delay = 150;
    }

    // XP CHEAT
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && keyb.is_key_down(SDLK_x))
    {
        PLA_REF docheat = INVALID_PLA_REF;
        if (keyb.is_key_down( SDLK_1 ) )  docheat = 0;
        else if (keyb.is_key_down( SDLK_2 ) )  docheat = 1;
        else if (keyb.is_key_down( SDLK_3 ) )  docheat = 2;
        else if (keyb.is_key_down( SDLK_4 ) )  docheat = 3;

        //Apply the cheat if valid
        if ( docheat != INVALID_PLA_REF )
        {
            const std::shared_ptr<Object> &player = _currentModule->getObjectHandler()[PlaStack.lst[docheat].index];
            if(player)
            {
                //Give 10% of XP needed for next level
                uint32_t xpgain = 0.1f * ( player->getProfile()->getXPNeededForLevel( std::min( player->experiencelevel+1, MAXLEVEL) ) - player->getProfile()->getXPNeededForLevel(player->experiencelevel));
                player->giveExperience(xpgain, XP_DIRECT, true);
                stat_check_delay = 1;
            }
        }
    }

    // LIFE CHEAT
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && keyb.is_key_down(SDLK_z))
    {
        PLA_REF docheat = INVALID_PLA_REF;

        if (keyb.is_key_down( SDLK_1 ) )  docheat = 0;
        else if (keyb.is_key_down( SDLK_2 ) )  docheat = 1;
        else if (keyb.is_key_down( SDLK_3 ) )  docheat = 2;
        else if (keyb.is_key_down( SDLK_4 ) )  docheat = 3;

        //Apply the cheat if valid
        if(docheat != INVALID_PLA_REF) {
            const std::shared_ptr<Object> &player = _currentModule->getObjectHandler()[PlaStack.lst[docheat].index];
            if (player)
            {
                //Heal 1 life
                player->heal(player, 256, true);
                stat_check_delay = 1;
            }

        }

    }

    // Display armor stats?
    if (keyb.is_key_down( SDLK_LSHIFT ) )
    {
        if (keyb.is_key_down( SDLK_1 ) )  { show_armor( 0 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_2 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_3 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_4 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_5 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_6 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_7 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_8 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
    }

    // Display enchantment stats?
    else if (keyb.is_key_down( SDLK_LCTRL ) )
    {
        if (keyb.is_key_down( SDLK_1 ) )  { show_full_status( 0 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_2 ) )  { show_full_status( 1 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_3 ) )  { show_full_status( 2 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_4 ) )  { show_full_status( 3 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_5 ) )  { show_full_status( 4 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_6 ) )  { show_full_status( 5 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_7 ) )  { show_full_status( 6 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_8 ) )  { show_full_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character special powers?
    else if (keyb.is_key_down( SDLK_LALT ) )
    {
        if (keyb.is_key_down( SDLK_1 ) )  { show_magic_status( 0 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_2 ) )  { show_magic_status( 1 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_3 ) )  { show_magic_status( 2 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_4 ) )  { show_magic_status( 3 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_5 ) )  { show_magic_status( 4 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_6 ) )  { show_magic_status( 5 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_7 ) )  { show_magic_status( 6 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_8 ) )  { show_magic_status( 7 ); stat_check_delay = 1000; }
    }

#if 0
    // Display character stats?
    else
    {
        if (keyb.is_key_down( SDLK_1 ) )  { show_stat( 0 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_2 ) )  { show_stat( 1 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_3 ) )  { show_stat( 2 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_4 ) )  { show_stat( 3 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_5 ) )  { show_stat( 4 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_6 ) )  { show_stat( 5 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_7 ) )  { show_stat( 6 ); stat_check_delay = 1000; }
        if (keyb.is_key_down( SDLK_8 ) )  { show_stat( 7 ); stat_check_delay = 1000; }
    }
#endif
}

//--------------------------------------------------------------------------------------------
#if 0
void show_stat( int statindex )
{
    /// @author ZZ
    /// @details This function shows the more specific stats for a character

    CHR_REF character;
    int     level;
    char    gender[8] = EMPTY_CSTR;

    const std::shared_ptr<Object> &pchr = _gameEngine->getActivePlayingState()->getStatusCharacter(statindex);

    if (pchr)
    {
        const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(pchr->profile_ref);

        // Name
        DisplayMsg_printf( "=%s=", pchr->getName(true, false, true).c_str());

        // Level and gender and class
        gender[0] = 0;
        if ( pchr->isAlive() )
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
                DisplayMsg_printf( "~%dst level %s%s", level, gender_str, profile->getClassName().c_str() );
            }
            else if ( 2 == itmp )
            {
                DisplayMsg_printf( "~%dnd level %s%s", level, gender_str, profile->getClassName().c_str() );
            }
            else if ( 3 == itmp )
            {
                DisplayMsg_printf( "~%drd level %s%s", level, gender_str, profile->getClassName().c_str() );
            }
            else
            {
                DisplayMsg_printf( "~%dth level %s%s", level, gender_str, profile->getClassName().c_str() );
            }
        }
        else
        {
            DisplayMsg_printf( "~Dead %s", profile->getClassName().c_str() );
        }

        // Stats
        DisplayMsg_printf( "~STR:~%2d~WIS:~%2d~DEF:~%d", SFP8_TO_SINT( pchr->strength ), SFP8_TO_SINT( pchr->wisdom ), 255 - pchr->defense );
        DisplayMsg_printf( "~INT:~%2d~DEX:~%2d~EXP:~%u", SFP8_TO_SINT( pchr->intelligence ), SFP8_TO_SINT( pchr->dexterity ), pchr->experience );
    }
}
#endif

//--------------------------------------------------------------------------------------------
void show_armor( int statindex )
{
    /// @author ZF
    /// @details This function shows detailed armor information for the character

    STRING tmps;

    SKIN_T  skinlevel;

    const std::shared_ptr<Object> &pchr = _gameEngine->getActivePlayingState()->getStatusCharacter(statindex);
    if(!pchr) {
        return;
    }

    skinlevel = pchr->skin;

    const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();
    const SkinInfo &skinInfo = profile->getSkinInfo(skinlevel);

    // Armor Name
    DisplayMsg_printf( "=%s=", skinInfo.name.c_str() );

    // Armor Stats
    DisplayMsg_printf( "~DEF: %d  SLASH:%3.0f%%~CRUSH:%3.0f%% POKE:%3.0f%%", skinInfo.defence,
                       skinInfo.damageResistance[DAMAGE_SLASH]*100.0f,
                       skinInfo.damageResistance[DAMAGE_CRUSH]*100.0f,
                       skinInfo.damageResistance[DAMAGE_POKE ]*100.0f );

    DisplayMsg_printf( "~HOLY:%3.0f%%~EVIL:%3.0f%%~FIRE:%3.0f%%~ICE:%3.0f%%~ZAP:%3.0f%%",
                       skinInfo.damageResistance[DAMAGE_HOLY]*100.0f,
                       skinInfo.damageResistance[DAMAGE_EVIL]*100.0f,
                       skinInfo.damageResistance[DAMAGE_FIRE]*100.0f,
                       skinInfo.damageResistance[DAMAGE_ICE ]*100.0f,
                       skinInfo.damageResistance[DAMAGE_ZAP ]*100.0f );

    DisplayMsg_printf( "~Type: %s", skinInfo.dressy ? "Light Armor" : "Heavy Armor" );

    // jumps
    tmps[0] = CSTR_END;
    switch ( static_cast<int>(pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS)) )
    {
        case 0:  snprintf( tmps, SDL_arraysize( tmps ), "None    (%d)", (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) ); break;
        case 1:  snprintf( tmps, SDL_arraysize( tmps ), "Novice  (%d)", (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) ); break;
        case 2:  snprintf( tmps, SDL_arraysize( tmps ), "Skilled (%d)", (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) ); break;
        case 3:  snprintf( tmps, SDL_arraysize( tmps ), "Adept   (%d)", (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) ); break;
        default: snprintf( tmps, SDL_arraysize( tmps ), "Master  (%d)", (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) ); break;
    };

    DisplayMsg_printf( "~Speed:~%3.0f~Jump Skill:~%s", skinInfo.maxAccel*80, tmps );
}

//--------------------------------------------------------------------------------------------
void show_full_status( int statindex )
{
    /// @author ZF
    /// @details This function shows detailed armor information for the character including magic

    const std::shared_ptr<Object> &pchr = _gameEngine->getActivePlayingState()->getStatusCharacter(statindex);
    if(!pchr) {
        return;
    }

    SKIN_T skinlevel = pchr->skin;

    // Enchanted?
    DisplayMsg_printf( "=%s is %s=", pchr->getName().c_str(), !pchr->getActiveEnchants().empty() ? "enchanted" : "unenchanted" );

    // Armor Stats
    DisplayMsg_printf( "~DEF: %d  SLASH:%3.0f%%~CRUSH:%3.0f%% POKE:%3.0f%%", pchr->getProfile()->getSkinInfo(skinlevel).defence,
                       pchr->getDamageReduction(DAMAGE_SLASH)*100.0f,
                       pchr->getDamageReduction(DAMAGE_CRUSH)*100.0f,
                       pchr->getDamageReduction(DAMAGE_POKE) *100.0f );

    DisplayMsg_printf( "~HOLY:%3.0f%%~EVIL:%3.0f%%~FIRE:%3.0f%%~ICE:%3.0f%%~ZAP:%3.0f%%",
                       pchr->getDamageReduction(DAMAGE_HOLY)*100.0f,
                       pchr->getDamageReduction(DAMAGE_EVIL)*100.0f,
                       pchr->getDamageReduction(DAMAGE_FIRE)*100.0f,
                       pchr->getDamageReduction(DAMAGE_ICE) *100.0f,
                       pchr->getDamageReduction(DAMAGE_ZAP) *100.0f );

    DisplayMsg_printf( "Mana Regen:~%4.2f Life Regen:~%4.2f", pchr->getAttribute(Ego::Attribute::MANA_REGEN), pchr->getAttribute(Ego::Attribute::LIFE_REGEN) );
}

//--------------------------------------------------------------------------------------------
void show_magic_status( int statindex )
{
    /// @author ZF
    /// @details Displays special enchantment effects for the character

    const std::shared_ptr<Object> &pchr = _gameEngine->getActivePlayingState()->getStatusCharacter(statindex);
    if(!pchr) {
        return;
    }

    // Enchanted?
    DisplayMsg_printf( "=%s is %s=", pchr->getName().c_str(), !pchr->getActiveEnchants().empty() ? "enchanted" : "unenchanted" );

    // Enchantment status
    DisplayMsg_printf( "~See Invisible: %s~~See Kurses: %s",
                       pchr->getAttribute(Ego::Attribute::SEE_INVISIBLE) > 0 ? "Yes" : "No",
                       pchr->getAttribute(Ego::Attribute::SENSE_KURSES) > 0 ? "Yes" : "No" );

    DisplayMsg_printf( "~Channel Life: %s~~Waterwalking: %s",
                       pchr->getAttribute(Ego::Attribute::CHANNEL_LIFE) > 0 ? "Yes" : "No",
                       pchr->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 ? "Yes" : "No" );

    DisplayMsg_printf( "~Flying: %s", pchr->isFlying() ? "Yes" : "No");
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    /// @author ZZ
    /// @details This function sets all of the character's starting tilt values

    Uint8 twist;

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( object->getProfile()->hasStickyButt() )
        {
            twist = _currentModule->getMeshPointer()->get_twist( object->getTile() );
            object->ori.map_twist_facing_y = g_meshLookupTables.twist_facing_y[twist];
            object->ori.map_twist_facing_x = g_meshLookupTables.twist_facing_x[twist];
        }
        else
        {
            object->ori.map_twist_facing_y = MAP_TURN_OFFSET;
            object->ori.map_twist_facing_x = MAP_TURN_OFFSET;
        }
    }
}

//--------------------------------------------------------------------------------------------
void import_dir_profiles_vfs( const std::string &dirname )
{
    if ( nullptr == _currentModule || dirname.empty() ) return;

    if ( !_currentModule->isImportValid() ) return;

    for (int cnt = 0; cnt < _currentModule->getImportAmount()*MAX_IMPORT_PER_PLAYER; cnt++ )
    {
        std::ostringstream pathFormat;
        pathFormat << dirname << "/temp" << std::setw(4) << std::setfill('0') << cnt << ".obj";

        // Make sure the object exists...
        const std::string importPath = pathFormat.str();
        const std::string dataFilePath = importPath + "/data.txt";

        if ( vfs_exists( dataFilePath.c_str() ) )
        {
            // new player found
            if ( 0 == ( cnt % MAX_IMPORT_PER_PLAYER ) ) import_data.player++;

            // store the slot info
            import_data.slot = cnt;

            // load it
            import_data.slot_lst[cnt] = ProfileSystem::get().loadOneProfile(importPath);
            import_data.max_slot      = std::max( import_data.max_slot, cnt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_all_profiles_import()
{
    // Clear the import slots...
    import_data.slot_lst.fill(INVALID_PRO_REF);
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
    // ensure that the script parser exists
    parser_state_t& ps = parser_state_t::get();

    for (const auto &element : ProfileSystem::get().getLoadedProfiles())
    {
        const std::shared_ptr<ObjectProfile> &profile = element.second;

        //Guard agains null elements
        if(profile == nullptr) continue;

        // Load the AI script for this iobj
        std::string filePath = profile->getPathname() + "/script.txt";

        load_ai_script_vfs( ps, filePath, profile.get(), profile->getAIScript() );
    }
}

//--------------------------------------------------------------------------------------------
void game_load_module_profiles( const std::string& modname )
{
    /// @author BB
    /// @details Search for .obj directories in the module directory and load them

    vfs_search_context_t * ctxt;
    const char *filehandle;
    STRING newloadname;

    import_data.slot = -100;
    make_newloadname( modname.c_str(), "objects", newloadname );

    ctxt = vfs_findFirst( newloadname, "obj", VFS_SEARCH_DIR );
    filehandle = vfs_search_context_get_current( ctxt );

    while ( NULL != ctxt && VALID_CSTR( filehandle ) )
    {
        ProfileSystem::get().loadOneProfile(filehandle);

        ctxt = vfs_findNext( &ctxt );
        filehandle = vfs_search_context_get_current( ctxt );
    }
    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void game_load_global_profiles()
{
    // load all special objects
    ProfileSystem::get().loadOneProfile("mp_data/globalobjects/book.obj", SPELLBOOK);

    // load the objects from various import directories
    load_all_profiles_import();
}

//--------------------------------------------------------------------------------------------
bool chr_setup_apply(std::shared_ptr<Object> pchr, spawn_file_info_t *pinfo ) //note: intentonally copy and not reference on pchr
{
    Object *pparent = nullptr;
    if ( _currentModule->getObjectHandler().exists( pinfo->parent ) ) {
        pparent = _currentModule->getObjectHandler().get( pinfo->parent );
    }

    pchr->money = pchr->money + pinfo->money;
    if ( pchr->money > MAXMONEY )  pchr->money = MAXMONEY;
    if ( pchr->money < 0 )  pchr->money = 0;

    pchr->ai.content = pinfo->content;
    pchr->ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        Inventory::add_item(ObjectRef(pinfo->parent), pchr->getObjRef(), pchr->getInventory().getFirstFreeSlotNumber(), true );

        //If the character got merged into a stack, then it will be marked as terminated
        if(pchr->isTerminated()) {
            return true;
        }

        // Make spellbooks change
        SET_BIT(pchr->ai.alert, ALERTIF_GRABBED);
    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        grip_offset_t grip_off = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;

		if (rv_success == attach_character_to_mount(pchr->getObjRef(), ObjectRef(pinfo->parent), grip_off))
		{
			// Handle the "grabbed" messages
			//scr_run_chr_script(pchr);
		}
    }

    // Set the starting pinfo->level
    if ( pinfo->level > 0 )
    {
        if ( pchr->experiencelevel < pinfo->level )
        {
            pchr->experience = pchr->getProfile()->getXPNeededForLevel(pinfo->level);
        }
    }

    // automatically identify and unkurse all player starting equipment? I think yes.
    if ( !_currentModule->isImportValid() && NULL != pparent && pparent->isPlayer() )
    {
        pchr->nameknown = true;
        pchr->iskursed = false;
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
    if (ProfileSystem::get().isValidProfileID(ipro)) return false;

    // do the loading
    if ( CSTR_END != psp_info->spawn_coment[0] )
    {
        // we are relying on the virtual mount point "mp_objects", so use
        // the vfs/PHYSFS file naming conventions
        snprintf( filename, SDL_arraysize( filename ), "mp_objects/%s", psp_info->spawn_coment );

        if(!vfs_exists(filename)) {
            if(psp_info->slot > MAX_IMPORT_PER_PLAYER * MAX_PLAYER) {
				Log::get().warn("activate_spawn_file_load_object() - Object does not exist: %s\n", filename);
            }

            return false;
        }

        psp_info->slot = ProfileSystem::get().loadOneProfile(filename, psp_info->slot);
    }

    return ProfileSystem::get().isValidProfileID((PRO_REF)psp_info->slot);
}

//--------------------------------------------------------------------------------------------
bool activate_spawn_file_spawn( spawn_file_info_t * psp_info )
{
    int     local_index = 0;
    PRO_REF iprofile;

    if ( NULL == psp_info || !psp_info->do_spawn || psp_info->slot < 0 ) return false;

    iprofile = ( PRO_REF )psp_info->slot;

    // Spawn the character
    std::shared_ptr<Object> pobject = _currentModule->spawnObject(psp_info->pos, iprofile, psp_info->team, psp_info->skin, psp_info->facing, psp_info->pname == nullptr ? "" : psp_info->pname, INVALID_CHR_REF);
    if (!pobject) return false;

    // determine the attachment
    if (psp_info->attach == ATTACH_NONE)
    {
        // Free character
        psp_info->parent = pobject->getObjRef().get();
        make_one_character_matrix( pobject->getObjRef() );
    }

    chr_setup_apply(pobject, psp_info);

    //Can happen if object gets merged into a stack
    if(!pobject) {
        return true;
    }

    // Turn on PlaStack.count input devices
    if ( psp_info->stat )
    {
        // what we do depends on what kind of module we're loading
        if ( 0 == _currentModule->getImportAmount() && PlaStack.count < _currentModule->getPlayerAmount() )
        {
            // a single player module

            bool player_added;

            player_added = add_player( pobject->getObjRef(), ( PLA_REF )PlaStack.count, &InputDevices.lst[local_stats.player_count] );

            if ( _currentModule->getImportAmount() == 0 && player_added )
            {
                // !!!! make sure the player is identified !!!!
                pobject->nameknown = true;
            }
        }
        else if ( PlaStack.count < _currentModule->getImportAmount() && PlaStack.count < _currentModule->getPlayerAmount() && PlaStack.count < g_importList.count )
        {
            // A multiplayer module

            local_index = -1;
            for ( size_t tnc = 0; tnc < g_importList.count; tnc++ )
            {
                if (pobject->getProfileID() <= import_data.max_slot && ProfileSystem::get().isValidProfileID(pobject->getProfileID()))
                {
                    int islot = REF_TO_INT( pobject->getProfileID() );

                    if ( import_data.slot_lst[islot] == g_importList.lst[tnc].slot )
                    {
                        local_index = tnc;
                        break;
                    }
                }
            }

            if ( -1 != local_index )
            {
                // It's a local PlaStack.count
                add_player( pobject->getObjRef(), ( PLA_REF )PlaStack.count, &InputDevices.lst[g_importList.lst[local_index].local_player_num] );
            }
            else
            {
                // It's a remote PlaStack.count
                add_player( pobject->getObjRef(), ( PLA_REF )PlaStack.count, NULL );
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void activate_spawn_file_vfs()
{
    /// @author ZZ
    /// @details This function sets up character data, loaded from "SPAWN.TXT"
    std::unordered_map<int, std::string> reservedSlots; //Keep track of which slot numbers are reserved by their load name
    std::unordered_set<std::string> dynamicObjectList;  //references to slots that need to be dynamically loaded later
    std::vector<spawn_file_info_t> objectsToSpawn;      //The full list of objects to be spawned 

    PlaStack.count = 0;

    // Turn some back on
    ReadContext ctxt("mp_data/spawn.txt");
    if (!ctxt.ensureOpen())
    {
		std::ostringstream os;
		os << "unable to read spawn file `" << ctxt.getLoadName() << "`" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }
    {
        CHR_REF parent = INVALID_CHR_REF;

        // First load spawn data of every object.
        ctxt.next(); /// @todo Remove this hack.
        while(!ctxt.is(ReadContext::Traits::endOfInput()))
        {
            spawn_file_info_t entry;

            // Read next entry
            if(!spawn_file_read(ctxt, entry))
            {
                break; //no more entries
            }

            //Spit out a warning if they break the limit
            if ( objectsToSpawn.size() >= OBJECTS_MAX )
            {
				Log::get().warn("Too many objects in file \"%s\"! Maximum number of objects is %d.\n", ctxt.getLoadName().c_str(), OBJECTS_MAX );
                break;
            }

            // check to see if the slot is valid
            if ( entry.slot >= INVALID_PRO_REF )
            {
				Log::get().warn("Invalid slot %d for \"%s\" in file \"%s\".\n", entry.slot, entry.spawn_coment, ctxt.getLoadName().c_str() );
                continue;
            }

            //convert the spawn name into a format we like
            convert_spawn_file_load_name(&entry);

            // If it is a dynamic slot, remember to dynamically allocate it for later
            if ( entry.slot <= -1 )
            {
                dynamicObjectList.insert(entry.spawn_coment);
            }

            //its a static slot number, mark it as reserved if it isnt already
            else if (reservedSlots[entry.slot].empty())
            {
                reservedSlots[entry.slot] = entry.spawn_coment;
            }

            //Finished with this object for now
            objectsToSpawn.push_back(entry);
        }

        //Next we dynamically find slot numbers for each of the objects in the dynamic list
        for(const std::string &spawnName : dynamicObjectList)
        {
            PRO_REF profileSlot;

            //Find first free slot that is not the spellbook slot
            for (profileSlot = 1 + MAX_IMPORT_PER_PLAYER * MAX_PLAYER; profileSlot < INVALID_PRO_REF; ++profileSlot)
            {
                //don't try to grab loaded profiles
                if (ProfileSystem::get().isValidProfileID(profileSlot)) continue;

                //the slot already dynamically loaded by a different spawn object of the same type that we are, no need to reload in a new slot
                if(reservedSlots[profileSlot] == spawnName) {
                     break;
                }

                //found a completely free slot
                if (reservedSlots[profileSlot].empty())
                {
                    //Reserve this one for us
                    reservedSlots[profileSlot] = spawnName;
                    break;
                }
            }

            //If all slots are reserved, spit out a warning (very unlikely unless there is a bug somewhere)
            if ( profileSlot == INVALID_PRO_REF ) {
				Log::get().warn( "Could not allocate free dynamic slot for object (%s). All %d slots in use?\n", spawnName.c_str(), INVALID_PRO_REF );
            }
        }

        //Now spawn each object in order
        for(spawn_file_info_t &spawnInfo : objectsToSpawn)
        {
            //Do we have a parent?
            if ( spawnInfo.attach != ATTACH_NONE && parent != INVALID_CHR_REF ) {
                spawnInfo.parent = parent;
            }

            //Dynamic slot number? Then figure out what slot number is assigned to us
            if(spawnInfo.slot <= -1) {
                for(const auto &element : reservedSlots)
                {
                    if(element.second == spawnInfo.spawn_coment)
                    {
                        spawnInfo.slot = element.first;
                        break;
                    }
                }
            }

            // If nothing is already in that slot, try to load it.
            if (!ProfileSystem::get().isValidProfileID(spawnInfo.slot))
            {
                bool import_object = spawnInfo.slot > (_currentModule->getImportAmount() * MAX_IMPORT_PER_PLAYER);

                if ( !activate_spawn_file_load_object( &spawnInfo ) )
                {
                    // no, give a warning if it is useful
                    if ( import_object )
                    {
						Log::get().warn("%s:%d:%s: the object \"%s\"(slot %d) in file \"%s\" does not exist on this machine\n", \
							            __FILE__, __LINE__, __FUNCTION__, spawnInfo.spawn_coment, spawnInfo.slot, \
							            ctxt.getLoadName().c_str() );
                    }
                    continue;
                }
            }

            // we only reach this if everything was loaded properly
            activate_spawn_file_spawn(&spawnInfo);

            //We might become the new parent
            if ( spawnInfo.attach == ATTACH_NONE ) {
                parent = spawnInfo.parent;
            }
        }

        ctxt.close();
    }

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

//--------------------------------------------------------------------------------------------
void game_reset_module_data()
{
    // reset all
	Log::get().info( "Resetting module data\n" );

    // unload a lot of data
    ProfileSystem::get().reset();
    free_all_objects();
    DisplayMsg_reset();
    DisplayMsg_clear();
    game_reset_players();

    reset_end_text();
}

//--------------------------------------------------------------------------------------------
/// @details This function loads a module
bool game_load_module_data( const char *smallname )
{
    //TODO: ZF> this should be moved to Module.cpp
	Log::get().info( "Loading module \"%s\"\n", smallname );

    // ensure that the script parser exists
    (void)parser_state_t::get();

    // generate the module directory
    std::string modname = str_append_slash(smallname);

    // load a bunch of assets that are used in the module
    AudioSystem::get().loadGlobalSounds();
    ProfileSystem::get().loadGlobalParticleProfiles();

    if (read_wawalite_vfs() == nullptr) {
		Log::get().warn( "wawalite.txt not loaded for %s.\n", modname.c_str() );
    }
    upload_wawalite();

    // load all module objects
    game_load_global_profiles();            // load the global objects
    game_load_module_profiles( modname );   // load the objects from the module's directory

	std::shared_ptr<ego_mesh_t> mesh = nullptr;
	try {
		mesh = LoadMesh(modname);
	} catch (Id::Exception& ex) {
		// do not cause the program to fail, in case we are using a script function to load a module
		// just return a failure value and log a warning message for debugging purposes
		Log::get().warn("%s\n", static_cast<std::string>(ex).c_str());
		return false;
	}
	_currentModule->setMeshPointer(mesh);

    //Load passage.txt
    _currentModule->loadAllPassages();

    return true;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles(ObjectRef objectRef) {
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator()) {
        if (!particle->isTerminated() && particle->getAttachedObjectID() == objectRef) {
            particle->requestTerminate();
        }
    }
    if (_currentModule->getObjectHandler().exists(objectRef)) {
        // Set the alert for disaffirmation (wet torch).
        SET_BIT( _currentModule->getObjectHandler().get(objectRef)->ai.alert, ALERTIF_DISAFFIRMED );
    }
}

int number_of_attached_particles(ObjectRef objectRef) {
    int cnt = 0;
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator()) {
		if (particle->isAttached() && !particle->isTerminated() && particle->getAttachedObject()->getObjRef() == objectRef) {
            cnt++;
        }
    }
    return cnt;
}

int reaffirm_attached_particles(ObjectRef objectRef) {
    const std::shared_ptr<Object>& object = _currentModule->getObjectHandler()[objectRef];
    if(!object) {
        return 0;
    }

    int amount = object->getProfile()->getAttachedParticleAmount();
    if (0 == amount) return 0;

    int number_attached = number_of_attached_particles(objectRef);
    if (number_attached >= amount) return 0;

    int number_added = 0;
    for (int attempts = 0; attempts < amount && number_attached < amount; ++attempts) {
        std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnParticle( 
			object->getPosition(), object->ori.facing_z, object->getProfile()->getSlotNumber(),
			object->getProfile()->getAttachedParticleProfile(), objectRef, GRIP_LAST + number_attached,
			object->getTeam().toRef(), objectRef, INVALID_PRT_REF, number_attached);

        if (particle) {
            particle->placeAtVertex(object, particle->attachedto_vrt_off);
            number_added++;
            number_attached++;
        }
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    SET_BIT(object->ai.alert, ALERTIF_REAFFIRMED);

    return number_added;
}

//--------------------------------------------------------------------------------------------
void game_quit_module()
{
    /// @author BB
    /// @details all of the de-initialization code after the module actually ends

    // stop the module
    _currentModule.reset(nullptr);

    // get rid of the game/module data
    game_release_module_data();

    // re-initialize all game/module data
    game_reset_module_data();

    // finish whatever in-game song is playing
    AudioSystem::get().fadeAllSounds();

    // remove the module-dependent mount points from the vfs
    setup_clear_module_vfs_paths();
}

//--------------------------------------------------------------------------------------------
bool game_begin_module(const std::shared_ptr<ModuleProfile> &module)
{
    /// @author BB
    /// @details all of the initialization code before the module actually starts

    // set up the virtual file system for the module (Do before loading the module)
    if ( !setup_init_module_vfs_paths( module->getPath().c_str() ) ) return false;

    // start the module
    _currentModule = std::unique_ptr<GameModule>(new GameModule(module, time(NULL)));

    // load all the in-game module data
    if ( !game_load_module_data( module->getPath().c_str() ) )
    {    
        // release any data that might have been allocated
        game_release_module_data();
        _currentModule.reset(nullptr);
        return false;
    };

    //After loading, spawn all the data and initialize everything
    activate_spawn_file_vfs();           // read and implement the "spawn script" spawn.txt
    activate_alliance_file_vfs();        // set up the non-default team interactions

    // now load the profile AI, do last so that all reserved slot numbers are initialized
    game_load_profile_ai();

    // log debug info for every object loaded into the module
    if (egoboo_config_t::get().debug_developerMode_enable.getValue())
    {
        log_madused_vfs("/debug/slotused.txt");
    }

    // initialize the timers as the very last thing
    timeron = false;
    game_reset_timers();

    return true;
}

//--------------------------------------------------------------------------------------------
bool game_finish_module()
{
    /// @author BB
    /// @details This function saves all the players to the players dir
    ///    and also copies them into the imports dir to prepare for the next module

    // save the players and their inventories to their original directory
    if ( _currentModule->isExportValid() )
    {
        // export the players
        export_all_players( false );

        // update the import list
        import_list_t::from_players(g_importList);
    }

    // erase the data in the import folder
    vfs_removeDirectoryAndContents( "import", VFS_TRUE );

    // copy the import data back into the import folder
    game_copy_imports( &g_importList );

    return true;
}

//--------------------------------------------------------------------------------------------
void game_release_module_data()
{
    /// @author ZZ
    /// @details This function frees up memory used by the module

    // Disable ESP
    local_stats.sense_enemies_idsz = IDSZ_NONE;
    local_stats.sense_enemies_team = ( TEAM_REF ) Team::TEAM_MAX;

    // make sure that the object lists are cleared out
    free_all_objects();

    // deallocate any dynamically allocated scripting memory
    scripting_system_end();

    // deal with dynamically allocated game assets
    gfx_system_release_all_graphics();
    ProfileSystem::get().reset();
}

//--------------------------------------------------------------------------------------------
bool add_player( ObjectRef objRef, const PLA_REF playerRef, input_device_t *pdevice )
{
    /// @author ZZ
    /// @details This function adds a player, returning false if it fails, true otherwise


	if (!VALID_PLA_RANGE(playerRef)) return false;
	player_t *player = PlaStack.get_ptr(playerRef);

    // does the player already exist?
    if (player->valid) return false;

    // re-construct the players
    pla_reinit(player);

    if (!_currentModule->getObjectHandler().exists(objRef)) return false;
    Object *obj = _currentModule->getObjectHandler().get(objRef);

    // set the reference to the player
    obj->is_which_player = playerRef;

    // download the quest info
    quest_log_download_vfs(player->quest_log, obj->getProfile()->getPathname().c_str());

    //---- skeleton for using a ConfigFile to save quests
    // ppla->quest_file = quest_file_open( chr_get_dir_name(character).c_str() );

    player->index     = objRef.get();
	player->valid     = true;
	player->pdevice   = pdevice;

    if (pdevice)
    {
        local_stats.noplayers = false;
        obj->islocalplayer = true;
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
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if(object->isTerminated()) {
            continue;
        }

        //Only inventory items marked as equipment has active AI scripts
        if(object->isInsideInventory() && !object->getProfile()->isEquipment()) {
            continue;
        }
        
        // check for actions that must always be handled
        bool is_cleanedup = HAS_SOME_BITS( object->ai.alert, ALERTIF_CLEANEDUP );
        bool is_crushed   = HAS_SOME_BITS( object->ai.alert, ALERTIF_CRUSHED );

        // only let dead/destroyed things think if they have beem crushed/cleanedup
        if (object->isAlive() || is_crushed || is_cleanedup )
        {
            // Figure out alerts that weren't already set
            set_alerts(object->getObjRef().get());

            // Cleaned up characters shouldn't be alert to anything else
            if (is_cleanedup) { 
                object->ai.alert = ALERTIF_CLEANEDUP; 
                /*object->ai.timer = update_wld + 1;*/ 
            }

            // Crushed characters shouldn't be alert to anything else
            if (is_crushed)  { 
                object->ai.alert = ALERTIF_CRUSHED; 
                object->ai.timer = update_wld + 1;  //Prevents IfTimeOut from triggering
            }

            scr_run_chr_script(object.get());
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_all_objects()
{
    /// @author BB
    /// @details free every instance of the three object types used in the game.

    //free all particles
    ParticleHandler::get().clear();

    // free all the characters
    if(_currentModule) {
        _currentModule->getObjectHandler().clear();
    }

    //free all players
    PlaStack.count = 0;
    local_stats.player_count = 0;
    local_stats.noplayers = true;
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

    Object      * pchr, *ptarget;
    ai_state_t * pai;

    pchr    = !_currentModule->getObjectHandler().exists( ichr ) ? NULL : _currentModule->getObjectHandler().get( ichr );
    pai     = ( NULL == pchr )    ? NULL : &( pchr->ai );

    ptarget = (( NULL == pai ) || !_currentModule->getObjectHandler().exists( pai->target ) ) ? pchr : _currentModule->getObjectHandler().get( pai->target );
    
    cnt = 0;
    while ( CSTR_END != *src && src < src_end && dst < dst_end )
    {
        if ( '%' == *src )
        {
            char cppToCBuffer[256];
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
                        strncpy(szTmp, pchr->getName(true, false, false).c_str(), SDL_arraysize(szTmp));
                    }
                    break;

                case 'c':  // Class name
                    {
                        if ( NULL != pchr )
                        {
                            strncpy(cppToCBuffer, pchr->getProfile()->getClassName().c_str(), 256);
                            ebuffer     = cppToCBuffer;
                            ebuffer_end = ebuffer + pchr->getProfile()->getClassName().length();
                        }
                    }
                    break;

                case 't':  // Target name
                    {
                        if ( NULL != pai )
                        {
                            const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[pai->target];
                            if(target)
                            {
                                strncpy(szTmp, target->getName(true, false, false).c_str(), SDL_arraysize(szTmp));
                            }
                        }
                    }
                    break;

                case 'o':  // Owner name
                    {
                        const std::shared_ptr<Object> &owner = _currentModule->getObjectHandler()[pai->owner];
                        if(owner)
                        {
                            strncpy(szTmp, owner->getName(true, false, false).c_str(), SDL_arraysize(szTmp));
                        }
                    }
                    break;

                case 's':  // Target class name
                    {
                        if ( NULL != ptarget )
                        {
                            strncpy(cppToCBuffer, ptarget->getProfile()->getClassName().c_str(), 256);
                            ebuffer     = cppToCBuffer;
                            ebuffer_end = ebuffer + ptarget->getProfile()->getClassName().length();
                        }
                    }
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9': // Target's skin name
                    {
                        if ( NULL != ptarget )
                        {
                            strncpy(cppToCBuffer, ptarget->getProfile()->getSkinInfo((*src)-'0').name.c_str(), 256);
                            ebuffer = cppToCBuffer;
                            ebuffer_end = ebuffer + ptarget->getProfile()->getSkinInfo((*src)-'0').name.length();
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
            if ( 0 == cnt && NULL != ebuffer )  *ebuffer = Ego::toupper( *ebuffer );

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

    local_stats.sense_enemies_team = ( TEAM_REF ) Team::TEAM_MAX;
    local_stats.sense_enemies_idsz = IDSZ_NONE;

    PlaStack_reset_all();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool upload_water_layer_data( water_instance_layer_t inst[], const wawalite_water_layer_t data[], const int layer_count )
{
    if ( nullptr == inst || 0 == layer_count ) return false;

    for ( int layer = 0; layer < layer_count; layer++ )
    {
        //Reset to default
        inst[layer] = water_instance_layer_t();
    }

    // set the frame
    for ( int layer = 0; layer < layer_count; layer++ )
    {
        inst[layer]._frame = Random::next<uint16_t>(WATERFRAMEAND);
    }

    if ( nullptr != data )
    {
        for ( int layer = 0; layer < layer_count; layer++ )
        {
            const wawalite_water_layer_t * pwawa  = data + layer;
            water_instance_layer_t       * player = inst + layer;

            player->_z         = pwawa->z;
            player->_amp       = pwawa->amp;

            player->_dist      = pwawa->dist;

            player->_light_dir = pwawa->light_dir / 63.0f;
            player->_light_add = pwawa->light_add / 63.0f;

            player->_tx_add    = pwawa->tx_add;

            player->_alpha     = pwawa->alpha;

            player->_frame_add = pwawa->frame_add;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void weather_instance_t::upload(const wawalite_weather_t& source)
{
	this->iplayer = 0;

	this->timer_reset = source.timer_reset;
	this->over_water = source.over_water;
	this->part_gpip = source.part_gpip;

    // Ensure an update.
	this->time = this->timer_reset;
}

//--------------------------------------------------------------------------------------------
void fog_instance_t::upload(const wawalite_fog_t& source)
{
	_on = source.found && egoboo_config_t::get().graphic_fog_enable.getValue();
	_top = source.top;
	_bottom = source.bottom;

	_red = source.red;
	_grn = source.grn;
	_blu = source.blu;

	_distance = (source.top - source.bottom);

	_on = (_distance < 1.0f) && _on;
}

//--------------------------------------------------------------------------------------------
void damagetile_instance_t::upload(const wawalite_damagetile_t& source)
{
	this->amount.base = source.amount;
	this->amount.rand = 1;
	this->damagetype = source.damagetype;

	this->part_gpip = source.part_gpip;
	this->partand = source.partand;
	this->sound_index = CLIP(source.sound_index, INVALID_SOUND_ID, MAX_WAVE);
}

//--------------------------------------------------------------------------------------------
bool upload_animtile_data( animtile_instance_t inst[], const wawalite_animtile_t * pdata, const size_t animtile_count )
{
    if ( nullptr == inst || 0 == animtile_count ) return false;

    for(size_t cnt = 0; cnt < animtile_count; ++cnt) {
        inst[cnt] = animtile_instance_t();
    }

    for (size_t cnt = 0; cnt < animtile_count; cnt++ )
    {
        inst[cnt].frame_and  = ( 1 << ( cnt + 2 ) ) - 1;
        inst[cnt].base_and   = ~inst[cnt].frame_and;
        inst[cnt].frame_add  = 0;
    }

    if ( nullptr != pdata )
    {
        inst[0].update_and = pdata->update_and;
        inst[0].frame_and  = pdata->frame_and;
        inst[0].base_and   = ~inst[0].frame_and;

        for (size_t cnt = 1; cnt < animtile_count; cnt++ )
        {
            inst[cnt].update_and = pdata->update_and;
            inst[cnt].frame_and  = ( inst[cnt-1].frame_and << 1 ) | 1;
            inst[cnt].base_and   = ~inst[cnt].frame_and;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void upload_light_data(const wawalite_data_t& data)
{
    // Upload the lighting data.
    light_nrm = data.light.light_d;
    light_a = data.light.light_a;

    if (light_nrm.length() > 0.0f)
    {
        float length = light_nrm.length();

        // Get the extra magnitude of the direct light.
        if (gfx.usefaredge)
        {
            // We are outside, do the direct light as sunlight.
            light_d = 1.0f;
            light_a = light_a / length;
            light_a = CLIP( light_a, 0.0f, 1.0f );
        }
        else
        {
            // We are inside. take the lighting values at face value.
            //light_d = (1.0f - light_a) * fTmp;
            //light_d = CLIP(light_d, 0.0f, 1.0f);
            light_d = CLIP(length, 0.0f, 1.0f);
        }

        light_nrm *= 1.0f / length;
    }
    else
    {
		Log::get().warn("%s:%d: directional light vector is 0\n", __FILE__, __LINE__);
    }

    //make_lighttable( pdata->light_x, pdata->light_y, pdata->light_z, pdata->light_a );
    //make_lighttospek();
}

void upload_phys_data( const wawalite_physics_t& data )
{
    // upload the physics data
    Ego::Physics::g_environment.hillslide = data.hillslide;
    Ego::Physics::g_environment.slippyfriction = data.slippyfriction;
    Ego::Physics::g_environment.noslipfriction = data.noslipfriction;
    Ego::Physics::g_environment.airfriction = data.airfriction;
    Ego::Physics::g_environment.waterfriction = data.waterfriction;
    Ego::Physics::g_environment.gravity = data.gravity;
}

void upload_graphics_data( const wawalite_graphics_t& data )
{
    // Read extra data
    gfx.exploremode = data.exploremode;
    gfx.usefaredge  = data.usefaredge;
}

void upload_camera_data( const wawalite_camera_t& data )
{
    CameraSystem::get()->getCameraOptions().swing     = data.swing;
    CameraSystem::get()->getCameraOptions().swingRate = data.swing_rate;
    CameraSystem::get()->getCameraOptions().swingAmp  = data.swing_amp;
}

//--------------------------------------------------------------------------------------------
void upload_wawalite()
{
    /// @author ZZ
    /// @details This function sets up water and lighting for the module
    upload_phys_data( wawalite_data.phys );
    upload_graphics_data( wawalite_data.graphics );
    upload_light_data( wawalite_data);                         // this statement depends on data from upload_graphics_data()
    upload_camera_data( wawalite_data.camera );
    fog.upload( wawalite_data.fog );
    water.upload( wawalite_data.water );
    weather.upload( wawalite_data.weather );
    damagetile.upload( wawalite_data.damagetile );
    upload_animtile_data( animtile, &(wawalite_data.animtile ), SDL_arraysize( animtile ) );
}


//--------------------------------------------------------------------------------------------
wawalite_data_t *read_wawalite_vfs()
{
    wawalite_data_t *data = wawalite_data_read("mp_data/wawalite.txt", &wawalite_data);
    if (!data)
    {
        return nullptr;
    }

    // Fix any out-of-bounds data.
    wawalite_limit(&wawalite_data);

    // Finish up any data that has to be calculated.
    wawalite_finalize(&wawalite_data);

    return &wawalite_data;
}

//--------------------------------------------------------------------------------------------
bool wawalite_finalize(wawalite_data_t *data)
{
    /// @author BB
    /// @details coerce all parameters to in-bounds values
    if (!data) return false;

    // No weather?
    if (data->weather.weather_name == "*NONE*")
    {
        data->weather.part_gpip = LocalParticleProfileRef::Invalid;
    }
    else
    {
        std::string weather_name = data->weather.weather_name;
        Ego::tolower(weather_name);

        // Compute load paths.
        std::string prt_file = "mp_data/weather_" + weather_name + ".txt";
        std::string prt_end_file = "mp_data/weather_" + weather_name + "_finish.txt";

        // Try to load the particle files. We need at least the first particle for weather to work.
        bool success = INVALID_PIP_REF != PipStack.load_one(prt_file.c_str(), (PIP_REF)PIP_WEATHER);
        PipStack.load_one(prt_end_file.c_str(), (PIP_REF)PIP_WEATHER_FINISH);

        // Unknown weather parsed.
        if (!success)
        {
            if(weather_name != "none") 
            {
				Log::get().warn("%s:%d: failed to load weather type from wawalite.txt: %s - (%s)\n", __FILE__,__LINE__, weather_name.c_str(), prt_file.c_str());
            }
            data->weather.part_gpip = LocalParticleProfileRef::Invalid;
            data->weather.weather_name = "*NONE*";
        }
    }

    int windspeed_count = 0;
    Ego::Physics::g_environment.windspeed = Vector3f::zero();

    int waterspeed_count = 0;
    Ego::Physics::g_environment.waterspeed = Vector3f::zero();

    wawalite_water_layer_t *ilayer = wawalite_data.water.layer + 0;
    if (wawalite_data.water.background_req)
    {
        // This is a bit complicated.
        // It is the best I can do at reverse engineering what I did in render_world_background().

        const float cam_height = 1500.0f;
        const float default_bg_repeat = 4.0f;

        windspeed_count++;
        Ego::Physics::g_environment.windspeed[kX] += -ilayer->tx_add[SS] * Info<float>::Grid::Size() / (wawalite_data.water.backgroundrepeat / default_bg_repeat) * (cam_height + 1.0f / ilayer->dist[XX]) / cam_height;
        Ego::Physics::g_environment.windspeed[kY] += -ilayer->tx_add[TT] * Info<float>::Grid::Size() / (wawalite_data.water.backgroundrepeat / default_bg_repeat) * (cam_height + 1.0f / ilayer->dist[YY]) / cam_height;
        Ego::Physics::g_environment.windspeed[kZ] += -0;
    }
    else
    {
        waterspeed_count++;
		Vector3f tmp(-ilayer->tx_add[SS] * Info<float>::Grid::Size(), -ilayer->tx_add[TT] * Info<float>::Grid::Size(), 0.0f);
        Ego::Physics::g_environment.waterspeed += tmp;
    }

    ilayer = wawalite_data.water.layer + 1;
    if ( wawalite_data.water.overlay_req )
    {
        windspeed_count++;

        Ego::Physics::g_environment.windspeed[kX] += -600 * ilayer->tx_add[SS] * Info<float>::Grid::Size() / wawalite_data.water.foregroundrepeat * 0.04f;
        Ego::Physics::g_environment.windspeed[kY] += -600 * ilayer->tx_add[TT] * Info<float>::Grid::Size() / wawalite_data.water.foregroundrepeat * 0.04f;
        Ego::Physics::g_environment.windspeed[kZ] += -0;
    }
    else
    {
        waterspeed_count++;

        Ego::Physics::g_environment.waterspeed[kX] += -ilayer->tx_add[SS] * Info<float>::Grid::Size();
        Ego::Physics::g_environment.waterspeed[kY] += -ilayer->tx_add[TT] * Info<float>::Grid::Size();
        Ego::Physics::g_environment.waterspeed[kZ] += -0;
    }

    if ( waterspeed_count > 1 )
    {
        Ego::Physics::g_environment.waterspeed *= 1.0f/(float)waterspeed_count;
    }

    if ( windspeed_count > 1 )
    {
        Ego::Physics::g_environment.windspeed *= 1.0f/(float)windspeed_count;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool write_wawalite_vfs(const wawalite_data_t *data)
{
    /// @author BB
    /// @details Prepare and write the wawalite file

    if (!data) return false;

    return wawalite_data_write("mp_data/wawalite.txt",data);
}

//--------------------------------------------------------------------------------------------
uint8_t get_alpha( int alpha, float seeinvis_mag )
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

    return Ego::Math::constrain(alpha, 0, 0xFF);
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
bool do_shop_drop( ObjectRef idropper, ObjectRef iitem )
{
    if ( !_currentModule->getObjectHandler().exists( iitem ) ) return false;
    Object *pitem = _currentModule->getObjectHandler().get( iitem );

    if ( !_currentModule->getObjectHandler().exists( idropper ) ) return false;
    Object *pdropper = _currentModule->getObjectHandler().get( idropper );

    bool inshop = false;
    if ( pitem->isitem )
    {
		ObjectRef ownerRef = _currentModule->getShopOwner(pitem->getPosX(), pitem->getPosY());
        if ( _currentModule->getObjectHandler().exists(ownerRef) )
        {
            Object *owner = _currentModule->getObjectHandler().get(ownerRef);

            inshop = true;

            int price = pitem->getPrice();

            // Are they are trying to sell junk or quest items?
            if ( 0 == price )
            {
                ai_state_t::add_order(owner->ai, (Uint32)price, Passage::SHOP_BUY);
            }
            else
            {
                pdropper->money  = pdropper->money + price;
                pdropper->money  = CLIP( pdropper->money, (Sint16)0, (Sint16)MAXMONEY );

                owner->money  = owner->money - price;
                owner->money  = CLIP( owner->money, (Sint16)0, (Sint16)MAXMONEY );

                ai_state_t::add_order(owner->ai, ( Uint32 ) price, Passage::SHOP_BUY);
            }
        }
    }

    return inshop;
}

//--------------------------------------------------------------------------------------------
bool do_shop_buy( ObjectRef ibuyer, ObjectRef iitem )
{
    if ( !_currentModule->getObjectHandler().exists( iitem ) ) return false;
    Object *item = _currentModule->getObjectHandler().get( iitem );

    if ( !_currentModule->getObjectHandler().exists(ibuyer) ) return false;
    Object *buyer = _currentModule->getObjectHandler().get(ibuyer);

    bool canGrab = true;
	if (item->isitem)
	{
		ObjectRef ownerRef = _currentModule->getShopOwner(item->getPosX(), item->getPosY());
		if (_currentModule->getObjectHandler().exists(ownerRef))
		{
			Object *owner = _currentModule->getObjectHandler().get(ownerRef);

			//in_shop = true;
			int price = item->getPrice();

			if (buyer->money >= price)
			{
				// Okay to sell
				ai_state_t::add_order(owner->ai, (Uint32)price, Passage::SHOP_SELL);

				buyer->money = buyer->money - price;
				buyer->money = CLIP((int)buyer->money, 0, MAXMONEY);

				owner->money = owner->money + price;
				owner->money = CLIP((int)owner->money, 0, MAXMONEY);

				canGrab = true;
			}
			else
			{
				// Don't allow purchase
				ai_state_t::add_order(owner->ai, price, Passage::SHOP_NOAFFORD);
				canGrab = false;
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

    return canGrab;
}

//--------------------------------------------------------------------------------------------
bool do_shop_steal( ObjectRef ithief, ObjectRef iitem )
{
    // Pets can try to steal in addition to invisible characters

    bool can_steal;

    std::shared_ptr<Object> pitem = _currentModule->getObjectHandler()[iitem];
    if(!pitem) {
        return false;
    }

    std::shared_ptr<Object> pthief = _currentModule->getObjectHandler()[ithief];
    if(!pthief) {
      return false;  
    }

    can_steal = true;
    if ( pitem->isitem )
    {
        ObjectRef ownerRef = _currentModule->getShopOwner( pitem->getPosX(), pitem->getPosY() );
        if ( _currentModule->getObjectHandler().exists(ownerRef) )
        {
            int detection = Random::getPercent();
            Object *owner = _currentModule->getObjectHandler().get(ownerRef);

            can_steal = true;
            if ( owner->canSeeObject(pthief) || detection <= 5 || ( detection - pthief->getAttribute(Ego::Attribute::AGILITY) + owner->getAttribute(Ego::Attribute::INTELLECT) ) > 50 )
            {
                ai_state_t::add_order(owner->ai, Passage::SHOP_STOLEN, Passage::SHOP_THEFT);
                owner->ai.target = ithief.get();
                can_steal = false;
            }
        }
    }

    return can_steal;
}

//--------------------------------------------------------------------------------------------
bool can_grab_item_in_shop( ObjectRef igrabber, ObjectRef iitem )
{
    const std::shared_ptr<Object> &grabber = _currentModule->getObjectHandler()[igrabber];
    if(!grabber) {
        return false;
    }

    Object *item = _currentModule->getObjectHandler().get( iitem );
    if(!item) {
        return false;
    }

    // Assume there is no shop so that the character can grab anything.
    bool canGrab = true;

    // check if we are doing this inside a shop
    ObjectRef shopKeeper_ref = _currentModule->getShopOwner(item->getPosX(), item->getPosY());
    Object *shopKeeper = _currentModule->getObjectHandler().get(shopKeeper_ref);
	if (INGAME_PCHR(shopKeeper))
	{
		// check for a stealthy pickup
		bool isInvisible = !shopKeeper->canSeeObject(grabber);

		// pets are automatically stealthy
		bool canSteal = isInvisible || grabber->isItem();

		if (canSteal)
		{
			canGrab = do_shop_steal(igrabber, iitem);

			if (!canGrab)
			{
				DisplayMsg_printf("%s was detected!!", grabber->getName().c_str());
			}
			else
			{
				DisplayMsg_printf("%s stole %s", grabber->getName().c_str(), item->getName(true, false, false).c_str());
			}
		}
		else
		{
			canGrab = do_shop_buy(igrabber, iitem);
		}
	}

    return canGrab;
}
//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_1( ego_mesh_t *mesh, const Index2D& point, oct_bb_t& bump, bool waterwalk )
{
    float zdone = mesh->get_max_vertex_1( point, bump._mins[OCT_X], bump._mins[OCT_Y], bump._maxs[OCT_X], bump._maxs[OCT_Y] );

    if ( waterwalk && water._surface_level > zdone && water._is_water )
    {
        Index1D tile = mesh->getTileIndex( point );

        if ( 0 != mesh->test_fx( tile, MAPFX_WATER ) )
        {
            zdone = water._surface_level;
        }
    }

    return zdone;
}

float get_mesh_max_vertex_2( ego_mesh_t *mesh, Object *object)
{
    /// @author BB
    /// @details the object does not overlap a single grid corner. Check the 4 corners of the collision volume

	if (nullptr == mesh) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
	}
	if (nullptr == object) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == object");
	}
	
    int corner;
    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    float pos_x[4];
    float pos_y[4];
    float zmax;

    for ( corner = 0; corner < 4; corner++ )
    {
        pos_x[corner] = object->getPosX() + (( 0 == ix_off[corner] ) ? object->chr_min_cv._mins[OCT_X] : object->chr_min_cv._maxs[OCT_X] );
        pos_y[corner] = object->getPosY() + (( 0 == iy_off[corner] ) ? object->chr_min_cv._mins[OCT_Y] : object->chr_min_cv._maxs[OCT_Y] );
    }

    zmax = mesh->getElevation(Vector2f(pos_x[0], pos_y[0]), object->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 );
    for ( corner = 1; corner < 4; corner++ )
    {
        float fval = mesh->getElevation(Vector2f(pos_x[corner], pos_y[corner]), object->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 );
        zmax = std::max( zmax, fval );
    }

    return zmax;
}
//--------------------------------------------------------------------------------------------
float get_chr_level( ego_mesh_t *mesh, Object *object )
{
    float zmax;
    int ix, ixmax, ixmin;
    int iy, iymax, iymin;

    int grid_vert_count = 0;
    int grid_vert_x[1024];
    int grid_vert_y[1024];

    oct_bb_t bump;

    if (!mesh || !object || object->isTerminated()) return 0;

    // certain scenery items like doors and such just need to be able to
    // collide with the mesh. They all have 0 == pchr->bump.size
    if ( 0.0f == object->bump_stt.size )
    {
        return mesh->getElevation(Vector2f(object->getPosX(), object->getPosY()),
			                      object->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0);
    }

    // otherwise, use the small collision volume to determine which tiles the object overlaps
    // move the collision volume so that it surrounds the object
    oct_bb_t::translate(object->chr_min_cv, object->getPosition(), bump);

    // determine the size of this object in tiles
    ixmin = bump._mins[OCT_X] / Info<float>::Grid::Size(); ixmin = CLIP( ixmin, 0, int(mesh->_info.getTileCountX()) - 1 );
    ixmax = bump._maxs[OCT_X] / Info<float>::Grid::Size(); ixmax = CLIP( ixmax, 0, int(mesh->_info.getTileCountX()) - 1 );

    iymin = bump._mins[OCT_Y] / Info<float>::Grid::Size(); iymin = CLIP( iymin, 0, int(mesh->_info.getTileCountY()) - 1 );
    iymax = bump._maxs[OCT_Y] / Info<float>::Grid::Size(); iymax = CLIP( iymax, 0, int(mesh->_info.getTileCountY()) - 1 );

    // do the simplest thing if the object is just on one tile
    if ( ixmax == ixmin && iymax == iymin )
    {
        return get_mesh_max_vertex_2( mesh, object);
    }

    // otherwise, make up a list of tiles that the object might overlap
    for ( iy = iymin; iy <= iymax; iy++ )
    {
        float grid_y = iy * Info<int>::Grid::Size();

        for ( ix = ixmin; ix <= ixmax; ix++ )
        {
            float ftmp;
            float grid_x = ix * Info<int>::Grid::Size();

            ftmp = grid_x + grid_y;
            if ( ftmp < bump._mins[OCT_XY] || ftmp > bump._maxs[OCT_XY] ) continue;

            ftmp = -grid_x + grid_y;
            if ( ftmp < bump._mins[OCT_YX] || ftmp > bump._maxs[OCT_YX] ) continue;

            Index1D itile = mesh->getTileIndex(Index2D(ix, iy));
            if (Index1D::Invalid == itile ) continue;

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
        return get_mesh_max_vertex_2( mesh, object);
    }
    else
    {
        int cnt;
        float fval;

        // scan through the vertices that we know will interact with the object
        zmax = get_mesh_max_vertex_1( mesh, Index2D(grid_vert_x[0], grid_vert_y[0]), bump, object->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 );
        for ( cnt = 1; cnt < grid_vert_count; cnt ++ )
        {
            fval = get_mesh_max_vertex_1( mesh, Index2D(grid_vert_x[cnt], grid_vert_y[cnt]), bump, object->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 );
            zmax = std::max( zmax, fval );
        }
    }

    if ( zmax == -1e6 ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void import_element_t::init(import_element_t& self)
{
	self.srcDir[0] = '\0';
	self.dstDir[0] = '\0';
	self.name[0] = '\0';
	self.local_player_num = 0;
    // all non-zero, non-null values
    self.player = INVALID_PLA_REF;
    self.slot = -1;
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
		Log::get().warn("%s:%d:%s: unable to create import folder `%s`\n", __FILE__, __LINE__, __FUNCTION__, vfs_getError() );
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
			Log::get().warn( "mnu_copy_local_imports() - Failed to copy an import character \"%s\" to \"%s\" (%s)\n", import_ptr->srcDir, import_ptr->dstDir, vfs_getError() );
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
					Log::get().warn( "mnu_copy_local_imports() - Failed to copy an import inventory item \"%s\" to \"%s\" (%s)\n", tmp_src_dir, tmp_dst_dir, vfs_getError() );
                }
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void import_list_t::init(import_list_t& self)
{
    for (size_t i = 0; i < (size_t)MAX_IMPORTS; ++i)
    {
        import_element_t::init(self.lst[i]);
    }
    self.count = 0;
}

//--------------------------------------------------------------------------------------------
egolib_rv import_list_t::from_players(import_list_t& self)
{
    // blank out the ImportList list
    import_list_t::init(self);

    // generate the ImportList list from the player info
	for (PLA_REF player_idx = 0, player = 0; player_idx < MAX_PLAYER; player_idx++)
	{
		if (!VALID_PLA(player_idx)) continue;
		player_t *player_ptr = PlaStack.get_ptr(player_idx);

		CHR_REF ichr = player_ptr->index;
		if (!_currentModule->getObjectHandler().exists(ichr)) continue;
		Object *pchr = _currentModule->getObjectHandler().get(ichr);

		bool is_local = (nullptr != player_ptr->pdevice);

		// grab a pointer
		import_element_t *import_ptr = self.lst + self.count;
		self.count++;

		import_ptr->player = player_idx;
		import_ptr->slot = REF_TO_INT(player) * MAX_IMPORT_PER_PLAYER;
		import_ptr->srcDir[0] = CSTR_END;
		import_ptr->dstDir[0] = CSTR_END;
		strncpy(import_ptr->name, pchr->getName().c_str(), SDL_arraysize(import_ptr->name));

		// only copy the "source" directory if the player is local
		if (is_local)
		{
			snprintf(import_ptr->srcDir, SDL_arraysize(import_ptr->srcDir), "mp_players/%s", str_encode_path(pchr->getName()).c_str());
		}
		else
		{
			snprintf(import_ptr->srcDir, SDL_arraysize(import_ptr->srcDir), "mp_remote/%s", str_encode_path(pchr->getName()).c_str());
		}

		player++;
	}

	return (self.count > 0) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
bool check_time( Uint32 check )
{
    /// @author ZF
    /// @details Returns true if and only if all time and date specifications determined by the e_time parameter is true. This
    ///    could indicate time of the day, a specific holiday season etc.
	Ego::Time::LocalTime localTime;
    switch ( check )
    {
		// Halloween between 31th october and the 1st of november
		case SEASON_HALLOWEEN: return (( 10 == localTime.getMonth() + 1 && localTime.getDayOfMonth() >= 31 ) ||
                                       ( 11 == localTime.getMonth() + 1 && localTime.getDayOfMonth() <= 1 ) );

		// Xmas from december 16th until newyear
        case SEASON_CHRISTMAS: return ( 12 == localTime.getMonth() + 1 && localTime.getDayOfMonth() >= 16 );

		// From 0:00 to 6:00 (spooky time!)
        case TIME_NIGHT: return localTime.getHours() <= 6;

		// Its day whenever it's not night
        case TIME_DAY: return !check_time( TIME_NIGHT );

		// Unhandled check
        default: Log::get().warn( "Unhandled time enum in check_time()\n" ); return false;
    }
}

//--------------------------------------------------------------------------------------------

float water_instance_layer_t::get_level() const
{
	return _z + _amp;
}

void water_instance_layer_t::move()
{
	_tx[SS] += _tx_add[SS];
	_tx[TT] += _tx_add[TT];

	if (_tx[SS] >  1.0f) _tx[SS] -= 1.0f;
	if (_tx[TT] >  1.0f) _tx[TT] -= 1.0f;
	if (_tx[SS] < -1.0f) _tx[SS] += 1.0f;
	if (_tx[TT] < -1.0f) _tx[TT] += 1.0f;

	_frame = (_frame + _frame_add) & WATERFRAMEAND;
}

//--------------------------------------------------------------------------------------------
void water_instance_t::make(const wawalite_water_t& source)
{
    /// @author ZZ
    /// @details This function sets up water movements

	/// @todo wawalite_water_t.layer_count should be an unsigned type.
	///       layer should be the same type. 
	for (int layer = 0; layer < source.layer_count; ++layer)
    {
        _layers[layer]._tx[SS] = 0;
        _layers[layer]._tx[TT] = 0;

        for (size_t frame = 0; frame < (size_t)MAXWATERFRAME; ++frame)
        {
            // Do first mode
            for (size_t point = 0; point < (size_t)WATERPOINTS; ++point)
            {
                using namespace Ego::Math;
                float temp = (frame * twoPi<float>() / MAXWATERFRAME)
                           + (twoPi<float>() * point / WATERPOINTS) + (piOverTwo<float>() * layer / MAXWATERLAYER);
                temp = std::sin(temp);
				_layer_z_add[layer][frame][point] = temp * source.layer[layer].amp;
            }
        }
    }

    // Calculate specular highlights
	for (size_t i = 0; i < 256; ++i)
    {
        Uint8 spek = 0;
		if (i > source.spek_start)
        {
			float temp = i - source.spek_start;
			temp = temp / (256 - source.spek_start);
            temp = temp * temp;
			spek = temp * source.spek_level;
        }

        /// @note claforte@> Probably need to replace this with a
        ///           GL_DEBUG(glColor4f)(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if (!gfx.gouraudShading_enable)
            _spek[i] = 0;
        else
            _spek[i] = spek;
    }
}

void water_instance_t::upload(const wawalite_water_t& source)
{
	// upload the data
	_surface_level = source.surface_level;
    _douse_level = source.douse_level;

    _is_water = source.is_water;
    _overlay_req = source.overlay_req;
    _background_req = source.background_req;

    _light = source.light;

    _foregroundrepeat = source.foregroundrepeat;
    _backgroundrepeat = source.backgroundrepeat;

    // upload the layer data
    _layer_count = source.layer_count;
    upload_water_layer_data(_layers, source.layer, source.layer_count);

	make(source);

    // Allow slow machines to ignore the fancy stuff
    if (!egoboo_config_t::get().graphic_twoLayerWater_enable.getValue() && _layer_count > 1)
    {
        int iTmp = source.layer[0].light_add;
        iTmp = (source.layer[1].light_add * iTmp * INV_FF) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        _layer_count        = 1;
        _layers[0]._light_add = iTmp * INV_FF;
    }
}

void water_instance_t::move() {
    for (size_t i = 0; i < (size_t)MAXWATERLAYER; ++i) {
		_layers[i].move();
    }
}

void water_instance_t::set_douse_level(float level)
{
    // get the level difference
    float dlevel = level - _douse_level;

    // update all special values
    _surface_level += dlevel;
    _douse_level += dlevel;

    // update the gfx height of the water
    for (size_t i = 0; i < (size_t)MAXWATERLAYER; ++i) {
        _layers[i]._z += dlevel;
    }
}

float water_instance_t::get_level() const
{
    float level = _layers[0].get_level();

    if (egoboo_config_t::get().graphic_twoLayerWater_enable.getValue())
    {
        for (size_t i = 1; i < (size_t)MAXWATERLAYER; ++i)
        {
			level = std::max(level, _layers[i].get_level());
        }
    }

    return level;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_t::getElevation(const Vector2f& p, bool waterwalk) const
{
    float zdone = getElevation(p);
    if (waterwalk && water._surface_level > zdone && water._is_water) {
        Index1D tile = getTileIndex(p);

		if (0 != test_fx(tile, MAPFX_WATER)) {
			zdone = water._surface_level;
		}
    }
    return zdone;
}

//--------------------------------------------------------------------------------------------
bool export_one_character_quest_vfs( const char *szSaveName, ObjectRef character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    if (!_currentModule->getObjectHandler().exists(character)) return false;
    player_t *ppla = chr_get_ppla(character.get());
    if (!ppla) return false;

	egolib_rv rv = quest_log_upload_vfs( ppla->quest_log, szSaveName );
    return rv_success == rv;
}

//--------------------------------------------------------------------------------------------
bool export_one_character_name_vfs( const char *szSaveName, ObjectRef character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    if ( !_currentModule->getObjectHandler().exists( character ) ) return false;

    return RandomName::exportName(_currentModule->getObjectHandler()[character]->getName(), szSaveName);
}
