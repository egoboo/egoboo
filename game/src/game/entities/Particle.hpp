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

/// @file  game/entities/Particle.hpp
/// @brief Particle entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/entities/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/graphic_prt.h"
#include "game/physics.h"
#include "egolib/_math.h"
#include "egolib/bbox.h"
#include "game/char.h"

// Forward declarations.
struct mesh_wall_data_t;
struct ParticleHandler;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Particles

#define MAXPARTICLEIMAGE                256         ///< Number of particle images ( frames )

#define SPAWNNOCHARACTER                255         ///< For particles that spawn characters...

//--------------------------------------------------------------------------------------------

/// Everything that is necessary to compute the character's interaction with the environment
struct prt_environment_t
{
    // floor stuff
    Uint8  twist;
    float  floor_level;           ///< Height of tile
    float  level;                 ///< Height of a tile or a platform
    float  zlerp;

    float adj_level;              ///< The level for the particle to sit on the floor or a platform
    float adj_floor;              ///< The level for the particle to sit on the floor or a platform

    // friction stuff
    bool is_slipping;
    bool is_slippy, is_watery;
    float  air_friction, ice_friction;
    float  fluid_friction_hrz, fluid_friction_vrt;
    float  friction_hrz;
    float  traction;

    // misc states
    bool   inwater;
    fvec3_t  acc;
};

//--------------------------------------------------------------------------------------------
struct prt_spawn_data_t
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



/**
 * @brief
 *  The definition of the particle entity.
 * @extends
 *  Ego::Entity
 */
struct prt_t : public _StateMachine<prt_t,ParticleHandler>
{
    bool is_ghost;                   ///< the particla has been killed, but is hanging around a while...

    prt_spawn_data_t  spawn_data;

    // profiles
    PIP_REF pip_ref;                           ///< The particle profile
    PRO_REF profile_ref;                       ///< the profile related to the spawned particle

    // links
    CHR_REF attachedto_ref;                    ///< For torch flame
    CHR_REF owner_ref;                         ///< The character that is attacking
    CHR_REF target_ref;                        ///< Who it's chasing
    PRT_REF parent_ref;                        ///< Did a another particle spawn this one?
    Uint32  parent_guid;                       ///< Just in case, the parent particle was despawned and a differnt particle now has the parent_ref

    Uint16   attachedto_vrt_off;               ///< It's vertex offset
    Uint8    type;                             ///< Transparency mode, 0-2
    FACING_T facing;                           ///< Direction of the part
    TEAM_REF team;                             ///< Team

    fvec3_t pos, pos_old, pos_stt;             ///< Position
    fvec3_t vel, vel_old, vel_stt;             ///< Velocity
    fvec3_t offset;                            ///< The initial offset when spawning the particle

    Uint32  onwhichgrid;                       ///< Where the part is
    Uint32  onwhichblock;                      ///< The particle's collision block
    bool  is_hidden;                           ///< Is the particle related to a hidden character?

    // platforms
    float   targetplatform_level;              ///< What is the height of the target platform?
    CHR_REF targetplatform_ref;                ///< Am I trying to attach to a platform?
    CHR_REF onwhichplatform_ref;               ///< Is the particle on a platform?
    Uint32  onwhichplatform_update;            ///< When was the last platform attachment made?

    FACING_T          rotate;                  ///< Rotation direction
    Sint16            rotate_add;              ///< Rotation rate

    UFP8_T            size_stt;                ///< The initial size of particle (8.8 fixed point)
    UFP8_T            size;                    ///< Size of particle (8.8 fixed point)
    SFP8_T            size_add;                ///< Change in size (8.8 fixed point)

    // which image
    UFP8_T            image_stt;               ///< Start of image loop (8.8 fixed point)
    UFP8_T            image_off;               ///< Which image (8.8 fixed point)
    UFP8_T            image_add;               ///< Image offset animation rate (8.8 fixed point)
    UFP8_T            image_max;               ///< Maximum image offset (8.8 fixed point)

