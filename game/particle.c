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
static int       numfreeprt = 0;                            // For allocation
static Uint16    freeprtlist[TOTALMAXPRT];                        //

int              numpip   = 0;
pip_t            PipList[TOTALMAXPRTPIP];

Uint16           particletexture = 0;                        // All in one bitmap
float            particleimageu[MAXPARTICLEIMAGE][2];        // Texture coordinates
float            particleimagev[MAXPARTICLEIMAGE][2];        //

Uint16           maxparticles = 512;                            // max number of particles
prt_t            PrtList[TOTALMAXPRT];

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
    if ( sound >= 0 && sound < MAXWAVE )
    {
        if ( PrtList[particle].model != MAXMODEL )
        {
            sound_play_chunk( PrtList[particle].xpos, PrtList[particle].ypos, CapList[PrtList[particle].model].wavelist[sound] );
        }
        else
        {
            sound_play_chunk( PrtList[particle].xpos, PrtList[particle].ypos, g_wavelist[sound] );
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack and
    //     plays the sound associated with the particle
    int child;
    if ( PrtList[particle].spawncharacterstate != SPAWNNOCHARACTER )
    {
        child = spawn_one_character( PrtList[particle].xpos, PrtList[particle].ypos, PrtList[particle].zpos,
                                     PrtList[particle].model, PrtList[particle].team, 0, PrtList[particle].facing,
                                     NULL, MAXCHR );
        if ( child != MAXCHR )
        {
            ChrList[child].ai.state = PrtList[particle].spawncharacterstate;
            ChrList[child].ai.owner = PrtList[particle].chr;
        }
    }

    play_particle_sound( particle, PipList[PrtList[particle].pip].soundend );
    freeprtlist[numfreeprt] = particle;
    numfreeprt++;
    PrtList[particle].on = bfalse;
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
                           Uint16 facing, Uint16 model, Uint16 pip,
                           Uint16 characterattach, Uint16 grip, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget )
{
    // ZZ> This function spawns a new particle, and returns the number of that particle
    int iprt, velocity;
    float xvel, yvel, zvel, tvel;
    int offsetfacing = 0, newrand;

    // Convert from local pip to global pip
    if ( model < MAXMODEL && pip < MAXPRTPIPPEROBJECT )
    {
        pip = MadList[model].prtpip[pip];
    }

    iprt = get_free_particle( PipList[pip].force );
    if ( iprt != TOTALMAXPRT )
    {
        // Necessary data for any part
        PrtList[iprt].on = btrue;
        PrtList[iprt].pip = pip;
        PrtList[iprt].model = model;
        PrtList[iprt].inview = bfalse;
        PrtList[iprt].level = 0;
        PrtList[iprt].team = team;
        PrtList[iprt].chr = characterorigin;
        PrtList[iprt].damagetype = PipList[pip].damagetype;
        PrtList[iprt].spawncharacterstate = SPAWNNOCHARACTER;

        // Lighting and sound
        PrtList[iprt].dynalighton = bfalse;
        if ( multispawn == 0 )
        {
            PrtList[iprt].dynalighton = PipList[pip].dynalightmode;
            if ( PipList[pip].dynalightmode == DYNALOCAL )
            {
                PrtList[iprt].dynalighton = bfalse;
            }
        }

        PrtList[iprt].dynalightlevel = PipList[pip].dynalevel;
        PrtList[iprt].dynalightfalloff = PipList[pip].dynafalloff;

        // Set character attachments ( characterattach==MAXCHR means none )
        PrtList[iprt].attachedtocharacter = characterattach;
        PrtList[iprt].grip = grip;

        // Correct facing
        facing += PipList[pip].facingbase;

        // Targeting...
        zvel = 0;
        newrand = RANDIE;
        z = z + PipList[pip].zspacingbase + ( newrand & PipList[pip].zspacingrand ) - ( PipList[pip].zspacingrand >> 1 );
        newrand = RANDIE;
        velocity = ( PipList[pip].xyvelbase + ( newrand & PipList[pip].xyvelrand ) );
        PrtList[iprt].target = oldtarget;
        if ( PipList[pip].newtargetonspawn )
        {
            if ( PipList[pip].targetcaster )
            {
                // Set the target to the caster
                PrtList[iprt].target = characterorigin;
            }
            else
            {

                // Find a target
                PrtList[iprt].target = get_particle_target( x, y, z, facing, pip, team, characterorigin, oldtarget );
                if ( PrtList[iprt].target != MAXCHR && !PipList[pip].homing )
                {
                    facing = facing - glouseangle;
                }

                // Correct facing for dexterity...
                offsetfacing = 0;
                if ( ChrList[characterorigin].dexterity < PERFECTSTAT )
                {
                    // Correct facing for randomness
                    offsetfacing = RANDIE;
                    offsetfacing = offsetfacing & PipList[pip].facingrand;
                    offsetfacing -= ( PipList[pip].facingrand >> 1 );
                    offsetfacing = ( offsetfacing * ( PERFECTSTAT - ChrList[characterorigin].dexterity ) ) / PERFECTSTAT;  // Divided by PERFECTSTAT
                }
                if ( PrtList[iprt].target != MAXCHR && PipList[pip].zaimspd != 0 )
                {
                    // These aren't velocities...  This is to do aiming on the Z axis
                    if ( velocity > 0 )
                    {
                        xvel = ChrList[PrtList[iprt].target].xpos - x;
                        yvel = ChrList[PrtList[iprt].target].ypos - y;
                        tvel = SQRT( xvel * xvel + yvel * yvel ) / velocity;  // This is the number of steps...
                        if ( tvel > 0 )
                        {
                            zvel = ( ChrList[PrtList[iprt].target].zpos + ( ChrList[PrtList[iprt].target].bumpsize >> 1 ) - z ) / tvel;  // This is the zvel alteration
                            if ( zvel < -( PipList[pip].zaimspd >> 1 ) ) zvel = -( PipList[pip].zaimspd >> 1 );
                            if ( zvel > PipList[pip].zaimspd ) zvel = PipList[pip].zaimspd;
                        }
                    }
                }
            }

            // Does it go away?
            if ( PrtList[iprt].target == MAXCHR && PipList[pip].needtarget )
            {
                free_one_particle( iprt );
                return maxparticles;
            }

            // Start on top of target
            if ( PrtList[iprt].target != MAXCHR && PipList[pip].startontarget )
            {
                x = ChrList[PrtList[iprt].target].xpos;
                y = ChrList[PrtList[iprt].target].ypos;
            }
        }
        else
        {
            // Correct facing for randomness
            offsetfacing = RANDIE;
            offsetfacing = offsetfacing & PipList[pip].facingrand;
            offsetfacing -= ( PipList[pip].facingrand >> 1 );
        }

        facing += offsetfacing;
        PrtList[iprt].facing = facing;
        facing = facing >> 2;

        // Location data from arguments
        newrand = RANDIE;
        x = x + turntocos[( facing+8192 )&TRIG_TABLE_MASK] * ( PipList[pip].xyspacingbase + ( newrand & PipList[pip].xyspacingrand ) );
        y = y + turntosin[( facing+8192 )&TRIG_TABLE_MASK] * ( PipList[pip].xyspacingbase + ( newrand & PipList[pip].xyspacingrand ) );
        if ( x < 0 )  x = 0;
        if ( x > mesh.info.edge_x - 2 )  x = mesh.info.edge_x - 2;
        if ( y < 0 )  y = 0;
        if ( y > mesh.info.edge_y - 2 )  y = mesh.info.edge_y - 2;

        PrtList[iprt].xpos = x;
        PrtList[iprt].ypos = y;
        PrtList[iprt].zpos = z;

        // Velocity data
        xvel = turntocos[( facing+8192 )&TRIG_TABLE_MASK] * velocity;
        yvel = turntosin[( facing+8192 )&TRIG_TABLE_MASK] * velocity;
        newrand = RANDIE;
        zvel += PipList[pip].zvelbase + ( newrand & PipList[pip].zvelrand ) - ( PipList[pip].zvelrand >> 1 );
        PrtList[iprt].xvel = xvel;
        PrtList[iprt].yvel = yvel;
        PrtList[iprt].zvel = zvel;

        // Template values
        PrtList[iprt].bumpsize = PipList[pip].bumpsize;
        PrtList[iprt].bumpsizebig = PrtList[iprt].bumpsize + ( PrtList[iprt].bumpsize >> 1 );
        PrtList[iprt].bumpheight = PipList[pip].bumpheight;
        PrtList[iprt].type = PipList[pip].type;

        // Image data
        newrand = RANDIE;
        PrtList[iprt].rotate = ( newrand & PipList[pip].rotaterand ) + PipList[pip].rotatebase;
        PrtList[iprt].rotateadd = PipList[pip].rotateadd;
        PrtList[iprt].size = PipList[pip].sizebase;
        PrtList[iprt].sizeadd = PipList[pip].sizeadd;
        PrtList[iprt].image = 0;
        newrand = RANDIE;
        PrtList[iprt].imageadd = PipList[pip].imageadd + ( newrand & PipList[pip].imageaddrand );
        PrtList[iprt].imagestt = INT_TO_FP8( PipList[pip].imagebase );
        PrtList[iprt].imagemax = INT_TO_FP8( PipList[pip].numframes );
        PrtList[iprt].time = PipList[pip].time;
        if ( PipList[pip].endlastframe && PrtList[iprt].imageadd != 0 )
        {
            if ( PrtList[iprt].time == 0 )
            {
                // Part time is set to 1 cycle
                PrtList[iprt].time = ( PrtList[iprt].imagemax / PrtList[iprt].imageadd ) - 1;
            }
            else
            {
                // Part time is used to give number of cycles
                PrtList[iprt].time = PrtList[iprt].time * ( ( PrtList[iprt].imagemax / PrtList[iprt].imageadd ) - 1 );
            }
        }

        // Set onwhichfan...
        PrtList[iprt].onwhichfan   = mesh_get_tile( PrtList[iprt].xpos, PrtList[iprt].ypos );
        PrtList[iprt].onwhichblock = mesh_get_block( PrtList[iprt].xpos, PrtList[iprt].ypos );

        // Damage stuff
        PrtList[iprt].damagebase = PipList[pip].damagebase;
        PrtList[iprt].damagerand = PipList[pip].damagerand;

        // Spawning data
        PrtList[iprt].spawntime = PipList[pip].contspawntime;
        if ( PrtList[iprt].spawntime != 0 )
        {
            PrtList[iprt].spawntime = 1;
            if ( PrtList[iprt].attachedtocharacter != MAXCHR )
            {
                PrtList[iprt].spawntime++; // Because attachment takes an update before it happens
            }
        }

        // Sound effect
        play_particle_sound( iprt, PipList[pip].soundspawn );
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
        PrtList[cnt].level      = get_level( PrtList[cnt].xpos, PrtList[cnt].ypos, bfalse );

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
        level = PrtList[cnt].level + ( PrtList[cnt].size >> 9 );

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
    Uint16 pip;
    Uint16 vertices;
    Uint16 direction, left, right, model;
    float fsin, fcos;

    pip = PrtList[particle].pip;
    amount = PipList[pip].bumpspawnamount;
    if ( amount != 0 || PipList[pip].spawnenchant )
    {
        // Only damage if hitting from proper direction
        model = ChrList[character].model;
        vertices = MadList[model].vertices;
        direction = ( ATAN2( PrtList[particle].yvel, PrtList[particle].xvel ) + PI ) * 0xFFFF / ( TWO_PI );
        direction = ChrList[character].turnleftright - direction + 32768;
        if ( Md2FrameList[ChrList[character].frame].framefx&MADFXINVICTUS )
        {
            // I Frame
            if ( PipList[pip].damfx&DAMFXBLOC )
            {
                left = 0xFFFF;
                right = 0;
            }
            else
            {
                direction -= CapList[model].iframefacing;
                left = ( ~CapList[model].iframeangle );
                right = CapList[model].iframeangle;
            }
        }
        else
        {
            // N Frame
            direction -= CapList[model].nframefacing;
            left = ( ~CapList[model].nframeangle );
            right = CapList[model].nframeangle;
        }

        // Check that direction
        if ( direction <= left && direction >= right )
        {
            // Spawn new enchantments
            if ( PipList[pip].spawnenchant )
            {
                spawn_enchant( PrtList[particle].chr, character, MAXCHR, MAXENCHANT, PrtList[particle].model );
            }

            // Spawn particles
            if ( amount != 0 && !CapList[ChrList[character].model].resistbumpspawn && !ChrList[character].invictus && vertices != 0 && ( ChrList[character].damagemodifier[PrtList[particle].damagetype]&DAMAGESHIFT ) < 3 )
            {
                if ( amount == 1 )
                {
                    // A single particle ( arrow? ) has been stuck in the character...
                    // Find best vertex to attach to
                    bestvertex = 0;
                    bestdistance = 9999999;
                    z = -ChrList[character].zpos + PrtList[particle].zpos + RAISE;
                    facing = PrtList[particle].facing - ChrList[character].turnleftright - 16384;
                    facing = facing >> 2;
                    fsin = turntosin[facing];
                    fcos = turntocos[facing];
                    y = 8192;
                    x = -y * fsin;
                    y = y * fcos;
                    z = z << 10;/// ChrList[character].scale;
                    frame = MadList[ChrList[character].model].framestart;
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

                    spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                                        character, bestvertex + 1, PrtList[particle].team, PrtList[particle].chr, cnt, character );
                }
                else
                {
                    amount = ( amount * vertices ) >> 5;  // Correct amount for size of character
                    cnt = 0;

                    while ( cnt < amount )
                    {
                        spawn_one_particle( ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                                            character, rand() % vertices, PrtList[particle].team, PrtList[particle].chr, cnt, character );
                        cnt++;
                    }
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
int load_one_particle( const char *szLoadName, Uint16 object, Uint16 pip )
{
    // ZZ> This function loads a particle template, returning bfalse if the file wasn't
    //     found
    FILE* fileread;
    int test, idsz;
    int iTmp;
    float fTmp;
    char cTmp;

    fileread = fopen( szLoadName, "r" );
    if ( fileread != NULL )
    {
        // General data
        parse_filename = szLoadName;    //For debugging missing colons
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].force = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  PipList[numpip].force = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'L' || cTmp == 'l' )  PipList[numpip].type = PRTLIGHTSPRITE;
        else if ( cTmp == 'S' || cTmp == 's' )  PipList[numpip].type = PRTSOLIDSPRITE;
        else if ( cTmp == 'T' || cTmp == 't' )  PipList[numpip].type = PRTALPHASPRITE;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].imagebase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].numframes = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].imageadd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].imageaddrand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].rotatebase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].rotaterand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].rotateadd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].sizebase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].sizeadd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); PipList[numpip].spdlimit = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].facingadd = iTmp;

        // Ending conditions
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].endwater = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  PipList[numpip].endwater = bfalse;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].endbump = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  PipList[numpip].endbump = bfalse;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].endground = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  PipList[numpip].endground = bfalse;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].endlastframe = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  PipList[numpip].endlastframe = bfalse;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].time = iTmp;

        // Collision data
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); PipList[numpip].dampen = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].bumpmoney = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].bumpsize = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].bumpheight = iTmp;
        goto_colon( fileread );  read_pair( fileread );
        PipList[numpip].damagebase = pairbase;
        PipList[numpip].damagerand = pairrand;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        if ( cTmp == 'S' || cTmp == 's' ) PipList[numpip].damagetype = DAMAGE_SLASH;
        if ( cTmp == 'C' || cTmp == 'c' ) PipList[numpip].damagetype = DAMAGE_CRUSH;
        if ( cTmp == 'P' || cTmp == 'p' ) PipList[numpip].damagetype = DAMAGE_POKE;
        if ( cTmp == 'H' || cTmp == 'h' ) PipList[numpip].damagetype = DAMAGE_HOLY;
        if ( cTmp == 'E' || cTmp == 'e' ) PipList[numpip].damagetype = DAMAGE_EVIL;
        if ( cTmp == 'F' || cTmp == 'f' ) PipList[numpip].damagetype = DAMAGE_FIRE;
        if ( cTmp == 'I' || cTmp == 'i' ) PipList[numpip].damagetype = DAMAGE_ICE;
        if ( cTmp == 'Z' || cTmp == 'z' ) PipList[numpip].damagetype = DAMAGE_ZAP;

        // Lighting data
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].dynalightmode = DYNAOFF;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].dynalightmode = DYNAON;
        if ( cTmp == 'L' || cTmp == 'l' ) PipList[numpip].dynalightmode = DYNALOCAL;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); PipList[numpip].dynalevel = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].dynafalloff = iTmp;
        if ( PipList[numpip].dynafalloff > MAXFALLOFF && rtscontrol )  PipList[numpip].dynafalloff = MAXFALLOFF;

        // Initial spawning of this particle
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].facingbase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].facingrand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].xyspacingbase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].xyspacingrand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].zspacingbase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].zspacingrand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].xyvelbase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].xyvelrand = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].zvelbase = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].zvelrand = iTmp;

        // Continuous spawning of other particles
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].contspawntime = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].contspawnamount = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].contspawnfacingadd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].contspawnpip = iTmp;

        // End spawning of other particles
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].endspawnamount = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].endspawnfacingadd = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].endspawnpip = iTmp;

        // Bump spawning of attached particles
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].bumpspawnamount = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].bumpspawnpip = iTmp;

        // Random stuff  !!!BAD!!! Not complete
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].dazetime = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].grogtime = iTmp;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].spawnenchant = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].spawnenchant = btrue;

        goto_colon( fileread );  // !!Cause roll
        goto_colon( fileread );  // !!Cause pancake
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].needtarget = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].needtarget = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].targetcaster = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].targetcaster = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].startontarget = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].startontarget = btrue;

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].onlydamagefriendly = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].onlydamagefriendly = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        PipList[numpip].soundspawn = CLIP(iTmp, -1, MAXWAVE);

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        PipList[numpip].soundend = CLIP(iTmp, -1, MAXWAVE);

        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].friendlyfire = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].friendlyfire = btrue;   
		
		goto_colon( fileread );
		//PipList[numpip].hateonly = bfalse; TODO: BAD not implemented yet
        
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].newtargetonspawn = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].newtargetonspawn = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); PipList[numpip].targetangle = iTmp >> 1;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].homing = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].homing = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); PipList[numpip].homingfriction = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); PipList[numpip].homingaccel = fTmp;
        goto_colon( fileread );  cTmp = fget_first_letter( fileread );
        PipList[numpip].rotatetoface = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) PipList[numpip].rotatetoface = btrue;

        // Clear expansions...
        PipList[numpip].zaimspd = 0;
        PipList[numpip].soundfloor = -1;
        PipList[numpip].soundwall = -1;
        PipList[numpip].endwall = PipList[numpip].endground;
        PipList[numpip].damfx = DAMFXTURN;
        if ( PipList[numpip].homing )  PipList[numpip].damfx = DAMFXNONE;

        PipList[numpip].allowpush = btrue;
        PipList[numpip].dynalightfalloffadd = 0;
        PipList[numpip].dynalightleveladd = 0;
        PipList[numpip].intdamagebonus = bfalse;
        PipList[numpip].wisdamagebonus = bfalse;

        // Read expansions
        while ( goto_colon_yesno( fileread ) )
        {
            idsz = fget_idsz( fileread );
            fscanf( fileread, "%c%d", &cTmp, &iTmp );
            test = Make_IDSZ( "TURN" );  // [TURN]
            if ( idsz == test )  PipList[numpip].damfx = DAMFXNONE;

            test = Make_IDSZ( "ZSPD" );  // [ZSPD]
            if ( idsz == test )  PipList[numpip].zaimspd = iTmp;

            test = Make_IDSZ( "FSND" );  // [FSND]
            if ( idsz == test )  PipList[numpip].soundfloor = iTmp;

            test = Make_IDSZ( "WSND" );  // [WSND]
            if ( idsz == test )  PipList[numpip].soundwall = iTmp;

            test = Make_IDSZ( "WEND" );  // [WEND]
            if ( idsz == test )  PipList[numpip].endwall = iTmp;

            test = Make_IDSZ( "ARMO");  // [ARMO]
            if ( idsz == test )  PipList[numpip].damfx |= DAMFXARMO;

            test = Make_IDSZ( "BLOC" );  // [BLOC]
            if ( idsz == test )  PipList[numpip].damfx |= DAMFXBLOC;

            test = Make_IDSZ( "ARRO" );  // [ARRO]
            if ( idsz == test )  PipList[numpip].damfx |= DAMFXARRO;

            test = Make_IDSZ( "TIME" );  // [TIME]
            if ( idsz == test )  PipList[numpip].damfx |= DAMFXTIME;

            test = Make_IDSZ( "PUSH" );  // [PUSH]
            if ( idsz == test )  PipList[numpip].allowpush = iTmp;

            test = Make_IDSZ( "DLEV" );  // [DLEV]
            if ( idsz == test )  PipList[numpip].dynalightleveladd = iTmp / 1000.0f;

            test = Make_IDSZ( "DRAD" );  // [DRAD]
            if ( idsz == test )  PipList[numpip].dynalightfalloffadd = iTmp / 1000.0f;

            test = Make_IDSZ( "IDAM");  // [IDAM]
            if ( idsz == test )  PipList[numpip].intdamagebonus = iTmp;

            test = Make_IDSZ( "WDAM" );  // [WDAM]
            if ( idsz == test )  PipList[numpip].wisdamagebonus = iTmp;
        }

        // Make sure it's referenced properly
        MadList[object].prtpip[pip] = numpip;
        numpip++;

        fclose( fileread );
        return btrue;
    }

    return bfalse;
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
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "5money.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "25money.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "100money.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)\n", loadpath );
    }

    // Load module specific information
    make_newloadname( modname, "gamedat" SLASH_STR "weather4.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "weather5.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)\n", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "splash.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        if (gDevMode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "splash.txt";
        if ( !load_one_particle( loadpath, 0, 0 ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    make_newloadname( modname, "gamedat" SLASH_STR "ripple.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        if (gDevMode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "ripple.txt";
        if ( !load_one_particle( loadpath, 0, 0 ) )
        {
            log_error( "Data file was not found! (%s)\n", loadpath );
        }
    }

    // This is also global...
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
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

