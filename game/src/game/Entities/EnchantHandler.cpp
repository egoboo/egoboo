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

/// @file  game/Entities/EnchantHandler.cpp
/// @brief Manager of enchantment entities.

#define GAME_ENTITIES_PRIVATE 1
#include "game/Entities/EnchantHandler.hpp"
#include "game/egoboo_object.h"
#include "game/Entities/Object.hpp"
#include "game/Entities/Enchant.hpp"
#include "game/char.h"

//--------------------------------------------------------------------------------------------

bool VALID_ENC_RANGE(const ENC_REF ref)
{
    return EnchantHandler::get().isValidRef(ref);
}

bool DEFINED_ENC(const ENC_REF ref)
{
    return EnchantHandler::get().DEFINED(ref);
}

bool ALLOCATED_ENC(const ENC_REF ref)
{
    return EnchantHandler::get().ALLOCATED(ref);
}

bool ACTIVE_ENC(const ENC_REF ref)
{
    return EnchantHandler::get().ACTIVE(ref);
}

bool WAITING_ENC(const ENC_REF ref)
{
    return EnchantHandler::get().WAITING(ref);
}

bool TERMINATED_ENC(const ENC_REF ref)
{
    return EnchantHandler::get().TERMINATED(ref);
}

ENC_REF GET_REF_PENC(const enc_t *ptr)
{
	return LAMBDA(!ptr, INVALID_ENC_REF, ptr->GET_REF_POBJ(INVALID_ENC_REF));
}

//--------------------------------------------------------------------------------------------

bool DEFINED_PENC(const enc_t *ptr)
{
    return EnchantHandler::get().DEFINED(ptr);
}

bool ALLOCATED_PENC(const enc_t *ptr)
{
    return EnchantHandler::get().ALLOCATED(ptr);
}

bool ACTIVE_PENC(const enc_t *ptr)
{
    return EnchantHandler::get().ACTIVE(ptr);
}

bool WAITING_PENC(const enc_t *ptr)
{
    return EnchantHandler::get().WAITING(ptr);
}

bool TERMINATED_PENC(const enc_t *ptr)
{
    return EnchantHandler::get().TERMINATED(ptr);
}

bool INGAME_ENC_BASE(const ENC_REF ref)
{
    return EnchantHandler::get().INGAME_BASE(ref);
}

bool INGAME_PENC_BASE(const enc_t *ptr)
{
    return EnchantHandler::get().INGAME_BASE(ptr);
}

bool INGAME_ENC(const ENC_REF ref)
{
    return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_ENC(ref), INGAME_ENC_BASE(ref));
}

bool INGAME_PENC(const enc_t *ptr)
{
    return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_PENC(ptr), INGAME_PENC_BASE(ptr));
}

//--------------------------------------------------------------------------------------------

static EnchantHandler EncList;

EnchantHandler& EnchantHandler::get()
{
    return EncList;
}

//--------------------------------------------------------------------------------------------
void EnchantHandler::update_used()
{
    prune_used_list();
    prune_free_list();

    // Go through the object list to see if there are any dangling objects.
    for (ENC_REF ref = 0; ref < getCount(); ++ref)
    {
        if (!isValidRef(ref)) continue;
        enc_t *x = get_ptr(ref);
        if (!POBJ_GET_PBASE(x)->ALLOCATED_PBASE()) continue;

        if (INGAME_PENC(x))
        {
            if (!x->in_used_list )
            {
                push_used(ref);
            }
        }
		else if (!POBJ_GET_PBASE(x)->DEFINED_BASE_RAW()) // We can use DEFINED_BASE_RAW as the reference is valid.
        {
            if (!x->in_free_list)
            {
                add_free_ref(ref);
            }
        }
    }

    // Blank out the unused elements of the used list.
    for (size_t i = getUsedCount(); i < getCount(); ++i)
    {
        used_ref[i] = INVALID_ENC_REF;
    }

    // Blank out the unused elements of the free list.
    for (size_t i = getFreeCount(); i < getCount(); ++i)
    {
        free_ref[i] = INVALID_ENC_REF;
    }
}

ENC_REF EnchantHandler::allocate(const ENC_REF override)
{
    ENC_REF ref = INVALID_ENC_REF;

    if (isValidRef(override))
    {
        ref = pop_free();
        if ( override != ref )
        {
            size_t override_index = find_free_ref( override );

            if ( override_index == std::numeric_limits<size_t>::max() || override_index >= freeCount )
            {
                ref = INVALID_ENC_REF;
            }
            else
            {
                // store the "wrong" value in the override enchant's index
                free_ref[override_index] = ref;

                // fix the in_free_list values
                get_ptr(ref)->in_free_list = true;
                get_ptr(override)->in_free_list = false;

                ref = override;
            }
        }

        if ( INVALID_ENC_REF == ref )
        {
            log_warning("%s:%d: failed to override object %d - object already spawned? \n", __FILE__,__LINE__, REF_TO_INT( override ) );
        }
    }
    else
    {
        ref = pop_free();
        if ( INVALID_ENC_REF == ref )
        {
            log_warning("%s:%d: failed to allocate a new object\n", __FILE__, __LINE__);
        }
    }

    if (isValidRef(ref))
    {
        // if the enchant is already being used, make sure to destroy the old one
        if (DEFINED_ENC(ref))
        {
            free_one(ref);
        }

        // Allocate the new object.
        get_ptr(ref)->allocate(REF_TO_INT(ref));
    }

    if (ALLOCATED_ENC(ref))
    {
        // construct the new structure
        get_ptr(ref)->config_construct(100);
    }

    return ref;
}

