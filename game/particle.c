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

/* Egoboo - particle.c
* Manages particle systems.
*/

#include "particle.h"

#include "log.h"
#include "sound.h"
#include "camera.h"
#include "enchant.h"
#include "char.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum e_particle_direction
{
    prt_v = 0x0000,    // particle is vertical on the bitmap
    prt_d = 0x2000,    // particle is diagonal (rotated 45 degrees to the right = 8192)
    prt_h = 0x4000,    // particle is horizontal (rotated 90 degrees = 16384)
    prt_u = 0xFFFF,    // particle is of unknown orientation
};
typedef enum e_particle_direction particle_direction_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int       numfreeprt = 0;                            // For allocation
static Uint16    freeprtlist[TOTALMAXPRT];

int              numpip   = 0;
pip_t            PipList[TOTALMAXPRTPIP];

Uint16           particletexture = 0;                        // All in one bitmap
float            particleimageu[MAXPARTICLEIMAGE][2];        // Texture coordinates
float            particleimagev[MAXPARTICLEIMAGE][2];

Uint16           maxparticles = 512;                            // max number of particles
prt_t            PrtList[TOTALMAXPRT];




particle_direction_t prt_direction[256] = 
{
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_d, prt_v, prt_v, prt_v, prt_v, prt_d, prt_d, prt_v, prt_d, prt_d, prt_d, prt_d, prt_d, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_d, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_d, prt_d, prt_d, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_d, prt_d, prt_d, prt_v, prt_v, prt_v, prt_v, 
    prt_u, prt_u, prt_u, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_d, prt_u, prt_u, prt_u, prt_u, 
    prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_v, prt_u, prt_u, prt_u, prt_u
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int prt_count_free()
{
    return numfreeprt;
}

//--------------------------------------------------------------------------------------------
void free_one_particle_no_sound( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack
    freeprtlist[numfreeprt] = particle;
    numfreeprt++;
    PrtList[particle].on = bfalse;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( Uint16 particle, Sint8 sound )
{
    // This function plays a sound effect for a particle

    prt_t * pprt;

    if( !VALID_PRT(particle) ) return;
    pprt = PrtList + particle;

    if ( sound >= 0 && sound < MAXWAVE )
    {
        if ( VALID_CAP( pprt->model ) )
        {
            cap_t * pcap = CapList + pprt->model;
            sound_play_chunk( pprt->xpos, pprt->ypos, pcap->wavelist[sound] );
        }
        else
        {
            sound_play_chunk( pprt->xpos, pprt->ypos, g_wavelist[sound] );
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack and
    //     plays the sound associated with the particle
    Uint16 child;
    prt_t * pprt;

    if( INVALID_PRT( particle) ) return;
    pprt = PrtList + particle;

    if ( pprt->spawncharacterstate != SPAWNNOCHARACTER )
    {
        child = spawn_one_character( pprt->xpos, pprt->ypos, pprt->zpos,
                                     pprt->model, pprt->team, 0, pprt->facing,
                                     NULL, MAXCHR );
        if ( VALID_CHR(child) )
        {
            ChrList[child].ai.state = pprt->spawncharacterstate;
            ChrList[child].ai.owner = pprt->chr;
        }
    }

    if( VALID_PIP(pprt->pip) )
    {
        play_particle_sound( particle, PipList[pprt->pip].soundend );
    }

    free_one_particle_no_sound( particle );
}

//--------------------------------------------------------------------------------------------
int get_free_particle( int force )
{
    // ZZ> This function gets an unused particle.  If all particles are in use
    //     and force is set, it grabs the first unimportant one.  The particle
    //     index is the return value
    int particle;

    // Return maxparticles if we can't find one
    particle = TOTALMAXPRT;
    if ( numfreeprt == 0 )
    {
        if ( force )
        {
            // Gotta find one, so go through the list
            particle = 0;

            while ( particle < maxparticles )
            {
                if ( PrtList[particle].bumpsize == 0 )
                {
                    // Found one
                    return particle;
                }

                particle++;
            }
        }
    }
    else
    {
        if ( force || numfreeprt > ( maxparticles / 4 ) )
        {
            // Just grab the next one
            numfreeprt--;
            particle = freeprtlist[numfreeprt];
        }
    }

    return (particle >= maxparticles) ? TOTALMAXPRT : particle;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 ipip,
                           Uint16 characterattach, Uint16 grip, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget )
{
    // ZZ> This function spawns a new particle, and returns the number of that particle
    int iprt, velocity;
    float xvel, yvel, zvel, tvel;
    int offsetfacing = 0, newrand;
    pip_t * ppip;

    // Convert from local ipip to global ipip
    if ( ipip < MAXPRTPIPPEROBJECT && model < MAXMODEL && MadList[model].used )
    {
        ipip = MadList[model].prtpip[ipip];
    }
    if( INVALID_PIP(ipip) ) return TOTALMAXPRT;
    ppip = PipList + ipip;

    iprt = get_free_particle( ppip->force );
    if ( VALID_PRT_RANGE(iprt) )
    {
        // Necessary data for any part
        prt_t * pprt = PrtList + iprt;

        pprt->on = btrue;
        pprt->pip = ipip;
        pprt->model = model;
        pprt->inview = bfalse;
        pprt->floor_level = 0;
        pprt->team = team;
        pprt->chr = characterorigin;
        pprt->damagetype = ppip->damagetype;
        pprt->spawncharacterstate = SPAWNNOCHARACTER;

        // Lighting and sound
        pprt->dynalighton = bfalse;
        if ( multispawn == 0 )
        {
            pprt->dynalighton = ppip->dynalightmode;
            if ( ppip->dynalightmode == DYNALOCAL )
            {
                pprt->dynalighton = bfalse;
            }
        }

        pprt->dynalightlevel = ppip->dynalevel;
        pprt->dynalightfalloff = ppip->dynafalloff;

        // Set character attachments ( characterattach==MAXCHR means none )
        pprt->attachedtocharacter = characterattach;
        pprt->grip = grip;

        // Correct facing
        facing += ppip->facingbase;

        // Targeting...
        zvel = 0;
        newrand = RANDIE;
        z = z + ppip->zspacingbase + ( newrand & ppip->zspacingrand ) - ( ppip->zspacingrand >> 1 );
        newrand = RANDIE;
        velocity = ( ppip->xyvelbase + ( newrand & ppip->xyvelrand ) );
        pprt->target = oldtarget;
        if ( ppip->newtargetonspawn )
        {
            if ( ppip->targetcaster )
            {
                // Set the target to the caster
                pprt->target = characterorigin;
            }
            else
            {

                // Find a target
                pprt->target = get_particle_target( x, y, z, facing, ipip, team, characterorigin, oldtarget );
                if ( pprt->target != MAXCHR && !ppip->homing )
                {
                    facing = facing - glouseangle;
                }

                // Correct facing for dexterity...
                offsetfacing = 0;
                if ( ChrList[characterorigin].dexterity < PERFECTSTAT )
                {
                    // Correct facing for randomness
                    offsetfacing = RANDIE;
                    offsetfacing = offsetfacing & ppip->facingrand;
                    offsetfacing -= ( ppip->facingrand >> 1 );
                    offsetfacing = ( offsetfacing * ( PERFECTSTAT - ChrList[characterorigin].dexterity ) ) / PERFECTSTAT;  // Divided by PERFECTSTAT
                }
                if ( pprt->target != MAXCHR && ppip->zaimspd != 0 )
                {
                    // These aren't velocities...  This is to do aiming on the Z axis
                    if ( velocity > 0 )
                    {
                        xvel = ChrList[pprt->target].xpos - x;
                        yvel = ChrList[pprt->target].ypos - y;
                        tvel = SQRT( xvel * xvel + yvel * yvel ) / velocity;  // This is the number of steps...
                        if ( tvel > 0 )
                        {
                            zvel = ( ChrList[pprt->target].zpos + ( ChrList[pprt->target].bumpsize >> 1 ) - z ) / tvel;  // This is the zvel alteration
                            if ( zvel < -( ppip->zaimspd >> 1 ) ) zvel = -( ppip->zaimspd >> 1 );
                            if ( zvel > ppip->zaimspd ) zvel = ppip->zaimspd;
                        }
                    }
                }
            }

            // Does it go away?
            if ( pprt->target == MAXCHR && ppip->needtarget )
            {
                free_one_particle( iprt );
                return maxparticles;
            }

            // Start on top of target
            if ( pprt->target != MAXCHR && ppip->startontarget )
            {
                x = ChrList[pprt->target].xpos;
                y = ChrList[pprt->target].ypos;
            }
        }
        else
        {
            // Correct facing for randomness
            offsetfacing = RANDIE;
            offsetfacing = offsetfacing & ppip->facingrand;
            offsetfacing -= ( ppip->facingrand >> 1 );
        }

        facing += offsetfacing;
        pprt->facing = facing;
        facing = facing >> 2;

        // Location data from arguments
        newrand = RANDIE;
        x = x + turntocos[( facing+8192 )&TRIG_TABLE_MASK] * ( ppip->xyspacingbase + ( newrand & ppip->xyspacingrand ) );
        y = y + turntosin[( facing+8192 )&TRIG_TABLE_MASK] * ( ppip->xyspacingbase + ( newrand & ppip->xyspacingrand ) );
        if ( x < 0 )  x = 0;
        if ( x > mesh.info.edge_x - 2 )  x = mesh.info.edge_x - 2;
        if ( y < 0 )  y = 0;
        if ( y > mesh.info.edge_y - 2 )  y = mesh.info.edge_y - 2;

        pprt->xpos = x;
        pprt->ypos = y;
        pprt->zpos = z;

        // Velocity data
        xvel = turntocos[( facing+8192 )&TRIG_TABLE_MASK] * velocity;
        yvel = turntosin[( facing+8192 )&TRIG_TABLE_MASK] * velocity;
        newrand = RANDIE;
        zvel += ppip->zvelbase + ( newrand & ppip->zvelrand ) - ( ppip->zvelrand >> 1 );
        pprt->xvel = xvel;
        pprt->yvel = yvel;
        pprt->zvel = zvel;

        // Template values
        pprt->bumpsize = ppip->bumpsize;
        pprt->bumpsizebig = pprt->bumpsize + ( pprt->bumpsize >> 1 );
        pprt->bumpheight = ppip->bumpheight;
        pprt->type = ppip->type;

        // Image data
        newrand = RANDIE;
        pprt->rotate = ( newrand & ppip->rotaterand ) + ppip->rotatebase;
        pprt->rotateadd = ppip->rotateadd;
        pprt->size = ppip->sizebase;
        pprt->sizeadd = ppip->sizeadd;
        pprt->image = 0;
        newrand = RANDIE;
        pprt->imageadd = ppip->imageadd + ( newrand & ppip->imageaddrand );
        pprt->imagestt = INT_TO_FP8( ppip->imagebase );
        pprt->imagemax = INT_TO_FP8( ppip->numframes );
        pprt->time = ppip->time;
        if ( ppip->endlastframe && pprt->imageadd != 0 )
        {
            if ( pprt->time == 0 )
            {
                // Part time is set to 1 cycle
                pprt->time = ( pprt->imagemax / pprt->imageadd ) - 1;
            }
            else
            {
                // Part time is used to give number of cycles
                pprt->time = pprt->time * ( ( pprt->imagemax / pprt->imageadd ) - 1 );
            }
        }

        // Set onwhichfan...
        pprt->onwhichfan   = mesh_get_tile( pprt->xpos, pprt->ypos );
        pprt->onwhichblock = mesh_get_block( pprt->xpos, pprt->ypos );

        // Damage stuff
        pprt->damagebase = ppip->damagebase;
        pprt->damagerand = ppip->damagerand;

        // Spawning data
        pprt->spawntime = ppip->contspawntime;
        if ( pprt->spawntime != 0 )
        {
            pprt->spawntime = 1;
            if ( pprt->attachedtocharacter != MAXCHR )
            {
                pprt->spawntime++; // Because attachment takes an update before it happens
            }
        }

        // Sound effect
        play_particle_sound( iprt, ppip->soundspawn );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
Uint8 __prthitawall( Uint16 particle )
{
    // ZZ> This function returns nonzero if the particle hit a wall

    Uint32 fan;
    Uint8  retval = MESHFX_IMPASS | MESHFX_WALL;

    fan = mesh_get_tile( PrtList[particle].xpos, PrtList[particle].ypos );
    if ( INVALID_TILE != fan )
    {
        if ( PipList[PrtList[particle].pip].bumpmoney )
        {
            retval = mesh.mem.tile_list[fan].fx & ( MESHFX_IMPASS | MESHFX_WALL );
        }
        else
        {
            retval = mesh.mem.tile_list[fan].fx & MESHFX_IMPASS;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void move_particles( void )
{
    // ZZ> This is the particle physics function
    int tnc;
    Uint16 cnt;
    Uint16 facing, pip, particle;
    float level;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        if ( !PrtList[cnt].on ) continue;

        PrtList[cnt].onwhichfan   = mesh_get_tile ( PrtList[cnt].xpos, PrtList[cnt].ypos );
        PrtList[cnt].onwhichblock = mesh_get_block( PrtList[cnt].xpos, PrtList[cnt].ypos );
        PrtList[cnt].floor_level  = get_level( PrtList[cnt].xpos, PrtList[cnt].ypos, bfalse );

        // To make it easier
        pip = PrtList[cnt].pip;

        // Animate particle
        PrtList[cnt].image = ( PrtList[cnt].image + PrtList[cnt].imageadd );
        if ( PrtList[cnt].image >= PrtList[cnt].imagemax )
            PrtList[cnt].image = 0;

        PrtList[cnt].rotate += PrtList[cnt].rotateadd;
        if ( ( (int)PrtList[cnt].size + (int)PrtList[cnt].sizeadd ) > (int)0x0000FFFF ) PrtList[cnt].size = 0xFFFF;
        else if ( ( PrtList[cnt].size + PrtList[cnt].sizeadd ) < 0 ) PrtList[cnt].size = 0;
        else PrtList[cnt].size += PrtList[cnt].sizeadd;

        // Change dyna light values
        PrtList[cnt].dynalightlevel += PipList[pip].dynalightleveladd;
        PrtList[cnt].dynalightfalloff += PipList[pip].dynalightfalloffadd;

        // Make it sit on the floor...  Shift is there to correct for sprite size
        level = PrtList[cnt].floor_level + ( PrtList[cnt].size >> 9 );

        // Check floor collision and do iterative physics
        if ( ( PrtList[cnt].zpos < level && PrtList[cnt].zvel < 0.1f ) || ( PrtList[cnt].zpos < level - PRTLEVELFIX ) )
        {
            PrtList[cnt].zpos = level;
            PrtList[cnt].xvel = PrtList[cnt].xvel * noslipfriction;
            PrtList[cnt].yvel = PrtList[cnt].yvel * noslipfriction;
            if ( PipList[pip].endground )  PrtList[cnt].time = 1;
            if ( PrtList[cnt].zvel < 0 )
            {
                if ( PrtList[cnt].zvel > -STOPBOUNCINGPART )
                {
                    // Make it not bounce
                    PrtList[cnt].zpos -= 0.0001f;
                }
                else
                {
                    // Make it bounce
                    PrtList[cnt].zvel = -PrtList[cnt].zvel * PipList[pip].dampen;
                    // Play the sound for hitting the floor [FSND]
                    play_particle_sound( cnt, PipList[pip].soundfloor );
                }
            }
        }
        else
        {
            if ( PrtList[cnt].attachedtocharacter == MAXCHR )
            {
                PrtList[cnt].xpos += PrtList[cnt].xvel;
                if ( __prthitawall( cnt ) )
                {
                    // Play the sound for hitting a wall [WSND]
                    play_particle_sound( cnt, PipList[pip].soundwall );
                    PrtList[cnt].xpos -= PrtList[cnt].xvel;
                    PrtList[cnt].xvel = ( -PrtList[cnt].xvel * PipList[pip].dampen );
                    if ( PipList[pip].endwall )
                    {
                        PrtList[cnt].time = 1;
                    }
                    else
                    {
                        // Change facing
                        facing = PrtList[cnt].facing;
                        if ( facing < 32768 )
                        {
                            facing -= NORTH;
                            facing = ~facing;
                            facing += NORTH;
                        }
                        else
                        {
                            facing -= SOUTH;
                            facing = ~facing;
                            facing += SOUTH;
                        }

                        PrtList[cnt].facing = facing;
                    }
                }

                PrtList[cnt].ypos += PrtList[cnt].yvel;
                if ( __prthitawall( cnt ) )
                {
                    PrtList[cnt].ypos -= PrtList[cnt].yvel;
                    PrtList[cnt].yvel = ( -PrtList[cnt].yvel * PipList[pip].dampen );
                    if ( PipList[pip].endwall )
                    {
                        PrtList[cnt].time = 1;
                    }
                    else
                    {
                        // Change facing
                        facing = PrtList[cnt].facing;
                        if ( facing < 16384 || facing > 49152 )
                        {
                            facing = ~facing;
                        }
                        else
                        {
                            facing -= EAST;
                            facing = ~facing;
                            facing += EAST;
                        }

                        PrtList[cnt].facing = facing;
                    }
                }

                PrtList[cnt].zpos += PrtList[cnt].zvel;
                PrtList[cnt].zvel += gravity;
            }
        }

        // Do homing
        if ( PipList[pip].homing && PrtList[cnt].target != MAXCHR )
        {
            if ( !ChrList[PrtList[cnt].target].alive )
            {
                PrtList[cnt].time = 1;
            }
            else
            {
                if ( PrtList[cnt].attachedtocharacter == MAXCHR )
                {
                    PrtList[cnt].xvel = ( PrtList[cnt].xvel + ( ( ChrList[PrtList[cnt].target].xpos - PrtList[cnt].xpos ) * PipList[pip].homingaccel ) ) * PipList[pip].homingfriction;
                    PrtList[cnt].yvel = ( PrtList[cnt].yvel + ( ( ChrList[PrtList[cnt].target].ypos - PrtList[cnt].ypos ) * PipList[pip].homingaccel ) ) * PipList[pip].homingfriction;
                    PrtList[cnt].zvel = ( PrtList[cnt].zvel + ( ( ChrList[PrtList[cnt].target].zpos + ( ChrList[PrtList[cnt].target].bumpheight >> 1 ) - PrtList[cnt].zpos ) * PipList[pip].homingaccel ) );

                }
                if ( PipList[pip].rotatetoface )
                {
                    // Turn to face target
                    facing = ATAN2( ChrList[PrtList[cnt].target].ypos - PrtList[cnt].ypos, ChrList[PrtList[cnt].target].xpos - PrtList[cnt].xpos ) * 0xFFFF / ( TWO_PI );
                    facing += 32768;
                    PrtList[cnt].facing = facing;
                }
            }
        }

        // Do speed limit on Z
        if ( PrtList[cnt].zvel < -PipList[pip].spdlimit )  PrtList[cnt].zvel = -PipList[pip].spdlimit;

        // Spawn new particles if continually spawning
        if ( PipList[pip].contspawnamount > 0 )
        {
            PrtList[cnt].spawntime--;
            if ( PrtList[cnt].spawntime == 0 )
            {
                PrtList[cnt].spawntime = PipList[pip].contspawntime;
                facing = PrtList[cnt].facing;
                tnc = 0;

                while ( tnc < PipList[pip].contspawnamount )
                {
                    particle = spawn_one_particle( PrtList[cnt].xpos, PrtList[cnt].ypos, PrtList[cnt].zpos,
                                                   facing, PrtList[cnt].model, PipList[pip].contspawnpip,
                                                   MAXCHR, GRIP_LAST, PrtList[cnt].team, PrtList[cnt].chr, tnc, PrtList[cnt].target );
                    if ( PipList[PrtList[cnt].pip].facingadd != 0 && particle != TOTALMAXPRT )
                    {
                        // Hack to fix velocity
                        PrtList[particle].xvel += PrtList[cnt].xvel;
                        PrtList[particle].yvel += PrtList[cnt].yvel;
                    }

                    facing += PipList[pip].contspawnfacingadd;
                    tnc++;
                }
            }
        }

        // Check underwater
        if ( PrtList[cnt].zpos < waterdouselevel && PipList[pip].endwater && INVALID_TILE != PrtList[cnt].onwhichfan && 0 != ( mesh.mem.tile_list[PrtList[cnt].onwhichfan].fx & MESHFX_WATER ) )
        {
            // Splash for particles is just a ripple
            spawn_one_particle( PrtList[cnt].xpos, PrtList[cnt].ypos, watersurfacelevel,
                                0, MAXMODEL, RIPPLE, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );

            // Check for disaffirming character
            if ( PrtList[cnt].attachedtocharacter != MAXCHR && PrtList[cnt].chr == PrtList[cnt].attachedtocharacter )
            {
                // Disaffirm the whole character
                disaffirm_attached_particles( PrtList[cnt].attachedtocharacter );
            }
            else
            {
                // Just destroy the particle
                //                    free_one_particle(cnt);
                PrtList[cnt].time = 1;
            }
        }

        //            else
        //            {
        // Spawn new particles if time for old one is up
        if ( PrtList[cnt].time != 0 )
        {
            PrtList[cnt].time--;
            if ( PrtList[cnt].time == 0 )
            {
                facing = PrtList[cnt].facing;
                tnc = 0;

                while ( tnc < PipList[pip].endspawnamount )
                {
                    spawn_one_particle( PrtList[cnt].xpos - PrtList[cnt].xvel, PrtList[cnt].ypos - PrtList[cnt].yvel, PrtList[cnt].zpos,
                                        facing, PrtList[cnt].model, PipList[pip].endspawnpip,
                                        MAXCHR, GRIP_LAST, PrtList[cnt].team, PrtList[cnt].chr, tnc, PrtList[cnt].target );
                    facing += PipList[pip].endspawnfacingadd;
                    tnc++;
                }

                free_one_particle( cnt );
            }
        }

        //            }
        PrtList[cnt].facing += PipList[pip].facingadd;
    }
}

//--------------------------------------------------------------------------------------------
void free_all_particles()
{
    // ZZ> This function resets the particle allocation lists
    numfreeprt = 0;

    while ( numfreeprt < maxparticles )
    {
        PrtList[numfreeprt].on = 0;
        freeprtlist[numfreeprt] = numfreeprt;
        numfreeprt++;
    }
}

//--------------------------------------------------------------------------------------------
void setup_particles()
{
    // ZZ> This function sets up particle data
    int cnt;
    double x, y;

    particletexture = 0;

    // Image coordinates on the big particle bitmap
    for ( cnt = 0; cnt < MAXPARTICLEIMAGE; cnt++ )
    {
        x = cnt & 15;
        y = cnt >> 4;
        particleimageu[cnt][0] = ( float )( ( 0.05f + x ) / 16.0f );
        particleimageu[cnt][1] = ( float )( ( 0.95f + x ) / 16.0f );
        particleimagev[cnt][0] = ( float )( ( 0.05f + y ) / 16.0f );
        particleimagev[cnt][1] = ( float )( ( 0.95f + y ) / 16.0f );
    }

    // Reset the allocation table
    free_all_particles();
}

//--------------------------------------------------------------------------------------------
void spawn_bump_particles( Uint16 character, Uint16 particle )
{
    // ZZ> This function is for catching characters on fire and such
    int cnt;
    Sint16 x, y, z;
    int distance, bestdistance;
    Uint16 frame;
    Uint16 facing, bestvertex;
    Uint16 amount;
    Uint16 vertices;
    Uint16 direction, left, right, model;
    float fsin, fcos;
    pip_t * ppip;
    chr_t * pchr;
    mad_t * pmad;
    prt_t * pprt;
    cap_t * pcap;

    if( INVALID_PRT(particle) ) return;
    pprt = PrtList + particle;

    if( INVALID_PIP(pprt->pip) ) return;
    ppip = PipList + pprt->pip;

    // no point in going on, is there?
    if( 0 == ppip->bumpspawnamount && !ppip->spawnenchant ) return;
    amount = ppip->bumpspawnamount;

    if( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    model = pchr->inst.imad;
    if( model > MAXMODEL || !MadList[model].used ) return;
    pmad = MadList + model;

    model = pchr->model;
    if( INVALID_CAP( model ) ) return;
    pcap = CapList + model;

    // Only damage if hitting from proper direction
    vertices = pmad->vertices;
    direction = ( ATAN2( pprt->yvel, pprt->xvel ) + PI ) * 0xFFFF / TWO_PI;
    direction = pchr->turnleftright - direction + 32768;
    if ( Md2FrameList[pchr->inst.frame].framefx&MADFXINVICTUS )
    {
        // I Frame
        if ( ppip->damfx&DAMFX_BLOC )
        {
            left = 0xFFFF;
            right = 0;
        }
        else
        {
            direction -= pcap->iframefacing;
            left = 0xFFFF - pcap->iframeangle;
            right = pcap->iframeangle;
        }
    }
    else
    {
        // N Frame
        direction -= pcap->nframefacing;
        left = 0xFFFF - pcap->nframeangle;
        right = pcap->nframeangle;
    }

    // Check that direction
    if ( direction <= left && direction >= right )
    {
        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_enchant( pprt->chr, character, MAXCHR, MAXENCHANT, pprt->model );
        }

        // Spawn particles
        if ( amount != 0 && !pcap->resistbumpspawn && !pchr->invictus && vertices != 0 && ( pchr->damagemodifier[pprt->damagetype]&DAMAGESHIFT ) < 3 )
        {
            if ( amount == 1 )
            {
                // A single particle ( arrow? ) has been stuck in the character...
                // Find best vertex to attach to
                bestvertex = 0;
                bestdistance = 9999999;
                z = -pchr->zpos + pprt->zpos + RAISE;
                facing = pprt->facing - pchr->turnleftright - 16384;
                facing = facing >> 2;
                fsin = turntosin[facing];
                fcos = turntocos[facing];
                y = 8192;
                x = -y * fsin;
                y = y * fcos;
                z = z << 10;/// pchr->scale;
                frame = pmad->framestart;
                cnt = 0;

                while ( cnt < vertices )
                {
                    distance = ABS( x - Md2FrameList[frame].vrtx[vertices-cnt-1] ) + ABS( y - Md2FrameList[frame].vrty[vertices-cnt-1] ) + ( ABS( z - Md2FrameList[frame].vrtz[vertices-cnt-1] ) );
                    if ( distance < bestdistance )
                    {
                        bestdistance = distance;
                        bestvertex = cnt;
                    }

                    cnt++;
                }

                spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, 0, pprt->model, ppip->bumpspawnpip,
                    character, bestvertex + 1, pprt->team, pprt->chr, cnt, character );
            }
            else
            {
                amount = ( amount * vertices ) >> 5;  // Correct amount for size of character
                cnt = 0;

                while ( cnt < amount )
                {
                    spawn_one_particle( pchr->xpos, pchr->ypos, pchr->zpos, 0, pprt->model, ppip->bumpspawnpip,
                        character, rand() % vertices, pprt->team, pprt->chr, cnt, character );
                    cnt++;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
int prt_is_over_water( Uint16 cnt )
{
    // This function returns btrue if the particle is over a water tile
    int fan;

    if ( cnt < maxparticles )
    {
        fan = mesh_get_tile( PrtList[cnt].xpos, PrtList[cnt].ypos );
        if ( INVALID_TILE != fan )
        {
            if ( 0 != ( mesh.mem.tile_list[fan].fx & MESHFX_WATER ) )  return btrue;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
int load_one_particle_profile( const char *szLoadName )
{
    // ZZ> This function loads a particle template, returning bfalse if the file wasn't
    //     found
    FILE* fileread;
    IDSZ idsz;
    int iTmp;
    float fTmp;
    char cTmp;
    pip_t * ppip;

    Uint16 retval = TOTALMAXPRTPIP;

    if( numpip >= TOTALMAXPRTPIP ) return TOTALMAXPRTPIP;
    ppip = PipList + numpip;

    // clear the pip
    memset( ppip, 0, sizeof(pip_t) );

    fileread = fopen( szLoadName, "r" );
    if ( NULL == fileread )
    {
        return TOTALMAXPRTPIP;
    }

    retval = numpip;
    numpip++;

    strncpy( ppip->name, szLoadName, SDL_arraysize(ppip->name) );
    ppip->loaded = btrue;

    // General data
    parse_filename = szLoadName;    //For debugging missing colons
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->force = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  ppip->force = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'L' || cTmp == 'l' )  ppip->type = PRTLIGHTSPRITE;
    else if ( cTmp == 'S' || cTmp == 's' )  ppip->type = PRTSOLIDSPRITE;
    else if ( cTmp == 'T' || cTmp == 't' )  ppip->type = PRTALPHASPRITE;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->imagebase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->numframes = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->imageadd = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->imageaddrand = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->rotatebase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->rotaterand = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->rotateadd = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->sizebase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->sizeadd = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); ppip->spdlimit = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->facingadd = iTmp;

    // override the base rotation
    if( 0xFFFF != prt_direction[ ppip->imagebase ] )
    {
        ppip->rotatebase = prt_direction[ ppip->imagebase ];
    };

    // Ending conditions
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->endwater = btrue;
    if ( cTmp == 'F' || cTmp == 'f' )  ppip->endwater = bfalse;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->endbump = btrue;
    if ( cTmp == 'F' || cTmp == 'f' )  ppip->endbump = bfalse;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->endground = btrue;
    if ( cTmp == 'F' || cTmp == 'f' )  ppip->endground = bfalse;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->endlastframe = btrue;
    if ( cTmp == 'F' || cTmp == 'f' )  ppip->endlastframe = bfalse;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->time = iTmp;

    // Collision data
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); ppip->dampen = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->bumpmoney = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->bumpsize = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->bumpheight = iTmp;
    goto_colon( fileread );  read_pair( fileread );
    ppip->damagebase = pairbase;
    ppip->damagerand = pairrand;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    if ( cTmp == 'S' || cTmp == 's' ) ppip->damagetype = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' ) ppip->damagetype = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' ) ppip->damagetype = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' ) ppip->damagetype = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' ) ppip->damagetype = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' ) ppip->damagetype = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' ) ppip->damagetype = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' ) ppip->damagetype = DAMAGE_ZAP;

    // Lighting data
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->dynalightmode = DYNAOFF;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->dynalightmode = DYNAON;
    if ( cTmp == 'L' || cTmp == 'l' ) ppip->dynalightmode = DYNALOCAL;

    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); ppip->dynalevel = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->dynafalloff = iTmp;
    if ( ppip->dynafalloff > MAXFALLOFF && rtscontrol )  ppip->dynafalloff = MAXFALLOFF;

    // Initial spawning of this particle
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->facingbase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->facingrand = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->xyspacingbase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->xyspacingrand = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->zspacingbase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->zspacingrand = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->xyvelbase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->xyvelrand = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->zvelbase = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->zvelrand = iTmp;

    // Continuous spawning of other particles
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->contspawntime = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->contspawnamount = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->contspawnfacingadd = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->contspawnpip = iTmp;

    // End spawning of other particles
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->endspawnamount = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->endspawnfacingadd = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->endspawnpip = iTmp;

    // Bump spawning of attached particles
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->bumpspawnamount = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->bumpspawnpip = iTmp;

    // Random stuff  !!!BAD!!! Not complete
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->dazetime = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->grogtime = iTmp;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->spawnenchant = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->spawnenchant = btrue;

    goto_colon( fileread );  // !!Cause roll
    goto_colon( fileread );  // !!Cause pancake
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->needtarget = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->needtarget = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->targetcaster = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->targetcaster = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->startontarget = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->startontarget = btrue;

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->onlydamagefriendly = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->onlydamagefriendly = btrue;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    ppip->soundspawn = CLIP(iTmp, -1, MAXWAVE);

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    ppip->soundend = CLIP(iTmp, -1, MAXWAVE);

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->friendlyfire = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->friendlyfire = btrue;

    goto_colon( fileread );
    //ppip->hateonly = bfalse; TODO: BAD not implemented yet

    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->newtargetonspawn = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->newtargetonspawn = btrue;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); ppip->targetangle = iTmp >> 1;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->homing = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->homing = btrue;

    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); ppip->homingfriction = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); ppip->homingaccel = fTmp;
    goto_colon( fileread );  cTmp = fget_first_letter( fileread );
    ppip->rotatetoface = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) ppip->rotatetoface = btrue;

    // Clear expansions...
    ppip->zaimspd = 0;
    ppip->soundfloor = -1;
    ppip->soundwall = -1;
    ppip->endwall = ppip->endground;
    ppip->damfx = DAMFX_TURN;
    if ( ppip->homing )  ppip->damfx = DAMFX_NONE;

    ppip->allowpush = btrue;
    ppip->dynalightfalloffadd = 0;
    ppip->dynalightleveladd = 0;
    ppip->intdamagebonus = bfalse;
    ppip->wisdamagebonus = bfalse;

    // Read expansions
    while ( goto_colon_yesno( fileread ) )
    {
        idsz = fget_idsz( fileread );

             if ( idsz == Make_IDSZ( "TURN" ) )  ppip->damfx = DAMFX_NONE;
        else if ( idsz == Make_IDSZ( "ARMO" ) )  ppip->damfx |= DAMFX_ARMO;
        else if ( idsz == Make_IDSZ( "BLOC" ) )  ppip->damfx |= DAMFX_BLOC;
        else if ( idsz == Make_IDSZ( "ARRO" ) )  ppip->damfx |= DAMFX_ARRO;
        else if ( idsz == Make_IDSZ( "TIME" ) )  ppip->damfx |= DAMFX_TIME;
        else if ( idsz == Make_IDSZ( "ZSPD" ) )  ppip->zaimspd = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "FSND" ) )  ppip->soundfloor = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "WSND" ) )  ppip->soundwall = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "WEND" ) )  ppip->endwall = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "PUSH" ) )  ppip->allowpush = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "DLEV" ) )  ppip->dynalightleveladd = fget_int( fileread ) / 1000.0f;
        else if ( idsz == Make_IDSZ( "DRAD" ) )  ppip->dynalightfalloffadd = fget_int( fileread ) / 1000.0f;
        else if ( idsz == Make_IDSZ( "IDAM" ) )  ppip->intdamagebonus = fget_int( fileread );
        else if ( idsz == Make_IDSZ( "WDAM" ) )  ppip->wisdamagebonus = fget_int( fileread );
    }

    fclose( fileread );

    return retval;
}

