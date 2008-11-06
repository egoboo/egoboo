/* Egoboo - particle.c
 * Manages particle systems.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "Log.h"
#include "particle.h"

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
  while ( cnt < MAXPRT )
  {
    prtinview[cnt] = bfalse;
    if ( prton[cnt] )
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
          if ( numdynalight < MAXDYNA )
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
            while ( tnc < MAXDYNA )
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
            while ( tnc < MAXDYNA )
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
void free_one_particle_no_sound( int particle )
{
  // ZZ> This function sticks a particle back on the free particle stack
  freeprtlist[numfreeprt] = particle;
  numfreeprt++;
  prton[particle] = bfalse;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( int particle, Sint8 sound )
{
  // This function plays a sound effect for a particle
  if ( sound != -1 )
  {
    if ( prtmodel[particle] != MAXMODEL ) play_sound( prtxpos[particle], prtypos[particle], capwaveindex[prtmodel[particle]][sound] );
    else  play_sound( prtxpos[particle], prtypos[particle], globalwave[sound] );
  }
}

//--------------------------------------------------------------------------------------------
void free_one_particle( int particle )
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
      chraistate[child] = prtspawncharacterstate[particle];
      chraiowner[child] = prtchr[particle];
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


  // Return MAXPRT if we can't find one
  particle = MAXPRT;
  if ( numfreeprt == 0 )
  {
    if ( force )
    {
      // Gotta find one, so go through the list
      particle = 0;
      while ( particle < MAXPRT )
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
    if ( force || numfreeprt > ( MAXPRT / 4 ) )
    {
      // Just grab the next one
      numfreeprt--;
      particle = freeprtlist[numfreeprt];
    }
  }
  return particle;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 pip,
                           Uint16 characterattach, Uint16 grip, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget )
{
  // ZZ> This function spawns a new particle, and returns the number of that particle
  int cnt, velocity;
  float xvel, yvel, zvel, tvel;
  int offsetfacing, newrand;


  // Convert from local pip to global pip
  if ( model < MAXMODEL )
    pip = madprtpip[model][pip];


  cnt = get_free_particle( pipforce[pip] );
  if ( cnt != MAXPRT )
  {
    // Necessary data for any part
    prton[cnt] = btrue;
    prtpip[cnt] = pip;
    prtmodel[cnt] = model;
    prtinview[cnt] = bfalse;
    prtlevel[cnt] = 0;
    prtteam[cnt] = team;
    prtchr[cnt] = characterorigin;
    prtdamagetype[cnt] = pipdamagetype[pip];
    prtspawncharacterstate[cnt] = SPAWNNOCHARACTER;


    // Lighting and sound
    prtdynalighton[cnt] = bfalse;
    if ( multispawn == 0 )
    {
      prtdynalighton[cnt] = pipdynalightmode[pip];
      if ( pipdynalightmode[pip] == DYNALOCAL )
      {
        prtdynalighton[cnt] = bfalse;
      }
    }
    prtdynalightlevel[cnt] = pipdynalevel[pip];
    prtdynalightfalloff[cnt] = pipdynafalloff[pip];



    // Set character attachments ( characterattach==MAXCHR means none )
    prtattachedtocharacter[cnt] = characterattach;
    prtgrip[cnt] = grip;



    // Correct facing
    facing += pipfacingbase[pip];


    // Targeting...
    zvel = 0;
    newrand = RANDIE;
    z = z + pipzspacingbase[pip] + ( newrand & pipzspacingrand[pip] ) - ( pipzspacingrand[pip] >> 1 );
    newrand = RANDIE;
    velocity = ( pipxyvelbase[pip] + ( newrand & pipxyvelrand[pip] ) );
    prttarget[cnt] = oldtarget;
    if ( pipnewtargetonspawn[pip] )
    {
      if ( piptargetcaster[pip] )
      {
        // Set the target to the caster
        prttarget[cnt] = characterorigin;
      }
      else
      {
        // Find a target
        prttarget[cnt] = find_target( x, y, facing, piptargetangle[pip], piponlydamagefriendly[pip], bfalse, team, characterorigin, oldtarget );
        if ( prttarget[cnt] != MAXCHR && !piphoming[pip] )
        {
          facing = facing - glouseangle;
        }
        // Correct facing for dexterity...
        offsetfacing = 0;
        if ( chrdexterity[characterorigin] < PERFECTSTAT )
        {
          // Correct facing for randomness
          offsetfacing = RANDIE;
          offsetfacing = offsetfacing & pipfacingrand[pip];
          offsetfacing -= ( pipfacingrand[pip] >> 1 );
          offsetfacing = ( offsetfacing * ( PERFECTSTAT - chrdexterity[characterorigin] ) ) / PERFECTSTAT;  // Divided by PERFECTSTAT
        }
        if ( prttarget[cnt] != MAXCHR && pipzaimspd[pip] != 0 )
        {
          // These aren't velocities...  This is to do aiming on the Z axis
          if ( velocity > 0 )
          {
            xvel = chrxpos[prttarget[cnt]] - x;
            yvel = chrypos[prttarget[cnt]] - y;
            tvel = SQRT( xvel * xvel + yvel * yvel ) / velocity;  // This is the number of steps...
            if ( tvel > 0 )
            {
              zvel = ( chrzpos[prttarget[cnt]] + ( chrbumpsize[prttarget[cnt]] >> 1 ) - z ) / tvel;  // This is the zvel alteration
              if ( zvel < -( pipzaimspd[pip] >> 1 ) ) zvel = -( pipzaimspd[pip] >> 1 );
              if ( zvel > pipzaimspd[pip] ) zvel = pipzaimspd[pip];
            }
          }
        }
      }
      // Does it go away?
      if ( prttarget[cnt] == MAXCHR && pipneedtarget[pip] )
      {
        free_one_particle( cnt );
        return MAXPRT;
      }
      // Start on top of target
      if ( prttarget[cnt] != MAXCHR && pipstartontarget[pip] )
      {
        x = chrxpos[prttarget[cnt]];
        y = chrypos[prttarget[cnt]];
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
    prtfacing[cnt] = facing;
    facing = facing >> 2;


    // Location data from arguments
    newrand = RANDIE;
    x = x + turntosin[( facing+12288 )&16383] * ( pipxyspacingbase[pip] + ( newrand & pipxyspacingrand[pip] ) );
    y = y + turntosin[( facing+8192 )&16383] * ( pipxyspacingbase[pip] + ( newrand & pipxyspacingrand[pip] ) );
    if ( x < 0 )  x = 0;
    if ( x > meshedgex - 2 )  x = meshedgex - 2;
    if ( y < 0 )  y = 0;
    if ( y > meshedgey - 2 )  y = meshedgey - 2;
    prtxpos[cnt] = x;
    prtypos[cnt] = y;
    prtzpos[cnt] = z;


    // Velocity data
    xvel = turntosin[( facing+12288 )&16383] * velocity;
    yvel = turntosin[( facing+8192 )&16383] * velocity;
    newrand = RANDIE;
    zvel += pipzvelbase[pip] + ( newrand & pipzvelrand[pip] ) - ( pipzvelrand[pip] >> 1 );
    prtxvel[cnt] = xvel;
    prtyvel[cnt] = yvel;
    prtzvel[cnt] = zvel;


    // Template values
    prtbumpsize[cnt] = pipbumpsize[pip];
    prtbumpsizebig[cnt] = prtbumpsize[cnt] + ( prtbumpsize[cnt] >> 1 );
    prtbumpheight[cnt] = pipbumpheight[pip];
    prttype[cnt] = piptype[pip];


    // Image data
    newrand = RANDIE;
    prtrotate[cnt] = ( newrand & piprotaterand[pip] ) + piprotatebase[pip];
    prtrotateadd[cnt] = piprotateadd[pip];
    prtsize[cnt] = pipsizebase[pip];
    prtsizeadd[cnt] = pipsizeadd[pip];
    prtimage[cnt] = 0;
    newrand = RANDIE;
    prtimageadd[cnt] = pipimageadd[pip] + ( newrand & pipimageaddrand[pip] );
    prtimagestt[cnt] = pipimagebase[pip] << 8;
    prtimagemax[cnt] = pipnumframes[pip] << 8;
    prttime[cnt] = piptime[pip];
    if ( pipendlastframe[pip] && prtimageadd[cnt] != 0 )
    {
      if ( prttime[cnt] == 0 )
      {
        // Part time is set to 1 cycle
        prttime[cnt] = ( prtimagemax[cnt] / prtimageadd[cnt] ) - 1;
      }
      else
      {
        // Part time is used to give number of cycles
        prttime[cnt] = prttime[cnt] * ( ( prtimagemax[cnt] / prtimageadd[cnt] ) - 1 );
      }
    }


    // Set onwhichfan...
    prtonwhichfan[cnt] = OFFEDGE;
    if ( prtxpos[cnt] > 0 && prtxpos[cnt] < meshedgex && prtypos[cnt] > 0 && prtypos[cnt] < meshedgey )
    {
      prtonwhichfan[cnt] = meshfanstart[( ( int )prtypos[cnt] ) >> 7] + ( ( ( int )prtxpos[cnt] ) >> 7 );
    }


    // Damage stuff
    prtdamagebase[cnt] = pipdamagebase[pip];
    prtdamagerand[cnt] = pipdamagerand[pip];



    // Spawning data
    prtspawntime[cnt] = pipcontspawntime[pip];
    if ( prtspawntime[cnt] != 0 )
    {
      prtspawntime[cnt] = 1;
      if ( prtattachedtocharacter[cnt] != MAXCHR )
      {
        prtspawntime[cnt]++; // Because attachment takes an update before it happens
      }
    }


    // Sound effect
    play_particle_sound( cnt, pipsoundspawn[pip] );
  }
  return cnt;
}

//--------------------------------------------------------------------------------------------
Uint8 __prthitawall( int particle )
{
  // ZZ> This function returns nonzero if the particle hit a wall
  int x, y;

  y = prtypos[particle];  x = prtxpos[particle];
  // !!!BAD!!! Should really do bound checking...
  if ( pipbumpmoney[prtpip[particle]] )
  {
    return ( ( meshfx[meshfanstart[y>>7] + ( x >> 7 )] )&( MESHFXIMPASS | MESHFXWALL ) );
  }
  else
  {
    return ( ( meshfx[meshfanstart[y>>7] + ( x >> 7 )] )&( MESHFXIMPASS ) );
  }
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( Uint16 character )
{
  // ZZ> This function makes sure a character has no attached particles
  Uint16 particle;

  particle = 0;
  while ( particle < MAXPRT )
  {
    if ( prton[particle] && prtattachedtocharacter[particle] == character )
    {
      free_one_particle( particle );
    }
    particle++;
  }

  // Set the alert for disaffirmation ( wet torch )
  chralert[character] |= ALERTIFDISAFFIRMED;
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( Uint16 character )
{
  // ZZ> This function returns the number of particles attached to the given character
  Uint16 cnt, particle;

  cnt = 0;
  particle = 0;
  while ( particle < MAXPRT )
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
  while ( numberattached < capattachedprtamount[chrmodel[character]] )
  {
    particle = spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], 0, chrmodel[character], capattachedprttype[chrmodel[character]], character, SPAWNLAST + numberattached, chrteam[character], character, numberattached, MAXCHR );
    if ( particle != MAXPRT )
    {
      attach_particle_to_character( particle, character, prtgrip[particle] );
    }
    numberattached++;
  }

  // Set the alert for reaffirmation ( for exploding barrels with fire )
  chralert[character] = chralert[character] | ALERTIFREAFFIRMED;
}


//--------------------------------------------------------------------------------------------
void move_particles( void )
{
  // ZZ> This is the particle physics function
  int cnt, tnc, x, y, fan;
  Uint16 facing, pip, particle;
  float level;

  cnt = 0;
  while ( cnt < MAXPRT )
  {
    if ( prton[cnt] )
    {
      prtonwhichfan[cnt] = OFFEDGE;
      prtlevel[cnt] = 0;
      if ( prtxpos[cnt] > 0 && prtxpos[cnt] < meshedgex && prtypos[cnt] > 0 && prtypos[cnt] < meshedgey )
      {
        x = prtxpos[cnt];
        y = prtypos[cnt];
        x = x >> 7;
        y = y >> 7;
        fan = meshfanstart[y] + x;
        prtonwhichfan[cnt] = fan;
        prtlevel[cnt] = get_level( prtxpos[cnt], prtypos[cnt], fan, bfalse );
      }


      // To make it easier
      pip = prtpip[cnt];

      // Animate particle
      prtimage[cnt] = ( prtimage[cnt] + prtimageadd[cnt] );
      if ( prtimage[cnt] >= prtimagemax[cnt] )
        prtimage[cnt] = 0;
      prtrotate[cnt] += prtrotateadd[cnt];
      if ( ( prtsize[cnt] + prtsizeadd[cnt] ) > 65535 ) prtsize[cnt] = 65535;
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
        if ( !chralive[prttarget[cnt]] )
        {
          prttime[cnt] = 1;
        }
        else
        {
          if ( prtattachedtocharacter[cnt] == MAXCHR )
          {
            prtxvel[cnt] = ( prtxvel[cnt] + ( ( chrxpos[prttarget[cnt]] - prtxpos[cnt] ) * piphomingaccel[pip] ) ) * piphomingfriction[pip];
            prtyvel[cnt] = ( prtyvel[cnt] + ( ( chrypos[prttarget[cnt]] - prtypos[cnt] ) * piphomingaccel[pip] ) ) * piphomingfriction[pip];
            prtzvel[cnt] = ( prtzvel[cnt] + ( ( chrzpos[prttarget[cnt]] + ( chrbumpheight[prttarget[cnt]] >> 1 ) - prtzpos[cnt] ) * piphomingaccel[pip] ) );

          }
          if ( piprotatetoface[pip] )
          {
            // Turn to face target
            facing = ATAN2( chrypos[prttarget[cnt]] - prtypos[cnt], chrxpos[prttarget[cnt]] - prtxpos[cnt] ) * 65535 / ( TWO_PI );
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
                                           MAXCHR, SPAWNLAST, prtteam[cnt], prtchr[cnt], tnc, prttarget[cnt] );
            if ( pipfacingadd[prtpip[cnt]] != 0 && particle < MAXPRT )
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
      if ( prtzpos[cnt] < waterdouselevel && ( meshfx[prtonwhichfan[cnt]]&MESHFXWATER ) && pipendwater[pip] )
      {
        // Splash for particles is just a ripple
        spawn_one_particle( prtxpos[cnt], prtypos[cnt], watersurfacelevel,
                            0, MAXMODEL, RIPPLE, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR );


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
                                MAXCHR, SPAWNLAST, prtteam[cnt], prtchr[cnt], tnc, prttarget[cnt] );
            facing += pipendspawnfacingadd[pip];
            tnc++;
          }
          free_one_particle( cnt );
        }
      }
//            }
      prtfacing[cnt] += pipfacingadd[pip];
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
  // ZZ> This function attaches particles to their characters so everything gets
  //     drawn right
  int cnt;

  cnt = 0;
  while ( cnt < MAXPRT )
  {
    if ( prton[cnt] && prtattachedtocharacter[cnt] != MAXCHR )
    {
      attach_particle_to_character( cnt, prtattachedtocharacter[cnt], prtgrip[cnt] );
      // Correct facing so swords knock characters in the right direction...
      if ( pipdamfx[prtpip[cnt]]&DAMFXTURN )
        prtfacing[cnt] = chrturnleftright[prtattachedtocharacter[cnt]];
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void free_all_particles()
{
  // ZZ> This function resets the particle allocation lists
  numfreeprt = 0;
  while ( numfreeprt < MAXPRT )
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
    temp = 65535;
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
    temp = 65535;
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
    model = chrmodel[character];
    vertices = madvertices[model];
    direction = ( ATAN2( prtyvel[particle], prtxvel[particle] ) + PI ) * 65535 / ( TWO_PI );
    direction = chrturnleftright[character] - direction + 32768;
    if ( madframefx[chrframe[character]]&MADFXINVICTUS )
    {
      // I Frame
      if ( pipdamfx[pip]&DAMFXBLOC )
      {
        left = 65535;
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
      if ( amount != 0 && !capresistbumpspawn[chrmodel[character]] && !chrinvictus[character] && vertices != 0 && ( chrdamagemodifier[character][prtdamagetype[particle]]&DAMAGESHIFT ) < 3 )
      {
        if ( amount == 1 )
        {
          // A single particle ( arrow? ) has been stuck in the character...
          // Find best vertex to attach to
          bestvertex = 0;
          bestdistance = 9999999;
          z = -chrzpos[character] + prtzpos[particle] + RAISE;
          facing = prtfacing[particle] - chrturnleftright[character] - 16384;
          facing = facing >> 2;
          fsin = turntosin[facing];
          fcos = turntosin[( facing+4096 )&16383];
          y = 8192;
          x = -y * fsin;
          y = y * fcos;
          z = z << 10;/// chrscale[character];
          frame = madframestart[chrmodel[character]];
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
          spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], 0, prtmodel[particle], pipbumpspawnpip[pip],
                              character, bestvertex + 1, prtteam[particle], prtchr[particle], cnt, character );
        }
        else
        {
          amount = ( amount * vertices ) >> 5;  // Correct amount for size of character
          cnt = 0;
          while ( cnt < amount )
          {
            spawn_one_particle( chrxpos[character], chrypos[character], chrzpos[character], 0, prtmodel[particle], pipbumpspawnpip[pip],
                                character, rand() % vertices, prtteam[particle], prtchr[particle], cnt, character );
            cnt++;
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
int prt_is_over_water( int cnt )
{
  // This function returns btrue if the particle is over a water tile
  int x, y, fan;


  if ( cnt < MAXPRT )
  {
    if ( prtxpos[cnt] > 0 && prtxpos[cnt] < meshedgex && prtypos[cnt] > 0 && prtypos[cnt] < meshedgey )
    {
      x = prtxpos[cnt];
      y = prtypos[cnt];
      x = x >> 7;
      y = y >> 7;
      fan = meshfanstart[y] + x;
      if ( meshfx[fan]&MESHFXWATER )  return btrue;
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
  Uint8 foundone;


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
        if ( chron[cnt] && !chrinpack[cnt] )
        {
          // Yes, so spawn over that character
          x = chrxpos[cnt];
          y = chrypos[cnt];
          z = chrzpos[cnt];
          particle = spawn_one_particle( x, y, z, 0, MAXMODEL, WEATHER4, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR );
          if ( weatheroverwater && particle != MAXPRT )
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
int load_one_particle( char *szLoadName, int object, int pip )
{
  // ZZ> This function loads a particle template, returning bfalse if the file wasn't
  //     found
  FILE* fileread;
  int test, idsz;
  int iTmp;
  float fTmp;
  char cTmp;


  fileread = fopen( szLoadName, "r" );
  if ( fileread )
  {
    // General data
    globalname = szLoadName;
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
    if ( cTmp == 'S' || cTmp == 's' ) pipdamagetype[numpip] = DAMAGESLASH;
    if ( cTmp == 'C' || cTmp == 'c' ) pipdamagetype[numpip] = DAMAGECRUSH;
    if ( cTmp == 'P' || cTmp == 'p' ) pipdamagetype[numpip] = DAMAGEPOKE;
    if ( cTmp == 'H' || cTmp == 'h' ) pipdamagetype[numpip] = DAMAGEHOLY;
    if ( cTmp == 'E' || cTmp == 'e' ) pipdamagetype[numpip] = DAMAGEEVIL;
    if ( cTmp == 'F' || cTmp == 'f' ) pipdamagetype[numpip] = DAMAGEFIRE;
    if ( cTmp == 'I' || cTmp == 'i' ) pipdamagetype[numpip] = DAMAGEICE;
    if ( cTmp == 'Z' || cTmp == 'z' ) pipdamagetype[numpip] = DAMAGEZAP;


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
    if ( iTmp < -1 ) iTmp = -1;
    if ( iTmp > MAXWAVE - 1 ) iTmp = MAXWAVE - 1;
    pipsoundspawn[numpip] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
    if ( iTmp < -1 ) iTmp = -1;
    if ( iTmp > MAXWAVE - 1 ) iTmp = MAXWAVE - 1;
    pipsoundend[numpip] = iTmp;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    pipfriendlyfire[numpip] = bfalse;
    if ( cTmp == 'T' || cTmp == 't' ) pipfriendlyfire[numpip] = btrue;
    goto_colon( fileread );  // !!Hate group only
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
      test = Make_IDSZ( 'T', 'U', 'R', 'N' );  // [TURN]
      if ( idsz == test )  pipdamfx[numpip] = DAMFXNONE;
      test = Make_IDSZ( 'Z', 'S', 'P', 'D' );  // [ZSPD]
      if ( idsz == test )  pipzaimspd[numpip] = iTmp;
      test = Make_IDSZ( 'F', 'S', 'N', 'D' );  // [FSND]
      if ( idsz == test )  pipsoundfloor[numpip] = iTmp;
      test = Make_IDSZ( 'W', 'S', 'N', 'D' );  // [WSND]
      if ( idsz == test )  pipsoundwall[numpip] = iTmp;
      test = Make_IDSZ( 'W', 'E', 'N', 'D' );  // [WEND]
      if ( idsz == test )  pipendwall[numpip] = iTmp;
      test = Make_IDSZ( 'A', 'R', 'M', 'O' );  // [ARMO]
      if ( idsz == test )  pipdamfx[numpip] |= DAMFXARMO;
      test = Make_IDSZ( 'B', 'L', 'O', 'C' );  // [BLOC]
      if ( idsz == test )  pipdamfx[numpip] |= DAMFXBLOC;
      test = Make_IDSZ( 'A', 'R', 'R', 'O' );  // [ARRO]
      if ( idsz == test )  pipdamfx[numpip] |= DAMFXARRO;
      test = Make_IDSZ( 'T', 'I', 'M', 'E' );  // [TIME]
      if ( idsz == test )  pipdamfx[numpip] |= DAMFXTIME;
      test = Make_IDSZ( 'P', 'U', 'S', 'H' );  // [PUSH]
      if ( idsz == test )  pipallowpush[numpip] = iTmp;
      test = Make_IDSZ( 'D', 'L', 'E', 'V' );  // [DLEV]
      if ( idsz == test )  pipdynalightleveladd[numpip] = iTmp / 1000.0f;
      test = Make_IDSZ( 'D', 'R', 'A', 'D' );  // [DRAD]
      if ( idsz == test )  pipdynalightfalloffadd[numpip] = iTmp / 1000.0f;
      test = Make_IDSZ( 'I', 'D', 'A', 'M' );  // [IDAM]
      if ( idsz == test )  pipintdamagebonus[numpip] = iTmp;
      test = Make_IDSZ( 'W', 'D', 'A', 'M' );  // [WDAM]
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
void reset_particles( char* modname )
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
    log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );
    loadpath = "basicdat" SLASH_STR "globalparticles" SLASH_STR "splash.txt";
    if ( !load_one_particle( loadpath, 0, 0 ) )
    {
      log_error( "Data file was not found! (%s)", loadpath );
    }
  }
  make_newloadname( modname, "gamedat" SLASH_STR "ripple.txt", newloadname );
  if ( !load_one_particle( newloadname, 0, 0 ) )
  {
    log_message( "DEBUG: Data file was not found! (%s) - Defaulting to global particle.\n", newloadname );
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

