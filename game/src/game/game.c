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

#include "game/GameStates/PlayingState.hpp"
#include "game/Inventory.hpp"
#include "game/Logic/Player.hpp"
#include "game/link.h"
#include "game/script_implementation.h"
#include "game/egoboo.h"
#include "game/Core/GameEngine.hpp"
#include "game/Module/Passage.hpp"
#include "game/Module/Module.hpp"
#include "game/Physics/CollisionSystem.hpp"
#include "game/physics.h"
#include "game/Physics/PhysicalConstants.hpp"
#include "egolib/Entities/_Include.hpp"
#include "game/GUI/MiniMap.hpp"
#include "game/GUI/MessageLog.hpp"
#include "game/graphic.h"
#include "game/graphic_fan.h"
#include "game/Graphics/BillboardSystem.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Graphics/Billboard.hpp"
#include "game/Graphics/BillboardSystem.hpp"

//--------------------------------------------------------------------------------------------
//Global variables! eww! TODO: remove these
#define THROWFIX            30.0f                    ///< To correct thrown velocities
#define MINTHROWVELOCITY    15.0f
#define MAXTHROWVELOCITY    75.0f

bool  overrideslots      = false;

EndText g_endText;

WeatherState g_weatherState;
fog_instance_t fog;
AnimatedTilesState g_animatedTilesState;

import_list_t g_importList;

uint32_t clock_chr_stat   = 0;
uint32_t update_wld       = 0;

int chr_stoppedby_tests = 0;
int chr_pressure_tests = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// looping - stuff called every loop - not accessible by scripts
static void game_reset_players();

// implementing wawalite data


