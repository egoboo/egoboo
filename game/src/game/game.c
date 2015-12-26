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

#include "egolib/egolib.h"
#include "egolib/FileFormats/Globals.hpp"

#include "game/GUI/MiniMap.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/Inventory.hpp"
#include "game/Logic/Player.hpp"
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
#include "game/ObjectAnimation.h"
#include "game/Physics/CollisionSystem.hpp"
#include "game/physics.h"
#include "game/Physics/PhysicalConstants.hpp"
#include "game/Entities/ObjectHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"

//--------------------------------------------------------------------------------------------
//Global variables! eww! TODO: remove these

bool  overrideslots      = false;

// End text
char   endtext[MAXENDTEXT] = EMPTY_CSTR;
size_t endtext_carat = 0;

WeatherState g_weatherState;
fog_instance_t fog;
AnimatedTilesState g_animatedTilesState;

import_list_t g_importList;

Uint32          clock_chr_stat   = 0;
Uint32          update_wld       = 0;

int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// game initialization / deinitialization - not accessible by scripts
static void game_reset_timers();

// looping - stuff called every loop - not accessible by scripts
static void check_stats();
static void tilt_characters_to_terrain();
static void readPlayerInput();
static void let_all_characters_think();

// module initialization / deinitialization - not accessible by scripts
static void   game_load_profile_ai();

static void   activate_spawn_file_vfs();

static bool chr_setup_apply(std::shared_ptr<Object> pchr, spawn_file_info_t& pinfo );

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

