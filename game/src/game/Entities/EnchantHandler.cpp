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
// testing macros
//--------------------------------------------------------------------------------------------

// macros without range checking
bool INGAME_PENC_BASE_RAW(const enc_t *ptr)
{
    return ACTIVE_PBASE(POBJ_GET_PBASE(ptr))
        && ON_PBASE(POBJ_GET_PBASE(ptr));
}

bool DEFINED_PENC_BASE_RAW(const enc_t *ptr)
{
    return ALLOCATED_PBASE(POBJ_GET_PBASE(ptr))
        && !TERMINATED_PBASE(POBJ_GET_PBASE(ptr));
}

bool ALLOCATED_PENC_BASE_RAW(const enc_t *ptr)
{
    return ALLOCATED_PBASE(POBJ_GET_PBASE(ptr));
}

bool ACTIVE_PENC_BASE_RAW(const enc_t *ptr)
{
    return ACTIVE_PBASE(POBJ_GET_PBASE(ptr));
}

bool WAITING_PENC_BASE_RAW(const enc_t *ptr)
{
    return WAITING_PBASE(POBJ_GET_PBASE(ptr));
}

bool TERMINATED_PENC_BASE_RAW(const enc_t *ptr)
{
    return TERMINATED_PBASE(POBJ_GET_PBASE(ptr));
}

//--------------------------------------------------------------------------------------------
//inlined before

bool VALID_ENC_RANGE(const ENC_REF IENC) { return EnchantHandler::get().isValidRef(IENC); }

bool DEFINED_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && DEFINED_PENC_BASE_RAW(EnchantHandler::get().get_ptr(IENC))); }

bool ALLOCATED_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && ALLOCATED_PENC_BASE_RAW(EnchantHandler::get().get_ptr(IENC))); }

bool ACTIVE_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && ACTIVE_PENC_BASE_RAW(EnchantHandler::get().get_ptr(IENC))); }

bool WAITING_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && WAITING_PENC_BASE_RAW(EnchantHandler::get().get_ptr(IENC))); }

bool TERMINATED_ENC(const ENC_REF IENC)  { return (VALID_ENC_RANGE(IENC) && TERMINATED_PENC_BASE_RAW(EnchantHandler::get().get_ptr(IENC))); }

ENC_REF GET_REF_PENC(const enc_t *PENC) { return LAMBDA(NULL == (PENC), INVALID_ENC_REF, GET_INDEX_POBJ(PENC, INVALID_ENC_REF)); }

bool DEFINED_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && DEFINED_PENC_BASE_RAW(PENC)); }

bool VALID_ENC_PTR(const enc_t *PENC) { return ((NULL != (PENC)) && VALID_ENC_RANGE(GET_REF_POBJ(PENC, INVALID_ENC_REF))); }

bool ALLOCATED_PENC(const enc_t *PENC)  { return (VALID_ENC_PTR(PENC) && ALLOCATED_PENC_BASE_RAW(PENC)); }

bool _ACTIVE_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && ACTIVE_PENC_BASE_RAW(PENC)); }

bool WAITING_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && WAITING_PENC_BASE_RAW(PENC)); }

bool TERMINATED_PENC(const enc_t * PENC) { return (VALID_ENC_PTR(PENC) && TERMINATED_PENC_BASE_RAW(PENC)); }

bool INGAME_ENC_BASE(const ENC_REF IENC)  { return (VALID_ENC_RANGE(IENC) && INGAME_PENC_BASE_RAW(EnchantHandler::get().get_ptr(IENC))); }

bool INGAME_PENC_BASE(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && INGAME_PENC_BASE_RAW(PENC)); }

bool INGAME_ENC(const ENC_REF IENC) { return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_ENC(IENC), INGAME_ENC_BASE(IENC)); }

bool INGAME_PENC(const enc_t *PENC) { return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_PENC(PENC), INGAME_PENC_BASE(PENC)); }

//--------------------------------------------------------------------------------------------

EnchantHandler EncList;

EnchantHandler& EnchantHandler::get()
{
    return EncList;
}

//--------------------------------------------------------------------------------------------
void EnchantHandler::prune_used_list()
{
    // prune the used list
    for (size_t i = 0; i < usedCount; ++i)
    {
        bool removed = false;

        ENC_REF ref = used_ref[i];

        if (!isValidRef(ref) || !DEFINED_ENC(ref))
        {
            removed = remove_used_idx(i);
        }

        if (removed && !lst[ref].obj_base.in_free_list)
        {
            add_free_ref(ref);
        }
    }
}

//--------------------------------------------------------------------------------------------
void EnchantHandler::prune_free_list()
{
    // Prune the free list.
    for (size_t i = 0; i < freeCount; ++i)
    {
        bool removed = false;

        ENC_REF ref = free_ref[i];

        if (isValidRef(ref) && INGAME_ENC_BASE(ref))
        {
            removed = remove_free_idx(i);
        }

        if (removed && !lst[ref].obj_base.in_free_list)
        {
            push_used(ref);
        }
    }
}

//--------------------------------------------------------------------------------------------
void EnchantHandler::update_used()
{
    prune_used_list();
    prune_free_list();

    // Go through the object list to see if there are any dangling objects.
    for (ENC_REF ref = 0; ref < getCount(); ++ref)
    {
        if (!isValidRef(ref)) continue; /// @todo Redundant.
        enc_t *x = get_ptr(ref);
        if (!ALLOCATED_PENC_BASE_RAW(x)) continue;

        if (INGAME_PENC(x))
        {
            if (!x->obj_base.in_used_list )
            {
                push_used(ref);
            }
        }
        else if (!DEFINED_PENC(x))
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
bool EnchantHandler::free_one(const ENC_REF ref)
{
    /// @details Stick an object back into the free object list.

    // Ensure that we have a valid reference.
    if (!isValidRef(ref)) return false;
    enc_t *obj = get_ptr(ref);

    // If the object is not allocated (i.e. in the state range ["constructing","destructing"])
    // then its reference is in the free list.
    if (!ALLOCATED_PENC(obj)) return false;

    Ego::Entity *parentObj = POBJ_GET_PBASE(obj);

    // If we are inside an iteration, do not actually change the length of the list.
    // This would invalidate all iterators.
    if (getLockCount() > 0)
    {
        return add_termination(ref);
    }
    else
    {
        // Ensure that the entity reaches the "destructing" state.
        // @todo This is redundant.
        obj = enc_t::config_deinitialize(obj, 100);
        if (!obj) return false;
        // Ensure that the entity reaches the "terminated" state.
        obj = enc_t::config_deconstruct(obj, 100);
        if (!obj) return false;

        if (parentObj->in_used_list)
        {
            remove_used_ref(ref);
        }
        if (parentObj->in_free_list)
        {
            return true;
        }
        else
        {
            return add_free_ref(ref);
        }
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

//--------------------------------------------------------------------------------------------
void EnchantHandler::maybeRunDeferred()
{
    // Go through the list and activate all the enchants that
    // were created while the list was iterating.
    for (size_t i = 0; i < activation_count; ++i)
    {
        ENC_REF ref = activation_list[i];

        if (!ALLOCATED_ENC(ref)) continue;
        enc_t *x = get_ptr(ref);

        if (!x->obj_base.turn_me_on) continue;

        x->obj_base.on         = true;
        x->obj_base.turn_me_on = false;
    }
    activation_count = 0;

    // Go through and delete any enchants that were
    // supposed to be deleted while the list was iterating
    for (size_t i = 0; i < termination_count; ++i)
    {
        free_one(termination_list[i]);
    }
    termination_count = 0;
}
