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

/* Egoboo - pip_file.h
 * routines for reading and writing the particle profile file "part*.txt"
 */

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// pre-defined global particles
#define COIN1               0                       // Coins are the first particles loaded
#define COIN5               1
#define COIN25              2
#define COIN100             3
#define WEATHER4            4                       // Weather particles
#define WEATHER5            5                       // Weather particle finish
#define SPLASH              6                       // Water effects are next
#define RIPPLE              7
#define DEFEND              8                       // Defend particle

#define PRTLIGHTSPRITE                  0           // Magic effect particle
#define PRTSOLIDSPRITE                  1           // Sprite particle
#define PRTALPHASPRITE                  2           // Smoke particle

// dynalight constants
#define DYNAOFF   0
#define DYNAON    1
#define DYNALOCAL 2
// #define MAXFALLOFF 1400

enum e_prt_orientations
{
    ORIENTATION_B = 0,   // billboard
    ORIENTATION_X,       // put particle up along the world or body-fixed x-axis
    ORIENTATION_Y,       // put particle up along the world or body-fixed y-axis
    ORIENTATION_Z,       // put particle up along the world or body-fixed z-axis
    ORIENTATION_V,       // vertical, like a candle
    ORIENTATION_H        // horizontal, like a plate
};
typedef enum e_prt_orientations prt_ori_t;

enum e_damage_fx
{
    DAMFX_NONE           = 0,                       // Damage effects
    DAMFX_ARMO           = (1 << 1),                // Armor piercing
    DAMFX_NBLOC          = (1 << 2),                // Cannot be blocked by shield
    DAMFX_ARRO           = (1 << 3),                // Only hurts the one it's attached to
    DAMFX_TURN           = (1 << 4),                // Turn to attached direction
    DAMFX_TIME           = (1 << 5)
};

//--------------------------------------------------------------------------------------------
// Particle template
//--------------------------------------------------------------------------------------------
struct s_pip
{
    EGO_PROFILE_STUFF;

    bool_t  force;                        // Force spawn?

    Uint8   type;                         // Transparency mode
    Uint8   numframes;                    // Number of frames
    Uint8   imagebase;                    // Starting image
    Uint16  imageadd;                     // Frame rate
    Uint16  imageaddrand;                 // Frame rate randomness
    Uint16  time;                         // Time until end
    IPair   rotate_pair;                   // Rotation
    Sint16  rotateadd;                    // Rotation rate
    Uint16  sizebase;                     // Size
    Sint16  sizeadd;                      // Size rate
    float   spdlimit;                     // Speed limit
    float   dampen;                       // Bounciness
    Sint8   bumpmoney;                    // Value of particle
    Uint32  bumpsize;                     // Bounding box size
    Uint32  bumpheight;                   // Bounding box height
    bool_t  endwater;                     // End if underwater
    bool_t  endbump;                      // End if bumped
    bool_t  endground;                    // End if on ground
    bool_t  endwall;                      // End if hit a wall
    bool_t  endlastframe;                 // End on last frame
    IPair   damage;                       // Damage
    Uint8   damagetype;                   // Damage type
    Uint16  facingadd;                    // Facing
    IPair   facing_pair;                   // Facing
    IPair   xyspacing_pair;                // Spacing
    IPair   zspacing_pair;                 // Altitude
    IPair   xyvel_pair;                    // Shot velocity
    IPair   zvel_pair;                     // Up velocity
    Uint16  contspawntime;                // Spawn timer
    Uint8   contspawnamount;              // Spawn amount
    Uint16  contspawnfacingadd;           // Spawn in circle
    Uint16  contspawnpip;                 // Spawn type ( local )
    Uint8   endspawnamount;               // Spawn amount
    Uint16  endspawnfacingadd;            // Spawn in circle
    Uint8   endspawnpip;                  // Spawn type ( local )
    Uint8   bumpspawnamount;              // Spawn amount
    Uint8   bumpspawnpip;                 // Spawn type ( global )
    Uint8   dynalight_mode;                // Dynamic light on?
    float   dynalight_level;                    // Intensity
    Uint16  dynalight_falloff;                  // Falloff
    Uint16  dazetime;                     // Daze
    Uint16  grogtime;                     // Drunkeness
    Sint8   soundspawn;                   // Beginning sound
    Sint8   soundend;                     // Ending sound
    Sint8   soundfloor;                   // Floor sound
    Sint8   soundwall;                    // Ricochet sound
    bool_t  friendlyfire;                 // Friendly fire
    bool_t  hateonly;                     // Only hit hategroup
    bool_t  rotatetoface;                 // Arrows/Missiles
    bool_t  newtargetonspawn;             // Get new target?
    bool_t  homing;                       // Homing?
    Uint16  targetangle;                  // To find target
    float   homingaccel;                  // Acceleration rate
    float   homingfriction;               // Deceleration rate
    float   dynalight_leveladd;            // Dyna light changes
    float   dynalight_falloffadd;
    bool_t  targetcaster;                 // Target caster?
    bool_t  spawnenchant;                 // Spawn enchant?
    bool_t  causepancake;                 // !!BAD: Not implemented!!
    bool_t  needtarget;                   // Need a target?
    bool_t  onlydamagefriendly;           // Only friends?
    bool_t  startontarget;                // Start on target?
    int     zaimspd;                      // [ZSPD] For Z aiming
    Uint16  damfx;                        // Damage effects
    bool_t  allowpush;                    // Allow particle to push characters around
    bool_t  intdamagebonus;               // Add intelligence as damage bonus
    bool_t  wisdamagebonus;               // Add wisdom as damage bonus

    prt_ori_t orientation;
};

typedef struct s_pip pip_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

pip_t * load_one_pip_file( const char *szLoadName, pip_t * ppip );