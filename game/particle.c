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

#include "egoboo.h"
#include "log.h"
#include "particle.h"
#include "sound.h"
#include "camera.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void make_prtlist( void )
{
    // ZZ> This function figures out which particles are visible, and it sets up dynamic
    //     lighting
    int cnt, tnc, disx, disy, distance, slot;

    // Don't really make a list, just set to visible or not
    numdynalight = 0;
    dynadistancetobeat = MAXDYNADIST;
    cnt = 0;

    while ( cnt < maxparticles )
    {
        prtinview[cnt] = bfalse;
        if ( prton[cnt] && INVALID_TILE != prtonwhichfan[cnt] )
        {
            prtinview[cnt] = meshinrenderlist[prtonwhichfan[cnt]];

            // Set up the lights we need
            if ( prtdynalighton[cnt] )
            {
                disx = prtxpos[cnt] - camtrackx;
                disx = ABS( disx );
                disy = prtypos[cnt] - camtracky;
                disy = ABS( disy );
                distance = disx + disy;
                if ( distance < dynadistancetobeat )
                {
                    if ( numdynalight < maxlights )
                    {
                        // Just add the light
                        slot = numdynalight;
                        dynadistance[slot] = distance;
                        numdynalight++;
                    }
                    else
                    {
                        // Overwrite the worst one
                        slot = 0;
                        tnc = 1;
                        dynadistancetobeat = dynadistance[0];

                        while ( tnc < maxlights )
                        {
                            if ( dynadistance[tnc] > dynadistancetobeat )
                            {
                                slot = tnc;
                            }

                            tnc++;
                        }

                        dynadistance[slot] = distance;

                        // Find the new distance to beat
                        tnc = 1;
                        dynadistancetobeat = dynadistance[0];

                        while ( tnc < maxlights )
                        {
                            if ( dynadistance[tnc] > dynadistancetobeat )
                            {
                                dynadistancetobeat = dynadistance[tnc];
                            }

                            tnc++;
                        }
                    }

                    dynalightlistx[slot] = prtxpos[cnt];
                    dynalightlisty[slot] = prtypos[cnt];
                    dynalightlevel[slot] = prtdynalightlevel[cnt];
                    dynalightfalloff[slot] = prtdynalightfalloff[cnt];
                }
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_no_sound( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack
    freeprtlist[numfreeprt] = particle;
    numfreeprt++;
    prton[particle] = bfalse;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( Uint16 particle, Sint8 sound )
{
    // This function plays a sound effect for a particle
    if ( sound >= 0 && sound < MAXWAVE )
    {
        if ( prtmodel[particle] != MAXMODEL )
        {
            sound_play_chunk( prtxpos[particle], prtypos[particle], capwavelist[prtmodel[particle]][sound] );
        }
        else
        {
            sound_play_chunk( prtxpos[particle], prtypos[particle], g_wavelist[sound] );
        }
    }
}

//--------------------------------------------------------------------------------------------
void free_one_particle( Uint16 particle )
{
    // ZZ> This function sticks a particle back on the free particle stack and
    //     plays the sound associated with the particle
    int child;
    if ( prtspawncharacterstate[particle] != SPAWNNOCHARACTER )
    {
        child = spawn_one_character( prtxpos[particle], prtypos[particle], prtzpos[particle],
                                     prtmodel[particle], prtteam[particle], 0, prtfacing[particle],
                                     NULL, MAXCHR );
        if ( child != MAXCHR )
        {
            chr[child].ai.state = prtspawncharacterstate[particle];
            chr[child].ai.owner = prtchr[particle];
        }
    }

    play_particle_sound( particle, pipsoundend[prtpip[particle]] );
    freeprtlist[numfreeprt] = particle;
    numfreeprt++;
    prton[particle] = bfalse;
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
                if ( prtbumpsize[particle] == 0 )
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
        pip = madprtpip[model][pip];
    }

    iprt = get_free_particle( pipforce[pip] );
    if ( iprt != TOTALMAXPRT )
    {
        // Necessary data for any part
        prton[iprt] = btrue;
        prtpip[iprt] = pip;
        prtmodel[iprt] = model;
        prtinview[iprt] = bfalse;
        prtlevel[iprt] = 0;
        prtteam[iprt] = team;
        prtchr[iprt] = characterorigin;
        prtdamagetype[iprt] = pipdamagetype[pip];
        prtspawncharacterstate[iprt] = SPAWNNOCHARACTER;

        // Lighting and sound
        prtdynalighton[iprt] = bfalse;
        if ( multispawn == 0 )
        {
            prtdynalighton[iprt] = pipdynalightmode[pip];
            if ( pipdynalightmode[pip] == DYNALOCAL )
            {
                prtdynalighton[iprt] = bfalse;
            }
        }

        prtdynalightlevel[iprt] = pipdynalevel[pip];
        prtdynalightfalloff[iprt] = pipdynafalloff[pip];

        // Set character attachments ( characterattach==MAXCHR means none )
        prtattachedtocharacter[iprt] = characterattach;
        prtgrip[iprt] = grip;

        // Correct facing
        facing += pipfacingbase[pip];

        // Targeting...
        zvel = 0;
        newrand = RANDIE;
        z = z + pipzspacingbase[pip] + ( newrand & pipzspacingrand[pip] ) - ( pipzspacingrand[pip] >> 1 );
        newrand = RANDIE;
        velocity = ( pipxyvelbase[pip] + ( newrand & pipxyvelrand[pip] ) );
        prttarget[iprt] = oldtarget;
        if ( pipnewtargetonspawn[pip] )
        {
            if ( piptargetcaster[pip] )
            {
                // Set the target to the caster
                prttarget[iprt] = characterorigin;
            }
            else
            {

                // Find a target
                prttarget[iprt] = get_particle_target( x, y, z, facing, pip, team, characterorigin, oldtarget );
                if ( prttarget[iprt] != MAXCHR && !piphoming[pip] )
                {
                    facing = facing - glouseangle;
                }

                // Correct facing for dexterity...
                offsetfacing = 0;
                if ( chr[characterorigin].dexterity < PERFECTSTAT )
                {
                    // Correct facing for randomness
                    offsetfacing = RANDIE;
                    offsetfacing = offsetfacing & pipfacingrand[pip];
                    offsetfacing -= ( pipfacingrand[pip] >> 1 );
                    offsetfacing = ( offsetfacing * ( PERFECTSTAT - chr[characterorigin].dexterity ) ) / PERFECTSTAT;  // Divided by PERFECTSTAT
                }
                if ( prttarget[iprt] != MAXCHR && pipzaimspd[pip] != 0 )
                {
                    // These aren't velocities...  This is to do aiming on the Z axis
                    if ( velocity > 0 )
                    {
                        xvel = chr[prttarget[iprt]].xpos - x;
                        yvel = chr[prttarget[iprt]].ypos - y;
                        tvel = SQRT( xvel * xvel + yvel * yvel ) / velocity;  // This is the number of steps...
                        if ( tvel > 0 )
                        {
                            zvel = ( chr[prttarget[iprt]].zpos + ( chr[prttarget[iprt]].bumpsize >> 1 ) - z ) / tvel;  // This is the zvel alteration
                            if ( zvel < -( pipzaimspd[pip] >> 1 ) ) zvel = -( pipzaimspd[pip] >> 1 );
                            if ( zvel > pipzaimspd[pip] ) zvel = pipzaimspd[pip];
                        }
                    }
                }
            }

            // Does it go away?
            if ( prttarget[iprt] == MAXCHR && pipneedtarget[pip] )
            {
                free_one_particle( iprt );
                return maxparticles;
            }

            // Start on top of target
            if ( prttarget[iprt] != MAXCHR && pipstartontarget[pip] )
            {
                x = chr[prttarget[iprt]].xpos;
                y = chr[prttarget[iprt]].ypos;
            }
        }
        else
        {
            // Correct facing for randomness
            offsetfacing = RANDIE;
            offsetfacing = offsetfacing & pipfacingrand[pip];
            offsetfacing -= ( pipfacingrand[pip] >> 1 );
        }

        facing += offsetfacing;
        prtfacing[iprt] = facing;
        facing = facing >> 2;

        // Location data from arguments
        newrand = RANDIE;
        x = x + turntocos[( facing+8192 )&TRIG_TABLE_MASK] * ( pipxyspacingbase[pip] + ( newrand & pipxyspacingrand[pip] ) );
        y = y + turntosin[( facing+8192 )&TRIG_TABLE_MASK] * ( pipxyspacingbase[pip] + ( newrand & pipxyspacingrand[pip] ) );
        if ( x < 0 )  x = 0;
        if ( x > meshedgex - 2 )  x = meshedgex - 2;
        if ( y < 0 )  y = 0;
        if ( y > meshedgey - 2 )  y = meshedgey - 2;

        prtxpos[iprt] = x;
        prtypos[iprt] = y;
        prtzpos[iprt] = z;

        // Velocity data
        xvel = turntocos[( facing+8192 )&TRIG_TABLE_MASK] * velocity;
        yvel = turntosin[( facing+8192 )&TRIG_TABLE_MASK] * velocity;
        newrand = RANDIE;
        zvel += pipzvelbase[pip] + ( newrand & pipzvelrand[pip] ) - ( pipzvelrand[pip] >> 1 );
        prtxvel[iprt] = xvel;
        prtyvel[iprt] = yvel;
        prtzvel[iprt] = zvel;

        // Template values
        prtbumpsize[iprt] = pipbumpsize[pip];
        prtbumpsizebig[iprt] = prtbumpsize[iprt] + ( prtbumpsize[iprt] >> 1 );
        prtbumpheight[iprt] = pipbumpheight[pip];
        prttype[iprt] = piptype[pip];

        // Image data
        newrand = RANDIE;
        prtrotate[iprt] = ( newrand & piprotaterand[pip] ) + piprotatebase[pip];
        prtrotateadd[iprt] = piprotateadd[pip];
        prtsize[iprt] = pipsizebase[pip];
        prtsizeadd[iprt] = pipsizeadd[pip];
        prtimage[iprt] = 0;
        newrand = RANDIE;
        prtimageadd[iprt] = pipimageadd[pip] + ( newrand & pipimageaddrand[pip] );
        prtimagestt[iprt] = INT_TO_FP8( pipimagebase[pip] );
        prtimagemax[iprt] = INT_TO_FP8( pipnumframes[pip] );
        prttime[iprt] = piptime[pip];
        if ( pipendlastframe[pip] && prtimageadd[iprt] != 0 )
        {
            if ( prttime[iprt] == 0 )
            {
                // Part time is set to 1 cycle
                prttime[iprt] = ( prtimagemax[iprt] / prtimageadd[iprt] ) - 1;
            }
            else
            {
                // Part time is used to give number of cycles
                prttime[iprt] = prttime[iprt] * ( ( prtimagemax[iprt] / prtimageadd[iprt] ) - 1 );
            }
        }

        // Set onwhichfan...
        prtonwhichfan[iprt]   = mesh_get_tile( prtxpos[iprt], prtypos[iprt] );
        prtonwhichblock[iprt] = mesh_get_block( prtxpos[iprt], prtypos[iprt] );

        // Damage stuff
        prtdamagebase[iprt] = pipdamagebase[pip];
        prtdamagerand[iprt] = pipdamagerand[pip];

        // Spawning data
        prtspawntime[iprt] = pipcontspawntime[pip];
        if ( prtspawntime[iprt] != 0 )
        {
            prtspawntime[iprt] = 1;
            if ( prtattachedtocharacter[iprt] != MAXCHR )
            {
                prtspawntime[iprt]++; // Because attachment takes an update before it happens
            }
        }

        // Sound effect
        play_particle_sound( iprt, pipsoundspawn[pip] );
    }

    return iprt;
}

//--------------------------------------------------------------------------------------------
Uint8 __prthitawall( Uint16 particle )
{
    // ZZ> This function returns nonzero if the particle hit a wall

    Uint32 fan;
    Uint8  retval = MESHFX_IMPASS | MESHFX_WALL;

    fan = mesh_get_tile( prtxpos[particle], prtypos[particle] );
    if ( INVALID_TILE != fan )
    {
        if ( pipbumpmoney[prtpip[particle]] )
        {
            retval = meshfx[fan] & ( MESHFX_IMPASS | MESHFX_WALL );
        }
        else
        {
            retval = meshfx[fan] & MESHFX_IMPASS;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( Uint16 character )
{
    // ZZ> This function makes sure a character has no attached particles
    Uint16 particle;

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        if ( prton[particle] && prtattachedtocharacter[particle] == character )
        {
            free_one_particle( particle );
        }
    }

    // Set the alert for disaffirmation ( wet torch )
    chr[character].ai.alert |= ALERTIF_DISAFFIRMED;
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( Uint16 character )
{
    // ZZ> This function returns the number of particles attached to the given character
    Uint16 cnt, particle;

    cnt = 0;
    particle = 0;

    while ( particle < maxparticles )
    {
        if ( prton[particle] && prtattachedtocharacter[particle] == character )
        {
            cnt++;
        }

        particle++;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles( Uint16 character )
{
    // ZZ> This function makes sure a character has all of it's particles
    Uint16 numberattached;
    Uint16 particle;

    numberattached = number_of_attached_particles( character );

    while ( numberattached < capattachedprtamount[chr[character].model] )
    {
        particle = spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, 0, chr[character].model, capattachedprttype[chr[character].model], character, GRIP_LAST + numberattached, chr[character].team, character, numberattached, MAXCHR );
        if ( particle != TOTALMAXPRT )
        {
            attach_particle_to_character( particle, character, prtgrip[particle] );
        }

        numberattached++;
    }

    // Set the alert for reaffirmation ( for exploding barrels with fire )
    chr[character].ai.alert |= ALERTIF_REAFFIRMED;
}

//--------------------------------------------------------------------------------------------
void move_particles( void )
{
    // ZZ> This is the particle physics function
    int cnt, tnc;
    Uint16 facing, pip, particle;
    float level;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        if ( !prton[cnt] ) continue;

        prtonwhichfan[cnt]   = mesh_get_tile ( prtxpos[cnt], prtypos[cnt] );
        prtonwhichblock[cnt] = mesh_get_block( prtxpos[cnt], prtypos[cnt] );
        prtlevel[cnt]      = get_level( prtxpos[cnt], prtypos[cnt], bfalse );

        // To make it easier
        pip = prtpip[cnt];

        // Animate particle
        prtimage[cnt] = ( prtimage[cnt] + prtimageadd[cnt] );
        if ( prtimage[cnt] >= prtimagemax[cnt] )
            prtimage[cnt] = 0;

        prtrotate[cnt] += prtrotateadd[cnt];
        if ( ( (int)prtsize[cnt] + (int)prtsizeadd[cnt] ) > (int)0x0000FFFF ) prtsize[cnt] = 0xFFFF;
        else if ( ( prtsize[cnt] + prtsizeadd[cnt] ) < 0 ) prtsize[cnt] = 0;
        else prtsize[cnt] += prtsizeadd[cnt];

        // Change dyna light values
        prtdynalightlevel[cnt] += pipdynalightleveladd[pip];
        prtdynalightfalloff[cnt] += pipdynalightfalloffadd[pip];

        // Make it sit on the floor...  Shift is there to correct for sprite size
        level = prtlevel[cnt] + ( prtsize[cnt] >> 9 );

        // Check floor collision and do iterative physics
        if ( ( prtzpos[cnt] < level && prtzvel[cnt] < 0.1f ) || ( prtzpos[cnt] < level - PRTLEVELFIX ) )
        {
            prtzpos[cnt] = level;
            prtxvel[cnt] = prtxvel[cnt] * noslipfriction;
            prtyvel[cnt] = prtyvel[cnt] * noslipfriction;
            if ( pipendground[pip] )  prttime[cnt] = 1;
            if ( prtzvel[cnt] < 0 )
            {
                if ( prtzvel[cnt] > -STOPBOUNCINGPART )
                {
                    // Make it not bounce
                    prtzpos[cnt] -= 0.0001f;
                }
                else
                {
                    // Make it bounce
                    prtzvel[cnt] = -prtzvel[cnt] * pipdampen[pip];
                    // Play the sound for hitting the floor [FSND]
                    play_particle_sound( cnt, pipsoundfloor[pip] );
                }
            }
        }
        else
        {
            if ( prtattachedtocharacter[cnt] == MAXCHR )
            {
                prtxpos[cnt] += prtxvel[cnt];
                if ( __prthitawall( cnt ) )
                {
                    // Play the sound for hitting a wall [WSND]
                    play_particle_sound( cnt, pipsoundwall[pip] );
                    prtxpos[cnt] -= prtxvel[cnt];
                    prtxvel[cnt] = ( -prtxvel[cnt] * pipdampen[pip] );
                    if ( pipendwall[pip] )
                    {
                        prttime[cnt] = 1;
                    }
                    else
                    {
                        // Change facing
                        facing = prtfacing[cnt];
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

                        prtfacing[cnt] = facing;
                    }
                }

                prtypos[cnt] += prtyvel[cnt];
                if ( __prthitawall( cnt ) )
                {
                    prtypos[cnt] -= prtyvel[cnt];
                    prtyvel[cnt] = ( -prtyvel[cnt] * pipdampen[pip] );
                    if ( pipendwall[pip] )
                    {
                        prttime[cnt] = 1;
                    }
                    else
                    {
                        // Change facing
                        facing = prtfacing[cnt];
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

                        prtfacing[cnt] = facing;
                    }
                }

                prtzpos[cnt] += prtzvel[cnt];
                prtzvel[cnt] += gravity;
            }
        }

        // Do homing
        if ( piphoming[pip] && prttarget[cnt] != MAXCHR )
        {
            if ( !chr[prttarget[cnt]].alive )
            {
                prttime[cnt] = 1;
            }
            else
            {
                if ( prtattachedtocharacter[cnt] == MAXCHR )
                {
                    prtxvel[cnt] = ( prtxvel[cnt] + ( ( chr[prttarget[cnt]].xpos - prtxpos[cnt] ) * piphomingaccel[pip] ) ) * piphomingfriction[pip];
                    prtyvel[cnt] = ( prtyvel[cnt] + ( ( chr[prttarget[cnt]].ypos - prtypos[cnt] ) * piphomingaccel[pip] ) ) * piphomingfriction[pip];
                    prtzvel[cnt] = ( prtzvel[cnt] + ( ( chr[prttarget[cnt]].zpos + ( chr[prttarget[cnt]].bumpheight >> 1 ) - prtzpos[cnt] ) * piphomingaccel[pip] ) );

                }
                if ( piprotatetoface[pip] )
                {
                    // Turn to face target
                    facing = ATAN2( chr[prttarget[cnt]].ypos - prtypos[cnt], chr[prttarget[cnt]].xpos - prtxpos[cnt] ) * 0xFFFF / ( TWO_PI );
                    facing += 32768;
                    prtfacing[cnt] = facing;
                }
            }
        }

        // Do speed limit on Z
        if ( prtzvel[cnt] < -pipspdlimit[pip] )  prtzvel[cnt] = -pipspdlimit[pip];

        // Spawn new particles if continually spawning
        if ( pipcontspawnamount[pip] > 0 )
        {
            prtspawntime[cnt]--;
            if ( prtspawntime[cnt] == 0 )
            {
                prtspawntime[cnt] = pipcontspawntime[pip];
                facing = prtfacing[cnt];
                tnc = 0;

                while ( tnc < pipcontspawnamount[pip] )
                {
                    particle = spawn_one_particle( prtxpos[cnt], prtypos[cnt], prtzpos[cnt],
                                                   facing, prtmodel[cnt], pipcontspawnpip[pip],
                                                   MAXCHR, GRIP_LAST, prtteam[cnt], prtchr[cnt], tnc, prttarget[cnt] );
                    if ( pipfacingadd[prtpip[cnt]] != 0 && particle != TOTALMAXPRT )
                    {
                        // Hack to fix velocity
                        prtxvel[particle] += prtxvel[cnt];
                        prtyvel[particle] += prtyvel[cnt];
                    }

                    facing += pipcontspawnfacingadd[pip];
                    tnc++;
                }
            }
        }

        // Check underwater
        if ( prtzpos[cnt] < waterdouselevel && pipendwater[pip] && INVALID_TILE != prtonwhichfan[cnt] && 0 != ( meshfx[prtonwhichfan[cnt]] & MESHFX_WATER ) )
        {
            // Splash for particles is just a ripple
            spawn_one_particle( prtxpos[cnt], prtypos[cnt], watersurfacelevel,
                                0, MAXMODEL, RIPPLE, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );

            // Check for disaffirming character
            if ( prtattachedtocharacter[cnt] != MAXCHR && prtchr[cnt] == prtattachedtocharacter[cnt] )
            {
                // Disaffirm the whole character
                disaffirm_attached_particles( prtattachedtocharacter[cnt] );
            }
            else
            {
                // Just destroy the particle
                //                    free_one_particle(cnt);
                prttime[cnt] = 1;
            }
        }

        //            else
        //            {
        // Spawn new particles if time for old one is up
        if ( prttime[cnt] != 0 )
        {
            prttime[cnt]--;
            if ( prttime[cnt] == 0 )
            {
                facing = prtfacing[cnt];
                tnc = 0;

                while ( tnc < pipendspawnamount[pip] )
                {
                    spawn_one_particle( prtxpos[cnt] - prtxvel[cnt], prtypos[cnt] - prtyvel[cnt], prtzpos[cnt],
                                        facing, prtmodel[cnt], pipendspawnpip[pip],
                                        MAXCHR, GRIP_LAST, prtteam[cnt], prtchr[cnt], tnc, prttarget[cnt] );
                    facing += pipendspawnfacingadd[pip];
                    tnc++;
                }

                free_one_particle( cnt );
            }
        }

        //            }
        prtfacing[cnt] += pipfacingadd[pip];
    }
}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
    // ZZ> This function attaches particles to their characters so everything gets
    //     drawn right
    int cnt;

    cnt = 0;

    while ( cnt < maxparticles )
    {
        if ( prton[cnt] && prtattachedtocharacter[cnt] != MAXCHR )
        {
            attach_particle_to_character( cnt, prtattachedtocharacter[cnt], prtgrip[cnt] );

            // Correct facing so swords knock characters in the right direction...
            if ( pipdamfx[prtpip[cnt]]&DAMFXTURN )
                prtfacing[cnt] = chr[prtattachedtocharacter[cnt]].turnleftright;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void free_all_particles()
{
    // ZZ> This function resets the particle allocation lists
    numfreeprt = 0;

    while ( numfreeprt < maxparticles )
    {
        prton[numfreeprt] = 0;
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
Uint16 terp_dir( Uint16 majordir, Uint16 minordir )
{
    // ZZ> This function returns a direction between the major and minor ones, closer
    //     to the major.
    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;
    if ( minordir > 32768 )
    {
        temp = 0xFFFF;
        minordir = ( minordir + ( temp << 3 ) - temp ) >> 3;
        minordir += majordir;
        return minordir;
    }

    temp = 0;
    minordir = ( minordir + ( temp << 3 ) - temp ) >> 3;
    minordir += majordir;
    return minordir;
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir )
{
    // ZZ> This function returns a direction between the major and minor ones, closer
    //     to the major, but not by much.  Makes turning faster.
    Uint16 temp;

    // Align major direction with 0
    minordir -= majordir;
    if ( minordir > 32768 )
    {
        temp = 0xFFFF;
        minordir = ( minordir + ( temp << 1 ) - temp ) >> 1;
        minordir += majordir;
        return minordir;
    }

    temp = 0;
    minordir = ( minordir + ( temp << 1 ) - temp ) >> 1;
    minordir += majordir;
    return minordir;
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

    pip = prtpip[particle];
    amount = pipbumpspawnamount[pip];
    if ( amount != 0 || pipspawnenchant[pip] )
    {
        // Only damage if hitting from proper direction
        model = chr[character].model;
        vertices = madvertices[model];
        direction = ( ATAN2( prtyvel[particle], prtxvel[particle] ) + PI ) * 0xFFFF / ( TWO_PI );
        direction = chr[character].turnleftright - direction + 32768;
        if ( madframefx[chr[character].frame]&MADFXINVICTUS )
        {
            // I Frame
            if ( pipdamfx[pip]&DAMFXBLOC )
            {
                left = 0xFFFF;
                right = 0;
            }
            else
            {
                direction -= capiframefacing[model];
                left = ( ~capiframeangle[model] );
                right = capiframeangle[model];
            }
        }
        else
        {
            // N Frame
            direction -= capnframefacing[model];
            left = ( ~capnframeangle[model] );
            right = capnframeangle[model];
        }

        // Check that direction
        if ( direction <= left && direction >= right )
        {
            // Spawn new enchantments
            if ( pipspawnenchant[pip] )
            {
                spawn_enchant( prtchr[particle], character, MAXCHR, MAXENCHANT, prtmodel[particle] );
            }

            // Spawn particles
            if ( amount != 0 && !capresistbumpspawn[chr[character].model] && !chr[character].invictus && vertices != 0 && ( chr[character].damagemodifier[prtdamagetype[particle]]&DAMAGESHIFT ) < 3 )
            {
                if ( amount == 1 )
                {
                    // A single particle ( arrow? ) has been stuck in the character...
                    // Find best vertex to attach to
                    bestvertex = 0;
                    bestdistance = 9999999;
                    z = -chr[character].zpos + prtzpos[particle] + RAISE;
                    facing = prtfacing[particle] - chr[character].turnleftright - 16384;
                    facing = facing >> 2;
                    fsin = turntosin[facing];
                    fcos = turntocos[facing];
                    y = 8192;
                    x = -y * fsin;
                    y = y * fcos;
                    z = z << 10;/// chr[character].scale;
                    frame = madframestart[chr[character].model];
                    cnt = 0;

                    while ( cnt < vertices )
                    {
                        distance = ABS( x - madvrtx[frame][vertices-cnt-1] ) + ABS( y - madvrty[frame][vertices-cnt-1] ) + ( ABS( z - madvrtz[frame][vertices-cnt-1] ) );
                        if ( distance < bestdistance )
                        {
                            bestdistance = distance;
                            bestvertex = cnt;
                        }

                        cnt++;
                    }

                    spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, 0, prtmodel[particle], pipbumpspawnpip[pip],
                                        character, bestvertex + 1, prtteam[particle], prtchr[particle], cnt, character );
                }
                else
                {
                    amount = ( amount * vertices ) >> 5;  // Correct amount for size of character
                    cnt = 0;

                    while ( cnt < amount )
                    {
                        spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, 0, prtmodel[particle], pipbumpspawnpip[pip],
                                            character, rand() % vertices, prtteam[particle], prtchr[particle], cnt, character );
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
        fan = mesh_get_tile( prtxpos[cnt], prtypos[cnt] );
        if ( INVALID_TILE != fan )
        {
            if ( 0 != ( meshfx[fan] & MESHFX_WATER ) )  return btrue;
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn()
{
    // ZZ> This function drops snowflakes or rain or whatever, also swings the camera
    int particle, cnt;
    float x, y, z;
    bool_t foundone;
    if ( weathertime > 0 )
    {
        weathertime--;
        if ( weathertime == 0 )
        {
            weathertime = weathertimereset;

            // Find a valid player
            foundone = bfalse;
            cnt = 0;

            while ( cnt < MAXPLAYER )
            {
                weatherplayer = ( weatherplayer + 1 ) & ( MAXPLAYER - 1 );
                if ( plavalid[weatherplayer] )
                {
                    foundone = btrue;
                    cnt = MAXPLAYER;
                }

                cnt++;
            }

            // Did we find one?
            if ( foundone )
            {
                // Yes, but is the character valid?
                cnt = plaindex[weatherplayer];
                if ( chr[cnt].on && !chr[cnt].inpack )
                {
                    // Yes, so spawn over that character
                    x = chr[cnt].xpos;
                    y = chr[cnt].ypos;
                    z = chr[cnt].zpos;
                    particle = spawn_one_particle( x, y, z, 0, MAXMODEL, WEATHER4, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    if ( weatheroverwater && particle != TOTALMAXPRT )
                    {
                        if ( !prt_is_over_water( particle ) )
                        {
                            free_one_particle_no_sound( particle );
                        }
                    }

                }
            }
        }
    }

    camswing = ( camswing + camswingrate ) & 16383;
}

//--------------------------------------------------------------------------------------------
int load_one_particle(  const char *szLoadName, Uint16 object, Uint16 pip )
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
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipforce[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  pipforce[numpip] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        if ( cTmp == 'L' || cTmp == 'l' )  piptype[numpip] = PRTLIGHTSPRITE;
        if ( cTmp == 'S' || cTmp == 's' )  piptype[numpip] = PRTSOLIDSPRITE;
        if ( cTmp == 'T' || cTmp == 't' )  piptype[numpip] = PRTALPHASPRITE;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipimagebase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipnumframes[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipimageadd[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipimageaddrand[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); piprotatebase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); piprotaterand[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); piprotateadd[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipsizebase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipsizeadd[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); pipspdlimit[numpip] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipfacingadd[numpip] = iTmp;

        // Ending conditions
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipendwater[numpip] = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  pipendwater[numpip] = bfalse;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipendbump[numpip] = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  pipendbump[numpip] = bfalse;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipendground[numpip] = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  pipendground[numpip] = bfalse;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipendlastframe[numpip] = btrue;
        if ( cTmp == 'F' || cTmp == 'f' )  pipendlastframe[numpip] = bfalse;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); piptime[numpip] = iTmp;

        // Collision data
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); pipdampen[numpip] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipbumpmoney[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipbumpsize[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipbumpheight[numpip] = iTmp;
        goto_colon( fileread );  read_pair( fileread );
        pipdamagebase[numpip] = pairbase;
        pipdamagerand[numpip] = pairrand;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        if ( cTmp == 'S' || cTmp == 's' ) pipdamagetype[numpip] = DAMAGE_SLASH;
        if ( cTmp == 'C' || cTmp == 'c' ) pipdamagetype[numpip] = DAMAGE_CRUSH;
        if ( cTmp == 'P' || cTmp == 'p' ) pipdamagetype[numpip] = DAMAGE_POKE;
        if ( cTmp == 'H' || cTmp == 'h' ) pipdamagetype[numpip] = DAMAGE_HOLY;
        if ( cTmp == 'E' || cTmp == 'e' ) pipdamagetype[numpip] = DAMAGE_EVIL;
        if ( cTmp == 'F' || cTmp == 'f' ) pipdamagetype[numpip] = DAMAGE_FIRE;
        if ( cTmp == 'I' || cTmp == 'i' ) pipdamagetype[numpip] = DAMAGE_ICE;
        if ( cTmp == 'Z' || cTmp == 'z' ) pipdamagetype[numpip] = DAMAGE_ZAP;

        // Lighting data
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipdynalightmode[numpip] = DYNAOFF;
        if ( cTmp == 'T' || cTmp == 't' ) pipdynalightmode[numpip] = DYNAON;
        if ( cTmp == 'L' || cTmp == 'l' ) pipdynalightmode[numpip] = DYNALOCAL;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); pipdynalevel[numpip] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipdynafalloff[numpip] = iTmp;
        if ( pipdynafalloff[numpip] > MAXFALLOFF && rtscontrol )  pipdynafalloff[numpip] = MAXFALLOFF;

        // Initial spawning of this particle
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipfacingbase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipfacingrand[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipxyspacingbase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipxyspacingrand[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipzspacingbase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipzspacingrand[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipxyvelbase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipxyvelrand[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipzvelbase[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipzvelrand[numpip] = iTmp;

        // Continuous spawning of other particles
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipcontspawntime[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipcontspawnamount[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipcontspawnfacingadd[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipcontspawnpip[numpip] = iTmp;

        // End spawning of other particles
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipendspawnamount[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipendspawnfacingadd[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipendspawnpip[numpip] = iTmp;

        // Bump spawning of attached particles
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipbumpspawnamount[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipbumpspawnpip[numpip] = iTmp;

        // Random stuff  !!!BAD!!! Not complete
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipdazetime[numpip] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); pipgrogtime[numpip] = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipspawnenchant[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) pipspawnenchant[numpip] = btrue;

        goto_colon( fileread );  // !!Cause roll
        goto_colon( fileread );  // !!Cause pancake
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipneedtarget[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) pipneedtarget[numpip] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        piptargetcaster[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) piptargetcaster[numpip] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipstartontarget[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) pipstartontarget[numpip] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        piponlydamagefriendly[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) piponlydamagefriendly[numpip] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        pipsoundspawn[numpip] = CLIP(iTmp, -1, MAXWAVE);

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
        pipsoundend[numpip] = CLIP(iTmp, -1, MAXWAVE);

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipfriendlyfire[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) pipfriendlyfire[numpip] = btrue;   //piphateonly[numpip] = bfalse; TODO: BAD not implemented yet

        goto_colon( fileread );
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        pipnewtargetonspawn[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) pipnewtargetonspawn[numpip] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); piptargetangle[numpip] = iTmp >> 1;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        piphoming[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) piphoming[numpip] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); piphomingfriction[numpip] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp ); piphomingaccel[numpip] = fTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        piprotatetoface[numpip] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' ) piprotatetoface[numpip] = btrue;

        // Clear expansions...
        pipzaimspd[numpip] = 0;
        pipsoundfloor[numpip] = -1;
        pipsoundwall[numpip] = -1;
        pipendwall[numpip] = pipendground[numpip];
        pipdamfx[numpip] = DAMFXTURN;
        if ( piphoming[numpip] )  pipdamfx[numpip] = DAMFXNONE;

        pipallowpush[numpip] = btrue;
        pipdynalightfalloffadd[numpip] = 0;
        pipdynalightleveladd[numpip] = 0;
        pipintdamagebonus[numpip] = bfalse;
        pipwisdamagebonus[numpip] = bfalse;

        // Read expansions
        while ( goto_colon_yesno( fileread ) )
        {
            idsz = get_idsz( fileread );
            fscanf( fileread, "%c%d", &cTmp, &iTmp );
            test = Make_IDSZ( "TURN" );  // [TURN]
            if ( idsz == test )  pipdamfx[numpip] = DAMFXNONE;

            test = Make_IDSZ( "ZSPD" );  // [ZSPD]
            if ( idsz == test )  pipzaimspd[numpip] = iTmp;

            test = Make_IDSZ( "FSND" );  // [FSND]
            if ( idsz == test )  pipsoundfloor[numpip] = iTmp;

            test = Make_IDSZ( "WSND" );  // [WSND]
            if ( idsz == test )  pipsoundwall[numpip] = iTmp;

            test = Make_IDSZ( "WEND" );  // [WEND]
            if ( idsz == test )  pipendwall[numpip] = iTmp;

            test = Make_IDSZ( "ARMO");  // [ARMO]
            if ( idsz == test )  pipdamfx[numpip] |= DAMFXARMO;

            test = Make_IDSZ( "BLOC" );  // [BLOC]
            if ( idsz == test )  pipdamfx[numpip] |= DAMFXBLOC;

            test = Make_IDSZ( "ARRO" );  // [ARRO]
            if ( idsz == test )  pipdamfx[numpip] |= DAMFXARRO;

            test = Make_IDSZ( "TIME" );  // [TIME]
            if ( idsz == test )  pipdamfx[numpip] |= DAMFXTIME;

            test = Make_IDSZ( "PUSH" );  // [PUSH]
            if ( idsz == test )  pipallowpush[numpip] = iTmp;

            test = Make_IDSZ( "DLEV" );  // [DLEV]
            if ( idsz == test )  pipdynalightleveladd[numpip] = iTmp / 1000.0f;

            test = Make_IDSZ( "DRAD" );  // [DRAD]
            if ( idsz == test )  pipdynalightfalloffadd[numpip] = iTmp / 1000.0f;

            test = Make_IDSZ( "IDAM");  // [IDAM]
            if ( idsz == test )  pipintdamagebonus[numpip] = iTmp;

            test = Make_IDSZ( "WDAM" );  // [WDAM]
            if ( idsz == test )  pipwisdamagebonus[numpip] = iTmp;
        }

        // Make sure it's referenced properly
        madprtpip[object][pip] = numpip;
        numpip++;

        fclose( fileread );
        return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void reset_particles(  const char* modname )
{
    // ZZ> This resets all particle data and reads in the coin and water particles
    int cnt, object;
    char newloadname[256];
    char *loadpath;

    // Load in the standard global particles ( the coins for example )
    // BAD! This should only be needed once at the start of the game
    numpip = 0;
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "1money.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "5money.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "25money.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)", loadpath );
    }

    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "100money.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)", loadpath );
    }

    // Load module specific information
    make_newloadname( modname, "gamedat" SLASH_STR "weather4.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "weather5.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)", newloadname );
    }

    make_newloadname( modname, "gamedat" SLASH_STR "splash.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        if (gDevMode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "splash.txt";
        if ( !load_one_particle( loadpath, 0, 0 ) )
        {
            log_error( "Data file was not found! (%s)", loadpath );
        }
    }

    make_newloadname( modname, "gamedat" SLASH_STR "ripple.txt", newloadname );
    if ( !load_one_particle( newloadname, 0, 0 ) )
    {
        if (gDevMode) log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );

        loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "ripple.txt";
        if ( !load_one_particle( loadpath, 0, 0 ) )
        {
            log_error( "Data file was not found! (%s)", loadpath );
        }
    }

    // This is also global...
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "defend.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
        log_error( "Data file was not found! (%s)", loadpath );
    }

    // Now clear out the local pips
    object = 0;
    while ( object < MAXMODEL )
    {
        cnt = 0;

        while ( cnt < MAXPRTPIPPEROBJECT )
        {
            madprtpip[object][cnt] = 0;
            cnt++;
        }

        object++;
    }
}

