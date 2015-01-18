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

/// @file  game/particle.c
/// @brief Manages particle systems.

#include "game/particle.h"
#include "game/audio/AudioSystem.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "game/game.h"
#include "game/mesh.h"
#include "game/obj_BSP.h"
#include "game/mesh_functions.h"
#include "game/mad.h"
#include "game/renderer_3d.h"
#include "game/egoboo.h"
#include "game/mesh.h"
#include "game/enchant.h"
#include "game/profiles/Profile.hpp"
#include "game/PrtList.h"
#include "game/ChrList.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PRT_TRANS 0x80

const float buoyancy_friction = 0.2f;          // how fast does a "cloud-like" object slow down?

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_stoppedby_tests = 0;
int prt_pressure_tests = 0;

Stack<pip_t, MAX_PIP> PipStack;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool  prt_free( prt_t * pprt );

static prt_t * prt_config_ctor( prt_t * pprt );
static prt_t * prt_config_init( prt_t * pprt );
static prt_t * prt_config_active( prt_t * pprt );
static prt_t * prt_config_deinit( prt_t * pprt );
static prt_t * prt_config_dtor( prt_t * pprt );

static prt_t * prt_config_do_init( prt_t * pprt );
static prt_t * prt_config_do_active( prt_t * pprt );
static prt_t * prt_config_do_deinit( prt_t * pprt );

static int prt_do_end_spawn( const PRT_REF iprt );
static int prt_do_contspawn( prt_bundle_t * pbdl_prt );
static prt_bundle_t * prt_do_bump_damage( prt_bundle_t * pbdl_prt );

static prt_bundle_t * prt_update_animation( prt_bundle_t * pbdl_prt );
static prt_bundle_t * prt_update_dynalight( prt_bundle_t * pbdl_prt );
static prt_bundle_t * prt_update_timers( prt_bundle_t * pbdl_prt );
static prt_bundle_t * prt_update_do_water( prt_bundle_t * pbdl_prt );
static prt_bundle_t * prt_update_ingame( prt_bundle_t * pbdl_prt );
static prt_bundle_t * prt_update_ghost( prt_bundle_t * pbdl_prt );
static prt_bundle_t * prt_update( prt_bundle_t * pbdl_prt );

static bool prt_update_pos( prt_t * pprt );
static bool prt_update_safe( prt_t * pprt, bool force );
static bool prt_update_safe_raw( prt_t * pprt );
static PIP_REF PipStack_get_free();
static bool move_one_particle( prt_bundle_t * pbdl_prt );
static prt_bundle_t * move_one_particle_integrate_motion( prt_bundle_t * pbdl_prt );
static prt_bundle_t * move_one_particle_integrate_motion_attached( prt_bundle_t * pbdl_prt );
static prt_bundle_t * move_one_particle_do_z_motion( prt_bundle_t * pbdl_prt );
static prt_bundle_t * move_one_particle_do_homing( prt_bundle_t * pbdl_prt );
static prt_bundle_t * move_one_particle_do_floor_friction( prt_bundle_t * pbdl_prt );
static prt_bundle_t * move_one_particle_do_fluid_friction( prt_bundle_t * pbdl_prt );
//static fvec2_t prt_get_mesh_diff( prt_t * pprt, float test_pos[], float center_pressure );
//static float prt_get_mesh_pressure( prt_t * pprt, float test_pos[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool prt_free( prt_t * pprt )
{
    if ( !ALLOCATED_PPRT( pprt ) ) return false;

    // do not allow this if you are inside a particle loop
    EGOBOO_ASSERT( 0 == prt_loop_depth );

    if ( TERMINATED_PPRT( pprt ) ) return true;

    // deallocate any dynamic data

    return true;
}

