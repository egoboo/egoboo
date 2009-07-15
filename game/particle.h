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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egoboo_typedef.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define PRTLEVELFIX         20                      // Fix for shooting over cliffs

// Particles
#define PRTLIGHTSPRITE                  0           // Magic effect particle
#define PRTSOLIDSPRITE                  1           // Sprite particle
#define PRTALPHASPRITE                  2           // Smoke particle
#define MAXPARTICLEIMAGE                256         // Number of particle images ( frames )
#define DYNAFANS  12

// dynalight constants
#define DYNAOFF   0
#define DYNAON    1
#define DYNALOCAL 2
//#define MAXFALLOFF 1400

//Physics
#define STOPBOUNCINGPART                5.0f         // To make particles stop bouncing

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

//------------------------------------
// Particle template
//------------------------------------
struct s_pip
{
    EGO_PROFILE_STUFF

    bool_t  force;                        // Force spawn?

    Uint8   type;                         // Transparency mode
    Uint8   numframes;                    // Number of frames
    Uint8   imagebase;                    // Starting image
    Uint16  imageadd;                     // Frame rate
    Uint16  imageaddrand;                 // Frame rate randomness
    Uint16  time;                         // Time until end
    Uint16  rotatebase;                   // Rotation
    Uint16  rotaterand;                   // Rotation
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
    Uint16  damagebase;                   // Damage
    Uint16  damagerand;                   // Damage
    Uint8   damagetype;                   // Damage type
    Sint16  facingbase;                   // Facing
    Uint16  facingadd;                    // Facing
    Uint16  facingrand;                   // Facing
    Sint16  xyspacingbase;                // Spacing
    Uint16  xyspacingrand;                // Spacing
    Sint16  zspacingbase;                 // Altitude
    Uint16  zspacingrand;                 // Altitude
    Sint8   xyvelbase;                    // Shot velocity
    Uint8   xyvelrand;                    // Shot velocity
    Sint8   zvelbase;                     // Up velocity
    Uint8   zvelrand;                     // Up velocity
    Uint16  contspawntime;                // Spawn timer
    Uint8   contspawnamount;              // Spawn amount
    Uint16  contspawnfacingadd;           // Spawn in circle
    Uint16  contspawnpip;                 // Spawn type ( local )
    Uint8   endspawnamount;               // Spawn amount
    Uint16  endspawnfacingadd;            // Spawn in circle
    Uint8   endspawnpip;                  // Spawn type ( local )
    Uint8   bumpspawnamount;              // Spawn amount
    Uint8   bumpspawnpip;                 // Spawn type ( global )
    Uint8   dynalightmode;                // Dynamic light on?
    float   dynalevel;                    // Intensity
    Uint16  dynafalloff;                  // Falloff
    Uint16  dazetime;                     // Daze
    Uint16  grogtime;                     // Drunkeness
    Sint8   soundspawn;                   // Beginning sound
    Sint8   soundend;                     // Ending sound
    Sint8   soundfloor;                   // Floor sound
    Sint8   soundwall;                    // Ricochet sound
    bool_t  friendlyfire;                 // Friendly fire
    bool_t  hateonly;                     //Only hit hategroup
    bool_t  rotatetoface;                 // Arrows/Missiles
    bool_t  newtargetonspawn;             // Get new target?
    bool_t  homing;                       // Homing?
    Uint16  targetangle;                  // To find target
    float   homingaccel;                  // Acceleration rate
    float   homingfriction;               // Deceleration rate
    float   dynalightleveladd;            // Dyna light changes
    float   dynalightfalloffadd;
    bool_t  targetcaster;                 // Target caster?
    bool_t  spawnenchant;                 // Spawn enchant?
	bool_t  causepancake;				  // !!BAD: Not implemented!!
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

extern int   numpip;
extern pip_t PipList[MAX_PIP];

#define VALID_PIP_RANGE( IPIP ) ( ((IPIP) >= 0) && ((IPIP) < MAX_PIP) )
#define VALID_PIP( IPIP )       ( VALID_PIP_RANGE( IPIP ) && PipList[IPIP].loaded )
#define INVALID_PIP( IPIP )     ( !VALID_PIP_RANGE( IPIP ) || !PipList[IPIP].loaded )

//------------------------------------
// Particle graphic data
//------------------------------------
struct s_prt_instance
{
    bool_t valid;