    // lifetime stuff
    bool is_eternal;                    ///< Does the particle ever time-out?
    size_t lifetime_total;              ///< Total particle lifetime in updates
    size_t lifetime_remaining;          ///< How many updates does the particle have left?
    size_t frames_total;                ///< Total number of particle frames
    size_t frames_remaining;            ///< How many frames does the particle have left?
    int               contspawn_timer;  ///< Time until spawn

    // bunping
    Uint32            bump_size_stt;           ///< the starting size of the particle (8.8 fixed point)
    bumper_t          bump_real;               ///< Actual size of the particle
    bumper_t          bump_padded;             ///< The size of the particle with the additional bumpers added in
    oct_bb_t          prt_min_cv;              ///< Collision volume for chr-prt interactions
    oct_bb_t          prt_max_cv;              ///< Collision volume for chr-prt interactions

    // damage
    Uint8             damagetype;              ///< Damage type
    IPair             damage;                  ///< For strength
    UFP8_T            lifedrain;               ///< (8.8 fixed point)
    UFP8_T            manadrain;               ///< (8.8 fixed point)

    // bump effects
    bool              is_bumpspawn;            ///< this particle is like a flame, burning something

    // motion effects
    float             buoyancy;                ///< an estimate of the particle bouyancy in air
    float             air_resistance;          ///< an estimate of the particle's extra resistance to air motion
    bool              is_homing;               ///< Is the particle in control of its motion?
    bool              no_gravity;              ///< does the particle ignore gravity?

    // some data that needs to be copied from the particle profile
    Uint8             endspawn_amount;         ///< The number of particles to be spawned at the end
    Uint16            endspawn_facingadd;      ///< The angular spacing for the end spawn
    int               endspawn_lpip;           ///< The actual local pip that will be spawned at the end
    int               endspawn_characterstate; ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    dynalight_info_t  dynalight;               ///< Dynamic lighting...
    prt_instance_t    inst;                    ///< Everything needed for rendering
    prt_environment_t enviro;                  ///< the particle's environment
    phys_data_t       phys;                    ///< the particle's physics data

    bool              safe_valid;              ///< is the last "safe" position valid?
    fvec3_t           safe_pos;                ///< the last "safe" position
    Uint32            safe_time;               ///< the last "safe" time
    Uint32            safe_grid;               ///< the last "safe" grid

    prt_t();
    ~prt_t();

    /// @brief Set all particle parameters to safe values.
    /// @details The C equivalent of a parameterless constructor.
    prt_t *ctor();
    prt_t *dtor();
    static bool request_terminate(prt_t *self);
    static void set_level(prt_t *self, const float level);
    static BIT_FIELD hit_wall(prt_t *self, float nrm[], float *pressure, mesh_wall_data_t *data);
    static BIT_FIELD hit_wall(prt_t *self, const fvec3_t& pos, float nrm[], float *pressure, mesh_wall_data_t *data);
    static BIT_FIELD test_wall(prt_t *self, mesh_wall_data_t *data);
    static BIT_FIELD test_wall(prt_t *self, const fvec3_t& pos, mesh_wall_data_t *data);
    static bool set_size(prt_t *self, int size);
    /// @brief Get the scale factor between the "graphical size" of the particle and the actual display size.
    static float get_scale(prt_t *self);
    static const fvec3_t& get_pos_v_const(const prt_t *self);

    static bool set_pos(prt_t *self, const fvec3_t& position);
    static bool get_pos(const prt_t *self, fvec3_t& position);
    static bool update_pos(prt_t *self);
    static bool update_safe(prt_t *self, bool force);
    static bool update_safe_raw(prt_t *self);
    static PIP_REF get_ipip(const prt_t *self);
    static pip_t *get_ppip(const prt_t *self);

    static bool free(prt_t * pprt);

    // particle state machine function
    prt_t *config_do_init();
    // particle state machine function
    prt_t *config_do_active();
    // particle state machine function
    prt_t *config_do_deinit();
};

/**
 * @brief
 *  Convenient access to a prt ref and prt as well as pip ref and pip.
 */
struct prt_bundle_t
{
    PRT_REF   prt_ref;
    prt_t   * prt_ptr;

