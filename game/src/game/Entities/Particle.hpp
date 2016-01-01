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
#include "game/Physics/Collidable.hpp"
#include "game/Physics/ParticlePhysics.hpp"

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

    float adj_level;              ///< The level for the particle to sit on the floor or a platform (including bump height)
    float adj_floor;              ///< The level for the particle to sit on the floor or a platform

    // friction stuff
    bool is_slipping;
    bool is_slippy, is_watery;
    float  air_friction;
    float  fluid_friction_hrz, fluid_friction_vrt;
    float  friction_hrz;
    float traction;

    // misc states
    bool   inwater;
    Vector3f  acc;

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
        air_friction = 0.0f;
        fluid_friction_hrz = fluid_friction_vrt = 0.0f;
        friction_hrz = 0.0f;
        traction = 0.0f;
        
        // misc states
        inwater = false;
        acc = Vector3f::zero();
    }
};

/**
 * @brief
 *  The definition of the particle entity.
 */
class Particle : public PhysicsData, public Id::NonCopyable, public Ego::Physics::Collidable
{
public:
    Particle();

    /**
    * @return
    *   true if this Particle can collide with Objects
    **/
    bool canCollide() const override;
    
    /**
     * @brief
     *  Get the profile index of this particle.
     * @return
     *  the profile index of this particle or INVALID_PIP_REF
     */
    PIP_REF getProfileID() const;

    /**
    * @brief
    *   Get the physics properties of this Particle 
    **/
    Ego::Physics::ParticlePhysics& getParticlePhysics();

    /**
     * @brief
     *  Get the unique particle reference of this particle. When this
     *  particle is removed from the game the particle reference will
     *  not be re-used and is forever invalid.
     * @return
     *  an unique particle reference of this particle
     */
    ParticleRef getParticleID() const;

    /**
     * @brief
     *  Get a pointer to the profile of this particle.
     * @return
     *  a pointer to the profile of this particle or a null pointer
     */
    const std::shared_ptr<ParticleProfile>& getProfile() const;

    /// @details Tell the game to get rid of this object and treat it as if it was already dead.
    /// @note This will force the game to (eventually) call end_one_particle_in_game() on this particle
    void requestTerminate();

    /**
    * @brief
    *   Set the altitude of this Particle
    **/
    void setElevation(const float level);

	BIT_FIELD hit_wall(const Vector3f& pos, Vector2f& nrm, float *pressure) override;
    BIT_FIELD hit_wall(const Vector3f& pos, Vector2f& nrm, float *pressure, mesh_wall_data_t& data) override;

	BIT_FIELD test_wall(const Vector3f& pos) override;

    /**
    * @brief
    *   Change the FP8 size of this Particle. Also updates collision bounds
    **/
    void setSize(int size);

    /// @brief Get the scale factor between the "graphical size" of the particle and the actual display size.
    float getScale() const;

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
    *   get the ID of the Object that this Particle is currently attached to (or ObjectRef::Invalid if not attached)
    **/
    ObjectRef getAttachedObjectID() const {return _attachedTo;}

    /**
    * @return
    *   true if this Particle has been terminated and will be removed from the game soon
    **/
    bool isTerminated() const;

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
    bool attach(const ObjectRef attach);

    /**
    * @brief
    *   Sets the target of this Particle
    **/
    void setTarget(const ObjectRef target);

    PRO_REF getSpawnerProfile() const { return _spawnerProfile; }

    /**
    * @return
    *   true if this Particle is currently in control of its own motion and
    *   moving in towards a valid Object target
    **/
    bool isHoming() const { return _isHoming; }

    /**
    * @brief
    *   Makes this Particle play a sound effect from its spawner sounds
    *   (or a sound from the basicdat folder if it is a global particle)
    **/
    void playSound(int8_t soundID);

    /**
    * @brief
    *   Sets wheter this Particle is control of its own motion and should
    *   home in towards its target. If the Particle has no valid target or is attached
    *   to an Object then it cannot be homing.
    **/
    void setHoming(bool homing);

    /**
    * @return
    *   true if this Particle has previously collided with the specified Object
    **/
    bool hasCollided(const std::shared_ptr<Object> &object) const;  

    /**
    * @brief
    *   Notify that this Particle has collided with the specified Object. It cannot
    *   collide with it again unless this particle is eternal
    **/
    void addCollision(const std::shared_ptr<Object> &object);

