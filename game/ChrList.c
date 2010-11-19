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

/// @file ChrList.c
/// @brief Implementation of the ChrList_* functions
/// @details

#include "ChrList.h"
#include "log.h"
#include "egoboo_object.h"

#include "char.inl"

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
//--------------------------------------------------------------------------------------------

static size_t  ChrList_get_free();

static bool_t ChrList_remove_used( const CHR_REF ichr );
static bool_t ChrList_remove_used_index( int index );
static bool_t ChrList_add_free( const CHR_REF ichr );
static bool_t ChrList_remove_free( const CHR_REF ichr );
static bool_t ChrList_remove_free_index( int index );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ChrList_init()
{
    int cnt;

    ChrList.free_count = 0;
    ChrList.used_count = 0;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList.free_ref[cnt] = MAX_CHR;
        ChrList.used_ref[cnt] = MAX_CHR;
    }

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        CHR_REF ichr = ( MAX_CHR - 1 ) - cnt;
        chr_t * pchr = ChrList.lst + ichr;

        // blank out all the data, including the obj_base data
        memset( pchr, 0, sizeof( *pchr ) );

        // character "initializer"
        ego_object_ctor( POBJ_GET_PBASE( pchr ) );

        ChrList_add_free( ichr );
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_dtor()
{
    CHR_REF cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        chr_config_deconstruct( ChrList.lst + cnt, 100 );
    }

    ChrList.free_count = 0;
    ChrList.used_count = 0;
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList.free_ref[cnt] = MAX_CHR;
        ChrList.used_ref[cnt] = MAX_CHR;
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_prune_used()
{
    // prune the used list

    size_t cnt;
    CHR_REF ichr;

    for ( cnt = 0; cnt < ChrList.used_count; cnt++ )
    {
        bool_t removed = bfalse;

        ichr = ChrList.used_ref[cnt];

        if ( !VALID_CHR_RANGE( ichr ) || !DEFINED_CHR( ichr ) )
        {
            removed = ChrList_remove_used_index( cnt );
        }

        if ( removed && !ChrList.lst[ichr].obj_base.in_free_list )
        {
            ChrList_add_free( ichr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_prune_free()
{
    // prune the free list

    size_t cnt;
    CHR_REF ichr;

    for ( cnt = 0; cnt < ChrList.free_count; cnt++ )
    {
        bool_t removed = bfalse;

        ichr = ChrList.free_ref[cnt];

        if ( VALID_CHR_RANGE( ichr ) && INGAME_CHR_BASE( ichr ) )
        {
            removed = ChrList_remove_free_index( cnt );
        }

        if ( removed && !ChrList.lst[ichr].obj_base.in_free_list )
        {
            ChrList_add_used( ichr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void ChrList_update_used()
{
    size_t cnt;
    CHR_REF ichr;

    ChrList_prune_used();
    ChrList_prune_free();

    // go through the character list to see if there are any dangling characters
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        if ( !ALLOCATED_CHR( ichr ) ) continue;

        if ( INGAME_CHR( ichr ) )
        {
            if ( !ChrList.lst[ichr].obj_base.in_used_list )
            {
                ChrList_add_used( ichr );
            }
        }
        else if ( !DEFINED_CHR( ichr ) )
        {
            if ( !ChrList.lst[ichr].obj_base.in_free_list )
            {
                ChrList_add_free( ichr );
            }
        }
    }

    // blank out the unused elements of the used list
    for ( cnt = ChrList.used_count; cnt < MAX_CHR; cnt++ )
    {
        ChrList.used_ref[cnt] = MAX_CHR;
    }

    // blank out the unused elements of the free list
    for ( cnt = ChrList.free_count; cnt < MAX_CHR; cnt++ )
    {
        ChrList.free_ref[cnt] = MAX_CHR;
    }
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_free_one( const CHR_REF ichr )
{
    /// @details ZZ@> This function sticks a character back on the free enchant stack
    ///
    /// @note Tying ALLOCATED_CHR() and POBJ_TERMINATE() to ChrList_free_one()
    /// should be enough to ensure that no character is freed more than once

    bool_t retval;
    chr_t * pchr;
    obj_data_t * pbase;

    if ( !ALLOCATED_CHR( ichr ) ) return bfalse;
    pchr = ChrList.lst + ichr;
    pbase = POBJ_GET_PBASE( pchr );

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
        if ( NULL == pchr ) return bfalse;

        if ( pbase->in_used_list )
        {
            ChrList_remove_used( ichr );
        }

        if ( pbase->in_free_list )
        {
            retval = btrue;
        }
        else
        {
            retval = ChrList_add_free( ichr );
        }

        // character "destructor"
        pchr = chr_dtor( pchr );
        if ( NULL == pchr ) return bfalse;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t ChrList_get_free()
{
    /// @details ZZ@> This function returns the next free character or MAX_CHR if there are none

    size_t retval = MAX_CHR;

    if ( ChrList.free_count > 0 )
    {
        ChrList.free_count--;
        ChrList.update_guid++;

        retval = ChrList.free_ref[ChrList.free_count];

        // completely remove it from the free list
        ChrList.free_ref[ChrList.free_count] = MAX_CHR;

        if ( VALID_CHR_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            ChrList.lst[retval].obj_base.in_free_list = bfalse;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void ChrList_free_all()
{
    CHR_REF cnt;

    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
int ChrList_get_free_list_index( const CHR_REF ichr )
{
    int retval = -1, cnt;

    if ( !VALID_CHR_RANGE( ichr ) ) return retval;

    for ( cnt = 0; cnt < ChrList.free_count; cnt++ )
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
bool_t ChrList_add_free( const CHR_REF ichr )
{
    bool_t retval;

    if ( !VALID_CHR_RANGE( ichr ) ) return bfalse;

#if defined(_DEBUG) && defined(DEBUG_CHR_LIST)
    if ( ChrList_get_free_list_index( ichr ) > 0 )
    {
        return bfalse;
    }
#endif

    EGOBOO_ASSERT( !ChrList.lst[ichr].obj_base.in_free_list );

    retval = bfalse;
    if ( ChrList.free_count < MAX_CHR )
    {
        ChrList.free_ref[ChrList.free_count] = ichr;

        ChrList.free_count++;
        ChrList.update_guid++;

        ChrList.lst[ichr].obj_base.in_free_list = btrue;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_free_index( int index )
{
    CHR_REF ichr;

    // was it found?
    if ( index < 0 || index >= ChrList.free_count ) return bfalse;

    ichr = ChrList.free_ref[index];

    // blank out the index in the list
    ChrList.free_ref[index] = MAX_CHR;

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // let the object know it is not in the list anymore
        ChrList.lst[ichr].obj_base.in_free_list = bfalse;
    }

    // shorten the list
    ChrList.free_count--;
    ChrList.update_guid++;

    if ( ChrList.free_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, ChrList.free_ref[index], ChrList.free_ref[ChrList.free_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_free( const CHR_REF ichr )
{
    // find the object in the free list
    int index = ChrList_get_free_list_index( ichr );

    return ChrList_remove_free_index( index );
}

//--------------------------------------------------------------------------------------------
int ChrList_get_used_list_index( const CHR_REF ichr )
{
    int retval = -1, cnt;

    if ( !VALID_CHR_RANGE( ichr ) ) return retval;

    for ( cnt = 0; cnt < ChrList.used_count; cnt++ )
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
bool_t ChrList_add_used( const CHR_REF ichr )
{
    bool_t retval;

    if ( !VALID_CHR_RANGE( ichr ) ) return bfalse;

#if defined(_DEBUG) && defined(DEBUG_CHR_LIST)
    if ( ChrList_get_used_list_index( ichr ) > 0 )
    {
        return bfalse;
    }
#endif

    EGOBOO_ASSERT( !ChrList.lst[ichr].obj_base.in_used_list );

    retval = bfalse;
    if ( ChrList.used_count < MAX_CHR )
    {
        ChrList.used_ref[ChrList.used_count] = ichr;

        ChrList.used_count++;
        ChrList.update_guid++;

        ChrList.lst[ichr].obj_base.in_used_list = btrue;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_used_index( int index )
{
    CHR_REF ichr;

    // was it found?
    if ( index < 0 || index >= ChrList.used_count ) return bfalse;

    ichr = ChrList.used_ref[index];

    // blank out the index in the list
    ChrList.used_ref[index] = MAX_CHR;

    if ( VALID_CHR_RANGE( ichr ) )
    {
        // let the object know it is not in the list anymore
        ChrList.lst[ichr].obj_base.in_used_list = bfalse;
    }

    // shorten the list
    ChrList.used_count--;
    ChrList.update_guid++;

    if ( ChrList.used_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, ChrList.used_ref[index], ChrList.used_ref[ChrList.used_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_remove_used( const CHR_REF ichr )
{
    // find the object in the used list
    int index = ChrList_get_used_list_index( ichr );

    return ChrList_remove_used_index( index );
}

//--------------------------------------------------------------------------------------------
CHR_REF ChrList_allocate( const CHR_REF override )
{
    CHR_REF ichr = ( CHR_REF )MAX_CHR;

    if ( VALID_CHR_RANGE( override ) )
    {
        ichr = ChrList_get_free();
        if ( override != ichr )
        {
            int override_index = ChrList_get_free_list_index( override );

            if ( override_index < 0 || override_index >= ChrList.free_count )
            {
                ichr = ( CHR_REF )MAX_CHR;
            }
            else
            {
                // store the "wrong" value in the override character's index
                ChrList.free_ref[override_index] = ichr;

                // fix the in_free_list values
                ChrList.lst[ichr].obj_base.in_free_list = btrue;
                ChrList.lst[override].obj_base.in_free_list = bfalse;

                ichr = override;
            }
        }

        if ( MAX_CHR == ichr )
        {
            log_warning( "ChrList_allocate() - failed to override a character? character %d already spawned? \n", REF_TO_INT( override ) );
        }
    }
    else
    {
        ichr = ChrList_get_free();
        if ( MAX_CHR == ichr )
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
        POBJ_ALLOCATE( ChrList.lst +  ichr , REF_TO_INT( ichr ) );
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
    int     cnt;
    chr_t * pchr;

    // go through the list and activate all the characters that
    // were created while the list was iterating
    for ( cnt = 0; cnt < chr_activation_count; cnt++ )
    {
        CHR_REF ichr = chr_activation_list[cnt];

        if ( !ALLOCATED_CHR( ichr ) ) continue;
        pchr = ChrList.lst + ichr;

        if ( !pchr->obj_base.turn_me_on ) continue;

        pchr->obj_base.on         = btrue;
        pchr->obj_base.turn_me_on = bfalse;
    }
    chr_activation_count = 0;

    // go through and delete any characters that were
    // supposed to be deleted while the list was iterating
    for ( cnt = 0; cnt < chr_termination_count; cnt++ )
    {
        ChrList_free_one( chr_termination_list[cnt] );
    }
    chr_termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_add_activation( CHR_REF ichr )
{
    // put this character into the activation list so that it can be activated right after
    // the ChrList loop is completed

    bool_t retval = bfalse;

    if ( !VALID_CHR_RANGE( ichr ) ) return bfalse;

    if ( chr_activation_count < MAX_CHR )
    {
        chr_activation_list[chr_activation_count] = ichr;
        chr_activation_count++;

        retval = btrue;
    }

    ChrList.lst[ichr].obj_base.turn_me_on = btrue;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ChrList_add_termination( CHR_REF ichr )
{
    bool_t retval = bfalse;

    if ( !VALID_CHR_RANGE( ichr ) ) return bfalse;

    if ( chr_termination_count < MAX_CHR )
    {
        chr_termination_list[chr_termination_count] = ichr;
        chr_termination_count++;

        retval = btrue;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE( ChrList.lst + ichr );

    return retval;
}
