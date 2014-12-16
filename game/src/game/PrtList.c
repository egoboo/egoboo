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

/// @file game/PrtList.c
/// @brief Implementation of the PrtList_* functions
/// @details

#include "game/PrtList.h"

#include "game/egoboo_object.h"

#include "game/particle.inl"

#include "egolib/log.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static size_t  prt_termination_count = 0;
static PRT_REF prt_termination_list[MAX_PRT];

static size_t  prt_activation_count = 0;
static PRT_REF prt_activation_list[MAX_PRT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_LIST( ACCESS_TYPE_NONE, prt_t, PrtList, MAX_PRT );

int prt_loop_depth = 0;

size_t maxparticles       = 512;
ego_bool maxparticles_dirty = ego_true;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void    PrtList_clear( void );
static void    PrtList_init( void );
static void    PrtList_deinit( void );

static ego_bool  PrtList_add_free_ref( const PRT_REF iprt );
static ego_bool  PrtList_remove_free_ref( const PRT_REF iprt );
static ego_bool  PrtList_remove_free_idx( const int index );

static ego_bool  PrtList_remove_used_ref( const PRT_REF iprt );
static ego_bool  PrtList_remove_used_idx( const int index );

static void    PrtList_prune_used_list( void );
static void    PrtList_prune_free_list( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_LIST( prt_t, PrtList, MAX_PRT );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void PrtList_ctor( void )
{
    PRT_REF cnt;
    prt_t * pprt;

    // initialize the list
    PrtList_init();

    // construct the sub-objects
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        pprt = PrtList.lst + cnt;

        // blank out all the data, including the obj_base data
        BLANK_STRUCT_PTR( pprt )

        // initialize the particle's parent
        ego_object_ctor( POBJ_GET_PBASE( pprt ), pprt, BSP_LEAF_PRT, cnt );

        // initialize particle
        prt_ctor( pprt );
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_dtor( void )
{
    PRT_REF cnt;
    prt_t * pprt;

    // construct the sub-objects
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        pprt = PrtList.lst + cnt;

        // destruct the object
        prt_dtor( pprt );

        // destruct the parent
        ego_object_dtor( POBJ_GET_PBASE( pprt ) );
    }

    // initialize particle
    PrtList_init();
}

//--------------------------------------------------------------------------------------------
void PrtList_clear( void )
{
    PRT_REF cnt;

    // fix any problems with maxparticles
    maxparticles = MIN( maxparticles, MAX_PRT );

    // clear out the list
    PrtList.free_count = 0;
    PrtList.used_count = 0;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        // blank out the list
        PrtList.free_ref[cnt] = INVALID_PRT_IDX;
        PrtList.used_ref[cnt] = INVALID_PRT_IDX;

        // let the particle data know that it is not in a list
        PrtList.lst[cnt].obj_base.in_free_list = ego_false;
        PrtList.lst[cnt].obj_base.in_used_list = ego_false;
    }

    maxparticles_dirty = ego_false;
}

//--------------------------------------------------------------------------------------------
void PrtList_init( void )
{
    PRT_REF cnt;

    PrtList_clear();

    // add the objects to the free list
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        PRT_REF iprt = ( maxparticles - 1 ) - cnt;

        PrtList_add_free_ref( iprt );
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_deinit( void )
{
    PRT_REF cnt;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_config_deconstruct( PrtList_get_ptr( cnt ), 100 );
    }

    PrtList_clear();
}

//--------------------------------------------------------------------------------------------
void PrtList_reinit( void )
{
    PrtList_deinit();
    PrtList_init();
}

//--------------------------------------------------------------------------------------------
void PrtList_prune_used_list( void )
{
    // prune the used list

    int cnt;
    PRT_REF iprt;

    for ( cnt = 0; cnt < PrtList.used_count; cnt++ )
    {
        ego_bool removed = ego_false;

        iprt = ( PRT_REF )PrtList.used_ref[cnt];

        if ( !VALID_PRT_RANGE( iprt ) || !DEFINED_PRT( iprt ) )
        {
            removed = PrtList_remove_used_idx( cnt );
        }

        if ( removed && !PrtList.lst[iprt].obj_base.in_free_list )
        {
            PrtList_add_free_ref( iprt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_prune_free_list( void )
{
    // prune the free list

    int cnt;
    PRT_REF iprt;

    for ( cnt = 0; cnt < PrtList.free_count; cnt++ )
    {
        ego_bool removed = ego_false;

        iprt = ( PRT_REF )PrtList.free_ref[cnt];

        if ( VALID_PRT_RANGE( iprt ) && INGAME_PRT_BASE( iprt ) )
        {
            removed = PrtList_remove_free_idx( cnt );
        }

        if ( removed && !PrtList.lst[iprt].obj_base.in_free_list )
        {
            PrtList_push_used( iprt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_update_used( void )
{
    PRT_REF iprt;
    int cnt;

    PrtList_prune_used_list();
    PrtList_prune_free_list();

    // go through the particle list to see if there are any dangling particles
    for ( iprt = 0; iprt < MAX_PRT; iprt++ )
    {
        if ( !ALLOCATED_PRT( iprt ) ) continue;

        if ( DISPLAY_PRT( iprt ) )
        {
            if ( !PrtList.lst[iprt].obj_base.in_used_list )
            {
                PrtList_push_used( iprt );
            }
        }
        else if ( !DEFINED_PRT( iprt ) )
        {
            if ( !PrtList.lst[iprt].obj_base.in_free_list )
            {
                PrtList_add_free_ref( iprt );
            }
        }
    }

    // blank out the unused elements of the used list
    for ( cnt = PrtList.used_count; cnt < MAX_PRT; cnt++ )
    {
        PrtList.used_ref[cnt] = INVALID_PRT_IDX;
    }

    // blank out the unused elements of the free list
    for ( cnt = PrtList.free_count; cnt < MAX_PRT; cnt++ )
    {
        PrtList.free_ref[cnt] = INVALID_PRT_IDX;
    }
}

//--------------------------------------------------------------------------------------------
ego_bool PrtList_free_one( const PRT_REF iprt )
{
    /// @author ZZ
    /// @details This function sticks a particle back on the free particle stack
    ///
    /// @note Tying ALLOCATED_PRT() and POBJ_TERMINATE() to PrtList_free_one()
    /// should be enough to ensure that no particle is freed more than once

    ego_bool retval;
    prt_t * pprt;
    obj_data_t * pbase;

    if ( !ALLOCATED_PRT( iprt ) ) return ego_false;
    pprt = PrtList_get_ptr( iprt );
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
        if ( NULL == pprt ) return ego_false;

        if ( pbase->in_used_list )
        {
            PrtList_remove_used_ref( iprt );
        }

        if ( pbase->in_free_list )
        {
            retval = ego_true;
        }
        else
        {
            retval = PrtList_add_free_ref( iprt );
        }

        // particle "destructor"
        pprt = prt_config_deconstruct( pprt, 100 );
        if ( NULL == pprt ) return ego_false;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
size_t PrtList_pop_free( const int idx )
{
    /// @author ZZ
    /// @details This function returns the next free particle or MAX_PRT if there are none

    size_t retval = INVALID_PRT_REF;
    size_t loops  = 0;

    if ( idx >= 0 && idx < PrtList.free_count )
    {
        // the user has specified a valid index in the free stack
        // that they want to use. make that happen.

        // from the conditions, PrtList.free_count must be greater than 1
        size_t itop = PrtList.free_count - 1;

        // move the desired index to the top of the stack
        SWAP( size_t, PrtList.free_ref[idx], PrtList.free_ref[itop] );
    }

    // shed any values that are greater than maxparticles
    while ( PrtList.free_count > 0 )
    {
        PrtList.free_count--;
        PrtList.update_guid++;

        retval = PrtList.free_ref[PrtList.free_count];

        // completely remove it from the free list
        PrtList.free_ref[PrtList.free_count] = INVALID_PRT_IDX;

        if ( VALID_PRT_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            PrtList.lst[retval].obj_base.in_free_list = ego_false;
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
PRT_REF PrtList_allocate( const ego_bool force )
{
    /// @author ZZ
    /// @details This function gets an unused particle.  If all particles are in use
    ///    and force is set, it grabs the first unimportant one.  The iprt
    ///    index is the return value

    PRT_REF iprt;

    // Return MAX_PRT if we can't find one
    iprt = INVALID_PRT_REF;

    if ( 0 == PrtList.free_count )
    {
        if ( force )
        {
            PRT_REF found           = INVALID_PRT_REF;
            size_t  min_life        = ( size_t )( ~0 );
            PRT_REF min_life_idx    = INVALID_PRT_REF;
            size_t  min_anim     = ( size_t )( ~0 );
            PRT_REF min_anim_idx = INVALID_PRT_REF;

            // Gotta find one, so go through the list and replace a unimportant one
            for ( iprt = 0; iprt < maxparticles; iprt++ )
            {
                ego_bool was_forced = ego_false;
                prt_t * pprt;

                // Is this an invalid particle? The particle allocation count is messed up! :(
                if ( !DEFINED_PRT( iprt ) )
                {
                    found = iprt;
                    break;
                }
                pprt =  PrtList_get_ptr( iprt );

                // does it have a valid profile?
                if ( !LOADED_PIP( pprt->pip_ref ) )
                {
                    found = iprt;
                    end_one_particle_in_game( iprt );
                    break;
                }

                // do not bump another
                was_forced = TO_EGO_BOOL( PipStack.lst[pprt->pip_ref].force );

                if ( WAITING_PRT( iprt ) )
                {
                    // if the particle has been "terminated" but is still waiting around, bump it to the
                    // front of the list

                    size_t min_time  = MIN( pprt->lifetime_remaining, pprt->frames_remaining );

                    if ( min_time < MAX( min_life, min_anim ) )
                    {
                        min_life     = pprt->lifetime_remaining;
                        min_life_idx = iprt;

                        min_anim     = pprt->frames_remaining;
                        min_anim_idx = iprt;
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

                    if ( pprt->frames_remaining < min_anim )
                    {
                        min_anim     = pprt->frames_remaining;
                        min_anim_idx = iprt;
                    }
                }
            }

            if ( VALID_PRT_RANGE( found ) )
            {
                // found a "bad" particle
                iprt = found;
            }
            else if ( VALID_PRT_RANGE( min_anim_idx ) )
            {
                // found a "terminated" particle
                iprt = min_anim_idx;
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
                iprt = INVALID_PRT_REF;
            }
        }
    }
    else
    {
        if ( PrtList.free_count > ( maxparticles / 4 ) )
        {
            // Just grab the next one
            iprt = ( PRT_REF )PrtList_pop_free( -1 );
        }
        else if ( force )
        {
            iprt = ( PRT_REF )PrtList_pop_free( -1 );
        }
    }

    // return a proper value
    iprt = ( iprt >= maxparticles ) ? INVALID_PRT_REF : iprt;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // if the particle is already being used, make sure to destroy the old one
        if ( DEFINED_PRT( iprt ) )
        {
            end_one_particle_in_game( iprt );
        }

        // allocate the new one
        POBJ_ALLOCATE( PrtList_get_ptr( iprt ), REF_TO_INT( iprt ) );
    }

    if ( ALLOCATED_PRT( iprt ) )
    {
        // construct the new structure
        prt_config_construct( PrtList_get_ptr( iprt ), 100 );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
void PrtList_free_all( void )
{
    /// @author ZZ
    /// @details This function resets the particle allocation lists

    PRT_REF cnt;

    // free all the particles
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        PrtList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
int PrtList_find_free_ref( const PRT_REF iprt )
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
ego_bool PrtList_add_free_ref( const PRT_REF iprt )
{
    ego_bool retval;

    if ( !VALID_PRT_RANGE( iprt ) ) return ego_false;

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
    if ( PrtList_find_free_ref( iprt ) > 0 )
    {
        return ego_false;
    }
#endif

    EGOBOO_ASSERT( !PrtList.lst[iprt].obj_base.in_free_list );

    retval = ego_false;
    if ( PrtList.free_count < maxparticles )
    {
        PrtList.free_ref[PrtList.free_count] = iprt;

        PrtList.free_count++;
        PrtList.update_guid++;

        PrtList.lst[iprt].obj_base.in_free_list = ego_true;

        retval = ego_true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_bool PrtList_remove_free_idx( const int index )
{
    PRT_REF iprt;

    // was it found?
    if ( index < 0 || index >= PrtList.free_count ) return ego_false;

    iprt = ( PRT_REF )PrtList.free_ref[index];

    // blank out the index in the list
    PrtList.free_ref[index] = INVALID_PRT_IDX;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // let the object know it is not in the list anymore
        PrtList.lst[iprt].obj_base.in_free_list = ego_false;
    }

    // shorten the list
    PrtList.free_count--;
    PrtList.update_guid++;

    if ( PrtList.free_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, PrtList.free_ref[index], PrtList.free_ref[PrtList.free_count] );
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
int PrtList_find_used_ref( const PRT_REF iprt )
{
    /// @author BB
    /// @details if an object of index iobj exists on the used list, return the used list index
    ///     otherwise return -1

    int retval = -1, cnt;

    if ( !VALID_PRT_RANGE( iprt ) ) return retval;

    for ( cnt = 0; cnt < PrtList.used_count; cnt++ )
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
ego_bool PrtList_push_used( const PRT_REF iprt )
{
    ego_bool retval;

    if ( !VALID_PRT_RANGE( iprt ) ) return ego_false;

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
    if ( PrtList_find_used_ref( iprt ) > 0 )
    {
        return ego_false;
    }
#endif

    EGOBOO_ASSERT( !PrtList.lst[iprt].obj_base.in_used_list );

    retval = ego_false;
    if ( PrtList.used_count < maxparticles )
    {
        PrtList.used_ref[PrtList.used_count] = REF_TO_INT( iprt );

        PrtList.used_count++;
        PrtList.update_guid++;

        PrtList.lst[iprt].obj_base.in_used_list = ego_true;

        retval = ego_true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_bool PrtList_remove_used_idx( const int index )
{
    PRT_REF iprt;

    // was it found?
    if ( index < 0 || index >= PrtList.used_count ) return ego_false;

    iprt = ( PRT_REF )PrtList.used_ref[index];

    // blank out the index in the list
    PrtList.used_ref[index] = INVALID_PRT_IDX;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // let the object know it is not in the list anymore
        PrtList.lst[iprt].obj_base.in_used_list = ego_false;
    }

    // shorten the list
    PrtList.used_count--;
    PrtList.update_guid++;

    if ( PrtList.used_count > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, PrtList.used_ref[index], PrtList.used_ref[PrtList.used_count] );
    }

    return ego_true;
}

//--------------------------------------------------------------------------------------------
ego_bool PrtList_remove_used_ref( const PRT_REF iprt )
{
    // find the object in the used list
    int index = PrtList_find_used_ref( iprt );

    return PrtList_remove_used_idx( index );
}

//--------------------------------------------------------------------------------------------
void PrtList_cleanup( void )
{
    size_t  cnt;
    prt_t * pprt;

    // go through the list and activate all the particles that
    // were created while the list was iterating
    for ( cnt = 0; cnt < prt_activation_count; cnt++ )
    {
        PRT_REF iprt = prt_activation_list[cnt];

        if ( !ALLOCATED_PRT( iprt ) ) continue;
        pprt = PrtList_get_ptr( iprt );

        if ( !pprt->obj_base.turn_me_on ) continue;

        pprt->obj_base.on         = ego_true;
        pprt->obj_base.turn_me_on = ego_false;
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
ego_bool PrtList_add_activation( const PRT_REF iprt )
{
    // put this particle into the activation list so that it can be activated right after
    // the PrtList loop is completed

    ego_bool retval = ego_false;

    if ( !VALID_PRT_RANGE( iprt ) ) return ego_false;

    if ( prt_activation_count < MAX_PRT )
    {
        prt_activation_list[prt_activation_count] = iprt;
        prt_activation_count++;

        retval = ego_true;
    }

    PrtList.lst[iprt].obj_base.turn_me_on = ego_true;

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_bool PrtList_add_termination( const PRT_REF iprt )
{
    ego_bool retval = ego_false;

    if ( !VALID_PRT_RANGE( iprt ) ) return ego_false;

    if ( prt_termination_count < MAX_PRT )
    {
        prt_termination_list[prt_termination_count] = iprt;
        prt_termination_count++;

        retval = ego_true;
    }

    // at least mark the object as "waiting to be terminated"
    POBJ_REQUEST_TERMINATE( PrtList_get_ptr( iprt ) );

    return retval;
}

//--------------------------------------------------------------------------------------------
int PrtList_count_free( void )
{
    return PrtList.free_count;
}

//--------------------------------------------------------------------------------------------
void PrtList_reset_all( void )
{
    /// @author ZZ
    /// @details This resets all particle data and reads in the coin and water particles

    const char *loadpath;

    release_all_local_pips();
    PipStack_release_all();

    // Load in the standard global particles ( the coins for example )
    loadpath = "mp_data/1money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_COIN1 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/5money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_COIN5 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/25money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_COIN25 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/100money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_COIN100 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/200money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_GEM200 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/500money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_GEM500 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/1000money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_GEM1000 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/2000money.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_GEM2000 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // Load module specific information
    loadpath = "mp_data/weather4.txt";
    PipStack_load_one( loadpath, ( PIP_REF )PIP_WEATHER );              //It's okay if weather particles fail

    loadpath = "mp_data/weather5.txt";
    PipStack_load_one( loadpath, ( PIP_REF )PIP_WEATHER_FINISH );

    loadpath = "mp_data/splash.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_SPLASH ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/ripple.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_RIPPLE ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // This is also global...
    loadpath = "mp_data/defend.txt";
    if ( INVALID_PIP_REF == PipStack_load_one( loadpath, ( PIP_REF )PIP_DEFEND ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    PipStack.count = GLOBAL_PIP_COUNT;
}

//--------------------------------------------------------------------------------------------
ego_bool PrtList_request_terminate( const PRT_REF iprt )
{
    prt_t * pprt = PrtList_get_ptr( iprt );

    return prt_request_terminate( pprt );
}

//--------------------------------------------------------------------------------------------
ego_bool PrtList_remove_free_ref( const PRT_REF iprt )
{
    // find the object in the free list
    int index = PrtList_find_free_ref( iprt );

    return PrtList_remove_free_idx( index );
}
