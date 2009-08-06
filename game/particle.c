// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

/* Egoboo - particle.c
* Manages particle systems.
*/

#include "particle.h"

#include "log.h"
#include "sound.h"
#include "camera.h"
#include "enchant.h"
#include "char.h"
#include "mad.h"
#include "mesh.h"
#include "game.h"

#include "egoboo_setup.h"
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
static Uint16    freeprtlist[TOTAL_MAX_PRT];

int              numpip   = 0;
pip_t            PipList[MAX_PIP];

float            sprite_list_u[MAXPARTICLEIMAGE][2];        // Texture coordinates
float            sprite_list_v[MAXPARTICLEIMAGE][2];

Uint16           maxparticles = 512;                            // max number of particles
prt_t            PrtList[TOTAL_MAX_PRT];

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
void free_one_particle( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack

    if ( VALID_PRT_RANGE(particle) )
    {
        // particle "destructor"
        // sets all boolean values to false, incluting the "on" flag
        memset( PrtList + particle, 0, sizeof(prt_t) );

        // push it on the stack
        if ( numfreeprt < TOTAL_MAX_PRT )
        {
            freeprtlist[numfreeprt] = particle;
            numfreeprt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( Uint16 particle, Sint8 sound )
{
    // This function plays a sound effect for a particle

    prt_t * pprt;

    if ( !VALID_PRT(particle) ) return;
    pprt = PrtList + particle;

    if ( sound >= 0 && sound < MAX_WAVE )
    {
        if ( VALID_CAP( pprt->model ) )
        {
            cap_t * pcap = CapList + pprt->model;
            sound_play_chunk( pprt->pos, pcap->wavelist[sound] );
        }
        else
        {
            sound_play_chunk( pprt->pos, g_wavelist[sound] );
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_in_game( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack and
    //    plays the sound associated with the particle

    if ( VALID_PRT( particle) )
    {
        Uint16 child;
        prt_t * pprt = PrtList + particle;

        if ( pprt->spawncharacterstate != SPAWNNOCHARACTER )
        {
            child = spawn_one_character( pprt->pos, pprt->model, pprt->team, 0, pprt->facing, NULL, MAX_CHR );
            if ( VALID_CHR(child) )
            {
                ChrList[child].ai.state = pprt->spawncharacterstate;
                ChrList[child].ai.owner = pprt->chr;
            }
        }

        if ( VALID_PIP(pprt->pip) )
        {
            play_particle_sound( particle, PipList[pprt->pip].soundend );
        }

    }

    free_one_particle( particle );
}

//--------------------------------------------------------------------------------------------
int get_free_particle( int force )
{
    // ZZ> This function gets an unused particle.  If all particles are in use
    //    and force is set, it grabs the first unimportant one.  The particle
    //    index is the return value
    int particle;

    // Return maxparticles if we can't find one
    particle = TOTAL_MAX_PRT;
    if ( numfreeprt == 0 )
    {
        if ( force )
        {
            // Gotta find one, so go through the list and replace a unimportant one
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

    return (particle >= maxparticles) ? TOTAL_MAX_PRT : particle;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 ipip,
                           Uint16 characterattach, Uint16 vrt_offset, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget )
{
    // ZZ> This function spawns a new particle, and returns the number of that particle
    int iprt, velocity;
    GLvector3 vel;
    float tvel;
    int offsetfacing = 0, newrand;
    prt_t * pprt;
    pip_t * ppip;
    Uint32 prt_lifetime;

    // Convert from local ipip to global ipip
    if ( ipip < MAX_PIP_PER_PROFILE && model < MAX_PROFILE && MadList[model].loaded )
    {
        ipip = MadList[model].prtpip[ipip];
    }
    if ( INVALID_PIP(ipip) ) return TOTAL_MAX_PRT;
    ppip = PipList + ipip;

    iprt = get_free_particle( ppip->force );
    if ( !VALID_PRT_RANGE(iprt) ) return TOTAL_MAX_PRT;
    pprt = PrtList + iprt;

    // clear out all data
    memset( pprt, 0, sizeof(prt_t));

    // Necessary data for any part
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

    // Set character attachments ( characterattach==MAX_CHR means none )
    pprt->attachedtocharacter = characterattach;
    pprt->vrt_off = vrt_offset;

    // Correct facing
    facing += ppip->facingbase;

    // Targeting...
    vel.z = 0;
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
            if ( pprt->target != MAX_CHR && !ppip->homing )
            {
                facing -= glouseangle;
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
            if ( pprt->target != MAX_CHR && ppip->zaimspd != 0 )
            {
                // These aren't velocities...  This is to do aiming on the Z axis
                if ( velocity > 0 )
                {
                    vel.x = ChrList[pprt->target].pos.x - x;
                    vel.y = ChrList[pprt->target].pos.y - y;
                    tvel = SQRT( vel.x * vel.x + vel.y * vel.y ) / velocity;  // This is the number of steps...
                    if ( tvel > 0 )
                    {
                        vel.z = ( ChrList[pprt->target].pos.z + ( ChrList[pprt->target].bumpsize >> 1 ) - z ) / tvel;  // This is the vel.z alteration
                        if ( vel.z < -( ppip->zaimspd >> 1 ) ) vel.z = -( ppip->zaimspd >> 1 );
                        if ( vel.z > ppip->zaimspd ) vel.z = ppip->zaimspd;
                    }
                }
            }
        }

        // Does it go away?
        if ( pprt->target == MAX_CHR && ppip->needtarget )
        {
            free_one_particle_in_game( iprt );
            return maxparticles;
        }

        // Start on top of target
        if ( pprt->target != MAX_CHR && ppip->startontarget )
        {
            x = ChrList[pprt->target].pos.x;
            y = ChrList[pprt->target].pos.y;
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
    x += turntocos[( facing+8192 ) & TRIG_TABLE_MASK] * ( ppip->xyspacingbase + ( newrand & ppip->xyspacingrand ) );
    y += turntosin[( facing+8192 ) & TRIG_TABLE_MASK] * ( ppip->xyspacingbase + ( newrand & ppip->xyspacingrand ) );
    x = CLIP(x, 0, PMesh->info.edge_x - 2);
    y = CLIP(y, 0, PMesh->info.edge_y - 2);

    pprt->pos.x = x;
    pprt->pos.y = y;
    pprt->pos.z = z;

    // Velocity data
    vel.x = turntocos[( facing+8192 ) & TRIG_TABLE_MASK] * velocity;
    vel.y = turntosin[( facing+8192 ) & TRIG_TABLE_MASK] * velocity;
    newrand = RANDIE;
    vel.z += ppip->zvelbase + ( newrand & ppip->zvelrand ) - ( ppip->zvelrand >> 1 );
    pprt->vel = vel;

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
    prt_lifetime = ppip->time;
    if ( ppip->endlastframe && pprt->imageadd != 0 )
    {
        if ( ppip->time == 0 )
        {
            // Part time is set to 1 cycle
            int frames = ( pprt->imagemax / pprt->imageadd ) - 1;
            prt_lifetime = frames;
        }
        else
        {
            // Part time is used to give number of cycles
            int frames = ( ( pprt->imagemax / pprt->imageadd ) - 1 );
            prt_lifetime = ppip->time * frames;
        }
    }

    // "no lifetime" = "eternal"
    pprt->is_eternal = bfalse;
    pprt->time      = (Uint32)~0;
    if ( 0 == prt_lifetime )
    {
        pprt->is_eternal = btrue;
    }
    else
    {
        pprt->time = frame_all + prt_lifetime;
    }

    // Set onwhichfan...
    pprt->onwhichfan   = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
    pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

    // Damage stuff
    pprt->damagebase = ppip->damagebase;
    pprt->damagerand = ppip->damagerand;

    // Spawning data
    pprt->spawntime = ppip->contspawntime;
    if ( pprt->spawntime != 0 )
    {
        pprt->spawntime = 1;
        if ( pprt->attachedtocharacter != MAX_CHR )
        {
            pprt->spawntime++; // Because attachment takes an update before it happens
        }
    }

    // Sound effect
    play_particle_sound( iprt, ppip->soundspawn );

    return iprt;
}

//--------------------------------------------------------------------------------------------
Uint8 __prthitawall( Uint16 particle )
{
    // ZZ> This function returns nonzero if the particle hit a wall

    Uint32 fan;
    Uint8  retval = MPDFX_IMPASS | MPDFX_WALL;

    fan = mesh_get_tile( PMesh, PrtList[particle].pos.x, PrtList[particle].pos.y );
    if ( VALID_TILE(PMesh, fan) )
    {
        if ( PipList[PrtList[particle].pip].bumpmoney )
        {
            retval = mesh_test_fx(PMesh, fan, MPDFX_IMPASS | MPDFX_WALL );
        }
        else
        {
            retval = mesh_test_fx(PMesh, fan, MPDFX_IMPASS);
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
    Uint16 facing, ipip, particle;
    float level;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        pip_t * ppip;
        prt_t * pprt;

        if ( !PrtList[cnt].on ) continue;
        pprt = PrtList + cnt;

        if ( pprt->is_hidden ) continue;

        pprt->onwhichfan   = mesh_get_tile ( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->floor_level  = mesh_get_level( PMesh, pprt->pos.x, pprt->pos.y );

        // To make it easier
        ipip = pprt->pip;
        if ( INVALID_PIP( ipip ) ) continue;
        ppip = PipList + ipip;

        // Animate particle
        pprt->image = pprt->image + pprt->imageadd;
        if ( pprt->image >= pprt->imagemax ) pprt->image = 0;

        pprt->rotate += pprt->rotateadd;
        if ( ( (int)pprt->size + (int)pprt->sizeadd ) > (int)0x0000FFFF ) pprt->size = 0xFFFF;
        else if ( ( pprt->size + pprt->sizeadd ) < 0 ) pprt->size = 0;
        else pprt->size += pprt->sizeadd;

        // Change dyna light values
        pprt->dynalightlevel += ppip->dynalightleveladd;
        pprt->dynalightfalloff += ppip->dynalightfalloffadd;

        // Make it sit on the floor...  Shift is there to correct for sprite size
        level = pprt->floor_level + (FP8_TO_INT( pprt->size ) >> 1);

        // Check floor collision and do iterative physics
        if ( ( pprt->pos.z < level && pprt->vel.z < 0.1f ) || ( pprt->pos.z < level - PRTLEVELFIX ) )
        {
            pprt->pos.z = level;
            pprt->vel.x = pprt->vel.x * noslipfriction;
            pprt->vel.y = pprt->vel.y * noslipfriction;
            if ( ppip->endground )
            {
                pprt->time  = frame_all + 1;
                pprt->poofme = btrue;
            }
            if ( pprt->vel.z < 0 )
            {
                if ( pprt->vel.z > -STOPBOUNCINGPART )
                {
                    // Make it not bounce
                    pprt->pos.z -= 0.0001f;
                }
                else
                {
                    // Make it bounce
                    pprt->vel.z = -pprt->vel.z * ppip->dampen;
                    // Play the sound for hitting the floor [FSND]
                    play_particle_sound( cnt, ppip->soundfloor );
                }
            }
        }
        else if ( INVALID_CHR( pprt->attachedtocharacter ) )
        {
            pprt->pos.x += pprt->vel.x;
            if ( __prthitawall( cnt ) )
            {
                // Play the sound for hitting a wall [WSND]
                play_particle_sound( cnt, ppip->soundwall );
                pprt->pos.x -= pprt->vel.x;
                pprt->vel.x = ( -pprt->vel.x * ppip->dampen );
                if ( ppip->endwall )
                {
                    pprt->time  = frame_all + 1;
                    pprt->poofme = btrue;
                }
                else
                {
                    // Change facing
                    facing = pprt->facing;
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

                    pprt->facing = facing;
                }
            }

            pprt->pos.y += pprt->vel.y;
            if ( __prthitawall( cnt ) )
            {
                pprt->pos.y -= pprt->vel.y;
                pprt->vel.y = ( -pprt->vel.y * ppip->dampen );
                if ( ppip->endwall )
                {
                    pprt->time  = frame_all + 1;
                    pprt->poofme = btrue;
                }
                else
                {
                    // Change facing
                    facing = pprt->facing;
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

                    pprt->facing = facing;
                }
            }

            pprt->pos.z += pprt->vel.z;
            pprt->vel.z += gravity;
        }

        // Do homing
        if ( ppip->homing && VALID_CHR( pprt->target ) )
        {
            if ( !ChrList[pprt->target].alive )
            {
                pprt->time  = frame_all + 1;
                pprt->poofme = btrue;
            }
            else
            {
                if ( INVALID_CHR( pprt->attachedtocharacter ) )
                {
                    pprt->vel.x = ( pprt->vel.x + ( ( ChrList[pprt->target].pos.x - pprt->pos.x ) * ppip->homingaccel ) ) * ppip->homingfriction;
                    pprt->vel.y = ( pprt->vel.y + ( ( ChrList[pprt->target].pos.y - pprt->pos.y ) * ppip->homingaccel ) ) * ppip->homingfriction;
                    pprt->vel.z = ( pprt->vel.z + ( ( ChrList[pprt->target].pos.z + ( ChrList[pprt->target].bumpheight >> 1 ) - pprt->pos.z ) * ppip->homingaccel ) );

                }
                if ( ppip->rotatetoface )
                {
                    // Turn to face target
                    facing = ATAN2( ChrList[pprt->target].pos.y - pprt->pos.y, ChrList[pprt->target].pos.x - pprt->pos.x ) * 0xFFFF / ( TWO_PI );
                    facing += 32768;
                    pprt->facing = facing;
                }
            }
        }

        // Do speed limit on Z
        if ( pprt->vel.z < -ppip->spdlimit )  pprt->vel.z = -ppip->spdlimit;

        // Spawn new particles if continually spawning
        if ( ppip->contspawnamount > 0 )
        {
            pprt->spawntime--;
            if ( pprt->spawntime == 0 )
            {
                pprt->spawntime = ppip->contspawntime;
                facing = pprt->facing;
                tnc = 0;

                while ( tnc < ppip->contspawnamount )
                {
                    particle = spawn_one_particle( pprt->pos.x, pprt->pos.y, pprt->pos.z,
                                                   facing, pprt->model, ppip->contspawnpip,
                                                   MAX_CHR, GRIP_LAST, pprt->team, pprt->chr, tnc, pprt->target );
                    if ( PipList[pprt->pip].facingadd != 0 && particle != TOTAL_MAX_PRT )
                    {
                        // Hack to fix velocity
                        PrtList[particle].vel.x += pprt->vel.x;
                        PrtList[particle].vel.y += pprt->vel.y;
                    }

                    facing += ppip->contspawnfacingadd;
                    tnc++;
                }
            }
        }

        // Check underwater
        if ( pprt->pos.z < water.douse_level && ppip->endwater && VALID_TILE(PMesh, pprt->onwhichfan) && (0 != mesh_test_fx( PMesh, pprt->onwhichfan, MPDFX_WATER )) )
        {
            // Splash for particles is just a ripple
            spawn_one_particle( pprt->pos.x, pprt->pos.y, water.surface_level,
                                0, MAX_PROFILE, RIPPLE, MAX_CHR, GRIP_LAST, TEAM_NULL, MAX_CHR, 0, MAX_CHR );

            // Check for disaffirming character
            if ( VALID_CHR( pprt->attachedtocharacter ) && pprt->chr == pprt->attachedtocharacter )
            {
                // Disaffirm the whole character
                disaffirm_attached_particles( pprt->attachedtocharacter );
            }
            else
            {
                // Just destroy the particle
                //                   free_one_particle_in_game(cnt);
                pprt->time  = frame_all + 1;
                pprt->poofme = btrue;
            }
        }

        // Spawn new particles if time for old one is up
        if ( pprt->poofme || ( !pprt->is_eternal && frame_all >= pprt->time ) )
        {
            facing = pprt->facing;

            for ( tnc = 0; tnc < ppip->endspawnamount; tnc++ )
            {
                spawn_one_particle( pprt->pos.x - pprt->vel.x, pprt->pos.y - pprt->vel.y, pprt->pos.z,
                                    facing, pprt->model, ppip->endspawnpip,
                                    MAX_CHR, GRIP_LAST, pprt->team, pprt->chr, tnc, pprt->target );
                facing += ppip->endspawnfacingadd;
                tnc++;
            }

            free_one_particle_in_game( cnt );
        }

        pprt->facing += ppip->facingadd;
    }
}

//--------------------------------------------------------------------------------------------
void free_all_particles()
{
    // ZZ> This function resets the particle allocation lists

    int cnt;

    // free all the particles
    numfreeprt = 0;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        // reuse this code
        free_one_particle( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void setup_particles()
{
    // ZZ> This function sets up particle data
    int cnt;
    double x, y;

    // Image coordinates on the big particle bitmap
    for ( cnt = 0; cnt < MAXPARTICLEIMAGE; cnt++ )
    {
        x = cnt & 15;
        y = cnt >> 4;
        sprite_list_u[cnt][0] = ( float )( ( 0.05f + x ) / 16.0f );
        sprite_list_u[cnt][1] = ( float )( ( 0.95f + x ) / 16.0f );
        sprite_list_v[cnt][0] = ( float )( ( 0.05f + y ) / 16.0f );
        sprite_list_v[cnt][1] = ( float )( ( 0.95f + y ) / 16.0f );
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
    Uint32 distance, bestdistance;
    Uint16 frame;
    Uint16 facing, bestvertex;
    Uint16 amount;
    Uint16 vertices;
    Uint16 direction, model;
    float fsin, fcos;
    pip_t * ppip;
    chr_t * pchr;
    mad_t * pmad;
    prt_t * pprt;
    cap_t * pcap;

    if ( INVALID_PRT(particle) ) return;
    pprt = PrtList + particle;

    if ( INVALID_PIP(pprt->pip) ) return;
    ppip = PipList + pprt->pip;

    // no point in going on, is there?
    if ( 0 == ppip->bumpspawnamount && !ppip->spawnenchant ) return;
    amount = ppip->bumpspawnamount;

    if ( INVALID_CHR(character) ) return;
    pchr = ChrList + character;

    model = pchr->inst.imad;
    if ( model > MAX_PROFILE || !MadList[model].loaded ) return;
    pmad = MadList + model;

    model = pchr->model;
    if ( INVALID_CAP( model ) ) return;
    pcap = CapList + model;

    // Only damage if hitting from proper direction
    direction = ( ATAN2( pprt->vel.y, pprt->vel.x ) + PI ) * 0xFFFF / TWO_PI;
    direction = pchr->turn_z - direction + 32768;

    // Check that direction
    if ( !is_invictus_direction( direction, character, ppip->damfx) )
    {
        vertices = pmad->md2.vertices;

        // Spawn new enchantments
        if ( ppip->spawnenchant )
        {
            spawn_enchant( pprt->chr, character, MAX_CHR, MAX_ENC, pprt->model );
        }

        // Spawn particles
        if ( amount != 0 && !pcap->resistbumpspawn && !pchr->invictus && vertices != 0 && ( pchr->damagemodifier[pprt->damagetype]&DAMAGESHIFT ) < 3 )
        {
            
			if ( amount == 1 )
            {
                // A single particle ( arrow? ) has been stuck in the character...
                // Find best vertex to attach to

                bestvertex = 0;
                bestdistance = 1 << 31;			//Really high number

                z = -pchr->pos.z + pprt->pos.z + RAISE;
                facing = pprt->facing - pchr->turn_z - NORTH;
                facing = facing >> 2;
                fsin = turntosin[facing & TRIG_TABLE_MASK ];
                fcos = turntocos[facing & TRIG_TABLE_MASK ];
                x = -8192 * fsin;
                y =  8192 * fcos;
                z = z << 10;// / pchr->scale;
                frame = pmad->md2.framestart;

                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    distance = ABS( x - Md2FrameList[frame].vrtx[vertices-cnt-1] ) + ABS( y - Md2FrameList[frame].vrty[vertices-cnt-1] ) + ( ABS( z - Md2FrameList[frame].vrtz[vertices-cnt-1] ) );
                    if ( distance < bestdistance )
                    {
                        bestdistance = distance;
                        bestvertex = cnt;
                    }
                }

                spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z, 0, pprt->model, ppip->bumpspawnpip,
                                    character, bestvertex + 1, pprt->team, pprt->chr, cnt, character );
            }

            else
            {
				//Multiple particles are attached to character

                amount = ( amount * vertices ) >> 5;  // Correct amount for size of character

                for ( cnt = 0; cnt < amount; cnt++ )
                {
                    spawn_one_particle( pchr->pos.x, pchr->pos.y, pchr->pos.z, 0, pprt->model, ppip->bumpspawnpip,
                                        character, rand() % vertices, pprt->team, pprt->chr, cnt, character );
                }
            }

        }
    }
}

//--------------------------------------------------------------------------------------------
int prt_is_over_water( Uint16 cnt )
{
    // This function returns btrue if the particle is over a water tile
    Uint32 fan;

    if ( INVALID_PRT(cnt) ) return bfalse;

    fan = mesh_get_tile( PMesh, PrtList[cnt].pos.x, PrtList[cnt].pos.y );
    if ( VALID_TILE(PMesh, fan) )
    {
        if ( 0 != mesh_test_fx( PMesh, fan, MPDFX_WATER ) )  return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
int load_one_particle_profile( const char *szLoadName )
{
    // ZZ> This function loads a particle template, returning bfalse if the file wasn't
    //    found
    FILE* fileread;
    IDSZ idsz;
    int iTmp;
    float fTmp;
    char cTmp;
    pip_t * ppip;

    Uint16 retval = MAX_PIP;

    if ( numpip >= MAX_PIP ) return MAX_PIP;
    ppip = PipList + numpip;

    // clear the pip
    memset( ppip, 0, sizeof(pip_t) );

    fileread = fopen( szLoadName, "r" );
    if ( NULL == fileread )
    {
        return MAX_PIP;
    }

    retval = numpip;
    numpip++;

    strncpy( ppip->name, szLoadName, SDL_arraysize(ppip->name) );
    ppip->loaded = btrue;

    // General data
    parse_filename = szLoadName;    // For debugging missing colons
    cTmp = fget_next_char( fileread );
    ppip->force = bfalse;
    if ( 'T' == toupper(cTmp) )  ppip->force = btrue;

    cTmp = fget_next_char( fileread );
    if ( 'L' == toupper(cTmp) )  ppip->type = PRTLIGHTSPRITE;
    else if ( 'S' == toupper(cTmp) )  ppip->type = PRTSOLIDSPRITE;
    else if ( 'T' == toupper(cTmp) )  ppip->type = PRTALPHASPRITE;

    iTmp = fget_next_int( fileread ); ppip->imagebase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->numframes = iTmp;
    iTmp = fget_next_int( fileread ); ppip->imageadd = iTmp;
    iTmp = fget_next_int( fileread ); ppip->imageaddrand = iTmp;
    iTmp = fget_next_int( fileread ); ppip->rotatebase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->rotaterand = iTmp;
    iTmp = fget_next_int( fileread ); ppip->rotateadd = iTmp;
    iTmp = fget_next_int( fileread ); ppip->sizebase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->sizeadd = iTmp;
    fTmp = fget_next_float( fileread ); ppip->spdlimit = fTmp;
    iTmp = fget_next_int( fileread ); ppip->facingadd = iTmp;

    // override the base rotation
    if ( 0xFFFF != prt_direction[ ppip->imagebase ] )
    {
        ppip->rotatebase = prt_direction[ ppip->imagebase ];
    };

    // Ending conditions
    cTmp = fget_next_char( fileread );
    ppip->endwater = btrue;
    if ( 'F' == toupper(cTmp) )  ppip->endwater = bfalse;

    cTmp = fget_next_char( fileread );
    ppip->endbump = btrue;
    if ( 'F' == toupper(cTmp) )  ppip->endbump = bfalse;

    cTmp = fget_next_char( fileread );
    ppip->endground = btrue;
    if ( 'F' == toupper(cTmp) )  ppip->endground = bfalse;

    cTmp = fget_next_char( fileread );
    ppip->endlastframe = btrue;
    if ( 'F' == toupper(cTmp) )  ppip->endlastframe = bfalse;

    iTmp = fget_next_int( fileread ); ppip->time = iTmp;

    // Collision data
    fTmp = fget_next_float( fileread ); ppip->dampen = fTmp;
    iTmp = fget_next_int( fileread );   ppip->bumpmoney = iTmp;
    iTmp = fget_next_int( fileread );   ppip->bumpsize = iTmp;
    iTmp = fget_next_int( fileread );   ppip->bumpheight = iTmp;
    fget_next_pair( fileread );
    ppip->damagebase = pairbase;
    ppip->damagerand = pairrand;
    cTmp = fget_next_char( fileread );
    if ( 'S' == toupper(cTmp) ) ppip->damagetype = DAMAGE_SLASH;
    if ( 'C' == toupper(cTmp) ) ppip->damagetype = DAMAGE_CRUSH;
    if ( 'P' == toupper(cTmp) ) ppip->damagetype = DAMAGE_POKE;
    if ( 'H' == toupper(cTmp) ) ppip->damagetype = DAMAGE_HOLY;
    if ( 'E' == toupper(cTmp) ) ppip->damagetype = DAMAGE_EVIL;
    if ( 'F' == toupper(cTmp) ) ppip->damagetype = DAMAGE_FIRE;
    if ( 'I' == toupper(cTmp) ) ppip->damagetype = DAMAGE_ICE;
    if ( 'Z' == toupper(cTmp) ) ppip->damagetype = DAMAGE_ZAP;

    // Lighting data
    cTmp = fget_next_char( fileread );
    ppip->dynalightmode = DYNAOFF;
    if ( 'T' == toupper(cTmp) ) ppip->dynalightmode = DYNAON;
    if ( 'L' == toupper(cTmp) ) ppip->dynalightmode = DYNALOCAL;

    fTmp = fget_next_float( fileread ); ppip->dynalevel = fTmp;
    iTmp = fget_next_int( fileread ); ppip->dynafalloff = iTmp;
//   if ( ppip->dynafalloff > MAXFALLOFF && PMod->rtscontrol )  ppip->dynafalloff = MAXFALLOFF;

    // Initial spawning of this particle
    iTmp = fget_next_int( fileread ); ppip->facingbase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->facingrand = iTmp;
    iTmp = fget_next_int( fileread ); ppip->xyspacingbase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->xyspacingrand = iTmp;
    iTmp = fget_next_int( fileread ); ppip->zspacingbase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->zspacingrand = iTmp;
    iTmp = fget_next_int( fileread ); ppip->xyvelbase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->xyvelrand = iTmp;
    iTmp = fget_next_int( fileread ); ppip->zvelbase = iTmp;
    iTmp = fget_next_int( fileread ); ppip->zvelrand = iTmp;

    // Continuous spawning of other particles
    iTmp = fget_next_int( fileread ); ppip->contspawntime = iTmp;
    iTmp = fget_next_int( fileread ); ppip->contspawnamount = iTmp;
    iTmp = fget_next_int( fileread ); ppip->contspawnfacingadd = iTmp;
    iTmp = fget_next_int( fileread ); ppip->contspawnpip = iTmp;

    // End spawning of other particles
    iTmp = fget_next_int( fileread ); ppip->endspawnamount = iTmp;
    iTmp = fget_next_int( fileread ); ppip->endspawnfacingadd = iTmp;
    iTmp = fget_next_int( fileread ); ppip->endspawnpip = iTmp;

    // Bump spawning of attached particles
    iTmp = fget_next_int( fileread ); ppip->bumpspawnamount = iTmp;
    iTmp = fget_next_int( fileread ); ppip->bumpspawnpip = iTmp;

    // Random stuff  !!!BAD!!! Not complete
    iTmp = fget_next_int( fileread ); ppip->dazetime = iTmp;
    iTmp = fget_next_int( fileread ); ppip->grogtime = iTmp;
    cTmp = fget_next_char( fileread );
    ppip->spawnenchant = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->spawnenchant = btrue;

    goto_colon( NULL, fileread, bfalse );  // !!Cause roll

    // Cause pancake
    goto_colon( NULL, fileread, bfalse );
    ppip->causepancake = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->causepancake = btrue;

    cTmp = fget_next_char( fileread );
    ppip->needtarget = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->needtarget = btrue;

    cTmp = fget_next_char( fileread );
    ppip->targetcaster = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->targetcaster = btrue;

    cTmp = fget_next_char( fileread );
    ppip->startontarget = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->startontarget = btrue;

    cTmp = fget_next_char( fileread );
    ppip->onlydamagefriendly = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->onlydamagefriendly = btrue;

    iTmp = fget_next_int( fileread );
    ppip->soundspawn = CLIP(iTmp, -1, MAX_WAVE);

    iTmp = fget_next_int( fileread );
    ppip->soundend = CLIP(iTmp, -1, MAX_WAVE);

    cTmp = fget_next_char( fileread );
    ppip->friendlyfire = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->friendlyfire = btrue;

    goto_colon( NULL, fileread, bfalse );
    // ppip->hateonly = bfalse; TODO: BAD not implemented yet

    cTmp = fget_next_char( fileread );
    ppip->newtargetonspawn = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->newtargetonspawn = btrue;

    iTmp = fget_next_int( fileread ); ppip->targetangle = iTmp >> 1;
    cTmp = fget_next_char( fileread );
    ppip->homing = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->homing = btrue;

    fTmp = fget_next_float( fileread ); ppip->homingfriction = fTmp;
    fTmp = fget_next_float( fileread ); ppip->homingaccel = fTmp;
    cTmp = fget_next_char( fileread );
    ppip->rotatetoface = bfalse;
    if ( 'T' == toupper(cTmp) ) ppip->rotatetoface = btrue;

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
    ppip->orientation = ORIENTATION_B;  // make the orientation the normal billboarded orientation

    // Read expansions
    while ( goto_colon( NULL, fileread, btrue ) )
    {
        idsz = fget_idsz( fileread );

        if ( idsz == MAKE_IDSZ( 'T', 'U', 'R', 'N' ) )  ppip->damfx = DAMFX_NONE;
        else if ( idsz == MAKE_IDSZ( 'A', 'R', 'M', 'O' ) )  ppip->damfx |= DAMFX_ARMO;
        else if ( idsz == MAKE_IDSZ( 'B', 'L', 'O', 'C' ) )  ppip->damfx |= DAMFX_NBLOC;
        else if ( idsz == MAKE_IDSZ( 'A', 'R', 'R', 'O' ) )  ppip->damfx |= DAMFX_ARRO;
        else if ( idsz == MAKE_IDSZ( 'T', 'I', 'M', 'E' ) )  ppip->damfx |= DAMFX_TIME;
        else if ( idsz == MAKE_IDSZ( 'Z', 'S', 'P', 'D' ) )  ppip->zaimspd = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'F', 'S', 'N', 'D' ) )  ppip->soundfloor = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'S', 'N', 'D' ) )  ppip->soundwall = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'E', 'N', 'D' ) )  ppip->endwall = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'P', 'U', 'S', 'H' ) )  ppip->allowpush = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'D', 'L', 'E', 'V' ) )  ppip->dynalightleveladd = fget_int( fileread ) / 1000.0f;
        else if ( idsz == MAKE_IDSZ( 'D', 'R', 'A', 'D' ) )  ppip->dynalightfalloffadd = fget_int( fileread ) / 1000.0f;
        else if ( idsz == MAKE_IDSZ( 'I', 'D', 'A', 'M' ) )  ppip->intdamagebonus = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'W', 'D', 'A', 'M' ) )  ppip->wisdamagebonus = fget_int( fileread );
        else if ( idsz == MAKE_IDSZ( 'O', 'R', 'N', 'T' ) )
        {
            char cTmp = fget_first_letter( fileread );
            switch ( toupper(cTmp) )
            {
                case 'X': ppip->orientation = ORIENTATION_X; break;  // put particle up along the world or body-fixed x-axis
                case 'Y': ppip->orientation = ORIENTATION_Y; break;  // put particle up along the world or body-fixed y-axis
                case 'Z': ppip->orientation = ORIENTATION_Z; break;  // put particle up along the world or body-fixed z-axis
                case 'V': ppip->orientation = ORIENTATION_V; break;  // vertical, like a candle
                case 'H': ppip->orientation = ORIENTATION_H; break;  // horizontal, like a plate
                case 'B': ppip->orientation = ORIENTATION_B; break;  // billboard
            }
        }
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
    if ( MAX_PIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "5money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "25money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "100money.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    // Load module specific information
    make_newloadname( modname, "gamedat" SLASH_STR "weather4.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "weather5.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "splash.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname ) )
    {
        if (cfg.dev_mode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "splash.txt";
        if ( MAX_PIP == load_one_particle_profile( loadpath ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    make_newloadname( modname, "gamedat" SLASH_STR "ripple.txt", newloadname );
    if ( MAX_PIP == load_one_particle_profile( newloadname ) )
    {
        if (cfg.dev_mode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "ripple.txt";
        if ( MAX_PIP == load_one_particle_profile( loadpath ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    // This is also global...
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.txt";
    if ( MAX_PIP == load_one_particle_profile( loadpath ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    // Now clear out the local pips
    object = 0;
    while ( object < MAX_PROFILE )
    {
        cnt = 0;

        while ( cnt < MAX_PIP_PER_PROFILE )
        {
            MadList[object].prtpip[cnt] = 0;
            cnt++;
        }

        object++;
    }
}
