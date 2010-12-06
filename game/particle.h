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

struct s_mesh_wall_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Particles

#define MAXPARTICLEIMAGE                256         ///< Number of particle images ( frames )

#define SPAWNNOCHARACTER                255         ///< For particles that spawn characters...

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
/// This "inherits" for obj_data_t
struct s_prt
{
    obj_data_t obj_base;              ///< the "inheritance" from obj_data_t
    bool_t     is_ghost;              ///< the particla has been killed, but is hanging around a while...

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
    bool_t  is_hidden;                       ///< Is the particle related to a hidden character?

    // platforms
    float   targetplatform_level;             ///< What is the height of the target platform?
    CHR_REF targetplatform_ref;               ///< Am I trying to attach to a platform?
    CHR_REF onwhichplatform_ref;              ///< Is the particle on a platform?
    Uint32  onwhichplatform_update;           ///< When was the last platform attachment made?

    FACING_T rotate;                          ///< Rotation direction
    Sint16   rotate_add;                      ///< Rotation rate

    UFP8_T  size_stt;                        ///< The initial size of particle (8.8-bit fixed point)
    UFP8_T  size;                            ///< Size of particle (8.8-bit fixed point)
    SFP8_T  size_add;                        ///< Change in size

    //bool_t  inview;                          ///< Render this one? (same as inst.indolist)
    UFP8_T  image;                           ///< Which image (8.8-bit fixed point)
    Uint16  image_add;                       ///< Animation rate
    Uint16  image_max;                       ///< End of image loop
    Uint16  image_stt;                       ///< Start of image loop

    bool_t  is_eternal;                      ///< Does the particle ever time-out?
    size_t  lifetime;                        ///< Total particle lifetime in updates
    size_t  lifetime_remaining;              ///< How many updates does the particle have left?
    size_t  frames_remaining;                ///< How many frames does the particle have left?
    int     contspawn_timer;                 ///< Time until spawn

    Uint32            bump_size_stt;         ///< the starting size of the particle (8.8-bit fixed point)
    bumper_t          bump_real;             ///< Actual size of the particle
    bumper_t          bump_padded;           ///< The size of the particle with the additional bumpers added in
    oct_bb_t          prt_min_cv;            ///< Collision volume for chr-prt interactions
    oct_bb_t          prt_max_cv;            ///< Collision volume for chr-prt interactions

    IPair             damage;                ///< For strength
    Uint8             damagetype;            ///< Damage type
    Uint16            lifedrain;
    Uint16            manadrain;

    bool_t            is_bumpspawn;          ///< this particle is like a flame, burning something
    bool_t            inwater;

    int               spawncharacterstate;   ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    bool_t            is_homing;                 ///< Is the particle in control of its motion?

    // some data that needs to be copied from the particle profile
    Uint8             endspawn_amount;        ///< The number of particles to be spawned at the end
    Uint16            endspawn_facingadd;     ///< The angular spacing for the end spawn
    int               endspawn_lpip;          ///< The actual local pip that will be spawned at the end

    dynalight_info_t  dynalight;              ///< Dynamic lighting...
    prt_instance_t    inst;                   ///< Everything needed for rendering
    prt_environment_t enviro;                 ///< the particle's environment
    phys_data_t       phys;                   ///< the particle's physics data

    bool_t         safe_valid;                ///< is the last "safe" position valid?
    fvec3_t        safe_pos;                  ///< the last "safe" position
    Uint32         safe_time;                 ///< the last "safe" time
    Uint32         safe_grid;                 ///< the last "safe" grid

    float          buoyancy;                  ///< an estimate of the particle bouyancy in air
    float          air_resistance;            ///< an estimate of the particle's extra resistance to air motion

    BSP_leaf_t        bsp_leaf;
};
typedef struct s_prt prt_t;

// counters for debugging wall collisions
extern int prt_stoppedby_tests;
extern int prt_pressure_tests;

//--------------------------------------------------------------------------------------------
struct s_prt_bundle
{
    PRT_REF   prt_ref;
    prt_t   * prt_ptr;

    PIP_REF   pip_ref;
    pip_t   * pip_ptr;
};
typedef struct s_prt_bundle prt_bundle_t;

//--------------------------------------------------------------------------------------------
// function prototypes

void particle_system_begin();
void particle_system_end();

prt_t * prt_ctor( prt_t * pprt );
prt_t * prt_dtor( prt_t * pprt );

void   init_all_pip();
void   release_all_pip();
bool_t release_one_pip( const PIP_REF ipip );

const PRT_REF end_one_particle_now( const PRT_REF particle );
const PRT_REF end_one_particle_in_game( const PRT_REF particle );

void update_all_particles( void );
void move_all_particles( void );
void cleanup_all_particles( void );
void bump_all_particles_update_counters( void );

void play_particle_sound( const PRT_REF particle, Sint8 sound );

PRT_REF spawn_one_particle( fvec3_t pos, FACING_T facing, const PRO_REF iprofile, int pip_index,
                            const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                            const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget );

#define spawn_one_particle_global( pos, facing, gpip_index, multispawn ) spawn_one_particle( pos, facing, (PRO_REF)MAX_PROFILE, gpip_index, (CHR_REF)MAX_CHR, GRIP_LAST, (TEAM_REF)TEAM_NULL, (CHR_REF)MAX_CHR, (PRT_REF)MAX_PRT, multispawn, (CHR_REF)MAX_CHR );

int     prt_count_free();

PIP_REF load_one_particle_profile_vfs( const char *szLoadName, const PIP_REF pip_override );
void    reset_particles();

BIT_FIELD prt_hit_wall( prt_t * pprt, const float test_pos[], float nrm[], float * pressure, struct s_mesh_wall_data * pdata );
BIT_FIELD prt_test_wall( prt_t * pprt, const float test_pos[], struct s_mesh_wall_data * pdata );

bool_t prt_is_over_water( const PRT_REF particle );

bool_t release_one_pip( const PIP_REF ipip );

bool_t prt_request_terminate( const PRT_REF iprt );

void prt_set_level( prt_t * pprt, float level );

prt_t * prt_run_config( prt_t * pprt );
prt_t * prt_config_construct( prt_t * pprt, int max_iterations );
prt_t * prt_config_initialize( prt_t * pprt, int max_iterations );
prt_t * prt_config_activate( prt_t * pprt, int max_iterations );
prt_t * prt_config_deinitialize( prt_t * pprt, int max_iterations );
prt_t * prt_config_deconstruct( prt_t * pprt, int max_iterations );

bool_t prt_set_pos( prt_t * pprt, fvec3_base_t pos );

prt_bundle_t * move_one_particle_get_environment( prt_bundle_t * pbdl_prt );
