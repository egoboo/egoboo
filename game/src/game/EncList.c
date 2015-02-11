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

bool VALID_ENC_RANGE(const ENC_REF IENC) { return (((ENC_REF)(IENC)) < MAX_ENC); }

bool DEFINED_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && DEFINED_PENC_RAW(EncList.lst + (IENC))); }

bool ALLOCATED_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && ALLOCATED_PENC_RAW(EncList.lst + (IENC))); }

bool ACTIVE_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && ACTIVE_PENC_RAW(EncList.lst + (IENC))); }

bool WAITING_ENC(const ENC_REF IENC) { return (VALID_ENC_RANGE(IENC) && WAITING_PENC_RAW(EncList.lst + (IENC))); }

bool TERMINATED_ENC(const ENC_REF IENC)  { return (VALID_ENC_RANGE(IENC) && TERMINATED_PENC_RAW(EncList.lst + (IENC))); }

size_t GET_INDEX_PENC(const enc_t *PENC) { return LAMBDA(NULL == (PENC), INVALID_ENC_IDX, (size_t)GET_INDEX_POBJ(PENC, INVALID_ENC_IDX)); }

ENC_REF GET_REF_PENC(const enc_t *PENC) { return ((ENC_REF)GET_INDEX_PENC(PENC)); }

bool DEFINED_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && DEFINED_PENC_RAW(PENC)); }

bool VALID_ENC_PTR(const enc_t *PENC) { return ((NULL != (PENC)) && VALID_ENC_RANGE(GET_REF_POBJ(PENC, INVALID_ENC_REF))); }

bool ALLOCATED_PENC(const enc_t *PENC)  { return (VALID_ENC_PTR(PENC) && ALLOCATED_PENC_RAW(PENC)); }

bool _ACTIVE_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && ACTIVE_PENC_RAW(PENC)); }

bool WAITING_PENC(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && WAITING_PENC_RAW(PENC)); }

bool TERMINATED_PENC(const enc_t * PENC) { return (VALID_ENC_PTR(PENC) && TERMINATED_PENC_RAW(PENC)); }

bool INGAME_ENC_BASE(const ENC_REF IENC)  { return (VALID_ENC_RANGE(IENC) && INGAME_PENC_BASE_RAW(EncList.lst + (IENC))); }

bool INGAME_PENC_BASE(const enc_t *PENC) { return (VALID_ENC_PTR(PENC) && INGAME_PENC_BASE_RAW(PENC)); }

bool INGAME_ENC(const ENC_REF IENC) { return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_ENC(IENC), INGAME_ENC_BASE(IENC)); }

bool INGAME_PENC(const enc_t *PENC) { return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_PENC(PENC), INGAME_PENC_BASE(PENC)); }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LOCKABLELIST(, enc_t, EncList, MAX_ENC );

static size_t  enc_termination_count = 0;
static ENC_REF enc_termination_list[MAX_ENC];

static size_t  enc_activation_count = 0;
static ENC_REF enc_activation_list[MAX_ENC];

//--------------------------------------------------------------------------------------------
// private EncList_t functions
//--------------------------------------------------------------------------------------------

static void    EncList_clear();
static void    EncList_init();
static void    EncList_deinit();

static bool  EncList_add_free_ref( const ENC_REF ienc );
static bool  EncList_remove_free_ref( const ENC_REF ienc );
static bool  EncList_remove_free_idx( const int index );

static bool EncList_remove_used_ref( const ENC_REF ienc );
static bool EncList_remove_used_idx( const int index );

static void   EncList_prune_used_list();
static void   EncList_prune_free_list();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( enc_t, EncList, MAX_ENC );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void EncList_ctor()
{
    ENC_REF cnt;
    enc_t * penc;

    // initialize the list
    EncList_init();

    // construct the sub-objects
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        penc = EncList.lst + cnt;

        // blank out all the data, including the obj_base data
        BLANK_STRUCT_PTR( penc )

        // construct the base object
        Ego::Entity::ctor( POBJ_GET_PBASE( penc ), penc, BSP_LEAF_ENC, cnt );

        // construct the object
        enc_t::ctor( penc );
    }
}

//--------------------------------------------------------------------------------------------
void EncList_dtor()
{
    ENC_REF cnt;
    enc_t * penc;

    // construct the sub-objects
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        penc = EncList.lst + cnt;

        // destruct the object
        enc_t::dtor( penc );

        // destruct the parent
        Ego::Entity::dtor( POBJ_GET_PBASE( penc ) );
    }

    // initialize particle
    EncList_init();
}

