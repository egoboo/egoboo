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

/// @file  game/PrtList.c
/// @brief Implementation of the PrtList_* functions
/// @details

#include "game/PrtList.h"
#include "game/egoboo_object.h"
#include "game/particle.h"

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

// macros without range checking
#define INGAME_PPRT_BASE_RAW(PPRT)      ( ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) ) && ON_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define DEFINED_PPRT_RAW( PPRT )        ( ALLOCATED_PBASE ( POBJ_GET_PBASE(PPRT) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PPRT) ) )
#define ALLOCATED_PPRT_RAW( PPRT )      ALLOCATED_PBASE( POBJ_GET_PBASE(PPRT) )
#define ACTIVE_PPRT_RAW( PPRT )         ACTIVE_PBASE( POBJ_GET_PBASE(PPRT) )
#define WAITING_PPRT_RAW( PPRT )        WAITING_PBASE   ( POBJ_GET_PBASE(PPRT) )
#define TERMINATED_PPRT_RAW( PPRT )     TERMINATED_PBASE( POBJ_GET_PBASE(PPRT) )

//--------------------------------------------------------------------------------------------
//Inline
//--------------------------------------------------------------------------------------------

bool VALID_PRT_RANGE(const PRT_REF IPRT) { return (IPRT < std::min<size_t>(PrtList.maxparticles, MAX_PRT)); }
bool DEFINED_PRT(const PRT_REF IPRT) { return (VALID_PRT_RANGE(IPRT) && DEFINED_PPRT_RAW(PrtList.lst + (IPRT))); }
bool ALLOCATED_PRT(const PRT_REF IPRT) { return (VALID_PRT_RANGE(IPRT) && ALLOCATED_PPRT_RAW(PrtList.lst + (IPRT))); }
bool ACTIVE_PRT(const PRT_REF IPRT) { return (VALID_PRT_RANGE(IPRT) && ACTIVE_PPRT_RAW(PrtList.lst + (IPRT))); }
bool WAITING_PRT(const PRT_REF IPRT) { return (VALID_PRT_RANGE(IPRT) && WAITING_PPRT_RAW(PrtList.lst + (IPRT))); }
bool TERMINATED_PRT(const PRT_REF IPRT) { return (VALID_PRT_RANGE(IPRT) && TERMINATED_PPRT_RAW(PrtList.lst + (IPRT))); }
size_t GET_INDEX_PPRT(const prt_t *PPRT) { return LAMBDA(NULL == (PPRT), INVALID_PRT_IDX, (size_t)GET_INDEX_POBJ(PPRT, INVALID_PRT_IDX)); }
PRT_REF GET_REF_PPRT(const prt_t *PPRT) { return ((PRT_REF)GET_INDEX_PPRT(PPRT)); }
bool  VALID_PRT_PTR(const prt_t *PPRT) { return ((NULL != (PPRT)) && VALID_PRT_RANGE(GET_REF_POBJ(PPRT, INVALID_PRT_REF))); }
bool  DEFINED_PPRT(const prt_t *PPRT) { return (VALID_PRT_PTR(PPRT) && DEFINED_PPRT_RAW(PPRT)); }
bool ALLOCATED_PPRT(const prt_t *PPRT) { return (VALID_PRT_PTR(PPRT) && ALLOCATED_PPRT_RAW(PPRT)); }
bool ACTIVE_PPRT(const prt_t *PPRT) { return (VALID_PRT_PTR(PPRT) && ACTIVE_PPRT_RAW(PPRT)); }
bool WAITING_PPRT(const prt_t *PPRT) { return (VALID_PRT_PTR(PPRT) && WAITING_PPRT_RAW(PPRT)); }
bool TERMINATED_PPRT(const prt_t *PPRT) { return (VALID_PRT_PTR(PPRT) && TERMINATED_PPRT_RAW(PPRT)); }
bool INGAME_PRT_BASE(const PRT_REF IPRT) { return (VALID_PRT_RANGE(IPRT) && INGAME_PPRT_BASE_RAW(PrtList.lst + (IPRT))); }
bool INGAME_PPRT_BASE(const prt_t *PPRT) { return (VALID_PRT_PTR(PPRT) && INGAME_PPRT_BASE_RAW(PPRT)); }
bool DISPLAY_PRT(const PRT_REF IPRT) { return INGAME_PRT_BASE(IPRT); }
bool DISPLAY_PPRT(const prt_t *PPRT) { return INGAME_PPRT_BASE(PPRT); }
bool INGAME_PRT(const PRT_REF IPRT) { return LAMBDA(Ego::Entities::spawnDepth > 0, DEFINED_PRT(IPRT), INGAME_PRT_BASE(IPRT) && (!PrtList.lst[IPRT].is_ghost)); }
bool INGAME_PPRT(const prt_t *PPRT) { return LAMBDA(Ego::Entities::spawnDepth > 0, INGAME_PPRT_BASE(PPRT), DISPLAY_PPRT(PPRT) && (!(PPRT)->is_ghost)); }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

