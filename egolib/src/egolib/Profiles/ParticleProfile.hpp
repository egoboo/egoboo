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

#include "egolib/typedef.h"

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
    GLOBAL_PIP_COUNT,

    // aliases
    PIP_MONEY_COUNT = 8
};

/// particle types / sprite dosplay modes
enum e_sprite_mode
{
    SPRITE_LIGHT = 0,                         ///< Magic effect particle
    SPRITE_SOLID,                             ///< Sprite particle
    SPRITE_ALPHA                              ///< Smoke particle
};

/// dynamic lighting modes
enum e_dyna_mode
{
    DYNA_MODE_OFF = 0,
    DYNA_MODE_ON,
    DYNA_MODE_LOCAL
};

/// Possible methods for computing the position and orientation of the quad used to display particle sprites
enum prt_ori_t
{
    ORIENTATION_B = 0,   ///< billboard
    ORIENTATION_X,       ///< put particle up along the world or body-fixed x-axis
    ORIENTATION_Y,       ///< put particle up along the world or body-fixed y-axis
    ORIENTATION_Z,       ///< put particle up along the world or body-fixed z-axis
    ORIENTATION_V,       ///< vertical, like a candle
    ORIENTATION_H        ///< horizontal, like a plate
};

// The special damage effects for particles
enum e_damage_fx
{
    DAMFX_NONE = 0,                       ///< Damage effects
    DAMFX_ARMO = (1 << 1),              ///< Armor piercing
    DAMFX_NBLOC = (1 << 2),              ///< Cannot be blocked by shield
    DAMFX_ARRO = (1 << 3),              ///< Only hurts the one it's attached to
    DAMFX_TURN = (1 << 4),              ///< Turn to attached direction
    DAMFX_TIME = (1 << 5)               ///< Do not reset the damage timer
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
    Uint8   mode;                ///< when is it?
    Uint8   on;                  ///< is it on now?

    float   level;               ///< intensity
    float   level_add;           ///< intensity changes

    float   falloff;             ///< range
    float   falloff_add;         ///< range changes
};

/// The definition of a particle profile
struct pip_t
{
    EGO_PROFILE_STUFF

    char comment[1024];   ///< The first line of the file has a comment line.

    // spawning
    bool force;             ///< Force spawn?
    Uint8 type;             ///< Transparency mode
    Uint8 numframes;        ///< Number of frames
    Uint8 image_base;       ///< Starting image
    IPair image_add;        ///< Frame rate
    IPair rotate_pair;      ///< Rotation
    Sint16 rotate_add;      ///< Rotation rate
    Uint16 size_base;       ///< Size
    Sint16 size_add;        ///< Size rate
    Sint8 soundspawn;       ///< Beginning sound
    Uint16 facingadd;       ///< Facing
    IPair facing_pair;      ///< Facing
    IPair spacing_hrz_pair; ///< Spacing
    IPair spacing_vrt_pair; ///< Altitude
    IPair vel_hrz_pair;     ///< Shot velocity
    IPair vel_vrt_pair;     ///< Up velocity
    bool newtargetonspawn;  ///< Get new target?
    bool needtarget;        ///< Need a target?
    bool startontarget;     ///< Start on target?

    // ending conditions
    int end_time;                    ///< Time until end
    bool end_water;                  ///< End if underwater
    bool end_bump;                   ///< End if bumped
    bool end_ground;                 ///< End if on ground
    bool end_wall;                   ///< End if hit a wall
    bool end_lastframe;              ///< End on last frame
    Sint8 end_sound;                 ///< Ending sound
    Sint8 end_sound_floor;           ///< Floor sound
    Sint8 end_sound_wall;            ///< Ricochet sound

    // end spawn
    Uint8 endspawn_amount;           ///< Spawn amount
    Uint16 endspawn_facingadd;       ///< Spawn in circle
    int endspawn_lpip;               ///< Spawn type ( local )

    // bumping
    int bump_money;                  ///< Value of particle
    Uint32 bump_size;                ///< Bounding box size
    Uint32 bump_height;              ///< Bounding box height

    // "bump particle" spawning
    Uint8 bumpspawn_amount;          ///< Spawn amount
    int bumpspawn_lpip;              ///< Spawn type ( local )

    // continuous spawning
    Uint16 contspawn_delay;       ///< Spawn timer
    Uint8 contspawn_amount;       ///< Spawn amount
    Uint16 contspawn_facingadd;   ///< Spawn in circle
    int contspawn_lpip;           ///< Spawn type ( local )

    // damage
    FRange damage;                    ///< Damage
    Uint8 damagetype;                 ///< Damage type
    int daze_time;                    ///< Daze
    int grog_time;                    ///< Drunkeness
    BIT_FIELD damfx;                  ///< Damage effects
    bool intdamagebonus;              ///< Add intelligence as damage bonus
    bool wisdamagebonus;              ///< Add wisdom as damage bonus
    bool spawnenchant;                ///< Spawn enchant?
    bool onlydamagefriendly;          ///< Only friends?
    bool friendlyfire;                ///< Friendly fire
    bool hateonly;                    ///< Only hit hategroup
    bool cause_roll;                  ///< @todo Not implemented!!
    bool cause_pancake;               ///< @todo Not implemented!!
    Uint16 lifedrain;                 ///< Steal this much life
    Uint16 manadrain;                 ///< Steal this much mana

    // homing
    bool homing;                       ///< Homing?
    FACING_T targetangle;              ///< To find target
    float homingaccel;                 ///< Acceleration rate
    float homingfriction;              ///< Deceleration rate
    float zaimspd;                     ///< [ZSPD] For Z aiming
    bool rotatetoface;                 ///< Arrows/Missiles
    bool targetcaster;                 ///< Target caster?

    // physics
    float spdlimit;                    ///< Speed limit
    float dampen;                      ///< Bounciness
    bool allowpush;                    ///< Allow particle to push characters around
    bool ignore_gravity;               ///< Ignores gravity

    dynalight_info_t dynalight;           ///< Dynamic lighting info

    prt_ori_t orientation;                ///< the way the particle orientation is calculated for display

    pip_t *init();
};

/// @todo Remove globals.
extern particle_direction_t prt_direction[256];
