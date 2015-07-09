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
/// @brief Particle entities
/// @author Johan Jansen aka Zefz

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif

#include "game/egoboo_typedef.h"
#include "game/graphic_prt.h"
#include "game/Entities/Common.hpp"
#include "egolib/Graphics/Animation2D.hpp"

namespace Ego
{


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

    void reset()
    {
        // floor stuff
        twist = 0;
        floor_level = 0.0f;
        level = 0.0f;
        zlerp = 0.0f;

        adj_level = 0.0f;
        adj_floor = 0.0f;

        // friction stuff
        is_slipping = false;
        is_slippy = is_watery = false;
        air_friction = ice_friction = 0.0f;
        fluid_friction_hrz = fluid_friction_vrt = 0.0f;
        friction_hrz = 0.0f;
        traction = 0.0f;

        // misc states
        inwater = false;
        acc = fvec3_t::zero();
    }
};

/**
 * @brief
 *  The definition of the particle entity.
 */
class Particle : public PhysicsData, public Id::NonCopyable
{
public:
    Particle(PRT_REF ref);
    
    /**
     * @brief
     *  Get the profile index of this particle.
     * @return
     *  the profile index of this particle or INVALID_PIP_REF
     */
    PIP_REF getProfileID() const;

    PRT_REF getParticleID() const;

    /**
     * @brief
     *  Get a pointer to the profile of this particle.
     * @return
     *  a pointer to the profile of this particle or a null pointer
     */
    const std::shared_ptr<pip_t>& getProfile() const;

    /**
    * @brief
    *   TODO: SHOULD BE PRIVATE (use requestTerminate instead!)
    **/
    void reset(PRT_REF ref);

    /// @details Tell the game to get rid of this object and treat it as if it was already dead.
    /// @note This will force the game to (eventually) call end_one_particle_in_game() on this particle
    void requestTerminate();

    /**
    * @brief
    *   Set the altitude of this Particle
    **/
    void setElevation(const float level);

    BIT_FIELD hit_wall(fvec2_t& nrm, float *pressure, mesh_wall_data_t *data) override;

    BIT_FIELD hit_wall(const fvec3_t& pos, fvec2_t& nrm, float *pressure, mesh_wall_data_t *data) override;

    BIT_FIELD test_wall(mesh_wall_data_t *data) override;

    BIT_FIELD test_wall(const fvec3_t& pos, mesh_wall_data_t *data) override;

    /**
    * @brief
    *   Change the FP8 size of this Particle. Also updates collision bounds
    **/
    void setSize(int size);

    /// @brief Get the scale factor between the "graphical size" of the particle and the actual display size.
    float getScale() const;

    /**
    * @brief
    *   Set the position of this Particle
    **/
    bool setPosition(const fvec3_t& position);

    /**
    * @brief
    *   Apply one logic frame update to this particle
    * @Note
    *   Should not be done the first time through the update loop (0 == update_wld)
    **/
    void update();

    /**
    * @brief
    *   true if it is attached to a hidden Object
    **/
    bool isHidden() const;

    /**
    * @brief
    *   true if this Particle is attached to a valid Object
    **/
    bool isAttached() const;
    
    /**
    * @brief
    *   true if this Particle has a valid Object as an target
    **/
    bool hasValidTarget() const;

    /**
    * @return
    *   get the target of this Particle (or nullptr if no valid target)
    **/
    const std::shared_ptr<Object>& getTarget() const;

    /**
    * @return
    *   get the Object that this Particle is currently attached to (or nullptr if not attached)
    **/
    const std::shared_ptr<Object>& getAttachedObject() const;

    /**
    * @return
    *   true if this Particle has been terminated and will be removed from the game soon
    **/
    bool isTerminated() const;

    bool initialize(const fvec3_t& spawnPos, const FACING_T spawnFacing, const PRO_REF spawnProfile,
                const PIP_REF particleProfile, const CHR_REF spawnAttach, Uint16 vrt_offset, const TEAM_REF spawnTeam,
                const CHR_REF spawnOrigin, const PRT_REF spawnParticleOrigin = INVALID_PRT_REF, const int multispawn = 0, 
                const CHR_REF spawnTarget = INVALID_CHR_REF);


    /**
    * @brief
    *   Final stuff done before the particle is removed from game
    * @note
    *   Should only ever be used by the ParticleHandler! *Do not use*
    **/
    void destroy();