    /**
    * @return
    *   true if this Particle has no lifetime and will not timeout
    **/
    bool isEternal() const;


    /**
    * @author BB
    * @brief 
    *   A helper function for determining the owner of a particle
    *
    * @details 
    *   There could be a possibility that a particle exists that was spawned by
    *   another particle, but has lost contact with its original spawner. For instance
    *   If an explosion particle bounces off of something with MISSILE_DEFLECT or
    *   MISSILE_REFLECT, which subsequently dies before the particle...
    *
    *   That is actually pretty far fetched, but at some point it might make sense to
    *   spawn particles just keeping track of the spawner (whether particle or character)
    *   and working backward to any potential owner using this function. ;)
    *
    * @note 
    *   this function should be completely trivial for anything other than
    *   damage particles created by an explosion
    **/
    ObjectRef getOwner(int depth = 0);

//ZF> These functions should only be accessed by the ParticleHandler
public:

    /**
    * @brief
    *   initialize a Particle so that it is ready to be used
    * @note
    *   Should only ever be used by the ParticleHandler! *Do not use*
    **/
    bool initialize(const ParticleRef particleID, const Vector3f& spawnPos, const FACING_T spawnFacing, const PRO_REF spawnProfile,
                    const PIP_REF particleProfile, const ObjectRef spawnAttach, Uint16 vrt_offset, const TEAM_REF spawnTeam,
                    const ObjectRef spawnOrigin, const ParticleRef spawnParticleOrigin, const int multispawn, const ObjectRef spawnTarget,
                    const bool onlyOverWater);

    /**
    * @brief
    *   Final stuff done before the particle is removed from game
    * @note
    *   Should only ever be used by the ParticleHandler! *Do not use*
    **/
    void destroy();

private:
    /**
     * @brief
     *  Handle the particle interaction with water
     */
    void updateWater();

    /**
     * @brief
     *  Update the animation of this particle.
     */
    void updateAnimation();

    /**
    * @brief
    *   Update particle lighting effects
    **/
    void updateDynamicLighting();

    /**
    * @brief
    *   Check if particle should spawn additional new particles
    **/
    size_t updateContinuousSpawning();

    /**
    * @brief
    *   This makes the particle deal damage to whomever it is attached to
    *   (happens about once every second)
    **/
    void updateAttachedDamage();

    /**
    * @brief
    *   Clear the data of a Particle
    **/
    void reset(ParticleRef ref);

public:
    static constexpr int SPAWNNOCHARACTER = 255;         ///< For particles that spawn characters...

    static const std::shared_ptr<Particle> INVALID_PARTICLE;

    // links
    /**
     * @brief
     *  Object reference of the object owning this particle if any, an invalid object reference otherwise.
     *  Example: A fire particle is owned by a torch.
     */
    ObjectRef owner_ref;
    /**
     * @brief
     *  Particle reference of the parent particle if any, an invalid particle reference otherwise.
     * @remark
     *  The parent particle is the particle which has spawned this particle.
     */
    ParticleRef parent_ref;

    uint16_t   attachedto_vrt_off;             ///< It's vertex offset
    uint8_t    type;                           ///< Transparency mode, 0-2
    FACING_T facing;                           ///< Direction of the part
    TEAM_REF team;                             ///< Team

	Vector3f vel_stt;                          ///< Starting/initial velocity.

	Vector3f offset;                           ///< The initial offset when spawning the particle

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
    ParticleRef _particleID;                 ///< Unique identifier

    //Collisions
    Ego::Physics::ParticlePhysics _particlePhysics;
    std::forward_list<ObjectRef> _collidedObjects;    ///< List of the ID's of all Object this particle has collided with

    //Profile
    PIP_REF _particleProfileID;                ///< The particle profile
    std::shared_ptr<ParticleProfile> _particleProfile;

    bool _isTerminated;                        ///< Marked for destruction. No longer part of the game and will be removed ASAP

    /**
     * @brief
     *  The object the particle is attached to.
     *  Example: A fire particle is attached to a torch.
     */
    ObjectRef _attachedTo;

    /**
     * @brief
     *  The object targeted by this particle.
     *  Example: Target-seeking arrows/bolts or similar particles.
     */
    ObjectRef _target;

    /**
     * @brief
     *  The profile related to the spawned particle.
     */
    PRO_REF _spawnerProfile;

    // motion effects
    bool _isHoming;             ///< Is the particle in control of its motion?
};

} //Ego