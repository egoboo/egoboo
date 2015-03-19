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
#include "game/Entities/Enchant.hpp"

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
    return LAMBDA(nullptr == ptr, INVALID_ENC_REF, GET_INDEX_POBJ(ptr, INVALID_ENC_REF));
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
        if (!ALLOCATED_BASE_RAW(x)) continue;

        if (INGAME_PENC(x))
        {
            if (!x->obj_base.in_used_list )
            {
                push_used(ref);
            }
        }
        else if (!DEFINED_BASE_RAW(x)) // We can use DEFINED_BASE_RAW as the reference is valid.
        {
            if (!x->obj_base.in_free_list)
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

//--------------------------------------------------------------------------------------------
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
                lst[ref].obj_base.in_free_list = true;
                lst[override].obj_base.in_free_list = false;

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
        get_ptr(ref)->obj_base.allocate(REF_TO_INT(ref));
    }

    if (ALLOCATED_ENC(ref))
    {
        // construct the new structure
        enc_t::config_construct(get_ptr(ref), 100 );
    }

    return ref;
}
