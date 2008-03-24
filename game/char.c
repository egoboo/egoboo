/* Egoboo - char.c
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

#include "egoboo_math.h"
#include "Network.h"
#include "Client.h"
#include "Server.h"
#include "Log.h"
#include "egoboo.h"
#include "mesh.h"
#include "egoboo_strutil.h"

#include <assert.h>

Uint32  numfanblock;                                    // Number of collision areas
Uint16  bumplistchr[MAXMESHFAN/16];                     // For character collisions
Uint16  bumplistchrnum[MAXMESHFAN/16];                  // Number on the block
Uint16  bumplistprt[MAXMESHFAN/16];                     // For particle collisions
Uint16  bumplistprtnum[MAXMESHFAN/16];                  // Number on the block

Uint32  cv_list_count = 0;
CVolume cv_list[1000];

void cv_list_add( CVolume * cv)
{
  if(NULL == cv || cv_list_count > 1000) return;
  cv_list[cv_list_count++] = *cv;
};

void cv_list_clear()
{
  cv_list_count = 0;
};

//--------------------------------------------------------------------------------------------
CVolume cvolume_merge(CVolume * pv1, CVolume * pv2)
{
  CVolume rv;

  rv.level = -1;

  if(NULL==pv1 && NULL==pv2)
  {
    return rv;
  }
  else if(NULL==pv2)
  {
    return *pv1;
  }
  else if(NULL==pv1)
  {
    return *pv2;
  }
  else
  {
    bool_t binvalid;

    // check for uninitialized volumes
    if(-1==pv1->level && -1==pv2->level)
    {
      return rv;
    }
    else if(-1==pv1->level)
    {
      return *pv2;
    }
    else if(-1==pv2->level)
    {
      return *pv1;
    };

    // merge the volumes

    rv.x_min = MIN(pv1->x_min, pv2->x_min);
    rv.x_max = MAX(pv1->x_max, pv2->x_max);

    rv.y_min = MIN(pv1->y_min, pv2->y_min);
    rv.y_max = MAX(pv1->y_max, pv2->y_max);

    rv.z_min = MIN(pv1->z_min, pv2->z_min);
    rv.z_max = MAX(pv1->z_max, pv2->z_max);

    rv.xy_min = MIN(pv1->xy_min, pv2->xy_min);
    rv.xy_max = MAX(pv1->xy_max, pv2->xy_max);

    rv.yx_min = MIN(pv1->yx_min, pv2->yx_min);
    rv.yx_max = MAX(pv1->yx_max, pv2->yx_max);

    // check for an invalid volume
    binvalid = (rv.x_min >= rv.x_max) || (rv.y_min >= rv.y_max) || (rv.z_min >= rv.z_max);
    binvalid = binvalid ||(rv.xy_min >= rv.xy_max) || (rv.yx_min >= rv.yx_max);

    rv.level = binvalid ? -1 : 1;
  }

  return rv;
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void flash_character_height( CHR_REF character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
  // ZZ> This function sets a character's lighting depending on vertex height...
  //     Can make feet dark and head light...
  int cnt;
  Uint16 model;
  float z, flip;

  Uint32 ilast, inext;
  MD2_Model * pmdl;
  MD2_Frame * plast, * pnext;

  model = chrmodel[character];
  inext = chrframe[character];
  ilast = chrframelast[character];
  flip = chrflip[character];

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  pmdl  = mad_md2[model];
  plast = md2_get_Frame(pmdl, ilast);
  pnext = md2_get_Frame(pmdl, inext);
  
  for ( cnt = 0; cnt < madtransvertices[model]; cnt++ )
  {
    z = pnext->vertices[cnt].z + (pnext->vertices[cnt].z - plast->vertices[cnt].z) * flip;

    if ( z < low )
    {
      chrvrtar_fp8[character][cnt] =
      chrvrtag_fp8[character][cnt] =
      chrvrtab_fp8[character][cnt] = valuelow;
    }
    else if ( z > high )
    {
      chrvrtar_fp8[character][cnt] =
      chrvrtag_fp8[character][cnt] =
      chrvrtab_fp8[character][cnt] = valuehigh;
    }
    else
    {
      float ftmp = (float)( z - low ) / (float)( high - low );
      chrvrtar_fp8[character][cnt] =
      chrvrtag_fp8[character][cnt] =
      chrvrtab_fp8[character][cnt] = valuelow + (valuehigh - valuelow) * ftmp;
    }
  }
}

//--------------------------------------------------------------------------------------------
void flash_character( CHR_REF character, Uint8 value )
{
  // ZZ> This function sets a character's lighting
  int cnt;
  Uint16 model = chrmodel[character];

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  cnt = 0;
  while ( cnt < madtransvertices[model] )
  {
    chrvrtar_fp8[character][cnt] =
      chrvrtag_fp8[character][cnt] =
        chrvrtab_fp8[character][cnt] = value;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void add_to_dolist( CHR_REF ichr )
{
  // This function puts a character in the list
  int fan;

  if ( !VALID_CHR( ichr ) || chrindolist[ichr] ) return;

  fan = chronwhichfan[ichr];
  //if ( mesh_in_renderlist( fan ) )
  {
    //chrlightspek_fp8[ichr] = meshvrtl_fp8[meshvrtstart[fan]];
    dolist[numdolist] = ichr;
    chrindolist[ichr] = btrue;
    numdolist++;


    // Do flashing
    if (( allframe&chrflashand[ichr] ) == 0 && chrflashand[ichr] != DONTFLASH )
    {
      flash_character( ichr, 255 );
    }
    // Do blacking
    if (( allframe&SEEKURSEAND ) == 0 && localseekurse && chriskursed[ichr] )
    {
      flash_character( ichr, 0 );
    }
  }
  //else
  //{
  //  Uint16 model = chrmodel[ichr];
  //  assert( MAXMODEL != VALIDATE_MDL( model ) );

  //  // Double check for large/special objects
  //  if ( capalwaysdraw[model] )
  //  {
  //    dolist[numdolist] = ichr;
  //    chrindolist[ichr] = btrue;
  //    numdolist++;
  //  }
  //}

  // Add its weapons too
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    add_to_dolist( chr_get_holdingwhich( ichr, _slot ) );
  };

}

//--------------------------------------------------------------------------------------------
void order_dolist( void )
{
  // ZZ> This function orders the dolist based on distance from camera,
  //     which is needed for reflections to properly clip themselves.
  //     Order from closest to farthest
  int cnt, tnc, character, order;
  int dist[MAXCHR];
  Uint16 olddolist[MAXCHR];


  // Figure the distance of each
  cnt = 0;
  while ( cnt < numdolist )
  {
    character = dolist[cnt];  olddolist[cnt] = character;
    if ( chrlight_fp8[character] != 255 || chralpha_fp8[character] != 255 )
    {
      // This makes stuff inside an invisible character visible...
      // A key inside a Jellcube, for example
      dist[cnt] = 0x7fffffff;
    }
    else
    {
      dist[cnt] = ABS( chrpos[character].x - campos.x ) + ABS( chrpos[character].y - campos.y );
    }
    cnt++;
  }


  // Put em in the right order
  cnt = 0;
  while ( cnt < numdolist )
  {
    character = olddolist[cnt];
    order = 0;  // Assume this character is closest
    tnc = 0;
    while ( tnc < numdolist )
    {
      // For each one closer, increment the order
      order += ( dist[cnt] > dist[tnc] );
      order += ( dist[cnt] == dist[tnc] ) && ( cnt < tnc );
      tnc++;
    }
    dolist[order] = character;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_dolist( void )
{
  // ZZ> This function finds the characters that need to be drawn and puts them in the list
  int cnt, character;


  // Remove everyone from the dolist
  cnt = 0;
  while ( cnt < numdolist )
  {
    character = dolist[cnt];
    chrindolist[character] = bfalse;
    cnt++;
  }
  numdolist = 0;


  // Now fill it up again
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( chron[cnt] && !chr_in_pack( cnt ) )
    {
      // Add the character
      add_to_dolist( cnt );
    }
    cnt++;
  }

}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
  // ZZ> This function keeps weapons near their holders
  int cnt, character;


  // !!!BAD!!!  May need to do 3 levels of attachment...

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( cnt ) ) continue;

    character = chr_get_attachedto( cnt );
    if ( !VALID_CHR( character ) )
    {
      // Keep inventory with character
      if ( !chr_in_pack( cnt ) )
      {
        character = chr_get_nextinpack( cnt );
        while ( VALID_CHR( character ) )
        {
          chrpos[character] = chrpos[cnt];
          chrpos_old[character] = chrpos_old[cnt];  // Copy olds to make SendMessageNear work
          character  = chr_get_nextinpack( character );
        }
      }
    }
    else
    {
      // Keep in hand weapons with character
      if ( chrmatrixvalid[character] && chrmatrixvalid[cnt] )
      {
        chrpos[cnt].x = chrmatrix[cnt]_CNV( 3, 0 );
        chrpos[cnt].y = chrmatrix[cnt]_CNV( 3, 1 );
        chrpos[cnt].z = chrmatrix[cnt]_CNV( 3, 2 );
      }
      else
      {
        chrpos[cnt].x = chrpos[character].x;
        chrpos[cnt].y = chrpos[character].y;
        chrpos[cnt].z = chrpos[character].z;
      }
      chrturn_lr[cnt] = chrturn_lr[character];

      // Copy this stuff ONLY if it's a weapon, not for mounts
      if ( chrtransferblend[character] && chrisitem[cnt] )
      {
        if ( chralpha_fp8[character] != 255 )
        {
          Uint16 model = chrmodel[cnt];
          assert( MAXMODEL != VALIDATE_MDL( model ) );
          chralpha_fp8[cnt] = chralpha_fp8[character];
          chrbumpstrength[cnt] = capbumpstrength[model] * FP8_TO_FLOAT( chralpha_fp8[cnt] );
        }
        if ( chrlight_fp8[character] != 255 )
        {
          chrlight_fp8[cnt] = chrlight_fp8[character];
        }
      }
    }

  }
}

//--------------------------------------------------------------------------------------------
void make_turntosin( void )
{
  // ZZ> This function makes the lookup table for chrturn...
  int cnt;

  for ( cnt = 0; cnt < TRIGTABLE_SIZE; cnt++ )
  {
    turntosin[cnt] = sin(( TWO_PI * cnt ) / ( float ) TRIGTABLE_SIZE );
  }
}

//--------------------------------------------------------------------------------------------
bool_t matrix_compare_3x3(GLmatrix * pm1, GLmatrix * pm2)
{
  // BB > compare two 3x3 matrices to see if the transformation is the same

  int i,j;

  for(i=0;i<3;i++)
  {
    for(j=0;j<3;j++)
    {
      if( (*pm1)_CNV(i,j) != (*pm2)_CNV(i,j) ) bfalse;
    };
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t make_one_character_matrix( CHR_REF cnt )
{
  // ZZ> This function sets one character's matrix
  Uint16 tnc;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  if ( !VALID_CHR( cnt ) ) return bfalse;

  mat_old = chrmatrix[cnt];
  chrmatrixvalid[cnt] = bfalse;

  if ( chroverlay[cnt] )
  {
    // Overlays are kept with their target...
    tnc = chr_get_aitarget( cnt );

    if ( VALID_CHR( tnc ) )
    {
      chrpos[cnt].x = chrmatrix[tnc]_CNV( 3, 0 );
      chrpos[cnt].y = chrmatrix[tnc]_CNV( 3, 1 );
      chrpos[cnt].z = chrmatrix[tnc]_CNV( 3, 2 );

      chrmatrix[cnt] = chrmatrix[tnc];

      chrmatrix[cnt]_CNV( 0, 0 ) *= chrpancakepos[cnt].x;
      chrmatrix[cnt]_CNV( 1, 0 ) *= chrpancakepos[cnt].x;
      chrmatrix[cnt]_CNV( 2, 0 ) *= chrpancakepos[cnt].x;

      chrmatrix[cnt]_CNV( 0, 1 ) *= chrpancakepos[cnt].y;
      chrmatrix[cnt]_CNV( 1, 1 ) *= chrpancakepos[cnt].y;
      chrmatrix[cnt]_CNV( 2, 1 ) *= chrpancakepos[cnt].y;

      chrmatrix[cnt]_CNV( 0, 2 ) *= chrpancakepos[cnt].z;
      chrmatrix[cnt]_CNV( 1, 2 ) *= chrpancakepos[cnt].z;
      chrmatrix[cnt]_CNV( 2, 2 ) *= chrpancakepos[cnt].z;

      chrmatrixvalid[cnt] = btrue;

      recalc_bumper = matrix_compare_3x3(&mat_old, &chrmatrix[cnt]);
    }
  }
  else
  {
    chrmatrix[cnt] = ScaleXYZRotateXYZTranslate( chrscale[cnt] * chrpancakepos[cnt].x, chrscale[cnt] * chrpancakepos[cnt].y, chrscale[cnt] * chrpancakepos[cnt].z,
                     chrturn_lr[cnt] >> 2,
                     (( Uint16 )( chrmapturn_ud[cnt] + 32768 ) ) >> 2,
                     (( Uint16 )( chrmapturn_lr[cnt] + 32768 ) ) >> 2,
                     chrpos[cnt].x, chrpos[cnt].y, chrpos[cnt].z );

    chrmatrixvalid[cnt] = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &chrmatrix[cnt]);
  }

  if(chrmatrixvalid[cnt] && recalc_bumper)
  {
    // invalidate the cached bumper data
    chrbmpdata[cnt].cv.level = -1;
  };

  return chrmatrixvalid[cnt];
}

//--------------------------------------------------------------------------------------------
void free_one_character( CHR_REF ichr )
{
  // ZZ> This function sticks a ichr back on the free ichr stack
  int cnt;

  if ( !VALID_CHR( ichr ) ) return;

  fprintf( stdout, "free_one_character() - \n\tprofile == %d, capclassname[profile] == \"%s\", index == %d\n", chrmodel[ichr], capclassname[chrmodel[ichr]], ichr );

  //remove any collision volume octree
  if(NULL != chrbmpdata[ichr].cv_tree)
  {
    free( chrbmpdata[ichr].cv_tree );
    chrbmpdata[ichr].cv_tree = NULL;
  }

  // add it to the free list
  freechrlist[numfreechr] = ichr;
  numfreechr++;

  // Remove from stat list
  if ( chrstaton[ichr] )
  {
    chrstaton[ichr] = bfalse;
    cnt = 0;
    while ( cnt < numstat )
    {
      if ( statlist[cnt] == ichr )
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
  assert( MAXMODEL != VALIDATE_MDL( chrmodel[ichr] ) );
  if ( chralive[ichr] && !capinvictus[chrmodel[ichr]] )
  {
    teammorale[chrbaseteam[ichr]]--;
  }

  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( chron[cnt] )
    {
      if ( chraitarget[cnt] == ichr )
      {
        chralert[cnt] |= ALERT_TARGETKILLED;
        chraitarget[cnt] = cnt;
      }
      if ( team_get_leader( chrteam[cnt] ) == ichr )
      {
        chralert[cnt] |= ALERT_LEADERKILLED;
      }
    }
    cnt++;
  }

  if ( team_get_leader( chrteam[ichr] ) == ichr )
  {
    teamleader[chrteam[ichr]] = MAXCHR;
  }

  if( INVALID_CHANNEL != chrloopingchannel[ichr] )
  {
    stop_sound( chrloopingchannel[ichr] );
    chrloopingchannel[ichr] = INVALID_CHANNEL;
  };

  chron[ichr] = bfalse;
  chralive[ichr] = bfalse;
  chrinwhichpack[ichr] = MAXCHR;
  VData_Blended_Deallocate(&chrvdata[ichr]);
}

//--------------------------------------------------------------------------------------------
void free_inventory( CHR_REF character )
{
  // ZZ> This function frees every item in the character's inventory
  int cnt, next;

  cnt  = chr_get_nextinpack( character );
  while ( cnt < MAXCHR )
  {
    next  = chr_get_nextinpack( cnt );
    chrfreeme[cnt] = btrue;
    cnt = next;
  }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( PRT_REF particle, CHR_REF character, Uint16 vertoffset )
{
  // ZZ> This function sets one particle's position to be attached to a character.
  //     It will kill the particle if the character is no longer around
  Uint16 vertex, model;
  float flip;
  GLvector point, nupoint;


  // Check validity of attachment
  if ( !VALID_CHR( character ) || chr_in_pack( character ) )
  {
    prtgopoof[particle] = btrue;
    return;
  }


  // Do we have a matrix???
  if ( chrmatrixvalid[character] )
  {
    // Transform the weapon grip from model to world space

    if ( vertoffset == GRIP_ORIGIN )
    {
      prtpos[particle].x = chrmatrix[character]_CNV( 3, 0 );
      prtpos[particle].y = chrmatrix[character]_CNV( 3, 1 );
      prtpos[particle].z = chrmatrix[character]_CNV( 3, 2 );
    }
    else
    {
      Uint32 ilast, inext;
      MD2_Model * pmdl;
      MD2_Frame * plast, * pnext;

      model = chrmodel[character];
      inext = chrframe[character];
      ilast = chrframelast[character];
      flip = chrflip[character];

      assert( MAXMODEL != VALIDATE_MDL( model ) );

      pmdl  = mad_md2[model];
      plast = md2_get_Frame(pmdl, ilast);
      pnext = md2_get_Frame(pmdl, inext);

      //handle possible invalid values
      vertex = ( madvertices[model] >= vertoffset ) ? madvertices[model] - vertoffset : madvertices[model] - GRIP_LAST;

      // Calculate grip point locations with linear interpolation and other silly things
      if ( inext == ilast )
      {
        point.x = plast->vertices[vertex].x;
        point.y = plast->vertices[vertex].y;
        point.z = plast->vertices[vertex].z;
        point.w = 1;
      }
      else
      {
        point.x = plast->vertices[vertex].x + ( pnext->vertices[vertex].x - plast->vertices[vertex].x ) * flip;
        point.y = plast->vertices[vertex].y + ( pnext->vertices[vertex].y - plast->vertices[vertex].y ) * flip;
        point.z = plast->vertices[vertex].z + ( pnext->vertices[vertex].z - plast->vertices[vertex].z ) * flip;
        point.w = 1;
      }

      // Do the transform
      Transform4_Full( &chrmatrix[character], &point, &nupoint, 1 );

      prtpos[particle].x = nupoint.x;
      prtpos[particle].y = nupoint.y;
      prtpos[particle].z = nupoint.z;
    }
  }
  else
  {
    // No matrix, so just wing it...
    prtpos[particle].x = chrpos[character].x;
    prtpos[particle].y = chrpos[character].y;
    prtpos[particle].z = chrpos[character].z;
  }
}

//--------------------------------------------------------------------------------------------
bool_t make_one_weapon_matrix( CHR_REF ichr )
{
  // ZZ> This function sets one weapon's matrix, based on who it's attached to
  int cnt;
  CHR_REF imount;
  Uint16 vertex;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  // check this character
  if ( !VALID_CHR( ichr ) )  return bfalse;

  // invalidate the matrix
  chrmatrixvalid[ichr] = bfalse;

  // check that the mount is valid
  imount = chr_get_attachedto( ichr );
  if ( !VALID_CHR( imount ) ) 
  {
    chrmatrix[ichr] = ZeroMatrix();
    return bfalse;
  }

  mat_old = chrmatrix[ichr];

  if(0xFFFF == chrattachedgrip[ichr][0])
  {
    // Calculate weapon's matrix
    chrmatrix[ichr] = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, chrturn_lr[ichr] + chrturn_lr[imount], chrpos[imount].x, chrpos[imount].y, chrpos[imount].z);
    chrmatrixvalid[ichr] = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &chrmatrix[ichr]);
  }
  else if(0xFFFF == chrattachedgrip[ichr][1])
  {
    // do the linear interpolation  
    vertex = chrattachedgrip[ichr][0];
    md2_blend_vertices(imount, vertex, vertex);

    // Calculate weapon's matrix
    chrmatrix[ichr] = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, chrturn_lr[ichr] + chrturn_lr[imount], chrvdata[imount].Vertices[vertex].x, chrvdata[imount].Vertices[vertex].y, chrvdata[imount].Vertices[vertex].z);
    chrmatrixvalid[ichr] = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &chrmatrix[ichr]);
  }
  else
  {
    GLvector point[GRIP_SIZE], nupoint[GRIP_SIZE];

    // do the linear interpolation
    vertex = chrattachedgrip[ichr][0];
    md2_blend_vertices(imount, vertex, vertex+GRIP_SIZE);
    
    for ( cnt = 0; cnt < GRIP_SIZE; cnt++ )
    {
      point[cnt].x = chrvdata[imount].Vertices[vertex+cnt].x;
      point[cnt].y = chrvdata[imount].Vertices[vertex+cnt].y;
      point[cnt].z = chrvdata[imount].Vertices[vertex+cnt].z;
      point[cnt].w = 1.0f; 
    };

    // Do the transform
    Transform4_Full( &chrmatrix[imount], point, nupoint, GRIP_SIZE );

    // Calculate weapon's matrix based on positions of grip points
    // chrscale is recomputed at time of attachment
    chrmatrix[ichr] = FourPoints( nupoint[0], nupoint[1], nupoint[2], nupoint[3], 1.0 );
    chrpos[ichr].x = (chrmatrix[ichr])_CNV(3,0);
    chrpos[ichr].y = (chrmatrix[ichr])_CNV(3,1);
    chrpos[ichr].z = (chrmatrix[ichr])_CNV(3,2);
    chrmatrixvalid[ichr] = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &chrmatrix[ichr]);
  };

  if(chrmatrixvalid[ichr] && recalc_bumper)
  {
    // invalidate the cached bumper data
    md2_calculate_bumpers(ichr, 0);
  };

  return chrmatrixvalid[ichr];
}

//--------------------------------------------------------------------------------------------
void make_character_matrices()
{
  // ZZ> This function makes all of the character's matrices
  CHR_REF ichr;
  bool_t  bfinished;

  // Forget about old matrices
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    chrmatrixvalid[ichr] = bfalse;
  }

  // Do base characters
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    CHR_REF attached;
    if ( !VALID_CHR( ichr ) ) continue;

    attached = chr_get_attachedto( ichr );
    if ( VALID_CHR( attached ) ) continue;  // Skip weapons for now

    make_one_character_matrix( ichr );
  }


  // Do all other levels of attachment
  bfinished = bfalse;
  while ( !bfinished )
  {
    bfinished = btrue;
    for ( ichr = 0; ichr < MAXCHR; ichr++ )
    {
      CHR_REF attached;
      if ( chrmatrixvalid[ichr] || !VALID_CHR( ichr ) ) continue;

      attached = chr_get_attachedto( ichr );
      if ( !VALID_CHR( attached ) ) continue;

      if ( !chrmatrixvalid[attached] )
      {
        bfinished = bfalse;
        continue;
      }

      make_one_weapon_matrix( ichr );
    }
  };

}

//--------------------------------------------------------------------------------------------
int get_free_character()
{
  // ZZ> This function gets an unused character and returns its index
  CHR_REF character;


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
bool_t prt_search_target_in_block( int block_x, int block_y, float prtx, float prty, Uint16 facing,
                                   bool_t request_friends, bool_t allow_anyone, TEAM team,
                                   Uint16 donttarget, Uint16 oldtarget )
{
  // ZZ> This function helps find a target, returning btrue if it found a decent target
  int cnt;
  Uint16 local_angle;
  CHR_REF charb;
  bool_t bfound, request_enemies = !request_friends;
  Uint32 fanblock;
  int local_distance;

  bfound = bfalse;

  // Current fanblock
  fanblock = mesh_convert_block( block_x, block_y );
  if ( INVALID_FAN == fanblock ) return bfound;

  for ( cnt = 0, charb = VALID_CHR( bumplistchr[fanblock] ); cnt < bumplistchrnum[fanblock] && VALID_CHR( charb ); cnt++, charb = chr_get_bumpnext( charb ) )
  {
    // don't find stupid stuff
    if ( !VALID_CHR( charb ) || 0.0f == chrbumpstrength[charb] ) continue;

    if ( !chralive[charb] || chrinvictus[charb] || chr_in_pack( charb ) ) continue;

    if ( charb == donttarget || charb == oldtarget ) continue;

    if ( allow_anyone || ( request_friends && !teamhatesteam[team][chrteam[charb]] ) || ( request_enemies && teamhatesteam[team][chrteam[charb]] ) )
    {
      local_distance = ABS( chrpos[charb].x - prtx ) + ABS( chrpos[charb].y - prty );
      if ( local_distance < search_bestdistance )
      {
        local_angle = facing - vec_to_turn( chrpos[charb].x - prtx, chrpos[charb].y - prty );
        if ( local_angle < search_bestangle || local_angle > ( 65535 - search_bestangle ) )
        {
          bfound = btrue;
          search_besttarget   = charb;
          search_bestdistance = local_distance;
          search_useangle     = local_angle;
          if ( local_angle  > 32767 )
            search_bestangle = UINT16_SIZE - local_angle;
          else
            search_bestangle = local_angle;
        }
      }
    }
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
CHR_REF prt_search_target( float prtx, float prty, Uint16 facing,
                           Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                           TEAM team, Uint16 donttarget, Uint16 oldtarget )
{
  // This function finds the best target for the given parameters
  bool_t done, btmp;
  int block_x, block_y;

  block_x = MESH_FLOAT_TO_BLOCK( prtx );
  block_y = MESH_FLOAT_TO_BLOCK( prty );
  search_besttarget   = MAXCHR;
  search_bestdistance = 9999;
  search_bestangle    = targetangle;
  done = bfalse;

  prt_search_target_in_block( block_x + 0, block_y + 0, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  if ( VALID_CHR( search_besttarget ) ) return search_besttarget;

  prt_search_target_in_block( block_x + 1, block_y + 0, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  prt_search_target_in_block( block_x - 1, block_y + 0, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  prt_search_target_in_block( block_x + 0, block_y + 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  prt_search_target_in_block( block_x + 0, block_y - 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  if ( VALID_CHR( search_besttarget ) ) return search_besttarget;

  btmp = prt_search_target_in_block( block_x + 1, block_y + 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  btmp = prt_search_target_in_block( block_x + 1, block_y - 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  btmp = prt_search_target_in_block( block_x - 1, block_y + 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  btmp = prt_search_target_in_block( block_x - 1, block_y - 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );

  return search_besttarget;
}

//--------------------------------------------------------------------------------------------
void free_all_characters()
{
  // ZZ> This function resets the character allocation list
  CHR_REF cnt;
  bool_t do_initialization;

  do_initialization = (-1 == numfreechr);

  nolocalplayers = btrue;
  numfreechr = 0;
  while ( numfreechr < MAXCHR )
  {
    
    if(do_initialization)
    {
      // initialize a non-existant collision volume octree
      chrbmpdata[numfreechr].cv_tree = NULL;

      // initialize the looping sounds
      chrloopingchannel[numfreechr] = INVALID_CHANNEL;
    }
    else if(NULL != chrbmpdata[numfreechr].cv_tree)
    {
      // remove existing collision volume octree
      free( chrbmpdata[numfreechr].cv_tree );
      chrbmpdata[numfreechr].cv_tree = NULL;

      // silence all looping sounds
      if( INVALID_CHANNEL != chrloopingchannel[numfreechr] )
      {
        stop_sound( chrloopingchannel[numfreechr] );
        chrloopingchannel[numfreechr] = INVALID_CHANNEL;
      };
    }

    // reset some values
    chron[numfreechr] = bfalse;
    chralive[numfreechr] = bfalse;
    chrstaton[numfreechr] = bfalse;
    chrmatrixvalid[numfreechr] = bfalse;
    chrmodel[numfreechr] = MAXMODEL;
    VData_Blended_Deallocate(&chrvdata[numfreechr]);
    chrname[numfreechr][0] = '\0';
    
    // invalidate pack
    chrnuminpack[numfreechr] = 0;
    chrinwhichpack[numfreechr] = MAXCHR;
    chrnextinpack[numfreechr] = MAXCHR;

    // invalidate attachmants
    chrinwhichslot[numfreechr] = SLOT_NONE;
    chrattachedto[numfreechr] = MAXCHR;
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
      chrholdingwhich[numfreechr][cnt] = MAXCHR;
    };


    // set the free list
    freechrlist[numfreechr] = numfreechr;
    numfreechr++;
  }
  numpla = 0;
  numlocalpla = 0;
  numstat = 0;
}

//--------------------------------------------------------------------------------------------
Uint32 __chrhitawall( CHR_REF ichr, vect3 * norm )
{
  // ZZ> This function returns nonzero if the character hit a wall that the
  //     ichr is not allowed to cross

  Uint32 retval;
  vect3  pos, size;

  if ( !VALID_CHR( ichr ) || 0.0f == chrbumpstrength[ichr] ) return 0;

  pos.x = ( chrbmpdata[ichr].cv.x_max + chrbmpdata[ichr].cv.x_min ) * 0.5f;
  pos.y = ( chrbmpdata[ichr].cv.y_max + chrbmpdata[ichr].cv.y_min ) * 0.5f;
  pos.z =   chrbmpdata[ichr].cv.z_min;

  size.x = ( chrbmpdata[ichr].cv.x_max - chrbmpdata[ichr].cv.x_min ) * 0.5f;
  size.y = ( chrbmpdata[ichr].cv.y_max - chrbmpdata[ichr].cv.y_min ) * 0.5f;
  size.z = ( chrbmpdata[ichr].cv.z_max - chrbmpdata[ichr].cv.z_min ) * 0.5f;

  retval = mesh_hitawall( pos, size.x, size.y, chrstoppedby[ichr] );

  if( 0!=retval && NULL!=norm )
  {
    vect3 pos2;

    VectorClear( norm->v );

    pos2.x = pos.x + chrpos[ichr].x - chrpos_old[ichr].x;
    pos2.y = pos.y + chrpos[ichr].y - chrpos_old[ichr].y;
    pos2.z = pos.z + chrpos[ichr].z - chrpos_old[ichr].z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, chrstoppedby[ichr] ) )
    {
      return 0;
    }
   
    pos2.x = pos.x;
    pos2.y = pos.y + chrpos[ichr].y - chrpos_old[ichr].y;
    pos2.z = pos.z + chrpos[ichr].z - chrpos_old[ichr].z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, chrstoppedby[ichr] ) )
    {
      norm->x = -SGN(chrpos[ichr].x - chrpos_old[ichr].x);
    }

    pos2.x = pos.x + chrpos[ichr].x - chrpos_old[ichr].x;
    pos2.y = pos.y;
    pos2.z = pos.z + chrpos[ichr].z - chrpos_old[ichr].z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, chrstoppedby[ichr] ) )
    {
      norm->y = -SGN(chrpos[ichr].y - chrpos_old[ichr].y);
    }

    pos2.x = pos.x + chrpos[ichr].x - chrpos_old[ichr].x;
    pos2.y = pos.y + chrpos[ichr].y - chrpos_old[ichr].y;
    pos2.z = pos.z;

    if( 0 != mesh_hitawall( pos, size.x, size.y, chrstoppedby[ichr] ) )
    {
      norm->z = -SGN(chrpos[ichr].z - chrpos_old[ichr].z);
    }

    if( ABS(norm->x) + ABS(norm->y) + ABS(norm->z) == 0.0f)
    {
      retval = 0;
      norm->z = 1.0f;
    }
    else
    {
      *norm = Normalize( *norm );
    };

  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel( CHR_REF character )
{
  // ZZ> This function fixes a character's MAX acceleration
  Uint16 enchant;

  if ( !VALID_CHR( character ) ) return;

  // Okay, remove all acceleration enchants
  enchant = chrfirstenchant[character];
  while ( enchant < MAXENCHANT )
  {
    remove_enchant_value( enchant, ADDACCEL );
    enchant = encnextenchant[enchant];
  }

  // Set the starting value
  assert( MAXMODEL != VALIDATE_MDL( chrmodel[character] ) );
  chrmaxaccel[character] = capmaxaccel[chrmodel[character]][( chrtexture[character] - madskinstart[chrmodel[character]] ) % MAXSKIN];

  // Put the acceleration enchants back on
  enchant = chrfirstenchant[character];
  while ( enchant < MAXENCHANT )
  {
    add_enchant_value( enchant, ADDACCEL, enceve[enchant] );
    enchant = encnextenchant[enchant];
  }

}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_mount( CHR_REF ichr, bool_t ignorekurse, bool_t doshop )
{
  // ZZ> This function drops an item
  Uint16 imount, iowner = MAXCHR;
  Uint16 enchant, passage;
  Uint16 cnt, price;
  bool_t inshop;



  // Make sure the ichr is valid
  if ( !VALID_CHR( ichr ) )
    return bfalse;


  // Make sure the ichr is mounted
  imount = chr_get_attachedto( ichr );
  if ( !VALID_CHR( imount ) )
    return bfalse;


  // Don't allow living characters to drop kursed weapons
  if ( !ignorekurse && chriskursed[ichr] && chralive[imount] )
  {
    chralert[ichr] |= ALERT_NOTDROPPED;
    return bfalse;
  }


  // Rip 'em apart
  _slot = chrinwhichslot[ichr];
  if(_slot == SLOT_INVENTORY)
  {
    chrattachedto[ichr] = MAXCHR;
    chrinwhichslot[ichr] = SLOT_NONE;
  }
  else
  {
    assert(_slot != SLOT_NONE);
    assert(ichr == chrholdingwhich[imount][_slot]);
    chrattachedto[ichr] = MAXCHR;
    chrinwhichslot[ichr] = SLOT_NONE;
    chrholdingwhich[imount][_slot] = MAXCHR;
  }


  chrscale[ichr] = chrfat[ichr]; // * madscale[chrmodel[ichr]] * 4;


  // Run the falling animation...
  play_action( ichr, ACTION_JB + ( chrinwhichslot[ichr] % 2 ), bfalse );

  // Set the positions
  if ( chrmatrixvalid[ichr] )
  {
    chrpos[ichr].x = chrmatrix[ichr]_CNV( 3, 0 );
    chrpos[ichr].y = chrmatrix[ichr]_CNV( 3, 1 );
    chrpos[ichr].z = chrmatrix[ichr]_CNV( 3, 2 );
  }
  else
  {
    chrpos[ichr].x = chrpos[imount].x;
    chrpos[ichr].y = chrpos[imount].y;
    chrpos[ichr].z = chrpos[imount].z;
  }



  // Make sure it's not dropped in a wall...
  if ( 0 != __chrhitawall( ichr, NULL ) )
  {
    chrpos[ichr].x = chrpos[imount].x;
    chrpos[ichr].y = chrpos[imount].y;
  }


  // Check for shop passages
  inshop = bfalse;
  if ( chrisitem[ichr] && numshoppassage != 0 && doshop )
  {
    for ( cnt = 0; cnt < numshoppassage; cnt++ )
    {
      passage = shoppassage[cnt];

      if ( passage_check_any( ichr, passage, NULL ) )
      {
        iowner = shopowner[passage];
        inshop = ( NOOWNER != iowner );
        break;
      }
    }

    if ( doshop && inshop )
    {
      Uint16 model = chrmodel[ichr];

      assert( MAXMODEL != VALIDATE_MDL( model ) );

      // Give the imount its money back, alert the shop iowner
      price = capskincost[model][( chrtexture[ichr] - madskinstart[model] ) % MAXSKIN];
      if ( capisstackable[model] )
      {
        price *= chrammo[ichr];
      }
      chrmoney[imount] += price;
      chrmoney[iowner] -= price;
      if ( chrmoney[iowner] < 0 )  chrmoney[iowner] = 0;
      if ( chrmoney[imount] > MAXMONEY )  chrmoney[imount] = MAXMONEY;

      chralert[iowner] |= ALERT_SIGNALED;
      chrmessage[iowner] = price;  // Tell iowner how much...
      chrmessagedata[iowner] = 0;  // 0 for buying an item
    }
  }

  // Make sure it works right
  chrhitready[ichr] = btrue;
  chralert[ichr]   |= ALERT_DROPPED;
  if ( inshop )
  {
    // Drop straight down to avoid theft
    chrvel[ichr].x = 0;
    chrvel[ichr].y = 0;
  }
  else
  {
    Uint16 sin_dir = RANDIE;
    chraccum_vel[ichr].x += chrvel[imount].x + 0.5 * DROPXYVEL * turntosin[(( sin_dir>>2 ) + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
    chraccum_vel[ichr].y += chrvel[imount].y + 0.5 * DROPXYVEL * turntosin[sin_dir>>2];
  }
  chraccum_vel[ichr].z += DROPZVEL;


  // Turn looping off
  chrloopaction[ichr] = bfalse;


  // Reset the team if it is a imount
  if ( chrismount[imount] )
  {
    chrteam[imount] = chrbaseteam[imount];
    chralert[imount] |= ALERT_DROPPED;
  }
  chrteam[ichr] = chrbaseteam[ichr];
  chralert[ichr] |= ALERT_DROPPED;


  // Reset transparency
  if ( chrisitem[ichr] && chrtransferblend[imount] )
  {
    Uint16 model = chrmodel[ichr];

    assert( MAXMODEL != VALIDATE_MDL( model ) );

    // Okay, reset transparency
    enchant = chrfirstenchant[ichr];
    while ( enchant < MAXENCHANT )
    {
      unset_enchant_value( enchant, SETALPHABLEND );
      unset_enchant_value( enchant, SETLIGHTBLEND );
      enchant = encnextenchant[enchant];
    }

    chralpha_fp8[ichr] = capalpha_fp8[model];
    chrbumpstrength[ichr] = capbumpstrength[model] * FP8_TO_FLOAT( chralpha_fp8[ichr] );
    chrlight_fp8[ichr] = caplight_fp8[model];
    enchant = chrfirstenchant[ichr];
    while ( enchant < MAXENCHANT )
    {
      set_enchant_value( enchant, SETALPHABLEND, enceve[enchant] );
      set_enchant_value( enchant, SETLIGHTBLEND, enceve[enchant] );
      enchant = encnextenchant[enchant];
    }
  }

  // Set twist
  chrmapturn_lr[ichr] = 32768;
  chrmapturn_ud[ichr] = 32768;

  if ( chrisplayer[ichr] )
    debug_message( 1, "dismounted %s(%s) from (%s)", chrname[ichr], capclassname[chrmodel[ichr]], capclassname[chrmodel[imount]] );


  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_character_to_mount( CHR_REF ichr, Uint16 imount, SLOT slot )
{
  // ZZ> This function attaches one ichr to another ( the imount )
  //     at either the left or right grip
  int tnc;

  // Make sure both are still around
  if ( !VALID_CHR( ichr ) || !VALID_CHR( imount ) )
    return bfalse;

  // the item may hit the floor if this fails
  chrhitready[ichr] = bfalse;

  //make sure you're not trying to mount yourself!
  if ( ichr == imount )
    return bfalse;

  // make sure that neither is in someone's pack
  if ( chr_in_pack( ichr ) || chr_in_pack( imount ) )
    return bfalse;

  // Make sure the the slot is valid
  assert( MAXMODEL != VALIDATE_MDL( chrmodel[imount] ) );
  if ( SLOT_NONE == slot || !capslotvalid[chrmodel[imount]][slot] )
    return bfalse;

  // Put 'em together
  assert(slot != SLOT_NONE);
  chrinwhichslot[ichr] = slot;
  chrattachedto[ichr]  = imount;
  chrholdingwhich[imount][slot] = ichr;

  // handle the vertices
  {
    Uint16 model = chrmodel[imount];
    Uint16 vrtoffset = slot_to_offset( slot );

    assert( MAXMODEL != VALIDATE_MDL( model ) );
    if ( madvertices[model] > vrtoffset && vrtoffset > 0 )
    {
      tnc = madvertices[model] - vrtoffset;
      chrattachedgrip[ichr][0] = tnc;
      chrattachedgrip[ichr][1] = tnc + 1;
      chrattachedgrip[ichr][2] = tnc + 2;
      chrattachedgrip[ichr][3] = tnc + 3;
    }
    else
    {
      chrattachedgrip[ichr][0] = madvertices[model] - 1;
      chrattachedgrip[ichr][1] = 0xFFFF;
      chrattachedgrip[ichr][2] = 0xFFFF;
      chrattachedgrip[ichr][3] = 0xFFFF;
    }
  }

  chrjumptime[ichr] = DELAY_JUMP * 4;


  // Run the held animation
  if ( chrbmpdata[imount].calc_is_mount && slot == SLOT_SADDLE )
  {
    // Riding imount
    play_action( ichr, ACTION_MI, btrue );
    chrloopaction[ichr] = btrue;
  }
  else
  {
    play_action( ichr, ACTION_MM + slot, bfalse );
    if ( chrisitem[ichr] )
    {
      // Item grab
      chrkeepaction[ichr] = btrue;
    }
  }

  // Set the team
  if ( chrisitem[ichr] )
  {
    chrteam[ichr] = chrteam[imount];
    // Set the alert
    chralert[ichr] |= ALERT_GRABBED;
  }
  else if ( chrbmpdata[imount].calc_is_mount )
  {
    chrteam[imount] = chrteam[ichr];
    // Set the alert
    if ( !chrisitem[imount] )
    {
      chralert[imount] |= ALERT_GRABBED;
    }
  }

  // It's not gonna hit the floor
  chrhitready[ichr] = bfalse;

  if ( chrisplayer[ichr] )
    debug_message( 1, "mounted %s(%s) to (%s)", chrname[ichr], capclassname[chrmodel[ichr]], capclassname[chrmodel[imount]] );


  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF stack_in_pack( CHR_REF item, CHR_REF character )
{
  // ZZ> This function looks in the character's pack for an item similar
  //     to the one given.  If it finds one, it returns the similar item's
  //     index number, otherwise it returns MAXCHR.
  Uint16 inpack, id;
  bool_t allok;

  Uint16 item_mdl = chrmodel[item];

  assert( MAXMODEL != VALIDATE_MDL( item_mdl ) );


  if ( capisstackable[item_mdl] )
  {
    Uint16 inpack_mdl;

    inpack = chr_get_nextinpack( character );
    inpack_mdl = chrmodel[inpack];

    assert( MAXMODEL != VALIDATE_MDL( inpack_mdl ) );

    allok = bfalse;
    while ( VALID_CHR( inpack ) && !allok )
    {
      allok = btrue;
      if ( inpack_mdl != item_mdl )
      {
        if ( !capisstackable[inpack_mdl] )
        {
          allok = bfalse;
        }

        if ( chrammomax[inpack] != chrammomax[item] )
        {
          allok = bfalse;
        }

        id = 0;
        while ( id < IDSZ_COUNT && allok )
        {
          if ( capidsz[inpack_mdl][id] != capidsz[item_mdl][id] )
          {
            allok = bfalse;
          }
          id++;
        }
      }
      if ( !allok )
      {
        inpack = chr_get_nextinpack( inpack );
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
static bool_t pack_push_front( Uint16 iitem, CHR_REF ichr )
{
  // make sure the item and character are valid
  if ( !VALID_CHR( iitem ) || !VALID_CHR( ichr ) ) return bfalse;

  // make sure the item is free to add
  if ( chr_attached( iitem ) || chr_in_pack( iitem ) ) return bfalse;

  // we cannot do packs within packs, so
  if ( chr_in_pack( ichr ) ) return bfalse;

  // make sure there is space for the item
  if ( chrnuminpack[ichr] >= MAXNUMINPACK ) return bfalse;

  // insert at the front of the list
  chrnextinpack[iitem]  = chr_get_nextinpack( ichr );
  chrnextinpack[ichr] = iitem;
  chrinwhichpack[iitem] = ichr;
  chrnuminpack[ichr]++;

  return btrue;
};

//--------------------------------------------------------------------------------------------
static Uint16 pack_pop_back( CHR_REF ichr )
{
  Uint16 iitem = MAXCHR, itail = MAXCHR;

  // make sure the character is valid
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  // if character is in a pack, it has no inventory of it's own
  if ( chr_in_pack( iitem ) ) return MAXCHR;

  // make sure there is something in the pack
  if ( !chr_has_inventory( ichr ) ) return MAXCHR;

  // remove from the back of the list
  itail = ichr;
  iitem = chr_get_nextinpack( ichr );
  while ( VALID_CHR( chrnextinpack[iitem] ) )
  {
    // do some error checking
    assert( 0 == chrnuminpack[iitem] );

    // go to the next element
    itail = iitem;
    iitem = chr_get_nextinpack( iitem );
  };

  // disconnect the item from the list
  chrnextinpack[itail] = MAXCHR;
  chrnuminpack[ichr]--;

  // do some error checking
  assert( VALID_CHR( iitem ) );

  // fix the removed item
  chrnuminpack[iitem]   = 0;
  chrnextinpack[iitem]  = MAXCHR;
  chrinwhichpack[iitem] = MAXCHR;
  chrisequipped[iitem] = bfalse;

  return iitem;
};

//--------------------------------------------------------------------------------------------
bool_t add_item_to_character_pack( Uint16 iitem, CHR_REF ichr )
{
  // ZZ> This function puts one ichr inside the other's pack
  Uint16 newammo, istack;

  // Make sure both objects exist
  if ( !VALID_CHR( ichr ) || !VALID_CHR( iitem ) ) return bfalse;

  // Make sure neither object is in a pack
  if ( chr_in_pack( ichr ) || chr_in_pack( iitem ) ) return bfalse;

  // make sure we the character IS NOT an item and the item IS an item
  if ( chrisitem[ichr] || !chrisitem[iitem] ) return bfalse;

  // make sure the item does not have an inventory of its own
  if ( chr_has_inventory( iitem ) ) return bfalse;

  istack = stack_in_pack( iitem, ichr );
  if ( VALID_CHR( istack ) )
  {
    // put out torches, etc.
    disaffirm_attached_particles( iitem );

    // We found a similar, stackable iitem in the pack
    if ( chrnameknown[iitem] || chrnameknown[istack] )
    {
      chrnameknown[iitem] = btrue;
      chrnameknown[istack] = btrue;
    }
    if ( capusageknown[chrmodel[iitem]] || capusageknown[chrmodel[istack]] )
    {
      capusageknown[chrmodel[iitem]] = btrue;
      capusageknown[chrmodel[istack]] = btrue;
    }
    newammo = chrammo[iitem] + chrammo[istack];
    if ( newammo <= chrammomax[istack] )
    {
      // All transfered, so kill the in hand iitem
      chrammo[istack] = newammo;
      detach_character_from_mount( iitem, btrue, bfalse );
      chrfreeme[iitem] = btrue;
    }
    else
    {
      // Only some were transfered,
      chrammo[iitem] += chrammo[istack] - chrammomax[istack];
      chrammo[istack] = chrammomax[istack];
      chralert[ichr] |= ALERT_TOOMUCHBAGGAGE;
    }
  }
  else
  {
    // Make sure we have room for another iitem
    if ( chrnuminpack[ichr] >= MAXNUMINPACK )
    {
      chralert[ichr] |= ALERT_TOOMUCHBAGGAGE;
      return bfalse;
    }

    // Take the item out of hand
    if ( detach_character_from_mount( iitem, btrue, bfalse ) )
    {
      chralert[iitem] &= ~ALERT_DROPPED;
    }

    if ( pack_push_front( iitem, ichr ) )
    {
      // put out torches, etc.
      disaffirm_attached_particles( iitem );
      chralert[iitem] |= ALERT_ATLASTWAYPOINT;

      // Remove the iitem from play
      chrhitready[iitem]    = bfalse;
    };
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 get_item_from_character_pack( CHR_REF character, SLOT slot, bool_t ignorekurse )
{
  // ZZ> This function takes the last item in the character's pack and puts
  //     it into the designated hand.  It returns the item number or MAXCHR.
  Uint16 item;

  // dose the character exist?
  if ( !VALID_CHR( character ) )
    return MAXCHR;

  // make sure a valid inventory exists
  if ( !chr_has_inventory( character ) )
    return MAXCHR;

  item = pack_pop_back( character );

  // Figure out what to do with it
  if ( chriskursed[item] && chrisequipped[item] && !ignorekurse )
  {
    // Flag the last item as not removed
    chralert[item] |= ALERT_NOTPUTAWAY;  // Doubles as IfNotTakenOut

    // push it back on the front of the list
    pack_push_front( item, character );

    // return the "fail" value
    item = MAXCHR;
  }
  else
  {
    // Attach the item to the character's hand
    attach_character_to_mount( item, character, slot );

    // fix some item values
    chralert[item] &= ( ~ALERT_GRABBED );
    chralert[item] |= ALERT_TAKENOUT;
    //chrteam[item]   = chrteam[character];
  }

  return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( CHR_REF character )
{
  // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
  //     inventory ( Not hands ).
  Uint16 item, lastitem, nextitem, direction, cosdir;
  IDSZ testa, testz;


  if ( !VALID_CHR( character ) ) return;


  if ( chrpos[character].z > -2 ) // Don't lose keys in pits...
  {
    // The IDSZs to find
    testa = MAKE_IDSZ( "KEYA" );   // [KEYA]
    testz = MAKE_IDSZ( "KEYZ" );   // [KEYZ]

    lastitem = character;
    item = chr_get_nextinpack( character );
    while ( VALID_CHR( item ) )
    {
      nextitem = chr_get_nextinpack( item );
      if ( item != character ) // Should never happen...
      {
        if ( CAP_INHERIT_IDSZ_RANGE( chrmodel[item], testa, testz ) )
        {
          // We found a key...
          chrinwhichpack[item] = MAXCHR;
          chrisequipped[item] = bfalse;

          chrnextinpack[lastitem] = nextitem;
          chrnextinpack[item] = MAXCHR;
          chrnuminpack[character]--;

          chrhitready[item] = btrue;
          chralert[item] |= ALERT_DROPPED;

          direction = RANDIE;
          chrturn_lr[item] = direction + 32768;
          cosdir = direction + 16384;
          chrlevel[item] = chrlevel[character];
          chrpos[item].x = chrpos[character].x;
          chrpos[item].y = chrpos[character].y;
          chrpos[item].z = chrpos[character].z;
          chraccum_vel[item].x += turntosin[( cosdir>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          chraccum_vel[item].y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          chraccum_vel[item].z += DROPZVEL;
          chrteam[item] = chrbaseteam[item];
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

//--------------------------------------------------------------------------------------------
void drop_all_items( CHR_REF character )
{
  // ZZ> This function drops all of a character's items
  Uint16 item, direction, cosdir, diradd;


  if ( !VALID_CHR( character ) ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    detach_character_from_mount( chr_get_holdingwhich( character, _slot ), !chralive[character], bfalse );
  };

  if ( chr_has_inventory( character ) )
  {
    direction = chrturn_lr[character] + 32768;
    diradd = (float)UINT16_SIZE / chrnuminpack[character];
    while ( chrnuminpack[character] > 0 )
    {
      item = get_item_from_character_pack( character, SLOT_NONE, !chralive[character] );
      if ( detach_character_from_mount( item, btrue, btrue ) )
      {
        chrhitready[item] = btrue;
        chralert[item] |= ALERT_DROPPED;
        chrpos[item].x = chrpos[character].x;
        chrpos[item].y = chrpos[character].y;
        chrpos[item].z = chrpos[character].z;
        chrlevel[item] = chrlevel[character];
        chrturn_lr[item] = direction + 32768;

        cosdir = direction + 16384;
        chraccum_vel[item].x += turntosin[( cosdir>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        chraccum_vel[item].y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        chraccum_vel[item].z += DROPZVEL;
        chrteam[item] = chrbaseteam[item];
      }

      direction += diradd;
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t character_grab_stuff( CHR_REF ichr, SLOT slot, bool_t people )
{
  // ZZ> This function makes the character pick up an item if there's one around
  vect4 posa, point;
  vect3 posb, posc;
  float dist, mindist;
  CHR_REF iobject, minchr, iholder, ipacker, owner = MAXCHR;
  Uint16 vertex, model, passage, cnt, price;
  bool_t inshop, can_disarm, can_pickpocket, bfound, ballowed;
  GRIP grip;
  float grab_width, grab_height;
  CHR_REF trg_chr = MAXCHR;
  Sint16 trg_strength_fp8, trg_intelligence_fp8;
  TEAM trg_team;

  if ( !VALID_CHR( ichr ) ) return bfalse;

  model = chrmodel[ichr];
  if ( !VALID_MDL( model ) ) return bfalse;

  // Make sure the character doesn't have something already, and that it has hands

  if ( chr_using_slot( ichr, slot ) || !capslotvalid[model][slot] )
    return bfalse;

  // Make life easier
  grip  = slot_to_grip( slot );

  // !!!!base the grab distance off of the character size!!!!
  grab_width  = ( chrbmpdata[ichr].calc_size_big + chrbmpdata[ichr].calc_size ) / 2.0f * 1.5f;
  grab_height = chrbmpdata[ichr].calc_height / 2.0f * 1.5f;

  // Do we have a matrix???
  if ( chrmatrixvalid[ichr] )
  {
    // Transform the weapon grip from model to world space
    vertex = chrattachedgrip[ichr][0];

    if(0xFFFF == vertex)
    {
      point.x = chrpos[ichr].x;
      point.y = chrpos[ichr].y;
      point.z = chrpos[ichr].z;
      point.w = 1.0f;
    }
    else
    {
      point.x = chrvdata[ichr].Vertices[vertex].x;
      point.y = chrvdata[ichr].Vertices[vertex].y;
      point.z = chrvdata[ichr].Vertices[vertex].z;
      point.w = 1.0f;
    }

    // Do the transform
    Transform4_Full( &chrmatrix[ichr], &posa, &point, 1 );
  }
  else
  {
    // Just wing it
    posa.x = chrpos[ichr].x;
    posa.y = chrpos[ichr].y;
    posa.z = chrpos[ichr].z;
  }

  // Go through all characters to find the best match
  can_disarm     = check_skills( ichr, MAKE_IDSZ( "DISA" ) );
  can_pickpocket = check_skills( ichr, MAKE_IDSZ( "PICK" ) );
  bfound = bfalse;
  for ( iobject = 0; iobject < MAXCHR; iobject++ )
  {
    // Don't mess with stuff that doesn't exist
    if ( !VALID_CHR( iobject ) ) continue;

    iholder = chr_get_attachedto(iobject);
    ipacker = chr_get_inwhichpack(iobject);

    // don't mess with yourself or anything you're already holding
    if ( iobject == ichr || ipacker == ichr || iholder == ichr ) continue;

    // don't mess with stuff you can't see
    if ( !chrcanseeinvisible[ichr] && chr_is_invisible( iobject ) ) continue;

    // if we can't pickpocket, don't mess with inventory items
    if ( !can_pickpocket && VALID_CHR( ipacker ) ) continue;

    // if we can't disarm, don't mess with held items
    if ( !can_disarm && VALID_CHR( iholder ) ) continue;

    // if we can't grab people, don't mess with them
    if ( !people && !chrisitem[iobject] ) continue;

    // get the target object position
    if ( !VALID_CHR( ipacker ) && !VALID_CHR(iholder) )
    {
      trg_strength_fp8     = chrstrength_fp8[iobject];
      trg_intelligence_fp8 = chrintelligence_fp8[iobject];
      trg_team             = chrteam[iobject];

      posb = chrpos[iobject];
    }
    else if ( VALID_CHR(iholder) )
    {
      trg_chr              = iholder;
      trg_strength_fp8     = chrstrength_fp8[iholder];
      trg_intelligence_fp8 = chrintelligence_fp8[iholder];

      trg_team = chrteam[iholder];
      posb     = chrpos[iobject];
    }
    else // must be in a pack
    {
      trg_chr              = ipacker;
      trg_strength_fp8     = chrstrength_fp8[ipacker];
      trg_intelligence_fp8 = chrintelligence_fp8[ipacker];
      trg_team = chrteam[ipacker];
      posb     = chrpos[ipacker];
      posb.z  += chrbmpdata[ipacker].calc_height / 2;
    };

    // First check absolute value diamond
    posc.x = ABS( posa.x - posb.x );
    posc.y = ABS( posa.y - posb.y );
    posc.z = ABS( posa.z - posb.z );
    dist = posc.x + posc.y;

    // close enough to grab ?
    if ( dist > grab_width || posc.z > grab_height ) continue;

    if ( VALID_CHR(ipacker) )
    {
      // check for pickpocket
      ballowed = chrdexterity_fp8[ichr] >= trg_intelligence_fp8 && teamhatesteam[chrteam[ichr]][trg_team];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        chralert[iholder] |= ALERT_ATTACKED;
        chraibumplast[iholder] = ichr;
      }
      else  // must be in a pack
      {
        // TODO : figure out a way to get the thing out of the pack!!
        //        get_item_from_character_pack() won't work?

      };
    }
    else if ( VALID_CHR( iholder ) )
    {
      // check for stealing item from hand
      ballowed = !chriskursed[iobject] && chrstrength_fp8[ichr] > trg_strength_fp8 && teamhatesteam[chrteam[ichr]][trg_team];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        chralert[iholder] |= ALERT_ATTACKED;
        chraibumplast[iholder] = ichr;
      }
      else
      {
        // TODO : do the dismount
      };
    }
    else
    {
      ballowed = btrue;
    }

    if ( ballowed || !bfound )
    {
      mindist = dist;
      minchr  = iobject;
      bfound  = btrue;
    };

  };

  if ( !bfound ) return bfalse;

  // Check for shop
  inshop = bfalse;
  ballowed = bfalse;
  if ( mesh_check( chrpos[minchr].x, chrpos[minchr].y ) )
  {
    if ( numshoppassage == 0 )
    {
      ballowed = btrue;
    }
    else if ( chrisitem[minchr] )
    {

      // loop through just in case there are overlapping shops with one owner deceased
      for ( cnt = 0; cnt < numshoppassage && !inshop; cnt++ )
      {
        passage = shoppassage[cnt];

        if ( passage_check_any( minchr, passage, NULL ) )
        {
          owner  = shopowner[passage];
          inshop = ( NOOWNER != owner );
        };
      };

    };
  }


  if ( inshop )
  {
    if ( chrisitem[ichr] )
    {
      ballowed = btrue; // As in NetHack, Pets can shop for free =]
    }
    else
    {
      // Pay the shop owner, or don't allow grab...
      chralert[owner] |= ALERT_SIGNALED;
      price = capskincost[chrmodel[minchr]][( chrtexture[minchr] - madskinstart[chrmodel[minchr]] ) % MAXSKIN];
      if ( capisstackable[chrmodel[minchr]] )
      {
        price *= chrammo[minchr];
      }
      chrmessage[owner] = price;  // Tell owner how much...
      if ( chrmoney[ichr] >= price )
      {
        // Okay to buy
        chrmoney[ichr]  -= price;  // Skin 0 cost is price
        chrmoney[owner] += price;
        if ( chrmoney[owner] > MAXMONEY )  chrmoney[owner] = MAXMONEY;

        ballowed = btrue;
        chrmessagedata[owner] = 1;  // 1 for selling an item
      }
      else
      {
        // Don't allow purchase
        chrmessagedata[owner] = 2;  // 2 for "you can't afford that"
        ballowed = bfalse;
      }
    }
  }


  if ( ballowed )
  {
    // Stick 'em together and quit
    ballowed = attach_character_to_mount( minchr, ichr, slot );
    if ( ballowed && people )
    {
      // Do a bodyslam animation...  ( Be sure to drop!!! )
      play_action( ichr, ACTION_MC + slot, bfalse );
    };
  }
  else
  {
    // Lift the item a little and quit...
    chraccum_vel[minchr].z += DROPZVEL;
    chrhitready[minchr] = btrue;
    chralert[minchr] |= ALERT_DROPPED;
  };

  return ballowed;
}

//--------------------------------------------------------------------------------------------
void character_swipe( CHR_REF ichr, SLOT slot )
{
  // ZZ> This function spawns an attack particle
  Uint16 weapon, particle, thrown;
  ACTION action;
  Uint16 tTmp;
  float dampen;
  vect3 pos;
  float velocity;
  GRIP spawngrip;


  weapon = chr_get_holdingwhich( ichr, slot );
  spawngrip = GRIP_LAST;
  action = chraction[ichr];
  // See if it's an unarmed attack...
  if ( !VALID_CHR( weapon ) )
  {
    weapon = ichr;
    spawngrip = slot_to_grip( slot );
  }


  if ( weapon != ichr && (( capisstackable[chrmodel[weapon]] && chrammo[weapon] > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
  {
    // Throw the weapon if it's stacked or a hurl animation
    pos.x = chrpos[ichr].x;
    pos.y = chrpos[ichr].y;
    pos.z = chrpos[ichr].z;
    thrown = spawn_one_character( chrpos[ichr], chrmodel[weapon], chrteam[ichr], 0, chrturn_lr[ichr], chrname[weapon], MAXCHR );
    if ( VALID_CHR( thrown ) )
    {
      chriskursed[thrown] = bfalse;
      chrammo[thrown] = 1;
      chralert[thrown] |= ALERT_THROWN;

      velocity = 0.0f;
      if ( chrweight[ichr] >= 0.0f )
      {
        velocity = chrstrength_fp8[ichr] / ( chrweight[thrown] * THROWFIX );
      };

      velocity += MINTHROWVELOCITY;
      if ( velocity > MAXTHROWVELOCITY )
      {
        velocity = MAXTHROWVELOCITY;
      }
      tTmp = ( 0x7FFF + chrturn_lr[ichr] ) >> 2;
      chraccum_vel[thrown].x += turntosin[( tTmp+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * velocity;
      chraccum_vel[thrown].y += turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * velocity;
      chraccum_vel[thrown].z += DROPZVEL;
      if ( chrammo[weapon] <= 1 )
      {
        // Poof the item
        detach_character_from_mount( weapon, btrue, bfalse );
        chrfreeme[weapon] = btrue;
      }
      else
      {
        chrammo[weapon]--;
      }
    }
  }
  else
  {
    // Spawn an attack particle
    if ( chrammomax[weapon] == 0 || chrammo[weapon] != 0 )
    {
      if ( chrammo[weapon] > 0 && !capisstackable[chrmodel[weapon]] )
      {
        chrammo[weapon]--;  // Ammo usage
      }

      //HERE
      if ( capattackprttype[chrmodel[weapon]] != -1 )
      {
        particle = spawn_one_particle( 1.0f, chrpos[weapon], chrturn_lr[ichr], chrmodel[weapon], capattackprttype[chrmodel[weapon]], weapon, spawngrip, chrteam[ichr], ichr, 0, MAXCHR );
        if ( particle != MAXPRT )
        {
          CHR_REF prt_target = prt_get_target( particle );

          if ( !capattackattached[chrmodel[weapon]] )
          {
            // Detach the particle
            if ( !pipstartontarget[prtpip[particle]] || !VALID_CHR( prt_target ) )
            {
              attach_particle_to_character( particle, weapon, spawngrip );
              // Correct Z spacing base, but nothing else...
              prtpos[particle].z += pipzspacing[prtpip[particle]].ibase;
            }
            prtattachedtochr[particle] = MAXCHR;
            // Don't spawn in walls
            if ( 0 != __prthitawall( particle, NULL ) )
            {
              prtpos[particle].x = chrpos[weapon].x;
              prtpos[particle].y = chrpos[weapon].y;
              if ( 0 != __prthitawall( particle, NULL ) )
              {
                prtpos[particle].x = chrpos[ichr].x;
                prtpos[particle].y = chrpos[ichr].y;
              }
            }
          }
          else
          {
            // Attached particles get a strength bonus for reeling...
            if ( pipcauseknockback[prtpip[particle]] ) dampen = ( REELBASE + ( chrstrength_fp8[ichr] / REEL ) ) * 4; //Extra knockback?
            else dampen = REELBASE + ( chrstrength_fp8[ichr] / REEL );      // No, do normal

            prtaccum_vel[particle].x += -(1.0f - dampen) * prtvel[particle].x;
            prtaccum_vel[particle].y += -(1.0f - dampen) * prtvel[particle].y;
            prtaccum_vel[particle].z += -(1.0f - dampen) * prtvel[particle].z;
          }

          // Initial particles get a strength bonus, which may be 0.00
          prtdamage[particle].ibase += ( chrstrength_fp8[ichr] * capstrengthdampen[chrmodel[weapon]] );

          // Initial particles get an enchantment bonus
          prtdamage[particle].ibase += chrdamageboost[weapon];

          // Initial particles inherit damage type of weapon
          prtdamagetype[particle] = chrdamagetargettype[weapon];
        }
      }
    }
    else
    {
      chrammoknown[weapon] = btrue;
    }
  }
}

//--------------------------------------------------------------------------------------------
void despawn_characters()
{
  CHR_REF ichr;

  // poof all characters that have pending poof requests
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) || !chrgopoof[ichr] ) continue;

    // detach from any imount
    detach_character_from_mount( ichr, btrue, bfalse );

    // Drop all possesions
    for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
    {
      if ( chr_using_slot( ichr, _slot ) )
        detach_character_from_mount( chr_get_holdingwhich( ichr, _slot ), btrue, bfalse );
    };

    free_inventory( ichr );
    chrfreeme[ichr] = btrue;
  };

  // free all characters that requested destruction last round
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) || !chrfreeme[ichr] ) continue;
    free_one_character( ichr );
  }
};

//--------------------------------------------------------------------------------------------
void move_characters( float dUpdate )
{
  // ZZ> This function handles character physics
  CHR_REF ichr, weapon, imount, item;
  Uint16 imdl;
  Uint8 twist;
  Uint8 speed, framelip;
  float maxvel, dvx, dvy, dvmax;
  ACTION action;
  bool_t actionready, allowedtoattack, watchtarget, grounded, dojumptimer;
  TURNMODE loc_turnmode;
  float ftmp, level;

  float horiz_friction, vert_friction;
  float loc_slippyfriction, loc_airfriction, loc_waterfriction, loc_noslipfriction;
  float loc_flydampen, loc_traction;
  float turnfactor;
  vect3 nrm = {0.0f, 0.0f, 0.0f};


  loc_airfriction    = airfriction;
  loc_waterfriction  = waterfriction;
  loc_slippyfriction = slippyfriction;
  loc_noslipfriction = noslipfriction;
  loc_flydampen      = pow( FLYDAMPEN     , dUpdate );

  // Move every character
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) ) continue;

    // Character's old location
    chrturn_lr_old[ichr] = chrturn_lr[ichr];

    if ( chr_in_pack( ichr ) ) continue;

    // get the model
    imdl = VALIDATE_MDL( chrmodel[ichr] );
    assert( MAXMODEL != imdl );

    // get the imount
    imount = chr_get_attachedto(ichr);

    // get the level
    level = chrlevel[ichr];

    // TURNMODE_VELOCITY acts strange when someone is mounted on a "bucking" imount, like the gelfeet
    loc_turnmode = chrturnmode[ichr];
    if ( VALID_CHR( imount ) ) loc_turnmode = TURNMODE_NONE;

    // make characters slide downhill
    twist = mesh_get_twist( chronwhichfan[ichr] );

    // calculate the normal dynamically from the mesh coordinates
    if ( !mesh_calc_normal( chrpos[ichr], &nrm ) )
    {
      nrm = mapnrm[twist];
    };

    // TODO : replace with line(s) below
    turnfactor = 2.0f;
    // scale the turn rate by the dexterity.
    // For zero dexterity, rate is half speed
    // For maximum dexterity, rate is 1.5 normal rate
    //turnfactor = (3.0f * (float)chrdexterity_fp8[ichr] / (float)PERFECTSTAT + 1.0f) / 2.0f;

    grounded    = bfalse;

    // Down that ol' damage timer
    chrdamagetime[ichr] -= dUpdate;
    if ( chrdamagetime[ichr] < 0 ) chrdamagetime[ichr] = 0.0f;

    // Texture movement
    chruoffset_fp8[ichr] += chruoffvel[ichr] * dUpdate;
    chrvoffset_fp8[ichr] += chrvoffvel[ichr] * dUpdate;

    // calculate the Character's environment
    {
      float wt;
      float air_traction = ( chrflyheight[ichr] == 0.0f ) ? ( 1.0 - airfriction ) : airfriction;

      wt = 0.0f;
      loc_traction   = 0;
      horiz_friction = 0;
      vert_friction  = 0;

      if ( chrinwater[ichr] )
      {
        // we are partialy under water
        float buoy, lerp;

        if ( chrweight[ichr] < 0.0f || chrholdingweight[ichr] < 0.0f )
        {
          buoy = 0.0f;
        }
        else
        {
          float volume, weight;

          weight = chrweight[ichr] + chrholdingweight[ichr];
          volume = ( chrbmpdata[ichr].cv.z_max - chrbmpdata[ichr].cv.z_min ) * ( chrbmpdata[ichr].cv.x_max - chrbmpdata[ichr].cv.x_min ) * ( chrbmpdata[ichr].cv.y_max - chrbmpdata[ichr].cv.y_min );

          // this adjusts the buoyancy so that the default adventurer gets a buoyancy of 0.3
          buoy = 0.3f * ( weight / volume ) * 1196.0f;
          if ( buoy < 0.0f ) buoy = 0.0f;
          if ( buoy > 1.0f ) buoy = 1.0f;
        };

        lerp = ( float )( watersurfacelevel - chrpos[ichr].z ) / ( float ) ( chrbmpdata[ichr].cv.z_max - chrbmpdata[ichr].cv.z_min );
        if ( lerp > 1.0f ) lerp = 1.0f;
        if ( lerp < 0.0f ) lerp = 0.0f;

        loc_traction   += waterfriction * lerp;
        horiz_friction += waterfriction * lerp;
        vert_friction  += waterfriction * lerp;
        chraccum_acc[ichr].z             -= buoy * gravity  * lerp;

        wt += lerp;
      }

      if ( chrpos[ichr].z < level + PLATTOLERANCE )
      {
        // we are close to something
        bool_t is_slippy;
        float lerp = ( level + PLATTOLERANCE - chrpos[ichr].z ) / ( float ) PLATTOLERANCE;
        if ( lerp > 1.0f ) lerp = 1.0f;
        if ( lerp < 0.0f ) lerp = 0.0f;

        if ( VALID_CHR( chronwhichplatform[ichr] ) )
        {
          is_slippy = bfalse;
        }
        else
        {
          is_slippy = ( INVALID_FAN != chronwhichfan[ichr] ) && mesh_has_some_bits( chronwhichfan[ichr], MESHFX_SLIPPY );
        }

        if ( is_slippy )
        {
          loc_traction   += ( 1.0 - slippyfriction ) * lerp;
          horiz_friction += slippyfriction * lerp;
        }
        else
        {
          loc_traction   += ( 1.0 - noslipfriction ) * lerp;
          horiz_friction += noslipfriction * lerp;
        };
        vert_friction += loc_airfriction * lerp;

        wt += lerp;
      };

      if ( wt < 1.0f )
      {
        // we are in clear air
        loc_traction   += ( 1.0f - wt ) * air_traction;
        horiz_friction += ( 1.0f - wt ) * loc_airfriction;
        vert_friction  += ( 1.0f - wt ) * loc_airfriction;
      }
      else
      {
        loc_traction   /= wt;
        horiz_friction /= wt;
        vert_friction  /= wt;
      };

      grounded = ( chrpos[ichr].z < level + PLATTOLERANCE / 20.0f );
    }

    // do volontary movement
    if ( chralive[ichr] )
    {
      // Apply the latches
      if ( !VALID_CHR( imount ) )
      {
        // Character latches for generalized movement
        dvx = chrlatchx[ichr];
        dvy = chrlatchy[ichr];

        // Reverse movements for daze
        if ( chrdazetime[ichr] > 0.0f )
        {
          dvx = -dvx;
          dvy = -dvy;
        }

        // Switch x and y for daze
        if ( chrgrogtime[ichr] > 0.0f )
        {
          dvmax = dvx;
          dvx = dvy;
          dvy = dvmax;
        }

        // Get direction from the DESIRED change in velocity
        if ( loc_turnmode == TURNMODE_WATCH )
        {
          if (( ABS( dvx ) > WATCHMIN || ABS( dvy ) > WATCHMIN ) )
          {
            chrturn_lr[ichr] = terp_dir( chrturn_lr[ichr], dvx, dvy, dUpdate * turnfactor );
          }
        }

        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_STOP ) )
        {
          dvx = 0;
          dvy = 0;
        }

        // TODO : change to line(s) below
        maxvel = chrmaxaccel[ichr] / ( 1.0 - noslipfriction );
        // set a minimum speed of 6 to fix some stupid slow speeds
        //maxvel = 1.5f * MAX(MAX(3,chrrunspd[ichr]), MAX(chrwalkspd[ichr],chrsneakspd[ichr]));
        chrtrgvel[ichr].x = dvx * maxvel;
        chrtrgvel[ichr].y = dvy * maxvel;
        chrtrgvel[ichr].z = 0;

        if ( chrmaxaccel[ichr] > 0.0f )
        {
          dvx = ( chrtrgvel[ichr].x - chrvel[ichr].x );
          dvy = ( chrtrgvel[ichr].y - chrvel[ichr].y );

          // TODO : change to line(s) below
          dvmax = chrmaxaccel[ichr];
          // Limit to max acceleration
          //if(maxvel==0.0)
          //{
          //  dvmax = 2.0f * chrmaxaccel[ichr];
          //}
          //else
          //{
          //  float ftmp;
          //  chrvel2 = chrvel[ichr].x*chrvel[ichr].x + chrvel[ichr].y*chrvel[ichr].y;
          //  ftmp = MIN(1.0 , chrvel2/maxvel/maxvel);
          //  dvmax   = 2.0f * chrmaxaccel[ichr] * (1.0-ftmp);
          //};

          if ( dvx < -dvmax ) dvx = -dvmax;
          if ( dvx >  dvmax ) dvx =  dvmax;
          if ( dvy < -dvmax ) dvy = -dvmax;
          if ( dvy >  dvmax ) dvy =  dvmax;

          loc_traction *= 11.0f;                    // 11.0f corrects traction so that it gives full traction for non-slip floors in advent.mod
          loc_traction = MIN( 1.0, loc_traction );

          chraccum_acc[ichr].x += dvx * loc_traction * nrm.z;
          chraccum_acc[ichr].y += dvy * loc_traction * nrm.z;
        };
      }

      // Apply chrlatchx[] and chrlatchy[]
      if ( !VALID_CHR( imount ) )
      {
        // Face the target
        watchtarget = ( loc_turnmode == TURNMODE_WATCHTARGET );
        if ( watchtarget )
        {
          CHR_REF ai_target = chr_get_aitarget( ichr );
          if ( VALID_CHR( ai_target ) && ichr != ai_target )
          {
            chrturn_lr[ichr] = terp_dir( chrturn_lr[ichr], chrpos[ai_target].x - chrpos[ichr].x, chrpos[ai_target].y - chrpos[ichr].y, dUpdate * turnfactor );
          };
        }

        // Get direction from ACTUAL change in velocity
        if ( loc_turnmode == TURNMODE_VELOCITY )
        {
          if ( chrisplayer[ichr] )
            chrturn_lr[ichr] = terp_dir( chrturn_lr[ichr], chrtrgvel[ichr].x, chrtrgvel[ichr].y, dUpdate * turnfactor );
          else
            chrturn_lr[ichr] = terp_dir( chrturn_lr[ichr], chrtrgvel[ichr].x, chrtrgvel[ichr].y, dUpdate * turnfactor / 4.0f );
        }

        // Otherwise make it spin
        else if ( loc_turnmode == TURNMODE_SPIN )
        {
          chrturn_lr[ichr] += SPINRATE * dUpdate * turnfactor;
        }
      };

      // Character latches for generalized buttons
      if ( LATCHBUTTON_NONE != chrlatchbutton[ichr] )
      {
        if ( HAS_SOME_BITS( chrlatchbutton[ichr], LATCHBUTTON_JUMP ) && chrjumptime[ichr] == 0.0f )
        {
          if ( detach_character_from_mount( ichr, btrue, btrue ) )
          {
            chrjumptime[ichr] = DELAY_JUMP;
            chraccum_vel[ichr].z += ( chrflyheight[ichr] == 0 ) ? DISMOUNTZVEL : DISMOUNTZVELFLY;
            if ( chrjumpnumberreset[ichr] != JUMPINFINITE && chrjumpnumber[ichr] > 0 )
              chrjumpnumber[ichr] -= dUpdate;

            // Play the jump sound
            if ( INVALID_SOUND != capjumpsound[imdl] )
            {
              play_sound( 1.0f, chrpos[ichr], capwavelist[imdl][capjumpsound[imdl]], 0 );
            };
          }
          else if ( chrjumpnumber[ichr] > 0 && ( chrjumpready[ichr] || chrjumpnumberreset[ichr] > 1 ) )
          {
            // Make the character jump
            if ( chrinwater[ichr] && !grounded )
            {
              chraccum_vel[ichr].z += WATERJUMP / 3.0f;
              chrjumptime[ichr] = DELAY_JUMP / 3.0f;
            }
            else
            {
              chraccum_vel[ichr].z += chrjump[ichr] * 2.0f;
              chrjumptime[ichr] = DELAY_JUMP;

              // Set to jump animation if not doing anything better
              if ( chractionready[ichr] )    play_action( ichr, ACTION_JA, btrue );

              // Play the jump sound (Boing!)
              if ( INVALID_SOUND != capjumpsound[imdl] )
              {
                play_sound( MIN( 1.0f, chrjump[ichr] / 50.0f ), chrpos[ichr], capwavelist[imdl][capjumpsound[imdl]], 0  );
              }
            };

            chrhitready[ichr]  = btrue;
            chrjumpready[ichr] = bfalse;
            if ( chrjumpnumberreset[ichr] != JUMPINFINITE ) chrjumpnumber[ichr] -= dUpdate;
          }
        }

        if ( HAS_SOME_BITS( chrlatchbutton[ichr], LATCHBUTTON_ALTLEFT ) && chractionready[ichr] && chrreloadtime[ichr] == 0 )
        {
          chrreloadtime[ichr] = DELAY_GRAB;
          if ( !chr_using_slot( ichr, SLOT_LEFT ) )
          {
            // Grab left
            play_action( ichr, ACTION_ME, bfalse );
          }
          else
          {
            // Drop left
            play_action( ichr, ACTION_MA, bfalse );
          }
        }

        if ( HAS_SOME_BITS( chrlatchbutton[ichr], LATCHBUTTON_ALTRIGHT ) && chractionready[ichr] && chrreloadtime[ichr] == 0 )
        {
          chrreloadtime[ichr] = DELAY_GRAB;
          if ( !chr_using_slot( ichr, SLOT_RIGHT ) )
          {
            // Grab right
            play_action( ichr, ACTION_MF, bfalse );
          }
          else
          {
            // Drop right
            play_action( ichr, ACTION_MB, bfalse );
          }
        }

        if ( HAS_SOME_BITS( chrlatchbutton[ichr], LATCHBUTTON_PACKLEFT ) && chractionready[ichr] && chrreloadtime[ichr] == 0 )
        {
          chrreloadtime[ichr] = DELAY_PACK;
          item = chr_get_holdingwhich( ichr, SLOT_LEFT );
          if ( VALID_CHR( item ) )
          {
            if (( chriskursed[item] || capistoobig[chrmodel[item]] ) && !capisequipment[chrmodel[item]] )
            {
              // The item couldn't be put away
              chralert[item] |= ALERT_NOTPUTAWAY;
            }
            else
            {
              // Put the item into the pack
              add_item_to_character_pack( item, ichr );
            }
          }
          else
          {
            // Get a new one out and put it in hand
            get_item_from_character_pack( ichr, SLOT_LEFT, bfalse );
          }

          // Make it take a little time
          play_action( ichr, ACTION_MG, bfalse );
        }

        if ( HAS_SOME_BITS( chrlatchbutton[ichr], LATCHBUTTON_PACKRIGHT ) && chractionready[ichr] && chrreloadtime[ichr] == 0 )
        {
          chrreloadtime[ichr] = DELAY_PACK;
          item = chr_get_holdingwhich( ichr, SLOT_RIGHT );
          if ( VALID_CHR( item ) )
          {
            if (( chriskursed[item] || capistoobig[chrmodel[item]] ) && !capisequipment[chrmodel[item]] )
            {
              // The item couldn't be put away
              chralert[item] |= ALERT_NOTPUTAWAY;
            }
            else
            {
              // Put the item into the pack
              add_item_to_character_pack( item, ichr );
            }
          }
          else
          {
            // Get a new one out and put it in hand
            get_item_from_character_pack( ichr, SLOT_RIGHT, bfalse );
          }

          // Make it take a little time
          play_action( ichr, ACTION_MG, bfalse );
        }

        if ( HAS_SOME_BITS( chrlatchbutton[ichr], LATCHBUTTON_LEFT ) && chrreloadtime[ichr] == 0 )
        {
          // Which weapon?
          weapon = chr_get_holdingwhich( ichr, SLOT_LEFT );
          if ( !VALID_CHR( weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = ichr;
          }
          action = capweaponaction[chrmodel[weapon]];


          // Can it do it?
          allowedtoattack = btrue;
          if ( !madactionvalid[imdl][action] || chrreloadtime[weapon] > 0 ||
               ( capneedskillidtouse[chrmodel[weapon]] && !check_skills( ichr, capidsz[chrmodel[weapon]][IDSZ_SKILL] ) ) )
          {
            allowedtoattack = bfalse;
            if ( chrreloadtime[weapon] == 0 )
            {
              // This character can't use this weapon
              chrreloadtime[weapon] = 50;
              if ( chrstaton[ichr] )
              {
                // Tell the player that they can't use this weapon
                debug_message( 1, "%s can't use this item...", chrname[ichr] );
              }
            }
          }

          if ( action == ACTION_DA )
          {
            allowedtoattack = bfalse;
            if ( chrreloadtime[weapon] == 0 )
            {
              chralert[weapon] |= ALERT_USED;
            }
          }


          if ( allowedtoattack )
          {
            // Rearing imount
            if ( VALID_CHR( imount ) )
            {
              allowedtoattack = capridercanattack[chrmodel[imount]];
              if ( chrismount[imount] && chralive[imount] && !chrisplayer[imount] && chractionready[imount] )
              {
                if (( action != ACTION_PA || !allowedtoattack ) && chractionready[ichr] )
                {
                  play_action( imount, ( ACTION )( ACTION_UA + ( rand() &1 ) ), bfalse );
                  chralert[imount] |= ALERT_USED;
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
              if ( chractionready[ichr] && madactionvalid[imdl][action] )
              {
                // Check mana cost
                if ( chrmana_fp8[ichr] >= chrmanacost[weapon] || chrcanchannel[ichr] )
                {
                  cost_mana( ichr, chrmanacost[weapon], weapon );
                  // Check life healing
                  chrlife_fp8[ichr] += chrlifeheal[weapon];
                  if ( chrlife_fp8[ichr] > chrlifemax_fp8[ichr] )  chrlife_fp8[ichr] = chrlifemax_fp8[ichr];
                  actionready = bfalse;
                  if ( action == ACTION_PA )
                    actionready = btrue;
                  action += rand() & 1;
                  play_action( ichr, action, actionready );
                  if ( weapon != ichr )
                  {
                    // Make the weapon attack too
                    play_action( weapon, ACTION_MJ, bfalse );
                    chralert[weapon] |= ALERT_USED;
                  }
                  else
                  {
                    // Flag for unarmed attack
                    chralert[ichr] |= ALERT_USED;
                  }
                }
              }
            }
          }
        }
        else if ( HAS_SOME_BITS( chrlatchbutton[ichr], LATCHBUTTON_RIGHT ) && chrreloadtime[ichr] == 0 )
        {
          // Which weapon?
          weapon = chr_get_holdingwhich( ichr, SLOT_RIGHT );
          if ( !VALID_CHR( weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = ichr;
          }
          action = capweaponaction[chrmodel[weapon]] + 2;


          // Can it do it?
          allowedtoattack = btrue;
          if ( !madactionvalid[imdl][action] || chrreloadtime[weapon] > 0 ||
               ( capneedskillidtouse[chrmodel[weapon]] && !check_skills( ichr, capidsz[chrmodel[weapon]][IDSZ_SKILL] ) ) )
          {
            allowedtoattack = bfalse;
            if ( chrreloadtime[weapon] == 0 )
            {
              // This character can't use this weapon
              chrreloadtime[weapon] = 50;
              if ( chrstaton[ichr] )
              {
                // Tell the player that they can't use this weapon
                debug_message( 1, "%s can't use this item...", chrname[ichr] );
              }
            }
          }
          if ( action == ACTION_DC )
          {
            allowedtoattack = bfalse;
            if ( chrreloadtime[weapon] == 0 )
            {
              chralert[weapon] |= ALERT_USED;
            }
          }


          if ( allowedtoattack )
          {
            // Rearing imount
            if ( VALID_CHR( imount ) )
            {
              allowedtoattack = capridercanattack[chrmodel[imount]];
              if ( chrismount[imount] && chralive[imount] && !chrisplayer[imount] && chractionready[imount] )
              {
                if (( action != ACTION_PC || !allowedtoattack ) && chractionready[ichr] )
                {
                  play_action( imount, ( ACTION )( ACTION_UC + ( rand() &1 ) ), bfalse );
                  chralert[imount] |= ALERT_USED;
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
              if ( chractionready[ichr] && madactionvalid[imdl][action] )
              {
                // Check mana cost
                if ( chrmana_fp8[ichr] >= chrmanacost[weapon] || chrcanchannel[ichr] )
                {
                  cost_mana( ichr, chrmanacost[weapon], weapon );
                  // Check life healing
                  chrlife_fp8[ichr] += chrlifeheal[weapon];
                  if ( chrlife_fp8[ichr] > chrlifemax_fp8[ichr] )  chrlife_fp8[ichr] = chrlifemax_fp8[ichr];
                  actionready = bfalse;
                  if ( action == ACTION_PC )
                    actionready = btrue;
                  action += rand() & 1;
                  play_action( ichr, action, actionready );
                  if ( weapon != ichr )
                  {
                    // Make the weapon attack too
                    play_action( weapon, ACTION_MJ, bfalse );
                    chralert[weapon] |= ALERT_USED;
                  }
                  else
                  {
                    // Flag for unarmed attack
                    chralert[ichr] |= ALERT_USED;
                  }
                }
              }
            }
          }
        }
      }
    }



    // Integrate the z direction
    if ( 0.0f != chrflyheight[ichr] )
    {
      if ( level < 0 ) chraccum_pos[ichr].z += level - chrpos[ichr].z; // Don't fall in pits...
      chraccum_acc[ichr].z += ( level + chrflyheight[ichr] - chrpos[ichr].z ) * FLYDAMPEN;

      vert_friction = 1.0;
    }
    else if ( chrpos[ichr].z > level + PLATTOLERANCE )
    {
      chraccum_acc[ichr].z += gravity;
    }
    else
    {
      float lerp_normal, lerp_tang;
      lerp_tang = ( level + PLATTOLERANCE - chrpos[ichr].z ) / ( float ) PLATTOLERANCE;
      if ( lerp_tang > 1.0f ) lerp_tang = 1.0f;
      if ( lerp_tang < 0.0f ) lerp_tang = 0.0f;

      // fix to make sure characters will hit the ground softly, but in a reasonable time
      lerp_normal = 1.0 - lerp_tang;
      if ( lerp_normal > 1.0f ) lerp_normal = 1.0f;
      if ( lerp_normal < 0.2f ) lerp_normal = 0.2f;

      // slippy hills make characters slide
      if ( chrweight[ichr] > 0 && wateriswater && !chrinwater[ichr] && INVALID_FAN != chronwhichfan[ichr] && mesh_has_some_bits( chronwhichfan[ichr], MESHFX_SLIPPY ) )
      {
        chraccum_acc[ichr].x -= nrm.x * gravity * lerp_tang * hillslide;
        chraccum_acc[ichr].y -= nrm.y * gravity * lerp_tang * hillslide;
        chraccum_acc[ichr].z += nrm.z * gravity * lerp_normal;
      }
      else
      {
        chraccum_acc[ichr].z += gravity * lerp_normal;
      };
    }

    // Apply friction for next time
    chraccum_acc[ichr].x -= ( 1.0f - horiz_friction ) * chrvel[ichr].x;
    chraccum_acc[ichr].y -= ( 1.0f - horiz_friction ) * chrvel[ichr].y;
    chraccum_acc[ichr].z -= ( 1.0f - vert_friction ) * chrvel[ichr].z;

    // reset the jump
    chrjumpready[ichr]  = grounded || chrinwater[ichr];
    if ( chrjumptime[ichr] == 0.0f )
    {
      if ( grounded && chrjumpnumber[ichr] < chrjumpnumberreset[ichr] )
      {
        chrjumpnumber[ichr] = chrjumpnumberreset[ichr];
        chrjumptime[ichr]   = DELAY_JUMP;
      }
      else if ( chrinwater[ichr] && chrjumpnumber[ichr] < 1 )
      {
        // "Swimming"
        chrjumpready[ichr]  = btrue;
        chrjumptime[ichr]   = DELAY_JUMP / 3.0f;
        chrjumpnumber[ichr] += 1;
      }
    };

    // check to see if it can jump
    dojumptimer = btrue;
    if ( grounded )
    {
      // only slippy, non-flat surfaces don't allow jumps
      if ( INVALID_FAN != chronwhichfan[ichr] && mesh_has_some_bits( chronwhichfan[ichr], MESHFX_SLIPPY ) )
      {
        if ( !maptwistflat[twist] )
        {
          chrjumpready[ichr] = bfalse;
          dojumptimer       = bfalse;
        };
      }
    }

    if ( dojumptimer )
    {
      chrjumptime[ichr]  -= dUpdate;
      if ( chrjumptime[ichr] < 0 ) chrjumptime[ichr] = 0.0f;
    }

    // Characters with sticky butts lie on the surface of the mesh
    if ( grounded && ( chrstickybutt[ichr] || !chralive[ichr] ) )
    {
      chrmapturn_lr[ichr] = chrmapturn_lr[ichr] * 0.9 + maptwist_lr[twist] * 0.1;
      chrmapturn_ud[ichr] = chrmapturn_ud[ichr] * 0.9 + maptwist_ud[twist] * 0.1;
    }

    // Animate the character

    // do pancake anim
    chrpancakevel[ichr].x *= 0.90;
    chrpancakevel[ichr].y *= 0.90;
    chrpancakevel[ichr].z *= 0.90;

    chrpancakepos[ichr].x += chrpancakevel[ichr].x * dUpdate;
    chrpancakepos[ichr].y += chrpancakevel[ichr].y * dUpdate;
    chrpancakepos[ichr].z += chrpancakevel[ichr].z * dUpdate;

    if ( chrpancakepos[ichr].x < 0 ) { chrpancakepos[ichr].x = 0.001; chrpancakevel[ichr].x *= -0.5f; };
    if ( chrpancakepos[ichr].y < 0 ) { chrpancakepos[ichr].y = 0.001; chrpancakevel[ichr].y *= -0.5f; };
    if ( chrpancakepos[ichr].z < 0 ) { chrpancakepos[ichr].z = 0.001; chrpancakevel[ichr].z *= -0.5f; };

    chrpancakevel[ichr].x += ( 1.0f - chrpancakepos[ichr].x ) * dUpdate / 10.0f;
    chrpancakevel[ichr].y += ( 1.0f - chrpancakepos[ichr].y ) * dUpdate / 10.0f;
    chrpancakevel[ichr].z += ( 1.0f - chrpancakepos[ichr].z ) * dUpdate / 10.0f;

    // so the model's animation
    chrflip[ichr] += dUpdate * 0.25;
    ftmp = chrflip[ichr];
    while ( ftmp > 0.25f || chrflip[ichr] > 1.0f )
    {
      // convert flip into lip
      ftmp -= 0.25f;
      chrlip_fp8[ichr] += 64;

      // handle the mad fx
      if ( chrlip_fp8[ichr] == 192 )
      {
        // Check frame effects
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_ACTLEFT ) )
          character_swipe( ichr, SLOT_LEFT );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_ACTRIGHT ) )
          character_swipe( ichr, SLOT_RIGHT );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_GRABLEFT ) )
          character_grab_stuff( ichr, SLOT_LEFT, bfalse );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_GRABRIGHT ) )
          character_grab_stuff( ichr, SLOT_RIGHT, bfalse );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_CHARLEFT ) )
          character_grab_stuff( ichr, SLOT_LEFT, btrue );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_CHARRIGHT ) )
          character_grab_stuff( ichr, SLOT_RIGHT, btrue );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_DROPLEFT ) )
          detach_character_from_mount( chr_get_holdingwhich( ichr, SLOT_LEFT ), bfalse, btrue );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_DROPRIGHT ) )
          detach_character_from_mount( chr_get_holdingwhich( ichr, SLOT_RIGHT ), bfalse, btrue );
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_POOF ) && !chrisplayer[ichr] )
          chrgopoof[ichr] = btrue;
        if ( HAS_SOME_BITS( madframefx[chrmodel[ichr]][chrframe[ichr]], MADFX_FOOTFALL ) )
        {
          if ( INVALID_SOUND != capfootfallsound[imdl] )
          {
            float volume = ( ABS( chrvel[ichr].x ) +  ABS( chrvel[ichr].y ) ) / capsneakspd[imdl];
            play_sound( MIN( 1.0f, volume ), chrpos[ichr], capwavelist[imdl][capfootfallsound[imdl]], 0  );
          }
        }
      }

      // change frames
      if ( chrlip_fp8[ichr] == 0 )
      {
        chrflip[ichr] -= 1.0f;
        // Change frames
        chrframelast[ichr] = chrframe[ichr];
        chrframe[ichr]++;
        if ( chrframe[ichr] == madactionend[imdl][chraction[ichr]] )
        {
          // Action finished
          if ( chrkeepaction[ichr] )
          {
            // Keep the last frame going
            chrframe[ichr] = chrframelast[ichr];
          }
          else
          {
            if ( !chrloopaction[ichr] )
            {
              // Go on to the next action
              chraction[ichr]  = chrnextaction[ichr];
              chrnextaction[ichr] = ACTION_DA;
            }
            else
            {
              // See if the character is mounted...
              if ( VALID_CHR(imount) )
              {
                chraction[ichr] = ACTION_MI;
              }
            }
            chrframe[ichr] = madactionstart[imdl][chraction[ichr]];
          }
          chractionready[ichr] = btrue;
        }
      }

    };



    // Do "Be careful!" delay
    chrcarefultime[ichr] -= dUpdate;
    if ( chrcarefultime[ichr] <= 0 ) chrcarefultime[ichr] = 0;


    // Get running, walking, sneaking, or dancing, from speed
    if ( !chrkeepaction[ichr] && !chrloopaction[ichr] )
    {
      framelip = madframelip[chrmodel[ichr]][chrframe[ichr]];  // 0 - 15...  Way through animation
      if ( chractionready[ichr] && chrlip_fp8[ichr] == 0 && grounded && chrflyheight[ichr] == 0 && ( framelip&7 ) < 2 )
      {
        // Do the motion stuff
        speed = ABS( chrvel[ichr].x ) + ABS( chrvel[ichr].y );
        if ( speed < chrsneakspd[ichr] )
        {
          //                        chrnextaction[ichr] = ACTION_DA;
          // Do boredom
          chrboretime[ichr] -= dUpdate;
          if ( chrboretime[ichr] <= 0 ) chrboretime[ichr] = 0;

          if ( chrboretime[ichr] <= 0 )
          {
            chralert[ichr] |= ALERT_BORED;
            chrboretime[ichr] = DELAY_BORE;
          }
          else
          {
            // Do standstill
            if ( chraction[ichr] > ACTION_DD )
            {
              chraction[ichr] = ACTION_DA;
              chrframe[ichr] = madactionstart[imdl][chraction[ichr]];
            }
          }
        }
        else
        {
          chrboretime[ichr] = DELAY_BORE;
          if ( speed < chrwalkspd[ichr] )
          {
            chrnextaction[ichr] = ACTION_WA;
            if ( chraction[ichr] != ACTION_WA )
            {
              chrframe[ichr] = madframeliptowalkframe[imdl][LIPT_WA][framelip];
              chraction[ichr] = ACTION_WA;
            }
          }
          else
          {
            if ( speed < chrrunspd[ichr] )
            {
              chrnextaction[ichr] = ACTION_WB;
              if ( chraction[ichr] != ACTION_WB )
              {
                chrframe[ichr] = madframeliptowalkframe[imdl][LIPT_WB][framelip];
                chraction[ichr] = ACTION_WB;
              }
            }
            else
            {
              chrnextaction[ichr] = ACTION_WC;
              if ( chraction[ichr] != ACTION_WC )
              {
                chrframe[ichr] = madframeliptowalkframe[imdl][LIPT_WC][framelip];
                chraction[ichr] = ACTION_WC;
              }
            }
          }
        }
      }
    }
  }


}

//--------------------------------------------------------------------------------------------
void setup_characters( char *modname )
{
  // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
  Uint16 currentcharacter = MAXCHR, lastcharacter = MAXCHR, tmpchr = MAXCHR;
  int passage, content, money, level, skin, tnc, localnumber = 0;
  bool_t stat, ghost;
  TEAM team;
  Uint8 cTmp;
  char *name;
  char itislocal;
  STRING myname, newloadname;
  Uint16 facing, attach;
  Uint32 slot;
  vect3 pos;
  FILE *fileread;
  GRIP grip;


  // Turn all characters off
  free_all_characters();


  // Turn some back on
  snprintf( newloadname, sizeof( newloadname ), "%s%s/%s", modname, CData.gamedat_dir, CData.spawn_file );
  fileread = fs_fileOpen( PRI_FAIL, "setup_characters()", newloadname, "r" );
  currentcharacter = MAXCHR;
  if ( NULL == fileread )
  {
    log_error( "Error loading module: %s\n", newloadname );  //Something went wrong
    return;
  }


  while ( fget_next_string( fileread, myname, sizeof( myname ) ) )
  {
    convert_underscores( myname, sizeof( myname ), myname );

    name = myname;
    if ( 0 == strcmp( "NONE", myname ) )
    {
      // Random name
      name = NULL;
    }

    fscanf( fileread, "%d", &slot );

    fscanf( fileread, "%f%f%f", &pos.x, &pos.y, &pos.z );
    pos.x = MESH_FAN_TO_FLOAT( pos.x );
    pos.y = MESH_FAN_TO_FLOAT( pos.y );
    pos.z = MESH_FAN_TO_FLOAT( pos.z );

    cTmp = fget_first_letter( fileread );
    attach = MAXCHR;
    facing = NORTH;
    grip = GRIP_SADDLE;
    switch ( toupper( cTmp ) )
    {
      case 'N':  facing = NORTH; break;
      case 'S':  facing = SOUTH; break;
      case 'E':  facing = EAST; break;
      case 'W':  facing = WEST; break;
      case 'L':  attach = currentcharacter; grip = GRIP_LEFT;      break;
      case 'R':  attach = currentcharacter; grip = GRIP_RIGHT;     break;
      case 'I':  attach = currentcharacter; grip = GRIP_INVENTORY; break;
    };
    fscanf( fileread, "%d%d%d%d%d", &money, &skin, &passage, &content, &level );
    stat = fget_bool( fileread );
    ghost = fget_bool( fileread );
    team = fget_first_letter( fileread ) - 'A';
    if ( team > TEAM_COUNT ) team = TEAM_NULL;


    // Spawn the character
    tmpchr = spawn_one_character( pos, slot, team, skin, facing, name, MAXCHR );
    if ( VALID_CHR( tmpchr ) )
    {
      lastcharacter = tmpchr;

      chrmoney[lastcharacter] += money;
      if ( chrmoney[lastcharacter] > MAXMONEY )  chrmoney[lastcharacter] = MAXMONEY;
      if ( chrmoney[lastcharacter] < 0 )  chrmoney[lastcharacter] = 0;
      chraicontent[lastcharacter] = content;
      chrpassage[lastcharacter] = passage;
      if ( !VALID_CHR( attach ) )
      {
        // Free character
        currentcharacter = lastcharacter;
      }
      else
      {
        // Attached character
        if ( grip == GRIP_INVENTORY )
        {
          // Inventory character
          if ( add_item_to_character_pack( lastcharacter, currentcharacter ) )
          {
            // actually do the attachment to the inventory
            Uint16 tmpchr = chr_get_attachedto(lastcharacter);
            chralert[lastcharacter] |= ALERT_GRABBED;                       // Make spellbooks change

            // fake that it was grabbed by the left hand
            chrattachedto[lastcharacter] = VALIDATE_CHR(currentcharacter);  // Make grab work
            chrinwhichslot[lastcharacter] = SLOT_INVENTORY;
            let_character_think( lastcharacter, 1.0f );                     // Empty the grabbed messages

            // restore the proper attachment and slot variables
            chrattachedto[lastcharacter] = MAXCHR;                          // Fix grab
            chrinwhichslot[lastcharacter] = SLOT_NONE;
          };
        }
        else
        {
          // Wielded character
          if ( attach_character_to_mount( lastcharacter, currentcharacter, grip_to_slot( grip ) ) )
          {
            let_character_think( lastcharacter, 1.0f );   // Empty the grabbed messages
          };
        }
      }

      // Turn on player input devices
      if ( stat )
      {
        if ( importamount == 0 )
        {
          if ( numstat == 0 )
          {
            // Single player module
            add_player( lastcharacter, numstat, INBITS_MOUS | INBITS_KEYB | INBITS_JOYA | INBITS_JOYB );
          };
        }
        else if ( numstat < importamount )
        {
          // Multiplayer module
          itislocal = bfalse;
          tnc = 0;
          while ( tnc < numimport )
          {
            if ( capimportslot[chrmodel[lastcharacter]] == localslot[tnc] )
            {
              itislocal = btrue;
              localnumber = tnc;
              break;
            }
            tnc++;
          }


          if ( itislocal )
          {
            // It's a local player
            add_player( lastcharacter, numstat, localcontrol[localnumber] );
          }
          else
          {
            // It's a remote player
            add_player( lastcharacter, numstat, INBITS_NONE );
          }
        }

        // Turn on the stat display
        add_stat( lastcharacter );
      }

      // Set the starting level
      if ( !chrisplayer[lastcharacter] )
      {
        // Let the character gain levels
        level -= 1;
        while ( chrexperiencelevel[lastcharacter] < level && chrexperience[lastcharacter] < MAXXP )
        {
          give_experience( lastcharacter, 100, XP_DIRECT );
        }
      }
      if ( ghost )  // Outdated, should be removed.
      {
        // Make the character a ghost !!!BAD!!!  Can do with enchants
        chralpha_fp8[lastcharacter] = 128;
        chrbumpstrength[lastcharacter] = capbumpstrength[chrmodel[lastcharacter]] * FP8_TO_FLOAT( chralpha_fp8[lastcharacter] );
        chrlight_fp8[lastcharacter] = 255;
      }
    }
  }
  fs_fileClose( fileread );



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
  float dist;
  float inputx, inputy;


  // Check to see if we need to bother
  if ( !VALID_PLA( player ) || INBITS_NONE == pladevice[player] ) return;

  // Make life easier
  character = pla_get_character( player );
  device    = pladevice[player];

  // Clear the player's latch buffers
  plalatchbutton[player] = 0;
  plalatchx[player] *= mousesustain;
  plalatchy[player] *= mousesustain;

  // Mouse routines
  if ( HAS_SOME_BITS( device, INBITS_MOUS ) && mouseon )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_mouse_is_pressed( CONTROL_CAMERA ) )   // Don't allow movement in camera control mode
    {
      inputx = 0;
      inputy = 0;
      dist = mousedx * mousedx + mousedy * mousedy;

      if ( dist > 0.0 )
      {
        dist = sqrt( dist );
        inputx = ( float ) mousedx / ( mousesense + dist );
        inputy = ( float ) mousedy / ( mousesense + dist );
      }
      if ( CData.autoturncamera == 255 && control_mouse_is_pressed( CONTROL_CAMERA ) == 0 )  inputx = 0;

      turncos = ((Uint16)camturn_lr) >> 2;
      turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
      newx = ( inputx * turntosin[turncos] + inputy * turntosin[turnsin] );
      newy = (-inputx * turntosin[turnsin] + inputy * turntosin[turncos] );
    }

    plalatchx[player] += newx * mousecover * 5;
    plalatchy[player] += newy * mousecover * 5;

    // Read buttons
    if ( control_mouse_is_pressed( CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !chralive[character] )
      {
        plalatchbutton[player] |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        plalatchbutton[player] |= LATCHBUTTON_JUMP;
      }
    };

    if ( control_mouse_is_pressed( CONTROL_LEFT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_LEFT;

    if ( control_mouse_is_pressed( CONTROL_LEFT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;

    if ( control_mouse_is_pressed( CONTROL_LEFT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_RIGHT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
  }


  // Joystick A routines
  if ( HAS_SOME_BITS( device, INBITS_JOYA ) && joyaon )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_joya_is_pressed( CONTROL_CAMERA ) )
    {
      inputx = joyax;
      inputy = joyay;
      dist = joyax * joyax + joyay * joyay;
      if ( dist > 1.0 )
      {
        dist = sqrt( dist );
        inputx /= dist;
        inputy /= dist;
      }
      if ( CData.autoturncamera == 255 && !control_joya_is_pressed( CONTROL_CAMERA ) )  inputx = 0;

      turncos = ((Uint16)camturn_lr) >> 2;
      turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
      newx = ( inputx * turntosin[turncos] + inputy * turntosin[turnsin] );
      newy = ( -inputx * turntosin[turnsin] + inputy * turntosin[turncos] );
    }

    plalatchx[player] += newx * mousecover;
    plalatchy[player] += newy * mousecover;

    // Read buttons
    if ( control_joya_is_pressed( CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !chralive[character] )
      {
        plalatchbutton[player] |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        plalatchbutton[player] |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_joya_is_pressed( CONTROL_LEFT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_LEFT;

    if ( control_joya_is_pressed( CONTROL_LEFT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;

    if ( control_joya_is_pressed( CONTROL_LEFT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;

    if ( control_joya_is_pressed( CONTROL_RIGHT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_RIGHT;

    if ( control_joya_is_pressed( CONTROL_RIGHT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;

    if ( control_joya_is_pressed( CONTROL_RIGHT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
  }


  // Joystick B routines
  if ( HAS_SOME_BITS( device, INBITS_JOYB ) && joybon )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_joyb_is_pressed( CONTROL_CAMERA ) )
    {
      inputx = joybx;
      inputy = joyby;
      dist = joybx * joybx + joyby * joyby;
      if ( dist > 1.0 )
      {
        dist = sqrt( dist );
        inputx = joybx / dist;
        inputy = joyby / dist;
      }
      if ( CData.autoturncamera == 255 && !control_joyb_is_pressed( CONTROL_CAMERA ) )  inputx = 0;

      turncos = ((Uint16)camturn_lr) >> 2;
      turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
      newx = ( inputx * turntosin[turncos] + inputy * turntosin[turnsin] );
      newy = ( -inputx * turntosin[turnsin] + inputy * turntosin[turncos] );
    }

    plalatchx[player] += newx * mousecover;
    plalatchy[player] += newy * mousecover;

    // Read buttons
    if ( control_joyb_is_pressed( CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !chralive[character] )
      {
        plalatchbutton[player] |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        plalatchbutton[player] |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_joyb_is_pressed( CONTROL_LEFT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_LEFT;

    if ( control_joyb_is_pressed( CONTROL_LEFT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;

    if ( control_joyb_is_pressed( CONTROL_LEFT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;

    if ( control_joyb_is_pressed( CONTROL_RIGHT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_RIGHT;

    if ( control_joyb_is_pressed( CONTROL_RIGHT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;

    if ( control_joyb_is_pressed( CONTROL_RIGHT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
  }

  // Keyboard routines
  if ( HAS_SOME_BITS( device, INBITS_KEYB ) && keyon )
  {
    // Movement
    newx = 0;
    newy = 0;
    inputx = inputy = 0;
    if ( control_key_is_pressed( KEY_RIGHT ) ) inputx += 1;
    if ( control_key_is_pressed( KEY_LEFT ) ) inputx -= 1;
    if ( control_key_is_pressed( KEY_DOWN ) ) inputy += 1;
    if ( control_key_is_pressed( KEY_UP ) ) inputy -= 1;
    dist = inputx * inputx + inputy * inputy;
    if ( dist > 1.0 )
    {
      dist = sqrt( dist );
      inputx /= dist;
      inputy /= dist;
    }
    if ( CData.autoturncamera == 255 && numlocalpla == 1 )  inputx = 0;

    
    turncos = ((Uint16)camturn_lr) >> 2;
    turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
    newx = ( inputx * turntosin[turncos]  + inputy * turntosin[turnsin] );
    newy = ( -inputx * turntosin[turnsin] + inputy * turntosin[turncos] );

    plalatchx[player] += newx * mousecover;
    plalatchy[player] += newy * mousecover;

    // Read buttons
    if ( control_key_is_pressed( CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !chralive[character] )
      {
        plalatchbutton[player] |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        plalatchbutton[player] |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_key_is_pressed( CONTROL_LEFT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_LEFT;

    if ( control_key_is_pressed( CONTROL_LEFT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTLEFT;

    if ( control_key_is_pressed( CONTROL_LEFT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKLEFT;

    if ( control_key_is_pressed( CONTROL_RIGHT_USE ) )
      plalatchbutton[player] |= LATCHBUTTON_RIGHT;

    if ( control_key_is_pressed( CONTROL_RIGHT_GET ) )
      plalatchbutton[player] |= LATCHBUTTON_ALTRIGHT;

    if ( control_key_is_pressed( CONTROL_RIGHT_PACK ) )
      plalatchbutton[player] |= LATCHBUTTON_PACKRIGHT;
  }

  dist = plalatchx[player] * plalatchx[player] + plalatchy[player] * plalatchy[player];
  if ( dist > 1 )
  {
    dist = sqrt( dist );
    plalatchx[player] /= dist;
    plalatchy[player] /= dist;
  };
}

//--------------------------------------------------------------------------------------------
void set_local_latches( void )
{
  // ZZ> This function emulates AI thinkin' by setting latches from input devices
  int cnt;

  cnt = 0;
  while ( cnt < MAXPLAYER )
  {
    set_one_player_latch( cnt );
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
float get_one_level( CHR_REF character )
{
  float level;
  Uint16 platform;

  // return a default for invalid characters
  if ( !VALID_CHR( character ) ) return bfalse;

  // return the cached value for pre-calculated characters
  if ( chrlevelvalid[character] ) return chrlevel[character];

  //get the base level
  chronwhichfan[character] = mesh_get_fan( chrpos[character] );
  level = mesh_get_level( chronwhichfan[character], chrpos[character].x, chrpos[character].y, chrwaterwalk[character] );

  // if there is a platform, choose whichever is higher
  platform = chr_get_onwhichplatform( character );
  if ( VALID_CHR( platform ) )
  {
    float ftmp = chrbmpdata[platform].cv.z_max;
    level = MAX( level, ftmp );
  }

  chrlevel[character]      = level;
  chrlevelvalid[character] = btrue;

  return chrlevel[character];
};

//--------------------------------------------------------------------------------------------
void get_all_levels( void )
{
  CHR_REF character;

  // Initialize all the objects
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) ) continue;

    chronwhichfan[character] = INVALID_FAN;
    chrlevelvalid[character] = bfalse;
  };

  // do the levels
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || chrlevelvalid[character] ) continue;
    get_one_level( character );
  }

}


//--------------------------------------------------------------------------------------------
void make_onwhichfan( void )
{
  // ZZ> This function figures out which fan characters are on and sets their level
  CHR_REF character;
  int ripand;

  float  splashstrength = 1.0f, ripplesize = 1.0f, ripplestrength = 0.0f;
  bool_t is_inwater    = bfalse;
  bool_t is_underwater = bfalse;


  // Get levels every update
  get_all_levels();

  // Get levels every update
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) ) continue;

    is_inwater = is_underwater = bfalse;
    splashstrength = 0.0f;
    ripplesize = 0.0f;
    if ( INVALID_FAN != chronwhichfan[character] && mesh_has_some_bits( chronwhichfan[character], MESHFX_WATER ) )
    {
      splashstrength = chrbmpdata[character].calc_size_big / 45.0f * chrbmpdata[character].calc_size / 30.0f;
      if ( chrvel[character].z > 0.0f ) splashstrength *= 0.5;
      splashstrength *= ABS( chrvel[character].z ) / 10.0f;
      splashstrength *= chrbumpstrength[character];
      if ( chrpos[character].z < watersurfacelevel )
      {
        is_inwater = btrue;
      }

      ripplesize = ( chrbmpdata[character].calc_size + chrbmpdata[character].calc_size_big ) * 0.5f;
      if ( chrbmpdata[character].cv.z_max < watersurfacelevel )
      {
        is_underwater = btrue;
      }

      // scale the ripple strength
      ripplestrength = - ( chrbmpdata[character].cv.z_min - watersurfacelevel ) * ( chrbmpdata[character].cv.z_max - watersurfacelevel );
      ripplestrength /= 0.75f * chrbmpdata[character].calc_height * chrbmpdata[character].calc_height;
      ripplestrength *= ripplesize / 37.5f * chrbumpstrength[character];
      ripplestrength = MAX( 0.0f, ripplestrength );
    };

    // splash stuff
    if ( chrinwater[character] != is_inwater && splashstrength > 0.1f )
    {
      vect3 prt_pos = {chrpos[character].x, chrpos[character].y, watersurfacelevel + RAISE};
      Uint16 prt_index;

      // Splash
      prt_index = spawn_one_particle( splashstrength, prt_pos, 0, MAXMODEL, PRTPIP_SPLASH, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );

      // scale the size of the particle
      prtsize_fp8[prt_index] *= splashstrength;

      // scale the animation speed so that velocity appears the same
      if ( 0 != prtimageadd_fp8[prt_index] && 0 != prtsizeadd_fp8[prt_index] )
      {
        splashstrength = sqrt( splashstrength );
        prtimageadd_fp8[prt_index] /= splashstrength;
        prtsizeadd_fp8[prt_index]  /= splashstrength;
      }
      else
      {
        prtimageadd_fp8[prt_index] /= splashstrength;
        prtsizeadd_fp8[prt_index]  /= splashstrength;
      }


      chrinwater[character] = is_inwater;
      if ( wateriswater && is_inwater )
      {
        chralert[character] |= ALERT_INWATER;
      }
    }
    else if ( is_inwater && ripplestrength > 0.0f )
    {
      // Ripples
      ripand = ((( int ) chrvel[character].x ) != 0 ) | ((( int ) chrvel[character].y ) != 0 );
      ripand = RIPPLEAND >> ripand;
      if ( 0 == ( wldframe&ripand ) )
      {
        vect3  prt_pos = {chrpos[character].x, chrpos[character].y, watersurfacelevel};
        Uint16 prt_index;

        prt_index = spawn_one_particle( ripplestrength, prt_pos, 0, MAXMODEL, PRTPIP_RIPPLE, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );

        // scale the size of the particle
        prtsize_fp8[prt_index] *= ripplesize;

        // scale the animation speed so that velocity appears the same
        if ( 0 != prtimageadd_fp8[prt_index] && 0 != prtsizeadd_fp8[prt_index] )
        {
          ripplesize = sqrt( ripplesize );
          prtimageadd_fp8[prt_index] /= ripplesize;
          prtsizeadd_fp8[prt_index]  /= ripplesize;
        }
        else
        {
          prtimageadd_fp8[prt_index] /= ripplesize;
          prtsizeadd_fp8[prt_index]  /= ripplesize;
        }
      }
    }

    // damage tile stuff
    if ( mesh_has_some_bits( chronwhichfan[character], MESHFX_DAMAGE ) && chrpos[character].z <= watersurfacelevel + DAMAGERAISE )
    {
      Uint8 loc_damagemodifier;
      CHR_REF imount;

      // augment the rider's damage immunity with the mount's
      loc_damagemodifier = chrdamagemodifier_fp8[character][damagetiletype];
      imount = chr_get_attachedto(character);
      if ( VALID_CHR(imount) )
      {
        Uint8 modbits1, modbits2, modshift1, modshift2;
        Uint8 tmp_damagemodifier;
        
        tmp_damagemodifier = chrdamagemodifier_fp8[imount][damagetiletype];

        modbits1  = loc_damagemodifier & (~DAMAGE_SHIFT);
        modshift1 = loc_damagemodifier & DAMAGE_SHIFT;

        modbits2  = tmp_damagemodifier & (~DAMAGE_SHIFT);
        modshift2 = tmp_damagemodifier & DAMAGE_SHIFT;

        loc_damagemodifier = (modbits1 | modbits2) | MAX(modshift1, modshift2);
      }

      if ( !HAS_ALL_BITS(loc_damagemodifier, DAMAGE_SHIFT ) && !chrinvictus[character] )  // DAMAGE_SHIFT means they're pretty well immune
      {
        if ( chrdamagetime[character] == 0 )
        {
          PAIR ptemp = {damagetileamount, 1};
          damage_character( character, 32768, &ptemp, damagetiletype, TEAM_DAMAGE, chr_get_aibumplast( character ), DAMFX_BLOC | DAMFX_ARMO );
          chrdamagetime[character] = DELAY_DAMAGETILE;
        }

        if ( damagetileparttype != MAXPRTPIP && ( wldframe&damagetilepartand ) == 0 )
        {
          spawn_one_particle( 1.0f, chrpos[character],
                              0, MAXMODEL, damagetileparttype, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
        }

      }

      if ( chrreaffirmdamagetype[character] == damagetiletype )
      {
        if (( wldframe&TILEREAFFIRMAND ) == 0 )
          reaffirm_attached_particles( character );
      }
    }


  }

}

//--------------------------------------------------------------------------------------------
bool_t remove_from_platform( Uint16 object )
{
  Uint16 platform;
  if ( !VALID_CHR( object ) ) return bfalse;

  platform  = chr_get_onwhichplatform( object );
  if ( !VALID_CHR( platform ) ) return bfalse;

  if ( chrweight[object] > 0.0f )
    chrweight[platform] -= chrweight[object];

  chronwhichplatform[object] = MAXCHR;
  chrlevel[object]           = chrlevel[platform];

  if ( chrisplayer[object] && CData.DevMode )
    debug_message( 1, "removel %s(%s) from platform", chrname[object], capclassname[chrmodel[object]] );


  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_to_platform( Uint16 object, Uint16 platform )
{
  remove_from_platform( object );

  if ( !VALID_CHR( object ) || !VALID_CHR( platform ) ) return
      bfalse;

  if ( !chrbmpdata[platform].calc_is_platform )
    return bfalse;

  chronwhichplatform[object]  = platform;
  if ( chrweight[object] > 0.0f )
    chrholdingweight[platform] += chrweight[object];

  chrjumpready[object]  = btrue;
  chrjumpnumber[object] = chrjumpnumberreset[object];

  chrlevel[object] = chrbmpdata[platform].cv.z_max;

  if ( chrisplayer[object] )
    debug_message( 1, "attached %s(%s) to platform", chrname[object], capclassname[chrmodel[object]] );

  return btrue;
};

//--------------------------------------------------------------------------------------------
void create_bumplists()
{
  CHR_REF ichr, entry;
  PRT_REF iprt;
  Uint32 fanblock;
  Sint8 hide;

  // Clear the lists
  fanblock = 0;
  while ( fanblock < numfanblock )
  {
    bumplistchrnum[fanblock] = 0;
    bumplistchr[fanblock]    = MAXCHR;
    bumplistprtnum[fanblock] = 0;
    bumplistprt[fanblock]    = MAXPRT;
    fanblock++;
  }


  // Fill 'em back up
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    int ix, ix_min, ix_max;
    int iy, iy_min, iy_max;

    // ignore invalid characters and objects that are packed away
    if ( !VALID_CHR( ichr ) || chr_in_pack( ichr ) ) continue;

    // do not include hidden objects
    hide = caphidestate[chrmodel[ichr]];
    if ( hide != NOHIDE && hide == chraistate[ichr] ) continue;

    ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( chrbmpdata[ichr].cv.x_min ) );
    ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( chrbmpdata[ichr].cv.x_max ) );
    iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( chrbmpdata[ichr].cv.y_min ) );
    iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( chrbmpdata[ichr].cv.y_max ) );

    for ( ix = ix_min; ix <= ix_max; ix++ )
    {
      for ( iy = iy_min; iy <= iy_max; iy++ )
      {
        fanblock = mesh_convert_block( ix, iy );
        if ( INVALID_FAN == fanblock ) continue;

        // Insert before any other characters on the block
        entry = bumplistchr[fanblock];
        chrbumpnext[ichr] = entry;
        bumplistchr[fanblock] = ichr;
        bumplistchrnum[fanblock]++;
      }
    }
  };


  for ( iprt = 0; iprt < MAXPRT; iprt++ )
  {
    // ignore invalid particles
    if ( !VALID_PRT( iprt ) || prtgopoof[iprt] ) continue;

    fanblock = mesh_get_block( prtpos[iprt] );
    if ( INVALID_FAN == fanblock ) continue;

    // Insert before any other particles on the block
    entry = bumplistprt[fanblock];
    prtbumpnext[iprt] = entry;
    bumplistprt[fanblock] = iprt;
    bumplistprtnum[fanblock]++;
  }
};


//--------------------------------------------------------------------------------------------
bool_t find_collision_volume( vect3 * ppa, CVolume * pva, vect3 * ppb, CVolume * pvb, bool_t exclude_vert, CVolume * pcv)
{
  bool_t retval = bfalse, bfound;
  CVolume cv, tmp_cv;
  float ftmp;

  if( NULL == ppa || NULL == pva || NULL == ppb || NULL == pvb ) return bfalse;


  //---- do the preliminary collision test ----

  // do diagonal
  cv.xy_min = MAX(pva->xy_min + ppa->x + ppa->y, pvb->xy_min + ppb->x + ppb->y);
  cv.xy_max = MIN(pva->xy_max + ppa->x + ppa->y, pvb->xy_max + ppb->x + ppb->y);
  if(cv.xy_min >= cv.xy_max) return bfalse;

  cv.yx_min = MAX(pva->yx_min - ppa->x + ppa->y, pvb->yx_min - ppb->x + ppb->y);
  cv.yx_max = MIN(pva->yx_max - ppa->x + ppa->y, pvb->yx_max - ppb->x + ppb->y);
  if(cv.yx_min >= cv.yx_max) return bfalse;

  // do square
  cv.x_min = MAX(pva->x_min + ppa->x, pvb->x_min + ppb->x);
  cv.x_max = MIN(pva->x_max + ppa->x, pvb->x_max + ppb->x);
  if(cv.x_min >= cv.x_max) return bfalse;

  cv.y_min = MAX(pva->y_min + ppa->y, pvb->y_min + ppb->y);
  cv.y_max = MIN(pva->y_max + ppa->y, pvb->y_max + ppb->y);
  if(cv.y_min >= cv.y_max) return bfalse;

  // do vert
  cv.z_min = MAX(pva->z_min + ppa->z, pvb->z_min + ppb->z);
  cv.z_max = MIN(pva->z_max + ppa->z, pvb->z_max + ppb->z);
  if(!exclude_vert && cv.z_min >= cv.z_max) return bfalse;

  //---- limit the collision volume ----

  tmp_cv = cv;


  //=================================================================
  // treat the edges of the square bbox as line segments and clip them using the 
  // diagonal bbox
  //=================================================================
  // do the y segments
  bfound = bfalse;
  {
    // do the y = x_min segment

    float y_min, y_max;
    bool_t bexcluded = bfalse;

    bexcluded = bfalse;
    y_min = cv.y_min;
    y_max = cv.y_max;

    if(!bexcluded)
    {
      ftmp = -cv.x_min + cv.xy_min;
      y_min = MAX(y_min, ftmp);

      ftmp = -cv.x_min + cv.xy_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    }

    if(!bexcluded)
    {
      ftmp = cv.yx_min + cv.x_min;
      y_min = MAX(y_min, ftmp);

      ftmp = cv.x_min + cv.yx_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    };

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {
      if(!bfound)
      {
        tmp_cv.y_min = y_min;
        tmp_cv.y_max = y_max;
      }
      else
      {
        tmp_cv.y_min = MIN(tmp_cv.y_min, y_min);
        tmp_cv.y_max = MAX(tmp_cv.y_max, y_max);
      }
      assert(tmp_cv.y_min <= tmp_cv.y_max);
      bfound = btrue;
    }
  }

  //=================================================================
  {
    // do the y = x_max segment

    float y_min, y_max;
    bool_t bexcluded = bfalse;

    //---------------------

    bexcluded = bfalse;
    y_min = cv.y_min;
    y_max = cv.y_max;

    if(!bexcluded)
    {
      ftmp = -cv.x_max + cv.xy_min;
      y_min = MAX(y_min, ftmp);

      ftmp = -cv.x_max + cv.xy_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    }


    if(!bexcluded)
    {
      ftmp = cv.yx_min + cv.x_max;
      y_min = MAX(y_min, ftmp);

      ftmp = cv.x_max + cv.yx_max;
      y_max = MIN(y_max, ftmp);

      bexcluded = y_min >= y_max;
    };

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {

      if(!bfound)
      {
        tmp_cv.y_min = y_min;
        tmp_cv.y_max = y_max;
      }
      else
      {
        tmp_cv.y_min = MIN(tmp_cv.y_min, y_min);
        tmp_cv.y_max = MAX(tmp_cv.y_max, y_max);
      };
      assert(tmp_cv.y_min <= tmp_cv.y_max);

      bfound = btrue;
    }
  }

  //=================================================================
  // do the x segments
  bfound = bfalse;
  {
    // do the x = y_min segment

    float x_min, x_max;
    bool_t bexcluded = bfalse;

    bexcluded = bfalse;
    x_min = cv.x_min;
    x_max = cv.x_max;

    if(!bexcluded)
    {
      ftmp = cv.xy_min - cv.y_min;
      x_min = MAX(x_min, ftmp);

      ftmp = -cv.yx_min + cv.y_min;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }


    if(!bexcluded)
    {
      ftmp = -cv.yx_max + cv.y_min;
      x_min = MAX(x_min, ftmp);

      ftmp = cv.xy_max - cv.y_min;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {
      if(!bfound)
      {
        tmp_cv.x_min = x_min;
        tmp_cv.x_max = x_max;
      }
      else
      {
        tmp_cv.x_min = MIN(tmp_cv.x_min, x_min);
        tmp_cv.x_max = MAX(tmp_cv.x_max, x_max);
      }
      assert(tmp_cv.x_min <= tmp_cv.x_max);
      bfound = btrue;
    }


  }

  //=================================================================
  {
    // do the x = y_max segment

    float x_min, x_max;
    bool_t bexcluded = bfalse;

    bexcluded = bfalse;
    x_min = cv.x_min;
    x_max = cv.x_max;

    if(!bexcluded)
    {
      ftmp = cv.xy_min - cv.y_max;
      x_min = MAX(x_min, ftmp);

      ftmp = -cv.yx_min + cv.y_max;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }


    if(!bexcluded)
    {
      ftmp = -cv.yx_max + cv.y_max;
      x_min = MAX(x_min, ftmp);

      ftmp = cv.xy_max - cv.y_max;
      x_max = MIN(x_max, ftmp);

      bexcluded = x_min >= x_max;
    }

    // if this line segment still exists, use it to define the collision colume
    if(!bexcluded)
    {
      if(!bfound)
      {
        tmp_cv.x_min = x_min;
        tmp_cv.x_max = x_max;
      }
      else
      {
        tmp_cv.x_min = MIN(tmp_cv.x_min, x_min);
        tmp_cv.x_max = MAX(tmp_cv.x_max, x_max);
      };
      assert(tmp_cv.x_min <= tmp_cv.x_max);
      bfound = btrue;
    }
  }


  if(NULL != pcv)
  {
    *pcv = tmp_cv;
  }

  return btrue;
}



//--------------------------------------------------------------------------------------------
bool_t chr_is_inside( CHR_REF chra, float lerp, CHR_REF chrb, bool_t exclude_vert )
{
  // BB > Find whether an active point of chra is "inside" chrb's bounding volume. 
  //      Abstraction of the old algorithm to see whether a character cpold be "above" another 

  float ftmp;
  BData * ba, * bb;

  if( !VALID_CHR(chra) || !VALID_CHR(chrb) ) return bfalse;

  ba = &chrbmpdata[chra];
  bb = &chrbmpdata[chrb];

  //---- vertical ----
  if( !exclude_vert )
  {
    ftmp = ba->mids_lo.z + (ba->mids_hi.z - ba->mids_lo.z) * lerp;
    if( ftmp < bb->cv.z_min + bb->mids_lo.z || ftmp > bb->cv.z_max + bb->mids_lo.z ) return bfalse;
  }
  
  //---- diamond ----
  if( bb->cv.level > 1 )
  {
    ftmp = ba->mids_lo.x + ba->mids_lo.y;
    if( ftmp < bb->cv.xy_min + bb->mids_lo.x + bb->mids_lo.y || ftmp > bb->cv.xy_max + bb->mids_lo.x + bb->mids_lo.y ) return bfalse;
  
    ftmp = -ba->mids_lo.x + ba->mids_lo.y;
    if( ftmp < bb->cv.yx_min - bb->mids_lo.x + bb->mids_lo.y || ftmp > bb->cv.yx_max - bb->mids_lo.x + bb->mids_lo.y ) return bfalse;
  };

  //---- square ----
  ftmp = ba->mids_lo.x;
  if( ftmp < bb->cv.x_min + bb->mids_lo.x || ftmp > bb->cv.x_max + bb->mids_lo.x ) return bfalse;

  ftmp = ba->mids_lo.y;
  if( ftmp < bb->cv.y_min + bb->mids_lo.y || ftmp > bb->cv.y_max + bb->mids_lo.y ) return bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_do_collision( CHR_REF chra, CHR_REF chrb, bool_t exclude_height, CVolume * cv)
{
  // BB > use the bounding boxes to determine whether a collision has occurred.
  //      there are currently 3 levels of collision detection.
  //      level 1 - the basic square axis-aligned bounding box
  //      level 2 - the octagon bounding box calculated from the actual vertex positions
  //      level 3 - an "octree" of bounding bounding boxes calculated from the actual trianglr positions
  //  the level is chosen by the global variable chrcollisionlevel

  bool_t retval = bfalse;
  
  // set the minimum bumper level for object a
  if(chrbmpdata[chra].cv.level < 1)
  {
    md2_calculate_bumpers(chra, 1);
  }

  // set the minimum bumper level for object b
  if(chrbmpdata[chrb].cv.level < 1)
  {
    md2_calculate_bumpers(chrb, 1);
  }

  // find the simplest collision volume
  find_collision_volume( &chrpos[chra], &chrbmpdata[chra].cv, &chrpos[chrb], &chrbmpdata[chrb].cv, exclude_height, cv);

  if ( chrcollisionlevel>1 && retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(chrbmpdata[chra].cv.level < 2)
    {
      md2_calculate_bumpers(chra, 2);
      was_refined = btrue;
    }

    // refine the bumper
    if(chrbmpdata[chrb].cv.level < 2)
    {
      md2_calculate_bumpers(chrb, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &chrpos[chra], &chrbmpdata[chra].cv, &chrpos[chrb], &chrbmpdata[chrb].cv, exclude_height, cv);
    };

    if(chrcollisionlevel>2 && retval)
    {
      was_refined = bfalse;

      // refine the bumper
      if(chrbmpdata[chra].cv.level < 3)
      {
        md2_calculate_bumpers(chra, 3);
        was_refined = btrue;
      }

      // refine the bumper
      if(chrbmpdata[chrb].cv.level < 3)
      {
        md2_calculate_bumpers(chrb, 3);
        was_refined = btrue;
      }

      assert(NULL != chrbmpdata[chra].cv_tree);
      assert(NULL != chrbmpdata[chrb].cv_tree);

      if(was_refined)
      {
        int cnt, tnc;
        CVolume cv3, tmp_cv;
        bool_t loc_retval;

        retval = bfalse;
        cv3.level = -1;
        for(cnt=0; cnt<8; cnt++)
        {
          if(-1 == (*chrbmpdata[chra].cv_tree)[cnt].level) continue;

          for(tnc=0; tnc<8; tnc++)
          {
            if(-1 == (*chrbmpdata[chrb].cv_tree)[cnt].level) continue;

            loc_retval = find_collision_volume( &chrpos[chra], &((*chrbmpdata[chra].cv_tree)[cnt]), &chrpos[chrb], &((*chrbmpdata[chrb].cv_tree)[cnt]), exclude_height, &tmp_cv);

            if(loc_retval)
            {
              retval = btrue;
              cv3 = cvolume_merge(&cv3, &tmp_cv);

#if defined(DEBUG_CVOLUME) && defined(_DEBUG)
              if(CData.DevMode)
              {              
                cv_list_add( &tmp_cv );
              }
#endif
            }
          };
        };

        if(retval)
        {
          *cv = cv3;
        };
      };
    };
  }

#if defined(DEBUG_CVOLUME) && defined(_DEBUG)
  if(CData.DevMode && retval)
  {
    cv_list_add( cv );
  }
#endif

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_do_collision( CHR_REF chra, PRT_REF prtb, bool_t exclude_height )
{
  bool_t retval = find_collision_volume( &chrpos[chra], &chrbmpdata[chra].cv, &chrpos[prtb], &chrbmpdata[prtb].cv, bfalse, NULL );

  if ( retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(chrbmpdata[chra].cv.level < 2)
    {
      md2_calculate_bumpers(chra, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &chrpos[chra], &chrbmpdata[chra].cv, &chrpos[prtb], &chrbmpdata[prtb].cv, bfalse, NULL );
    };
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void do_bumping( float dUpdate )
{
  // ZZ> This function sets handles characters hitting other characters or particles
  CHR_REF chra, chrb;
  Uint32 fanblock;
  int cnt, tnc, chrinblock, prtinblock;
  vect3 apos, bpos;

  float loc_platkeep, loc_platascend, loc_platstick;

  loc_platascend = pow( PLATASCEND, 1.0 / dUpdate );
  loc_platkeep   = 1.0 - loc_platascend;
  loc_platstick  = pow( platstick, 1.0 / dUpdate );

  create_bumplists();

  cv_list_clear();

  // Check collisions with other characters and bump particles
  // Only check each pair once
  for ( fanblock = 0; fanblock < numfanblock; fanblock++ )
  {
    chrinblock = bumplistchrnum[fanblock];
    prtinblock = bumplistprtnum[fanblock];

    //// remove bad platforms
    //for ( cnt = 0, chra = bumplistchr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  // detach character from invalid platforms
    //  chrb  = chr_get_onwhichplatform( chra );
    //  if ( VALID_CHR( chrb ) )
    //  {
    //    if ( !chr_is_inside( chra, 0.0f, chrb, btrue ) ||
    //         chrbmpdata[chrb].cv.z_min  > chrbmpdata[chrb].cv.z_max - PLATTOLERANCE )
    //    {
    //      remove_from_platform( chra );
    //    }
    //  }
    //};

    //// do attachments
    //for ( cnt = 0, chra = bumplistchr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  // Do platforms (no interaction with held or mounted items)
    //  if ( chr_attached( chra ) ) continue;

    //  for ( chrb = chr_get_bumpnext( chra ), tnc = cnt + 1;
    //        tnc < chrinblock && VALID_CHR( chrb );
    //        tnc++, chrb = chr_get_bumpnext( chrb ) )
    //  {
    //    // do not put something on a platform that is being carried by someone
    //    if ( chr_attached( chrb ) ) continue;

    //    // do not consider anything that is already a item/platform combo
    //    if ( chronwhichplatform[chra] == chrb || chronwhichplatform[chrb] == chra ) continue;

    //    if ( chr_is_inside( chra, 0.0f, chrb, btrue) )
    //    {
    //      // check for compatibility
    //      if ( chrbmpdata[chrb].calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( chrpos[chra].z > MAX( chrbmpdata[chrb].cv.z_min, chrbmpdata[chrb].cv.z_max - PLATTOLERANCE ) && chrlevel[chra] < chrbmpdata[chrb].cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chra, chrb );
    //        }
    //      }
    //    }
    //    
    //    if( chr_is_inside( chrb, 0.0f, chra, btrue) )
    //    {
    //      if ( chrbmpdata[chra].calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( chrpos[chrb].z > MAX( chrbmpdata[chra].cv.z_min, chrbmpdata[chra].cv.z_max - PLATTOLERANCE ) && chrlevel[chrb] < chrbmpdata[chra].cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chrb, chra );
    //        }
    //      }
    //    }

    //  }
    //}

    //// Do mounting
    //for ( cnt = 0, chra = bumplistchr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  if ( chr_attached( chra ) ) continue;

    //  for ( chrb = chr_get_bumpnext( chra ), tnc = cnt + 1;
    //        tnc < chrinblock && VALID_CHR( chrb );
    //        tnc++, chrb = chr_get_bumpnext( chrb ) )
    //  {

    //    // do not mount something that is being carried by someone
    //    if ( chr_attached( chrb ) ) continue;

    //    if ( chr_is_inside( chra, 0.0f, chrb, btrue)   )
    //    {

    //      // Now see if either is on top the other like a platform
    //      if ( chrpos[chra].z > chrbmpdata[chrb].cv.z_max - PLATTOLERANCE && chrpos[chra].z < chrbmpdata[chrb].cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is A falling on B?
    //        if ( chrvel[chra].z < chrvel[chrb].z )
    //        {
    //          if ( chrflyheight[chra] == 0 && chralive[chra] && madactionvalid[chrmodel[chra]][ACTION_MI] && !chrisitem[chra] )
    //          {
    //            if ( chralive[chrb] && chrismount[chrb] && !chr_using_slot( chrb, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chra );
    //              if ( !attach_character_to_mount( chra, chrb, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                chralert[chra] |= ALERT_BUMPED;
    //                chralert[chrb] |= ALERT_BUMPED;
    //                chraibumplast[chra] = chrb;
    //                chraibumplast[chrb] = chra;
    //              };
    //            }
    //          }
    //        }
    //      }

    //    }

    //    if( chr_is_inside( chrb, 0.0f, chra, btrue)   )
    //    {
    //      if ( chrpos[chrb].z > chrbmpdata[chra].cv.z_max - PLATTOLERANCE && chrpos[chrb].z < chrbmpdata[chra].cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is B falling on A?
    //        if ( chrvel[chrb].z < chrvel[chra].z )
    //        {
    //          if ( chrflyheight[chrb] == 0 && chralive[chrb] && madactionvalid[chrmodel[chrb]][ACTION_MI] && !chrisitem[chrb] )
    //          {
    //            if ( chralive[chra] && chrismount[chra] && !chr_using_slot( chra, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chrb );
    //              if ( !attach_character_to_mount( chrb, chra, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                chralert[chra] |= ALERT_BUMPED;
    //                chralert[chrb] |= ALERT_BUMPED;
    //                chraibumplast[chra] = chrb;
    //                chraibumplast[chrb] = chra;
    //              };
    //            };
    //          }
    //        }
    //      }
    //    }
    //  }
    //}

    // do collisions
    for ( cnt = 0, chra = bumplistchr[fanblock];
          cnt < chrinblock && VALID_CHR( chra );
          cnt++, chra = chr_get_bumpnext( chra ) )
    {
      float lerpa;
      lerpa = (chrpos[chra].z - chrlevel[chra]) / PLATTOLERANCE;
      lerpa = CLIP(lerpa, 0, 1);

      apos = chrpos[chra];

      // don't do object-object collisions if they won't feel each other
      if ( chrbumpstrength[chra] == 0.0f ) continue;

      // Do collisions (but not with attached items/characers)
      for ( chrb = chr_get_bumpnext( chra ), tnc = cnt + 1;
            tnc < chrinblock && VALID_CHR( chrb );
            tnc++, chrb = chr_get_bumpnext( chrb ) )
      {
        CVolume cv;
        float lerpb;

        float bumpstrength = chrbumpstrength[chra] * chrbumpstrength[chrb];

        // don't do object-object collisions if they won't feel eachother
        if ( bumpstrength == 0.0f ) continue;

        // do not collide with something you are already holding
        if ( chrb == chrattachedto[chra] || chra == chrattachedto[chrb] ) continue;

        // do not collide with a your platform
        if ( chrb == chronwhichplatform[chra] || chra == chronwhichplatform[chrb] ) continue;

        bpos = chrpos[chrb];

        lerpb = (chrpos[chrb].z - chrlevel[chrb]) / PLATTOLERANCE;
        lerpb = CLIP(lerpb, 0, 1);

        if ( chr_do_collision( chra, chrb, bfalse, &cv) )
        {
          vect3 depth, ovlap, nrm, diffa, diffb;
          float ftmp, dotprod, pressure;
          float cr, m0, m1, psum, msum, udif, u0, u1, ln_cr;
          bool_t bfound;

          depth.x = (cv.x_max - cv.x_min);
          ovlap.x = depth.x / MIN(chrbmpdata[chra].cv.x_max - chrbmpdata[chra].cv.x_min, chrbmpdata[chrb].cv.x_max - chrbmpdata[chrb].cv.x_min);
          ovlap.x = CLIP(ovlap.x,-1,1);
          nrm.x = 1.0f / ovlap.x;

          depth.y = (cv.y_max - cv.y_min);
          ovlap.y = depth.y / MIN(chrbmpdata[chra].cv.y_max - chrbmpdata[chra].cv.y_min, chrbmpdata[chrb].cv.y_max - chrbmpdata[chrb].cv.y_min);
          ovlap.y = CLIP(ovlap.y,-1,1);
          nrm.y = 1.0f / ovlap.y;

          depth.z = (cv.z_max - cv.z_min);
          ovlap.z = depth.z / MIN(chrbmpdata[chra].cv.z_max - chrbmpdata[chra].cv.z_min, chrbmpdata[chrb].cv.z_max - chrbmpdata[chrb].cv.z_min);
          ovlap.z = CLIP(ovlap.z,-1,1);
          nrm.z = 1.0f / ovlap.z;

          nrm = Normalize(nrm);

          pressure = (depth.x / 30.0f) * (depth.y / 30.0f) * (depth.z / 30.0f);

          if(ovlap.x != 1.0)
          {
            diffa.x = chrbmpdata[chra].mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
            diffb.x = chrbmpdata[chrb].mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
          }
          else
          {
            diffa.x = chrbmpdata[chra].mids_lo.x - chrbmpdata[chrb].mids_lo.x;
            diffb.x =-diffa.x;
          }

          if(ovlap.y != 1.0)
          {
            diffa.y = chrbmpdata[chra].mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
            diffb.y = chrbmpdata[chrb].mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
          }
          else
          {
            diffa.y = chrbmpdata[chra].mids_lo.y - chrbmpdata[chrb].mids_lo.y;
            diffb.y =-diffa.y;
          }

          if(ovlap.y != 1.0)
          {
            diffa.z = chrbmpdata[chra].mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffa.z += (chrbmpdata[chra].mids_hi.z - chrbmpdata[chra].mids_lo.z) * lerpa;

            diffb.z = chrbmpdata[chrb].mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffb.z += (chrbmpdata[chrb].mids_hi.z - chrbmpdata[chrb].mids_lo.z) * lerpb;
          }
          else
          {
            diffa.z  = chrbmpdata[chra].mids_lo.z - chrbmpdata[chrb].mids_lo.z;
            diffa.z += (chrbmpdata[chra].mids_hi.z - chrbmpdata[chra].mids_lo.z) * lerpa;
            diffa.z -= (chrbmpdata[chrb].mids_hi.z - chrbmpdata[chrb].mids_lo.z) * lerpb;

            diffb.z =-diffa.z;
          }

          diffa = Normalize(diffa);
          diffb = Normalize(diffb);

          if(diffa.x < 0) nrm.x *= -1.0f;
          if(diffa.y < 0) nrm.y *= -1.0f;
          if(diffa.z < 0) nrm.z *= -1.0f;

          dotprod = DotProduct(diffa, nrm);
          if(dotprod != 0.0f)
          {
            diffa.x = pressure * dotprod * nrm.x;
            diffa.y = pressure * dotprod * nrm.y;
            diffa.z = pressure * dotprod * nrm.z;
          }
          else
          {
            diffa.x = pressure * nrm.x;
            diffa.y = pressure * nrm.y;
            diffa.z = pressure * nrm.z;
          };

          dotprod = DotProduct(diffb, nrm);
          if(dotprod != 0.0f)
          {
            diffb.x = pressure * dotprod * nrm.x;
            diffb.y = pressure * dotprod * nrm.y;
            diffb.z = pressure * dotprod * nrm.z;
          }
          else
          {
            diffb.x = - pressure * nrm.x;
            diffb.y = - pressure * nrm.y;
            diffb.z = - pressure * nrm.z;
          };

          // calculate a coefficient of restitution
          //ftmp = nrm.x * nrm.x + nrm.y * nrm.y;
          //cr = chrbumpdampen[chrb] * chrbumpdampen[chra] * bumpstrength * ovlap.z * ( nrm.x * nrm.x * ovlap.x + nrm.y * nrm.y * ovlap.y ) / ftmp;

          // determine a usable mass
          m0 = -1;
          m1 = -1;
          if ( chrweight[chra] < 0 && chrweight[chrb] < 0 )
          {
            m0 = m1 = 110.0f;
          }
          else if (chrweight[chra] == 0 && chrweight[chrb] == 0)
          {
            m0 = m1 = 1.0f;
          }
          else
          {
            m0 = (chrweight[chra] == 0.0f) ? 1.0 : chrweight[chra];
            m1 = (chrweight[chrb] == 0.0f) ? 1.0 : chrweight[chrb];
          }

          bfound = btrue;
          cr = chrbumpdampen[chrb] * chrbumpdampen[chra];
          //ln_cr = log(cr);

          if( m0 > 0.0f && bumpstrength > 0.0f )
          {
            float k = 250.0f / m0;
            float gamma = 0.5f * (1.0f - cr) * (1.0f - cr);
            
            //if(cr != 0.0f)
            //{
            //  gamma = 2.0f * ABS(ln_cr) * sqrt( k / (ln_cr*ln_cr + PI*PI) );
            //}
            //else
            //{
            //  gamma = 2.0f * sqrt(k);
            //}

            chraccum_acc[chra].x += (diffa.x * k  - chrvel[chra].x * gamma) * bumpstrength;
            chraccum_acc[chra].y += (diffa.y * k  - chrvel[chra].y * gamma) * bumpstrength;
            chraccum_acc[chra].z += (diffa.z * k  - chrvel[chra].z * gamma) * bumpstrength;
          }

          if( m1 > 0.0f && bumpstrength > 0.0f )
          {
            float k = 250.0f / m1;
            float gamma = 0.5f * (1.0f - cr) * (1.0f - cr);

            //if(cr != 0.0f)
            //{
            //  gamma = 2.0f * ABS(ln_cr) * sqrt( k / (ln_cr*ln_cr + PI*PI) );
            //}
            //else
            //{
            //  gamma = 2.0f * sqrt(k);
            //}

            chraccum_acc[chrb].x += (diffb.x * k  - chrvel[chrb].x * gamma) * bumpstrength;
            chraccum_acc[chrb].y += (diffb.y * k  - chrvel[chrb].y * gamma) * bumpstrength;
            chraccum_acc[chrb].z += (diffb.z * k  - chrvel[chrb].z * gamma) * bumpstrength;
          }


          //bfound = bfalse;
          //if (( chrbmpdata[chra].mids_lo.x - chrbmpdata[chrb].mids_lo.x ) * ( chrvel[chra].x - chrvel[chrb].x ) < 0.0f )
          //{
          //  u0 = chrvel[chra].x;
          //  u1 = chrvel[chrb].x;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  chrvel[chra].x = ( psum - m1 * udif * cr ) / msum;
          //  chrvel[chrb].x = ( psum + m0 * udif * cr ) / msum;

          //  //chrbmpdata[chra].mids_lo.x -= chrvel[chra].x*dUpdate;
          //  //chrbmpdata[chrb].mids_lo.x -= chrvel[chrb].x*dUpdate;

          //  bfound = btrue;
          //}



          //if (( chrbmpdata[chra].mids_lo.y - chrbmpdata[chrb].mids_lo.y ) * ( chrvel[chra].y - chrvel[chrb].y ) < 0.0f )
          //{
          //  u0 = chrvel[chra].y;
          //  u1 = chrvel[chrb].y;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  chrvel[chra].y = ( psum - m1 * udif * cr ) / msum;
          //  chrvel[chrb].y = ( psum + m0 * udif * cr ) / msum;

          //  //chrbmpdata[chra].mids_lo.y -= chrvel[chra].y*dUpdate;
          //  //chrbmpdata[chrb].mids_lo.y -= chrvel[chrb].y*dUpdate;

          //  bfound = btrue;
          //}

          //if ( ovlap.x > 0 && ovlap.z > 0 )
          //{
          //  chrbmpdata[chra].mids_lo.x += m1 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  chrbmpdata[chrb].mids_lo.x -= m0 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  bfound = btrue;
          //}

          //if ( ovlap.y > 0 && ovlap.z > 0 )
          //{
          //  chrbmpdata[chra].mids_lo.y += m1 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  chrbmpdata[chrb].mids_lo.y -= m0 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  bfound = btrue;
          //}

          if ( bfound )
          {
            //apos = chrpos[chra];
            chralert[chra] |= ALERT_BUMPED;
            chralert[chrb] |= ALERT_BUMPED;
            chraibumplast[chra] = chrb;
            chraibumplast[chrb] = chra;
          };
        }
      }
    };

    // Now check collisions with every bump particle in same area
    //for ( cnt = 0, chra = bumplistchr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  IDSZ chridvulnerability, eveidremove;
    //  float chrbump = 1.0f;

    //  apos = chrpos[chra];
    //  chridvulnerability = capidsz[chrmodel[chra]][IDSZ_VULNERABILITY];
    //  chrbump = chrbumpstrength[chra];

    //  // Check for object-particle interaction
    //  for ( tnc = 0, prtb = bumplistprt[fanblock];
    //        tnc < prtinblock && MAXPRT != prtb;
    //        tnc++ , prtb = prt_get_bumpnext( prtb ) )
    //  {
    //    float bumpstrength, prtbump;
    //    bool_t chr_is_vulnerable;

    //    CHR_REF prt_owner = prt_get_owner( prtb );
    //    CHR_REF prt_attached = prt_get_attachedtochr( prtb );

    //    pip = prtpip[prtb];
    //    bpos = prtpos[prtb];

    //    chr_is_vulnerable = !chrinvictus[chra] && ( IDSZ_NONE != chridvulnerability ) && CAP_INHERIT_IDSZ( prtmodel[prtb], chridvulnerability );

    //    prtbump = prtbumpstrength[prtb];
    //    bumpstrength = chr_is_vulnerable ? 1.0f : chrbump * prtbump;

    //    if ( 0.0f == bumpstrength ) continue;

    //    // First check absolute value diamond
    //    diff.x = ABS( apos.x - bpos.x );
    //    diff.y = ABS( apos.y - bpos.y );
    //    dist = diff.x + diff.y;
    //    if ( prt_do_collision( chra, prtb, bfalse ) )
    //    {
    //      vect3 pvel;

    //      if ( MAXCHR != prt_get_attachedtochr( prtb ) )
    //      {
    //        pvel.x = ( prtpos[prtb].x - prtpos_old[prtb].x ) / dUpdate;
    //        pvel.y = ( prtpos[prtb].y - prtpos_old[prtb].y ) / dUpdate;
    //        pvel.z = ( prtpos[prtb].z - prtpos_old[prtb].z ) / dUpdate;
    //      }
    //      else
    //      {
    //        pvel = prtvel[prtb];
    //      }

    //      if ( bpos.z > chrbmpdata[chra].cv.z_max + pvel.z && pvel.z < 0 && chrbmpdata[chra].calc_is_platform && !VALID_CHR( prt_attached ) )
    //      {
    //        // Particle is falling on A
    //        prtaccum_pos[prtb].z += chrbmpdata[chra].cv.z_max - prtpos[prtb].z;

    //        prtaccum_vel[prtb].z = - (1.0f - pipdampen[pip] * chrbumpdampen[chra]) * prtvel[prtb].z;

    //        prtaccum_acc[prtb].x += ( pvel.x - chrvel[chra].x ) * ( 1.0 - loc_platstick ) + chrvel[chra].x;
    //        prtaccum_acc[prtb].y += ( pvel.y - chrvel[chra].y ) * ( 1.0 - loc_platstick ) + chrvel[chra].y;
    //      }

    //      // Check reaffirmation of particles
    //      if ( prt_attached != chra )
    //      {
    //        if ( chrreloadtime[chra] == 0 )
    //        {
    //          if ( chrreaffirmdamagetype[chra] == prtdamagetype[prtb] && chrdamagetime[chra] == 0 )
    //          {
    //            reaffirm_attached_particles( chra );
    //          }
    //        }
    //      }

    //      // Check for missile treatment
    //      if (( chrdamagemodifier_fp8[chra][prtdamagetype[prtb]]&DAMAGE_SHIFT ) != DAMAGE_SHIFT ||
    //            MIS_NORMAL == chrmissiletreatment[chra]  ||
    //            VALID_CHR( prt_attached ) ||
    //            ( prt_owner == chra && !pipfriendlyfire[pip] ) ||
    //            ( chrmana_fp8[chrmissilehandler[chra]] < ( chrmissilecost[chra] << 4 ) && !chrcanchannel[chrmissilehandler[chra]] ) )
    //      {
    //        if (( teamhatesteam[prtteam[prtb]][chrteam[chra]] || ( pipfriendlyfire[pip] && (( chra != prt_owner && chra != chrattachedto[prt_owner] ) || piponlydamagefriendly[pip] ) ) ) && !chrinvictus[chra] )
    //        {
    //          spawn_bump_particles( chra, prtb );  // Catch on fire

    //          if (( prtdamage[prtb].ibase > 0 ) && ( prtdamage[prtb].irand > 0 ) )
    //          {
    //            if ( chrdamagetime[chra] == 0 && prt_attached != chra && HAS_NO_BITS( pipdamfx[pip], DAMFX_ARRO ) )
    //            {

    //              // Normal prtb damage
    //              if ( pipallowpush[pip] )
    //              {
    //                float ftmp = 0.2;

    //                if ( chrweight[chra] < 0 )
    //                {
    //                  ftmp = 0;
    //                }
    //                else if ( chrweight[chra] != 0 )
    //                {
    //                  ftmp *= ( 1.0f + chrbumpdampen[chra] ) * prtweight[prtb] / chrweight[chra];
    //                }

    //                chraccum_vel[chra].x += pvel.x * ftmp;
    //                chraccum_vel[chra].y += pvel.y * ftmp;
    //                chraccum_vel[chra].z += pvel.z * ftmp;

    //                prtaccum_vel[prtb].x += -chrbumpdampen[chra] * pvel.x - prtvel[prtb].x;
    //                prtaccum_vel[prtb].y += -chrbumpdampen[chra] * pvel.y - prtvel[prtb].y;
    //                prtaccum_vel[prtb].z += -chrbumpdampen[chra] * pvel.z - prtvel[prtb].z;
    //              }

    //              direction = RAD_TO_TURN( atan2( pvel.y, pvel.x ) );
    //              direction = 32768 + chrturn_lr[chra] - direction;

    //              // Check all enchants to see if they are removed
    //              enchant = chrfirstenchant[chra];
    //              while ( enchant != MAXENCHANT )
    //              {
    //                eveidremove = everemovedbyidsz[enceve[enchant]];
    //                temp = encnextenchant[enchant];
    //                if ( eveidremove != IDSZ_NONE && CAP_INHERIT_IDSZ( prtmodel[prtb], eveidremove ) )
    //                {
    //                  remove_enchant( enchant );
    //                }
    //                enchant = temp;
    //              }

    //              //Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
    //              //+1 (256) bonus for every 4 points of intelligence and/or wisdom above 14. Below 14 gives -1 instead!
    //			        //Enemy IDAM spells damage is reduced by 1% per defender's wisdom, opposite for WDAM spells
    //			        if ( pipintdamagebonus[pip] )
    //					    {
    //						    prtdamage[prtb].ibase += (( chrintelligence_fp8[prt_owner] - 3584 ) * 0.25 );		//First increase damage by the attacker
    //						    if(!chrdamagemodifier_fp8[chra][prtdamagetype[prtb]]&DAMAGE_INVERT || !chrdamagemodifier_fp8[chra][prtdamagetype[prtb]]&DAMAGE_CHARGE) 
    //						    prtdamage[prtb].ibase -= (prtdamage[prtb].ibase * ( chrwisdom_fp8[chra] > 8 ));		//Then reduce it by defender
    //					    }
    //					    if ( pipwisdamagebonus[pip] )	//Same with divine spells
    //					    {
    //						    prtdamage[prtb].ibase += (( chrwisdom_fp8[prt_owner] - 3584 ) * 0.25 );
    //						    if(!chrdamagemodifier_fp8[chra][prtdamagetype[prtb]]&DAMAGE_INVERT || !chrdamagemodifier_fp8[chra][prtdamagetype[prtb]]&DAMAGE_CHARGE) 
    //						    prtdamage[prtb].ibase -= (prtdamage[prtb].ibase * ( chrintelligence_fp8[chra] > 8 ));
    //					    }

    //              //Force Pancake animation?
    //              if ( pipcausepancake[pip] )
    //              {
    //                vect3 panc;
    //                Uint16 rotate_cos, rotate_sin;
    //                float cv, sv;

    //                // just a guess
    //                panc.x = 0.25 * ABS( pvel.x ) * 2.0f / ( float )( 1 + chrbmpdata[chra].cv.x_max - chrbmpdata[chra].cv.x_min  );
    //                panc.y = 0.25 * ABS( pvel.y ) * 2.0f / ( float )( 1 + chrbmpdata[chra].cv.y_max - chrbmpdata[chra].cv.y_min );
    //                panc.z = 0.25 * ABS( pvel.z ) * 2.0f / ( float )( 1 + chrbmpdata[chra].cv.z_max - chrbmpdata[chra].cv.z_min );

    //                rotate_sin = chrturn_lr[chra] >> 2;
    //                rotate_cos = ( rotate_sin + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;

    //                cv = turntosin[rotate_cos];
    //                sv = turntosin[rotate_sin];

    //                chrpancakevel[chra].x = - ( panc.x * cv - panc.y * sv );
    //                chrpancakevel[chra].y = - ( panc.x * sv + panc.y * cv );
    //                chrpancakevel[chra].z = -panc.z;
    //              }

    //              // Damage the character
    //              if ( chr_is_vulnerable )
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = prtdamage[prtb].ibase * 2.0f * bumpstrength;
    //                ptemp.irand = prtdamage[prtb].irand * 2.0f * bumpstrength;
    //                damage_character( chra, direction, &ptemp, prtdamagetype[prtb], prtteam[prtb], prt_owner, pipdamfx[pip] );
    //                chralert[chra] |= ALERT_HITVULNERABLE;
    //                cost_mana( chra, pipmanadrain[pip]*2, prt_owner );  //Do mana drain too
    //              }
    //              else
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = prtdamage[prtb].ibase * bumpstrength;
    //                ptemp.irand = prtdamage[prtb].irand * bumpstrength;

    //                damage_character( chra, direction, &prtdamage[prtb], prtdamagetype[prtb], prtteam[prtb], prt_owner, pipdamfx[pip] );
    //                cost_mana( chra, pipmanadrain[pip], prt_owner );  //Do mana drain too
    //              }

    //              // Do confuse effects
    //              if ( HAS_NO_BITS( madframefx[chrmodel[chra]][chrframe[chra]], MADFX_INVICTUS ) || HAS_SOME_BITS( pipdamfx[pip], DAMFX_BLOC ) )
    //              {

    //                if ( pipgrogtime[pip] != 0 && capcanbegrogged[chrmodel[chra]] )
    //                {
    //                  chrgrogtime[chra] += pipgrogtime[pip] * bumpstrength;
    //                  if ( chrgrogtime[chra] < 0 )
    //                  {
    //                    chrgrogtime[chra] = -1;
    //                    debug_message( 1, "placing infinite grog on %s (%s)", chrname[chra], capclassname[chrmodel[chra]] );
    //                  }
    //                  chralert[chra] |= ALERT_GROGGED;
    //                }

    //                if ( pipdazetime[pip] != 0 && capcanbedazed[chrmodel[chra]] )
    //                {
    //                  chrdazetime[chra] += pipdazetime[pip] * bumpstrength;
    //                  if ( chrdazetime[chra] < 0 )
    //                  {
    //                    chrdazetime[chra] = -1;
    //                    debug_message( 1, "placing infinite daze on %s (%s)", chrname[chra], capclassname[chrmodel[chra]] );
    //                  };
    //                  chralert[chra] |= ALERT_DAZED;
    //                }
    //              }

    //              // Notify the attacker of a scored hit
    //              if ( VALID_CHR( prt_owner ) )
    //              {
    //                chralert[prt_owner] |= ALERT_SCOREDAHIT;
    //                chraihitlast[prt_owner] = chra;
    //              }
    //            }

    //            if (( wldframe&31 ) == 0 && prt_attached == chra )
    //            {
    //              // Attached prtb damage ( Burning )
    //              if ( pipxyvel[pip].ibase == 0 )
    //              {
    //                // Make character limp
    //                chrvel[chra].x = 0;
    //                chrvel[chra].y = 0;
    //              }
    //              damage_character( chra, 32768, &prtdamage[prtb], prtdamagetype[prtb], prtteam[prtb], prt_owner, pipdamfx[pip] );
    //              cost_mana( chra, pipmanadrain[pip], prt_owner );  //Do mana drain too

    //            }
    //          }

    //          if ( pipendbump[pip] )
    //          {
    //            if ( pipbumpmoney[pip] )
    //            {
    //              if ( chrcangrabmoney[chra] && chralive[chra] && chrdamagetime[chra] == 0 && chrmoney[chra] != MAXMONEY )
    //              {
    //                if ( chrismount[chra] )
    //                {
    //                  CHR_REF irider = chr_get_holdingwhich( chra, SLOT_SADDLE );

    //                  // Let mounts collect money for their riders
    //                  if ( VALID_CHR( irider ) )
    //                  {
    //                    chrmoney[irider] += pipbumpmoney[pip];
    //                    if ( chrmoney[irider] > MAXMONEY ) chrmoney[irider] = MAXMONEY;
    //                    if ( chrmoney[irider] <        0 ) chrmoney[irider] = 0;
    //                    prtgopoof[prtb] = btrue;
    //                  }
    //                }
    //                else
    //                {
    //                  // Normal money collection
    //                  chrmoney[chra] += pipbumpmoney[pip];
    //                  if ( chrmoney[chra] > MAXMONEY ) chrmoney[chra] = MAXMONEY;
    //                  if ( chrmoney[chra] < 0 ) chrmoney[chra] = 0;
    //                  prtgopoof[prtb] = btrue;
    //                }
    //              }
    //            }
    //            else
    //            {
    //              // Only hit one character, not several
    //              prtdamage[prtb].ibase *= 1.0f - bumpstrength;
    //              prtdamage[prtb].irand *= 1.0f - bumpstrength;

    //              if ( prtdamage[prtb].ibase == 0 && prtdamage[prtb].irand <= 1 )
    //              {
    //                prtgopoof[prtb] = btrue;
    //              };
    //            }
    //          }
    //        }
    //      }
    //      else if ( prt_owner != chra )
    //      {
    //        cost_mana( chrmissilehandler[chra], ( chrmissilecost[chra] << 4 ), prt_owner );

    //        // Treat the missile
    //        switch ( chrmissiletreatment[chra] )
    //        {
    //          case MIS_DEFLECT:
    //            {
    //              // Use old position to find normal
    //              acc.x = prtpos[prtb].x - pvel.x * dUpdate;
    //              acc.y = prtpos[prtb].y - pvel.y * dUpdate;
    //              acc.x = chrpos[chra].x - acc.x;
    //              acc.y = chrpos[chra].y - acc.y;
    //              // Find size of normal
    //              scale = acc.x * acc.x + acc.y * acc.y;
    //              if ( scale > 0 )
    //              {
    //                // Make the normal a unit normal
    //                scale = sqrt( scale );
    //                nrm.x = acc.x / scale;
    //                nrm.y = acc.y / scale;

    //                // Deflect the incoming ray off the normal
    //                scale = ( pvel.x * nrm.x + pvel.y * nrm.y ) * 2;
    //                acc.x = scale * nrm.x;
    //                acc.y = scale * nrm.y;
    //                prtaccum_vel[prtb].x += -acc.x;
    //                prtaccum_vel[prtb].y += -acc.y;
    //              }
    //            }
    //            break;

    //          case MIS_REFLECT:
    //            {
    //              // Reflect it back in the direction it came
    //              prtaccum_vel[prtb].x += -2.0f * prtvel[prtb].x;
    //              prtaccum_vel[prtb].y += -2.0f * prtvel[prtb].y;
    //            };
    //            break;
    //        };


    //        // Change the owner of the missile
    //        if ( !piphoming[pip] )
    //        {
    //          prtteam[prtb] = chrteam[chra];
    //          prt_owner = chra;
    //        }
    //      }
    //    }
    //  }
    //}

    // do platform physics
    //for ( cnt = 0, chra = bumplistchr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  // detach character from invalid platforms
    //  chrb  = chr_get_onwhichplatform( chra );
    //  if ( !VALID_CHR( chrb ) ) continue;

    //  if ( chrpos[chra].z < chrbmpdata[chrb].cv.z_max + RAISE )
    //  {
    //    chrpos[chra].z = chrbmpdata[chrb].cv.z_max + RAISE;
    //    if ( chrvel[chra].z < chrvel[chrb].z )
    //    {
    //      chrvel[chra].z = - ( chrvel[chra].z - chrvel[chrb].z ) * chrbumpdampen[chra] * chrbumpdampen[chrb] + chrvel[chrb].z;
    //    };
    //  }

    //  chrvel[chra].x = ( chrvel[chra].x - chrvel[chrb].x ) * ( 1.0 - loc_platstick ) + chrvel[chrb].x;
    //  chrvel[chra].y = ( chrvel[chra].y - chrvel[chrb].y ) * ( 1.0 - loc_platstick ) + chrvel[chrb].y;
    //  chrturn_lr[chra] += ( chrturn_lr[chrb] - chrturn_lr_old[chrb] ) * ( 1.0 - loc_platstick );
    //}


  };
}


//--------------------------------------------------------------------------------------------
void stat_return( float dUpdate )
{
  // ZZ> This function brings mana and life back
  int cnt, owner, target, eve;
  static int stat_return_counter = 0;


  // Do reload time
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    chrreloadtime[cnt] -= dUpdate;
    if ( chrreloadtime[cnt] < 0 ) chrreloadtime[cnt] = 0;
    cnt++;
  }



  // Do stats
  if ( statclock == ONESECOND )
  {
    // Reset the clock
    statclock = 0;
    stat_return_counter++;

    // Do all the characters
    for ( cnt = 0; cnt < MAXCHR; cnt++ )
    {
      if ( !VALID_CHR( cnt ) ) continue;

      if ( chralive[cnt] )
      {
        chrmana_fp8[cnt] += chrmanareturn_fp8[cnt] >> MANARETURNSHIFT;
        if ( chrmana_fp8[cnt] < 0 ) chrmana_fp8[cnt] = 0;
        if ( chrmana_fp8[cnt] > chrmanamax_fp8[cnt] ) chrmana_fp8[cnt] = chrmanamax_fp8[cnt];

        chrlife_fp8[cnt] += chrlifereturn[cnt];
        if ( chrlife_fp8[cnt] < 1 ) chrlife_fp8[cnt] = 1;
        if ( chrlife_fp8[cnt] > chrlifemax_fp8[cnt] ) chrlife_fp8[cnt] = chrlifemax_fp8[cnt];
      };

      if ( chrgrogtime[cnt] > 0 )
      {
        chrgrogtime[cnt]--;
        if ( chrgrogtime[cnt] < 0 ) chrgrogtime[cnt] = 0;

        if ( chrgrogtime[cnt] == 0 )
        {
          debug_message( 1, "stat_return() - removing grog on %s (%s)", chrname[cnt], capclassname[chrmodel[cnt]] );
        };
      }

      if ( chrdazetime[cnt] > 0 )
      {
        chrdazetime[cnt]--;
        if ( chrdazetime[cnt] < 0 ) chrdazetime[cnt] = 0;
        if ( chrgrogtime[cnt] == 0 )
        {
          debug_message( 1, "stat_return() - removing daze on %s (%s)", chrname[cnt], capclassname[chrmodel[cnt]] );
        };
      }

    }


    // Run through all the enchants as well
    for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
    {
      bool_t kill_enchant = bfalse;
      if ( !encon[cnt] ) continue;

      if ( enctime[cnt] == 0 )
      {
        kill_enchant = btrue;
      };

      if ( enctime[cnt] > 0 ) enctime[cnt]--;

      owner = encowner[cnt];
      target = enctarget[cnt];
      eve = enceve[cnt];

      // Do drains
      if ( !kill_enchant && chralive[owner] )
      {
        // Change life
        chrlife_fp8[owner] += encownerlife[cnt];
        if ( chrlife_fp8[owner] < 1 )
        {
          chrlife_fp8[owner] = 1;
          kill_character( owner, target );
        }
        if ( chrlife_fp8[owner] > chrlifemax_fp8[owner] )
        {
          chrlife_fp8[owner] = chrlifemax_fp8[owner];
        }
        // Change mana
        if ( !cost_mana( owner, -encownermana[cnt], target ) && eveendifcantpay[eve] )
        {
          kill_enchant = btrue;
        }
      }
      else if ( !evestayifnoowner[eve] )
      {
        kill_enchant = btrue;
      }


      if ( !kill_enchant && encon[cnt] )
      {
        if ( chralive[target] )
        {
          // Change life
          chrlife_fp8[target] += enctargetlife[cnt];
          if ( chrlife_fp8[target] < 1 )
          {
            chrlife_fp8[target] = 1;
            kill_character( target, owner );
          }
          if ( chrlife_fp8[target] > chrlifemax_fp8[target] )
          {
            chrlife_fp8[target] = chrlifemax_fp8[target];
          }

          // Change mana
          if ( !cost_mana( target, -enctargetmana[cnt], owner ) && eveendifcantpay[eve] )
          {
            kill_enchant = btrue;
          }
        }
        else
        {
          kill_enchant = btrue;
        }
      }

      if ( kill_enchant )
      {
        remove_enchant( cnt );
      };
    }
  }
}

//--------------------------------------------------------------------------------------------
void pit_kill( float dUpdate )
{
  // ZZ> This function kills any character in a deep pit...
  int cnt;

  if ( pitskill )
  {
    if ( pitclock > 19 )
    {
      pitclock = 0;


      // Kill any particles that fell in a pit, if they die in water...

      for ( cnt = 0; cnt < MAXPRT; cnt++ )
      {
        if ( !VALID_PRT( cnt ) ) continue;

        if ( prtpos[cnt].z < PITDEPTH && pipendwater[prtpip[cnt]] )
        {
          prtgopoof[cnt] = btrue;
        }
      }



      // Kill any characters that fell in a pit...
      cnt = 0;
      while ( cnt < MAXCHR )
      {
        if ( chron[cnt] && chralive[cnt] && !chr_in_pack( cnt ) )
        {
          if ( !chrinvictus[cnt] && chrpos[cnt].z < PITDEPTH && !chr_attached( cnt ) )
          {
            // Got one!
            kill_character( cnt, MAXCHR );
            chrvel[cnt].x = 0;
            chrvel[cnt].y = 0;
          }
        }
        cnt++;
      }
    }
    else
    {
      pitclock += dUpdate;
    }
  }
}

//--------------------------------------------------------------------------------------------
void reset_players()
{
  // ZZ> This function clears the player list data
  int cnt;

  // Reset the local data stuff
  localseekurse = bfalse;
  localseeinvisible = bfalse;
  somelocalpladead = bfalse;
  alllocalpladead = bfalse;

  // Reset the initial player data and latches
  cnt = 0;
  while ( cnt < MAXPLAYER )
  {
    plavalid[cnt] = bfalse;
    plachr[cnt] = 0;
    plalatchx[cnt] = 0;
    plalatchy[cnt] = 0;
    plalatchbutton[cnt] = 0;
    pladevice[cnt] = INBITS_NONE;
    cnt++;
  }
  numpla = 0;

  cl_reset( &AClientState );
  sv_reset( &AServerState );
}

//--------------------------------------------------------------------------------------------
void resize_characters( float dUpdate )
{
  // ZZ> This function makes the characters get bigger or smaller, depending
  //     on their sizegoto and sizegototime
  CHR_REF ichr;
  bool_t willgetcaught;
  float newsize, fkeep;

  fkeep = pow( 0.9995, dUpdate );

  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) || chrsizegototime[ichr] <= 0 ) continue;

    // Make sure it won't get caught in a wall
    willgetcaught = bfalse;
    if ( chrsizegoto[ichr] > chrfat[ichr] )
    {
      float x_min_save, x_max_save;
      float y_min_save, y_max_save;

      x_min_save = chrbmpdata[ichr].cv.x_min;
      x_max_save = chrbmpdata[ichr].cv.x_max;

      y_min_save = chrbmpdata[ichr].cv.y_min;
      y_max_save = chrbmpdata[ichr].cv.y_max;

      chrbmpdata[ichr].cv.x_min -= 5;
      chrbmpdata[ichr].cv.y_min -= 5;

      chrbmpdata[ichr].cv.x_max += 5;
      chrbmpdata[ichr].cv.y_max += 5;

      if ( 0 != __chrhitawall( ichr, NULL ) )
      {
        willgetcaught = btrue;
      }

      chrbmpdata[ichr].cv.x_min = x_min_save;
      chrbmpdata[ichr].cv.x_max = x_max_save;

      chrbmpdata[ichr].cv.y_min = y_min_save;
      chrbmpdata[ichr].cv.y_max = y_max_save;
    }


    // If it is getting caught, simply halt growth until later
    if ( willgetcaught ) continue;

    // Figure out how big it is
    chrsizegototime[ichr] -= dUpdate;
    if ( chrsizegototime[ichr] < 0 )
    {
      chrsizegototime[ichr] = 0;
    }

    if ( chrsizegototime[ichr] > 0 )
    {
      newsize = chrfat[ichr] * fkeep + chrsizegoto[ichr] * ( 1.0f - fkeep );
    }
    else if ( chrsizegototime[ichr] <= 0 )
    {
      newsize = chrfat[ichr];
    }

    // Make it that big...
    chrfat[ichr]             = newsize;
    chrbmpdata[ichr].shadow  = chrbmpdata_save[ichr].shadow * newsize;
    chrbmpdata[ichr].size    = chrbmpdata_save[ichr].size * newsize;
    chrbmpdata[ichr].sizebig = chrbmpdata_save[ichr].sizebig * newsize;
    chrbmpdata[ichr].height  = chrbmpdata_save[ichr].height * newsize;
    chrweight[ichr]          = capweight[chrmodel[ichr]] * newsize * newsize * newsize;  // preserve density

    // Now come up with the magic number
    chrscale[ichr] = newsize;

    // calculate the bumpers
    make_one_character_matrix( ichr );
  }
}

//--------------------------------------------------------------------------------------------
void export_one_character_name( char *szSaveName, CHR_REF character )
{
  // ZZ> This function makes the naming.txt file for the character
  FILE* filewrite;
  int profile;

  // Can it export?
  profile = chrmodel[character];
  filewrite = fs_fileOpen( PRI_FAIL, "export_one_character_name()", szSaveName, "w" );
  if ( filewrite )
  {
    convert_spaces( chrname[character], sizeof( chrname[character] ), chrname[character] );
    fprintf( filewrite, ":%s\n", chrname[character] );
    fprintf( filewrite, ":STOP\n\n" );
    fs_fileClose( filewrite );
  }
  else log_error( "Error writing file (%s)\n", szSaveName );
}

//--------------------------------------------------------------------------------------------
void export_one_character_profile( char *szSaveName, CHR_REF character )
{
  // ZZ> This function creates a data.txt file for the given character.
  //     it is assumed that all enchantments have been done away with
  FILE* filewrite;
  int profile;
  int damagetype, skin;
  char types[10] = "SCPHEFIZ";
  char codes[4];

  disenchant_character( character );

  // General stuff
  profile = chrmodel[character];


  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( filewrite )
  {
    // Real general data
    fprintf( filewrite, "Slot number    : -1\n" );   // -1 signals a flexible load thing
    funderf( filewrite, "Class name     : ", capclassname[profile] );
    ftruthf( filewrite, "Uniform light  : ", capuniformlit[profile] );
    fprintf( filewrite, "Maximum ammo   : %d\n", capammomax[profile] );
    fprintf( filewrite, "Current ammo   : %d\n", capammo[character] );
    fgendef( filewrite, "Gender         : ", capgender[character] );
    fprintf( filewrite, "\n" );



    // Object stats
    fprintf( filewrite, "Life color     : %d\n", caplifecolor[character] );
    fprintf( filewrite, "Mana color     : %d\n", capmanacolor[character] );
    fprintf( filewrite, "Life           : %4.2f\n", FP8_TO_FLOAT( chrlifemax_fp8[character] ) );
    fpairof( filewrite, "Life up        : ", &caplifeperlevel_fp8[profile] );
    fprintf( filewrite, "Mana           : %4.2f\n", FP8_TO_FLOAT( chrmanamax_fp8[character] ) );
    fpairof( filewrite, "Mana up        : ", &capmanaperlevel_fp8[profile] );
    fprintf( filewrite, "Mana return    : %4.2f\n", FP8_TO_FLOAT( chrmanareturn_fp8[character] ) );
    fpairof( filewrite, "Mana return up : ", &capmanareturnperlevel_fp8[profile] );
    fprintf( filewrite, "Mana flow      : %4.2f\n", FP8_TO_FLOAT( chrmanaflow_fp8[character] ) );
    fpairof( filewrite, "Mana flow up   : ", &capmanaflowperlevel_fp8[profile] );
    fprintf( filewrite, "STR            : %4.2f\n", FP8_TO_FLOAT( chrstrength_fp8[character] ) );
    fpairof( filewrite, "STR up         : ", &capstrengthperlevel_fp8[profile] );
    fprintf( filewrite, "WIS            : %4.2f\n", FP8_TO_FLOAT( chrwisdom_fp8[character] ) );
    fpairof( filewrite, "WIS up         : ", &capwisdomperlevel_fp8[profile] );
    fprintf( filewrite, "INT            : %4.2f\n", FP8_TO_FLOAT( chrintelligence_fp8[character] ) );
    fpairof( filewrite, "INT up         : ", &capintelligenceperlevel_fp8[profile] );
    fprintf( filewrite, "DEX            : %4.2f\n", FP8_TO_FLOAT( chrdexterity_fp8[character] ) );
    fpairof( filewrite, "DEX up         : ", &capdexterityperlevel_fp8[profile] );
    fprintf( filewrite, "\n" );



    // More physical attributes
    fprintf( filewrite, "Size           : %4.2f\n", chrsizegoto[character] );
    fprintf( filewrite, "Size up        : %4.2f\n", capsizeperlevel[profile] );
    fprintf( filewrite, "Shadow size    : %d\n", capshadowsize[profile] );
    fprintf( filewrite, "Bump size      : %d\n", capbumpsize[profile] );
    fprintf( filewrite, "Bump height    : %d\n", capbumpheight[profile] );
    fprintf( filewrite, "Bump dampen    : %4.2f\n", capbumpdampen[profile] );
    fprintf( filewrite, "Weight         : %d\n", capweight[profile] < 0.0f ? 0xFF : ( Uint8 ) capweight[profile] );
    fprintf( filewrite, "Jump power     : %4.2f\n", capjump[profile] );
    fprintf( filewrite, "Jump number    : %d\n", capjumpnumber[profile] );
    fprintf( filewrite, "Sneak speed    : %d\n", capsneakspd[profile] );
    fprintf( filewrite, "Walk speed     : %d\n", capwalkspd[profile] );
    fprintf( filewrite, "Run speed      : %d\n", caprunspd[profile] );
    fprintf( filewrite, "Fly to height  : %d\n", capflyheight[profile] );
    fprintf( filewrite, "Flashing AND   : %d\n", capflashand[profile] );
    fprintf( filewrite, "Alpha blending : %d\n", capalpha_fp8[profile] );
    fprintf( filewrite, "Light blending : %d\n", caplight_fp8[profile] );
    ftruthf( filewrite, "Transfer blend : ", captransferblend[profile] );
    fprintf( filewrite, "Sheen          : %d\n", capsheen_fp8[profile] );
    ftruthf( filewrite, "Phong mapping  : ", capenviro[profile] );
    fprintf( filewrite, "Texture X add  : %4.2f\n", capuoffvel[profile] / (float)UINT16_SIZE );
    fprintf( filewrite, "Texture Y add  : %4.2f\n", capvoffvel[profile] / (float)UINT16_SIZE );
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
    fprintf( filewrite, "Base defense   : " );
    for ( skin = 0; skin < MAXSKIN; skin++ ) { fprintf( filewrite, "%3d ", 255 - capdefense_fp8[profile][skin] ); }
    fprintf( filewrite, "\n" );

    for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
    {
      fprintf( filewrite, "%c damage shift :", types[damagetype] );
      for ( skin = 0; skin < MAXSKIN; skin++ ) { fprintf( filewrite, "%3d ", capdamagemodifier_fp8[profile][damagetype][skin]&DAMAGE_SHIFT ); };
      fprintf( filewrite, "\n" );
    }

    for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
    {
      fprintf( filewrite, "%c damage code  : ", types[damagetype] );
      for ( skin = 0; skin < MAXSKIN; skin++ )
      {
        codes[skin] = 'F';
        if ( capdamagemodifier_fp8[profile][damagetype][skin]&DAMAGE_CHARGE ) codes[skin] = 'C';
        if ( capdamagemodifier_fp8[profile][damagetype][skin]&DAMAGE_INVERT ) codes[skin] = 'T';
        if ( capdamagemodifier_fp8[profile][damagetype][skin]&DAMAGE_MANA )   codes[skin] = 'M';
        fprintf( filewrite, "%3c ", codes[skin] );
      }
      fprintf( filewrite, "\n" );
    }

    fprintf( filewrite, "Acceleration   : " );
    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      fprintf( filewrite, "%3.0f ", capmaxaccel[profile][skin]*80 );
    }
    fprintf( filewrite, "\n" );



    // Experience and level data
    fprintf( filewrite, "EXP for 2nd    : %d\n", capexperienceforlevel[profile][1] );
    fprintf( filewrite, "EXP for 3rd    : %d\n", capexperienceforlevel[profile][2] );
    fprintf( filewrite, "EXP for 4th    : %d\n", capexperienceforlevel[profile][3] );
    fprintf( filewrite, "EXP for 5th    : %d\n", capexperienceforlevel[profile][4] );
    fprintf( filewrite, "EXP for 6th    : %d\n", capexperienceforlevel[profile][5] );
    fprintf( filewrite, "Starting EXP   : %d\n", capexperience[character] );
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
    fprintf( filewrite, "IDSZ Parent    : [%s]\n", undo_idsz( capidsz[profile][0] ) );
    fprintf( filewrite, "IDSZ Type      : [%s]\n", undo_idsz( capidsz[profile][1] ) );
    fprintf( filewrite, "IDSZ Skill     : [%s]\n", undo_idsz( capidsz[profile][2] ) );
    fprintf( filewrite, "IDSZ Special   : [%s]\n", undo_idsz( capidsz[profile][3] ) );
    fprintf( filewrite, "IDSZ Hate      : [%s]\n", undo_idsz( capidsz[profile][4] ) );
    fprintf( filewrite, "IDSZ Vulnie    : [%s]\n", undo_idsz( capidsz[profile][5] ) );
    fprintf( filewrite, "\n" );



    // Item and damage flags
    ftruthf( filewrite, "Is an item     : ", capisitem[profile] );
    ftruthf( filewrite, "Is a mount     : ", capismount[profile] );
    ftruthf( filewrite, "Is stackable   : ", capisstackable[profile] );
    ftruthf( filewrite, "Name known     : ", capnameknown[character] );
    ftruthf( filewrite, "Usage known    : ", capusageknown[profile] );
    ftruthf( filewrite, "Is exportable  : ", capcancarrytonextmodule[profile] );
    ftruthf( filewrite, "Requires skill : ", capneedskillidtouse[profile] );
    ftruthf( filewrite, "Is platform    : ", capisplatform[profile] );
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
    ftruthf( filewrite, "Left valid     : ", capslotvalid[profile][SLOT_LEFT] );
    ftruthf( filewrite, "Right valid    : ", capslotvalid[profile][SLOT_RIGHT] );
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
    ftruthf( filewrite, "Blud valid    : ", capbludlevel[profile] );
    fprintf( filewrite, "Part type      : %d\n", capbludprttype[profile] );
    fprintf( filewrite, "\n" );



    // Extra stuff
    ftruthf( filewrite, "Waterwalking   : ", capwaterwalk[profile] );
    fprintf( filewrite, "Bounce dampen  : %5.3f\n", capdampen[profile] );
    fprintf( filewrite, "\n" );



    // More stuff
    fprintf( filewrite, "Life healing   : %5.3f\n", FP8_TO_FLOAT( caplifeheal_fp8[profile] ) );
    fprintf( filewrite, "Mana cost      : %5.3f\n", FP8_TO_FLOAT( capmanacost_fp8[profile] ) );
    fprintf( filewrite, "Life return    : %d\n", caplifereturn_fp8[profile] );
    fprintf( filewrite, "Stopped by     : %d\n", capstoppedby[profile] );

    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      STRING stmp;
      snprintf( stmp, sizeof( stmp ), "Skin %d name    : ", skin );
      funderf( filewrite, stmp, capskinname[profile][skin] );
    };

    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      fprintf( filewrite, "Skin %d cost    : %d\n", skin, capskincost[profile][skin] );
    };

    fprintf( filewrite, "STR dampen     : %5.3f\n", capstrengthdampen[profile] );
    fprintf( filewrite, "\n" );



    // Another memory lapse
    ftruthf( filewrite, "No rider attak : ", btrue - capridercanattack[profile] );
    ftruthf( filewrite, "Can be dazed   : ", capcanbedazed[profile] );
    ftruthf( filewrite, "Can be grogged : ", capcanbegrogged[profile] );
    fprintf( filewrite, "NOT USED       : 0\n" );
    fprintf( filewrite, "NOT USED       : 0\n" );
    ftruthf( filewrite, "Can see invisi : ", capcanseeinvisible[profile] );
    fprintf( filewrite, "Kursed chance  : %d\n", chriskursed[character]*100 );
    fprintf( filewrite, "Footfall sound : %d\n", capfootfallsound[profile] );
    fprintf( filewrite, "Jump sound     : %d\n", capjumpsound[profile] );
    fprintf( filewrite, "\n" );


    // Expansions
    fprintf( filewrite, ":[GOLD] %d\n", capmoney[character] );

    if ( capskindressy[profile]&1 ) fprintf( filewrite, ":[DRES] 0\n" );
    if ( capskindressy[profile]&2 ) fprintf( filewrite, ":[DRES] 1\n" );
    if ( capskindressy[profile]&4 ) fprintf( filewrite, ":[DRES] 2\n" );
    if ( capskindressy[profile]&8 ) fprintf( filewrite, ":[DRES] 3\n" );
    if ( capresistbumpspawn[profile] ) fprintf( filewrite, ":[STUK] 0\n" );
    if ( capistoobig[profile] ) fprintf( filewrite, ":[PACK] 0\n" );
    if ( !capreflect[profile] ) fprintf( filewrite, ":[VAMP] 1\n" );
    if ( capalwaysdraw[profile] ) fprintf( filewrite, ":[DRAW] 1\n" );
    if ( capisranged[profile] ) fprintf( filewrite, ":[RANG] 1\n" );
    if ( caphidestate[profile] != NOHIDE ) fprintf( filewrite, ":[HIDE] %d\n", caphidestate[profile] );
    if ( capisequipment[profile] ) fprintf( filewrite, ":[EQUI] 1\n" );
    if ( capbumpsizebig[character] >= capbumpsize[character] ) fprintf( filewrite, ":[SQUA] 1\n" );
    if ( capicon[character] != capusageknown[profile] ) fprintf( filewrite, ":[ICON] %d\n", capicon[character] );
    if ( capforceshadow[profile] ) fprintf( filewrite, ":[SHAD] 1\n" );

    //Skill expansions
    if ( capcanseekurse[character] )  fprintf( filewrite, ":[CKUR] 1\n" );
    if ( capcanusearcane[character] ) fprintf( filewrite, ":[WMAG] 1\n" );
    if ( capcanjoust[character] )     fprintf( filewrite, ":[JOUS] 1\n" );
    if ( capcanusedivine[character] ) fprintf( filewrite, ":[HMAG] 1\n" );
    if ( capcandisarm[character] )    fprintf( filewrite, ":[DISA] 1\n" );
    if ( capcanusetech[character] )   fprintf( filewrite, ":[TECH] 1\n" );
    if ( capcanbackstab[character] )  fprintf( filewrite, ":[STAB] 1\n" );
    if ( capcanuseadvancedweapons[character] ) fprintf( filewrite, ":[AWEP] 1\n" );
    if ( capcanusepoison[character] ) fprintf( filewrite, ":[POIS] 1\n" );
    if ( capcanread[character] )  fprintf( filewrite, ":[READ] 1\n" );

    //General exported character information
    fprintf( filewrite, ":[PLAT] %d\n", capcanuseplatforms[profile] );
    fprintf( filewrite, ":[SKIN] %d\n", ( chrtexture[character] - madskinstart[profile] ) % MAXSKIN );
    fprintf( filewrite, ":[CONT] %d\n", chraicontent[character] );
    fprintf( filewrite, ":[STAT] %d\n", chraistate[character] );
    fprintf( filewrite, ":[LEVL] %d\n", chrexperiencelevel[character] );
    fs_fileClose( filewrite );
  }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin( char *szSaveName, CHR_REF character )
{
  // ZZ> This function creates a skin.txt file for the given character.
  FILE* filewrite;
  int profile;

  // General stuff
  profile = chrmodel[character];

  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( NULL != filewrite )
  {
    fprintf( filewrite, "This file is used only by the import menu\n" );
    fprintf( filewrite, ": %d\n", ( chrtexture[character] - madskinstart[profile] ) % MAXSKIN );
    fs_fileClose( filewrite );
  }
}

//--------------------------------------------------------------------------------------------
void calc_cap_experience( Uint16 profile )
{
  float statdebt, statperlevel;

  statdebt  = caplife_fp8[profile].ibase + capmana_fp8[profile].ibase + capmanareturn_fp8[profile].ibase + capmanaflow_fp8[profile].ibase;
  statdebt += capstrength_fp8[profile].ibase + capwisdom_fp8[profile].ibase + capintelligence_fp8[profile].ibase + capdexterity_fp8[profile].ibase;
  statdebt += ( caplife_fp8[profile].irand + capmana_fp8[profile].irand + capmanareturn_fp8[profile].irand + capmanaflow_fp8[profile].irand ) * 0.5f;
  statdebt += ( capstrength_fp8[profile].irand + capwisdom_fp8[profile].irand + capintelligence_fp8[profile].irand + capdexterity_fp8[profile].irand ) * 0.5f;

  statperlevel  = caplifeperlevel_fp8[profile].ibase + capmanaperlevel_fp8[profile].ibase + capmanareturnperlevel_fp8[profile].ibase + capmanaflowperlevel_fp8[profile].ibase;
  statperlevel += capstrengthperlevel_fp8[profile].ibase + capwisdomperlevel_fp8[profile].ibase + capintelligenceperlevel_fp8[profile].ibase + capdexterityperlevel_fp8[profile].ibase;
  statperlevel += ( caplifeperlevel_fp8[profile].irand + capmanaperlevel_fp8[profile].irand + capmanareturnperlevel_fp8[profile].irand + capmanaflowperlevel_fp8[profile].irand ) * 0.5f;
  statperlevel += ( capstrengthperlevel_fp8[profile].irand + capwisdomperlevel_fp8[profile].irand + capintelligenceperlevel_fp8[profile].irand + capdexterityperlevel_fp8[profile].irand ) * 0.5f;

  capexperienceconst[profile] = 50.6f * ( FP8_TO_FLOAT( statdebt ) - 51.5 );
  capexperiencecoeff[profile] = 26.3f * MAX( 1, FP8_TO_FLOAT( statperlevel ) );
};

//--------------------------------------------------------------------------------------------
int calc_chr_experience( Uint16 object, float level )
{
  Uint16 profile;

  if ( !VALID_CHR( object ) ) return 0;

  profile = chrmodel[object];

  return level*level*capexperiencecoeff[profile] + capexperienceconst[profile] + 1;
};

//--------------------------------------------------------------------------------------------
float calc_chr_level( Uint16 object )
{
  Uint16 profile;
  float  level;

  if ( !VALID_CHR( object ) ) return 0.0f;

  profile = chrmodel[object];

  level = ( chrexperience[object] - capexperienceconst[profile] ) / capexperiencecoeff[profile];
  if ( level <= 0.0f )
  {
    level = 0.0f;
  }
  else
  {
    level = sqrt( level );
  }

  return level;
};


//--------------------------------------------------------------------------------------------
int load_one_character_profile( char *szLoadName )
{
  // ZZ> This function fills a character profile with data from CData.data_file, returning
  // the object slot that the profile was stuck into.  It may cause the program
  // to abort if bad things happen.
  FILE* fileread;
  int object = MAXMODEL;
  int skin, cnt;
  int iTmp;
  char cTmp;
  int damagetype, level, xptype;
  IDSZ idsz;

  // Open the file
  fileread = fs_fileOpen( PRI_FAIL, "load_one_character_profile()", szLoadName, "r" );
  if ( fileread == NULL )
  {
    // The data file wasn't found
    log_error( "Data.txt could not be correctly read! (%s) \n", szLoadName );
    return object;
  }

  globalname = szLoadName;
  // Read in the object slot
  object = fget_next_int( fileread );
  if ( object < 0 )
  {
    if ( importobject < 0 )
    {
      log_error( "Object slot number %i is invalid. (%s) \n", object, szLoadName );
    }
    else
    {
      object = importobject;
    }
  }

  // Read in the real general data
  fget_next_name( fileread, capclassname[object], sizeof( capclassname[object] ) );


  // Make sure we don't load over an existing model
  if ( madused[object] )
  {
    log_error( "Object slot %i is already used. (%s)\n", object, szLoadName );
  }
  madused[object] = btrue;


  // Light cheat
  capuniformlit[object] = fget_next_bool( fileread );

  // Ammo
  capammomax[object] = fget_next_int( fileread );
  capammo[object] = fget_next_int( fileread );

  // Gender
  capgender[object] = fget_next_gender( fileread );

  // Read in the object stats
  caplifecolor[object] = fget_next_int( fileread );
  capmanacolor[object] = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &caplife_fp8[object] );
  fget_next_pair_fp8( fileread, &caplifeperlevel_fp8[object] );
  fget_next_pair_fp8( fileread, &capmana_fp8[object] );
  fget_next_pair_fp8( fileread, &capmanaperlevel_fp8[object] );
  fget_next_pair_fp8( fileread, &capmanareturn_fp8[object] );
  fget_next_pair_fp8( fileread, &capmanareturnperlevel_fp8[object] );
  fget_next_pair_fp8( fileread, &capmanaflow_fp8[object] );
  fget_next_pair_fp8( fileread, &capmanaflowperlevel_fp8[object] );
  fget_next_pair_fp8( fileread, &capstrength_fp8[object] );
  fget_next_pair_fp8( fileread, &capstrengthperlevel_fp8[object] );
  fget_next_pair_fp8( fileread, &capwisdom_fp8[object] );
  fget_next_pair_fp8( fileread, &capwisdomperlevel_fp8[object] );
  fget_next_pair_fp8( fileread, &capintelligence_fp8[object] );
  fget_next_pair_fp8( fileread, &capintelligenceperlevel_fp8[object] );
  fget_next_pair_fp8( fileread, &capdexterity_fp8[object] );
  fget_next_pair_fp8( fileread, &capdexterityperlevel_fp8[object] );

  // More physical attributes
  capsize[object] = fget_next_float( fileread );
  capsizeperlevel[object] = fget_next_float( fileread );
  capshadowsize[object] = fget_next_int( fileread );
  capbumpsize[object] = fget_next_int( fileread );
  capbumpheight[object] = fget_next_int( fileread );
  capbumpdampen[object] = fget_next_float( fileread );
  capweight[object] = fget_next_int( fileread );
  if ( capweight[object] == 255.0f ) capweight[object] = -1.0f;
  if ( capweight[object] ==   0.0f ) capweight[object] = 1.0f;

  capbumpstrength[object] = ( capbumpsize[object] > 0.0f ) ? 1.0f : 0.0f;
  if ( capbumpsize[object]   == 0.0f ) capbumpsize[object]   = 1.0f;
  if ( capbumpheight[object] == 0.0f ) capbumpheight[object] = 1.0f;
  if ( capweight[object]     == 0.0f ) capweight[object]     = 1.0f;

  capjump[object] = fget_next_float( fileread );
  capjumpnumber[object] = fget_next_int( fileread );
  capsneakspd[object] = fget_next_int( fileread );
  capwalkspd[object] = fget_next_int( fileread );
  caprunspd[object] = fget_next_int( fileread );
  capflyheight[object] = fget_next_int( fileread );
  capflashand[object] = fget_next_int( fileread );
  capalpha_fp8[object] = fget_next_int( fileread );
  caplight_fp8[object] = fget_next_int( fileread );
  if ( caplight_fp8[object] < 0xff )
  {
    capalpha_fp8[object] = MIN( capalpha_fp8[object], 0xff - caplight_fp8[object] );
  };

  captransferblend[object] = fget_next_bool( fileread );
  capsheen_fp8[object] = fget_next_int( fileread );
  capenviro[object] = fget_next_bool( fileread );
  capuoffvel[object] = fget_next_float( fileread ) * (float)UINT16_MAX;
  capvoffvel[object] = fget_next_float( fileread ) * (float)UINT16_MAX;
  capstickybutt[object] = fget_next_bool( fileread );


  // Invulnerability data
  capinvictus[object] = fget_next_bool( fileread );
  capnframefacing[object] = fget_next_int( fileread );
  capnframeangle[object] = fget_next_int( fileread );
  capiframefacing[object] = fget_next_int( fileread );
  capiframeangle[object] = fget_next_int( fileread );
  // Resist burning and stuck arrows with nframe angle of 1 or more
  if ( capnframeangle[object] > 0 )
  {
    if ( capnframeangle[object] == 1 )
    {
      capnframeangle[object] = 0;
    }
  }


  // Skin defenses ( 4 skins )
  fgoto_colon( fileread );
  for ( skin = 0; skin < MAXSKIN; skin++ )
    { capdefense_fp8[object][skin] = 255 - fget_int( fileread ); };

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fgoto_colon( fileread );
    for ( skin = 0;skin < MAXSKIN;skin++ )
      { capdamagemodifier_fp8[object][damagetype][skin] = fget_int( fileread ); };
  }

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fgoto_colon( fileread );
    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      cTmp = fget_first_letter( fileread );
      switch ( toupper( cTmp ) )
      {
        case 'T': capdamagemodifier_fp8[object][damagetype][skin] |= DAMAGE_INVERT; break;
        case 'C': capdamagemodifier_fp8[object][damagetype][skin] |= DAMAGE_CHARGE; break;
        case 'M': capdamagemodifier_fp8[object][damagetype][skin] |= DAMAGE_MANA;   break;
      };
    }
  }

  fgoto_colon( fileread );
  for ( skin = 0;skin < MAXSKIN;skin++ )
    { capmaxaccel[object][skin] = fget_float( fileread ) / 80.0; };


  // Experience and level data
  capexperienceforlevel[object][0] = 0;
  for ( level = 1; level < MAXLEVEL; level++ )
    { capexperienceforlevel[object][level] = fget_next_int( fileread ); }

  fget_next_pair( fileread, &capexperience[object] );
  capexperienceworth[object] = fget_next_int( fileread );
  capexperienceexchange[object] = fget_next_float( fileread );

  for ( xptype = 0; xptype < XP_COUNT; xptype++ )
    { capexperiencerate[object][xptype] = fget_next_float( fileread ) + 0.001f; }


  // IDSZ tags
  for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
    { capidsz[object][cnt] = fget_next_idsz( fileread ); }


  // Item and damage flags
  capisitem[object] = fget_next_bool( fileread );
  capismount[object] = fget_next_bool( fileread );
  capisstackable[object] = fget_next_bool( fileread );
  capnameknown[object] = fget_next_bool( fileread );
  capusageknown[object] = fget_next_bool( fileread );
  capcancarrytonextmodule[object] = fget_next_bool( fileread );
  capneedskillidtouse[object] = fget_next_bool( fileread );
  capisplatform[object] = fget_next_bool( fileread );
  capcangrabmoney[object] = fget_next_bool( fileread );
  capcanopenstuff[object] = fget_next_bool( fileread );



  // More item and damage stuff
  capdamagetargettype[object] = fget_next_damage( fileread );
  capweaponaction[object] = fget_next_action( fileread );


  // Particle attachments
  capattachedprtamount[object] = fget_next_int( fileread );
  capattachedprtreaffirmdamagetype[object] = fget_next_damage( fileread );
  capattachedprttype[object] = fget_next_int( fileread );


  // Character hands
  capslotvalid[object][SLOT_LEFT] = fget_next_bool( fileread );
  capslotvalid[object][SLOT_RIGHT] = fget_next_bool( fileread );
  if ( capismount[object] )
  {
    capslotvalid[object][SLOT_SADDLE] = capslotvalid[object][SLOT_LEFT];
    capslotvalid[object][SLOT_LEFT]   = bfalse;
    //capslotvalid[object][SLOT_RIGHT]  = bfalse;
  };



  // Attack order ( weapon )
  capattackattached[object] = fget_next_bool( fileread );
  capattackprttype[object] = fget_next_int( fileread );


  // GoPoof
  capgopoofprtamount[object] = fget_next_int( fileread );
  capgopoofprtfacingadd[object] = fget_next_int( fileread );
  capgopoofprttype[object] = fget_next_int( fileread );


  // Blud
  capbludlevel[object] = fget_next_blud( fileread );
  capbludprttype[object] = fget_next_int( fileread );


  // Stuff I forgot
  capwaterwalk[object] = fget_next_bool( fileread );
  capdampen[object] = fget_next_float( fileread );


  // More stuff I forgot
  caplifeheal_fp8[object] = fget_next_fixed( fileread );
  capmanacost_fp8[object] = fget_next_fixed( fileread );
  caplifereturn_fp8[object] = fget_next_int( fileread );
  capstoppedby[object] = fget_next_int( fileread ) | MESHFX_IMPASS;

  for ( skin = 0;skin < MAXSKIN;skin++ )
    { fget_next_name( fileread, capskinname[object][skin], sizeof( capskinname[object][skin] ) ); };

  for ( skin = 0;skin < MAXSKIN;skin++ )
    { capskincost[object][skin] = fget_next_int( fileread ); };

  capstrengthdampen[object] = fget_next_float( fileread );



  // Another memory lapse
  capridercanattack[object] = !fget_next_bool( fileread );
  capcanbedazed[object] = fget_next_bool( fileread );
  capcanbegrogged[object] = fget_next_bool( fileread );
  fget_next_int( fileread );   // !!!BAD!!! Life add
  fget_next_int( fileread );   // !!!BAD!!! Mana add
  capcanseeinvisible[object] = fget_next_bool( fileread );
  capkursechance[object] = fget_next_int( fileread );

  iTmp = fget_next_int( fileread ); capfootfallsound[object] = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); capjumpsound[object]     = FIX_SOUND( iTmp );



  // Clear expansions...
  capskindressy[object] = bfalse;
  capresistbumpspawn[object] = bfalse;
  capistoobig[object] = bfalse;
  capreflect[object] = btrue;
  capalwaysdraw[object] = bfalse;
  capisranged[object] = bfalse;
  caphidestate[object] = NOHIDE;
  capisequipment[object] = bfalse;
  capbumpsizebig[object] = capbumpsize[object] * SQRT_TWO;
  capmoney[object] = 0;
  capicon[object] = capusageknown[object];
  capforceshadow[object] = bfalse;
  capskinoverride[object] = NOSKINOVERRIDE;
  capcontentoverride[object] = 0;
  capstateoverride[object] = 0;
  capleveloverride[object] = 0;
  capcanuseplatforms[object] = !capisplatform[object];

  //Reset Skill Expansions
  capcanseekurse[object] = bfalse;
  capcanusearcane[object] = bfalse;
  capcanjoust[object] = bfalse;
  capcanusedivine[object] = bfalse;
  capcandisarm[object] = bfalse;
  capcanusetech[object] = bfalse;
  capcanbackstab[object] = bfalse;
  capcanusepoison[object] = bfalse;
  capcanuseadvancedweapons[object] = bfalse;

  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );
    if ( MAKE_IDSZ( "GOLD" ) == idsz )  capmoney[object] = iTmp;
    else if ( MAKE_IDSZ( "STUK" ) == idsz )  capresistbumpspawn[object] = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "PACK" ) == idsz )  capistoobig[object] = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "VAMP" ) == idsz )  capreflect[object] = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DRAW" ) == idsz )  capalwaysdraw[object] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RANG" ) == idsz )  capisranged[object] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "HIDE" ) == idsz )  caphidestate[object] = iTmp;
    else if ( MAKE_IDSZ( "EQUI" ) == idsz )  capisequipment[object] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SQUA" ) == idsz )  capbumpsizebig[object] = capbumpsize[object] * 2.0f;
    else if ( MAKE_IDSZ( "ICON" ) == idsz )  capicon[object] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SHAD" ) == idsz )  capforceshadow[object] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SKIN" ) == idsz )  capskinoverride[object] = iTmp % MAXSKIN;
    else if ( MAKE_IDSZ( "CONT" ) == idsz )  capcontentoverride[object] = iTmp;
    else if ( MAKE_IDSZ( "STAT" ) == idsz )  capstateoverride[object] = iTmp;
    else if ( MAKE_IDSZ( "LEVL" ) == idsz )  capleveloverride[object] = iTmp;
    else if ( MAKE_IDSZ( "PLAT" ) == idsz )  capcanuseplatforms[object] = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RIPP" ) == idsz )  capripple[object] = INT_TO_BOOL( iTmp );

    //Skill Expansions
    // [CKUR] Can it see kurses?
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  capcanseekurse[object]  = INT_TO_BOOL( iTmp );
    // [WMAG] Can the character use arcane spellbooks?
    else if ( MAKE_IDSZ( "WMAG" ) == idsz )  capcanusearcane[object] = INT_TO_BOOL( iTmp );
    // [JOUS] Can the character joust with a lance?
    else if ( MAKE_IDSZ( "JOUS" ) == idsz )  capcanjoust[object]     = INT_TO_BOOL( iTmp );
    // [HMAG] Can the character use divine spells?
    else if ( MAKE_IDSZ( "HMAG" ) == idsz )  capcanusedivine[object] = INT_TO_BOOL( iTmp );
    // [TECH] Able to use items technological items?
    else if ( MAKE_IDSZ( "TECH" ) == idsz )  capcanusetech[object]   = INT_TO_BOOL( iTmp );
    // [DISA] Find and disarm traps?
    else if ( MAKE_IDSZ( "DISA" ) == idsz )  capcandisarm[object]    = INT_TO_BOOL( iTmp );
    // [STAB] Backstab and murder?
    else if ( idsz == MAKE_IDSZ( "STAB" ) )  capcanbackstab[object]  = INT_TO_BOOL( iTmp );
    // [AWEP] Profiency with advanced weapons?
    else if ( idsz == MAKE_IDSZ( "AWEP" ) )  capcanuseadvancedweapons[object] = INT_TO_BOOL( iTmp );
    // [POIS] Use poison without err?
    else if ( idsz == MAKE_IDSZ( "POIS" ) )  capcanusepoison[object] = INT_TO_BOOL( iTmp );
  }

  fs_fileClose( fileread );

  calc_cap_experience( object );

  return object;
}

//--------------------------------------------------------------------------------------------
int get_skin( char *filename )
{
  // ZZ> This function reads the skin.txt file...
  FILE*   fileread;
  int skin;


  skin = 0;
  fileread = fs_fileOpen( PRI_NONE, NULL, filename, "r" );
  if ( NULL != fileread )
  {
    skin = fget_next_int( fileread );
    skin %= MAXSKIN;
    fs_fileClose( fileread );
  }
  return skin;
}

//--------------------------------------------------------------------------------------------
void check_player_import( char *dirname )
{
  // ZZ> This function figures out which players may be imported, and loads basic
  //     data for each
  char searchname[128];
  char filename[128];
  int skin;
  bool_t keeplooking;
  const char *foundfile;


  // Set up...
  numloadplayer = 0;

  // Search for all objects
  snprintf( searchname, sizeof( searchname ), "%s/*.obj", dirname );
  foundfile = fs_findFirstFile( dirname, "obj" );
  keeplooking = 1;
  if ( foundfile != NULL )
  {
    while ( keeplooking && numloadplayer < MAXLOADPLAYER )
    {
      prime_names();
      strncpy( loadplayerdir[numloadplayer], foundfile, sizeof( loadplayerdir[numloadplayer] ) );

      snprintf( filename, sizeof( filename ), "%s/%s/%s", dirname, foundfile, CData.skin_file );
      skin = get_skin( filename );

      snprintf( filename, sizeof( filename ), "%s/%s/tris.md2", dirname, foundfile );
      load_one_md2( filename, numloadplayer );

      snprintf( filename, sizeof( filename ), "%s/%s/icon%d.bmp", dirname, foundfile, skin );
      load_one_icon( filename );

      snprintf( filename, sizeof( filename ), "%s/%s/naming.txt", dirname, foundfile );
      read_naming( 0, filename );
      naming_names( 0 );
      strncpy( loadplayername[numloadplayer], namingnames, sizeof( loadplayername[numloadplayer] ) );

      numloadplayer++;

      foundfile = fs_findNextFile();
      if ( foundfile == NULL ) keeplooking = 0; else keeplooking = 1;
    }
  }
  fs_findClose();

  nullicon = globalnumicon;
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.nullicon_bitmap );
  load_one_icon( CStringTmp1 );

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.keybicon_bitmap );
  keybicon = globalnumicon;
  load_one_icon( CStringTmp1 );

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.mousicon_bitmap );
  mousicon = globalnumicon;
  load_one_icon( CStringTmp1 );

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.joyaicon_bitmap );
  joyaicon = globalnumicon;
  load_one_icon( CStringTmp1 );

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.joybicon_bitmap );
  joybicon = globalnumicon;
  load_one_icon( CStringTmp1 );


  keybplayer = 0;
  mousplayer = 0;
  joyaplayer = 0;
  joybplayer = 0;
}

//--------------------------------------------------------------------------------------------
bool_t check_skills( int who, Uint32 whichskill )
{
  // ZF> This checks if the specified character has the required skill. Returns btrue if true
  // and bfalse if not. Also checks Skill expansions.
  bool_t result = bfalse;

  // First check the character Skill ID matches
  // Then check for expansion skills too.
  if ( capidsz[chrmodel[who]][IDSZ_SKILL] == whichskill ) result = btrue;
  else if ( MAKE_IDSZ( "CKUR" ) == whichskill ) result = chrcanseekurse[who];
  else if ( MAKE_IDSZ( "WMAG" ) == whichskill ) result = chrcanusearcane[who];
  else if ( MAKE_IDSZ( "JOUS" ) == whichskill ) result = chrcanjoust[who];
  else if ( MAKE_IDSZ( "HMAG" ) == whichskill ) result = chrcanusedivine[who];
  else if ( MAKE_IDSZ( "DISA" ) == whichskill ) result = chrcandisarm[who];
  else if ( MAKE_IDSZ( "TECH" ) == whichskill ) result = chrcanusetech[who];
  else if ( MAKE_IDSZ( "AWEP" ) == whichskill ) result = chrcanuseadvancedweapons[who];
  else if ( MAKE_IDSZ( "STAB" ) == whichskill ) result = chrcanbackstab[who];
  else if ( MAKE_IDSZ( "POIS" ) == whichskill ) result = chrcanusepoison[who];
  else if ( MAKE_IDSZ( "READ" ) == whichskill ) result = chrcanread[who];

  return result;
}

//--------------------------------------------------------------------------------------------
int check_player_quest( char *whichplayer, IDSZ idsz )
{
  // ZF> This function checks if the specified player has the IDSZ in his or her quest.txt
  // and returns the quest level of that specific quest (Or -2 if it is not found, -1 if it is finished)

  //TODO: should also check if the IDSZ isnt beaten
  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz = bfalse;
  int result = -2;
  int iTmp;

  //Always return "true" for [NONE] IDSZ checks
  if (idsz == IDSZ_NONE) result = -1;

  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread )
  {
    log_warning( "File could not be read. (%s)\n", newloadname );
    return result;
  };

  // Check each expansion
  while ( fgoto_colon_yesno( fileread ) && !foundidsz )
  {
    newidsz = fget_idsz( fileread );
    if ( newidsz == idsz )
    {
      foundidsz = btrue;
      //iTmp = fget_int(fileread);  //Read value behind colon (TODO)
      iTmp = 0; //BAD should be read value
      result = iTmp;
    }
  }

  fs_fileClose( fileread ); 

  return result;
}


//--------------------------------------------------------------------------------------------
bool_t add_quest_idsz( char *whichplayer, IDSZ idsz )
{
  // ZF> This function writes a IDSZ into a player quest.txt file, returns btrue if succeeded
  FILE *filewrite;
  STRING newloadname;
  bool_t result = bfalse;

  // Only add quest IDSZ if it doesnt have it already
  if (check_player_quest(whichplayer, idsz) >= -1)
  {
    return result;
  };

  // Try to open the file in read and append mode
  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
  filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "a+" );
  if ( NULL == filewrite )
  {
    log_warning( "Could not write into %s\n", newloadname );
    return result;
  };

  fprintf( filewrite, "\n:[%4s]: 0", undo_idsz( idsz ) );
  fs_fileClose( filewrite );

  return result;
}

//--------------------------------------------------------------------------------------------
int modify_quest_level( char *whichplayer, IDSZ idsz, int adjustment )
// ZF> This function increases or decreases a Quest IDSZ quest level by the amount determined in
// adjustment. It then returns the current quest level it now has (Or -1 if not found).
{
  //TODO
  return -1;
}

//--------------------------------------------------------------------------------------------
bool_t beat_quest_idsz( char *whichplayer, IDSZ idsz )
{
  // ZF> This function marks a IDSZ in the quest.txt file as beaten (-1)
  //     and returns btrue if it succeeded.
  FILE *filewrite;
  STRING newloadname;
  bool_t result = bfalse;
  bool_t foundidsz = bfalse;
  IDSZ newidsz;
  int QuestLevel;

  //TODO: This also needs to be done

  // Try to open the file in read/write mode
  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
  filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "w+" );
  if ( NULL == filewrite )
  {
    log_warning( "Could not write into %s\n", newloadname );
    return result;
  };

  //Now check each expansion until we find correct IDSZ
  while ( fgoto_colon_yesno( filewrite ) && !foundidsz )
  {
    newidsz = fget_idsz( filewrite );
    if ( newidsz == idsz )
    {
      foundidsz = btrue;
      QuestLevel = fget_int( filewrite );
      if ( QuestLevel == -1 ) result = bfalse;  //Is quest is already finished?
      break;
    }
  }
  fs_fileClose( filewrite );

  return result;
}

//--------------------------------------------------------------------------------------------
bool_t chr_attached( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return bfalse;

  chrattachedto[ichr] = VALIDATE_CHR( chrattachedto[ichr] );
  if(!VALID_CHR(ichr)) chrinwhichslot[ichr] = SLOT_NONE;

  return VALID_CHR( chrattachedto[ichr] );
};

//--------------------------------------------------------------------------------------------
bool_t chr_in_pack( CHR_REF ichr )
{
  CHR_REF inwhichpack = chr_get_inwhichpack( ichr );
  return VALID_CHR( inwhichpack );
}

//--------------------------------------------------------------------------------------------
bool_t chr_has_inventory( CHR_REF ichr )
{
  bool_t retval = bfalse;
  CHR_REF nextinpack = chr_get_nextinpack( ichr );

  if ( VALID_CHR( nextinpack ) )
  {
    retval = btrue;
  }
#if defined(_DEBUG) || !defined(NDEBUG)
  else
  {
    assert( chrnuminpack[ichr] == 0 );
  }
#endif

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t chr_is_invisible( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return btrue;

  return FP8_MUL( chralpha_fp8[ichr], chrlight_fp8[ichr] ) <= INVISIBLE;
};

//--------------------------------------------------------------------------------------------
bool_t chr_using_slot( CHR_REF ichr, SLOT slot )
{
  CHR_REF inslot = chr_get_holdingwhich( ichr, slot );

  return VALID_CHR( inslot );
};


//--------------------------------------------------------------------------------------------
CHR_REF chr_get_nextinpack( CHR_REF ichr )
{
  CHR_REF nextinpack = MAXCHR;

  if ( !VALID_CHR( ichr ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  nextinpack = chrnextinpack[ichr];
  if ( MAXCHR != nextinpack && !chron[ichr] )
  {
    // this is an invalid configuration that may indicate a corrupted list
    nextinpack = chrnextinpack[nextinpack];
    if ( VALID_CHR( nextinpack ) )
    {
      // the list is definitely corrupted
      assert( bfalse );
    }
  }
#endif

  chrnextinpack[ichr] = VALIDATE_CHR( chrnextinpack[ichr] );
  return chrnextinpack[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_onwhichplatform( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chronwhichplatform[ichr] = VALIDATE_CHR( chronwhichplatform[ichr] );
  return chronwhichplatform[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_holdingwhich( CHR_REF ichr, SLOT slot )
{
  CHR_REF inslot;

  if ( !VALID_CHR( ichr ) || slot >= SLOT_COUNT ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  inslot = chrholdingwhich[ichr][slot];
  if ( MAXCHR != inslot )
  {
    CHR_REF holder = chrattachedto[inslot];

    if ( ichr != holder )
    {
      // invalid configuration
      assert( bfalse );
    }
  };
#endif

  chrholdingwhich[ichr][slot] = VALIDATE_CHR( chrholdingwhich[ichr][slot] );
  return chrholdingwhich[ichr][slot];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_inwhichpack( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chrinwhichpack[ichr] = VALIDATE_CHR( chrinwhichpack[ichr] );
  return chrinwhichpack[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_attachedto( CHR_REF ichr )
{
  CHR_REF holder;

  if ( !VALID_CHR( ichr ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)

  if( MAXCHR != chrattachedto[ichr] )
  {
    SLOT slot = chrinwhichslot[ichr];
    if(slot != SLOT_INVENTORY)
    {
      assert(SLOT_NONE != slot);
      holder = chrattachedto[ichr];
      assert( chrholdingwhich[holder][slot] == ichr );
    };
  }
  else
  {
    assert(SLOT_NONE == chrinwhichslot[ichr]);
  };
#endif

  chrattachedto[ichr] = VALIDATE_CHR( chrattachedto[ichr] );
  if( !VALID_CHR( chrattachedto[ichr] ) ) chrinwhichslot[ichr] = SLOT_NONE;
  return chrattachedto[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_bumpnext( CHR_REF ichr )
{
  CHR_REF bumpnext;

  if ( !VALID_CHR( ichr ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  bumpnext = chrbumpnext[ichr];
  if ( bumpnext < MAXCHR && !chron[bumpnext] && chrbumpnext[bumpnext] < MAXCHR )
  {
    // this is an invalid configuration
    assert( bfalse );
    return chr_get_bumpnext( bumpnext );
  }
#endif

  chrbumpnext[ichr] = VALIDATE_CHR( chrbumpnext[ichr] );
  return chrbumpnext[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aitarget( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chraitarget[ichr] = VALIDATE_CHR( chraitarget[ichr] );
  return chraitarget[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aiowner( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chraiowner[ichr] = VALIDATE_CHR( chraiowner[ichr] );
  return chraiowner[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aichild( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chraichild[ichr] = VALIDATE_CHR( chraichild[ichr] );
  return chraichild[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aiattacklast( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chraiattacklast[ichr] = VALIDATE_CHR( chraiattacklast[ichr] );
  return chraiattacklast[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aibumplast( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chraibumplast[ichr] = VALIDATE_CHR( chraibumplast[ichr] );
  return chraibumplast[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aihitlast( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  chraihitlast[ichr] = VALIDATE_CHR( chraihitlast[ichr] );
  return chraihitlast[ichr];
};

//--------------------------------------------------------------------------------------------
CHR_REF team_get_sissy( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  teamsissy[iteam] = VALIDATE_CHR( teamsissy[iteam] );
  return teamsissy[iteam];
};

//--------------------------------------------------------------------------------------------
CHR_REF team_get_leader( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  teamleader[iteam] = VALIDATE_CHR( teamleader[iteam] );
  return teamleader[iteam];
};


//--------------------------------------------------------------------------------------------
CHR_REF pla_get_character( PLA_REF iplayer )
{
  if ( !VALID_PLA( iplayer ) ) return MAXCHR;

  plachr[iplayer] = VALIDATE_CHR( plachr[iplayer] );
  return plachr[iplayer];
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void VData_Blended_construct(VData_Blended * v)
{
  if(NULL == v) return;

  v->Vertices = NULL;
  v->Normals  = NULL;
  v->Colors   = NULL;
  v->Texture  = NULL;
  v->Ambient  = NULL;

  v->frame0 = 0;
  v->frame1 = 0;
  v->vrtmin = 0;
  v->vrtmax = 0;
  v->lerp   = 0.0f;
  v->needs_lighting = btrue;
}

//--------------------------------------------------------------------------------------------
void VData_Blended_destruct(VData_Blended * v)
{
  VData_Blended_Deallocate(v);
};


//--------------------------------------------------------------------------------------------
void VData_Blended_Deallocate(VData_Blended * v)
{
  if(NULL == v) return;

  if(NULL!=v->Vertices)
  {
    free( v->Vertices );
    v->Vertices = NULL;
  } 

  if(NULL!=v->Normals)
  {
    free( v->Normals );
    v->Normals = NULL;
  } 

  if(NULL!=v->Colors)
  {
    free( v->Colors );
    v->Colors = NULL;
  } 

  if(NULL!=v->Texture)
  {
    free( v->Texture );
    v->Texture = NULL;
  } 

  if(NULL!=v->Ambient)
  {
    free( v->Ambient );
    v->Ambient = NULL;
  } 
}

//--------------------------------------------------------------------------------------------
VData_Blended * VData_Blended_new()
{
  VData_Blended * retval = calloc(sizeof(VData_Blended), 1);
  if(NULL != retval)
  {
    VData_Blended_construct(retval);
  };
  return retval;
};

//--------------------------------------------------------------------------------------------
void VData_Blended_delete(VData_Blended * v)
{
  if(NULL != v) return;

  VData_Blended_destruct(v);
  free(v);
};

//--------------------------------------------------------------------------------------------
void VData_Blended_Allocate(VData_Blended * v, size_t verts)
{
  if(NULL == v) return;

  VData_Blended_destruct(v);

  v->Vertices = calloc( sizeof(vect3), verts);
  v->Normals  = calloc( sizeof(vect3), verts);
  v->Colors   = calloc( sizeof(vect4), verts);
  v->Texture  = calloc( sizeof(vect2), verts);
  v->Ambient  = calloc( sizeof(float), verts);
}

//--------------------------------------------------------------------------------------------
bool_t chr_cvolume_reinit(CHR_REF ichr, CVolume * pcv)
{
  if(!VALID_CHR(ichr) || NULL == pcv) return bfalse;

  pcv->x_min = chrpos[ichr].x - chrbmpdata[ichr].size * chrscale[ichr] * chrpancakepos[ichr].x;
  pcv->y_min = chrpos[ichr].y - chrbmpdata[ichr].size * chrscale[ichr] * chrpancakepos[ichr].y;
  pcv->z_min = chrpos[ichr].z;

  pcv->x_max = chrpos[ichr].x + chrbmpdata[ichr].size * chrscale[ichr] * chrpancakepos[ichr].x;
  pcv->y_max = chrpos[ichr].y + chrbmpdata[ichr].size * chrscale[ichr] * chrpancakepos[ichr].y;
  pcv->z_max = chrpos[ichr].z + chrbmpdata[ichr].height * chrscale[ichr] * chrpancakepos[ichr].z;

  pcv->xy_min = -(chrpos[ichr].x + chrpos[ichr].y) - chrbmpdata[ichr].sizebig * chrscale[ichr] * (chrpancakepos[ichr].x + chrpancakepos[ichr].y) * 0.5f;
  pcv->xy_max = -(chrpos[ichr].x + chrpos[ichr].y) + chrbmpdata[ichr].sizebig * chrscale[ichr] * (chrpancakepos[ichr].x + chrpancakepos[ichr].y) * 0.5f;

  pcv->yx_min = -(-chrpos[ichr].x + chrpos[ichr].y) - chrbmpdata[ichr].sizebig * chrscale[ichr] * (chrpancakepos[ichr].x + chrpancakepos[ichr].y) * 0.5f;
  pcv->yx_max = -(-chrpos[ichr].x + chrpos[ichr].y) + chrbmpdata[ichr].sizebig * chrscale[ichr] * (chrpancakepos[ichr].x + chrpancakepos[ichr].y) * 0.5f;

  pcv->level = -1;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bdata_reinit(CHR_REF ichr, BData * pbd)
{
  if(!VALID_CHR(ichr) || NULL == pbd) return bfalse;

  pbd->calc_is_platform   = chrisplatform[ichr];
  pbd->calc_is_mount      = chrismount[ichr];

  pbd->mids_lo = pbd->mids_hi = chrpos[ichr];
  pbd->mids_hi.z += pbd->height * 0.5f;

  pbd->calc_size     = pbd->size    * chrscale[ichr] * (chrpancakepos[ichr].x + chrpancakepos[ichr].y) * 0.5f;
  pbd->calc_size_big = pbd->sizebig * chrscale[ichr] * (chrpancakepos[ichr].x + chrpancakepos[ichr].y) * 0.5f;

  chr_cvolume_reinit(ichr, &pbd->cv);

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers_0( CHR_REF ichr )
{
  bool_t rv = bfalse;

  if( !VALID_CHR(ichr) ) return bfalse;

  chrbmpdata[ichr].cv_tree = NULL;

  rv = chr_bdata_reinit( ichr, &chrbmpdata[ichr] );

  chrbmpdata[ichr].cv.level = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//bool_t md2_calculate_bumpers_1(CHR_REF ichr)
//{
//  BData * bd;
//  Uint16 imdl;
//  MD2_Model * pmdl;
//  const MD2_Frame * fl, * fc;
//  float lerp;
//  vect3 xdir, ydir, zdir;
//  vect3 points[8], bbmax, bbmin;
//  int cnt;
//
//  CVolume cv;
//
//  if( !VALID_CHR(ichr) ) return bfalse;
//  bd = &chrbmpdata[ichr];
//
//  imdl = chrmodel[ichr];
//  if(!VALID_MDL(imdl) || !chrmatrixvalid[ichr] )
//  {
//    set_default_bump_data( ichr );
//    return bfalse;
//  };
//
//  pmdl = mad_md2[imdl];
//  if(NULL == pmdl)
//  {
//    set_default_bump_data( ichr );
//    return bfalse;
//  }
//
//  fl = md2_get_Frame(pmdl, chrframelast[ichr]);
//  fc = md2_get_Frame(pmdl, chrframe[ichr] );
//  lerp = chrflip[ichr];
//
//  if(NULL==fl && NULL==fc)
//  {
//    set_default_bump_data( ichr );
//    return bfalse;
//  };
//
//  xdir.x = (chrmatrix[ichr])_CNV(0,0);
//  xdir.y = (chrmatrix[ichr])_CNV(0,1);
//  xdir.z = (chrmatrix[ichr])_CNV(0,2);
//
//  ydir.x = (chrmatrix[ichr])_CNV(1,0);
//  ydir.y = (chrmatrix[ichr])_CNV(1,1);
//  ydir.z = (chrmatrix[ichr])_CNV(1,2);
//
//  zdir.x = (chrmatrix[ichr])_CNV(2,0);
//  zdir.y = (chrmatrix[ichr])_CNV(2,1);
//  zdir.z = (chrmatrix[ichr])_CNV(2,2);
//
//  if(NULL==fl || lerp >= 1.0f)
//  {
//    bbmin.x = MIN(fc->bbmin[0], fc->bbmax[0]);
//    bbmin.y = MIN(fc->bbmin[1], fc->bbmax[1]);
//    bbmin.z = MIN(fc->bbmin[2], fc->bbmax[2]);
//
//    bbmax.x = MAX(fc->bbmin[0], fc->bbmax[0]);
//    bbmax.y = MAX(fc->bbmin[1], fc->bbmax[1]);
//    bbmax.z = MAX(fc->bbmin[2], fc->bbmax[2]);
//  }
//  else if(NULL==fc || lerp <= 0.0f)
//  {
//    bbmin.x = MIN(fl->bbmin[0], fl->bbmax[0]);
//    bbmin.y = MIN(fl->bbmin[1], fl->bbmax[1]);
//    bbmin.z = MIN(fl->bbmin[2], fl->bbmax[2]);
//
//    bbmax.x = MAX(fl->bbmin[0], fl->bbmax[0]);
//    bbmax.y = MAX(fl->bbmin[1], fl->bbmax[1]);
//    bbmax.z = MAX(fl->bbmin[2], fl->bbmax[2]);
//  }
//  else
//  {
//    vect3 tmpmin, tmpmax;
//
//    tmpmin.x = fl->bbmin[0] + (fc->bbmin[0]-fl->bbmin[0])*lerp;
//    tmpmin.y = fl->bbmin[1] + (fc->bbmin[1]-fl->bbmin[1])*lerp;
//    tmpmin.z = fl->bbmin[2] + (fc->bbmin[2]-fl->bbmin[2])*lerp;
//
//    tmpmax.x = fl->bbmax[0] + (fc->bbmax[0]-fl->bbmax[0])*lerp;
//    tmpmax.y = fl->bbmax[1] + (fc->bbmax[1]-fl->bbmax[1])*lerp;
//    tmpmax.z = fl->bbmax[2] + (fc->bbmax[2]-fl->bbmax[2])*lerp;
//
//    bbmin.x = MIN(tmpmin.x, tmpmax.x);
//    bbmin.y = MIN(tmpmin.y, tmpmax.y);
//    bbmin.z = MIN(tmpmin.z, tmpmax.z);
//
//    bbmax.x = MAX(tmpmin.x, tmpmax.x);
//    bbmax.y = MAX(tmpmin.y, tmpmax.y);
//    bbmax.z = MAX(tmpmin.z, tmpmax.z);
//  };
//
//  cnt = 0;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmax.z;
//
//  cnt++;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmin.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmax.y;
//  points[cnt].z = bbmin.z;
//
//  cnt++;
//  points[cnt].x = bbmax.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmin.z;
//
//  cnt++;
//  points[cnt].x = bbmin.x;
//  points[cnt].y = bbmin.y;
//  points[cnt].z = bbmin.z;
//
//  cv.x_min  = cv.x_max  = points[0].x*xdir.x + points[0].y*ydir.x + points[0].z*zdir.x;
//  cv.y_min  = cv.y_max  = points[0].x*xdir.y + points[0].y*ydir.y + points[0].z*zdir.y;
//  cv.z_min  = cv.z_max  = points[0].x*xdir.z + points[0].y*ydir.z + points[0].z*zdir.z;
//  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
//  cv.yx_min = cv.yx_max = cv.x_min - cv.y_min;
//
//  for(cnt=1; cnt<8; cnt++)
//  {
//    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;
//
//    tmp_x = points[cnt].x*xdir.x + points[cnt].y*ydir.x + points[cnt].z*zdir.x;
//    if(tmp_x < cv.x_min) cv.x_min = tmp_x - 0.001f;
//    else if(tmp_x > cv.x_max) cv.x_max = tmp_x + 0.001f;
//
//    tmp_y = points[cnt].x*xdir.y + points[cnt].y*ydir.y + points[cnt].z*zdir.y;
//    if(tmp_y < cv.y_min) cv.y_min = tmp_y - 0.001f;
//    else if(tmp_y > cv.y_max) cv.y_max = tmp_y + 0.001f;
//
//    tmp_z = points[cnt].x*xdir.z + points[cnt].y*ydir.z + points[cnt].z*zdir.z;
//    if(tmp_z < cv.z_min) cv.z_min = tmp_z - 0.001f;
//    else if(tmp_z > cv.z_max) cv.z_max = tmp_z + 0.001f;
//
//    tmp_xy = tmp_x + tmp_y;
//    if(tmp_xy < cv.xy_min) cv.xy_min = tmp_xy - 0.001f;
//    else if(tmp_xy > cv.xy_max) cv.xy_max = tmp_xy + 0.001f;
//
//    tmp_yx = -tmp_x + tmp_y;
//    if(tmp_yx < cv.yx_min) cv.yx_min = tmp_yx - 0.001f;
//    else if(tmp_yx > cv.yx_max) cv.yx_max = tmp_yx + 0.001f;
//  };
//
//  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
//  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);
//
//  bd->cv.x_min  = cv.x_min  * 4.125 + chrpos[ichr].x;
//  bd->cv.y_min  = cv.y_min  * 4.125 + chrpos[ichr].y;
//  bd->cv.z_min  = cv.z_min  * 4.125 + chrpos[ichr].z;
//  bd->cv.xy_min = cv.xy_min * 4.125 + ( chrpos[ichr].x + chrpos[ichr].y);
//  bd->cv.yx_min = cv.yx_min * 4.125 + (-chrpos[ichr].x + chrpos[ichr].y);
//
//
//  bd->cv.x_max  = cv.x_max  * 4.125 + chrpos[ichr].x;
//  bd->cv.y_max  = cv.y_max  * 4.125 + chrpos[ichr].y;
//  bd->cv.z_max  = cv.z_max  * 4.125 + chrpos[ichr].z;
//  bd->cv.xy_max = cv.xy_max * 4.125 + ( chrpos[ichr].x + chrpos[ichr].y);
//  bd->cv.yx_max = cv.yx_max * 4.125 + (-chrpos[ichr].x + chrpos[ichr].y);
//
//  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f * 4.125 + chrpos[ichr].x;
//  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f * 4.125 + chrpos[ichr].y;
//  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f * 4.125 + chrpos[ichr].z;
//
//  bd->mids_lo   = bd->mids_hi;
//  bd->mids_lo.z = cv.z_min * 4.125  + chrpos[ichr].z;
//
//  bd->calc_height   = bd->cv.z_max;
//  bd->calc_size     = MAX( bd->cv.x_max, bd->cv.y_max ) - MIN( bd->cv.x_min, bd->cv.y_min );
//  bd->calc_size_big = 0.5f * ( MAX( bd->cv.xy_max, bd->cv.yx_max) - MIN( bd->cv.xy_min, bd->cv.yx_min) );
//
//  if(bd->calc_size_big < bd->calc_size*1.1)
//  {
//    bd->calc_size     *= -1;
//  }
//  else if (bd->calc_size*2 < bd->calc_size_big*1.1)
//  {
//    bd->calc_size_big *= -1;
//  }
//
//  return btrue;
//};
//
//

//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers_1(CHR_REF ichr)
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  const MD2_Frame * fl, * fc;
  float lerp;
  vect3 xdir, ydir, zdir;
  vect3 points[8], bbmax, bbmin;
  int cnt;

  CVolume cv;

  if( !VALID_CHR(ichr) ) return bfalse;
  bd = &chrbmpdata[ichr];

  imdl = chrmodel[ichr];
  if(!VALID_MDL(imdl) || !chrmatrixvalid[ichr] )
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  };

  xdir.x = (chrmatrix[ichr])_CNV(0,0);
  xdir.y = (chrmatrix[ichr])_CNV(0,1);
  xdir.z = (chrmatrix[ichr])_CNV(0,2);

  ydir.x = (chrmatrix[ichr])_CNV(1,0);
  ydir.y = (chrmatrix[ichr])_CNV(1,1);
  ydir.z = (chrmatrix[ichr])_CNV(1,2);

  zdir.x = (chrmatrix[ichr])_CNV(2,0);
  zdir.y = (chrmatrix[ichr])_CNV(2,1);
  zdir.z = (chrmatrix[ichr])_CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = mad_md2[imdl];
  if(NULL == pmdl)
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  }

  fl = md2_get_Frame(pmdl, chrframelast[ichr]);
  fc = md2_get_Frame(pmdl, chrframe[ichr] );
  lerp = chrflip[ichr];

  if(NULL==fl && NULL==fc)
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  };


  if(NULL==fl || lerp >= 1.0f)
  {
    bbmin.x = fc->bbmin[0];
    bbmin.y = fc->bbmin[1];
    bbmin.z = fc->bbmin[2];

    bbmax.x = fc->bbmax[0];
    bbmax.y = fc->bbmax[1];
    bbmax.z = fc->bbmax[2];
  }
  else if(NULL==fc || lerp <= 0.0f)
  {
    bbmin.x = fl->bbmin[0];
    bbmin.y = fl->bbmin[1];
    bbmin.z = fl->bbmin[2];

    bbmax.x = fl->bbmax[0];
    bbmax.y = fl->bbmax[1];
    bbmax.z = fl->bbmax[2];
  }
  else
  {
    bbmin.x = MIN(fl->bbmin[0], fc->bbmin[0]);
    bbmin.y = MIN(fl->bbmin[1], fc->bbmin[1]);
    bbmin.z = MIN(fl->bbmin[2], fc->bbmin[2]);

    bbmax.x = MAX(fl->bbmax[0], fc->bbmax[0]);
    bbmax.y = MAX(fl->bbmax[1], fc->bbmax[1]);
    bbmax.z = MAX(fl->bbmax[2], fc->bbmax[2]);
  };

  cnt = 0;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmax.z;

  cnt++;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmin.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmax.y;
  points[cnt].z = bbmin.z;

  cnt++;
  points[cnt].x = bbmax.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmin.z;

  cnt++;
  points[cnt].x = bbmin.x;
  points[cnt].y = bbmin.y;
  points[cnt].z = bbmin.z;

  cv.x_min  = cv.x_max  = points[0].x*xdir.x + points[0].y*ydir.x + points[0].z*zdir.x;
  cv.y_min  = cv.y_max  = points[0].x*xdir.y + points[0].y*ydir.y + points[0].z*zdir.y;
  cv.z_min  = cv.z_max  = points[0].x*xdir.z + points[0].y*ydir.z + points[0].z*zdir.z;
  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
  cv.yx_min = cv.yx_max = cv.x_min - cv.y_min;

  for(cnt=1; cnt<8; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

    tmp_x = points[cnt].x*xdir.x + points[cnt].y*ydir.x + points[cnt].z*zdir.x;
    if(tmp_x < cv.x_min) cv.x_min = tmp_x - 0.001f;
    else if(tmp_x > cv.x_max) cv.x_max = tmp_x + 0.001f;

    tmp_y = points[cnt].x*xdir.y + points[cnt].y*ydir.y + points[cnt].z*zdir.y;
    if(tmp_y < cv.y_min) cv.y_min = tmp_y - 0.001f;
    else if(tmp_y > cv.y_max) cv.y_max = tmp_y + 0.001f;

    tmp_z = points[cnt].x*xdir.z + points[cnt].y*ydir.z + points[cnt].z*zdir.z;
    if(tmp_z < cv.z_min) cv.z_min = tmp_z - 0.001f;
    else if(tmp_z > cv.z_max) cv.z_max = tmp_z + 0.001f;

    tmp_xy = tmp_x + tmp_y;
    if(tmp_xy < cv.xy_min) cv.xy_min = tmp_xy - 0.001f;
    else if(tmp_xy > cv.xy_max) cv.xy_max = tmp_xy + 0.001f;

    tmp_yx = -tmp_x + tmp_y;
    if(tmp_yx < cv.yx_min) cv.yx_min = tmp_yx - 0.001f;
    else if(tmp_yx > cv.yx_max) cv.yx_max = tmp_yx + 0.001f;
  };

  bd->cv.x_min  = cv.x_min  * 4.125;
  bd->cv.y_min  = cv.y_min  * 4.125;
  bd->cv.z_min  = cv.z_min  * 4.125;
  bd->cv.xy_min = cv.xy_min * 4.125;
  bd->cv.yx_min = cv.yx_min * 4.125;

  bd->cv.x_max  = cv.x_max  * 4.125;
  bd->cv.y_max  = cv.y_max  * 4.125;
  bd->cv.z_max  = cv.z_max  * 4.125;
  bd->cv.xy_max = cv.xy_max * 4.125;
  bd->cv.yx_max = cv.yx_max * 4.125;

  bd->mids_hi.x = (bd->cv.x_min + bd->cv.x_max) * 0.5f + chrpos[ichr].x;
  bd->mids_hi.y = (bd->cv.y_min + bd->cv.y_max) * 0.5f + chrpos[ichr].y;
  bd->mids_hi.z = (bd->cv.z_min + bd->cv.z_max) * 0.5f + chrpos[ichr].z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = bd->cv.z_min + chrpos[ichr].z;

  bd->calc_height   = bd->cv.z_max;
  bd->calc_size     = MAX( MAX( bd->cv.x_max,  bd->cv.y_max ), - MIN( bd->cv.x_min,  bd->cv.y_min ) );
  bd->calc_size_big = MAX( MAX( bd->cv.xy_max, bd->cv.yx_max), - MIN( bd->cv.xy_min, bd->cv.yx_min) );

  if(bd->calc_size_big < bd->calc_size*1.1)
  {
    bd->calc_size     *= -1;
  }
  else if (bd->calc_size*2 < bd->calc_size_big*1.1)
  {
    bd->calc_size_big *= -1;
  }

  bd->cv.level = 1;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers_2(CHR_REF ichr, vect3 * vrt_ary)
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  vect3 xdir, ydir, zdir;
  int cnt;
  Uint32  vrt_count;
  bool_t  free_array = bfalse;

  CVolume cv;

  if( !VALID_CHR(ichr) ) return bfalse;
  bd = &chrbmpdata[ichr];

  imdl = chrmodel[ichr];
  if(!VALID_MDL(imdl) || !chrmatrixvalid[ichr] )
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  };

  xdir.x = (chrmatrix[ichr])_CNV(0,0);
  xdir.y = (chrmatrix[ichr])_CNV(0,1);
  xdir.z = (chrmatrix[ichr])_CNV(0,2);

  ydir.x = (chrmatrix[ichr])_CNV(1,0);
  ydir.y = (chrmatrix[ichr])_CNV(1,1);
  ydir.z = (chrmatrix[ichr])_CNV(1,2);

  zdir.x = (chrmatrix[ichr])_CNV(2,0);
  zdir.y = (chrmatrix[ichr])_CNV(2,1);
  zdir.z = (chrmatrix[ichr])_CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = mad_md2[imdl];
  if(NULL == pmdl)
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  }

  md2_blend_vertices(ichr, -1, -1);

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  if(NULL == vrt_ary)
  {
    vrt_ary = malloc(vrt_count * sizeof(vect3));
    if(NULL==vrt_ary)
    {
      return md2_calculate_bumpers_1( ichr );
    }
    free_array = btrue;
  }

  // transform the verts all at once, to reduce function calling overhead
  Transform3(&chrmatrix[ichr], chrvdata[ichr].Vertices, vrt_ary, vrt_count);

  cv.x_min  = cv.x_max  = vrt_ary[0].x;
  cv.y_min  = cv.y_max  = vrt_ary[0].y;
  cv.z_min  = cv.z_max  = vrt_ary[0].z;
  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
  cv.yx_min = cv.yx_max = cv.x_min - cv.y_min;

  vrt_count = madtransvertices[imdl];
  for(cnt=1; cnt<vrt_count; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;

    tmp_x = vrt_ary[cnt].x;
    if(tmp_x < cv.x_min) cv.x_min = tmp_x - 0.001f;
    else if(tmp_x > cv.x_max) cv.x_max = tmp_x + 0.001f;

    tmp_y = vrt_ary[cnt].y;
    if(tmp_y < cv.y_min) cv.y_min = tmp_y - 0.001f;
    else if(tmp_y > cv.y_max) cv.y_max = tmp_y + 0.001f;

    tmp_z = vrt_ary[cnt].z;
    if(tmp_z < cv.z_min) cv.z_min = tmp_z - 0.001f;
    else if(tmp_z > cv.z_max) cv.z_max = tmp_z + 0.001f;

    tmp_xy = tmp_x + tmp_y;
    if(tmp_xy < cv.xy_min) cv.xy_min = tmp_xy - 0.001f;
    else if(tmp_xy > cv.xy_max) cv.xy_max = tmp_xy + 0.001f;

    tmp_yx = -tmp_x + tmp_y;
    if(tmp_yx < cv.yx_min) cv.yx_min = tmp_yx - 0.001f;
    else if(tmp_yx > cv.yx_max) cv.yx_max = tmp_yx + 0.001f;
  };

  bd->cv = cv;

  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + chrpos[ichr].x;
  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + chrpos[ichr].y;
  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + chrpos[ichr].z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = cv.z_min + chrpos[ichr].z;

  bd->calc_height   = bd->cv.z_max;
  bd->calc_size     = MAX( bd->cv.x_max, bd->cv.y_max ) - MIN( bd->cv.x_min, bd->cv.y_min );
  bd->calc_size_big = 0.5f * ( MAX( bd->cv.xy_max, bd->cv.yx_max) - MIN( bd->cv.xy_min, bd->cv.yx_min) );

  if(bd->calc_size_big < bd->calc_size*1.1)
  {
    bd->calc_size     *= -1;
  }
  else if (bd->calc_size*2 < bd->calc_size_big*1.1)
  {
    bd->calc_size_big *= -1;
  }

  bd->cv.level = 2;

  if(free_array)
  {
    free( vrt_ary );
  }

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers_3(CHR_REF ichr, CVolume_Tree * cv_tree)
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  int cnt, tnc;
  Uint32  tri_count, vrt_count;
  vect3 * vrt_ary;
  CVolume *pcv, cv_node[8];

  if( !VALID_CHR(ichr) ) return bfalse;
  bd = &chrbmpdata[ichr];

  imdl = chrmodel[ichr];
  if(!VALID_MDL(imdl) || !chrmatrixvalid[ichr] )
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  };

  pmdl = mad_md2[imdl];
  if(NULL == pmdl)
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  }

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  vrt_ary   = malloc(vrt_count * sizeof(vect3));
  if(NULL==vrt_ary) return md2_calculate_bumpers_1( ichr );

  // make sure that we have the correct bounds
  if(bd->cv.level < 2)
  {
    md2_calculate_bumpers_2(ichr, vrt_ary);
  }

  if(NULL == cv_tree) return bfalse;

  // transform the verts all at once, to reduce function calling overhead
  Transform3(&chrmatrix[ichr], chrvdata[ichr].Vertices, vrt_ary, vrt_count);

  pcv = &chrbmpdata[ichr].cv;

  // initialize the octree
  for(tnc=0; tnc<8; tnc++)
  {
    (*cv_tree)[tnc].level = -1;
  }

  // calculate the raw CVolumes for the octree nodes
  for(tnc=0; tnc<8; tnc++)
  {
    
    if(0 == ((tnc >> 0) & 1))
    {
      cv_node[tnc].x_min = pcv->x_min;
      cv_node[tnc].x_max = (pcv->x_min + pcv->x_max)*0.5f;
    }
    else
    {
      cv_node[tnc].x_min = (pcv->x_min + pcv->x_max)*0.5f;
      cv_node[tnc].x_max = pcv->x_max;
    };

    if(0 == ((tnc >> 1) & 1))
    {
      cv_node[tnc].y_min = pcv->y_min;
      cv_node[tnc].y_max = (pcv->y_min + pcv->y_max)*0.5f;
    }
    else
    {
      cv_node[tnc].y_min = (pcv->y_min + pcv->y_max)*0.5f;
      cv_node[tnc].y_max = pcv->y_max;
    };
    
    if(0 == ((tnc >> 2) & 1))
    {
      cv_node[tnc].z_min = pcv->z_min;
      cv_node[tnc].z_max = (pcv->z_min + pcv->z_max)*0.5f;
    }
    else
    {
      cv_node[tnc].z_min = (pcv->z_min + pcv->z_max)*0.5f;
      cv_node[tnc].z_max = pcv->z_max;
    };

    cv_node[tnc].level = 0;
  }

  tri_count = pmdl->m_numTriangles;
  for(cnt=0; cnt < tri_count; cnt++)
  {
    float tmp_x, tmp_y, tmp_z, tmp_xy, tmp_yx;
    CVolume cv_tri;
    short * tri = pmdl->m_triangles[cnt].vertexIndices;
    int ivrt = tri[0];

    // find the collision volume for this triangle
    tmp_x = vrt_ary[ivrt].x;
    cv_tri.x_min = cv_tri.x_max = tmp_x;

    tmp_y = vrt_ary[ivrt].y;
    cv_tri.y_min = cv_tri.y_max = tmp_y;

    tmp_z = vrt_ary[ivrt].z;
    cv_tri.z_min = cv_tri.z_max = tmp_z;

    tmp_xy = tmp_x + tmp_y;
    cv_tri.xy_min = cv_tri.xy_max = tmp_xy;

    tmp_yx = -tmp_x + tmp_y;
    cv_tri.yx_min = cv_tri.yx_max = tmp_yx;
    
    for(tnc=1; tnc<3; tnc++)
    {
      ivrt = tri[tnc];

      tmp_x = vrt_ary[ivrt].x;
      if(tmp_x < cv_tri.x_min) cv_tri.x_min = tmp_x - 0.001f;
      else if(tmp_x > cv_tri.x_max) cv_tri.x_max = tmp_x + 0.001f;

      tmp_y = vrt_ary[ivrt].y;
      if(tmp_y < cv_tri.y_min) cv_tri.y_min = tmp_y - 0.001f;
      else if(tmp_y > cv_tri.y_max) cv_tri.y_max = tmp_y + 0.001f;

      tmp_z = vrt_ary[ivrt].z;
      if(tmp_z < cv_tri.z_min) cv_tri.z_min = tmp_z - 0.001f;
      else if(tmp_z > cv_tri.z_max) cv_tri.z_max = tmp_z + 0.001f;

      tmp_xy = tmp_x + tmp_y;
      if(tmp_xy < cv_tri.xy_min) cv_tri.xy_min = tmp_xy - 0.001f;
      else if(tmp_xy > cv_tri.xy_max) cv_tri.xy_max = tmp_xy + 0.001f;

      tmp_yx = -tmp_x + tmp_y;
      if(tmp_yx < cv_tri.yx_min) cv_tri.yx_min = tmp_yx - 0.001f;
      else if(tmp_yx > cv_tri.yx_max) cv_tri.yx_max = tmp_yx + 0.001f;
    };

    cv_tri.level = 0;

    // add the triangle to the octree 
    for(tnc=0; tnc<8; tnc++)
    {
      if(cv_tri.x_min >= cv_node[tnc].x_max || cv_tri.x_max <= cv_node[tnc].x_min) continue;
      if(cv_tri.y_min >= cv_node[tnc].y_max || cv_tri.y_max <= cv_node[tnc].y_min) continue;
      if(cv_tri.z_min >= cv_node[tnc].z_max || cv_tri.z_max <= cv_node[tnc].z_min) continue;

      //there is an overlap with the default otree cv
      (*cv_tree)[tnc] = cvolume_merge(&(*cv_tree)[tnc], &cv_tri);
    }
    
  };


  bd->cv.level = 3;

  free( vrt_ary );

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers(CHR_REF ichr, int level)
{
  bool_t retval = bfalse;

  if(chrbmpdata[ichr].cv.level >= level) return btrue;

  switch(level)
  {    
    case 2:
      // the collision volume is an octagon, the ranges are calculated using the model's vertices
      retval = md2_calculate_bumpers_2(ichr, NULL);
      break;

    case 3:
      {
        // calculate the octree collision volume
        if(NULL == chrbmpdata[ichr].cv_tree)
        {
          chrbmpdata[ichr].cv_tree = calloc(1, sizeof(CVolume_Tree));
        };
        retval = md2_calculate_bumpers_3(ichr, chrbmpdata[ichr].cv_tree);
      };
      break;

    case 1:
      // the collision volume is a simple axis-aligned bounding box, the range is calculated from the
      // md2's bounding box
      retval = md2_calculate_bumpers_1(ichr);

    default:
    case 0:
      // make the simplest estimation of the bounding box using the data in data.txt
      retval = md2_calculate_bumpers_0(ichr);
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
void cv_list_draw()
{
  int cnt;

  for(cnt=0; cnt<cv_list_count; cnt++)
  {
    draw_CVolume( &(cv_list[cnt]) );
  };
}