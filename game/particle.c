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


#include "particle.h"
#include "Log.h"
#include "mesh.h"
#include "camera.h"
#include "sound.h"
#include "char.h"
#include "enchant.h"

#include "egoboo_math.h"
#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>

DYNALIGHT_LIST GDynaLight[MAXDYNA];
PIP            PipList[MAXPRTPIP];
PRT            PrtList[MAXPRT];

int      numfreeprt = 0;                         // For allocation
PRT_REF  freeprtlist[MAXPRT];

DYNALIGHT_INFO GDyna;

int    numpip = 0;

Uint16 particletexture;                            // All in one bitmap

//--------------------------------------------------------------------------------------------
void make_prtlist( void )
{
  // ZZ> This function figures out which particles are visible, and it sets up GDyna.mic
  //     lighting
  int cnt, tnc, disx, disy, distance, slot;


  // Don't really make a list, just set to visible or not
  GDyna.count = 0;
  GDyna.distancetobeat = MAXDYNADIST;
  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    PrtList[cnt].inview = bfalse;
    if ( !VALID_PRT( cnt ) ) continue;

    PrtList[cnt].inview = mesh_in_renderlist( PrtList[cnt].onwhichfan );
    // Set up the lights we need
    if ( PrtList[cnt].dyna.on )
    {
      disx = ABS( PrtList[cnt].pos.x - GCamera.trackpos.x );
      disy = ABS( PrtList[cnt].pos.y - GCamera.trackpos.y );
      distance = disx + disy;
      if ( distance < GDyna.distancetobeat )
      {
        if ( GDyna.count < MAXDYNA )
        {
          // Just add the light
          slot = GDyna.count;
          GDynaLight[slot].distance = distance;
          GDyna.count++;
        }
        else
        {
          // Overwrite the worst one
          slot = 0;
          GDyna.distancetobeat = GDynaLight[0].distance;
          for ( tnc = 1; tnc < MAXDYNA; tnc++ )
          {
            if ( GDynaLight[tnc].distance > GDyna.distancetobeat )
            {
              slot = tnc;
            }
          }
          GDynaLight[slot].distance = distance;

          // Find the new distance to beat
          GDyna.distancetobeat = GDynaLight[0].distance;
          for ( tnc = 1; tnc < MAXDYNA; tnc++ )
          {
            if ( GDynaLight[tnc].distance > GDyna.distancetobeat )
            {
              GDyna.distancetobeat = GDynaLight[tnc].distance;
            }
          }
        }
        GDynaLight[slot].pos.x = PrtList[cnt].pos.x;
        GDynaLight[slot].pos.y = PrtList[cnt].pos.y;
        GDynaLight[slot].pos.z = PrtList[cnt].pos.z;
        GDynaLight[slot].level = PrtList[cnt].dyna.level;
        GDynaLight[slot].falloff = PrtList[cnt].dyna.falloff;
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
  PrtList[particle].on = bfalse;
}

//--------------------------------------------------------------------------------------------
void free_one_particle( PRT_REF particle )
{
  // ZZ> This function sticks a particle back on the free particle stack and
  //     plays the sound associated with the particle
  int child;
  if ( PrtList[particle].spawncharacterstate != SPAWN_NOCHARACTER )
  {
    child = spawn_one_character( PrtList[particle].pos, PrtList[particle].model, PrtList[particle].team, 0, PrtList[particle].facing, NULL, MAXCHR );
    if ( VALID_CHR( child ) )
    {
      ChrList[child].aistate = PrtList[particle].spawncharacterstate;
      ChrList[child].aiowner = prt_get_owner( particle );
    }
  }
  play_particle_sound( 1.0f, particle, PipList[PrtList[particle].pip].soundend );

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
  FILE * filewrite;
  PRT * pprt;

  if( CData.DevMode ) filewrite = fs_fileOpen( PRI_NONE, NULL, CData.debug_file, "a");

  if ( local_pip >= MAXPRTPIP )
  {
    if( CData.DevMode ) 
    {
      fprintf( stderr, "spawn_one_particle() - \n\tfailed to spawn : local_pip == %d is an invalid value\n", local_pip );
      fprintf( filewrite, "WARN: spawn_one_particle() - failed to spawn : local_pip == %d is an invalid value\n", local_pip );
      fs_fileClose( filewrite );
    }
    return MAXPRT;
  }

  // Convert from local local_pip to global local_pip
  if ( model < MAXMODEL && local_pip < PRTPIP_PEROBJECT_COUNT )
    glob_pip = MadList[model].prtpip[local_pip];

  // assume we were given a global local_pip
  if ( MAXPRTPIP == glob_pip )
    glob_pip = local_pip;



  iprt = get_free_particle( PipList[glob_pip].force );
  if ( iprt == MAXPRT )
  {
    if( CData.DevMode ) 
    {
      fprintf( stderr, "spawn_one_particle() - \n\tfailed to spawn : get_free_particle() returned invalid value %d\n", iprt );
      fprintf( filewrite, "WARN: spawn_one_particle() - failed to spawn : get_free_particle() returned invalid value %d\n", iprt );
      fs_fileClose( filewrite );
    }
    return MAXPRT;
  }

  weight = 1.0f;
  if ( VALID_CHR( characterorigin ) ) weight = MAX( weight, ChrList[characterorigin].weight );
  if ( VALID_CHR( characterattach ) ) weight = MAX( weight, ChrList[characterattach].weight );
  PrtList[iprt].weight = weight;

  //if( CData.DevMode ) 
  //{
  //  fprintf( stderr, "spawn_one_particle() - \n\tlocal local_pip == %d, global local_pip == %d, part == %d\n", local_pip, glob_pip, iprt);
  //  fprintf( filewrite, "SUCCESS: spawn_one_particle() - local local_pip == %d, global local_pip == %d, part == %d\n", local_pip, glob_pip, iprt);
  //  fs_fileClose( filewrite );
  //}

  // "simplify" the notation
  pprt = PrtList + iprt;

  // clear any old data
  memset(pprt, 0, sizeof(PRT));

  // Necessary data for any part
  pprt->on = btrue;
  pprt->gopoof = bfalse;
  pprt->pip = glob_pip;
  pprt->model = model;
  pprt->inview = bfalse;
  pprt->level = 0;
  pprt->team = team;
  pprt->owner = characterorigin;
  pprt->damagetype = PipList[glob_pip].damagetype;
  pprt->spawncharacterstate = SPAWN_NOCHARACTER;


  // Lighting and sound
  pprt->dyna.on = bfalse;
  if ( multispawn == 0 )
  {
    pprt->dyna.on = ( DYNA_OFF != PipList[glob_pip].dyna.mode );
    if ( PipList[glob_pip].dyna.mode == DYNA_LOCAL )
    {
      pprt->dyna.on = bfalse;
    }
  }
  pprt->dyna.level   = PipList[glob_pip].dyna.level * intensity;
  pprt->dyna.falloff = PipList[glob_pip].dyna.falloff;



  // Set character attachments ( characterattach==MAXCHR means none )
  pprt->attachedtochr = characterattach;
  pprt->vertoffset = grip;



  // Targeting...
  offsetfacing = 0;
  zvel = 0;
  pos.z += generate_signed( &PipList[glob_pip].zspacing );
  velocity = generate_unsigned( &PipList[glob_pip].xyvel );
  pprt->target = oldtarget;
  prt_target = MAXCHR;
  if ( PipList[glob_pip].newtargetonspawn )
  {
    if ( PipList[glob_pip].targetcaster )
    {
      // Set the target to the caster
      pprt->target = characterorigin;
    }
    else
    {
      // Correct facing for dexterity...
      if ( ChrList[characterorigin].dexterity_fp8 < PERFECTSTAT )
      {
        // Correct facing for randomness
        newrand = FP8_DIV( PERFECTSTAT - ChrList[characterorigin].dexterity_fp8,  PERFECTSTAT );
        offsetfacing += generate_dither( &PipList[glob_pip].facing, newrand );
      }

      // Find a target
      pprt->target = prt_search_target( pos.x, pos.y, facing, PipList[glob_pip].targetangle, PipList[glob_pip].onlydamagefriendly, bfalse, team, characterorigin, oldtarget );
      prt_target = prt_get_target( iprt );
      if ( VALID_CHR( prt_target ) )
      {
        offsetfacing -= g_search.useangle;

        if ( PipList[glob_pip].zaimspd != 0 )
        {
          // These aren't velocities...  This is to do aiming on the Z axis
          if ( velocity > 0 )
          {
            xvel = ChrList[prt_target].pos.x - pos.x;
            yvel = ChrList[prt_target].pos.y - pos.y;
            tvel = sqrt( xvel * xvel + yvel * yvel ) / velocity;   // This is the number of steps...
            if ( tvel > 0 )
            {
              zvel = ( ChrList[prt_target].pos.z + ( ChrList[prt_target].bmpdata.calc_size * 0.5f ) - pos.z ) / tvel;  // This is the zvel alteration
              if ( zvel < - ( PipList[glob_pip].zaimspd >> 1 ) ) zvel = - ( PipList[glob_pip].zaimspd >> 1 );
              if ( zvel > PipList[glob_pip].zaimspd ) zvel = PipList[glob_pip].zaimspd;
            }
          }
        }
      }
    }

    // Does it go away?
    if ( !VALID_CHR( prt_target ) && PipList[glob_pip].needtarget )
    {
      if( CData.DevMode ) 
      {
        fprintf( stderr, "spawn_one_particle() - \n\tfailed to spawn : pip requires target and no target specified\n", iprt );
        fprintf( filewrite, "WARN: spawn_one_particle() - failed to spawn : pip requires target and no target specified\n", iprt );
        fs_fileClose( filewrite );
      }

      free_one_particle( iprt );
      return MAXPRT;
    }

    // Start on top of target
    if ( VALID_CHR( prt_target ) && PipList[glob_pip].startontarget )
    {
      pos.x = ChrList[prt_target].pos.x;
      pos.y = ChrList[prt_target].pos.y;
    }
  }
  else
  {
    // Correct facing for randomness
    offsetfacing += generate_dither( &PipList[glob_pip].facing, INT_TO_FP8( 1 ) );
  }
  facing += PipList[glob_pip].facing.ibase + offsetfacing;
  pprt->facing = facing;
  facing >>= 2;

  if( CData.DevMode ) fs_fileClose( filewrite );


  // Location data from arguments
  newrand = generate_unsigned( &PipList[glob_pip].xyspacing );
  pos.x += turntosin[( facing+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * newrand;
  pos.y += turntosin[( facing+8192 ) & TRIGTABLE_MASK] * newrand;

  pos.x = mesh_clip_x( pos.x );
  pos.y = mesh_clip_x( pos.y );

  pprt->pos.x = pos.x;
  pprt->pos.y = pos.y;
  pprt->pos.z = pos.z;


  // Velocity data
  xvel = turntosin[( facing+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * velocity;
  yvel = turntosin[( facing+8192 ) & TRIGTABLE_MASK] * velocity;
  zvel += generate_signed( &PipList[glob_pip].zvel );
  pprt->vel.x = xvel;
  pprt->vel.y = yvel;
  pprt->vel.z = zvel;

  pprt->pos_old.x = pprt->pos.x - pprt->vel.x;
  pprt->pos_old.y = pprt->pos.y - pprt->vel.y;
  pprt->pos_old.z = pprt->pos.z - pprt->vel.z;

  // Template values
  pprt->bumpsize = PipList[glob_pip].bumpsize;
  pprt->bumpsizebig = pprt->bumpsize + ( pprt->bumpsize >> 1 );
  pprt->bumpheight = PipList[glob_pip].bumpheight;
  pprt->bumpstrength = PipList[glob_pip].bumpstrength * intensity;

  // figure out the particle type and transparency
  pprt->type = PipList[glob_pip].type;
  pprt->alpha_fp8 = 255;
  switch ( PipList[glob_pip].type )
  {
    case PRTTYPE_SOLID:
      if ( intensity < 1.0f )
      {
        pprt->type  = PRTTYPE_ALPHA;
        pprt->alpha_fp8 = 255 * intensity;
      }
      break;

    case PRTTYPE_LIGHT:
      pprt->alpha_fp8 = 255 * intensity;
      break;

    case PRTTYPE_ALPHA:
      pprt->alpha_fp8 = particletrans * intensity;
      break;
  };



  // Image data
  pprt->rotate = generate_unsigned( &PipList[glob_pip].rotate );
  pprt->rotateadd = PipList[glob_pip].rotateadd;
  pprt->size_fp8 = PipList[glob_pip].sizebase_fp8;
  pprt->sizeadd_fp8 = PipList[glob_pip].sizeadd;
  pprt->image_fp8 = 0;
  pprt->imageadd_fp8 = generate_unsigned( &PipList[glob_pip].imageadd );
  pprt->imagestt_fp8 = INT_TO_FP8( PipList[glob_pip].imagebase );
  pprt->imagemax_fp8 = INT_TO_FP8( PipList[glob_pip].numframes );
  pprt->time = PipList[glob_pip].time;
  if ( PipList[glob_pip].endlastframe )
  {
    if ( pprt->imageadd_fp8 != 0 ) pprt->time = 0.0;
  }


  // Set onwhichfan...
  pprt->onwhichfan = mesh_get_fan( pprt->pos );


  // Damage stuff
  pprt->damage.ibase = PipList[glob_pip].damage_fp8.ibase * intensity;
  pprt->damage.irand = PipList[glob_pip].damage_fp8.irand * intensity;


  // Spawning data
  pprt->spawntime = PipList[glob_pip].contspawntime;
  if ( pprt->spawntime != 0 )
  {
    CHR_REF prt_attachedto = prt_get_attachedtochr( iprt );

    pprt->spawntime = 1;
    if ( VALID_CHR( prt_attachedto ) )
    {
      pprt->spawntime++; // Because attachment takes an update before it happens
    }
  }


  // Sound effect
  play_particle_sound( intensity, iprt, PipList[glob_pip].soundspawn );

  return iprt;
}

//--------------------------------------------------------------------------------------------
Uint32 __prthitawall( PRT_REF particle, vect3 * norm )
{
  // ZZ> This function returns nonzero if the particle hit a wall

  Uint32 retval, collision_bits;

  if ( !VALID_PRT( particle ) ) return 0;

  collision_bits = MESHFX_IMPASS;
  if ( 0 != PipList[PrtList[particle].pip].bumpmoney )
  {
    collision_bits |= MESHFX_WALL;
  }

  retval = mesh_hitawall( PrtList[particle].pos, PrtList[particle].bumpsize, PrtList[particle].bumpsize, collision_bits );

  if( 0!=retval && NULL!=norm )
  {
    vect3 pos;

    norm->x = norm->y = norm->z = 0;

    pos.x = PrtList[particle].pos.x;
    pos.y = PrtList[particle].pos_old.y;
    pos.z = PrtList[particle].pos_old.z;

    if( 0!=mesh_hitawall( pos, PrtList[particle].bumpsize, PrtList[particle].bumpsize, collision_bits ) )
    {
      norm->x = SGN(PrtList[particle].pos.x - PrtList[particle].pos_old.x);
    }

    pos.x = PrtList[particle].pos_old.x;
    pos.y = PrtList[particle].pos.y;
    pos.z = PrtList[particle].pos_old.z;

    if( 0!=mesh_hitawall( pos, PrtList[particle].bumpsize, PrtList[particle].bumpsize, collision_bits ) )
    {
      norm->y = SGN(PrtList[particle].pos.y - PrtList[particle].pos_old.y);
    }

    pos.x = PrtList[particle].pos_old.x;
    pos.y = PrtList[particle].pos_old.y;
    pos.z = PrtList[particle].pos.z;

    if( 0!=mesh_hitawall( pos, PrtList[particle].bumpsize, PrtList[particle].bumpsize, collision_bits ) )
    {
      norm->z = SGN(PrtList[particle].pos.z - PrtList[particle].pos_old.z);
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
      PrtList[particle].gopoof        = btrue;
      PrtList[particle].attachedtochr = MAXCHR;
      useful = btrue;
    }
  }

  // Set the alert for disaffirmation ( wet torch )
  if ( useful )
  {
    ChrList[character].alert |= ALERT_DISAFFIRMED;
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
  while ( numberattached < CapList[ChrList[character].model].attachedprtamount )
  {
    particle = spawn_one_particle( 1.0f, ChrList[character].pos, 0, ChrList[character].model, CapList[ChrList[character].model].attachedprttype, character, GRIP_LAST + numberattached, ChrList[character].team, character, numberattached, MAXCHR );
    if ( particle != MAXPRT )
    {
      attach_particle_to_character( particle, character, PrtList[particle].vertoffset );
    }
    numberattached++;
  }

  // Set the alert for reaffirmation ( for exploding barrels with fire )
  ChrList[character].alert |= ALERT_REAFFIRMED;
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
    if ( !VALID_PRT( iprt ) || !PrtList[iprt].gopoof ) continue;

    // To make it easier
    pip = PrtList[iprt].pip;
    facing = PrtList[iprt].facing;
    prt_target = prt_get_target( iprt );
    prt_owner = prt_get_owner( iprt );
    prt_attachedto = prt_get_attachedtochr( iprt );

    for ( tnc = 0; tnc < PipList[pip].endspawnamount; tnc++ )
    {
      spawn_one_particle( 1.0f, PrtList[iprt].pos,
                          facing, PrtList[iprt].model, PipList[pip].endspawnpip,
                          MAXCHR, GRIP_LAST, PrtList[iprt].team, prt_owner, tnc, prt_target );
      facing += PipList[pip].endspawnfacingadd;
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

    PrtList[iprt].pos_old = PrtList[iprt].pos;

    PrtList[iprt].onwhichfan = INVALID_FAN;
    PrtList[iprt].level = 0;
    fan = mesh_get_fan( PrtList[iprt].pos );
    PrtList[iprt].onwhichfan = fan;
    PrtList[iprt].level = ( INVALID_FAN == fan ) ? 0 : mesh_get_level( fan, PrtList[iprt].pos.x, PrtList[iprt].pos.y, bfalse );

    // To make it easier
    pip = PrtList[iprt].pip;

    loc_homingfriction = pow( PipList[pip].homingfriction, dUpdate );
    loc_homingaccel    = PipList[pip].homingaccel;

    // Animate particle
    PrtList[iprt].image_fp8 += PrtList[iprt].imageadd_fp8 * dUpdate;
    if ( PrtList[iprt].image_fp8 >= PrtList[iprt].imagemax_fp8 )
    {
      if ( PipList[pip].endlastframe )
      {
        PrtList[iprt].image_fp8 = PrtList[iprt].imagemax_fp8 - INT_TO_FP8( 1 );
        PrtList[iprt].gopoof    = btrue;
      }
      else if ( PrtList[iprt].imagemax_fp8 > 0 )
      {
        // if the PrtList[].image_fp8 is a fraction of an image over PrtList[].imagemax_fp8,
        // keep the fraction
        PrtList[iprt].image_fp8 %= PrtList[iprt].imagemax_fp8;
      }
      else
      {
        // a strange case
        PrtList[iprt].image_fp8 = 0;
      }
    };

    PrtList[iprt].rotate += PrtList[iprt].rotateadd * dUpdate;
    PrtList[iprt].size_fp8 = ( PrtList[iprt].size_fp8 + PrtList[iprt].sizeadd_fp8 < 0 ) ? 0 : PrtList[iprt].size_fp8 + PrtList[iprt].sizeadd_fp8;

    // Change dyna light values
    PrtList[iprt].dyna.level   += PipList[pip].dyna.leveladd * dUpdate;
    PrtList[iprt].dyna.falloff += PipList[pip].dyna.falloffadd * dUpdate;


    // Make it sit on the floor...  Shift is there to correct for sprite size
    level = PrtList[iprt].level + FP8_TO_FLOAT( PrtList[iprt].size_fp8 ) * 0.5f;

    // do interaction with the floor
    if(  !VALID_CHR( prt_attachedto ) && PrtList[iprt].pos.z > level )
    {
      float lerp = ( PrtList[iprt].pos.z - ( PrtList[iprt].level + PLATTOLERANCE ) ) / ( float ) PLATTOLERANCE;
      if ( lerp < 0.2f ) lerp = 0.2f;
      if ( lerp > 1.0f ) lerp = 1.0f;

      PrtList[iprt].accum_acc.z += gravity * lerp;

      PrtList[iprt].accum_acc.x -= ( 1.0f - noslipfriction ) * lerp * PrtList[iprt].vel.x;
      PrtList[iprt].accum_acc.y -= ( 1.0f - noslipfriction ) * lerp * PrtList[iprt].vel.y;
    }

    // Do speed limit on Z
    if ( PrtList[iprt].vel.z < -PipList[pip].spdlimit )  PrtList[iprt].accum_vel.z += -PipList[pip].spdlimit - PrtList[iprt].vel.z;

    // Do homing
    if ( PipList[pip].homing && VALID_CHR( prt_target ) )
    {
      if ( !ChrList[prt_target].alive )
      {
        PrtList[iprt].gopoof = btrue;
      }
      else
      {
        if ( !VALID_CHR( prt_attachedto ) )
        {
          PrtList[iprt].accum_acc.x += -(1.0f-loc_homingfriction) * PrtList[iprt].vel.x;
          PrtList[iprt].accum_acc.y += -(1.0f-loc_homingfriction) * PrtList[iprt].vel.y;
          PrtList[iprt].accum_acc.z += -(1.0f-loc_homingfriction) * PrtList[iprt].vel.z;

          PrtList[iprt].accum_acc.x += ( ChrList[prt_target].pos.x - PrtList[iprt].pos.x ) * loc_homingaccel * 4.0f;
          PrtList[iprt].accum_acc.y += ( ChrList[prt_target].pos.y - PrtList[iprt].pos.y ) * loc_homingaccel * 4.0f;
          PrtList[iprt].accum_acc.z += ( ChrList[prt_target].pos.z + ( ChrList[prt_target].bmpdata.calc_height * 0.5f ) - PrtList[iprt].pos.z ) * loc_homingaccel * 4.0f;
        }
      }
    }


    // Spawn new particles if continually spawning
    if ( PipList[pip].contspawnamount > 0.0f )
    {
      PrtList[iprt].spawntime -= dUpdate;
      if ( PrtList[iprt].spawntime <= 0.0f )
      {
        PrtList[iprt].spawntime = PipList[pip].contspawntime;
        facing = PrtList[iprt].facing;
        tnc = 0;
        while ( tnc < PipList[pip].contspawnamount )
        {
          particle = spawn_one_particle( 1.0f, PrtList[iprt].pos,
                                         facing, PrtList[iprt].model, PipList[pip].contspawnpip,
                                         MAXCHR, GRIP_LAST, PrtList[iprt].team, prt_get_owner( iprt ), tnc, prt_target );
          if ( PipList[PrtList[iprt].pip].facingadd != 0 && particle < MAXPRT )
          {
            // Hack to fix velocity
            PrtList[particle].vel.x += PrtList[iprt].vel.x;
            PrtList[particle].vel.y += PrtList[iprt].vel.y;
          }
          facing += PipList[pip].contspawnfacingadd;
          tnc++;
        }
      }
    }


    // Check underwater
    if ( PrtList[iprt].pos.z < GWater.douselevel && mesh_has_some_bits( PrtList[iprt].onwhichfan, MESHFX_WATER ) && PipList[pip].endwater )
    {
      vect3 prt_pos = {PrtList[iprt].pos.x, PrtList[iprt].pos.y, GWater.surfacelevel};

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
        PrtList[iprt].gopoof = btrue;
      }
    }

    // Down the particle timer
    if ( PrtList[iprt].time > 0.0f )
    {
      PrtList[iprt].time -= dUpdate;
      if ( PrtList[iprt].time <= 0.0f ) PrtList[iprt].gopoof = btrue;
    };

    PrtList[iprt].facing += PipList[pip].facingadd * dUpdate;
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

    attach_particle_to_character( cnt, prt_attachedto, PrtList[cnt].vertoffset );

    // Correct facing so swords knock characters in the right direction...
    if ( HAS_SOME_BITS( PipList[PrtList[cnt].pip].damfx, DAMFX_TURN ) )
      PrtList[cnt].facing = ChrList[prt_attachedto].turn_lr;
  }
}

//--------------------------------------------------------------------------------------------
void free_all_particles()
{
  // ZZ> This function resets the particle allocation lists
  numfreeprt = 0;
  while ( numfreeprt < MAXPRT )
  {
    PrtList[numfreeprt].on = bfalse;
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
    direction = ChrList[character].turn_lr - vec_to_turn( -PrtList[particle].vel.x, -PrtList[particle].vel.y );
    if ( HAS_SOME_BITS( MadList[ChrList[character].model].framefx[ChrList[character].anim.next], MADFX_INVICTUS ) )
    {
      // I Frame
      if ( HAS_SOME_BITS( PipList[pip].damfx, DAMFX_BLOC ) )
      {
        left  = UINT16_MAX;
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
        spawn_enchant( prt_get_owner( particle ), character, MAXCHR, MAXENCHANT, PrtList[particle].model );
      }

      // Spawn particles
      if ( amount != 0 && !CapList[ChrList[character].model].resistbumpspawn && !ChrList[character].invictus &&
           vertices != 0 && ( ChrList[character].damagemodifier_fp8[PrtList[particle].damagetype]&DAMAGE_SHIFT ) != DAMAGE_SHIFT )
      {
        if ( amount == 1 )
        {
          // A single particle ( arrow? ) has been stuck in the character...
          // Find best vertex to attach to

          Uint32 ilast, inext;
          MD2_Model * pmdl;
          MD2_Frame * plast, * pnext;
          float flip;

          model = ChrList[character].model;
          inext = ChrList[character].anim.next;
          ilast = ChrList[character].anim.last;
          flip = ChrList[character].anim.flip;

          assert( MAXMODEL != VALIDATE_MDL( model ) );

          pmdl  = MadList[model].md2_ptr;
          plast = md2_get_Frame(pmdl, ilast);
          pnext = md2_get_Frame(pmdl, inext);

          bestvertex = 0;
          bestdistance = 9999999;
          z =  PrtList[particle].pos.z - ( ChrList[character].pos.z + RAISE );
          facing = PrtList[particle].facing - ChrList[character].turn_lr - 16384;
          facing >>= 2;
          fsin = turntosin[facing & TRIGTABLE_MASK];
          fcos = turntosin[( facing+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
          y = 8192;
          x = -y * fsin;
          y = y * fcos;

          for (cnt = 0; cnt < vertices; cnt++ )
          {
            vect3 vpos;

            vpos.x = pnext->vertices[cnt].x + (plast->vertices[cnt].x - pnext->vertices[cnt].x)*flip;
            vpos.y = pnext->vertices[cnt].y + (plast->vertices[cnt].y - pnext->vertices[cnt].y)*flip;
            vpos.z = pnext->vertices[cnt].z + (plast->vertices[cnt].z - pnext->vertices[cnt].z)*flip;

            distance = ABS( x - vpos.x ) + ABS( y - vpos.y ) + ABS( z - vpos.z );
            if ( distance < bestdistance )
            {
              bestdistance = distance;
              bestvertex = cnt;
            }

          }

          spawn_one_particle( 1.0f, ChrList[character].pos, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                              character, bestvertex + 1, PrtList[particle].team, prt_get_owner( particle ), cnt, character );
        }
        else
        {
          amount = ( amount * vertices ) >> 5;  // Correct amount for size of character
          cnt = 0;
          while ( cnt < amount )
          {
            spawn_one_particle( 1.0f, ChrList[character].pos, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                                character, rand() % vertices, PrtList[particle].team, prt_get_owner( particle ), cnt, character );
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
    fan = mesh_get_fan( PrtList[cnt].pos );
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

  if ( GWeather.time > 0 )
  {
    GWeather.time -= dUpdate;
    if ( GWeather.time < 0 ) GWeather.time = 0;

    if ( GWeather.time == 0 )
    {
      GWeather.time = GWeather.timereset;

      // Find a valid player
      foundone = bfalse;
      cnt = 0;
      while ( cnt < MAXPLAYER )
      {
        GWeather.player = ( GWeather.player + 1 ) % MAXPLAYER;
        if ( VALID_PLA( GWeather.player ) )
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
      cnt = pla_get_character( GWeather.player );
      if ( VALID_CHR( cnt ) && !chr_in_pack( cnt ) )
      {
        // Yes, so spawn over that character
        particle = spawn_one_particle( 1.0f, ChrList[cnt].pos, 0, MAXMODEL, PRTPIP_WEATHER_1, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
        if ( GWeather.overwater && particle != MAXPRT )
        {
          if ( !prt_is_over_water( particle ) )
          {
            free_one_particle_no_sound( particle );
          }
        }
      }
    }
  }
  GCamera.swing = ( GCamera.swing + GCamera.swingrate ) & TRIGTABLE_MASK;
}


//--------------------------------------------------------------------------------------------
Uint32 load_one_pip( char *szLoadName, Uint16 object, int local_pip )
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
  strncpy( PipList[ipip].fname, szLoadName, sizeof( PipList[ipip].fname ) );
  fgets( PipList[ipip].comment, sizeof( PipList[ipip].comment ), fileread );
  rewind( fileread );
  if ( PipList[ipip].comment[0] != '/' )  PipList[ipip].comment[0] = '\0';

  // General data
  globalname = szLoadName;
  PipList[ipip].force = fget_next_bool( fileread );
  PipList[ipip].type = fget_next_prttype( fileread );
  PipList[ipip].imagebase = fget_next_int( fileread );
  PipList[ipip].numframes = fget_next_int( fileread );
  PipList[ipip].imageadd.ibase = fget_next_int( fileread );
  PipList[ipip].imageadd.irand = fget_next_int( fileread );
  PipList[ipip].rotate.ibase = fget_next_int( fileread );
  PipList[ipip].rotate.irand = fget_next_int( fileread );
  PipList[ipip].rotateadd = fget_next_int( fileread );
  PipList[ipip].sizebase_fp8 = fget_next_int( fileread );
  PipList[ipip].sizeadd = fget_next_int( fileread );
  PipList[ipip].spdlimit = fget_next_float( fileread );
  PipList[ipip].facingadd = fget_next_int( fileread );


  // Ending conditions
  PipList[ipip].endwater = fget_next_bool( fileread );
  PipList[ipip].endbump = fget_next_bool( fileread );
  PipList[ipip].endground = fget_next_bool( fileread );
  PipList[ipip].endlastframe = fget_next_bool( fileread );
  PipList[ipip].time = fget_next_int( fileread );


  // Collision data
  PipList[ipip].dampen = fget_next_float( fileread );
  PipList[ipip].bumpmoney = fget_next_int( fileread );
  PipList[ipip].bumpsize = fget_next_int( fileread );
  PipList[ipip].bumpheight = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &PipList[ipip].damage_fp8 );
  PipList[ipip].damagetype = fget_next_damage( fileread );
  PipList[ipip].bumpstrength = 1.0f;
  if ( PipList[ipip].bumpsize == 0 )
  {
    PipList[ipip].bumpstrength = 0.0f;
    PipList[ipip].bumpsize   = 0.5f * FP8_TO_FLOAT( PipList[ipip].sizebase_fp8 );
    PipList[ipip].bumpheight = 0.5f * FP8_TO_FLOAT( PipList[ipip].sizebase_fp8 );
  };

  // Lighting data
  PipList[ipip].dyna.mode = fget_next_dynamode( fileread );
  PipList[ipip].dyna.level = fget_next_float( fileread );
  PipList[ipip].dyna.falloff = fget_next_int( fileread );
  if ( PipList[ipip].dyna.falloff > MAXFALLOFF )  PipList[ipip].dyna.falloff = MAXFALLOFF;



  // Initial spawning of this particle
  PipList[ipip].facing.ibase    = fget_next_int( fileread );
  PipList[ipip].facing.irand    = fget_next_int( fileread );
  PipList[ipip].xyspacing.ibase = fget_next_int( fileread );
  PipList[ipip].xyspacing.irand = fget_next_int( fileread );
  PipList[ipip].zspacing.ibase  = fget_next_int( fileread );
  PipList[ipip].zspacing.irand  = fget_next_int( fileread );
  PipList[ipip].xyvel.ibase     = fget_next_int( fileread );
  PipList[ipip].xyvel.irand     = fget_next_int( fileread );
  PipList[ipip].zvel.ibase      = fget_next_int( fileread );
  PipList[ipip].zvel.irand      = fget_next_int( fileread );


  // Continuous spawning of other particles
  PipList[ipip].contspawntime = fget_next_int( fileread );
  PipList[ipip].contspawnamount = fget_next_int( fileread );
  PipList[ipip].contspawnfacingadd = fget_next_int( fileread );
  PipList[ipip].contspawnpip = fget_next_int( fileread );


  // End spawning of other particles
  PipList[ipip].endspawnamount = fget_next_int( fileread );
  PipList[ipip].endspawnfacingadd = fget_next_int( fileread );
  PipList[ipip].endspawnpip = fget_next_int( fileread );

  // Bump spawning of attached particles
  PipList[ipip].bumpspawnamount = fget_next_int( fileread );
  PipList[ipip].bumpspawnpip = fget_next_int( fileread );


  // Random stuff  !!!BAD!!! Not complete
  PipList[ipip].dazetime = fget_next_int( fileread );
  PipList[ipip].grogtime = fget_next_int( fileread );
  PipList[ipip].spawnenchant = fget_next_bool( fileread );
  PipList[ipip].causeknockback = fget_next_bool( fileread );
  PipList[ipip].causepancake = fget_next_bool( fileread );
  PipList[ipip].needtarget = fget_next_bool( fileread );
  PipList[ipip].targetcaster = fget_next_bool( fileread );
  PipList[ipip].startontarget = fget_next_bool( fileread );
  PipList[ipip].onlydamagefriendly = fget_next_bool( fileread );

  iTmp = fget_next_int( fileread ); PipList[ipip].soundspawn = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); PipList[ipip].soundend   = FIX_SOUND( iTmp );

  PipList[ipip].friendlyfire = fget_next_bool( fileread );
  PipList[ipip].hateonly = fget_next_bool( fileread );
  PipList[ipip].newtargetonspawn = fget_next_bool( fileread );
  PipList[ipip].targetangle = fget_next_int( fileread ) >> 1;
  PipList[ipip].homing = fget_next_bool( fileread );
  PipList[ipip].homingfriction = fget_next_float( fileread );
  PipList[ipip].homingaccel = fget_next_float( fileread );
  PipList[ipip].rotatetoface = fget_next_bool( fileread );
  fgoto_colon( fileread );   //BAD! Not used
  PipList[numpip].manadrain = fget_next_fixed( fileread );   //Mana drain (Mana damage)
  PipList[numpip].lifedrain = fget_next_fixed( fileread );   //Life drain (Life steal)



  // Clear expansions...
  PipList[ipip].zaimspd     = 0;
  PipList[ipip].soundfloor = INVALID_SOUND;
  PipList[ipip].soundwall  = INVALID_SOUND;
  PipList[ipip].endwall    = PipList[ipip].endground;
  PipList[ipip].damfx      = DAMFX_TURN;
  if ( PipList[ipip].homing )  PipList[ipip].damfx = DAMFX_NONE;
  PipList[ipip].allowpush = btrue;
  PipList[ipip].dyna.falloffadd = 0;
  PipList[ipip].dyna.leveladd = 0;
  PipList[ipip].intdamagebonus = bfalse;
  PipList[ipip].wisdamagebonus = bfalse;
  PipList[ipip].rotatewithattached = btrue;
  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );

    if ( MAKE_IDSZ( "TURN" ) == idsz ) PipList[ipip].rotatewithattached = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "ZSPD" ) == idsz )  PipList[ipip].zaimspd = iTmp;
    else if ( MAKE_IDSZ( "FSND" ) == idsz )  PipList[ipip].soundfloor = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WSND" ) == idsz )  PipList[ipip].soundwall = FIX_SOUND( iTmp );
    else if ( MAKE_IDSZ( "WEND" ) == idsz )  PipList[ipip].endwall = iTmp;
    else if ( MAKE_IDSZ( "ARMO" ) == idsz )  PipList[ipip].damfx |= DAMFX_ARMO;
    else if ( MAKE_IDSZ( "BLOC" ) == idsz )  PipList[ipip].damfx |= DAMFX_BLOC;
    else if ( MAKE_IDSZ( "ARRO" ) == idsz )  PipList[ipip].damfx |= DAMFX_ARRO;
    else if ( MAKE_IDSZ( "TIME" ) == idsz )  PipList[ipip].damfx |= DAMFX_TIME;
    else if ( MAKE_IDSZ( "PUSH" ) == idsz )  PipList[ipip].allowpush = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DLEV" ) == idsz )  PipList[ipip].dyna.leveladd = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "DRAD" ) == idsz )  PipList[ipip].dyna.falloffadd = iTmp / 1000.0;
    else if ( MAKE_IDSZ( "IDAM" ) == idsz )  PipList[ipip].intdamagebonus = iTmp;
    else if ( MAKE_IDSZ( "WDAM" ) == idsz )  PipList[ipip].wisdamagebonus = iTmp;
  }


  // Make sure it's referenced properly
  MadList[object].prtpip[local_pip] = ipip;

  fs_fileClose( fileread );

  return ipip;
}

//--------------------------------------------------------------------------------------------
void load_global_particles()
{
  // ZF> Load in the standard global particles ( the coins for example )
  // This should only be needed done once at the start of the game

  log_info("load_global_particles() - Loading global particles into memory... ");
  //Defend particle
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 1
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money1_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 5
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money5_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 25
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money25_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Money 100
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money100_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_message("Failed!\n");
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Everything went fine
  log_message("Succeeded!\n");

}

//--------------------------------------------------------------------------------------------
void reset_particles( char* modname )
{
  // ZZ> This resets all particle data and reads in the coin and water particles
  int cnt, object;

  // Load in the standard global particles ( the coins for example )
  //BAD! This should only be needed once at the start of the game, using load_global_particles
  numpip = 0;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money1_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money5_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money25_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money100_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //Load module specific information
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.weather4_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.weather5_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.splash_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s%s/%s", modname, CData.gamedat_dir, CData.ripple_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  //This is also global...
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_file );
  if ( MAXPRTPIP == load_one_pip( CStringTmp1, 0, 0 ) )
  {
    log_error( "Data file was not found! (%s)\n", CStringTmp1 );
  }

  // Now clear out the local pips
  for ( object = 0; object < MAXMODEL; object++ )
  {
    for ( cnt = 0; cnt < PRTPIP_PEROBJECT_COUNT; cnt++ )
    {
      MadList[object].prtpip[cnt] = MAXPRTPIP;
    }
  }
}

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_owner( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  PrtList[iprt].owner = VALIDATE_CHR( PrtList[iprt].owner );
  return PrtList[iprt].owner;
};

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_target( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  PrtList[iprt].target = VALIDATE_CHR( PrtList[iprt].target );
  return PrtList[iprt].target;
};

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_attachedtochr( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  PrtList[iprt].attachedtochr = VALIDATE_CHR( PrtList[iprt].attachedtochr );
  return PrtList[iprt].attachedtochr;
};

//--------------------------------------------------------------------------------------------
PRT_REF prt_get_bumpnext( PRT_REF iprt )
{
  PRT_REF bumpnext;

  if ( !VALID_PRT( iprt ) ) return MAXPRT;

#if defined(_DEBUG) || !defined(NDEBUG)
  bumpnext = PrtList[iprt].bumpnext;
  if ( bumpnext < MAXPRT && !PrtList[bumpnext].on && PrtList[bumpnext].bumpnext < MAXPRT )
  {
    // this is an invalid configuration
    assert( bfalse );
    return prt_get_bumpnext( bumpnext );
  }
#endif

  PrtList[iprt].bumpnext = VALIDATE_PRT( PrtList[iprt].bumpnext );
  return PrtList[iprt].bumpnext;
};


//--------------------------------------------------------------------------------------------
bool_t prt_calculate_bumpers(PRT_REF iprt)
{
  float ftmp;

  if( !VALID_PRT(iprt) ) return bfalse;

  PrtList[iprt].bmpdata.mids_lo = PrtList[iprt].pos;

  // calculate the particle radius
  ftmp = FP8_TO_FLOAT(PrtList[iprt].size_fp8) * 0.5f;

  // calculate the "perfect" bbox for a sphere
  PrtList[iprt].bmpdata.cv.x_min = PrtList[iprt].pos.x - ftmp - 0.001f;
  PrtList[iprt].bmpdata.cv.x_max = PrtList[iprt].pos.x + ftmp + 0.001f;

  PrtList[iprt].bmpdata.cv.y_min = PrtList[iprt].pos.y - ftmp - 0.001f;
  PrtList[iprt].bmpdata.cv.y_max = PrtList[iprt].pos.y + ftmp + 0.001f;

  PrtList[iprt].bmpdata.cv.z_min = PrtList[iprt].pos.z - ftmp - 0.001f;
  PrtList[iprt].bmpdata.cv.z_max = PrtList[iprt].pos.z + ftmp + 0.001f;

  PrtList[iprt].bmpdata.cv.xy_min = PrtList[iprt].pos.x - ftmp * SQRT_TWO - 0.001f;
  PrtList[iprt].bmpdata.cv.xy_max = PrtList[iprt].pos.x + ftmp * SQRT_TWO + 0.001f;

  PrtList[iprt].bmpdata.cv.yx_min = PrtList[iprt].pos.y - ftmp * SQRT_TWO - 0.001f;
  PrtList[iprt].bmpdata.cv.yx_max = PrtList[iprt].pos.y + ftmp * SQRT_TWO + 0.001f;

  return btrue;
};

//--------------------------------------------------------------------------------------------
//void play_particle_sound( float intensity, PRT_REF particle, Sint8 sound )
//{
//  //This function plays a sound effect for a particle
//  if ( INVALID_SOUND == sound ) return;
//
//  //Play local sound or else global (coins for example)
//  if ( MAXMODEL != PrtList[particle].model )
//  {
//    play_sound( intensity, PrtList[particle].pos, CapList[PrtList[particle].model].wavelist[sound], 0  );
//  }
//  else
//  {
//    play_sound( intensity, PrtList[particle].pos, globalwave[sound], 0  );
//  };
//}