//--------------------------------------------------------------------------------------------
// Random Things
//--------------------------------------------------------------------------------------------
egolib_rv export_one_character( ObjectRef character, ObjectRef owner, int chr_obj_index, bool is_local )
{
    /// @author ZZ
    /// @details This function exports a character
    std::string fromdir;
    std::string todir;
    std::string fromfile;
    std::string tofile;
    std::string todirname;
    std::string todirfullname;

    const std::shared_ptr<Object> &object = _currentModule->getObjectHandler()[character];
    if(!object) {
        return rv_error;
    }

    if ( !_currentModule->isExportValid() || ( object->getProfile()->isItem() && !object->getProfile()->canCarryToNextModule() ) )
    {
        return rv_fail;
    }

    // TWINK_BO.OBJ
    todirname = str_encode_path(_currentModule->getObjectHandler()[owner]->getName());

    // Is it a character or an item?
    if ( chr_obj_index < 0 )
    {
        // Character directory
        todirfullname = todirname;
    }
    else
    {
        // Item is a subdirectory of the owner directory...
        std::stringstream stringStream;
        stringStream << todirname << "/" << chr_obj_index << ".obj";
        todirfullname = stringStream.str();
    }

    // players/twink.obj or players/twink.obj/0.obj
    if ( is_local )
    {
        todir = "/players/" + todirfullname;
    }
    else
    {
        todir = "/remote/" + todirfullname;
    }

    // Remove all the original info
    if ( chr_obj_index < 0 )
    {
        vfs_removeDirectoryAndContents( todir.c_str() );
        if ( !vfs_mkdir( todir ) )
        {
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create object directory ", "`", todir, "`", Log::EndOfEntry);
            return rv_error;
        }
    }

    // modules/advent.mod/objects/advent.obj
    fromdir = object->getProfile()->getPathname();

    // Build the DATA.TXT file
    if(!ObjectProfile::exportCharacterToFile(todir + "/data.txt", object.get())) {
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to save ", "`", todir, "/data.txt`", Log::EndOfEntry);
        return rv_error;
    }

    // Build the NAMING.TXT file
    tofile = todir + "/naming.txt"; /*NAMING.TXT*/
    export_one_character_name_vfs( tofile.c_str(), character );

    // Build the QUEST.TXT file
    export_one_character_quest_vfs( todir.c_str(), character );

    // copy every file that does not already exist in the todir
    {
        SearchContext *ctxt = new SearchContext(Ego::VfsPath(fromdir), VFS_SEARCH_FILE | VFS_SEARCH_BARE );
        if (!ctxt) return rv_success;
        while (ctxt->hasData()) {
            auto searchResult = ctxt->getData();
            fromfile = fromdir + "/" + searchResult.string();
            tofile = todir + "/" + searchResult.string();
            if (!vfs_exists(tofile)) {
                vfs_copyFile(fromfile, tofile);
            }
            ctxt->nextData();
        }

        delete ctxt;
        ctxt = nullptr;
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
    int number;

    // Stop if export isnt valid
    if ( !_currentModule->isExportValid() ) return rv_fail;

    // assume the best
    retval = rv_success;

    // Check each player
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList()) {
        ObjectRef item;

        // Is it alive?
        std::shared_ptr<Object> pchr = player->getObject();
        if(!pchr || pchr->isTerminated()) continue;

        ObjectRef character = pchr->getObjRef();

        // don't export dead characters
        if ( !pchr->isAlive() ) continue;

        // Export the character
        export_chr_rv = export_one_character( character, character, -1, true );
        if ( rv_error == export_chr_rv )
        {
            retval = rv_error;
        }

        // Export the left hand item
        item = pchr->holdingwhich[SLOT_LEFT];
        if ( _currentModule->getObjectHandler().exists( item ) )
        {
            export_chr_rv = export_one_character( item, character, SLOT_LEFT, true );
            if ( rv_error == export_chr_rv )
            {
                retval = rv_error;
            }
        }

        // Export the right hand item
        item = pchr->holdingwhich[SLOT_RIGHT];
        if ( _currentModule->getObjectHandler().exists( item ) )
        {
            export_chr_rv = export_one_character( item, character, SLOT_RIGHT, true );
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

            export_chr_rv = export_one_character( pitem->getObjRef(), character, number + SLOT_COUNT, true);
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
void MainLoop::move_all_objects()
{
	g_meshStats.mpdfxTests = 0;
    chr_stoppedby_tests = 0;

    // move every particle
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) {
            continue;
        }
        particle->getParticlePhysics().updatePhysics();
    }

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

void MainLoop::updateLocalStats()
{
    // Check for all local players being dead
    local_stats.allpladead      = false;
    local_stats.seeinvis_level  = 0.0f;
    local_stats.seekurse_level  = 0.0f;
    local_stats.seedark_level   = 0.0f;
    local_stats.grog_level      = 0.0f;
    local_stats.daze_level      = 0.0f;
    AudioSystem::get().setMaxHearingDistance(AudioSystem::DEFAULT_MAX_DISTANCE);

    int numdead = 0;
    int numalive = 0;
    for(const std::shared_ptr<Ego::Player> &player : _currentModule->getPlayerList())
    {
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

    // Timers
    clock_chr_stat++;

    // Reset the respawn timer
    if ( local_stats.revivetimer > 0 )
    {
        local_stats.revivetimer--;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ObjectRef prt_find_target( const Vector3f& pos, Facing facing,
                           const PIP_REF particletype, const TEAM_REF team, 
	                       ObjectRef donttarget, ObjectRef oldtarget, Facing *targetAngle )
{
    /// @author ZF
    /// @details This is the new improved targeting system for particles. Also includes distance in the Z direction.

    const float max_dist2 = WIDE * WIDE;

    std::shared_ptr<ParticleProfile> ppip;

    ObjectRef besttarget = ObjectRef::Invalid;
    float  longdist2 = max_dist2;

    facing = id::canonicalize(facing);

    if ( !LOADED_PIP( particletype ) ) return ObjectRef::Invalid;
    ppip = ProfileSystem::get().ParticleProfileSystem.get_ptr( particletype );

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
            Facing angle = Facing(FACING_T(-facing + vec_to_facing( pchr->getPosX() - pos[kX] , pchr->getPosY() - pos[kY] )));

            // Only proceed if we are facing the target
            if ( angle < Facing(ppip->targetangle) || angle > Facing( 0xFFFF - ppip->targetangle ))
            {
                float dist2 = id::squared_euclidean_norm(pchr->getPosition() - pos);

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
                float distance = id::euclidean_norm(object->getPosition() - psrc->getPosition());
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

		float dist2 = id::squared_euclidean_norm(psrc->getPosition() - ptst->getPosition());
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
void MainLoop::readPlayerInput()
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
        bool respawnRequested = false;
        if (Ego::Input::InputSystem::get().isKeyDown(SDLK_SPACE)
            && (local_stats.allpladead || _currentModule->canRespawnAnyTime())
            && _currentModule->isRespawnValid()
            && egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard)
        {
            respawnRequested = true;
        }

        // Let players respawn
        if (egoboo_config_t::get().game_difficulty.getValue() < Ego::GameDifficulty::Hard && respawnRequested && _currentModule->isRespawnValid())
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
                    pchr->giveMoney(-pchr->getMoney() * EXPKEEP);
                }
            }
        }
   }
}

//--------------------------------------------------------------------------------------------
void MainLoop::check_stats()
{
    /// @author ZZ
    /// @details This function lets the players check character stats

    static int stat_check_timer = 0;
    static int stat_check_delay = 0;

    int ticks = Time::now<Time::Unit::Ticks>();
    if ( ticks > stat_check_timer + 20 )
    {
        stat_check_timer = ticks;
    }

    stat_check_delay -= 20;
    if ( stat_check_delay > 0 )
        return;

    // Show map cheat
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && Ego::Input::InputSystem::get().isKeyDown(SDLK_m) && Ego::Input::InputSystem::get().isKeyDown(SDLK_LSHIFT))
    {
        _gameEngine->getActivePlayingState()->getMiniMap()->setVisible(true);
        _gameEngine->getActivePlayingState()->getMiniMap()->setShowPlayerPosition(true);
        stat_check_delay = 150;
    }

    // XP CHEAT
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() &&
        Ego::Input::InputSystem::get().isKeyDown(SDLK_x))
    {
        PLA_REF docheat = INVALID_PLA_REF;
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_1 ) )  docheat = 0;
        else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_2 ) )  docheat = 1;
        else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_3 ) )  docheat = 2;
        else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_4 ) )  docheat = 3;

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
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && Ego::Input::InputSystem::get().isKeyDown(SDLK_z))
    {
        PLA_REF docheat = INVALID_PLA_REF;

        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_1 ) )  docheat = 0;
        else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_2 ) )  docheat = 1;
        else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_3 ) )  docheat = 2;
        else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_4 ) )  docheat = 3;

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
    if (Ego::Input::InputSystem::get().isKeyDown( SDLK_LSHIFT ) )
    {
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_1 ) )  { show_armor( 0 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_2 ) )  { show_armor( 1 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_3 ) )  { show_armor( 2 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_4 ) )  { show_armor( 3 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_5 ) )  { show_armor( 4 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_6 ) )  { show_armor( 5 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_7 ) )  { show_armor( 6 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_8 ) )  { show_armor( 7 ); stat_check_delay = 1000; }
    }

    // Display enchantment stats?
    else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_LCTRL ) )
    {
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_1 ) )  { show_full_status( 0 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_2 ) )  { show_full_status( 1 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_3 ) )  { show_full_status( 2 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_4 ) )  { show_full_status( 3 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_5 ) )  { show_full_status( 4 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_6 ) )  { show_full_status( 5 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_7 ) )  { show_full_status( 6 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_8 ) )  { show_full_status( 7 ); stat_check_delay = 1000; }
    }

    // Display character special powers?
    else if (Ego::Input::InputSystem::get().isKeyDown( SDLK_LALT ) )
    {
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_1 ) )  { show_magic_status( 0 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_2 ) )  { show_magic_status( 1 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_3 ) )  { show_magic_status( 2 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_4 ) )  { show_magic_status( 3 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_5 ) )  { show_magic_status( 4 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_6 ) )  { show_magic_status( 5 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_7 ) )  { show_magic_status( 6 ); stat_check_delay = 1000; }
        if (Ego::Input::InputSystem::get().isKeyDown( SDLK_8 ) )  { show_magic_status( 7 ); stat_check_delay = 1000; }
    }
}