//--------------------------------------------------------------------------------------------
void reset_particles( const char* modname )
{
    // ZZ> This resets all particle data and reads in the coin and water particles
    int cnt, object;
    STRING newloadname;
    char *loadpath;

    // Load in the standard global particles ( the coins for example )
    numpip = 0;
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "1money.txt";
    if ( TOTALMAXPRTPIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "5money.txt";
    if ( TOTALMAXPRTPIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "25money.txt";
    if ( TOTALMAXPRTPIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "100money.txt";
    if ( TOTALMAXPRTPIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    // Load module specific information
    make_newloadname( modname, "gamedat" SLASH_STR "weather4.txt", newloadname );
    if ( TOTALMAXPRTPIP == load_one_particle_profile( newloadname ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "weather5.txt", newloadname );
    if ( TOTALMAXPRTPIP == load_one_particle_profile( newloadname ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "splash.txt", newloadname );
    if ( TOTALMAXPRTPIP == load_one_particle_profile( newloadname ) )
    {
        if (gDevMode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "splash.txt";
        if ( TOTALMAXPRTPIP == load_one_particle_profile( loadpath ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    make_newloadname( modname, "gamedat" SLASH_STR "ripple.txt", newloadname );
    if ( TOTALMAXPRTPIP == load_one_particle_profile( newloadname ) )
    {
        if (gDevMode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "ripple.txt";
        if ( TOTALMAXPRTPIP == load_one_particle_profile( loadpath ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    // This is also global...
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.txt";
    if ( TOTALMAXPRTPIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    // Now clear out the local pips
    object = 0;
    while ( object < MAXMODEL )
    {
        cnt = 0;

        while ( cnt < MAXPRTPIPPEROBJECT )
        {
            MadList[object].prtpip[cnt] = 0;
            cnt++;
        }

        object++;
    }
}