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

/// @file particle.c
/// @brief Manages particle systems.

#include "particle.h"
#include "enchant.h"
#include "char.h"
#include "mad.h"
#include "profile.h"

#include "log.h"
#include "sound.h"
#include "camera.h"
#include "mesh.h"
#include "game.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float            sprite_list_u[MAXPARTICLEIMAGE][2];        // Texture coordinates
float            sprite_list_v[MAXPARTICLEIMAGE][2];

Uint16           maxparticles = 512;                            // max number of particles

DECLARE_STACK( ACCESS_TYPE_NONE, pip_t, PipStack );
DECLARE_LIST( ACCESS_TYPE_NONE, prt_t, PrtList );

static const Uint32  particletrans = 0x80;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void prt_init( prt_t * pprt );

static void   PrtList_init();
static Uint16 PrtList_get_free();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_count_free()
{
    return PrtList.free_count;
}

//--------------------------------------------------------------------------------------------
void PrtList_init()
{
    int cnt;

    // free all the particles
    PrtList.free_count = 0;
    for ( cnt = 0; cnt < maxparticles && cnt < TOTAL_MAX_PRT; cnt++ )
    {
        prt_t * pprt = PrtList.lst + cnt;

        // blank out all the data, including the obj_base data
        memset( pprt, 0, sizeof( *pprt ) );

        prt_init( pprt );

        PrtList.free_ref[PrtList.free_count] = PrtList.free_count;
        PrtList.free_count++;
    }
}

//--------------------------------------------------------------------------------------------
void PrtList_update_used()
{
    int cnt;

    PrtList.used_count = 0;
    for ( cnt = 0; cnt < TOTAL_MAX_PRT; cnt++ )
    {
        if ( !DISPLAY_PRT( cnt ) ) continue;

        PrtList.used_ref[PrtList.used_count] = cnt;
        PrtList.used_count++;
    }

    for ( cnt = PrtList.used_count; cnt < TOTAL_MAX_PRT; cnt++ )
    {
        PrtList.used_ref[PrtList.used_count] = TOTAL_MAX_PRT;
    }
}