//--------------------------------------------------------------------------------------------
void show_armor( int statindex )
{
    /// @author ZF
    /// @details This function shows detailed armor information for the character

    const std::shared_ptr<Object> &pchr = _gameEngine->getActivePlayingState()->getStatusCharacter(statindex);
    if(!pchr) {
        return;
    }

    SKIN_T skinlevel = pchr->skin;

    const std::shared_ptr<ObjectProfile> &profile = pchr->getProfile();
    const SkinInfo &skinInfo = profile->getSkinInfo(skinlevel);

    // Armor Name
    DisplayMsg_printf("=%s=", skinInfo.name.c_str());

    // Armor Stats
    DisplayMsg_printf("~DEF: %d  SLASH:%3.0f%%~CRUSH:%3.0f%% POKE:%3.0f%%", skinInfo.defence,
                              skinInfo.damageResistance[DAMAGE_SLASH]*100.0f,
                              skinInfo.damageResistance[DAMAGE_CRUSH]*100.0f,
                              skinInfo.damageResistance[DAMAGE_POKE ]*100.0f);

    DisplayMsg_printf("~HOLY:%3.0f%%~EVIL:%3.0f%%~FIRE:%3.0f%%~ICE:%3.0f%%~ZAP:%3.0f%%",
                              skinInfo.damageResistance[DAMAGE_HOLY]*100.0f,
                              skinInfo.damageResistance[DAMAGE_EVIL]*100.0f,
                              skinInfo.damageResistance[DAMAGE_FIRE]*100.0f,
                              skinInfo.damageResistance[DAMAGE_ICE ]*100.0f,
                              skinInfo.damageResistance[DAMAGE_ZAP ]*100.0f );

    DisplayMsg_printf("~Type: %s", skinInfo.dressy ? "Light Armor" : "Heavy Armor");

    // jumps
    std::stringstream stringStream;
    switch ( static_cast<int>(pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS)) )
    {
        case 0:  stringStream << "None    (" << (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) << ")"; break;
        case 1:  stringStream << "Novice  (" << (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) << ")"; break;
        case 2:  stringStream << "Skilled (" << (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) << ")"; break;
        case 3:  stringStream << "Adept   (" << (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) << ")"; break;
        default: stringStream << "Master  (" << (int)pchr->getAttribute(Ego::Attribute::NUMBER_OF_JUMPS) << ")"; break;
    };

    DisplayMsg_printf( "~Speed:~%3.0f~Jump Skill:~%s", skinInfo.maxAccel*80, stringStream.str().c_str() );
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
    DisplayMsg_printf("=%s is %s=", pchr->getName().c_str(), !pchr->getActiveEnchants().empty() ? "enchanted" : "unenchanted" );

    // Armor Stats
    DisplayMsg_printf("~DEF: %d  SLASH:%3.0f%%~CRUSH:%3.0f%% POKE:%3.0f%%", pchr->getProfile()->getSkinInfo(skinlevel).defence,
                              pchr->getDamageReduction(DAMAGE_SLASH)*100.0f,
                              pchr->getDamageReduction(DAMAGE_CRUSH)*100.0f,
                              pchr->getDamageReduction(DAMAGE_POKE) *100.0f);

    DisplayMsg_printf("~HOLY:%3.0f%%~EVIL:%3.0f%%~FIRE:%3.0f%%~ICE:%3.0f%%~ZAP:%3.0f%%",
                              pchr->getDamageReduction(DAMAGE_HOLY)*100.0f,
                              pchr->getDamageReduction(DAMAGE_EVIL)*100.0f,
                              pchr->getDamageReduction(DAMAGE_FIRE)*100.0f,
                              pchr->getDamageReduction(DAMAGE_ICE) *100.0f,
                              pchr->getDamageReduction(DAMAGE_ZAP) *100.0f);

    DisplayMsg_printf("Mana Regen:~%4.2f Life Regen:~%4.2f", pchr->getAttribute(Ego::Attribute::MANA_REGEN), pchr->getAttribute(Ego::Attribute::LIFE_REGEN));
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
    DisplayMsg_printf("=%s is %s=", pchr->getName().c_str(), !pchr->getActiveEnchants().empty() ? "enchanted" : "unenchanted");

    // Enchantment status
    DisplayMsg_printf("~See Invisible: %s~~See Kurses: %s",
                              pchr->getAttribute(Ego::Attribute::SEE_INVISIBLE) > 0 ? "Yes" : "No",
                              pchr->getAttribute(Ego::Attribute::SENSE_KURSES) > 0 ? "Yes" : "No");

    DisplayMsg_printf("~Channel Life: %s~~Waterwalking: %s",
                              pchr->getAttribute(Ego::Attribute::CHANNEL_LIFE) > 0 ? "Yes" : "No",
                              pchr->getAttribute(Ego::Attribute::WALK_ON_WATER) > 0 ? "Yes" : "No");

    DisplayMsg_printf("~Flying: %s", pchr->isFlying() ? "Yes" : "No");
}

