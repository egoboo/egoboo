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

/// @file game/EncList.c
/// @brief Implementation of the EncList_* functions
/// @details

#include "game/EncList.h"
#include "game/egoboo_object.h"
#include "game/enchant.h"

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

// macros without range checking
#define INGAME_PENC_BASE_RAW(PENC)    (ACTIVE_PBASE( POBJ_GET_PBASE(PENC)) && ON_PBASE(POBJ_GET_PBASE(PENC)))
#define DEFINED_PENC_RAW(PENC)        (ALLOCATED_PBASE (POBJ_GET_PBASE(PENC)) && !TERMINATED_PBASE (POBJ_GET_PBASE(PENC)))
#define ALLOCATED_PENC_RAW(PENC)      ALLOCATED_PBASE(POBJ_GET_PBASE(PENC))
#define ACTIVE_PENC_RAW(PENC)         ACTIVE_PBASE(POBJ_GET_PBASE(PENC))
#define WAITING_PENC_RAW(PENC)        WAITING_PBASE(POBJ_GET_PBASE(PENC))
#define TERMINATED_PENC_RAW(PENC)     TERMINATED_PBASE(POBJ_GET_PBASE(PENC))

//--------------------------------------------------------------------------------------------
//inlined before

bool VALID_ENC_RANGE(const ENC_REF IENC) { return EncList.isValidRef(IENC); }

bool DEFINED_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && DEFINED_PENC_RAW(EncList.get_ptr(IENC))); }

bool ALLOCATED_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && ALLOCATED_PENC_RAW(EncList.get_ptr(IENC))); }

bool ACTIVE_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && ACTIVE_PENC_RAW(EncList.get_ptr(IENC))); }

bool WAITING_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && WAITING_PENC_RAW(EncList.get_ptr(IENC))); }

bool TERMINATED_ENC(const ENC_REF IENC)  { return (VALID_ENC_RANGE(IENC) && TERMINATED_PENC_RAW(EncList.get_ptr(IENC))); }

ENC_REF GET_REF_PENC(const enc_t *PENC) { return LAMBDA(NULL == (PENC), INVALID_ENC_REF, GET_INDEX_POBJ(PENC, INVALID_ENC_REF)); }

bool DEFINED_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && DEFINED_PENC_RAW(PENC)); }

bool VALID_ENC_PTR(const enc_t *PENC) { return ((NULL != (PENC)) && VALID_ENC_RANGE(GET_REF_POBJ(PENC, INVALID_ENC_REF))); }

bool ALLOCATED_PENC(const enc_t *PENC)  { return (VALID_ENC_PTR(PENC) && ALLOCATED_PENC_RAW(PENC)); }

bool _ACTIVE_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && ACTIVE_PENC_RAW(PENC)); }

bool WAITING_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && WAITING_PENC_RAW(PENC)); }

bool TERMINATED_PENC(const enc_t * PENC) { return (VALID_ENC_PTR(PENC) && TERMINATED_PENC_RAW(PENC)); }

bool INGAME_ENC_BASE(const ENC_REF IENC)  { return (VALID_ENC_RANGE(IENC) && INGAME_PENC_BASE_RAW(EncList.get_ptr(IENC))); }

bool INGAME_PENC_BASE(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && INGAME_PENC_BASE_RAW(PENC)); }

bool INGAME_ENC(const ENC_REF IENC) { return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_ENC(IENC), INGAME_ENC_BASE(IENC)); }

bool INGAME_PENC(const enc_t *PENC) { return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_PENC(PENC), INGAME_PENC_BASE(PENC)); }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

EnchantManager EncList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void EnchantManager::ctor()
{
    // Initialize the list.
    init();

    // Construct the sub-objects.
    for (size_t i = 0; i < getCount(); ++i)
    {
        enc_t *x = lst + i;

        // Blank out all the data, including the obj_base data.
        BLANK_STRUCT_PTR(x);

        // construct the base object
        Ego::Entity::ctor(POBJ_GET_PBASE(x), x, BSP_LEAF_ENC, i);

        // Construct the object.
        enc_t::ctor(x);
    }
}

//--------------------------------------------------------------------------------------------
void EnchantManager::dtor()
{
    // Construct the sub-objects.
    for (size_t i = 0; i < getCount(); ++i)
    {
        enc_t *x = lst + i;

        // Destruct the object
        enc_t::dtor(x);

        // Destruct the entity.
        Ego::Entity::dtor(POBJ_GET_PBASE(x));
    }

    // Initialize the list.
    EnchantManager::init();
}

