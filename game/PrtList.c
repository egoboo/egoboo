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

/// @file PrtList.c
/// @brief Implementation of the PrtList_* functions
/// @details

#include "PrtList.h"

#include "log.h"
#include "egoboo_object.h"

#include "particle.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, prt_t, PrtList, MAX_PRT );

static size_t  prt_termination_count = 0;
static PRT_REF prt_termination_list[MAX_PRT];

static size_t  prt_activation_count = 0;
static PRT_REF prt_activation_list[MAX_PRT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_loop_depth = 0;

size_t maxparticles       = 512;
bool_t maxparticles_dirty = btrue;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static size_t  PrtList_get_free();

static bool_t PrtList_remove_used( const PRT_REF iprt );
static bool_t PrtList_remove_used_index( int index );
static bool_t PrtList_add_free( const PRT_REF iprt );
static bool_t PrtList_remove_free( const PRT_REF iprt );
static bool_t PrtList_remove_free_index( int index );

static size_t  PrtList_get_free();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void PrtList_init()
{
    int cnt;

    // fix any problems with maxparticles
    maxparticles = MIN( maxparticles, MAX_PRT );

    // free all the particles
    PrtList.free_count = 0;
    PrtList.used_count = 0;
    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
    {
        PrtList.free_ref[cnt] = MAX_PRT;
        PrtList.used_ref[cnt] = MAX_PRT;
    }

    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
    {
        PRT_REF iprt = ( MAX_PRT - 1 ) - cnt;
        prt_t * pprt = PrtList.lst + iprt;

        // blank out all the data, including the obj_base data
        memset( pprt, 0, sizeof( *pprt ) );

        // particle "initializer"
        ego_object_ctor( POBJ_GET_PBASE( pprt ) );

        PrtList_add_free( iprt );
    }

    maxparticles_dirty = bfalse;
}

//--------------------------------------------------------------------------------------------
void PrtList_dtor()
{
    PRT_REF cnt;

    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
    {
        prt_config_deconstruct( PrtList.lst + cnt, 100 );
    }

    PrtList.free_count = 0;
    PrtList.used_count = 0;
    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
    {
        PrtList.free_ref[cnt] = MAX_PRT;
        PrtList.used_ref[cnt] = MAX_PRT;
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_prune_used()
{
    // prune the used list

    int cnt;
    PRT_REF iprt;

    for ( cnt = 0; cnt < PrtList.used_count; cnt++ )
    {
        bool_t removed = bfalse;

        iprt = PrtList.used_ref[cnt];

        if ( !VALID_PRT_RANGE( iprt ) || !DEFINED_PRT( iprt ) )
        {
            removed = PrtList_remove_used_index( cnt );
        }

        if ( removed && !PrtList.lst[iprt].obj_base.in_free_list )
        {
            PrtList_add_free( iprt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_prune_free()
{
    // prune the free list

    int cnt;
    PRT_REF iprt;

    for ( cnt = 0; cnt < PrtList.free_count; cnt++ )
    {
        bool_t removed = bfalse;

        iprt = PrtList.free_ref[cnt];

        if ( VALID_PRT_RANGE( iprt ) && INGAME_PRT_BASE( iprt ) )
        {
            removed = PrtList_remove_free_index( cnt );
        }

        if ( removed && !PrtList.lst[iprt].obj_base.in_free_list )
        {
            PrtList_add_used( iprt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_update_used()
{
    PRT_REF iprt;

    PrtList_prune_used();
    PrtList_prune_free();

    // go through the particle list to see if there are any dangling particles
    for ( iprt = 0; iprt < MAX_PRT; iprt++ )
    {
        if ( !ALLOCATED_PRT( iprt ) ) continue;

        if ( DISPLAY_PRT( iprt ) )
        {
            if ( !PrtList.lst[iprt].obj_base.in_used_list )
            {
                PrtList_add_used( iprt );
            }
        }
        else if ( !DEFINED_PRT( iprt ) )
        {
            if ( !PrtList.lst[iprt].obj_base.in_free_list )
            {
                PrtList_add_free( iprt );
            }
        }
    }

    // blank out the unused elements of the used list
    for ( iprt = PrtList.used_count; iprt < MAX_PRT; iprt++ )
    {
        PrtList.used_ref[iprt] = MAX_PRT;
    }

    // blank out the unused elements of the free list
    for ( iprt = PrtList.free_count; iprt < MAX_PRT; iprt++ )
    {
        PrtList.free_ref[iprt] = MAX_PRT;
    }
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_free_one( const PRT_REF iprt )
{
    /// @details ZZ@> This function sticks a particle back on the free particle stack
    ///
    /// @note Tying ALLOCATED_PRT() and POBJ_TERMINATE() to PrtList_free_one()
    /// should be enough to ensure that no particle is freed more than once

    bool_t retval;
    prt_t * pprt;
    obj_data_t * pbase;

    if ( !ALLOCATED_PRT( iprt ) ) return bfalse;
    pprt = PrtList.lst + iprt;
    pbase = POBJ_GET_PBASE( pprt );

    // if we are inside a PrtList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( prt_loop_depth > 0 )
    {
        retval = PrtList_add_termination( iprt );
    }
    else
    {
        // deallocate any dynamically allocated memory
        pprt = prt_config_deinitialize( pprt, 100 );
        if ( NULL == pprt ) return bfalse;

        if ( pbase->in_used_list )
        {
            PrtList_remove_used( iprt );
        }

        if ( pbase->in_free_list )
        {
            retval = btrue;
        }
        else
        {
            retval = PrtList_add_free( iprt );
        }

        // particle "destructor"
        pprt = prt_config_deconstruct( pprt, 100 );
        if ( NULL == pprt ) return bfalse;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t PrtList_get_free()
{
    /// @details ZZ@> This function returns the next free particle or MAX_PRT if there are none

    size_t retval = MAX_PRT;

    // shed any values that are greater than maxparticles
    while ( PrtList.free_count > 0 && retval >= maxparticles )
    {
        PrtList.free_count--;
        PrtList.update_guid++;

        retval = PrtList.free_ref[PrtList.free_count];

        // completely remove it from the free list
        PrtList.free_ref[PrtList.free_count] = MAX_PRT;

        if ( VALID_PRT_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            PrtList.lst[retval].obj_base.in_free_list = bfalse;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
PRT_REF PrtList_allocate( bool_t force )
{
    /// @details ZZ@> This function gets an unused particle.  If all particles are in use
    ///    and force is set, it grabs the first unimportant one.  The iprt
    ///    index is the return value

    PRT_REF iprt;

    // Return MAX_PRT if we can't find one
    iprt = ( PRT_REF )MAX_PRT;

    if ( 0 == PrtList.free_count )
    {
        if ( force )
        {
            PRT_REF found           = ( PRT_REF )MAX_PRT;
            size_t  min_life        = ( size_t )( ~0 );
            PRT_REF min_life_idx    = ( PRT_REF )MAX_PRT;
            size_t  min_display     = ( size_t )( ~0 );
            PRT_REF min_display_idx = ( PRT_REF )MAX_PRT;

            // Gotta find one, so go through the list and replace a unimportant one
            for ( iprt = 0; iprt < maxparticles; iprt++ )
            {
                bool_t was_forced = bfalse;
                prt_t * pprt;

                // Is this an invalid particle? The particle allocation count is messed up! :(
                if ( !DEFINED_PRT( iprt ) )
                {
                    found = iprt;
                    break;
                }
                pprt =  PrtList.lst +  iprt;

                // does it have a valid profile?
                if ( !LOADED_PIP( pprt->pip_ref ) )
                {
                    found = iprt;
                    end_one_particle_in_game( iprt );
                    break;
                }

                // do not bump another
                was_forced = ( PipStack.lst[pprt->pip_ref].force );

                if ( WAITING_PRT( iprt ) )
                {
                    // if the particle has been "terminated" but is still waiting around, bump it to the
                    // front of the list

                    size_t min_time  = MIN( pprt->lifetime_remaining, pprt->frames_remaining );

                    if ( min_time < MAX( min_life, min_display ) )
                    {
                        min_life     = pprt->lifetime_remaining;
                        min_life_idx = iprt;

                        min_display     = pprt->frames_remaining;
                        min_display_idx = iprt;
                    }
                }
                else if ( !was_forced )
                {
                    // if the particle has not yet died, let choose the worst one

                    if ( pprt->lifetime_remaining < min_life )
                    {
                        min_life     = pprt->lifetime_remaining;
                        min_life_idx = iprt;
                    }

                    if ( pprt->frames_remaining < min_display )
                    {
                        min_display     = pprt->frames_remaining;
                        min_display_idx = iprt;
                    }
                }
            }

            if ( VALID_PRT_RANGE( found ) )
            {
                // found a "bad" particle
                iprt = found;
            }
            else if ( VALID_PRT_RANGE( min_display_idx ) )
            {
                // found a "terminated" particle
                iprt = min_display_idx;
            }
            else if ( VALID_PRT_RANGE( min_life_idx ) )
            {
                // found a particle that closest to death
                iprt = min_life_idx;
            }
            else
            {
                // found nothing. this should only happen if all the
                // particles are forced
                iprt = ( PRT_REF )MAX_PRT;
            }
        }
    }
    else
    {
        if ( PrtList.free_count > ( maxparticles / 4 ) )
        {
            // Just grab the next one
            iprt = PrtList_get_free();
        }
        else if ( force )
        {
            iprt = PrtList_get_free();
        }
    }

    // return a proper value
    iprt = ( iprt >= maxparticles ) ? ( PRT_REF )MAX_PRT : iprt;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // if the particle is already being used, make sure to destroy the old one
        if ( DEFINED_PRT( iprt ) )
        {
            end_one_particle_in_game( iprt );
        }

        // allocate the new one
        POBJ_ALLOCATE( PrtList.lst +  iprt , REF_TO_INT( iprt ) );
    }

    if ( ALLOCATED_PRT( iprt ) )
    {
        // construct the new structure
        prt_config_construct( PrtList.lst +  iprt, 100 );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
void PrtList_free_all()
{
    /// @details ZZ@> This function resets the particle allocation lists

    PRT_REF cnt;

    // free all the particles
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        PrtList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
int PrtList_get_free_list_index( const PRT_REF iprt )
{
    int retval = -1, cnt;

    if ( !VALID_PRT_RANGE( iprt ) ) return retval;

    for ( cnt = 0; cnt < PrtList.free_count; cnt++ )
    {
        if ( iprt == PrtList.free_ref[cnt] )
        {
            EGOBOO_ASSERT( PrtList.lst[iprt].obj_base.in_free_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_add_free( const PRT_REF iprt )
{
    bool_t retval;

    if ( !VALID_PRT_RANGE( iprt ) ) return bfalse;

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
    if ( PrtList_get_free_list_index( iprt ) > 0 )
    {
        return bfalse;
    }
#endif

    EGOBOO_ASSERT( !PrtList.lst[iprt].obj_base.in_free_list );

    retval = bfalse;
    if ( PrtList.free_count < maxparticles )
    {
        PrtList.free_ref[PrtList.free_count] = iprt;

        PrtList.free_count++;
        PrtList.update_guid++;

        PrtList.lst[iprt].obj_base.in_free_list = btrue;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_remove_free_index( int index )
{
    PRT_REF iprt;

    // was it found?
    if ( index < 0 || index >= PrtList.free_count ) return bfalse;

    iprt = PrtList.free_ref[index];

    // blank out the index in the list
    PrtList.free_ref[index] = MAX_PRT;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // let the object know it is not in the list anymore
        PrtList.lst[iprt].obj_base.in_free_list = bfalse;
    }

    // shorten the list
    PrtList.free_count--;
    PrtList.update_guid++;

    if ( PrtList.free_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, PrtList.free_ref[index], PrtList.free_ref[PrtList.free_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_remove_free( const PRT_REF iprt )
{
    // find the object in the free list
    int index = PrtList_get_free_list_index( iprt );

    return PrtList_remove_free_index( index );
}

//--------------------------------------------------------------------------------------------
int PrtList_get_used_list_index( const PRT_REF iprt )
{
    int retval = -1, cnt;

    if ( !VALID_PRT_RANGE( iprt ) ) return retval;

    for ( cnt = 0; cnt < MAX_PRT; cnt++ )
    {
        if ( iprt == PrtList.used_ref[cnt] )
        {
            EGOBOO_ASSERT( PrtList.lst[iprt].obj_base.in_used_list );
            retval = cnt;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_add_used( const PRT_REF iprt )
{
    bool_t retval;

    if ( !VALID_PRT_RANGE( iprt ) ) return bfalse;

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
    if ( PrtList_get_used_list_index( iprt ) > 0 )
    {
        return bfalse;
    }
#endif

    EGOBOO_ASSERT( !PrtList.lst[iprt].obj_base.in_used_list );

    retval = bfalse;
    if ( PrtList.used_count < maxparticles )
    {
        PrtList.used_ref[PrtList.used_count] = iprt;

        PrtList.used_count++;
        PrtList.update_guid++;

        PrtList.lst[iprt].obj_base.in_used_list = btrue;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_remove_used_index( int index )
{
    PRT_REF iprt;

    // was it found?
    if ( index < 0 || index >= PrtList.used_count ) return bfalse;

    iprt = PrtList.used_ref[index];

    // blank out the index in the list
    PrtList.used_ref[index] = MAX_PRT;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // let the object know it is not in the list anymore
        PrtList.lst[iprt].obj_base.in_used_list = bfalse;
    }

    // shorten the list
    PrtList.used_count--;
    PrtList.update_guid++;

    if ( PrtList.used_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, PrtList.used_ref[index], PrtList.used_ref[PrtList.used_count] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_remove_used( const PRT_REF iprt )
{
    // find the object in the used list
    int index = PrtList_get_used_list_index( iprt );

    return PrtList_remove_used_index( index );
}

//--------------------------------------------------------------------------------------------
void PrtList_cleanup()
{
    int     cnt;
    prt_t * pprt;

    // go through the list and activate all the particles that
    // were created while the list was iterating
    for ( cnt = 0; cnt < prt_activation_count; cnt++ )
    {
        PRT_REF iprt = prt_activation_list[cnt];

        if ( !ALLOCATED_PRT( iprt ) ) continue;
        pprt = PrtList.lst + iprt;

        if ( !pprt->obj_base.turn_me_on ) continue;

        pprt->obj_base.on         = btrue;
        pprt->obj_base.turn_me_on = bfalse;
    }
    prt_activation_count = 0;

    // go through and delete any particles that were
    // supposed to be deleted while the list was iterating
    for ( cnt = 0; cnt < prt_termination_count; cnt++ )
    {
        PrtList_free_one( prt_termination_list[cnt] );
    }
    prt_termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_add_activation( PRT_REF iprt )
{
    // put this particle into the activation list so that it can be activated right after
    // the PrtList loop is completed

    bool_t retval = bfalse;

    if ( !VALID_PRT_RANGE( iprt ) ) return bfalse;

    if ( prt_activation_count < MAX_PRT )
    {
        prt_activation_list[prt_activation_count] = iprt;
        prt_activation_count++;

        retval = btrue;
    }

    PrtList.lst[iprt].obj_base.turn_me_on = btrue;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_add_termination( PRT_REF iprt )
{
    bool_t retval = bfalse;

    if ( !VALID_PRT_RANGE( iprt ) ) return bfalse;

    if ( prt_termination_count < MAX_PRT )
    {
        prt_termination_list[prt_termination_count] = iprt;
        prt_termination_count++;

        retval = btrue;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE( PrtList.lst + iprt );

    return retval;
}
