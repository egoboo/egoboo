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

/// @file  game/bsp.c
/// @brief Global mesh, character and particle BSPs.
#include "game/bsp.h"
#include "game/char.h"
#include "game/mesh.h"
#include "game/game.h"
#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------
bool chr_BSP_is_visible(const std::shared_ptr<Object> &pchr)
{
    if (pchr->isTerminated()) return false;

    // no interactions with hidden objects
    if (pchr->isHidden()) return false;

    // no interactions with packed objects
    if (pchr->isInsideInventory()) return false;

    return true;
}

bool prt_BSP_is_visible(const std::shared_ptr<Ego::Particle> &pprt)
{
    // is the particle in-game?
    if (pprt->isTerminated() || pprt->isHidden()) return false;

    // zero sized particles are not visible
    if (0 == pprt->size)
    {
        return false;
    }
    else if (pprt->inst.valid && pprt->inst.size <= 0.0f)
    {
        return false;
    }

    return true;
}

bool chr_BSP_can_collide(const std::shared_ptr<Object> &pchr)
{
    if ( pchr->isTerminated() ) return false;

    // no interactions with hidden objects
    if ( pchr->isHidden() ) return false;

    // no interactions with packed or held objects
    if ( pchr->isBeingHeld() ) return false;

    // generic flags for character interaction
    bool can_be_reaffirmed = ( pchr->reaffirm_damagetype < DAMAGE_COUNT );
    //bool can_use_platforms = pchr->canuseplatforms;
    bool can_collide       = pchr->bump_stt.size > 0;

    // conditions for normal chr-chr interaction
    // platform tests are done elsewhere
    bool requires_chr_chr = can_collide /* || can_use_platforms */;

    // conditions for chr-prt interaction
    bool requires_chr_prt = can_be_reaffirmed;

    // even if an object does not interact with other characters,
    // it must still be inserted if it might interact with a particle
    if ( !requires_chr_chr && !requires_chr_prt ) return false;

    if (oct_bb_empty(pchr->chr_max_cv)) return false;

    return true;
}

bool prt_BSP_can_collide(const std::shared_ptr<Ego::Particle> &pprt)
{
    // Each one of these tests allows one MORE reason to include the particle, not one less.
    // Removed bump particles. We have another loop that can detect these, and there
    // is no reason to fill up the BSP with particles like coins.

    // is the particle in-game?
    if ( pprt->isTerminated() || pprt->isHidden() ) return false;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    bool has_enchant = false;
    if ( pprt->getProfile()->spawnenchant )
    {
        has_enchant = LOADED_EVE(ProfileSystem::get().getProfile(pprt->getSpawnerProfile())->getEnchantRef());
    }

    // any possible damage?
    bool does_damage = (std::abs(pprt->damage.base) + std::abs(pprt->damage.rand)) > 0;

    // the other possible status effects
    // do not require damage
    bool does_status_effect  = ( 0 != pprt->getProfile()->grogTime ) || ( 0 != pprt->getProfile()->dazeTime ) || ( 0 != pprt->getProfile()->lifeDrain ) || ( 0 != pprt->getProfile()->manaDrain );

    // these are not implemented yet
    bool does_special_effect = pprt->getProfile()->cause_pancake || pprt->getProfile()->cause_roll;

    // according to v1.0, only particles that cause damage can push
    bool can_push = does_damage && pprt->getProfile()->allowpush;
    
    /// @todo this is a stopgap solution, figure out if this is the correct place or
    ///       we need to fix the loop in fill_interaction_list instead
    bool has_bump = pprt->getProfile()->bump_height > 0 && pprt->getProfile()->bump_size > 0;

    // particles with no effect
    if ( !can_push && !has_enchant && !does_damage && !does_status_effect && !does_special_effect && !has_bump ) return false;

    return true;
}