//--------------------------------------------------------------------------------------------
void game_load_module_profiles( const std::string& modname )
{
    /// @author BB
    /// @details Search for .obj directories in the module directory and load them
    import_data.slot = -100;
    std::string folderPath = modname + "/objects";

    SearchContext* ctxt = new SearchContext(Ego::VfsPath(folderPath), Ego::Extension("obj"), VFS_SEARCH_DIR);
    if (!ctxt) return;

    while (ctxt->hasData()) {
        auto searchResult = ctxt->getData();
        ProfileSystem::get().loadOneProfile(searchResult.string());
        ctxt->nextData();
    }
    delete ctxt;
    ctxt = nullptr;
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
			object->getPosition(), id::canonicalize(object->ori.facing_z), object->getProfile()->getSlotNumber(),
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

    // start the module
    _currentModule = std::make_unique<GameModule>(module, time(NULL));

    //After loading, spawn all the data and initialize everything (spawn.txt)
    //Due to dependency on the global _currentModule, we cannot do this in the constructor above
    _currentModule->spawnAllObjects();

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
    vfs_removeDirectoryAndContents( "import" );

    // copy the import data back into the import folder
    game_copy_imports( &g_importList );

    return true;
}

//--------------------------------------------------------------------------------------------
void MainLoop::let_all_characters_think()
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

    g_endText.setText("The game has ended ...");

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
}