ParticleManager PrtList;

static int PrtList_find_free_ref(const PRT_REF);
static bool PrtList_push_free(const PRT_REF);
static size_t PrtList_pop_free(const int);
static int PrtList_find_used_ref(const PRT_REF);
static size_t PrtList_pop_used(const int);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void    PrtList_clear();
static void    PrtList_init();
static void    PrtList_deinit();

static bool  PrtList_add_free_ref( const PRT_REF iprt );
static bool  PrtList_remove_free_ref( const PRT_REF iprt );
static bool  PrtList_remove_free_idx( const int index );

static bool  PrtList_remove_used_ref( const PRT_REF iprt );
static bool  PrtList_remove_used_idx( const int index );

static void    PrtList_prune_used_list();
static void    PrtList_prune_free_list();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ParticleManager::ctor()
{
    PRT_REF cnt;
    prt_t * pprt;

    // initialize the list
    PrtList_init();

    // construct the sub-objects
    for ( cnt = 0; cnt < PrtList.maxparticles; cnt++ )
    {
        pprt = PrtList.lst + cnt;

        // blank out all the data, including the obj_base data
        BLANK_STRUCT_PTR( pprt )

        // initialize the particle's parent
        Ego::Entity::ctor( POBJ_GET_PBASE( pprt ), pprt, BSP_LEAF_PRT, cnt );

        // initialize particle
        prt_t::ctor( pprt );
    }
}

//--------------------------------------------------------------------------------------------
void ParticleManager::dtor()
{
    PRT_REF cnt;
    prt_t * pprt;

    // construct the sub-objects
    for ( cnt = 0; cnt < PrtList.maxparticles; cnt++ )
    {
        pprt = PrtList.lst + cnt;

        // destruct the object
        prt_t::dtor( pprt );

        // destruct the parent
        Ego::Entity::dtor( POBJ_GET_PBASE( pprt ) );
    }

    // initialize particle
    PrtList_init();
}

//--------------------------------------------------------------------------------------------
void PrtList_clear()
{
    // fix any problems with maxparticles
    PrtList.maxparticles = std::min( PrtList.maxparticles, (size_t)MAX_PRT );

    // clear out the list
    PrtList.freeCount = 0;
    PrtList.usedCount = 0;
    for (PRT_REF cnt = 0; cnt < PrtList.maxparticles; cnt++)
    {
        // blank out the list
        PrtList.free_ref[cnt] = INVALID_PRT_IDX;
        PrtList.used_ref[cnt] = INVALID_PRT_IDX;

        // let the particle data know that it is not in a list
        PrtList.lst[cnt].obj_base.in_free_list = false;
        PrtList.lst[cnt].obj_base.in_used_list = false;
    }

    PrtList.maxparticles_dirty = false;
}

