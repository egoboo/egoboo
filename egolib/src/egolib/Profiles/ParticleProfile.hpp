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

#pragma once
#if !defined(EGOLIB_PROFILES_PRIVATE) || EGOLIB_PROFILES_PRIVATE != 1
#error(do not include directly, include `egolib/Profiles/_Include.hpp` instead)
#endif

#include "egolib/Profiles/AbstractProfile.hpp"

/// Pre-defined global particle types
/// @note I can't place all the money particles in the same place because it is expected
/// that certain particles have certain slots (i.e. PIP_WEATHER4 being slot 4...)
/// @todo This should not be here.
enum e_global_pips
{
    PIP_COIN1 = 0,                                ///< Coins are the first particles loaded
    PIP_COIN5,
    PIP_COIN25,
    PIP_COIN100,
    PIP_WEATHER,                                  ///< Weather particles
    PIP_WEATHER_FINISH,                           ///< Weather particle finish
    PIP_SPLASH,                                   ///< Water effects are next
    PIP_RIPPLE,
    PIP_DEFEND,                                   ///< Defend particle
    PIP_GEM200,
    PIP_GEM500,
    PIP_GEM1000,
    PIP_GEM2000,
    PIP_DISINTEGRATE_START,                       ///< Disintegrate explosion start
    PIP_DISINTEGRATE_PARTICLE,                    ///< Explosion particles spawned by PIP_DISINTEGRATE_START
    GLOBAL_PIP_COUNT,

    // aliases
    PIP_MONEY_COUNT = 8
};

/// particle types / sprite dosplay modes
enum e_sprite_mode
{
    SPRITE_ALPHA = 0, ///< Smoke particle
    SPRITE_LIGHT = 1, ///< Magic effect particle
    SPRITE_SOLID = 2  ///< Sprite particle
};

/// dynamic lighting modes
enum e_dyna_mode
{
    DYNA_MODE_OFF = 0,
    DYNA_MODE_ON,
    DYNA_MODE_LOCAL
};

/// Possible methods for computing the position and orientation of the quad used to display particle sprites
enum class prt_ori_t
{
    ORIENTATION_B = 0,   ///< billboard
    ORIENTATION_X,       ///< put particle up along the world or body-fixed x-axis
    ORIENTATION_Y,       ///< put particle up along the world or body-fixed y-axis
    ORIENTATION_Z,       ///< put particle up along the world or body-fixed z-axis
    ORIENTATION_V,       ///< vertical, like a candle
    ORIENTATION_H        ///< horizontal, like a plate
};

// The special damage effects for particles
enum ParticleDamageEffectBits : size_t
{
    DAMFX_NONE,              ///< Damage effects
    DAMFX_ARMO,              ///< Armor piercing
    DAMFX_NBLOC,             ///< Cannot be blocked by shield
    DAMFX_ARRO,              ///< Only hurts the one it's attached to
    DAMFX_TURN,              ///< Turn to attached direction
    DAMFX_TIME,              ///< Do not reset the damage timer
    NR_OF_DAMFX_BITS         ///< Always last
};

/// Turn values specifying corrections to the rotation of particles
enum particle_direction_t
{
    prt_v = 0x0000,    ///< particle is vertical on the bitmap
    prt_r = 0x2000,    ///< particle is diagonal (rotated 45 degrees to the right = 8192)
    prt_h = 0x4000,    ///< particle is horizontal (rotated 90 degrees = 16384)
    prt_l = 0xE000,    ///< particle is diagonal (rotated 45 degrees to the right = 8192)
    prt_u = 0xFFFF     ///< particle is of unknown orientation
};

struct dynalight_info_t
{
    uint8_t   mode;                ///< when is it?
    uint8_t   on;                  ///< is it on now?

    float   level;               ///< intensity
    float   level_add;           ///< intensity changes

    float   falloff;             ///< range
    float   falloff_add;         ///< range changes

    dynalight_info_t();

    void reset();
};

/// The definition of a particle profile
class ParticleProfile : public AbstractProfile
{
public:
    /**
     * @brief
     *  Construct this particle profile with default values.
     */
    ParticleProfile();

    /**
     * @brief
     *  Destruct this particle profile.
     */
    virtual ~ParticleProfile();

    bool hasBit(const ParticleDamageEffectBits bit) const;

    /**
     * @brief
     *  Read a particle profile.
     * @param pathname
     *  the pathname of the file to read the data from
     * @return
     *  @a the ParticleProfile object on success, @a nullptr on failure
     */
    static std::shared_ptr<ParticleProfile> readFromFile(const std::string& pathname);

