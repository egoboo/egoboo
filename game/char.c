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

/* Egoboo - char.c
 */

#include "char.h"
#include "enchant.h"
#include "egoboo.h"
#include "log.h"
#include "script.h"
#include "menu.h"
#include "sound.h"
#include "camera.h"
#include "egoboo_math.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void do_level_up( Uint16 character );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void flash_character_height( Uint16 character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
    // ZZ> This function sets a character's lighting depending on vertex height...
    //     Can make feet dark and head light...
    int cnt;
    Uint16 frame;
    Sint16 z;

    frame = chr[character].frame;

    for ( cnt = 0; cnt < madtransvertices[chr[character].model]; cnt++  )
    {
        z = madvrtz[frame][cnt];
        if ( z < low )
        {
            chr[character].vrta[cnt] = valuelow;
        }
        else
        {
            if ( z > high )
            {
                chr[character].vrta[cnt] = valuehigh;
            }
            else
            {
                chr[character].vrta[cnt] = ( valuehigh * ( z - low ) / ( high - low ) ) +
                                           ( valuelow * ( high - z ) / ( high - low ) );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
    // ZZ> This function keeps weapons near their holders
    int cnt, character;

    // !!!BAD!!!  May need to do 3 levels of attachment...
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on )
        {
            character = chr[cnt].attachedto;
            if ( character == MAXCHR )
            {
                // Keep inventory with character
                if ( !chr[cnt].inpack )
                {
                    character = chr[cnt].nextinpack;

                    while ( character != MAXCHR )
                    {
                        chr[character].xpos = chr[cnt].xpos;
                        chr[character].ypos = chr[cnt].ypos;
                        chr[character].zpos = chr[cnt].zpos;
                        // Copy olds to make SendMessageNear work
                        chr[character].oldx = chr[cnt].xpos;
                        chr[character].oldy = chr[cnt].ypos;
                        chr[character].oldz = chr[cnt].zpos;
                        character = chr[character].nextinpack;
                    }
                }
            }
            else
            {
                // Keep in hand weapons with character
                if ( chr[character].matrixvalid && chr[cnt].matrixvalid )
                {
                    chr[cnt].xpos = chr[cnt].matrix.CNV( 3, 0 );
                    chr[cnt].ypos = chr[cnt].matrix.CNV( 3, 1 );
                    chr[cnt].zpos = chr[cnt].matrix.CNV( 3, 2 );
                }
                else
                {
                    chr[cnt].xpos = chr[character].xpos;
                    chr[cnt].ypos = chr[character].ypos;
                    chr[cnt].zpos = chr[character].zpos;
                }

                chr[cnt].turnleftright = chr[character].turnleftright;

                // Copy this stuff ONLY if it's a weapon, not for mounts
                if ( chr[character].transferblend && chr[cnt].isitem )
                {

                    // Items become partially invisible in hands of players
                    if ( chr[character].isplayer && chr[character].alpha != 255 )
                        chr[cnt].alpha = 128;
                    else
                    {
                        // Only if not naturally transparent
                        if ( capalpha[chr[cnt].model] == 255 )
                            chr[cnt].alpha = chr[character].alpha;
                        else chr[cnt].alpha = capalpha[chr[cnt].model];
                    }

                    //Do light too
                    if ( chr[character].isplayer && chr[character].light != 255 )
                        chr[cnt].light = 128;
                    else
                    {
                        // Only if not naturally transparent
                        if ( caplight[chr[cnt].model] == 255 )
                            chr[cnt].light = chr[character].light;
                        else chr[cnt].light = caplight[chr[cnt].model];
                    }
                }
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix( Uint16 cnt )
{
    // ZZ> This function sets one character's matrix
    Uint16 tnc;
    chr[cnt].matrixvalid = btrue;
    if ( chr[cnt].overlay )
    {
        // Overlays are kept with their target...
        tnc = chr[cnt].ai.target;
        chr[cnt].xpos = chr[tnc].xpos;
        chr[cnt].ypos = chr[tnc].ypos;
        chr[cnt].zpos = chr[tnc].zpos;
        chr[cnt].matrix.CNV( 0, 0 ) = chr[tnc].matrix.CNV( 0, 0 );
        chr[cnt].matrix.CNV( 0, 1 ) = chr[tnc].matrix.CNV( 0, 1 );
        chr[cnt].matrix.CNV( 0, 2 ) = chr[tnc].matrix.CNV( 0, 2 );
        chr[cnt].matrix.CNV( 0, 3 ) = chr[tnc].matrix.CNV( 0, 3 );
        chr[cnt].matrix.CNV( 1, 0 ) = chr[tnc].matrix.CNV( 1, 0 );
        chr[cnt].matrix.CNV( 1, 1 ) = chr[tnc].matrix.CNV( 1, 1 );
        chr[cnt].matrix.CNV( 1, 2 ) = chr[tnc].matrix.CNV( 1, 2 );
        chr[cnt].matrix.CNV( 1, 3 ) = chr[tnc].matrix.CNV( 1, 3 );
        chr[cnt].matrix.CNV( 2, 0 ) = chr[tnc].matrix.CNV( 2, 0 );
        chr[cnt].matrix.CNV( 2, 1 ) = chr[tnc].matrix.CNV( 2, 1 );
        chr[cnt].matrix.CNV( 2, 2 ) = chr[tnc].matrix.CNV( 2, 2 );
        chr[cnt].matrix.CNV( 2, 3 ) = chr[tnc].matrix.CNV( 2, 3 );
        chr[cnt].matrix.CNV( 3, 0 ) = chr[tnc].matrix.CNV( 3, 0 );
        chr[cnt].matrix.CNV( 3, 1 ) = chr[tnc].matrix.CNV( 3, 1 );
        chr[cnt].matrix.CNV( 3, 2 ) = chr[tnc].matrix.CNV( 3, 2 );
        chr[cnt].matrix.CNV( 3, 3 ) = chr[tnc].matrix.CNV( 3, 3 );
    }
    else
    {
        chr[cnt].matrix = ScaleXYZRotateXYZTranslate( chr[cnt].fat, chr[cnt].fat, chr[cnt].fat,
                          chr[cnt].turnleftright >> 2,
                          ( ( Uint16 ) ( chr[cnt].turnmapud + 32768 ) ) >> 2,
                          ( ( Uint16 ) ( chr[cnt].turnmaplr + 32768 ) ) >> 2,
                          chr[cnt].xpos, chr[cnt].ypos, chr[cnt].zpos );
    }
}

//--------------------------------------------------------------------------------------------
void free_one_character( Uint16 character )
{
    // ZZ> This function sticks a character back on the free character stack
    int cnt;

    freechrlist[numfreechr] = character;
    numfreechr++;

    // Remove from stat list
    if ( chr[character].staton )
    {
        chr[character].staton = bfalse;
        cnt = 0;

        while ( cnt < numstat )
        {
            if ( statlist[cnt] == character )
            {
                cnt++;

                while ( cnt < numstat )
                {
                    statlist[cnt-1] = statlist[cnt];
                    cnt++;
                }

                numstat--;
            }

            cnt++;
        }
    }

    // Make sure everyone knows it died
    if ( chr[character].alive && !capinvictus[chr[character].model] )
    {
        teammorale[chr[character].baseteam]--;
    }

    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on )
        {
            if ( chr[cnt].ai.target == character )
            {
                chr[cnt].ai.alert |= ALERTIF_TARGETKILLED;
                chr[cnt].ai.target = cnt;
            }
            if ( teamleader[chr[cnt].team] == character )
            {
                chr[cnt].ai.alert |= ALERTIF_LEADERKILLED;
            }
        }

        cnt++;
    }
    if ( teamleader[chr[character].team] == character )
    {
        teamleader[chr[character].team] = NOLEADER;
    }

    chr[character].on = bfalse;
    chr[character].alive = bfalse;
    chr[character].inpack = bfalse;
}

//--------------------------------------------------------------------------------------------
void free_inventory( Uint16 character )
{
    // ZZ> This function frees every item in the character's inventory
    int cnt, next;

    cnt = chr[character].nextinpack;
    while ( cnt < MAXCHR )
    {
        next = chr[cnt].nextinpack;
        free_one_character( cnt );
        cnt = next;
    }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( Uint16 particle, Uint16 character, int grip )
{
    // ZZ> This function sets one particle's position to be attached to a character.
    //     It will kill the particle if the character is no longer around
    Uint16 vertex, model, frame, lastframe;
    Uint8 lip;
    float flip;
    float pointx = 0;
    float pointy = 0;
    float pointz = 0;

    // Check validity of attachment
    if ( !chr[character].on || chr[character].inpack )
    {
        prttime[particle] = 1;
        return;
    }

    // Do we have a matrix???
    if ( chr[character].matrixvalid )// meshinrenderlist[chr[character].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        model = chr[character].model;
        frame = chr[character].frame;
        lastframe = chr[character].lastframe;
        lip = chr[character].lip >> 6;
        flip = lip / 4.0f;

        if ( grip == GRIP_ORIGIN )
        {
            prtxpos[particle] = chr[character].matrix.CNV( 3, 0 );
            prtypos[particle] = chr[character].matrix.CNV( 3, 1 );
            prtzpos[particle] = chr[character].matrix.CNV( 3, 2 );
            return;
        }

        vertex = madvertices[model] - grip;

        // Calculate grip point locations with linear interpolation and other silly things
        pointx = madvrtx[lastframe][vertex] + (madvrtx[frame][vertex] - madvrtx[lastframe][vertex]) * flip;
        pointy = madvrty[lastframe][vertex] + (madvrty[frame][vertex] - madvrty[lastframe][vertex]) * flip;
        pointz = madvrtz[lastframe][vertex] + (madvrtz[frame][vertex] - madvrtz[lastframe][vertex]) * flip;

        // Do the transform
        prtxpos[particle] = ( pointx * chr[character].matrix.CNV( 0, 0 ) +
                              pointy * chr[character].matrix.CNV( 1, 0 ) +
                              pointz * chr[character].matrix.CNV( 2, 0 ) );
        prtypos[particle] = ( pointx * chr[character].matrix.CNV( 0, 1 ) +
                              pointy * chr[character].matrix.CNV( 1, 1 ) +
                              pointz * chr[character].matrix.CNV( 2, 1 ) );
        prtzpos[particle] = ( pointx * chr[character].matrix.CNV( 0, 2 ) +
                              pointy * chr[character].matrix.CNV( 1, 2 ) +
                              pointz * chr[character].matrix.CNV( 2, 2 ) );

        prtxpos[particle] += chr[character].matrix.CNV( 3, 0 );
        prtypos[particle] += chr[character].matrix.CNV( 3, 1 );
        prtzpos[particle] += chr[character].matrix.CNV( 3, 2 );
    }
    else
    {
        // No matrix, so just wing it...
        prtxpos[particle] = chr[character].xpos;
        prtypos[particle] = chr[character].ypos;
        prtzpos[particle] = chr[character].zpos;
    }
}

//--------------------------------------------------------------------------------------------
void make_one_weapon_matrix( Uint16 iweap )
{
    // ZZ> This function sets one weapon's matrix, based on who it's attached to
    int    cnt, tnc, vertex;
    Uint16 ichr, ichr_model, ichr_frame, ichr_lastframe;
    Uint8  ichr_lip;
    float  ichr_flip;
    float  pointx[GRIP_VERTS], pointy[GRIP_VERTS], pointz[GRIP_VERTS];
    float  nupointx[GRIP_VERTS], nupointy[GRIP_VERTS], nupointz[GRIP_VERTS];
    int    iweappoints;

    // make sure that we are attached to a valid character
    ichr = chr[iweap].attachedto;
    if (ichr >= MAXCHR || !chr[ichr].on) return;

    // make sure that the matrix is invalid incase of an error
    chr[iweap].matrixvalid = bfalse;

    // Transform the weapon grip from model space to world space
    ichr_model = chr[ichr].model;
    ichr_frame = chr[ichr].frame;
    ichr_lastframe = chr[ichr].lastframe;
    ichr_lip = chr[ichr].lip >> 6;
    ichr_flip = ichr_lip / 4.0f;

    iweappoints = 0;
    for (cnt = 0; cnt < GRIP_VERTS; cnt++)
    {
        if (0xFFFF != chr[iweap].weapongrip[cnt])
        {
            iweappoints++;
        }
    }
    if (0 == iweappoints)
    {
        // punt! attach to origin
        pointx[0] = chr[0].xpos;
        pointy[0] = chr[0].ypos;
        pointz[0] = chr[0].zpos;
        iweappoints = 1;
    }
    else
    {
        // Calculate grip point locations with linear interpolation and other silly things
        for (cnt = 0; cnt < GRIP_VERTS; cnt++ )
        {
            vertex = chr[iweap].weapongrip[cnt];
            if (0xFFFF == vertex) continue;

            // Calculate grip point locations with linear interpolation and other silly things
            pointx[cnt] = madvrtx[ichr_lastframe][vertex] + (madvrtx[ichr_frame][vertex] - madvrtx[ichr_lastframe][vertex]) * ichr_flip;
            pointy[cnt] = madvrty[ichr_lastframe][vertex] + (madvrty[ichr_frame][vertex] - madvrty[ichr_lastframe][vertex]) * ichr_flip;
            pointz[cnt] = madvrtz[ichr_lastframe][vertex] + (madvrtz[ichr_frame][vertex] - madvrtz[ichr_lastframe][vertex]) * ichr_flip;
        }
    }

    for ( tnc = 0; tnc < iweappoints; tnc++ )
    {
        // Do the transform
        nupointx[tnc] = ( pointx[tnc] * chr[ichr].matrix.CNV( 0, 0 ) +
                          pointy[tnc] * chr[ichr].matrix.CNV( 1, 0 ) +
                          pointz[tnc] * chr[ichr].matrix.CNV( 2, 0 ) );
        nupointy[tnc] = ( pointx[tnc] * chr[ichr].matrix.CNV( 0, 1 ) +
                          pointy[tnc] * chr[ichr].matrix.CNV( 1, 1 ) +
                          pointz[tnc] * chr[ichr].matrix.CNV( 2, 1 ) );
        nupointz[tnc] = ( pointx[tnc] * chr[ichr].matrix.CNV( 0, 2 ) +
                          pointy[tnc] * chr[ichr].matrix.CNV( 1, 2 ) +
                          pointz[tnc] * chr[ichr].matrix.CNV( 2, 2 ) );

        nupointx[tnc] += chr[ichr].matrix.CNV( 3, 0 );
        nupointy[tnc] += chr[ichr].matrix.CNV( 3, 1 );
        nupointz[tnc] += chr[ichr].matrix.CNV( 3, 2 );
    }

    if (1 == iweappoints)
    {
        // attach to single point
        chr[iweap].matrix = ScaleXYZRotateXYZTranslate(chr[iweap].fat, chr[iweap].fat, chr[iweap].fat,
                            chr[iweap].turnleftright >> 2,
                            ( ( Uint16 ) ( chr[iweap].turnmapud + 32768 ) ) >> 2,
                            ( ( Uint16 ) ( chr[iweap].turnmaplr + 32768 ) ) >> 2,
                            nupointx[0], nupointy[0], nupointz[0]);

        chr[iweap].matrixvalid = btrue;
    }
    else if (4 == iweappoints)
    {
        // Calculate weapon's matrix based on positions of grip points
        // chrscale is recomputed at time of attachment
        chr[iweap].matrix = FourPoints( nupointx[0], nupointy[0], nupointz[0],
                                        nupointx[1], nupointy[1], nupointz[1],
                                        nupointx[2], nupointy[2], nupointz[2],
                                        nupointx[3], nupointy[3], nupointz[3],
                                        chr[iweap].fat );
        chr[iweap].matrixvalid = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void make_character_matrices()
{
    // ZZ> This function makes all of the character's matrices
    int cnt, tnc;

    // Forget about old matrices
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        chr[cnt].matrixvalid = bfalse;
        cnt++;
    }

    // Do base characters
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( chr[tnc].attachedto == MAXCHR && chr[tnc].on )  // Skip weapons for now
        {
            make_one_character_matrix( tnc );
        }

        tnc++;
    }

    // Do first level of attachments
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( chr[tnc].attachedto != MAXCHR && chr[tnc].on )
        {
            if ( chr[chr[tnc].attachedto].attachedto == MAXCHR )
            {
                make_one_weapon_matrix( tnc );
            }
        }

        tnc++;
    }

    // Do second level of attachments
    tnc = 0;

    while ( tnc < MAXCHR )
    {
        if ( chr[tnc].attachedto != MAXCHR && chr[tnc].on )
        {
            if ( chr[chr[tnc].attachedto].attachedto != MAXCHR )
            {
                make_one_weapon_matrix( tnc );
            }
        }

        tnc++;
    }
}

//--------------------------------------------------------------------------------------------
int get_free_character()
{
    // ZZ> This function gets an unused character and returns its index
    int character;
    if ( numfreechr == 0 )
    {
        // Return MAXCHR if we can't find one
        return MAXCHR;
    }
    else
    {
        // Just grab the next one
        numfreechr--;
        character = freechrlist[numfreechr];
    }

    return character;
}

//--------------------------------------------------------------------------------------------
/*Uint8 find_target_in_block( int x, int y, float chrx, float chry, Uint16 facing,
                            Uint8 onlyfriends, Uint8 anyone, Uint8 team,
                            Uint16 donttarget, Uint16 oldtarget )
{
  // ZZ> This function helps find a target, returning btrue if it found a decent target
  int cnt;
  Uint16 angle;
  Uint16 charb;
  Uint8 enemies, returncode;
  Uint32 fanblock;
  int distance;

  returncode = bfalse;

  // Current fanblock
  if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
  {
    fanblock = x + meshblockstart[y];

    enemies = bfalse;
    if ( !onlyfriends ) enemies = btrue;

    charb = meshbumplistchr[fanblock];
    cnt = 0;
    while ( cnt < meshbumplistchrnum[fanblock] )
    {
      if ( chr[charb].alive && !chr[charb].invictus && charb != donttarget && charb != oldtarget )
      {
        if ( anyone || ( chr[charb].team == team && onlyfriends ) || ( teamhatesteam[team][chr[charb].team] && enemies ) )
        {
          distance = ABS( chr[charb].xpos - chrx ) + ABS( chr[charb].ypos - chry );
          if ( distance < globestdistance )
          {
            angle = ( ATAN2( chr[charb].ypos - chry, chr[charb].xpos - chrx ) + PI ) * 0xFFFF / ( TWO_PI );
            angle = facing - angle;
            if ( angle < globestangle || angle > ( 0xFFFF - globestangle ) )
            {
              returncode = btrue;
              globesttarget = charb;
              globestdistance = distance;
              glouseangle = angle;
              if ( angle  > 32767 )
                globestangle = -angle;
              else
                globestangle = angle;
            }
          }
        }
      }
      charb = chr[charb].bumpnext;
      cnt++;
    }
  }
  return returncode;
}*/

//--------------------------------------------------------------------------------------------
Uint16 get_particle_target( float xpos, float ypos, float zpos, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget )
{
    //ZF> This is the new improved targeting system for particles. Also includes distance in the Z direction.
    Uint16 besttarget = MAXCHR, cnt;
    Uint16 longdist = WIDE;

    for (cnt = 0; cnt < MAXCHR; cnt++)
    {
        if (chr[cnt].on && chr[cnt].alive && !chr[cnt].isitem && chr[cnt].attachedto == MAXCHR
                && !chr[cnt].invictus)
        {
            if ((piponlydamagefriendly[particletype] && team == chr[cnt].team) || (!piponlydamagefriendly[particletype] && teamhatesteam[team][chr[cnt].team]) )
            {
                //Don't retarget someone we already had or not supposed to target
                if (cnt != oldtarget && cnt != donttarget)
                {
                    Uint16 angle = (ATAN2( chr[cnt].ypos - ypos, chr[cnt].xpos - xpos ) * 0xFFFF / ( TWO_PI ))
                                   + BEHIND - facing;

                    //Only proceed if we are facing the target
                    if (angle < piptargetangle[particletype] || angle > ( 0xFFFF - piptargetangle[particletype] ) )
                    {
                        Uint32 dist = ( Uint32 ) SQRT(ABS( pow(chr[cnt].xpos - xpos, 2))
                                                      + ABS( pow(chr[cnt].ypos - ypos, 2))
                                                      + ABS( pow(chr[cnt].zpos - zpos, 2)) );
                        if (dist < longdist && dist < WIDE )
                        {
                            besttarget = cnt;
                            longdist = dist;
                        }
                    }
                }
            }
        }

    }

    //All done
    return besttarget;
}
//--------------------------------------------------------------------------------------------
/*Uint16 find_target( float chrx, float chry, Uint16 facing,
                    Uint16 targetangle, Uint8 onlyfriends, Uint8 anyone,
                    Uint8 team, Uint16 donttarget, Uint16 oldtarget )
{
  // This function finds the best target for the given parameters
  Uint8 done;
  int x, y;

  x = chrx;
  y = chry;
  x = x >> 9;
  y = y >> 9;
  globestdistance = 9999;
  globestangle = targetangle;
  done = find_target_in_block( x, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x + 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x - 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  if ( done ) return globesttarget;

  done = find_target_in_block( x + 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x + 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x - 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  done |= find_target_in_block( x - 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget );
  if ( done ) return globesttarget;

  return MAXCHR;
}*/

//--------------------------------------------------------------------------------------------
void free_all_characters()
{
    // ZZ> This function resets the character allocation list
    local_noplayers = btrue;
    numfreechr = 0;

    while ( numfreechr < MAXCHR )
    {
        chr[numfreechr].on = bfalse;
        chr[numfreechr].alive = bfalse;
        chr[numfreechr].inpack = bfalse;
        chr[numfreechr].numinpack = 0;
        chr[numfreechr].nextinpack = MAXCHR;
        chr[numfreechr].staton = bfalse;
        chr[numfreechr].matrixvalid = bfalse;
        freechrlist[numfreechr] = numfreechr;
        numfreechr++;
    }

    numpla = 0;
    local_numlpla = 0;
    numstat = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 __chrhitawall( Uint16 character )
{
    // ZZ> This function returns nonzero if the character hit a wall that the
    //     character is not allowed to cross

    Uint8 passtl, passtr, passbr, passbl;
    float x, y, bs;
    float fx, fy;
    Uint32 itile;

    y = chr[character].ypos;  x = chr[character].xpos;  bs = chr[character].bumpsize >> 1;

    fx = x - bs; fy = y - bs;
    passtl = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passtl = meshfx[itile];
    }

    fx = x + bs; fy = y - bs;
    passtr = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passtr = meshfx[itile];
    }

    fx = x - bs; fy = y + bs;
    passbl = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passbl = meshfx[itile];
    }

    fx = x + bs; fy = y + bs;
    passbr = MESHFX_IMPASS;
    itile  = mesh_get_tile( fx, fy );
    if ( INVALID_TILE != itile )
    {
        passbr = meshfx[itile];
    }

    return ( passtl | passtr | passbr | passbl ) & chr[character].stoppedby;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( Uint16 character )
{
    // ZZ> This function fixes a character's max acceleration
    Uint16 enchant;
    if ( character != MAXCHR )
    {
        if ( chr[character].on )
        {
            // Okay, remove all acceleration enchants
            enchant = chr[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                remove_enchant_value( enchant, ADDACCEL );
                enchant = encnextenchant[enchant];
            }

            // Set the starting value
            chr[character].maxaccel = capmaxaccel[chr[character].model][chr[character].skin];
            // Put the acceleration enchants back on
            enchant = chr[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                add_enchant_value( enchant, ADDACCEL, enceve[enchant] );
                enchant = encnextenchant[enchant];
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void detach_character_from_mount( Uint16 character, Uint8 ignorekurse,
                                  Uint8 doshop )
{
    // ZZ> This function drops an item
    Uint16 mount, hand, enchant, cnt, passage, owner = NOOWNER;
    bool_t inshop;
    int loc;
    float price;

    // Make sure the character is valid
    if ( character == MAXCHR )
        return;

    // Make sure the character is mounted
    mount = chr[character].attachedto;
    if ( mount >= MAXCHR )
        return;

    // Make sure both are still around
    if ( !chr[character].on || !chr[mount].on )
        return;

    // Don't allow living characters to drop kursed weapons
    if ( !ignorekurse && chr[character].iskursed && chr[mount].alive && chr[character].isitem )
    {
        chr[character].ai.alert |= ALERTIF_NOTDROPPED;
        return;
    }

    // Figure out which hand it's in
    hand = 0;
    if ( chr[character].inwhichhand == GRIP_RIGHT )
    {
        hand = 1;
    }

    // Rip 'em apart
    chr[character].attachedto = MAXCHR;
    if ( chr[mount].holdingwhich[0] == character )
        chr[mount].holdingwhich[0] = MAXCHR;
    if ( chr[mount].holdingwhich[1] == character )
        chr[mount].holdingwhich[1] = MAXCHR;

    // Run the falling animation...
    play_action( character, ACTIONJB + hand, bfalse );

    // Set the positions
    if ( chr[character].matrixvalid )
    {
        chr[character].xpos = chr[character].matrix.CNV( 3, 0 );
        chr[character].ypos = chr[character].matrix.CNV( 3, 1 );
        chr[character].zpos = chr[character].matrix.CNV( 3, 2 );
    }
    else
    {
        chr[character].xpos = chr[mount].xpos;
        chr[character].ypos = chr[mount].ypos;
        chr[character].zpos = chr[mount].zpos;
    }

    // Make sure it's not dropped in a wall...
    if ( __chrhitawall( character ) )
    {
        chr[character].xpos = chr[mount].xpos;
        chr[character].ypos = chr[mount].ypos;
    }

    // Check for shop passages
    inshop = bfalse;
    if ( chr[character].isitem && numshoppassage != 0 && doshop )
    {
        //This is a hack that makes spellbooks in shops cost correctly
        if (chr[mount].isshopitem) chr[character].isshopitem = btrue;

        cnt = 0;

        while ( cnt < numshoppassage )
        {
            passage = shoppassage[cnt];
            loc = chr[character].xpos;
            loc = loc >> 7;
            if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
            {
                loc = chr[character].ypos;
                loc = loc >> 7;
                if ( loc >= passtly[passage] && loc <= passbry[passage] )
                {
                    inshop = btrue;
                    owner = shopowner[cnt];
                    cnt = numshoppassage;  // Finish loop
                    if ( owner == NOOWNER )
                    {
                        // The owner has died!!!
                        inshop = bfalse;
                    }
                }
            }

            cnt++;
        }
        if ( inshop )
        {
            // Give the mount its money back, alert the shop owner
            price = (float) capskincost[chr[character].model][0];
            if ( capisstackable[chr[character].model] )
            {
                price = price * chr[character].ammo;
            }

            // Reduce value depending on charges left
            else if (capisranged[chr[character].model] && chr[character].ammo < chr[character].ammomax)
            {
                if (chr[character].ammo == 0)
                {
                    price /= 2;
                }
                else price -= ((chr[character].ammomax - chr[character].ammo) * ((float)(price / chr[character].ammomax))) / 2;
            }

            //Items spawned within shops are more valuable
            if (!chr[character].isshopitem) price *= 0.5;

            chr[mount].money += (Sint16) price;
            chr[owner].money -= (Sint16) price;
            if ( chr[owner].money < 0 )  chr[owner].money = 0;
            if ( chr[mount].money > MAXMONEY )  chr[mount].money = MAXMONEY;

            chr[owner].ai.alert |= ALERTIF_ORDERED;
            chr[owner].ai.order = (Uint32) price;  // Tell owner how much...
            chr[owner].ai.rank  = BUY;  // 0 for buying an item
        }
    }

    // Make sure it works right
    chr[character].hitready = btrue;
    if ( inshop )
    {
        // Drop straight down to avoid theft
        chr[character].xvel = 0;
        chr[character].yvel = 0;
    }
    else
    {
        chr[character].xvel = chr[mount].xvel;
        chr[character].yvel = chr[mount].yvel;
    }

    chr[character].zvel = DROPZVEL;

    // Turn looping off
    chr[character].loopaction = bfalse;

    // Reset the team if it is a mount
    if ( chr[mount].ismount )
    {
        chr[mount].team = chr[mount].baseteam;
        chr[mount].ai.alert |= ALERTIF_DROPPED;
    }

    chr[character].team = chr[character].baseteam;
    chr[character].ai.alert |= ALERTIF_DROPPED;

    // Reset transparency
    if ( chr[character].isitem && chr[mount].transferblend )
    {
        // Okay, reset transparency
        enchant = chr[character].firstenchant;

        while ( enchant < MAXENCHANT )
        {
            unset_enchant_value( enchant, SETALPHABLEND );
            unset_enchant_value( enchant, SETLIGHTBLEND );
            enchant = encnextenchant[enchant];
        }

        chr[character].alpha = chr[character].basealpha;
        chr[character].light = caplight[chr[character].model];
        enchant = chr[character].firstenchant;

        while ( enchant < MAXENCHANT )
        {
            set_enchant_value( enchant, SETALPHABLEND, enceve[enchant] );
            set_enchant_value( enchant, SETLIGHTBLEND, enceve[enchant] );
            enchant = encnextenchant[enchant];
        }
    }

    // Set twist
    chr[character].turnmaplr = 32768;
    chr[character].turnmapud = 32768;
}
//--------------------------------------------------------------------------------------------
void reset_character_alpha( Uint16 character )
{
    // ZZ> This function fixes an item's transparency
    Uint16 enchant, mount;
    if ( character != MAXCHR )
    {
        mount = chr[character].attachedto;
        if ( chr[character].on && mount != MAXCHR && chr[character].isitem && chr[mount].transferblend )
        {
            // Okay, reset transparency
            enchant = chr[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                unset_enchant_value( enchant, SETALPHABLEND );
                unset_enchant_value( enchant, SETLIGHTBLEND );
                enchant = encnextenchant[enchant];
            }

            chr[character].alpha = chr[character].basealpha;
            chr[character].light = caplight[chr[character].model];
            enchant = chr[character].firstenchant;

            while ( enchant < MAXENCHANT )
            {
                set_enchant_value( enchant, SETALPHABLEND, enceve[enchant] );
                set_enchant_value( enchant, SETLIGHTBLEND, enceve[enchant] );
                enchant = encnextenchant[enchant];
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount( Uint16 character, Uint16 mount, Uint16 grip )
{
    // ZZ> This function attaches one character to another ( the mount )
    //     at either the left or right grip
    int i, tnc, slot;

    // Make sure both are still around
    if ( !chr[character].on || !chr[mount].on || chr[character].inpack || chr[mount].inpack )
        return;

    // Figure out which slot this grip relates to
    if ( grip == GRIP_LEFT )
    {
        slot = SLOT_LEFT;
    }
    else
    {
        slot = SLOT_RIGHT;
    }

    // Make sure the the slot is valid
    if ( !capgripvalid[chr[mount].model][slot] )
        return;

    // Put 'em together
    chr[character].inwhichhand    = grip;
    chr[character].attachedto     = mount;
    chr[mount].holdingwhich[slot] = character;

    tnc = madvertices[chr[mount].model] - grip;

    for (i = 0; i < GRIP_VERTS; i++)
    {
        if (tnc + i < madvertices[chr[mount].model] )
        {
            chr[character].weapongrip[i] = i + tnc;
        }
        else
        {
            chr[character].weapongrip[i] = 0xFFFF;
        }
    }

    // catually make position of the object coincide with its actual held position
    make_one_weapon_matrix( character );

    chr[character].xpos = chr[character].matrix.CNV( 3, 0 );
    chr[character].ypos = chr[character].matrix.CNV( 3, 1 );
    chr[character].zpos = chr[character].matrix.CNV( 3, 2 );

    chr[character].inwater = bfalse;
    chr[character].jumptime = JUMPDELAY * 4;

    // Run the held animation
    if ( chr[mount].ismount && grip == GRIP_ONLY )
    {
        // Riding mount
        play_action( character, ACTIONMI, btrue );
        chr[character].loopaction = btrue;
    }
    else
    {
        play_action( character, ACTIONMM + slot, bfalse );
        if ( chr[character].isitem )
        {
            // Item grab
            chr[character].keepaction = btrue;
        }
    }

    // Set the team
    if ( chr[character].isitem )
    {
        chr[character].team = chr[mount].team;
        // Set the alert
        chr[character].ai.alert |= ALERTIF_GRABBED;
    }
    if ( chr[mount].ismount )
    {
        chr[mount].team = chr[character].team;

        // Set the alert
        if ( !chr[mount].isitem )
        {
            chr[mount].ai.alert |= ALERTIF_GRABBED;
        }
    }

    // It's not gonna hit the floor
    chr[character].hitready = bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 stack_in_pack( Uint16 item, Uint16 character )
{
    // ZZ> This function looks in the character's pack for an item similar
    //     to the one given.  If it finds one, it returns the similar item's
    //     index number, otherwise it returns MAXCHR.
    Uint16 inpack, id;
    bool_t allok;
    if ( capisstackable[chr[item].model] )
    {
        inpack = chr[character].nextinpack;
        allok = bfalse;

        while ( inpack != MAXCHR && !allok )
        {
            allok = btrue;
            if ( chr[inpack].model != chr[item].model )
            {
                if ( !capisstackable[chr[inpack].model] )
                {
                    allok = bfalse;
                }
                if ( chr[inpack].ammomax != chr[item].ammomax )
                {
                    allok = bfalse;
                }

                id = 0;

                while ( id < IDSZ_COUNT && allok )
                {
                    if ( capidsz[chr[inpack].model][id] != capidsz[chr[item].model][id] )
                    {
                        allok = bfalse;
                    }

                    id++;
                }
            }
            if ( !allok )
            {
                inpack = chr[inpack].nextinpack;
            }
        }
        if ( allok )
        {
            return inpack;
        }
    }

    return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void add_item_to_character_pack( Uint16 item, Uint16 character )
{
    // ZZ> This function puts one character inside the other's pack
    Uint16 oldfirstitem, newammo, stack;

    // Make sure everything is hunkydori
    if ( ( !chr[item].on ) || ( !chr[character].on ) || chr[item].inpack || chr[character].inpack ||
            chr[character].isitem )
        return;

    stack = stack_in_pack( item, character );
    if ( stack != MAXCHR )
    {
        // We found a similar, stackable item in the pack
        if ( chr[item].nameknown || chr[stack].nameknown )
        {
            chr[item].nameknown = btrue;
            chr[stack].nameknown = btrue;
        }
        if ( capusageknown[chr[item].model] || capusageknown[chr[stack].model] )
        {
            capusageknown[chr[item].model] = btrue;
            capusageknown[chr[stack].model] = btrue;
        }

        newammo = chr[item].ammo + chr[stack].ammo;
        if ( newammo <= chr[stack].ammomax )
        {
            // All transfered, so kill the in hand item
            chr[stack].ammo = newammo;
            if ( chr[item].attachedto != MAXCHR )
            {
                detach_character_from_mount( item, btrue, bfalse );
            }

            free_one_character( item );
        }
        else
        {
            // Only some were transfered,
            chr[item].ammo = chr[item].ammo + chr[stack].ammo - chr[stack].ammomax;
            chr[stack].ammo = chr[stack].ammomax;
            chr[character].ai.alert |= ALERTIF_TOOMUCHBAGGAGE;
        }
    }
    else
    {
        // Make sure we have room for another item
        if ( chr[character].numinpack >= MAXNUMINPACK )
        {
            chr[character].ai.alert |= ALERTIF_TOOMUCHBAGGAGE;
            return;
        }

        // Take the item out of hand
        if ( chr[item].attachedto != MAXCHR )
        {
            detach_character_from_mount( item, btrue, bfalse );
            chr[item].ai.alert &= ( ~ALERTIF_DROPPED );
        }

        // Remove the item from play
        chr[item].hitready = bfalse;
        chr[item].inpack = btrue;

        // Insert the item into the pack as the first one
        oldfirstitem = chr[character].nextinpack;
        chr[character].nextinpack = item;
        chr[item].nextinpack = oldfirstitem;
        chr[character].numinpack++;
        if ( capisequipment[chr[item].model] )
        {
            // AtLastWaypoint doubles as PutAway
            chr[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;
        }
    }

    return;
}

//--------------------------------------------------------------------------------------------
Uint16 get_item_from_character_pack( Uint16 character, Uint16 grip, Uint8 ignorekurse )
{
    // ZZ> This function takes the last item in the character's pack and puts
    //     it into the designated hand.  It returns the item number or MAXCHR.
    Uint16 item, nexttolastitem;

    // Make sure everything is hunkydori
    if ( ( !chr[character].on ) || chr[character].inpack || chr[character].isitem || chr[character].nextinpack == MAXCHR )
        return MAXCHR;
    if ( chr[character].numinpack == 0 )
        return MAXCHR;

    // Find the last item in the pack
    nexttolastitem = character;
    item = chr[character].nextinpack;

    while ( chr[item].nextinpack != MAXCHR )
    {
        nexttolastitem = item;
        item = chr[item].nextinpack;
    }

    // Figure out what to do with it
    if ( chr[item].iskursed && chr[item].isequipped && !ignorekurse )
    {
        // Flag the last item as not removed
        chr[item].ai.alert |= ALERTIF_NOTPUTAWAY;  // Doubles as IfNotTakenOut
        // Cycle it to the front
        chr[item].nextinpack = chr[character].nextinpack;
        chr[nexttolastitem].nextinpack = MAXCHR;
        chr[character].nextinpack = item;
        if ( character == nexttolastitem )
        {
            chr[item].nextinpack = MAXCHR;
        }

        return MAXCHR;
    }
    else
    {
        // Remove the last item from the pack
        chr[item].inpack = bfalse;
        chr[item].isequipped = bfalse;
        chr[nexttolastitem].nextinpack = MAXCHR;
        chr[character].numinpack--;
        chr[item].team = chr[character].team;

        // Attach the item to the character's hand
        attach_character_to_mount( item, character, grip );
        chr[item].ai.alert &= ( ~ALERTIF_GRABBED );
        chr[item].ai.alert |= ( ALERTIF_TAKENOUT );
    }

    return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( Uint16 character )
{
    // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
    //     inventory ( Not hands ).
    Uint16 item, lastitem, nextitem, direction, cosdir;
    IDSZ testa, testz;
    if ( character < MAXCHR )
    {
        if ( chr[character].on )
        {
            if ( chr[character].zpos > -2 ) // Don't lose keys in pits...
            {
                // The IDSZs to find
                testa = Make_IDSZ( "KEYA" );  // [KEYA]
                testz = Make_IDSZ( "KEYZ" );  // [KEYZ]

                lastitem = character;
                item = chr[character].nextinpack;

                while ( item != MAXCHR )
                {
                    nextitem = chr[item].nextinpack;
                    if ( item != character )  // Should never happen...
                    {
                        if ( ( capidsz[chr[item].model][IDSZ_PARENT] >= testa &&
                                capidsz[chr[item].model][IDSZ_PARENT] <= testz ) ||
                                ( capidsz[chr[item].model][IDSZ_TYPE] >= testa &&
                                  capidsz[chr[item].model][IDSZ_TYPE] <= testz ) )
                        {
                            // We found a key...
                            chr[item].inpack = bfalse;
                            chr[item].isequipped = bfalse;
                            chr[lastitem].nextinpack = nextitem;
                            chr[item].nextinpack = MAXCHR;
                            chr[character].numinpack--;
                            chr[item].attachedto = MAXCHR;
                            chr[item].ai.alert |= ALERTIF_DROPPED;
                            chr[item].hitready = btrue;

                            direction = RANDIE;
                            chr[item].turnleftright = direction + 32768;
                            cosdir = direction + 16384;
                            chr[item].level = chr[character].level;
                            chr[item].xpos = chr[character].xpos;
                            chr[item].ypos = chr[character].ypos;
                            chr[item].zpos = chr[character].zpos;
                            chr[item].xvel = turntocos[direction>>2] * DROPXYVEL;
                            chr[item].yvel = turntosin[direction>>2] * DROPXYVEL;
                            chr[item].zvel = DROPZVEL;
                            chr[item].team = chr[item].baseteam;
                        }
                        else
                        {
                            lastitem = item;
                        }
                    }

                    item = nextitem;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void drop_all_items( Uint16 character )
{
    // ZZ> This function drops all of a character's items
    Uint16 item, direction, diradd;
    if ( character < MAXCHR )
    {
        if ( chr[character].on )
        {
            detach_character_from_mount( chr[character].holdingwhich[0], btrue, bfalse );
            detach_character_from_mount( chr[character].holdingwhich[1], btrue, bfalse );
            if ( chr[character].numinpack > 0 )
            {
                direction = chr[character].turnleftright + 32768;
                diradd = 0xFFFF / chr[character].numinpack;

                while ( chr[character].numinpack > 0 )
                {
                    item = get_item_from_character_pack( character, GRIP_LEFT, bfalse );
                    if ( item < MAXCHR )
                    {
                        detach_character_from_mount( item, btrue, btrue );
                        chr[item].hitready = btrue;
                        chr[item].ai.alert |= ALERTIF_DROPPED;
                        chr[item].xpos = chr[character].xpos;
                        chr[item].ypos = chr[character].ypos;
                        chr[item].zpos = chr[character].zpos;
                        chr[item].level = chr[character].level;
                        chr[item].turnleftright = direction + 32768;
                        chr[item].xvel = turntocos[direction>>2] * DROPXYVEL;
                        chr[item].yvel = turntosin[direction>>2] * DROPXYVEL;
                        chr[item].zvel = DROPZVEL;
                        chr[item].team = chr[item].baseteam;
                    }

                    direction += diradd;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void character_grab_stuff( Uint16 chara, int grip, Uint8 people )
{
    // ZZ> This function makes the character pick up an item if there's one around
    float xa, ya, za, xb, yb, zb, dist;
    int charb, slot;
    Uint16 vertex, model, frame, passage, cnt, owner = NOOWNER;
    float pointx, pointy, pointz;
    bool_t inshop;
    int loc;
    float price;

    // Make life easier
    model = chr[chara].model;
    slot = ( grip / GRIP_VERTS ) - 1;  // 0 is left, 1 is right

    // Make sure the character doesn't have something already, and that it has hands
    if ( chr[chara].holdingwhich[slot] != MAXCHR || !capgripvalid[model][slot] )
        return;

    // Do we have a matrix???
    if ( chr[chara].matrixvalid )// meshinrenderlist[chr[chara].onwhichfan])
    {
        // Transform the weapon grip from model to world space
        frame = chr[chara].frame;
        vertex = madvertices[model] - grip;

        // Calculate grip point locations
        pointx = madvrtx[frame][vertex];/// chr[cnt].scale;
        pointy = madvrty[frame][vertex];/// chr[cnt].scale;
        pointz = madvrtz[frame][vertex];/// chr[cnt].scale;

        // Do the transform
        xa = ( pointx * chr[chara].matrix.CNV( 0, 0 ) +
               pointy * chr[chara].matrix.CNV( 1, 0 ) +
               pointz * chr[chara].matrix.CNV( 2, 0 ) );
        ya = ( pointx * chr[chara].matrix.CNV( 0, 1 ) +
               pointy * chr[chara].matrix.CNV( 1, 1 ) +
               pointz * chr[chara].matrix.CNV( 2, 1 ) );
        za = ( pointx * chr[chara].matrix.CNV( 0, 2 ) +
               pointy * chr[chara].matrix.CNV( 1, 2 ) +
               pointz * chr[chara].matrix.CNV( 2, 2 ) );
        xa += chr[chara].matrix.CNV( 3, 0 );
        ya += chr[chara].matrix.CNV( 3, 1 );
        za += chr[chara].matrix.CNV( 3, 2 );
    }
    else
    {
        // Just wing it
        xa = chr[chara].xpos;
        ya = chr[chara].ypos;
        za = chr[chara].zpos;
    }

    // Go through all characters to find the best match
    charb = 0;

    while ( charb < MAXCHR )
    {
        if ( chr[charb].on && ( !chr[charb].inpack ) && chr[charb].weight < chr[chara].weight && chr[charb].alive && chr[charb].attachedto == MAXCHR && ( ( !people && chr[charb].isitem ) || ( people && !chr[charb].isitem ) ) )
        {
            xb = chr[charb].xpos;
            yb = chr[charb].ypos;
            zb = chr[charb].zpos;
            // First check absolute value diamond
            xb = ABS( xa - xb );
            yb = ABS( ya - yb );
            zb = ABS( za - zb );
            dist = xb + yb;
            if ( dist < GRABSIZE && zb < GRABSIZE )
            {
                // Don't grab your mount
                if ( chr[charb].holdingwhich[0] != chara && chr[charb].holdingwhich[1] != chara )
                {
                    // Check for shop
                    inshop = bfalse;
                    if ( chr[charb].isitem && numshoppassage != 0 )
                    {
                        cnt = 0;

                        while ( cnt < numshoppassage )
                        {
                            passage = shoppassage[cnt];
                            loc = chr[charb].xpos;
                            loc = loc >> 7;
                            if ( loc >= passtlx[passage] && loc <= passbrx[passage] )
                            {
                                loc = chr[charb].ypos;
                                loc = loc >> 7;
                                if ( loc >= passtly[passage] && loc <= passbry[passage] )
                                {
                                    inshop = btrue;
                                    owner = shopowner[cnt];
                                    cnt = numshoppassage;  // Finish loop
                                    if ( owner == NOOWNER )
                                    {
                                        // The owner has died!!!
                                        inshop = bfalse;
                                    }
                                }
                            }

                            cnt++;
                        }
                        if ( inshop )
                        {
                            // Pay the shop owner, or don't allow grab...
                            if ( chr[chara].isitem || ( chr[chara].alpha < INVISIBLE) )
                            {
                                // Pets can try to steal in addition to invisible characters
                                STRING text;
                                inshop = bfalse;
                                snprintf( text, sizeof(text), "%s stole something! (%s)", chr[chara].name, capclassname[chr[charb].model] );
                                debug_message( text );

                                // Check if it was detected. 50% chance +2% per pet DEX and -2% per shopkeeper wisdom
                                if (chr[owner].canseeinvisible || generate_number( 1, 100 ) - ( chr[chara].dexterity >> 7 ) + ( chr[owner].wisdom >> 7 ) > 50 )
                                {
                                    snprintf( text, sizeof(text), "%s was detected!!", chr[chara].name );
                                    debug_message( text );
                                    chr[owner].ai.alert |= ALERTIF_ORDERED;
                                    chr[owner].ai.order = STOLEN;
                                    chr[owner].ai.rank  = 3;
                                }
                            }
                            else
                            {
                                chr[owner].ai.alert |= ALERTIF_ORDERED;
                                price = (float) capskincost[chr[charb].model][0];
                                if ( capisstackable[chr[charb].model] )
                                {
                                    price = price * chr[charb].ammo;
                                }

                                // Reduce value depending on charges left
                                else if (capisranged[chr[charb].model] && chr[charb].ammo < chr[charb].ammomax)
                                {
                                    if (chr[charb].ammo == 0) price /= 2;
                                    else price -= ((chr[charb].ammomax - chr[charb].ammo) * ((float)(price / chr[charb].ammomax))) / 2;
                                }

                                //Items spawned in shops are more valuable
                                if (!chr[charb].isshopitem) price *= 0.5;

                                chr[owner].ai.order = (Uint32) price;  // Tell owner how much...
                                if ( chr[chara].money >= price )
                                {
                                    // Okay to buy
                                    chr[owner].ai.rank = SELL;  // 1 for selling an item
                                    chr[chara].money  -= (Sint16) price;  // Skin 0 cost is price
                                    chr[owner].money  += (Sint16) price;
                                    if ( chr[owner].money > MAXMONEY )  chr[owner].money = MAXMONEY;

                                    inshop = bfalse;
                                }
                                else
                                {
                                    // Don't allow purchase
                                    chr[owner].ai.rank = 2;  // 2 for "you can't afford that"
                                    inshop = btrue;
                                }
                            }
                        }
                    }
                    if ( !inshop )
                    {
                        // Stick 'em together and quit
                        attach_character_to_mount( charb, chara, grip );
                        charb = MAXCHR;
                        if ( people )
                        {
                            // Do a slam animation...  ( Be sure to drop!!! )
                            play_action( chara, ACTIONMC + slot, bfalse );
                        }
                    }
                    else
                    {
                        // Lift the item a little and quit...
                        chr[charb].zvel = DROPZVEL;
                        chr[charb].hitready = btrue;
                        chr[charb].ai.alert |= ALERTIF_DROPPED;
                        charb = MAXCHR;
                    }
                }
            }
        }

        charb++;
    }
}

//--------------------------------------------------------------------------------------------
void character_swipe( Uint16 cnt, Uint8 slot )
{
    // ZZ> This function spawns an attack particle
    int weapon, particle, spawngrip, thrown;
    Uint8 action;
    Uint16 tTmp;
    float dampen;
    float x, y, z, velocity;

    weapon = chr[cnt].holdingwhich[slot];
    spawngrip = GRIP_LAST;
    action = chr[cnt].action;

    // See if it's an unarmed attack...
    if ( weapon == MAXCHR )
    {
        weapon = cnt;
        spawngrip = (slot + 1) * GRIP_VERTS;  // 0 -> GRIP_LEFT, 1 -> GRIP_RIGHT
    }
    if ( weapon != cnt && ( ( capisstackable[chr[weapon].model] && chr[weapon].ammo > 1 ) || ( action >= ACTIONFA && action <= ACTIONFD ) ) )
    {
        // Throw the weapon if it's stacked or a hurl animation
        x = chr[cnt].xpos;
        y = chr[cnt].ypos;
        z = chr[cnt].zpos;
        thrown = spawn_one_character( x, y, z, chr[weapon].model, chr[cnt].team, 0, chr[cnt].turnleftright, chr[weapon].name, MAXCHR );
        if ( thrown < MAXCHR )
        {
            chr[thrown].iskursed = bfalse;
            chr[thrown].ammo = 1;
            chr[thrown].ai.alert |= ALERTIF_THROWN;
            velocity = chr[cnt].strength / ( chr[thrown].weight * THROWFIX );
            velocity += MINTHROWVELOCITY;
            if ( velocity > MAXTHROWVELOCITY )
            {
                velocity = MAXTHROWVELOCITY;
            }

            tTmp = chr[cnt].turnleftright >> 2;
            chr[thrown].xvel += turntocos[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            chr[thrown].yvel += turntosin[( tTmp+8192 )&TRIG_TABLE_MASK] * velocity;
            chr[thrown].zvel = DROPZVEL;
            if ( chr[weapon].ammo <= 1 )
            {
                // Poof the item
                detach_character_from_mount( weapon, btrue, bfalse );
                free_one_character( weapon );
            }
            else
            {
                chr[weapon].ammo--;
            }
        }
    }
    else
    {
        // Spawn an attack particle
        if ( chr[weapon].ammomax == 0 || chr[weapon].ammo != 0 )
        {
            if ( chr[weapon].ammo > 0 && !capisstackable[chr[weapon].model] )
            {
                chr[weapon].ammo--;  // Ammo usage
            }

            // HERE
            if ( capattackprttype[chr[weapon].model] != -1 )
            {
                particle = spawn_one_particle( chr[weapon].xpos, chr[weapon].ypos, chr[weapon].zpos, chr[cnt].turnleftright, chr[weapon].model, capattackprttype[chr[weapon].model], weapon, spawngrip, chr[cnt].team, cnt, 0, MAXCHR );
                if ( particle != TOTALMAXPRT )
                {
                    if ( !capattackattached[chr[weapon].model] )
                    {
                        // Detach the particle
                        if ( !pipstartontarget[prtpip[particle]] || prttarget[particle] == MAXCHR )
                        {
                            attach_particle_to_character( particle, weapon, spawngrip );
                            // Correct Z spacing base, but nothing else...
                            prtzpos[particle] += pipzspacingbase[prtpip[particle]];
                        }

                        prtattachedtocharacter[particle] = MAXCHR;

                        // Don't spawn in walls
                        if ( __prthitawall( particle ) )
                        {
                            prtxpos[particle] = chr[weapon].xpos;
                            prtypos[particle] = chr[weapon].ypos;
                            if ( __prthitawall( particle ) )
                            {
                                prtxpos[particle] = chr[cnt].xpos;
                                prtypos[particle] = chr[cnt].ypos;
                            }
                        }
                    }
                    else
                    {
                        // Attached particles get a strength bonus for reeling...
                        dampen = REELBASE + ( chr[cnt].strength / REEL );
                        prtxvel[particle] = prtxvel[particle] * dampen;
                        prtyvel[particle] = prtyvel[particle] * dampen;
                        prtzvel[particle] = prtzvel[particle] * dampen;
                    }

                    // Initial particles get a strength bonus, which may be 0.00f
                    prtdamagebase[particle] += ( chr[cnt].strength * capstrengthdampen[chr[weapon].model] );
                    // Initial particles get an enchantment bonus
                    prtdamagebase[particle] += chr[weapon].damageboost;
                    // Initial particles inherit damage type of weapon
                    prtdamagetype[particle] = chr[weapon].damagetargettype;
                }
            }
        }
        else
        {
            chr[weapon].ammoknown = btrue;
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_characters( void )
{
    // ZZ> This function handles character physics
    Uint16 cnt;
    Uint32 mapud, maplr;
    Uint8 twist, actionready;
    Uint8 speed, framelip, allowedtoattack;
    float level, friction;
    float dvx, dvy, dvmax;
    Uint16 action, weapon, mount, item;
    int distance, volume;
    bool_t watchtarget, grounded;

    // Move every character
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !chr[cnt].on || chr[cnt].inpack ) continue;

        grounded = bfalse;

        // Down that ol' damage timer
        chr[cnt].damagetime -= ( chr[cnt].damagetime != 0 );

        // Character's old location
        chr[cnt].oldx = chr[cnt].xpos;
        chr[cnt].oldy = chr[cnt].ypos;
        chr[cnt].oldz = chr[cnt].zpos;
        chr[cnt].oldturn = chr[cnt].turnleftright;
        //            if(chr[cnt].attachedto!=MAXCHR)
        //            {
        //                chr[cnt].turnleftright = chr[chr[cnt].attachedto].turnleftright;
        //                if(chr[cnt].indolist==bfalse)
        //                {
        //                    chr[cnt].xpos = chr[chr[cnt].attachedto].xpos;
        //                    chr[cnt].ypos = chr[chr[cnt].attachedto].ypos;
        //                    chr[cnt].zpos = chr[chr[cnt].attachedto].zpos;
        //                }
        //            }

        // Texture movement
        chr[cnt].uoffset += chr[cnt].uoffvel;
        chr[cnt].voffset += chr[cnt].voffvel;
        if ( chr[cnt].alive )
        {
            if ( chr[cnt].attachedto == MAXCHR )
            {
                // Character latches for generalized movement
                dvx = chr[cnt].latchx;
                dvy = chr[cnt].latchy;

                // Reverse movements for daze
                if ( chr[cnt].dazetime > 0 )
                {
                    dvx = -dvx;
                    dvy = -dvy;
                }

                // Switch x and y for daze
                if ( chr[cnt].grogtime > 0 )
                {
                    dvmax = dvx;
                    dvx = dvy;
                    dvy = dvmax;
                }

                // Get direction from the DESIRED change in velocity
                if ( chr[cnt].turnmode == TURNMODEWATCH )
                {
                    if ( ( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
                    {
                        chr[cnt].turnleftright = terp_dir( chr[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                    }
                }

                // Face the target
                watchtarget = ( chr[cnt].turnmode == TURNMODEWATCHTARGET );
                if ( watchtarget )
                {
                    if ( cnt != chr[cnt].ai.target )
                        chr[cnt].turnleftright = terp_dir( chr[cnt].turnleftright, ( ATAN2( chr[chr[cnt].ai.target].ypos - chr[cnt].ypos, chr[chr[cnt].ai.target].xpos - chr[cnt].xpos ) + PI ) * 0xFFFF / ( TWO_PI ) );
                }
                if ( madframefx[chr[cnt].frame]&MADFXSTOP )
                {
                    dvx = 0;
                    dvy = 0;
                }
                else
                {
                    // Limit to max acceleration
                    dvmax = chr[cnt].maxaccel;
                    if ( dvx < -dvmax ) dvx = -dvmax;
                    if ( dvx > dvmax ) dvx = dvmax;
                    if ( dvy < -dvmax ) dvy = -dvmax;
                    if ( dvy > dvmax ) dvy = dvmax;

                    chr[cnt].xvel += dvx;
                    chr[cnt].yvel += dvy;
                }

                // Get direction from ACTUAL change in velocity
                if ( chr[cnt].turnmode == TURNMODEVELOCITY )
                {
                    if ( dvx < -TURNSPD || dvx > TURNSPD || dvy < -TURNSPD || dvy > TURNSPD )
                    {
                        if ( chr[cnt].isplayer )
                        {
                            // Players turn quickly
                            chr[cnt].turnleftright = terp_dir_fast( chr[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                        else
                        {
                            // AI turn slowly
                            chr[cnt].turnleftright = terp_dir( chr[cnt].turnleftright, ( ATAN2( dvy, dvx ) + PI ) * 0xFFFF / ( TWO_PI ) );
                        }
                    }
                }

                // Otherwise make it spin
                else if ( chr[cnt].turnmode == TURNMODESPIN )
                {
                    chr[cnt].turnleftright += SPINRATE;
                }
            }

            // Character latches for generalized buttons
            if ( chr[cnt].latchbutton != 0 )
            {
                if ( 0 != (chr[cnt].latchbutton & LATCHBUTTON_JUMP) )
                {
                    if ( chr[cnt].attachedto != MAXCHR && chr[cnt].jumptime == 0 )
                    {
                        int ijump;

                        detach_character_from_mount( cnt, btrue, btrue );
                        chr[cnt].jumptime = JUMPDELAY;
                        chr[cnt].zvel = DISMOUNTZVEL;
                        if ( chr[cnt].flyheight != 0 )
                            chr[cnt].zvel = DISMOUNTZVELFLY;

                        chr[cnt].zpos += chr[cnt].zvel;
                        if ( chr[cnt].jumpnumberreset != JUMPINFINITE && chr[cnt].jumpnumber != 0 )
                            chr[cnt].jumpnumber--;

                        // Play the jump sound
                        ijump = capsoundindex[chr[cnt].model][SOUND_JUMP];
                        if ( ijump >= 0 && ijump < MAXWAVE )
                        {
                            sound_play_chunk( chr[cnt].xpos, chr[cnt].ypos, capwavelist[chr[cnt].model][ijump] );
                        }

                    }
                    if ( chr[cnt].jumptime == 0 && chr[cnt].jumpnumber != 0 && chr[cnt].flyheight == 0 )
                    {
                        if ( chr[cnt].jumpnumberreset != 1 || chr[cnt].jumpready )
                        {
                            // Make the character jump
                            chr[cnt].hitready = btrue;
                            if ( chr[cnt].inwater )
                            {
                                chr[cnt].zvel = WATERJUMP;
                            }
                            else
                            {
                                chr[cnt].zvel = chr[cnt].jump;
                            }

                            chr[cnt].jumptime = JUMPDELAY;
                            chr[cnt].jumpready = bfalse;
                            if ( chr[cnt].jumpnumberreset != JUMPINFINITE ) chr[cnt].jumpnumber--;

                            // Set to jump animation if not doing anything better
                            if ( chr[cnt].actionready )    play_action( cnt, ACTIONJA, btrue );

                            // Play the jump sound (Boing!)
                            distance = ABS( camtrackx - chr[cnt].xpos ) + ABS( camtracky - chr[cnt].ypos );
                            volume = -distance;
                            if ( volume > VOLMIN )
                            {
                                int ijump = capsoundindex[chr[cnt].model][SOUND_JUMP];
                                if ( ijump >= 0 && ijump < MAXWAVE )
                                {
                                    sound_play_chunk( chr[cnt].xpos, chr[cnt].ypos, capwavelist[chr[cnt].model][ijump] );
                                }
                            }

                        }
                    }
                }
                if ( 0 != ( chr[cnt].latchbutton & LATCHBUTTON_ALTLEFT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                {
                    chr[cnt].reloadtime = GRABDELAY;
                    if ( chr[cnt].holdingwhich[0] == MAXCHR )
                    {
                        // Grab left
                        play_action( cnt, ACTIONME, bfalse );
                    }
                    else
                    {
                        // Drop left
                        play_action( cnt, ACTIONMA, bfalse );
                    }
                }
                if ( 0 != ( chr[cnt].latchbutton & LATCHBUTTON_ALTRIGHT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                {
                    chr[cnt].reloadtime = GRABDELAY;
                    if ( chr[cnt].holdingwhich[1] == MAXCHR )
                    {
                        // Grab right
                        play_action( cnt, ACTIONMF, bfalse );
                    }
                    else
                    {
                        // Drop right
                        play_action( cnt, ACTIONMB, bfalse );
                    }
                }
                if ( 0 != ( chr[cnt].latchbutton & LATCHBUTTON_PACKLEFT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                {
                    chr[cnt].reloadtime = PACKDELAY;
                    item = chr[cnt].holdingwhich[0];
                    if ( item != MAXCHR )
                    {
                        if ( ( chr[item].iskursed || capistoobig[chr[item].model] ) && !capisequipment[chr[item].model] )
                        {
                            // The item couldn't be put away
                            chr[item].ai.alert |= ALERTIF_NOTPUTAWAY;
                        }
                        else
                        {
                            // Put the item into the pack
                            add_item_to_character_pack( item, cnt );
                        }
                    }
                    else
                    {
                        // Get a new one out and put it in hand
                        get_item_from_character_pack( cnt, GRIP_LEFT, bfalse );
                    }

                    // Make it take a little time
                    play_action( cnt, ACTIONMG, bfalse );
                }
                if ( 0 != ( chr[cnt].latchbutton & LATCHBUTTON_PACKRIGHT ) && chr[cnt].actionready && chr[cnt].reloadtime == 0 )
                {
                    chr[cnt].reloadtime = PACKDELAY;
                    item = chr[cnt].holdingwhich[1];
                    if ( item != MAXCHR )
                    {
                        if ( ( chr[item].iskursed || capistoobig[chr[item].model] ) && !capisequipment[chr[item].model] )
                        {
                            // The item couldn't be put away
                            chr[item].ai.alert |= ALERTIF_NOTPUTAWAY;
                        }
                        else
                        {
                            // Put the item into the pack
                            add_item_to_character_pack( item, cnt );
                        }
                    }
                    else
                    {
                        // Get a new one out and put it in hand
                        get_item_from_character_pack( cnt, GRIP_RIGHT, bfalse );
                    }

                    // Make it take a little time
                    play_action( cnt, ACTIONMG, bfalse );
                }
                if ( 0 != ( chr[cnt].latchbutton & LATCHBUTTON_LEFT ) && chr[cnt].reloadtime == 0 )
                {
                    // Which weapon?
                    weapon = chr[cnt].holdingwhich[0];
                    if ( weapon == MAXCHR )
                    {
                        // Unarmed means character itself is the weapon
                        weapon = cnt;
                    }

                    action = capweaponaction[chr[weapon].model];

                    // Can it do it?
                    allowedtoattack = btrue;

                    // First check if reload time and action is okay
                    if ( !madactionvalid[chr[cnt].model][action] || chr[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
                    else
                    {
                        // Then check if a skill is needed
                        if ( capneedskillidtouse[chr[weapon].model] )
                        {
                            if (check_skills( cnt, capidsz[chr[weapon].model][IDSZ_SKILL]) == bfalse )
                                allowedtoattack = bfalse;
                        }
                    }
                    if ( !allowedtoattack )
                    {
                        if ( chr[weapon].reloadtime == 0 )
                        {
                            // This character can't use this weapon
                            chr[weapon].reloadtime = 50;
                            if ( chr[cnt].staton )
                            {
                                // Tell the player that they can't use this weapon
                                STRING text;
                                snprintf( text, sizeof(text),  "%s can't use this item...", chr[cnt].name );
                                debug_message( text );
                            }
                        }
                    }
                    if ( action == ACTIONDA )
                    {
                        allowedtoattack = bfalse;
                        if ( chr[weapon].reloadtime == 0 )
                        {
                            chr[weapon].ai.alert |= ALERTIF_USED;
                        }
                    }
                    if ( allowedtoattack )
                    {
                        // Rearing mount
                        mount = chr[cnt].attachedto;
                        if ( mount != MAXCHR )
                        {
                            allowedtoattack = capridercanattack[chr[mount].model];
                            if ( chr[mount].ismount && chr[mount].alive && !chr[mount].isplayer && chr[mount].actionready )
                            {
                                if ( ( action != ACTIONPA || !allowedtoattack ) && chr[cnt].actionready )
                                {
                                    play_action( chr[cnt].attachedto, ACTIONUA + ( rand()&1 ), bfalse );
                                    chr[chr[cnt].attachedto].ai.alert |= ALERTIF_USED;
                                    chr[cnt].ai.lastitemused = mount;
                                }
                                else
                                {
                                    allowedtoattack = bfalse;
                                }
                            }
                        }

                        // Attack button
                        if ( allowedtoattack )
                        {
                            if ( chr[cnt].actionready && madactionvalid[chr[cnt].model][action] )
                            {
                                // Check mana cost
                                if ( chr[cnt].mana >= chr[weapon].manacost || chr[cnt].canchannel )
                                {
                                    cost_mana( cnt, chr[weapon].manacost, weapon );
                                    // Check life healing
                                    chr[cnt].life += chr[weapon].lifeheal;
                                    if ( chr[cnt].life > chr[cnt].lifemax )  chr[cnt].life = chr[cnt].lifemax;

                                    actionready = bfalse;
                                    if ( action == ACTIONPA )
                                        actionready = btrue;

                                    action += rand() & 1;
                                    play_action( cnt, action, actionready );
                                    if ( weapon != cnt )
                                    {
                                        // Make the weapon attack too
                                        play_action( weapon, ACTIONMJ, bfalse );
                                        chr[weapon].ai.alert |= ALERTIF_USED;
                                        chr[cnt].ai.lastitemused = weapon;
                                    }
                                    else
                                    {
                                        // Flag for unarmed attack
                                        chr[cnt].ai.alert |= ALERTIF_USED;
                                        chr[cnt].ai.lastitemused = cnt;
                                    }
                                }
                            }
                        }
                    }
                }
                else if ( 0 != (chr[cnt].latchbutton & LATCHBUTTON_RIGHT) && chr[cnt].reloadtime == 0 )
                {
                    // Which weapon?
                    weapon = chr[cnt].holdingwhich[1];
                    if ( weapon == MAXCHR )
                    {
                        // Unarmed means character itself is the weapon
                        weapon = cnt;
                    }

                    action = capweaponaction[chr[weapon].model] + 2;

                    // Can it do it? (other hand)
                    allowedtoattack = btrue;

                    // First check if reload time and action is okay
                    if ( !madactionvalid[chr[cnt].model][action] || chr[weapon].reloadtime > 0 ) allowedtoattack = bfalse;
                    else
                    {
                        // Then check if a skill is needed
                        if ( capneedskillidtouse[chr[weapon].model] )
                        {
                            if ( check_skills( cnt, capidsz[chr[weapon].model][IDSZ_SKILL]) == bfalse   )
                                allowedtoattack = bfalse;
                        }
                    }
                    if ( !allowedtoattack )
                    {
                        if ( chr[weapon].reloadtime == 0 )
                        {
                            // This character can't use this weapon
                            chr[weapon].reloadtime = 50;
                            if ( chr[cnt].staton )
                            {
                                // Tell the player that they can't use this weapon
                                STRING text;
                                snprintf( text, sizeof(text), "%s can't use this item...", chr[cnt].name );
                                debug_message( text );
                            }
                        }
                    }
                    if ( action == ACTIONDC )
                    {
                        allowedtoattack = bfalse;
                        if ( chr[weapon].reloadtime == 0 )
                        {
                            chr[weapon].ai.alert |= ALERTIF_USED;
                            chr[cnt].ai.lastitemused = weapon;
                        }
                    }
                    if ( allowedtoattack )
                    {
                        // Rearing mount
                        mount = chr[cnt].attachedto;
                        if ( mount != MAXCHR )
                        {
                            allowedtoattack = capridercanattack[chr[mount].model];
                            if ( chr[mount].ismount && chr[mount].alive && !chr[mount].isplayer && chr[mount].actionready )
                            {
                                if ( ( action != ACTIONPC || !allowedtoattack ) && chr[cnt].actionready )
                                {
                                    play_action( chr[cnt].attachedto, ACTIONUC + ( rand()&1 ), bfalse );
                                    chr[chr[cnt].attachedto].ai.alert |= ALERTIF_USED;
                                    chr[cnt].ai.lastitemused = chr[cnt].attachedto;
                                }
                                else
                                {
                                    allowedtoattack = bfalse;
                                }
                            }
                        }

                        // Attack button
                        if ( allowedtoattack )
                        {
                            if ( chr[cnt].actionready && madactionvalid[chr[cnt].model][action] )
                            {
                                // Check mana cost
                                if ( chr[cnt].mana >= chr[weapon].manacost || chr[cnt].canchannel )
                                {
                                    cost_mana( cnt, chr[weapon].manacost, weapon );
                                    // Check life healing
                                    chr[cnt].life += chr[weapon].lifeheal;
                                    if ( chr[cnt].life > chr[cnt].lifemax )  chr[cnt].life = chr[cnt].lifemax;

                                    actionready = bfalse;
                                    if ( action == ACTIONPC )
                                        actionready = btrue;

                                    action += rand() & 1;
                                    play_action( cnt, action, actionready );
                                    if ( weapon != cnt )
                                    {
                                        // Make the weapon attack too
                                        play_action( weapon, ACTIONMJ, bfalse );
                                        chr[weapon].ai.alert |= ALERTIF_USED;
                                        chr[cnt].ai.lastitemused = weapon;
                                    }
                                    else
                                    {
                                        // Flag for unarmed attack
                                        chr[cnt].ai.alert |= ALERTIF_USED;
                                        chr[cnt].ai.lastitemused = cnt;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Is the character in the air?
        level = chr[cnt].level;
        if ( chr[cnt].flyheight == 0 )
        {
            chr[cnt].zpos += chr[cnt].zvel;
            if ( chr[cnt].zpos > level || ( chr[cnt].zvel > STOPBOUNCING && chr[cnt].zpos > level - STOPBOUNCING ) )
            {
                // Character is in the air
                chr[cnt].jumpready = bfalse;
                chr[cnt].zvel += gravity;

                // Down jump timers for flapping characters
                if ( chr[cnt].jumptime != 0 ) chr[cnt].jumptime--;

                // Airborne characters still get friction to make control easier
                friction = airfriction;
            }
            else
            {
                // Character is on the ground
                chr[cnt].zpos = level;
                grounded = btrue;
                if ( chr[cnt].hitready )
                {
                    chr[cnt].ai.alert |= ALERTIF_HITGROUND;
                    chr[cnt].hitready = bfalse;
                }

                // Make the characters slide
                twist = 119;
                friction = noslipfriction;
                if ( INVALID_TILE != chr[cnt].onwhichfan )
                {
                    Uint32 itile = chr[cnt].onwhichfan;

                    twist = meshtwist[itile];

                    if ( 0 != ( meshfx[itile] & MESHFX_SLIPPY ) )
                    {
                        if ( wateriswater && 0 != ( meshfx[itile] & MESHFX_WATER ) && chr[cnt].level < watersurfacelevel + RAISE + 1 )
                        {
                            // It says it's slippy, but the water is covering it...
                            // Treat exactly as normal
                            chr[cnt].jumpready = btrue;
                            chr[cnt].jumpnumber = chr[cnt].jumpnumberreset;
                            if ( chr[cnt].jumptime > 0 ) chr[cnt].jumptime--;

                            chr[cnt].zvel = -chr[cnt].zvel * chr[cnt].dampen;
                            chr[cnt].zvel += gravity;
                        }
                        else
                        {
                            // It's slippy all right...
                            friction = slippyfriction;
                            chr[cnt].jumpready = btrue;
                            if ( chr[cnt].jumptime > 0 ) chr[cnt].jumptime--;
                            if ( chr[cnt].weight != 0xFFFF )
                            {
                                // Slippy hills make characters slide
                                chr[cnt].xvel += vellrtwist[twist];
                                chr[cnt].yvel += veludtwist[twist];
                                chr[cnt].zvel = -SLIDETOLERANCE;
                            }
                            if ( flattwist[twist] )
                            {
                                // Reset jumping on flat areas of slippiness
                                chr[cnt].jumpnumber = chr[cnt].jumpnumberreset;
                            }
                        }
                    }
                    else
                    {
                        // Reset jumping
                        chr[cnt].jumpready = btrue;
                        chr[cnt].jumpnumber = chr[cnt].jumpnumberreset;
                        if ( chr[cnt].jumptime > 0 ) chr[cnt].jumptime--;

                        chr[cnt].zvel = -chr[cnt].zvel * chr[cnt].dampen;
                        chr[cnt].zvel += gravity;
                    }
                }

                // Characters with sticky butts lie on the surface of the mesh
                if ( chr[cnt].stickybutt || !chr[cnt].alive )
                {
                    maplr = chr[cnt].turnmaplr;
                    maplr = ( maplr << 3 ) - maplr + maplrtwist[twist];
                    mapud = chr[cnt].turnmapud;
                    mapud = ( mapud << 3 ) - mapud + mapudtwist[twist];
                    chr[cnt].turnmaplr = maplr >> 3;
                    chr[cnt].turnmapud = mapud >> 3;
                }
            }
        }
        else
        {
            //  Flying
            chr[cnt].jumpready = bfalse;
            chr[cnt].zpos += chr[cnt].zvel;
            if ( level < 0 ) level = 0;  // Don't fall in pits...

            chr[cnt].zvel += ( level + chr[cnt].flyheight - chr[cnt].zpos ) * FLYDAMPEN;
            if ( chr[cnt].zpos < level )
            {
                chr[cnt].zpos = level;
                chr[cnt].zvel = 0;
            }

            // Airborne characters still get friction to make control easier
            friction = airfriction;
        }

        // Move the character
        chr[cnt].xpos += chr[cnt].xvel;
        if ( __chrhitawall( cnt ) ) { chr[cnt].xpos = chr[cnt].oldx; chr[cnt].xvel = -chr[cnt].xvel; }

        chr[cnt].ypos += chr[cnt].yvel;
        if ( __chrhitawall( cnt ) ) { chr[cnt].ypos = chr[cnt].oldy; chr[cnt].yvel = -chr[cnt].yvel; }

        // Apply friction for next time
        chr[cnt].xvel = chr[cnt].xvel * friction;
        chr[cnt].yvel = chr[cnt].yvel * friction;

        // Animate the character
        chr[cnt].lip = ( chr[cnt].lip + 64 );
        if ( chr[cnt].lip == 192 )
        {
            // Check frame effects
            if ( madframefx[chr[cnt].frame]&MADFXACTLEFT )
                character_swipe( cnt, 0 );
            if ( madframefx[chr[cnt].frame]&MADFXACTRIGHT )
                character_swipe( cnt, 1 );
            if ( madframefx[chr[cnt].frame]&MADFXGRABLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, bfalse );
            if ( madframefx[chr[cnt].frame]&MADFXGRABRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, bfalse );
            if ( madframefx[chr[cnt].frame]&MADFXCHARLEFT )
                character_grab_stuff( cnt, GRIP_LEFT, btrue );
            if ( madframefx[chr[cnt].frame]&MADFXCHARRIGHT )
                character_grab_stuff( cnt, GRIP_RIGHT, btrue );
            if ( madframefx[chr[cnt].frame]&MADFXDROPLEFT )
                detach_character_from_mount( chr[cnt].holdingwhich[0], bfalse, btrue );
            if ( madframefx[chr[cnt].frame]&MADFXDROPRIGHT )
                detach_character_from_mount( chr[cnt].holdingwhich[1], bfalse, btrue );
            if ( madframefx[chr[cnt].frame]&MADFXPOOF && !chr[cnt].isplayer )
                chr[cnt].ai.poof_time = frame_wld;
            if ( madframefx[chr[cnt].frame]&MADFXFOOTFALL )
            {
                int ifoot = capsoundindex[chr[cnt].model][SOUND_FOOTFALL];
                if ( ifoot >= 0 && ifoot < MAXWAVE )
                {
                    sound_play_chunk( chr[cnt].xpos, chr[cnt].ypos, capwavelist[chr[cnt].model][ifoot] );
                }
            }
        }
        if ( chr[cnt].lip == 0 )
        {
            // Change frames
            chr[cnt].lastframe = chr[cnt].frame;
            chr[cnt].frame++;
            if ( chr[cnt].frame == madactionend[chr[cnt].model][chr[cnt].action] )
            {
                // Action finished
                if ( chr[cnt].keepaction )
                {
                    // Keep the last frame going
                    chr[cnt].frame = chr[cnt].lastframe;
                }
                else
                {
                    if ( !chr[cnt].loopaction )
                    {
                        // Go on to the next action
                        chr[cnt].action = chr[cnt].nextaction;
                        chr[cnt].nextaction = ACTIONDA;
                    }
                    else
                    {
                        // See if the character is mounted...
                        if ( chr[cnt].attachedto != MAXCHR )
                        {
                            chr[cnt].action = ACTIONMI;
                        }
                    }

                    chr[cnt].frame = madactionstart[chr[cnt].model][chr[cnt].action];
                }

                chr[cnt].actionready = btrue;
            }
        }

        // Do "Be careful!" delay
        if ( chr[cnt].carefultime != 0 )
        {
            chr[cnt].carefultime--;
        }

        // Get running, walking, sneaking, or dancing, from speed
        if ( !( chr[cnt].keepaction || chr[cnt].loopaction ) )
        {
            framelip = madframelip[chr[cnt].frame];  // 0 - 15...  Way through animation
            if ( chr[cnt].actionready && chr[cnt].lip == 0 && grounded && chr[cnt].flyheight == 0 && ( framelip&7 ) < 2 )
            {
                // Do the motion stuff
                speed = ABS( ( int ) chr[cnt].xvel ) + ABS( ( int ) chr[cnt].yvel );
                if ( speed < chr[cnt].sneakspd )
                {
                    //                        chr[cnt].nextaction = ACTIONDA;
                    // Do boredom
                    chr[cnt].boretime--;
                    if ( chr[cnt].boretime < 0 )
                    {
                        chr[cnt].ai.alert |= ALERTIF_BORED;
                        chr[cnt].boretime = BORETIME;
                    }
                    else
                    {
                        // Do standstill
                        if ( chr[cnt].action > ACTIONDD )
                        {
                            chr[cnt].action = ACTIONDA;
                            chr[cnt].frame = madactionstart[chr[cnt].model][chr[cnt].action];
                        }
                    }
                }
                else
                {
                    chr[cnt].boretime = BORETIME;
                    if ( speed < chr[cnt].walkspd )
                    {
                        chr[cnt].nextaction = ACTIONWA;
                        if ( chr[cnt].action != ACTIONWA )
                        {
                            chr[cnt].frame = madframeliptowalkframe[chr[cnt].model][LIPWA][framelip];
                            chr[cnt].action = ACTIONWA;
                        }
                    }
                    else
                    {
                        if ( speed < chr[cnt].runspd )
                        {
                            chr[cnt].nextaction = ACTIONWB;
                            if ( chr[cnt].action != ACTIONWB )
                            {
                                chr[cnt].frame = madframeliptowalkframe[chr[cnt].model][LIPWB][framelip];
                                chr[cnt].action = ACTIONWB;
                            }
                        }
                        else
                        {
                            chr[cnt].nextaction = ACTIONWC;
                            if ( chr[cnt].action != ACTIONWC )
                            {
                                chr[cnt].frame = madframeliptowalkframe[chr[cnt].model][LIPWC][framelip];
                                chr[cnt].action = ACTIONWC;
                            }
                        }
                    }
                }
            }
        }
    }

    // Do poofing
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !chr[cnt].on || !(chr[cnt].ai.poof_time >= 0 && chr[cnt].ai.poof_time <= frame_wld)  ) continue;

        if ( chr[cnt].attachedto != MAXCHR )
        {
            detach_character_from_mount( cnt, btrue, bfalse );
        }

        if ( chr[cnt].holdingwhich[0] != MAXCHR )
        {
            detach_character_from_mount( chr[cnt].holdingwhich[0], btrue, bfalse );
        }

        if ( chr[cnt].holdingwhich[1] != MAXCHR )
        {
            detach_character_from_mount( chr[cnt].holdingwhich[1], btrue, bfalse );
        }

        free_inventory( cnt );
        free_one_character( cnt );
    }
}

struct s_chr_setup_info
{
    STRING spawn_name;
    char   *pname;
    Sint32 slot;
    float  x, y, z;
    int    passage, content, money, level, skin;
    bool_t ghost;
    bool_t stat;
    Uint8  team;
    Uint16 facing;
    Uint16 attach;
    Uint16 parent;
};
typedef struct s_chr_setup_info chr_setup_info_t;

//--------------------------------------------------------------------------------------------
bool_t chr_setup_read( FILE * fileread, chr_setup_info_t *pinfo )
{
    int cnt;
    char cTmp;

    // trap bad pointers
    if ( NULL == fileread || NULL == pinfo ) return bfalse;

    // check for another entry
    if ( !goto_colon_yesno( fileread ) ) return bfalse;

    fscanf( fileread, "%s", pinfo->spawn_name );
    for ( cnt = 0; cnt < sizeof(pinfo->spawn_name); cnt++ )
    {
        if ( pinfo->spawn_name[cnt] == '_' )  pinfo->spawn_name[cnt] = ' ';
    }

    pinfo->pname = pinfo->spawn_name;
    if ( 0 == strcmp( pinfo->spawn_name, "NONE") )
    {
        // Random pinfo->pname
        pinfo->pname = NULL;
    }

    fscanf( fileread, "%d", &pinfo->slot );

    fscanf( fileread, "%f%f%f", &pinfo->x, &pinfo->y, &pinfo->z );
    pinfo->x *= 128;  pinfo->y *= 128;  pinfo->z *= 128;

    pinfo->facing = NORTH;
    pinfo->attach = ATTACH_NONE;
    cTmp = get_first_letter( fileread );
    if ( 'S' == toupper(cTmp) )  pinfo->facing = SOUTH;
    if ( 'E' == toupper(cTmp) )  pinfo->facing = EAST;
    if ( 'W' == toupper(cTmp) )  pinfo->facing = WEST;
    if ( 'L' == toupper(cTmp) )  pinfo->attach = ATTACH_LEFT;
    if ( 'R' == toupper(cTmp) )  pinfo->attach = ATTACH_RIGHT;
    if ( 'I' == toupper(cTmp) )  pinfo->attach = ATTACH_INVENTORY;

    fscanf( fileread, "%d%d%d%d%d", &pinfo->money, &pinfo->skin, &pinfo->passage, &pinfo->content, &pinfo->level );
    cTmp = get_first_letter( fileread );
    pinfo->stat = ( 'T' == toupper(cTmp) );

    cTmp = get_first_letter( fileread );
    pinfo->ghost = ( 'T' == toupper(cTmp) );

    cTmp = get_first_letter( fileread );
    pinfo->team = ( cTmp - 'A' ) % MAXTEAM;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_setup_apply( Uint16 ichr, chr_setup_info_t *pinfo )
{
    // trap bad pointers
    if ( NULL == pinfo ) return bfalse;

    if ( ichr >= MAXCHR || !chr[ichr].on ) return bfalse;

    chr[ichr].money += pinfo->money;
    if ( chr[ichr].money > MAXMONEY )  chr[ichr].money = MAXMONEY;
    if ( chr[ichr].money < 0 )  chr[ichr].money = 0;

    chr[ichr].ai.content = pinfo->content;
    chr[ichr].ai.passage = pinfo->passage;

    if ( pinfo->attach == ATTACH_INVENTORY )
    {
        // Inventory character
        add_item_to_character_pack( ichr, pinfo->parent );

        chr[ichr].ai.alert |= ALERTIF_GRABBED;  // Make spellbooks change
        chr[ichr].attachedto = pinfo->parent;  // Make grab work
        let_character_think( ichr );  // Empty the grabbed messages

        chr[ichr].attachedto = MAXCHR;  // Fix grab
    }
    else if ( pinfo->attach == ATTACH_LEFT || pinfo->attach == ATTACH_RIGHT )
    {
        // Wielded character
        Uint16 grip = ( ATTACH_LEFT == pinfo->attach ) ? GRIP_LEFT : GRIP_RIGHT;
        attach_character_to_mount( ichr, pinfo->parent, grip );

        // Handle the "grabbed" messages
        let_character_think( ichr );
    }

    // Set the starting pinfo->level
    if ( pinfo->level > 0 )
    {
        while ( chr[ichr].experiencelevel < pinfo->level && chr[ichr].experience < MAXXP )
        {
            give_experience( ichr, 25, XPDIRECT, btrue );
            do_level_up( ichr );
        }
    }

    if ( pinfo->ghost )
    {
        // Make the character a pinfo->ghost !!!BAD!!!  Can do with enchants
        chr[ichr].alpha = 128;
        chr[ichr].light = 255;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void setup_characters(  const char *modname )
{
    // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
    int new_object, tnc, local_index = 0;
    STRING newloadname;
    FILE *fileread;

    chr_setup_info_t info;

    // Turn all characters off
    free_all_characters();

    // Turn some back on
    make_newloadname( modname, "gamedat" SLASH_STR "spawn.txt", newloadname );
    fileread = fopen( newloadname, "r" );

    numpla = 0;
    info.parent = MAXCHR;
    if ( NULL == fileread )
    {
        log_error( "Cannot read file: %s", newloadname );
    }
    else
    {
        info.parent = 0;

        while ( chr_setup_read( fileread, &info ) )
        {
            // Spawn the character
            if ( info.team < numplayer || !rtscontrol || info.team >= MAXPLAYER )
            {
                new_object = spawn_one_character( info.x, info.y, info.z, info.slot, info.team, info.skin, info.facing, info.pname, MAXCHR );

                if ( MAXCHR != new_object )
                {
                    // determine the attachment
                    if ( info.attach == ATTACH_NONE )
                    {
                        // Free character
                        info.parent = new_object;
                        make_one_character_matrix( new_object );
                    }

                    chr_setup_apply( new_object, &info );

                    // Turn on numpla input devices
                    if ( info.stat )
                    {

                        if ( 0 == importamount && numpla < playeramount )
                        {
                            if ( 0 == local_numlpla )
                            {
                                // the first player gets everything
                                add_player( new_object, numpla, (Uint32)(~0) );
                            }
                            else
                            {
                                int i;
                                Uint32 bits;

                                // each new player steals an input device from the 1st player
                                bits = 1 << local_numlpla;
                                for ( i = 0; i < MAXPLAYER; i++ )
                                {
                                    pladevice[i] &= ~bits;
                                }

                                add_player( new_object, numpla, bits );
                            }

                        }
                        else if ( numpla < numimport && numpla < importamount && numpla < playeramount )
                        {
                            // Multiplayer import module
                            local_index = -1;
                            for ( tnc = 0; tnc < numimport; tnc++ )
                            {
                                if ( capimportslot[chr[new_object].model] == local_slot[tnc] )
                                {
                                    local_index = tnc;
                                    break;
                                }
                            }

                            if ( -1 != local_index )
                            {
                                // It's a local numpla
                                add_player( new_object, numpla, local_control[local_index] );
                            }
                            else
                            {
                                // It's a remote numpla
                                add_player( new_object, numpla, INPUT_BITS_NONE );
                            }
                        }

                        // Turn on the stat display
                        add_stat( new_object );
                    }
                }
            }
        }

        fclose( fileread );
    }

    clear_messages();

    // Make sure local players are displayed first
    sort_stat();

    // Fix tilting trees problem
    tilt_characters_to_terrain();
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch( Uint16 player )
{
    // ZZ> This function converts input readings to latch settings, so players can
    //     move around
    float newx, newy;
    Uint16 turnsin, turncos, character;
    Uint8 device;
    float dist, scale;
    float inputx, inputy;

    // Check to see if we need to bother
    if ( plavalid[player] && pladevice[player] != INPUT_BITS_NONE )
    {
        // Make life easier
        character = plaindex[player];
        device = pladevice[player];

        // Clear the player's latch buffers
        plalatchbutton[player] = 0;
        plalatchx[player] = 0;
        plalatchy[player] = 0;

        // Mouse routines
        if ( ( device & INPUT_BITS_MOUSE ) && mous.on )
        {
            // Movement
            newx = 0;
            newy = 0;
            if ( ( autoturncamera == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) )  // Don't allow movement in camera control mode
            {
                dist = SQRT( mous.x * mous.x + mous.y * mous.y );
                if ( dist > 0 )
                {
                    scale = mous.sense / dist;
                    if ( dist < mous.sense )
                    {
                        scale = dist / mous.sense;
                    }

                    scale = scale / mous.sense;
                    if ( chr[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = mous.x * chr[chr[character].attachedto].maxaccel * scale;
                        inputy = mous.y * chr[chr[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = mous.x * chr[character].maxaccel * scale;
                        inputy = mous.y * chr[character].maxaccel * scale;
                    }

                    turnsin = ( camturnleftrightone * 16383 );
                    turnsin = turnsin & 16383;
                    turncos = ( turnsin + 4096 ) & 16383;
                    if ( autoturncamera == 255 &&
                            local_numlpla == 1 &&
                            control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_CAMERA ) == 0 )  inputx = 0;

                    newx = ( inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                    newy = (-inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
//                    plalatchx[player]+=newx;
//                    plalatchy[player]+=newy;
                }
            }

            plalatchx[player] += newx * mous.cover + mous.latcholdx * mous.sustain;
            plalatchy[player] += newy * mous.cover + mous.latcholdy * mous.sustain;
            mous.latcholdx = plalatchx[player];
            mous.latcholdy = plalatchy[player];

            // Sustain old movements to ease mouse play
//            plalatchx[player]+=mous.latcholdx*mous.sustain;
//            plalatchy[player]+=mous.latcholdy*mous.sustain;
//            mous.latcholdx = plalatchx[player];
//            mous.latcholdy = plalatchy[player];
            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_MOUSE,  CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
        }

        // Joystick A routines
        if ( ( device & INPUT_BITS_JOYA ) && joy[0].on )
        {
            // Movement
            if ( ( autoturncamera == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[0].x * joy[0].x + joy[0].y * joy[0].y );
                if ( dist > 0 )
                {
                    scale = 1.0f / dist;
                    if ( chr[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = joy[0].x * chr[chr[character].attachedto].maxaccel * scale;
                        inputy = joy[0].y * chr[chr[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joy[0].x * chr[character].maxaccel * scale;
                        inputy = joy[0].y * chr[character].maxaccel * scale;
                    }
                }

                turnsin = ( camturnleftrightone * 16383 );
                turnsin = turnsin & 16383;
                turncos = ( turnsin + 4096 ) & 16383;
                if ( autoturncamera == 255 &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
                plalatchx[player] += newx;
                plalatchy[player] += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 0, CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
        }

        // Joystick B routines
        if ( ( device & INPUT_BITS_JOYB ) && joy[1].on )
        {
            // Movement
            if ( ( autoturncamera == 255 && local_numlpla == 1 ) ||
                    !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )
            {
                newx = 0;
                newy = 0;
                inputx = 0;
                inputy = 0;
                dist = SQRT( joy[1].x * joy[1].x + joy[1].y * joy[1].y );
                if ( dist > 0 )
                {
                    scale = 1.0f / dist;
                    if ( chr[character].attachedto != MAXCHR )
                    {
                        // Mounted
                        inputx = joy[1].x * chr[chr[character].attachedto].maxaccel * scale;
                        inputy = joy[1].y * chr[chr[character].attachedto].maxaccel * scale;
                    }
                    else
                    {
                        // Unmounted
                        inputx = joy[1].x * chr[character].maxaccel * scale;
                        inputy = joy[1].y * chr[character].maxaccel * scale;
                    }
                }

                turnsin = ( camturnleftrightone * 16383 );
                turnsin = turnsin & 16383;
                turncos = ( turnsin + 4096 ) & 16383;
                if ( autoturncamera == 255 &&
                        local_numlpla == 1 &&
                        !control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_CAMERA ) )  inputx = 0;

                newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
                newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
                plalatchx[player] += newx;
                plalatchy[player] += newy;
            }

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_JOY + 1, CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
        }

        // Keyboard routines
        if ( ( device & INPUT_BITS_KEYBOARD ) && keyb.on )
        {
            // Movement
            if ( chr[character].attachedto != MAXCHR )
            {
                // Mounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) ) * chr[chr[character].attachedto].maxaccel;
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) ) * chr[chr[character].attachedto].maxaccel;
            }
            else
            {
                // Unmounted
                inputx = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT ) ) * chr[character].maxaccel;
                inputy = ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_DOWN ) - control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_UP ) ) * chr[character].maxaccel;
            }

            turnsin = ( camturnleftrightone * 16383 );
            turnsin = turnsin & 16383;
            turncos = ( turnsin + 4096 ) & 16383;
            if ( autoturncamera == 255 && local_numlpla == 1 )  inputx = 0;

            newx = (  inputx * turntocos[turnsin] + inputy * turntosin[turnsin] );
            newy = ( -inputx * turntosin[turnsin] + inputy * turntocos[turnsin] );
            plalatchx[player] += newx;
            plalatchy[player] += newy;

            // Read buttons
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_JUMP ) )
                plalatchbutton[player] |= LATCHBUTTON_JUMP;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_LEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_LEFT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_USE ) )
                plalatchbutton[player] |= LATCHBUTTON_RIGHT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_GET ) )
                plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;
            if ( control_is_pressed( INPUT_DEVICE_KEYBOARD,  CONTROL_RIGHT_PACK ) )
                plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
        }
    }
}

//--------------------------------------------------------------------------------------------
void set_local_latches( void )
{
    // ZZ> This function emulates AI thinkin' by setting latches from input devices
    int cnt;

    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        set_one_player_latch( cnt );
    }
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan( void )
{
    // ZZ> This function figures out which fan characters are on and sets their level
    Uint16 character, distance;
    int ripand;
    // int volume;
    float level;

    // First figure out which fan each character is in
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( !chr[character].on ) continue;

        chr[character].onwhichfan   = mesh_get_tile ( chr[character].xpos, chr[character].ypos );
        chr[character].onwhichblock = mesh_get_block( chr[character].xpos, chr[character].ypos );
    }

    // Get levels every update
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( !chr[character].on || chr[character].inpack ) continue;

        level = get_level( chr[character].xpos, chr[character].ypos, chr[character].waterwalk ) + RAISE;
        if ( chr[character].alive )
        {
            if ( ( INVALID_TILE != chr[character].onwhichfan ) && ( 0 != ( meshfx[chr[character].onwhichfan] & MESHFX_DAMAGE ) ) && ( chr[character].zpos <= chr[character].level + DAMAGERAISE ) && ( MAXCHR == chr[character].attachedto ) )
            {
                if ( ( chr[character].damagemodifier[damagetiletype]&DAMAGESHIFT ) != 3 && !chr[character].invictus ) // 3 means they're pretty well immune
                {
                    distance = ABS( camtrackx - chr[character].xpos ) + ABS( camtracky - chr[character].ypos );
                    if ( distance < damagetilemindistance )
                    {
                        damagetilemindistance = distance;
                    }
                    if ( distance < damagetilemindistance + 256 )
                    {
                        damagetilesoundtime = 0;
                    }
                    if ( chr[character].damagetime == 0 )
                    {
                        damage_character( character, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, chr[character].ai.bumplast, DAMFXBLOC | DAMFXARMO, bfalse );
                        chr[character].damagetime = DAMAGETILETIME;
                    }
                    if ( (damagetileparttype != ((Sint16)~0)) && ( frame_wld&damagetilepartand ) == 0 )
                    {
                        spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos,
                                            0, MAXMODEL, damagetileparttype, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    }
                }
                if ( chr[character].reaffirmdamagetype == damagetiletype )
                {
                    if ( ( frame_wld&TILEREAFFIRMAND ) == 0 )
                        reaffirm_attached_particles( character );
                }
            }
        }

        if ( chr[character].zpos < watersurfacelevel && INVALID_TILE != chr[character].onwhichfan && 0 != ( meshfx[chr[character].onwhichfan] & MESHFX_WATER ) )
        {
            if ( !chr[character].inwater )
            {
                // Splash
                if ( chr[character].attachedto == MAXCHR )
                {
                    spawn_one_particle( chr[character].xpos, chr[character].ypos, watersurfacelevel + RAISE,
                                        0, MAXMODEL, SPLASH, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                }

                chr[character].inwater = btrue;
                if ( wateriswater )
                {
                    chr[character].ai.alert |= ALERTIF_INWATER;
                }
            }
            else
            {
                if ( chr[character].zpos > watersurfacelevel - RIPPLETOLERANCE && capripple[chr[character].model] )
                {
                    // Ripples
                    ripand = ( ( int )chr[character].xvel != 0 ) | ( ( int )chr[character].yvel != 0 );
                    ripand = RIPPLEAND >> ripand;
                    if ( ( frame_wld&ripand ) == 0 && chr[character].zpos < watersurfacelevel && chr[character].alive )
                    {
                        spawn_one_particle( chr[character].xpos, chr[character].ypos, watersurfacelevel,
                                            0, MAXMODEL, RIPPLE, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    }
                }
                if ( wateriswater && ( frame_wld&7 ) == 0 )
                {
                    chr[character].jumpready = btrue;
                    chr[character].jumpnumber = 1; // chr[character].jumpnumberreset;
                }
            }

            chr[character].xvel = chr[character].xvel * waterfriction;
            chr[character].yvel = chr[character].yvel * waterfriction;
            chr[character].zvel = chr[character].zvel * waterfriction;
        }
        else
        {
            chr[character].inwater = bfalse;
        }

        chr[character].level = level;
    }

    // Play the damage tile sound
    if ( damagetilesound >= 0 )
    {
        if ( ( frame_wld & 3 ) == 0 )
        {
            // Change the volume...
            /*PORT
                        volume = -(damagetilemindistance + (damagetilesoundtime<<8));
                        volume = volume<<VOLSHIFT;
                        if(volume > VOLMIN)
                        {
                            lpDSBuffer[damagetilesound]->SetVolume(volume);
                        }
                        if(damagetilesoundtime < TILESOUNDTIME)  damagetilesoundtime++;
                        else damagetilemindistance = 9999;
            */
        }
    }
}

//--------------------------------------------------------------------------------------------
void bump_characters( void )
{
    // ZZ> This function sets handles characters hitting other characters or particles
    Uint16 character, particle, chara, charb, partb;
    Uint16 pip, direction;
    Uint32 fanblock, prtidparent, prtidtype, eveidremove;
    IDSZ   chridvulnerability;
    Sint8 hide;
    int tnc, dist, chrinblock, prtinblock, enchant, temp;
    float xa, ya, za, xb, yb, zb;
    float ax, ay, nx, ny, scale;  // For deflection
    Uint16 facing;

    // Clear the lists
    for ( fanblock = 0; fanblock < meshblocks; fanblock++ )
    {
        meshbumplistchr[fanblock]    = MAXCHR;
        meshbumplistchrnum[fanblock] = 0;

        meshbumplistprt[fanblock]    = TOTALMAXPRT;
        meshbumplistprtnum[fanblock] = 0;
    }

    // Fill 'em back up
    for ( character = 0; character < MAXCHR; character++ )
    {
        if ( !chr[character].on ) continue;

        // reset the holding weight each update
        chr[character].holdingweight = 0;

        // reset the fan and block position
        chr[character].onwhichfan   = mesh_get_tile ( chr[character].xpos, chr[character].ypos );
        chr[character].onwhichblock = mesh_get_block( chr[character].xpos, chr[character].ypos );

        // reject characters that are in packs, or are marked as non-colliding
        if ( chr[character].inpack || 0 == chr[character].bumpheight ) continue;

        // reject characters that are hidden
        hide = caphidestate[ chr[character].model ];
        if ( hide != NOHIDE && hide == chr[character].ai.state ) continue;

        if ( INVALID_BLOCK != chr[character].onwhichblock )
        {
            // Insert before any other characters on the block
            chr[character].bumpnext = meshbumplistchr[chr[character].onwhichblock];
            meshbumplistchr[chr[character].onwhichblock] = character;
            meshbumplistchrnum[chr[character].onwhichblock]++;
        }
    }

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        // reject invalid particles
        if ( !prton[particle] ) continue;

        // reset the fan and block position
        prtonwhichfan[particle]   = mesh_get_tile ( prtxpos[particle], prtypos[particle] );
        prtonwhichblock[particle] = mesh_get_block( prtxpos[particle], prtypos[particle] );

        if ( INVALID_BLOCK != prtonwhichblock[particle] )
        {
            // Insert before any other particles on the block
            prtbumpnext[particle] = meshbumplistprt[prtonwhichblock[particle]];
            meshbumplistprt[prtonwhichblock[particle]] = particle;
            meshbumplistprtnum[prtonwhichblock[particle]]++;
        }
    }

    // blank the accumulators
    for ( character = 0; character < MAXCHR; character++ )
    {
        chr[character].phys_pos_x = 0.0f;
        chr[character].phys_pos_y = 0.0f;
        chr[character].phys_pos_z = 0.0f;
        chr[character].phys_vel_x = 0.0f;
        chr[character].phys_vel_y = 0.0f;
        chr[character].phys_vel_z = 0.0f;
    }

    // Check collisions with other characters and bump particles
    // Only check each pair once
    for ( chara = 0; chara < MAXCHR; chara++ )
    {
        int ixmax, ixmin;
        int iymax, iymin;

        int ix_block, ixmax_block, ixmin_block;
        int iy_block, iymax_block, iymin_block;

        // make sure that it is on
        if ( !chr[chara].on ) continue;

        // Don't bump held objects
        if ( MAXCHR != chr[chara].attachedto ) continue;

        // Don't bump items
        if ( 0 == chr[chara].bumpheight /* || chr[chara].isitem */ ) continue;

        xa = chr[chara].xpos;
        ya = chr[chara].ypos;
        za = chr[chara].zpos;
        chridvulnerability = capidsz[chr[chara].model][IDSZ_VULNERABILITY];

        // determine the size of this object in blocks
        ixmin = chr[chara].xpos - chr[chara].bumpsize; ixmin = CLIP(ixmin, 0, meshedgex);
        ixmax = chr[chara].xpos + chr[chara].bumpsize; ixmax = CLIP(ixmax, 0, meshedgex);

        iymin = chr[chara].ypos - chr[chara].bumpsize; iymin = CLIP(iymin, 0, meshedgey);
        iymax = chr[chara].ypos + chr[chara].bumpsize; iymax = CLIP(iymax, 0, meshedgey);

        ixmax_block = ixmax >> 9; ixmax_block = CLIP( ixmax_block, 0, MAXMESHBLOCKY );
        ixmin_block = ixmin >> 9; ixmin_block = CLIP( ixmin_block, 0, MAXMESHBLOCKY );

        iymax_block = iymax >> 9; iymax_block = CLIP( iymax_block, 0, MAXMESHBLOCKY );
        iymin_block = iymin >> 9; iymin_block = CLIP( iymin_block, 0, MAXMESHBLOCKY );

        for (ix_block = ixmin_block; ix_block <= ixmax_block; ix_block++)
        {
            for (iy_block = iymin_block; iy_block <= iymax_block; iy_block++)
            {
                // Allow raw access here because we were careful :)
                fanblock = ix_block + meshblockstart[iy_block];

                chrinblock = meshbumplistchrnum[fanblock];
                prtinblock = meshbumplistprtnum[fanblock];

                for ( tnc = 0, charb = meshbumplistchr[fanblock];
                        tnc < chrinblock && charb != MAXCHR;
                        tnc++, charb = chr[charb].bumpnext)
                {
                    float dx, dy;

                    bool_t collide_x = bfalse;
                    bool_t collide_y  = bfalse;
                    bool_t collide_xy = bfalse;
                    bool_t collide_z  = bfalse;
                    bool_t collide_platform_a = bfalse;
                    bool_t collide_platform_b = bfalse;
                    bool_t collision = bfalse;

                    // Don't collide with self, and only do each collision pair once
                    if ( charb <= chara ) continue;

                    // don't interact with your mount, or your held items
                    if ( chara == chr[charb].attachedto || charb == chr[chara].attachedto ) continue;

                    xb = chr[charb].xpos;
                    yb = chr[charb].ypos;
                    zb = chr[charb].zpos;

                    dx = ABS( xa - xb );
                    dy = ABS( ya - yb );
                    dist = dx + dy;

                    //------------------
                    // do platforms

                    // check the absolute value diamond
                    if ( dist < chr[chara].bumpsizebig || dist < chr[charb].bumpsizebig )
                    {
                        collide_xy = btrue;
                    }

                    // check bounding box x
                    if ( ( dx < chr[chara].bumpsize || dx < chr[charb].bumpsize ) )
                    {
                        collide_x = btrue;
                    }

                    // check bounding box y
                    if ( ( dy < chr[chara].bumpsize || dy < chr[charb].bumpsize ) )
                    {
                        collide_y = btrue;
                    }

                    collide_platform_a = ( za > (zb + chr[charb].bumpheight - PLATTOLERANCE) ) && ( za < (zb + chr[charb].bumpheight) );
                    collide_platform_b = ( zb > (za + chr[chara].bumpheight - PLATTOLERANCE) ) && ( zb < (za + chr[chara].bumpheight) );

                    // do platforms
                    if ( collide_x && collide_y && collide_xy && ( collide_platform_a || collide_platform_b ) )
                    {
                        bool_t was_collide_platform_a = bfalse;
                        bool_t was_collide_platform_b = bfalse;

                        was_collide_platform_a = ( chr[chara].oldz > (chr[charb].oldz + chr[charb].bumpheight - PLATTOLERANCE) ) && ( chr[chara].oldz < (chr[charb].oldz + chr[charb].bumpheight) );
                        was_collide_platform_b = ( chr[charb].oldz > (chr[chara].oldz + chr[chara].bumpheight - PLATTOLERANCE) ) && ( chr[charb].oldz < (chr[chara].oldz + chr[chara].bumpheight) );

                        // Is A falling on B?
                        if ( !collision && collide_platform_a )
                        {
                            if ( capcanuseplatforms[chr[chara].model] && chr[charb].platform )//&&chr[chara].flyheight==0)
                            {
                                // make the character float up to the level of the platform
                                if ( was_collide_platform_a )
                                {
                                    chr[chara].phys_pos_z += -chr[chara].zpos + ( chr[chara].zpos ) * PLATKEEP + ( chr[charb].zpos + chr[charb].bumpheight + PLATADD ) * PLATASCEND;
                                    if ( chr[chara].zvel < chr[charb].zvel ) chr[chara].phys_vel_z += -chr[chara].zvel + chr[charb].zvel;
                                }

                                chr[chara].phys_vel_x += ( chr[charb].xvel ) * platstick;
                                chr[chara].phys_vel_y += ( chr[charb].yvel ) * platstick;
                                chr[chara].turnleftright += ( chr[charb].turnleftright - chr[charb].oldturn ) * platstick;

                                chr[chara].jumpready = btrue;
                                chr[chara].jumpnumber = chr[chara].jumpnumberreset;
                                chr[charb].holdingweight = chr[chara].weight;

                                chr[chara].ai.bumplast = charb;
                                chr[charb].ai.bumplast = chara;

                                collision = btrue;
                            }
                        }

                        // Is B falling on A?
                        if ( !collision && collide_platform_b )
                        {
                            if ( capcanuseplatforms[chr[charb].model] && chr[chara].platform )//&&chr[charb].flyheight==0)
                            {
                                // make the character float up to the level of the platform
                                if ( was_collide_platform_b )
                                {
                                    chr[charb].phys_pos_z  += -chr[charb].zpos + ( chr[charb].zpos ) * PLATKEEP + ( chr[chara].zpos + chr[chara].bumpheight + PLATADD ) * PLATASCEND;
                                    if ( chr[charb].zvel < chr[chara].zvel ) chr[charb].phys_vel_z  += -chr[charb].zvel + chr[chara].zvel;
                                }

                                chr[charb].phys_vel_x += ( chr[chara].xvel ) * platstick;
                                chr[charb].phys_vel_y += ( chr[chara].yvel ) * platstick;
                                chr[charb].turnleftright += ( chr[chara].turnleftright - chr[chara].oldturn ) * platstick;

                                chr[charb].jumpready = btrue;
                                chr[charb].jumpnumber = chr[charb].jumpnumberreset;
                                chr[chara].holdingweight = chr[charb].weight;

                                chr[chara].ai.bumplast = charb;
                                chr[charb].ai.bumplast = chara;

                                collision = btrue;
                            }
                        }

                    }

                    //------------------
                    // do characters

                    collide_xy = ( dist < chr[chara].bumpsizebig + chr[charb].bumpsizebig );
                    collide_x  = ( dx   < chr[chara].bumpsize    + chr[charb].bumpsize    );
                    collide_y  = ( dy   < chr[chara].bumpsize    + chr[charb].bumpsize    );
                    collide_z  = ( MIN(za + chr[chara].bumpheight, zb + chr[charb].bumpheight) - MAX(za, zb) > 0 );

                    if ( !collision && collide_z && collide_x && collide_y && collide_xy )
                    {
                        float vdot;

                        float depth_x, depth_y, depth_xy, depth_yx, depth_z;
                        glVector nrm;

						nrm.x = nrm.y = nrm.z = 0.0f;
						nrm.w = 1.0f;

                        depth_x  = MIN(xa + chr[chara].bumpsize, xb + chr[charb].bumpsize) - MAX(xa - chr[chara].bumpsize, xb - chr[charb].bumpsize);
                        if ( depth_x < 0.0f )
                        {
                            depth_x = 0.0f;
                        }
                        else
                        {
                            nrm.x += 1 / depth_x;
                        }

                        depth_y  = MIN(ya + chr[chara].bumpsize, yb + chr[charb].bumpsize) - MAX(ya - chr[chara].bumpsize, yb - chr[charb].bumpsize);
                        if ( depth_y < 0.0f )
                        {
                            depth_y = 0.0f;
                        }
                        else
                        {
                            nrm.y += 1 / depth_y;
                        }

                        depth_xy = MIN(xa + ya + chr[chara].bumpsizebig, xb + yb + chr[charb].bumpsizebig) - MAX(xa + ya - chr[chara].bumpsize, xb + yb - chr[charb].bumpsizebig);
                        if ( depth_xy < 0.0f )
                        {
                            depth_xy = 0.0f;
                        }
                        else
                        {
                            nrm.x += 1 / depth_xy;
                            nrm.y += 1 / depth_xy;
                        }

                        depth_yx = MIN(-xa + ya + chr[chara].bumpsizebig, -xb + yb + chr[charb].bumpsizebig) - MAX(-xa + ya - chr[chara].bumpsize, -xb + yb - chr[charb].bumpsizebig);
                        if ( depth_yx < 0.0f )
                        {
                            depth_yx = 0.0f;
                        }
                        else
                        {
                            nrm.x -= 1 / depth_yx;
                            nrm.y += 1 / depth_yx;
                        }

                        depth_z  = MIN(za + chr[chara].bumpheight, zb + chr[charb].bumpheight) - MAX( za, zb );
                        if ( depth_z < 0.0f )
                        {
                            depth_z = 0.0f;
                        }
                        else
                        {
                            nrm.z += 1 / depth_z;
                        }

                        if ( xa < xb ) nrm.x *= -1;
                        if ( ya < yb ) nrm.y *= -1;
                        if ( za < zb ) nrm.z *= -1;

                        vdot = (chr[chara].xvel - chr[charb].xvel) * nrm.x +
                               (chr[chara].yvel - chr[charb].yvel) * nrm.y +
                               (chr[chara].zvel - chr[charb].zvel) * nrm.z;

                        if ( vdot < 0.0f && ABS(nrm.x) + ABS(nrm.y) + ABS(nrm.z) > 0.0f )
                        {
                            float was_dx, was_dy, was_dist;

                            bool_t was_collide_x = bfalse;
                            bool_t was_collide_y  = bfalse;
                            bool_t was_collide_xy = bfalse;
                            bool_t was_collide_z  = bfalse;

                            nrm = Normalize( nrm );

                            was_dx = ABS( (xa - chr[chara].xvel) - (xb - chr[charb].xvel) );
                            was_dy = ABS( (ya - chr[chara].yvel) - (yb - chr[charb].yvel) );
                            was_dist = dx + dy;

                            was_collide_xy = ( was_dist < chr[chara].bumpsizebig + chr[charb].bumpsizebig );
                            was_collide_x  = ( was_dx   < chr[chara].bumpsize    + chr[charb].bumpsize    );
                            was_collide_y  = ( was_dy   < chr[chara].bumpsize    + chr[charb].bumpsize    );
                            was_collide_z = ( MIN(chr[chara].oldz + chr[chara].bumpheight, chr[charb].oldz + chr[charb].bumpheight) > MAX(chr[chara].oldz, chr[charb].oldz) );

                            if ( collide_xy != was_collide_xy || collide_x != was_collide_x || collide_y != was_collide_y )
                            {
                                // an acrual collision
                                float vdot;
                                glVector vcom;

                                vcom.x = chr[chara].xvel * chr[chara].weight + chr[charb].xvel * chr[charb].weight;
                                vcom.y = chr[chara].yvel * chr[chara].weight + chr[charb].yvel * chr[charb].weight;
                                vcom.z = chr[chara].zvel * chr[chara].weight + chr[charb].zvel * chr[charb].weight;

                                if ( chr[chara].weight + chr[charb].weight > 0 )
                                {
                                    vcom.x /= chr[chara].weight + chr[charb].weight;
                                    vcom.y /= chr[chara].weight + chr[charb].weight;
                                    vcom.z /= chr[chara].weight + chr[charb].weight;
                                }

                                // do the bounce
                                if ( chr[chara].weight != 0xFFFF )
                                {
                                    vdot = ( chr[chara].xvel - vcom.x ) * nrm.x + ( chr[chara].yvel - vcom.y ) * nrm.y + ( chr[chara].zvel - vcom.z ) * nrm.z;

                                    chr[chara].phys_vel_x  += -chr[chara].xvel + vcom.x - vdot * nrm.x * chr[chara].bumpdampen;
                                    chr[chara].phys_vel_y  += -chr[chara].yvel + vcom.y - vdot * nrm.y * chr[chara].bumpdampen;
                                    chr[chara].phys_vel_z  += -chr[chara].zvel + vcom.z - vdot * nrm.z * chr[chara].bumpdampen;
                                }

                                if ( chr[charb].weight != 0xFFFF )
                                {
                                    vdot = ( chr[charb].xvel - vcom.x ) * nrm.x + ( chr[charb].yvel - vcom.y ) * nrm.y + ( chr[charb].zvel - vcom.z ) * nrm.z;

                                    chr[charb].phys_vel_x  += -chr[charb].xvel + vcom.x - vdot * nrm.x * chr[charb].bumpdampen;
                                    chr[charb].phys_vel_y  += -chr[charb].yvel + vcom.y - vdot * nrm.y * chr[charb].bumpdampen;
                                    chr[charb].phys_vel_z  += -chr[charb].zvel + vcom.z - vdot * nrm.z * chr[charb].bumpdampen;
                                }

                                collision = btrue;
                            }
                            else
                            {
                                float tmin;

                                tmin = 1e6;
                                if ( nrm.x != 0 )
                                {
                                    tmin = MIN(tmin, depth_x / ABS(nrm.x) );
                                }
                                if ( nrm.y != 0 )
                                {
                                    tmin = MIN(tmin, depth_y / ABS(nrm.y) );
                                }
                                if ( nrm.z != 0 )
                                {
                                    tmin = MIN(tmin, depth_z / ABS(nrm.z) );
                                }

                                if ( nrm.x + nrm.y != 0 )
                                {
                                    tmin = MIN(tmin, depth_xy / ABS(nrm.x + nrm.y) );
                                }

                                if ( -nrm.x + nrm.y != 0 )
                                {
                                    tmin = MIN(tmin, depth_yx / ABS(-nrm.x + nrm.y) );
                                }

                                if ( tmin < 1e6 )
                                {
                                    if ( chr[chara].weight != 0xFFFF )
                                    {
                                        chr[chara].phys_pos_x += tmin * nrm.x * 0.125f;
                                        chr[chara].phys_pos_y += tmin * nrm.y * 0.125f;
                                        chr[chara].phys_pos_z += tmin * nrm.z * 0.125f;
                                    }

                                    if ( chr[charb].weight != 0xFFFF )
                                    {
                                        chr[charb].phys_pos_x -= tmin * nrm.x * 0.125f;
                                        chr[charb].phys_pos_y -= tmin * nrm.y * 0.125f;
                                        chr[charb].phys_pos_z -= tmin * nrm.z * 0.125f;
                                    }
                                }
                            }
                        }

                        if ( collision )
                        {
                            chr[chara].ai.bumplast = charb;
                            chr[charb].ai.bumplast = chara;

                            chr[chara].ai.alert |= ALERTIF_BUMPED;
                            chr[charb].ai.alert |= ALERTIF_BUMPED;
                        }
                    }
                }

                // do mounting
                charb = chr[chara].ai.bumplast;
                if ( charb != chara && chr[charb].on && !chr[charb].inpack && chr[charb].attachedto == MAXCHR && 0 != chr[chara].bumpheight && 0 != chr[charb].bumpheight )
                {
                    float dx, dy;

                    xb = chr[charb].xpos;
                    yb = chr[charb].ypos;
                    zb = chr[charb].zpos;

                    // First check absolute value diamond
                    dx = ABS( xa - xb );
                    dy = ABS( ya - yb );
                    dist = dx + dy;
                    if ( dist < chr[chara].bumpsizebig || dist < chr[charb].bumpsizebig )
                    {
                        // Then check bounding box square...  Square+Diamond=Octagon
                        if ( ( dx < chr[chara].bumpsize || dx < chr[charb].bumpsize ) &&
                                ( dy < chr[chara].bumpsize || dy < chr[charb].bumpsize ) )
                        {
                            // Now see if either is on top the other like a platform
                            if ( za > zb + chr[charb].bumpheight - PLATTOLERANCE + chr[chara].zvel - chr[charb].zvel && ( capcanuseplatforms[chr[chara].model] || za > zb + chr[charb].bumpheight ) )
                            {
                                // Is A falling on B?
                                if ( za < zb + chr[charb].bumpheight && chr[charb].platform && chr[chara].alive )//&&chr[chara].flyheight==0)
                                {
                                    if ( madactionvalid[chr[chara].model][ACTIONMI] && chr[chara].alive && chr[charb].alive && chr[charb].ismount && !chr[chara].isitem && chr[charb].holdingwhich[0] == MAXCHR && chr[chara].attachedto == MAXCHR && chr[chara].jumptime == 0 && chr[chara].flyheight == 0 )
                                    {
                                        attach_character_to_mount( chara, charb, GRIP_ONLY );
                                        chr[chara].ai.bumplast = chara;
                                        chr[charb].ai.bumplast = charb;
                                    }
                                }
                            }
                            else
                            {
                                if ( zb > za + chr[chara].bumpheight - PLATTOLERANCE + chr[charb].zvel - chr[chara].zvel && ( capcanuseplatforms[chr[charb].model] || zb > za + chr[chara].bumpheight ) )
                                {
                                    // Is B falling on A?
                                    if ( zb < za + chr[chara].bumpheight && chr[chara].platform && chr[charb].alive )//&&chr[charb].flyheight==0)
                                    {
                                        if ( madactionvalid[chr[charb].model][ACTIONMI] && chr[chara].alive && chr[charb].alive && chr[chara].ismount && !chr[charb].isitem && chr[chara].holdingwhich[0] == MAXCHR && chr[charb].attachedto == MAXCHR && chr[charb].jumptime == 0 && chr[charb].flyheight == 0 )
                                        {
                                            attach_character_to_mount( charb, chara, GRIP_ONLY );

                                            chr[chara].ai.bumplast = chara;
                                            chr[charb].ai.bumplast = charb;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Now check collisions with every bump particle in same area
                if ( chr[chara].alive )
                {
                    for ( tnc = 0, partb = meshbumplistprt[fanblock];
                            tnc < prtinblock;
                            tnc++, partb = prtbumpnext[partb] )
                    {
                        float dx, dy;

                        // do not collide with the thing that you're attached to
                        if ( chara == prtattachedtocharacter[partb] ) continue;

                        // if there's no friendly fire, particles issued by chara can't hit it
                        if ( !pipfriendlyfire[prtpip[partb]] && chara == prtchr[partb] ) continue;

                        xb = prtxpos[partb];
                        yb = prtypos[partb];
                        zb = prtzpos[partb];

                        // First check absolute value diamond
                        dx = ABS( xa - xb );
                        dy = ABS( ya - yb );
                        dist = dx + dy;
                        if ( dist < chr[chara].bumpsizebig || dist < prtbumpsizebig[partb] )
                        {
                            // Then check bounding box square...  Square+Diamond=Octagon
                            if ( ( dx < chr[chara].bumpsize  || dx < prtbumpsize[partb] ) &&
                                    ( dy < chr[chara].bumpsize  || dy < prtbumpsize[partb] ) &&
                                    ( zb > za - prtbumpheight[partb] && zb < za + chr[chara].bumpheight + prtbumpheight[partb] ) )
                            {
                                pip = prtpip[partb];
                                if ( zb > za + chr[chara].bumpheight + prtzvel[partb] && prtzvel[partb] < 0 && chr[chara].platform && prtattachedtocharacter[partb] == MAXCHR )
                                {
                                    // Particle is falling on A
                                    prtzpos[partb] = za + chr[chara].bumpheight;
                                    prtzvel[partb] = -prtzvel[partb] * pipdampen[pip];
                                    prtxvel[partb] += ( chr[chara].xvel ) * platstick;
                                    prtyvel[partb] += ( chr[chara].yvel ) * platstick;
                                }

                                // Check reaffirmation of particles
                                if ( prtattachedtocharacter[partb] != chara )
                                {
                                    if ( chr[chara].reloadtime == 0 )
                                    {
                                        if ( chr[chara].reaffirmdamagetype == prtdamagetype[partb] && chr[chara].damagetime == 0 )
                                        {
                                            reaffirm_attached_particles( chara );
                                        }
                                    }
                                }

                                // Check for missile treatment
                                if ( ( chr[chara].damagemodifier[prtdamagetype[partb]]&3 ) < 2 ||
                                        chr[chara].missiletreatment == MISNORMAL ||
                                        prtattachedtocharacter[partb] != MAXCHR ||
                                        ( prtchr[partb] == chara && !pipfriendlyfire[pip] ) ||
                                        ( chr[chr[chara].missilehandler].mana < ( chr[chara].missilecost << 4 ) && !chr[chr[chara].missilehandler].canchannel ) )
                                {
                                    if ( ( teamhatesteam[prtteam[partb]][chr[chara].team] || ( pipfriendlyfire[pip] && ( ( chara != prtchr[partb] && chara != chr[prtchr[partb]].attachedto ) || piponlydamagefriendly[pip] ) ) ) && !chr[chara].invictus )
                                    {
                                        spawn_bump_particles( chara, partb ); // Catch on fire
                                        if ( ( prtdamagebase[partb] | prtdamagerand[partb] ) > 1 )
                                        {
                                            prtidparent = capidsz[prtmodel[partb]][IDSZ_PARENT];
                                            prtidtype = capidsz[prtmodel[partb]][IDSZ_TYPE];
                                            if ( chr[chara].damagetime == 0 && prtattachedtocharacter[partb] != chara && ( pipdamfx[pip]&DAMFXARRO ) == 0 )
                                            {
                                                // Normal partb damage
                                                if ( pipallowpush[pip] )
                                                {
                                                    chr[chara].phys_vel_x  += -chr[chara].xvel + prtxvel[partb] * chr[chara].bumpdampen;
                                                    chr[chara].phys_vel_y  += -chr[chara].yvel + prtyvel[partb] * chr[chara].bumpdampen;
                                                    chr[chara].phys_vel_z  += -chr[chara].zvel + prtzvel[partb] * chr[chara].bumpdampen;
                                                }

                                                direction = ( ATAN2( prtyvel[partb], prtxvel[partb] ) + PI ) * 0xFFFF / ( TWO_PI );
                                                direction = chr[chara].turnleftright - direction + 32768;
                                                // Check all enchants to see if they are removed
                                                enchant = chr[chara].firstenchant;

                                                while ( enchant != MAXENCHANT )
                                                {
                                                    eveidremove = everemovedbyidsz[enceve[enchant]];
                                                    temp = encnextenchant[enchant];
                                                    if ( eveidremove != IDSZ_NONE && ( eveidremove == prtidtype || eveidremove == prtidparent ) )
                                                    {
                                                        remove_enchant( enchant );
                                                    }

                                                    enchant = temp;
                                                }

                                                // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                                                //+2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                                                if ( pipintdamagebonus[pip] )
                                                {
                                                    int percent;
                                                    percent = ( chr[prtchr[partb]].intelligence - 3584 ) >> 7;
                                                    percent /= 100;
                                                    prtdamagebase[partb] *= 1 + percent;
                                                }
                                                if ( pipwisdamagebonus[pip] )
                                                {
                                                    int percent;
                                                    percent = ( chr[prtchr[partb]].wisdom - 3584 ) >> 7;
                                                    percent /= 100;
                                                    prtdamagebase[partb] *= 1 + percent;
                                                }

                                                // Damage the character
                                                if ( chridvulnerability != IDSZ_NONE && ( chridvulnerability == prtidtype || chridvulnerability == prtidparent ) )
                                                {
                                                    damage_character( chara, direction, prtdamagebase[partb] << 1, prtdamagerand[partb] << 1, prtdamagetype[partb], prtteam[partb], prtchr[partb], pipdamfx[pip], bfalse );
                                                    chr[chara].ai.alert |= ALERTIF_HITVULNERABLE;
                                                }
                                                else
                                                {
                                                    damage_character( chara, direction, prtdamagebase[partb], prtdamagerand[partb], prtdamagetype[partb], prtteam[partb], prtchr[partb], pipdamfx[pip], bfalse );
                                                }

                                                // Do confuse effects
                                                if ( 0 == ( madframefx[chr[chara].frame]&MADFXINVICTUS ) || pipdamfx[pip]&DAMFXBLOC )
                                                {
                                                    if ( pipgrogtime[pip] != 0 && capcanbegrogged[chr[chara].model] )
                                                    {
                                                        chr[chara].grogtime += pipgrogtime[pip];
                                                        if ( chr[chara].grogtime < 0 )  chr[chara].grogtime = 32767;

                                                        chr[chara].ai.alert |= ALERTIF_GROGGED;
                                                    }
                                                    if ( pipdazetime[pip] != 0 && capcanbedazed[chr[chara].model] )
                                                    {
                                                        chr[chara].dazetime += pipdazetime[pip];
                                                        if ( chr[chara].dazetime < 0 )  chr[chara].dazetime = 32767;

                                                        chr[chara].ai.alert |= ALERTIF_DAZED;
                                                    }
                                                }

                                                // Notify the attacker of a scored hit
                                                if ( prtchr[partb] != MAXCHR )
                                                {
                                                    chr[prtchr[partb]].ai.alert |= ALERTIF_SCOREDAHIT;
                                                    chr[prtchr[partb]].ai.hitlast = chara;
                                                }
                                            }
                                            if ( ( frame_wld&31 ) == 0 && prtattachedtocharacter[partb] == chara )
                                            {
                                                // Attached partb damage ( Burning )
                                                if ( pipxyvelbase[pip] == 0 )
                                                {
                                                    // Make character limp
                                                    chr[chara].phys_vel_x += -chr[chara].xvel;
                                                    chr[chara].phys_vel_y += -chr[chara].yvel;
                                                }

                                                damage_character( chara, 32768, prtdamagebase[partb], prtdamagerand[partb], prtdamagetype[partb], prtteam[partb], prtchr[partb], pipdamfx[pip], bfalse );
                                            }
                                        }
                                        if ( pipendbump[pip] )
                                        {
                                            if ( pipbumpmoney[pip] )
                                            {
                                                if ( chr[chara].cangrabmoney && chr[chara].alive && chr[chara].damagetime == 0 && chr[chara].money != MAXMONEY )
                                                {
                                                    if ( chr[chara].ismount )
                                                    {
                                                        // Let mounts collect money for their riders
                                                        if ( chr[chara].holdingwhich[0] != MAXCHR )
                                                        {
                                                            chr[chr[chara].holdingwhich[0]].money += pipbumpmoney[pip];
                                                            if ( chr[chr[chara].holdingwhich[0]].money > MAXMONEY ) chr[chr[chara].holdingwhich[0]].money = MAXMONEY;
                                                            if ( chr[chr[chara].holdingwhich[0]].money < 0 ) chr[chr[chara].holdingwhich[0]].money = 0;

                                                            prttime[partb] = 1;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        // Normal money collection
                                                        chr[chara].money += pipbumpmoney[pip];
                                                        if ( chr[chara].money > MAXMONEY ) chr[chara].money = MAXMONEY;
                                                        if ( chr[chara].money < 0 ) chr[chara].money = 0;

                                                        prttime[partb] = 1;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                prttime[partb] = 1;
                                                // Only hit one character, not several
                                                prtdamagebase[partb] = 0;
                                                prtdamagerand[partb] = 1;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    if ( prtchr[partb] != chara )
                                    {
                                        cost_mana( chr[chara].missilehandler, ( chr[chara].missilecost << 4 ), prtchr[partb] );

                                        // Treat the missile
                                        if ( chr[chara].missiletreatment == MISDEFLECT )
                                        {
                                            // Use old position to find normal
                                            ax = prtxpos[partb] - prtxvel[partb];
                                            ay = prtypos[partb] - prtyvel[partb];
                                            ax = chr[chara].xpos - ax;
                                            ay = chr[chara].ypos - ay;
                                            // Find size of normal
                                            scale = ax * ax + ay * ay;
                                            if ( scale > 0 )
                                            {
                                                // Make the normal a unit normal
                                                scale = SQRT( scale );
                                                nx = ax / scale;
                                                ny = ay / scale;
                                                // Deflect the incoming ray off the normal
                                                scale = ( prtxvel[partb] * nx + prtyvel[partb] * ny ) * 2;
                                                ax = scale * nx;
                                                ay = scale * ny;
                                                prtxvel[partb] = prtxvel[partb] - ax;
                                                prtyvel[partb] = prtyvel[partb] - ay;
                                            }
                                        }
                                        else
                                        {
                                            // Reflect it back in the direction it came
                                            prtxvel[partb] = -prtxvel[partb];
                                            prtyvel[partb] = -prtyvel[partb];
                                        }

                                        // Change the owner of the missile
                                        if ( !piphoming[pip] )
                                        {
                                            prtteam[partb] = chr[chara].team;
                                            prtchr[partb] = chara;
                                        }

                                        // Change the direction of the partb
                                        if ( piprotatetoface[pip] )
                                        {
                                            // Turn to face new direction
                                            facing = ATAN2( prtyvel[partb], prtxvel[partb] ) * 0xFFFF / ( TWO_PI );
                                            facing += 32768;
                                            prtfacing[partb] = facing;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // accumulate the accumulators
    for ( character = 0; character < MAXCHR; character++ )
    {
        float tmpx, tmpy, tmpz;

        // do the "integration" of the accumulated accelerations
        chr[character].xvel += chr[character].phys_vel_x;
        chr[character].yvel += chr[character].phys_vel_y;
        chr[character].zvel += chr[character].phys_vel_z;

        // do the "integration" on the position
        tmpx = chr[character].xpos;
        chr[character].xpos += chr[character].phys_pos_x;
        if ( __chrhitawall(character) )
        {
            // restore the old values
            chr[character].xpos = tmpx;
        }

        tmpy = chr[character].ypos;
        chr[character].ypos += chr[character].phys_pos_y;
        if ( __chrhitawall(character) )
        {
            // restore the old values
            chr[character].ypos = tmpy;
        }

        tmpz = chr[character].zpos;
        chr[character].zpos += chr[character].phys_pos_z;
        if ( chr[character].zpos < chr[character].level )
        {
            // restore the old values
            chr[character].zpos = tmpz;
        }

    }
}

//--------------------------------------------------------------------------------------------
void stat_return()
{
    // ZZ> This function brings mana and life back
    int cnt, owner, target, eve;

    // Do reload time
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].reloadtime > 0 )
        {
            chr[cnt].reloadtime--;
        }

        cnt++;
    }

    // Do stats
    if ( clock_stat >= ONESECOND )
    {
        // Reset the clock
        clock_stat -= ONESECOND;

        // Do all the characters
        for ( cnt = 0; cnt < MAXCHR; cnt++ )
        {
            if ( !chr[cnt].on ) continue;

            // check for a level up
            do_level_up( cnt );

            // do the mana and life regen for "living" characters
            if ( chr[cnt].alive )
            {
                chr[cnt].mana += ( chr[cnt].manareturn / MANARETURNSHIFT );
                chr[cnt].mana = MAX(0, MIN(chr[cnt].mana, chr[cnt].manamax));

                chr[cnt].life += chr[cnt].lifereturn;
                chr[cnt].life = MAX(1, MIN(chr[cnt].life, chr[cnt].life));
            }
            if ( chr[cnt].grogtime > 0 )
            {
                chr[cnt].grogtime--;
            }
            if ( chr[cnt].dazetime > 0 )
            {
                chr[cnt].dazetime--;
            }
        }

        // Run through all the enchants as well
        for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
        {
            if ( !encon[cnt] ) continue;

            if ( enctime[cnt] != 0 )
            {
                if ( enctime[cnt] > 0 )
                {
                    enctime[cnt]--;
                }

                owner = encowner[cnt];
                target = enctarget[cnt];
                eve = enceve[cnt];

                // Do drains
                if ( chr[owner].alive )
                {
                    // Change life
                    chr[owner].life += encownerlife[cnt];
                    if ( chr[owner].life < 1 )
                    {
                        chr[owner].life = 1;
                        kill_character( owner, target );
                    }
                    if ( chr[owner].life > chr[owner].lifemax )
                    {
                        chr[owner].life = chr[owner].lifemax;
                    }

                    // Change mana
                    if ( !cost_mana( owner, -encownermana[cnt], target ) && eveendifcantpay[eve] )
                    {
                        remove_enchant( cnt );
                    }
                }
                else if ( !evestayifnoowner[eve] )
                {
                    remove_enchant( cnt );
                }

                if ( encon[cnt] )
                {
                    if ( chr[target].alive )
                    {
                        // Change life
                        chr[target].life += enctargetlife[cnt];
                        if ( chr[target].life < 1 )
                        {
                            chr[target].life = 1;
                            kill_character( target, owner );
                        }
                        if ( chr[target].life > chr[target].lifemax )
                        {
                            chr[target].life = chr[target].lifemax;
                        }

                        // Change mana
                        if ( !cost_mana( target, -enctargetmana[cnt], owner ) && eveendifcantpay[eve] )
                        {
                            remove_enchant( cnt );
                        }
                    }
                    else
                    {
                        remove_enchant( cnt );
                    }
                }
            }
            else
            {
                remove_enchant( cnt );
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void update_pits()
{
    // ZZ> This function kills any character in a deep pit...
    int cnt;
    if ( pitskill || pitsfall )
    {
        if ( pitclock > 19 )
        {
            pitclock = 0;

            // Kill any particles that fell in a pit, if they die in water...
            cnt = 0;

            while ( cnt < maxparticles )
            {
                if ( prton[cnt] )
                {
                    if ( prtzpos[cnt] < PITDEPTH && pipendwater[prtpip[cnt]] )
                    {
                        prttime[cnt] = 1;
                    }
                }

                cnt++;
            }

            // Kill or teleport any characters that fell in a pit...
            cnt = 0;

            while ( cnt < MAXCHR )
            {
                if ( chr[cnt].on && chr[cnt].alive && !chr[cnt].inpack )
                {
                    if ( !chr[cnt].invictus && chr[cnt].zpos < PITDEPTH && chr[cnt].attachedto == MAXCHR )
                    {
                        //Do we kill it?
                        if (pitskill)
                        {
                            // Got one!
                            kill_character( cnt, MAXCHR );
                            chr[cnt].xvel = 0;
                            chr[cnt].yvel = 0;

                            //Play sound effect
                            sound_play_chunk( chr[cnt].xpos, chr[cnt].ypos, g_wavelist[GSND_PITFALL] );
                        }

                        //Do we teleport it?
                        if (pitsfall && chr[cnt].zpos < PITDEPTH*8)
                        {
                            // Yeah!  It worked!
                            detach_character_from_mount( cnt, btrue, bfalse );
                            chr[cnt].oldx = chr[cnt].xpos;
                            chr[cnt].oldy = chr[cnt].ypos;
                            chr[cnt].xpos = pitx;
                            chr[cnt].ypos = pity;
                            chr[cnt].zpos = pitz;
                            if ( __chrhitawall( cnt ) )
                            {
                                // No it didn't...
                                chr[cnt].xpos = chr[cnt].oldx;
                                chr[cnt].ypos = chr[cnt].oldy;
                                chr[cnt].zpos = chr[cnt].oldz;

                                // Kill it instead
                                kill_character( cnt, MAXCHR );
                                chr[cnt].xvel = 0;
                                chr[cnt].yvel = 0;
                            }
                            else
                            {
                                chr[cnt].oldx = chr[cnt].xpos;
                                chr[cnt].oldy = chr[cnt].ypos;
                                chr[cnt].oldz = chr[cnt].zpos;

                                //Stop movement
                                chr[cnt].zvel = 0;
                                chr[cnt].xvel = 0;
                                chr[cnt].yvel = 0;

                                //Play sound effect
                                if (chr[cnt].isplayer)
                                {
                                    sound_play_chunk( camtrackx, camtracky, g_wavelist[GSND_PITFALL] );
                                }
                                else
                                {
                                    sound_play_chunk( chr[cnt].xpos, chr[cnt].ypos, g_wavelist[GSND_PITFALL] );
                                }

                                //Do some damage (same as damage tile)
                                damage_character( cnt, 32768, damagetileamount, 1, damagetiletype, DAMAGETEAM, chr[cnt].ai.bumplast, DAMFXBLOC | DAMFXARMO, btrue );
                            }
                        }
                    }

                }

                cnt++;
            }
        }
        else
        {
            pitclock++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void reset_players()
{
    // ZZ> This function clears the player list data
    int cnt, tnc;

    // Reset the local data stuff
    local_seekurse     = bfalse;
    local_senseenemies = MAXCHR;
    local_seeinvisible = bfalse;
    local_allpladead    = bfalse;

    // Reset the initial player data and latches
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        plavalid[cnt] = bfalse;
        plaindex[cnt] = 0;
        plalatchx[cnt] = 0;
        plalatchy[cnt] = 0;
        plalatchbutton[cnt] = 0;

        platimetimes[cnt] = 0;
        for ( tnc = 0; tnc < MAXLAG; tnc++ )
        {
            platimelatchx[cnt][tnc]      = 0;
            platimelatchy[cnt][tnc]      = 0;
            platimelatchbutton[cnt][tnc] = 0;
            platimetime[cnt][tnc]        = 0;
        }

        pladevice[cnt] = INPUT_BITS_NONE;
    }

    numpla = 0;
    nexttimestamp = ((Uint32)~0);
    numplatimes   = 0;
}
//--------------------------------------------------------------------------------------------
void drop_money( Uint16 character, Uint16 money )
{
    // ZZ> This function drops some of a character's money
    Uint16 huns, tfives, fives, ones, cnt;
    if ( money > chr[character].money )  money = chr[character].money;
    if ( money > 0 && chr[character].zpos > -2 )
    {
        chr[character].money = chr[character].money - money;
        huns = money / 100;  money -= ( huns << 7 ) - ( huns << 5 ) + ( huns << 2 );
        tfives = money / 25;  money -= ( tfives << 5 ) - ( tfives << 3 ) + tfives;
        fives = money / 5;  money -= ( fives << 2 ) + fives;
        ones = money;

        for ( cnt = 0; cnt < ones; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN1, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < fives; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN5, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < tfives; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN25, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        for ( cnt = 0; cnt < huns; cnt++ )
        {
            spawn_one_particle( chr[character].xpos, chr[character].ypos,  chr[character].zpos, 0, MAXMODEL, COIN100, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, cnt, MAXCHR );
        }

        chr[character].damagetime = DAMAGETIME;  // So it doesn't grab it again
    }
}

//--------------------------------------------------------------------------------------------
void call_for_help( Uint16 character )
{
    // ZZ> This function issues a call for help to all allies
    Uint8 team;
    Uint16 cnt;

    team = chr[character].team;
    teamsissy[team] = character;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( chr[cnt].on && cnt != character && !teamhatesteam[chr[cnt].team][team] )
        {
            chr[cnt].ai.alert |= ALERTIF_CALLEDFORHELP;
        }
    }
}

//--------------------------------------------------------------------------------------------
Uint32 xp_for_next_level(Uint16 character)
{
    Uint32 curlevel;
    Uint16 profile;
    Uint32 xpneeded = (Uint32)(~0);
    if ( !chr[character].on ) return xpneeded;

    profile  = chr[character].model;
    if (profile == MAXMODEL) return xpneeded;

    // Do level ups and stat changes
    curlevel = chr[character].experiencelevel;
    if ( curlevel + 1 < MAXLEVEL )
    {
        xpneeded = capexperienceforlevel[profile][curlevel+1];
    }
    else
    {
        xpneeded = capexperienceforlevel[profile][MAXLEVEL - 1];
        xpneeded += ( ( curlevel + 1 ) * ( curlevel + 1 ) * ( curlevel + 1 ) * 15 );
        xpneeded -= ( ( MAXLEVEL - 1 ) * ( MAXLEVEL - 1 ) * ( MAXLEVEL - 1 ) * 15 );
    }

    return xpneeded;
}

//--------------------------------------------------------------------------------------------
void do_level_up( Uint16 character )
{
    // BB > level gains are done here, but only once a second

    Uint8 curlevel;
    int number;
    Uint16 profile;
    STRING text;
    if (character >= MAXCHR || !chr[character].on) return;

    profile = chr[character].model;
    if ( profile >= MAXMODEL ) return;

    // Do level ups and stat changes
    curlevel = chr[character].experiencelevel;
    if ( curlevel + 1 < 20 )
    {
        Uint32 xpcurrent, xpneeded;

        xpcurrent = chr[character].experience;
        xpneeded  = xp_for_next_level(character);
        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            chr[character].experiencelevel++;
            xpneeded  = xp_for_next_level(character);

            // The character is ready to advance...
            if ( chr[character].isplayer )
            {
                snprintf( text, sizeof(text), "%s gained a level!!!", chr[character].name );
                debug_message( text );
                sound_play_chunk( camtrackx, camtracky, g_wavelist[GSND_LEVELUP] );
            }

            // Size
            chr[character].sizegoto += capsizeperlevel[profile] * 0.5f;  // Limit this?
            chr[character].sizegototime += SIZETIME;

            // Strength
            number = generate_number( capstrengthperlevelbase[profile], capstrengthperlevelrand[profile] );
            number = number + chr[character].strength;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].strength = number;

            // Wisdom
            number = generate_number( capwisdomperlevelbase[profile], capwisdomperlevelrand[profile] );
            number = number + chr[character].wisdom;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].wisdom = number;

            // Intelligence
            number = generate_number( capintelligenceperlevelbase[profile], capintelligenceperlevelrand[profile] );
            number = number + chr[character].intelligence;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].intelligence = number;

            // Dexterity
            number = generate_number( capdexterityperlevelbase[profile], capdexterityperlevelrand[profile] );
            number = number + chr[character].dexterity;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].dexterity = number;

            // Life
            number = generate_number( caplifeperlevelbase[profile], caplifeperlevelrand[profile] );
            number = number + chr[character].lifemax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            chr[character].life += ( number - chr[character].lifemax );
            chr[character].lifemax = number;

            // Mana
            number = generate_number( capmanaperlevelbase[profile], capmanaperlevelrand[profile] );
            number = number + chr[character].manamax;
            if ( number > PERFECTBIG ) number = PERFECTBIG;
            chr[character].mana += ( number - chr[character].manamax );
            chr[character].manamax = number;

            // Mana Return
            number = generate_number( capmanareturnperlevelbase[profile], capmanareturnperlevelrand[profile] );
            number = number + chr[character].manareturn;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].manareturn = number;

            // Mana Flow
            number = generate_number( capmanaflowperlevelbase[profile], capmanaflowperlevelrand[profile] );
            number = number + chr[character].manaflow;
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            chr[character].manaflow = number;
        }
    }
}

//--------------------------------------------------------------------------------------------
void give_experience( Uint16 character, int amount, Uint8 xptype, bool_t override_invictus )
{
    // ZZ> This function gives a character experience

    int newamount;
    int profile;
    if (amount == 0) return;
    if ( !chr[character].invictus || override_invictus )
    {
        // Figure out how much experience to give
        profile = chr[character].model;
        newamount = amount;
        if ( xptype < XP_COUNT )
        {
            newamount = amount * capexperiencerate[profile][xptype];
        }

        //Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
        newamount = newamount * (1 + ((float)FP8_TO_INT(chr[character].intelligence - 2560) / 200))
                    + (1 + ((float)FP8_TO_INT(chr[character].wisdom - 2560) / 400));

        chr[character].experience += newamount;
    }
}

//--------------------------------------------------------------------------------------------
void give_team_experience( Uint8 team, int amount, Uint8 xptype )
{
    // ZZ> This function gives a character experience, and pawns off level gains to
    //     another function
    int cnt;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( chr[cnt].team == team && chr[cnt].on )
        {
            give_experience( cnt, amount, xptype, bfalse );
        }
    }
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
    // ZZ> This function makes the characters get bigger or smaller, depending
    //     on their sizegoto and sizegototime
    int cnt = 0;
    bool_t willgetcaught;
    float newsize;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on && chr[cnt].sizegototime && chr[cnt].sizegoto != chr[cnt].fat )
        {
            int bump_increase;

            bump_increase = ( chr[cnt].sizegoto - chr[cnt].fat ) * 0.10f * chr[cnt].bumpsize;

            // Make sure it won't get caught in a wall
            willgetcaught = bfalse;
            if ( chr[cnt].sizegoto > chr[cnt].fat )
            {
                chr[cnt].bumpsize += bump_increase;

                if ( __chrhitawall( cnt ) )
                {
                    willgetcaught = btrue;
                }

                chr[cnt].bumpsize -= bump_increase;
            }

            // If it is getting caught, simply halt growth until later
            if ( !willgetcaught )
            {
                // Figure out how big it is
                chr[cnt].sizegototime--;

                newsize = chr[cnt].sizegoto;
                if ( chr[cnt].sizegototime > 0 )
                {
                    newsize = ( chr[cnt].fat * 0.90f ) + ( newsize * 0.10f );
                }

                // Make it that big...
                chr[cnt].fat   = newsize;
                chr[cnt].shadowsize = chr[cnt].shadowsizesave * newsize;
                chr[cnt].bumpsize = chr[cnt].bumpsizesave * newsize;
                chr[cnt].bumpsizebig = chr[cnt].bumpsizebigsave * newsize;
                chr[cnt].bumpheight = chr[cnt].bumpheightsave * newsize;

                if ( capweight[chr[cnt].model] == 0xFF )
                {
                    chr[cnt].weight = 0xFFFF;
                }
                else
                {
                    int itmp = capweight[chr[cnt].model] * chr[cnt].fat * chr[cnt].fat * chr[cnt].fat;
                    chr[cnt].weight = MIN( itmp, 0xFFFE );
                }
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_name(  const char *szSaveName, Uint16 character )
{
    // ZZ> This function makes the naming.txt file for the character
    FILE* filewrite;
    int profile;
    char cTmp;
    int cnt, tnc;

    // Can it export?
    profile = chr[character].model;
    filewrite = fopen( szSaveName, "w" );
    if ( filewrite )
    {
        cnt = 0;
        cTmp = chr[character].name[0];
        cnt++;

        while ( cnt < MAXCAPNAMESIZE && cTmp != 0 )
        {
            fprintf( filewrite, ":" );
            tnc = 0;

            while ( tnc < 8 && cTmp != 0 )
            {
                if ( cTmp == ' ' )
                {
                    fprintf( filewrite, "_" );
                }
                else
                {
                    fprintf( filewrite, "%c", cTmp );
                }

                cTmp = chr[character].name[cnt];
                tnc++;
                cnt++;
            }

            fprintf( filewrite, "\n" );
            fprintf( filewrite, ":STOP\n\n" );
        }

        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_profile(  const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a data.txt file for the given character.
    //     it is assumed that all enchantments have been done away with
    FILE* filewrite;
    int profile;
    int damagetype, skin;
    char types[10] = "SCPHEFIZ";
    char codes[4];

    // General stuff
    profile = chr[character].model;

    // Open the file
    filewrite = fopen( szSaveName, "w" );
    if ( filewrite )
    {
        // Real general data
        fprintf( filewrite, "Slot number    : -1\n" );  // -1 signals a flexible load thing
        funderf( filewrite, "Class name     : ", capclassname[profile] );
        ftruthf( filewrite, "Uniform light  : ", capuniformlit[profile] );
        fprintf( filewrite, "Maximum ammo   : %d\n", capammomax[profile] );
        fprintf( filewrite, "Current ammo   : %d\n", chr[character].ammo );
        fgendef( filewrite, "Gender         : ", chr[character].gender );
        fprintf( filewrite, "\n" );

        // Object stats
        fprintf( filewrite, "Life color     : %d\n", chr[character].lifecolor );
        fprintf( filewrite, "Mana color     : %d\n", chr[character].manacolor );
        fprintf( filewrite, "Life           : %4.2f\n", chr[character].lifemax / 256.0f );
        fpairof( filewrite, "Life up        : ", caplifeperlevelbase[profile], caplifeperlevelrand[profile] );
        fprintf( filewrite, "Mana           : %4.2f\n", chr[character].manamax / 256.0f );
        fpairof( filewrite, "Mana up        : ", capmanaperlevelbase[profile], capmanaperlevelrand[profile] );
        fprintf( filewrite, "Mana return    : %4.2f\n", chr[character].manareturn / 256.0f );
        fpairof( filewrite, "Mana return up : ", capmanareturnperlevelbase[profile], capmanareturnperlevelrand[profile] );
        fprintf( filewrite, "Mana flow      : %4.2f\n", chr[character].manaflow / 256.0f );
        fpairof( filewrite, "Mana flow up   : ", capmanaflowperlevelbase[profile], capmanaflowperlevelrand[profile] );
        fprintf( filewrite, "STR            : %4.2f\n", chr[character].strength / 256.0f );
        fpairof( filewrite, "STR up         : ", capstrengthperlevelbase[profile], capstrengthperlevelrand[profile] );
        fprintf( filewrite, "WIS            : %4.2f\n", chr[character].wisdom / 256.0f );
        fpairof( filewrite, "WIS up         : ", capwisdomperlevelbase[profile], capwisdomperlevelrand[profile] );
        fprintf( filewrite, "INT            : %4.2f\n", chr[character].intelligence / 256.0f );
        fpairof( filewrite, "INT up         : ", capintelligenceperlevelbase[profile], capintelligenceperlevelrand[profile] );
        fprintf( filewrite, "DEX            : %4.2f\n", chr[character].dexterity / 256.0f );
        fpairof( filewrite, "DEX up         : ", capdexterityperlevelbase[profile], capdexterityperlevelrand[profile] );
        fprintf( filewrite, "\n" );

        // More physical attributes
        fprintf( filewrite, "Size           : %4.2f\n", chr[character].sizegoto );
        fprintf( filewrite, "Size up        : %4.2f\n", capsizeperlevel[profile] );
        fprintf( filewrite, "Shadow size    : %d\n", capshadowsize[profile] );
        fprintf( filewrite, "Bump size      : %d\n", capbumpsize[profile] );
        fprintf( filewrite, "Bump height    : %d\n", capbumpheight[profile] );
        fprintf( filewrite, "Bump dampen    : %4.2f\n", capbumpdampen[profile] );
        fprintf( filewrite, "Weight         : %d\n", capweight[profile] );
        fprintf( filewrite, "Jump power     : %4.2f\n", capjump[profile] );
        fprintf( filewrite, "Jump number    : %d\n", capjumpnumber[profile] );
        fprintf( filewrite, "Sneak speed    : %d\n", capsneakspd[profile] );
        fprintf( filewrite, "Walk speed     : %d\n", capwalkspd[profile] );
        fprintf( filewrite, "Run speed      : %d\n", caprunspd[profile] );
        fprintf( filewrite, "Fly to height  : %d\n", capflyheight[profile] );
        fprintf( filewrite, "Flashing AND   : %d\n", capflashand[profile] );
        fprintf( filewrite, "Alpha blending : %d\n", capalpha[profile] );
        fprintf( filewrite, "Light blending : %d\n", caplight[profile] );
        ftruthf( filewrite, "Transfer blend : ", captransferblend[profile] );
        fprintf( filewrite, "Sheen          : %d\n", capsheen[profile] );
        ftruthf( filewrite, "Phong mapping  : ", capenviro[profile] );
        fprintf( filewrite, "Texture X add  : %4.2f\n", capuoffvel[profile] / (float)0xFFFF );
        fprintf( filewrite, "Texture Y add  : %4.2f\n", capvoffvel[profile] / (float)0xFFFF );
        ftruthf( filewrite, "Sticky butt    : ", capstickybutt[profile] );
        fprintf( filewrite, "\n" );

        // Invulnerability data
        ftruthf( filewrite, "Invictus       : ", capinvictus[profile] );
        fprintf( filewrite, "NonI facing    : %d\n", capnframefacing[profile] );
        fprintf( filewrite, "NonI angle     : %d\n", capnframeangle[profile] );
        fprintf( filewrite, "I facing       : %d\n", capiframefacing[profile] );
        fprintf( filewrite, "I angle        : %d\n", capiframeangle[profile] );
        fprintf( filewrite, "\n" );

        // Skin defenses
        fprintf( filewrite, "Base defense   : %3d %3d %3d %3d\n", 255 - capdefense[profile][0], 255 - capdefense[profile][1],
                 255 - capdefense[profile][2], 255 - capdefense[profile][3] );
        damagetype = 0;

        while ( damagetype < DAMAGE_COUNT )
        {
            fprintf( filewrite, "%c damage shift : %3d %3d %3d %3d\n", types[damagetype],
                     capdamagemodifier[profile][damagetype][0]&DAMAGESHIFT,
                     capdamagemodifier[profile][damagetype][1]&DAMAGESHIFT,
                     capdamagemodifier[profile][damagetype][2]&DAMAGESHIFT,
                     capdamagemodifier[profile][damagetype][3]&DAMAGESHIFT );
            damagetype++;
        }

        damagetype = 0;

        while ( damagetype < DAMAGE_COUNT )
        {
            skin = 0;

            while ( skin < MAXSKIN )
            {
                codes[skin] = 'F';
                if ( capdamagemodifier[profile][damagetype][skin]&DAMAGEINVERT )
                    codes[skin] = 'T';
                if ( capdamagemodifier[profile][damagetype][skin]&DAMAGECHARGE )
                    codes[skin] = 'C';

                skin++;
            }

            fprintf( filewrite, "%c damage code  : %3c %3c %3c %3c\n", types[damagetype], codes[0], codes[1], codes[2], codes[3] );
            damagetype++;
        }

        fprintf( filewrite, "Acceleration   : %3.0f %3.0f %3.0f %3.0f\n", capmaxaccel[profile][0]*80,
                 capmaxaccel[profile][1]*80,
                 capmaxaccel[profile][2]*80,
                 capmaxaccel[profile][3]*80 );
        fprintf( filewrite, "\n" );

        // Experience and level data
        fprintf( filewrite, "EXP for 2nd    : %d\n", capexperienceforlevel[profile][1] );
        fprintf( filewrite, "EXP for 3rd    : %d\n", capexperienceforlevel[profile][2] );
        fprintf( filewrite, "EXP for 4th    : %d\n", capexperienceforlevel[profile][3] );
        fprintf( filewrite, "EXP for 5th    : %d\n", capexperienceforlevel[profile][4] );
        fprintf( filewrite, "EXP for 6th    : %d\n", capexperienceforlevel[profile][5] );
        fprintf( filewrite, "Starting EXP   : %d\n", chr[character].experience );
        fprintf( filewrite, "EXP worth      : %d\n", capexperienceworth[profile] );
        fprintf( filewrite, "EXP exchange   : %5.3f\n", capexperienceexchange[profile] );
        fprintf( filewrite, "EXPSECRET      : %4.2f\n", capexperiencerate[profile][0] );
        fprintf( filewrite, "EXPQUEST       : %4.2f\n", capexperiencerate[profile][1] );
        fprintf( filewrite, "EXPDARE        : %4.2f\n", capexperiencerate[profile][2] );
        fprintf( filewrite, "EXPKILL        : %4.2f\n", capexperiencerate[profile][3] );
        fprintf( filewrite, "EXPMURDER      : %4.2f\n", capexperiencerate[profile][4] );
        fprintf( filewrite, "EXPREVENGE     : %4.2f\n", capexperiencerate[profile][5] );
        fprintf( filewrite, "EXPTEAMWORK    : %4.2f\n", capexperiencerate[profile][6] );
        fprintf( filewrite, "EXPROLEPLAY    : %4.2f\n", capexperiencerate[profile][7] );
        fprintf( filewrite, "\n" );

        // IDSZ identification tags
        undo_idsz( capidsz[profile][IDSZ_PARENT] );
        fprintf( filewrite, "IDSZ Parent    : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_TYPE] );
        fprintf( filewrite, "IDSZ Type      : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_SKILL] );
        fprintf( filewrite, "IDSZ Skill     : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_SPECIAL] );
        fprintf( filewrite, "IDSZ Special   : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_HATE] );
        fprintf( filewrite, "IDSZ Hate      : [%s]\n", idsz_string );
        undo_idsz( capidsz[profile][IDSZ_VULNERABILITY] );
        fprintf( filewrite, "IDSZ Vulnie    : [%s]\n", idsz_string );
        fprintf( filewrite, "\n" );

        // Item and damage flags
        ftruthf( filewrite, "Is an item     : ", capisitem[profile] );
        ftruthf( filewrite, "Is a mount     : ", capismount[profile] );
        ftruthf( filewrite, "Is stackable   : ", capisstackable[profile] );
        ftruthf( filewrite, "Name known     : ", chr[character].nameknown );
        ftruthf( filewrite, "Usage known    : ", capusageknown[profile] );
        ftruthf( filewrite, "Is exportable  : ", capcancarrytonextmodule[profile] );
        ftruthf( filewrite, "Requires skill : ", capneedskillidtouse[profile] );
        ftruthf( filewrite, "Is platform    : ", capplatform[profile] );
        ftruthf( filewrite, "Collects money : ", capcangrabmoney[profile] );
        ftruthf( filewrite, "Can open stuff : ", capcanopenstuff[profile] );
        fprintf( filewrite, "\n" );

        // Other item and damage stuff
        fdamagf( filewrite, "Damage type    : ", capdamagetargettype[profile] );
        factiof( filewrite, "Attack type    : ", capweaponaction[profile] );
        fprintf( filewrite, "\n" );

        // Particle attachments
        fprintf( filewrite, "Attached parts : %d\n", capattachedprtamount[profile] );
        fdamagf( filewrite, "Reaffirm type  : ", capattachedprtreaffirmdamagetype[profile] );
        fprintf( filewrite, "Particle type  : %d\n", capattachedprttype[profile] );
        fprintf( filewrite, "\n" );

        // Character hands
        ftruthf( filewrite, "Left valid     : ", capgripvalid[profile][0] );
        ftruthf( filewrite, "Right valid    : ", capgripvalid[profile][1] );
        fprintf( filewrite, "\n" );

        // Particle spawning on attack
        ftruthf( filewrite, "Part on weapon : ", capattackattached[profile] );
        fprintf( filewrite, "Part type      : %d\n", capattackprttype[profile] );
        fprintf( filewrite, "\n" );

        // Particle spawning for GoPoof
        fprintf( filewrite, "Poof amount    : %d\n", capgopoofprtamount[profile] );
        fprintf( filewrite, "Facing add     : %d\n", capgopoofprtfacingadd[profile] );
        fprintf( filewrite, "Part type      : %d\n", capgopoofprttype[profile] );
        fprintf( filewrite, "\n" );

        // Particle spawning for blud
        ftruthf( filewrite, "Blud valid    : ", capbludvalid[profile] );
        fprintf( filewrite, "Part type      : %d\n", capbludprttype[profile] );
        fprintf( filewrite, "\n" );

        // Extra stuff
        ftruthf( filewrite, "Waterwalking   : ", capwaterwalk[profile] );
        fprintf( filewrite, "Bounce dampen  : %5.3f\n", capdampen[profile] );
        fprintf( filewrite, "\n" );

        // More stuff
        fprintf( filewrite, "Life healing   : %5.3f\n", caplifeheal[profile] / 256.0f );
        fprintf( filewrite, "Mana cost      : %5.3f\n", capmanacost[profile] / 256.0f );
        fprintf( filewrite, "Life return    : %d\n", caplifereturn[profile] );
        fprintf( filewrite, "Stopped by     : %d\n", capstoppedby[profile] );
        funderf( filewrite, "Skin 0 name    : ", capskinname[profile][0] );
        funderf( filewrite, "Skin 1 name    : ", capskinname[profile][1] );
        funderf( filewrite, "Skin 2 name    : ", capskinname[profile][2] );
        funderf( filewrite, "Skin 3 name    : ", capskinname[profile][3] );
        fprintf( filewrite, "Skin 0 cost    : %d\n", capskincost[profile][0] );
        fprintf( filewrite, "Skin 1 cost    : %d\n", capskincost[profile][1] );
        fprintf( filewrite, "Skin 2 cost    : %d\n", capskincost[profile][2] );
        fprintf( filewrite, "Skin 3 cost    : %d\n", capskincost[profile][3] );
        fprintf( filewrite, "STR dampen     : %5.3f\n", capstrengthdampen[profile] );
        fprintf( filewrite, "\n" );

        // Another memory lapse
        ftruthf( filewrite, "No rider attak : ", btrue - capridercanattack[profile] );
        ftruthf( filewrite, "Can be dazed   : ", capcanbedazed[profile] );
        ftruthf( filewrite, "Can be grogged : ", capcanbegrogged[profile] );
        fprintf( filewrite, "NOT USED       : 0\n" );
        fprintf( filewrite, "NOT USED       : 0\n" );
        ftruthf( filewrite, "Can see invisi : ", capcanseeinvisible[profile] );
        fprintf( filewrite, "Kursed chance  : %d\n", chr[character].iskursed*100 );
        fprintf( filewrite, "Footfall sound : %d\n", capsoundindex[profile][SOUND_FOOTFALL] );
        fprintf( filewrite, "Jump sound     : %d\n", capsoundindex[profile][SOUND_JUMP] );
        fprintf( filewrite, "\n" );

        // Expansions
        if ( capskindressy[profile]&1 )
            fprintf( filewrite, ":[DRES] 0\n" );
        if ( capskindressy[profile]&2 )
            fprintf( filewrite, ":[DRES] 1\n" );
        if ( capskindressy[profile]&4 )
            fprintf( filewrite, ":[DRES] 2\n" );
        if ( capskindressy[profile]&8 )
            fprintf( filewrite, ":[DRES] 3\n" );
        if ( capresistbumpspawn[profile] )
            fprintf( filewrite, ":[STUK] 0\n" );
        if ( capistoobig[profile] )
            fprintf( filewrite, ":[PACK] 0\n" );
        if ( !capreflect[profile] )
            fprintf( filewrite, ":[VAMP] 1\n" );
        if ( capalwaysdraw[profile] )
            fprintf( filewrite, ":[DRAW] 1\n" );
        if ( capisranged[profile] )
            fprintf( filewrite, ":[RANG] 1\n" );
        if ( caphidestate[profile] != NOHIDE )
            fprintf( filewrite, ":[HIDE] %d\n", caphidestate[profile] );
        if ( capisequipment[profile] )
            fprintf( filewrite, ":[EQUI] 1\n" );
        if ( capbumpsizebig[profile] == ( capbumpsize[profile] << 1 ) )
            fprintf( filewrite, ":[SQUA] 1\n" );
        if ( capicon[profile] != capusageknown[profile] )
            fprintf( filewrite, ":[ICON] %d\n", capicon[profile] );
        if ( capforceshadow[profile] )
            fprintf( filewrite, ":[SHAD] 1\n" );
        if ( capripple[profile] == capisitem[profile] )
            fprintf( filewrite, ":[RIPP] %d\n", capripple[profile] );
        if ( capisvaluable[profile] != -1 )
            fprintf( filewrite, ":[VALU] %d\n", capisvaluable[profile] );

        //Basic stuff that is always written
        fprintf( filewrite, ":[GOLD] %d\n", chr[character].money );
        fprintf( filewrite, ":[PLAT] %d\n", capcanuseplatforms[profile] );
        fprintf( filewrite, ":[SKIN] %d\n", chr[character].skin );
        fprintf( filewrite, ":[CONT] %d\n", chr[character].ai.content );
        fprintf( filewrite, ":[STAT] %d\n", chr[character].ai.state );
        fprintf( filewrite, ":[LEVL] %d\n", chr[character].experiencelevel );

        //Copy all skill expansions
        fprintf( filewrite, ":[SHPR] %d\n", chr[character].shieldproficiency );
        if ( chr[character].canuseadvancedweapons )
            fprintf( filewrite, ":[AWEP] 1\n" );
        if ( chr[character].canjoust )
            fprintf( filewrite, ":[JOUS] 1\n" );
        if ( chr[character].candisarm )
            fprintf( filewrite, ":[DISA] 1\n" );
        if ( capcanseekurse[profile] )
            fprintf( filewrite, ":[CKUR] 1\n" );
        if ( chr[character].canusepoison )
            fprintf( filewrite, ":[POIS] 1\n" );
        if ( chr[character].canread )
            fprintf( filewrite, ":[READ] 1\n" );
        if ( chr[character].canbackstab )
            fprintf( filewrite, ":[STAB] 1\n" );
        if ( chr[character].canusedivine )
            fprintf( filewrite, ":[HMAG] 1\n" );
        if ( chr[character].canusearcane )
            fprintf( filewrite, ":[WMAG] 1\n" );
        if ( chr[character].canusetech )
            fprintf( filewrite, ":[TECH] 1\n" );

        //The end
        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin(  const char *szSaveName, Uint16 character )
{
    // ZZ> This function creates a skin.txt file for the given character.
    FILE* filewrite;
    int profile;

    // General stuff
    profile = chr[character].model;

    // Open the file
    filewrite = fopen( szSaveName, "w" );
    if ( filewrite )
    {
        fprintf( filewrite, "//This file is used only by the import menu\n" );
        fprintf( filewrite, ": %d\n", chr[character].skin );
        fclose( filewrite );
    }
}

//--------------------------------------------------------------------------------------------
int load_one_character_profile(  const char *szLoadName )
{
    // ZZ> This function fills a character profile with data from data.txt, returning
    // the object slot that the profile was stuck into.  It may cause the program
    // to abort if bad things happen.
    FILE* fileread;
    Sint16 object = -1;
    int iTmp;
    float fTmp;
    char cTmp;
    Uint8 damagetype, level, xptype;
    int idsz_cnt;
    IDSZ idsz, test;

    // Open the file
    fileread = fopen( szLoadName, "r" );
    if ( fileread != NULL )
    {
        parse_filename = szLoadName;  //For debugging goto_colon()

        // Read in the object slot
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp ); object = iTmp;
        if ( object < 0 )
        {
            if ( importobject < 0 )
            {
                log_warning( "Object slot number cannot be negative (%s)\n", szLoadName );
            }
            else
            {
                object = importobject;
            }
        }

        // Make sure global objects don't load over existing models
        if ( madused[object] )
        {
            if ( object == SPELLBOOK ) log_error( "Object slot %i is a special reserved slot number (cannot be used by %s).\n", SPELLBOOK, szLoadName );
            else if (overrideslots)  log_error( "Object slot %i used twice (%s)\n", object, szLoadName );
            else return -1;   //Stop, we don't want to override it
        }

        madused[object] = btrue;

        // clear out the sounds
        for ( iTmp = 0; iTmp < SOUND_COUNT; iTmp++ )
        {
            capsoundindex[object][iTmp] = -1;
        }

        // Read in the real general data
        goto_colon( fileread );  get_name( fileread, capclassname[object] );

        // Light cheat
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capuniformlit[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' || GL_FLAT == shading )  capuniformlit[object] = btrue;

        // Ammo
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capammomax[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capammo[object] = iTmp;
        // Gender
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capgender[object] = GENOTHER;
        if ( cTmp == 'F' || cTmp == 'f' )  capgender[object] = GENFEMALE;
        if ( cTmp == 'M' || cTmp == 'm' )  capgender[object] = GENMALE;
        if ( cTmp == 'R' || cTmp == 'r' )  capgender[object] = GENRANDOM;

        // Read in the object stats
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caplifecolor[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capmanacolor[object] = iTmp;
        goto_colon( fileread );  read_pair( fileread );
        caplifebase[object] = pairbase;  capliferand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        caplifeperlevelbase[object] = pairbase;  caplifeperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanabase[object] = pairbase;  capmanarand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanaperlevelbase[object] = pairbase;  capmanaperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanareturnbase[object] = pairbase;  capmanareturnrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanareturnperlevelbase[object] = pairbase;  capmanareturnperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanaflowbase[object] = pairbase;  capmanaflowrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capmanaflowperlevelbase[object] = pairbase;  capmanaflowperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capstrengthbase[object] = pairbase;  capstrengthrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capstrengthperlevelbase[object] = pairbase;  capstrengthperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capwisdombase[object] = pairbase;  capwisdomrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capwisdomperlevelbase[object] = pairbase;  capwisdomperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capintelligencebase[object] = pairbase;  capintelligencerand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capintelligenceperlevelbase[object] = pairbase;  capintelligenceperlevelrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capdexteritybase[object] = pairbase;  capdexterityrand[object] = pairrand;
        goto_colon( fileread );  read_pair( fileread );
        capdexterityperlevelbase[object] = pairbase;  capdexterityperlevelrand[object] = pairrand;

        // More physical attributes
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capsize[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capsizeperlevel[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capshadowsize[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capbumpsize[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capbumpheight[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capbumpdampen[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capweight[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capjump[object] = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capjumpnumber[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capsneakspd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capwalkspd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caprunspd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capflyheight[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capflashand[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capalpha[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caplight[object] = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        captransferblend[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  captransferblend[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capsheen[object] = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capenviro[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capenviro[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capuoffvel[object] = fTmp * 0xFFFF;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capvoffvel[object] = fTmp * 0xFFFF;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capstickybutt[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capstickybutt[object] = btrue;

        // Invulnerability data
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capinvictus[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capinvictus[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capnframefacing[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capnframeangle[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capiframefacing[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capiframeangle[object] = iTmp;

        // Resist burning and stuck arrows with nframe angle of 1 or more
        if ( capnframeangle[object] > 0 )
        {
            if ( capnframeangle[object] == 1 )
            {
                capnframeangle[object] = 0;
            }
        }

        // Skin defenses ( 4 skins )
        goto_colon( fileread );
        fscanf( fileread, "%d", &iTmp );  capdefense[object][0] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  capdefense[object][1] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  capdefense[object][2] = 255 - iTmp;
        fscanf( fileread, "%d", &iTmp );  capdefense[object][3] = 255 - iTmp;

        for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
        {
            goto_colon( fileread );
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][0] = iTmp;
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][1] = iTmp;
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][2] = iTmp;
            fscanf( fileread, "%d", &iTmp );  capdamagemodifier[object][damagetype][3] = iTmp;
        }

        for ( damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++ )
        {
            goto_colon( fileread );

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][0] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][0] |= DAMAGECHARGE;

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][1] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][1] |= DAMAGECHARGE;

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][2] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][2] |= DAMAGECHARGE;

            cTmp = get_first_letter( fileread );  if ( cTmp == 'T' || cTmp == 't' )  capdamagemodifier[object][damagetype][3] |= DAMAGEINVERT;
            if ( cTmp == 'C' || cTmp == 'c' )  capdamagemodifier[object][damagetype][3] |= DAMAGECHARGE;
        }

        goto_colon( fileread );
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][0] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][1] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][2] = fTmp / 80.0f;
        fscanf( fileread, "%f", &fTmp );  capmaxaccel[object][3] = fTmp / 80.0f;

        // Experience and level data
        capexperienceforlevel[object][0] = 0;

        for ( level = 1; level < MAXLEVEL; level++ )
        {
            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capexperienceforlevel[object][level] = iTmp;
        }

        goto_colon( fileread );  read_pair( fileread );
        pairbase = pairbase >> 8;
        pairrand = pairrand >> 8;
        if ( pairrand < 1 )  pairrand = 1;

        capexperiencebase[object] = pairbase;
        capexperiencerand[object] = pairrand;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capexperienceworth[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capexperienceexchange[object] = fTmp;

        for ( xptype = 0; xptype < XP_COUNT; xptype++ )
        {
            goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capexperiencerate[object][xptype] = fTmp + 0.001f;
        }

        // IDSZ tags
        for ( idsz_cnt = 0; idsz_cnt < IDSZ_COUNT; idsz_cnt++ )
        {
            goto_colon( fileread );  iTmp = get_idsz( fileread );  capidsz[object][idsz_cnt] = iTmp;
        }

        // Item and damage flags
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capisitem[object] = bfalse;  capripple[object] = btrue;
        if ( cTmp == 'T' || cTmp == 't' )  { capisitem[object] = btrue; capripple[object] = bfalse; }

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capismount[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capismount[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capisstackable[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capisstackable[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capnameknown[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capnameknown[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capusageknown[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capusageknown[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capcancarrytonextmodule[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capcancarrytonextmodule[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capneedskillidtouse[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capneedskillidtouse[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capplatform[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capplatform[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capcangrabmoney[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capcangrabmoney[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capcanopenstuff[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capcanopenstuff[object] = btrue;

        // More item and damage stuff
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        if ( cTmp == 'S' || cTmp == 's' )  capdamagetargettype[object] = DAMAGE_SLASH;
        if ( cTmp == 'C' || cTmp == 'c' )  capdamagetargettype[object] = DAMAGE_CRUSH;
        if ( cTmp == 'P' || cTmp == 'p' )  capdamagetargettype[object] = DAMAGE_POKE;
        if ( cTmp == 'H' || cTmp == 'h' )  capdamagetargettype[object] = DAMAGE_HOLY;
        if ( cTmp == 'E' || cTmp == 'e' )  capdamagetargettype[object] = DAMAGE_EVIL;
        if ( cTmp == 'F' || cTmp == 'f' )  capdamagetargettype[object] = DAMAGE_FIRE;
        if ( cTmp == 'I' || cTmp == 'i' )  capdamagetargettype[object] = DAMAGE_ICE;
        if ( cTmp == 'Z' || cTmp == 'z' )  capdamagetargettype[object] = DAMAGE_ZAP;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capweaponaction[object] = what_action( cTmp );

        // Particle attachments
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capattachedprtamount[object] = iTmp;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        if ( cTmp == 'N' || cTmp == 'n' )  capattachedprtreaffirmdamagetype[object] = DAMAGENULL;
        if ( cTmp == 'S' || cTmp == 's' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_SLASH;
        if ( cTmp == 'C' || cTmp == 'c' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_CRUSH;
        if ( cTmp == 'P' || cTmp == 'p' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_POKE;
        if ( cTmp == 'H' || cTmp == 'h' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_HOLY;
        if ( cTmp == 'E' || cTmp == 'e' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_EVIL;
        if ( cTmp == 'F' || cTmp == 'f' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_FIRE;
        if ( cTmp == 'I' || cTmp == 'i' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_ICE;
        if ( cTmp == 'Z' || cTmp == 'z' )  capattachedprtreaffirmdamagetype[object] = DAMAGE_ZAP;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capattachedprttype[object] = iTmp;

        // Character hands
        capgripvalid[object][0] = bfalse;
        capgripvalid[object][1] = bfalse;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  capgripvalid[object][0] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        if ( cTmp == 'T' || cTmp == 't' )  capgripvalid[object][1] = btrue;

        // Attack order ( weapon )
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capattackattached[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capattackattached[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capattackprttype[object] = iTmp;

        // GoPoof
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capgopoofprtamount[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capgopoofprtfacingadd[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capgopoofprttype[object] = iTmp;

        // Blud
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capbludvalid[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capbludvalid[object] = btrue;
        if ( cTmp == 'U' || cTmp == 'u' )  capbludvalid[object] = ULTRABLUDY;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capbludprttype[object] = iTmp;

        // Stuff I forgot
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capwaterwalk[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capwaterwalk[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capdampen[object] = fTmp;

        // More stuff I forgot
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  caplifeheal[object] = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capmanacost[object] = fTmp * 256;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  caplifereturn[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capstoppedby[object] = iTmp | MESHFX_IMPASS;
        goto_colon( fileread );  get_name( fileread, capskinname[object][0] );
        goto_colon( fileread );  get_name( fileread, capskinname[object][1] );
        goto_colon( fileread );  get_name( fileread, capskinname[object][2] );
        goto_colon( fileread );  get_name( fileread, capskinname[object][3] );
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][0] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][1] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][2] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  capskincost[object][3] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  capstrengthdampen[object] = fTmp;

        // Another memory lapse
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        capridercanattack[object] = btrue;
        if ( cTmp == 'T' || cTmp == 't' )  capridercanattack[object] = bfalse;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );  // Can be dazed
        capcanbedazed[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capcanbedazed[object] = btrue;

        goto_colon( fileread );  cTmp = get_first_letter( fileread );  // Can be grogged
        capcanbegrogged[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capcanbegrogged[object] = btrue;

        goto_colon( fileread );  // !!!BAD!!! Life add
        goto_colon( fileread );  // !!!BAD!!! Mana add
        goto_colon( fileread );  cTmp = get_first_letter( fileread );  // Can see invisible
        capcanseeinvisible[object] = bfalse;
        if ( cTmp == 'T' || cTmp == 't' )  capcanseeinvisible[object] = btrue;

        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Chance of kursed
        capkursechance[object] = iTmp;
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Footfall sound
        capsoundindex[object][SOUND_FOOTFALL] = CLIP(iTmp, -1, MAXWAVE);
        goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  // Jump sound
        capsoundindex[object][SOUND_JUMP] = CLIP(iTmp, -1, MAXWAVE);

        // Clear expansions...
        capskindressy[object] = bfalse;
        capresistbumpspawn[object] = bfalse;
        capistoobig[object] = bfalse;
        capreflect[object] = btrue;
        capalwaysdraw[object] = bfalse;
        capisranged[object] = bfalse;
        caphidestate[object] = NOHIDE;
        capisequipment[object] = bfalse;
        capbumpsizebig[object] = capbumpsize[object] + ( capbumpsize[object] >> 1 );
        capcanseekurse[object] = bfalse;
        capmoney[object] = 0;
        capicon[object] = capusageknown[object];
        capforceshadow[object] = bfalse;
        capskinoverride[object] = NOSKINOVERRIDE;
        capcontentoverride[object] = 0;
        capstateoverride[object] = 0;
        capleveloverride[object] = 0;
        capcanuseplatforms[object] = !capplatform[object];
        capisvaluable[object] = 0;

        //Skills
        capcanuseadvancedweapons[object] = 0;
        capcanjoust[object] = 0;
        capcanusetech[object] = 0;
        capcanusedivine[object] = 0;
        capcanusearcane[object] = 0;
        capshieldproficiency[object] = 0;
        capcandisarm[object] = 0;
        capcanbackstab[object] = 0;
        capcanusepoison[object] = 0;
        capcanread[object] = 0;

        // Read expansions
        while ( goto_colon_yesno( fileread ) )
        {
            idsz = get_idsz( fileread );
            fscanf( fileread, "%c%d", &cTmp, &iTmp );

            test = Make_IDSZ( "DRES" );  // [DRES]
            if ( idsz == test )  capskindressy[object] |= 1 << iTmp;

            test = Make_IDSZ( "GOLD" );  // [GOLD]
            if ( idsz == test )  capmoney[object] = iTmp;

            test = Make_IDSZ( "STUK" );  // [STUK]
            if ( idsz == test )  capresistbumpspawn[object] = 1 - iTmp;

            test = Make_IDSZ( "PACK" );  // [PACK]
            if ( idsz == test )  capistoobig[object] = 1 - iTmp;

            test = Make_IDSZ( "VAMP" );  // [VAMP]
            if ( idsz == test )  capreflect[object] = 1 - iTmp;

            test = Make_IDSZ( "DRAW" );  // [DRAW]
            if ( idsz == test )  capalwaysdraw[object] = iTmp;

            test = Make_IDSZ( "RANG" );  // [RANG]
            if ( idsz == test )  capisranged[object] = iTmp;

            test = Make_IDSZ( "HIDE" );  // [HIDE]
            if ( idsz == test )  caphidestate[object] = iTmp;

            test = Make_IDSZ( "EQUI");  // [EQUI]
            if ( idsz == test )  capisequipment[object] = iTmp;

            test = Make_IDSZ( "SQUA");  // [SQUA]
            if ( idsz == test )  capbumpsizebig[object] = capbumpsize[object] << 1;

            test = Make_IDSZ( "ICON" );  // [ICON]
            if ( idsz == test )  capicon[object] = iTmp;

            test = Make_IDSZ( "SHAD" );  // [SHAD]
            if ( idsz == test )  capforceshadow[object] = iTmp;

            test = Make_IDSZ( "CKUR" );  // [CKUR]
            if ( idsz == test )  capcanseekurse[object] = iTmp;

            test = Make_IDSZ( "SKIN" );  // [SKIN]
            if ( idsz == test )  capskinoverride[object] = iTmp & 3;

            test = Make_IDSZ( "CONT" );  // [CONT]
            if ( idsz == test )  capcontentoverride[object] = iTmp;

            test = Make_IDSZ( "STAT" );  // [STAT]
            if ( idsz == test )  capstateoverride[object] = iTmp;

            test = Make_IDSZ( "LEVL" );  // [LEVL]
            if ( idsz == test )  capleveloverride[object] = iTmp;

            test = Make_IDSZ( "PLAT" );  // [PLAT]
            if ( idsz == test )  capcanuseplatforms[object] = iTmp;

            test = Make_IDSZ( "RIPP" );  // [RIPP]
            if ( idsz == test )  capripple[object] = iTmp;

            test = Make_IDSZ( "VALU" );  // [VALU]
            if ( idsz == test ) capisvaluable[object] = iTmp;

            //Read Skills
            test = Make_IDSZ( "AWEP" );  // [AWEP]
            if ( idsz == test )  capcanuseadvancedweapons[object] = iTmp;

            test = Make_IDSZ( "SHPR" );  // [SHPR]
            if ( idsz == test )  capshieldproficiency[object] = iTmp;

            test = Make_IDSZ( "JOUS" );  // [JOUS]
            if ( idsz == test )  capcanjoust[object] = iTmp;

            test = Make_IDSZ( "WMAG" );  // [WMAG]
            if ( idsz == test )  capcanusearcane[object] = iTmp;

            test = Make_IDSZ( "HMAG" );  // [HMAG]
            if ( idsz == test )  capcanusedivine[object] = iTmp;

            test = Make_IDSZ( "TECH" );  // [TECH]
            if ( idsz == test )  capcanusetech[object] = iTmp;

            test = Make_IDSZ( "DISA" );  // [DISA]
            if ( idsz == test )  capcandisarm[object] = iTmp;

            test = Make_IDSZ( "STAB" );  // [STAB]
            if ( idsz == test )  capcanbackstab[object] = iTmp;

            test = Make_IDSZ( "POIS" );  // [POIS]
            if ( idsz == test )  capcanusepoison[object] = iTmp;

            test = Make_IDSZ( "READ" );  // [READ]
            if ( idsz == test )  capcanread[object] = iTmp;
        }

        fclose( fileread );
    }
    else
    {
        // The data file wasn't found
        log_error( "DATA.TXT was not found! (%s)\n", szLoadName );
    }

    return object;
}

//--------------------------------------------------------------------------------------------
int get_skin(  const char *filename )
{
    // ZZ> This function reads the skin.txt file...
    FILE*   fileread;
    int skin;

    skin = 0;
    fileread = fopen( filename, "r" );
    if ( fileread )
    {
        goto_colon_yesno( fileread );
        fscanf( fileread, "%d", &skin );
        skin %= MAXSKIN;
        fclose( fileread );
    }

    return skin;
}

//--------------------------------------------------------------------------------------------
void check_player_import(  const char *dirname, bool_t initialize )
{
    // ZZ> This function figures out which players may be imported, and loads basic
    //     data for each
    char searchname[128];
    STRING filename;
    int skin;
    const char *foundfile;

    if ( initialize )
    {
        // restart from nothing
        loadplayer_count = 0;
        globalicon_count = 0;
    };

    // Search for all objects
    sprintf( searchname, "%s" SLASH_STR "*.obj", dirname );
    foundfile = fs_findFirstFile( dirname, "obj" );

    while ( NULL != foundfile && loadplayer_count < MAXLOADPLAYER )
    {
        prime_names();
        sprintf( loadplayer[loadplayer_count].dir, "%s", foundfile );

        sprintf( filename, "%s" SLASH_STR "%s" SLASH_STR "skin.txt", dirname, foundfile );
        skin = get_skin( filename );

        snprintf( filename, sizeof(filename), "%s" SLASH_STR "%s" SLASH_STR "tris.md2", dirname, foundfile );
        load_one_md2( filename, loadplayer_count );

        sprintf( filename, "%s" SLASH_STR "%s" SLASH_STR "icon%d", dirname, foundfile, skin );
        load_one_icon( filename );

        sprintf( filename, "%s" SLASH_STR "%s" SLASH_STR "naming.txt", dirname, foundfile );
        read_naming( 0, filename );
        naming_names( 0 );
        sprintf( loadplayer[loadplayer_count].name, "%s", namingnames );

        loadplayer_count++;

        foundfile = fs_findNextFile();
    }

    fs_findClose();
}

//--------------------------------------------------------------------------------------------
void damage_character( Uint16 character, Uint16 direction,
                       int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                       Uint16 attacker, Uint16 effects, bool_t ignoreinvincible )
{
    // ZZ> This function calculates and applies damage to a character.  It also
    //     sets alerts and begins actions.  Blocking and frame invincibility
    //     are done here too.  Direction is 0 if the attack is coming head on,
    //     16384 if from the right, 32768 if from the back, 49152 if from the
    //     left.

    int tnc;
    Uint16 action;
    int damage, basedamage;
    Uint16 experience, model, left, right;
    if ( chr[character].alive && damagebase >= 0 && damagerand >= 1 )
    {
        // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
        // This can also be used to lessen effectiveness of healing
        damage = damagebase + ( rand() % damagerand );
        basedamage = damage;
        damage = damage >> ( chr[character].damagemodifier[damagetype] & DAMAGESHIFT );

        // Allow charging (Invert damage to mana)
        if ( chr[character].damagemodifier[damagetype]&DAMAGECHARGE )
        {
            chr[character].mana += damage;
            if ( chr[character].mana > chr[character].manamax )
            {
                chr[character].mana = chr[character].manamax;
            }

            return;
        }

        // Invert damage to heal
        if ( chr[character].damagemodifier[damagetype]&DAMAGEINVERT )
            damage = -damage;

        // Remember the damage type
        chr[character].ai.damagetypelast = damagetype;
        chr[character].ai.directionlast = direction;

        // Do it already
        if ( damage > 0 )
        {
            // Only damage if not invincible
            if ( 0 == chr[character].damagetime && ( !chr[character].invictus || ignoreinvincible ) )
            {
                model = chr[character].model;

                if ( 0 == ( effects&DAMFXBLOC ) )
                {
                    // Only damage if hitting from proper direction
                    if ( madframefx[chr[character].frame]&MADFXINVICTUS )
                    {
                        // I Frame...
                        direction -= capiframefacing[model];
                        left = ( ~capiframeangle[model] );
                        right = capiframeangle[model];

                        // Check for shield
                        if ( chr[character].action >= ACTIONPA && chr[character].action <= ACTIONPD )
                        {
                            // Using a shield?
                            if ( chr[character].action < ACTIONPC )
                            {
                                // Check left hand
                                if ( chr[character].holdingwhich[0] != MAXCHR )
                                {
                                    left = ( ~capiframeangle[chr[chr[character].holdingwhich[0]].model] );
                                    right = capiframeangle[chr[chr[character].holdingwhich[0]].model];
                                }
                            }
                            else
                            {
                                // Check right hand
                                if ( chr[character].holdingwhich[1] != MAXCHR )
                                {
                                    left = ( ~capiframeangle[chr[chr[character].holdingwhich[1]].model] );
                                    right = capiframeangle[chr[chr[character].holdingwhich[1]].model];
                                }
                            }
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
                    if ( direction > left || direction < right )
                    {
                        damage = 0;
                    }
                }

                if ( damage != 0 )
                {
                    if ( effects&DAMFXARMO )
                    {
                        chr[character].life -= damage;
                    }
                    else
                    {
                        chr[character].life -= FP8_MUL( damage, chr[character].defense );
                    }
                    if ( basedamage > HURTDAMAGE )
                    {
                        // Call for help if below 1/2 life
                        /*if(chr[character].life < (chr[character].lifemax>>1))
                            call_for_help(character);*/

                        // Spawn blud particles
                        if ( capbludvalid[model] && ( damagetype < DAMAGE_HOLY || capbludvalid[model] == ULTRABLUDY ) )
                        {
                            spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos,
                                                chr[character].turnleftright + direction, chr[character].model, capbludprttype[model],
                                                MAXCHR, GRIP_LAST, chr[character].team, character, 0, MAXCHR );
                        }

                        // Set attack alert if it wasn't an accident
                        if ( team == DAMAGETEAM )
                        {
                            chr[character].ai.attacklast = MAXCHR;
                        }
                        else
                        {
                            // Don't alert the character too much if under constant fire
                            if ( chr[character].carefultime == 0 )
                            {
                                // Don't let characters chase themselves...  That would be silly
                                if ( attacker != character )
                                {
                                    chr[character].ai.alert |= ALERTIF_ATTACKED;
                                    chr[character].ai.attacklast = attacker;
                                    chr[character].carefultime = CAREFULTIME;
                                }
                            }
                        }
                    }

                    // Taking damage action
                    action = ACTIONHA;
                    if ( chr[character].life < 0 )
                    {
                        // Character has died
                        chr[character].alive = bfalse;
                        disenchant_character( character );
                        chr[character].waskilled = btrue;
                        chr[character].keepaction = btrue;
                        chr[character].life = -1;
                        chr[character].platform = btrue;
                        chr[character].bumpdampen = chr[character].bumpdampen / 2.0f;
                        action = ACTIONKA;
                        // Give kill experience
                        experience = capexperienceworth[model] + ( chr[character].experience * capexperienceexchange[model] );
                        if ( attacker < MAXCHR )
                        {
                            // Set target
                            chr[character].ai.target = attacker;
                            if ( team == DAMAGETEAM )  chr[character].ai.target = character;
                            if ( team == NULLTEAM )  chr[character].ai.target = character;

                            // Award direct kill experience
                            if ( teamhatesteam[chr[attacker].team][chr[character].team] )
                            {
                                give_experience( attacker, experience, XP_KILLENEMY, bfalse );
                            }

                            // Check for hated
                            if ( capidsz[chr[attacker].model][IDSZ_HATE] == capidsz[model][IDSZ_PARENT] ||
                                    capidsz[chr[attacker].model][IDSZ_HATE] == capidsz[model][IDSZ_TYPE] )
                            {
                                give_experience( attacker, experience, XP_KILLHATED, bfalse );
                            }
                        }

                        // Clear all shop passages that it owned...
                        tnc = 0;

                        while ( tnc < numshoppassage )
                        {
                            if ( shopowner[tnc] == character )
                            {
                                shopowner[tnc] = NOOWNER;
                            }

                            tnc++;
                        }

                        // Let the other characters know it died
                        tnc = 0;

                        while ( tnc < MAXCHR )
                        {
                            if ( chr[tnc].on && chr[tnc].alive )
                            {
                                if ( chr[tnc].ai.target == character )
                                {
                                    chr[tnc].ai.alert |= ALERTIF_TARGETKILLED;
                                }
                                if ( !teamhatesteam[chr[tnc].team][team] && ( teamhatesteam[chr[tnc].team][chr[character].team] ) )
                                {
                                    // All allies get team experience, but only if they also hate the dead guy's team
                                    give_experience( tnc, experience, XP_TEAMKILL, bfalse );
                                }
                            }

                            tnc++;
                        }

                        // Check if it was a leader
                        if ( teamleader[chr[character].team] == character )
                        {
                            // It was a leader, so set more alerts
                            tnc = 0;

                            while ( tnc < MAXCHR )
                            {
                                if ( chr[tnc].on && chr[tnc].team == chr[character].team )
                                {
                                    // All folks on the leaders team get the alert
                                    chr[tnc].ai.alert |= ALERTIF_LEADERKILLED;
                                }

                                tnc++;
                            }

                            // The team now has no leader
                            teamleader[chr[character].team] = NOLEADER;
                        }

                        detach_character_from_mount( character, btrue, bfalse );

                        //Play the death animation
                        action += ( rand() & 3 );
                        play_action( character, action, bfalse );

                        //If it's a player, let it die properly before enabling respawn
                        if (chr[character].isplayer) revivetimer = ONESECOND; //1 second

                        // Afford it one last thought if it's an AI
                        teammorale[chr[character].baseteam]--;
                        chr[character].team = chr[character].baseteam;
                        chr[character].ai.alert |= ALERTIF_KILLED;
                        chr[character].sparkle = NOSPARKLE;
                        chr[character].ai.timer = frame_wld + 1;  // No timeout...

                        let_character_think( character );
                    }
                    else
                    {
                        if ( basedamage > HURTDAMAGE )
                        {
                            action += ( rand() & 3 );
                            play_action( character, action, bfalse );

                            // Make the character invincible for a limited time only
                            if ( !( effects & DAMFXTIME ) )
                                chr[character].damagetime = DAMAGETIME;
                        }
                    }
                }
                else
                {
                    // Spawn a defend particle
                    spawn_one_particle( chr[character].xpos, chr[character].ypos, chr[character].zpos, chr[character].turnleftright, MAXMODEL, DEFEND, MAXCHR, GRIP_LAST, NULLTEAM, MAXCHR, 0, MAXCHR );
                    chr[character].damagetime    = DEFENDTIME;
                    chr[character].ai.alert     |= ALERTIF_BLOCKED;
                    chr[character].ai.attacklast = attacker;     // For the ones attacking a shield
                }
            }
        }
        else if ( damage < 0 )
        {
            chr[character].life -= damage;
            if ( chr[character].life > chr[character].lifemax )  chr[character].life = chr[character].lifemax;

            // Isssue an alert
            chr[character].ai.alert |= ALERTIF_HEALED;
            chr[character].ai.attacklast = attacker;
            if ( team != DAMAGETEAM )
            {
                chr[character].ai.attacklast = MAXCHR;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void kill_character( Uint16 character, Uint16 killer )
{
    // ZZ> This function kills a character...  MAXCHR killer for accidental death
    Uint8 modifier;
    if ( chr[character].alive )
    {
        chr[character].damagetime = 0;
        chr[character].life = 1;
        modifier = chr[character].damagemodifier[DAMAGE_CRUSH];
        chr[character].damagemodifier[DAMAGE_CRUSH] = 1;
        if ( killer != MAXCHR )
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, chr[killer].team, killer, DAMFXARMO | DAMFXBLOC, btrue );
        }
        else
        {
            damage_character( character, 0, 512, 1, DAMAGE_CRUSH, DAMAGETEAM, chr[character].ai.bumplast, DAMFXARMO | DAMFXBLOC, btrue );
        }

        chr[character].damagemodifier[DAMAGE_CRUSH] = modifier;
    }
}

//--------------------------------------------------------------------------------------------
void spawn_poof( Uint16 character, Uint16 profile )
{
    // ZZ> This function spawns a character poof
    Uint16 sTmp;
    Uint16 origin;
    int iTmp;

    sTmp = chr[character].turnleftright;
    iTmp = 0;
    origin = chr[character].ai.owner;

    while ( iTmp < capgopoofprtamount[profile] )
    {
        spawn_one_particle( chr[character].oldx, chr[character].oldy, chr[character].oldz,
                            sTmp, profile, capgopoofprttype[profile],
                            MAXCHR, GRIP_LAST, chr[character].team, origin, iTmp, MAXCHR );
        sTmp += capgopoofprtfacingadd[profile];
        iTmp++;
    }
}

//--------------------------------------------------------------------------------------------
void naming_names( Uint16 profile )
{
    // ZZ> This function generates a random name
    int read, write, section, mychop;
    char cTmp;
    if ( capsectionsize[profile][0] == 0 )
    {
        namingnames[0] = 'B';
        namingnames[1] = 'l';
        namingnames[2] = 'a';
        namingnames[3] = 'h';
        namingnames[4] = 0;
    }
    else
    {
        write = 0;
        section = 0;

        while ( section < MAXSECTION )
        {
            if ( capsectionsize[profile][section] != 0 )
            {
                mychop = capsectionstart[profile][section] + ( rand() % capsectionsize[profile][section] );
                read = chopstart[mychop];
                cTmp = chopdata[read];

                while ( cTmp != 0 && write < MAXCAPNAMESIZE - 1 )
                {
                    namingnames[write] = cTmp;
                    write++;
                    read++;
                    cTmp = chopdata[read];
                }
            }

            section++;
        }
        if ( write >= MAXCAPNAMESIZE ) write = MAXCAPNAMESIZE - 1;

        namingnames[write] = 0;
    }
}

//--------------------------------------------------------------------------------------------
void read_naming( Uint16 profile,  const char *szLoadname )
{
    // ZZ> This function reads a naming file
    FILE *fileread;
    int section, chopinsection, cnt;
    char mychop[32], cTmp;

    fileread = fopen( szLoadname, "r" );
    if ( fileread )
    {
        section = 0;
        chopinsection = 0;

        while ( goto_colon_yesno( fileread ) && section < MAXSECTION )
        {
            fscanf( fileread, "%s", mychop );
            if ( mychop[0] != 'S' || mychop[1] != 'T' || mychop[2] != 'O' || mychop[3] != 'P' )
            {
                if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;

                chopstart[numchop] = chopwrite;
                cnt = 0;
                cTmp = mychop[0];

                while ( cTmp != 0 && cnt < 31 && chopwrite < CHOPDATACHUNK )
                {
                    if ( cTmp == '_' ) cTmp = ' ';

                    chopdata[chopwrite] = cTmp;
                    cnt++;
                    chopwrite++;
                    cTmp = mychop[cnt];
                }
                if ( chopwrite >= CHOPDATACHUNK )  chopwrite = CHOPDATACHUNK - 1;

                chopdata[chopwrite] = 0;  chopwrite++;
                chopinsection++;
                numchop++;
            }
            else
            {
                capsectionsize[profile][section] = chopinsection;
                capsectionstart[profile][section] = numchop - chopinsection;
                section++;
                chopinsection = 0;
            }
        }

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
void prime_names()
{
    // ZZ> This function prepares the name chopper for use
    int cnt, tnc;

    numchop = 0;
    chopwrite = 0;
    cnt = 0;

    while ( cnt < MAXMODEL )
    {
        tnc = 0;

        while ( tnc < MAXSECTION )
        {
            capsectionstart[cnt][tnc] = MAXCHOP;
            capsectionsize[cnt][tnc] = 0;
            tnc++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
    // ZZ> This function sets all of the character's starting tilt values
    int cnt;
    Uint8 twist;

    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !chr[cnt].on ) continue;

        if ( chr[cnt].stickybutt && INVALID_TILE != chr[cnt].onwhichfan )
        {
            twist = meshtwist[chr[cnt].onwhichfan];
            chr[cnt].turnmaplr = maplrtwist[twist];
            chr[cnt].turnmapud = mapudtwist[twist];
        }
        else
        {
            chr[cnt].turnmaplr = 32768;
            chr[cnt].turnmapud = 32768;
        }
    }

}

//--------------------------------------------------------------------------------------------
void init_ai_state( ai_state_t * pself, Uint16 index, Uint16 profile, Uint16 model, Uint16 rank )
{
    int tnc;
    if ( NULL == pself || index >= MAXCHR ) return;

    // clear out everything
    memset( pself, 0, sizeof(ai_state_t) );

    pself->index      = index;
    pself->type       = madai[model];
    pself->alert      = ALERTIF_SPAWNED;
    pself->state      = capstateoverride[profile];
    pself->content    = capcontentoverride[profile];
    pself->passage    = 0;
    pself->target     = index;
    pself->owner      = index;
    pself->child      = index;
    pself->target_old = pself->target;
    pself->poof_time  = -1;

    pself->timer      = 0;

    pself->bumplast   = index;
    pself->attacklast = MAXCHR;
    pself->hitlast    = index;

    pself->rank       = rank;
    pself->order      = 0;

    pself->wp_tail = 0;
    pself->wp_head = 0;
    for ( tnc = 0; tnc < MAXSTOR; tnc++ )
    {
        pself->x[tnc] = 0;
        pself->y[tnc] = 0;
    }
}

//--------------------------------------------------------------------------------------------
int spawn_one_character( float x, float y, float z, Uint16 profile, Uint8 team,
                         Uint8 skin, Uint16 facing,  const char *name, int override )
{
    // ZZ> This function spawns a character and returns the character's index number
    //     if it worked, MAXCHR otherwise

    Uint16 ichr;
    int tnc;

    if ( profile < 0 || profile > MAXMODEL )
    {
        return MAXCHR;
    }

    if ( !madused[profile] )
    {
        if ( profile > importamount * 9 )
        {
            log_warning( "spawn_one_character() - trying to spawn using invalid profile %d\n", profile );
        }

        return MAXCHR;
    }

    // allocate a new character
    ichr = MAXCHR;
    if ( override < MAXCHR )
    {
        ichr = get_free_character();
        if ( ichr != override )
        {
            // Picked the wrong one, so put this one back and find the right one

            for ( tnc = 0; tnc < MAXCHR; tnc++ )
            {
                if ( freechrlist[tnc] == override )
                {
                    freechrlist[tnc] = ichr;
                    break;
                }
            }

            ichr = override;
        }

        if ( MAXCHR == ichr )
        {
            log_warning( "spawn_one_character() - failed to override a character? character %d already spawned? \n", override );
            return ichr;
        }
    }
    else
    {
        ichr = get_free_character();
        if ( MAXCHR == ichr )
        {
            log_warning( "spawn_one_character() - failed to allocate a new character\n" );
            return ichr;
        }
    }

    if ( MAXCHR == ichr )
    {
        log_warning( "spawn_one_character() - failed to spawn character\n" );
        return ichr;
    }

    // Make sure the team is valid
    team = MIN( team, MAXTEAM - 1 );

    // IMPORTANT!!!
    chr[ichr].indolist = bfalse;
    chr[ichr].isequipped = bfalse;
    chr[ichr].sparkle = NOSPARKLE;
    chr[ichr].overlay = bfalse;
    chr[ichr].missilehandler = ichr;

    // sound stuff...  copy from the cap
    for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
    {
        chr[ichr].soundindex[tnc] = capsoundindex[profile][tnc];
    }

    // Set up model stuff
    chr[ichr].reloadtime = 0;
    chr[ichr].inwhichhand = GRIP_LEFT;
    chr[ichr].waskilled = bfalse;
    chr[ichr].inpack = bfalse;
    chr[ichr].nextinpack = MAXCHR;
    chr[ichr].numinpack = 0;
    chr[ichr].model = profile;
    chr[ichr].basemodel = profile;
    chr[ichr].stoppedby = capstoppedby[profile];
    chr[ichr].lifeheal = caplifeheal[profile];
    chr[ichr].manacost = capmanacost[profile];
    chr[ichr].inwater = bfalse;
    chr[ichr].nameknown = capnameknown[profile];
    chr[ichr].ammoknown = capnameknown[profile];
    chr[ichr].hitready = btrue;
    chr[ichr].boretime = BORETIME;
    chr[ichr].carefultime = CAREFULTIME;
    chr[ichr].canbecrushed = bfalse;
    chr[ichr].damageboost = 0;
    chr[ichr].icon = capicon[profile];

    // Enchant stuff
    chr[ichr].firstenchant = MAXENCHANT;
    chr[ichr].undoenchant = MAXENCHANT;
    chr[ichr].canseeinvisible = capcanseeinvisible[profile];
    chr[ichr].canchannel = bfalse;
    chr[ichr].missiletreatment = MISNORMAL;
    chr[ichr].missilecost = 0;

    //Skillz
    chr[ichr].canjoust = capcanjoust[profile];
    chr[ichr].canuseadvancedweapons = capcanuseadvancedweapons[profile];
    chr[ichr].shieldproficiency = capshieldproficiency[profile];
    chr[ichr].canusedivine = capcanusedivine[profile];
    chr[ichr].canusearcane = capcanusearcane[profile];
    chr[ichr].canusetech = capcanusetech[profile];
    chr[ichr].candisarm = capcandisarm[profile];
    chr[ichr].canbackstab = capcanbackstab[profile];
    chr[ichr].canusepoison = capcanusepoison[profile];
    chr[ichr].canread = capcanread[profile];
    chr[ichr].canseekurse = capcanseekurse[profile];

    // Kurse state
    chr[ichr].iskursed = ( ( rand() % 100 ) < capkursechance[profile] );
    if ( !capisitem[profile] )  chr[ichr].iskursed = bfalse;

    // Ammo
    chr[ichr].ammomax = capammomax[profile];
    chr[ichr].ammo = capammo[profile];

    // Gender
    chr[ichr].gender = capgender[profile];
    if ( chr[ichr].gender == GENRANDOM )  chr[ichr].gender = GENFEMALE + ( rand() & 1 );

    chr[ichr].isplayer = bfalse;
    chr[ichr].islocalplayer = bfalse;

    // AI stuff
    init_ai_state( &(chr[ichr].ai), ichr, profile, chr[ichr].model, teammorale[team] );

    // Team stuff
    chr[ichr].team = team;
    chr[ichr].baseteam = team;
    if ( !capinvictus[profile] )  teammorale[team]++;

    // Firstborn becomes the leader
    if ( teamleader[team] == NOLEADER )
    {
        teamleader[team] = ichr;
    }

    // Skin
    if ( capskinoverride[profile] != NOSKINOVERRIDE )
    {
        skin = capskinoverride[profile] % MAXSKIN;
    }
    if ( skin >= madskins[profile] )
    {
        skin = 0;
        if ( madskins[profile] > 1 )
        {
            skin = rand() % madskins[profile];
        }
    }

    chr[ichr].skin    = skin;
    chr[ichr].texture = madskinstart[profile] + skin;

    // Life and Mana
    chr[ichr].alive = btrue;
    chr[ichr].lifecolor = caplifecolor[profile];
    chr[ichr].manacolor = capmanacolor[profile];
    chr[ichr].lifemax = generate_number( caplifebase[profile], capliferand[profile] );
    chr[ichr].life = chr[ichr].lifemax;
    chr[ichr].lifereturn = caplifereturn[profile];
    chr[ichr].manamax = generate_number( capmanabase[profile], capmanarand[profile] );
    chr[ichr].manaflow = generate_number( capmanaflowbase[profile], capmanaflowrand[profile] );
    chr[ichr].manareturn = generate_number( capmanareturnbase[profile], capmanareturnrand[profile] );
    chr[ichr].mana = chr[ichr].manamax;

    // SWID
    chr[ichr].strength = generate_number( capstrengthbase[profile], capstrengthrand[profile] );
    chr[ichr].wisdom = generate_number( capwisdombase[profile], capwisdomrand[profile] );
    chr[ichr].intelligence = generate_number( capintelligencebase[profile], capintelligencerand[profile] );
    chr[ichr].dexterity = generate_number( capdexteritybase[profile], capdexterityrand[profile] );

    // Damage
    chr[ichr].defense = capdefense[profile][skin];
    chr[ichr].reaffirmdamagetype = capattachedprtreaffirmdamagetype[profile];
    chr[ichr].damagetargettype = capdamagetargettype[profile];
    tnc = 0;

    while ( tnc < DAMAGE_COUNT )
    {
        chr[ichr].damagemodifier[tnc] = capdamagemodifier[profile][tnc][skin];
        tnc++;
    }

    //latches
    chr[ichr].latchx = 0;
    chr[ichr].latchy = 0;
    chr[ichr].latchbutton = 0;

    chr[ichr].turnmode = TURNMODEVELOCITY;

    // Flags
    chr[ichr].stickybutt = capstickybutt[profile];
    chr[ichr].openstuff = capcanopenstuff[profile];
    chr[ichr].transferblend = captransferblend[profile];
    chr[ichr].enviro = capenviro[profile];
    chr[ichr].waterwalk = capwaterwalk[profile];
    chr[ichr].platform = capplatform[profile];
    chr[ichr].isitem = capisitem[profile];
    chr[ichr].invictus = capinvictus[profile];
    chr[ichr].ismount = capismount[profile];
    chr[ichr].cangrabmoney = capcangrabmoney[profile];

    // Jumping
    chr[ichr].jump = capjump[profile];
    chr[ichr].jumpnumber = 0;
    chr[ichr].jumpnumberreset = capjumpnumber[profile];
    chr[ichr].jumptime = JUMPDELAY;

    // Other junk
    chr[ichr].flyheight = capflyheight[profile];
    chr[ichr].maxaccel = capmaxaccel[profile][skin];
    chr[ichr].alpha = chr[ichr].basealpha = capalpha[profile];
    chr[ichr].light = caplight[profile];
    chr[ichr].flashand = capflashand[profile];
    chr[ichr].sheen = capsheen[profile];
    chr[ichr].dampen = capdampen[profile];

    // Character size and bumping
    chr[ichr].fat = capsize[profile];
    chr[ichr].sizegoto = chr[ichr].fat;
    chr[ichr].sizegototime = 0;
    chr[ichr].shadowsize = capshadowsize[profile] * chr[ichr].fat;
    chr[ichr].bumpsize = capbumpsize[profile] * chr[ichr].fat;
    chr[ichr].bumpsizebig = capbumpsizebig[profile] * chr[ichr].fat;
    chr[ichr].bumpheight = capbumpheight[profile] * chr[ichr].fat;

    chr[ichr].shadowsizesave = capshadowsize[profile];
    chr[ichr].bumpsizesave = capbumpsize[profile];
    chr[ichr].bumpsizebigsave = capbumpsizebig[profile];
    chr[ichr].bumpheightsave = capbumpheight[profile];

    chr[ichr].bumpdampen = capbumpdampen[profile];
    if ( capweight[profile] == 0xFF )
    {
        chr[ichr].weight = 0xFFFF;
    }
    else
    {
        int itmp = capweight[profile] * chr[ichr].fat * chr[ichr].fat * chr[ichr].fat;
        chr[ichr].weight = MIN( itmp, 0xFFFE );
    }

    // Grip info
    chr[ichr].attachedto = MAXCHR;
    chr[ichr].holdingwhich[0] = MAXCHR;
    chr[ichr].holdingwhich[1] = MAXCHR;

    // Image rendering
    chr[ichr].uoffset = 0;
    chr[ichr].voffset = 0;
    chr[ichr].uoffvel = capuoffvel[profile];
    chr[ichr].voffvel = capvoffvel[profile];
    chr[ichr].redshift = 0;
    chr[ichr].grnshift = 0;
    chr[ichr].blushift = 0;

    // Movement
    chr[ichr].sneakspd = capsneakspd[profile];
    chr[ichr].walkspd = capwalkspd[profile];
    chr[ichr].runspd = caprunspd[profile];

    // Set up position
    chr[ichr].xpos = x;
    chr[ichr].ypos = y;
    chr[ichr].oldx = x;
    chr[ichr].oldy = y;
    chr[ichr].turnleftright = facing;
    chr[ichr].lightturnleftright = 0;
    chr[ichr].onwhichfan   = mesh_get_tile(x, y);
    chr[ichr].onwhichblock = mesh_get_block( x, y );
    chr[ichr].level = get_level( chr[ichr].xpos, chr[ichr].ypos, chr[ichr].waterwalk ) + RAISE;
    if ( z < chr[ichr].level ) z = chr[ichr].level;

    chr[ichr].zpos = z;
    chr[ichr].oldz = z;
    chr[ichr].xstt = chr[ichr].xpos;
    chr[ichr].ystt = chr[ichr].ypos;
    chr[ichr].zstt = chr[ichr].zpos;
    chr[ichr].xvel = 0;
    chr[ichr].yvel = 0;
    chr[ichr].zvel = 0;
    chr[ichr].turnmaplr = 32768;  // These two mean on level surface
    chr[ichr].turnmapud = 32768;

    // action stuff
    chr[ichr].actionready = btrue;
    chr[ichr].keepaction = bfalse;
    chr[ichr].loopaction = bfalse;
    chr[ichr].action = ACTIONDA;
    chr[ichr].nextaction = ACTIONDA;
    chr[ichr].lip = 0;
    chr[ichr].frame = madframestart[chr[ichr].model];
    chr[ichr].lastframe = chr[ichr].frame;

    chr[ichr].holdingweight = 0;

    // Timers set to 0
    chr[ichr].grogtime = 0;
    chr[ichr].dazetime = 0;

    // Money is added later
    chr[ichr].money = capmoney[profile];

    // Name the character
    if ( name == NULL )
    {
        // Generate a random name
        naming_names( profile );
        sprintf( chr[ichr].name, "%s", namingnames );
    }
    else
    {
        // A name has been given
        tnc = 0;

        while ( tnc < MAXCAPNAMESIZE - 1 )
        {
            chr[ichr].name[tnc] = name[tnc];
            tnc++;
        }

        chr[ichr].name[tnc] = 0;
    }

    // Set up initial fade in lighting
    for ( tnc = 0; tnc < madtransvertices[chr[ichr].model]; tnc++ )
    {
        chr[ichr].vrta[tnc] = 0;
    }

    // Particle attachments
    tnc = 0;
    while ( tnc < capattachedprtamount[profile] )
    {
        spawn_one_particle( chr[ichr].xpos, chr[ichr].ypos, chr[ichr].zpos,
                            0, chr[ichr].model, capattachedprttype[profile],
                            ichr, GRIP_LAST + tnc, chr[ichr].team, ichr, tnc, MAXCHR );
        tnc++;
    }

    chr[ichr].reaffirmdamagetype = capattachedprtreaffirmdamagetype[profile];

    // Experience
    tnc = generate_number( capexperiencebase[profile], capexperiencerand[profile] );
    if ( tnc > MAXXP ) tnc = MAXXP;

    chr[ichr].experience = tnc;
    chr[ichr].experiencelevel = capleveloverride[profile];

    //Items that are spawned inside shop passages are more expensive than normal
    if (capisvaluable[profile])
    {
        chr[ichr].isshopitem = btrue;
    }
    else
    {
        chr[ichr].isshopitem = bfalse;
        if (chr[ichr].isitem && !chr[ichr].inpack && chr[ichr].attachedto == MAXCHR)
        {
            float tlx, tly, brx, bry;
            Uint16 passage = 0;
            float bumpsize;

            bumpsize = chr[ichr].bumpsize;

            while (passage < numpassage)
            {
                // Passage area
                tlx = ( passtlx[passage] << 7 ) - CLOSETOLERANCE;
                tly = ( passtly[passage] << 7 ) - CLOSETOLERANCE;
                brx = ( ( passbrx[passage] + 1 ) << 7 ) + CLOSETOLERANCE;
                bry = ( ( passbry[passage] + 1 ) << 7 ) + CLOSETOLERANCE;

                //Check if the character is inside that passage
                if ( chr[ichr].xpos > tlx - bumpsize && chr[ichr].xpos < brx + bumpsize )
                {
                    if ( chr[ichr].ypos > tly - bumpsize && chr[ichr].ypos < bry + bumpsize )
                    {
                        //Yep, flag as valuable (does not export)
                        chr[ichr].isshopitem = btrue;
                        break;
                    }
                }

                passage++;
            }
        }
    }

    chr[ichr].on = btrue;

    return ichr;
}

//--------------------------------------------------------------------------------------------
void respawn_character( Uint16 character )
{
    // ZZ> This function respawns a character
    Uint16 item;
    if ( chr[character].alive ) return;

    spawn_poof( character, chr[character].model );
    disaffirm_attached_particles( character );
    chr[character].alive = btrue;
    chr[character].boretime = BORETIME;
    chr[character].carefultime = CAREFULTIME;
    chr[character].life = chr[character].lifemax;
    chr[character].mana = chr[character].manamax;
    chr[character].xpos = chr[character].xstt;
    chr[character].ypos = chr[character].ystt;
    chr[character].zpos = chr[character].zstt;
    chr[character].xvel = 0;
    chr[character].yvel = 0;
    chr[character].zvel = 0;
    chr[character].team = chr[character].baseteam;
    chr[character].canbecrushed = bfalse;
    chr[character].turnmaplr = 32768;  // These two mean on level surface
    chr[character].turnmapud = 32768;
    if ( teamleader[chr[character].team] == NOLEADER )  teamleader[chr[character].team] = character;
    if ( !chr[character].invictus )  teammorale[chr[character].baseteam]++;

    chr[character].actionready = btrue;
    chr[character].keepaction = bfalse;
    chr[character].loopaction = bfalse;
    chr[character].action = ACTIONDA;
    chr[character].nextaction = ACTIONDA;
    chr[character].lip = 0;
    chr[character].frame = madframestart[chr[character].model];
    chr[character].lastframe = chr[character].frame;
    chr[character].platform = capplatform[chr[character].model];
    chr[character].flyheight = capflyheight[chr[character].model];
    chr[character].bumpdampen = capbumpdampen[chr[character].model];
    chr[character].bumpsize = capbumpsize[chr[character].model] * chr[character].fat;
    chr[character].bumpsizebig = capbumpsizebig[chr[character].model] * chr[character].fat;
    chr[character].bumpheight = capbumpheight[chr[character].model] * chr[character].fat;

    chr[character].bumpsizesave = capbumpsize[chr[character].model];
    chr[character].bumpsizebigsave = capbumpsizebig[chr[character].model];
    chr[character].bumpheightsave = capbumpheight[chr[character].model];

    chr[character].ai.alert = 0;
    chr[character].ai.target = character;
    chr[character].ai.timer  = 0;

    chr[character].grogtime = 0;
    chr[character].dazetime = 0;
    reaffirm_attached_particles( character );

    // Let worn items come back
    for ( item = chr[character].nextinpack; item < MAXCHR; item = chr[item].nextinpack )
    {
        if ( chr[item].on && chr[item].isequipped )
        {
            chr[item].isequipped = bfalse;
            chr[item].ai.alert |= ALERTIF_ATLASTWAYPOINT;  // doubles as PutAway
        }
    }

}

//--------------------------------------------------------------------------------------------
Uint16 change_armor( Uint16 character, Uint16 skin )
{
    // ZZ> This function changes the armor of the character

    Uint16 enchant, sTmp;
    int iTmp;

    // Remove armor enchantments
    enchant = chr[character].firstenchant;

    while ( enchant < MAXENCHANT )
    {
        unset_enchant_value( enchant, SETSLASHMODIFIER );
        unset_enchant_value( enchant, SETCRUSHMODIFIER );
        unset_enchant_value( enchant, SETPOKEMODIFIER );
        unset_enchant_value( enchant, SETHOLYMODIFIER );
        unset_enchant_value( enchant, SETEVILMODIFIER );
        unset_enchant_value( enchant, SETFIREMODIFIER );
        unset_enchant_value( enchant, SETICEMODIFIER );
        unset_enchant_value( enchant, SETZAPMODIFIER );
        enchant = encnextenchant[enchant];
    }

    // Change the skin
    sTmp = chr[character].model;
    if ( skin > madskins[sTmp] )  skin = 0;

    chr[character].skin    = skin;
    chr[character].texture = madskinstart[sTmp] + skin;

    // Change stats associated with skin
    chr[character].defense = capdefense[sTmp][skin];
    iTmp = 0;

    while ( iTmp < DAMAGE_COUNT )
    {
        chr[character].damagemodifier[iTmp] = capdamagemodifier[sTmp][iTmp][skin];
        iTmp++;
    }

    chr[character].maxaccel = capmaxaccel[sTmp][skin];

    // Reset armor enchantments
    // These should really be done in reverse order ( Start with last enchant ), but
    // I don't care at this point !!!BAD!!!
    enchant = chr[character].firstenchant;

    while ( enchant < MAXENCHANT )
    {
        set_enchant_value( enchant, SETSLASHMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETCRUSHMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETPOKEMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETHOLYMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETEVILMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETFIREMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETICEMODIFIER, enceve[enchant] );
        set_enchant_value( enchant, SETZAPMODIFIER, enceve[enchant] );
        add_enchant_value( enchant, ADDACCEL, enceve[enchant] );
        add_enchant_value( enchant, ADDDEFENSE, enceve[enchant] );
        enchant = encnextenchant[enchant];
    }

    return skin;
}

//--------------------------------------------------------------------------------------------
void change_character( Uint16 ichr, Uint16 profile, Uint8 skin,
                       Uint8 leavewhich )
{
    // ZZ> This function polymorphs a character, changing stats, dropping weapons
    int tnc, enchant;
    Uint16 sTmp, item;

    profile = profile % MAXMODEL;
    if ( madused[profile] )
    {
        // Drop left weapon
        sTmp = chr[ichr].holdingwhich[0];
        if ( sTmp != MAXCHR && ( !capgripvalid[profile][0] || capismount[profile] ) )
        {
            detach_character_from_mount( sTmp, btrue, btrue );
            if ( chr[ichr].ismount )
            {
                chr[sTmp].zvel = DISMOUNTZVEL;
                chr[sTmp].zpos += DISMOUNTZVEL;
                chr[sTmp].jumptime = JUMPDELAY;
            }
        }

        // Drop right weapon
        sTmp = chr[ichr].holdingwhich[1];
        if ( sTmp != MAXCHR && !capgripvalid[profile][1] )
        {
            detach_character_from_mount( sTmp, btrue, btrue );
            if ( chr[ichr].ismount )
            {
                chr[sTmp].zvel = DISMOUNTZVEL;
                chr[sTmp].zpos += DISMOUNTZVEL;
                chr[sTmp].jumptime = JUMPDELAY;
            }
        }

        // Remove particles
        disaffirm_attached_particles( ichr );

        // Remove enchantments
        if ( leavewhich == LEAVEFIRST )
        {
            // Remove all enchantments except top one
            enchant = chr[ichr].firstenchant;
            if ( enchant != MAXENCHANT )
            {
                while ( encnextenchant[enchant] != MAXENCHANT )
                {
                    remove_enchant( encnextenchant[enchant] );
                }
            }
        }

        if ( leavewhich == LEAVENONE )
        {
            // Remove all enchantments
            disenchant_character( ichr );
        }

        // Stuff that must be set
        chr[ichr].model = profile;
        chr[ichr].stoppedby = capstoppedby[profile];
        chr[ichr].lifeheal = caplifeheal[profile];
        chr[ichr].manacost = capmanacost[profile];

        // Ammo
        chr[ichr].ammomax = capammomax[profile];
        chr[ichr].ammo = capammo[profile];

        // Gender
        if ( capgender[profile] != GENRANDOM )  // GENRANDOM means keep old gender
        {
            chr[ichr].gender = capgender[profile];
        }

        for ( tnc = 0; tnc < SOUND_COUNT; tnc++ )
        {
            chr[ichr].soundindex[tnc] = capsoundindex[profile][tnc];
        }

        // AI stuff
        chr[ichr].ai.type = madai[profile];
        chr[ichr].ai.state = 0;
        chr[ichr].ai.timer = 0;

        chr[ichr].latchx = 0;
        chr[ichr].latchy = 0;
        chr[ichr].latchbutton = 0;
        chr[ichr].turnmode = TURNMODEVELOCITY;
        // Flags
        chr[ichr].stickybutt = capstickybutt[profile];
        chr[ichr].openstuff = capcanopenstuff[profile];
        chr[ichr].transferblend = captransferblend[profile];
        chr[ichr].enviro = capenviro[profile];
        chr[ichr].platform = capplatform[profile];
        chr[ichr].isitem = capisitem[profile];
        chr[ichr].invictus = capinvictus[profile];
        chr[ichr].ismount = capismount[profile];
        chr[ichr].cangrabmoney = capcangrabmoney[profile];
        chr[ichr].jumptime = JUMPDELAY;
        // Character size and bumping
        chr[ichr].shadowsize = (Uint8)(capshadowsize[profile] * chr[ichr].fat);
        chr[ichr].bumpsize = (Uint8) (capbumpsize[profile] * chr[ichr].fat);
        chr[ichr].bumpsizebig = capbumpsizebig[profile] * chr[ichr].fat;
        chr[ichr].bumpheight = capbumpheight[profile] * chr[ichr].fat;

        chr[ichr].shadowsizesave = capshadowsize[profile];
        chr[ichr].bumpsizesave = capbumpsize[profile];
        chr[ichr].bumpsizebigsave = capbumpsizebig[profile];
        chr[ichr].bumpheightsave = capbumpheight[profile];

        chr[ichr].bumpdampen = capbumpdampen[profile];

        if ( capweight[profile] == 0xFF )
        {
            chr[ichr].weight = 0xFFFF;
        }
        else
        {
            int itmp = capweight[profile] * chr[ichr].fat * chr[ichr].fat * chr[ichr].fat;
            chr[ichr].weight = MIN( itmp, 0xFFFE );
        }

        // Character scales...  Magic numbers
        if ( chr[ichr].attachedto != MAXCHR )
        {
            int i;
            Uint16 iholder = chr[ichr].attachedto;
            tnc = madvertices[chr[iholder].model] - chr[ichr].inwhichhand;

            for (i = 0; i < GRIP_VERTS; i++)
            {
                if (tnc + i < madvertices[chr[iholder].model] )
                {
                    chr[ichr].weapongrip[i] = tnc + i;
                }
                else
                {
                    chr[ichr].weapongrip[i] = 0xFFFF;
                }
            }
        }

        item = chr[ichr].holdingwhich[0];
        if ( item != MAXCHR )
        {
            int i;

            tnc = madvertices[chr[ichr].model] - GRIP_LEFT;

            for (i = 0; i < GRIP_VERTS; i++)
            {
                if (tnc + i < madvertices[chr[ichr].model] )
                {
                    chr[item].weapongrip[i] = i + tnc;
                }
                else
                {
                    chr[item].weapongrip[i] = 0xFFFF;
                }
            }
        }

        item = chr[ichr].holdingwhich[1];
        if ( item != MAXCHR )
        {
            int i;

            tnc = madvertices[chr[ichr].model] - GRIP_RIGHT;

            for (i = 0; i < GRIP_VERTS; i++)
            {
                if (tnc + i < madvertices[chr[ichr].model] )
                {
                    chr[item].weapongrip[i] = i + tnc;
                }
                else
                {
                    chr[item].weapongrip[i] = 0xFFFF;
                }
            }
        }

        // Image rendering
        chr[ichr].uoffset = 0;
        chr[ichr].voffset = 0;
        chr[ichr].uoffvel = capuoffvel[profile];
        chr[ichr].voffvel = capvoffvel[profile];

        // Movement
        chr[ichr].sneakspd = capsneakspd[profile];
        chr[ichr].walkspd = capwalkspd[profile];
        chr[ichr].runspd = caprunspd[profile];

        // AI and action stuff
        chr[ichr].actionready = btrue;
        chr[ichr].keepaction = bfalse;
        chr[ichr].loopaction = bfalse;
        chr[ichr].action = ACTIONDA;
        chr[ichr].nextaction = ACTIONDA;
        chr[ichr].lip = 0;
        chr[ichr].frame = madframestart[profile];
        chr[ichr].lastframe = chr[ichr].frame;
        chr[ichr].holdingweight = 0;

        // Set the skin
        change_armor( ichr, skin );

        // Reaffirm them particles...
        chr[ichr].reaffirmdamagetype = capattachedprtreaffirmdamagetype[profile];
        reaffirm_attached_particles( ichr );

        // Set up initial fade in lighting
        for ( tnc = 0; tnc < madtransvertices[chr[ichr].model]; tnc++ )
        {
            chr[ichr].vrta[tnc] = 0;
        }
    }
}

//--------------------------------------------------------------------------------------------
/*Uint16 get_target_in_block( int x, int y, Uint16 character, char items,
                            char friends, char enemies, char dead, char seeinvisible, IDSZ idsz,
                            char excludeid )
{
  // ZZ> This is a good little helper, that returns != MAXCHR if a suitable target
  //     was found
  int cnt;
  Uint16 charb;
  Uint32 fanblock;
  Uint8 team;
  if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
  {
    team = chr[character].team;
    fanblock = x + meshblockstart[y];
    charb = meshbumplistchr[fanblock];
    cnt = 0;
    while ( cnt < meshbumplistchrnum[fanblock] )
    {
      if ( dead != chr[charb].alive && ( seeinvisible || ( chr[charb].alpha > INVISIBLE && chr[charb].light > INVISIBLE ) ) )
      {
        if ( ( enemies && teamhatesteam[team][chr[charb].team] && !chr[charb].invictus ) ||
             ( items && chr[charb].isitem ) ||
             ( friends && chr[charb].baseteam == team ) )
        {
          if ( charb != character && chr[character].attachedto != charb )
          {
            if ( !chr[charb].isitem || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( capidsz[chr[charb].model][IDSZ_PARENT] == idsz ||
                     capidsz[chr[charb].model][IDSZ_TYPE] == idsz )
                {
                  if ( !excludeid ) return charb;
                }
                else
                {
                  if ( excludeid )  return charb;
                }
              }
              else
              {
                return charb;
              }
            }
          }
        }
      }
      charb = chr[charb].bumpnext;
      cnt++;
    }
  }
  return MAXCHR;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 get_nearby_target( Uint16 character, char items,
                          char friends, char enemies, char dead, IDSZ idsz )
{
  // ZZ> This function finds a nearby target, or it returns MAXCHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = chr[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )chr[character].xpos ) >> 9;
  y = ( ( int )chr[character].ypos ) >> 9;
  return get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, 0 );
}*/

//--------------------------------------------------------------------------------------------
Uint8 cost_mana( Uint16 character, int amount, Uint16 killer )
{
    // ZZ> This function takes mana from a character ( or gives mana ),
    //     and returns btrue if the character had enough to pay, or bfalse
    //     otherwise
    int iTmp;

    iTmp = chr[character].mana - amount;
    if ( iTmp < 0 )
    {
        chr[character].mana = 0;
        if ( chr[character].canchannel )
        {
            chr[character].life += iTmp;
            if ( chr[character].life <= 0 )
            {
                kill_character( character, (killer == MAXCHR) ? character : killer );
            }

            return btrue;
        }

        return bfalse;
    }
    else
    {
        chr[character].mana = iTmp;
        if ( iTmp > chr[character].manamax )
        {
            chr[character].mana = chr[character].manamax;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
/*Uint16 find_distant_target( Uint16 character, int maxdistance )
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one...
  //     maxdistance should be the square of the actual distance you want to use
  //     as the cutoff...
  int cnt, distance, xdistance, ydistance;
  Uint8 team;

  team = chr[character].team;
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( chr[cnt].on )
    {
      if ( chr[cnt].attachedto == MAXCHR && !chr[cnt].inpack )
      {
        if ( teamhatesteam[team][chr[cnt].team] && chr[cnt].alive && !chr[cnt].invictus )
        {
          if ( chr[character].canseeinvisible || ( chr[cnt].alpha > INVISIBLE && chr[cnt].light > INVISIBLE ) )
          {
            xdistance = (int) (chr[cnt].xpos - chr[character].xpos);
            ydistance = (int) (chr[cnt].ypos - chr[character].ypos);
            distance = xdistance * xdistance + ydistance * ydistance;
            if ( distance < maxdistance )
            {
              return cnt;
            }
          }
        }
      }
    }
    cnt++;
  }
  return MAXCHR;
}*/

//--------------------------------------------------------------------------------------------
void switch_team( Uint16 character, Uint8 team )
{
    // ZZ> This function makes a character join another team...
    if ( team < MAXTEAM )
    {
        if ( !chr[character].invictus )
        {
            teammorale[chr[character].baseteam]--;
            teammorale[team]++;
        }
        if ( ( !chr[character].ismount || chr[character].holdingwhich[0] == MAXCHR ) &&
                ( !chr[character].isitem || chr[character].attachedto == MAXCHR ) )
        {
            chr[character].team = team;
        }

        chr[character].baseteam = team;
        if ( teamleader[team] == NOLEADER )
        {
            teamleader[team] = character;
        }
    }
}

//--------------------------------------------------------------------------------------------
/*void get_nearest_in_block( int x, int y, Uint16 character, char items,
                           char friends, char enemies, char dead, char seeinvisible, IDSZ idsz )
{
  // ZZ> This is a good little helper
  float distance, xdis, ydis;
  int cnt;
  Uint8 team;
  Uint16 charb;
  Uint32 fanblock;
  if ( x >= 0 && x < meshbloksx && y >= 0 && y < meshbloksy )
  {
    team = chr[character].team;
    fanblock = x + meshblockstart[y];
    charb = meshbumplistchr[fanblock];
    cnt = 0;
    while ( cnt < meshbumplistchrnum[fanblock] )
    {
      if ( dead != chr[charb].alive && ( seeinvisible || ( chr[charb].alpha > INVISIBLE && chr[charb].light > INVISIBLE ) ) )
      {
        if ( ( enemies && teamhatesteam[team][chr[charb].team] ) ||
             ( items && chr[charb].isitem ) ||
             ( friends && chr[charb].team == team ) ||
             ( friends && enemies ) )
        {
          if ( charb != character && chr[character].attachedto != charb && chr[charb].attachedto == MAXCHR && !chr[charb].inpack )
          {
            if ( !chr[charb].invictus || items )
            {
              if ( idsz != IDSZ_NONE )
              {
                if ( capidsz[chr[charb].model][IDSZ_PARENT] == idsz ||
                     capidsz[chr[charb].model][IDSZ_TYPE] == idsz )
                {
                  xdis = chr[character].xpos - chr[charb].xpos;
                  ydis = chr[character].ypos - chr[charb].ypos;
                  xdis = xdis * xdis;
                  ydis = ydis * ydis;
                  distance = xdis + ydis;
                  if ( distance < globaldistance )
                  {
                    globalnearest = charb;
                    globaldistance = distance;
                  }
                }
              }
              else
              {
                xdis = chr[character].xpos - chr[charb].xpos;
                ydis = chr[character].ypos - chr[charb].ypos;
                xdis = xdis * xdis;
                ydis = ydis * ydis;
                distance = xdis + ydis;
                if ( distance < globaldistance )
                {
                  globalnearest = charb;
                  globaldistance = distance;
                }
              }
            }
          }
        }
      }
      charb = chr[charb].bumpnext;
      cnt++;
    }
  }
  return;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 get_nearest_target( Uint16 character, char items,
                           char friends, char enemies, char dead, IDSZ idsz )
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = chr[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )chr[character].xpos ) >> 9;
  y = ( ( int )chr[character].ypos ) >> 9;

  globalnearest = MAXCHR;
  globaldistance = 999999;
  get_nearest_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz );

  get_nearest_in_block( x - 1, y, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x + 1, y, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz );

  get_nearest_in_block( x - 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x + 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x - 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  get_nearest_in_block( x + 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz );
  return globalnearest;
}*/

//--------------------------------------------------------------------------------------------
/*Uint16 get_wide_target( Uint16 character, char items,
                        char friends, char enemies, char dead, IDSZ idsz, char excludeid )
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one
  int x, y;
  Uint16 enemy;
  char seeinvisible;
  seeinvisible = chr[character].canseeinvisible;

  // Current fanblock
  x = ( ( int )chr[character].xpos ) >> 9;
  y = ( ( int )chr[character].ypos ) >> 9;
  enemy = get_target_in_block( x, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;

  enemy = get_target_in_block( x - 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x + 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;

  enemy = get_target_in_block( x - 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x + 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x - 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  if ( enemy != MAXCHR )  return enemy;
  enemy = get_target_in_block( x + 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid );
  return enemy;
}*/

//--------------------------------------------------------------------------------------------
void issue_clean( Uint16 character )
{
    // ZZ> This function issues a clean up order to all teammates
    Uint8 team;
    Uint16 cnt;

    team = chr[character].team;
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
        if ( !chr[cnt].on || team != chr[cnt].team ) continue;

        chr[cnt].ai.timer  = frame_wld + 2;  // Don't let it think too much...
        chr[cnt].ai.alert |= ALERTIF_CLEANEDUP;
    }
}

//--------------------------------------------------------------------------------------------
int restock_ammo( Uint16 character, IDSZ idsz )
{
    // ZZ> This function restocks the characters ammo, if it needs ammo and if
    //     either its parent or type idsz match the given idsz.  This
    //     function returns the amount of ammo given.
    int amount, model;

    amount = 0;
    if ( character < MAXCHR )
    {
        if ( chr[character].on )
        {
            model = chr[character].model;
            if ( capidsz[model][IDSZ_PARENT] == idsz || capidsz[model][IDSZ_TYPE] == idsz )
            {
                if ( chr[character].ammo < chr[character].ammomax )
                {
                    amount = chr[character].ammomax - chr[character].ammo;
                    chr[character].ammo = chr[character].ammomax;
                }
            }
        }
    }

    return amount;
}

//--------------------------------------------------------------------------------------------
void issue_order( Uint16 character, Uint32 order )
{
    // ZZ> This function issues an order for help to all teammates
    Uint8 team;
    Uint16 counter;
    Uint16 cnt;

    team = chr[character].team;
    counter = 0;
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].team == team )
        {
            chr[cnt].ai.order = order;
            chr[cnt].ai.rank  = counter;
            chr[cnt].ai.alert |= ALERTIF_ORDERED;
            counter++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( Uint32 order, IDSZ idsz )
{
    // ZZ> This function issues an order to all characters with the a matching special IDSZ
    Uint16 counter;
    Uint16 cnt;

    counter = 0;
    cnt = 0;

    while ( cnt < MAXCHR )
    {
        if ( chr[cnt].on )
        {
            if ( capidsz[chr[cnt].model][IDSZ_SPECIAL] == idsz )
            {
                chr[cnt].ai.order = order;
                chr[cnt].ai.rank  = counter;
                chr[cnt].ai.alert |= ALERTIF_ORDERED;
                counter++;
            }
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz )
{
    //ZF> This is the new improved AI targeting system. Also includes distance in the Z direction.
    //If maxdistance is 0 then it searches without a max limit.
    Uint16 besttarget = MAXCHR, cnt;
    Uint32 longdist = pow(2, 31);
    if (team == NONE) return MAXCHR;

    for (cnt = 0; cnt < MAXCHR; cnt++)
    {
        if (chr[cnt].on && cnt != character && (targetdead || chr[cnt].alive) && (targetitems || (!chr[cnt].isitem && !chr[cnt].invictus)) && chr[cnt].attachedto == MAXCHR
                && (team != ENEMY || chr[character].canseeinvisible || ( chr[cnt].alpha > INVISIBLE && chr[cnt].light > INVISIBLE ) )
                && (team == ALL || team != teamhatesteam[chr[character].team][chr[cnt].team]) )
        {
            //Check for IDSZ too
            if (idsz == IDSZ_NONE
                    || (excludeidsz != (capidsz[chr[cnt].model][IDSZ_PARENT] == idsz))
                    || (excludeidsz != (capidsz[chr[cnt].model][IDSZ_TYPE]   == idsz)) )
            {
                Uint32 dist = ( Uint32 ) SQRT(ABS( pow(chr[cnt].xpos - chr[character].xpos, 2))
                                              + ABS( pow(chr[cnt].ypos - chr[character].ypos, 2))
                                              + ABS( pow(chr[cnt].zpos - chr[character].zpos, 2)) );
                if (dist < longdist && (maxdistance == 0 || dist < maxdistance) )
                {
                    besttarget = cnt;
                    longdist = dist;
                }
            }
        }
    }

    //Now set the target
    if (besttarget != MAXCHR)
    {
        chr[character].ai.target = besttarget;
    }

    return besttarget;
}

//--------------------------------------------------------------------------------------------
void set_alerts( Uint16 character )
{
    // ZZ> This function polls some alert conditions

    // invalid characters do not think
    if ( !chr[character].on ) return;

    // mounts do not get to think for themselves
    if ( MAXCHR != chr[character].attachedto ) return;
    if ( chr[character].ai.wp_tail != chr[character].ai.wp_head )
    {
        if ( chr[character].xpos < chr[character].ai.wp_pos_x[chr[character].ai.wp_tail] + WAYTHRESH &&
                chr[character].xpos > chr[character].ai.wp_pos_x[chr[character].ai.wp_tail] - WAYTHRESH &&
                chr[character].ypos < chr[character].ai.wp_pos_y[chr[character].ai.wp_tail] + WAYTHRESH &&
                chr[character].ypos > chr[character].ai.wp_pos_y[chr[character].ai.wp_tail] - WAYTHRESH )
        {
            chr[character].ai.alert |= ALERTIF_ATWAYPOINT;
            chr[character].ai.wp_tail++;
            if ( chr[character].ai.wp_tail > MAXWAY - 1 ) chr[character].ai.wp_tail = MAXWAY - 1;
        }
        if ( chr[character].ai.wp_tail >= chr[character].ai.wp_head )
        {
            // !!!!restart the waypoint list, do not clear them!!!!
            chr[character].ai.wp_tail    = 0;

            // if the object can be alerted to last waypoint, do it
            if ( !capisequipment[chr[character].model] )
            {
                chr[character].ai.alert |= ALERTIF_ATLASTWAYPOINT;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t add_quest_idsz(  const char *whichplayer, IDSZ idsz )
{
    /// @details ZF@> This function writes a IDSZ (With quest level 0) into a player quest.txt file, returns btrue if succeeded

    FILE *filewrite;
    STRING newloadname;

    // Only add quest IDSZ if it doesnt have it already
    if (check_player_quest(whichplayer, idsz) >= QUEST_BEATEN) return bfalse;

    // Try to open the file in read and append mode
    snprintf(newloadname, sizeof(newloadname), "players/%s/quest.txt", get_file_path(whichplayer) );
    filewrite = fopen( newloadname, "a" );
    if ( !filewrite )
    {
        //Create the file if it does not exist
        filewrite = fopen( newloadname, "w" );
        if (!filewrite)
        {
            log_warning("Cannot write to %s!\n", newloadname);
            return bfalse;
        }

        fprintf( filewrite, "//This file keeps order of all the quests for the player (%s)\n", whichplayer);
        fprintf( filewrite, "//The number after the IDSZ shows the quest level. -1 means it is completed.");
    }

    fprintf( filewrite, "\n:[%4s] 0", undo_idsz( idsz ));
    fclose( filewrite );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Sint16 modify_quest_idsz(  const char *whichplayer, IDSZ idsz, Sint16 adjustment )
{
    /// @details ZF@> This function increases or decreases a Quest IDSZ quest level by the amount determined in
    ///     adjustment. It then returns the current quest level it now has.
    ///     It returns QUEST_NONE if failed and if the adjustment is 0, the quest is marked as beaten...

    FILE *filewrite, *fileread;
    STRING newloadname, copybuffer;
    IDSZ newidsz;
    Sint8 NewQuestLevel = QUEST_NONE, QuestLevel;

    //Now check each expansion until we find correct IDSZ
    if (check_player_quest(whichplayer, idsz) <= QUEST_BEATEN || adjustment == 0)  return NewQuestLevel;
    else
    {
        // modify the CData.quest_file
        char ctmp;

        // create a "tmp_*" copy of the file
        snprintf( newloadname, sizeof( newloadname ), "players/%s/quest.txt", get_file_path(whichplayer));
        snprintf( copybuffer, sizeof( copybuffer ), "players/%s/tmp_quest.txt", whichplayer);
        fs_copyFile( newloadname, copybuffer );

        // open the tmp file for reading and overwrite the original file
        fileread  = fopen( copybuffer, "r" );
        filewrite = fopen( newloadname, "w" );

        //Something went wrong
        if (!fileread || !filewrite)
        {
            log_warning("Could not modify quest IDSZ (%s).\n", newloadname);
            return QUEST_NONE;
        }

        // read the tmp file line-by line
        while ( !feof(fileread) )
        {
            ctmp = fgetc(fileread);
            ungetc(ctmp, fileread);
            if ( ctmp == '/' )
            {
                // copy comments exactly
                fcopy_line(fileread, filewrite);
            }
            else if ( goto_colon_yesno( fileread ) )
            {
                // scan the line for quest info
                newidsz = get_idsz( fileread );
                get_first_letter( fileread );      //Skip the ] bracket
                QuestLevel = fget_int( fileread );

                // modify it
                if ( newidsz == idsz )
                {
                    QuestLevel += adjustment;
                    if (QuestLevel < 0) QuestLevel = 0;   //Don't get negative

                    NewQuestLevel = QuestLevel;
                }

                fprintf(filewrite, "\n:[%s] %i", undo_idsz(newidsz), QuestLevel);
            }
        }
    }

    //clean it up
    fclose( fileread );
    fclose( filewrite );
    fs_deleteFile( copybuffer );

    return NewQuestLevel;
}

//--------------------------------------------------------------------------------------------
char * undo_idsz( IDSZ idsz )
{
    // ZZ> This function takes an integer and makes a text IDSZ out of it.

    static char value_string[5] = {"NONE"};
    if ( idsz == IDSZ_NONE )
    {
        sprintf( idsz_string, "NONE" );
        snprintf( value_string, sizeof( value_string ), "NONE" );
    }
    else
    {
        idsz_string[0] = ( ( idsz >> 15 ) & 31 ) + 'A';
        idsz_string[1] = ( ( idsz >> 10 ) & 31 ) + 'A';
        idsz_string[2] = ( ( idsz >> 5 ) & 31 ) + 'A';
        idsz_string[3] = ( ( idsz ) & 31 ) + 'A';
        idsz_string[4] = 0;

        //Bad! both function return and return to global variable!
        value_string[0] = (( idsz >> 15 ) & 31 ) + 'A';
        value_string[1] = (( idsz >> 10 ) & 31 ) + 'A';
        value_string[2] = (( idsz >> 5 ) & 31 ) + 'A';
        value_string[3] = (( idsz ) & 31 ) + 'A';
        value_string[4] = 0;
    }

    return value_string;
}

//--------------------------------------------------------------------------------------------
Sint16 check_player_quest(  const char *whichplayer, IDSZ idsz )
{
    /// @details ZF@> This function checks if the specified player has the IDSZ in his or her quest.txt
    /// and returns the quest level of that specific quest (Or QUEST_NONE if it is not found, QUEST_BEATEN if it is finished)

    FILE *fileread;
    STRING newloadname;
    IDSZ newidsz;
    bool_t foundidsz = bfalse;
    Sint8 result = QUEST_NONE;

    snprintf( newloadname, sizeof(newloadname), "players/%s/quest.txt", get_file_path(whichplayer) );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread ) return result;

    //Always return "true" for [NONE] IDSZ checks
    if (idsz == IDSZ_NONE) result = QUEST_BEATEN;

    // Check each expansion
    while ( !foundidsz && goto_colon_yesno( fileread ) )
    {
        newidsz = get_idsz( fileread );
        if ( newidsz == idsz )
        {
            foundidsz = btrue;
            get_first_letter( fileread );   //Skip the ] bracket
            result = fget_int( fileread );  //Read value behind colon and IDSZ
        }
    }

    fclose( fileread );

    return result;
}

//--------------------------------------------------------------------------------------------
int check_skills( Uint16 who, IDSZ whichskill )
{
    // @details ZF@> This checks if the specified character has the required skill. Returns the level
    // of the skill. Also checks Skill expansions.

    bool_t result = bfalse;

    // First check the character Skill ID matches
    // Then check for expansion skills too.
    if ( capidsz[chr[who].model][IDSZ_SKILL]  == whichskill ) result = btrue;
    else if ( Make_IDSZ( "AWEP" ) == whichskill ) result = chr[who].canuseadvancedweapons;
    else if ( Make_IDSZ( "CKUR" ) == whichskill ) result = chr[who].canseekurse;
    else if ( Make_IDSZ( "JOUS" ) == whichskill ) result = chr[who].canjoust;
    else if ( Make_IDSZ( "SHPR" ) == whichskill ) result = chr[who].shieldproficiency;
    else if ( Make_IDSZ( "TECH" ) == whichskill ) result = chr[who].canusetech;
    else if ( Make_IDSZ( "WMAG" ) == whichskill ) result = chr[who].canusearcane;
    else if ( Make_IDSZ( "HMAG" ) == whichskill ) result = chr[who].canusedivine;
    else if ( Make_IDSZ( "DISA" ) == whichskill ) result = chr[who].candisarm;
    else if ( Make_IDSZ( "STAB" ) == whichskill ) result = chr[who].canbackstab;
    else if ( Make_IDSZ( "POIS" ) == whichskill ) result = chr[who].canusepoison;
    else if ( Make_IDSZ( "READ" ) == whichskill ) result = chr[who].canread;

    return result;
}
