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

#include "egoboo_typedef.h"
#include "egoboo_math.h"

#include "pip_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define PRTLEVELFIX         20                      // Fix for shooting over cliffs

// Particles
#define MAXPARTICLEIMAGE                256         // Number of particle images ( frames )
#define DYNAFANS  12

// Physics
#define STOPBOUNCINGPART                5.0f         // To make particles stop bouncing

DEFINE_STACK_EXTERN(pip_t, PipStack, MAX_PIP );

#define VALID_PIP_RANGE( IPIP ) ( ((IPIP) >= 0) && ((IPIP) < MAX_PIP) )
#define VALID_PIP( IPIP )       ( VALID_PIP_RANGE( IPIP ) && PipStack.lst[IPIP].loaded )
#define INVALID_PIP( IPIP )     ( !VALID_PIP_RANGE( IPIP ) || !PipStack.lst[IPIP].loaded )

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
    Uint16  iprofile;                        // the profile related to the spawned particle

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
    Uint16  fanblock_next;                   // Next particle on fanblock
    IPair   damage;                          // For strength
    Uint8   damagetype;                      // Damage type
    Uint16  chr;                             // The character that is attacking
    float   dynalight_falloff;                // Dyna light...
    float   dynalight_level;
    bool_t  dynalight_on;                     // Dynamic light?
    Uint16  target;                          // Who it's chasing

    bool_t  is_eternal;

    prt_instance_t inst;
};
typedef struct s_prt prt_t;

extern float            sprite_list_u[MAXPARTICLEIMAGE][2];        // Texture coordinates
extern float            sprite_list_v[MAXPARTICLEIMAGE][2];

extern Uint16           maxparticles;                              // max number of particles

DEFINE_LIST_EXTERN(prt_t, PrtList, TOTAL_MAX_PRT);

#define VALID_PRT_RANGE( IPRT ) ( ((IPRT) >= 0) && ((IPRT) < maxparticles) && ((IPRT) < TOTAL_MAX_PRT) )
#define VALID_PRT( IPRT )       ( VALID_PRT_RANGE( IPRT ) && PrtList.lst[IPRT].on )
#define INVALID_PRT( IPRT )     ( !VALID_PRT_RANGE( IPRT ) || !PrtList.lst[IPRT].on )

//--------------------------------------------------------------------------------------------
// function prototypes

void   init_all_pip();
void   release_all_pip();
bool_t release_one_pip( Uint16 ipip );

bool_t PrtList_free_one( Uint16 particle );
void   free_one_particle_in_game( Uint16 particle );

void move_particles( void );
void PrtList_free_all();

void setup_particles();

void play_particle_sound( Uint16 particle, Sint8 sound );
int get_free_particle( int force );
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 pip,
                           Uint16 characterattach, Uint16 vrt_offset, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget );

int prt_count_free();

int load_one_particle_profile( const char *szLoadName, Uint16 pip_override );
void reset_particles( const char* modname );

Uint8 __prthitawall( Uint16 particle );

int    prt_is_over_water( Uint16 cnt );

bool_t release_one_pip( Uint16 ipip );

Uint16  prt_get_ipip( Uint16 cnt );
pip_t * prt_get_ppip( Uint16 cnt );