//--------------------------------------------------------------------------------------------
void PrtList_init()
{
    PRT_REF cnt;

    PrtList_clear();

    // add the objects to the free list
    for ( cnt = 0; cnt < PrtList.maxparticles; cnt++ )
    {
        PRT_REF iprt = ( PrtList.maxparticles - 1 ) - cnt;

        PrtList_add_free_ref( iprt );
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_deinit()
{
    PRT_REF cnt;

    for ( cnt = 0; cnt < PrtList.maxparticles; cnt++ )
    {
        prt_config_deconstruct( PrtList.get_ptr( cnt ), 100 );
    }

    PrtList_clear();
}

//--------------------------------------------------------------------------------------------
void ParticleManager::reinit()
{
    PrtList_deinit();
    PrtList_init();
}

//--------------------------------------------------------------------------------------------
void PrtList_prune_used_list()
{
    // prune the used list

    int cnt;
    PRT_REF iprt;

    for ( cnt = 0; cnt < PrtList.usedCount; cnt++ )
    {
        bool removed = false;

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
void PrtList_prune_free_list()
{
    // prune the free list

    int cnt;
    PRT_REF iprt;

    for ( cnt = 0; cnt < PrtList.freeCount; cnt++ )
    {
        bool removed = false;

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
void ParticleManager::update_used()
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
    for ( cnt = PrtList.usedCount; cnt < MAX_PRT; cnt++ )
    {
        PrtList.used_ref[cnt] = INVALID_PRT_IDX;
    }

    // blank out the unused elements of the free list
    for ( cnt = PrtList.freeCount; cnt < MAX_PRT; cnt++ )
    {
        PrtList.free_ref[cnt] = INVALID_PRT_IDX;
    }
}

//--------------------------------------------------------------------------------------------
bool PrtList_free_one( const PRT_REF iprt )
{
    /// @author ZZ
    /// @details This function sticks a particle back on the free particle stack
    ///
    /// @note Tying ALLOCATED_PRT() and POBJ_TERMINATE() to PrtList_free_one()
    /// should be enough to ensure that no particle is freed more than once

    bool retval;
    prt_t * pprt;
    Ego::Entity * pbase;

    if ( !ALLOCATED_PRT( iprt ) ) return false;
    pprt = PrtList.get_ptr( iprt );
    pbase = POBJ_GET_PBASE( pprt );

    // if we are inside a PrtList loop, do not actually change the length of the
    // list. This will cause some problems later.
    if ( PrtList.getLockCount() > 0 )
    {
        retval = PrtList.add_termination( iprt );
    }
    else
    {
        // deallocate any dynamically allocated memory
        pprt = prt_config_deinitialize( pprt, 100 );
        if ( NULL == pprt ) return false;

        if ( pbase->in_used_list )
        {
            PrtList_remove_used_ref( iprt );
        }

        if ( pbase->in_free_list )
        {
            retval = true;
        }
        else
        {
            retval = PrtList_add_free_ref( iprt );
        }

        // particle "destructor"
        pprt = prt_config_deconstruct( pprt, 100 );
        if ( NULL == pprt ) return false;
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

    if ( idx >= 0 && idx < PrtList.freeCount )
    {
        // the user has specified a valid index in the free stack
        // that they want to use. make that happen.

        // from the conditions, PrtList.free_count must be greater than 1
        size_t itop = PrtList.freeCount - 1;

        // move the desired index to the top of the stack
        SWAP( size_t, PrtList.free_ref[idx], PrtList.free_ref[itop] );
    }

    // shed any values that are greater than maxparticles
    while ( PrtList.freeCount > 0 )
    {
        PrtList.freeCount--;
        PrtList.update_guid++;

        retval = PrtList.free_ref[PrtList.freeCount];

        // completely remove it from the free list
        PrtList.free_ref[PrtList.freeCount] = INVALID_PRT_IDX;

        if ( VALID_PRT_RANGE( retval ) )
        {
            // let the object know it is not in the free list any more
            PrtList.lst[retval].obj_base.in_free_list = false;
            break;
        }

        loops++;
    }

    if ( loops > 0 )
    {
        log_warning( "%s - there is something wrong with the free stack. %lu loops.\n", __FUNCTION__, loops );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
PRT_REF ParticleManager::allocate(const bool force)
{
    PRT_REF iprt;

    // Return MAX_PRT if we can't find one.
    iprt = INVALID_PRT_REF;

    if (0 == PrtList.freeCount)
    {
        if (force)
        {
            PRT_REF found        = INVALID_PRT_REF;
            size_t  min_life     = ( size_t )( ~0 );
            PRT_REF min_life_idx = INVALID_PRT_REF;
            size_t  min_anim     = ( size_t )( ~0 );
            PRT_REF min_anim_idx = INVALID_PRT_REF;

            // Gotta find one, so go through the list and replace a unimportant one.
            for (iprt = 0; iprt < PrtList.maxparticles; iprt++)
            {
                bool was_forced = false;
                prt_t *pprt;

                // Is this an invalid particle? The particle allocation count is messed up! :(
                if (!DEFINED_PRT(iprt))
                {
                    found = iprt;
                    break;
                }
                pprt =  PrtList.get_ptr(iprt);

                // Does it have a valid profile?
                if (!LOADED_PIP(pprt->pip_ref))
                {
                    found = iprt;
                    end_one_particle_in_game(iprt);
                    break;
                }

                // Do not bump another.
                was_forced = TO_C_BOOL(PipStack.lst[pprt->pip_ref].force);

                if (WAITING_PRT(iprt))
                {
                    // If the particle has been "terminated" but is still waiting around,
                    // bump it to the front of the list.

                    size_t min_time = std::min( pprt->lifetime_remaining, pprt->frames_remaining );

                    if ( min_time < std::max( min_life, min_anim ) )
                    {
                        min_life     = pprt->lifetime_remaining;
                        min_life_idx = iprt;

                        min_anim     = pprt->frames_remaining;
                        min_anim_idx = iprt;
                    }
                }
                else if (!was_forced)
                {
                    // If the particle has not yet died, let choose the worst one.
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

            if (VALID_PRT_RANGE(found))
            {
                // Found a "bad" particle.
                iprt = found;
            }
            else if (VALID_PRT_RANGE(min_anim_idx))
            {
                // Found a "terminated" particle.
                iprt = min_anim_idx;
            }
            else if (VALID_PRT_RANGE(min_life_idx))
            {
                // Found a particle that closest to death.
                iprt = min_life_idx;
            }
            else
            {
                // Found nothing. This should only happen if all the particles are forced.
                iprt = INVALID_PRT_REF;
            }
        }
    }
    else
    {
        if ( PrtList.freeCount > ( PrtList.maxparticles / 4 ) )
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
    iprt = ( iprt >= PrtList.maxparticles ) ? INVALID_PRT_REF : iprt;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // if the particle is already being used, make sure to destroy the old one
        if ( DEFINED_PRT( iprt ) )
        {
            end_one_particle_in_game( iprt );
        }

        // allocate the new one
        POBJ_ALLOCATE( PrtList.get_ptr( iprt ), REF_TO_INT( iprt ) );
    }

    if ( ALLOCATED_PRT( iprt ) )
    {
        // construct the new structure
        prt_config_construct( PrtList.get_ptr( iprt ), 100 );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
void PrtList_free_all()
{
    /// @author ZZ
    /// @details This function resets the particle allocation lists

    // free all the particles
    for (PRT_REF cnt = 0; cnt < PrtList.maxparticles; cnt++ )
    {
        PrtList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
int PrtList_find_free_ref( const PRT_REF iprt )
{
    int retval = -1, cnt;

    if ( !VALID_PRT_RANGE( iprt ) ) return retval;

    for ( cnt = 0; cnt < PrtList.freeCount; cnt++ )
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
bool PrtList_add_free_ref( const PRT_REF iprt )
{
    bool retval;

    if ( !VALID_PRT_RANGE( iprt ) ) return false;

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
    if ( PrtList_find_free_ref( iprt ) > 0 )
    {
        return false;
    }
#endif

    EGOBOO_ASSERT( !PrtList.lst[iprt].obj_base.in_free_list );

    retval = false;
    if ( PrtList.freeCount < PrtList.maxparticles )
    {
        PrtList.free_ref[PrtList.freeCount] = iprt;

        PrtList.freeCount++;
        PrtList.update_guid++;

        PrtList.lst[iprt].obj_base.in_free_list = true;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool PrtList_remove_free_idx( const int index )
{
    PRT_REF iprt;

    // was it found?
    if ( index < 0 || index >= PrtList.freeCount ) return false;

    iprt = ( PRT_REF )PrtList.free_ref[index];

    // blank out the index in the list
    PrtList.free_ref[index] = INVALID_PRT_IDX;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // let the object know it is not in the list anymore
        PrtList.lst[iprt].obj_base.in_free_list = false;
    }

    // shorten the list
    PrtList.freeCount--;
    PrtList.update_guid++;

    if ( PrtList.freeCount > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, PrtList.free_ref[index], PrtList.free_ref[PrtList.freeCount] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
int PrtList_find_used_ref( const PRT_REF iprt )
{
    /// @author BB
    /// @details if an object of index iobj exists on the used list, return the used list index
    ///     otherwise return -1

    int retval = -1, cnt;

    if ( !VALID_PRT_RANGE( iprt ) ) return retval;

    for ( cnt = 0; cnt < PrtList.usedCount; cnt++ )
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
bool PrtList_push_used( const PRT_REF iprt )
{
    bool retval;

    if ( !VALID_PRT_RANGE( iprt ) ) return false;

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
    if ( PrtList_find_used_ref( iprt ) > 0 )
    {
        return false;
    }
#endif

    EGOBOO_ASSERT( !PrtList.lst[iprt].obj_base.in_used_list );

    retval = false;
    if ( PrtList.usedCount < PrtList.maxparticles )
    {
        PrtList.used_ref[PrtList.usedCount] = REF_TO_INT( iprt );

        PrtList.usedCount++;
        PrtList.update_guid++;

        PrtList.lst[iprt].obj_base.in_used_list = true;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool PrtList_remove_used_idx( const int index )
{
    PRT_REF iprt;

    // was it found?
    if ( index < 0 || index >= PrtList.usedCount ) return false;

    iprt = ( PRT_REF )PrtList.used_ref[index];

    // blank out the index in the list
    PrtList.used_ref[index] = INVALID_PRT_IDX;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        // let the object know it is not in the list anymore
        PrtList.lst[iprt].obj_base.in_used_list = false;
    }

    // shorten the list
    PrtList.usedCount--;
    PrtList.update_guid++;

    if ( PrtList.usedCount > 0 )
    {
        // swap the last element for the deleted element
        SWAP( size_t, PrtList.used_ref[index], PrtList.used_ref[PrtList.usedCount] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool PrtList_remove_used_ref( const PRT_REF iprt )
{
    // find the object in the used list
    int index = PrtList_find_used_ref( iprt );

    return PrtList_remove_used_idx( index );
}

//--------------------------------------------------------------------------------------------
void ParticleManager::maybeRunDeferred()
{
    // Go through the list and activate all the particles that
    // were created while the list was iterating.
    for (size_t i = 0; i < activation_count; i++)
    {
        PRT_REF iprt = activation_list[i];

        if (!ALLOCATED_PRT(iprt)) continue;
        prt_t *pprt = get_ptr(iprt);

        if (!pprt->obj_base.turn_me_on) continue;

        pprt->obj_base.on         = true;
        pprt->obj_base.turn_me_on = false;
    }
    activation_count = 0;

    // Go through and delete any particles that
    // were supposed to be deleted while the list was iterating.
    for (size_t i = 0; i < termination_count; i++ )
    {
        PrtList_free_one(termination_list[i]);
    }
    termination_count = 0;
}

//--------------------------------------------------------------------------------------------
bool ParticleManager::add_activation( const PRT_REF iprt )
{
    bool retval = false;

    if ( !VALID_PRT_RANGE( iprt ) ) return false;

    if ( PrtList.activation_count < MAX_PRT )
    {
        PrtList.activation_list[PrtList.activation_count] = iprt;
        PrtList.activation_count++;

        retval = true;
    }

    PrtList.lst[iprt].obj_base.turn_me_on = true;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ParticleManager::add_termination( const PRT_REF iprt )
{
    bool retval = false;

    if (!VALID_PRT_RANGE(iprt)) return false;

    if (PrtList.termination_count < MAX_PRT)
    {
        PrtList.termination_list[PrtList.termination_count] = iprt;
        PrtList.termination_count++;

        retval = true;
    }

    // Mrk the object as "waiting to be terminated".
    POBJ_REQUEST_TERMINATE(PrtList.get_ptr(iprt));

    return retval;
}

//--------------------------------------------------------------------------------------------
int PrtList_count_free()
{
    return PrtList.freeCount;
}

//--------------------------------------------------------------------------------------------
void ParticleManager::reset_all()
{
    /// @author ZZ
    /// @details This resets all particle data and reads in the coin and water particles

    const char *loadpath;

    //release_all_local_pips();
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
bool PrtList_request_terminate( const PRT_REF iprt )
{
    prt_t * pprt = PrtList.get_ptr( iprt );

    return prt_t::request_terminate( pprt );
}

//--------------------------------------------------------------------------------------------
bool PrtList_remove_free_ref( const PRT_REF iprt )
{
    // find the object in the free list
    int index = PrtList_find_free_ref( iprt );

    return PrtList_remove_free_idx( index );
}