//--------------------------------------------------------------------------------------------
void EncList_deinit()
{
    ENC_REF cnt;

    // request that the sub-objects destroy themselves
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        enc_config_deconstruct( EncList.lst + cnt, 100 );
    }

    // reset the list
    EncList_init();
}

//--------------------------------------------------------------------------------------------
void EncList_reinit()
{
    EncList_deinit();
    EncList_init();
}

//--------------------------------------------------------------------------------------------
void EncList_clear()
{
    ENC_REF cnt;

    // clear out the list
    EncList.freeCount = 0;
    EncList.usedCount = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        // blank out the list
        EncList.free_ref[cnt] = INVALID_ENC_IDX;
        EncList.used_ref[cnt] = INVALID_ENC_IDX;

        // let the particle data know that it is not in a list
        EncList.lst[cnt].obj_base.in_free_list = false;
        EncList.lst[cnt].obj_base.in_used_list = false;
    }
}

//--------------------------------------------------------------------------------------------
void EncList_init()
{
    int cnt;

    EncList_clear();

    // place all the characters on the free list
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        ENC_REF ichr = ( MAX_ENC - 1 ) - cnt;

        EncList_add_free_ref( ichr );
    }
}

//--------------------------------------------------------------------------------------------
void EncList_prune_used_list()
{
    // prune the used list

    size_t cnt;
    ENC_REF ienc;

    for ( cnt = 0; cnt < EncList.usedCount; cnt++ )
    {
        bool removed = false;

        ienc = ( ENC_REF )EncList.used_ref[cnt];

        if ( !VALID_ENC_RANGE( ienc ) || !DEFINED_ENC( ienc ) )
        {
            removed = EncList_remove_used_idx( cnt );
        }

        if ( removed && !EncList.lst[ienc].obj_base.in_free_list )
        {
            EncList_add_free_ref( ienc );
        }
    }
}

//--------------------------------------------------------------------------------------------
void EncList_prune_free_list()
{
    // prune the free list
    ENC_REF ienc;

    for ( size_t cnt = 0; cnt < EncList.freeCount; cnt++ )
    {
        bool removed = false;

        ienc = ( ENC_REF )EncList.free_ref[cnt];

        if ( VALID_ENC_RANGE( ienc ) && INGAME_ENC_BASE( ienc ) )
        {
            removed = EncList_remove_free_idx( cnt );
        }

        if ( removed && !EncList.lst[ienc].obj_base.in_free_list )
        {
            EncList_push_used( ienc );
        }
    }
}

//--------------------------------------------------------------------------------------------
void EncList_update_used()
{
    size_t cnt;
    ENC_REF ienc;

    EncList_prune_used_list();

    EncList_prune_free_list();

    // go through the enchant list to see if there are any dangling enchants
    for ( ienc = 0; ienc < MAX_ENC; ienc++ )
    {
        if ( !ALLOCATED_ENC( ienc ) ) continue;

        if ( INGAME_ENC( ienc ) )
        {
            if ( !EncList.lst[ienc].obj_base.in_used_list )
            {
                EncList_push_used( ienc );
            }
        }
        else if ( !DEFINED_ENC( ienc ) )
        {
            if ( !EncList.lst[ienc].obj_base.in_free_list )
            {
                EncList_add_free_ref( ienc );
            }
        }
    }

    // blank out the unused elements of the used list
    for ( cnt = EncList.getUsedCount(); cnt < MAX_ENC; cnt++ )
    {
        EncList.used_ref[cnt] = INVALID_ENC_IDX;
    }

    // blank out the unused elements of the free list
    for ( cnt = EncList.freeCount; cnt < MAX_ENC; cnt++ )
    {
        EncList.free_ref[cnt] = INVALID_ENC_IDX;
    }
}