static bool activate_spawn_file_spawn( spawn_file_info_t& psp_info );
static bool activate_spawn_file_load_object( spawn_file_info_t& psp_info );
static void convert_spawn_file_load_name( spawn_file_info_t& psp_info );

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
    export_one_character_quest_vfs( todir, character );

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
    int number;

    // Stop if export isnt valid
    if ( !_currentModule->isExportValid() ) return rv_fail;

    // assume the best
    retval = rv_success;

    // Check each player
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList()) {
        ObjectRef item;

        is_local = ( nullptr != player->getInputDevice() );
        if ( require_local && !is_local ) continue;

        // Is it alive?
        std::shared_ptr<Object> pchr = player->getObject();
        if(!pchr || pchr->isTerminated()) continue;

        ObjectRef character = pchr->getObjRef();

        // don't export dead characters
        if ( !pchr->isAlive() ) continue;

        // Export the character
        export_chr_rv = export_one_character( character, character, -1, is_local );
        if ( rv_error == export_chr_rv )
        {
            retval = rv_error;
        }

        // Export the left hand item
        item = pchr->holdingwhich[SLOT_LEFT];
        if ( _currentModule->getObjectHandler().exists( item ) )
        {
            export_chr_rv = export_one_character( item, character, SLOT_LEFT, is_local );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
        }

        // Export the right hand item
        item = pchr->holdingwhich[SLOT_RIGHT];
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
    chr_stoppedby_tests = 0;

    move_all_particles();

    // Move every character
    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if(object->isTerminated()) {
            continue;
        }
        object->getObjectPhysics().updatePhysics();
        //chr_update_matrix( object.get(), true );
    }
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
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList())
    {
        // only interested in local players
        if ( nullptr == player->getInputDevice() ) continue;

        std::shared_ptr<Object> pchr = player->getObject();
        if(!pchr || pchr->isTerminated()) {
            continue;
        }

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

    // do important stuff to keep in sync inside this loop

    // keep the mpdfx lists up-to-date. No calculation is done unless one
    // of the mpdfx values was changed during the last update
    _currentModule->getMeshPointer()->_fxlists.synch( _currentModule->getMeshPointer()->_tmem, false );
    
    // Get immediate mode state for the rest of the game
    InputSystem::read_keyboard();
    InputSystem::read_mouse();
    InputSystem::read_joysticks();

    //Rebuild the quadtree for fast object lookup
    _currentModule->getObjectHandler().updateQuadTree(0.0f, 0.0f, _currentModule->getMeshPointer()->_info.getTileCountX()*Info<float>::Grid::Size(),
		                                                          _currentModule->getMeshPointer()->_info.getTileCountY()*Info<float>::Grid::Size());

    //---- begin the code for updating misc. game stuff
    {
        AudioSystem::get().updateLoopingSounds();
        BillboardSystem::get().update();
        g_animatedTilesState.animate();
        _currentModule->getWater().move();
        _currentModule->updateDamageTiles();
        _currentModule->updatePits();
        g_weatherState.animate();
    }
    //---- end the code for updating misc. game stuff

    //---- Run AI (but not on first update frame)
    if(update_wld > 0)
    {
        let_all_characters_think();           //sets the non-player latches
        readPlayerInput();                    //sets latches generated by players
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

    std::shared_ptr<ParticleProfile> ppip;

    ObjectRef besttarget = ObjectRef::Invalid;
    float  longdist2 = max_dist2;

    if ( !LOADED_PIP( particletype ) ) return ObjectRef::Invalid;
    ppip = ParticleProfileSystem::get().get_ptr( particletype );

    for(const std::shared_ptr<Object> &pchr : _currentModule->getObjectHandler().iterator())
    {
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

        bool target_friend = ppip->onlydamagefriendly && particleTeam == pchr->getTeam();
        bool target_enemy  = !ppip->onlydamagefriendly && particleTeam.hatesTeam(pchr->getTeam() );

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
bool chr_check_target( Object * psrc, const std::shared_ptr<Object>& ptst, const IDSZ2 &idsz, const BIT_FIELD targeting_bits )
{
    bool retval = false;

    // Skip non-existing objects
    if (!psrc || psrc->isTerminated()) return false;

    // Skip hidden characters
    if ( ptst->isHidden() ) return false;

    // Players only?
    if (( HAS_SOME_BITS( targeting_bits, TARGET_PLAYERS ) || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) ) && !ptst->isPlayer() ) return false;

    // Skip held objects
    if ( ptst->isBeingHeld() ) return false;

    // Allow to target ourselves?
    if ( psrc == ptst.get() && HAS_NO_BITS( targeting_bits, TARGET_SELF ) ) return false;

    // Don't target our holder if we are an item and being held
    if ( psrc->isItem() && psrc->attachedto == ptst->getObjRef() ) return false;

    // Allow to target dead stuff?
    if ( ptst->isAlive() == HAS_SOME_BITS( targeting_bits, TARGET_DEAD ) ) return false;

    // Don't target invisible stuff, unless we can actually see them
    if ( !psrc->canSeeObject(ptst) ) return false;

    //Need specific skill? ([NONE] always passes)
    if ( HAS_SOME_BITS( targeting_bits, TARGET_SKILL ) && !ptst->hasSkillIDSZ(idsz) ) return false;

    // Require player to have specific quest?
    if ( HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
    {
        if(!ptst->isPlayer()) {
            return false;
        }

        std::shared_ptr<Ego::Player>& player = _currentModule->getPlayer(ptst->is_which_player);

        // find only active quests?
        // this makes it backward-compatible with zefz's version
        if (!player->getQuestLog().hasActiveQuest(idsz)) {
            return false;
        }
    }

    bool is_hated = psrc->getTeam().hatesTeam(ptst->getTeam());

    // Target neutral items? (still target evil items, could be pets)
    if (( ptst->isItem() || ptst->isInvincible() ) && !HAS_SOME_BITS( targeting_bits, TARGET_ITEMS ) ) return false;

    // Only target those of proper team. Skip this part if it's a item
    if ( !ptst->isItem() )
    {
        if (( HAS_NO_BITS( targeting_bits, TARGET_ENEMIES ) && is_hated ) ) return false;
        if (( HAS_NO_BITS( targeting_bits, TARGET_FRIENDS ) && !is_hated ) ) return false;
    }

    //This is the last and final step! Check for specific IDSZ too? (not needed if we are looking for a quest)
    if ( IDSZ2::None == idsz || HAS_SOME_BITS( targeting_bits, TARGET_QUEST ) )
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
ObjectRef chr_find_target( Object * psrc, float max_dist, const IDSZ2& idsz, const BIT_FIELD targeting_bits )
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
        for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList())
        {
            const std::shared_ptr<Object> &object = player->getObject();
            if(player) {

                //Within range?
                float distance = (object->getPosition() - psrc->getPosition()).length();
                if(max_dist == NEAREST || distance < max_dist) {
                    searchList.push_back(object);
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

        if (!chr_check_target(psrc, ptst, idsz, targeting_bits)) continue;

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

                if ( line_of_sight_info_t::blocked( los_info, _currentModule->getMeshPointer() ) ) continue;
            }

            //Set the new best target found
            best_target = ptst->getObjRef();
            best_dist2  = dist2;
        }
    }

    return best_target;
}

//--------------------------------------------------------------------------------------------
void WeatherState::animate()
{
    //Does this module have valid weather?
    if (time < 0 || part_gpip == LocalParticleProfileRef::Invalid) {
        return;
    }

    time--;
    if (0 == time)
    {
        time = timer_reset;

        // Find a valid player
        std::shared_ptr<Ego::Player> player = nullptr;
        if(!_currentModule->getPlayerList().empty()) {
            iplayer = (iplayer + 1) % _currentModule->getPlayerList().size();
            player = _currentModule->getPlayerList()[iplayer];
        }

        // Did we find one?
        if (player)
        {
            const std::shared_ptr<Object> pchr = player->getObject();
            if (pchr)
            {
                // Yes, so spawn nearby that character
                std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnGlobalParticle(pchr->getPosition(), ATK_FRONT, part_gpip, 0, over_water);
                if (particle)
                {
                    // Weather particles spawned at the edge of the map look ugly, so don't spawn them there
                    if (particle->getPosX() < EDGE || particle->getPosX() > _currentModule->getMeshPointer()->_tmem._edge_x - EDGE)
                    {
                        particle->requestTerminate();
                    }
                    else if (particle->getPosY() < EDGE || particle->getPosY() > _currentModule->getMeshPointer()->_tmem._edge_y - EDGE)
                    {
                        particle->requestTerminate();
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void readPlayerInput()
{
    for(const std::shared_ptr<Ego::Player>& player : _currentModule->getPlayerList()) {

        //Only valid players
        const std::shared_ptr<Object> &pchr = player->getObject();
        if(!pchr || pchr->isTerminated()) {
            continue;
        }

        //Read input from the device controlling the player into object latches
        player->updateLatches();

        //Press space to respawn!
        if (keyb.is_key_down(SDLK_SPACE)
            && (local_stats.allpladead || _currentModule->canRespawnAnyTime())
            && _currentModule->isRespawnValid()
            && egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard
            && !keyb.chat_mode
            && player->getInputDevice() != nullptr)
        {
            pchr->latch.b[LATCHBUTTON_RESPAWN] = true;
        }

        // Let players respawn
        if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard && pchr->latch.b[LATCHBUTTON_RESPAWN] && _currentModule->isRespawnValid())
        {
            if (!pchr->isAlive() && 0 == local_stats.revivetimer)
            {
                pchr->respawn();
                _currentModule->getTeamList()[pchr->team].setLeader(pchr);
                SET_BIT(pchr->ai.alert, ALERTIF_CLEANEDUP);

                // cost some experience for doing this...  never lose a level
                pchr->experience *= EXPKEEP;

                //Also lose some gold in non-easy modes
                if (egoboo_config_t::get().game_difficulty.getValue() > Ego::GameDifficulty::Easy) {
                    pchr->money *= EXPKEEP;
                }
            }

            // remove all latches other than latchbutton_respawn
            pchr->latch.b[LATCHBUTTON_RESPAWN] = false;
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

    ticks = Time::now<Time::Unit::Ticks>();
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
        if ( docheat != INVALID_PLA_REF && docheat < _currentModule->getPlayerList().size() )
        {
            const std::shared_ptr<Object> &object = _currentModule->getPlayer(docheat)->getObject();
            if(object)
            {
                //Give 10% of XP needed for next level
                uint32_t xpgain = 0.1f * ( object->getProfile()->getXPNeededForLevel( std::min( object->experiencelevel+1, MAXLEVEL) ) - object->getProfile()->getXPNeededForLevel(object->experiencelevel));
                object->giveExperience(xpgain, XP_DIRECT, true);
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
        if(docheat != INVALID_PLA_REF && docheat < _currentModule->getPlayerList().size()) {
            const std::shared_ptr<Object> &object = _currentModule->getPlayer(docheat)->getObject();
            if (object)
            {
                //Heal 1 life
                object->heal(object, 256, true);
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
}

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
    import_data.slot = -100;
    std::string folderPath = modname + "/objects";

    vfs_search_context_t* ctxt = vfs_findFirst(folderPath.c_str(), "obj", VFS_SEARCH_DIR);
    const char* filehandle = vfs_search_context_get_current(ctxt);

    while (NULL != ctxt && VALID_CSTR(filehandle)) {
        ProfileSystem::get().loadOneProfile(filehandle);

        ctxt = vfs_findNext(&ctxt);
        filehandle = vfs_search_context_get_current(ctxt);
    }
    vfs_findClose(&ctxt);
}

//--------------------------------------------------------------------------------------------
bool chr_setup_apply(std::shared_ptr<Object> pchr, spawn_file_info_t& info ) //note: intentonally copy and not reference on pchr
{
    const std::shared_ptr<Object> &parentObject = _currentModule->getObjectHandler()[info.parent];

    //Add money
    pchr->money = Ego::Math::constrain(pchr->money + info.money, 0, MAXMONEY);

    //Set AI stuff
    pchr->ai.content = info.content;
    pchr->ai.passage = info.passage;

    if (info.attach == ATTACH_INVENTORY)
    {
        // Inventory character
        Inventory::add_item(info.parent, pchr->getObjRef(), pchr->getInventory().getFirstFreeSlotNumber(), true);

        //If the character got merged into a stack, then it will be marked as terminated
        if(pchr->isTerminated()) {
            return true;
        }

        // Make spellbooks change
        SET_BIT(pchr->ai.alert, ALERTIF_GRABBED);
    }
    else if (info.attach == ATTACH_LEFT || info.attach == ATTACH_RIGHT)
    {
        // Wielded character
        grip_offset_t grip_off = (ATTACH_LEFT == info.attach) ? GRIP_LEFT : GRIP_RIGHT;

        if(pchr->getObjectPhysics().attachToObject(parentObject, grip_off)) {
			// Handle the "grabbed" messages
			//scr_run_chr_script(pchr);
            UNSET_BIT(pchr->ai.alert, ALERTIF_GRABBED);
		}
    }

    // Set the starting pinfo->level
    if (info.level > 0)
    {
        if (pchr->experiencelevel < info.level) {
            pchr->experience = pchr->getProfile()->getXPNeededForLevel(info.level);
        }
    }

    // automatically identify and unkurse all player starting equipment? I think yes.
    if (!_currentModule->isImportValid() && nullptr != parentObject && parentObject->isPlayer()) {
        pchr->nameknown = true;
        pchr->iskursed = false;
    }

    return true;
}

void convert_spawn_file_load_name( spawn_file_info_t& psp_info )
{
    /// @author ZF
    /// @details This turns a spawn comment line into an actual folder name we can use to load something with

    // trim any excess spaces off the psp_info->spawn_coment
    str_trim( psp_info.spawn_comment );

    //If it is a reference to a random treasure table then get a random object from that table
    if ( '%' == psp_info.spawn_comment[0] )
    {
        std::string treasureTableName = psp_info.spawn_comment;
        std::string treasureName;
        get_random_treasure(treasureTableName, treasureName);
        strncpy(psp_info.spawn_comment, treasureName.c_str(), SDL_arraysize(psp_info.spawn_comment));
    }

    // make sure it ends with a .obj extension
    if ( NULL == strstr( psp_info.spawn_comment, ".obj" ) )
    {
        strcat( psp_info.spawn_comment, ".obj" );
    }

    // no capital letters
    strlwr( psp_info.spawn_comment );
}

//--------------------------------------------------------------------------------------------
bool activate_spawn_file_load_object( spawn_file_info_t& psp_info )
{
    /// @author BB
    /// @details Try to load a global object named int psp_info->spawn_coment into
    ///               slot psp_info->slot

    STRING filename;
    PRO_REF ipro;

    if ( psp_info.slot < 0 ) return false;

    //Is it already loaded?
    ipro = ( PRO_REF )psp_info.slot;
    if (ProfileSystem::get().isValidProfileID(ipro)) return false;

    // do the loading
    if ( CSTR_END != psp_info.spawn_comment[0] )
    {
        // we are relying on the virtual mount point "mp_objects", so use
        // the vfs/PHYSFS file naming conventions
        snprintf( filename, SDL_arraysize( filename ), "mp_objects/%s", psp_info.spawn_comment );

        if(!vfs_exists(filename)) {
            if(psp_info.slot > MAX_IMPORT_PER_PLAYER * MAX_PLAYER) {
				Log::get().warn("activate_spawn_file_load_object() - Object does not exist: %s\n", filename);
            }

            return false;
        }

        psp_info.slot = ProfileSystem::get().loadOneProfile(filename, psp_info.slot);
    }

    return ProfileSystem::get().isValidProfileID((PRO_REF)psp_info.slot);
}

//--------------------------------------------------------------------------------------------
bool activate_spawn_file_spawn( spawn_file_info_t& psp_info )
{
    int     local_index = 0;
    PRO_REF iprofile;

    if ( !psp_info.do_spawn || psp_info.slot < 0 ) return false;

    iprofile = ( PRO_REF )psp_info.slot;

    // Spawn the character
    std::shared_ptr<Object> pobject = _currentModule->spawnObject(psp_info.pos, iprofile, psp_info.team, psp_info.skin, psp_info.facing, psp_info.pname == nullptr ? "" : psp_info.pname, ObjectRef::Invalid);
    if (!pobject) return false;

    // determine the attachment
    if (psp_info.attach == ATTACH_NONE)
    {
        // Free character
        psp_info.parent = pobject->getObjRef();
        make_one_character_matrix( pobject->getObjRef() );
    }

    chr_setup_apply(pobject, psp_info);

    //Can happen if object gets merged into a stack
    if(!pobject) {
        return true;
    }

    // Turn on input devices
    if ( psp_info.stat )
    {
        // what we do depends on what kind of module we're loading
        if ( 0 == _currentModule->getImportAmount() && _currentModule->getPlayerList().size() < _currentModule->getPlayerAmount() )
        {
            // a single player module

            bool player_added = _currentModule->addPlayer(pobject, &InputDevices.lst[local_stats.player_count] );

            if ( _currentModule->getImportAmount() == 0 && player_added )
            {
                // !!!! make sure the player is identified !!!!
                pobject->nameknown = true;
            }
        }
        else if ( _currentModule->getPlayerList().size() < _currentModule->getImportAmount() && _currentModule->getPlayerList().size() < _currentModule->getPlayerAmount() && _currentModule->getPlayerList().size() < g_importList.count )
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
                // It's a local input
                _currentModule->addPlayer(pobject, &InputDevices.lst[g_importList.lst[local_index].local_player_num]);
            }
            else
            {
                // It's a remote input
                _currentModule->addPlayer(pobject, nullptr);
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
        ObjectRef parent = ObjectRef::Invalid;

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
				Log::get().warn("Invalid slot %d for \"%s\" in file \"%s\".\n", entry.slot, entry.spawn_comment, ctxt.getLoadName().c_str() );
                continue;
            }

            //convert the spawn name into a format we like
            convert_spawn_file_load_name(entry);

            // If it is a dynamic slot, remember to dynamically allocate it for later
            if ( entry.slot <= -1 )
            {
                dynamicObjectList.insert(entry.spawn_comment);
            }

            //its a static slot number, mark it as reserved if it isnt already
            else if (reservedSlots[entry.slot].empty())
            {
                reservedSlots[entry.slot] = entry.spawn_comment;
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
            if ( spawnInfo.attach != ATTACH_NONE && parent != ObjectRef::Invalid ) {
                spawnInfo.parent = parent;
            }

            //Dynamic slot number? Then figure out what slot number is assigned to us
            if(spawnInfo.slot <= -1) {
                for(const auto &element : reservedSlots)
                {
                    if(element.second == spawnInfo.spawn_comment)
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

                if ( !activate_spawn_file_load_object( spawnInfo ) )
                {
                    // no, give a warning if it is useful
                    if ( import_object )
                    {
						Log::get().warn("%s:%d:%s: the object \"%s\"(slot %d) in file \"%s\" does not exist on this machine\n", \
							            __FILE__, __LINE__, __FUNCTION__, spawnInfo.spawn_comment, spawnInfo.slot, \
							            ctxt.getLoadName().c_str() );
                    }
                    continue;
                }
            }

            // we only reach this if everything was loaded properly
            activate_spawn_file_spawn(spawnInfo);

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
			object->getTeam().toRef(), objectRef, ParticleRef::Invalid, number_attached);

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

    // deallocate any dynamically allocated scripting memory
    scripting_system_end();

    // re-initialize all game/module data
    ProfileSystem::get().reset();
    DisplayMsg_reset();
    DisplayMsg_clear();
    game_reset_players();
    reset_end_text();

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

    //Initialize player data
    game_reset_players();

    // start the module
    _currentModule = std::make_unique<GameModule>(module, time(NULL));

    //After loading, spawn all the data and initialize everything
    activate_spawn_file_vfs();           // read and implement the "spawn script" spawn.txt

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
            set_alerts(object->getObjRef());

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
void expand_escape_codes( const ObjectRef ichr, script_state_t * pstate, char * src, char * src_end, char * dst, char * dst_end )
{
    int    cnt;
    STRING szTmp;

    Object      * pchr, *ptarget;
    ai_state_t * pai;

    pchr    = !_currentModule->getObjectHandler().exists( ichr ) ? NULL : _currentModule->getObjectHandler().get( ichr );
    pai     = ( NULL == pchr )    ? NULL : &( pchr->ai );

    ptarget = (( NULL == pai ) || !_currentModule->getObjectHandler().exists( pai->getTarget() ) ) ? pchr : _currentModule->getObjectHandler().get( pai->getTarget() );
    
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
                            const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[pai->getTarget()];
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
    local_stats.player_count = 0;
    local_stats.noplayers = true;

    local_stats.seeinvis_level = 0.0f;
    local_stats.seeinvis_level = 0.0f;
    local_stats.seekurse_level = 0.0f;
    local_stats.seedark_level  = 0.0f;
    local_stats.grog_level     = 0.0f;
    local_stats.daze_level     = 0.0f;

    // Disable ESP by default
    local_stats.sense_enemies_idsz = IDSZ2::None;
    local_stats.sense_enemies_team = static_cast<TEAM_REF>(Team::TEAM_MAX);
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
void WeatherState::upload(const wawalite_weather_t& source)
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
void AnimatedTilesState::upload(const wawalite_animtile_t& source)
{
    elements.fill(Layer());

    for (size_t i = 0; i < elements.size(); ++i)
    {
        elements[i].frame_and = (1 << (i + 2)) - 1;
        elements[i].base_and = ~elements[i].frame_and;
        elements[i].frame_add = 0;
    }

    elements[0].update_and = source.update_and;
    elements[0].frame_and  = source.frame_and;
    elements[0].base_and   = ~elements[0].frame_and;

    for (size_t i = 1; i < elements.size(); ++i)
    {
        elements[i].update_and = source.update_and;
        elements[i].frame_and = (elements[i - 1].frame_and << 1) | 1;
        elements[i].base_and = ~elements[i].frame_and;
    }
}

void AnimatedTilesState::animate()
{
    for (size_t cnt = 0; cnt < 2; cnt++)
    {
        // grab the tile data
        auto& element = elements[cnt];

        // skip it if there were no updates
        if (element.frame_update_old == update_wld) continue;

        // save the old frame_add when we update to detect changes
        element.frame_add_old = element.frame_add;

        // cycle through all frames since the last time
        for (Uint32 tnc = element.frame_update_old + 1; tnc <= update_wld; tnc++)
        {
            if (0 == (tnc & element.update_and))
            {
                element.frame_add = (element.frame_add + 1) & element.frame_and;
            }
        }

        // save the frame update
        element.frame_update_old = update_wld;
    }
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
            light_d = light_a * length;
            light_a = 0.0f;
            //light_a = Ego::Math::constrain( light_a, 0.0f, 1.0f );
        }
        else
        {
            // We are inside. take the lighting values at face value.
            //light_d = (1.0f - light_a) * fTmp;
            //light_d = Ego::Math::constrain(light_d, 0.0f, 1.0f);
            light_d = Ego::Math::constrain(length, 0.0f, 1.0f);
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
    g_weatherState.upload( wawalite_data.weather );
    g_animatedTilesState.upload(wawalite_data.animtile);
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
        bool success = INVALID_PIP_REF != ParticleProfileSystem::get().load_one(prt_file.c_str(), (PIP_REF)PIP_WEATHER);
        ParticleProfileSystem::get().load_one(prt_end_file.c_str(), (PIP_REF)PIP_WEATHER_FINISH);

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

    return Ego::Math::constrain( light, 0, 0xFE );
}


//--------------------------------------------------------------------------------------------
float get_mesh_max_vertex_1( ego_mesh_t *mesh, const Index2D& point, oct_bb_t& bump, bool waterwalk )
{
    float zdone = mesh->get_max_vertex_1( point, bump._mins[OCT_X], bump._mins[OCT_Y], bump._maxs[OCT_X], bump._maxs[OCT_Y] );

    if ( waterwalk && _currentModule->getWater()._surface_level > zdone && _currentModule->getWater()._is_water )
    {
        Index1D tile = mesh->getTileIndex( point );

        if ( 0 != mesh->test_fx( tile, MAPFX_WATER ) )
        {
            zdone = _currentModule->getWater()._surface_level;
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
    ixmin = bump._mins[OCT_X] / Info<float>::Grid::Size(); ixmin = Ego::Math::constrain( ixmin, 0, int(mesh->_info.getTileCountX()) - 1 );
    ixmax = bump._maxs[OCT_X] / Info<float>::Grid::Size(); ixmax = Ego::Math::constrain( ixmax, 0, int(mesh->_info.getTileCountX()) - 1 );

    iymin = bump._mins[OCT_Y] / Info<float>::Grid::Size(); iymin = Ego::Math::constrain( iymin, 0, int(mesh->_info.getTileCountY()) - 1 );
    iymax = bump._maxs[OCT_Y] / Info<float>::Grid::Size(); iymax = Ego::Math::constrain( iymax, 0, int(mesh->_info.getTileCountY()) - 1 );

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
    for(size_t player_idx = 0; player_idx < _currentModule->getPlayerList().size(); ++player_idx) {
        const std::shared_ptr<Ego::Player>& player = _currentModule->getPlayerList()[player_idx];

		std::shared_ptr<Object> pchr = player->getObject();
        if(!pchr || pchr->isTerminated()) {
            continue;
        }

		bool is_local = (nullptr != player->getInputDevice());

		// grab a pointer
		import_element_t *import_ptr = self.lst + self.count;
		self.count++;

		import_ptr->player = player_idx;
		import_ptr->slot = player_idx * MAX_IMPORT_PER_PLAYER;
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
	}

	return (self.count > 0) ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
namespace Zeitgeist {
bool CheckTime(Time time) {
    Ego::Time::LocalTime localTime;
    switch (time)
    {
    // Halloween is from 31th october 31th (incl.) until the november 1st (incl.).
    case Time::Halloween: 
        return ((10 == localTime.getMonth() + 1 && localTime.getDayOfMonth() >= 31) ||
                (11 == localTime.getMonth() + 1 && localTime.getDayOfMonth() <= 1));

    // Chrsitmas is from december 16th (incl.) until january 1st/newyear (excl.).
    case Time::Christmas:
        return (12 == localTime.getMonth() + 1 && localTime.getDayOfMonth() >= 16);

    // From 0:00 to 6:00 (spooky time!).
    case Time::Nighttime:
        return localTime.getHours() <= 6;

     // Its day whenever it's not night.
    case Time::Daytime:
        return localTime.getHours() > 6;

    // Unhandled check.
    default:
        throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
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
        iTmp = (source.layer[1].light_add * iTmp * INV_FF<float>()) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        _layer_count        = 1;
        _layers[0]._light_add = iTmp * INV_FF<float>();
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
bool export_one_character_quest_vfs( const char *szSaveName, ObjectRef character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    const std::shared_ptr<Object> &object = _currentModule->getObjectHandler()[character];
    if(!object) {
        return false;
    }

    if(!object->isPlayer()) {
        return false;
    }

    std::shared_ptr<Ego::Player>& player = _currentModule->getPlayer(object->is_which_player);

    return player->getQuestLog().exportToFile(szSaveName);
}

//--------------------------------------------------------------------------------------------
bool export_one_character_name_vfs( const char *szSaveName, ObjectRef character )
{
    /// @author ZZ
    /// @details This function makes the naming.txt file for the character

    if ( !_currentModule->getObjectHandler().exists( character ) ) return false;

    return RandomName::exportName(_currentModule->getObjectHandler()[character]->getName(), szSaveName);
}

bool chr_do_latch_button( Object * pchr )
{
    /// @author BB
    /// @details Character latches for generalized buttons

    auto ichr = pchr->getObjRef();

    if ( !pchr->isAlive() || pchr->latch.b.none() ) return true;

    const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();

    if ( pchr->latch.b[LATCHBUTTON_JUMP] && 0 == pchr->jump_timer )
    {

        //Jump from our mount
        if (pchr->isBeingHeld())
        {
            pchr->detatchFromHolder(true, true);
            pchr->getObjectPhysics().detachFromPlatform();

            pchr->jump_timer = JUMPDELAY;
            if ( pchr->isFlying() )
            {
                pchr->vel[kZ] += DISMOUNTZVELFLY;
            }
            else
            {
                pchr->vel[kZ] += DISMOUNTZVEL;
            }

            pchr->setPosition(pchr->getPosX(), pchr->getPosY(), pchr->getPosZ() + pchr->vel[kZ]);

            if ( pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) != JUMPINFINITE && 0 != pchr->jumpnumber ) {
                pchr->jumpnumber--;
            }

            // Play the jump sound
            AudioSystem::get().playSound(pchr->getPosition(), profile->getJumpSound());
        }

        //Normal jump
        else if ( 0 != pchr->jumpnumber && !pchr->isFlying() )
        {
            if (1 != pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) || pchr->jumpready)
            {
                //Exit stealth unless character has Stalker Perk
                if(!pchr->hasPerk(Ego::Perks::STALKER)) {
                    pchr->deactivateStealth();
                }

                // Make the character jump
                float jumpPower = pchr->getAttribute(Ego::Attribute::JUMP_POWER) * 1.5f;
                pchr->hitready = true;
                pchr->jump_timer = JUMPDELAY;

                //To prevent 'bunny jumping' in water
                if (pchr->isSubmerged() || pchr->getObjectPhysics().floorIsSlippy()) {
                    pchr->jump_timer *= pchr->hasPerk(Ego::Perks::ATHLETICS) ? 2 : 4;       
                    jumpPower *= 0.5f;
                }

                pchr->vel.z() += jumpPower;
                pchr->jumpready = false;

                if (pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) != JUMPINFINITE) { 
                    pchr->jumpnumber--;
                }

                // Set to jump animation if not doing anything better
                if ( pchr->inst.action_ready )
                {
                    chr_play_action( pchr, ACTION_JA, true );
                }

                // Play the jump sound (Boing!)
                AudioSystem::get().playSound(pchr->getPosition(), profile->getJumpSound());
            }
        }

    }
    if ( pchr->latch.b[LATCHBUTTON_PACKLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        Inventory::swap_item( ichr, pchr->getInventory().getFirstFreeSlotNumber(), SLOT_LEFT, false );
    }
    if ( pchr->latch.b[LATCHBUTTON_PACKRIGHT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = PACKDELAY;
        Inventory::swap_item( ichr, pchr->getInventory().getFirstFreeSlotNumber(), SLOT_RIGHT, false );
    }

    if ( pchr->latch.b[LATCHBUTTON_ALTLEFT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        pchr->reload_timer = GRABDELAY;
        if ( !pchr->getLeftHandItem() )
        {
            // Grab left
            if(!pchr->getProfile()->getModel()->isActionValid(ACTION_ME)) {
                //No grab animation valid
                pchr->getObjectPhysics().grabStuff(GRIP_LEFT, false );
            }
            else {
                chr_play_action( pchr, ACTION_ME, false );
            }
        }
        else
        {
            // Drop left
            chr_play_action( pchr, ACTION_MA, false );
        }
    }
    if ( pchr->latch.b[LATCHBUTTON_ALTRIGHT] && pchr->inst.action_ready && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_ALTRIGHT;

        pchr->reload_timer = GRABDELAY;
        if ( !pchr->getRightHandItem() )
        {
            // Grab right
            if(!pchr->getProfile()->getModel()->isActionValid(ACTION_MF)) {
                //No grab animation valid
                pchr->getObjectPhysics().grabStuff(GRIP_RIGHT, false );
            }
            else {
                chr_play_action( pchr, ACTION_MF, false );
            }
        }
        else
        {
            // Drop right
            chr_play_action( pchr, ACTION_MB, false );
        }
    }

    // LATCHBUTTON_LEFT and LATCHBUTTON_RIGHT are mutually exclusive
    bool attack_handled = false;
    if ( !attack_handled && pchr->latch.b[LATCHBUTTON_LEFT] && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_LEFT;
        attack_handled = chr_do_latch_attack( pchr, SLOT_LEFT );
    }
    if ( !attack_handled && pchr->latch.b[LATCHBUTTON_RIGHT] && 0 == pchr->reload_timer )
    {
        //pchr->latch.b &= ~LATCHBUTTON_RIGHT;

        attack_handled = chr_do_latch_attack( pchr, SLOT_RIGHT );
    }

    return true;
}

bool chr_do_latch_attack( Object * pchr, slot_t which_slot )
{
    int base_action, hand_action, action;
    bool action_valid, allowedtoattack;

    bool retval = false;

    if (!pchr || pchr->isTerminated()) return false;
    auto iobj = GET_INDEX_PCHR( pchr );


    if (which_slot >= SLOT_COUNT) return false;

    // Which iweapon?
    auto iweapon = pchr->holdingwhich[which_slot];
    if ( !_currentModule->getObjectHandler().exists( iweapon ) )
    {
        // Unarmed means object itself is the weapon
        iweapon = iobj;
    }
    Object *pweapon = _currentModule->getObjectHandler().get(iweapon);
    const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

    // No need to continue if we have an attack cooldown
    if ( 0 != pweapon->reload_timer ) return false;

    // grab the iweapon's action
    base_action = weaponProfile->getWeaponAction();
    hand_action = pchr->getProfile()->getModel()->randomizeAction( base_action, which_slot );

    // see if the character can play this action
    action       = pchr->getProfile()->getModel()->getAction(hand_action);
    action_valid = ACTION_COUNT != action;

    // Can it do it?
    allowedtoattack = true;

    // First check if reload time and action is okay
    if ( !action_valid )
    {
        allowedtoattack = false;
    }
    else
    {
        // Then check if a skill is needed
        if ( weaponProfile->requiresSkillIDToUse() )
        {
            if (!pchr->hasSkillIDSZ(pweapon->getProfile()->getIDSZ(IDSZ_SKILL)))
            {
                allowedtoattack = false;
            }
        }
    }

    // Don't allow users with kursed weapon in the other hand to use longbows
    if ( allowedtoattack && ACTION_IS_TYPE( action, L ) )
    {
        const std::shared_ptr<Object> &offhandItem = which_slot == SLOT_LEFT ? pchr->getLeftHandItem() : pchr->getRightHandItem();
        if(offhandItem && offhandItem->iskursed) allowedtoattack = false;
    }

    if ( !allowedtoattack )
    {
        // This character can't use this iweapon
        pweapon->reload_timer = ONESECOND;
        if (pchr->getShowStatus() || egoboo_config_t::get().debug_developerMode_enable.getValue())
        {
            // Tell the player that they can't use this iweapon
            DisplayMsg_printf( "%s can't use this item...", pchr->getName(false, true, true).c_str());
        }
        return false;
    }

    if ( ACTION_DA == action )
    {
        allowedtoattack = false;
        if ( 0 == pweapon->reload_timer )
        {
            SET_BIT( pweapon->ai.alert, ALERTIF_USED );
        }
    }

    // deal with your mount (which could steal your attack)
    if ( allowedtoattack )
    {
        // Rearing mount
        const std::shared_ptr<Object> &pmount = _currentModule->getObjectHandler()[pchr->attachedto];

        if (pmount)
        {
            const std::shared_ptr<ObjectProfile> &mountProfile = pmount->getProfile();

            // let the mount steal the rider's attack
            if (!mountProfile->riderCanAttack()) allowedtoattack = false;

            // can the mount do anything?
            if ( pmount->isMount() && pmount->isAlive() )
            {
                // can the mount be told what to do?
                if ( !pmount->isPlayer() && pmount->inst.action_ready )
                {
                    if ( !ACTION_IS_TYPE( action, P ) || !mountProfile->riderCanAttack() )
                    {
                        chr_play_action( pmount.get(), Random::next((int)ACTION_UA, ACTION_UA + 1), false );
                        SET_BIT( pmount->ai.alert, ALERTIF_USED );
                        pchr->ai.lastitemused = pmount->getObjRef();

                        retval = true;
                    }
                }
            }
        }
    }

    // Attack button
    if ( allowedtoattack )
    {
        //Attacking or using an item disables stealth
        pchr->deactivateStealth();

        if ( pchr->inst.action_ready && action_valid )
        {
            //Check if we are attacking unarmed and cost mana to do so
            if(iweapon == pchr->getObjRef())
            {
                if(pchr->getProfile()->getUseManaCost() <= pchr->getMana())
                {
                    pchr->costMana(pchr->getProfile()->getUseManaCost(), pchr->getObjRef());
                }
                else
                {
                    allowedtoattack = false;
                }
            }

            if(allowedtoattack)
            {
                // randomize the action
                action = pchr->getProfile()->getModel()->randomizeAction( action, which_slot );

                // make sure it is valid
                action = pchr->getProfile()->getModel()->getAction(action);

                if ( ACTION_IS_TYPE( action, P ) )
                {
                    // we must set parry actions to be interrupted by anything
                    chr_play_action( pchr, action, true );
                }
                else
                {
                    float agility = pchr->getAttribute(Ego::Attribute::AGILITY);

                    chr_play_action( pchr, action, false );

                    // Make the weapon animate the attack as well as the character holding it
                    if (iweapon != iobj)
                    {
                        chr_play_action(pweapon, ACTION_MJ, false);
                    }

                    //Crossbow Mastery increases XBow attack speed by 30%
                    if(pchr->hasPerk(Ego::Perks::CROSSBOW_MASTERY) && 
                       pweapon->getProfile()->getIDSZ(IDSZ_PARENT).equals('X','B','O','W')) {
                        agility *= 1.30f;
                    }

                    //Determine the attack speed (how fast we play the animation)
                    pchr->inst.rate  = 0.80f;                                 //base attack speed
                    pchr->inst.rate += std::min(3.00f, agility * 0.02f);      //every Agility increases base attack speed by 2%

                    //If Quick Strike perk triggers then we have fastest possible attack (10% chance)
                    if(pchr->hasPerk(Ego::Perks::QUICK_STRIKE) && pweapon->getProfile()->isMeleeWeapon() && Random::getPercent() <= 10) {
                        pchr->inst.rate = 3.00f;
                        BillboardSystem::get().makeBillboard(pchr->getObjRef(), "Quick Strike!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Billboard::Flags::All);
                    }

                    //Add some reload time as a true limit to attacks per second
                    //Dexterity decreases the reload time for all weapons. We could allow other stats like intelligence
                    //reduce reload time for spells or gonnes here.
                    else if ( !weaponProfile->hasFastAttack() )
                    {
                        int base_reload_time = -agility;
                        if ( ACTION_IS_TYPE( action, U ) )      base_reload_time += 50;     //Unarmed  (Fists)
                        else if ( ACTION_IS_TYPE( action, T ) ) base_reload_time += 55;     //Thrust   (Spear)
                        else if ( ACTION_IS_TYPE( action, C ) ) base_reload_time += 85;     //Chop     (Axe)
                        else if ( ACTION_IS_TYPE( action, S ) ) base_reload_time += 65;     //Slice    (Sword)
                        else if ( ACTION_IS_TYPE( action, B ) ) base_reload_time += 70;     //Bash     (Mace)
                        else if ( ACTION_IS_TYPE( action, L ) ) base_reload_time += 60;     //Longbow  (Longbow)
                        else if ( ACTION_IS_TYPE( action, X ) ) base_reload_time += 130;    //Xbow     (Crossbow)
                        else if ( ACTION_IS_TYPE( action, F ) ) base_reload_time += 60;     //Flinged  (Unused)

                        //it is possible to have so high dex to eliminate all reload time
                        if ( base_reload_time > 0 ) pweapon->reload_timer += base_reload_time;
                    }
                }

                // let everyone know what we did
                pchr->ai.lastitemused = iweapon;

                /// @note ZF@> why should there any reason the weapon should NOT be alerted when it is used?
                // grab the MADFX_* flags for this action
//                BIT_FIELD action_madfx = getProfile()->getModel()->getActionFX(action);
//                if ( iweapon == ichr || HAS_NO_BITS( action, MADFX_ACTLEFT | MADFX_ACTRIGHT ) )
                {
                    SET_BIT( pweapon->ai.alert, ALERTIF_USED );
                }

                retval = true;
            }
        }
    }

    //Reset boredom timer if the attack succeeded
    if ( retval )
    {
        pchr->bore_timer = BORETIME;
    }

    return retval;
}

void character_swipe( ObjectRef ichr, slot_t slot )
{
    /// @author ZZ
    /// @details This function spawns an attack particle
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[ichr];
    if(!pchr) {
        return;
    }

    ObjectRef iweapon = pchr->holdingwhich[slot];

    // See if it's an unarmed attack...
    bool unarmed_attack;
    int spawn_vrt_offset;
    if ( !_currentModule->getObjectHandler().exists(iweapon) )
    {
        unarmed_attack   = true;
        iweapon          = ichr;
        spawn_vrt_offset = slot_to_grip_offset( slot );  // SLOT_LEFT -> GRIP_LEFT, SLOT_RIGHT -> GRIP_RIGHT
    }
    else
    {
        unarmed_attack   = false;
        spawn_vrt_offset = GRIP_LAST;
    }

    const std::shared_ptr<Object> &pweapon = _currentModule->getObjectHandler()[iweapon];
    const std::shared_ptr<ObjectProfile> &weaponProfile = pweapon->getProfile();

    // find the 1st non-item that is holding the weapon
    ObjectRef iholder = chr_get_lowest_attachment( iweapon, true );
    const std::shared_ptr<Object> &pholder = _currentModule->getObjectHandler()[iholder];

    /*
        if ( iweapon != iholder && iweapon != ichr )
        {
            // This seems to be the "proper" place to activate the held object.
            // If the attack action  of the character holding the weapon does not have
            // MADFX_ACTLEFT or MADFX_ACTRIGHT bits (and so character_swipe function is never called)
            // then the action is played and the ALERTIF_USED bit is set in the chr_do_latch_attack()
            // function.
            //
            // It would be better to move all of this to the character_swipe() function, but we cannot be assured
            // that all models have the proper bits set.

            // Make the iweapon attack too
            chr_play_action( pweapon, ACTION_MJ, false );

            SET_BIT( pweapon->ai.alert, ALERTIF_USED );
        }
    */

    // What kind of attack are we going to do?
    if ( !unarmed_attack && (( weaponProfile->isStackable() && pweapon->ammo > 1 ) || ACTION_IS_TYPE( pweapon->inst.action_which, F ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        std::shared_ptr<Object> pthrown = _currentModule->spawnObject(pchr->getPosition(), pweapon->getProfileID(), pholder->getTeam().toRef(), pweapon->skin, pchr->ori.facing_z, pweapon->getName(), ObjectRef::Invalid);
        if (pthrown)
        {
            pthrown->iskursed = false;
            pthrown->ammo = 1;
            SET_BIT( pthrown->ai.alert, ALERTIF_THROWN );

            // deterimine the throw velocity
            float velocity = MINTHROWVELOCITY;
            if ( 0 == pthrown->phys.weight )
            {
                velocity += MAXTHROWVELOCITY;
            }
            else
            {
                velocity += FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MIGHT)) / ( pthrown->phys.weight * THROWFIX );
            }
            velocity = Ego::Math::constrain( velocity, MINTHROWVELOCITY, MAXTHROWVELOCITY );

            TURN_T turn = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );
            pthrown->vel[kX] += turntocos[ turn ] * velocity;
            pthrown->vel[kY] += turntosin[ turn ] * velocity;
            pthrown->vel[kZ] = DROPZVEL;

            //Was that the last one?
            if ( pweapon->ammo <= 1 ) {
                // Poof the item
                pweapon->requestTerminate();
                return;
            }
            else {
                pweapon->ammo--;
            }
        }
    }
    else
    {
        // A generic attack. Spawn the damage particle.
        if ( 0 == pweapon->ammomax || 0 != pweapon->ammo )
        {
            if ( pweapon->ammo > 0 && !weaponProfile->isStackable() )
            {
                //Is it a wand? (Wand Mastery perk has chance to not use charge)
                if(pweapon->getProfile()->getIDSZ(IDSZ_SKILL).equals('W','A','N','D')
                    && pchr->hasPerk(Ego::Perks::WAND_MASTERY)) {

                    //1% chance per Intellect
                    if(Random::getPercent() <= pchr->getAttribute(Ego::Attribute::INTELLECT)) {
                        BillboardSystem::get().makeBillboard(pchr->getObjRef(), "Wand Mastery!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::purple(), 3, Billboard::Flags::All);
                    }
                    else {
                        pweapon->ammo--;  // Ammo usage
                    }
                }
                else {
                    pweapon->ammo--;  // Ammo usage
                }
            }

            PIP_REF attackParticle = weaponProfile->getAttackParticleProfile();
            int NR_OF_ATTACK_PARTICLES = 1;

            //Handle Double Shot perk
            if(pchr->hasPerk(Ego::Perks::DOUBLE_SHOT) && weaponProfile->getIDSZ(IDSZ_PARENT).equals('L','B','O','W'))
            {
                //1% chance per Agility
                if(Random::getPercent() <= pchr->getAttribute(Ego::Attribute::AGILITY) && pweapon->ammo > 0) {
                    NR_OF_ATTACK_PARTICLES = 2;
                    BillboardSystem::get().makeBillboard(pchr->getObjRef(), "Double Shot!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::green(), 3, Billboard::Flags::All);                    

                    //Spend one extra ammo
                    pweapon->ammo--;
                }
            }

            // Spawn an attack particle
            if (INVALID_PIP_REF != attackParticle)
            {
                for(int i = 0; i < NR_OF_ATTACK_PARTICLES; ++i)
                {
                    // make the weapon's holder the owner of the attack particle?
                    // will this mess up wands?
                    std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnParticle(pweapon->getPosition(), 
                        pchr->ori.facing_z, weaponProfile->getSlotNumber(), 
                        attackParticle, weaponProfile->hasAttachParticleToWeapon() ? iweapon : ObjectRef::Invalid,  
                        spawn_vrt_offset, pholder->getTeam().toRef(), iholder);

                    if (particle)
                    {
                        Vector3f tmp_pos = particle->getPosition();

                        if ( weaponProfile->hasAttachParticleToWeapon() )
                        {
                            particle->phys.weight     = pchr->phys.weight;
                            particle->phys.bumpdampen = pweapon->phys.bumpdampen;

                            particle->placeAtVertex(pweapon, spawn_vrt_offset);
                        }
                        else if ( particle->getProfile()->startontarget && particle->hasValidTarget() )
                        {
                            particle->placeAtVertex(particle->getTarget(), spawn_vrt_offset);

                            // Correct Z spacing base, but nothing else...
                            tmp_pos[kZ] += particle->getProfile()->getSpawnPositionOffsetZ().base;
                        }
                        else
                        {
                            // NOT ATTACHED

                            // Don't spawn in walls
                            if (EMPTY_BIT_FIELD != particle->test_wall( tmp_pos ))
                            {
                                tmp_pos[kX] = pweapon->getPosX();
                                tmp_pos[kY] = pweapon->getPosY();
                                if (EMPTY_BIT_FIELD != particle->test_wall( tmp_pos ))
                                {
                                    tmp_pos[kX] = pchr->getPosX();
                                    tmp_pos[kY] = pchr->getPosY();
                                }
                            }
                        }

                        // Initial particles get a bonus, which may be zero. Increases damage with +(factor)% per attribute point (e.g Might=10 and MightFactor=0.06 then damageBonus=0.6=60%)
                        particle->damage.base += (pchr->getAttribute(Ego::Attribute::MIGHT)     * weaponProfile->getStrengthDamageFactor());
                        particle->damage.base += (pchr->getAttribute(Ego::Attribute::INTELLECT) * weaponProfile->getIntelligenceDamageFactor());
                        particle->damage.base += (pchr->getAttribute(Ego::Attribute::AGILITY)   * weaponProfile->getDexterityDamageFactor());

                        // Initial particles get an enchantment bonus
                        particle->damage.base += pweapon->getAttribute(Ego::Attribute::DAMAGE_BONUS);

                        //Handle traits that increase weapon damage
                        float damageBonus = 1.0f;
                        switch(weaponProfile->getIDSZ(IDSZ_PARENT).toUint32())
                        {
                            //Wolverine perk gives +100% Claw damage
                            case IDSZ2::caseLabel('C','L','A','W'):
                                if(pchr->hasPerk(Ego::Perks::WOLVERINE)) {
                                    damageBonus += 1.0f;
                                }
                            break;

                            //+20% damage with polearms
                            case IDSZ2::caseLabel('P','O','L','E'):
                                if(pchr->hasPerk(Ego::Perks::POLEARM_MASTERY)) {
                                    damageBonus += 0.2f;
                                }
                            break;

                            //+20% damage with swords
                            case IDSZ2::caseLabel('S','W','O','R'):
                                if(pchr->hasPerk(Ego::Perks::SWORD_MASTERY)) {
                                    damageBonus += 0.2f;
                                }
                            break;

                            //+20% damage with Axes
                            case IDSZ2::caseLabel('A','X','E','E'):
                                if(pchr->hasPerk(Ego::Perks::AXE_MASTERY)) {
                                    damageBonus += 0.2f;
                                }       
                            break;

                            //+20% damage with Longbows
                            case IDSZ2::caseLabel('L','B','O','W'):
                                if(pchr->hasPerk(Ego::Perks::BOW_MASTERY)) {
                                    damageBonus += 0.2f;
                                }
                            break;

                            //+100% damage with Whips
                            case IDSZ2::caseLabel('W','H','I','P'):
                                if(pchr->hasPerk(Ego::Perks::WHIP_MASTERY)) {
                                    damageBonus += 1.0f;
                                }
                            break;
                        }

                        //Improvised Weapons perk gives +100% to some unusual weapons
                        if(pchr->hasPerk(Ego::Perks::IMPROVISED_WEAPONS)) {
                            if (weaponProfile->getIDSZ(IDSZ_PARENT).equals('T','O','R','C')    //Torch
                             || weaponProfile->getIDSZ(IDSZ_TYPE).equals('S','H','O','V')      //Shovel
                             || weaponProfile->getIDSZ(IDSZ_TYPE).equals('P','L','U','N')      //Toilet Plunger
                             || weaponProfile->getIDSZ(IDSZ_TYPE).equals('C','R','O','W')      //Crowbar
                             || weaponProfile->getIDSZ(IDSZ_TYPE).equals('P','I','C','K')) {   //Pick
                                damageBonus += 1.0f;
                            }
                        }

                        //Berserker perk deals +25% damage if you are below 25% life
                        if(pchr->hasPerk(Ego::Perks::BERSERKER) && pchr->getLife() <= pchr->getAttribute(Ego::Attribute::MAX_LIFE)/4) {
                            damageBonus += 0.25f;
                        }
    
                        //If it is a ranged attack then Sharpshooter increases damage by 10%
                        if(pchr->hasPerk(Ego::Perks::SHARPSHOOTER) && weaponProfile->isRangedWeapon() && DamageType_isPhysical(particle->damagetype)) {
                            damageBonus += 0.1f;
                        }

                        //+25% damage with Blunt Weapons Mastery
                        if(particle->damagetype == DAMAGE_CRUSH && pchr->hasPerk(Ego::Perks::BLUNT_WEAPONS_MASTERY) && weaponProfile->isMeleeWeapon()) {
                            damageBonus += 0.25f;
                        }

                        //If it is a melee attack then Brute perk increases damage by 10%
                        if(pchr->hasPerk(Ego::Perks::BRUTE) && weaponProfile->isMeleeWeapon()) {
                            damageBonus += 0.1f;
                        }

                        //Rally Bonus? (+10%)
                        if(pchr->hasPerk(Ego::Perks::RALLY) && update_wld < pchr->getRallyDuration()) {
                            damageBonus += 0.1f;
                        }

                        //Apply damage bonus modifiers
                        particle->damage.base *= damageBonus;
                        particle->damage.rand *= damageBonus;                            

                        //If this is a double shot particle, then add a little space between the arrows
                        if(i > 0) {
                            float x, y;
                            facing_to_vec(particle->facing, &x, &y);
                            tmp_pos[kX] -= x*32.0f;
                            tmp_pos[kY] -= x*32.0f;
                        }

                        particle->setPosition(tmp_pos);
                    }
                    else
                    {
                        Log::get().debug("%s: - unable to spawn attack particle for %s\n", __FUNCTION__, weaponProfile->getClassName().c_str());
                    }
                }
            }
            else
            {
                Log::get().debug("%s: invalid attack particle: %s\n", __FUNCTION__, weaponProfile->getClassName().c_str());
            }
        }
        else
        {
            pweapon->ammoknown = true;
        }
    }
}

//--------------------------------------------------------------------------------------------
ObjectRef chr_get_lowest_attachment( ObjectRef ichr, bool non_item )
{
    /// @author BB
    /// @details Find the lowest attachment for a given object.
    ///               This was basically taken from the script function scr_set_TargetToLowestTarget()
    ///
    ///               You should be able to find the holder of a weapon by specifying non_item == true
    ///
    ///               To prevent possible loops in the data structures, use a counter to limit
    ///               the depth of the search, and make sure that ichr != _currentModule->getObjectHandler().get(object)->attachedto

    if (!_currentModule->getObjectHandler().exists(ichr)) return ObjectRef::Invalid;

    ObjectRef original_object, object;
    original_object = object = ichr;
    for (size_t cnt = 0; cnt < OBJECTS_MAX; cnt++)
    {
        // check for one of the ending condiitons
        if (non_item && !_currentModule->getObjectHandler().get(object)->isitem)
        {
            break;
        }

        // grab the next object in the list
        ObjectRef object_next = _currentModule->getObjectHandler().get(object)->attachedto;

        // check for an end of the list
        if (!_currentModule->getObjectHandler().exists(object_next))
        {
            break;
        }

        // check for a list with a loop. shouldn't happen, but...
        if (object_next == original_object)
        {
            break;
        }

        // go to the next object
        object = object_next;
    }

    return object;
}

void playMainMenuSong()
{
    //Special xmas theme
    if (Zeitgeist::CheckTime(Zeitgeist::Time::Christmas)) {
        AudioSystem::get().playMusic("xmas.ogg");
    }

    //Special Halloween theme
    else if (Zeitgeist::CheckTime(Zeitgeist::Time::Halloween)) {
        AudioSystem::get().playMusic("halloween.ogg");
    }

    //Default egoboo theme
    else {
        AudioSystem::get().playMusic("themesong.ogg");
    }    
}