std::string expandEscapeCodes(const std::shared_ptr<Object> &object, const script_state_t &scriptState, const std::string &text)
{
    std::stringstream result;
    bool escapeEncountered = false;

    for(const char &c : text)
    {
        if(escapeEncountered)
        {
            switch(c)
            {
                //Percentile symbol
                case '%':
                    result << '%';
                break;

                //Character name
                case 'n':
                    result << object->getName(true, false, false);
                break;

                //Class name
                case 'c':
                    result << object->getProfile()->getClassName();
                break;

                //AI target name
                case 't':
                {
                    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[object->ai.getTarget()];
                    if(target) {
                        result << target->getName();
                    }
                }
                break;

                //Owner's name
                case 'o':
                {
                    const std::shared_ptr<Object> &owner = _currentModule->getObjectHandler()[object->ai.owner];
                    if(owner) {
                        result << owner->getName(true, false, false);
                    }
                }
                break;

                //Target class name
                case 's':
                {
                    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[object->ai.getTarget()];
                    if(target) {
                        result << target->getProfile()->getClassName();
                    }
                }
                break;

                //Character's ammo
                case 'a':
                    if(object->ammoknown) {
                        result << object->getAmmo();
                    }
                    else {
                        result << '?';
                    }
                break;

                // Kurse state
                case 'k':
                    if (object->iskursed) {
                        result << "kursed";
                    }
                    else {
                        result << "unkursed";
                    }
                break;

                //Character's possessive
                case 'p':
                    if (object->gender == Gender::Female) {
                        result << "her";
                    }
                    else if (object->gender == Gender::Male) {
                        result << "his";
                    }
                    else {
                        result << "its";
                    }
                break;

                //Character's gender
                case 'm':
                    if (object->gender == Gender::Female) {
                        result << "female ";
                    }
                    else if (object->gender == Gender::Male) {
                        result << "male ";
                    }
                    else {
                        result << "other ";
                    }
                break;

                case 'g':  // Target's possessive
                {
                    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[object->ai.getTarget()];
                    if(target) {
                        if (target->gender == Gender::Female) {
                            result << "her";
                        }
                        else if (target->gender == Gender::Male) {
                            result << "his";
                        }
                        else {
                            result << "its";
                        }
                    }
                }
                break;

                //Target's skin name
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                {
                    const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[object->ai.getTarget()];
                    if(target) {
                        result << target->getProfile()->getSkinInfo(c-'0').name;
                    }
                }
                break;

                // New line (enter)
                case '#':
                    result << '\n';
                break;

                // tmpdistance value
                case 'd':
                    result << scriptState.distance;
                break;

                // tmpx value
                case 'x':
                    result << scriptState.x;
                break;

                // tmpy value
                case 'y':
                    result << scriptState.y;
                break;

                // tmpdistance value with fixed width
                case 'D':
                    result << std::setw(2) << scriptState.distance;
                break;

                //tmpx value with fixed width
                case 'X':
                    result << std::setw(2) << scriptState.x;
                break;

                //tmpy value with fixed width
                case 'Y':
                    result << std::setw(2) << scriptState.y;
                break;

                //Unknown escape character
                default:
                    result << '%' << c;
                    Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unknown escape character ",
                                                     "`", c, "`", Log::EndOfEntry);
                break;
            }

            //Escape character is now handled
            escapeEncountered = false;
            continue;
        }

        //Is it an escape character?
        if(c == '%') {
            escapeEncountered = true;
        }
        else {
            //Normal character, append to string
            result << c;
        }
    }

    //Ensure that the frist character in the string is always capitalized
    std::string stringResult = result.str();
    if(!stringResult.empty()) {
        stringResult[0] = std::toupper(stringResult[0]);
    }
    return stringResult;
}

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

