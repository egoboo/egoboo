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
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "mathstuff.h"
#include "Log.h"
#include "mesh.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
void make_prtlist( void )
{
  // ZZ> This function figures out which particles are visible, and it sets up dynamic
  //     lighting
  int cnt, tnc, disx, disy, distance, slot;


  // Don't really make a list, just set to visible or not
  numdynalight = 0;
  dynadistancetobeat = MAXDYNADIST;
  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    prtinview[cnt] = bfalse;
    if ( !VALID_PRT( cnt ) ) continue;

    prtinview[cnt] = mesh_in_renderlist( prtonwhichfan[cnt] );
    // Set up the lights we need
    if ( prtdynalighton[cnt] )
    {
      disx = ABS( prtpos[cnt].x - camtrackpos.x );
      disy = ABS( prtpos[cnt].y - camtrackpos.y );
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
          dynadistancetobeat = dynadistance[0];
          for ( tnc = 1; tnc < MAXDYNA; tnc++ )
          {
            if ( dynadistance[tnc] > dynadistancetobeat )
            {
              slot = tnc;
            }
          }
          dynadistance[slot] = distance;

          // Find the new distance to beat
          dynadistancetobeat = dynadistance[0];
          for ( tnc = 1; tnc < MAXDYNA; tnc++ )
          {
            if ( dynadistance[tnc] > dynadistancetobeat )
            {
              dynadistancetobeat = dynadistance[tnc];
            }
          }
        }
        dynalightlist[slot].x = prtpos[cnt].x;
        dynalightlist[slot].y = prtpos[cnt].y;
        dynalightlist[slot].z = prtpos[cnt].z;
        dynalightlevel[slot] = prtdynalightlevel[cnt];
        dynalightfalloff[slot] = prtdynalightfalloff[cnt];
      }
    }

  }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_no_sound( PRT_REF particle )
{
  // ZZ> This function sticks a particle back on the free particle stack
  freeprtlist[numfreeprt] = particle;
  numfreeprt++;
  prton[particle] = bfalse;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound( float intensity, PRT_REF particle, Sint8 sound )
{
  //This function plays a sound effect for a particle
  if ( INVALID_SOUND == sound ) return;

  //Play local sound or else global (coins for example)
  if ( MAXMODEL != prtmodel[particle] )
  {
    play_sound( intensity, prtpos[particle], capwavelist[prtmodel[particle]][sound], 0  );
  }
  else
  {
    play_sound( intensity, prtpos[particle], globalwave[sound], 0  );
  };
}

//--------------------------------------------------------------------------------------------
void free_one_particle( PRT_REF particle )
{
  // ZZ> This function sticks a particle back on the free particle stack and
  //     plays the sound associated with the particle
  int child;
  if ( prtspawncharacterstate[particle] != SPAWN_NOCHARACTER )
  {
    child = spawn_one_character( prtpos[particle], prtmodel[particle], prtteam[particle], 0, prtfacing[particle], NULL, MAXCHR );
    if ( VALID_CHR( child ) )
    {
      chraistate[child] = prtspawncharacterstate[particle];
      chraiowner[child] = prt_get_owner( particle );
    }
  }
  play_particle_sound( 1.0f, particle, pipsoundend[prtpip[particle]] );

  free_one_particle_no_sound( particle );
}

//--------------------------------------------------------------------------------------------
int get_free_particle( int force )
{
  // ZZ> This function gets an unused particle.  If all particles are in use
  //     and force is set, it grabs the first unimportant one.  The particle
  //     index is the return value
  PRT_REF particle;


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
PRT_REF spawn_one_particle( float intensity, vect3 pos,
                           Uint16 facing, Uint16 model, Uint16 local_pip,
                           CHR_REF characterattach, GRIP grip, TEAM team,
                           CHR_REF characterorigin, Uint16 multispawn, CHR_REF oldtarget )
{
  // ZZ> This function spawns a new particle, and returns the number of that particle
  int iprt, velocity;
  float xvel, yvel, zvel, tvel;
  int offsetfacing, newrand;
  Uint16 glob_pip = MAXPRTPIP;
  float weight;
  Uint16 prt_target;

  if ( local_pip >= MAXPRTPIP )
  {
    fprintf( stderr, "spawn_one_particle() - \n\tfailed to spawn : local_pip == %d is an invalid value\n", local_pip );
    return MAXPRT;
  }

  // Convert from local local_pip to global local_pip
  if ( model < MAXMODEL && local_pip < PRTPIP_PEROBJECT_COUNT )
    glob_pip = madprtpip[model][local_pip];

  // assume we were given a global local_pip
  if ( MAXPRTPIP == glob_pip )
    glob_pip = local_pip;



  iprt = get_free_particle( pipforce[glob_pip] );
  if ( iprt == MAXPRT )
  {
    fprintf( stderr, "spawn_one_particle() - \n\tfailed to spawn : get_free_particle() returned invalid value %d\n", iprt );
    return MAXPRT;
  }

  weight = 1.0f;
  if ( VALID_CHR( characterorigin ) ) weight = MAX( weight, chrweight[characterorigin] );
  if ( VALID_CHR( characterattach ) ) weight = MAX( weight, chrweight[characterattach] );
  prtweight[iprt] = weight;

  //fprintf(stdout, "spawn_one_particle() - \n\tlocal local_pip == %d, global local_pip == %d, part == %d\n", local_pip, glob_pip, iprt);

  // Necessary data for any part
  prton[iprt] = btrue;
  prtgopoof[iprt] = bfalse;
  prtpip[iprt] = glob_pip;
  prtmodel[iprt] = model;
  prtinview[iprt] = bfalse;
  prtlevel[iprt] = 0;
  prtteam[iprt] = team;
  prtowner[iprt] = characterorigin;
  prtdamagetype[iprt] = pipdamagetype[glob_pip];
  prtspawncharacterstate[iprt] = SPAWN_NOCHARACTER;


  // Lighting and sound
  prtdynalighton[iprt] = bfalse;
  if ( multispawn == 0 )
  {
    prtdynalighton[iprt] = ( DYNA_OFF != pipdynalightmode[glob_pip] );
    if ( pipdynalightmode[glob_pip] == DYNA_LOCAL )
    {
      prtdynalighton[iprt] = bfalse;
    }
  }
  prtdynalightlevel[iprt]   = pipdynalevel[glob_pip] * intensity;
  prtdynalightfalloff[iprt] = pipdynafalloff[glob_pip];



  // Set character attachments ( characterattach==MAXCHR means none )
  prtattachedtochr[iprt] = characterattach;
  prtvertoffset[iprt] = grip;



  // Targeting...
  offsetfacing = 0;
  zvel = 0;
  pos.z += generate_signed( &pipzspacing[glob_pip] );
  velocity = generate_unsigned( &pipxyvel[glob_pip] );
  prttarget[iprt] = oldtarget;
  prt_target = MAXCHR;
  if ( pipnewtargetonspawn[glob_pip] )
  {
    if ( piptargetcaster[glob_pip] )
    {
      // Set the target to the caster
      prttarget[iprt] = characterorigin;
    }
    else
    {
      // Correct facing for dexterity...
      if ( chrdexterity_fp8[characterorigin] < PERFECTSTAT )
      {
        // Correct facing for randomness
        newrand = FP8_DIV( PERFECTSTAT - chrdexterity_fp8[characterorigin],  PERFECTSTAT );
        offsetfacing += generate_dither( &pipfacing[glob_pip], newrand );
      }

      // Find a target
      prttarget[iprt] = prt_search_target( pos.x, pos.y, facing, piptargetangle[glob_pip], piponlydamagefriendly[glob_pip], bfalse, team, characterorigin, oldtarget );
      prt_target = prt_get_target( iprt );
      if ( VALID_CHR( prt_target ) )
      {
        offsetfacing -= search_useangle;

        if ( pipzaimspd[glob_pip] != 0 )
        {
          // These aren't velocities...  This is to do aiming on the Z axis
          if ( velocity > 0 )
          {
            xvel = chrpos[prt_target].x - pos.x;
            yvel = chrpos[prt_target].y - pos.y;
            tvel = sqrt( xvel * xvel + yvel * yvel ) / velocity;   // This is the number of steps...
            if ( tvel > 0 )
            {
              zvel = ( chrpos[prt_target].z + ( chrbumpsize[prt_target] >> 1 ) - pos.z ) / tvel;  // This is the zvel alteration
              if ( zvel < - ( pipzaimspd[glob_pip] >> 1 ) ) zvel = - ( pipzaimspd[glob_pip] >> 1 );
              if ( zvel > pipzaimspd[glob_pip] ) zvel = pipzaimspd[glob_pip];
            }
          }
        }
      }
    }

    // Does it go away?
    if ( !VALID_CHR( prt_target ) && pipneedtarget[glob_pip] )
    {
      fprintf( stderr, "spawn_one_particle() - \n\tfailed to spawn : pip requires target and no target specified\n", iprt );

      free_one_particle( iprt );
      return MAXPRT;
    }

    // Start on top of target
    if ( VALID_CHR( prt_target ) && pipstartontarget[glob_pip] )
    {
      pos.x = chrpos[prt_target].x;
      pos.y = chrpos[prt_target].y;
    }
  }
  else
  {
    // Correct facing for randomness
    offsetfacing += generate_dither( &pipfacing[glob_pip], INT_TO_FP8( 1 ) );
  }
  facing += pipfacing[glob_pip].ibase + offsetfacing;
  prtfacing[iprt] = facing;
  facing >>= 2;


  // Location data from arguments
  newrand = generate_unsigned( &pipxyspacing[glob_pip] );
  pos.x += turntosin[( facing+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * newrand;
  pos.y += turntosin[( facing+8192 ) & TRIGTABLE_MASK] * newrand;

  pos.x = mesh_clip_x( pos.x );
  pos.y = mesh_clip_x( pos.y );

  prtpos[iprt].x = pos.x;
  prtpos[iprt].y = pos.y;
  prtpos[iprt].z = pos.z;


  // Velocity data
  xvel = turntosin[( facing+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * velocity;
  yvel = turntosin[( facing+8192 ) & TRIGTABLE_MASK] * velocity;
  zvel += generate_signed( &pipzvel[glob_pip] );
  prtvel[iprt].x = xvel;
  prtvel[iprt].y = yvel;
  prtvel[iprt].z = zvel;

  prtpos_old[iprt].x = prtpos[iprt].x - prtvel[iprt].x;
  prtpos_old[iprt].y = prtpos[iprt].y - prtvel[iprt].y;
  prtpos_old[iprt].z = prtpos[iprt].z - prtvel[iprt].z;

  // Template values
  prtbumpsize[iprt] = pipbumpsize[glob_pip];
  prtbumpsizebig[iprt] = prtbumpsize[iprt] + ( prtbumpsize[iprt] >> 1 );
  prtbumpheight[iprt] = pipbumpheight[glob_pip];
  prtbumpstrength[iprt] = pipbumpstrength[glob_pip] * intensity;

  // figure out the particle type and transparency
  prttype[iprt] = piptype[glob_pip];
  prtalpha_fp8[iprt] = 255;
  switch ( piptype[glob_pip] )
  {
    case PRTTYPE_SOLID:
      if ( intensity < 1.0f )
      {
        prttype[iprt]  = PRTTYPE_ALPHA;
        prtalpha_fp8[iprt] = 255 * intensity;
      }
      break;

    case PRTTYPE_LIGHT:
      prtalpha_fp8[iprt] = 255 * intensity;
      break;

    case PRTTYPE_ALPHA:
      prtalpha_fp8[iprt] = particletrans * intensity;
      break;
  };



  // Image data
  prtrotate[iprt] = generate_unsigned( &piprotate[glob_pip] );
  prtrotateadd[iprt] = piprotateadd[glob_pip];
  prtsize_fp8[iprt] = pipsizebase_fp8[glob_pip];
  prtsizeadd_fp8[iprt] = pipsizeadd[glob_pip];
  prtimage_fp8[iprt] = 0;
  prtimageadd_fp8[iprt] = generate_unsigned( &pipimageadd[glob_pip] );
  prtimagestt_fp8[iprt] = INT_TO_FP8( pipimagebase[glob_pip] );
  prtimagemax_fp8[iprt] = INT_TO_FP8( pipnumframes[glob_pip] );
  prttime[iprt] = piptime[glob_pip];
  if ( pipendlastframe[glob_pip] )
  {
    if ( prtimageadd_fp8[iprt] != 0 ) prttime[iprt] = 0.0;
  }


  // Set onwhichfan...
  prtonwhichfan[iprt] = mesh_get_fan( prtpos[iprt] );


  // Damage stuff
  prtdamage[iprt].ibase = pipdamage_fp8[glob_pip].ibase * intensity;
  prtdamage[iprt].irand = pipdamage_fp8[glob_pip].irand * intensity;


  // Spawning data
  prtspawntime[iprt] = pipcontspawntime[glob_pip];
  if ( prtspawntime[iprt] != 0 )
  {
    CHR_REF prt_attachedto = prt_get_attachedtochr( iprt );

    prtspawntime[iprt] = 1;
    if ( VALID_CHR( prt_attachedto ) )
    {
      prtspawntime[iprt]++; // Because attachment takes an update before it happens
    }
  }


  // Sound effect
  play_particle_sound( intensity, iprt, pipsoundspawn[glob_pip] );

  return iprt;
}

//--------------------------------------------------------------------------------------------
Uint32 __prthitawall( PRT_REF particle, vect3 * norm )
{
  // ZZ> This function returns nonzero if the particle hit a wall

  Uint32 retval, collision_bits;

  if ( !VALID_PRT( particle ) ) return 0;

  collision_bits = MESHFX_IMPASS;
  if ( 0 != pipbumpmoney[prtpip[particle]] )
  {
    collision_bits |= MESHFX_WALL;
  }

  retval = mesh_hitawall( prtpos[particle], prtbumpsize[particle], collision_bits );

  if( 0!=retval && NULL!=norm )
  {
    vect3 pos;

    norm->x = norm->y = norm->z = 0;

    pos.x = prtpos[particle].x;
    pos.y = prtpos_old[particle].y;
    pos.z = prtpos_old[particle].z;

    if( 0!=mesh_hitawall( pos, prtbumpsize[particle], collision_bits ) )
    {
      norm->x = SGN(prtpos[particle].x - prtpos_old[particle].x);
    }

    pos.x = prtpos_old[particle].x;
    pos.y = prtpos[particle].y;
    pos.z = prtpos_old[particle].z;

    if( 0!=mesh_hitawall( pos, prtbumpsize[particle], collision_bits ) )
    {
      norm->y = SGN(prtpos[particle].y - prtpos_old[particle].y);
    }

    pos.x = prtpos_old[particle].x;
    pos.y = prtpos_old[particle].y;
    pos.z = prtpos[particle].z;

    if( 0!=mesh_hitawall( pos, prtbumpsize[particle], collision_bits ) )
    {
      norm->z = SGN(prtpos[particle].z - prtpos_old[particle].z);
    }

    *norm = Normalize( *norm );
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles( CHR_REF character )
{
  // ZZ> This function makes sure a character has no attached particles
  PRT_REF particle;
  bool_t useful = bfalse;

  if ( !VALID_CHR( character ) ) return;

  for ( particle = 0; particle < MAXPRT; particle++ )
  {
    if ( !VALID_PRT( particle ) ) continue;

    if ( prt_get_attachedtochr( particle ) == character )
    {
      prtgopoof[particle]        = btrue;
      prtattachedtochr[particle] = MAXCHR;
      useful = btrue;
    }
  }

  // Set the alert for disaffirmation ( wet torch )
  if ( useful )
  {
    chralert[character] |= ALERT_DISAFFIRMED;
  };
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles( CHR_REF character )
{
  // ZZ> This function returns the number of particles attached to the given character
  Uint16 cnt, particle;

  cnt = 0;
  for ( particle = 0; particle < MAXPRT; particle++ )
  {
    if ( VALID_PRT( particle ) && prt_get_attachedtochr( particle ) == character )
    {
      cnt++;
    }
  }

  return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles( CHR_REF character )
{
  // ZZ> This function makes sure a character has all of it's particles
  Uint16 numberattached;
  PRT_REF particle;

  numberattached = number_of_attached_particles( character );
  while ( numberattached < capattachedprtamount[chrmodel[character]] )
  {
    particle = spawn_one_particle( 1.0f, chrpos[character], 0, chrmodel[character], capattachedprttype[chrmodel[character]], character, GRIP_LAST + numberattached, chrteam[character], character, numberattached, MAXCHR );
    if ( particle != MAXPRT )
    {
      attach_particle_to_character( particle, character, prtvertoffset[particle] );
    }
    numberattached++;
  }

  // Set the alert for reaffirmation ( for exploding barrels with fire )
  chralert[character] |= ALERT_REAFFIRMED;
}

//--------------------------------------------------------------------------------------------
void despawn_particles()
{
  int iprt, tnc;
  Uint16 facing, pip;
  CHR_REF prt_target, prt_owner, prt_attachedto;

  // actually destroy all particles that requested destruction last time through the loop
  for ( iprt = 0; iprt < MAXPRT; iprt++ )
  {
    if ( !VALID_PRT( iprt ) || !prtgopoof[iprt] ) continue;

    // To make it easier
    pip = prtpip[iprt];
    facing = prtfacing[iprt];
    prt_target = prt_get_target( iprt );
    prt_owner = prt_get_owner( iprt );
    prt_attachedto = prt_get_attachedtochr( iprt );

    for ( tnc = 0; tnc < pipendspawnamount[pip]; tnc++ )
    {
      spawn_one_particle( 1.0f, prtpos[iprt],
                          facing, prtmodel[iprt], pipendspawnpip[pip],
                          MAXCHR, GRIP_LAST, prtteam[iprt], prt_owner, tnc, prt_target );
      facing += pipendspawnfacingadd[pip];
    }

    free_one_particle( iprt );
  }

};

//--------------------------------------------------------------------------------------------
void move_particles( float dUpdate )
{
  // ZZ> This is the particle physics function
  int iprt, tnc;
  Uint32 fan;
  Uint16 facing, pip, particle;
  float level;
  CHR_REF prt_target, prt_owner, prt_attachedto;

  float loc_noslipfriction, loc_homingfriction, loc_homingaccel;

  loc_noslipfriction = pow( noslipfriction, dUpdate );

  for ( iprt = 0; iprt < MAXPRT; iprt++ )
  {
    if ( !VALID_PRT( iprt ) ) continue;

    prt_target     = prt_get_target( iprt );
    prt_owner      = prt_get_owner( iprt );
    prt_attachedto = prt_get_attachedtochr( iprt );

    prtpos_old[iprt] = prtpos[iprt];

    prtonwhichfan[iprt] = INVALID_FAN;
    prtlevel[iprt] = 0;
    fan = mesh_get_fan( prtpos[iprt] );
    prtonwhichfan[iprt] = fan;
    prtlevel[iprt] = ( INVALID_FAN == fan ) ? 0 : mesh_get_level( fan, prtpos[iprt].x, prtpos[iprt].y, bfalse );

    // To make it easier
    pip = prtpip[iprt];

    loc_homingfriction = pow( piphomingfriction[pip], dUpdate );
    loc_homingaccel    = piphomingaccel[pip];

    // Animate particle
    prtimage_fp8[iprt] += prtimageadd_fp8[iprt] * dUpdate;
    if ( prtimage_fp8[iprt] >= prtimagemax_fp8[iprt] )
    {
      if ( pipendlastframe[pip] )
      {
        prtimage_fp8[iprt] = prtimagemax_fp8[iprt] - INT_TO_FP8( 1 );
        prtgopoof[iprt]    = btrue;
      }
      else if ( prtimagemax_fp8[iprt] > 0 )
      {
        // if the prtimage_fp8[] is a fraction of an image over prtimagemax_fp8[],
        // keep the fraction
        prtimage_fp8[iprt] %= prtimagemax_fp8[iprt];
      }
      else
      {
        // a strange case
        prtimage_fp8[iprt] = 0;
      }
    };

    prtrotate[iprt] += prtrotateadd[iprt] * dUpdate;
    prtsize_fp8[iprt] = ( prtsize_fp8[iprt] + prtsizeadd_fp8[iprt] < 0 ) ? 0 : prtsize_fp8[iprt] + prtsizeadd_fp8[iprt];

    // Change dyna light values
    prtdynalightlevel[iprt]   += pipdynalightleveladd[pip] * dUpdate;
    prtdynalightfalloff[iprt] += pipdynalightfalloffadd[pip] * dUpdate;


    // Make it sit on the floor...  Shift is there to correct for sprite size
    level = prtlevel[iprt] + FP8_TO_FLOAT( prtsize_fp8[iprt] ) * 0.5f;

    // do interaction with the floor
    if(  !VALID_CHR( prt_attachedto ) && prtpos[iprt].z > level )
    {
      float lerp = ( prtpos[iprt].z - ( prtlevel[iprt] + PLATTOLERANCE ) ) / ( float ) PLATTOLERANCE;
      if ( lerp < 0.2f ) lerp = 0.2f;
      if ( lerp > 1.0f ) lerp = 1.0f;

      prtaccum_acc[iprt].z += gravity * lerp;

      prtaccum_acc[iprt].x -= ( 1.0f - noslipfriction ) * lerp * prtvel[iprt].x;
      prtaccum_acc[iprt].y -= ( 1.0f - noslipfriction ) * lerp * prtvel[iprt].y;
    }

    // Do speed limit on Z
    if ( prtvel[iprt].z < -pipspdlimit[pip] )  prtaccum_vel[iprt].z += -pipspdlimit[pip] - prtvel[iprt].z;

    // Do homing
    if ( piphoming[pip] && VALID_CHR( prt_target ) )
    {
      if ( !chralive[prt_target] )
      {
        prtgopoof[iprt] = btrue;
      }
      else
      {
        if ( !VALID_CHR( prt_attachedto ) )
        {
          prtaccum_acc[iprt].x += -(1.0f-loc_homingfriction) * prtvel[iprt].x;
          prtaccum_acc[iprt].y += -(1.0f-loc_homingfriction) * prtvel[iprt].y;
          prtaccum_acc[iprt].z += -(1.0f-loc_homingfriction) * prtvel[iprt].z;

          prtaccum_acc[iprt].x += ( chrpos[prt_target].x - prtpos[iprt].x ) * loc_homingaccel * 4.0f;
          prtaccum_acc[iprt].y += ( chrpos[prt_target].y - prtpos[iprt].y ) * loc_homingaccel * 4.0f;
          prtaccum_acc[iprt].z += ( chrpos[prt_target].z + ( chrbumpheight[prt_target] >> 1 ) - prtpos[iprt].z ) * loc_homingaccel * 4.0f;
        }
      }
    }


    // Spawn new particles if continually spawning
    if ( pipcontspawnamount[pip] > 0.0f )
    {
      prtspawntime[iprt] -= dUpdate;
      if ( prtspawntime[iprt] <= 0.0f )
      {
        prtspawntime[iprt] = pipcontspawntime[pip];
        facing = prtfacing[iprt];
        tnc = 0;
        while ( tnc < pipcontspawnamount[pip] )
        {
          particle = spawn_one_particle( 1.0f, prtpos[iprt],
                                         facing, prtmodel[iprt], pipcontspawnpip[pip],
                                         MAXCHR, GRIP_LAST, prtteam[iprt], prt_get_owner( iprt ), tnc, prt_target );
          if ( pipfacingadd[prtpip[iprt]] != 0 && particle < MAXPRT )
          {
            // Hack to fix velocity
            prtvel[particle].x += prtvel[iprt].x;
            prtvel[particle].y += prtvel[iprt].y;
          }
          facing += pipcontspawnfacingadd[pip];
          tnc++;
        }
      }
    }


    // Check underwater
    if ( prtpos[iprt].z < waterdouselevel && mesh_has_some_bits( prtonwhichfan[iprt], MESHFX_WATER ) && pipendwater[pip] )
    {
      vect3 prt_pos = {prtpos[iprt].x, prtpos[iprt].y, watersurfacelevel};

      // Splash for particles is just a ripple
      spawn_one_particle( 1.0f, prt_pos, 0, MAXMODEL, PRTPIP_RIPPLE, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );


      // Check for disaffirming character
      if ( VALID_CHR( prt_attachedto ) && prt_get_owner( iprt ) == prt_attachedto )
      {
        // Disaffirm the whole character
        disaffirm_attached_particles( prt_attachedto );
      }
      else
      {
        prtgopoof[iprt] = btrue;
      }
    }

    // Down the particle timer
    if ( prttime[iprt] > 0.0f )
    {
      prttime[iprt] -= dUpdate;
      if ( prttime[iprt] <= 0.0f ) prtgopoof[iprt] = btrue;
    };

    prtfacing[iprt] += pipfacingadd[pip] * dUpdate;
  }

}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
  // ZZ> This function attaches particles to their characters so everything gets
  //     drawn right
  int cnt;


  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    CHR_REF prt_attachedto;

    if ( !VALID_PRT( cnt ) ) continue;

    prt_attachedto = prt_get_attachedtochr( cnt );

    if ( !VALID_CHR( prt_attachedto ) ) continue;

    attach_particle_to_character( cnt, prt_attachedto, prtvertoffset[cnt] );

    // Correct facing so swords knock characters in the right direction...
    if ( HAS_SOME_BITS( pipdamfx[prtpip[cnt]], DAMFX_TURN ) )
      prtfacing[cnt] = chrturn_lr[prt_attachedto];
  }
}

//--------------------------------------------------------------------------------------------
void free_all_particles()
{
  // ZZ> This function resets the particle allocation lists
  numfreeprt = 0;
  while ( numfreeprt < MAXPRT )
  {
    prton[numfreeprt] = bfalse;
    freeprtlist[numfreeprt] = numfreeprt;
    numfreeprt++;
  }
}

//--------------------------------------------------------------------------------------------
void setup_particles()
{
  // ZZ> This function sets up particle data

  particletexture = 0;

  // Reset the allocation table
  free_all_particles();
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir( Uint16 majordir, float dx, float dy, float dUpdate )
{
  // ZZ> This function returns a direction between the major and minor ones, closer
  //     to the major.

  Uint16 rotate_sin, minordir;
  Sint16 diff_dir;
  const float turnspeed = 2000.0f;

  if ( ABS( dx ) + ABS( dy ) > TURNSPD )
  {
    minordir = vec_to_turn( dx, dy );

    diff_dir = (( Sint16 ) minordir ) - (( Sint16 ) majordir );

    if (( diff_dir > -turnspeed * dUpdate ) && ( diff_dir < turnspeed * dUpdate ) )
    {
      rotate_sin = ( Uint16 ) diff_dir;
    }
    else
    {
      rotate_sin = ( Uint16 )( turnspeed * dUpdate * SGN( diff_dir ) );
    };

    return majordir + rotate_sin;
  }
  else
    return majordir;
}

//--------------------------------------------------------------------------------------------
void spawn_bump_particles( CHR_REF character, PRT_REF particle )
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
    direction = chrturn_lr[character] - vec_to_turn( -prtvel[particle].x, -prtvel[particle].y );
    if ( HAS_SOME_BITS( madframefx[chrframe[character]], MADFX_INVICTUS ) )
    {
      // I Frame
      if ( HAS_SOME_BITS( pipdamfx[pip], DAMFX_BLOC ) )
      {
        left  = UINT16_MAX;
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
        spawn_enchant( prt_get_owner( particle ), character, MAXCHR, MAXENCHANT, prtmodel[particle] );
      }

      // Spawn particles
      if ( amount != 0 && !capresistbumpspawn[chrmodel[character]] && !chrinvictus[character] &&
           vertices != 0 && ( chrdamagemodifier_fp8[character][prtdamagetype[particle]]&DAMAGE_SHIFT ) != DAMAGE_SHIFT )
      {
        if ( amount == 1 )
        {
          // A single particle ( arrow? ) has been stuck in the character...
          // Find best vertex to attach to
          bestvertex = 0;
          bestdistance = 9999999;
          z =  prtpos[particle].z - ( chrpos[character].z + RAISE );
          facing = prtfacing[particle] - chrturn_lr[character] - 16384;
          facing >>= 2;
          fsin = turntosin[facing & TRIGTABLE_MASK];
          fcos = turntosin[( facing+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
          y = 8192;
          x = -y * fsin;
          y = y * fcos;
          z <<= 10;///chrscale[character];
          frame = madframestart[chrmodel[character]];
          cnt = 0;
          while ( cnt < vertices )
          {
            distance = ABS( x - madvrtx[frame][vertices-cnt-1] ) + ABS( y - madvrty[frame][vertices-cnt-1] ) + ABS( z - madvrtz[frame][vertices-cnt-1] );
            if ( distance < bestdistance )
            {
              bestdistance = distance;
              bestvertex = cnt;
            }
            cnt++;
          }
          spawn_one_particle( 1.0f, chrpos[character], 0, prtmodel[particle], pipbumpspawnpip[pip],
                              character, bestvertex + 1, prtteam[particle], prt_get_owner( particle ), cnt, character );
        }
        else
        {
          amount = ( amount * vertices ) >> 5;  // Correct amount for size of character
          cnt = 0;
          while ( cnt < amount )
          {
            spawn_one_particle( 1.0f, chrpos[character], 0, prtmodel[particle], pipbumpspawnpip[pip],
                                character, rand() % vertices, prtteam[particle], prt_get_owner( particle ), cnt, character );
            cnt++;
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
bool_t prt_is_over_water( int cnt )
{
  // This function returns btrue if the particle is over a water tile

  Uint32 fan;

  if ( cnt < MAXPRT )
  {
    fan = mesh_get_fan( prtpos[cnt] );
    if ( mesh_has_some_bits( fan, MESHFX_WATER ) )  return ( INVALID_FAN != fan );
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn( float dUpdate )
{
  // ZZ> This function drops snowflakes or rain or whatever, also swings the camera
  PRT_REF particle;
  int cnt;
  bool_t foundone = bfalse;

  if ( weathertime > 0 )
  {
    weathertime -= dUpdate;
    if ( weathertime < 0 ) weathertime = 0;

    if ( weathertime == 0 )
    {
      weathertime = weathertimereset;

      // Find a valid player
      foundone = bfalse;
      cnt = 0;
      while ( cnt < MAXPLAYER )
      {
        weatherplayer = ( weatherplayer + 1 ) % MAXPLAYER;
        if ( VALID_PLA( weatherplayer ) )
        {
          foundone = btrue;
          cnt = MAXPLAYER;
        }
        cnt++;
      }
    }


    // Did we find one?
    if ( foundone )
    {
      // Yes, but is the character valid?
      cnt = pla_get_character( weatherplayer );
      if ( VALID_CHR( cnt ) && !chr_in_pack( cnt ) )
      {
        // Yes, so spawn over that character
        particle = spawn_one_particle( 1.0f, chrpos[cnt], 0, MAXMODEL, PRTPIP_WEATHER_1, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
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
  camswing = ( camswing + camswingrate ) & TRIGTABLE_MASK;
}


//--------------------------------------------------------------------------------------------
Uint32 load_one_particle_profile( char *szLoadName, Uint16 object, int local_pip )
{
  // ZZ> This function loads a particle template, returning MAXPRTPIP if the file wasn't
  //     found
  FILE* fileread;
  IDSZ idsz;
  int iTmp;
  Uint32 ipip = MAXPRTPIP;


  fileread = fs_fileOpen( PRI_NONE, NULL, szLoadName, "r" );
  if ( NULL == fileread ) return ipip;

  ipip = numpip;
  numpip++;

  // store some info for debugging
  strncpy( pipfname[ipip], szLoadName, sizeof( pipfname[ipip] ) );
  fgets( pipcomment[ipip], sizeof( pipcomment[ipip] ), fileread );
  rewind( fileread );
  if ( pipcomment[ipip][0] != '/' )  pipcomment[ipip][0] = '\0';

  // General data
  globalname = szLoadName;
  pipforce[ipip] = fget_next_bool( fileread );
  piptype[ipip] = fget_next_prttype( fileread );
  pipimagebase[ipip] = fget_next_int( fileread );
  pipnumframes[ipip] = fget_next_int( fileread );
  pipimageadd[ipip].ibase = fget_next_int( fileread );
  pipimageadd[ipip].irand = fget_next_int( fileread );
  piprotate[ipip].ibase = fget_next_int( fileread );
  piprotate[ipip].irand = fget_next_int( fileread );
  piprotateadd[ipip] = fget_next_int( fileread );
  pipsizebase_fp8[ipip] = fget_next_int( fileread );
  pipsizeadd[ipip] = fget_next_int( fileread );
  pipspdlimit[ipip] = fget_next_float( fileread );
  pipfacingadd[ipip] = fget_next_int( fileread );


  // Ending conditions
  pipendwater[ipip] = fget_next_bool( fileread );
  pipendbump[ipip] = fget_next_bool( fileread );
  pipendground[ipip] = fget_next_bool( fileread );
  pipendlastframe[ipip] = fget_next_bool( fileread );
  piptime[ipip] = fget_next_int( fileread );


  // Collision data
  pipdampen[ipip] = fget_next_float( fileread );
  pipbumpmoney[ipip] = fget_next_int( fileread );
  pipbumpsize[ipip] = fget_next_int( fileread );
  pipbumpheight[ipip] = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &pipdamage_fp8[ipip] );
  pipdamagetype[ipip] = fget_next_damage( fileread );
  pipbumpstrength[ipip] = 1.0f;
  if ( pipbumpsize[ipip] == 0 )
  {
    pipbumpstrength[ipip] = 0.0f;
    pipbumpsize[ipip]   = 0.5f * FP8_TO_FLOAT( pipsizebase_fp8[ipip] );
    pipbumpheight[ipip] = 0.5f * FP8_TO_FLOAT( pipsizebase_fp8[ipip] );
  };

  // Lighting data
  pipdynalightmode[ipip] = fget_next_dynamode( fileread );
  pipdynalevel[ipip] = fget_next_float( fileread );
  pipdynafalloff[ipip] = fget_next_int( fileread );
  if ( pipdynafalloff[ipip] > MAXFALLOFF )  pipdynafalloff[ipip] = MAXFALLOFF;



  // Initial spawning of this particle
  pipfacing[ipip].ibase    = fget_next_int( fileread );
  pipfacing[ipip].irand    = fget_next_int( fileread );
  pipxyspacing[ipip].ibase = fget_next_int( fileread );
  pipxyspacing[ipip].irand = fget_next_int( fileread );
  pipzspacing[ipip].ibase  = fget_next_int( fileread );
  pipzspacing[ipip].irand  = fget_next_int( fileread );
  pipxyvel[ipip].ibase     = fget_next_int( fileread );
  pipxyvel[ipip].irand     = fget_next_int( fileread );
  pipzvel[ipip].ibase      = fget_next_int( fileread );
  pipzvel[ipip].irand      = fget_next_int( fileread );


  // Continuous spawning of other particles
  pipcontspawntime[ipip] = fget_next_int( fileread );
  pipcontspawnamount[ipip] = fget_next_int( fileread );
  pipcontspawnfacingadd[ipip] = fget_next_int( fileread );
  pipcontspawnpip[ipip] = fget_next_int( fileread );


  // End spawning of other particles
  pipendspawnamount[ipip] = fget_next_int( fileread );
  pipendspawnfacingadd[ipip] = fget_next_int( fileread );
  pipendspawnpip[ipip] = fget_next_int( fileread );

  // Bump spawning of attached particles
  pipbumpspawnamount[ipip] = fget_next_int( fileread );
  pipbumpspawnpip[ipip] = fget_next_int( fileread );


  // Random stuff  !!!BAD!!! Not complete
  pipdazetime[ipip] = fget_next_int( fileread );
  pipgrogtime[ipip] = fget_next_int( fileread );
  pipspawnenchant[ipip] = fget_next_bool( fileread );
  pipcauseknockback[ipip] = fget_next_bool( fileread );
  pipcausepancake[ipip] = fget_next_bool( fileread );
  pipneedtarget[ipip] = fget_next_bool( fileread );
  piptargetcaster[ipip] = fget_next_bool( fileread );
  pipstartontarget[ipip] = fget_next_bool( fileread );
  piponlydamagefriendly[ipip] = fget_next_bool( fileread );

  iTmp = fget_next_int( fileread ); pipsoundspawn[ipip] = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); pipsoundend[ipip]   = FIX_SOUND( iTmp );

  pipfriendlyfire[ipip] = fget_next_bool( fileread );
  piphateonly[ipip] = fget_next_bool( fileread );
  pipnewtargetonspawn[ipip] = fget_next_bool( fileread );
  piptargetangle[ipip] = fget_next_int( fileread ) >> 1;
  piphoming[ipip] = fget_next_bool( fileread );
  piphomingfriction[ipip] = fget_next_float( fileread );
  piphomingaccel[ipip] = fget_next_float( fileread );
  piprotatetoface[ipip] = fget_next_bool( fileread );
  fgoto_colon( fileread );   //BAD! Not used
  pipmanadrain[numpip] = fget_next_fixed( fileread );   //Mana drain (Mana damage)
  piplifedrain[numpip] = fget_next_fixed( fileread );   //Life drain (Mana damage)



  // Clear expansions...
  pipzaimspd[ipip]     = 0;
  pipsoundfloor[ipip] = INVALID_SOUND;
  pipsoundwall[ipip]  = INVALID_SOUND;
  pipendwall[ipip]    = pipendground[ipip];
  pipdamfx[ipip]      = DAMFX_TURN;
  if ( piphoming[ipip] )  pipdamfx[ipip] = DAMFX_NONE;
  pipallowpush[ipip] = btrue;
  pipdynalightfalloffadd[ipip] = 0;
  pipdynalightleveladd[ipip] = 0;
  pipintdamagebonus[ipip] = bfalse;
  pipwisdamagebonus[ipip] = bfalse;
  piprotatewithattached[ipip] = btrue;
  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );

    if ( MAKE_IDSZ( "TURN" ) == idsz )
      piprotatewithattached[ipip] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "ZSPD" ) == idsz )  pipzaimspd[ipip] = iTmp;
    else if ( MAKE_IDSZ( "FSND" ) == idsz )  pipsoundfloor[ipip] = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WSND" ) == idsz )  pipsoundwall[ipip] = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WEND" ) == idsz )  pipendwall[ipip] = iTmp;
    else if ( MAKE_IDSZ( "ARMO" ) == idsz )  pipdamfx[ipip] |= DAMFX_ARMO;
    else if ( MAKE_IDSZ( "BLOC" ) == idsz )  pipdamfx[ipip] |= DAMFX_BLOC;
    else if ( MAKE_IDSZ( "ARRO" ) == idsz )  pipdamfx[ipip] |= DAMFX_ARRO;
    else if ( MAKE_IDSZ( "TIME" ) == idsz )  pipdamfx[ipip] |= DAMFX_TIME;
    else if ( MAKE_IDSZ( "PUSH" ) == idsz )  pipallowpush[ipip] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DLEV" ) == idsz )  pipdynalightleveladd[ipip] = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "DRAD" ) == idsz )  pipdynalightfalloffadd[ipip] = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "IDAM" ) == idsz )  pipintdamagebonus[ipip] = iTmp;
    else if ( MAKE_IDSZ( "WDAM" ) == idsz )  pipwisdamagebonus[ipip] = iTmp;
  }


  // Make sure it's referenced properly
  madprtpip[object][local_pip] = ipip;

  fs_fileClose( fileread );

  return ipip;
}

//--------------------------------------------------------------------------------------------
void reset_particles( char* modname )
{
  // ZZ> This resets all particle data and reads in the coin and water particles
  int cnt, object;

  // Load in the standard global particles ( the coins for example )
  //BAD! This should only be needed once at the start of the game
  numpip = 0;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money1_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money5_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money25_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money100_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Load module specific information
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.weather4_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.weather5_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.splash_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.ripple_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //This is also global...
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_file );
  if ( MAXPRTPIP == load_one_particle_profile( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  // Now clear out the local pips
  for ( object = 0; object < MAXMODEL; object++ )
  {
    for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
    {
      madprtpip[object][cnt] = MAXPRTPIP;
    }
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_owner( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  prtowner[iprt] = VALIDATE_CHR( prtowner[iprt] );
  return prtowner[iprt];
};

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_target( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  prttarget[iprt] = VALIDATE_CHR( prttarget[iprt] );
  return prttarget[iprt];
};

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_attachedtochr( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  prtattachedtochr[iprt] = VALIDATE_CHR( prtattachedtochr[iprt] );
  return prtattachedtochr[iprt];
};

//--------------------------------------------------------------------------------------------
PRT_REF prt_get_bumpnext( PRT_REF iprt )
{
  PRT_REF bumpnext;

  if ( !VALID_PRT( iprt ) ) return MAXPRT;

#if defined(_DEBUG) || !defined(NDEBUG)
  bumpnext = prtbumpnext[iprt];
  if ( bumpnext < MAXPRT && !prton[bumpnext] && prtbumpnext[bumpnext] < MAXPRT )
  {
    // this is an invalid configuration
    assert( bfalse );
    return prt_get_bumpnext( bumpnext );
  }
#endif

  prtbumpnext[iprt] = VALIDATE_PRT( prtbumpnext[iprt] );
  return prtbumpnext[iprt];
};