    PIP_REF   pip_ref;
    pip_t   * pip_ptr;

    static prt_bundle_t *ctor(prt_bundle_t *self);
    static prt_bundle_t *validate(prt_bundle_t *self);
    static prt_bundle_t *set(prt_bundle_t *self, prt_t *prt);
    static prt_bundle_t *do_bump_damage(prt_bundle_t *self);
    static prt_bundle_t *update(prt_bundle_t *self);
    static int do_contspawn(prt_bundle_t *self);
    static bool move_one_particle(prt_bundle_t *self);
private:
    static prt_bundle_t *updateParticleSimpleGravity(prt_bundle_t * pbdl_prt);
    static prt_bundle_t *move_one_particle_integrate_motion(prt_bundle_t *self);
    static prt_bundle_t *move_one_particle_integrate_motion_attached(prt_bundle_t *self);
    static prt_bundle_t *move_one_particle_do_z_motion(prt_bundle_t *self);
    static prt_bundle_t *move_one_particle_do_homing(prt_bundle_t *self);
    static prt_bundle_t *move_one_particle_do_floor_friction(prt_bundle_t *self);
    static prt_bundle_t *move_one_particle_do_fluid_friction(prt_bundle_t *self);
public:
    static prt_bundle_t *move_one_particle_get_environment(prt_bundle_t *self);
private:
    static prt_bundle_t *update_animation(prt_bundle_t *self);
    static prt_bundle_t *update_dynalight(prt_bundle_t *self);
    static prt_bundle_t *update_timers(prt_bundle_t *self);
    static prt_bundle_t *update_ingame(prt_bundle_t *self);
    static prt_bundle_t *update_ghost(prt_bundle_t *self);
    static prt_bundle_t *update_do_water(prt_bundle_t *self);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// counters for debugging wall collisions
extern int prt_stoppedby_tests;
extern int prt_pressure_tests;

//--------------------------------------------------------------------------------------------
// function prototypes
//--------------------------------------------------------------------------------------------

void update_all_particles();
void move_all_particles();
void cleanup_all_particles();
void bump_all_particles_update_counters();

/**
 * @brief
 *	Spawn a particle.
 * @return
 *	the index of the particle on success, INVALID_PRT_REF on failure
 */
PRT_REF spawn_one_particle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, int pip_index,
                           const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                           const CHR_REF chr_origin, const PRT_REF prt_origin, int multispawn, const CHR_REF oldtarget);

/**
* @brief Spawns a particle
*        This function is slightly different than spawn_one_particle because it takes a PIP_REF rather than a pip_index
* @return the PRT_REF of the spawned particle or INVALID_PRT_REF on failure
**/
PRT_REF spawnOneParticle(const fvec3_t& pos, FACING_T facing, const PRO_REF iprofile, const PIP_REF ipip,
                         const CHR_REF chr_attach, Uint16 vrt_offset, const TEAM_REF team,
                         const CHR_REF chr_origin, const PRT_REF prt_origin = INVALID_PRT_REF, const int multispawn = 0, 
                         const CHR_REF oldtarget = INVALID_CHR_REF);

#define spawn_one_particle_global( pos, facing, gpip_index, multispawn ) spawn_one_particle( pos, facing, INVALID_PRO_REF, gpip_index, INVALID_CHR_REF, GRIP_LAST, (TEAM_REF)TEAM_NULL, INVALID_CHR_REF, INVALID_PRT_REF, multispawn, INVALID_CHR_REF );

/// @brief Mark particle as ghost.
PRT_REF end_one_particle_now(const PRT_REF particle);
/// @brief End a particle and mark it as a ghost.
PRT_REF end_one_particle_in_game(const PRT_REF particle);
bool prt_is_over_water(const PRT_REF particle);
void prt_play_sound(const PRT_REF particle, Sint8 sound);
CHR_REF prt_get_iowner(const PRT_REF iprt, int depth);

PIP_REF prt_get_ipip(const PRT_REF ref); /**< @deprecated */
pip_t *prt_get_ppip(const PRT_REF ref);  /**< @deprecated */
