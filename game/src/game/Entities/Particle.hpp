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

/// @file  game/Entities/Particle.hpp
/// @brief Particle entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/graphic_prt.h"
#include "game/physics.h"
#include "egolib/_math.h"
#include "egolib/bbox.h"
#include "game/Entities/Common.hpp"
#include "egolib/Graphics/Animation2D.hpp"
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

    static void reset(prt_environment_t *self)
    {
        // floor stuff
        self->twist = 0;
        self->floor_level = 0.0f;
        self->level = 0.0f;
        self->zlerp = 0.0f;

        self->adj_level = 0.0f;
        self->adj_floor = 0.0f;

        // friction stuff
        self->is_slipping = false;
        self->is_slippy = self->is_watery = false;
        self->air_friction = self->ice_friction = 0.0f;
        self->fluid_friction_hrz = self->fluid_friction_vrt = 0.0f;
        self->friction_hrz = 0.0f;
        self->traction = 0.0f;

        // misc states
        self->inwater = false;
        self->acc = fvec3_t::zero();
    }
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

    static void reset(prt_spawn_data_t *self)
    {
        self->pos = fvec3_t::zero();
        self->facing = 0;
        self->iprofile = INVALID_PRO_REF;
        self->ipip = INVALID_PIP_REF;

        self->chr_attach = INVALID_CHR_REF;
        self->vrt_offset = 0;
        self->team = 0; /// @todo Should be INVALID_TEAM_REF.
        
        self->chr_origin = INVALID_CHR_REF;
        self->prt_origin = INVALID_PRT_REF;
        self->multispawn = 0;
        self->oldtarget  = INVALID_CHR_REF;
    }
};

/**
 * @brief
 *  The definition of the particle entity.
 * @extends
 *  Ego::Entity
 */
struct prt_t : public PhysicsData, _StateMachine<prt_t, PRT_REF, ParticleHandler>
{
    bool is_ghost;                   ///< the particle has been killed, but is hanging around a while...

    prt_spawn_data_t  spawn_data;

    // profiles
    PIP_REF pip_ref;                           ///< The particle profile
    /**
     * @brief
     *  The profile related to the spawned particle.
     */
    PRO_REF profile_ref;

    // links
    /**
     * @brief
     *  The object the particle is attached to.
     *  Example: A fire particle is attached to a torch.
     */
    CHR_REF attachedto_ref;
    /**
     * @brief
     *  The object owning this particle.
     *  Example: A fire particle is owned by a torch.
     */
    CHR_REF owner_ref;
    /**
     * @brief
     *  The object targeted by this particle.
     *  Example: Target-seeking arrows/bolts or similar particles.
     */
    CHR_REF target_ref;
    /**
     * @brief
     *  The original parent particle if any.
     *  The particle which has spawned this particle if any, INVALID_PRT_REF otherwise.
     */
    PRT_REF parent_ref;                        ///< Did a another particle spawn this one?
    /**
     * @brief
     *  The new parent particle (if the original parent was despawned).
     */
    Ego::GUID parent_guid; 


    Uint16   attachedto_vrt_off;               ///< It's vertex offset
    Uint8    type;                             ///< Transparency mode, 0-2
    FACING_T facing;                           ///< Direction of the part
    TEAM_REF team;                             ///< Team

    fvec3_t vel_stt;        ///< Starting/initial velocity.

    fvec3_t offset;                            ///< The initial offset when spawning the particle

    bool  is_hidden;                           ///< Is the particle related to a hidden character?

    FACING_T          rotate;                  ///< Rotation direction
    Sint16            rotate_add;              ///< Rotation rate

    UFP8_T            size_stt;                ///< The initial size of particle (8.8 fixed point)
    UFP8_T            size;                    ///< Size of particle (8.8 fixed point)
    SFP8_T            size_add;                ///< Change in size (8.8 fixed point)

    /// The state of a 2D animation used for rendering the particle.
    AnimationLoop _image;

    /** @name lifetime */
    /**@{*/
    
    /**
     * @brief
     *  Does the particle ever time-out?
     */
    bool is_eternal;

    /**
     * @brief
     *  The total lifetime in updates.
     * @todo
     *  Use a count-down timer.
     */
    size_t lifetime_total;
    /**
     * @brief
     *  The remaining lifetime in updates.
     * @todo
     *  Use a count-down timer.
     */
    size_t lifetime_remaining;

    /**
     * @brief
     *  The total number of frames.
     * @todo
     *  Use a count-down timer.
     */
    size_t frames_total;
    /**
     * @brief
     *  The remaining number of frames.
     * @todo
     *  Use a count-down timer.
     */
    size_t frames_remaining;

    /**
     * @brief
     *  The updates until spawn.
     */
    int contspawn_timer;

    /**@}*/

    // bumping
    Uint32            bump_size_stt;           ///< the starting size of the particle (8.8 fixed point)
    bumper_t          bump_real;               ///< Actual size of the particle
    bumper_t          bump_padded;             ///< The size of the particle with the additional bumpers added in
    oct_bb_t          prt_min_cv;              ///< Collision volume for chr-prt interactions
    oct_bb_t          prt_max_cv;              ///< Collision volume for chr-prt interactions

    // damage
    DamageType        damagetype;              ///< Damage type
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
    LocalParticleProfileRef  endspawn_lpip;    ///< The actual local pip that will be spawned at the end
    int               endspawn_characterstate; ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    dynalight_info_t  dynalight;               ///< Dynamic lighting...
    prt_instance_t    inst;                    ///< Everything needed for rendering
    prt_environment_t enviro;                  ///< the particle's environment

    prt_t();
    ~prt_t();