//--------------------------------------------------------------------------------------------
bool EncList_free_one( const ENC_REF ienc )
{
    /// @author ZZ
    /// @details This function sticks a enchant back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_ENC() and POBJ_TERMINATE() to EncList_free_one()
    /// should be enough to ensure that no enchant is freed more than once

    bool retval;

    if ( !ALLOCATED_ENC( ienc ) ) return false;
    enc_t *penc = EncList_get_ptr( ienc );
    Ego::Entity *pbase = POBJ_GET_PBASE( penc );

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && defined(_DEBUG)
    enc_log_script_time( ienc );
#endif

    // if we are inside a EncList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( EncList.getLockCount() > 0 )
    {
        retval = EncList_add_termination( ienc );
    }
    else
    {
        // deallocate any dynamically allocated memory
        penc = enc_config_deinitialize( penc, 100 );
        if ( NULL == penc ) return false;

        if ( pbase->in_used_list )
        {
            EncList_remove_used_ref( ienc );
        }

        if ( pbase->in_free_list )
        {
            retval = true;
        }
        else
        {
            retval = EncList_add_free_ref( ienc );
        }

        // enchant "destructor"
        penc = enc_t::dtor( penc );
        if ( NULL == penc ) return false;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t EncList_pop_free( const int idx )
{
    /// @author ZZ
    /// @details This function returns the next free enchant or MAX_ENC if there are none

    size_t retval = MAX_ENC;
    size_t loops  = 0;

    if ( idx >= 0 && idx < EncList.freeCount )
    {
        // the user has specified a valid index in the free stack
        // that they want to use. make that happen.

        // from the conditions, EncList.free_count must be greater than 1
        size_t itop = EncList.freeCount - 1;

        // move the desired index to the top of the stack
        SWAP( size_t, EncList.free_ref[idx], EncList.free_ref[itop] );
    }

    while ( EncList.freeCount > 0 )
    {
        EncList.freeCount--;
        EncList.update_guid++;

        retval = EncList.free_ref[EncList.freeCount];

        // completely remove it from the free list
        EncList.free_ref[EncList.freeCount] = INVALID_ENC_IDX;

        if ( VALID_ENC_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            EncList.lst[retval].obj_base.in_free_list = false;
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the free stack. %d loops.\n", __FUNCTION__, loops );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void EncList_free_all()
{
    ENC_REF cnt;

    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        EncList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
int EncList_find_free_ref( const ENC_REF ienc )
{
    int retval = -1, cnt;

    if ( !VALID_ENC_RANGE( ienc ) ) return retval;

    for ( cnt = 0; cnt < EncList.freeCount; cnt++ )
    {
        if ( ienc == EncList.free_ref[cnt] )
        {
            EGOBOO_ASSERT( EncList.lst[ienc].obj_base.in_free_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EncList_add_free_ref( const ENC_REF ienc )
{
    bool retval;

    if ( !VALID_ENC_RANGE( ienc ) ) return false;

#if defined(_DEBUG) && defined(DEBUG_ENC_LIST)
    if ( EncList_find_free_ref( ienc ) > 0 )
    {
        return false;
    }
#endif

    EGOBOO_ASSERT( !EncList.lst[ienc].obj_base.in_free_list );

    retval = false;
    if ( EncList.freeCount < MAX_ENC )
    {
        EncList.free_ref[EncList.freeCount] = ienc;

        EncList.freeCount++;
        EncList.update_guid++;

        EncList.lst[ienc].obj_base.in_free_list = true;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EncList_remove_free_idx( const int index )
{
    ENC_REF ienc;

    // was it found?
    if ( index < 0 || index >= EncList.freeCount ) return false;

    ienc = ( ENC_REF )EncList.free_ref[index];

    // blank out the index in the list
    EncList.free_ref[index] = INVALID_ENC_IDX;

    if ( VALID_ENC_RANGE( ienc ) )
    {
        // let the object know it is not in the list anymore
        EncList.lst[ienc].obj_base.in_free_list = false;
    }

    // shorten the list
    EncList.freeCount--;
    EncList.update_guid++;

    if ( EncList.freeCount > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, EncList.free_ref[index], EncList.free_ref[EncList.freeCount] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
int EncList_find_used_ref( const ENC_REF ienc )
{
    /// @author BB
    /// @details if an object of index iobj exists on the used list, return the used list index
    ///     otherwise return -1

    int retval = -1, cnt;

    if ( !VALID_ENC_RANGE( ienc ) ) return retval;

    for ( cnt = 0; cnt < EncList.usedCount; cnt++ )
    {
        if ( ienc == EncList.used_ref[cnt] )
        {
            EGOBOO_ASSERT( EncList.lst[ienc].obj_base.in_used_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EncList_push_used( const ENC_REF ienc )
{
    bool retval;

    if ( !VALID_ENC_RANGE( ienc ) ) return false;

#if defined(_DEBUG) && defined(DEBUG_ENC_LIST)
    if ( EncList_find_used_ref( ienc ) > 0 )
    {
        return false;
    }
#endif

    EGOBOO_ASSERT( !EncList.lst[ienc].obj_base.in_used_list );

    retval = false;
    if ( EncList.usedCount < MAX_ENC )
    {
        EncList.used_ref[EncList.usedCount] = REF_TO_INT( ienc );

        EncList.usedCount++;
        EncList.update_guid++;

        EncList.lst[ienc].obj_base.in_used_list = true;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EncList_remove_used_idx( const int index )
{
    ENC_REF ienc;

    // was it found?
    if ( index < 0 || index >= EncList.usedCount ) return false;

    ienc = ( ENC_REF )EncList.used_ref[index];

    // blank out the index in the list
    EncList.used_ref[index] = INVALID_ENC_IDX;

    if ( VALID_ENC_RANGE( ienc ) )
    {
        // let the object know it is not in the list anymore
        EncList.lst[ienc].obj_base.in_used_list = false;
    }

    // shorten the list
    EncList.usedCount--;
    EncList.update_guid++;

    if ( EncList.usedCount > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, EncList.used_ref[index], EncList.used_ref[EncList.usedCount] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool EncList_remove_used_ref( const ENC_REF ienc )
{
    // find the object in the used list
    int index = EncList_find_used_ref( ienc );

    return EncList_remove_used_idx( index );
}

//--------------------------------------------------------------------------------------------
ENC_REF EncList_allocate( const ENC_REF override )
{
    ENC_REF ienc = INVALID_ENC_REF;

    if ( VALID_ENC_RANGE( override ) )
    {
        ienc = ( ENC_REF )EncList_pop_free( -1 );
        if ( override != ienc )
        {
            int override_index = EncList_find_free_ref( override );

            if ( override_index < 0 || override_index >= EncList.freeCount )
            {
                ienc = INVALID_ENC_REF;
            }
            else
            {
                // store the "wrong" value in the override enchant's index
                EncList.free_ref[override_index] = REF_TO_INT( ienc );

                // fix the in_free_list values
                EncList.lst[ienc].obj_base.in_free_list = true;
                EncList.lst[override].obj_base.in_free_list = false;

                ienc = override;
            }
        }

        if ( INVALID_ENC_REF == ienc )
        {
            log_warning( "EncList_allocate() - failed to override a enchant? enchant %d already spawned? \n", REF_TO_INT( override ) );
        }
    }
    else
    {
        ienc = ( ENC_REF )EncList_pop_free( -1 );
        if ( INVALID_ENC_REF == ienc )
        {
            log_warning( "EncList_allocate() - failed to allocate a new enchant\n" );
        }
    }

    if ( VALID_ENC_RANGE( ienc ) )
    {
        // if the enchant is already being used, make sure to destroy the old one
        if ( DEFINED_ENC( ienc ) )
        {
            EncList_free_one( ienc );
        }

        // allocate the new one
        POBJ_ALLOCATE( EncList_get_ptr( ienc ), REF_TO_INT( ienc ) );
    }

    if ( ALLOCATED_ENC( ienc ) )
    {
        // construct the new structure
        enc_config_construct( EncList_get_ptr( ienc ), 100 );
    }

    return ienc;
}

//--------------------------------------------------------------------------------------------
void EncList_cleanup()
{
    enc_t * penc;

    // go through the list and activate all the enchants that
    // were created while the list was iterating
    for ( size_t cnt = 0; cnt < enc_activation_count; cnt++ )
    {
        ENC_REF ienc = enc_activation_list[cnt];

        if ( !ALLOCATED_ENC( ienc ) ) continue;
        penc = EncList_get_ptr( ienc );

        if ( !penc->obj_base.turn_me_on ) continue;

        penc->obj_base.on         = true;
        penc->obj_base.turn_me_on = false;
    }
    enc_activation_count = 0;

    // go through and delete any enchants that were
    // supposed to be deleted while the list was iterating
    for ( size_t cnt = 0; cnt < enc_termination_count; cnt++ )
    {
        EncList_free_one( enc_termination_list[cnt] );
    }
    enc_termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool EncList_add_activation( const ENC_REF ienc )
{
    // put this enchant into the activation list so that it can be activated right after
    // the EncList loop is completed

    bool retval = false;

    if ( !VALID_ENC_RANGE( ienc ) ) return false;

    if ( enc_activation_count < MAX_ENC )
    {
        enc_activation_list[enc_activation_count] = ienc;
        enc_activation_count++;

        retval = true;
    }

    EncList.lst[ienc].obj_base.turn_me_on = true;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EncList_add_termination( const ENC_REF ienc )
{
    bool retval = false;

    if ( !VALID_ENC_RANGE( ienc ) ) return false;

    if ( enc_termination_count < MAX_ENC )
    {
        enc_termination_list[enc_termination_count] = ienc;
        enc_termination_count++;

        retval = true;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE( EncList_get_ptr( ienc ) );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool EncList_request_terminate( const ENC_REF ienc )
{
    enc_t * penc = EncList_get_ptr( ienc );

    return enc_request_terminate( penc );
}

//--------------------------------------------------------------------------------------------
bool EncList_remove_free_ref( const ENC_REF ienc )
{
    // find the object in the free list
    int index = EncList_find_free_ref( ienc );

    return EncList_remove_free_idx( index );
}