void Upload::upload_light_data(const wawalite_data_t& data)
{
    // Upload the lighting data.
    light_nrm = data.light.light_d;
    light_a = data.light.light_a;

    if (id::euclidean_norm(light_nrm) > 0.0f)
    {
        float length = id::euclidean_norm(light_nrm);

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
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "directional light vector is 0", Log::EndOfEntry);
    }

    //make_lighttable( pdata->light_x, pdata->light_y, pdata->light_z, pdata->light_a );
    //make_lighttospek();
}

void Upload::upload_phys_data( const wawalite_physics_t& data )
{
    // upload the physics data
    Ego::Physics::g_environment.hillslide = data.hillslide;
    Ego::Physics::g_environment.slippyfriction = data.slippyfriction;
    Ego::Physics::g_environment.noslipfriction = data.noslipfriction;
    Ego::Physics::g_environment.airfriction = data.airfriction;
    Ego::Physics::g_environment.waterfriction = data.waterfriction;
    Ego::Physics::g_environment.gravity = data.gravity;
}

void Upload::upload_graphics_data( const wawalite_graphics_t& data )
{
    // Read extra data
    gfx.exploremode = data.exploremode;
    gfx.usefaredge  = data.usefaredge;
}

void Upload::upload_camera_data( const wawalite_camera_t& data )
{
    CameraSystem::get().getCameraOptions().swing     = data.swing;
    CameraSystem::get().getCameraOptions().swingRate = data.swing_rate;
    CameraSystem::get().getCameraOptions().swingAmp  = data.swing_amp;
}