    /// @details Tell the game to get rid of this object and treat it as if it was already dead.
    /// @note This will force the game to (eventually) call end_one_particle_in_game() on this particle
    void requestTerminate();
    void set_level(const float level);
    /** @override */
    BIT_FIELD hit_wall(fvec2_t& nrm, float *pressure, mesh_wall_data_t *data) override;
    /** @override */
    BIT_FIELD hit_wall(const fvec3_t& pos, fvec2_t& nrm, float *pressure, mesh_wall_data_t *data) override;
    /** @override */
    BIT_FIELD test_wall(mesh_wall_data_t *data) override;
    /** @override */
    BIT_FIELD test_wall(const fvec3_t& pos, mesh_wall_data_t *data) override;
    bool set_size(int size);
    /// @brief Get the scale factor between the "graphical size" of the particle and the actual display size.
    float get_scale() const;

    bool setPosition(const fvec3_t& position);
protected:
    static bool update_safe(prt_t *self, bool force);
    static bool update_safe_raw(prt_t *self);
public:
    /**
     * @brief
     *  Get the profile index of this particle.
     * @return
     *  the profile index of this particle or INVALID_PIP_REF
     */
    PIP_REF get_ipip() const;
    /**
     * @brief
     *  Get a pointer to the profile of this particle.
     * @return
     *  a pointer to the profile of this particle or a null pointer
     */
    pip_t *get_ppip() const;

    static bool free(prt_t * pprt);

    // particle state machine function
    prt_t *config_do_ctor() override;
    // particle state machine function
    prt_t *config_do_init() override;
    // particle state machine function
    prt_t *config_do_active() override;
    // particle state machine function
    void config_do_deinit() override;
    // particle state machine function
    void config_do_dtor() override;
};

/**
 * @brief
 *  Convenient access to a prt ref and prt as well as pip ref and pip.
 */
struct prt_bundle_t
{
    PRT_REF _prt_ref;
    prt_t *_prt_ptr;

    PIP_REF _pip_ref;
    pip_t *_pip_ptr;

    void ctor();
    static prt_bundle_t *validate(prt_bundle_t *self);
    prt_bundle_t *set(prt_t *prt);
    static prt_bundle_t *do_bump_damage(prt_bundle_t *self);
    prt_bundle_t *update();
    /**
     * @brief
     *  Spawn new particles if continually spawning
     * @return
     *  the number of new particles spawned
     */
    int do_contspawn();
	/// @brief
	/// The master method to compute a particle's motion.
    bool move_one_particle();
private:
	/// @brief
	///	A helper method to compute the next valid position of this particle.
	/// Collisions with the mesh are included in this step.
    prt_bundle_t *move_one_particle_integrate_motion();
	/// @brief
	///	A helper method to compute the next valid position of this particle.
	/// Collisions with the mesh are included in this step.
    prt_bundle_t *move_one_particle_integrate_motion_attached();
	/// @brief
	/// A helper method to compute gravitational acceleration and buoyancy of this particle.
	prt_bundle_t *updateParticleSimpleGravity();
    prt_bundle_t *move_one_particle_do_z_motion();
    prt_bundle_t *move_one_particle_do_homing();
	/// @brief
	/// Helper method to compute the friction of this particle with the floor.
    prt_bundle_t *move_one_particle_do_floor_friction();
	/// @brief
	///	Helper method to compute the friction of this particle with the water.
    prt_bundle_t *move_one_particle_do_fluid_friction();
public:
	/// @brief
	/// Helper method to get all of the information about the particle's environment
	/// (like friction, etc.) that will be necessary for the other move_one_particle_*()
	/// functions to work
    prt_bundle_t *move_one_particle_get_environment();
private:
    /**
     * @brief
     *  Update the animation of this particle.
     * @return
     *  a pointer to this particle bundle if
     *  - the bundle holds a particle and
     *  - this particle was not ended by this function,
     *  a null pointer otherwise
     */
    prt_bundle_t *update_animation();
    /**
     * @brief
     *  Handle the particle ?.
     * @return
     *  a pointer to this particle bundle if
     *  - the bundle holds a particle and
     *  - this particle was not ended by this function,
     *  a null pointer otherwise
     * @remark
     *  This can not end the particle at least for now.
     * @todo
     *  Figure out what this crap is doing.
     */
    prt_bundle_t *update_dynalight();
    /**
     * @brief
     *  Update the lifetime timers of this particle.
     * @return
     *  a pointer to this particle bundle if the bundle holds a particle, a null pointer otherwise
     */
    prt_bundle_t *update_timers();
    /// @details update everything about a particle that does not depend on collisions
    ///               or interactions with characters
    prt_bundle_t *update_ingame();
    /**
     * @brief
     *  Handle the case where the particle is a ghost.
     * @return
     *  a pointer to this particle bundle if
     *  - the bundle holds a particle and
     *  - this particle was not ended by this function,
     *  a null pointer otherwise
     * @remark
     *  A particle is a "ghost" if it is still displayed, but is no longer in game.
     */
    prt_bundle_t *update_ghost();
    /**
     * @brief
     *  Handle the particle interaction with water
     * @return
     *  a pointer to this particle bundle if
     *  - the bundle holds a particle and
     *  - this particle was not ended by this function,
     *  a null pointer otherwise
     */
    prt_bundle_t *update_do_water();
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

/// @brief Mark particle as ghost.
void end_one_particle_now(const PRT_REF particle);
/// @brief End a particle and mark it as a ghost.
void end_one_particle_in_game(const PRT_REF particle);
bool prt_is_over_water(const PRT_REF particle);
void prt_play_sound(const PRT_REF particle, Sint8 sound);
CHR_REF prt_get_iowner(const PRT_REF iprt, int depth);

PIP_REF prt_get_ipip(const PRT_REF ref); /**< @deprecated */
pip_t *prt_get_ppip(const PRT_REF ref);  /**< @deprecated */