//--------------------------------------------------------------------------------------------
/// @brief Set all particle parameters to safe values.
/// @details The C equivalent of a parameterless constructor.
prt_t * prt_ctor( prt_t * pprt )
{
    // save the base object data, do not construct it with this function.
    if ( NULL == pprt ) return NULL;
    Ego::Entity *base_ptr = POBJ_GET_PBASE( pprt );

	Ego::Entity save_base;
    memcpy( &save_base, base_ptr, sizeof( save_base ) );

    BLANK_STRUCT_PTR( pprt )

    // restore the base object data
    memcpy( base_ptr, &save_base, sizeof( save_base ) );

    // reset the base counters
    base_ptr->update_count = 0;
    base_ptr->frame_count = 0;

    // "no lifetime" = "eternal"
    pprt->lifetime_total     = ( size_t )( ~0 );
    pprt->lifetime_remaining = pprt->lifetime_total;
    pprt->frames_total       = ( size_t )( ~0 );
    pprt->frames_remaining   = pprt->frames_total;

    pprt->pip_ref      = INVALID_PIP_REF;
    pprt->profile_ref  = INVALID_PRO_REF;

    pprt->attachedto_ref = INVALID_CHR_REF;
    pprt->owner_ref      = INVALID_CHR_REF;
    pprt->target_ref     = INVALID_CHR_REF;
    pprt->parent_ref     = INVALID_PRT_REF;
    pprt->parent_guid    = 0xFFFFFFFF;

    pprt->onwhichplatform_ref    = INVALID_CHR_REF;
    pprt->onwhichplatform_update = 0;
    pprt->targetplatform_ref     = INVALID_CHR_REF;

    // initialize the bsp node for this particle
    POBJ_GET_PLEAF(pprt)->ctor(pprt, BSP_LEAF_PRT, GET_INDEX_PPRT(pprt));

    // initialize the physics
    phys_data_ctor( &( pprt->phys ) );

    pprt->obj_base.state = Ego::Entity::State::Initializing;

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_dtor( prt_t * pprt )
{
    if ( NULL == pprt ) return pprt;

    // destruct/free any allocated data
    prt_free( pprt );

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    POBJ_TERMINATE( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void prt_play_sound( const PRT_REF particle, Sint8 sound )
{
    /// @author ZZ
    /// @details This function plays a sound effect for a particle

    prt_t * pprt;

    if ( !DEFINED_PRT( particle ) ) return;
    pprt = PrtList_get_ptr( particle );

    if ( _profileSystem.isValidProfileID( pprt->profile_ref ) )
    {
        _audioSystem.playSound( pprt->pos, _profileSystem.getProfile(pprt->profile_ref)->getSoundID(sound) );
    }
    else if (sound >= 0 && sound < GSND_COUNT)
    {
        GlobalSound globalSound = static_cast<GlobalSound>(sound);
        _audioSystem.playSound(pprt->pos, _audioSystem.getGlobalSound(globalSound));
    }
}

//--------------------------------------------------------------------------------------------
PRT_REF end_one_particle_now( const PRT_REF particle )
{
    // this turns the particle into a ghost

    PRT_REF retval;

    if ( !ALLOCATED_PRT( particle ) ) return INVALID_PRT_REF;

    retval = particle;
    if ( PrtList_request_terminate( particle ) )
    {
        retval = INVALID_PRT_REF;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
PRT_REF end_one_particle_in_game( const PRT_REF particle )
{
    /// @author ZZ
    /// @details this function causes the game to end a particle
    ///               and mark it as a ghost.

    CHR_REF child;

    // does the particle have valid data?
    if ( DEFINED_PRT( particle ) )
    {
        prt_t * pprt = PrtList_get_ptr( particle );
        pip_t * ppip = prt_get_ppip( particle );

        // the object is waiting to be killed, so
        // do all of the end of life care for the particle
        prt_do_end_spawn( particle );

        if ( SPAWNNOCHARACTER != pprt->endspawn_characterstate )
        {
            child = spawn_one_character(prt_get_pos_v_const(pprt), pprt->profile_ref, pprt->team, 0, pprt->facing, NULL, INVALID_CHR_REF );
            if ( DEFINED_CHR( child ) )
            {
                chr_t * pchild = ChrList_get_ptr( child );

                chr_set_ai_state( pchild , pprt->endspawn_characterstate );
                pchild->ai.owner = pprt->owner_ref;
            }
        }

        //Play end sound
        if ( NULL != ppip )
        {
            prt_play_sound( particle, ppip->end_sound );
        }
    }

    return end_one_particle_now( particle );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

prt_t * prt_config_do_init( prt_t * pprt )
{
    const int INFINITE_UPDATES = INT32_MAX;

    PRT_REF            iprt;
    pip_t            * ppip;
    prt_spawn_data_t * pdata;

    int     velocity;
    fvec3_t vel;
    float   tvel;
    int     offsetfacing = 0, newrand;
    fvec3_t tmp_pos;
    TURN_T  turn;
    float   loc_spdlimit;
    int     prt_life_frames_updates, prt_anim_frames_updates;
    bool  prt_life_infinite, prt_anim_infinite;

    FACING_T loc_facing;
    CHR_REF loc_chr_origin;

    if ( NULL == pprt ) return NULL;
    pdata = &( pprt->spawn_data );
    iprt  = GET_INDEX_PPRT( pprt );

    // Convert from local pdata->ipip to global pdata->ipip
    if ( !LOADED_PIP( pdata->ipip ) )
    {
        log_debug( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                   REF_TO_INT( pdata->ipip ), REF_TO_INT( pdata->chr_origin ), DEFINED_CHR( pdata->chr_origin ) ? ChrList.lst[pdata->chr_origin].Name : "INVALID",
                   REF_TO_INT( pdata->iprofile ), _profileSystem.isValidProfileID( pdata->iprofile ) ? _profileSystem.getProfile(pdata->iprofile)->getFilePath().c_str() : "INVALID" );

        return NULL;
    }
    ppip = PipStack.get_ptr( pdata->ipip );

    // let the object be activated
    POBJ_ACTIVATE( pprt, ppip->name );

    // make some local copies of the spawn data
    loc_facing     = pdata->facing;

    // Save a version of the position for local use.
    // In cpp, will be passed by reference, so we do not want to alter the
    // components of the original vector.
    tmp_pos = pdata->pos;

    // try to get an idea of who our owner is even if we are
    // given bogus info
    loc_chr_origin = pdata->chr_origin;
    if ( !DEFINED_CHR( pdata->chr_origin ) && DEFINED_PRT( pdata->prt_origin ) )
    {
        loc_chr_origin = prt_get_iowner( pdata->prt_origin, 0 );
    }

    pprt->pip_ref     = pdata->ipip;
    pprt->profile_ref = pdata->iprofile;
    pprt->team        = pdata->team;
    pprt->owner_ref   = loc_chr_origin;
    pprt->parent_ref  = pdata->prt_origin;
    pprt->parent_guid = ALLOCATED_PRT( pdata->prt_origin ) ? PrtList.lst[pdata->prt_origin].obj_base.guid : (( Uint32 )( ~0 ) );
    pprt->damagetype  = ppip->damagetype;
    pprt->lifedrain   = ppip->lifedrain;
    pprt->manadrain   = ppip->manadrain;

    // Lighting and sound
    pprt->dynalight    = ppip->dynalight;
    pprt->dynalight.on = false;
    if ( 0 == pdata->multispawn )
    {
        pprt->dynalight.on = ppip->dynalight.mode;
        if ( DYNA_MODE_LOCAL == ppip->dynalight.mode )
        {
            pprt->dynalight.on = DYNA_MODE_OFF;
        }
    }

    // Set character attachments ( pdata->chr_attach == INVALID_CHR_REF means none )
    pprt->attachedto_ref     = pdata->chr_attach;
    pprt->attachedto_vrt_off = pdata->vrt_offset;

    // Correct loc_facing
    loc_facing = loc_facing + ppip->facing_pair.base;

    // Targeting...
    vel.z = 0;

    pprt->offset.z = generate_irand_pair( ppip->spacing_vrt_pair ) - ( ppip->spacing_vrt_pair.rand / 2 );
    tmp_pos.z += pprt->offset.z;
    velocity = generate_irand_pair( ppip->vel_hrz_pair );
    pprt->target_ref = pdata->oldtarget;
    if ( ppip->newtargetonspawn )
    {
        if ( ppip->targetcaster )
        {
            // Set the target to the caster
            pprt->target_ref = loc_chr_origin;
        }
        else
        {
            const int PERFECT_AIM = 45 * 256;   // 45 dex is perfect aim

            // Find a target
            pprt->target_ref = prt_find_target( pdata->pos, loc_facing, pdata->ipip, pdata->team, loc_chr_origin, pdata->oldtarget );
            if ( DEFINED_CHR( pprt->target_ref ) && !ppip->homing )
            {
                /// @note ZF@> ?What does this do?!
                /// @note BB@> glouseangle is the angle found in prt_find_target()
                loc_facing -= glouseangle;
            }

            // Correct loc_facing for dexterity...
            offsetfacing = 0;
            if ( ChrList.lst[loc_chr_origin].dexterity < PERFECT_AIM )
            {
                // Correct loc_facing for randomness
                offsetfacing  = generate_irand_pair( ppip->facing_pair ) - ( ppip->facing_pair.base + ppip->facing_pair.rand / 2 );
                offsetfacing  = ( offsetfacing * ( PERFECT_AIM - ChrList.lst[loc_chr_origin].dexterity ) ) / PERFECT_AIM;
            }

            if ( 0.0f != ppip->zaimspd )
            {
                if ( DEFINED_CHR( pprt->target_ref ) )
                {
                    // These aren't velocities...  This is to do aiming on the Z axis
                    if ( velocity > 0 )
                    {
                        vel.x = ChrList.lst[pprt->target_ref].pos.x - pdata->pos.x;
                        vel.y = ChrList.lst[pprt->target_ref].pos.y - pdata->pos.y;
                        tvel = std::sqrt( vel.x * vel.x + vel.y * vel.y ) / velocity;  // This is the number of steps...
                        if ( tvel > 0.0f )
                        {
                            // This is the vel.z alteration
                            vel.z = ( ChrList.lst[pprt->target_ref].pos.z + ( ChrList.lst[pprt->target_ref].bump.height * 0.5f ) - tmp_pos.z ) / tvel;
                        }
                    }
                }
                else
                {
                    vel.z = 0.5f * ppip->zaimspd;
                }

                vel.z = CLIP( vel.z, -0.5f * ppip->zaimspd, ppip->zaimspd );
            }
        }

        // Does it go away?
        if ( !DEFINED_CHR( pprt->target_ref ) && ppip->needtarget )
        {
            end_one_particle_in_game( iprt );
            return NULL;
        }

        // Start on top of target
        if ( DEFINED_CHR( pprt->target_ref ) && ppip->startontarget )
        {
            tmp_pos.x = ChrList.lst[pprt->target_ref].pos.x;
            tmp_pos.y = ChrList.lst[pprt->target_ref].pos.y;
        }
    }
    else
    {
        // Correct loc_facing for randomness
        offsetfacing = generate_irand_pair( ppip->facing_pair ) - ( ppip->facing_pair.base + ppip->facing_pair.rand / 2 );
    }
    loc_facing   = loc_facing + offsetfacing;
    pprt->facing = loc_facing;

    // this is actually pointing in the opposite direction?
    turn = TO_TURN( loc_facing );

    // Location data from arguments
    newrand = generate_irand_pair( ppip->spacing_hrz_pair );
    pprt->offset.x = -turntocos[ turn ] * newrand;
    pprt->offset.y = -turntosin[ turn ] * newrand;

    tmp_pos.x += pprt->offset.x;
    tmp_pos.y += pprt->offset.y;

    tmp_pos.x = CLIP( tmp_pos.x, 0.0f, PMesh->gmem.edge_x - 2.0f );
    tmp_pos.y = CLIP( tmp_pos.y, 0.0f, PMesh->gmem.edge_y - 2.0f );

    prt_set_pos(pprt, tmp_pos);
    pprt->pos_old  = tmp_pos;
    pprt->pos_stt  = tmp_pos;

    // Velocity data
    vel.x = -turntocos[ turn ] * velocity;
    vel.y = -turntosin[ turn ] * velocity;
    vel.z += generate_irand_pair( ppip->vel_vrt_pair ) - ( ppip->vel_vrt_pair.rand / 2 );
    pprt->vel = pprt->vel_old = pprt->vel_stt = vel;

    // Template values
    pprt->bump_size_stt = ppip->bump_size;
    pprt->type          = ppip->type;

    // Image data
    pprt->rotate        = ( FACING_T )generate_irand_pair( ppip->rotate_pair );
    pprt->rotate_add    = ppip->rotate_add;

    pprt->size_stt      = ppip->size_base;
    pprt->size_add      = ppip->size_add;

    pprt->image_stt     = UINT_TO_UFP8( ppip->image_base );
    pprt->image_add     = generate_irand_pair( ppip->image_add );
    pprt->image_max     = UINT_TO_UFP8( ppip->numframes );

    // a particle can EITHER end_lastframe or end_time.
    // if it ends after the last frame, end_time tells you the number of cycles through
    // the animation
    prt_anim_frames_updates  = 0;
    prt_anim_infinite = false;
    if ( ppip->end_lastframe )
    {
        if ( 0 == pprt->image_add )
        {
            prt_anim_frames_updates = INFINITE_UPDATES;
            prt_anim_infinite = true;
        }
        else
        {
            prt_anim_frames_updates = CEIL(( float )pprt->image_max / ( float )pprt->image_add ) - 1;

            if ( ppip->end_time > 0 )
            {
                // Part time is used to give number of cycles
                prt_anim_frames_updates *= ppip->end_time;
            }
        }
    }
    else
    {
        // no end to the frames
        prt_anim_frames_updates  = INFINITE_UPDATES;
        prt_anim_infinite = true;
    }
    prt_anim_frames_updates = std::max( 1, prt_anim_frames_updates );

    // estimate the number of frames
    prt_life_frames_updates  = 0;
    prt_life_infinite = false;
    if ( ppip->end_lastframe )
    {
        // for end last frame, the lifetime is given by the
        prt_life_frames_updates  = prt_anim_frames_updates;
        prt_life_infinite = prt_anim_infinite;
    }
    else if ( ppip->end_time <= 0 )
    {
        // zero or negative lifetime == infinite lifetime
        prt_life_frames_updates = INFINITE_UPDATES;
        prt_life_infinite = true;
    }
    else
    {
        prt_life_frames_updates = ppip->end_time;
    }
    prt_life_frames_updates = std::max( 1, prt_life_frames_updates );

    // set lifetime counter
    if ( prt_life_infinite )
    {
        pprt->lifetime_total = ( size_t )( ~0 );
        pprt->is_eternal     = true;
    }
    else
    {
        // the lifetime is really supposed tp be in terms of frames, but
        // to keep the number of updates stable, the frames could lag.
        // sooo... we just rescale the prt_life_frames_updates so that it will work with the
        // updates and cross our fingers
        pprt->lifetime_total     = CEIL(( float ) prt_life_frames_updates * ( float )TARGET_UPS / ( float )TARGET_FPS );
    }
    // make the particle exists for AT LEAST one update
    pprt->lifetime_total     = std::max( (size_t)1, pprt->lifetime_total );
    pprt->lifetime_remaining = pprt->lifetime_total;

    // set the frame counters
    // make the particle display AT LEAST one frame, regardless of how many updates
    // it has or when someone requests for it to terminate
    pprt->frames_total       = std::max( 1, prt_anim_frames_updates );
    pprt->lifetime_remaining = pprt->lifetime_total;

    // Damage stuff
    range_to_pair( ppip->damage, &( pprt->damage ) );

    // Spawning data
    pprt->contspawn_timer = ppip->contspawn_delay;
    if ( 0 != pprt->contspawn_timer )
    {
        pprt->contspawn_timer = 1;
        if ( DEFINED_CHR( pprt->attachedto_ref ) )
        {
            pprt->contspawn_timer++; // Because attachment takes an update before it happens
        }
    }

    // the end-spawn data. determine the
    pprt->endspawn_amount    = ppip->endspawn_amount;
    pprt->endspawn_facingadd = ppip->endspawn_facingadd;
    pprt->endspawn_lpip      = ppip->endspawn_lpip;

    // set up the particle transparency
    pprt->inst.alpha = 0xFF;
    switch ( pprt->inst.type )
    {
        case SPRITE_SOLID: break;
        case SPRITE_ALPHA: pprt->inst.alpha = PRT_TRANS; break;
        case SPRITE_LIGHT: break;
    }

    // is the spawn location safe?
    if ( 0 == prt_hit_wall( pprt, tmp_pos.v, NULL, NULL, NULL ) )
    {
        pprt->safe_pos   = tmp_pos;
        pprt->safe_valid = true;
        pprt->safe_grid  = pprt->onwhichgrid;
    }

    // get an initial value for the is_homing variable
    pprt->is_homing = ppip->homing && !DEFINED_CHR( pprt->attachedto_ref );

    //enable or disable gravity
    pprt->no_gravity = ppip->ignore_gravity;

    // estimate some parameters for buoyancy and air resistance
    loc_spdlimit = ppip->spdlimit;

    {
        const float buoyancy_min       = 0.0f;
        const float buoyancy_max       = 2.0f * ABS( STANDARD_GRAVITY );
        const float air_resistance_min = 0.0f;
        const float air_resistance_max = 1.0f;

        // find the buoyancy, assuming that the air_resistance of the particle
        // is equal to air_friction at standard gravity
        pprt->buoyancy = -loc_spdlimit * ( 1.0f - air_friction ) - STANDARD_GRAVITY;
        pprt->buoyancy = CLIP( pprt->buoyancy, buoyancy_min, buoyancy_max );

        // reduce the buoyancy if the particle falls
        if ( loc_spdlimit > 0.0f ) pprt->buoyancy *= 0.5f;

        // determine if there is any left-over air resistance
        if ( std::abs(loc_spdlimit) > 0.0001f )
        {
            pprt->air_resistance  = 1.0f - ( pprt->buoyancy + STANDARD_GRAVITY ) / -loc_spdlimit;
            pprt->air_resistance = CLIP( pprt->air_resistance, air_resistance_min, air_resistance_max );
            
            pprt->air_resistance /= air_friction;
            pprt->air_resistance = CLIP( pprt->air_resistance, 0.0f, 1.0f );
        }
        else
        {
            pprt->air_resistance = 0.0f;
        }
    }

    pprt->endspawn_characterstate = SPAWNNOCHARACTER;

    prt_set_size( pprt, ppip->size_base );

#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)

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
               update_wld, pprt->lifetime, game_frame_all, pprt->safe_time,
               loc_chr_origin, DEFINED_CHR( loc_chr_origin ) ? ChrList.lst[loc_chr_origin].Name : "INVALID",
               pdata->ipip, ( NULL != ppip ) ? ppip->name : "INVALID", ( NULL != ppip ) ? ppip->comment : "",
               pdata->iprofile, _profileSystem.isValidProfileID( pdata->iprofile ) ? ProList.lst[pdata->iprofile].name : "INVALID" );
#endif

    if ( INVALID_CHR_REF != pprt->attachedto_ref )
    {
        prt_bundle_t prt_bdl;

        prt_bundle_set( &prt_bdl, pprt );

        attach_one_particle( &prt_bdl );
    }

    // Sound effect
    prt_play_sound(iprt, ppip->soundspawn);

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_do_active( prt_t * pprt )
{
    // is there ever a reason to change the state?

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_do_deinit( prt_t * pprt )
{
    if ( NULL == pprt ) return pprt;

    // go to the next state
    pprt->obj_base.state = Ego::Entity::State::Destructing;

    return pprt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_t * prt_config_construct( prt_t * pprt, int max_iterations )
{
    if ( NULL == pprt ) return NULL;

    Ego::Entity *base_ptr = POBJ_GET_PBASE( pprt );
    if ( !base_ptr->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( base_ptr->state > ( int )( Ego::Entity::State::Constructing + 1 ) )
    {
        prt_t * tmp_prt = prt_config_deconstruct( pprt, max_iterations );
        if ( tmp_prt == pprt ) return NULL;
    }

    int iterations = 0;
    while ( NULL != pprt && base_ptr->state <= Ego::Entity::Constructing && iterations < max_iterations )
    {
        prt_t * ptmp = prt_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_initialize( prt_t * pprt, int max_iterations )
{
    if ( NULL == pprt ) return NULL;

    Ego::Entity *base_ptr = POBJ_GET_PBASE( pprt );
    if ( !base_ptr->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( base_ptr->state > ( int )( Ego::Entity::State::Initializing + 1 ) )
    {
        prt_t * tmp_prt = prt_config_deconstruct( pprt, max_iterations );
        if ( tmp_prt == pprt ) return NULL;
    }

    int iterations = 0;
    while ( NULL != pprt && base_ptr->state <= Ego::Entity::State::Initializing && iterations < max_iterations )
    {
        prt_t * ptmp = prt_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_activate( prt_t * pprt, int max_iterations )
{
    if ( NULL == pprt ) return NULL;

    Ego::Entity *base_ptr = POBJ_GET_PBASE( pprt );
    if ( !base_ptr->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( base_ptr->state > ( int )( Ego::Entity::State::Active + 1 ) )
    {
        prt_t * tmp_prt = prt_config_deconstruct( pprt, max_iterations );
        if ( tmp_prt == pprt ) return NULL;
    }

	int iterations = 0;
    while ( NULL != pprt && base_ptr->state < Ego::Entity::State::Active && iterations < max_iterations )
    {
        prt_t * ptmp = prt_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

    EGOBOO_ASSERT( base_ptr->state == Ego::Entity::State::Active );
    if ( base_ptr->state == Ego::Entity::State::Active )
    {
        PrtList_push_used( GET_INDEX_PPRT( pprt ) );
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_deinitialize( prt_t * pprt, int max_iterations )
{
    if ( NULL == pprt ) return NULL;
    Ego::Entity *base_ptr = POBJ_GET_PBASE( pprt );

    if ( !base_ptr->allocated ) return NULL;

    // if the particle is already beyond this stage, deinitialize it
    if ( base_ptr->state > ( int )( Ego::Entity::State::DeInitializing + 1 ) )
    {
        return pprt;
    }
    else if ( base_ptr->state < Ego::Entity::State::DeInitializing )
    {
        base_ptr->state = Ego::Entity::State::DeInitializing;
    }

	int iterations = 0;
    while ( NULL != pprt && base_ptr->state <= Ego::Entity::State::DeInitializing && iterations < max_iterations )
    {
        prt_t * ptmp = prt_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_deconstruct( prt_t * pprt, int max_iterations )
{
    int          iterations;
    Ego::Entity *base_ptr;

    if ( NULL == pprt ) return NULL;
    base_ptr = POBJ_GET_PBASE( pprt );

    if ( !base_ptr->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it
    if ( base_ptr->state > ( int )( Ego::Entity::State::Destructing + 1 ) )
    {
        return pprt;
    }
    else if ( base_ptr->state < Ego::Entity::State::Destructing )
    {
        // make sure that you deinitialize before destructing
        base_ptr->state = Ego::Entity::State::DeInitializing;
    }

    iterations = 0;
    while ( NULL != pprt && base_ptr->state <= Ego::Entity::State::Destructing && iterations < max_iterations )
    {
        prt_t * ptmp = prt_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_t * prt_run_config( prt_t * pprt )
{
    Ego::Entity *base_ptr;

    if ( NULL == pprt ) return NULL;
    base_ptr = POBJ_GET_PBASE( pprt );

    if ( !base_ptr->allocated ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( base_ptr->kill_me )
    {
        if ( !TERMINATED_PBASE( base_ptr ) )
        {
            if ( base_ptr->state < Ego::Entity::State::DeInitializing )
            {
                base_ptr->state = Ego::Entity::State::DeInitializing;
            }
        }

        base_ptr->kill_me = false;
    }

    switch ( base_ptr->state )
    {
        default:
		case Ego::Entity::State::Invalid:
            pprt = NULL;
            break;

		case Ego::Entity::State::Constructing:
            pprt = prt_config_ctor( pprt );
            break;

		case Ego::Entity::State::Initializing:
            pprt = prt_config_init( pprt );
            break;

		case Ego::Entity::State::Active:
            pprt = prt_config_active( pprt );
            break;

		case Ego::Entity::State::DeInitializing:
            pprt = prt_config_deinit( pprt );
            break;

		case Ego::Entity::State::Destructing:
            pprt = prt_config_dtor( pprt );
            break;

		case Ego::Entity::State::Waiting:
		case Ego::Entity::State::Terminated:
            /* do nothing */
            break;
    }

    if ( NULL == pprt )
    {
        base_ptr->update_guid = INVALID_UPDATE_GUID;
    }
    else if ( Ego::Entity::State::Active == base_ptr->state )
    {
        base_ptr->update_guid = PrtList.update_guid;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_ctor( prt_t * pprt )
{
    Ego::Entity *base_ptr;

    // grab the base object
    if ( NULL == pprt ) return NULL;
    base_ptr = POBJ_GET_PBASE( pprt );

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( base_ptr ) ) return pprt;

    return prt_ctor( pprt );
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_init( prt_t * pprt )
{
    Ego::Entity *base_ptr;

    if ( NULL == pprt ) return NULL;

    base_ptr = POBJ_GET_PBASE( pprt );
    if ( !STATE_INITIALIZING_PBASE( base_ptr ) ) return pprt;

    pprt = prt_config_do_init( pprt );
    if ( NULL == pprt ) return NULL;

    if ( 0 == prt_loop_depth )
    {
        pprt->obj_base.on = true;
    }
    else
    {
        PrtList_add_activation( GET_INDEX_PPRT( pprt ) );
    }

    base_ptr->state = Ego::Entity::State::Active;

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_active( prt_t * pprt )
{
    // there's nothing to configure if the object is active...

    Ego::Entity *base_ptr;

    if ( NULL == pprt ) return NULL;

    base_ptr = POBJ_GET_PBASE( pprt );
    if ( !base_ptr->allocated ) return NULL;

    if ( !STATE_ACTIVE_PBASE( base_ptr ) ) return pprt;

    POBJ_END_SPAWN( pprt );

    pprt = prt_config_do_active( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_deinit( prt_t * pprt )
{
    /// @author BB
    /// @details deinitialize the character data

    Ego::Entity *base_ptr;

    if ( NULL == pprt ) return NULL;

    base_ptr = POBJ_GET_PBASE( pprt );
    if ( !STATE_DEINITIALIZING_PBASE( base_ptr ) ) return pprt;

    POBJ_END_SPAWN( pprt );

    pprt = prt_config_do_deinit( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_dtor( prt_t * pprt )
{
    Ego::Entity *base_ptr;

    if ( NULL == pprt ) return NULL;

    base_ptr = POBJ_GET_PBASE( pprt );
    if ( !STATE_DESTRUCTING_PBASE( base_ptr ) ) return pprt;

    POBJ_END_SPAWN( pprt );

    return prt_dtor( pprt );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PRT_REF spawn_one_particle( const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, int pip_index,
                            const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                            const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget )
{
    PIP_REF ipip = _profileSystem.pro_get_ipip( iprofile, pip_index );
    return spawn_one_particle(pos, facing, iprofile, ipip, chr_attach, vrt_offset, team, chr_origin, prt_origin, multispawn, oldtarget);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PRT_REF spawn_one_particle( const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, PIP_REF ipip,
                            const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                            const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget )
{
    PRT_REF iprt;

    prt_t * pprt;
    pip_t * ppip;

    if ( !LOADED_PIP( ipip ) )
    {
        log_debug( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                   REF_TO_INT( ipip ), REF_TO_INT( chr_origin ), INGAME_CHR( chr_origin ) ? ChrList.lst[chr_origin].Name : "INVALID",
                   REF_TO_INT( iprofile ), _profileSystem.isValidProfileID( iprofile ) ? _profileSystem.getProfile(iprofile)->getFilePath().c_str() : "INVALID" );

        return INVALID_PRT_REF;
    }
    ppip = PipStack.get_ptr( ipip );

    // count all the requests for this particle type
    ppip->request_count++;

    iprt = PrtList_allocate( ppip->force );
    if ( !DEFINED_PRT( iprt ) )
    {
#if defined(_DEBUG) && defined(DEBUG_PRT_LIST)
        log_debug( "spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
                   chr_origin, INGAME_CHR( chr_origin ) ? ChrList.lst[chr_origin].Name : "INVALID",
                   ipip, LOADED_PIP( ipip ) ? PipStack.lst[ipip].name : "INVALID",
                   iprofile, _profileSystem.isValidProfileID( iprofile ) ? _profileSystem.getProfile(iprofile)->getFilePath().c_str() : "INVALID" );
#endif

        return INVALID_PRT_REF;
    }
    pprt = PrtList_get_ptr( iprt );

    POBJ_BEGIN_SPAWN( pprt );

	pprt->spawn_data.pos = pos;

    pprt->spawn_data.facing     = facing;
    pprt->spawn_data.iprofile   = iprofile;
    pprt->spawn_data.ipip       = ipip;

    pprt->spawn_data.chr_attach = chr_attach;
    pprt->spawn_data.vrt_offset = vrt_offset;
    pprt->spawn_data.team       = team;

    pprt->spawn_data.chr_origin = chr_origin;
    pprt->spawn_data.prt_origin = prt_origin;
    pprt->spawn_data.multispawn = multispawn;
    pprt->spawn_data.oldtarget  = oldtarget;

    // actually force the character to spawn
    pprt = prt_config_activate( pprt, 100 );

    // count all the successful spawns of this particle
    if ( NULL != pprt )
    {
        POBJ_END_SPAWN( pprt );
        ppip->create_count++;
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
/*float prt_get_mesh_pressure( prt_t * pprt, float test_pos[] )
{
    float retval = 0.0f;
    BIT_FIELD  stoppedby;
    pip_t      * ppip;
    const float * loc_test_pos = NULL;

    if ( !DEFINED_PPRT( pprt ) ) return retval;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return retval;
    ppip = PipStack_get_ptr( pprt->pip_ref );

    stoppedby = MAPFX_IMPASS;
    if ( 0 != ppip->bump_money ) SET_BIT( stoppedby, MAPFX_WALL );

    // deal with the optional parameters
    loc_test_pos = ( NULL == test_pos ) ? prt_get_pos_v_const( pprt ).v : test_pos;
    if ( NULL == test_pos ) return 0;

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_get_pressure( PMesh, loc_test_pos, 0.0f, stoppedby );
    }
    prt_stoppedby_tests += mesh_mpdfx_tests;
    prt_pressure_tests += mesh_pressure_tests;

    return retval;
}*/

//--------------------------------------------------------------------------------------------
/*fvec2_t prt_get_mesh_diff( prt_t * pprt, float test_pos[], float center_pressure )
{
    fvec2_t retval = fvec2_t::zero;
    float radius;
    BIT_FIELD stoppedby;
    pip_t *ppip;
    ego_tile_info_t *ptile = NULL;
    const float *loc_test_pos = NULL;

    if ( !DEFINED_PPRT( pprt ) ) return retval;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return retval;
    ppip = PipStack_get_ptr( pprt->pip_ref );

    stoppedby = MAPFX_IMPASS;
    if ( 0 != ppip->bump_money ) SET_BIT( stoppedby, MAPFX_WALL );

    // deal with the optional parameters
    loc_test_pos = ( NULL == test_pos ) ? prt_get_pos_v_const( pprt ).v : test_pos;
    if ( NULL == test_pos ) return retval;

    // calculate the radius based on whether the particle is on camera
    radius = 0.0f;
    ptile = ego_mesh_get_ptile( PMesh, pprt->onwhichgrid );
    if ( NULL != ptile && ptile->inrenderlist )
    {
        radius = pprt->bump_real.size;
    }

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_get_diff( PMesh, loc_test_pos, radius, center_pressure, stoppedby );
    }
    prt_stoppedby_tests += mesh_mpdfx_tests;
    prt_pressure_tests += mesh_pressure_tests;

    return retval;
}
*/

//--------------------------------------------------------------------------------------------
BIT_FIELD prt_hit_wall( prt_t * pprt, const float test_pos[], float nrm[], float * pressure, mesh_wall_data_t * pdata )
{
    /// @author ZZ
    /// @details This function returns nonzero if the particle hit a wall that the
    ///    particle is not allowed to cross

    BIT_FIELD  retval;
    BIT_FIELD  stoppedby;
    pip_t      * ppip;

    if ( !DEFINED_PPRT( pprt ) ) return EMPTY_BIT_FIELD;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return EMPTY_BIT_FIELD;
    ppip = PipStack.get_ptr( pprt->pip_ref );

    stoppedby = MAPFX_IMPASS;
    if ( 0 != ppip->bump_money ) SET_BIT( stoppedby, MAPFX_WALL );

    // deal with the optional parameters
    if ( NULL == test_pos ) test_pos = prt_get_pos_v_const(pprt).v;
    if ( NULL == test_pos ) return EMPTY_BIT_FIELD;

    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_hit_wall( PMesh, test_pos, 0.0f, stoppedby, nrm, pressure, pdata );
    }
    prt_stoppedby_tests += mesh_mpdfx_tests;
    prt_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD prt_test_wall( prt_t * pprt, const float test_pos[], mesh_wall_data_t * pdata )
{
    /// @author ZZ
    /// @details This function returns nonzero if the particle hit a wall that the
    ///    particle is not allowed to cross

    BIT_FIELD retval;
    pip_t * ppip;
    BIT_FIELD  stoppedby;

    if ( !ACTIVE_PPRT( pprt ) ) return EMPTY_BIT_FIELD;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return EMPTY_BIT_FIELD;
    ppip = PipStack.get_ptr( pprt->pip_ref );

    stoppedby = MAPFX_IMPASS;
    if ( 0 != ppip->bump_money ) SET_BIT( stoppedby, MAPFX_WALL );

    // handle optional parameters
    if ( NULL == test_pos ) test_pos = prt_get_pos_v_const(pprt).v;
    if ( NULL == test_pos ) return EMPTY_BIT_FIELD;

    // do the wall test
    mesh_mpdfx_tests = 0;
    mesh_bound_tests = 0;
    mesh_pressure_tests = 0;
    {
        retval = ego_mesh_test_wall( PMesh, test_pos, 0.0f, stoppedby, pdata );
    }
    prt_stoppedby_tests += mesh_mpdfx_tests;
    prt_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
void update_all_particles()
{
    /// @author BB
    /// @details main loop for updating particles. Do not use the
    ///               PRT_BEGIN_LOOP_* macro.
    ///               Converted all the update functions to the prt_run_config() paradigm.

    PRT_REF iprt;
    prt_bundle_t prt_bdl;

    // activate any particles might have been generated last update in an in-active state
    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        if ( !ALLOCATED_PRT( iprt ) ) continue;

        prt_bundle_set( &prt_bdl, PrtList_get_ptr( iprt ) );

        prt_update( &prt_bdl );
    }
}

//--------------------------------------------------------------------------------------------
void prt_set_level( prt_t * pprt, const float level )
{
    float loc_height;

    if ( !DISPLAY_PPRT( pprt ) ) return;

    pprt->enviro.level = level;

    loc_height = prt_get_scale( pprt ) * std::max( FP8_TO_FLOAT( pprt->size ), pprt->offset.z * 0.5f );

    pprt->enviro.adj_level = pprt->enviro.level;
    pprt->enviro.adj_floor = pprt->enviro.floor_level;

    pprt->enviro.adj_level += loc_height;
    pprt->enviro.adj_floor += loc_height;

    // set the zlerp after we have done everything to the particle's level we care to
    pprt->enviro.zlerp = ( pprt->pos.z - pprt->enviro.adj_level ) / PLATTOLERANCE;
    pprt->enviro.zlerp = CLIP( pprt->enviro.zlerp, 0.0f, 1.0f );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_bundle_t * move_one_particle_get_environment( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details A helper function that gets all of the information about the particle's
    ///               environment (like friction, etc.) that will be necessary for the other
    ///               move_one_particle_*() functions to work

    Uint32 itile;
    float loc_level = 0.0f;

    prt_t             * loc_pprt;
    prt_environment_t * penviro;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    penviro  = &( loc_pprt->enviro );

    //---- character "floor" level
    penviro->floor_level = ego_mesh_get_level( PMesh, loc_pprt->pos.x, loc_pprt->pos.y );
    penviro->level       = penviro->floor_level;

    //---- The actual level of the characer.
    //     Estimate platform attachment from whatever is in the onwhichplatform_ref variable from the
    //     last loop
    loc_level = penviro->floor_level;
    if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        loc_level = std::max( penviro->floor_level, ChrList.lst[loc_pprt->onwhichplatform_ref].pos.z + ChrList.lst[loc_pprt->onwhichplatform_ref].chr_min_cv.maxs[OCT_Z] );
    }
    prt_set_level( loc_pprt, loc_level );

    //---- the "twist" of the floor
    penviro->twist = TWIST_FLAT;
    itile              = INVALID_TILE;
    if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        // this only works for 1 level of attachment
        itile = ChrList.lst[loc_pprt->onwhichplatform_ref].onwhichgrid;
    }
    else
    {
        itile = loc_pprt->onwhichgrid;
    }

    penviro->twist = ego_mesh_get_twist( PMesh, itile );

    // the "watery-ness" of whatever water might be here
    penviro->is_watery = water.is_water && penviro->inwater;
    penviro->is_slippy = !penviro->is_watery && ( 0 != ego_mesh_test_fx( PMesh, loc_pprt->onwhichgrid, MAPFX_SLIPPY ) );

    //---- traction
    penviro->traction = 1.0f;
    if ( loc_pprt->is_homing )
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        // in case the platform is tilted
        // unfortunately platforms are attached in the collision section
        // which occurs after the movement section.

        fvec3_t platform_up;

        chr_getMatUp(ChrList_get_ptr(loc_pprt->onwhichplatform_ref), platform_up);
        fvec3_self_normalize(platform_up);

        penviro->traction = ABS( platform_up.z ) * ( 1.0f - penviro->zlerp ) + 0.25f * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= hillslide * ( 1.0f - penviro->zlerp ) + 1.0f * penviro->zlerp;
        }
    }
    else if ( ego_mesh_grid_is_valid( PMesh, loc_pprt->onwhichgrid ) )
    {
        penviro->traction = ABS( map_twist_nrm[penviro->twist].z ) * ( 1.0f - penviro->zlerp ) + 0.25f * penviro->zlerp;

        if ( penviro->is_slippy )
        {
            penviro->traction /= hillslide * ( 1.0f - penviro->zlerp ) + 1.0f * penviro->zlerp;
        }
    }

    //---- the friction of the fluid we are in
    if ( penviro->is_watery )
    {
        penviro->fluid_friction_vrt  = waterfriction;
        penviro->fluid_friction_hrz = waterfriction;
    }
    else
    {
        penviro->fluid_friction_hrz = penviro->air_friction;       // like real-life air friction
        penviro->fluid_friction_vrt  = penviro->air_friction;
    }

    //---- friction
    penviro->friction_hrz = 1.0f;
    if ( !loc_pprt->is_homing )
    {
        // Make the characters slide
        float temp_friction_xy = noslipfriction;
        if ( ego_mesh_grid_is_valid( PMesh, loc_pprt->onwhichgrid ) && penviro->is_slippy )
        {
            // It's slippy all right...
            temp_friction_xy = slippyfriction;
        }

        penviro->friction_hrz = penviro->zlerp * 1.0f + ( 1.0f - penviro->zlerp ) * temp_friction_xy;
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * move_one_particle_do_fluid_friction( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details A helper function that computes particle friction with the floor
    ///
    /// @note this is pretty much ripped from the character version of this function and may
    ///       contain some features that are not necessary for any particles that are actually in game.
    ///       For instance, the only particles that is under their own control are the homing particles
    ///       but they do not have friction with the mesh, but that case is still treated in the code below.

    prt_t             * loc_pprt;
    pip_t             * loc_ppip;
    prt_environment_t * loc_penviro;
    phys_data_t       * loc_pphys;

    fvec3_t fluid_acc;

    if ( NULL == pbdl_prt ) return NULL;
    loc_pprt    = pbdl_prt->prt_ptr;
    loc_ppip    = pbdl_prt->pip_ptr;
    loc_penviro = &( loc_pprt->enviro );
    loc_pphys   = &( loc_pprt->phys );

    // if the particle is a homing-type particle, ignore friction
    if ( loc_ppip->homing ) return pbdl_prt;

    // Light isn't affected by fluid velocity
    if ( SPRITE_LIGHT == loc_pprt->type ) return pbdl_prt;

    // assume no acceleration
    fvec3_self_clear( fluid_acc.v );

    // get the speed relative to the fluid
    if ( loc_pprt->enviro.inwater )
    {
        fluid_acc = fvec3_sub(waterspeed, loc_pprt->vel);
    }
    else
    {
		fluid_acc = fvec3_sub(windspeed, loc_pprt->vel);
    }

    // get the fluid friction
    if ( loc_pprt->buoyancy > 0.0f )
    {
        // this is a buoyant particle, like smoke

        float loc_buoyancy_friction = air_friction * loc_pprt->air_resistance;

        if ( loc_pprt->enviro.inwater )
        {
            float water_friction = POW( loc_buoyancy_friction, 2.0f );

            fluid_acc *= 1.0f - water_friction;
        }
        else
        {
            fluid_acc *= 1.0f - loc_buoyancy_friction;
        }
    }
    else
    {
        // this is a normal particle, like a mushroom

        if ( loc_pprt->enviro.inwater )
        {
            fluid_acc.x *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc.y *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc.z *= 1.0f - loc_penviro->fluid_friction_vrt * loc_pprt->air_resistance;
        }
        else
        {
            fluid_acc.x *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc.y *= 1.0f - loc_penviro->fluid_friction_hrz * loc_pprt->air_resistance;
            fluid_acc.z *= 1.0f - loc_penviro->fluid_friction_vrt * loc_pprt->air_resistance;
        }
    }

    // apply the fluid friction
    loc_pprt->vel.x += fluid_acc.x;
    loc_pprt->vel.y += fluid_acc.y;
    loc_pprt->vel.z += fluid_acc.z;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * move_one_particle_do_floor_friction( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details A helper function that computes particle friction with the floor
    ///
    /// @note this is pretty much ripped from the character version of this function and may
    ///       contain some features that are not necessary for any particles that are actually in game.
    ///       For instance, the only particles that is under their own control are the homing particles
    ///       but they do not have friction with the mesh, but that case is still treated in the code below.

    float temp_friction_xy;
    fvec3_t   vup, fric, fric_floor;
    fvec3_t   floor_acc;

    prt_t             * loc_pprt;
    pip_t             * loc_ppip;
    prt_environment_t * penviro;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    // if the particle is homing in on something, ignore friction
    if ( loc_pprt->is_homing ) return pbdl_prt;

    // limit floor friction effects to solid objects
    if ( SPRITE_SOLID != loc_pprt->type )  return pbdl_prt;

    // figure out the acceleration due to the current "floor"
    floor_acc.x = floor_acc.y = floor_acc.z = 0.0f;
    temp_friction_xy = 1.0f;
    if ( INGAME_CHR( loc_pprt->onwhichplatform_ref ) )
    {
        chr_t * pplat = ChrList_get_ptr( loc_pprt->onwhichplatform_ref );

        temp_friction_xy = platstick;

        floor_acc.x = pplat->vel.x - pplat->vel_old.x;
        floor_acc.y = pplat->vel.y - pplat->vel_old.y;
        floor_acc.z = pplat->vel.z - pplat->vel_old.z;

        chr_getMatUp(pplat, vup);
    }
    else
    {
        temp_friction_xy = 0.5f;
        floor_acc.x = -loc_pprt->vel.x;
        floor_acc.y = -loc_pprt->vel.y;
        floor_acc.z = -loc_pprt->vel.z;

        if ( TWIST_FLAT == penviro->twist )
        {
            vup.x = vup.y = 0.0f;
            vup.z = 1.0f;
        }
        else
        {
            vup = map_twist_nrm[penviro->twist];
        }
    }

    // the first guess about the floor friction
    fric_floor.x = floor_acc.x * ( 1.0f - penviro->zlerp ) * ( 1.0f - temp_friction_xy ) * penviro->traction;
    fric_floor.y = floor_acc.y * ( 1.0f - penviro->zlerp ) * ( 1.0f - temp_friction_xy ) * penviro->traction;
    fric_floor.z = floor_acc.z * ( 1.0f - penviro->zlerp ) * ( 1.0f - temp_friction_xy ) * penviro->traction;

    // the total "friction" due to the floor
    fric.x = fric_floor.x + penviro->acc.x;
    fric.y = fric_floor.y + penviro->acc.y;
    fric.z = fric_floor.z + penviro->acc.z;

    //---- limit the friction to whatever is horizontal to the mesh
    if ( ABS( vup.z ) > 0.9999f )
    {
        floor_acc.z = 0.0f;
        fric.z      = 0.0f;
    }
    else
    {
        float ftmp;

		ftmp = floor_acc.dot(vup);
        floor_acc.x -= ftmp * vup.x;
        floor_acc.y -= ftmp * vup.y;
        floor_acc.z -= ftmp * vup.z;

        ftmp = fric.dot(vup);
        fric.x -= ftmp * vup.x;
        fric.y -= ftmp * vup.y;
        fric.z -= ftmp * vup.z;
    }

    // test to see if the player has any more friction left?
    penviro->is_slipping = fvec3_length_abs(fric) > penviro->friction_hrz;

    if ( penviro->is_slipping )
    {
        penviro->traction *= 0.5f;
        temp_friction_xy  = std::sqrt( temp_friction_xy );

		fric_floor = floor_acc * ((1.0f - penviro->zlerp) * (1.0f - temp_friction_xy) * penviro->traction);
		float ftmp = fric_floor.dot(vup);
        fric_floor.x -= ftmp * vup.x;
        fric_floor.y -= ftmp * vup.y;
        fric_floor.z -= ftmp * vup.z;
    }

    // Apply the floor friction
	loc_pprt->vel += fric_floor;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * move_one_particle_do_homing( prt_bundle_t * pbdl_prt )
{
    chr_t * ptarget;

    prt_t             * loc_pprt;
    PRT_REF             loc_iprt;
    pip_t             * loc_ppip;
    prt_environment_t * penviro;

    int       ival;
    float     vlen, min_length, uncertainty;
    fvec3_t   vdiff, vdither;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_iprt = pbdl_prt->prt_ref;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    // is the particle a homing type?
    if ( !loc_ppip->homing ) return pbdl_prt;

    // the particle update function is supposed to turn homing off if the particle looses its target
    if ( !loc_pprt->is_homing ) return pbdl_prt;

    // the loc_pprt->is_homing variable is supposed to track the following, but it could have lost synch by this point
    if ( INGAME_CHR( loc_pprt->attachedto_ref ) || !INGAME_CHR( loc_pprt->target_ref ) ) return pbdl_prt;

    // grab a pointer to the target
    ptarget = ChrList_get_ptr( loc_pprt->target_ref );

    vdiff = fvec3_sub(ptarget->pos, prt_get_pos_v_const(loc_pprt));
    vdiff.z += ptarget->bump.height * 0.5f;

    min_length = 2 * 5 * 256 * ( ChrList.lst[loc_pprt->owner_ref].wisdom / ( float )PERFECTBIG );

    // make a little incertainty about the target
    uncertainty = 256.0f * ( 1.0f - ChrList.lst[loc_pprt->owner_ref].intelligence  / ( float )PERFECTBIG );

    ival = RANDIE;
    vdither.x = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

    ival = RANDIE;
    vdither.y = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

    ival = RANDIE;
    vdither.z = ((( float ) ival / 0x8000 ) - 1.0f )  * uncertainty;

    // take away any dithering along the direction of motion of the particle
    vlen = fvec3_dot_product(loc_pprt->vel, loc_pprt->vel);
    if ( vlen > 0.0f )
    {
        float vdot = fvec3_dot_product(vdither, loc_pprt->vel) / vlen;

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

    loc_pprt->vel.x = ( loc_pprt->vel.x + vdiff.x * loc_ppip->homingaccel ) * loc_ppip->homingfriction;
    loc_pprt->vel.y = ( loc_pprt->vel.y + vdiff.y * loc_ppip->homingaccel ) * loc_ppip->homingfriction;
    loc_pprt->vel.z = ( loc_pprt->vel.z + vdiff.z * loc_ppip->homingaccel ) * loc_ppip->homingfriction;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * move_one_particle_do_z_motion( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details A helper function that does gravitational acceleration and buoyancy

    float loc_zlerp, tmp_buoyancy, loc_buoyancy;
    //float loc_zacc;

    prt_t             * loc_pprt;
    PRT_REF             loc_iprt;
    pip_t             * loc_ppip;
    prt_environment_t * penviro;

    fvec3_t z_motion_acc;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_iprt = pbdl_prt->prt_ref;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    /// @note ZF@> We really can't do gravity for Light! A lot of magical effects and attacks in the game depend on being able
    ///            to move forward in a straight line without being dragged down into the dust!
    /// @note BB@> however, the fireball particle is light, and without gravity it will never bounce on the
    ///            ground as it is supposed to
    /// @note ZF@> I will try to fix this by adding a new  no_gravity expansion for particles
    if ( loc_pprt->no_gravity || /* loc_pprt->type == SPRITE_LIGHT || */ loc_pprt->is_homing || INGAME_CHR( loc_pprt->attachedto_ref ) ) return pbdl_prt;

    loc_zlerp = CLIP( penviro->zlerp, 0.0f, 1.0f );

    fvec3_self_clear( z_motion_acc.v );

    // in higher gravity environments, buoyancy is larger
    tmp_buoyancy = loc_pprt->buoyancy * gravity / STANDARD_GRAVITY;

    // handle bouyancy near the ground
    if ( loc_zlerp >= 1.0f )
    {
        loc_buoyancy = tmp_buoyancy;
    }
    else if ( loc_zlerp <= 0.0f )
    {
        loc_buoyancy = 0.0f;
    }
    else
    {
        // Do particle buoyancy. This is kinda BS the way it is calculated
        loc_buoyancy = 0.0f;
        if ( tmp_buoyancy + gravity < 0.0f )
        {
            // the particle cannot hold itself up

            // loc_zacc = ( tmp_buoyancy + gravity ) * loc_zlerp;
            loc_buoyancy = tmp_buoyancy * loc_zlerp;
        }
        else
        {
            // the particle is floating, make the normal force cancel gravity, only

            // loc_zacc = tmp_buoyancy + gravity * loc_zlerp;
            loc_buoyancy = tmp_buoyancy;
        }
    }

    // do the buoyancy calculation
    z_motion_acc.z += loc_buoyancy;

    // do gravity
    if ( penviro->is_slippy && ( TWIST_FLAT != penviro->twist ) && loc_zlerp < 1.0f )
    {
        // hills make particles slide

        fvec3_t   gperp;    // gravity perpendicular to the mesh
        fvec3_t   gpara;    // gravity parallel      to the mesh (what pushes you)

        gpara = map_twist_vel[penviro->twist];

        gperp.x = 0       - gpara.x;
        gperp.y = 0       - gpara.y;
        gperp.z = gravity - gpara.z;

        z_motion_acc.x += gpara.x * ( 1.0f - loc_zlerp ) + gperp.x * loc_zlerp;
        z_motion_acc.y += gpara.y * ( 1.0f - loc_zlerp ) + gperp.y * loc_zlerp;
        z_motion_acc.z += gpara.z * ( 1.0f - loc_zlerp ) + gperp.z * loc_zlerp;
    }
    else
    {
        z_motion_acc.z += loc_zlerp * gravity;
    }

    loc_pprt->vel.x += z_motion_acc.x;
    loc_pprt->vel.y += z_motion_acc.y;
    loc_pprt->vel.z += z_motion_acc.z;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * move_one_particle_integrate_motion_attached( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    float loc_level;
    bool touch_a_floor, hit_a_wall, needs_test;
    fvec3_t nrm_total;
    fvec3_t tmp_pos;

    prt_t             * loc_pprt;
    PRT_REF             loc_iprt;
    pip_t             * loc_ppip;
    prt_environment_t * penviro;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_iprt = pbdl_prt->prt_ref;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    // if the particle is not still in "display mode" there is no point in going on
    if ( !DISPLAY_PPRT( loc_pprt ) ) return pbdl_prt;

    // capture the particle position
    prt_get_pos(loc_pprt, tmp_pos);

    // only deal with attached particles
    if ( INVALID_CHR_REF == loc_pprt->attachedto_ref ) return pbdl_prt;

    touch_a_floor = false;
    hit_a_wall  = false;
    nrm_total.x = nrm_total.y = nrm_total.z = 0;

    loc_level = penviro->adj_level;

    // Move the particle
    if ( tmp_pos.z < loc_level )
    {
        touch_a_floor = true;
    }

    if ( touch_a_floor )
    {
        // Play the sound for hitting the floor [FSND]
        prt_play_sound( loc_iprt, loc_ppip->end_sound_floor );
    }

    // handle the collision
    if ( touch_a_floor && loc_ppip->end_ground )
    {
        end_one_particle_in_game( pbdl_prt->prt_ref );
        return NULL;
    }

    // interaction with the mesh walls
    hit_a_wall = false;
    needs_test = false;
    if ( ABS( loc_pprt->vel.x ) + ABS( loc_pprt->vel.y ) > 0.0f )
    {
        mesh_wall_data_t wdata;

        if ( EMPTY_BIT_FIELD != prt_test_wall( loc_pprt, tmp_pos.v, &wdata ) )
        {
            Uint32  hit_bits;
            fvec2_t nrm;
            float   pressure;

            // how is the character hitting the wall?
            hit_bits = prt_hit_wall( loc_pprt, tmp_pos.v, nrm.v, &pressure, &wdata );

            if ( 0 != hit_bits )
            {
                hit_a_wall = true;
            }
        }
    }

    // handle the sounds
    if ( hit_a_wall )
    {
        // Play the sound for hitting the floor [FSND]
        prt_play_sound( loc_iprt, loc_ppip->end_sound_wall );
    }

    // handle the collision
    if ( hit_a_wall && ( loc_ppip->end_wall || loc_ppip->end_bump ) )
    {
        end_one_particle_in_game( pbdl_prt->prt_ref );
        return NULL;
    }

    prt_set_pos(loc_pprt, tmp_pos);

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * move_one_particle_integrate_motion( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    float ftmp, loc_level;
    bool hit_a_floor, hit_a_wall, needs_test;
    bool touch_a_floor, touch_a_wall;
    fvec3_t nrm_total;
    fvec3_t tmp_pos;

    prt_t             * loc_pprt;
    PRT_REF             loc_iprt;
    pip_t             * loc_ppip;
    prt_environment_t * penviro;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_iprt = pbdl_prt->prt_ref;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    // if the particle is not still in "display mode" there is no point in going on
    if ( !DISPLAY_PPRT( loc_pprt ) ) return pbdl_prt;

    // capture the position
    prt_get_pos(loc_pprt, tmp_pos);

    // no point in doing this if the particle thinks it's attached
    if ( INVALID_CHR_REF != loc_pprt->attachedto_ref )
    {
        return move_one_particle_integrate_motion_attached( pbdl_prt );
    }

    hit_a_floor   = false;
    hit_a_wall    = false;
    touch_a_floor = false;
    touch_a_wall  = false;
    nrm_total.x = nrm_total.y = nrm_total.z = 0.0f;

    loc_level = penviro->adj_level;

    // Move the particle
    ftmp = tmp_pos.z;
    tmp_pos.z += loc_pprt->vel.z;
    LOG_NAN( tmp_pos.z );
    if ( tmp_pos.z < loc_level )
    {
        fvec3_t floor_nrm = fvec3_t( 0.0f, 0.0f, 1.0f );
        float vel_dot;
        fvec3_t vel_perp, vel_para;
        Uint8 tmp_twist = TWIST_FLAT;

        touch_a_floor = true;

        tmp_twist = cartman_get_fan_twist( PMesh, loc_pprt->onwhichgrid );

        if ( TWIST_FLAT != tmp_twist )
        {
            floor_nrm = map_twist_nrm[penviro->twist];
        }

        vel_dot = fvec3_dot_product( floor_nrm, loc_pprt->vel );
        if ( 0.0f == vel_dot )
        {
            fvec3_self_clear( vel_perp.v );
            vel_para = loc_pprt->vel;
        }
        else
        {
            vel_perp.x = floor_nrm.x * vel_dot;
            vel_perp.y = floor_nrm.y * vel_dot;
            vel_perp.z = floor_nrm.z * vel_dot;

            vel_para.x = loc_pprt->vel.x - vel_perp.x;
            vel_para.y = loc_pprt->vel.y - vel_perp.y;
            vel_para.z = loc_pprt->vel.z - vel_perp.z;
        }

        if ( vel_dot < - STOPBOUNCINGPART )
        {
            // the particle will bounce
            nrm_total.x += floor_nrm.x;
            nrm_total.y += floor_nrm.y;
            nrm_total.z += floor_nrm.z;

            // take reflection in the floor into account when computing the new level
            tmp_pos.z = loc_level + ( loc_level - ftmp ) * loc_ppip->dampen + 0.1f;

            hit_a_floor = true;
        }
        else if ( vel_dot > 0.0f )
        {
            // the particle is not bouncing, it is just at the wrong height
            tmp_pos.z = loc_level + 0.1f;
        }
        else
        {
            // the particle is in the "stop bouncing zone"
            tmp_pos.z     = loc_level + 0.1f;
            loc_pprt->vel = vel_para;
        }
    }

    // handle the sounds
    if ( hit_a_floor )
    {
        // Play the sound for hitting the floor [FSND]
        prt_play_sound( loc_iprt, loc_ppip->end_sound_floor );
    }

    // handle the collision
    if ( touch_a_floor && loc_ppip->end_ground )
    {
        end_one_particle_in_game( pbdl_prt->prt_ref );
        return NULL;
    }

    // interaction with the mesh walls
    hit_a_wall = false;
    needs_test = false;
    if ( ABS( loc_pprt->vel.x ) + ABS( loc_pprt->vel.y ) > 0.0f )
    {
        mesh_wall_data_t wdata;

        float old_x, old_y, new_x, new_y;

        old_x = tmp_pos.x; LOG_NAN( old_x );
        old_y = tmp_pos.y; LOG_NAN( old_y );

        new_x = old_x + loc_pprt->vel.x; LOG_NAN( new_x );
        new_y = old_y + loc_pprt->vel.y; LOG_NAN( new_y );

        tmp_pos.x = new_x;
        tmp_pos.y = new_y;

        //Hitting a wall?
        if ( EMPTY_BIT_FIELD != prt_test_wall( loc_pprt, tmp_pos.v, &wdata ) )
        {
            fvec2_t nrm;
            float   pressure;

            // how is the character hitting the wall?
            if ( EMPTY_BIT_FIELD != prt_hit_wall( loc_pprt, tmp_pos.v, nrm.v, &pressure, &wdata ) )
            {
                touch_a_wall = true;

                nrm_total.x += nrm.x;
                nrm_total.y += nrm.y;

                hit_a_wall = ( fvec2_dot_product( loc_pprt->vel.v, nrm.v ) < 0.0f );
            }
        }
    }

    // handle the sounds
    if ( hit_a_wall )
    {
        // Play the sound for hitting the wall [WSND]
        prt_play_sound( loc_iprt, loc_ppip->end_sound_wall );
    }

    // handle the collision
    if ( touch_a_wall && loc_ppip->end_wall )
    {
        end_one_particle_in_game( pbdl_prt->prt_ref );
        return NULL;
    }

    // do the reflections off the walls and floors
    if ( hit_a_wall || hit_a_floor )
    {

        if (( hit_a_wall && ( loc_pprt->vel.x * nrm_total.x + loc_pprt->vel.y * nrm_total.y ) < 0.0f ) ||
            ( hit_a_floor && ( loc_pprt->vel.z * nrm_total.z ) < 0.0f ) )
        {
            float vdot;
            fvec3_t   vpara, vperp;

            fvec3_self_normalize(nrm_total);

            vdot  = fvec3_dot_product( nrm_total, loc_pprt->vel );

            vperp.x = nrm_total.x * vdot;
            vperp.y = nrm_total.y * vdot;
            vperp.z = nrm_total.z * vdot;

            vpara.x = loc_pprt->vel.x - vperp.x;
            vpara.y = loc_pprt->vel.y - vperp.y;
            vpara.z = loc_pprt->vel.z - vperp.z;

            // we can use the impulse to determine how much velocity to kill in the parallel direction
            //imp.x = vperp.x * (1.0f + loc_ppip->dampen);
            //imp.y = vperp.y * (1.0f + loc_ppip->dampen);
            //imp.z = vperp.z * (1.0f + loc_ppip->dampen);

            // do the reflection
            vperp.x *= -loc_ppip->dampen;
            vperp.y *= -loc_ppip->dampen;
            vperp.z *= -loc_ppip->dampen;

            // fake the friction, for now
            if ( 0.0f != nrm_total.y || 0.0f != nrm_total.z )
            {
                vpara.x *= loc_ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.z )
            {
                vpara.y *= loc_ppip->dampen;
            }

            if ( 0.0f != nrm_total.x || 0.0f != nrm_total.y )
            {
                vpara.z *= loc_ppip->dampen;
            }

            // add the components back together
            loc_pprt->vel.x = vpara.x + vperp.x;
            loc_pprt->vel.y = vpara.y + vperp.y;
            loc_pprt->vel.z = vpara.z + vperp.z;
        }

        if ( nrm_total.z != 0.0f && loc_pprt->vel.z < STOPBOUNCINGPART )
        {
            // this is the very last bounce
            loc_pprt->vel.z = 0.0f;
            tmp_pos.z = loc_level + 0.0001f;
        }

        if ( hit_a_wall )
        {
            float fx, fy;

            // fix the facing
            facing_to_vec( loc_pprt->facing, &fx, &fy );

            if ( 0.0f != nrm_total.x )
            {
                fx *= -1;
            }

            if ( 0.0f != nrm_total.y )
            {
                fy *= -1;
            }

            loc_pprt->facing = vec_to_facing( fx, fy );
        }
    }

    //Don't fall in pits...
    if ( loc_pprt->is_homing ) tmp_pos.z = std::max( tmp_pos.z, 0.0f );

    //Rotate particle to the direction we are moving
    if ( loc_ppip->rotatetoface )
    {
        if ( ABS( loc_pprt->vel.x ) + ABS( loc_pprt->vel.y ) > 1e-6 )
        {
            // use velocity to find the angle
            loc_pprt->facing = vec_to_facing( loc_pprt->vel.x, loc_pprt->vel.y );
        }
        else if ( INGAME_CHR( loc_pprt->target_ref ) )
        {
            chr_t * ptarget =  ChrList_get_ptr( loc_pprt->target_ref );

            // face your target
            loc_pprt->facing = vec_to_facing( ptarget->pos.x - tmp_pos.x , ptarget->pos.y - tmp_pos.y );
        }
    }

    prt_set_pos(loc_pprt, tmp_pos);

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
bool move_one_particle( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details The master function for controlling a particle's motion

    prt_t             * loc_pprt;
    prt_environment_t * penviro;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;
    loc_pprt = pbdl_prt->prt_ptr;
    penviro  = &( loc_pprt->enviro );

    if ( !DISPLAY_PPRT( loc_pprt ) ) return false;

    // if the particle is hidden it is frozen in time. do nothing.
    if ( loc_pprt->is_hidden ) return false;

    // save the acceleration from the last time-step
    penviro->acc = fvec3_sub(loc_pprt->vel, loc_pprt->vel_old);

    // determine the actual velocity for attached particles
    if ( INGAME_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->vel = fvec3_sub(prt_get_pos_v_const(loc_pprt), loc_pprt->pos_old);
    }

    // Particle's old location
    prt_get_pos(loc_pprt, loc_pprt->pos_old);
    loc_pprt->vel_old = loc_pprt->vel;

    // what is the local environment like?
    pbdl_prt = move_one_particle_get_environment( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;

    // wind, current, and other fluid friction effects
    pbdl_prt = move_one_particle_do_fluid_friction( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;

    // do friction with the floor before voluntary motion
    pbdl_prt = move_one_particle_do_floor_friction( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;

    pbdl_prt = move_one_particle_do_homing( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;

    pbdl_prt = move_one_particle_do_z_motion( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;

    pbdl_prt = move_one_particle_integrate_motion( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
void move_all_particles()
{
    /// @author ZZ
    /// @details This is the particle physics function

    prt_stoppedby_tests = 0;

    // move every particle
    PRT_BEGIN_LOOP_DISPLAY( cnt, prt_bdl )
    {
        // prime the environment
        prt_bdl.prt_ptr->enviro.air_friction = air_friction;
        prt_bdl.prt_ptr->enviro.ice_friction = ice_friction;

        move_one_particle( &prt_bdl );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void particle_system_begin()
{
    /// @author ZZ
    /// @details This function sets up particle data

    // Reset the allocation table
    PrtList_ctor();
    PipStack_init_all();
}

//--------------------------------------------------------------------------------------------
void particle_system_end()
{
    PipStack_release_all();
    PrtList_dtor();
}

//--------------------------------------------------------------------------------------------
int spawn_bump_particles( const CHR_REF character, const PRT_REF particle )
{
    /// @author ZZ
    /// @details This function is for catching characters on fire and such

    int      cnt, bs_count;
    float    x, y, z;
    FACING_T facing;
    int      amount;
    FACING_T direction;
    float    fsin, fcos;

    pip_t * ppip;
    chr_t * pchr;
    mad_t * pmad;
    prt_t * pprt;

    if ( !INGAME_PRT( particle ) ) return 0;
    pprt = PrtList_get_ptr( particle );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return 0;
    ppip = PipStack.get_ptr( pprt->pip_ref );

    // no point in going on, is there?
    if ( 0 == ppip->bumpspawn_amount && !ppip->spawnenchant ) return 0;
    amount = ppip->bumpspawn_amount;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList_get_ptr( character );

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return 0;

    const std::shared_ptr<ObjectProfile> &profile = _profileSystem.getProfile( pchr->profile_ref );

    bs_count = 0;

    // Only damage if hitting from proper direction
    direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
    direction = ATK_BEHIND + ( pchr->ori.facing_z - direction );

    // Check that direction
    if ( !is_invictus_direction( direction, character, ppip->damfx ) )
    {
        IPair loc_rand = {0, 100};
        int damage_resistance;

        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_one_enchant( pprt->owner_ref, character, INVALID_CHR_REF, INVALID_ENC_REF, pprt->profile_ref );
        }

        // Spawn particles - this has been modded to maximize the visual effect
        // on a given target. It is not the most optimal solution for lots of particles
        // spawning. Thst would probably be to make the distance calculations and then
        // to quicksort the list and choose the n closest points.
        //
        // however, it seems that the bump particles in game rarely attach more than
        // one bump particle

        //check if we resisted the attack, we could resist some of the particles or none
        damage_resistance = ( pprt->damagetype >= DAMAGE_COUNT ) ? 0 : pchr->damage_resistance[pprt->damagetype] * 100;
        for ( cnt = 0; cnt < amount; cnt++ )
        {
            if ( generate_irand_pair( loc_rand ) <= damage_resistance ) amount--;
        }

        if ( amount > 0 && !profile->hasResistBumpSpawn() && !pchr->invictus )
        {
            int grip_verts, vertices;
            int slot_count;

            slot_count = 0;
            if ( profile->isSlotValid(SLOT_LEFT) ) slot_count++;
            if ( profile->isSlotValid(SLOT_RIGHT) ) slot_count++;

            if ( 0 == slot_count )
            {
                grip_verts = 1;  // always at least 1?
            }
            else
            {
                grip_verts = GRIP_VERTS * slot_count;
            }

            vertices = ( int )pchr->inst.vrt_count - ( int )grip_verts;
            vertices = std::max( 0, vertices );

            if ( vertices != 0 )
            {
                PRT_REF *vertex_occupied;
                float   *vertex_distance;
                float    dist;
                TURN_T   turn;

                vertex_occupied = EGOBOO_NEW_ARY( PRT_REF, vertices );
                vertex_distance = EGOBOO_NEW_ARY( float,   vertices );

                // this could be done more easily with a quicksort....
                // but I guess it doesn't happen all the time
                dist = fvec3_dist_abs(prt_get_pos_v_const(pprt), chr_get_pos_v_const(pchr));

                // clear the occupied list
                z = pprt->pos.z - pchr->pos.z;
                facing = pprt->facing - pchr->ori.facing_z;
                turn   = TO_TURN( facing );
                fsin = turntosin[ turn ];
                fcos = turntocos[ turn ];
                x = dist * fcos;
                y = dist * fsin;

                // prepare the array values
                for ( cnt = 0; cnt < vertices; cnt++ )
                {
                    dist = ABS( x - pchr->inst.vrt_lst[vertices-cnt-1].pos[XX] ) +
                           ABS( y - pchr->inst.vrt_lst[vertices-cnt-1].pos[YY] ) +
                           ABS( z - pchr->inst.vrt_lst[vertices-cnt-1].pos[ZZ] );

                    vertex_distance[cnt] = dist;
                    vertex_occupied[cnt] = INVALID_PRT_REF;
                }

                // determine if some of the vertex sites are already occupied
                PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
                {
                    if ( character != prt_bdl.prt_ptr->attachedto_ref ) continue;

                    if ( prt_bdl.prt_ptr->attachedto_vrt_off >= 0 && prt_bdl.prt_ptr->attachedto_vrt_off < vertices )
                    {
                        vertex_occupied[prt_bdl.prt_ptr->attachedto_vrt_off] = prt_bdl.prt_ref;
                    }
                }
                PRT_END_LOOP()

                // Find best vertices to attach the particles to
                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    PRT_REF bs_part;
                    Uint32  bestdistance;
                    int     bestvertex;

                    bestvertex   = 0;
                    bestdistance = 0xFFFFFFFF;         //Really high number

                    for ( cnt = 0; cnt < vertices; cnt++ )
                    {
                        if ( INVALID_PRT_REF != vertex_occupied[cnt] )
                            continue;

                        if ( vertex_distance[cnt] < bestdistance )
                        {
                            bestdistance = vertex_distance[cnt];
                            bestvertex   = cnt;
                        }
                    }

                    bs_part = spawn_one_particle( pchr->pos, pchr->ori.facing_z, pprt->profile_ref, ppip->bumpspawn_lpip,
                                                  character, bestvertex + 1, pprt->team, pprt->owner_ref, particle, cnt, character );

                    if ( DEFINED_PRT( bs_part ) )
                    {
                        vertex_occupied[bestvertex] = bs_part;
                        PrtList.lst[bs_part].is_bumpspawn = true;
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

                //        bs_part = spawn_one_particle( pchr->pos, pchr->ori.facing_z, pprt->profile_ref, ppip->bumpspawn_lpip,
                //                            character, irand % vertices, pprt->team, pprt->owner_ref, particle, cnt, character );

                //        if( DEFINED_PRT(bs_part) )
                //        {
                //            PrtList.lst[bs_part].is_bumpspawn = true;
                //            bs_count++;
                //        }
                //    }
                //}

                EGOBOO_DELETE_ARY( vertex_occupied );
                EGOBOO_DELETE_ARY( vertex_distance );
            }
        }
    }

    return bs_count;
}

//--------------------------------------------------------------------------------------------
bool prt_is_over_water( const PRT_REF iprt )
{
    /// @author ZZ
    /// @details This function returns true if the particle is over a water tile
    Uint32 fan;

    if ( !ALLOCATED_PRT( iprt ) ) return false;

    fan = ego_mesh_get_grid( PMesh, PrtList.lst[iprt].pos.x, PrtList.lst[iprt].pos.y );
    if ( ego_mesh_grid_is_valid( PMesh, fan ) )
    {
        if ( 0 != ego_mesh_test_fx( PMesh, fan, MAPFX_WATER ) )  return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------
PIP_REF PipStack_get_free()
{
    int retval = INVALID_PIP_REF;

    if ( PipStack.count < MAX_PIP )
    {
        retval = PipStack.count;
        PipStack.count++;
    }

    return CLIP( retval, 0, MAX_PIP );
}

//--------------------------------------------------------------------------------------------
PIP_REF PipStack_load_one( const char *szLoadName, const PIP_REF pip_override )
{
    /// @author ZZ
    /// @details This function loads a particle template, returning MAX_PIP if the file wasn't
    ///    found

    PIP_REF ipip;
    pip_t * ppip;

    ipip = INVALID_PIP_REF;
    if ( VALID_PIP_RANGE( pip_override ) )
    {
        PipStack_release_one( pip_override );
        ipip = pip_override;
    }
    else
    {
        ipip = PipStack_get_free();
    }

    if ( !VALID_PIP_RANGE( ipip ) )
    {
        return INVALID_PIP_REF;
    }
    ppip = PipStack.get_ptr( ipip );

    if ( NULL == load_one_pip_file_vfs( szLoadName, ppip ) )
    {
        return INVALID_PIP_REF;
    }

    ppip->end_sound = CLIP<Sint8>( ppip->end_sound, INVALID_SOUND_ID, MAX_WAVE );
    ppip->soundspawn = CLIP<Sint8>( ppip->soundspawn, INVALID_SOUND_ID, MAX_WAVE );

    return ipip;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void PipStack_init_all()
{
    PIP_REF cnt;

    for ( cnt = 0; cnt < MAX_PIP; cnt++ )
    {
        pip_init( PipStack.get_ptr( cnt ) );
    }

    // Reset the pip stack "pointer"
    PipStack.count = 0;
}

//--------------------------------------------------------------------------------------------
void PipStack_release_all()
{
    PIP_REF cnt;
    int tnc;
    int max_request;

    max_request = 0;
    for ( cnt = 0, tnc = 0; cnt < MAX_PIP; cnt++ )
    {
        if ( LOADED_PIP( cnt ) )
        {
            pip_t * ppip = PipStack.get_ptr( cnt );

            max_request = std::max( max_request, ppip->request_count );
            tnc++;
        }
    }

    if ( tnc > 0 && max_request > 0 )
    {
        vfs_FILE * ftmp = vfs_openWriteB("/debug/pip_usage.txt");
        if ( NULL != ftmp )
        {
            vfs_printf( ftmp, "List of used pips\n\n" );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                if ( LOADED_PIP( cnt ) )
                {
                    pip_t * ppip = PipStack.get_ptr( cnt );
                    vfs_printf( ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n", REF_TO_INT( cnt ), ppip->name, ppip->create_count, ppip->request_count );
                }
            }

            vfs_close( ftmp );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                PipStack_release_one( cnt );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool PipStack_release_one( const PIP_REF ipip )
{
    pip_t * ppip;

    if ( !VALID_PIP_RANGE( ipip ) ) return false;
    ppip = PipStack.get_ptr( ipip );

    if ( !ppip->loaded ) return true;

    pip_init( ppip );

    ppip->loaded  = false;
    ppip->name[0] = CSTR_END;

    return true;
}

//--------------------------------------------------------------------------------------------
bool prt_request_terminate( prt_t * pprt )
{
    /// @author BB
    /// @details Tell the game to get rid of this object and treat it
    ///               as if it was already dead
    ///
    /// @note PrtList_request_terminate() will force the game to
    ///       (eventually) call end_one_particle_in_game() on this particle

    bool  is_visible;

    if ( NULL == pprt || !ALLOCATED_PPRT( pprt ) || TERMINATED_PPRT( pprt ) )
    {
        return false;
    }

    is_visible =
        pprt->size > 0 &&
        !pprt->is_hidden &&
        pprt->inst.alpha > 0.0f;

    if ( is_visible && 0 == pprt->obj_base.frame_count )
    {
        // turn the particle into a ghost
        pprt->is_ghost = true;
    }
    else
    {
        // the particle has already been seen or is not visible, so just
        // terminate it, as normal
        POBJ_REQUEST_TERMINATE( pprt );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
int prt_do_end_spawn( const PRT_REF iprt )
{
    int endspawn_count = 0;
    prt_t * pprt;

    if ( !ALLOCATED_PRT( iprt ) ) return endspawn_count;

    pprt = PrtList_get_ptr( iprt );

    // Spawn new particles if time for old one is up
    if ( pprt->endspawn_amount > 0 && _profileSystem.isValidProfileID( pprt->profile_ref ) && pprt->endspawn_lpip > -1 )
    {
        FACING_T facing;
        int      tnc;

        facing = pprt->facing;
        for ( tnc = 0; tnc < pprt->endspawn_amount; tnc++ )
        {
            // we have determined the absolute pip reference when the particle was spawned
            // so, set the profile reference to INVALID_PRO_REF, so that the
            // value of pprt->endspawn_lpip will be used directly
            PRT_REF spawned_prt = spawn_one_particle( pprt->pos_old, facing, pprt->profile_ref, pprt->endspawn_lpip,
                                  INVALID_CHR_REF, GRIP_LAST, pprt->team, prt_get_iowner( iprt, 0 ), iprt, tnc, pprt->target_ref );

            if ( DEFINED_PRT( spawned_prt ) )
            {
                endspawn_count++;
            }

            facing += pprt->endspawn_facingadd;
        }

        // we have already spawned these particles, so set this amount to
        // zero in case we are not actually calling end_one_particle_in_game()
        // this time around.
        pprt->endspawn_amount = 0;
    }

    return endspawn_count;
}

//--------------------------------------------------------------------------------------------
void cleanup_all_particles()
{
    PRT_REF iprt;

    // do end-of-life care for particles. Must iterate over all particles since the
    // number of particles could change inside this list
    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        prt_t *pprt;
        Ego::Entity *base_ptr;

        pprt = PrtList_get_ptr( iprt );

        base_ptr = POBJ_GET_PBASE( pprt );
        if ( !FLAG_ALLOCATED_PBASE( base_ptr ) ) continue;

        if ( TERMINATED_PBASE( base_ptr ) )
        {
            // now that the object is in the "killed" state,
            // actually put it back into the free store
            PrtList_free_one( GET_REF_PPRT( pprt ) );
        }
        else if ( STATE_WAITING_PBASE( base_ptr ) )
        {
            // do everything to end the particle in-game (spawn secondary particles,
            // play end sound, etc.) amd mark it with kill_me
            end_one_particle_in_game( iprt );
        }
    }
}

//--------------------------------------------------------------------------------------------
void bump_all_particles_update_counters()
{
    PRT_REF cnt;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        Ego::Entity *base_ptr;

        base_ptr = POBJ_GET_PBASE( PrtList_get_ptr( cnt ) );
        if ( !ACTIVE_PBASE( base_ptr ) ) continue;

        base_ptr->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_do_bump_damage( prt_bundle_t * pbdl_prt )
{
    // apply damage from  attatched bump particles (about once a second)

    CHR_REF ichr, iholder;
    Uint32  update_count;
    IPair   local_damage;
    int     max_damage, actual_damage;

    prt_t * loc_pprt;
    pip_t * loc_ppip;
    chr_t * loc_pchr;

    bool skewered_by_arrow;
    bool has_vulnie;
    bool is_immolated_by;
    bool no_protection_from;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    // this is often set to zero when the particle hits something
    max_damage = ABS( loc_pprt->damage.base ) + ABS( loc_pprt->damage.rand );

    // wait until the right time
    update_count = update_wld + loc_pprt->obj_base.guid;
    if ( 0 != ( update_count & 31 ) ) return pbdl_prt;

    /// @note ZF@> This is already checked in prt_update_ingame()
    // do nothing if the particle is hidden
    //if ( loc_pprt->is_hidden ) return;

    // we must be attached to something
    if ( !INGAME_CHR( loc_pprt->attachedto_ref ) ) return pbdl_prt;

    ichr     = loc_pprt->attachedto_ref;
    loc_pchr = ChrList_get_ptr( loc_pprt->attachedto_ref );

    // find out who is holding the owner of this object
    iholder = chr_get_lowest_attachment( ichr, true );
    if ( INVALID_CHR_REF == iholder ) iholder = ichr;

    // do nothing if you are attached to your owner
    if (( INVALID_CHR_REF != loc_pprt->owner_ref ) && ( iholder == loc_pprt->owner_ref || ichr == loc_pprt->owner_ref ) ) return pbdl_prt;

    //---- only do damage in certain cases:

    // 1) the particle has the DAMFX_ARRO bit
    skewered_by_arrow = HAS_SOME_BITS( loc_ppip->damfx, DAMFX_ARRO );

    // 2) the character is vulnerable to this damage type
    has_vulnie = chr_has_vulnie( GET_REF_PCHR( loc_pchr ), loc_pprt->profile_ref );

    // 3) the character is "lit on fire" by the particle damage type
    is_immolated_by = ( loc_pprt->damagetype < DAMAGE_COUNT && loc_pchr->reaffirm_damagetype == loc_pprt->damagetype );

    // 4) the character has no protection to the particle
    no_protection_from = ( 0 != max_damage ) && ( loc_pprt->damagetype < DAMAGE_COUNT ) && ( 0 == loc_pchr->damage_resistance[loc_pprt->damagetype] );

    if ( !skewered_by_arrow && !has_vulnie && !is_immolated_by && !no_protection_from )
    {
        return pbdl_prt;
    }

    if ( has_vulnie || is_immolated_by )
    {
        // the damage is the maximum damage over and over again until the particle dies
        range_to_pair( loc_ppip->damage, &local_damage );
    }
    else if ( no_protection_from )
    {
        // take a portion of whatever damage remains
        local_damage = loc_pprt->damage;
    }
    else
    {
        range_to_pair( loc_ppip->damage, &local_damage );

        local_damage.base /= 2;
        local_damage.rand /= 2;

        // distribute 1/2 of the maximum damage over the particle's lifetime
        if ( !loc_pprt->is_eternal )
        {
            // how many 32 update cycles will this particle live through?
            int cycles = loc_pprt->lifetime_total / 32;

            if ( cycles > 1 )
            {
                local_damage.base /= cycles;
                local_damage.rand /= cycles;
            }
        }
    }

    //---- special effects
    if ( loc_ppip->allowpush && 0 == loc_ppip->vel_hrz_pair.base )
    {
        // Make character limp
        ChrList.lst[ichr].vel.x *= 0.5f;
        ChrList.lst[ichr].vel.y *= 0.5f;
    }

    //---- do the damage
    actual_damage = damage_character( ichr, ATK_BEHIND, local_damage, loc_pprt->damagetype, loc_pprt->team, loc_pprt->owner_ref, loc_ppip->damfx, false );

    // adjust any remaining particle damage
    if ( loc_pprt->damage.base > 0 )
    {
        loc_pprt->damage.base -= actual_damage;
        loc_pprt->damage.base  = std::max( 0, loc_pprt->damage.base );

        // properly scale the random amount
        loc_pprt->damage.rand  = ABS( loc_ppip->damage.to - loc_ppip->damage.from ) * loc_pprt->damage.base / loc_ppip->damage.from;
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
int prt_do_contspawn( prt_bundle_t * pbdl_prt )
{
    /// Spawn new particles if continually spawning

    int      spawn_count = 0;
    FACING_T facing;
    unsigned tnc;

    prt_t             * loc_pprt;
    pip_t             * loc_ppip;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return spawn_count;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    if ( loc_ppip->contspawn_amount <= 0 || -1 == loc_ppip->contspawn_lpip )
    {
        return spawn_count;
    }

    if ( loc_pprt->contspawn_timer > 0 ) return spawn_count;

    // reset the spawn timer
    loc_pprt->contspawn_timer = loc_ppip->contspawn_delay;

    facing = loc_pprt->facing;
    for ( tnc = 0; tnc < loc_ppip->contspawn_amount; tnc++ )
    {
        PRT_REF prt_child = spawn_one_particle( prt_get_pos_v_const( loc_pprt ), facing, loc_pprt->profile_ref, loc_ppip->contspawn_lpip,
                                                INVALID_CHR_REF, GRIP_LAST, loc_pprt->team, loc_pprt->owner_ref, pbdl_prt->prt_ref, tnc, loc_pprt->target_ref );

        if ( DEFINED_PRT( prt_child ) )
        {
            // Inherit velocities from the particle we were spawned from, but only if it wasn't attached to something

            /// @note ZF@> I have disabled this at the moment. This is what caused the erratic particle movement for the Adventurer Torch
            /// @note BB@> taking out the test works, though  I should have checked vs. loc_pprt->attached_ref, anyway,
            ///            since we already specified that the particle is not attached in the function call :P
            /// @note ZF@> I have again disabled this. Is this really needed? It wasn't implemented before and causes
            ///            many, many, many issues with all particles around the game.
            //if( !ACTIVE_CHR( loc_pprt->attachedto_ref ) )
            /*{
                PrtList.lst[prt_child].vel.x += loc_pprt->vel.x;
                PrtList.lst[prt_child].vel.y += loc_pprt->vel.y;
                PrtList.lst[prt_child].vel.z += loc_pprt->vel.z;
            }*/

            //Keep count of how many were actually spawned
            spawn_count++;
        }

        facing += loc_ppip->contspawn_facingadd;
    }

    return spawn_count;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_update_do_water( prt_bundle_t * pbdl_prt )
{
    /// handle the particle interaction with water

    bool inwater;

    prt_t             * loc_pprt;
    pip_t             * loc_ppip;
    prt_environment_t * penviro;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );

    inwater = ( pbdl_prt->prt_ptr->pos.z < water.surface_level ) && ( 0 != ego_mesh_test_fx( PMesh, pbdl_prt->prt_ptr->onwhichgrid, MAPFX_WATER ) );

    if ( inwater && water.is_water && pbdl_prt->pip_ptr->end_water )
    {
        // Check for disaffirming character
        if ( INGAME_CHR( pbdl_prt->prt_ptr->attachedto_ref ) && pbdl_prt->prt_ptr->owner_ref == pbdl_prt->prt_ptr->attachedto_ref )
        {
            // Disaffirm the whole character
            disaffirm_attached_particles( pbdl_prt->prt_ptr->attachedto_ref );
        }
        else
        {
            // destroy the particle
            end_one_particle_in_game( pbdl_prt->prt_ref );
            return NULL;
        }
    }
    else if ( inwater )
    {
        bool  spawn_valid     = false;
        int     global_pip_index = -1;
        fvec3_t vtmp            = fvec3_t( pbdl_prt->prt_ptr->pos.x, pbdl_prt->prt_ptr->pos.y, water.surface_level );

        if ( INVALID_CHR_REF == pbdl_prt->prt_ptr->owner_ref && ( PIP_SPLASH == pbdl_prt->prt_ptr->pip_ref || PIP_RIPPLE == pbdl_prt->prt_ptr->pip_ref ) )
        {
            /* do not spawn anything for a splash or a ripple */
            spawn_valid = false;
        }
        else
        {
            if ( !pbdl_prt->prt_ptr->enviro.inwater )
            {
                if ( SPRITE_SOLID == pbdl_prt->prt_ptr->type )
                {
                    global_pip_index = PIP_SPLASH;
                }
                else
                {
                    global_pip_index = PIP_RIPPLE;
                }
                spawn_valid = true;
            }
            else
            {
                if ( SPRITE_SOLID == pbdl_prt->prt_ptr->type && !INGAME_CHR( pbdl_prt->prt_ptr->attachedto_ref ) )
                {
                    // only spawn ripples if you are touching the water surface!
                    if ( pbdl_prt->prt_ptr->pos.z + pbdl_prt->prt_ptr->bump_real.height > water.surface_level && pbdl_prt->prt_ptr->pos.z - pbdl_prt->prt_ptr->bump_real.height < water.surface_level )
                    {
                        int ripand = ~(( ~RIPPLEAND ) << 1 );
                        if ( 0 == (( update_wld + pbdl_prt->prt_ptr->obj_base.guid ) & ripand ) )
                        {

                            spawn_valid = true;
                            global_pip_index = PIP_RIPPLE;
                        }
                    }
                }
            }
        }

        if ( spawn_valid )
        {
            // Splash for particles is just a ripple
            spawn_one_particle_global( vtmp, 0, global_pip_index, 0 );
        }

        pbdl_prt->prt_ptr->enviro.inwater  = true;
    }
    else
    {
        pbdl_prt->prt_ptr->enviro.inwater = false;
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_update_animation( prt_bundle_t * pbdl_prt )
{
    /// animate the particle

    prt_t             * loc_pprt;
    pip_t             * loc_ppip;
    bool              image_overflow;
    Uint16              image_overflow_amount;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    image_overflow = false;
    image_overflow_amount = 0;
    if ( loc_pprt->image_off >= loc_pprt->image_max )
    {
        // how did the image get here?
        image_overflow = true;

        // cast the unsigned integers to a larger type to make sure there are no overflows
        image_overflow_amount = (( signed )loc_pprt->image_off ) + (( signed )loc_pprt->image_add ) - (( signed )loc_pprt->image_max );
    }
    else
    {
        // the image is in the correct range
        if (( loc_pprt->image_max - loc_pprt->image_off ) > loc_pprt->image_add )
        {
            // the image will not overflow this update
            loc_pprt->image_off = loc_pprt->image_off + loc_pprt->image_add;
        }
        else
        {
            image_overflow = true;
            image_overflow_amount = (( signed )loc_pprt->image_off ) + (( signed )loc_pprt->image_add ) - (( signed )loc_pprt->image_max );
        }
    }

    // what do you do about an image overflow?
    if ( image_overflow )
    {
        if ( loc_ppip->end_lastframe && loc_ppip->end_time > 0 )
        {
            // the animation is looped. set the value to image_overflow_amount
            // so that we get the exact number of image updates called for
            loc_pprt->image_off  = image_overflow_amount;
        }
        else
        {
            // freeze it at the last frame
            loc_pprt->image_off = std::max( 0, ( signed )loc_pprt->image_max - 1 );
        }
    }

    // rotate the particle
    loc_pprt->rotate += loc_pprt->rotate_add;

    // update the particle size
    if ( 0 != loc_pprt->size_add )
    {
        int size_new;

        // resize the paricle
        size_new = loc_pprt->size + loc_pprt->size_add;
        size_new = CLIP( size_new, 0, 0xFFFF );

        prt_set_size( loc_pprt, size_new );
    }

    // spin the particle
    loc_pprt->facing += loc_ppip->facingadd;

    // frames_remaining refers to the number of animation updates, not the
    // number of frames displayed
    if ( loc_pprt->frames_remaining > 0 )
    {
        loc_pprt->frames_remaining--;
    }

    // the animation has terminated
    if ( loc_ppip->end_lastframe && 0 == loc_pprt->frames_remaining )
    {
        end_one_particle_in_game( pbdl_prt->prt_ref );
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_update_dynalight( prt_bundle_t * pbdl_prt )
{
    prt_t             * loc_pprt;
    pip_t             * loc_ppip;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    // Change dyna light values
    if ( loc_pprt->dynalight.level > 0 )
    {
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
        if ( loc_pprt->dynalight.level < 0 ) loc_pprt->dynalight.level = 0;
    }
    else if ( loc_pprt->dynalight.level < 0 )
    {
        // try to guess what should happen for negative lighting
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
        if ( loc_pprt->dynalight.level > 0 ) loc_pprt->dynalight.level = 0;
    }
    else
    {
        loc_pprt->dynalight.level += loc_ppip->dynalight.level_add;
    }

    loc_pprt->dynalight.falloff += loc_ppip->dynalight.falloff_add;

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_update_timers( prt_bundle_t * pbdl_prt )
{
    prt_t             * loc_pprt;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;

    // down the remaining lifetime of the particle
    if ( loc_pprt->lifetime_remaining > 0 )
    {
        loc_pprt->lifetime_remaining--;
    }

    // down the continuous spawn timer
    if ( loc_pprt->contspawn_timer > 0 )
    {
        loc_pprt->contspawn_timer--;
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_update_ingame( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details update everything about a particle that does not depend on collisions
    ///               or interactions with characters

    Ego::Entity *base_ptr;
    prt_t *loc_pprt;
    pip_t *loc_ppip;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;
    base_ptr = POBJ_GET_PBASE( loc_pprt );

    // determine whether the pbdl_prt->prt_ref is hidden
    loc_pprt->is_hidden = false;
    if ( INGAME_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->is_hidden = ChrList.lst[loc_pprt->attachedto_ref].is_hidden;
    }

    // nothing to do if the particle is hidden
    if ( loc_pprt->is_hidden ) return pbdl_prt;

    // clear out the attachment if the character doesn't exist at all
    if ( !DEFINED_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->attachedto_ref = INVALID_CHR_REF;
    }

    // figure out where the particle is on the mesh and update the particle states
    {
        // determine whether the pbdl_prt->prt_ref is hidden
        loc_pprt->is_hidden = false;
        if ( INGAME_CHR( loc_pprt->attachedto_ref ) )
        {
            loc_pprt->is_hidden = ChrList.lst[loc_pprt->attachedto_ref].is_hidden;
        }

        loc_pprt->is_homing = loc_ppip->homing && !INGAME_CHR( loc_pprt->attachedto_ref ) && INGAME_CHR( loc_pprt->target_ref );
    }

    // figure out where the particle is on the mesh and update pbdl_prt->prt_ref states
    pbdl_prt = prt_update_do_water( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == loc_pprt ) return pbdl_prt;

    // the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pbdl_prt;

    pbdl_prt = prt_update_animation( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

    pbdl_prt = prt_update_dynalight( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

    pbdl_prt = prt_update_timers( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

    prt_do_contspawn( pbdl_prt );
    if ( NULL == pbdl_prt->prt_ptr ) return NULL;

    pbdl_prt = prt_do_bump_damage( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;

    base_ptr->update_count++;

    // If the particle is done updating, remove it from the game, but do not kill it
    if ( !loc_pprt->is_eternal && ( base_ptr->update_count > 0 && 0 == loc_pprt->lifetime_remaining ) )
    {
        end_one_particle_in_game( pbdl_prt->prt_ref );
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_update_ghost( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details handle the case where the particle is still being diaplayed, but is no longer
    ///               in the game

    bool prt_visible;

    Ego::Entity *base_ptr;
    prt_t *loc_pprt;
    pip_t *loc_ppip;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;
    base_ptr = POBJ_GET_PBASE( loc_pprt );

    // is this the right function?
    if ( !loc_pprt->is_ghost )
        return pbdl_prt;

    // is the prt visible
    prt_visible = ( loc_pprt->size > 0 ) && ( loc_pprt->inst.alpha > 0 ) && !loc_pprt->is_hidden;

    // are we done?
    if ( !prt_visible || base_ptr->frame_count > 0 )
    {
        prt_request_terminate( pbdl_prt->prt_ptr );
        return NULL;
    }

    // clear out the attachment if the character doesn't exist at all
    if ( !DEFINED_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->attachedto_ref = INVALID_CHR_REF;
    }

    // determine whether the pbdl_prt->prt_ref is hidden
    loc_pprt->is_hidden = false;
    if ( INGAME_CHR( loc_pprt->attachedto_ref ) )
    {
        loc_pprt->is_hidden = ChrList.lst[loc_pprt->attachedto_ref].is_hidden;
    }

    loc_pprt->is_homing = loc_ppip->homing && !INGAME_CHR( loc_pprt->attachedto_ref ) && INGAME_CHR( loc_pprt->target_ref );

    // the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pbdl_prt;

    pbdl_prt = prt_update_animation( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == loc_pprt ) return NULL;

    pbdl_prt = prt_update_dynalight( pbdl_prt );
    if ( NULL == pbdl_prt || NULL == loc_pprt ) return NULL;

    if ( !loc_pprt->is_hidden )
    {
        base_ptr->update_count++;
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_update( prt_bundle_t * pbdl_prt )
{
    Ego::Entity *loc_pbase;
    prt_t *loc_pprt,
		  *tmp_pprt;
    pip_t *loc_ppip;
    prt_environment_t *penviro;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return NULL;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;
    penviro  = &( loc_pprt->enviro );
    loc_pbase = POBJ_GET_PBASE( loc_pprt );

    // do the next step in the particle configuration
    tmp_pprt = prt_run_config( pbdl_prt->prt_ptr );
    if ( NULL == tmp_pprt ) { prt_bundle_ctor( pbdl_prt ); return NULL; }

    if ( tmp_pprt != pbdl_prt->prt_ptr )
    {
        // "new" particle, so re-validate the bundle
        prt_bundle_set( pbdl_prt, pbdl_prt->prt_ptr );
    }

    // if the bundle is no longer valid, return
    if ( NULL == pbdl_prt->prt_ptr || NULL == pbdl_prt->pip_ptr ) return pbdl_prt;

    // if the particle is no longer allocated, return
    if ( !ALLOCATED_PPRT( pbdl_prt->prt_ptr ) ) return pbdl_prt;

    // handle different particle states differently
    if ( loc_pprt->is_ghost )
    {
        // the particle is not on
        pbdl_prt = prt_update_ghost( pbdl_prt );
    }
    else
    {
        // the particle is on
        pbdl_prt = prt_update_ingame( pbdl_prt );
    }

    return pbdl_prt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool prt_update_safe_raw( prt_t * pprt )
{
    bool retval = false;

    BIT_FIELD hit_a_wall;
    float  pressure;

    if ( !ALLOCATED_PPRT( pprt ) ) return false;

    hit_a_wall = prt_hit_wall( pprt, NULL, NULL, &pressure, NULL );
    if (( 0 == hit_a_wall ) && ( 0.0f == pressure ) )
    {
        pprt->safe_valid = true;
        prt_get_pos(pprt, pprt->safe_pos);
        pprt->safe_time  = update_wld;
        pprt->safe_grid  = ego_mesh_get_grid( PMesh, pprt->pos.x, pprt->pos.y );

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool prt_update_safe( prt_t * pprt, bool force )
{
    Uint32 new_grid;
    bool retval = false;
    bool needs_update = false;

    if ( !ALLOCATED_PPRT( pprt ) ) return false;

    if ( force || !pprt->safe_valid )
    {
        needs_update = true;
    }
    else
    {
        new_grid = ego_mesh_get_grid( PMesh, pprt->pos.x, pprt->pos.y );

        if ( INVALID_TILE == new_grid )
        {
            if ( ABS( pprt->pos.x - pprt->safe_pos.x ) > GRID_FSIZE ||
                 ABS( pprt->pos.y - pprt->safe_pos.y ) > GRID_FSIZE )
            {
                needs_update = true;
            }
        }
        else if ( new_grid != pprt->safe_grid )
        {
            needs_update = true;
        }
    }

    if ( needs_update )
    {
        retval = prt_update_safe_raw( pprt );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool prt_update_pos( prt_t * pprt )
{
    if ( !ALLOCATED_PPRT( pprt ) ) return false;

    pprt->onwhichgrid  = ego_mesh_get_grid( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->onwhichblock = ego_mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

    // update whether the current character position is safe
    prt_update_safe( pprt, false );

    // update the breadcrumb list (does not exist for particles )
    // prt_update_breadcrumb( pprt, false );

    return true;
}

//--------------------------------------------------------------------------------------------
bool prt_set_pos(prt_t *pprt, const fvec3_t& pos)
{
	bool retval = false;
	if (!ALLOCATED_PPRT(pprt)) return retval;
	retval = true;
	/// @todo Use overloaded != operator.
	if ((pos[kX] != pprt->pos.v[kX]) || (pos[kY] != pprt->pos.v[kY]) || (pos[kZ] != pprt->pos.v[kZ]))
	{
		pprt->pos = pos;
		retval = prt_update_pos(pprt);
	}
	return retval;
}

//--------------------------------------------------------------------------------------------
//Inline below
//--------------------------------------------------------------------------------------------

PIP_REF prt_get_ipip( const PRT_REF iprt )
{
    prt_t * pprt;

    if ( !DEFINED_PRT( iprt ) ) return INVALID_PIP_REF;
    pprt = PrtList_get_ptr( iprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return INVALID_PIP_REF;

    return pprt->pip_ref;
}

//--------------------------------------------------------------------------------------------
pip_t * prt_get_ppip( const PRT_REF iprt )
{
    prt_t * pprt;

    if ( !DEFINED_PRT( iprt ) ) return NULL;
    pprt = PrtList_get_ptr( iprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return NULL;

    return PipStack.get_ptr( pprt->pip_ref );
}

//--------------------------------------------------------------------------------------------
bool prt_set_size( prt_t * pprt, int size )
{
    pip_t *ppip;

    if ( !DEFINED_PPRT( pprt ) ) return false;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return false;
    ppip = PipStack.get_ptr( pprt->pip_ref );

    // set the graphical size
    pprt->size = size;

    // set the bumper size, if available
    if ( 0 == pprt->bump_size_stt )
    {
        // make the particle non-interacting if the initial bumper size was 0
        pprt->bump_real.size   = 0;
        pprt->bump_padded.size = 0;
    }
    else
    {
        float real_size  = FP8_TO_FLOAT( size ) * prt_get_scale( pprt );

        if ( 0.0f == pprt->bump_real.size || 0.0f == size )
        {
            // just set the size, assuming a spherical particle
            pprt->bump_real.size     = real_size;
            pprt->bump_real.size_big = real_size * SQRT_TWO;
            pprt->bump_real.height   = real_size;
        }
        else
        {
            float mag = real_size / pprt->bump_real.size;

            // resize all dimensions equally
            pprt->bump_real.size     *= mag;
            pprt->bump_real.size_big *= mag;
            pprt->bump_real.height   *= mag;
        }

        // make sure that the virtual bumper size is at least as big as what is in the pip file
        pprt->bump_padded.size     = std::max( pprt->bump_real.size,     ((float)ppip->bump_size) );
        pprt->bump_padded.size_big = std::max( pprt->bump_real.size_big, ((float)ppip->bump_size) * ((float)SQRT_TWO) );
        pprt->bump_padded.height   = std::max( pprt->bump_real.height,   ((float)ppip->bump_height) );
    }

    // set the real size of the particle
    oct_bb_set_bumper( &( pprt->prt_min_cv ), pprt->bump_real );

    // use the padded bumper to figure out the chr_max_cv
    oct_bb_set_bumper( &( pprt->prt_max_cv ), pprt->bump_padded );

    return true;
}

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_iowner( const PRT_REF iprt, int depth )
{
    /// @author BB
    /// @details A helper function for determining the owner of a paricle
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
    /// @note this function should be completely trivial for anything other than
    ///       damage particles created by an explosion

    CHR_REF iowner = INVALID_CHR_REF;

    prt_t * pprt;

    // be careful because this can be recursive
    if ( depth > ( int )maxparticles - ( int )PrtList.free_count ) return INVALID_CHR_REF;

    if ( !DEFINED_PRT( iprt ) ) return INVALID_CHR_REF;
    pprt = PrtList_get_ptr( iprt );

    if ( DEFINED_CHR( pprt->owner_ref ) )
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
            pprt->parent_ref = INVALID_PRT_REF;
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
                pprt->parent_ref = INVALID_PRT_REF;
                pprt->parent_guid = 0xFFFFFFFF;
            }
        }
    }

    return iowner;
}

//--------------------------------------------------------------------------------------------
float prt_get_scale( prt_t * pprt )
{
    /// @author BB
    /// @details get the scale factor between the "graphical size" of the particle and the actual
    ///               display size

    float scale = 0.25f;

    if ( !DEFINED_PPRT( pprt ) ) return scale;

    // set some particle dependent properties
    switch ( pprt->type )
    {
        case SPRITE_SOLID: scale *= 0.9384f; break;
        case SPRITE_ALPHA: scale *= 0.9353f; break;
        case SPRITE_LIGHT: scale *= 1.5912f; break;
    }

    return scale;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_bundle_ctor( prt_bundle_t * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    pbundle->prt_ref = INVALID_PRT_REF;
    pbundle->prt_ptr = NULL;

    pbundle->pip_ref = INVALID_PIP_REF;
    pbundle->pip_ptr = NULL;

    return pbundle;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_bundle_validate( prt_bundle_t * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    if ( ALLOCATED_PRT( pbundle->prt_ref ) )
    {
        pbundle->prt_ptr = PrtList_get_ptr( pbundle->prt_ref );
    }
    else if ( NULL != pbundle->prt_ptr )
    {
        pbundle->prt_ref = GET_REF_PPRT( pbundle->prt_ptr );
    }
    else
    {
        pbundle->prt_ref = INVALID_PRT_REF;
        pbundle->prt_ptr = NULL;
    }

    if ( !LOADED_PIP( pbundle->pip_ref ) && NULL != pbundle->prt_ptr )
    {
        pbundle->pip_ref = pbundle->prt_ptr->pip_ref;
    }

    if ( LOADED_PIP( pbundle->pip_ref ) )
    {
        pbundle->pip_ptr = PipStack.get_ptr( pbundle->pip_ref );
    }
    else
    {
        pbundle->pip_ref = INVALID_PIP_REF;
        pbundle->pip_ptr = NULL;
    }

    return pbundle;
}

//--------------------------------------------------------------------------------------------
prt_bundle_t * prt_bundle_set( prt_bundle_t * pbundle, prt_t * pprt )
{
    if ( NULL == pbundle ) return NULL;

    // blank out old data
    pbundle = prt_bundle_ctor( pbundle );

    if ( NULL == pbundle || NULL == pprt ) return pbundle;

    // set the particle pointer
    pbundle->prt_ptr = pprt;

    // validate the particle data
    pbundle = prt_bundle_validate( pbundle );

    return pbundle;
}

//--------------------------------------------------------------------------------------------
bool prt_get_pos(const prt_t *self, fvec3_t& position)
{
	if (!ALLOCATED_PPRT(self)) return false;
	position = self->pos;
	return true;
}
bool prt_get_pos(const prt_t *self, fvec3_base_t position)
{
    float *copy_rv;

    if (!ALLOCATED_PPRT(self)) return false;

    copy_rv = fvec3_base_copy(position, self->pos.v);

    return ( NULL == copy_rv ) ? false : true;
}

//--------------------------------------------------------------------------------------------
const fvec3_t& prt_get_pos_v_const(const prt_t *pprt)
{
    if (!ALLOCATED_PPRT(pprt)) return fvec3_t::zero;
    return pprt->pos;
}

//--------------------------------------------------------------------------------------------
float *prt_get_pos_v(prt_t *pprt)
{
    static fvec3_t vtmp = fvec3_t::zero;
    if (!ALLOCATED_PPRT(pprt)) return vtmp.v;
    return pprt->pos.v;
}
