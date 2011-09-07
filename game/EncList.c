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

/// @file EncList.c
/// @brief Implementation of the EncList_* functions
/// @details

#include "EncList.h"
#include "egoboo_object.h"

#include "../egolib/log.h"

#include "enchant.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, enc_t, EncList, MAX_ENC );

static size_t  enc_termination_count = 0;
static ENC_REF enc_termination_list[MAX_ENC];

static size_t  enc_activation_count = 0;
static ENC_REF enc_activation_list[MAX_ENC];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int enc_loop_depth = 0;

//--------------------------------------------------------------------------------------------
// private EncList_t functions
//--------------------------------------------------------------------------------------------

static void    EncList_clear( void );
static void    EncList_init( void );
static void    EncList_deinit( void );

static bool_t  EncList_add_free_ref( const ENC_REF ienc );
static bool_t  EncList_remove_free_ref( const ENC_REF ienc );
static bool_t  EncList_remove_free_idx( const int index );

static bool_t EncList_remove_used_ref( const ENC_REF ienc );
static bool_t EncList_remove_used_idx( const int index );

static void   EncList_prune_used_list( void );
static void   EncList_prune_free_list( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( enc_t, EncList, MAX_ENC );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void EncList_ctor( void )
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
        ego_object_ctor( POBJ_GET_PBASE( penc ), penc, BSP_LEAF_ENC, cnt );

        // construct the object
        enc_ctor( penc );
    }
}

//--------------------------------------------------------------------------------------------
void EncList_dtor( void )
{
    ENC_REF cnt;
    enc_t * penc;

    // construct the sub-objects
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        penc = EncList.lst + cnt;

        // destruct the object
        enc_dtor( penc );

        // destruct the parent
        ego_object_dtor( POBJ_GET_PBASE( penc ) );
    }

    // initialize particle
    EncList_init();
}

//--------------------------------------------------------------------------------------------
void EncList_deinit( void )
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
void EncList_reinit( void )
{
    EncList_deinit();
    EncList_init();
}