//--------------------------------------------------------------------------------------------
void EnchantManager::deinit()
{
    // Request that the sub-objects destroy themselves.
    for (size_t i = 0; i < getCount(); ++i)
    {
        enc_config_deconstruct(get_ptr(i), 100);
    }

    // Initialize the list.
    init(); /// @todo Shouldn't this be "clear"?.
}
//--------------------------------------------------------------------------------------------
void EnchantManager::prune_used_list()
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
void EnchantManager::prune_free_list()
{
    // prune the free list
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
void EnchantManager::update_used()
{
    prune_used_list();
    prune_free_list();

    // Go through the object list to see if there are any dangling objects.
    for (ENC_REF ref = 0; ref < getCount(); ++ref)
    {
        if (!ALLOCATED_ENC(ref)) continue;

        if (INGAME_ENC(ref))
        {
            if (!lst[ref].obj_base.in_used_list )
            {
                push_used(ref);
            }
        }
        else if (!DEFINED_ENC(ref))
        {
            if (!lst[ref].obj_base.in_free_list)
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
    for (size_t i = freeCount; i < getCount(); ++i)
    {
        free_ref[i] = INVALID_ENC_REF;
    }
}

//--------------------------------------------------------------------------------------------
bool EnchantManager::free_one(const ENC_REF ienc)
{
    /// @author ZZ
    /// @details This function sticks a enchant back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_ENC() and POBJ_TERMINATE() to EncList_free_one()
    /// should be enough to ensure that no enchant is freed more than once

    bool retval;

    if ( !ALLOCATED_ENC( ienc ) ) return false;
    enc_t *penc = EncList.get_ptr( ienc );
    Ego::Entity *pbase = POBJ_GET_PBASE( penc );

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && defined(_DEBUG)
    enc_log_script_time( ienc );
#endif

    // if we are inside a EncList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( EncList.getLockCount() > 0 )
    {
        retval = EncList.add_termination( ienc );
    }
    else
    {
        // deallocate any dynamically allocated memory
        penc = enc_config_deinitialize( penc, 100 );
        if ( NULL == penc ) return false;

        if ( pbase->in_used_list )
        {
            remove_used_ref( ienc );
        }

        if ( pbase->in_free_list )
        {
            retval = true;
        }
        else
        {
            retval = EncList.add_free_ref( ienc );
        }

        // enchant "destructor"
        penc = enc_t::dtor( penc );
        if ( NULL == penc ) return false;
    }

    return retval;
}
//--------------------------------------------------------------------------------------------
bool EnchantManager::push_used(const ENC_REF ref)
{
    if (!isValidRef(ref))
    {
        return false;
    }
#if defined(_DEBUG) && defined(DEBUG_ENC_LIST)
    if (find_used_ref(ref) != std::numeric_limits<size_t>::max())
    {
        return false;
    }
#endif

    EGOBOO_ASSERT(!lst[ref].obj_base.in_used_list);

    if (usedCount < getCount())
    {
        used_ref[usedCount] = ref;

        usedCount++;
        update_guid++;

        lst[ref].obj_base.in_used_list = true;
        return true;
    }
    return false;
}
//--------------------------------------------------------------------------------------------
ENC_REF EnchantManager::allocate(const ENC_REF override)
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
            log_warning( "EncList_allocate() - failed to override a enchant? enchant %d already spawned? \n", REF_TO_INT( override ) );
        }
    }
    else
    {
        ref = ( ENC_REF )pop_free();
        if ( INVALID_ENC_REF == ref )
        {
            log_warning( "EncList_allocate() - failed to allocate a new enchant\n" );
        }
    }

    if (isValidRef(ref))
    {
        // if the enchant is already being used, make sure to destroy the old one
        if ( DEFINED_ENC( ref ) )
        {
            free_one(ref);
        }

        // allocate the new one
        POBJ_ALLOCATE(get_ptr( ref ), REF_TO_INT( ref ) );
    }

    if ( ALLOCATED_ENC( ref ) )
    {
        // construct the new structure
        enc_config_construct( EncList.get_ptr( ref ), 100 );
    }

    return ref;
}

//--------------------------------------------------------------------------------------------
bool EnchantManager::isValidRef(const ENC_REF ref) const
{
    return ref < getCount();
}

//--------------------------------------------------------------------------------------------
void EnchantManager::maybeRunDeferred()
{
    // Go through the list and activate all the enchants that
    // were created while the list was iterating.
    for (size_t cnt = 0; cnt < activation_count; cnt++ )
    {
        ENC_REF ienc = activation_list[cnt];

        if (!ALLOCATED_ENC(ienc)) continue;
        enc_t *penc = get_ptr(ienc);

        if (!penc->obj_base.turn_me_on) continue;

        penc->obj_base.on         = true;
        penc->obj_base.turn_me_on = false;
    }
    activation_count = 0;

    // Go through and delete any enchants that were
    // supposed to be deleted while the list was iterating
    for (size_t cnt = 0; cnt < termination_count; cnt++)
    {
        free_one(termination_list[cnt]);
    }
    termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool EnchantManager::add_activation(const ENC_REF ref)
{
    // put this enchant into the activation list so that it can be activated right after
    // the EncList loop is completed

    bool retval = false;

    if (!isValidRef(ref)) return false;

    if (activation_count < getCount())
    {
        activation_list[activation_count] = ref;
        activation_count++;

        retval = true;
    }

    lst[ref].obj_base.turn_me_on = true;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EnchantManager::add_termination( const ENC_REF ref )
{
    bool retval = false;

    if (!isValidRef(ref)) return false;

    if (termination_count < getCount())
    {
        termination_list[termination_count] = ref;
        termination_count++;

        retval = true;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE(get_ptr(ref));

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EnchantManager::request_terminate(const ENC_REF ref)
{
    enc_t *enc = get_ptr(ref);
    EGOBOO_ASSERT(nullptr != enc);
    return enc_request_terminate(enc);
}