ENC_REF EnchantHandler::spawn_one_enchant(const CHR_REF owner, const CHR_REF target, const CHR_REF spawner, const ENC_REF enc_override, const PRO_REF modeloptional)
{
    /// @author ZZ
    /// @details This function enchants a target, returning the enchantment index or INVALID_ENC_REF
    ///    if failed

    ENC_REF enc_ref;
    EVE_REF eve_ref;

    eve_t * peve;
    enc_t * penc;
    Object * ptarget;

    PRO_REF loc_profile;
    CHR_REF loc_target;

    // Target must both be alive and on and valid
    loc_target = target;
    if (!_gameObjects.exists(loc_target))
    {
        log_warning("spawn_one_enchant() - failed because the target does not exist.\n");
        return INVALID_ENC_REF;
    }
    ptarget = _gameObjects.get(loc_target);

    // you should be able to enchant dead stuff to raise the dead...
    // if( !ptarget->alive ) return INVALID_ENC_REF;

    if (ProfileSystem::get().isValidProfileID(modeloptional))
    {
        // The enchantment type is given explicitly
        loc_profile = modeloptional;
    }
    else
    {
        // The enchantment type is given by the spawner
        loc_profile = _gameObjects[spawner]->profile_ref;

        if (!ProfileSystem::get().isValidProfileID(loc_profile))
        {
            log_warning("spawn_one_enchant() - no valid profile for the spawning character \"%s\"(%d).\n", _gameObjects.get(spawner)->Name, REF_TO_INT(spawner));
            return INVALID_ENC_REF;
        }
    }

    eve_ref = ProfileSystem::get().pro_get_ieve(loc_profile);
    if (!LOADED_EVE(eve_ref))
    {
        log_warning("spawn_one_enchant() - the object \"%s\"(%d) does not have an enchant profile.\n", ProfileSystem::get().getProfile(loc_profile)->getPathname().c_str(), REF_TO_INT(loc_profile));

        return INVALID_ENC_REF;
    }
    peve = EveStack.get_ptr(eve_ref);

    // count all the requests for this enchantment type
    peve->_spawnRequestCount++;

    // Owner must both be alive and on and valid if it isn't a stayifnoowner enchant
    if (!peve->_owner._stay && (!_gameObjects.exists(owner) || !_gameObjects.get(owner)->alive))
    {
        log_warning("spawn_one_enchant() - failed because the required enchant owner cannot be found.\n");
        return INVALID_ENC_REF;
    }

    // do retargeting, if necessary
    // Should it choose an inhand item?
    if (peve->retarget)
    {
        // Left, right, or both are valid
        if (_gameObjects.exists(ptarget->holdingwhich[SLOT_LEFT]))
        {
            // Only right hand is valid
            loc_target = ptarget->holdingwhich[SLOT_RIGHT];
        }
        else if (_gameObjects.exists(ptarget->holdingwhich[SLOT_LEFT]))
        {
            // Pick left hand
            loc_target = ptarget->holdingwhich[SLOT_LEFT];
        }
        else
        {
            // No weapons to pick, should it pick itself???
            loc_target = INVALID_CHR_REF;
        }
    }

    // make sure the loc_target is alive
    if (nullptr == (ptarget) || !ptarget->alive)
    {
        log_warning("spawn_one_enchant() - failed because the target is not alive.\n");
        return INVALID_ENC_REF;
    }
    ptarget = _gameObjects.get(loc_target);

    // Check peve->required_damagetype, 90% damage resistance is enough to resist the enchant
    if (peve->required_damagetype < DAMAGE_COUNT)
    {
        if (ptarget->damage_resistance[peve->required_damagetype] >= 0.90f ||
            HAS_SOME_BITS(ptarget->damage_modifier[peve->required_damagetype], DAMAGEINVICTUS))
        {
            log_debug("spawn_one_enchant() - failed because the target is immune to the enchant.\n");
            return INVALID_ENC_REF;
        }
    }

    // Check peve->require_damagetarget_damagetype
    if (peve->require_damagetarget_damagetype < DAMAGE_COUNT)
    {
        if (ptarget->damagetarget_damagetype != peve->require_damagetarget_damagetype)
        {
            log_warning("spawn_one_enchant() - failed because the target not have the right damagetarget_damagetype.\n");
            return INVALID_ENC_REF;
        }
    }

    // Find an enchant index to use
    enc_ref = EnchantHandler::get().allocate(enc_override);

    if (!ALLOCATED_ENC(enc_ref))
    {
        log_warning("spawn_one_enchant() - could not allocate an enchant.\n");
        return INVALID_ENC_REF;
    }
    penc = EnchantHandler::get().get_ptr(enc_ref);

    if (NULL != penc && penc->isAllocated())
    {
        if (!penc->spawning)
        {
            penc->spawning = true;
            Ego::Entities::spawnDepth++;
        }
    }

    penc->spawn_data.owner_ref = owner;
    penc->spawn_data.target_ref = loc_target;
    penc->spawn_data.spawner_ref = spawner;
    penc->spawn_data.profile_ref = loc_profile;
    penc->spawn_data.eve_ref = eve_ref;

    // actually force the character to spawn
    // log all the successful spawns
    if (penc->config_activate(100))
    {
		penc->POBJ_END_SPAWN();
        peve->_spawnCount++;
    }

    return enc_ref;
}