//--------------------------------------------------------------------------------------------
void EncList_clear( void )
{
    ENC_REF cnt;

    // clear out the list
    EncList.free_count = 0;
    EncList.used_count = 0;
    for ( cnt = 0; cnt < MAX_ENC; cnt++ )
    {
        // blank out the list
        EncList.free_ref[cnt] = INVALID_ENC_IDX;
        EncList.used_ref[cnt] = INVALID_ENC_IDX;

        // let the particle data know that it is not in a list
        EncList.lst[cnt].obj_base.in_free_list = bfalse;
        EncList.lst[cnt].obj_base.in_used_list = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
void EncList_init( void )
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
void EncList_prune_used_list( void )
{
    // prune the used list

    size_t cnt;
    ENC_REF ienc;

    for ( cnt = 0; cnt < EncList.used_count; cnt++ )
    {
        bool_t removed = bfalse;

        ienc = EncList.used_ref[cnt];

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
void EncList_prune_free_list( void )
{
    // prune the free list

    size_t cnt;
    ENC_REF ienc;

    for ( cnt = 0; cnt < EncList.free_count; cnt++ )
    {
        bool_t removed = bfalse;

        ienc = EncList.free_ref[cnt];

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
void EncList_update_used( void )
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
    for ( cnt = EncList.used_count; cnt < MAX_ENC; cnt++ )
    {
        EncList.used_ref[cnt] = INVALID_ENC_IDX;
    }

    // blank out the unused elements of the free list
    for ( cnt = EncList.free_count; cnt < MAX_ENC; cnt++ )
    {
        EncList.free_ref[cnt] = INVALID_ENC_IDX;
    }
}

//--------------------------------------------------------------------------------------------
bool_t EncList_free_one( const ENC_REF ienc )
{
    /// @author ZZ
    /// @details This function sticks a enchant back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_ENC() and POBJ_TERMINATE() to EncList_free_one()
    /// should be enough to ensure that no enchant is freed more than once

    bool_t retval;
    enc_t * penc;
    obj_data_t * pbase;

    if ( !ALLOCATED_ENC( ienc ) ) return bfalse;
    penc  = EncList_get_ptr( ienc );
    pbase = POBJ_GET_PBASE( penc );

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && defined(_DEBUG)
    enc_log_script_time( ienc );
#endif

    // if we are inside a EncList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( enc_loop_depth > 0 )
    {
        retval = EncList_add_termination( ienc );
    }
    else
    {
        // deallocate any dynamically allocated memory
        penc = enc_config_deinitialize( penc, 100 );
        if ( NULL == penc ) return bfalse;

        if ( pbase->in_used_list )
        {
            EncList_remove_used_ref( ienc );
        }

        if ( pbase->in_free_list )
        {
            retval = btrue;
        }
        else
        {
            retval = EncList_add_free_ref( ienc );
        }

        // enchant "destructor"
        penc = enc_dtor( penc );
        if ( NULL == penc ) return bfalse;
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

    if ( idx >= 0 && idx < EncList.free_count )
    {
        // the user has specified a valid index in the free stack
        // that they want to use. make that happen.

        // from the conditions, EncList.free_count must be greater than 1
        size_t itop = EncList.free_count - 1;

        // move the desired index to the top of the stack
        SWAP( size_t, EncList.free_ref[idx], EncList.free_ref[itop] );
    }

    while ( EncList.free_count > 0 )
    {
        EncList.free_count--;
        EncList.update_guid++;

        retval = EncList.free_ref[EncList.free_count];

        // completely remove it from the free list
        EncList.free_ref[EncList.free_count] = INVALID_ENC_IDX;

        if ( VALID_ENC_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            EncList.lst[retval].obj_base.in_free_list = bfalse;
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
void EncList_free_all( void )
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

    for ( cnt = 0; cnt < EncList.free_count; cnt++ )
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
bool_t EncList_add_free_ref( const ENC_REF ienc )
{
    bool_t retval;

    if ( !VALID_ENC_RANGE( ienc ) ) return bfalse;

#if defined(_DEBUG) && defined(DEBUG_ENC_LIST)
    if ( EncList_find_free_ref( ienc ) > 0 )
    {
        return bfalse;
    }
#endif

    EGOBOO_ASSERT( !EncList.lst[ienc].obj_base.in_free_list );

    retval = bfalse;
    if ( EncList.free_count < MAX_ENC )
    {
        EncList.free_ref[EncList.free_count] = ienc;

        EncList.free_count++;
        EncList.update_guid++;

        EncList.lst[ienc].obj_base.in_free_list = btrue;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_remove_free_idx( const int index )
{
    ENC_REF ienc;

    // was it found?
    if ( index < 0 || index >= EncList.free_count ) return bfalse;

    ienc = EncList.free_ref[index];

    // blank out the index in the list
    EncList.free_ref[index] = INVALID_ENC_IDX;

    if ( VALID_ENC_RANGE( ienc ) )
    {
        // let the object know it is not in the list anymore
        EncList.lst[ienc].obj_base.in_free_list = bfalse;
    }

    // shorten the list
    EncList.free_count--;
    EncList.update_guid++;

    if ( EncList.free_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, EncList.free_ref[index], EncList.free_ref[EncList.free_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int EncList_find_used_ref( const ENC_REF ienc )
{
    /// @author BB
    /// @details if an object of index iobj exists on the used list, return the used list index
    ///     otherwise return -1

    int retval = -1, cnt;

    if ( !VALID_ENC_RANGE( ienc ) ) return retval;

    for ( cnt = 0; cnt < EncList.used_count; cnt++ )
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
bool_t EncList_push_used( const ENC_REF ienc )
{
    bool_t retval;

    if ( !VALID_ENC_RANGE( ienc ) ) return bfalse;

#if defined(_DEBUG) && defined(DEBUG_ENC_LIST)
    if ( EncList_find_used_ref( ienc ) > 0 )
    {
        return bfalse;
    }
#endif

    EGOBOO_ASSERT( !EncList.lst[ienc].obj_base.in_used_list );

    retval = bfalse;
    if ( EncList.used_count < MAX_ENC )
    {
        EncList.used_ref[EncList.used_count] = REF_TO_INT( ienc );

        EncList.used_count++;
        EncList.update_guid++;

        EncList.lst[ienc].obj_base.in_used_list = btrue;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_remove_used_idx( const int index )
{
    ENC_REF ienc;

    // was it found?
    if ( index < 0 || index >= EncList.used_count ) return bfalse;

    ienc = ( ENC_REF )EncList.used_ref[index];

    // blank out the index in the list
    EncList.used_ref[index] = INVALID_ENC_IDX;

    if ( VALID_ENC_RANGE( ienc ) )
    {
        // let the object know it is not in the list anymore
        EncList.lst[ienc].obj_base.in_used_list = bfalse;
    }

    // shorten the list
    EncList.used_count--;
    EncList.update_guid++;

    if ( EncList.used_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, EncList.used_ref[index], EncList.used_ref[EncList.used_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_remove_used_ref( const ENC_REF ienc )
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
        ienc = EncList_pop_free( -1 );
        if ( override != ienc )
        {
            int override_index = EncList_find_free_ref( override );

            if ( override_index < 0 || override_index >= EncList.free_count )
            {
                ienc = INVALID_ENC_REF;
            }
            else
            {
                // store the "wrong" value in the override enchant's index
                EncList.free_ref[override_index] = REF_TO_INT( ienc );

                // fix the in_free_list values
                EncList.lst[ienc].obj_base.in_free_list = btrue;
                EncList.lst[override].obj_base.in_free_list = bfalse;

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
        ienc = EncList_pop_free( -1 );
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
void EncList_cleanup( void )
{
    int     cnt;
    enc_t * penc;

    // go through the list and activate all the enchants that
    // were created while the list was iterating
    for ( cnt = 0; cnt < enc_activation_count; cnt++ )
    {
        ENC_REF ienc = enc_activation_list[cnt];

        if ( !ALLOCATED_ENC( ienc ) ) continue;
        penc = EncList_get_ptr( ienc );

        if ( !penc->obj_base.turn_me_on ) continue;

        penc->obj_base.on         = btrue;
        penc->obj_base.turn_me_on = bfalse;
    }
    enc_activation_count = 0;

    // go through and delete any enchants that were
    // supposed to be deleted while the list was iterating
    for ( cnt = 0; cnt < enc_termination_count; cnt++ )
    {
        EncList_free_one( enc_termination_list[cnt] );
    }
    enc_termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_add_activation( const ENC_REF ienc )
{
    // put this enchant into the activation list so that it can be activated right after
    // the EncList loop is completed

    bool_t retval = bfalse;

    if ( !VALID_ENC_RANGE( ienc ) ) return bfalse;

    if ( enc_activation_count < MAX_ENC )
    {
        enc_activation_list[enc_activation_count] = ienc;
        enc_activation_count++;

        retval = btrue;
    }

    EncList.lst[ienc].obj_base.turn_me_on = btrue;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_add_termination( const ENC_REF ienc )
{
    bool_t retval = bfalse;

    if ( !VALID_ENC_RANGE( ienc ) ) return bfalse;

    if ( enc_termination_count < MAX_ENC )
    {
        enc_termination_list[enc_termination_count] = ienc;
        enc_termination_count++;

        retval = btrue;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE( EncList_get_ptr( ienc ) );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t EncList_request_terminate( const ENC_REF ienc )
{
    enc_t * penc = EncList_get_ptr( ienc );

    return enc_request_terminate( penc );
}

//--------------------------------------------------------------------------------------------
bool_t EncList_remove_free_ref( const ENC_REF ienc )
{
    // find the object in the free list
    int index = EncList_find_free_ref( ienc );

    return EncList_remove_free_idx( index );
}