    /// @author ZZ
    /// @details This function sets one particle's position to be attached to a character.
    ///    It will kill the particle if the character is no longer around
    bool placeAtVertex(const std::shared_ptr<Object> &object, int vertex_offset);

    /**
    * @return
    *   true if this Particles position is on a water tile
    **/
    bool isOverWater() const;

    /**
    * @brief
    *   Attaches this Particle to an Object
    * @return
    *   true if successfull
    **/
    bool attach(const CHR_REF attach);

    /**
    * @brief
    *   Sets the target of this Particle
    **/
    void setTarget(const CHR_REF target);

    BSP_leaf_t& getBSPLeaf() { return _bspLeaf; }

    PRO_REF getSpawnerProfile() const { return _spawnerProfile; }

    /**
    * @return
    *   true if this Particle is currently in control of its own motion and
    *   moving in towards a valid Object target
    **/
    bool isHoming() const { return _isHoming; }

    bool isGhost() const;

    /**
    * @brief
    *   Particle cannot be interacted with but is still rendered normally
    **/
    bool isActive() const { return !isGhost() && !isTerminated(); }

    /**
    * @brief
    *   Makes this Particle play a sound effect from its spawner sounds
    *   (or a sound from the basicdat folder if it is a global particle)
    **/
    void playSound(int8_t soundID);

private:
    bool updateSafe(bool force);
    bool updateSafeRaw();

    void updateWater();

    void updateAnimation();

    void updateDynamicLighting();

    size_t updateContinuousSpawning();

    /**
    * @brief
    *   This makes the particle deal damage to whomever it is attached to
    *   (happens about once every second)
    **/
    void updateAttachedDamage();

    bool isVisible() const;

public:
    static const std::shared_ptr<Particle> INVALID_PARTICLE;

    // links
    /**
     * @brief
     *  The object owning this particle.
     *  Example: A fire particle is owned by a torch.
     */
    CHR_REF owner_ref;
    /**
     * @brief
     *  The original parent particle if any.
     *  The particle which has spawned this particle if any, INVALID_PRT_REF otherwise.
     */
    PRT_REF parent_ref;                        ///< Did a another particle spawn this one?

    uint16_t   attachedto_vrt_off;               ///< It's vertex offset
    uint8_t    type;                             ///< Transparency mode, 0-2
    FACING_T facing;                           ///< Direction of the part
    TEAM_REF team;                             ///< Team

    fvec3_t vel_stt;        ///< Starting/initial velocity.

    fvec3_t offset;                            ///< The initial offset when spawning the particle

    FACING_T          rotate;                  ///< Rotation direction
    Sint16            rotate_add;              ///< Rotation rate

    UFP8_T            size_stt;                ///< The initial size of particle (8.8 fixed point)
    UFP8_T            size;                    ///< Size of particle (8.8 fixed point)
    SFP8_T            size_add;                ///< Change in size (8.8 fixed point)

    /// The state of a 2D animation used for rendering the particle.
    AnimationLoop _image;

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
    *   Number of frames it has been rendered
    **/
    size_t frame_count;

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

    // bumping
    uint32_t          bump_size_stt;           ///< the starting size of the particle (8.8 fixed point)
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
    bool              no_gravity;              ///< does the particle ignore gravity?

    // some data that needs to be copied from the particle profile
    int               endspawn_characterstate; ///< if != SPAWNNOCHARACTER, then a character is spawned on end

    dynalight_info_t  dynalight;               ///< Dynamic lighting...
    prt_instance_t    inst;                    ///< Everything needed for rendering
    Ego::prt_environment_t enviro;                  ///< the particle's environment

private:
    PRT_REF       _particleID;                 ///< Unique identifier
    BSP_leaf_t    _bspLeaf;

    PIP_REF _particleProfileID;                ///< The particle profile
    std::shared_ptr<pip_t> _particleProfile;

    bool _isTerminated;                        ///< Marked for destruction. No longer part of the game and will be removed ASAP
    bool _isGhost;                             ///< the particle has been killed, but is hanging around a while...

    /**
     * @brief
     *  The object the particle is attached to.
     *  Example: A fire particle is attached to a torch.
     */
    CHR_REF _attachedTo;

    /**
     * @brief
     *  The object targeted by this particle.
     *  Example: Target-seeking arrows/bolts or similar particles.
     */
    CHR_REF _target;

    /**
     * @brief
     *  The profile related to the spawned particle.
     */
    PRO_REF _spawnerProfile;

    // motion effects
    bool _isHoming;             ///< Is the particle in control of its motion?
};

} //Ego