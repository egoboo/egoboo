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

/// @file  game/ChrList.c
/// @brief Implementation of the ChrList_* functions

#include "game/ChrList.h"
#include "game/char.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, chr_t, ChrList, MAX_CHR );

static size_t  chr_termination_count = 0;
static CHR_REF chr_termination_list[MAX_CHR];

static size_t  chr_activation_count = 0;
static CHR_REF chr_activation_list[MAX_CHR];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int chr_loop_depth = 0;

//--------------------------------------------------------------------------------------------
// private ChrList_t functions
//--------------------------------------------------------------------------------------------

static void ChrList_clear();
static void ChrList_init();
static void ChrList_deinit();

static bool ChrList_add_free_ref( const CHR_REF ichr );
static bool ChrList_remove_free_ref( const CHR_REF ichr );
static bool ChrList_remove_free_idx( const int index );

static bool ChrList_remove_used_ref( const CHR_REF ichr );
static bool ChrList_remove_used_idx( const int index );

static void ChrList_prune_used_list();
static void ChrList_prune_free_list();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( chr_t, ChrList, MAX_CHR );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ChrList_ctor()
{
    // Initialize the list.
    ChrList_init();

    // Construct the sub-objects.
    for (CHR_REF cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t *chr = ChrList.lst + cnt;

        // Blank out all the data, including the entity data.
		BLANK_STRUCT_PTR(chr);
        // Initialize the entity.
        Ego::Entity::ctor(POBJ_GET_PBASE(chr), chr, BSP_LEAF_CHR, cnt);
        // Initialize character.
        chr_t::ctor(chr);
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_dtor()
{
    // construct the sub-objects
    for ( CHR_REF cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_t *pchr = ChrList.lst + cnt;

        // destruct the object
        chr_dtor( pchr );

        // destruct the parent
        Ego::Entity::dtor( POBJ_GET_PBASE( pchr ) );
    }

    // initialize particle
    ChrList_init();
}

//--------------------------------------------------------------------------------------------
void ChrList_clear()
{
    // clear out the list
    ChrList.free_count = 0;
    ChrList.used_count = 0;
    for ( CHR_REF cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        // blank out the list
        ChrList.free_ref[cnt] = INVALID_CHR_IDX;
        ChrList.used_ref[cnt] = INVALID_CHR_IDX;

        // let the particle data know that it is not in a list
        ChrList.lst[cnt].obj_base.in_free_list = false;
        ChrList.lst[cnt].obj_base.in_used_list = false;
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_init()
{
    ChrList_clear();

    // place all the characters on the free list
    for ( size_t cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        CHR_REF ichr = ( MAX_CHR - 1 ) - cnt;

        ChrList_add_free_ref( ichr );
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_deinit()
{
    // request that the sub-objects deconstruct themselves
    for ( CHR_REF cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_config_deconstruct( ChrList_get_ptr( cnt ), 100 );
    }

    // initialize the list
    ChrList_init();
}

//--------------------------------------------------------------------------------------------
void ChrList_reinit()
{
    ChrList_deinit();
    ChrList_init();
}

//--------------------------------------------------------------------------------------------
void ChrList_prune_used_list()
{
    // prune the used list
    for ( size_t cnt = 0; cnt < ChrList.used_count; cnt++ )
    {
        bool removed = false;

        CHR_REF ichr = ( CHR_REF )ChrList.used_ref[cnt];

        if ( !VALID_CHR_RANGE( ichr ) || !DEFINED_CHR( ichr ) )
        {
            removed = ChrList_remove_used_idx( cnt );
        }

        if ( removed && !ChrList.lst[ichr].obj_base.in_free_list )
        {
            ChrList_add_free_ref( ichr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_prune_free_list()
{
    // prune the free list
    for ( size_t cnt = 0; cnt < ChrList.free_count; cnt++ )
    {
        bool removed = false;

        CHR_REF ichr = ( CHR_REF )ChrList.free_ref[cnt];

        if ( VALID_CHR_RANGE( ichr ) && INGAME_CHR_BASE( ichr ) )
        {
            removed = ChrList_remove_free_idx( cnt );
        }

        if ( removed && !ChrList.lst[ichr].obj_base.in_free_list )
        {
            ChrList_push_used( ichr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_update_used()
{
    ChrList_prune_used_list();
    ChrList_prune_free_list();

    // go through the character list to see if there are any dangling characters
    for ( CHR_REF ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( !ALLOCATED_CHR( ichr ) ) continue;

        if ( INGAME_CHR( ichr ) )
        {
            if ( !ChrList.lst[ichr].obj_base.in_used_list )
            {
                ChrList_push_used( ichr );
            }
        }
        else if ( !DEFINED_CHR( ichr ) )
        {
            if ( !ChrList.lst[ichr].obj_base.in_free_list )
            {
                ChrList_add_free_ref( ichr );
            }
        }
    }

    // blank out the unused elements of the used list
    for ( size_t cnt = ChrList.used_count; cnt < MAX_CHR; cnt++ )
    {
        ChrList.used_ref[cnt] = INVALID_CHR_IDX;
    }

    // blank out the unused elements of the free list
    for ( size_t cnt = ChrList.free_count; cnt < MAX_CHR; cnt++ )
    {
        ChrList.free_ref[cnt] = INVALID_CHR_IDX;
    }
}

//--------------------------------------------------------------------------------------------
bool ChrList_free_one( const CHR_REF ichr )
{
    /// @author ZZ
    /// @details This function sticks a character back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_CHR() and POBJ_TERMINATE() to ChrList_free_one()
    /// should be enough to ensure that no character is freed more than once

    bool retval;

    if ( !ALLOCATED_CHR( ichr ) ) return false;
    chr_t *pchr = ChrList_get_ptr( ichr );
    Ego::Entity *pbase = POBJ_GET_PBASE( pchr );

#if (DEBUG_SCRIPT_LEVEL > 0) && defined(DEBUG_PROFILE) && defined(_DEBUG)
    chr_log_script_time( ichr );
#endif

    // if we are inside a ChrList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( chr_loop_depth > 0 )
    {
        retval = ChrList_add_termination( ichr );
    }
    else
    {
        // deallocate any dynamically allocated memory
        pchr = chr_config_deinitialize( pchr, 100 );
        if ( NULL == pchr ) return false;

        if ( pbase->in_used_list )
        {
            ChrList_remove_used_ref( ichr );
        }

        if ( pbase->in_free_list )
        {
            retval = true;
        }
        else
        {
            retval = ChrList_add_free_ref( ichr );
        }

        // character "destructor"
        pchr = chr_dtor( pchr );
        if ( NULL == pchr ) return false;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t ChrList_pop_free( const int idx )
{
    /// @author ZZ
    /// @details This function returns the next free character or MAX_CHR if there are none

    size_t retval = INVALID_CHR_REF;
    size_t loops  = 0;

    if ( idx >= 0 && idx < ChrList.free_count )
    {
        // the user has specified a valid index in the free stack
        // that they want to use. make that happen.

        // from the conditions, ChrList.free_count must be greater than 1
        size_t itop = ChrList.free_count - 1;

        // move the desired index to the top of the stack
        SWAP( size_t, ChrList.free_ref[idx], ChrList.free_ref[itop] );
    }

    while ( ChrList.free_count > 0 )
    {
        ChrList.free_count--;
        ChrList.update_guid++;

        retval = ChrList.free_ref[ChrList.free_count];

        // completely remove it from the free list
        ChrList.free_ref[ChrList.free_count] = INVALID_CHR_IDX;

        if ( VALID_CHR_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            ChrList.lst[retval].obj_base.in_free_list = false;
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
void ChrList_free_all()
{
    for ( CHR_REF cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
int ChrList_find_free_ref( const CHR_REF ichr )
{
    int retval = -1;

    if ( !VALID_CHR_RANGE( ichr ) ) return retval;

    for ( int cnt = 0; cnt < ChrList.free_count; cnt++ )
    {
        if ( ichr == ChrList.free_ref[cnt] )
        {
            EGOBOO_ASSERT( ChrList.lst[ichr].obj_base.in_free_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ChrList_add_free_ref( const CHR_REF ichr )
{
    if ( !VALID_CHR_RANGE( ichr ) ) return false;

#if defined(_DEBUG) && defined(DEBUG_CHR_LIST)
    if ( ChrList_find_free_ref( ichr ) > 0 )
    {
        return false;
    }
#endif

    EGOBOO_ASSERT( !ChrList.lst[ichr].obj_base.in_free_list );

    bool retval = false;
    if ( ChrList.free_count < MAX_CHR )
    {
        ChrList.free_ref[ChrList.free_count] = ichr;

        ChrList.free_count++;
        ChrList.update_guid++;

        ChrList.lst[ichr].obj_base.in_free_list = true;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ChrList_remove_free_idx( const int index )
{
    CHR_REF ichr;

    // was it found?
    if ( index < 0 || index >= ChrList.free_count ) return false;

    ichr = ( CHR_REF )ChrList.free_ref[index];

    // blank out the index in the list
    ChrList.free_ref[index] = INVALID_CHR_IDX;

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // let the object know it is not in the list anymore
        ChrList.lst[ichr].obj_base.in_free_list = false;
    }

    // shorten the list
    ChrList.free_count--;
    ChrList.update_guid++;

    if ( ChrList.free_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, ChrList.free_ref[index], ChrList.free_ref[ChrList.free_count] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
int ChrList_find_used_ref( const CHR_REF ichr )
{
    /// @author BB
    /// @details if an object of index iobj exists on the used list, return the used list index
    ///     otherwise return -1

    int retval = -1;

    if ( !VALID_CHR_RANGE( ichr ) ) return retval;

    for ( int cnt = 0; cnt < ChrList.used_count; cnt++ )
    {
        if ( ichr == ChrList.used_ref[cnt] )
        {
            EGOBOO_ASSERT( ChrList.lst[ichr].obj_base.in_used_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ChrList_push_used( const CHR_REF ichr )
{
    bool retval;

    if ( !VALID_CHR_RANGE( ichr ) ) return false;

#if defined(_DEBUG) && defined(DEBUG_CHR_LIST)
    if ( ChrList_find_used_ref( ichr ) > 0 )
    {
        return false;
    }
#endif

    EGOBOO_ASSERT( !ChrList.lst[ichr].obj_base.in_used_list );

    retval = false;
    if ( ChrList.used_count < MAX_CHR )
    {
        ChrList.used_ref[ChrList.used_count] = REF_TO_INT( ichr );

        ChrList.used_count++;
        ChrList.update_guid++;

        ChrList.lst[ichr].obj_base.in_used_list = true;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ChrList_remove_used_idx( const int index )
{
    // was it found?
    if ( index < 0 || index >= ChrList.used_count ) return false;

    CHR_REF ichr = ( CHR_REF )ChrList.used_ref[index];

    // blank out the index in the list
    ChrList.used_ref[index] = INVALID_CHR_IDX;

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // let the object know it is not in the list anymore
        ChrList.lst[ichr].obj_base.in_used_list = false;
    }

    // shorten the list
    ChrList.used_count--;
    ChrList.update_guid++;

    if ( ChrList.used_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, ChrList.used_ref[index], ChrList.used_ref[ChrList.used_count] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ChrList_remove_used_ref( const CHR_REF ichr )
{
    // find the object in the used list
    int index = ChrList_find_used_ref( ichr );

    return ChrList_remove_used_idx( index );
}

//--------------------------------------------------------------------------------------------
CHR_REF ChrList_allocate( const CHR_REF override )
{
    CHR_REF ichr = INVALID_CHR_REF;

    if ( VALID_CHR_RANGE( override ) )
    {
        ichr = ( CHR_REF )ChrList_pop_free( -1 );
        if ( override != ichr )
        {
            int override_index = ChrList_find_free_ref( override );

            if ( override_index < 0 || override_index >= ChrList.free_count )
            {
                ichr = INVALID_CHR_REF;
            }
            else
            {
                // store the "wrong" value in the override character's index
                ChrList.free_ref[override_index] = ichr;

                // fix the in_free_list values
                ChrList.lst[ichr].obj_base.in_free_list = true;
                ChrList.lst[override].obj_base.in_free_list = false;

                ichr = override;
            }
        }

        if ( INVALID_CHR_REF == ichr )
        {
            log_warning( "ChrList_allocate() - failed to override a character? character %d already spawned? \n", REF_TO_INT( override ) );
        }
    }
    else
    {
        ichr = ( CHR_REF )ChrList_pop_free( -1 );
        if ( INVALID_CHR_REF == ichr )
        {
            log_warning( "ChrList_allocate() - failed to allocate a new character\n" );
        }
    }

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // if the character is already being used, make sure to destroy the old one
        if ( DEFINED_CHR( ichr ) )
        {
            ChrList_free_one( ichr );
        }

        // allocate the new one
        POBJ_ALLOCATE( ChrList.lst + ichr, REF_TO_INT( ichr ) );
    }

    if ( ALLOCATED_CHR( ichr ) )
    {
        // construct the new structure
        chr_config_construct( ChrList.lst + ichr, 100 );
    }

    return ichr;
}

//--------------------------------------------------------------------------------------------
void ChrList_cleanup()
{
    chr_t * pchr;

    // go through the list and activate all the characters that
    // were created while the list was iterating
    for ( size_t cnt = 0; cnt < chr_activation_count; cnt++ )
    {
        CHR_REF ichr = chr_activation_list[cnt];

        if ( !ALLOCATED_CHR( ichr ) ) continue;
        pchr = ChrList_get_ptr( ichr );

        if ( !pchr->obj_base.turn_me_on ) continue;

        pchr->obj_base.on         = true;
        pchr->obj_base.turn_me_on = false;
    }
    chr_activation_count = 0;

    // go through and delete any characters that were
    // supposed to be deleted while the list was iterating
    for ( size_t cnt = 0; cnt < chr_termination_count; cnt++ )
    {
        ChrList_free_one( chr_termination_list[cnt] );
    }
    chr_termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool ChrList_add_activation( const CHR_REF ichr )
{
    // put this character into the activation list so that it can be activated right after
    // the ChrList loop is completed

    bool retval = false;

    if ( !VALID_CHR_RANGE( ichr ) ) return false;

    if ( chr_activation_count < MAX_CHR )
    {
        chr_activation_list[chr_activation_count] = ichr;
        chr_activation_count++;

        retval = true;
    }

    ChrList.lst[ichr].obj_base.turn_me_on = true;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ChrList_add_termination( const CHR_REF ichr )
{
    bool retval = false;

    if ( !VALID_CHR_RANGE( ichr ) ) return false;

    if ( chr_termination_count < MAX_CHR )
    {
        chr_termination_list[chr_termination_count] = ichr;
        chr_termination_count++;

        retval = true;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE( ChrList.lst + ichr );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ChrList_request_terminate( const CHR_REF ichr )
{
    /// @author BB
    /// @details Mark this character for deletion

    chr_t * pchr = ChrList_get_ptr( ichr );

    return chr_request_terminate( pchr );
}

//--------------------------------------------------------------------------------------------
int ChrList_count_free()
{
    return ChrList.free_count;
}

//--------------------------------------------------------------------------------------------
int ChrList_count_used()
{
    return ChrList.used_count; // MAX_CHR - ChrList.free_count;
}

//--------------------------------------------------------------------------------------------
bool ChrList_remove_free_ref( const CHR_REF ichr )
{
    // find the object in the free list
    int index = ChrList_find_free_ref( ichr );

    return ChrList_remove_free_idx( index );
}