//--------------------------------------------------------------------------------------------
void upload_wawalite()
{
    /// @author ZZ
    /// @details This function sets up water and lighting for the module
    Upload::upload_phys_data( wawalite_data.phys );
    Upload::upload_graphics_data( wawalite_data.graphics );
    Upload::upload_light_data( wawalite_data);                         // this statement depends on data from upload_graphics_data()
    Upload::upload_camera_data( wawalite_data.camera );
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
        id::to_lower_in_situ(weather_name);

        // Compute load paths.
        std::string prt_file = "mp_data/weather_" + weather_name + ".txt";
        std::string prt_end_file = "mp_data/weather_" + weather_name + "_finish.txt";

        // Try to load the particle files. We need at least the first particle for weather to work.
        bool success = INVALID_PIP_REF != ProfileSystem::get().ParticleProfileSystem.load(prt_file.c_str(), (PIP_REF)PIP_WEATHER);
        ProfileSystem::get().ParticleProfileSystem.load(prt_end_file, (PIP_REF)PIP_WEATHER_FINISH);

        // Unknown weather parsed.
        if (!success)
        {
            if(weather_name != "none") 
            {
				Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "failed to load weather type ", "`", weather_name, "`", "/", "`", prt_file, "`", " from wawalite.txt", Log::EndOfEntry);
            }
            data->weather.part_gpip = LocalParticleProfileRef::Invalid;
            data->weather.weather_name = "*NONE*";
        }
    }

    int windspeed_count = 0;
    Ego::Physics::g_environment.windspeed = id::zero<Vector3f>();

    int waterspeed_count = 0;
    Ego::Physics::g_environment.waterspeed = id::zero<Vector3f>();

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
uint8_t get_light( int light, float seedark_mag )
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
		throw id::runtime_error(__FILE__, __LINE__, "nullptr == mesh");
	}
	if (nullptr == object) {
		throw id::runtime_error(__FILE__, __LINE__, "nullptr == object");
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
    bump = id::translate(object->chr_min_cv, object->getPosition());

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
egolib_rv game_copy_imports( import_list_t * imp_lst )
{
    egolib_rv retval;

    if ( NULL == imp_lst ) return rv_error;

    if ( 0 == imp_lst->count ) return rv_success;

    // assume the best
    retval = rv_success;

    // delete the data in the directory
    vfs_removeDirectoryAndContents( "import" );
    vfs_remove_mount_point(Ego::VfsPath("import"));

    // make sure the directory exists
    if ( !vfs_mkdir( "/import" ) )
    {
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create import folder: ", vfs_getError(), Log::EndOfEntry);
        return rv_error;
    }
    vfs_add_mount_point( fs_getUserDirectory(), Ego::FsPath("import"), Ego::VfsPath("mp_import"), 1 );

    // copy all of the imports over
    for (auto import_idx = 0; import_idx < imp_lst->count; import_idx++ )
    {
        // grab the loadplayer info
        import_element_t * import_ptr = imp_lst->lst + import_idx;

        std::stringstream stringStream;
        stringStream << "/import/temp" << std::setfill('0') << std::setw(2) << import_ptr->slot << ".obj";
        import_ptr->dstDir = stringStream.str();

        if ( !vfs_copyDirectory( import_ptr->srcDir.c_str(), import_ptr->dstDir.c_str() ) )
        {
            retval = rv_error;
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "failed to copy an import character ",
                                             "from ", "`", import_ptr->srcDir, "`", " to ", "`", import_ptr->dstDir, "`", "(",
                                             vfs_getError(), ")");
        }

        // Copy all of the character's items to the import directory
        for (auto tnc = 0; tnc < MAX_IMPORT_OBJECTS; tnc++ )
        {
            stringStream.clear();
            stringStream << import_ptr->srcDir << tnc;
            auto tmp_src_dir = stringStream.str();

            // make sure the source directory exists
            if ( vfs_isDirectory( tmp_src_dir ) )
            {
                stringStream.clear();
                stringStream << "/import/temp" << std::setfill('0') << std::setw(4) << import_ptr->slot + tnc + 1 << ".obj";
                auto tmp_dst_dir = stringStream.str();
                if ( !vfs_copyDirectory( tmp_src_dir.c_str(), tmp_dst_dir.c_str() ) )
                {
                    retval = rv_error;
					Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "failed to copy an import inventory item from ",
                                                     "`", tmp_src_dir, "` to `", tmp_dst_dir, "`: ", vfs_getError());
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
        self.lst[i] = import_element_t();
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

		// grab a pointer
		import_element_t *import_ptr = self.lst + self.count;
		self.count++;

		import_ptr->player = player_idx;
		import_ptr->slot = player_idx * MAX_IMPORT_PER_PLAYER;
		import_ptr->srcDir[0] = CSTR_END;
		import_ptr->dstDir[0] = CSTR_END;
        import_ptr->name = pchr->getName();
        import_ptr->srcDir = "mp_players/" + str_encode_path(pchr->getName());
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
        throw id::unhandled_switch_case_error(__FILE__, __LINE__);
    }
}
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

bool chr_do_latch_attack( Object * pchr, slot_t which_slot )
{
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
    ModelAction base_action = static_cast<ModelAction>(weaponProfile->getWeaponAction());
    ModelAction hand_action = pchr->getProfile()->getModel()->randomizeAction( base_action, which_slot );

    // see if the character can play this action
    ModelAction action       = pchr->getProfile()->getModel()->getAction(hand_action);
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
            DisplayMsg_printf("%s can't use this item...", pchr->getName(false, true, true).c_str());
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
                if ( !pmount->isPlayer() && pmount->inst.canBeInterrupted() )
                {
                    if ( !ACTION_IS_TYPE( action, P ) || !mountProfile->riderCanAttack() )
                    {
                        const ModelAction action = pmount->getProfile()->getModel()->randomizeAction(ACTION_UA);
                        pmount->inst.playAction(action, false );
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

        if ( pchr->inst.canBeInterrupted() && action_valid )
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
                    pchr->inst.playAction(action, true);
                }
                else
                {
                    float agility = pchr->getAttribute(Ego::Attribute::AGILITY);

                    pchr->inst.playAction(action, false);

                    // Make the weapon animate the attack as well as the character holding it
                    if (iweapon != iobj)
                    {
                        pweapon->inst.playAction(ACTION_MJ, false);
                    }

                    //Crossbow Mastery increases XBow attack speed by 30%
                    if(pchr->hasPerk(Ego::Perks::CROSSBOW_MASTERY) && 
                       pweapon->getProfile()->getIDSZ(IDSZ_PARENT).equals('X','B','O','W')) {
                        agility *= 1.30f;
                    }

                    //Determine the attack speed (how fast we play the animation)
                    pchr->inst.setAnimationSpeed(0.80f + agility * 0.02f);   //every Agility increases base attack speed by 2%

                    //If Quick Strike perk triggers then we have fastest possible attack (10% chance)
                    if(pchr->hasPerk(Ego::Perks::QUICK_STRIKE) && pweapon->getProfile()->isMeleeWeapon() && Random::getPercent() <= 10) {
                        pchr->inst.setAnimationSpeed(3.0f);
                        GFX::get().getBillboardSystem().makeBillboard(pchr->getObjRef(), "Quick Strike!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::blue(), 3, Ego::Graphics::Billboard::Flags::All);
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
        pchr->resetBoredTimer();
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
    if ( !unarmed_attack && (( weaponProfile->isStackable() && pweapon->ammo > 1 ) || ACTION_IS_TYPE( pweapon->inst.getCurrentAnimation(), F ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        std::shared_ptr<Object> pthrown = _currentModule->spawnObject(pchr->getPosition(), ObjectProfileRef(pweapon->getProfileID()), pholder->getTeam().toRef(), pweapon->skin, pchr->ori.facing_z, pweapon->getName(), ObjectRef::Invalid);
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

            Facing turn = pchr->ori.facing_z + ATK_BEHIND;
            pthrown->setVelocity({pthrown->getVelocity().x() + std::cos(turn) * velocity,
                                  pthrown->getVelocity().y() + std::sin(turn) * velocity,
                                  Object::DROPZVEL});

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
                        GFX::get().getBillboardSystem().makeBillboard(pchr->getObjRef(), "Wand Mastery!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::purple(), 3, Ego::Graphics::Billboard::Flags::All);
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
                    GFX::get().getBillboardSystem().makeBillboard(pchr->getObjRef(), "Double Shot!", Ego::Math::Colour4f::white(), Ego::Math::Colour4f::green(), 3, Ego::Graphics::Billboard::Flags::All);                    

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
                        id::canonicalize(pchr->ori.facing_z), weaponProfile->getSlotNumber(), 
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
                        Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "unable to spawn attack particle for ", "`", weaponProfile->getClassName(), "`", Log::EndOfEntry);
                    }
                }
            }
            else
            {
                Log::get() << Log::Entry::create(Log::Level::Debug, __FILE__, __LINE__, "invalid attack particle ", "`", weaponProfile->getClassName(), "`", Log::EndOfEntry);
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

int DisplayMsg_printf( const char *format, ... )
{
    STRING szTmp;
    
    va_list args;
    va_start( args, format );
    int retval = vsnprintf(szTmp, SDL_arraysize(szTmp), format, args);
    DisplayMsg_print(szTmp);
    va_end( args );

    return retval;
}

void DisplayMsg_print(const std::string &text)
{
    auto state = _gameEngine->getActivePlayingState();
    if (state) state->getMessageLog()->addMessage(text);
}