    const IPair& getSpawnFacing() const;
    
    const IPair& getSpawnPositionOffsetXY() const;
    
    const IPair& getSpawnPositionOffsetZ() const;
    
    const IPair& getSpawnVelocityOffsetXY() const;
    
    const IPair& getSpawnVelocityOffsetZ() const;
    
public:

    // Spawning.
    int8_t soundspawn;       ///< Beginning sound
    bool force;             ///< Force spawn?
    bool newtargetonspawn;  ///< Get new target?
    bool needtarget;        ///< Need a target?
    bool startontarget;     ///< Start on target?

    // Ending conditions.
    int end_time;                    ///< Time until end in seconds, (-1 for infinite).
    bool end_water;                  ///< End if underwater
    bool end_bump;                   ///< End if bumped
    bool end_ground;                 ///< End if on ground
    bool end_wall;                   ///< End if hit a wall
    bool end_lastframe;              ///< End on last frame
    
    // Ending sounds.
    int8_t end_sound;                 ///< Ending sound (-1 for none).
    int8_t end_sound_floor;           ///< Floor sound (-1 for none).
    int8_t end_sound_wall;            ///< Ricochet sound (-1 for none).

    // What/how to spawn continuously.
    ContinuousSpawnDescriptor contspawn;
    // What/how to spawn at the end.
    SpawnDescriptor endspawn;
    // What/how to spawn when bumped.
    SpawnDescriptor bumpspawn;

    // Bumping of particle into particles/objects.
    int bump_money;                  ///< Value of particle
    uint32_t bump_size;                ///< Bounding box size
    uint32_t bump_height;              ///< Bounding box height

    // Hitting.
    FRange damage;                    ///< Damage
    DamageType damageType;            ///< Damage type
    uint32_t dazeTime;            ///< How long is an Object "dazed" if hit by this particle.
    uint32_t grogTime;            ///< How long is an Object "grogged" if hit by this particle.
    bool _intellectDamageBonus;       ///< Add intellect as damage bonus.
    bool spawnenchant;                ///< Spawn enchant?
    
    ///@{ @todo The semantics of those variables are not clear.
    bool onlydamagefriendly;          ///< Only friends?
    bool friendlyfire;                ///< Friendly fire
    bool hateonly;                    ///< Only hit hategroup
    ///@}
    bool cause_roll;                  ///< @todo Not implemented!!
    bool cause_pancake;               ///< @todo Not implemented!!

    // Drains.
    UFP8_T lifeDrain;                 ///< Life drain from target and given to the source when the target is hit.
    UFP8_T manaDrain;                 ///< Mana drain from target and given to the source when the target is hit.

    // Homing.
    bool homing;                       ///< Is the particle homing?
    FACING_T targetangle;              ///< To find target.
    float homingaccel;                 ///< Acceleration rate.
    float homingfriction;              ///< Deceleration rate.
    float zaimspd;                     ///< [ZSPD] For Z aiming.
    bool rotatetoface;                 ///< Arrows/Missiles.
    bool targetcaster;                 ///< Target caster?

    // Physics.
    float spdlimit;                    ///< Speed limit
    float dampen;                      ///< Bounciness
    bool allowpush;                    ///< Allow particle to push characters around
    bool ignore_gravity;               ///< Ignores gravity

    // Visual properties.
    dynalight_info_t dynalight; ///< Dynamic lighting info
    e_sprite_mode type;         ///< Transparency mode

    /**
     * @brief
     *  The number of frames.
     */
    uint8_t image_max;            ///< Number of frames

    /**
     * @brief
     *  The index of the starting frame.
     */
    uint8_t image_stt;

    /**
     * @brief
     *  The frame rate ("base" + "range" form).
     */
    IPair image_add;

    IPair rotate_pair;          ///< Rotation
    int16_t rotate_add;          ///< Rotation rate
    uint16_t size_base;           ///< Size
    int16_t size_add;            ///< Size rate
    uint16_t facingadd;           ///< Facing
    prt_ori_t orientation;      ///< The way the particle orientation is calculated for display

private:
    std::string _comment;
    std::bitset<NR_OF_DAMFX_BITS> _particleEffectBits;

    // Initial spawning of this particle.
    IPair _spawnFacing;             ///< Facing
    IPair _spawnPositionOffsetXY;   ///< Spacing
    IPair _spawnPositionOffsetZ;    ///< Altitude
    IPair _spawnVelocityOffsetXY;   ///< Shot velocity
    IPair _spawnVelocityOffsetZ;    ///< Up velocity
};

/// @todo Remove globals.
extern particle_direction_t prt_direction[256];
