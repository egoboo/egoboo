#pragma once

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

#include "egoboo_object.h"

#include "pip_file.h"
#include "graphic_prt.h"
#include "physics.h"
#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Particles
#define MAXPARTICLEIMAGE                256         ///< Number of particle images ( frames )

/// Physics
#define STOPBOUNCINGPART                5.0f         ///< To make particles stop bouncing

DECLARE_STACK_EXTERN( pip_t, PipStack, MAX_PIP );

#define VALID_PIP_RANGE( IPIP ) ( ((IPIP) >= 0) && ((IPIP) < MAX_PIP) )
#define LOADED_PIP( IPIP )       ( VALID_PIP_RANGE( IPIP ) && PipStack.lst[IPIP].loaded )

//--------------------------------------------------------------------------------------------
/// Everything that is necessary to compute the character's interaction with the environment
struct s_prt_environment
{
    // floor stuff
    Uint8  twist;
    float  floor_level;           ///< Height of tile
    float  level;                 ///< Height of a tile or a platform
    float  zlerp;

    float adj_level;              ///< The level for the particle to sit on the floor or a platform
    float adj_floor;              ///< The level for the particle to sit on the floor or a platform

    // friction stuff
    bool_t is_slipping;
    bool_t is_slippy,    is_watery;
    float  air_friction, ice_friction;
    float  fluid_friction_hrz, fluid_friction_vrt;
    float  friction_hrz;
    float  traction;

    // misc states
    bool_t   inwater;
    fvec3_t  acc;
};
typedef struct s_prt_environment prt_environment_t;

//--------------------------------------------------------------------------------------------
struct s_prt_spawn_data
{
    fvec3_t  pos;
    FACING_T facing;
    PRO_REF  iprofile;
    PIP_REF  ipip;

    CHR_REF  chr_attach;
    Uint16   vrt_offset;
    TEAM_REF team;

    CHR_REF  chr_origin;
    PRT_REF  prt_origin;
    int      multispawn;
    CHR_REF  oldtarget;
};

typedef struct s_prt_spawn_data prt_spawn_data_t;

//--------------------------------------------------------------------------------------------
// Particle variables
//--------------------------------------------------------------------------------------------

/// The definition of the particle object
/// This "inherits" for ego_object_base_t
struct s_prt
{
    ego_object_base_t obj_base;              ///< the "inheritance" from ego_object_base_t

    prt_spawn_data_t  spawn_data;

    // profiles
    PIP_REF pip_ref;                         ///< The part template
    PRO_REF profile_ref;                     ///< the profile related to the spawned particle

    // links
    CHR_REF attachedto_ref;                  ///< For torch flame
    CHR_REF owner_ref;                       ///< The character that is attacking
    CHR_REF target_ref;                      ///< Who it's chasing
    PRT_REF parent_ref;                      ///< Did a another particle spawn this one?
    Uint32  parent_guid;                     ///< Just in case, the parent particle was despawned and a differnt particle now has the parent_ref

    Uint16   attachedto_vrt_off;              ///< It's vertex offset
    Uint8    type;                            ///< Transparency mode, 0-2
    FACING_T facing;                          ///< Direction of the part
    TEAM_REF team;                            ///< Team

    fvec3_t pos, pos_old, pos_stt;           ///< Position
    fvec3_t vel, vel_old, vel_stt;           ///< Velocity
    fvec3_t offset;                          ///< The initial offset when spawning the particle

    Uint32  onwhichgrid;                      ///< Where the part is
    Uint32  onwhichblock;                    ///< The particle's collision block
    CHR_REF onwhichplatform;                 ///< Is the particle on a platform?
    bool_t  is_hidden;                       ///< Is the particle related to a hidden character?

    FACING_T rotate;                          ///< Rotation direction
    Sint16   rotate_add;                      ///< Rotation rate

    int     size;                            ///< Size of particle (8.8-bit fixed point)
    UFP8_T  size_stt;                        ///< the starting size of the particle (8.8-bit fixed point)
    SFP8_T  size_add;                        ///< Change in size

