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

#include "particle.inl"

#include "PrtList.h"

#include "log.h"
#include "sound.h"
#include "camera.h"
#include "mesh.inl"
#include "game.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo.h"

#include "egoboo_mem.h"

#include "enchant.inl"
#include "mad.h"
#include "profile.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define PRT_TRANS 0x80

const float buoyancy_friction = 0.2f;          // how fast does a "cloud-like" object slow down?

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_stoppedby_tests = 0;
int prt_pressure_tests = 0;

INSTANTIATE_STACK( ACCESS_TYPE_NONE, pip_t, PipStack, MAX_PIP );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t  prt_free( prt_t * pprt );

static prt_t * prt_config_ctor( prt_t * pprt );
static prt_t * prt_config_init( prt_t * pprt );
static prt_t * prt_config_active( prt_t * pprt );
static prt_t * prt_config_deinit( prt_t * pprt );
static prt_t * prt_config_dtor( prt_t * pprt );

static prt_t * prt_config_do_init( prt_t * pprt );
static prt_t * prt_config_do_active( prt_t * pprt );
static prt_t * prt_config_do_deinit( prt_t * pprt );

int prt_do_endspawn( const PRT_REF by_reference iprt );
int prt_do_contspawn( prt_t * pprt, pip_t * ppip );
void prt_do_bump_damage( prt_t * pprt, pip_t * ppip );