//--------------------------------------------------------------------------------------------
bool_t PrtList_free_one( Uint16 iprt )
{
    /// @details ZZ@> This function sticks a particle back on the free particle stack
    ///
    /// @note Tying ALLOCATED_PRT() and EGO_OBJECT_TERMINATE() to PrtList_free_one()
    /// should be enough to ensure that no particle is freed more than once
    bool_t retval;
    prt_t * pprt;

    if ( !ALLOCATED_PRT( iprt ) ) return bfalse;
    pprt = PrtList.lst + iprt;

    // particle "destructor"
    // sets all boolean values to false, incluting the "on" flag
    prt_init( pprt );

#if defined(USE_DEBUG) && defined(DEBUG_PRT_LIST)
    {
        int cnt;
        // determine whether this particle is already in the list of free textures
        // that is an error
        for ( cnt = 0; cnt < PrtList.free_count; cnt++ )
        {
            if ( iprt == PrtList.free_ref[cnt] )
            {
                return bfalse;
            }
        }
    }
#endif

    // push it on the free stack
    retval = bfalse;
    if ( PrtList.free_count < TOTAL_MAX_PRT )
    {
        PrtList.free_ref[PrtList.free_count] = iprt;
        PrtList.free_count++;
        retval = btrue;
    }

    EGO_OBJECT_TERMINATE( pprt );

    return retval;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( Uint16 particle, Sint8 sound )
{
    /// ZZ@> This function plays a sound effect for a particle

    prt_t * pprt;

    if ( !ALLOCATED_PRT( particle ) ) return;
    pprt = PrtList.lst + particle;

    if ( !VALID_SND( sound ) ) return;

    if ( LOADED_PRO( pprt->profile_ref ) )
    {
        sound_play_chunk( pprt->pos, pro_get_chunk( pprt->profile_ref, sound ) );
    }
    else
    {
        sound_play_chunk( pprt->pos, g_wavelist[sound] );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_in_game( Uint16 particle )
{
    /// @details ZZ@> This function sticks a particle back on the free particle stack and
    ///    plays the sound associated with the particle
    ///
    /// @note BB@> Use prt_request_terminate() instead of calling this function directly.
    ///            Requesting termination will defer the actual deletion of a particle until
    ///            it is finally destroyed by cleanup_all_particles()

    Uint16 child;
    prt_t * pprt;

    if ( !ALLOCATED_PRT( particle ) ) return;
    pprt = PrtList.lst + particle;

    if ( pprt->spawncharacterstate )
    {
        child = spawn_one_character( pprt->pos, pprt->profile_ref, pprt->team, 0, pprt->facing, NULL, MAX_CHR );
        if ( ACTIVE_CHR( child ) )
        {
            chr_get_pai( child )->state = pprt->spawncharacterstate;
            chr_get_pai( child )->owner = pprt->owner_ref;
        }
    }

    if ( LOADED_PIP( pprt->pip_ref ) )
    {
        play_particle_sound( particle, PipStack.lst[pprt->pip_ref].soundend );
    }

    PrtList_free_one( particle );
}

//--------------------------------------------------------------------------------------------
Uint16 PrtList_get_free()
{
    /// @details ZZ@> This function returns the next free particle or TOTAL_MAX_PRT if there are none

    Uint16 retval = TOTAL_MAX_PRT;

    if ( PrtList.free_count > 0 )
    {
        PrtList.free_count--;
        retval = PrtList.free_ref[PrtList.free_count];
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int prt_get_free( int force )
{
    /// @details ZZ@> This function gets an unused particle.  If all particles are in use
    ///    and force is set, it grabs the first unimportant one.  The iprt
    ///    index is the return value

    int iprt;

    // Return TOTAL_MAX_PRT if we can't find one
    iprt = TOTAL_MAX_PRT;

    if ( 0 == PrtList.free_count )
    {
        if ( force )
        {
            int found = TOTAL_MAX_PRT;
            int min_life = 65535, min_life_idx = TOTAL_MAX_PRT;
            int min_display = 65535, min_display_idx = TOTAL_MAX_PRT;

            // Gotta find one, so go through the list and replace a unimportant one
            for ( iprt = 0; iprt < maxparticles; iprt++ )
            {
                bool_t was_forced = bfalse;
                prt_t * pprt;

                // Is this an invalid particle? The particle allocation count is messed up! :(
                if ( !ALLOCATED_PRT( iprt ) )
                {
                    found = iprt;
                    break;
                }
                pprt = PrtList.lst + iprt;

                // does it have a valid profile?
                if ( !LOADED_PIP( pprt->pip_ref ) )
                {
                    found = iprt;
                    free_one_particle_in_game( iprt );
                    break;
                }

                // do not bump another
                was_forced = ( PipStack.lst[pprt->pip_ref].force );

                if ( WAITING_PRT( iprt ) )
                {
                    // if the particle has been "terminated" but is still waiting around, bump it to the
                    // front of the list

                    int lifetime  = pprt->time_update - update_wld;
                    int frametime = pprt->time_frame  - frame_all;
                    int min_time = MIN( lifetime, frametime );

                    if ( min_time < MAX( min_life, min_display ) )
                    {
                        min_life     = lifetime;
                        min_life_idx = iprt;

                        min_display     = frametime;
                        min_display_idx = iprt;
                    }
                }
                else if ( !was_forced )
                {
                    int lifetime, frametime;
                    // if the particle has not yet died, let choose the worst one

                    lifetime = pprt->time_update - update_wld;
                    if ( lifetime < min_life )
                    {
                        min_life     = lifetime;
                        min_life_idx = iprt;
                    }

                    frametime = pprt->time_frame - frame_all;
                    if ( frametime < min_display )
                    {
                        min_display     = frametime;
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
                iprt = TOTAL_MAX_PRT;
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
    iprt = ( iprt >= maxparticles ) ? TOTAL_MAX_PRT : iprt;

    if ( VALID_PRT_RANGE( iprt ) )
    {
        if ( ALLOCATED_PRT( iprt ) && !TERMINATED_PRT( iprt ) )
        {
            free_one_particle_in_game( iprt );
        }

        EGO_OBJECT_ALLOCATE( PrtList.lst + iprt, iprt );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
void prt_init( prt_t * pprt )
{
    /// BB@> Set all particle parameters to safe values.
    ///      @details The c equivalent of the particle prt::new() function.

    ego_object_base_t save_base;

    if ( NULL == pprt ) return;

    // save the base object data
    memcpy( &save_base, OBJ_GET_PBASE( pprt ), sizeof( ego_object_base_t ) );

    memset( pprt, 0, sizeof( *pprt ) );

    // restore the base object data
    memcpy( OBJ_GET_PBASE( pprt ), &save_base, sizeof( ego_object_base_t ) );

    // "no lifetime" = "eternal"
    pprt->time_update  = ( Uint32 )~0;
    pprt->time_frame   = ( Uint32 )~0;

    pprt->pip_ref      = MAX_PIP;
    pprt->profile_ref  = MAX_PROFILE;

    pprt->attachedto_ref = MAX_CHR;
    pprt->owner_ref      = MAX_CHR;
    pprt->target_ref     = MAX_CHR;
    pprt->parent_ref     = TOTAL_MAX_PRT;
    pprt->parent_guid    = 0xFFFFFFFF;

    pprt->onwhichplatform = MAX_CHR;
}

//--------------------------------------------------------------------------------------------
Uint16 prt_get_iowner( Uint16 iprt, int depth )
{
    /// BB@> A helper function for determining the owner of a paricle
    ///
    ///      @details There could be a possibility that a particle exists that was spawned by
    ///      another particle, but has lost contact with its original spawner. For instance
    ///      If an explosion particle bounces off of something with MISSILE_DEFLECT or
    ///      MISSILE_REFLECT, which subsequently dies before the particle...
    ///
    ///      That is actually pretty far fetched, but at some point it might make sense to
    ///      spawn particles just keeping track of the spawner (whether particle or character)
    ///      and working backward to any potential owner using this function. ;)
    ///
    ///      @note this function should be completely trivial for anything other than
    ///       namage particles created by an explosion

    Uint16 iowner = MAX_CHR;

    prt_t * pprt;

    // be careful because this can be recursive
    if ( depth > maxparticles - PrtList.free_count )
        return MAX_CHR;

    if ( !ACTIVE_PRT( iprt ) ) return MAX_CHR;
    pprt = PrtList.lst + iprt;

    if ( ACTIVE_CHR( pprt->owner_ref ) )
    {
        iowner = pprt->owner_ref;
    }
    else
    {
        // make a check for a stupid looping structure...
        // cannot be sure you could never get a loop, though

        if ( !ALLOCATED_PRT( pprt->parent_ref ) )
        {
            // make sure that a non valid parent_ref is marked as non-valid
            pprt->parent_ref = TOTAL_MAX_PRT;
            pprt->parent_guid = 0xFFFFFFFF;
        }
        else
        {
            // if a particle has been poofed, and another particle lives at that address,
            // it is possible that the pprt->parent_ref points to a valid particle that is
            // not the parent. Depending on how scrambled the list gets, there could actually
            // be looping structures. I have actually seen this, so don't laugh :)

            if ( PrtList.lst[pprt->parent_ref].obj_base.guid == pprt->parent_guid )
            {
                if ( iprt != pprt->parent_ref )
                {
                    iowner = prt_get_iowner( pprt->parent_ref, depth + 1 );
                }
            }
            else
            {
                // the parent particle doesn't exist anymore
                // fix the reference
                pprt->parent_ref = TOTAL_MAX_PRT;
                pprt->parent_guid = 0xFFFFFFFF;
            }
        }
    }

    return iowner;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle( fvec3_t   pos, Uint16 facing, Uint16 iprofile, Uint16 ipip,
                           Uint16 chr_attach, Uint16 vrt_offset, Uint8 team,
                           Uint16 chr_origin, Uint16 prt_origin, Uint16 multispawn, Uint16 oldtarget )
{
    /// @details ZZ@> This function spawns a new particle.
    ///               Returns the index of that particle or TOTAL_MAX_PRT on a failure.

    int iprt, velocity;
    fvec3_t   vel;
    float tvel;
    int offsetfacing = 0, newrand;
    prt_t * pprt;
    pip_t * ppip;
    Uint32 prt_lifetime;
    fvec3_t   tmp_pos;
    Uint16 turn;

    // Convert from local ipip to global ipip
    if ( LOADED_PRO( iprofile ) && ipip < MAX_PIP_PER_PROFILE )
    {
        ipip = pro_get_ipip( iprofile, ipip );
    }

    if ( !LOADED_PIP( ipip ) )
    {
        log_debug( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                   ipip, chr_origin, ACTIVE_CHR( chr_origin ) ? ChrList.lst[chr_origin].Name : "INVALID",
                   iprofile, LOADED_PRO( iprofile ) ? ProList.lst[iprofile].name : "INVALID" );

        return TOTAL_MAX_PRT;
    }
    ppip = PipStack.lst + ipip;

    // count all the requests for this particle type
    ppip->prt_request_count++;

    iprt = prt_get_free( ppip->force );
    if ( !ALLOCATED_PRT( iprt ) )
    {
#if defined(USE_DEBUG) && defined(DEBUG_PRT_LIST)
        log_debug( "spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
                   chr_origin, ACTIVE_CHR( chr_origin ) ? ChrList.lst[chr_origin].Name : "INVALID",
                   ipip, LOADED_PIP( ipip ) ? PipStack.lst[ipip].name : "INVALID",
                   iprofile, LOADED_PRO( iprofile ) ? ProList.lst[iprofile].name : "INVALID" );
#endif

        return TOTAL_MAX_PRT;
    }
    pprt = PrtList.lst + iprt;

    // clear out all data
    prt_init( pprt );

    tmp_pos = pos;

    // Necessary data for any part
    EGO_OBJECT_ACTIVATE( pprt, ppip->name );

    // try to get an idea of who our owner is even if we are
    // given bogus info
    if ( !ACTIVE_CHR( chr_origin ) && ACTIVE_PRT( prt_origin ) )
    {
        chr_origin = prt_get_iowner( prt_origin, 0 );
    }

    pprt->pip_ref     = ipip;
    pprt->profile_ref = iprofile;
    pprt->team        = team;
    pprt->owner_ref   = chr_origin;
    pprt->parent_ref  = prt_origin;
    pprt->parent_guid = ALLOCATED_PRT( prt_origin ) ? PrtList.lst[prt_origin].obj_base.guid : ( ~0 );
    pprt->damagetype  = ppip->damagetype;

    // Lighting and sound
    pprt->dynalight    = ppip->dynalight;
    pprt->dynalight.on = bfalse;
    if ( 0 == multispawn )
    {
        pprt->dynalight.on = ppip->dynalight.mode;
        if ( DYNA_MODE_LOCAL == ppip->dynalight.mode )
        {
            pprt->dynalight.on = DYNA_MODE_OFF;
        }
    }

    // Set character attachments ( chr_attach==MAX_CHR means none )
    pprt->attachedto_ref = chr_attach;
    pprt->vrt_off = vrt_offset;

    // Correct facing
    facing += ppip->facing_pair.base;

    // Targeting...
    vel.z = 0;

    pprt->offset.z = generate_randmask( ppip->zspacing_pair.base, ppip->zspacing_pair.rand ) - ( ppip->zspacing_pair.rand >> 1 );
    tmp_pos.z += pprt->offset.z;
    velocity = generate_randmask( ppip->xyvel_pair.base, ppip->xyvel_pair.rand );
    pprt->target_ref = oldtarget;
    if ( ppip->newtargetonspawn )
    {
        if ( ppip->targetcaster )
        {
            // Set the target to the caster
            pprt->target_ref = chr_origin;
        }
        else
        {
            // Find a target
            pprt->target_ref = prt_find_target( pos.x, pos.y, pos.z, facing, ipip, team, chr_origin, oldtarget );
            if ( ACTIVE_CHR( pprt->target_ref ) && !ppip->homing )
            {
                facing -= glouseangle;
            }

            // Correct facing for dexterity...
            offsetfacing = 0;
            if ( ChrList.lst[chr_origin].dexterity < PERFECTSTAT )
            {
                // Correct facing for randomness
                offsetfacing  = generate_randmask( 0, ppip->facing_pair.rand ) - ( ppip->facing_pair.rand >> 1 );
                offsetfacing  = ( offsetfacing * ( PERFECTSTAT - ChrList.lst[chr_origin].dexterity ) ) / PERFECTSTAT;
            }

            if ( ACTIVE_CHR( pprt->target_ref ) && ppip->zaimspd != 0 )
            {
                // These aren't velocities...  This is to do aiming on the Z axis
                if ( velocity > 0 )
                {
                    vel.x = ChrList.lst[pprt->target_ref].pos.x - pos.x;
                    vel.y = ChrList.lst[pprt->target_ref].pos.y - pos.y;
                    tvel = SQRT( vel.x * vel.x + vel.y * vel.y ) / velocity;  // This is the number of steps...
                    if ( tvel > 0 )
                    {
                        vel.z = ( ChrList.lst[pprt->target_ref].pos.z + ( ChrList.lst[pprt->target_ref].bump.height * 0.5f ) - tmp_pos.z ) / tvel;  // This is the vel.z alteration
                        if ( vel.z < -( ppip->zaimspd >> 1 ) ) vel.z = -( ppip->zaimspd >> 1 );
                        if ( vel.z > ppip->zaimspd ) vel.z = ppip->zaimspd;
                    }
                }
            }
        }

        // Does it go away?
        if ( !ACTIVE_CHR( pprt->target_ref ) && ppip->needtarget )
        {
            prt_request_terminate( iprt );
            return maxparticles;
        }

        // Start on top of target
        if ( ACTIVE_CHR( pprt->target_ref ) && ppip->startontarget )
        {
            tmp_pos.x = ChrList.lst[pprt->target_ref].pos.x;
            tmp_pos.y = ChrList.lst[pprt->target_ref].pos.y;
        }
    }
    else
    {
        // Correct facing for randomness
        offsetfacing = generate_randmask( 0,  ppip->facing_pair.rand ) - ( ppip->facing_pair.rand >> 1 );
    }
    facing += offsetfacing;
    pprt->facing = facing;

    // this is actually pointint in the opposite direction?
    turn = ( facing + ATK_BEHIND ) >> 2;

    // Location data from arguments
    newrand = generate_randmask( ppip->xyspacing_pair.base, ppip->xyspacing_pair.rand );
    pprt->offset.x = -turntocos[turn & TRIG_TABLE_MASK] * newrand;
    pprt->offset.y = -turntosin[turn & TRIG_TABLE_MASK] * newrand;

    tmp_pos.x += pprt->offset.x;
    tmp_pos.y += pprt->offset.y;

    tmp_pos.x = CLIP( tmp_pos.x, 0, PMesh->gmem.edge_x - 2 );
    tmp_pos.y = CLIP( tmp_pos.y, 0, PMesh->gmem.edge_y - 2 );

    pprt->pos      = tmp_pos;
    pprt->pos_old  = tmp_pos;
    pprt->pos_stt  = tmp_pos;
    pprt->pos_safe = tmp_pos;

    // Velocity data
    vel.x = turntocos[turn & TRIG_TABLE_MASK] * velocity;
    vel.y = turntosin[turn & TRIG_TABLE_MASK] * velocity;
    vel.z += generate_randmask( ppip->zvel_pair.base, ppip->zvel_pair.rand ) - ( ppip->zvel_pair.rand >> 1 );
    pprt->vel = pprt->vel_old = pprt->vel_stt = vel;

    // Template values
    pprt->bump.size    = ppip->bumpsize;
    pprt->bump.sizebig = ppip->bumpsize * SQRT_TWO;
    pprt->bump.height  = ppip->bumpheight;
    pprt->type = ppip->type;

    // Image data
    pprt->rotate = generate_irand_pair( ppip->rotate_pair );
    pprt->rotateadd = ppip->rotateadd;
    pprt->size_stt = pprt->size = MAX( ppip->sizebase, 1 );
    pprt->size_add = ppip->sizeadd;
    pprt->imageadd = generate_irand_pair( ppip->imageadd );
    pprt->imagestt = INT_TO_FP8( ppip->imagebase );
    pprt->imagemax = INT_TO_FP8( ppip->numframes );
    prt_lifetime = ppip->time;
    if ( ppip->endlastframe && pprt->imageadd != 0 )
    {
        if ( ppip->time == 0 )
        {
            // Part time is set to 1 cycle
            int frames = ( pprt->imagemax / pprt->imageadd ) - 1;
            prt_lifetime = frames;
        }
        else
        {
            // Part time is used to give number of cycles
            int frames = (( pprt->imagemax / pprt->imageadd ) - 1 );
            prt_lifetime = ppip->time * frames;
        }
    }

    // "no lifetime" = "eternal"
    if ( 0 == prt_lifetime )
    {
        pprt->time_update = ~0;
        pprt->is_eternal  = btrue;
    }
    else
    {
        // the lifetime is really supposed tp be in terms of frames, but
        // to keep the number of updates stable, the frames could lag.
        // sooo... we just rescale the prt_lifetime so that it will work with the
        // updates and cross our fingers
        pprt->time_update = ceil( update_wld + ( float ) prt_lifetime * ( float )TARGET_UPS / ( float )TARGET_FPS );
    }

    // make the particle display AT LEAST one frame, regardless of how many updates
    // it has or when someone requests for it to terminate
    pprt->time_frame  = frame_all;

    // Set onwhichfan...
    pprt->onwhichfan   = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

    // Damage stuff
    range_to_pair( ppip->damage, &( pprt->damage ) );

    // Spawning data
    pprt->spawntime = ppip->contspawn_time;
    if ( pprt->spawntime != 0 )
    {
        pprt->spawntime = 1;
        if ( ACTIVE_CHR( pprt->attachedto_ref ) )
        {
            pprt->spawntime++; // Because attachment takes an update before it happens
        }
    }

    // Sound effect
    play_particle_sound( iprt, ppip->soundspawn );

    // set up the particle transparency
    pprt->inst.alpha = 0xFF;
    switch ( pprt->inst.type )
    {
        case SPRITE_SOLID: break;
        case SPRITE_ALPHA: pprt->inst.alpha = particletrans; break;
        case SPRITE_LIGHT: break;
    }

    if ( 0 == __prthitawall( pprt, NULL ) )
    {
        pprt->safe_valid = btrue;
    };

    // gat an initial value for the is_homing variable
    pprt->is_homing = ppip->homing && !ACTIVE_CHR( pprt->attachedto_ref );

#if defined(USE_DEBUG) && defined(DEBUG_PRT_LIST)

    // some code to track all allocated particles, where they came from, how long they are going to last,
    // what they are being used for...
    log_debug( "spawn_one_particle() - spawned a particle %d\n"
               "\tupdate == %d, last update == %d, frame == %d, minimum frame == %d\n"
               "\towner == %d(\"%s\")\n"
               "\tpip == %d(\"%s\")\n"
               "\t\t%s"
               "\tprofile == %d(\"%s\")\n"
               "\n",
               iprt,
               update_wld, pprt->time_update, frame_all, pprt->time_frame,
               chr_origin, ACTIVE_CHR( chr_origin ) ? ChrList.lst[chr_origin].Name : "INVALID",
               ipip, LOADED_PIP( ipip ) ? PipStack.lst[ipip].name : "INVALID",
               LOADED_PIP( ipip ) ? PipStack.lst[ipip].comment : "",
               iprofile, LOADED_PRO( iprofile ) ? ProList.lst[iprofile].name : "INVALID" );
#endif

    // c  ount ou all the requests for this particle type
    ppip->prt_create_count++;

    return iprt;
}

//--------------------------------------------------------------------------------------------
Uint32 __prthitawall( prt_t * pprt, float nrm[] )
{
    /// @details ZZ@> This function returns nonzero if the character hit a wall that the
    ///    character is not allowed to cross

    pip_t * ppip;
    Uint32 bits;

    if ( !ACTIVE_PPRT( pprt ) ) return 0;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return 0;
    ppip = PipStack.lst + pprt->pip_ref;

    bits = MPDFX_IMPASS;
    if ( ppip->bumpmoney ) bits |= MPDFX_WALL;

    return mesh_hitawall( PMesh, pprt->pos.v, 0.0f, bits, nrm );
}

//--------------------------------------------------------------------------------------------
// This is BB's most recent version of the update_all_particles() function that should treat
// all zombie/limbo particles properly and completely eliminates the improper modification of the
// particle loop-control-variable inside the loop (thanks zefz!)
void update_all_particles()
{
    /// @details BB@> update everything about a particle that does not depend on collisions
    ///               or interactions with characters
    int size_new;
    Uint16 particle;

    // figure out where the particle is on the mesh and update particle states
    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;
        pip_t * ppip;

        if ( !DISPLAY_PRT( particle ) ) continue;
        pprt = PrtList.lst + particle;

        pprt->onwhichfan   = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

        // update various particle states
        if ( !LOADED_PIP( pprt->pip_ref ) ) continue;
        ppip = PipStack.lst + pprt->pip_ref;

        // reject particles that are hidden
        pprt->is_hidden = bfalse;
        if ( ACTIVE_CHR( pprt->attachedto_ref ) )
        {
            pprt->is_hidden = ChrList.lst[pprt->attachedto_ref].is_hidden;
        }

        pprt->is_homing = ppip->homing && !ACTIVE_CHR( pprt->attachedto_ref );

    }

    // figure out where the particle is on the mesh and update particle states
    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;
        pip_t * ppip;
        bool_t inwater;

        if ( !ACTIVE_PRT( particle ) ) continue;
        pprt = PrtList.lst + particle;

        // stop here if the particle is hidden
        if ( pprt->is_hidden ) continue;

        // update various particle states
        if ( !LOADED_PIP( pprt->pip_ref ) ) continue;
        ppip = PipStack.lst + pprt->pip_ref;

        // do the particle interaction with water
        inwater = ( pprt->pos.z < water.surface_level ) && ( 0 != mesh_test_fx( PMesh, pprt->onwhichfan, MPDFX_WATER ) );

        if ( inwater && water.is_water && ppip->endwater )
        {
            // Check for disaffirming character
            if ( ACTIVE_CHR( pprt->attachedto_ref ) && pprt->owner_ref == pprt->attachedto_ref )
            {
                // Disaffirm the whole character
                disaffirm_attached_particles( pprt->attachedto_ref );
            }
            else
            {
                // destroy the particle
                prt_request_terminate( particle );
            }
        }
        else if ( inwater )
        {
            bool_t  spawn_valid = bfalse;
            Uint16  spawn_pip   = MAX_PIP;
            fvec3_t vtmp = VECT3( pprt->pos.x, pprt->pos.y, water.surface_level );

            if ( MAX_CHR == pprt->owner_ref &&
                 ( PIP_SPLASH == pprt->pip_ref || PIP_RIPPLE == pprt->pip_ref ) )
            {
                /* do not spawn anything for a splash or a ripple */
                spawn_valid = bfalse;
            }
            else
            {

                if ( !pprt->inwater )
                {
                    if ( SPRITE_SOLID == pprt->type )
                    {
                        spawn_pip = PIP_SPLASH;
                    }
                    else
                    {
                        spawn_pip = PIP_RIPPLE;
                    }
                    spawn_valid = btrue;
                }
                else
                {
                    if ( SPRITE_SOLID == pprt->type && !ACTIVE_CHR( pprt->attachedto_ref ) )
                    {
                        // only spawn ripples if you are touching the water surface!
                        if ( pprt->pos.z + pprt->bump.height > water.surface_level && pprt->pos.z - pprt->bump.height < water.surface_level )
                        {
                            int ripand = ~(( ~RIPPLEAND ) << 1 );
                            if ( 0 == (( update_wld + pprt->obj_base.guid ) & ripand ) )
                            {

                                spawn_valid = btrue;
                                spawn_pip = PIP_RIPPLE;
                            }
                        }
                    }
                }
            }

            if ( spawn_valid )
            {
                // Splash for particles is just a ripple
                spawn_one_particle( vtmp, 0, MAX_PROFILE, spawn_pip, MAX_CHR, GRIP_LAST,
                                    TEAM_NULL, MAX_CHR, TOTAL_MAX_PRT, 0, MAX_CHR );
            }

            pprt->inwater  = btrue;
        }
        else
        {
            pprt->inwater = bfalse;
        }
    }

    // the following functions should not be done the first time through the update loop
    if ( 0 == clock_wld ) return;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;
        pip_t * ppip;

        if ( !DISPLAY_PRT( particle ) ) continue;
        pprt = PrtList.lst + particle;

        // update various particle states
        if ( !LOADED_PIP( pprt->pip_ref ) ) continue;
        ppip = PipStack.lst + pprt->pip_ref;

        // Animate particle
        pprt->image = pprt->image + pprt->imageadd;
        if ( pprt->image >= pprt->imagemax ) pprt->image = 0;

        // rotate the particle
        pprt->rotate += pprt->rotateadd;

        // update the particle size
        if ( 0 != pprt->size_add )
        {
            // resize the paricle
            size_new =  pprt->size_add + pprt->size;
            pprt->size = CLIP( size_new, 0, 0xFFFF );

            /*if( pprt->type != SPRITE_SOLID && pprt->inst.alpha != 0.0f )
            {
                // adjust the particle alpha
                if( size_new > 0 )
                {
                    float ftmp = 1.0f - (float)ABS(pprt->size_add) / (float)size_new;
                    pprt->inst.alpha *= ftmp;
                }
                else
                {
                    pprt->inst.alpha = 0xFF;
                }
            }*/
        }

        // Change dyna light values
        if ( pprt->dynalight.level > 0 )
        {
            pprt->dynalight.level   += ppip->dynalight.level_add;
            if ( pprt->dynalight.level < 0 ) pprt->dynalight.level = 0;
        }
        else if ( pprt->dynalight.level < 0 )
        {
            // try to guess what should happen for negative lighting
            pprt->dynalight.level   += ppip->dynalight.level_add;
            if ( pprt->dynalight.level > 0 ) pprt->dynalight.level = 0;
        }
        else
        {
            pprt->dynalight.level += ppip->dynalight.level_add;
        }

        pprt->dynalight.falloff += ppip->dynalight.falloff_add;

        // spin the particle
        pprt->facing += ppip->facingadd;
    }

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;
        pip_t * ppip;
        Uint16 facing;

        if ( !ACTIVE_PRT( particle ) ) continue;
        pprt = PrtList.lst + particle;

        // update various particle states
        if ( !LOADED_PIP( pprt->pip_ref ) ) continue;
        ppip = PipStack.lst + pprt->pip_ref;

        // down the spawn timer
        if ( pprt->spawntime > 0 ) pprt->spawntime--;

        // Spawn new particles if continually spawning
        if ( 0 == pprt->spawntime && ppip->contspawn_amount > 0 )
        {
            Uint8 tnc;

            // reset the spawn timer
            pprt->spawntime = ppip->contspawn_time;

            facing = pprt->facing;
            for ( tnc = 0; tnc < ppip->contspawn_amount; tnc++ )
            {
                Uint16 prt_child = spawn_one_particle( pprt->pos, facing, pprt->profile_ref, ppip->contspawn_pip,
                                                       MAX_CHR, GRIP_LAST, pprt->team, pprt->owner_ref, particle, tnc, pprt->target_ref );

                if ( ppip->facingadd != 0 && ACTIVE_PRT( prt_child ) )
                {
                    // Hack to fix velocity
                    PrtList.lst[prt_child].vel.x += pprt->vel.x;
                    PrtList.lst[prt_child].vel.y += pprt->vel.y;
                }
                facing += ppip->contspawn_facingadd;
            }
        }
    }

    // apply damage from  attatched bump particles (about once a second)
    if ( 0 == ( update_wld & 31 ) )
    {
        for ( particle = 0; particle < maxparticles; particle++ )
        {
            prt_t * pprt;
            pip_t * ppip;
            Uint16 ichr;

            if ( !ACTIVE_PRT( particle ) ) continue;
            pprt = PrtList.lst + particle;

            // do nothing if the particle is hidden
            if ( pprt->is_hidden ) continue;

            // is this is not a damage particle for me?
            if ( pprt->attachedto_ref == pprt->owner_ref ) continue;

            ppip = prt_get_ppip( particle );
            if ( NULL == ppip ) continue;

            ichr = pprt->attachedto_ref;
            if ( !ACTIVE_CHR( ichr ) ) continue;

            // Attached iprt_b damage ( Burning )
            if ( ppip->allowpush && ppip->xyvel_pair.base == 0 )
            {
                // Make character limp
                ChrList.lst[ichr].vel.x *= 0.5f;
                ChrList.lst[ichr].vel.y *= 0.5f;
            }

            damage_character( ichr, ATK_BEHIND, pprt->damage, pprt->damagetype, pprt->team, pprt->owner_ref, ppip->damfx, bfalse );
        }
    }
}

void particle_set_level( prt_t * pprt, float level )
{
    float loc_height;

    if ( !DISPLAY_PPRT( pprt ) ) return;

    pprt->enviro.level = level;

    loc_height = pprt->inst.scale * MAX( FP8_TO_FLOAT( pprt->size ), pprt->offset.z * 0.54 );

    // if the particle is resting on the ground, modify its
    //pprt->enviro.hlerp = 1.0f;
    //if( !ACTIVE_CHR(pprt->attachedto_ref) && loc_height > 0 )
    //{
    //    pprt->enviro.hlerp = (pprt->pos.z - (pprt->enviro.level + loc_height)) / loc_height;
    //    pprt->enviro.hlerp = CLIP(pprt->enviro.hlerp, 0, 1);
    //}

    pprt->enviro.adj_level = pprt->enviro.level;
    pprt->enviro.adj_floor = pprt->enviro.floor_level;
    //if ( pprt->enviro.hlerp < 1.0f )
    //{
    //    float adjustment = loc_height * (1.0f - pprt->enviro.hlerp);

    //    pprt->enviro.adj_level += adjustment;
    //    pprt->enviro.adj_floor += adjustment;
    //}

    pprt->enviro.adj_level += loc_height;
    pprt->enviro.adj_floor += loc_height;

    // set the zlerp after we have done everything to the particle's level we care to
    pprt->enviro.zlerp = ( pprt->pos.z - pprt->enviro.adj_level ) / PLATTOLERANCE;
    pprt->enviro.zlerp = CLIP( pprt->enviro.zlerp, 0, 1 );
}

//--------------------------------------------------------------------------------------------
void move_one_particle_get_environment( prt_t * pprt )
{
    /// @details BB@> A helper function that gets all of the information about the particle's
    ///               environment (like friction, etc.) that will be necessary for the other
    ///               move_one_particle_*() functions to work

    Uint32 itile;
    float loc_level = 0.0f;

    pip_t * ppip;

    if ( !DISPLAY_PPRT( pprt ) ) return;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    //---- character "floor" level
    pprt->enviro.floor_level = mesh_get_level( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->enviro.level       = pprt->enviro.floor_level;

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform variable from the
    //     last loop
    loc_level = pprt->enviro.floor_level;
    if ( ACTIVE_CHR( pprt->onwhichplatform ) )
    {
        loc_level = MAX( pprt->enviro.floor_level, ChrList.lst[pprt->onwhichplatform].pos.z + ChrList.lst[pprt->onwhichplatform].chr_chr_cv.maxs[OCT_Z] );
    }
    particle_set_level( pprt, loc_level );

    //---- the "twist" of the floor
    pprt->enviro.twist = TWIST_FLAT;
    itile              = INVALID_TILE;
    if ( ACTIVE_CHR( pprt->onwhichplatform ) )
    {
        // this only works for 1 level of attachment
        itile = ChrList.lst[pprt->onwhichplatform].onwhichfan;
    }
    else
    {
        itile = pprt->onwhichfan;
    }

    if ( VALID_TILE( PMesh, itile ) )
    {
        pprt->enviro.twist = PMesh->gmem.grid_list[itile].twist;
    }

    // the "watery-ness" of whatever water might be here
    pprt->enviro.is_watery = water.is_water && pprt->enviro.inwater;
    pprt->enviro.is_slippy = !pprt->enviro.is_watery && ( 0 != mesh_test_fx( PMesh, pprt->onwhichfan, MPDFX_SLIPPY ) );

    //---- traction
    pprt->enviro.traction = 1.0f;
    if ( pprt->is_homing )
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if ( ACTIVE_CHR( pprt->onwhichplatform ) )
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        fvec3_t   platform_up;

        chr_getMatUp( ChrList.lst + pprt->onwhichplatform, &platform_up );
        platform_up = fvec3_normalize( platform_up.v );

        pprt->enviro.traction = ABS( platform_up.z ) * ( 1.0f - pprt->enviro.zlerp ) + 0.25 * pprt->enviro.zlerp;

        if ( pprt->enviro.is_slippy )
        {
            pprt->enviro.traction /= hillslide * ( 1.0f - pprt->enviro.zlerp ) + 1.0f * pprt->enviro.zlerp;
        }
    }
    else if ( VALID_TILE( PMesh, pprt->onwhichfan ) )
    {
        pprt->enviro.traction = ABS( map_twist_nrm[pprt->enviro.twist].z ) * ( 1.0f - pprt->enviro.zlerp ) + 0.25 * pprt->enviro.zlerp;

        if ( pprt->enviro.is_slippy )
        {
            pprt->enviro.traction /= hillslide * ( 1.0f - pprt->enviro.zlerp ) + 1.0f * pprt->enviro.zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if ( pprt->enviro.is_watery )
    {
        pprt->enviro.fluid_friction_z  = waterfriction;
        pprt->enviro.fluid_friction_xy = waterfriction;
    }
    else
    {
        pprt->enviro.fluid_friction_xy = pprt->enviro.air_friction;       // like real-life air friction
        pprt->enviro.fluid_friction_z  = pprt->enviro.air_friction;
    }

    //---- friction
    pprt->enviro.friction_xy = 1.0f;
    if ( !pprt->is_homing )
    {
        // Make the characters slide
        float temp_friction_xy = noslipfriction;
        if ( VALID_TILE( PMesh, pprt->onwhichfan ) && pprt->enviro.is_slippy )
        {
            // It's slippy all right...
            temp_friction_xy = slippyfriction;
        }

        pprt->enviro.friction_xy = pprt->enviro.zlerp * 1.0f + ( 1.0f - pprt->enviro.zlerp ) * temp_friction_xy;
    }
}

//--------------------------------------------------------------------------------------------
void move_one_particle_do_floor_friction( prt_t * pprt )
{
    /// @details BB@> A helper function that computes particle friction with the floor
    /// @note this is pretty much ripped from the character version of this function and may
    ///       contain some features that are not necessary for any particles that are actually in game.
    ///       For instance, the only particles that is under their own control are the homing particles
    ///       but they do not have friction with the mesh, but that case is still treated in the code below.

    float temp_friction_xy;
    fvec3_t   vup, floor_acc, fric, fric_floor;

    pip_t * ppip;

    if ( !DISPLAY_PPRT( pprt ) ) return;

    // limit friction effects to solid objects?
    if ( SPRITE_SOLID != pprt->type ) return;

    // if the particle is homing in on something, ignore friction
    if ( pprt->is_homing ) return;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    // figure out the acceleration due to the current "floor"
    floor_acc.x = floor_acc.y = floor_acc.z = 0.0f;
    temp_friction_xy = 1.0f;
    if ( ACTIVE_CHR( pprt->onwhichplatform ) )
    {
        chr_t * pplat = ChrList.lst + pprt->onwhichplatform;

        temp_friction_xy = platstick;

        floor_acc.x = pplat->vel.x - pplat->vel_old.x;
        floor_acc.y = pplat->vel.y - pplat->vel_old.y;
        floor_acc.z = pplat->vel.z - pplat->vel_old.z;

        chr_getMatUp( pplat, &vup );
    }
    else
        // if ( !pprt->alive || pprt->isitem )
    {
        temp_friction_xy = 0.5f;
        floor_acc.x = -pprt->vel.x;
        floor_acc.y = -pprt->vel.y;
        floor_acc.z = -pprt->vel.z;

        if ( TWIST_FLAT == pprt->enviro.twist )
        {
            vup.x = vup.y = 0.0f;
            vup.z = 1.0f;
        }
        else
        {
            vup = map_twist_nrm[pprt->enviro.twist];
        }
    }
    //else
    //{
    //    temp_friction_xy = pprt->enviro.friction_xy;

    //    if( TWIST_FLAT == pprt->enviro.twist )
    //    {
    //        vup.x = vup.y = 0.0f;
    //        vup.z = 1.0f;
    //    }
    //    else
    //    {
    //        vup = map_twist_nrm[pprt->enviro.twist];
    //    }

    //    if( ABS(pprt->vel.x) + ABS(pprt->vel.y) + ABS(pprt->vel.z) > 0.0f )
    //    {
    //        float ftmp;
    //        fvec3_t   vfront = mat_getChrForward( pprt->inst.matrix );

    //        floor_acc.x = -pprt->vel.x;
    //        floor_acc.y = -pprt->vel.y;
    //        floor_acc.z = -pprt->vel.z;

    //        //---- get the "bad" velocity (perpendicular to the direction of motion)
    //        vfront = fvec3_normalize( vfront.v );
    //        ftmp = fvec3_dot_product( floor_acc.v, vfront.v );

    //        floor_acc.x -= ftmp * vfront.x;
    //        floor_acc.y -= ftmp * vfront.y;
    //        floor_acc.z -= ftmp * vfront.z;
    //    }
    //}

    // the first guess about the floor friction
    fric_floor.x = floor_acc.x * ( 1.0f - pprt->enviro.zlerp ) * ( 1.0f - temp_friction_xy ) * pprt->enviro.traction;
    fric_floor.y = floor_acc.y * ( 1.0f - pprt->enviro.zlerp ) * ( 1.0f - temp_friction_xy ) * pprt->enviro.traction;
    fric_floor.z = floor_acc.z * ( 1.0f - pprt->enviro.zlerp ) * ( 1.0f - temp_friction_xy ) * pprt->enviro.traction;

    // the total "friction" due to the floor
    fric.x = fric_floor.x + pprt->enviro.acc.x;
    fric.y = fric_floor.y + pprt->enviro.acc.y;
    fric.z = fric_floor.z + pprt->enviro.acc.z;

    //---- limit the friction to whatever is horizontal to the mesh
    if ( TWIST_FLAT == pprt->enviro.twist )
    {
        floor_acc.z = 0.0f;
        fric.z      = 0.0f;
    }
    else
    {
        float ftmp;
        fvec3_t   vup = map_twist_nrm[pprt->enviro.twist];

        ftmp = fvec3_dot_product( floor_acc.v, vup.v );

        floor_acc.x -= ftmp * vup.x;
        floor_acc.y -= ftmp * vup.y;
        floor_acc.z -= ftmp * vup.z;

        ftmp = fvec3_dot_product( fric.v, vup.v );

        fric.x -= ftmp * vup.x;
        fric.y -= ftmp * vup.y;
        fric.z -= ftmp * vup.z;
    }

    // test to see if the player has any more friction left?
    pprt->enviro.is_slipping = ( ABS( fric.x ) + ABS( fric.y ) + ABS( fric.z ) > pprt->enviro.friction_xy );

    if ( pprt->enviro.is_slipping )
    {
        pprt->enviro.traction *= 0.5f;
        temp_friction_xy  = SQRT( temp_friction_xy );

        fric_floor.x = floor_acc.x * ( 1.0f - pprt->enviro.zlerp ) * ( 1.0f - temp_friction_xy ) * pprt->enviro.traction;
        fric_floor.y = floor_acc.y * ( 1.0f - pprt->enviro.zlerp ) * ( 1.0f - temp_friction_xy ) * pprt->enviro.traction;
        fric_floor.z = floor_acc.z * ( 1.0f - pprt->enviro.zlerp ) * ( 1.0f - temp_friction_xy ) * pprt->enviro.traction;
    }

    //apply the floor friction
    pprt->vel.x += fric_floor.x;
    pprt->vel.y += fric_floor.y;
    pprt->vel.z += fric_floor.z;

    // Apply fluid friction from last time
    pprt->vel.x += -pprt->vel.x * ( 1.0f - pprt->enviro.fluid_friction_xy );
    pprt->vel.y += -pprt->vel.y * ( 1.0f - pprt->enviro.fluid_friction_xy );
    pprt->vel.z += -pprt->vel.z * ( 1.0f - pprt->enviro.fluid_friction_z );
}

//--------------------------------------------------------------------------------------------
//// Do homing
//if( !ACTIVE_CHR( pprt->attachedto_ref ) )
//{
//    if ( ppip->homing && ACTIVE_CHR( pprt->target_ref ) )
//    {
//        if ( !ChrList.lst[pprt->target_ref].alive )
//        {
//            prt_request_terminate( iprt );
//        }
//        else
//        {
//            if ( !ACTIVE_CHR( pprt->attachedto_ref ) )
//            {
//                int       ival;
//                float     vlen, min_length, uncertainty;
//                fvec3_t   vdiff, vdither;

//                vdiff = fvec3_sub( ChrList.lst[pprt->target_ref].pos.v, pprt->pos.v );
//                vdiff.z += ChrList.lst[pprt->target_ref].bump.height * 0.5f;

//                min_length = ( 2 * 5 * 256 * ChrList.lst[pprt->owner_ref].wisdom ) / PERFECTBIG;

//                // make a little incertainty about the target
//                uncertainty = 256 - ( 256 * ChrList.lst[pprt->owner_ref].intelligence ) / PERFECTBIG;

//                ival = RANDIE;
//                vdither.x = ( ((float) ival / 0x8000) - 1.0f )  * uncertainty;

//                ival = RANDIE;
//                vdither.y = ( ((float) ival / 0x8000) - 1.0f )  * uncertainty;

//                ival = RANDIE;
//                vdither.z = ( ((float) ival / 0x8000) - 1.0f )  * uncertainty;

//                // take away any dithering along the direction of motion of the particle
//                vlen = fvec3_dot_product( pprt->vel.v, pprt->vel.v );
//                if( vlen > 0.0f )
//                {
//                    float vdot = fvec3_dot_product( vdither.v, pprt->vel.v ) / vlen;

//                    vdither.x -= vdot * vdiff.x / vlen;
//                    vdither.y -= vdot * vdiff.y / vlen;
//                    vdither.z -= vdot * vdiff.z / vlen;
//                }

//                // add in the dithering
//                vdiff.x += vdither.x;
//                vdiff.y += vdither.y;
//                vdiff.z += vdither.z;

//                // Make sure that vdiff doesn't ever get too small.
//                // That just makes the particle slooooowww down when it approaches the target.
//                // Do a real kludge here. this should be a lot faster than a square root, but ...
//                vlen = ABS(vdiff.x) + ABS(vdiff.y) + ABS(vdiff.z);
//                if( vlen != 0.0f )
//                {
//                    float factor = min_length / vlen;

//                    vdiff.x *= factor;
//                    vdiff.y *= factor;
//                    vdiff.z *= factor;
//                }

//                pprt->vel.x = ( pprt->vel.x + vdiff.x * ppip->homingaccel ) * ppip->homingfriction;
//                pprt->vel.y = ( pprt->vel.y + vdiff.y * ppip->homingaccel ) * ppip->homingfriction;
//                pprt->vel.z = ( pprt->vel.z + vdiff.z * ppip->homingaccel ) * ppip->homingfriction;
//            }

//            if ( ppip->rotatetoface )
//            {
//                // Turn to face target
//                pprt->facing =vec_to_facing( ChrList.lst[pprt->target_ref].pos.x - pprt->pos.x , ChrList.lst[pprt->target_ref].pos.y - pprt->pos.y );
//            }
//        }
//    }

//    // do gravitational acceleration
//    if( !ppip->homing )
//    {
//        pprt->vel.z += gravity * lerp_z;

//        // Do speed limit on Z
//        if ( pprt->vel.z < -ppip->spdlimit )
//        {
//            pprt->vel.z = -ppip->spdlimit;
//        }
//    }

//}
void move_one_particle_do_homing( prt_t * pprt )
{
    pip_t * ppip;

    int iprt;

    if ( !DISPLAY_PPRT( pprt ) ) return;
    iprt = GET_INDEX_PPRT( pprt );

    if ( !pprt->is_homing || !ACTIVE_CHR( pprt->target_ref ) ) return;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    if ( !ChrList.lst[pprt->target_ref].alive )
    {
        prt_request_terminate( iprt );
    }
    else
    {
        chr_t * ptarget = ChrList.lst + pprt->target_ref;

        if ( !ACTIVE_CHR( pprt->attachedto_ref ) )
        {
            int       ival;
            float     vlen, min_length, uncertainty;
            fvec3_t   vdiff, vdither;

            vdiff = fvec3_sub( ptarget->pos.v, pprt->pos.v );
            vdiff.z += ptarget->bump.height * 0.5f;

            min_length = ( 2 * 5 * 256 * ChrList.lst[pprt->owner_ref].wisdom ) / PERFECTBIG;

            // make a little incertainty about the target
            uncertainty = 256 - ( 256 * ChrList.lst[pprt->owner_ref].intelligence ) / PERFECTBIG;

            ival = RANDIE;
            vdither.x = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

            ival = RANDIE;
            vdither.y = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

            ival = RANDIE;
            vdither.z = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

            // take away any dithering along the direction of motion of the particle
            vlen = fvec3_dot_product( pprt->vel.v, pprt->vel.v );
            if ( vlen > 0.0f )
            {
                float vdot = fvec3_dot_product( vdither.v, pprt->vel.v ) / vlen;

                vdither.x -= vdot * vdiff.x / vlen;
                vdither.y -= vdot * vdiff.y / vlen;
                vdither.z -= vdot * vdiff.z / vlen;
            }

            // add in the dithering
            vdiff.x += vdither.x;
            vdiff.y += vdither.y;
            vdiff.z += vdither.z;

            // Make sure that vdiff doesn't ever get too small.
            // That just makes the particle slooooowww down when it approaches the target.
            // Do a real kludge here. this should be a lot faster than a square root, but ...
            vlen = ABS( vdiff.x ) + ABS( vdiff.y ) + ABS( vdiff.z );
            if ( vlen != 0.0f )
            {
                float factor = min_length / vlen;

                vdiff.x *= factor;
                vdiff.y *= factor;
                vdiff.z *= factor;
            }

            pprt->vel.x = ( pprt->vel.x + vdiff.x * ppip->homingaccel ) * ppip->homingfriction;
            pprt->vel.y = ( pprt->vel.y + vdiff.y * ppip->homingaccel ) * ppip->homingfriction;
            pprt->vel.z = ( pprt->vel.z + vdiff.z * ppip->homingaccel ) * ppip->homingfriction;
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_one_particle_do_z_motion( prt_t * pprt )
{
    /// @details BB@> A helper function that does gravitational acceleration and buoyancy

    pip_t * ppip;
    float loc_zlerp;

    if ( !DISPLAY_PPRT( pprt ) ) return;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    if ( pprt->is_homing || ACTIVE_CHR( pprt->attachedto_ref ) ) return;

    // Do particle buoyancy. This is kinda BS the way it is calculated
    if ( pprt->vel.z < -ppip->spdlimit )
    {
        pprt->vel.z = -ppip->spdlimit;
    }

    loc_zlerp = CLIP( pprt->enviro.zlerp, 0.0f, 1.0f );

    // do gravity
    if ( pprt->enviro.is_slippy && pprt->enviro.twist != TWIST_FLAT && loc_zlerp < 1.0f )
    {
        // hills make particles slide

        fvec3_t   gpara, gperp;

        gperp.x = map_twistvel_x[pprt->enviro.twist];
        gperp.y = map_twistvel_y[pprt->enviro.twist];
        gperp.z = map_twistvel_z[pprt->enviro.twist];

        gpara.x = 0       - gperp.x;
        gpara.y = 0       - gperp.y;
        gpara.z = gravity - gperp.z;

        pprt->vel.x += gpara.x + gperp.x * loc_zlerp;
        pprt->vel.y += gpara.y + gperp.y * loc_zlerp;
        pprt->vel.z += gpara.z + gperp.z * loc_zlerp;
    }
    else
    {
        pprt->vel.z += loc_zlerp * gravity;
    }
}

//--------------------------------------------------------------------------------------------
//// check for a floor collision
//ftmp = pprt->pos.z;
//pprt->pos.z += pprt->vel.z;
//if ( pprt->pos.z < level )
//{
//    if( pprt->vel.z < - STOPBOUNCINGPART )
//    {
//        // the particle will bounce
//        hit_a_floor = btrue;

//        nrm.z = 1.0f;
//        pprt->pos.z = ftmp;
//    }
//    else if ( pprt->vel.z > 0.0f )
//    {
//        // the particle is not bouncing, it is just at the wrong height
//        pprt->pos.z = level;
//    }
//    else
//    {
//        // the particle is in the "stop bouncing zone"
//        pprt->pos.z = level + 1;
//        pprt->vel.z = 0.0f;
//    }
//}

//// check for an x wall collision
//if( ABS(pprt->vel.x) > 0.0f )
//{
//    ftmp = pprt->pos.x;
//    pprt->pos.x += pprt->vel.x;
//    if ( __prthitawall( iprt, NULL ) )
//    {
//        hit_a_wall = btrue;

//        nrm.x = -SGN(pprt->vel.x);
//        pprt->pos.x = ftmp;
//    }
//}

//// check for an y wall collision
//if( ABS(pprt->vel.y) > 0.0f )
//{
//    ftmp = pprt->pos.y;
//    pprt->pos.y += pprt->vel.y;
//    if ( __prthitawall( iprt, NULL ) )
//    {
//        hit_a_wall = btrue;

//        nrm.y = -SGN(pprt->vel.y);
//        pprt->pos.y = ftmp;
//    }
//}

//// handle the collision
//if( (hit_a_wall && ppip->endwall) || (hit_a_floor && ppip->endground) )
//{
//    prt_request_terminate( iprt );
//    continue;
//}

//// handle the sounds
//if( hit_a_wall )
//{
//    // Play the sound for hitting the floor [FSND]
//    play_particle_sound( iprt, ppip->soundwall );
//}

//if( hit_a_floor )
//{
//    // Play the sound for hitting the floor [FSND]
//    play_particle_sound( iprt, ppip->soundfloor );
//}

//// do the reflections off the walls and floors
//if( !ACTIVE_CHR( pprt->attachedto_ref ) && (hit_a_wall || hit_a_floor) )
//{
//    float fx, fy;

//    if( (hit_a_wall && ABS(pprt->vel.x) + ABS(pprt->vel.y) > 0.0f) ||
//        (hit_a_floor && pprt->vel.z < 0.0f) )
//    {
//        float vdot;
//        fvec3_t   vpara, vperp;

//        nrm = fvec3_normalize( nrm.v );

//        vdot  = fvec3_dot_product( nrm.v, pprt->vel.v );

//        vperp.x = nrm.x * vdot;
//        vperp.y = nrm.y * vdot;
//        vperp.z = nrm.z * vdot;

//        vpara.x = pprt->vel.x - vperp.x;
//        vpara.y = pprt->vel.y - vperp.y;
//        vpara.z = pprt->vel.z - vperp.z;

//        // we can use the impulse to determine how much velocity to kill in the parallel direction
//        //imp.x = vperp.x * (1.0f + ppip->dampen);
//        //imp.y = vperp.y * (1.0f + ppip->dampen);
//        //imp.z = vperp.z * (1.0f + ppip->dampen);

//        // do the reflection
//        vperp.x *= -ppip->dampen;
//        vperp.y *= -ppip->dampen;
//        vperp.z *= -ppip->dampen;

//        // fake the friction, for now
//        if( 0.0f != nrm.y || 0.0f != nrm.z )
//        {
//            vpara.x *= ppip->dampen;
//        }

//        if( 0.0f != nrm.x || 0.0f != nrm.z )
//        {
//            vpara.y *= ppip->dampen;
//        }

//        if( 0.0f != nrm.x || 0.0f != nrm.y )
//        {
//            vpara.z *= ppip->dampen;
//        }

//        // add the components back together
//        pprt->vel.x = vpara.x + vperp.x;
//        pprt->vel.y = vpara.y + vperp.y;
//        pprt->vel.z = vpara.z + vperp.z;
//    }

//    if( nrm.z != 0.0f && pprt->vel.z < STOPBOUNCINGPART )
//    {
//        // this is the very last bounce
//        pprt->vel.z = 0.0f;
//        pprt->pos.z = level + 0.0001f;
//    }

//    if( hit_a_wall )
//    {
//        // fix the facing
//        facing_to_vec( pprt->facing, &fx, &fy );

//        if( 0.0f != nrm.x )
//        {
//            fx *= -1;
//        }

//        if( 0.0f != nrm.y )
//        {
//            fy *= -1;
//        }

//        pprt->facing = vec_to_facing( fx, fy );
//    }
//}
bool_t move_one_particle_integrate_motion( prt_t * pprt )
{
    /// @details BB@> A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    pip_t * ppip;

    float ftmp, loc_level;
    Uint16 iprt;
    bool_t hit_a_floor, hit_a_wall;
    fvec3_t nrm_total;
    fvec2_t nrm;

    if ( !DISPLAY_PPRT( pprt ) ) return bfalse;
    iprt = GET_INDEX_PPRT( pprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    hit_a_floor = bfalse;
    hit_a_wall  = bfalse;
    nrm_total.x = nrm_total.y = nrm_total.z = 0;

    loc_level = pprt->enviro.adj_level;

    // Move the particle
    ftmp = pprt->pos.z;
    pprt->pos.z += pprt->vel.z;
    LOG_NAN( pprt->pos.z );
    if ( pprt->pos.z < loc_level )
    {
        hit_a_floor = btrue;

        if ( pprt->vel.z < - STOPBOUNCINGPART )
        {
            // the particle will bounce
            nrm_total.z -= SGN( pprt->vel.z );
            pprt->pos.z = ftmp;
        }
        else if ( pprt->vel.z > 0.0f )
        {
            // the particle is not bouncing, it is just at the wrong height
            pprt->pos.z = loc_level;
        }
        else
        {
            // the particle is in the "stop bouncing zone"
            pprt->pos.z = loc_level + 1;
            pprt->vel.z = 0.0f;
        }
    }

    ftmp = pprt->pos.x;
    pprt->pos.x += pprt->vel.x;
    LOG_NAN( pprt->pos.x );
    if ( __prthitawall( pprt, nrm.v ) )
    {
        hit_a_wall = btrue;

        nrm_total.x -= SGN( pprt->vel.x );

        pprt->pos.x = ftmp;
    }

    ftmp = pprt->pos.y;
    pprt->pos.y += pprt->vel.y;
    LOG_NAN( pprt->pos.y );
    if ( __prthitawall( pprt, nrm.v ) )
    {
        hit_a_wall = btrue;

        nrm_total.y -= SGN( pprt->vel.y );

        pprt->pos.y = ftmp;
    }

    // handle the collision
    if (( hit_a_wall && ppip->endwall ) || ( hit_a_floor && ppip->endground ) )
    {
        prt_request_terminate( iprt );
        return btrue;
    }

    // handle the sounds
    if ( hit_a_wall )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( iprt, ppip->soundwall );
    }

    if ( hit_a_floor )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( iprt, ppip->soundfloor );
    }

    // do the reflections off the walls and floors
    if ( !ACTIVE_CHR( pprt->attachedto_ref ) && ( hit_a_wall || hit_a_floor ) )
    {
        float fx, fy;

        if (( hit_a_wall && ABS( pprt->vel.x ) + ABS( pprt->vel.y ) > 0.0f ) ||
            ( hit_a_floor && pprt->vel.z < 0.0f ) )
        {
            float vdot;
            fvec3_t   vpara, vperp;

            nrm_total = fvec3_normalize( nrm_total.v );

            vdot  = fvec3_dot_product( nrm_total.v, pprt->vel.v );

            vperp.x = nrm_total.x * vdot;
            vperp.y = nrm_total.y * vdot;
            vperp.z = nrm_total.z * vdot;

            vpara.x = pprt->vel.x - vperp.x;
            vpara.y = pprt->vel.y - vperp.y;
            vpara.z = pprt->vel.z - vperp.z;

            // we can use the impulse to determine how much velocity to kill in the parallel direction
            //imp.x = vperp.x * (1.0f + ppip->dampen);
            //imp.y = vperp.y * (1.0f + ppip->dampen);
            //imp.z = vperp.z * (1.0f + ppip->dampen);

            // do the reflection
            vperp.x *= -ppip->dampen;
            vperp.y *= -ppip->dampen;
            vperp.z *= -ppip->dampen;

            // fake the friction, for now
            if ( 0.0f != nrm_total.y || 0.0f != nrm_total.z )
            {
                vpara.x *= ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.z )
            {
                vpara.y *= ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.y )
            {
                vpara.z *= ppip->dampen;
            }

            // add the components back together
            pprt->vel.x = vpara.x + vperp.x;
            pprt->vel.y = vpara.y + vperp.y;
            pprt->vel.z = vpara.z + vperp.z;
        }

        if ( nrm_total.z != 0.0f && pprt->vel.z < STOPBOUNCINGPART )
        {
            // this is the very last bounce
            pprt->vel.z = 0.0f;
            pprt->pos.z = loc_level + 0.0001f;
        }

        if ( hit_a_wall )
        {
            // fix the facing
            facing_to_vec( pprt->facing, &fx, &fy );

            if ( 0.0f != nrm_total.x )
            {
                fx *= -1;
            }

            if ( 0.0f != nrm_total.y )
            {
                fy *= -1;
            }

            pprt->facing = vec_to_facing( fx, fy );
        }
    }

    if ( pprt->is_homing )
    {
        if ( pprt->pos.z < 0 )
        {
            pprt->pos.z = 0;  // Don't fall in pits...
        }
    }

    if ( ppip->rotatetoface )
    {
        if ( ABS( pprt->vel.x ) + ABS( pprt->vel.y ) > 1e-6 )
        {
            // use velocity to find the angle
            pprt->facing = vec_to_facing( pprt->vel.x, pprt->vel.y );
        }
        else if ( ACTIVE_CHR( pprt->target_ref ) )
        {
            chr_t * ptarget = ChrList.lst + pprt->target_ref;

            // face your target
            pprt->facing = vec_to_facing( ptarget->pos.x - pprt->pos.x , ptarget->pos.y - pprt->pos.y );
        }
    }

    pprt->safe_valid = bfalse;
    if ( !__prthitawall( pprt, NULL ) )
    {
        pprt->pos_safe   = pprt->pos;
        pprt->safe_valid = btrue;
    }
    else
    {
        pprt->pos = pprt->pos_safe;
        if ( !__prthitawall( pprt, NULL ) )
        {
            pprt->safe_valid = btrue;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t move_one_particle( prt_t * pprt )
{
    /// @details BB@> The master function for controlling a particle's motion

    prt_environment_t * penviro;
    pip_t * ppip;

    int iprt;

    if ( !DISPLAY_PPRT( pprt ) ) return bfalse;
    penviro = &( pprt->enviro );
    iprt = GET_INDEX_PPRT( pprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    // if the particle is hidden it is frozen in time. do nothing.
    if ( pprt->is_hidden ) return bfalse;

    // save the acceleration from the last time-step
    pprt->enviro.acc = fvec3_sub( pprt->vel.v, pprt->vel_old.v );

    // determine the actual velocity for attached particles
    if ( ACTIVE_CHR( pprt->attachedto_ref ) )
    {
        pprt->vel = fvec3_sub( pprt->pos.v, pprt->pos_old.v );
    }

    // Particle's old location
    pprt->pos_old    = pprt->pos;
    pprt->vel_old    = pprt->vel;

    // what is the local environment like?
    move_one_particle_get_environment( pprt );

    // do friction with the floor before volontary motion
    move_one_particle_do_floor_friction( pprt );

    move_one_particle_do_homing( pprt );

    move_one_particle_do_z_motion( pprt );

    move_one_particle_integrate_motion( pprt );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void move_all_particles( void )
{
    /// @details ZZ@> This is the particle physics function

    int cnt;

    const float air_friction = 0.9868f;  // gives the same terminal velocity in terms of the size of the game characters
    const float ice_friction = 0.9738f;  // the square of air_friction

    // move every particle
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_t * pprt;

        if ( !DISPLAY_PRT( cnt ) ) continue;
        pprt = PrtList.lst + cnt;

        // prime the environment
        pprt->enviro.air_friction = air_friction;
        pprt->enviro.ice_friction = ice_friction;

        move_one_particle( pprt );
    }
}

struct s_spawn_particle_info
{
    fvec3_t  pos;
    Uint16   facing;
    Uint16   iprofile;
    Uint16   ipip;

    Uint16   chr_attach;
    Uint16   vrt_offset;
    Uint8    team;

    Uint16   chr_origin;
    Uint16   prt_origin;
    Uint16   multispawn;
    Uint16   oldtarget;
};
typedef struct s_spawn_particle_info spawn_particle_info_t;

//--------------------------------------------------------------------------------------------
void cleanup_all_particles()
{
    int iprt, cnt, tnc;

    int                   delay_spawn_count = 0;
    spawn_particle_info_t delay_spawn_list[TOTAL_MAX_PRT];

    //printf("\n----cleanup_all_particles()----\n");

    // do end-of-life care for particles
    for ( iprt = 0, cnt = 0; iprt < maxparticles; iprt++ )
    {
        prt_t * pprt;
        Uint16  ipip;
        bool_t  time_out;

        if ( !ALLOCATED_PRT( iprt ) ) continue;
        pprt = PrtList.lst + iprt;

        time_out = !pprt->is_eternal && ( update_wld >= pprt->time_update );
        if (( ego_object_waiting != pprt->obj_base.state ) && !time_out ) continue;

        // make sure the particle has been DISPLAYED at least once, or you can get
        // some wierd particle flickering
        if ( pprt->time_frame >= frame_all + 1 ) continue;

        // Spawn new particles if time for old one is up
        ipip = pprt->pip_ref;
        if ( LOADED_PIP( ipip ) )
        {
            pip_t * ppip;
            Uint16 facing;

            ppip = PipStack.lst + ipip;

            facing = pprt->facing;
            for ( tnc = 0; tnc < ppip->endspawn_amount; tnc++ )
            {
                if ( delay_spawn_count < TOTAL_MAX_PRT )
                {
                    spawn_particle_info_t * pinfo = delay_spawn_list + delay_spawn_count;
                    delay_spawn_count++;

                    pinfo->pos        = pprt->pos_old;
                    pinfo->facing     = facing;
                    pinfo->iprofile   = pprt->profile_ref;
                    pinfo->ipip       = ppip->endspawn_pip;
                    pinfo->chr_attach = MAX_CHR;
                    pinfo->vrt_offset = GRIP_LAST;
                    pinfo->team       = pprt->team;
                    pinfo->chr_origin = prt_get_iowner( iprt, 0 );
                    pinfo->prt_origin = iprt;
                    pinfo->multispawn = tnc;
                    pinfo->oldtarget  = pprt->target_ref;
                };

                facing += ppip->endspawn_facingadd;
            }
        }

        //printf("\tcnt==%d,iprt==%d,free==%d\n", cnt, iprt, PrtList.free_count );

        // free the particle.
        free_one_particle_in_game( iprt );
        cnt++;
    }

    // delay the spawning of particles so that it dies not happen while we are scanning the
    // list of particles to be removed. That just confuses everything.
    for ( tnc = 0; tnc < delay_spawn_count && tnc < TOTAL_MAX_PRT; tnc++ )
    {
        spawn_particle_info_t * pinfo = delay_spawn_list + tnc;

        if ( !ACTIVE_PRT( pinfo->prt_origin ) ) pinfo->prt_origin = TOTAL_MAX_PRT;
        if ( !ACTIVE_CHR( pinfo->chr_origin ) ) pinfo->chr_origin = MAX_CHR;

        spawn_one_particle( pinfo->pos, pinfo->facing, pinfo->iprofile, pinfo->ipip,
                            pinfo->chr_attach, pinfo->vrt_offset, pinfo->team, pinfo->chr_origin,
                            pinfo->prt_origin, pinfo->multispawn, pinfo->oldtarget );
    }

}

//--------------------------------------------------------------------------------------------
void PrtList_free_all()
{
    /// @details ZZ@> This function resets the particle allocation lists

    int cnt;

    // free all the particles
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        PrtList_free_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void particle_system_init()
{
    /// @details ZZ@> This function sets up particle data
    int cnt;
    float x, y;

    // Image coordinates on the big particle bitmap
    for ( cnt = 0; cnt < MAXPARTICLEIMAGE; cnt++ )
    {
        x = cnt & 15;
        y = cnt >> 4;
        sprite_list_u[cnt][0] = ( float )(( 0.05f + x ) / 16.0f );
        sprite_list_u[cnt][1] = ( float )(( 0.95f + x ) / 16.0f );
        sprite_list_v[cnt][0] = ( float )(( 0.05f + y ) / 16.0f );
        sprite_list_v[cnt][1] = ( float )(( 0.95f + y ) / 16.0f );
    }

    // Reset the allocation table
    PrtList_init();

    init_all_pip();
}

//--------------------------------------------------------------------------------------------
int spawn_bump_particles( Uint16 character, Uint16 particle )
{
    /// @details ZZ@> This function is for catching characters on fire and such

    int cnt, bs_count;
    float x, y, z;
    Uint16 facing;
    Uint16 amount;
    Uint16 direction;
    float fsin, fcos;

    pip_t * ppip;
    chr_t * pchr;
    mad_t * pmad;
    prt_t * pprt;
    cap_t * pcap;

    if ( !ACTIVE_PRT( particle ) ) return 0;
    pprt = PrtList.lst + particle;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return 0;
    ppip = PipStack.lst + pprt->pip_ref;

    // no point in going on, is there?
    if ( 0 == ppip->bumpspawn_amount && !ppip->spawnenchant ) return 0;
    amount = ppip->bumpspawn_amount;

    if ( !ACTIVE_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return 0;

    pcap = pro_get_pcap( pchr->iprofile );
    if ( NULL == pcap ) return 0;

    bs_count = 0;

    // Only damage if hitting from proper direction
    direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
    direction = ATK_BEHIND + ( pchr->turn_z - direction );

    // Check that direction
    if ( !is_invictus_direction( direction, character, ppip->damfx ) )
    {
        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_one_enchant( pprt->owner_ref, character, MAX_CHR, MAX_ENC, pprt->profile_ref );
        }

        // Spawn particles - this has been modded to maximize the visual effect
        // on a given target. It is not the most optimal solution for lots of particles
        // spawning. Thst would probably be to make the distance calculations and then
        // to quicksort the list and choose the n closest points.
        //
        // however, it seems that the bump particles in game rarely attach more than
        // one bump particle
        if ( amount != 0 && !pcap->resistbumpspawn && !pchr->invictus && ( pchr->damagemodifier[pprt->damagetype]&DAMAGESHIFT ) < 3 )
        {
            int grip_verts, vertices;
            int slot_count;

            slot_count = 0;
            if ( pcap->slotvalid[SLOT_LEFT] ) slot_count++;
            if ( pcap->slotvalid[SLOT_RIGHT] ) slot_count++;

            if ( slot_count == 0 )
            {
                grip_verts = 1;  // always at least 1?
            }
            else
            {
                grip_verts = GRIP_VERTS * slot_count;
            }

            vertices = pchr->inst.vlst_size - grip_verts;
            vertices = MAX( 0, vertices );

            if ( vertices != 0 )
            {
                int    vertex_occupied[MAXVERTICES];
                float  vertex_distance[MAXVERTICES];
                float dist;

                // this could be done more easily with a quicksort....
                // but I guess it doesn't happen all the time

                dist = ABS( pprt->pos.x - pchr->pos.x ) + ABS( pprt->pos.y - pchr->pos.y ) + ABS( pprt->pos.z - pchr->pos.z );

                // clear the occupied list
                z = pprt->pos.z - pchr->pos.z;
                facing = pprt->facing - pchr->turn_z;
                fsin = turntosin[( facing >> 2 ) & TRIG_TABLE_MASK ];
                fcos = turntocos[( facing >> 2 ) & TRIG_TABLE_MASK ];
                x = dist * fcos;
                y = dist * fsin;

                // prepare the array values
                for ( cnt = 0; cnt < vertices; cnt++ )
                {
                    dist = ABS( x - pchr->inst.vlst[vertices-cnt-1].pos[XX] ) +
                           ABS( y - pchr->inst.vlst[vertices-cnt-1].pos[YY] ) +
                           ABS( z - pchr->inst.vlst[vertices-cnt-1].pos[ZZ] );

                    vertex_distance[cnt] = dist;
                    vertex_occupied[cnt] = TOTAL_MAX_PRT;
                }

                // determine if some of the vertex sites are already occupied
                for ( cnt = 0; cnt < maxparticles; cnt++ )
                {
                    prt_t * pprt;
                    if ( !ACTIVE_PRT( cnt ) ) continue;
                    pprt = PrtList.lst + cnt;

                    if ( character != pprt->attachedto_ref ) continue;

                    if ( pprt->vrt_off >= 0 && pprt->vrt_off < vertices )
                    {
                        vertex_occupied[pprt->vrt_off] = cnt;
                    }
                }

                // Find best vertices to attach the particles to
                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    Uint16 bs_part;
                    Uint32 bestdistance;
                    int    bestvertex;

                    bestvertex   = 0;
                    bestdistance = 0xFFFFFFFF;         //Really high number

                    for ( cnt = 0; cnt < vertices; cnt++ )
                    {
                        if ( vertex_occupied[cnt] != TOTAL_MAX_PRT )
                            continue;

                        if ( vertex_distance[cnt] < bestdistance )
                        {
                            bestdistance = vertex_distance[cnt];
                            bestvertex   = cnt;
                        }
                    }

                    bs_part = spawn_one_particle( pchr->pos, 0, pprt->profile_ref, ppip->bumpspawn_pip,
                                                  character, bestvertex + 1, pprt->team, pprt->owner_ref, particle, cnt, character );

                    if ( ACTIVE_PRT( bs_part ) )
                    {
                        vertex_occupied[bestvertex] = bs_part;
                        PrtList.lst[bs_part].is_bumpspawn = btrue;
                        bs_count++;
                    }
                }
                //}
                //else
                //{
                //    // Multiple particles are attached to character
                //    for ( cnt = 0; cnt < amount; cnt++ )
                //    {
                //        int irand = RANDIE;

                //        bs_part = spawn_one_particle( pchr->pos, 0, pprt->profile_ref, ppip->bumpspawn_pip,
                //                            character, irand % vertices, pprt->team, pprt->owner_ref, particle, cnt, character );

                //        if( ACTIVE_PRT(bs_part) )
                //        {
                //            PrtList.lst[bs_part].is_bumpspawn = btrue;
                //            bs_count++;
                //        }
                //    }
                //}
            }
        }
    }

    return bs_count;
}

//--------------------------------------------------------------------------------------------
int prt_is_over_water( Uint16 cnt )
{
    /// ZZ@> This function returns btrue if the particle is over a water tile
    Uint32 fan;

    if ( !ACTIVE_PRT( cnt ) ) return bfalse;

    fan = mesh_get_tile( PMesh, PrtList.lst[cnt].pos.x, PrtList.lst[cnt].pos.y );
    if ( VALID_TILE( PMesh, fan ) )
    {
        if ( 0 != mesh_test_fx( PMesh, fan, MPDFX_WATER ) )  return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 PipStack_get_free()
{
    Uint16 retval = MAX_PIP;

    if ( PipStack.count < MAX_PIP )
    {
        retval = PipStack.count;
        PipStack.count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int load_one_particle_profile( const char *szLoadName, Uint16 pip_override )
{
    /// @details ZZ@> This function loads a particle template, returning bfalse if the file wasn't
    ///    found

    Uint16  ipip;
    pip_t * ppip;

    ipip = MAX_PIP;
    if ( VALID_PIP_RANGE( pip_override ) )
    {
        release_one_pip( pip_override );
        ipip = pip_override;
    }
    else
    {
        ipip = PipStack_get_free();
    }

    if ( !VALID_PIP_RANGE( ipip ) )
    {
        return MAX_PIP;
    }
    ppip = PipStack.lst + ipip;

    if ( NULL == load_one_pip_file( szLoadName, ppip ) )
    {
        return MAX_PIP;
    }

    ppip->soundend = CLIP( ppip->soundend, INVALID_SOUND, MAX_WAVE );
    ppip->soundspawn = CLIP( ppip->soundspawn, INVALID_SOUND, MAX_WAVE );

    return ipip;
}

//--------------------------------------------------------------------------------------------
void reset_particles( /* const char* modname */ )
{
    /// @details ZZ@> This resets all particle data and reads in the coin and water particles

    char *loadpath;

    release_all_local_pips();
    release_all_pip();

    // Load in the standard global particles ( the coins for example )
    loadpath = "data/1money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN1 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "data/5money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN5 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "data/25money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN25 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "data/100money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_COIN100 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // Load module specific information
    loadpath = "data/weather4.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_WEATHER4 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "data/weather5.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_WEATHER5 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "data/splash.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_SPLASH ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "data/ripple.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_RIPPLE ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // This is also global...
    loadpath = "data/defend.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath, PIP_DEFEND ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    PipStack.count = GLOBAL_PIP_COUNT;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint16  prt_get_ipip( Uint16 iprt )
{
    prt_t * pprt;

    if ( !ACTIVE_PRT( iprt ) ) return MAX_PIP;
    pprt = PrtList.lst + iprt;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return MAX_PIP;

    return pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
pip_t * prt_get_ppip( Uint16 iprt )
{
    prt_t * pprt;

    if ( !ACTIVE_PRT( iprt ) ) return NULL;
    pprt = PrtList.lst + iprt;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return NULL;

    return PipStack.lst + pprt->pip_ref;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
void init_all_pip()
{
    Uint16 cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        pip_init( PipStack.lst + cnt );
    }

    // Reset the pip stack "pointer"
    PipStack.count = 0;
}

//--------------------------------------------------------------------------------------------
void release_all_pip()
{
    int cnt, tnc;
    int max_request;

    max_request = 0;
    for ( cnt = 0, tnc = 0; cnt < MAX_PIP; cnt++ )
    {
        if ( LOADED_PIP( cnt ) )
        {
            pip_t * ppip = PipStack.lst + cnt;

            max_request = MAX( max_request, ppip->prt_request_count );
            tnc++;
        }
    }

    if ( tnc > 0 && max_request > 0 )
    {
        FILE * ftmp = EGO_fopen( "pip_usage.txt", "w" );
        if ( ftmp != NULL )
        {
            fprintf( ftmp, "List of used pips\n\n" );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                if ( LOADED_PIP( cnt ) )
                {
                    pip_t * ppip = PipStack.lst + cnt;
                    fprintf( ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n", cnt, ppip->name, ppip->prt_create_count, ppip->prt_request_count );
                }
            }

            EGO_fflush( ftmp );

            EGO_fclose( ftmp );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                release_one_pip( cnt );
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
bool_t release_one_pip( Uint16 ipip )
{
    pip_t * ppip;

    if ( !VALID_PIP_RANGE( ipip ) ) return bfalse;
    ppip = PipStack.lst + ipip;

    if ( !ppip->loaded ) return btrue;

    pip_init( ppip );

    ppip->loaded  = bfalse;
    ppip->name[0] = CSTR_END;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_request_terminate( Uint16 iprt )
{
    /// @details BB@> Tell the game to get rid of this object and treat it
    ///               as if it was already dead

    /// @note prt_request_terminate() will call force the game to
    ///       (eventually) call free_one_particle_in_game() on this particle

    if ( !ACTIVE_PRT( iprt ) ) return bfalse;

    EGO_OBJECT_REQUST_TERMINATE( PrtList.lst + iprt );

    return btrue;
}

//--------------------------------------------------------------------------------------------
// This is zefz version of the update_all_particles() code with less loops
// is still ahs the problem of not updating the zombie/limbo
// particles ( DISPLAY_PRT() vs. ACTIVE_PRT() )
// also I added one more fix to the use of prt_child vs. particle
//void update_all_particles()
//{
//    /// @details BB@> update everything about a particle that does not depend on collisions
//    ///               or interactions with characters
//    Uint16 particle;
//
//    //Go through every particle in the game and do the appropiate updates
//    for ( particle = 0; particle < maxparticles; particle++ )
//    {
//        prt_t * pprt;
//        pip_t * ppip;
//      int size_new;
//      bool_t inwater;
//      Uint16 facing;
//
//        if ( !ACTIVE_PRT(particle) ) continue;
//        pprt = PrtList.lst + particle;
//
//        ppip = prt_get_ppip( particle );
//        if( NULL == ppip ) continue;
//
//      // figure out where the particle is on the mesh and update particle states
//        pprt->onwhichfan   = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
//        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );
//
//        // update various particle states
//        if ( ACTIVE_CHR( pprt->attachedto_ref ) )
//        {
//            pprt->is_hidden = ChrList.lst[pprt->attachedto_ref].is_hidden;
//        }
//      else pprt->is_hidden = bfalse;
//        pprt->is_homing = ppip->homing && !ACTIVE_CHR( pprt->attachedto_ref );
//
//        // stop here if the particle is hidden
//        if( pprt->is_hidden ) continue;
//
//        // do the particle interaction with water
//        inwater = (pprt->pos.z < water.surface_level) && (0 != mesh_test_fx( PMesh, pprt->onwhichfan, MPDFX_WATER ));
//
//        if( inwater && water.is_water && ppip->endwater )
//        {
//            // Check for disaffirming character
//            if ( ACTIVE_CHR( pprt->attachedto_ref ) && pprt->owner_ref == pprt->attachedto_ref )
//            {
//                // Disaffirm the whole character
//                disaffirm_attached_particles( pprt->attachedto_ref );
//            }
//            else
//            {
//                // destroy the particle
//                prt_request_terminate( particle );
//              continue;
//            }
//        }
//        else if ( inwater )
//        {
//            bool_t spawn_valid = bfalse;
//            Uint16 spawn_pip   = MAX_PIP;
//            fvec3_t   vtmp = VECT3( pprt->pos.x, pprt->pos.y, water.surface_level );
//
//            if ( !pprt->inwater )
//            {
//                spawn_valid = btrue;
//
//                if( SPRITE_SOLID == pprt->type )
//                {
//                    spawn_pip = PIP_SPLASH;
//                }
//                else
//                {
//                    spawn_pip = PIP_RIPPLE;
//                }
//            }
//            else
//            {
//                if( SPRITE_SOLID == pprt->type && !ACTIVE_CHR( pprt->attachedto_ref ) )
//                {
//                    // only spawn ripples if you are touching the water surface!
//                    if( pprt->pos.z + pprt->bumpheight > water.surface_level && pprt->pos.z - pprt->bumpheight < water.surface_level )
//                    {
//                        int ripand = ~((~RIPPLEAND) << 1);
//                        if ( 0 == ((update_wld + pprt->obj_base.guid) & ripand) )
//                        {
//
//                            spawn_valid = btrue;
//                            spawn_pip = PIP_RIPPLE;
//                        }
//                    }
//                }
//            }
//
//            if( spawn_valid )
//            {
//                // Splash for particles is just a ripple
//                spawn_one_particle( vtmp, 0, MAX_PROFILE, spawn_pip, MAX_CHR, GRIP_LAST,
//                                    TEAM_NULL, MAX_CHR, TOTAL_MAX_PRT, 0, MAX_CHR );
//            }
//
//            pprt->inwater  = btrue;
//        }
//        else
//        {
//            pprt->inwater = bfalse;
//        }
//
//      // the following functions should not be done the first time through the update loop
//      if( 0 == clock_wld ) continue;
//
//      // Animate particle
//      pprt->image = pprt->image + pprt->imageadd;
//      if ( pprt->image >= pprt->imagemax ) pprt->image = 0;
//
//      // rotate the particle
//      pprt->rotate += pprt->rotateadd;
//
//      // update the particle size
//      if( 0 != pprt->size_add )
//      {
//          // resize the paricle
//          size_new =  pprt->size_add + pprt->size;
//          pprt->size = CLIP(size_new, 0, 0xFFFF);
//
//          /*if( pprt->type != SPRITE_SOLID && pprt->inst.alpha != 0.0f )
//          {
//              // adjust the particle alpha
//              if( size_new > 0 )
//              {
//                  float ftmp = 1.0f - (float)ABS(pprt->size_add) / (float)size_new;
//                  pprt->inst.alpha *= ftmp;
//              }
//              else
//              {
//                  pprt->inst.alpha = 0xFF;
//              }
//          }*/
//      }
//
//      // down the spawn timer
//      if ( pprt->spawntime > 0 ) pprt->spawntime--;
//
//      // Spawn new particles if continually spawning
//      if ( 0 == pprt->spawntime && ppip->contspawn_amount > 0 )
//      {
//          Uint8 tnc;
//
//          // reset the spawn timer
//          pprt->spawntime = ppip->contspawn_time;
//
//          facing = pprt->facing;
//          for ( tnc = 0; tnc < ppip->contspawn_amount; tnc++ )
//          {
//              Uint16 prt_child;
//              prt_child = spawn_one_particle( pprt->pos, facing, pprt->profile_ref, ppip->contspawn_pip,
//                  MAX_CHR, GRIP_LAST, pprt->team, pprt->owner_ref, particle, tnc, pprt->target_ref );
//
//              if ( ppip->facingadd != 0 && ACTIVE_PRT(prt_child) )
//              {
//                  // Hack to fix velocity
//                  PrtList.lst[prt_child].vel.x += pprt->vel.x;
//                  PrtList.lst[prt_child].vel.y += pprt->vel.y;
//              }
//              facing += ppip->contspawn_facingadd;
//          }
//      }
//
//      // Change dyna light values
//      if( pprt->dynalight.level > 0 )
//      {
//          pprt->dynalight.level   += ppip->dynalight.level_add;
//          if( pprt->dynalight.level < 0 ) pprt->dynalight.level = 0;
//      }
//      else if( pprt->dynalight.level < 0 )
//      {
//          // try to guess what should happen for negative lighting
//          pprt->dynalight.level   += ppip->dynalight.level_add;
//          if( pprt->dynalight.level > 0 ) pprt->dynalight.level = 0;
//      }
//      else
//      {
//          pprt->dynalight.level += ppip->dynalight.level_add;
//      }
//
//      pprt->dynalight.falloff += ppip->dynalight.falloff_add;
//
//      // spin the particle
//      pprt->facing += ppip->facingadd;
//    }
//
//    // apply damage from  attatched bump particles (about once a second)
//    if ( 0 == ( update_wld & 31 ) )
//    {
//        for ( particle = 0; particle < maxparticles; particle++ )
//        {
//            prt_t * pprt;
//            pip_t * ppip;
//            Uint16 ichr;
//
//            if ( !ACTIVE_PRT(particle) ) continue;
//            pprt = PrtList.lst + particle;
//
//            // do nothing if the particle is hidden
//            if( pprt->is_hidden ) continue;
//
//            // is this is not a damage particle for me?
//            if( pprt->attachedto_ref == pprt->owner_ref ) continue;
//
//            ppip = prt_get_ppip( particle );
//            if( NULL == ppip ) continue;
//
//            ichr = pprt->attachedto_ref;
//            if( !ACTIVE_CHR( ichr ) ) continue;
//
//            // Attached iprt_b damage ( Burning )
//            if ( ppip->allowpush && ppip->xyvel_pair.base == 0 )
//            {
//                // Make character limp
//                ChrList.lst[ichr].vel.x *= 0.5f;
//                ChrList.lst[ichr].vel.y *= 0.5f;
//            }
//
//            damage_character( ichr, ATK_BEHIND, pprt->damage, pprt->damagetype, pprt->team, pprt->owner_ref, ppip->damfx, bfalse );
//        }
//    }
//}