    bool_t  inview;                          ///< Render this one?
    UFP8_T  image;                           ///< Which image (8.8-bit fixed point)
    Uint16  image_add;                       ///< Animation rate
    Uint16  image_max;                       ///< End of image loop
    Uint16  image_stt;                       ///< Start of image loop

    bool_t  is_eternal;                      ///< Does the particle ever time-out?
    size_t  lifetime;                        ///< Total particle lifetime in updates
    size_t  lifetime_remaining;              ///< How many updates does the particle have left?
    size_t  frames_remaining;                ///< How many frames does the particle have left?
    int     contspawn_delay;                 ///< Time until spawn

    bumper_t          bump;                            ///< Size of bumpers
    oct_bb_t          chr_prt_cv;                      ///< Collision volume for chr-prt interactions
    PRT_REF           bumplist_next;                   ///< Next particle on fanblock
    IPair             damage;                          ///< For strength
    Uint8             damagetype;                      ///< Damage type
    Uint16              lifedrain;
    Uint16              manadrain;

    bool_t            is_bumpspawn;                      ///< this particle is like a flame, burning something
    bool_t            inwater;

    int               spawncharacterstate;              ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    bool_t            safe_valid;
    fvec3_t           safe_pos;                         ///< Character's last safe position
    Uint32            safe_grid;

    bool_t            is_homing;                 ///< Is the particle in control of its motion?

    // some data that needs to be copied from the particle profile
    Uint8             endspawn_amount;        ///< The number of particles to be spawned at the end
    Uint16            endspawn_facingadd;     ///< The angular spacing for the end spawn
    int               endspawn_pip;           ///< The actual pip that will be spawned at the end

    dynalight_info_t  dynalight;              ///< Dynamic lighting...
    prt_instance_t    inst;                   ///< Everything needed for rendering
    prt_environment_t enviro;                 ///< the particle's environment
    phys_data_t       phys;                   ///< the particle's physics data

    BSP_leaf_t        bsp_leaf;

#if defined(__cplusplus)
    s_prt();
    ~s_prt();
#endif
};
typedef struct s_prt prt_t;

extern size_t maxparticles;                              ///< max number of particles

DECLARE_LIST_EXTERN( prt_t, PrtList, TOTAL_MAX_PRT );

#define VALID_PRT_RANGE( IPRT ) ( ((IPRT) >= 0) && ((IPRT) < maxparticles) && ((IPRT) < TOTAL_MAX_PRT) )
#define ALLOCATED_PRT( IPRT )   ( VALID_PRT_RANGE( IPRT ) && ALLOCATED_PBASE (POBJ_GET_PBASE(PrtList.lst + (IPRT))) )
#define ACTIVE_PRT( IPRT )      ( VALID_PRT_RANGE( IPRT ) && ACTIVE_PBASE    (POBJ_GET_PBASE(PrtList.lst + (IPRT))) )
#define WAITING_PRT( IPRT )     ( VALID_PRT_RANGE( IPRT ) && WAITING_PBASE   (POBJ_GET_PBASE(PrtList.lst + (IPRT))) )
#define TERMINATED_PRT( IPRT )  ( VALID_PRT_RANGE( IPRT ) && TERMINATED_PBASE(POBJ_GET_PBASE(PrtList.lst + (IPRT))) )

#define DEFINED_PRT( IPRT )     ( VALID_PRT_RANGE( IPRT ) && ALLOCATED_PBASE (POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define INGAME_PRT(IPRT)        ( VALID_PRT_RANGE( IPRT ) && ACTIVE_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) && ON_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define DISPLAY_PRT( IPRT )     ( VALID_PRT_RANGE( IPRT ) && (ACTIVE_PBASE(POBJ_GET_PBASE(PrtList.lst + (IPRT))) || WAITING_PBASE(POBJ_GET_PBASE(PrtList.lst + (IPRT)))) && ON_PBASE(POBJ_GET_PBASE(PrtList.lst + (IPRT))) )