    // basic info
    Uint8    type;               // particle type
    Uint16   image;              // which image
    Uint8    alpha;              // base alpha
    Uint8    light;              // base self lighting

    // position info
    GLvector3 pos;
    float     size;

    // billboard info
    prt_ori_t orientation;
    GLvector3 up;
    GLvector3 right;
    GLvector3 nrm;

    // lighting info
    float    famb;               // cached ambient light
    float    fdir;               // cached directional light

    float    fintens;            // current brightness
    float    falpha;             // current alpha

    // graphical optimizations
    bool_t         indolist;        // Has it been added yet?

};
typedef struct s_prt_instance prt_instance_t;

//------------------------------------
// Particle variables
//------------------------------------
#define SPAWNNOCHARACTER        255                                      // For particles that spawn characters...
#define TOTAL_MAX_PRT            2048                                      // True max number of particles

struct s_prt
{
    EGO_OBJECT_STUFF

    // profiles
    Uint16  pip;                             // The part template
    Uint16  model;                           // Pip spawn model

    Uint16  attachedtocharacter;             // For torch flame
    Uint16  vrt_off;                         // It's vertex offset
    Uint8   type;                            // Transparency mode, 0-2
    Uint16  facing;                          // Direction of the part
    Uint8   team;                            // Team

    GLvector3   pos;                            // Position
    GLvector3   vel;                            // Velocity

    Uint32  onwhichfan;                      // Where the part is
    Uint32  onwhichblock;                         // The particle's collision block
    bool_t  is_hidden;

    float   floor_level;                           // Height of tile
    Uint8   spawncharacterstate;
    Uint16  rotate;                          // Rotation direction
    Sint16  rotateadd;                       // Rotation rate
    Uint16  size;                            // Size of particle>>8
    Sint16  sizeadd;                         // Change in size
    bool_t  inview;                          // Render this one?
    Uint16  image;                           // Which image ( >> 8 )
    Uint16  imageadd;                        // Animation rate
    Uint16  imagemax;                        // End of image loop
    Uint16  imagestt;                        // Start of image loop

    Uint32  time;                           // Duration of particle
    bool_t  poofme;                          // end this particle for being out of time
    Uint16  spawntime;                       // Time until spawn

    Uint32  bumpsize;                        // Size of bumpers
    Uint32  bumpsizebig;
    Uint8   bumpheight;                      // Bounding box height
    Uint16  fanblock_next;                        // Next particle on fanblock
    Uint16  damagebase;                      // For strength
    Uint16  damagerand;                      // For fixes...
    Uint8   damagetype;                      // Damage type
    Uint16  chr;                             // The character that is attacking
    float   dynalightfalloff;                // Dyna light...
    float   dynalightlevel;
    bool_t  dynalighton;                     // Dynamic light?
    Uint16  target;                          // Who it's chasing

    bool_t  is_eternal;

    prt_instance_t inst;
};
typedef struct s_prt prt_t;

extern float            sprite_list_u[MAXPARTICLEIMAGE][2];        // Texture coordinates
extern float            sprite_list_v[MAXPARTICLEIMAGE][2];

extern Uint16           maxparticles;                              // max number of particles
extern prt_t            PrtList[TOTAL_MAX_PRT];

#define VALID_PRT_RANGE( IPRT ) ( ((IPRT) >= 0) && ((IPRT) < maxparticles) && ((IPRT) < TOTAL_MAX_PRT) )
#define VALID_PRT( IPRT )       ( VALID_PRT_RANGE( IPRT ) && PrtList[IPRT].on )
#define INVALID_PRT( IPRT )     ( !VALID_PRT_RANGE( IPRT ) || !PrtList[IPRT].on )

//--------------------------------------------------------------------------------------------
// function prototypes

void free_one_particle( Uint16 particle );
void free_one_particle_in_game( Uint16 particle );

void move_particles( void );
void free_all_particles();

void setup_particles();

void play_particle_sound( Uint16 particle, Sint8 sound );
int get_free_particle( int force );
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 pip,
                           Uint16 characterattach, Uint16 vrt_offset, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget );

int prt_count_free();

int load_one_particle_profile( const char *szLoadName );
void reset_particles( const char* modname );

Uint8 __prthitawall( Uint16 particle );

int    prt_is_over_water( Uint16 cnt );