void prt_update_animation( prt_t * pprt, pip_t * ppip );
void prt_update_dynalight( prt_t * pprt, pip_t * ppip );
void prt_update_timers( prt_t * pprt );
prt_t * prt_update_do_water( prt_t * pprt, pip_t * ppip );
prt_t * prt_update_ingame( prt_t * pprt, pip_t * ppip  );
prt_t * prt_update_display( prt_t * pprt, pip_t * ppip );
prt_t * prt_update( prt_t * pprt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t prt_free( prt_t * pprt )
{
    if ( !ALLOCATED_PPRT( pprt ) ) return bfalse;

    // do not allow this if you are inside a particle loop
    EGOBOO_ASSERT( 0 == prt_loop_depth );

    if ( TERMINATED_PPRT( pprt ) ) return btrue;

    // deallocate any dynamic data
    BSP_leaf_dtor( &( pprt->bsp_leaf ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_ctor( prt_t * pprt )
{
    /// BB@> Set all particle parameters to safe values.
    ///      @details The c equivalent of the particle prt::new() function.

    ego_object_base_t save_base;
    ego_object_base_t * base_ptr;

    // save the base object data, do not construct it with this function.
    base_ptr = POBJ_GET_PBASE( pprt );
	if( NULL == base_ptr ) return NULL;

    memcpy( &save_base, base_ptr, sizeof( save_base ) );

    memset( pprt, 0, sizeof( *pprt ) );

    // restore the base object data
    memcpy( base_ptr, &save_base, sizeof( save_base ) );

    // "no lifetime" = "eternal"
    pprt->lifetime           = ( size_t )( ~0 );
    pprt->lifetime_remaining = pprt->lifetime;
    pprt->frames_remaining   = ( size_t )( ~0 );

    pprt->pip_ref      = MAX_PIP;
    pprt->profile_ref  = MAX_PROFILE;

    pprt->attachedto_ref = ( CHR_REF )MAX_CHR;
    pprt->owner_ref      = ( CHR_REF )MAX_CHR;
    pprt->target_ref     = ( CHR_REF )MAX_CHR;
    pprt->parent_ref     = TOTAL_MAX_PRT;
    pprt->parent_guid    = 0xFFFFFFFF;

    pprt->onwhichplatform = ( CHR_REF )MAX_CHR;

    // initialize the bsp node for this particle
    BSP_leaf_ctor( &( pprt->bsp_leaf ), 3, pprt, 2 );
    pprt->bsp_leaf.index = GET_INDEX_PPRT( pprt );

    pprt->obj_base.state = ego_object_initializing;

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_dtor( prt_t * pprt )
{
	if( NULL == pprt ) return pprt;

    // destruct/free any allocated data
    prt_free( pprt );

    // Destroy the base object.
    // Sets the state to ego_object_terminated automatically.
    POBJ_TERMINATE( pprt );

	return pprt;
}

//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
s_prt::s_prt() { prt_ctor( this ); }
s_prt::~s_prt() { prt_dtor( this ); }
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_count_free()
{
    return PrtList.free_count;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void play_particle_sound( const PRT_REF by_reference particle, Sint8 sound )
{
    /// ZZ@> This function plays a sound effect for a particle

    prt_t * pprt;

    if ( !DEFINED_PRT( particle ) ) return;
    pprt = PrtList.lst + particle;

    if ( !VALID_SND( sound ) ) return;

    if ( LOADED_PRO( pprt->profile_ref ) )
    {
        sound_play_chunk( prt_get_pos(pprt), pro_get_chunk( pprt->profile_ref, sound ) );
    }
    else
    {
        sound_play_chunk( prt_get_pos(pprt), g_wavelist[sound] );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_in_game( const PRT_REF by_reference particle )
{
    /// @details ZZ@> This function sticks a particle back on the free particle stack and
    ///    plays the sound associated with the particle
    ///
    /// @note BB@> Use prt_request_terminate() instead of calling this function directly.
    ///            Requesting termination will defer the actual deletion of a particle until
    ///            it is finally destroyed by cleanup_all_particles()

    CHR_REF child;
    prt_t * pprt;

    if ( !ALLOCATED_PRT( particle ) ) return;
    pprt = PrtList.lst + particle;

    if ( DEFINED_PRT( particle ) )
    {
        // the particle has valid data

        if ( pprt->spawncharacterstate )
        {
            child = spawn_one_character( prt_get_pos(pprt), pprt->profile_ref, pprt->team, 0, pprt->facing, NULL, ( CHR_REF )MAX_CHR );
            if ( INGAME_CHR( child ) )
            {
                chr_get_pai( child )->state = pprt->spawncharacterstate;
                chr_get_pai( child )->owner = pprt->owner_ref;
            }
        }

        if ( LOADED_PIP( pprt->pip_ref ) )
        {
            play_particle_sound( particle, PipStack.lst[pprt->pip_ref].soundend );
        }
    }

    PrtList_free_one( particle );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_t * prt_config_do_init( prt_t * pprt )
{
    PRT_REF            iprt;
    pip_t            * ppip;
    prt_spawn_data_t * pdata;

    int     velocity;
    fvec3_t vel;
    float   tvel;
    int     offsetfacing = 0, newrand;
    Uint32  prt_lifetime;
    fvec3_t tmp_pos;
    Uint16  turn;

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
                   REF_TO_INT( pdata->iprofile ), LOADED_PRO( pdata->iprofile ) ? ProList.lst[pdata->iprofile].name : "INVALID" );

        return NULL;
    }
    ppip = PipStack.lst + pdata->ipip;

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
    pprt->dynalight.on = bfalse;
    if ( 0 == pdata->multispawn )
    {
        pprt->dynalight.on = ppip->dynalight.mode;
        if ( DYNA_MODE_LOCAL == ppip->dynalight.mode )
        {
            pprt->dynalight.on = DYNA_MODE_OFF;
        }
    }

    // Set character attachments ( pdata->chr_attach==MAX_CHR means none )
    pprt->attachedto_ref     = pdata->chr_attach;
    pprt->attachedto_vrt_off = pdata->vrt_offset;

    // Correct loc_facing
    loc_facing += ppip->facing_pair.base;

    // Targeting...
    vel.z = 0;

    pprt->offset.z = generate_randmask( ppip->spacing_vrt_pair.base, ppip->spacing_vrt_pair.rand ) - ( ppip->spacing_vrt_pair.rand >> 1 );
    tmp_pos.z += pprt->offset.z;
    velocity = generate_randmask( ppip->vel_hrz_pair.base, ppip->vel_hrz_pair.rand );
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
            // Find a target
            pprt->target_ref = prt_find_target( pdata->pos.x, pdata->pos.y, pdata->pos.z, loc_facing, pdata->ipip, pdata->team, loc_chr_origin, pdata->oldtarget );
            if ( DEFINED_CHR( pprt->target_ref ) && !ppip->homing )
            {
                loc_facing -= glouseangle;		//?What does this do?!
            }

            // Correct loc_facing for dexterity...
            offsetfacing = 0;
            if ( ChrList.lst[loc_chr_origin].dexterity < PERFECTSTAT )
            {
                // Correct loc_facing for randomness
                offsetfacing  = generate_randmask( 0, ppip->facing_pair.rand ) - ( ppip->facing_pair.rand >> 1 );
                offsetfacing  = ( offsetfacing * ( PERFECTSTAT - ChrList.lst[loc_chr_origin].dexterity ) ) / PERFECTSTAT;
            }

            if ( DEFINED_CHR( pprt->target_ref ) && ppip->zaimspd != 0 )
            {
                // These aren't velocities...  This is to do aiming on the Z axis
                if ( velocity > 0 )
                {
                    vel.x = ChrList.lst[pprt->target_ref].pos.x - pdata->pos.x;
                    vel.y = ChrList.lst[pprt->target_ref].pos.y - pdata->pos.y;
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
        if ( !DEFINED_CHR( pprt->target_ref ) && ppip->needtarget )
        {
            prt_request_terminate( iprt );
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
        offsetfacing = generate_randmask( 0,  ppip->facing_pair.rand ) - ( ppip->facing_pair.rand >> 1 );
    }
    loc_facing += offsetfacing;
    pprt->facing = loc_facing;

    // this is actually pointing in the opposite direction?
    turn = TO_TURN( loc_facing );

    // Location data from arguments
    newrand = generate_randmask( ppip->spacing_hrz_pair.base, ppip->spacing_hrz_pair.rand );
    pprt->offset.x = -turntocos[ turn ] * newrand;
    pprt->offset.y = -turntosin[ turn ] * newrand;

    tmp_pos.x += pprt->offset.x;
    tmp_pos.y += pprt->offset.y;

    tmp_pos.x = CLIP( tmp_pos.x, 0, PMesh->gmem.edge_x - 2 );
    tmp_pos.y = CLIP( tmp_pos.y, 0, PMesh->gmem.edge_y - 2 );

	prt_set_pos( pprt, tmp_pos.v );
    pprt->pos_old  = tmp_pos;
    pprt->pos_stt  = tmp_pos;

    // Velocity data
    vel.x = -turntocos[ turn ] * velocity;
    vel.y = -turntosin[ turn ] * velocity;
    vel.z += generate_randmask( ppip->vel_vrt_pair.base, ppip->vel_vrt_pair.rand ) - ( ppip->vel_vrt_pair.rand >> 1 );
    pprt->vel = pprt->vel_old = pprt->vel_stt = vel;

    // Template values
    pprt->bump_size_stt = ppip->bump_size;
    pprt->type          = ppip->type;

    // Image data
    pprt->rotate        = generate_irand_pair( ppip->rotate_pair );
    pprt->rotate_add    = ppip->rotate_add;

	pprt->size_stt      = ppip->size_base;
    pprt->size_add      = ppip->size_add;

    pprt->image_stt     = INT_TO_FP8( ppip->image_base );
    pprt->image_add     = generate_irand_pair( ppip->image_add );
    pprt->image_max     = INT_TO_FP8( ppip->numframes );

	// figure out the actual particle lifetime
    prt_lifetime        = ppip->time;
    if ( ppip->endlastframe && pprt->image_add != 0 )
    {
        if ( 0 == ppip->time )
        {
            // Part time is set to 1 cycle
            int frames = ( pprt->image_max / pprt->image_add ) - 1;
            prt_lifetime = frames;
        }
        else
        {
            // Part time is used to give number of cycles
            int frames = (( pprt->image_max / pprt->image_add ) - 1 );
            prt_lifetime = ppip->time * frames;
        }
    }

    // "no lifetime" = "eternal"
    if ( 0 == prt_lifetime )
    {
        pprt->lifetime           = ( size_t )( ~0 );
        pprt->lifetime_remaining = pprt->lifetime;
        pprt->is_eternal         = btrue;
    }
    else
    {
        // the lifetime is really supposed tp be in terms of frames, but
        // to keep the number of updates stable, the frames could lag.
        // sooo... we just rescale the prt_lifetime so that it will work with the
        // updates and cross our fingers
        pprt->lifetime           = ceil(( float ) prt_lifetime * ( float )TARGET_UPS / ( float )TARGET_FPS );
        pprt->lifetime_remaining = pprt->lifetime;
    }

    // make the particle display AT LEAST one frame, regardless of how many updates
    // it has or when someone requests for it to terminate
    pprt->frames_remaining = MAX( 1, prt_lifetime );

    // Damage stuff
    range_to_pair( ppip->damage, &( pprt->damage ) );

    // Spawning data
    pprt->contspawn_delay = ppip->contspawn_delay;
    if ( pprt->contspawn_delay != 0 )
    {
        pprt->contspawn_delay = 1;
        if ( DEFINED_CHR( pprt->attachedto_ref ) )
        {
            pprt->contspawn_delay++; // Because attachment takes an update before it happens
        }
    }

    // Sound effect
    play_particle_sound( iprt, ppip->soundspawn );

    // set up the particle transparency
    pprt->inst.alpha = 0xFF;
    switch ( pprt->inst.type )
    {
        case SPRITE_SOLID: break;
        case SPRITE_ALPHA: pprt->inst.alpha = PRT_TRANS; break;
        case SPRITE_LIGHT: break;
    }

    // is the spawn location safe?
	if( 0 == prt_hit_wall( pprt, tmp_pos.v, NULL, NULL ) )
	{
		pprt->safe_pos   = tmp_pos;
		pprt->safe_valid = btrue;
		pprt->safe_grid  = pprt->onwhichgrid;
	}

    // get an initial value for the is_homing variable
    pprt->is_homing = ppip->homing && !DEFINED_CHR( pprt->attachedto_ref );

    prt_set_size( pprt, ppip->size_base );

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
               loc_chr_origin, DEFINED_CHR( loc_chr_origin ) ? ChrList.lst[loc_chr_origin].Name : "INVALID",
               pdata->ipip, ( NULL != ppip ) ? ppip->name : "INVALID", ( NULL != ppip ) ? ppip->comment : "",
               pdata->iprofile, LOADED_PRO( pdata->iprofile ) ? ProList.lst[pdata->iprofile].name : "INVALID" );
#endif

    // count out all the requests for this particle type
    ppip->prt_create_count++;

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
	if( NULL == pprt ) return pprt;

	// go to the next state
	pprt->obj_base.state = ego_object_destructing;

	return pprt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_t * prt_config_construct( prt_t * pprt, int max_iterations )
{
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_constructing + 1 ) )
    {
        prt_t * tmp_prt = prt_config_deconstruct( pprt, max_iterations );
        if ( tmp_prt == pprt ) return NULL;
    }

    iterations = 0;
    while ( NULL != pprt && pbase->state <= ego_object_constructing && iterations < max_iterations )
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
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_initializing + 1 ) )
    {
        prt_t * tmp_prt = prt_config_deconstruct( pprt, max_iterations );
        if ( tmp_prt == pprt ) return NULL;
    }

    iterations = 0;
    while ( NULL != pprt && pbase->state <= ego_object_initializing && iterations < max_iterations )
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
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it and start over
    if ( pbase->state > ( int )( ego_object_active + 1 ) )
    {
        prt_t * tmp_prt = prt_config_deconstruct( pprt, max_iterations );
        if ( tmp_prt == pprt ) return NULL;
    }

    iterations = 0;
    while ( NULL != pprt && pbase->state < ego_object_active && iterations < max_iterations )
    {
        prt_t * ptmp = prt_run_config( pprt );
        if ( ptmp != pprt ) return NULL;
        iterations++;
    }

	assert( pbase->state == ego_object_active );
	if( pbase->state == ego_object_active )
	{
		PrtList_add_used( GET_INDEX_PPRT( pprt ) );
	}

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_deinitialize( prt_t * pprt, int max_iterations )
{
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deinitialize it
    if ( pbase->state > ( int )( ego_object_deinitializing + 1 ) )
    {
        return pprt;
    }
    else if ( pbase->state < ego_object_deinitializing )
    {
        pbase->state = ego_object_deinitializing;
    }

    iterations = 0;
    while ( NULL != pprt && pbase->state <= ego_object_deinitializing && iterations < max_iterations )
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
    int                 iterations;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // if the particle is already beyond this stage, deconstruct it
    if ( pbase->state > ( int )( ego_object_destructing + 1 ) )
    {
        return pprt;
    }
    else if ( pbase->state < ego_object_destructing )
    {
        // make sure that you deinitialize before destructing
        pbase->state = ego_object_deinitializing;
    }

    iterations = 0;
    while ( NULL != pprt && pbase->state <= ego_object_destructing && iterations < max_iterations )
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
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    // set the object to deinitialize if it is not "dangerous" and if was requested
    if ( pbase->kill_me )
    {
		if( !TERMINATED_PBASE(pbase) )
		{
			if( pbase->state < ego_object_deinitializing )
			{
				pbase->state = ego_object_deinitializing;
			}
		}

        pbase->kill_me = bfalse;
    }

    switch ( pbase->state )
    {
        default:
        case ego_object_invalid:
            pprt = NULL;
            break;

        case ego_object_constructing:
            pprt = prt_config_ctor( pprt );
            break;

        case ego_object_initializing:
            pprt = prt_config_init( pprt );
            break;

        case ego_object_active:
            pprt = prt_config_active( pprt );
            break;

        case ego_object_deinitializing:
            pprt = prt_config_deinit( pprt );
            break;

        case ego_object_destructing:
            pprt = prt_config_dtor( pprt );
            break;

        case ego_object_waiting:
        case ego_object_terminated:
            /* do nothing */
            break;
    }

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_ctor( prt_t * pprt )
{
    ego_object_base_t * pbase;

    // grab the base object
    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase ) return NULL;

    // if we aren't in the correct state, abort.
    if ( !STATE_CONSTRUCTING_PBASE( pbase ) ) return pprt;

    return prt_ctor( pprt );
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_init( prt_t * pprt )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_INITIALIZING_PBASE( pbase ) ) return pprt;

	POBJ_BEGIN_SPAWN( pprt );

    pprt = prt_config_do_init( pprt );
	if( NULL == pprt ) return NULL;

    if ( 0 == prt_loop_depth )
    {
        pprt->obj_base.on = btrue;
    }
    else
    {
		PrtList_add_activation( GET_INDEX_PPRT( pprt ) );
    }

    pbase->state = ego_object_active;

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_active( prt_t * pprt )
{
    // there's nothing to configure if the object is active...

    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase || !pbase->allocated ) return NULL;

    if ( !STATE_ACTIVE_PBASE( pbase ) ) return pprt;

	POBJ_END_SPAWN( pprt );

    pprt = prt_config_do_active( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_deinit( prt_t * pprt )
{
    /// @details BB@> deinitialize the character data

    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_DEINITIALIZING_PBASE( pbase ) ) return pprt;

	POBJ_END_SPAWN( pprt );

	pprt = prt_config_do_deinit( pprt );

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_config_dtor( prt_t * pprt )
{
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
    if ( NULL == pbase ) return NULL;

    if ( !STATE_DESTRUCTING_PBASE( pbase ) ) return pprt;

	POBJ_END_SPAWN( pprt );

    return prt_dtor( pprt );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PRT_REF spawn_one_particle( fvec3_t pos, FACING_T facing, const PRO_REF by_reference iprofile, int pip_index,
                            const CHR_REF by_reference chr_attach, Uint16 vrt_offset, const TEAM_REF by_reference team,
                            const CHR_REF by_reference chr_origin, const PRT_REF by_reference prt_origin, int multispawn, const CHR_REF by_reference oldtarget )
{
    /// @details ZZ@> This function spawns a new particle.
    ///               Returns the index of that particle or TOTAL_MAX_PRT on a failure.

    PIP_REF ipip;
    PRT_REF iprt;

    prt_t * pprt;
    pip_t * ppip;

    // Convert from local ipip to global ipip
    ipip = pro_get_ipip( iprofile, pip_index );

    if ( !LOADED_PIP( ipip ) )
    {
        log_debug( "spawn_one_particle() - cannot spawn particle with invalid pip == %d (owner == %d(\"%s\"), profile == %d(\"%s\"))\n",
                   REF_TO_INT( ipip ), REF_TO_INT( chr_origin ), INGAME_CHR( chr_origin ) ? ChrList.lst[chr_origin].Name : "INVALID",
                   REF_TO_INT( iprofile ), LOADED_PRO( iprofile ) ? ProList.lst[iprofile].name : "INVALID" );

        return ( PRT_REF )TOTAL_MAX_PRT;
    }
    ppip = PipStack.lst + ipip;

    // count all the requests for this particle type
    ppip->prt_request_count++;

    iprt = PrtList_allocate( ppip->force );
    if ( !DEFINED_PRT( iprt ) )
    {
#if defined(USE_DEBUG) && defined(DEBUG_PRT_LIST)
        log_debug( "spawn_one_particle() - cannot allocate a particle owner == %d(\"%s\"), pip == %d(\"%s\"), profile == %d(\"%s\")\n",
                   chr_origin, INGAME_CHR( chr_origin ) ? ChrList.lst[chr_origin].Name : "INVALID",
                   ipip, LOADED_PIP( ipip ) ? PipStack.lst[ipip].name : "INVALID",
                   iprofile, LOADED_PRO( iprofile ) ? ProList.lst[iprofile].name : "INVALID" );
#endif

        return ( PRT_REF )TOTAL_MAX_PRT;
    }
    pprt = PrtList.lst + iprt;

    pprt->spawn_data.pos        = pos;
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

    // count out all the requests for this particle type
    if ( NULL != pprt )
    {
        ppip->prt_create_count++;
    }

    return iprt;
}
//--------------------------------------------------------------------------------------------
float prt_get_mesh_pressure( prt_t * pprt, float test_pos[] )
{
    float retval = 0.0f;
	Uint32       stoppedby;
    pip_t      * ppip;

    if ( !DEFINED_PPRT( pprt ) ) return retval;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return retval;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( ppip->bumpmoney ) stoppedby |= MPDFX_WALL;

    // deal with the optional parameters
 	if ( NULL == test_pos ) test_pos = prt_get_pos_v(pprt);
	if ( NULL == test_pos ) return 0;

	mesh_mpdfx_tests = 0;
	mesh_bound_tests = 0;
	mesh_pressure_tests = 0;
	{
		retval = mesh_get_pressure( PMesh, test_pos, 0.0f, stoppedby );
	}
    prt_stoppedby_tests += mesh_mpdfx_tests;
	prt_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
fvec2_t prt_get_diff( prt_t * pprt, float test_pos[], float center_pressure )
{
    fvec2_t retval = ZERO_VECT2;
	float   radius;
	Uint32       stoppedby;
    pip_t      * ppip;

    if ( !DEFINED_PPRT( pprt ) ) return retval;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return retval;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( ppip->bumpmoney ) stoppedby |= MPDFX_WALL;

    // deal with the optional parameters
 	if ( NULL == test_pos ) test_pos = prt_get_pos_v(pprt);
	if ( NULL == test_pos ) return retval;

	// calculate the radius based on whether the particle is on camera
    radius = 0.0f;
    if ( mesh_grid_is_valid( PMesh, pprt->onwhichgrid ) )
    {
        if ( PMesh->tmem.tile_list[ pprt->onwhichgrid ].inrenderlist )
        {
            radius = pprt->bump_real.size;
        }
    }

	mesh_mpdfx_tests = 0;
	mesh_bound_tests = 0;
	mesh_pressure_tests = 0;
	{
		retval = mesh_get_diff( PMesh, test_pos, radius, center_pressure, stoppedby );
	}
    prt_stoppedby_tests += mesh_mpdfx_tests;
	prt_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
Uint32 prt_hit_wall( prt_t * pprt, float test_pos[], float nrm[], float * pressure )
{
    /// @details ZZ@> This function returns nonzero if the particle hit a wall that the
    ///    particle is not allowed to cross

    Uint32       retval;
	Uint32       stoppedby;
    pip_t      * ppip;

    if ( !DEFINED_PPRT( pprt ) ) return 0;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return 0;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( ppip->bumpmoney ) stoppedby |= MPDFX_WALL;

    // deal with the optional parameters
 	if ( NULL == test_pos ) test_pos = prt_get_pos_v(pprt);
	if ( NULL == test_pos ) return 0;

	mesh_mpdfx_tests = 0;
	mesh_bound_tests = 0;
	mesh_pressure_tests = 0;
	{
		retval = mesh_hit_wall( PMesh, test_pos, 0.0f, stoppedby, nrm, pressure );
	}
    prt_stoppedby_tests += mesh_mpdfx_tests;
	prt_pressure_tests += mesh_pressure_tests;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_test_wall( prt_t * pprt, float test_pos[] )
{
    /// @details ZZ@> This function returns nonzero if the particle hit a wall that the
    ///    particle is not allowed to cross

    bool_t retval;
    pip_t * ppip;
    Uint32  stoppedby;

    if ( !ACTIVE_PPRT( pprt ) ) return 0;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    stoppedby = MPDFX_IMPASS;
    if ( ppip->bumpmoney ) stoppedby |= MPDFX_WALL;

	// handle optional parameters
 	if ( NULL == test_pos ) test_pos = prt_get_pos_v(pprt);
	if ( NULL == test_pos ) return 0;

	// do the wall test
	mesh_mpdfx_tests = 0;
	mesh_bound_tests = 0;
	mesh_pressure_tests = 0;
	{
		retval = mesh_test_wall( PMesh, test_pos, 0.0f, stoppedby, NULL );
	}
    prt_stoppedby_tests += mesh_mpdfx_tests;
	prt_pressure_tests += mesh_pressure_tests;

    return retval;
}

////--------------------------------------------------------------------------------------------
//// This is BB's most recent version of the update_one_particle() function that should treat
//// all zombie/limbo particles properly and completely eliminates the improper modification of the
//// particle loop-control-variable inside the loop (thanks zefz!)
//bool_t update_one_particle( prt_t * pprt )
//{
//    /// @details BB@> update everything about a particle that does not depend on collisions
//    ///               or interactions with characters
//
//    int size_new;
//    bool_t prt_display, prt_active;
//
//    PRT_REF iprt;
//    pip_t * ppip;
//
//    // ASSUME that this function is only going to be called from update_all_particles(), where we already did this test
//    //if( !DEFINED_PPRT(pprt) ) return bfalse;
//    iprt = GET_REF_PPRT( pprt );
//
//    prt_active  = ACTIVE_PBASE( POBJ_GET_PBASE( pprt ) );
//    prt_display = prt_active || WAITING_PBASE( POBJ_GET_PBASE( pprt ) );
//
//    // update various iprt states
//    ppip = prt_get_ppip( iprt );
//    if ( NULL == ppip ) return bfalse;
//
//    // clear out the attachment if the character doesn't exist at all
//    if ( !DEFINED_CHR( pprt->attachedto_ref ) )
//    {
//        pprt->attachedto_ref = ( CHR_REF )MAX_CHR;
//    }
//
//    // figure out where the particle is on the mesh and update iprt states
//    if ( prt_display )
//    {
//        pprt->onwhichgrid  = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
//        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );
//
//        // determine whether the iprt is hidden
//        pprt->is_hidden = bfalse;
//        if ( INGAME_CHR( pprt->attachedto_ref ) )
//        {
//            pprt->is_hidden = ChrList.lst[pprt->attachedto_ref].is_hidden;
//        }
//
//        pprt->is_homing = ppip->homing && !INGAME_CHR( pprt->attachedto_ref );
//    }
//
//    // figure out where the particle is on the mesh and update iprt states
//    if ( prt_active && !pprt->is_hidden )
//    {
//        bool_t inwater;
//
//        // do the iprt interaction with water
//        inwater = ( pprt->pos.z < water.surface_level ) && ( 0 != mesh_test_fx( PMesh, pprt->onwhichgrid, MPDFX_WATER ) );
//
//        if ( inwater && water.is_water && ppip->endwater )
//        {
//            // Check for disaffirming character
//            if ( INGAME_CHR( pprt->attachedto_ref ) && pprt->owner_ref == pprt->attachedto_ref )
//            {
//                // Disaffirm the whole character
//                disaffirm_attached_particles( pprt->attachedto_ref );
//            }
//            else
//            {
//                // destroy the particle
//                prt_request_terminate( iprt );
//            }
//        }
//        else if ( inwater )
//        {
//            bool_t  spawn_valid     = bfalse;
//            int     spawn_pip_index = -1;
//            fvec3_t vtmp            = VECT3( pprt->pos.x, pprt->pos.y, water.surface_level );
//
//            if ( MAX_CHR == pprt->owner_ref &&
//                 ( PIP_SPLASH == pprt->pip_ref || PIP_RIPPLE == pprt->pip_ref ) )
//            {
//                /* do not spawn anything for a splash or a ripple */
//                spawn_valid = bfalse;
//            }
//            else
//            {
//                if ( !pprt->inwater )
//                {
//                    if ( SPRITE_SOLID == pprt->type )
//                    {
//                        spawn_pip_index = PIP_SPLASH;
//                    }
//                    else
//                    {
//                        spawn_pip_index = PIP_RIPPLE;
//                    }
//                    spawn_valid = btrue;
//                }
//                else
//                {
//                    if ( SPRITE_SOLID == pprt->type && !INGAME_CHR( pprt->attachedto_ref ) )
//                    {
//                        // only spawn ripples if you are touching the water surface!
//                        if ( pprt->pos.z + pprt->bump.height > water.surface_level && pprt->pos.z - pprt->bump.height < water.surface_level )
//                        {
//                            int ripand = ~(( ~RIPPLEAND ) << 1 );
//                            if ( 0 == (( update_wld + pprt->obj_base.guid ) & ripand ) )
//                            {
//
//                                spawn_valid = btrue;
//                                spawn_pip_index = PIP_RIPPLE;
//                            }
//                        }
//                    }
//                }
//            }
//
//            if ( spawn_valid )
//            {
//                // Splash for particles is just a ripple
//                spawn_one_particle( vtmp, 0, ( PRO_REF )MAX_PROFILE, spawn_pip_index, ( CHR_REF )MAX_CHR, GRIP_LAST,
//                                    ( TEAM_REF )TEAM_NULL, ( CHR_REF )MAX_CHR, ( PRT_REF )TOTAL_MAX_PRT, 0, ( CHR_REF )MAX_CHR );
//            }
//
//            pprt->inwater  = btrue;
//        }
//        else
//        {
//            pprt->inwater = bfalse;
//        }
//    }
//
//    // the following functions should not be done the first time through the update loop
//    if ( 0 == update_wld ) return btrue;
//
//    if ( prt_display )
//    {
//        // animate the particle
//        pprt->image = pprt->image + pprt->image_add;
//        if ( pprt->image >= pprt->image_max ) pprt->image = 0;
//
//        // rotate the particle
//        pprt->rotate += pprt->rotate_add;
//
//        // update the particle size
//        if ( 0 != pprt->size_add )
//        {
//            // resize the paricle
//            size_new = pprt->size + pprt->size_add;
//            size_new = CLIP( size_new, 0, 0xFFFF );
//
//            prt_set_size( pprt, size_new );
//        }
//
//        // Change dyna light values
//        if ( pprt->dynalight.level > 0 )
//        {
//            pprt->dynalight.level += ppip->dynalight.level_add;
//            if ( pprt->dynalight.level < 0 ) pprt->dynalight.level = 0;
//        }
//        else if ( pprt->dynalight.level < 0 )
//        {
//            // try to guess what should happen for negative lighting
//            pprt->dynalight.level += ppip->dynalight.level_add;
//            if ( pprt->dynalight.level > 0 ) pprt->dynalight.level = 0;
//        }
//        else
//        {
//            pprt->dynalight.level += ppip->dynalight.level_add;
//        }
//
//        pprt->dynalight.falloff += ppip->dynalight.falloff_add;
//
//        // spin the iprt
//        pprt->facing += ppip->facingadd;
//    }
//
//    if ( prt_active )
//    {
//        // down the remaining lifetime of the particle
//        if ( pprt->lifetime_remaining > 0 ) pprt->lifetime_remaining--;
//
//        // down the continuous spawn timer
//        if ( pprt->contspawn_delay > 0 ) pprt->contspawn_delay--;
//
//        // Spawn new particles if continually spawning
//        if ( 0 == pprt->contspawn_delay && ppip->contspawn_amount > 0 && -1 != ppip->contspawn_pip )
//        {
//            FACING_T facing;
//            Uint8    tnc;
//
//            // reset the spawn timer
//            pprt->contspawn_delay = ppip->contspawn_delay;
//
//            facing = pprt->facing;
//            for ( tnc = 0; tnc < ppip->contspawn_amount; tnc++ )
//            {
//                PRT_REF prt_child = spawn_one_particle( pprt->pos, facing, pprt->profile_ref, ppip->contspawn_pip,
//                                                        ( CHR_REF )MAX_CHR, GRIP_LAST, pprt->team, pprt->owner_ref, iprt, tnc, pprt->target_ref );
//
//                if ( ALLOCATED_PRT( prt_child ) )
//                {
//                    // Hack to fix velocity
//                    PrtList.lst[prt_child].vel.x += pprt->vel.x;
//                    PrtList.lst[prt_child].vel.y += pprt->vel.y;
//                }
//                facing += ppip->contspawn_facingadd;
//            }
//        }
//    }
//
//    // apply damage from  attatched bump particles (about once a second)
//    if ( 0 == ( update_wld & 31 ) )
//    {
//        CHR_REF ichr;
//
//        // do nothing if the particle is hidden
//        if ( prt_active && !pprt->is_hidden && pprt->attachedto_ref != pprt->owner_ref )
//        {
//            ichr = pprt->attachedto_ref;
//
//            // is this is not a damage particle for me?
//            if ( INGAME_CHR( ichr ) )
//            {
//                // Attached particle damage ( Burning )
//                if ( ppip->allowpush && 0 == ppip->vel_hrz_pair.base )
//                {
//                    // Make character limp
//                    ChrList.lst[ichr].vel.x *= 0.5f;
//                    ChrList.lst[ichr].vel.y *= 0.5f;
//                }
//
//                /// @note  Why is this commented out? Attached arrows need to do damage.
//                //damage_character( ichr, ATK_BEHIND, pprt->damage, pprt->damagetype, pprt->team, pprt->owner_ref, ppip->damfx, bfalse );
//            }
//        }
//    }
//
//    return btrue;
//}

//--------------------------------------------------------------------------------------------
void update_all_particles()
{
    /// @details BB@> main loop for updating particles. Do not use the
    ///               PRT_BEGIN_LOOP_* macro.
    ///               Converted all the update functions to the prt_run_config() paradigm.

    PRT_REF iprt;

    // activate any particles might have been generated last update in an in-active state
    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
		prt_update( PrtList.lst + iprt );
    }
}

//--------------------------------------------------------------------------------------------
void prt_set_level( prt_t * pprt, float level )
{
    float loc_height;

    if ( !DISPLAY_PPRT( pprt ) ) return;

    pprt->enviro.level = level;

    loc_height = prt_get_scale(pprt) * MAX( FP8_TO_FLOAT( pprt->size ), pprt->offset.z * 0.5 );

    pprt->enviro.adj_level = pprt->enviro.level;
    pprt->enviro.adj_floor = pprt->enviro.floor_level;

    pprt->enviro.adj_level += loc_height;
    pprt->enviro.adj_floor += loc_height;

    // set the zlerp after we have done everything to the particle's level we care to
    pprt->enviro.zlerp = ( pprt->pos.z - pprt->enviro.adj_level ) / PLATTOLERANCE;
    pprt->enviro.zlerp = CLIP( pprt->enviro.zlerp, 0.0f, 1.0f );
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
    if ( INGAME_CHR( pprt->onwhichplatform ) )
    {
        loc_level = MAX( pprt->enviro.floor_level, ChrList.lst[pprt->onwhichplatform].pos.z + ChrList.lst[pprt->onwhichplatform].chr_chr_cv.maxs[OCT_Z] );
    }
    prt_set_level( pprt, loc_level );

    //---- the "twist" of the floor
    pprt->enviro.twist = TWIST_FLAT;
    itile              = INVALID_TILE;
    if ( INGAME_CHR( pprt->onwhichplatform ) )
    {
        // this only works for 1 level of attachment
        itile = ChrList.lst[pprt->onwhichplatform].onwhichgrid;
    }
    else
    {
        itile = pprt->onwhichgrid;
    }

    if ( mesh_grid_is_valid( PMesh, itile ) )
    {
        pprt->enviro.twist = PMesh->gmem.grid_list[itile].twist;
    }

    // the "watery-ness" of whatever water might be here
    pprt->enviro.is_watery = water.is_water && pprt->enviro.inwater;
    pprt->enviro.is_slippy = !pprt->enviro.is_watery && ( 0 != mesh_test_fx( PMesh, pprt->onwhichgrid, MPDFX_SLIPPY ) );

    //---- traction
    pprt->enviro.traction = 1.0f;
    if ( pprt->is_homing )
    {
        // any traction factor here
        /* traction = ??; */
    }
    else if ( INGAME_CHR( pprt->onwhichplatform ) )
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
    else if ( mesh_grid_is_valid( PMesh, pprt->onwhichgrid ) )
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
        pprt->enviro.fluid_friction_vrt  = waterfriction;
        pprt->enviro.fluid_friction_hrz = waterfriction;
    }
    else
    {
        pprt->enviro.fluid_friction_hrz = pprt->enviro.air_friction;       // like real-life air friction
        pprt->enviro.fluid_friction_vrt  = pprt->enviro.air_friction;
    }

    //---- friction
    pprt->enviro.friction_hrz = 1.0f;
    if ( !pprt->is_homing )
    {
        // Make the characters slide
        float temp_friction_xy = noslipfriction;
        if ( mesh_grid_is_valid( PMesh, pprt->onwhichgrid ) && pprt->enviro.is_slippy )
        {
            // It's slippy all right...
            temp_friction_xy = slippyfriction;
        }

        pprt->enviro.friction_hrz = pprt->enviro.zlerp * 1.0f + ( 1.0f - pprt->enviro.zlerp ) * temp_friction_xy;
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
	return;
    // if the particle is homing in on something, ignore friction
    if ( pprt->is_homing ) return;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    // limit floor friction effects to solid objects
    if ( SPRITE_SOLID == pprt->type )
    {
        // figure out the acceleration due to the current "floor"
        floor_acc.x = floor_acc.y = floor_acc.z = 0.0f;
        temp_friction_xy = 1.0f;
        if ( INGAME_CHR( pprt->onwhichplatform ) )
        {
            chr_t * pplat = ChrList.lst + pprt->onwhichplatform;

            temp_friction_xy = platstick;

            floor_acc.x = pplat->vel.x - pplat->vel_old.x;
            floor_acc.y = pplat->vel.y - pplat->vel_old.y;
            floor_acc.z = pplat->vel.z - pplat->vel_old.z;

            chr_getMatUp( pplat, &vup );
        }
        else
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
        pprt->enviro.is_slipping = ( ABS( fric.x ) + ABS( fric.y ) + ABS( fric.z ) > pprt->enviro.friction_hrz );

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
    }

    // Apply fluid friction for all particles
    if( ppip->spdlimit < 0 )
    {
		// this is a buoyant particle, like smoke
		if( pprt->inwater )
		{
			float water_friction = POW( buoyancy_friction, 2.0f );

			pprt->vel.x += (waterspeed.x-pprt->vel.x) * ( 1.0f - water_friction  );
			pprt->vel.y += (waterspeed.y-pprt->vel.y) * ( 1.0f - water_friction  );
			pprt->vel.z += (waterspeed.z-pprt->vel.z) * ( 1.0f - water_friction  );
		}
		else
		{
			pprt->vel.x += (windspeed.x-pprt->vel.x) * ( 1.0f - buoyancy_friction  );
			pprt->vel.y += (windspeed.y-pprt->vel.y) * ( 1.0f - buoyancy_friction  );
			pprt->vel.z += (windspeed.z-pprt->vel.z) * ( 1.0f - buoyancy_friction  );
		}
    }
    else
    {
		// this is a normal particle
		if( pprt->inwater )
		{
			pprt->vel.x += (waterspeed.x-pprt->vel.x) * ( 1.0f - pprt->enviro.fluid_friction_hrz  );
			pprt->vel.y += (waterspeed.y-pprt->vel.y) * ( 1.0f - pprt->enviro.fluid_friction_hrz  );
			pprt->vel.z += (waterspeed.z-pprt->vel.z) * ( 1.0f - pprt->enviro.fluid_friction_vrt  );
		}
		else
		{
			pprt->vel.x += (windspeed.x-pprt->vel.x) * ( 1.0f - pprt->enviro.fluid_friction_hrz );
			pprt->vel.y += (windspeed.y-pprt->vel.y) * ( 1.0f - pprt->enviro.fluid_friction_hrz );
			pprt->vel.z += (windspeed.z-pprt->vel.z) * ( 1.0f - pprt->enviro.fluid_friction_vrt );
		}
	}

}

//--------------------------------------------------------------------------------------------
//// Do homing
//if( !INGAME_CHR( pprt->attachedto_ref ) )
//{
//    if ( ppip->homing && INGAME_CHR( pprt->target_ref ) )
//    {
//        if ( !ChrList.lst[pprt->target_ref].alive )
//        {
//            prt_request_terminate( iprt );
//        }
//        else
//        {
//            if ( !INGAME_CHR( pprt->attachedto_ref ) )
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

prt_t * move_one_particle_do_homing( prt_t * pprt )
{
    PRT_REF iprt;
    pip_t * ppip;
    chr_t * ptarget;

    if ( !DISPLAY_PPRT( pprt ) ) return pprt;
    iprt = GET_REF_PPRT( pprt );

    if ( !pprt->is_homing || !INGAME_CHR( pprt->target_ref ) ) return pprt;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return pprt;
    ppip = PipStack.lst + pprt->pip_ref;

    if ( !INGAME_CHR( pprt->target_ref ) )
    {
        goto move_one_particle_do_homing_fail;
    }
    ptarget = ChrList.lst + pprt->target_ref;

    if ( !ptarget->alive )
    {
        goto move_one_particle_do_homing_fail;
    }
    else if ( !INGAME_CHR( pprt->attachedto_ref ) )
    {
        int       ival;
        float     vlen, min_length, uncertainty;
        fvec3_t   vdiff, vdither;

        vdiff = fvec3_sub( ptarget->pos.v, prt_get_pos_v(pprt) );
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

    return pprt;

move_one_particle_do_homing_fail:

    prt_request_terminate( iprt );

	return NULL;
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
	
	if ( pprt->is_homing || INGAME_CHR( pprt->attachedto_ref ) ) return;

    loc_zlerp = CLIP( pprt->enviro.zlerp, 0.0f, 1.0f );

    // Do particle buoyancy. This is kinda BS the way it is calculated
    if( 0 != ppip->spdlimit )
    {
        pprt->vel.z += (-ppip->spdlimit - pprt->vel.z) * buoyancy_friction - gravity * loc_zlerp;
    }

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
//    if ( prt_hit_wall( iprt, NULL, NULL ) )
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
//    if ( prt_hit_wall( iprt, NULL, NULL ) )
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
//    play_particle_sound( iprt, ppip->soundend_wall );
//}

//if( hit_a_floor )
//{
//    // Play the sound for hitting the floor [FSND]
//    play_particle_sound( iprt, ppip->soundend_floor );
//}

//// do the reflections off the walls and floors
//if( !INGAME_CHR( pprt->attachedto_ref ) && (hit_a_wall || hit_a_floor) )
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

prt_t * move_one_particle_integrate_motion_attached( prt_t * pprt )
{
    /// @details BB@> A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    pip_t * ppip;

    float loc_level;
    PRT_REF iprt;
    bool_t hit_a_floor, hit_a_wall, needs_test, updated_2d;
    fvec3_t nrm_total;
	fvec3_t tmp_pos;

	// if the particle is not still in "display mode" there is no point in going on
    if ( !DISPLAY_PPRT( pprt ) ) return pprt;
    iprt = GET_INDEX_PPRT( pprt );

	// capture the particle position
	tmp_pos = prt_get_pos( pprt );

	// only deal with attached particles
	if( MAX_CHR == pprt->attachedto_ref ) return pprt;

	// if the particle profile is not defined, there is some error
    if ( !LOADED_PIP( pprt->pip_ref ) ) return pprt;
    ppip =  PipStack.lst +  pprt->pip_ref;

    hit_a_floor = bfalse;
    hit_a_wall  = bfalse;
    nrm_total.x = nrm_total.y = nrm_total.z = 0;

    loc_level = pprt->enviro.adj_level;

    // Move the particle
    if ( tmp_pos.z < loc_level )
    {
        hit_a_floor = btrue;
    }

    // handle the collision
    if ( hit_a_floor && ( ppip->endground || ppip->endbump ) )
    {
        prt_request_terminate( iprt );
        return NULL;
    }

    // interaction with the mesh walls
    hit_a_wall = bfalse;
    updated_2d = bfalse;
    needs_test = bfalse;
    if ( ABS( pprt->vel.x ) + ABS( pprt->vel.y ) > 0.0f )
    {
        if ( prt_test_wall( pprt, tmp_pos.v ) )
        {
            Uint32  hit_bits;
            fvec2_t nrm;
            float   pressure;

            // how is the character hitting the wall?
            hit_bits = prt_hit_wall( pprt, tmp_pos.v, nrm.v, &pressure );

            if ( 0 != hit_bits )
            {
                hit_a_wall = btrue;
            }
        }
    }

    // handle the collision
    if ( hit_a_wall && ( ppip->endwall || ppip->endbump ) )
    {
        prt_request_terminate( iprt );
        return NULL;
    }

    // handle the sounds
    if ( hit_a_wall )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( iprt, ppip->soundend_wall );
    }

    if ( hit_a_floor )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( iprt, ppip->soundend_floor );
    }
	
	prt_set_pos( pprt, tmp_pos.v );

    return pprt;
}

prt_t * move_one_particle_integrate_motion( prt_t * pprt )
{
    /// @details BB@> A helper function that figures out the next valid position of the particle.
    ///               Collisions with the mesh are included in this step.

    pip_t * ppip;

    float ftmp, loc_level;
    PRT_REF iprt;
    bool_t hit_a_floor, hit_a_wall, needs_test, updated_2d;
    fvec3_t nrm_total;
    fvec3_t tmp_pos;

	// if the particle is not still in "display mode" there is no point in going on
    if ( !DISPLAY_PPRT( pprt ) ) return pprt;
    iprt = GET_INDEX_PPRT( pprt );

	// capture the position
	tmp_pos = prt_get_pos( pprt );

	// no point in doing this if the particle thinks it's attached
	if( MAX_CHR != pprt->attachedto_ref )
	{
		return move_one_particle_integrate_motion_attached( pprt );
	}

	// if the particle profile is not defined, there is some error
    if ( !LOADED_PIP( pprt->pip_ref ) ) return pprt;
    ppip =  PipStack.lst +  pprt->pip_ref;

    hit_a_floor = bfalse;
    hit_a_wall  = bfalse;
    nrm_total.x = nrm_total.y = nrm_total.z = 0;

    loc_level = pprt->enviro.adj_level;

    // Move the particle
    ftmp = tmp_pos.z;
    tmp_pos.z += pprt->vel.z;
    LOG_NAN( tmp_pos.z );
    if ( tmp_pos.z < loc_level )
    {
        hit_a_floor = btrue;

        if ( pprt->vel.z < - STOPBOUNCINGPART )
        {
            // the particle will bounce
            nrm_total.z -= SGN( gravity );
            tmp_pos.z = ftmp;
        }
        else if ( pprt->vel.z > 0.0f )
        {
            // the particle is not bouncing, it is just at the wrong height
            tmp_pos.z = loc_level;
        }
        else
        {
            // the particle is in the "stop bouncing zone"
            tmp_pos.z = loc_level + 0.0001f;
            pprt->vel.z = 0.0f;
        }
    }

	// handle the sounds
    if ( hit_a_floor )
    {
        // Play the sound for hitting the floor [FSND]
        play_particle_sound( iprt, ppip->soundend_floor );
    }

    // handle the collision
    if ( hit_a_floor && ( ppip->endground || ppip->endbump ) )
    {
        prt_request_terminate( iprt );
        return NULL;
    }

    // interaction with the mesh walls
    hit_a_wall = bfalse;
    updated_2d = bfalse;
    needs_test = bfalse;
    if ( ABS( pprt->vel.x ) + ABS( pprt->vel.y ) > 0.0f )
    {
        float old_x, old_y, new_x, new_y;

        old_x = tmp_pos.x; LOG_NAN( old_x );
        old_y = tmp_pos.y; LOG_NAN( old_y );

        new_x = old_x + pprt->vel.x; LOG_NAN( new_x );
        new_y = old_y + pprt->vel.y; LOG_NAN( new_y );

        tmp_pos.x = new_x;
        tmp_pos.y = new_y;

        if ( !prt_test_wall( pprt, tmp_pos.v ) )
        {
            updated_2d = btrue;
        }
        else
        {
            Uint32  hit_bits;
            fvec2_t nrm;
            float   pressure;

            // how is the character hitting the wall?
            hit_bits = prt_hit_wall( pprt, tmp_pos.v, nrm.v, &pressure );

            if ( 0 != hit_bits )
            {
                hit_a_wall = btrue;

                tmp_pos.x = old_x;
                tmp_pos.y = old_y;

                nrm_total.x += nrm.x;
                nrm_total.y += nrm.y;
            }
        }
    }

	// handle the sounds
    if ( hit_a_wall )
    {
        // Play the sound for hitting the wall [WSND]
        play_particle_sound( iprt, ppip->soundend_wall );
    }
	
    // handle the collision
    if ( hit_a_wall && ( ppip->endwall || ppip->endbump ) )
    {
        prt_request_terminate( iprt );
        return NULL;
    }

    // do the reflections off the walls and floors
    if ( !INGAME_CHR( pprt->attachedto_ref ) && ( hit_a_wall || hit_a_floor ) )
    {
        if (( hit_a_wall && ( pprt->vel.x * nrm_total.x + pprt->vel.y * nrm_total.y ) < 0.0f ) ||
            ( hit_a_floor && ( pprt->vel.z * nrm_total.z ) < 0.0f ) )
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
            tmp_pos.z = loc_level + 0.0001f;
        }

        if ( hit_a_wall )
        {
            float fx, fy;

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

    if ( pprt->is_homing && tmp_pos.z < 0 )
    {
        tmp_pos.z = 0;  // Don't fall in pits...
    }

    if ( ppip->rotatetoface )
    {
        if ( ABS( pprt->vel.x ) + ABS( pprt->vel.y ) > 1e-6 )
        {
            // use velocity to find the angle
            pprt->facing = vec_to_facing( pprt->vel.x, pprt->vel.y );
        }
        else if ( INGAME_CHR( pprt->target_ref ) )
        {
            chr_t * ptarget =  ChrList.lst +  pprt->target_ref;

            // face your target
            pprt->facing = vec_to_facing( ptarget->pos.x - tmp_pos.x , ptarget->pos.y - tmp_pos.y );
        }
    }

	prt_set_pos( pprt, tmp_pos.v );

    return pprt;
}

//--------------------------------------------------------------------------------------------
bool_t move_one_particle( prt_t * pprt )
{
    /// @details BB@> The master function for controlling a particle's motion

    PRT_REF iprt;
    pip_t * ppip;
    prt_environment_t * penviro;

    if ( !DISPLAY_PPRT( pprt ) ) return bfalse;
    penviro = &( pprt->enviro );
    iprt = GET_REF_PPRT( pprt );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    // if the particle is hidden it is frozen in time. do nothing.
    if ( pprt->is_hidden ) return bfalse;

    // save the acceleration from the last time-step
    pprt->enviro.acc = fvec3_sub( pprt->vel.v, pprt->vel_old.v );

    // determine the actual velocity for attached particles
    if ( INGAME_CHR( pprt->attachedto_ref ) )
    {
        pprt->vel = fvec3_sub( prt_get_pos_v(pprt), pprt->pos_old.v );
    }

    // Particle's old location
    pprt->pos_old = prt_get_pos(pprt);
    pprt->vel_old = pprt->vel;

    // what is the local environment like?
    move_one_particle_get_environment( pprt );

    // do friction with the floor before voluntary motion
    move_one_particle_do_floor_friction( pprt );

    pprt = move_one_particle_do_homing( pprt );
	if( NULL == pprt ) return bfalse;

    move_one_particle_do_z_motion( pprt );

    pprt = move_one_particle_integrate_motion( pprt );
	if( NULL == pprt ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void move_all_particles( void )
{
    /// @details ZZ@> This is the particle physics function

    const float air_friction = 0.9868f;  // gives the same terminal velocity in terms of the size of the game characters
    const float ice_friction = 0.9738f;  // the square of air_friction

    prt_stoppedby_tests = 0;

    // move every particle
    PRT_BEGIN_LOOP_DISPLAY( cnt, pprt )
    {
        // prime the environment
        pprt->enviro.air_friction = air_friction;
        pprt->enviro.ice_friction = ice_friction;

        move_one_particle( pprt );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void particle_system_begin()
{
    /// @details ZZ@> This function sets up particle data

    // Reset the allocation table
    PrtList_init();

    init_all_pip();
}

//--------------------------------------------------------------------------------------------
void particle_system_end()
{
    release_all_pip();

    PrtList_dtor();
}

//--------------------------------------------------------------------------------------------
int spawn_bump_particles( const CHR_REF by_reference character, const PRT_REF by_reference particle )
{
    /// @details ZZ@> This function is for catching characters on fire and such

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
    cap_t * pcap;

    if ( !INGAME_PRT( particle ) ) return 0;
    pprt = PrtList.lst + particle;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return 0;
    ppip = PipStack.lst + pprt->pip_ref;

    // no point in going on, is there?
    if ( 0 == ppip->bumpspawn_amount && !ppip->spawnenchant ) return 0;
    amount = ppip->bumpspawn_amount;

    if ( !INGAME_CHR( character ) ) return 0;
    pchr = ChrList.lst + character;

    pmad = chr_get_pmad( character );
    if ( NULL == pmad ) return 0;

    pcap = pro_get_pcap( pchr->iprofile );
    if ( NULL == pcap ) return 0;

    bs_count = 0;

    // Only damage if hitting from proper direction
    direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
    direction = ATK_BEHIND + ( pchr->ori.facing_z - direction );

    // Check that direction
    if ( !is_invictus_direction( direction, character, ppip->damfx ) )
    {
        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_one_enchant( pprt->owner_ref, character, ( CHR_REF )MAX_CHR, ( ENC_REF )MAX_ENC, pprt->profile_ref );
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

            vertices = ( int )pchr->inst.vrt_count - ( int )grip_verts;
            vertices = MAX( 0, vertices );

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
                dist = fvec3_dist_abs( prt_get_pos_v(pprt), chr_get_pos_v(pchr) );

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
                    vertex_occupied[cnt] = TOTAL_MAX_PRT;
                }

                // determine if some of the vertex sites are already occupied
                PRT_BEGIN_LOOP_ACTIVE( iprt, pprt )
                {
                    prt_t * pprt;
                    if ( !INGAME_PRT( iprt ) ) continue;
                    pprt = PrtList.lst + iprt;

                    if ( character != pprt->attachedto_ref ) continue;

                    if ( pprt->attachedto_vrt_off >= 0 && pprt->attachedto_vrt_off < vertices )
                    {
                        vertex_occupied[pprt->attachedto_vrt_off] = iprt;
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

                    if ( ALLOCATED_PRT( bs_part ) )
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

                //        if( ALLOCATED_PRT(bs_part) )
                //        {
                //            PrtList.lst[bs_part].is_bumpspawn = btrue;
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
bool_t prt_is_over_water( const PRT_REF by_reference iprt )
{
    /// ZZ@> This function returns btrue if the particle is over a water tile
    Uint32 fan;

    if ( !ALLOCATED_PRT( iprt ) ) return bfalse;

    fan = mesh_get_tile( PMesh, PrtList.lst[iprt].pos.x, PrtList.lst[iprt].pos.y );
    if ( mesh_grid_is_valid( PMesh, fan ) )
    {
        if ( 0 != mesh_test_fx( PMesh, fan, MPDFX_WATER ) )  return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
PIP_REF PipStack_get_free()
{
    PIP_REF retval = ( PIP_REF )MAX_PIP;

    if ( PipStack.count < MAX_PIP )
    {
        retval = PipStack.count;
        PipStack.count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
PIP_REF load_one_particle_profile_vfs( const char *szLoadName, const PIP_REF by_reference pip_override )
{
    /// @details ZZ@> This function loads a particle template, returning bfalse if the file wasn't
    ///    found

    PIP_REF ipip;
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
        return ( PIP_REF )MAX_PIP;
    }
    ppip = PipStack.lst + ipip;

    if ( NULL == load_one_pip_file_vfs( szLoadName, ppip ) )
    {
        return ( PIP_REF )MAX_PIP;
    }

    ppip->soundend = CLIP( ppip->soundend, INVALID_SOUND, MAX_WAVE );
    ppip->soundspawn = CLIP( ppip->soundspawn, INVALID_SOUND, MAX_WAVE );

    return ipip;
}

//--------------------------------------------------------------------------------------------
void reset_particles( /* const char* modname */ )
{
    /// @details ZZ@> This resets all particle data and reads in the coin and water particles

    const char *loadpath;

    release_all_local_pips();
    release_all_pip();

    // Load in the standard global particles ( the coins for example )
    loadpath = "mp_data/1money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_COIN1 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/5money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_COIN5 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/25money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_COIN25 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/100money.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_COIN100 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // Load module specific information
    loadpath = "mp_data/weather4.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_WEATHER4 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/weather5.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_WEATHER5 ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/splash.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_SPLASH ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    loadpath = "mp_data/ripple.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_RIPPLE ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    // This is also global...
    loadpath = "mp_data/defend.txt";
    if ( MAX_PIP == load_one_particle_profile_vfs( loadpath, ( PIP_REF )PIP_DEFEND ) )
    {
        log_error( "Data file was not found! (\"%s\")\n", loadpath );
    }

    PipStack.count = GLOBAL_PIP_COUNT;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void init_all_pip()
{
    PIP_REF cnt;

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
    PIP_REF cnt;
    int tnc;
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
        FILE * ftmp = fopen( vfs_resolveWriteFilename( "/debug/pip_usage.txt" ), "w" );
        if ( NULL != ftmp )
        {
            fprintf( ftmp, "List of used pips\n\n" );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                if ( LOADED_PIP( cnt ) )
                {
                    pip_t * ppip = PipStack.lst + cnt;
                    fprintf( ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n", REF_TO_INT( cnt ), ppip->name, ppip->prt_create_count, ppip->prt_request_count );
                }
            }

            fflush( ftmp );

            fclose( ftmp );

            for ( cnt = 0; cnt < MAX_PIP; cnt++ )
            {
                release_one_pip( cnt );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t release_one_pip( const PIP_REF by_reference ipip )
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
bool_t prt_request_terminate( const PRT_REF by_reference iprt )
{
    /// @details BB@> Tell the game to get rid of this object and treat it
    ///               as if it was already dead

    /// @note prt_request_terminate() will call force the game to
    ///       (eventually) call free_one_particle_in_game() on this particle

    if ( !ALLOCATED_PRT( iprt ) || TERMINATED_PRT( iprt ) ) return bfalse;

    POBJ_REQUEST_TERMINATE( PrtList.lst + iprt );

    return btrue;
}

//--------------------------------------------------------------------------------------------
int prt_do_endspawn( const PRT_REF by_reference iprt )
{
	int endspawn_count = 0;
	prt_t * pprt;

	if( !ALLOCATED_PRT(iprt) ) return endspawn_count;

    pprt = PrtList.lst + iprt;

	// Spawn new particles if time for old one is up
	if ( pprt->endspawn_amount > 0 && LOADED_PIP( pprt->endspawn_pip ) )
	{
		FACING_T facing;
		int      tnc;

		facing = pprt->facing;
		for ( tnc = 0; tnc < pprt->endspawn_amount; tnc++ )
		{
			// we have been given the absolute pip reference when the particle was spawned
			// so, set the profile reference to (PRO_REF)MAX_PROFILE, so that the
			// value of pprt->endspawn_pip will be used directly
			PRT_REF spawned_prt = spawn_one_particle( pprt->pos_old, facing, ( PRO_REF )MAX_PROFILE, REF_TO_INT( pprt->endspawn_pip ),
								( CHR_REF )MAX_CHR, GRIP_LAST, pprt->team, prt_get_iowner( iprt, 0 ), iprt, tnc, pprt->target_ref );

			if( ALLOCATED_PRT(spawned_prt) )
			{
				endspawn_count++;
			}

			facing += pprt->endspawn_facingadd;
		}

		// we have already spawned these particles, so set this amount to
		// zero in case we are not actually calling free_one_particle_in_game()
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
        prt_t * pprt;

        bool_t prt_allocated, prt_waiting, prt_terminated;

        pprt = PrtList.lst + iprt;

        prt_allocated = FLAG_ALLOCATED_PBASE( POBJ_GET_PBASE( pprt ) );
        if ( !prt_allocated ) continue;

        prt_terminated = STATE_TERMINATED_PBASE( POBJ_GET_PBASE( pprt ) );
        if ( prt_terminated ) continue;

        prt_waiting    = STATE_WAITING_PBASE( POBJ_GET_PBASE( pprt ) );
		if( !prt_waiting ) continue;

		prt_do_endspawn( iprt );

        free_one_particle_in_game( iprt );
	}
}

//--------------------------------------------------------------------------------------------
void bump_all_particles_update_counters()
{
    PRT_REF cnt;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        ego_object_base_t * pbase;

        pbase = POBJ_GET_PBASE( PrtList.lst + cnt );
        if ( !ACTIVE_PBASE( pbase ) ) continue;

        pbase->update_count++;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
prt_t * prt_update_do_water( prt_t * pprt, pip_t * ppip )
{
    /// handle the particle interaction with water

    PRT_REF iprt;
	bool_t inwater;

	if( NULL == pprt || NULL == ppip ) return pprt;

    iprt = GET_REF_PPRT( pprt );

    inwater = ( pprt->pos.z < water.surface_level ) && ( 0 != mesh_test_fx( PMesh, pprt->onwhichgrid, MPDFX_WATER ) );

    if ( inwater && water.is_water && ppip->endwater )
    {
        // Check for disaffirming character
        if ( INGAME_CHR( pprt->attachedto_ref ) && pprt->owner_ref == pprt->attachedto_ref )
        {
            // Disaffirm the whole character
            disaffirm_attached_particles( pprt->attachedto_ref );
        }
        else
        {
            // destroy the particle
            prt_request_terminate( iprt );
			return NULL;
        }
    }
    else if ( inwater )
    {
        bool_t  spawn_valid     = bfalse;
        int     spawn_pip_index = -1;
        fvec3_t vtmp            = VECT3( pprt->pos.x, pprt->pos.y, water.surface_level );

        if ( MAX_CHR == pprt->owner_ref && ( PIP_SPLASH == pprt->pip_ref || PIP_RIPPLE == pprt->pip_ref ) )
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
                    spawn_pip_index = PIP_SPLASH;
                }
                else
                {
                    spawn_pip_index = PIP_RIPPLE;
                }
                spawn_valid = btrue;
            }
            else
            {
                if ( SPRITE_SOLID == pprt->type && !INGAME_CHR( pprt->attachedto_ref ) )
                {
                    // only spawn ripples if you are touching the water surface!
                    if ( pprt->pos.z + pprt->bump_real.height > water.surface_level && pprt->pos.z - pprt->bump_real.height < water.surface_level )
                    {
                        int ripand = ~(( ~RIPPLEAND ) << 1 );
                        if ( 0 == (( update_wld + pprt->obj_base.guid ) & ripand ) )
                        {

                            spawn_valid = btrue;
                            spawn_pip_index = PIP_RIPPLE;
                        }
                    }
                }
            }
        }

        if ( spawn_valid )
        {
            // Splash for particles is just a ripple
            spawn_one_particle( vtmp, 0, ( PRO_REF )MAX_PROFILE, spawn_pip_index, ( CHR_REF )MAX_CHR, GRIP_LAST,
                                ( TEAM_REF )TEAM_NULL, ( CHR_REF )MAX_CHR, ( PRT_REF )TOTAL_MAX_PRT, 0, ( CHR_REF )MAX_CHR );
        }

        pprt->inwater  = btrue;
    }
    else
    {
        pprt->inwater = bfalse;
    }

	return pprt;
}

//--------------------------------------------------------------------------------------------
int prt_do_contspawn( prt_t * pprt, pip_t * ppip )
{
    /// Spawn new particles if continually spawning

	int spawn_count = 0;
    PRT_REF iprt;

	if( NULL == pprt || NULL == ppip ) return spawn_count;
    iprt = GET_REF_PPRT( pprt );

	if( ppip->contspawn_amount <= 0 || -1 == ppip->contspawn_pip )
	{
		return spawn_count;
	}

    if ( 0 == pprt->contspawn_delay )
    {
        FACING_T facing;
        Uint8    tnc;

        // reset the spawn timer
        pprt->contspawn_delay = ppip->contspawn_delay;

        facing = pprt->facing;
        for ( tnc = 0; tnc < ppip->contspawn_amount; tnc++ )
        {
            PRT_REF prt_child = spawn_one_particle( prt_get_pos(pprt), facing, pprt->profile_ref, ppip->contspawn_pip,
                                                    ( CHR_REF )MAX_CHR, GRIP_LAST, pprt->team, pprt->owner_ref, iprt, tnc, pprt->target_ref );

            if ( ALLOCATED_PRT( prt_child ) )
            {
                // Hack to fix velocity
                PrtList.lst[prt_child].vel.x += pprt->vel.x;
                PrtList.lst[prt_child].vel.y += pprt->vel.y;

				spawn_count++;
            }

            facing += ppip->contspawn_facingadd;
        }
    }

	return spawn_count;
}

//--------------------------------------------------------------------------------------------
void prt_update_animation( prt_t * pprt, pip_t * ppip )
{
    /// animate the particle

	if( NULL == pprt || NULL == ppip ) return;

    pprt->image = pprt->image + pprt->image_add;
    if ( pprt->image >= pprt->image_max ) pprt->image = 0;

    // rotate the particle
    pprt->rotate += pprt->rotate_add;

    // update the particle size
    if ( 0 != pprt->size_add )
    {
		int size_new;

        // resize the paricle
        size_new = pprt->size + pprt->size_add;
        size_new = CLIP( size_new, 0, 0xFFFF );

        prt_set_size( pprt, size_new );
    }

    // spin the iprt
    pprt->facing += ppip->facingadd;
}

//--------------------------------------------------------------------------------------------
void prt_update_dynalight( prt_t * pprt, pip_t * ppip )
{
	if( NULL == pprt || NULL == ppip ) return;

	// Change dyna light values
    if ( pprt->dynalight.level > 0 )
    {
        pprt->dynalight.level += ppip->dynalight.level_add;
        if ( pprt->dynalight.level < 0 ) pprt->dynalight.level = 0;
    }
    else if ( pprt->dynalight.level < 0 )
    {
        // try to guess what should happen for negative lighting
        pprt->dynalight.level += ppip->dynalight.level_add;
        if ( pprt->dynalight.level > 0 ) pprt->dynalight.level = 0;
    }
    else
    {
        pprt->dynalight.level += ppip->dynalight.level_add;
    }

    pprt->dynalight.falloff += ppip->dynalight.falloff_add;
}

//--------------------------------------------------------------------------------------------
void prt_update_timers( prt_t * pprt )
{
	if( NULL == pprt ) return;

    // down the remaining lifetime of the particle
    if ( pprt->lifetime_remaining > 0 ) pprt->lifetime_remaining--;

    // down the continuous spawn timer
    if ( pprt->contspawn_delay > 0 ) pprt->contspawn_delay--;
}

//--------------------------------------------------------------------------------------------
void prt_do_bump_damage( prt_t * pprt, pip_t * ppip )
{
    // apply damage from  attatched bump particles (about once a second)

	CHR_REF ichr, iholder;
	Uint32  update_count;
	IPair  local_damage;

	if( NULL == pprt || NULL == ppip ) return;

	// wait until the right time
	update_count = update_wld + pprt->obj_base.guid;
	if ( 0 != (update_count & 31) ) return;

	// do nothing if the particle is hidden
	//if ( pprt->is_hidden ) return;		//ZF> This is already checked in prt_update_ingame()

	// we must be attached to something
	ichr = pprt->attachedto_ref;
	if( !INGAME_CHR(ichr) ) return;
	
	// find out who is holding the owner of this object
	iholder = chr_get_lowest_attachment( ichr, btrue );
	if ( MAX_CHR == iholder ) iholder = ichr;

	// do nothing if you are attached to your owner
	if( (MAX_CHR != pprt->owner_ref) && (iholder == pprt->owner_ref || ichr == pprt->owner_ref) ) return;

    // Attached particle damage ( Burning )
    if ( ppip->allowpush && 0 == ppip->vel_hrz_pair.base )
    {
        // Make character limp
        ChrList.lst[ichr].vel.x *= 0.5f;
        ChrList.lst[ichr].vel.y *= 0.5f;
    }

    /// @note  Why is this commented out? Attached arrows need to do damage.
	local_damage = pprt->damage;

	// distribute the damage over the particle's lifetime
	if( !pprt->is_eternal )
	{
		local_damage.base /= pprt->lifetime;
		local_damage.rand /= pprt->lifetime;
	}

    damage_character( ichr, ATK_BEHIND, local_damage, pprt->damagetype, pprt->team, pprt->owner_ref, ppip->damfx, bfalse );
}

//--------------------------------------------------------------------------------------------
prt_t * prt_update_ingame( prt_t * pprt, pip_t * ppip  )
{
    /// @details BB@> update everything about a particle that does not depend on collisions
    ///               or interactions with characters

    PRT_REF iprt;

    ego_object_base_t * pbase;

    if( NULL == pprt ) return pprt;

    pbase = POBJ_GET_PBASE( pprt );

    // if the object is not "on", it is no longer "in game" but still needs to be displayed
	if( !INGAME_PPRT( pprt ) )
	{
		return pprt;
	}

    // ASSUME that this function is only going to be called from prt_config_active(),
    // where we already determined that the particle was in its "active" state
    iprt = GET_REF_PPRT( pprt );

    // clear out the attachment if the character doesn't exist at all
    if ( !DEFINED_CHR( pprt->attachedto_ref ) )
    {
        pprt->attachedto_ref = ( CHR_REF )MAX_CHR;
    }

    // figure out where the particle is on the mesh and update the particle states
    {
        // determine whether the iprt is hidden
        pprt->is_hidden = bfalse;
        if ( INGAME_CHR( pprt->attachedto_ref ) )
        {
            pprt->is_hidden = ChrList.lst[pprt->attachedto_ref].is_hidden;
        }

        pprt->is_homing = ppip->homing && !INGAME_CHR( pprt->attachedto_ref );
    }

    // figure out where the particle is on the mesh and update iprt states
    if ( !pprt->is_hidden )
    {
		pprt = prt_update_do_water( pprt, ppip );
		if( NULL == pprt ) return pprt;
    }

    // the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pprt;

	prt_update_animation( pprt, ppip );

	prt_update_dynalight( pprt, ppip );

    if ( !pprt->is_hidden )
	{
		prt_update_timers( pprt );

		prt_do_contspawn( pprt, ppip );

		prt_do_bump_damage( pprt, ppip );
	}

	// If the particle is done updating, remove it from the game, but do not kill it
	if( !pprt->is_eternal && (pbase->update_count > 0 && 0 == pprt->lifetime_remaining) )
	{
		pbase->on = bfalse;
	}

    if ( !pprt->is_hidden )
	{
		pbase->update_count++;
	}

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_update_display( prt_t * pprt, pip_t * ppip )
{
    /// @details BB@> handle the case where the particle is still being diaplayed, but is no longer
	///               in the game

    bool_t prt_display;

    PRT_REF iprt;
    ego_object_base_t * pbase;

    pbase = POBJ_GET_PBASE( pprt );
	if( NULL == pbase ) return pprt;

    iprt = GET_REF_PPRT( pprt );

	// if it is not displaying, we are done here
	prt_display = (0 == pbase->frame_count) && (pprt->size > 0) && (pprt->inst.alpha > 0);
	if( !prt_display )
	{
		prt_request_terminate( iprt );
		return NULL;
	}

    // clear out the attachment if the character doesn't exist at all
    if ( !DEFINED_CHR( pprt->attachedto_ref ) )
    {
        pprt->attachedto_ref = ( CHR_REF )MAX_CHR;
    }

	// determine whether the iprt is hidden
    pprt->is_hidden = bfalse;
    if ( INGAME_CHR( pprt->attachedto_ref ) )
    {
        pprt->is_hidden = ChrList.lst[pprt->attachedto_ref].is_hidden;
    }

    pprt->is_homing = ppip->homing && !INGAME_CHR( pprt->attachedto_ref );

    // the following functions should not be done the first time through the update loop
    if ( 0 == update_wld ) return pprt;

	prt_update_animation( pprt, ppip );

	prt_update_dynalight( pprt, ppip );

    if ( !pprt->is_hidden )
	{
		pbase->update_count++;
	}

    return pprt;
}

//--------------------------------------------------------------------------------------------
prt_t * prt_update( prt_t * pprt )
{
	PRT_REF iprt;
	pip_t * ppip;

	// do the next step in the particle configuration
	pprt = prt_run_config( pprt );
	if( NULL == pprt ) return pprt;

	// is the particle is no longer allocated, return
	if( !ALLOCATED_PPRT(pprt) ) return pprt;

    iprt = GET_REF_PPRT( pprt );

    // update various iprt states
    ppip = prt_get_ppip( iprt );
	if( NULL == ppip )
		return pprt;

	// handle different particle states differently
	if( ON_PBASE(POBJ_GET_PBASE(pprt)) )
	{
		// the particle is on
		pprt = prt_update_ingame( pprt, ppip );
	}
	else
	{
		// the particle is not on
		pprt = prt_update_display( pprt, ppip );
	}

	return pprt;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t prt_update_safe_raw( prt_t * pprt )
{
	bool_t retval = bfalse;

	bool_t hit_a_wall;
	float  pressure;

	if( !ALLOCATED_PPRT( pprt ) ) return bfalse;

	hit_a_wall = prt_hit_wall( pprt, NULL, NULL, &pressure );
	if( hit_a_wall && 0.0f == pressure )
	{
		pprt->safe_valid = btrue;
		pprt->safe_pos   = prt_get_pos( pprt );
		pprt->safe_time  = update_wld;
		pprt->safe_grid  = mesh_get_tile(PMesh, pprt->pos.x, pprt->pos.y);

		retval = btrue;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_update_safe( prt_t * pprt, bool_t force )
{
	Uint32 new_grid;
	bool_t retval = bfalse;
	bool_t needs_update = bfalse;

	if( !ALLOCATED_PPRT(pprt) ) return bfalse;

	if( force || !pprt->safe_valid )
	{
		needs_update = btrue;
	}
	else
	{
		new_grid = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );

		if( INVALID_TILE == new_grid )
		{
			if( ABS(pprt->pos.x - pprt->safe_pos.x) > GRID_SIZE ||
				ABS(pprt->pos.y - pprt->safe_pos.y) > GRID_SIZE )
			{
				needs_update = btrue;
			}
		}
		else if ( new_grid != pprt->safe_grid )
		{
			needs_update = btrue;
		}
	}

	if( needs_update )
	{
		retval = prt_update_safe_raw( pprt );
    }

	return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
fvec3_t prt_get_pos( prt_t * pprt )
{
	fvec3_t vtmp = ZERO_VECT3;

	if( !ALLOCATED_PPRT(pprt) ) return vtmp;

	return pprt->pos;
}

//--------------------------------------------------------------------------------------------
float * prt_get_pos_v( prt_t * pprt )
{
	static fvec3_t vtmp = ZERO_VECT3;

	if( !ALLOCATED_PPRT(pprt) ) return vtmp.v;

	return pprt->pos.v;
}

//--------------------------------------------------------------------------------------------
bool_t prt_update_pos( prt_t * pprt )
{
	if( !ALLOCATED_PPRT(pprt) ) return bfalse;

    pprt->onwhichgrid  = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

	// update whether the current character position is safe
	prt_update_safe( pprt, bfalse );

	// update the breadcrumb list (does not exist for particles )
	// prt_update_breadcrumb( pprt, bfalse );

	return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_set_pos( prt_t * pprt, fvec3_base_t pos )
{
	bool_t retval = bfalse;

	if( !ALLOCATED_PPRT(pprt) ) return retval;

	retval = btrue;

	if( (pos[kX] != pprt->pos.v[kX]) || (pos[kY] != pprt->pos.v[kY]) || (pos[kZ] != pprt->pos.v[kZ]) )
	{
		memmove( pprt->pos.v, pos, sizeof(fvec3_base_t) );

		retval = prt_update_pos( pprt );
	}

	return retval;
}