#define GET_INDEX_PPRT( PPRT )  ((size_t)GET_INDEX_POBJ( PPRT, TOTAL_MAX_PRT ))
#define GET_REF_PPRT( PPRT )    ((PRT_REF)GET_INDEX_PPRT( PPRT ))
#define VALID_PRT_PTR( PPRT )   ( (NULL != (PPRT)) && VALID_PRT_RANGE( GET_REF_POBJ( PPRT, TOTAL_MAX_PRT) ) )
#define ALLOCATED_PPRT( PPRT )  ( VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define TERMINATED_PPRT( PPRT ) ( VALID_PRT_PTR( PPRT ) && TERMINATED_PBASE(POBJ_GET_PBASE(PPRT)) )
#define ACTIVE_PPRT( PPRT )     ( VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE(POBJ_GET_PBASE(PPRT)) )

#define DEFINED_PPRT( PPRT )     ( VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE (POBJ_GET_PBASE(PPRT)) && !TERMINATED_PBASE (POBJ_GET_PBASE(PPRT)) )
#define INGAME_PPRT ( PPRT )     ( VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE(POBJ_GET_PBASE(PPRT)) && ON_PBASE(POBJ_GET_PBASE(PPRT)) )
#define DISPLAY_PPRT( PPRT )     ( VALID_PRT_PTR( PPRT ) && (ACTIVE_PBASE(POBJ_GET_PBASE(PPRT)) || WAITING_PBASE(POBJ_GET_PBASE(PPRT))) )

#define PRT_BEGIN_LOOP_ACTIVE(IT, PPRT)  {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_t * PPRT = NULL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!ACTIVE_PRT (IT)) continue; PPRT =  PrtList.lst + IT;
#define PRT_BEGIN_LOOP_DISPLAY(IT, PPRT) {int IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; prt_t * PPRT = NULL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!DISPLAY_PRT(IT)) continue; PPRT =  PrtList.lst + IT;
#define PRT_END_LOOP() } prt_loop_depth--; EGOBOO_ASSERT(prt_loop_start_depth == prt_loop_depth); PrtList_cleanup(); }

extern int prt_wall_tests;

extern int prt_wall_tests;
extern int prt_loop_depth;

//--------------------------------------------------------------------------------------------
/// function prototypes

void PrtList_cleanup();

void   init_all_pip();
void   release_all_pip();
bool_t release_one_pip( const PIP_REF by_reference ipip );

bool_t PrtList_free_one( const PRT_REF by_reference particle );
void   PrtList_free_all();
void   PrtList_update_used();

void   free_one_particle_in_game( const PRT_REF by_reference particle );

void particle_system_begin();
void particle_system_end();

void update_all_particles( void );
void move_all_particles( void );
void cleanup_all_particles( void );
void bump_all_particles_update_counters( void );

void play_particle_sound( const PRT_REF by_reference particle, Sint8 sound );

PRT_REF spawn_one_particle( fvec3_t pos, FACING_T facing, const PRO_REF by_reference iprofile, int pip_index,
                            const CHR_REF by_reference chr_attach, Uint16 vrt_offset, const TEAM_REF by_reference team,
                            const CHR_REF by_reference chr_origin, const PRT_REF by_reference prt_origin, int multispawn, const CHR_REF by_reference oldtarget );

#define spawn_one_particle_global( pos, facing, ipip, multispawn ) spawn_one_particle( pos, facing, (PRO_REF)MAX_PROFILE, ipip, (CHR_REF)MAX_CHR, GRIP_LAST, (TEAM_REF)TEAM_NULL, (CHR_REF)MAX_CHR, (PRT_REF)TOTAL_MAX_PRT, multispawn, (CHR_REF)MAX_CHR );

int     prt_count_free();

PIP_REF load_one_particle_profile_vfs( const char *szLoadName, const PIP_REF by_reference pip_override );
void    reset_particles();

Uint32 prt_hit_wall( prt_t * pprt, float nrm[], float * pressure );
bool_t prt_test_wall( prt_t * pprt );

bool_t prt_is_over_water( const PRT_REF by_reference particle );

bool_t release_one_pip( const PIP_REF by_reference ipip );

bool_t prt_request_terminate( const PRT_REF by_reference iprt );

void particle_set_level( prt_t * pprt, float level );

size_t spawn_all_delayed_particles();

prt_t * prt_run_config( prt_t * pprt );