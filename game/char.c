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

#include "char.h"
#include "Network.h"
#include "Client.h"
#include "Server.h"
#include "Log.h"
#include "mesh.h"
#include "input.h"
#include "camera.h"
#include "particle.h"
#include "enchant.h"
#include "passage.h"
#include "Menu.h"
#include "Md2.inl"
#include "script.h"
#include "cartman.h"

#include "egoboo_math.h"
#include "egoboo_strutil.h"
#include "egoboo_utility.h"
#include "egoboo.h"

#include <assert.h>

BUMPLIST bumplist = {0};
Uint32  cv_list_count = 0;
CVolume cv_list[1000];
SLOT    _slot;

Uint16  numdolist = 0;
Uint16  dolist[MAXCHR];

int     numfreechr = -1;         // -1 means that the characters need to initialize some data structures
Uint16  freechrlist[MAXCHR];     //

Uint16  chrcollisionlevel = 2;

int    importobject;

TILE_DAMAGE GTile_Dam;


CAP CapList[MAXCAP];
CHR ChrList[MAXCHR];

TEAM_INFO TeamList[TEAM_COUNT];



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
void flash_character_height( CHR_REF chr_ref, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high )
{
  // ZZ> This function sets a chr_ref's lighting depending on vertex height...
  //     Can make feet dark and head light...
  int cnt;
  Uint16 model;
  float z, flip;

  Uint32 ilast, inext;
  MD2_Model * pmdl;
  MD2_Frame * plast, * pnext;

  model = ChrList[chr_ref].model;
  inext = ChrList[chr_ref].anim.next;
  ilast = ChrList[chr_ref].anim.last;
  flip = ChrList[chr_ref].anim.flip;

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  pmdl  = MadList[model].md2_ptr;
  plast = md2_get_Frame(pmdl, ilast);
  pnext = md2_get_Frame(pmdl, inext);

  for ( cnt = 0; cnt < MadList[model].transvertices; cnt++ )
  {
    z = pnext->vertices[cnt].z + (pnext->vertices[cnt].z - plast->vertices[cnt].z) * flip;

    if ( z < low )
    {
      ChrList[chr_ref].vrta_fp8[cnt].r =
      ChrList[chr_ref].vrta_fp8[cnt].g =
      ChrList[chr_ref].vrta_fp8[cnt].b = valuelow;
    }
    else if ( z > high )
    {
      ChrList[chr_ref].vrta_fp8[cnt].r =
      ChrList[chr_ref].vrta_fp8[cnt].g =
      ChrList[chr_ref].vrta_fp8[cnt].b = valuehigh;
    }
    else
    {
      float ftmp = (float)( z - low ) / (float)( high - low );
      ChrList[chr_ref].vrta_fp8[cnt].r =
      ChrList[chr_ref].vrta_fp8[cnt].g =
      ChrList[chr_ref].vrta_fp8[cnt].b = valuelow + (valuehigh - valuelow) * ftmp;
    }
  }
}

//--------------------------------------------------------------------------------------------
void flash_character( CHR_REF chr_ref, Uint8 value )
{
  // ZZ> This function sets a chr_ref's lighting
  int cnt;
  Uint16 model = ChrList[chr_ref].model;

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  cnt = 0;
  while ( cnt < MadList[model].transvertices )
  {
    ChrList[chr_ref].vrta_fp8[cnt].r =
    ChrList[chr_ref].vrta_fp8[cnt].g =
    ChrList[chr_ref].vrta_fp8[cnt].b = value;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void add_to_dolist( CHR_REF chr_ref )
{
  // This function puts a character in the list
  int fan;

  if ( !VALID_CHR( chr_ref ) || ChrList[chr_ref].indolist ) return;

  fan = ChrList[chr_ref].onwhichfan;
  //if ( mesh_in_renderlist( fan ) )
  {
    //ChrList[chr_ref].lightspek_fp8 = Mesh[meshvrtstart[fan]].vrtl_fp8;
    dolist[numdolist] = chr_ref;
    ChrList[chr_ref].indolist = btrue;
    numdolist++;


    // Do flashing
    if (( allframe & ChrList[chr_ref].flashand ) == 0 && ChrList[chr_ref].flashand != DONTFLASH )
    {
      flash_character( chr_ref, 255 );
    }
    // Do blacking
    if (( allframe&SEEKURSEAND ) == 0 && localseekurse && ChrList[chr_ref].iskursed )
    {
      flash_character( chr_ref, 0 );
    }
  }
  //else
  //{
  //  Uint16 model = ChrList[chr_ref].model;
  //  assert( MAXMODEL != VALIDATE_MDL( model ) );

  //  // Double check for large/special objects
  //  if ( CapList[model].alwaysdraw )
  //  {
  //    dolist[numdolist] = chr_ref;
  //    ChrList[chr_ref].indolist = btrue;
  //    numdolist++;
  //  }
  //}

  // Add its weapons too
  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    add_to_dolist( chr_get_holdingwhich( chr_ref, _slot ) );
  };

}

//--------------------------------------------------------------------------------------------
void order_dolist( void )
{
  // ZZ> This function GOrder.s the dolist based on distance from camera,
  //     which is needed for reflections to properly clip themselves.
  //     Order from closest to farthest

  CHR_REF chr_ref, olddolist[MAXCHR];
  int cnt, tnc, order;
  int dist[MAXCHR];

  // Figure the distance of each
  cnt = 0;
  while ( cnt < numdolist )
  {
    chr_ref = dolist[cnt];  olddolist[cnt] = chr_ref;
    if ( ChrList[chr_ref].light_fp8 != 255 || ChrList[chr_ref].alpha_fp8 != 255 )
    {
      // This makes stuff inside an invisible chr_ref visible...
      // A key inside a Jellcube, for example
      dist[cnt] = 0x7fffffff;
    }
    else
    {
      dist[cnt] = ABS( ChrList[chr_ref].pos.x - GCamera.pos.x ) + ABS( ChrList[chr_ref].pos.y - GCamera.pos.y );
    }
    cnt++;
  }


  // Put em in the right order
  cnt = 0;
  while ( cnt < numdolist )
  {
    chr_ref = olddolist[cnt];
    order = 0;  // Assume this chr_ref is closest
    tnc = 0;
    while ( tnc < numdolist )
    {
      // For each one closer, increment the order
      order += ( dist[cnt] > dist[tnc] );
      order += ( dist[cnt] == dist[tnc] ) && ( cnt < tnc );
      tnc++;
    }
    dolist[order] = chr_ref;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_dolist( void )
{
  // ZZ> This function finds the characters that need to be drawn and puts them in the list
  int cnt;
  CHR_REF chr_ref;


  // Remove everyone from the dolist
  cnt = 0;
  while ( cnt < numdolist )
  {
    chr_ref = dolist[cnt];
    ChrList[chr_ref].indolist = bfalse;
    cnt++;
  }
  numdolist = 0;


  // Now fill it up again
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( ChrList[cnt].on && !chr_in_pack( cnt ) )
    {
      // Add the chr_ref
      add_to_dolist( cnt );
    }
    cnt++;
  }

}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
  // ZZ> This function keeps weapons near their holders
  int cnt;
  CHR_REF chr_ref;

  // !!!BAD!!!  May need to do 3 levels of attachment...

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    if ( !VALID_CHR( cnt ) ) continue;

    chr_ref = chr_get_attachedto( cnt );
    if ( !VALID_CHR( chr_ref ) )
    {
      // Keep inventory with character
      if ( !chr_in_pack( cnt ) )
      {
        chr_ref = chr_get_nextinpack( cnt );
        while ( VALID_CHR( chr_ref ) )
        {
          ChrList[chr_ref].pos = ChrList[cnt].pos;
          ChrList[chr_ref].pos_old = ChrList[cnt].pos_old;  // Copy olds to make SendMessageNear work
          chr_ref  = chr_get_nextinpack( chr_ref );
        }
      }
    }
    else
    {
      // Keep in hand weapons with character
      if ( ChrList[chr_ref].matrixvalid && ChrList[cnt].matrixvalid )
      {
        ChrList[cnt].pos.x = ChrList[cnt].matrix.CNV( 3, 0 );
        ChrList[cnt].pos.y = ChrList[cnt].matrix.CNV( 3, 1 );
        ChrList[cnt].pos.z = ChrList[cnt].matrix.CNV( 3, 2 );
      }
      else
      {
        ChrList[cnt].pos.x = ChrList[chr_ref].pos.x;
        ChrList[cnt].pos.y = ChrList[chr_ref].pos.y;
        ChrList[cnt].pos.z = ChrList[chr_ref].pos.z;
      }
      ChrList[cnt].turn_lr = ChrList[chr_ref].turn_lr;

      // Copy this stuff ONLY if it's a weapon, not for mounts
      if ( ChrList[chr_ref].transferblend && ChrList[cnt].isitem )
      {
        if ( ChrList[chr_ref].alpha_fp8 != 255 )
        {
          Uint16 model = ChrList[cnt].model;
          assert( MAXMODEL != VALIDATE_MDL( model ) );
          ChrList[cnt].alpha_fp8 = ChrList[chr_ref].alpha_fp8;
          ChrList[cnt].bumpstrength = CapList[model].bumpstrength * FP8_TO_FLOAT( ChrList[cnt].alpha_fp8 );
        }
        if ( ChrList[chr_ref].light_fp8 != 255 )
        {
          ChrList[cnt].light_fp8 = ChrList[chr_ref].light_fp8;
        }
      }
    }
  }

}


//--------------------------------------------------------------------------------------------
bool_t make_one_character_matrix( CHR_REF chr_ref )
{
  // ZZ> This function sets one character's matrix

  CHR * pchr, * povl;

  Uint16 tnc;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  if ( !VALID_CHR( chr_ref ) ) return bfalse;
  pchr = ChrList + chr_ref;

  mat_old = pchr->matrix;
  pchr->matrixvalid = bfalse;

  if ( pchr->overlay )
  {
    // Overlays are kept with their target...
    tnc = chr_get_aitarget( chr_ref );

    if ( VALID_CHR( tnc ) )
    {
      povl = ChrList + tnc;

      pchr->pos.x = povl->matrix.CNV( 3, 0 );
      pchr->pos.y = povl->matrix.CNV( 3, 1 );
      pchr->pos.z = povl->matrix.CNV( 3, 2 );

      pchr->matrix = povl->matrix;

      pchr->matrix.CNV( 0, 0 ) *= pchr->pancakepos.x;
      pchr->matrix.CNV( 1, 0 ) *= pchr->pancakepos.x;
      pchr->matrix.CNV( 2, 0 ) *= pchr->pancakepos.x;

      pchr->matrix.CNV( 0, 1 ) *= pchr->pancakepos.y;
      pchr->matrix.CNV( 1, 1 ) *= pchr->pancakepos.y;
      pchr->matrix.CNV( 2, 1 ) *= pchr->pancakepos.y;

      pchr->matrix.CNV( 0, 2 ) *= pchr->pancakepos.z;
      pchr->matrix.CNV( 1, 2 ) *= pchr->pancakepos.z;
      pchr->matrix.CNV( 2, 2 ) *= pchr->pancakepos.z;

      pchr->matrixvalid = btrue;

      recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
    }
  }
  else
  {
    pchr->matrix = ScaleXYZRotateXYZTranslate( pchr->scale * pchr->pancakepos.x, pchr->scale * pchr->pancakepos.y, pchr->scale * pchr->pancakepos.z,
                     pchr->turn_lr >> 2,
                     (( Uint16 )( pchr->mapturn_ud + 32768 ) ) >> 2,
                     (( Uint16 )( pchr->mapturn_lr + 32768 ) ) >> 2,
                     pchr->pos.x, pchr->pos.y, pchr->pos.z );

    pchr->matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(pchr->matrix));
  }

  //if(pchr->matrixvalid && recalc_bumper)
  //{
  //  // invalidate the cached bumper data
  //  pchr->bmpdata.cv.level = -1;
  //};

  if(pchr->matrixvalid && recalc_bumper)
  {
    // invalidate the cached bumper data
    pchr->bmpdata.cv.level = -1;
    md2_calculate_bumpers(chr_ref, 0);
  };

  return pchr->matrixvalid;
}

//--------------------------------------------------------------------------------------------
void free_one_character( CHR_REF chr_ref )
{
  // ZZ> This function sticks a chr_ref back on the free chr_ref stack
  int cnt;

  if ( !VALID_CHR( chr_ref ) ) return;

  fprintf( stdout, "free_one_character() - \n\tprofile == %d, CapList[profile].classname == \"%s\", index == %d\n", ChrList[chr_ref].model, CapList[ChrList[chr_ref].model].classname, chr_ref );

  //remove any collision volume octree
  if(NULL != ChrList[chr_ref].bmpdata.cv_tree)
  {
    FREE( ChrList[chr_ref].bmpdata.cv_tree );
  }

  // add it to the free list
  freechrlist[numfreechr] = chr_ref;
  numfreechr++;

  // Remove from stat list
  if ( ChrList[chr_ref].staton )
  {
    ChrList[chr_ref].staton = bfalse;
    cnt = 0;
    while ( cnt < numstat )
    {
      if ( statlist[cnt] == chr_ref )
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
  assert( MAXMODEL != VALIDATE_MDL( ChrList[chr_ref].model ) );
  if ( ChrList[chr_ref].alive && !CapList[ChrList[chr_ref].model].invictus )
  {
    TeamList[ChrList[chr_ref].baseteam].morale--;
  }

  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( ChrList[cnt].on )
    {
      if ( ChrList[cnt].aistate.target == chr_ref )
      {
        ChrList[cnt].aistate.alert |= ALERT_TARGETKILLED;
        ChrList[cnt].aistate.target = cnt;
      }
      if ( team_get_leader( ChrList[cnt].team ) == chr_ref )
      {
        ChrList[cnt].aistate.alert |= ALERT_LEADERKILLED;
      }
    }
    cnt++;
  }

  if ( team_get_leader( ChrList[chr_ref].team ) == chr_ref )
  {
    TeamList[ChrList[chr_ref].team].leader = MAXCHR;
  }

  if( INVALID_CHANNEL != ChrList[chr_ref].loopingchannel )
  {
    stop_sound( ChrList[chr_ref].loopingchannel );
    ChrList[chr_ref].loopingchannel = INVALID_CHANNEL;
  };

  ChrList[chr_ref].on = bfalse;
  ChrList[chr_ref].alive = bfalse;
  ChrList[chr_ref].inwhichpack = MAXCHR;
  VData_Blended_Deallocate(&(ChrList[chr_ref].vdata));
}

//--------------------------------------------------------------------------------------------
void free_inventory( CHR_REF chr_ref )
{
  // ZZ> This function frees every item in the chr_ref's inventory
  int cnt, next;

  cnt  = chr_get_nextinpack( chr_ref );
  while ( cnt < MAXCHR )
  {
    next  = chr_get_nextinpack( cnt );
    ChrList[cnt].freeme = btrue;
    cnt = next;
  }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character( PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset )
{
  // ZZ> This function sets one particle's position to be attached to a chr_ref.
  //     It will kill the particle if the chr_ref is no longer around

  Uint16 vertex, model;
  float flip;
  GLvector point, nupoint;
  PRT * pprt;
  CHR * pchr;

  // Check validity of attachment
  if ( !VALID_CHR( chr_ref ) || chr_in_pack( chr_ref ) || !VALID_PRT( particle ) )
  {
    PrtList[particle].gopoof = btrue;
    return;
  }

  pprt = PrtList + particle;
  pchr = ChrList + chr_ref;

  // Do we have a matrix???
  if ( !pchr->matrixvalid )
  {
    // No matrix, so just wing it...

    pprt->pos.x = pchr->pos.x;
    pprt->pos.y = pchr->pos.y;
    pprt->pos.z = pchr->pos.z;
  }
  else   if ( vertoffset == GRIP_ORIGIN )
  {
    // Transform the origin to world space

    pprt->pos.x = pchr->matrix.CNV( 3, 0 );
    pprt->pos.y = pchr->matrix.CNV( 3, 1 );
    pprt->pos.z = pchr->matrix.CNV( 3, 2 );
  }
  else
  {
    // Transform the grip vertex position to world space

    Uint32      ilast, inext;
    MAD       * pmad;
    MD2_Model * pmdl;
    MD2_Frame * plast, * pnext;

    model = pchr->model;
    inext = pchr->anim.next;
    ilast = pchr->anim.last;
    flip  = pchr->anim.flip;

    assert( MAXMODEL != VALIDATE_MDL( model ) );

    pmad = MadList + model;
    pmdl  = pmad->md2_ptr;
    plast = md2_get_Frame(pmdl, ilast);
    pnext = md2_get_Frame(pmdl, inext);

    //handle possible invalid values
    vertex = pmad->vertices - vertoffset;
    if(vertoffset >= pmad->vertices)
    {
      vertex = pmad->vertices - GRIP_LAST;
    }

    // Calculate grip point locations with linear interpolation and other silly things
    if ( inext == ilast )
    {
      point.x = plast->vertices[vertex].x;
      point.y = plast->vertices[vertex].y;
      point.z = plast->vertices[vertex].z;
      point.w = 1.0f;
    }
    else
    {
      point.x = plast->vertices[vertex].x + ( pnext->vertices[vertex].x - plast->vertices[vertex].x ) * flip;
      point.y = plast->vertices[vertex].y + ( pnext->vertices[vertex].y - plast->vertices[vertex].y ) * flip;
      point.z = plast->vertices[vertex].z + ( pnext->vertices[vertex].z - plast->vertices[vertex].z ) * flip;
      point.w = 1.0f;
    }

    // Do the transform
    Transform4_Full( 1.0f, 1.0f, &(pchr->matrix), &point, &nupoint, 1 );

    pprt->pos.x = nupoint.x;
    pprt->pos.y = nupoint.y;
    pprt->pos.z = nupoint.z;
  }



}

//--------------------------------------------------------------------------------------------
bool_t make_one_weapon_matrix( CHR_REF chr_ref )
{
  // ZZ> This function sets one weapon's matrix, based on who it's attached to

  int cnt;
  CHR_REF mount_ref;
  Uint16 vertex;
  matrix_4x4 mat_old;
  bool_t recalc_bumper = bfalse;

  // check this character
  if ( !VALID_CHR( chr_ref ) )  return bfalse;

  // invalidate the matrix
  ChrList[chr_ref].matrixvalid = bfalse;

  // check that the mount is valid
  mount_ref = chr_get_attachedto( chr_ref );
  if ( !VALID_CHR( mount_ref ) )
  {
    ChrList[chr_ref].matrix = ZeroMatrix();
    return bfalse;
  }

  mat_old = ChrList[chr_ref].matrix;

  if(0xFFFF == ChrList[chr_ref].attachedgrip[0])
  {
    // Calculate weapon's matrix
    ChrList[chr_ref].matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, ChrList[chr_ref].turn_lr + ChrList[mount_ref].turn_lr, ChrList[mount_ref].pos.x, ChrList[mount_ref].pos.y, ChrList[mount_ref].pos.z);
    ChrList[chr_ref].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[chr_ref].matrix));
  }
  else if(0xFFFF == ChrList[chr_ref].attachedgrip[1])
  {
    // do the linear interpolation
    vertex = ChrList[chr_ref].attachedgrip[0];
    md2_blend_vertices(mount_ref, vertex, vertex);

    // Calculate weapon's matrix
    ChrList[chr_ref].matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, ChrList[chr_ref].turn_lr + ChrList[mount_ref].turn_lr, ChrList[mount_ref].vdata.Vertices[vertex].x, ChrList[mount_ref].vdata.Vertices[vertex].y, ChrList[mount_ref].vdata.Vertices[vertex].z);
    ChrList[chr_ref].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[chr_ref].matrix));
  }
  else
  {
    GLvector point[GRIP_SIZE], nupoint[GRIP_SIZE];

    // do the linear interpolation
    vertex = ChrList[chr_ref].attachedgrip[0];
    md2_blend_vertices(mount_ref, vertex, vertex+GRIP_SIZE);

    for ( cnt = 0; cnt < GRIP_SIZE; cnt++ )
    {
      point[cnt].x = ChrList[mount_ref].vdata.Vertices[vertex+cnt].x;
      point[cnt].y = ChrList[mount_ref].vdata.Vertices[vertex+cnt].y;
      point[cnt].z = ChrList[mount_ref].vdata.Vertices[vertex+cnt].z;
      point[cnt].w = 1.0f;
    };

    // Do the transform
    Transform4_Full( 1.0f, 1.0f, &(ChrList[mount_ref].matrix), point, nupoint, GRIP_SIZE );

    // Calculate weapon's matrix based on positions of grip points
    // chrscale is recomputed at time of attachment
    ChrList[chr_ref].matrix = FourPoints( nupoint[0], nupoint[1], nupoint[2], nupoint[3], 1.0 );
    ChrList[chr_ref].pos.x = (ChrList[chr_ref].matrix).CNV(3,0);
    ChrList[chr_ref].pos.y = (ChrList[chr_ref].matrix).CNV(3,1);
    ChrList[chr_ref].pos.z = (ChrList[chr_ref].matrix).CNV(3,2);
    ChrList[chr_ref].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[chr_ref].matrix));
  };

  if(ChrList[chr_ref].matrixvalid && recalc_bumper)
  {
    // invalidate the cached bumper data
    md2_calculate_bumpers(chr_ref, 0);
  };

  return ChrList[chr_ref].matrixvalid;
}

//--------------------------------------------------------------------------------------------
void make_character_matrices()
{
  // ZZ> This function makes all of the character's matrices
  CHR_REF chr_ref;
  bool_t  bfinished;

  // Forget about old matrices
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    ChrList[chr_ref].matrixvalid = bfalse;
  }

  // Do base characters
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    CHR_REF attached_ref;
    if ( !VALID_CHR( chr_ref ) ) continue;

    attached_ref = chr_get_attachedto( chr_ref );
    if ( VALID_CHR( attached_ref ) ) continue;  // Skip weapons for now

    make_one_character_matrix( chr_ref );
  }


  // Do all other levels of attachment
  bfinished = bfalse;
  while ( !bfinished )
  {
    bfinished = btrue;
    for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
    {
      CHR_REF attached_ref;
      if ( ChrList[chr_ref].matrixvalid || !VALID_CHR( chr_ref ) ) continue;

      attached_ref = chr_get_attachedto( chr_ref );
      if ( !VALID_CHR( attached_ref ) ) continue;

      if ( !ChrList[attached_ref].matrixvalid )
      {
        bfinished = bfalse;
        continue;
      }

      make_one_weapon_matrix( chr_ref );
    }
  };

}

//--------------------------------------------------------------------------------------------
int get_free_character()
{
  // ZZ> This function gets an unused chr_ref and returns its index
  CHR_REF chr_ref;


  if ( numfreechr == 0 )
  {
    // Return MAXCHR if we can't find one
    return MAXCHR;
  }
  else
  {
    // Just grab the next one
    numfreechr--;
    chr_ref = freechrlist[numfreechr];
  }
  return chr_ref;
}

//--------------------------------------------------------------------------------------------
bool_t prt_search_block( SEARCH_CONTEXT * psearch, int block_x, int block_y, PRT_REF prt_ref, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, TEAM team,
                         Uint16 donttarget, Uint16 oldtarget )
{
  // ZZ> This function helps find a target, returning btrue if it found a decent target

  int cnt;
  Uint16 local_angle;
  CHR_REF chrb_ref;
  bool_t bfound, request_enemies = !request_friends;
  Uint32 fanblock;
  int local_distance;

  if( !VALID_PRT(prt_ref) ) return bfalse;

  bfound = bfalse;

  // Current fanblock
  fanblock = mesh_convert_block( block_x, block_y );
  if ( INVALID_FAN == fanblock ) return bfound;

  for ( cnt = 0, chrb_ref = VALID_CHR( bumplist.chr[fanblock] ); cnt < bumplist.num_chr[fanblock] && VALID_CHR( chrb_ref ); cnt++, chrb_ref = chr_get_bumpnext( chrb_ref ) )
  {
    // don't find stupid stuff
    if ( !VALID_CHR( chrb_ref ) || 0.0f == ChrList[chrb_ref].bumpstrength ) continue;

    if ( !ChrList[chrb_ref].alive || ChrList[chrb_ref].invictus || chr_in_pack( chrb_ref ) ) continue;

    if ( chrb_ref == donttarget || chrb_ref == oldtarget ) continue;

    if ( allow_anyone || ( request_friends && !TeamList[team].hatesteam[ChrList[chrb_ref].team] ) || ( request_enemies && TeamList[team].hatesteam[ChrList[chrb_ref].team] ) )
    {
      local_distance = ABS( ChrList[chrb_ref].pos.x - PrtList[prt_ref].pos.x ) + ABS( ChrList[chrb_ref].pos.y - PrtList[prt_ref].pos.y );
      if ( local_distance < psearch->bestdistance )
      {
        local_angle = facing - vec_to_turn( ChrList[chrb_ref].pos.x - PrtList[prt_ref].pos.x, ChrList[chrb_ref].pos.y - PrtList[prt_ref].pos.y );
        if ( local_angle < psearch->bestangle || local_angle > ( 65535 - psearch->bestangle ) )
        {
          bfound = btrue;
          psearch->besttarget   = chrb_ref;
          psearch->bestdistance = local_distance;
          psearch->useangle     = local_angle;
          if ( local_angle  > 32767 )
            psearch->bestangle = UINT16_SIZE - local_angle;
          else
            psearch->bestangle = local_angle;
        }
      }
    }
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
void free_all_characters()
{
  // ZZ> This function resets the character allocation list
  CHR_REF   chr_ref;
  CHR     * pchr;  
  bool_t    do_initialization;

  do_initialization = (-1 == numfreechr);

  nolocalplayers = btrue;
  numfreechr = 0;
  while ( numfreechr < MAXCHR )
  {
    pchr = ChrList + numfreechr;

    if(do_initialization)
    {
      // initialize a non-existant collision volume octree
      pchr->bmpdata.cv_tree = NULL;

      // initialize the looping sounds
      pchr->loopingchannel = INVALID_CHANNEL;
    }
    else if(NULL != pchr->bmpdata.cv_tree)
    {
      // remove existing collision volume octree
      FREE( pchr->bmpdata.cv_tree );

      // silence all looping sounds
      if( INVALID_CHANNEL != pchr->loopingchannel )
      {
        stop_sound( pchr->loopingchannel );
        pchr->loopingchannel = INVALID_CHANNEL;
      };
    }

    // reset some values
    pchr->on = bfalse;
    pchr->alive = bfalse;
    pchr->staton = bfalse;
    pchr->matrixvalid = bfalse;
    pchr->model = MAXMODEL;
    VData_Blended_Deallocate(&(pchr->vdata));
    pchr->name[0] = '\0';

    // invalidate pack
    pchr->numinpack = 0;
    pchr->inwhichpack = MAXCHR;
    pchr->nextinpack = MAXCHR;

    // invalidate attachmants
    pchr->inwhichslot = SLOT_NONE;
    pchr->attachedto = MAXCHR;
    for ( chr_ref = 0; chr_ref < SLOT_COUNT; chr_ref++ )
    {
      pchr->holdingwhich[chr_ref] = MAXCHR;
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
Uint32 __chrhitawall( CHR_REF chr_ref, vect3 * norm )
{
  // ZZ> This function returns nonzero if the character hit a wall that the
  //     chr_ref is not allowed to cross

  Uint32 retval;
  vect3  pos, size;

  if ( !VALID_CHR( chr_ref ) || 0.0f == ChrList[chr_ref].bumpstrength ) return 0;

  pos.x = ( ChrList[chr_ref].bmpdata.cv.x_max + ChrList[chr_ref].bmpdata.cv.x_min ) * 0.5f;
  pos.y = ( ChrList[chr_ref].bmpdata.cv.y_max + ChrList[chr_ref].bmpdata.cv.y_min ) * 0.5f;
  pos.z =   ChrList[chr_ref].bmpdata.cv.z_min;

  size.x = ( ChrList[chr_ref].bmpdata.cv.x_max - ChrList[chr_ref].bmpdata.cv.x_min ) * 0.5f;
  size.y = ( ChrList[chr_ref].bmpdata.cv.y_max - ChrList[chr_ref].bmpdata.cv.y_min ) * 0.5f;
  size.z = ( ChrList[chr_ref].bmpdata.cv.z_max - ChrList[chr_ref].bmpdata.cv.z_min ) * 0.5f;

  retval = mesh_hitawall( pos, size.x, size.y, ChrList[chr_ref].stoppedby );

  if( 0!=retval && NULL!=norm )
  {
    vect3 pos2;

    VectorClear( norm->v );

    pos2.x = pos.x + ChrList[chr_ref].pos.x - ChrList[chr_ref].pos_old.x;
    pos2.y = pos.y + ChrList[chr_ref].pos.y - ChrList[chr_ref].pos_old.y;
    pos2.z = pos.z + ChrList[chr_ref].pos.z - ChrList[chr_ref].pos_old.z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, ChrList[chr_ref].stoppedby ) )
    {
      return 0;
    }

    pos2.x = pos.x;
    pos2.y = pos.y + ChrList[chr_ref].pos.y - ChrList[chr_ref].pos_old.y;
    pos2.z = pos.z + ChrList[chr_ref].pos.z - ChrList[chr_ref].pos_old.z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, ChrList[chr_ref].stoppedby ) )
    {
      norm->x = -SGN(ChrList[chr_ref].pos.x - ChrList[chr_ref].pos_old.x);
    }

    pos2.x = pos.x + ChrList[chr_ref].pos.x - ChrList[chr_ref].pos_old.x;
    pos2.y = pos.y;
    pos2.z = pos.z + ChrList[chr_ref].pos.z - ChrList[chr_ref].pos_old.z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, ChrList[chr_ref].stoppedby ) )
    {
      norm->y = -SGN(ChrList[chr_ref].pos.y - ChrList[chr_ref].pos_old.y);
    }

    pos2.x = pos.x + ChrList[chr_ref].pos.x - ChrList[chr_ref].pos_old.x;
    pos2.y = pos.y + ChrList[chr_ref].pos.y - ChrList[chr_ref].pos_old.y;
    pos2.z = pos.z;

    if( 0 != mesh_hitawall( pos, size.x, size.y, ChrList[chr_ref].stoppedby ) )
    {
      norm->z = -SGN(ChrList[chr_ref].pos.z - ChrList[chr_ref].pos_old.z);
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
void reset_character_accel( CHR_REF chr_ref )
{
  // ZZ> This function fixes a chr_ref's MAX acceleration
  Uint16 enchant;

  if ( !VALID_CHR( chr_ref ) ) return;

  // Okay, remove all acceleration enchants
  enchant = ChrList[chr_ref].firstenchant;
  while ( enchant < MAXENCHANT )
  {
    remove_enchant_value( enchant, ADDACCEL );
    enchant = EncList[enchant].nextenchant;
  }

  // Set the starting value
  assert( MAXMODEL != VALIDATE_MDL( ChrList[chr_ref].model ) );
  ChrList[chr_ref].maxaccel = CapList[ChrList[chr_ref].model].maxaccel[( ChrList[chr_ref].texture - MadList[ChrList[chr_ref].model].skinstart ) % MAXSKIN];

  // Put the acceleration enchants back on
  enchant = ChrList[chr_ref].firstenchant;
  while ( enchant < MAXENCHANT )
  {
    add_enchant_value( enchant, ADDACCEL, EncList[enchant].eve );
    enchant = EncList[enchant].nextenchant;
  }

}

//--------------------------------------------------------------------------------------------
bool_t detach_character_from_mount( CHR_REF chr_ref, bool_t ignorekurse, bool_t doshop )
{
  // ZZ> This function drops an item
  Uint16 imount, iowner = MAXCHR;
  Uint16 enchant, passage;
  Uint16 cnt, price;
  bool_t inshop;



  // Make sure the chr_ref is valid
  if ( !VALID_CHR( chr_ref ) )
    return bfalse;


  // Make sure the chr_ref is mounted
  imount = chr_get_attachedto( chr_ref );
  if ( !VALID_CHR( imount ) )
    return bfalse;


  // Don't allow living characters to drop kursed weapons
  if ( !ignorekurse && ChrList[chr_ref].iskursed && ChrList[imount].alive )
  {
    ChrList[chr_ref].aistate.alert |= ALERT_NOTDROPPED;
    return bfalse;
  }


  // Rip 'em apart
  _slot = ChrList[chr_ref].inwhichslot;
  if(_slot == SLOT_INVENTORY)
  {
    ChrList[chr_ref].attachedto = MAXCHR;
    ChrList[chr_ref].inwhichslot = SLOT_NONE;
  }
  else
  {
    assert(_slot != SLOT_NONE);
    assert(chr_ref == ChrList[imount].holdingwhich[_slot]);
    ChrList[chr_ref].attachedto = MAXCHR;
    ChrList[chr_ref].inwhichslot = SLOT_NONE;
    ChrList[imount].holdingwhich[_slot] = MAXCHR;
  }


  ChrList[chr_ref].scale = ChrList[chr_ref].fat; // * MadList[ChrList[chr_ref].model].scale * 4;


  // Run the falling animation...
  play_action( chr_ref, ACTION_JB + ( ChrList[chr_ref].inwhichslot % 2 ), bfalse );

  // Set the positions
  if ( ChrList[chr_ref].matrixvalid )
  {
    ChrList[chr_ref].pos.x = ChrList[chr_ref].matrix.CNV( 3, 0 );
    ChrList[chr_ref].pos.y = ChrList[chr_ref].matrix.CNV( 3, 1 );
    ChrList[chr_ref].pos.z = ChrList[chr_ref].matrix.CNV( 3, 2 );
  }
  else
  {
    ChrList[chr_ref].pos.x = ChrList[imount].pos.x;
    ChrList[chr_ref].pos.y = ChrList[imount].pos.y;
    ChrList[chr_ref].pos.z = ChrList[imount].pos.z;
  }



  // Make sure it's not dropped in a wall...
  if ( 0 != __chrhitawall( chr_ref, NULL ) )
  {
    ChrList[chr_ref].pos.x = ChrList[imount].pos.x;
    ChrList[chr_ref].pos.y = ChrList[imount].pos.y;
  }


  // Check for shop passages
  inshop = bfalse;
  if ( ChrList[chr_ref].isitem && numshoppassage != 0 && doshop )
  {
    for ( cnt = 0; cnt < numshoppassage; cnt++ )
    {
      passage = shoppassage[cnt];

      if ( passage_check_any( chr_ref, passage, NULL ) )
      {
        iowner = shopowner[passage];
        inshop = ( NOOWNER != iowner );
        break;
      }
    }

    if ( doshop && inshop )
    {
      Uint16 model = ChrList[chr_ref].model;

      assert( MAXMODEL != VALIDATE_MDL( model ) );

      // Give the imount its money back, alert the shop iowner
      price = CapList[model].skincost[( ChrList[chr_ref].texture - MadList[model].skinstart ) % MAXSKIN];
      if ( CapList[model].isstackable )
      {
        price *= ChrList[chr_ref].ammo;
      }
      ChrList[imount].money += price;
      ChrList[iowner].money -= price;
      if ( ChrList[iowner].money < 0 )  ChrList[iowner].money = 0;
      if ( ChrList[imount].money > MAXMONEY )  ChrList[imount].money = MAXMONEY;

      ChrList[iowner].aistate.alert |= ALERT_SIGNALED;
      ChrList[iowner].message = price;  // Tell iowner how much...
      ChrList[iowner].messagedata = 0;  // 0 for buying an item
    }
  }

  // Make sure it works right
  ChrList[chr_ref].hitready = btrue;
  ChrList[chr_ref].aistate.alert   |= ALERT_DROPPED;
  if ( inshop )
  {
    // Drop straight down to avoid theft
    ChrList[chr_ref].vel.x = 0;
    ChrList[chr_ref].vel.y = 0;
  }
  else
  {
    Uint16 sin_dir = RANDIE;
    ChrList[chr_ref].accum_vel.x += ChrList[imount].vel.x + 0.5 * DROPXYVEL * turntosin[(( sin_dir>>2 ) + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
    ChrList[chr_ref].accum_vel.y += ChrList[imount].vel.y + 0.5 * DROPXYVEL * turntosin[sin_dir>>2];
  }
  ChrList[chr_ref].accum_vel.z += DROPZVEL;


  // Turn looping off
  ChrList[chr_ref].action.loop = bfalse;


  // Reset the team if it is a imount
  if ( ChrList[imount].ismount )
  {
    ChrList[imount].team = ChrList[imount].baseteam;
    ChrList[imount].aistate.alert |= ALERT_DROPPED;
  }
  ChrList[chr_ref].team = ChrList[chr_ref].baseteam;
  ChrList[chr_ref].aistate.alert |= ALERT_DROPPED;


  // Reset transparency
  if ( ChrList[chr_ref].isitem && ChrList[imount].transferblend )
  {
    Uint16 model = ChrList[chr_ref].model;

    assert( MAXMODEL != VALIDATE_MDL( model ) );

    // Okay, reset transparency
    enchant = ChrList[chr_ref].firstenchant;
    while ( enchant < MAXENCHANT )
    {
      unset_enchant_value( enchant, SETALPHABLEND );
      unset_enchant_value( enchant, SETLIGHTBLEND );
      enchant = EncList[enchant].nextenchant;
    }

    ChrList[chr_ref].alpha_fp8 = CapList[model].alpha_fp8;
    ChrList[chr_ref].bumpstrength = CapList[model].bumpstrength * FP8_TO_FLOAT( ChrList[chr_ref].alpha_fp8 );
    ChrList[chr_ref].light_fp8 = CapList[model].light_fp8;
    enchant = ChrList[chr_ref].firstenchant;
    while ( enchant < MAXENCHANT )
    {
      set_enchant_value( enchant, SETALPHABLEND, EncList[enchant].eve );
      set_enchant_value( enchant, SETLIGHTBLEND, EncList[enchant].eve );
      enchant = EncList[enchant].nextenchant;
    }
  }

  // Set twist
  ChrList[chr_ref].mapturn_lr = 32768;
  ChrList[chr_ref].mapturn_ud = 32768;

  if ( ChrList[chr_ref].isplayer )
    debug_message( 1, "dismounted %s(%s) from (%s)", ChrList[chr_ref].name, CapList[ChrList[chr_ref].model].classname, CapList[ChrList[imount].model].classname );


  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_character_to_mount( CHR_REF chr_ref, CHR_REF mount_ref, SLOT slot )
{
  // ZZ> This function attaches one chr_ref to another ( the mount_ref )
  //     at either the left or right grip
  int tnc;

  // Make sure both are still around
  if ( !VALID_CHR( chr_ref ) || !VALID_CHR( mount_ref ) )
    return bfalse;

  // the item may hit the floor if this fails
  ChrList[chr_ref].hitready = bfalse;

  //make sure you're not trying to mount yourself!
  if ( chr_ref == mount_ref )
    return bfalse;

  // make sure that neither is in someone's pack
  if ( chr_in_pack( chr_ref ) || chr_in_pack( mount_ref ) )
    return bfalse;

  // Make sure the the slot is valid
  assert( MAXMODEL != VALIDATE_MDL( ChrList[mount_ref].model ) );
  if ( SLOT_NONE == slot || !CapList[ChrList[mount_ref].model].slotvalid[slot] )
    return bfalse;

  // Put 'em together
  assert(slot != SLOT_NONE);
  ChrList[chr_ref].inwhichslot = slot;
  ChrList[chr_ref].attachedto  = mount_ref;
  ChrList[mount_ref].holdingwhich[slot] = chr_ref;

  // handle the vertices
  {
    Uint16 model = ChrList[mount_ref].model;
    Uint16 vrtoffset = slot_to_offset( slot );

    assert( MAXMODEL != VALIDATE_MDL( model ) );
    if ( MadList[model].vertices > vrtoffset && vrtoffset > 0 )
    {
      tnc = MadList[model].vertices - vrtoffset;
      ChrList[chr_ref].attachedgrip[0] = tnc;
      ChrList[chr_ref].attachedgrip[1] = tnc + 1;
      ChrList[chr_ref].attachedgrip[2] = tnc + 2;
      ChrList[chr_ref].attachedgrip[3] = tnc + 3;
    }
    else
    {
      ChrList[chr_ref].attachedgrip[0] = MadList[model].vertices - 1;
      ChrList[chr_ref].attachedgrip[1] = 0xFFFF;
      ChrList[chr_ref].attachedgrip[2] = 0xFFFF;
      ChrList[chr_ref].attachedgrip[3] = 0xFFFF;
    }
  }

  ChrList[chr_ref].jumptime = DELAY_JUMP * 4;


  // Run the held animation
  if ( ChrList[mount_ref].bmpdata.calc_is_mount && slot == SLOT_SADDLE )
  {
    // Riding mount_ref
    play_action( chr_ref, ACTION_MI, btrue );
    ChrList[chr_ref].action.loop = btrue;
  }
  else
  {
    play_action( chr_ref, ACTION_MM + slot, bfalse );
    if ( ChrList[chr_ref].isitem )
    {
      // Item grab
      ChrList[chr_ref].action.keep = btrue;
    }
  }

  // Set the team
  if ( ChrList[chr_ref].isitem )
  {
    ChrList[chr_ref].team = ChrList[mount_ref].team;
    // Set the alert
    ChrList[chr_ref].aistate.alert |= ALERT_GRABBED;
  }
  else if ( ChrList[mount_ref].bmpdata.calc_is_mount )
  {
    ChrList[mount_ref].team = ChrList[chr_ref].team;
    // Set the alert
    if ( !ChrList[mount_ref].isitem )
    {
      ChrList[mount_ref].aistate.alert |= ALERT_GRABBED;
    }
  }

  // It's not gonna hit the floor
  ChrList[chr_ref].hitready = bfalse;

  if ( ChrList[chr_ref].isplayer )
    debug_message( 1, "mounted %s(%s) to (%s)", ChrList[chr_ref].name, CapList[ChrList[chr_ref].model].classname, CapList[ChrList[mount_ref].model].classname );


  return btrue;
}

//--------------------------------------------------------------------------------------------
CHR_REF stack_in_pack( CHR_REF item_ref, CHR_REF chr_ref )
{
  // ZZ> This function looks in the chr_ref's pack for an item_ref similar
  //     to the one given.  If it finds one, it returns the similar item_ref's
  //     index number, otherwise it returns MAXCHR.
  Uint16 inpack, id;
  bool_t allok;

  Uint16 item_mdl = ChrList[item_ref].model;

  assert( MAXMODEL != VALIDATE_MDL( item_mdl ) );


  if ( CapList[item_mdl].isstackable )
  {
    Uint16 inpack_mdl;

    inpack = chr_get_nextinpack( chr_ref );
    inpack_mdl = ChrList[inpack].model;

    assert( MAXMODEL != VALIDATE_MDL( inpack_mdl ) );

    allok = bfalse;
    while ( VALID_CHR( inpack ) && !allok )
    {
      allok = btrue;
      if ( inpack_mdl != item_mdl )
      {
        if ( !CapList[inpack_mdl].isstackable )
        {
          allok = bfalse;
        }

        if ( ChrList[inpack].ammomax != ChrList[item_ref].ammomax )
        {
          allok = bfalse;
        }

        id = 0;
        while ( id < IDSZ_COUNT && allok )
        {
          if ( CapList[inpack_mdl].idsz[id] != CapList[item_mdl].idsz[id] )
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
static bool_t pack_push_front( CHR_REF item_ref, CHR_REF chr_ref )
{
  // make sure the item and character are valid
  if ( !VALID_CHR( item_ref ) || !VALID_CHR( chr_ref ) ) return bfalse;

  // make sure the item is free to add
  if ( chr_attached( item_ref ) || chr_in_pack( item_ref ) ) return bfalse;

  // we cannot do packs within packs, so
  if ( chr_in_pack( chr_ref ) ) return bfalse;

  // make sure there is space for the item
  if ( ChrList[chr_ref].numinpack >= MAXNUMINPACK ) return bfalse;

  // insert at the front of the list
  ChrList[item_ref].nextinpack  = chr_get_nextinpack( chr_ref );
  ChrList[chr_ref].nextinpack = item_ref;
  ChrList[item_ref].inwhichpack = chr_ref;
  ChrList[chr_ref].numinpack++;

  return btrue;
};

//--------------------------------------------------------------------------------------------
static Uint16 pack_pop_back( CHR_REF chr_ref )
{
  Uint16 iitem = MAXCHR, itail = MAXCHR;

  // make sure the character is valid
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  // if character is in a pack, it has no inventory of it's own
  if ( chr_in_pack( iitem ) ) return MAXCHR;

  // make sure there is something in the pack
  if ( !chr_has_inventory( chr_ref ) ) return MAXCHR;

  // remove from the back of the list
  itail = chr_ref;
  iitem = chr_get_nextinpack( chr_ref );
  while ( VALID_CHR( ChrList[iitem].nextinpack ) )
  {
    // do some error checking
    assert( 0 == ChrList[iitem].numinpack );

    // go to the next element
    itail = iitem;
    iitem = chr_get_nextinpack( iitem );
  };

  // disconnect the item from the list
  ChrList[itail].nextinpack = MAXCHR;
  ChrList[chr_ref].numinpack--;

  // do some error checking
  assert( VALID_CHR( iitem ) );

  // fix the removed item
  ChrList[iitem].numinpack   = 0;
  ChrList[iitem].nextinpack  = MAXCHR;
  ChrList[iitem].inwhichpack = MAXCHR;
  ChrList[iitem].isequipped = bfalse;

  return iitem;
};

//--------------------------------------------------------------------------------------------
bool_t add_item_to_character_pack( CHR_REF item_ref, CHR_REF chr_ref )
{
  // ZZ> This function puts one chr_ref inside the other's pack
  Uint16 newammo, istack;

  // Make sure both objects exist
  if ( !VALID_CHR( chr_ref ) || !VALID_CHR( item_ref ) ) return bfalse;

  // Make sure neither object is in a pack
  if ( chr_in_pack( chr_ref ) || chr_in_pack( item_ref ) ) return bfalse;

  // make sure we the character IS NOT an item and the item IS an item
  if ( ChrList[chr_ref].isitem || !ChrList[item_ref].isitem ) return bfalse;

  // make sure the item does not have an inventory of its own
  if ( chr_has_inventory( item_ref ) ) return bfalse;

  istack = stack_in_pack( item_ref, chr_ref );
  if ( VALID_CHR( istack ) )
  {
    // put out torches, etc.
    disaffirm_attached_particles( item_ref );

    // We found a similar, stackable item_ref in the pack
    if ( ChrList[item_ref].nameknown || ChrList[istack].nameknown )
    {
      ChrList[item_ref].nameknown = btrue;
      ChrList[istack].nameknown = btrue;
    }
    if ( CapList[ChrList[item_ref].model].usageknown || CapList[ChrList[istack].model].usageknown )
    {
      CapList[ChrList[item_ref].model].usageknown = btrue;
      CapList[ChrList[istack].model].usageknown = btrue;
    }
    newammo = ChrList[item_ref].ammo + ChrList[istack].ammo;
    if ( newammo <= ChrList[istack].ammomax )
    {
      // All transfered, so kill the in hand item_ref
      ChrList[istack].ammo = newammo;
      detach_character_from_mount( item_ref, btrue, bfalse );
      ChrList[item_ref].freeme = btrue;
    }
    else
    {
      // Only some were transfered,
      ChrList[item_ref].ammo += ChrList[istack].ammo - ChrList[istack].ammomax;
      ChrList[istack].ammo = ChrList[istack].ammomax;
      ChrList[chr_ref].aistate.alert |= ALERT_TOOMUCHBAGGAGE;
    }
  }
  else
  {
    // Make sure we have room for another item_ref
    if ( ChrList[chr_ref].numinpack >= MAXNUMINPACK )
    {
      ChrList[chr_ref].aistate.alert |= ALERT_TOOMUCHBAGGAGE;
      return bfalse;
    }

    // Take the item out of hand
    if ( detach_character_from_mount( item_ref, btrue, bfalse ) )
    {
      ChrList[item_ref].aistate.alert &= ~ALERT_DROPPED;
    }

    if ( pack_push_front( item_ref, chr_ref ) )
    {
      // put out torches, etc.
      disaffirm_attached_particles( item_ref );
      ChrList[item_ref].aistate.alert |= ALERT_ATLASTWAYPOINT;

      // Remove the item_ref from play
      ChrList[item_ref].hitready    = bfalse;
    };
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 get_item_from_character_pack( CHR_REF chr_ref, SLOT slot, bool_t ignorekurse )
{
  // ZZ> This function takes the last item in the chr_ref's pack and puts
  //     it into the designated hand.  It returns the item number or MAXCHR.
  Uint16 item;

  // dose the chr_ref exist?
  if ( !VALID_CHR( chr_ref ) )
    return MAXCHR;

  // make sure a valid inventory exists
  if ( !chr_has_inventory( chr_ref ) )
    return MAXCHR;

  item = pack_pop_back( chr_ref );

  // Figure out what to do with it
  if ( ChrList[item].iskursed && ChrList[item].isequipped && !ignorekurse )
  {
    // Flag the last item as not removed
    ChrList[item].aistate.alert |= ALERT_NOTPUTAWAY;  // Doubles as IfNotTakenOut

    // push it back on the front of the list
    pack_push_front( item, chr_ref );

    // return the "fail" value
    item = MAXCHR;
  }
  else
  {
    // Attach the item to the chr_ref's hand
    attach_character_to_mount( item, chr_ref, slot );

    // fix some item values
    ChrList[item].aistate.alert &= ( ~ALERT_GRABBED );
    ChrList[item].aistate.alert |= ALERT_TAKENOUT;
    //ChrList[item].team   = ChrList[chr_ref].team;
  }

  return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys( CHR_REF chr_ref )
{
  // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a chr_ref's
  //     inventory ( Not hands ).
  Uint16 item, lastitem, nextitem, direction, cosdir;
  IDSZ testa, testz;


  if ( !VALID_CHR( chr_ref ) ) return;


  if ( ChrList[chr_ref].pos.z > -2 ) // Don't lose keys in pits...
  {
    // The IDSZs to find
    testa = MAKE_IDSZ( "KEYA" );   // [KEYA]
    testz = MAKE_IDSZ( "KEYZ" );   // [KEYZ]

    lastitem = chr_ref;
    item = chr_get_nextinpack( chr_ref );
    while ( VALID_CHR( item ) )
    {
      nextitem = chr_get_nextinpack( item );
      if ( item != chr_ref ) // Should never happen...
      {
        if ( CAP_INHERIT_IDSZ_RANGE( ChrList[item].model, testa, testz ) )
        {
          // We found a key...
          ChrList[item].inwhichpack = MAXCHR;
          ChrList[item].isequipped = bfalse;

          ChrList[lastitem].nextinpack = nextitem;
          ChrList[item].nextinpack = MAXCHR;
          ChrList[chr_ref].numinpack--;

          ChrList[item].hitready = btrue;
          ChrList[item].aistate.alert |= ALERT_DROPPED;

          direction = RANDIE;
          ChrList[item].turn_lr = direction + 32768;
          cosdir = direction + 16384;
          ChrList[item].level = ChrList[chr_ref].level;
          ChrList[item].pos.x = ChrList[chr_ref].pos.x;
          ChrList[item].pos.y = ChrList[chr_ref].pos.y;
          ChrList[item].pos.z = ChrList[chr_ref].pos.z;
          ChrList[item].accum_vel.x += turntosin[( cosdir>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          ChrList[item].accum_vel.y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
          ChrList[item].accum_vel.z += DROPZVEL;
          ChrList[item].team = ChrList[item].baseteam;
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
void drop_all_items( CHR_REF chr_ref )
{
  // ZZ> This function drops all of a chr_ref's items
  Uint16 item, direction, cosdir, diradd;


  if ( !VALID_CHR( chr_ref ) ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    detach_character_from_mount( chr_get_holdingwhich( chr_ref, _slot ), !ChrList[chr_ref].alive, bfalse );
  };

  if ( chr_has_inventory( chr_ref ) )
  {
    direction = ChrList[chr_ref].turn_lr + 32768;
    diradd = (float)UINT16_SIZE / ChrList[chr_ref].numinpack;
    while ( ChrList[chr_ref].numinpack > 0 )
    {
      item = get_item_from_character_pack( chr_ref, SLOT_NONE, !ChrList[chr_ref].alive );
      if ( detach_character_from_mount( item, btrue, btrue ) )
      {
        ChrList[item].hitready = btrue;
        ChrList[item].aistate.alert |= ALERT_DROPPED;
        ChrList[item].pos.x = ChrList[chr_ref].pos.x;
        ChrList[item].pos.y = ChrList[chr_ref].pos.y;
        ChrList[item].pos.z = ChrList[chr_ref].pos.z;
        ChrList[item].level = ChrList[chr_ref].level;
        ChrList[item].turn_lr = direction + 32768;

        cosdir = direction + 16384;
        ChrList[item].accum_vel.x += turntosin[( cosdir>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        ChrList[item].accum_vel.y += turntosin[( direction>>2 ) & TRIGTABLE_MASK] * DROPXYVEL;
        ChrList[item].accum_vel.z += DROPZVEL;
        ChrList[item].team = ChrList[item].baseteam;
      }

      direction += diradd;
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t character_grab_stuff( CHR_REF chr_ref, SLOT slot, bool_t people )
{
  // ZZ> This function makes the character pick up an item if there's one around
  vect4 posa, point;
  vect3 posb, posc;
  float dist, mindist;
  CHR_REF object_ref, minchr_ref, holder_ref, packer_ref, owner_ref = MAXCHR;
  Uint16 vertex, model, passage, cnt, price;
  bool_t inshop, can_disarm, can_pickpocket, bfound, ballowed;
  GRIP grip;
  float grab_width, grab_height;

  CHR_REF trg_chr = MAXCHR;
  Sint16 trg_strength_fp8, trg_intelligence_fp8;
  TEAM trg_team;

  if ( !VALID_CHR( chr_ref ) ) return bfalse;

  model = ChrList[chr_ref].model;
  if ( !VALID_MDL( model ) ) return bfalse;

  // Make sure the character doesn't have something already, and that it has hands

  if ( chr_using_slot( chr_ref, slot ) || !CapList[model].slotvalid[slot] )
    return bfalse;

  // Make life easier
  grip  = slot_to_grip( slot );

  // !!!!base the grab distance off of the character size!!!!
  grab_width  = ( ChrList[chr_ref].bmpdata.calc_size_big + ChrList[chr_ref].bmpdata.calc_size ) / 2.0f * 1.5f;
  grab_height = ChrList[chr_ref].bmpdata.calc_height / 2.0f * 1.5f;

  // Do we have a matrix???
  if ( ChrList[chr_ref].matrixvalid )
  {
    // Transform the weapon grip from model to world space
    vertex = ChrList[chr_ref].attachedgrip[0];

    if(0xFFFF == vertex)
    {
      point.x = ChrList[chr_ref].pos.x;
      point.y = ChrList[chr_ref].pos.y;
      point.z = ChrList[chr_ref].pos.z;
      point.w = 1.0f;
    }
    else
    {
      point.x = ChrList[chr_ref].vdata.Vertices[vertex].x;
      point.y = ChrList[chr_ref].vdata.Vertices[vertex].y;
      point.z = ChrList[chr_ref].vdata.Vertices[vertex].z;
      point.w = 1.0f;
    }

    // Do the transform
    Transform4_Full( 1.0f, 1.0f, &(ChrList[chr_ref].matrix), &posa, &point, 1 );
  }
  else
  {
    // Just wing it
    posa.x = ChrList[chr_ref].pos.x;
    posa.y = ChrList[chr_ref].pos.y;
    posa.z = ChrList[chr_ref].pos.z;
  }

  // Go through all characters to find the best match
  can_disarm     = check_skills( chr_ref, MAKE_IDSZ( "DISA" ) );
  can_pickpocket = check_skills( chr_ref, MAKE_IDSZ( "PICK" ) );
  bfound = bfalse;
  for ( object_ref = 0; object_ref < MAXCHR; object_ref++ )
  {
    // Don't mess with stuff that doesn't exist
    if ( !VALID_CHR( object_ref ) ) continue;

    holder_ref = chr_get_attachedto(object_ref);
    packer_ref = chr_get_inwhichpack(object_ref);

    // don't mess with yourself or anything you're already holding
    if ( object_ref == chr_ref || packer_ref == chr_ref || holder_ref == chr_ref ) continue;

    // don't mess with stuff you can't see
    if ( !ChrList[chr_ref].canseeinvisible && chr_is_invisible( object_ref ) ) continue;

    // if we can't pickpocket, don't mess with inventory items
    if ( !can_pickpocket && VALID_CHR( packer_ref ) ) continue;

    // if we can't disarm, don't mess with held items
    if ( !can_disarm && VALID_CHR( holder_ref ) ) continue;

    // if we can't grab people, don't mess with them
    if ( !people && !ChrList[object_ref].isitem ) continue;

    // get the target object position
    if ( !VALID_CHR( packer_ref ) && !VALID_CHR(holder_ref) )
    {
      trg_strength_fp8     = ChrList[object_ref].strength_fp8;
      trg_intelligence_fp8 = ChrList[object_ref].intelligence_fp8;
      trg_team             = ChrList[object_ref].team;

      posb = ChrList[object_ref].pos;
    }
    else if ( VALID_CHR(holder_ref) )
    {
      trg_chr              = holder_ref;
      trg_strength_fp8     = ChrList[holder_ref].strength_fp8;
      trg_intelligence_fp8 = ChrList[holder_ref].intelligence_fp8;

      trg_team = ChrList[holder_ref].team;
      posb     = ChrList[object_ref].pos;
    }
    else // must be in a pack
    {
      trg_chr              = packer_ref;
      trg_strength_fp8     = ChrList[packer_ref].strength_fp8;
      trg_intelligence_fp8 = ChrList[packer_ref].intelligence_fp8;
      trg_team = ChrList[packer_ref].team;
      posb     = ChrList[packer_ref].pos;
      posb.z  += ChrList[packer_ref].bmpdata.calc_height / 2;
    };

    // First check absolute value diamond
    posc.x = ABS( posa.x - posb.x );
    posc.y = ABS( posa.y - posb.y );
    posc.z = ABS( posa.z - posb.z );
    dist = posc.x + posc.y;

    // close enough to grab ?
    if ( dist > grab_width || posc.z > grab_height ) continue;

    if ( VALID_CHR(packer_ref) )
    {
      // check for pickpocket
      ballowed = ChrList[chr_ref].dexterity_fp8 >= trg_intelligence_fp8 && TeamList[ChrList[chr_ref].team].hatesteam[trg_team];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        ChrList[holder_ref].aistate.alert |= ALERT_ATTACKED;
        ChrList[holder_ref].aistate.bumplast = chr_ref;
      }
      else  // must be in a pack
      {
        // TODO : figure out a way to get the thing out of the pack!!
        //        get_item_from_character_pack() won't work?

      };
    }
    else if ( VALID_CHR( holder_ref ) )
    {
      // check for stealing item from hand
      ballowed = !ChrList[object_ref].iskursed && ChrList[chr_ref].strength_fp8 > trg_strength_fp8 && TeamList[ChrList[chr_ref].team].hatesteam[trg_team];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        ChrList[holder_ref].aistate.alert |= ALERT_ATTACKED;
        ChrList[holder_ref].aistate.bumplast = chr_ref;
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
      minchr_ref  = object_ref;
      bfound  = btrue;
    };

  };

  if ( !bfound ) return bfalse;

  // Check for shop
  inshop = bfalse;
  ballowed = bfalse;
  if ( mesh_check( ChrList[minchr_ref].pos.x, ChrList[minchr_ref].pos.y ) )
  {
    if ( numshoppassage == 0 )
    {
      ballowed = btrue;
    }
    else if ( ChrList[minchr_ref].isitem )
    {

      // loop through just in case there are overlapping shops with one owner_ref deceased
      for ( cnt = 0; cnt < numshoppassage && !inshop; cnt++ )
      {
        passage = shoppassage[cnt];

        if ( passage_check_any( minchr_ref, passage, NULL ) )
        {
          owner_ref  = shopowner[passage];
          inshop = ( NOOWNER != owner_ref );
        };
      };

    };
  }


  if ( inshop )
  {
    if ( ChrList[chr_ref].isitem )
    {
      ballowed = btrue; // As in NetHack, Pets can shop for free =]
    }
    else
    {
      // Pay the shop owner_ref, or don't allow grab...
      ChrList[owner_ref].aistate.alert |= ALERT_SIGNALED;
      price = CapList[ChrList[minchr_ref].model].skincost[( ChrList[minchr_ref].texture - MadList[ChrList[minchr_ref].model].skinstart ) % MAXSKIN];
      if ( CapList[ChrList[minchr_ref].model].isstackable )
      {
        price *= ChrList[minchr_ref].ammo;
      }
      ChrList[owner_ref].message = price;  // Tell owner_ref how much...
      if ( ChrList[chr_ref].money >= price )
      {
        // Okay to buy
        ChrList[chr_ref].money  -= price;  // Skin 0 cost is price
        ChrList[owner_ref].money += price;
        if ( ChrList[owner_ref].money > MAXMONEY )  ChrList[owner_ref].money = MAXMONEY;

        ballowed = btrue;
        ChrList[owner_ref].messagedata = 1;  // 1 for selling an item
      }
      else
      {
        // Don't allow purchase
        ChrList[owner_ref].messagedata = 2;  // 2 for "you can't afford that"
        ballowed = bfalse;
      }
    }
  }


  if ( ballowed )
  {
    // Stick 'em together and quit
    ballowed = attach_character_to_mount( minchr_ref, chr_ref, slot );
    if ( ballowed && people )
    {
      // Do a bodyslam animation...  ( Be sure to drop!!! )
      play_action( chr_ref, ACTION_MC + slot, bfalse );
    };
  }
  else
  {
    // Lift the item a little and quit...
    ChrList[minchr_ref].accum_vel.z += DROPZVEL;
    ChrList[minchr_ref].hitready = btrue;
    ChrList[minchr_ref].aistate.alert |= ALERT_DROPPED;
  };

  return ballowed;
}

//--------------------------------------------------------------------------------------------
void character_swipe( CHR_REF chr_ref, SLOT slot )
{
  // ZZ> This function spawns an attack particle
  Uint16 weapon, particle, thrown;
  ACTION action;
  Uint16 tTmp;
  float dampen;
  vect3 pos;
  float velocity;
  GRIP spawngrip;


  weapon = chr_get_holdingwhich( chr_ref, slot );
  spawngrip = GRIP_LAST;
  action = ChrList[chr_ref].action.now;
  // See if it's an unarmed attack...
  if ( !VALID_CHR( weapon ) )
  {
    weapon = chr_ref;
    spawngrip = slot_to_grip( slot );
  }


  if ( weapon != chr_ref && (( CapList[ChrList[weapon].model].isstackable && ChrList[weapon].ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
  {
    // Throw the weapon if it's stacked or a hurl animation
    pos.x = ChrList[chr_ref].pos.x;
    pos.y = ChrList[chr_ref].pos.y;
    pos.z = ChrList[chr_ref].pos.z;
    thrown = spawn_one_character( ChrList[chr_ref].pos, ChrList[weapon].model, ChrList[chr_ref].team, 0, ChrList[chr_ref].turn_lr, ChrList[weapon].name, MAXCHR );
    if ( VALID_CHR( thrown ) )
    {
      ChrList[thrown].iskursed = bfalse;
      ChrList[thrown].ammo = 1;
      ChrList[thrown].aistate.alert |= ALERT_THROWN;

      velocity = 0.0f;
      if ( ChrList[chr_ref].weight >= 0.0f )
      {
        velocity = ChrList[chr_ref].strength_fp8 / ( ChrList[thrown].weight * THROWFIX );
      };

      velocity += MINTHROWVELOCITY;
      if ( velocity > MAXTHROWVELOCITY )
      {
        velocity = MAXTHROWVELOCITY;
      }
      tTmp = ( 0x7FFF + ChrList[chr_ref].turn_lr ) >> 2;
      ChrList[thrown].accum_vel.x += turntosin[( tTmp+8192+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK] * velocity;
      ChrList[thrown].accum_vel.y += turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * velocity;
      ChrList[thrown].accum_vel.z += DROPZVEL;
      if ( ChrList[weapon].ammo <= 1 )
      {
        // Poof the item
        detach_character_from_mount( weapon, btrue, bfalse );
        ChrList[weapon].freeme = btrue;
      }
      else
      {
        ChrList[weapon].ammo--;
      }
    }
  }
  else
  {
    // Spawn an attack particle
    if ( ChrList[weapon].ammomax == 0 || ChrList[weapon].ammo != 0 )
    {
      if ( ChrList[weapon].ammo > 0 && !CapList[ChrList[weapon].model].isstackable )
      {
        ChrList[weapon].ammo--;  // Ammo usage
      }

      //HERE
      if ( CapList[ChrList[weapon].model].attackprttype != -1 )
      {
        particle = spawn_one_particle( 1.0f, ChrList[weapon].pos, ChrList[chr_ref].turn_lr, ChrList[weapon].model, CapList[ChrList[weapon].model].attackprttype, weapon, spawngrip, ChrList[chr_ref].team, chr_ref, 0, MAXCHR );
        if ( particle != MAXPRT )
        {
          CHR_REF prt_target = prt_get_target( particle );

          if ( !CapList[ChrList[weapon].model].attackattached )
          {
            // Detach the particle
            if ( !PipList[PrtList[particle].pip].startontarget || !VALID_CHR( prt_target ) )
            {
              attach_particle_to_character( particle, weapon, spawngrip );

              // Correct Z spacing base, but nothing else...
              PrtList[particle].pos.z += PipList[PrtList[particle].pip].zspacing.ibase;
            }
            PrtList[particle].attachedtochr = MAXCHR;

            // Don't spawn in walls
            if ( 0 != __prthitawall( particle, NULL ) )
            {
              PrtList[particle].pos.x = ChrList[weapon].pos.x;
              PrtList[particle].pos.y = ChrList[weapon].pos.y;
              if ( 0 != __prthitawall( particle, NULL ) )
              {
                PrtList[particle].pos.x = ChrList[chr_ref].pos.x;
                PrtList[particle].pos.y = ChrList[chr_ref].pos.y;
              }
            }
          }
          else
          {
            // Attached particles get a strength bonus for reeling...
            if ( PipList[PrtList[particle].pip].causeknockback ) dampen = ( REELBASE + ( ChrList[chr_ref].strength_fp8 / REEL ) ) * 4; //Extra knockback?
            else dampen = REELBASE + ( ChrList[chr_ref].strength_fp8 / REEL );      // No, do normal

            PrtList[particle].accum_vel.x += -(1.0f - dampen) * PrtList[particle].vel.x;
            PrtList[particle].accum_vel.y += -(1.0f - dampen) * PrtList[particle].vel.y;
            PrtList[particle].accum_vel.z += -(1.0f - dampen) * PrtList[particle].vel.z;
          }

          // Initial particles get a strength bonus, which may be 0.00
          PrtList[particle].damage.ibase += ( ChrList[chr_ref].strength_fp8 * CapList[ChrList[weapon].model].strengthdampen );

          // Initial particles get an enchantment bonus
          PrtList[particle].damage.ibase += ChrList[weapon].damageboost;

          // Initial particles inherit damage type of weapon
          PrtList[particle].damagetype = ChrList[weapon].damagetargettype;
        }
      }
    }
    else
    {
      ChrList[weapon].ammoknown = btrue;
    }
  }
}

//--------------------------------------------------------------------------------------------
void despawn_characters()
{
  CHR_REF chr_ref;

  // poof all characters that have pending poof requests
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( chr_ref ) || !ChrList[chr_ref].gopoof ) continue;

    // detach from any imount
    detach_character_from_mount( chr_ref, btrue, bfalse );

    // Drop all possesions
    for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
    {
      if ( chr_using_slot( chr_ref, _slot ) )
        detach_character_from_mount( chr_get_holdingwhich( chr_ref, _slot ), btrue, bfalse );
    };

    free_inventory( chr_ref );
    ChrList[chr_ref].freeme = btrue;
  };

  // free all characters that requested destruction last round
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( chr_ref ) || !ChrList[chr_ref].freeme ) continue;
    free_one_character( chr_ref );
  }
};

//--------------------------------------------------------------------------------------------
void move_characters( float dUpdate )
{
  // ZZ> This function handles character physics
  CHR_REF chr_ref, weapon, imount, item;
  Uint16 imdl;
  Uint8 twist;
  Uint8 speed, framelip;
  float maxvel, dvx, dvy, dvmax;
  ACTION action;
  bool_t   ready, allowedtoattack, watchtarget, grounded, dojumptimer;
  TURNMODE loc_turnmode;
  float ftmp, level;

  float horiz_friction, vert_friction;
  float loc_slippyfriction, loc_airfriction, loc_waterfriction, loc_noslipfriction;
  float loc_flydampen, loc_traction;
  float turnfactor;
  vect3 nrm = {0.0f, 0.0f, 0.0f};

  AI_STATE * pstate;
  CHR * pchr;
  MAD * pmad;

  loc_airfriction    = airfriction;
  loc_waterfriction  = waterfriction;
  loc_slippyfriction = slippyfriction;
  loc_noslipfriction = noslipfriction;
  loc_flydampen      = pow( FLYDAMPEN     , dUpdate );

  // Move every character
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( chr_ref ) ) continue;

    pchr = ChrList + chr_ref;
    pstate = &(pchr->aistate);

    // Character's old location
    pchr->turn_lr_old = pchr->turn_lr;

    if ( chr_in_pack( chr_ref ) ) continue;

    // get the model
    imdl = VALIDATE_MDL( pchr->model );
    assert( MAXMODEL != imdl );
    pmad = MadList + pchr->model;

    // get the imount
    imount = chr_get_attachedto(chr_ref);

    // get the level
    level = pchr->level;

    // TURNMODE_VELOCITY acts strange when someone is mounted on a "bucking" imount, like the gelfeet
    loc_turnmode = pstate->turnmode;
    if ( VALID_CHR( imount ) ) loc_turnmode = TURNMODE_NONE;

    // make characters slide downhill
    twist = mesh_get_twist( pchr->onwhichfan );

    // calculate the normal GDyna.mically from the mesh coordinates
    if ( !mesh_calc_normal( pchr->pos, &nrm ) )
    {
      nrm = mapnrm[twist];
    };

    // TODO : replace with line(s) below
    turnfactor = 2.0f;
    // scale the turn rate by the dexterity.
    // For zero dexterity, rate is half speed
    // For maximum dexterity, rate is 1.5 normal rate
    //turnfactor = (3.0f * (float)pchr->dexterity_fp8 / (float)PERFECTSTAT + 1.0f) / 2.0f;

    grounded    = bfalse;

    // Down that ol' damage timer
    pchr->damagetime -= dUpdate;
    if ( pchr->damagetime < 0 ) pchr->damagetime = 0.0f;

    // Texture movement
    pchr->uoffset_fp8 += pchr->uoffvel * dUpdate;
    pchr->voffset_fp8 += pchr->voffvel * dUpdate;

    // calculate the Character's environment
    {
      float wt;
      float air_traction = ( pchr->flyheight == 0.0f ) ? ( 1.0 - airfriction ) : airfriction;

      wt = 0.0f;
      loc_traction   = 0;
      horiz_friction = 0;
      vert_friction  = 0;

      if ( pchr->inwater )
      {
        // we are partialy under water
        float buoy, lerp;

        if ( pchr->weight < 0.0f || pchr->holdingweight < 0.0f )
        {
          buoy = 0.0f;
        }
        else
        {
          float volume, weight;

          weight = pchr->weight + pchr->holdingweight;
          volume = ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min ) * ( pchr->bmpdata.cv.x_max - pchr->bmpdata.cv.x_min ) * ( pchr->bmpdata.cv.y_max - pchr->bmpdata.cv.y_min );

          // this adjusts the buoyancy so that the default adventurer gets a buoyancy of 0.3
          buoy = 0.3f * ( weight / volume ) * 1196.0f;
          if ( buoy < 0.0f ) buoy = 0.0f;
          if ( buoy > 1.0f ) buoy = 1.0f;
        };

        lerp = ( float )( GWater.surfacelevel - pchr->pos.z ) / ( float ) ( pchr->bmpdata.cv.z_max - pchr->bmpdata.cv.z_min );
        if ( lerp > 1.0f ) lerp = 1.0f;
        if ( lerp < 0.0f ) lerp = 0.0f;

        loc_traction   += waterfriction * lerp;
        horiz_friction += waterfriction * lerp;
        vert_friction  += waterfriction * lerp;
        pchr->accum_acc.z             -= buoy * gravity  * lerp;

        wt += lerp;
      }

      if ( pchr->pos.z < level + PLATTOLERANCE )
      {
        // we are close to something
        bool_t is_slippy;
        float lerp = ( level + PLATTOLERANCE - pchr->pos.z ) / ( float ) PLATTOLERANCE;
        if ( lerp > 1.0f ) lerp = 1.0f;
        if ( lerp < 0.0f ) lerp = 0.0f;

        if ( VALID_CHR( pchr->onwhichplatform ) )
        {
          is_slippy = bfalse;
        }
        else
        {
          is_slippy = ( INVALID_FAN != pchr->onwhichfan ) && mesh_has_some_bits( pchr->onwhichfan, MESHFX_SLIPPY );
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

      grounded = ( pchr->pos.z < level + PLATTOLERANCE / 20.0f );
    }

    // do volontary movement
    if ( pchr->alive )
    {
      // Apply the latches
      if ( !VALID_CHR( imount ) )
      {
        // Character latches for generalized movement
        dvx = pstate->latch.x;
        dvy = pstate->latch.y;

        // Reverse movements for daze
        if ( pchr->dazetime > 0.0f )
        {
          dvx = -dvx;
          dvy = -dvy;
        }

        // Switch x and y for grog
        if ( pchr->grogtime > 0.0f )
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
            pchr->turn_lr = terp_dir( pchr->turn_lr, dvx, dvy, dUpdate * turnfactor );
          }
        }

        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_STOP ) )
        {
          dvx = 0;
          dvy = 0;
        }

        // TODO : change to line(s) below
        maxvel = pchr->maxaccel / ( 1.0 - noslipfriction );
        // set a minimum speed of 6 to fix some stupid slow speeds
        //maxvel = 1.5f * MAX(MAX(3,pchr->spd_run), MAX(pchr->spd_walk,pchr->spd_sneak));
        pstate->trgvel.x = dvx * maxvel;
        pstate->trgvel.y = dvy * maxvel;
        pstate->trgvel.z = 0;

        if ( pchr->maxaccel > 0.0f )
        {
          dvx = ( pstate->trgvel.x - pchr->vel.x );
          dvy = ( pstate->trgvel.y - pchr->vel.y );

          // TODO : change to line(s) below
          dvmax = pchr->maxaccel;
          // Limit to max acceleration
          //if(maxvel==0.0)
          //{
          //  dvmax = 2.0f * pchr->maxaccel;
          //}
          //else
          //{
          //  float ftmp;
          //  chrvel2 = pchr->vel.x*pchr->vel.x + pchr->vel.y*pchr->vel.y;
          //  ftmp = MIN(1.0 , chrvel2/maxvel/maxvel);
          //  dvmax   = 2.0f * pchr->maxaccel * (1.0-ftmp);
          //};

          if ( dvx < -dvmax ) dvx = -dvmax;
          if ( dvx >  dvmax ) dvx =  dvmax;
          if ( dvy < -dvmax ) dvy = -dvmax;
          if ( dvy >  dvmax ) dvy =  dvmax;

          loc_traction *= 11.0f;                    // 11.0f corrects traction so that it gives full traction for non-slip floors in advent.mod
          loc_traction = MIN( 1.0, loc_traction );

          pchr->accum_acc.x += dvx * loc_traction * nrm.z;
          pchr->accum_acc.y += dvy * loc_traction * nrm.z;
        };
      }

      // Apply ChrList[].latch.x and ChrList[].latch.y
      if ( !VALID_CHR( imount ) )
      {
        // Face the target
        watchtarget = ( loc_turnmode == TURNMODE_WATCHTARGET );
        if ( watchtarget )
        {
          CHR_REF ai_target = chr_get_aitarget( chr_ref );
          if ( VALID_CHR( ai_target ) && chr_ref != ai_target )
          {
            pchr->turn_lr = terp_dir( pchr->turn_lr, ChrList[ai_target].pos.x - pchr->pos.x, ChrList[ai_target].pos.y - pchr->pos.y, dUpdate * turnfactor );
          };
        }

        // Get direction from ACTUAL change in velocity
        if ( loc_turnmode == TURNMODE_VELOCITY )
        {
          if ( pchr->isplayer )
            pchr->turn_lr = terp_dir( pchr->turn_lr, pstate->trgvel.x, pstate->trgvel.y, dUpdate * turnfactor );
          else
            pchr->turn_lr = terp_dir( pchr->turn_lr, pstate->trgvel.x, pstate->trgvel.y, dUpdate * turnfactor / 4.0f );
        }

        // Otherwise make it spin
        else if ( loc_turnmode == TURNMODE_SPIN )
        {
          pchr->turn_lr += SPINRATE * dUpdate * turnfactor;
        }
      };

      // Character latches for generalized buttons
      if ( LATCHBUTTON_NONE != pstate->latch.b )
      {
        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_JUMP ) && pchr->jumptime == 0.0f )
        {
          if ( detach_character_from_mount( chr_ref, btrue, btrue ) )
          {
            pchr->jumptime = DELAY_JUMP;
            pchr->accum_vel.z += ( pchr->flyheight == 0 ) ? DISMOUNTZVEL : DISMOUNTZVELFLY;
            if ( pchr->jumpnumberreset != JUMPINFINITE && pchr->jumpnumber > 0 )
              pchr->jumpnumber -= dUpdate;

            // Play the jump sound
            if ( INVALID_SOUND != CapList[imdl].jumpsound )
            {
              play_sound( 1.0f, pchr->pos, CapList[imdl].wavelist[CapList[imdl].jumpsound], 0, imdl, CapList[imdl].jumpsound );
            };
          }
          else if ( pchr->jumpnumber > 0 && ( pchr->jumpready || pchr->jumpnumberreset > 1 ) )
          {
            // Make the character jump
            if ( pchr->inwater && !grounded )
            {
              pchr->accum_vel.z += WATERJUMP / 3.0f;
              pchr->jumptime = DELAY_JUMP / 3.0f;
            }
            else
            {
              pchr->accum_vel.z += pchr->jump * 2.0f;
              pchr->jumptime = DELAY_JUMP;

              // Set to jump animation if not doing anything better
              if ( pchr->action.ready )    play_action( chr_ref, ACTION_JA, btrue );

              // Play the jump sound (Boing!)
              if ( INVALID_SOUND != CapList[imdl].jumpsound )
              {
                play_sound( MIN( 1.0f, pchr->jump / 50.0f ), pchr->pos, CapList[imdl].wavelist[CapList[imdl].jumpsound], 0, imdl, CapList[imdl].jumpsound );
              }
            };

            pchr->hitready  = btrue;
            pchr->jumpready = bfalse;
            if ( pchr->jumpnumberreset != JUMPINFINITE ) pchr->jumpnumber -= dUpdate;
          }
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_ALTLEFT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_GRAB;
          if ( !chr_using_slot( chr_ref, SLOT_LEFT ) )
          {
            // Grab left
            play_action( chr_ref, ACTION_ME, bfalse );
          }
          else
          {
            // Drop left
            play_action( chr_ref, ACTION_MA, bfalse );
          }
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_ALTRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_GRAB;
          if ( !chr_using_slot( chr_ref, SLOT_RIGHT ) )
          {
            // Grab right
            play_action( chr_ref, ACTION_MF, bfalse );
          }
          else
          {
            // Drop right
            play_action( chr_ref, ACTION_MB, bfalse );
          }
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_PACKLEFT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_PACK;
          item = chr_get_holdingwhich( chr_ref, SLOT_LEFT );
          if ( VALID_CHR( item ) )
          {
            if (( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
            {
              // The item couldn't be put away
              ChrList[item].aistate.alert |= ALERT_NOTPUTAWAY;
            }
            else
            {
              // Put the item into the pack
              add_item_to_character_pack( item, chr_ref );
            }
          }
          else
          {
            // Get a new one out and put it in hand
            get_item_from_character_pack( chr_ref, SLOT_LEFT, bfalse );
          }

          // Make it take a little time
          play_action( chr_ref, ACTION_MG, bfalse );
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_PACKRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_PACK;
          item = chr_get_holdingwhich( chr_ref, SLOT_RIGHT );
          if ( VALID_CHR( item ) )
          {
            if (( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
            {
              // The item couldn't be put away
              ChrList[item].aistate.alert |= ALERT_NOTPUTAWAY;
            }
            else
            {
              // Put the item into the pack
              add_item_to_character_pack( item, chr_ref );
            }
          }
          else
          {
            // Get a new one out and put it in hand
            get_item_from_character_pack( chr_ref, SLOT_RIGHT, bfalse );
          }

          // Make it take a little time
          play_action( chr_ref, ACTION_MG, bfalse );
        }

        if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_LEFT ) && pchr->reloadtime == 0 )
        {
          // Which weapon?
          weapon = chr_get_holdingwhich( chr_ref, SLOT_LEFT );
          if ( !VALID_CHR( weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = chr_ref;
          }
          action = CapList[ChrList[weapon].model].weaponaction;


          // Can it do it?
          allowedtoattack = btrue;
          if ( !pmad->actionvalid[action] || ChrList[weapon].reloadtime > 0 ||
               ( CapList[ChrList[weapon].model].needskillidtouse && !check_skills( chr_ref, CapList[ChrList[weapon].model].idsz[IDSZ_SKILL] ) ) )
          {
            allowedtoattack = bfalse;
            if ( ChrList[weapon].reloadtime == 0 )
            {
              // This character can't use this weapon
              ChrList[weapon].reloadtime = 50;
              if ( pchr->staton )
              {
                // Tell the player that they can't use this weapon
                debug_message( 1, "%s can't use this item...", pchr->name );
              }
            }
          }

          if ( action == ACTION_DA )
          {
            allowedtoattack = bfalse;
            if ( ChrList[weapon].reloadtime == 0 )
            {
              ChrList[weapon].aistate.alert |= ALERT_USED;
            }
          }


          if ( allowedtoattack )
          {
            // Rearing imount
            if ( VALID_CHR( imount ) )
            {
              allowedtoattack = CapList[ChrList[imount].model].ridercanattack;
              if ( ChrList[imount].ismount && ChrList[imount].alive && !ChrList[imount].isplayer && ChrList[imount].action.ready )
              {
                if (( action != ACTION_PA || !allowedtoattack ) && pchr->action.ready )
                {
                  play_action( imount, ( ACTION )( ACTION_UA + ( rand() &1 ) ), bfalse );
                  ChrList[imount].aistate.alert |= ALERT_USED;
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
              if ( pchr->action.ready && pmad->actionvalid[action] )
              {
                // Check mana cost
                if ( pchr->mana_fp8 >= ChrList[weapon].manacost || pchr->canchannel )
                {
                  cost_mana( chr_ref, ChrList[weapon].manacost, weapon );
                  // Check life healing
                  pchr->life_fp8 += ChrList[weapon].lifeheal;
                  if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;
                  ready = ( action == ACTION_PA );
                  action += rand() & 1;
                  play_action( chr_ref, action, ready );
                  if ( weapon != chr_ref )
                  {
                    // Make the weapon attack too
                    play_action( weapon, ACTION_MJ, bfalse );
                    ChrList[weapon].aistate.alert |= ALERT_USED;
                  }
                  else
                  {
                    // Flag for unarmed attack
                    pstate->alert |= ALERT_USED;
                  }
                }
              }
            }
          }
        }
        else if ( HAS_SOME_BITS( pstate->latch.b, LATCHBUTTON_RIGHT ) && pchr->reloadtime == 0 )
        {
          // Which weapon?
          weapon = chr_get_holdingwhich( chr_ref, SLOT_RIGHT );
          if ( !VALID_CHR( weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = chr_ref;
          }
          action = CapList[ChrList[weapon].model].weaponaction + 2;


          // Can it do it?
          allowedtoattack = btrue;
          if ( !pmad->actionvalid[action] || ChrList[weapon].reloadtime > 0 ||
               ( CapList[ChrList[weapon].model].needskillidtouse && !check_skills( chr_ref, CapList[ChrList[weapon].model].idsz[IDSZ_SKILL] ) ) )
          {
            allowedtoattack = bfalse;
            if ( ChrList[weapon].reloadtime == 0 )
            {
              // This character can't use this weapon
              ChrList[weapon].reloadtime = 50;
              if ( pchr->staton )
              {
                // Tell the player that they can't use this weapon
                debug_message( 1, "%s can't use this item...", pchr->name );
              }
            }
          }
          if ( action == ACTION_DC )
          {
            allowedtoattack = bfalse;
            if ( ChrList[weapon].reloadtime == 0 )
            {
              ChrList[weapon].aistate.alert |= ALERT_USED;
            }
          }


          if ( allowedtoattack )
          {
            // Rearing imount
            if ( VALID_CHR( imount ) )
            {
              allowedtoattack = CapList[ChrList[imount].model].ridercanattack;
              if ( ChrList[imount].ismount && ChrList[imount].alive && !ChrList[imount].isplayer && ChrList[imount].action.ready )
              {
                if (( action != ACTION_PC || !allowedtoattack ) && pchr->action.ready )
                {
                  play_action( imount, ( ACTION )( ACTION_UC + ( rand() &1 ) ), bfalse );
                  ChrList[imount].aistate.alert |= ALERT_USED;
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
              if ( pchr->action.ready && pmad->actionvalid[action] )
              {
                // Check mana cost
                if ( pchr->mana_fp8 >= ChrList[weapon].manacost || pchr->canchannel )
                {
                  cost_mana( chr_ref, ChrList[weapon].manacost, weapon );
                  // Check life healing
                  pchr->life_fp8 += ChrList[weapon].lifeheal;
                  if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;
                  ready = ( action == ACTION_PC );
                  action += rand() & 1;
                  play_action( chr_ref, action, ready );
                  if ( weapon != chr_ref )
                  {
                    // Make the weapon attack too
                    play_action( weapon, ACTION_MJ, bfalse );
                    ChrList[weapon].aistate.alert |= ALERT_USED;
                  }
                  else
                  {
                    // Flag for unarmed attack
                    pstate->alert |= ALERT_USED;
                  }
                }
              }
            }
          }
        }
      }
    }



    // Integrate the z direction
    if ( 0.0f != pchr->flyheight )
    {
      if ( level < 0 ) pchr->accum_pos.z += level - pchr->pos.z; // Don't fall in pits...
      pchr->accum_acc.z += ( level + pchr->flyheight - pchr->pos.z ) * FLYDAMPEN;

      vert_friction = 1.0;
    }
    else if ( pchr->pos.z > level + PLATTOLERANCE )
    {
      pchr->accum_acc.z += gravity;
    }
    else
    {
      float lerp_normal, lerp_tang;
      lerp_tang = ( level + PLATTOLERANCE - pchr->pos.z ) / ( float ) PLATTOLERANCE;
      if ( lerp_tang > 1.0f ) lerp_tang = 1.0f;
      if ( lerp_tang < 0.0f ) lerp_tang = 0.0f;

      // fix to make sure characters will hit the ground softly, but in a reasonable time
      lerp_normal = 1.0 - lerp_tang;
      if ( lerp_normal > 1.0f ) lerp_normal = 1.0f;
      if ( lerp_normal < 0.2f ) lerp_normal = 0.2f;

      // slippy hills make characters slide
      if ( pchr->weight > 0 && GWater.iswater && !pchr->inwater && INVALID_FAN != pchr->onwhichfan && mesh_has_some_bits( pchr->onwhichfan, MESHFX_SLIPPY ) )
      {
        pchr->accum_acc.x -= nrm.x * gravity * lerp_tang * hillslide;
        pchr->accum_acc.y -= nrm.y * gravity * lerp_tang * hillslide;
        pchr->accum_acc.z += nrm.z * gravity * lerp_normal;
      }
      else
      {
        pchr->accum_acc.z += gravity * lerp_normal;
      };
    }

    // Apply friction for next time
    pchr->accum_acc.x -= ( 1.0f - horiz_friction ) * pchr->vel.x;
    pchr->accum_acc.y -= ( 1.0f - horiz_friction ) * pchr->vel.y;
    pchr->accum_acc.z -= ( 1.0f - vert_friction ) * pchr->vel.z;

    // reset the jump
    pchr->jumpready  = grounded || pchr->inwater;
    if ( pchr->jumptime == 0.0f )
    {
      if ( grounded && pchr->jumpnumber < pchr->jumpnumberreset )
      {
        pchr->jumpnumber = pchr->jumpnumberreset;
        pchr->jumptime   = DELAY_JUMP;
      }
      else if ( pchr->inwater && pchr->jumpnumber < 1 )
      {
        // "Swimming"
        pchr->jumpready  = btrue;
        pchr->jumptime   = DELAY_JUMP / 3.0f;
        pchr->jumpnumber += 1;
      }
    };

    // check to see if it can jump
    dojumptimer = btrue;
    if ( grounded )
    {
      // only slippy, non-flat surfaces don't allow jumps
      if ( INVALID_FAN != pchr->onwhichfan && mesh_has_some_bits( pchr->onwhichfan, MESHFX_SLIPPY ) )
      {
        if ( !maptwistflat[twist] )
        {
          pchr->jumpready = bfalse;
          dojumptimer       = bfalse;
        };
      }
    }

    if ( dojumptimer )
    {
      pchr->jumptime  -= dUpdate;
      if ( pchr->jumptime < 0 ) pchr->jumptime = 0.0f;
    }

    // Characters with sticky butts lie on the surface of the mesh
    if ( grounded && ( pchr->stickybutt || !pchr->alive ) )
    {
      pchr->mapturn_lr = pchr->mapturn_lr * 0.9 + maptwist_lr[twist] * 0.1;
      pchr->mapturn_ud = pchr->mapturn_ud * 0.9 + maptwist_ud[twist] * 0.1;
    }

    // Animate the character

    // do pancake anim
    pchr->pancakevel.x *= 0.90;
    pchr->pancakevel.y *= 0.90;
    pchr->pancakevel.z *= 0.90;

    pchr->pancakepos.x += pchr->pancakevel.x * dUpdate;
    pchr->pancakepos.y += pchr->pancakevel.y * dUpdate;
    pchr->pancakepos.z += pchr->pancakevel.z * dUpdate;

    if ( pchr->pancakepos.x < 0 ) { pchr->pancakepos.x = 0.001; pchr->pancakevel.x *= -0.5f; };
    if ( pchr->pancakepos.y < 0 ) { pchr->pancakepos.y = 0.001; pchr->pancakevel.y *= -0.5f; };
    if ( pchr->pancakepos.z < 0 ) { pchr->pancakepos.z = 0.001; pchr->pancakevel.z *= -0.5f; };

    pchr->pancakevel.x += ( 1.0f - pchr->pancakepos.x ) * dUpdate / 10.0f;
    pchr->pancakevel.y += ( 1.0f - pchr->pancakepos.y ) * dUpdate / 10.0f;
    pchr->pancakevel.z += ( 1.0f - pchr->pancakepos.z ) * dUpdate / 10.0f;

    // so the model's animation
    pchr->anim.flip += dUpdate * 0.25;
    while ( pchr->anim.flip > 0.25f )
    {
      // convert flip into lip
      pchr->anim.flip -= 0.25f;
      pchr->anim.lip_fp8 += 64;

      // handle the mad fx
      if ( pchr->anim.lip_fp8 == 192 )
      {
        // Check frame effects
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_ACTLEFT ) )
          character_swipe( chr_ref, SLOT_LEFT );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_ACTRIGHT ) )
          character_swipe( chr_ref, SLOT_RIGHT );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_GRABLEFT ) )
          character_grab_stuff( chr_ref, SLOT_LEFT, bfalse );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_GRABRIGHT ) )
          character_grab_stuff( chr_ref, SLOT_RIGHT, bfalse );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_CHARLEFT ) )
          character_grab_stuff( chr_ref, SLOT_LEFT, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_CHARRIGHT ) )
          character_grab_stuff( chr_ref, SLOT_RIGHT, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_DROPLEFT ) )
          detach_character_from_mount( chr_get_holdingwhich( chr_ref, SLOT_LEFT ), bfalse, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_DROPRIGHT ) )
          detach_character_from_mount( chr_get_holdingwhich( chr_ref, SLOT_RIGHT ), bfalse, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_POOF ) && !pchr->isplayer )
          pchr->gopoof = btrue;
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_FOOTFALL ) )
        {
          if ( INVALID_SOUND != CapList[imdl].footfallsound )
          {
            float volume = ( ABS( pchr->vel.x ) +  ABS( pchr->vel.y ) ) / CapList[imdl].spd_sneak;
            play_sound( MIN( 1.0f, volume ), pchr->pos, CapList[imdl].wavelist[CapList[imdl].footfallsound], 0, imdl, CapList[imdl].footfallsound );
          }
        }
      }

      // change frames
      if ( pchr->anim.lip_fp8 == 0 )
      {
        // Change frames
        pchr->anim.last = pchr->anim.next;
        pchr->anim.next++;

        if ( pchr->anim.next >= pmad->actionend[pchr->action.now] )
        {
          // Action finished
          if ( pchr->action.keep )
          {
            // Keep the last frame going
            pchr->anim.next = pchr->anim.last;
          }
          else if ( !pchr->action.loop )
          {
            // Go on to the next action
            pchr->action.now  = pchr->action.next;
            pchr->action.next = ACTION_DA;

            pchr->anim.next = pmad->actionstart[pchr->action.now];
          }
          else if ( VALID_CHR(imount) )
          {
            // See if the character is mounted...
            pchr->action.now = ACTION_MI;

            pchr->anim.next = pmad->actionstart[pchr->action.now];
          }

          pchr->action.ready = btrue;
        }
      }

    };



    // Do "Be careful!" delay
    pchr->carefultime -= dUpdate;
    if ( pchr->carefultime <= 0 ) pchr->carefultime = 0;


    // Get running, walking, sneaking, or dancing, from speed
    if ( !pchr->action.keep && !pchr->action.loop )
    {
      framelip = pmad->framelip[pchr->anim.next];  // 0 - 15...  Way through animation
      if ( pchr->action.ready && pchr->anim.lip_fp8 == 0 && grounded && pchr->flyheight == 0 && ( framelip&7 ) < 2 )
      {
        // Do the motion stuff
        speed = ABS( pchr->vel.x ) + ABS( pchr->vel.y );
        if ( speed < pchr->spd_sneak )
        {
          //                        pchr->action.next = ACTION_DA;
          // Do boredom
          pchr->boretime -= dUpdate;
          if ( pchr->boretime <= 0 ) pchr->boretime = 0;

          if ( pchr->boretime <= 0 )
          {
            pstate->alert |= ALERT_BORED;
            pchr->boretime = DELAY_BORE;
          }
          else
          {
            // Do standstill
            if ( pchr->action.now > ACTION_DD )
            {
              pchr->action.now = ACTION_DA;
              pchr->anim.next = pmad->actionstart[pchr->action.now];
            }
          }
        }
        else
        {
          pchr->boretime = DELAY_BORE;
          if ( speed < pchr->spd_walk )
          {
            pchr->action.next = ACTION_WA;
            if ( pchr->action.now != ACTION_WA )
            {
              pchr->anim.next = pmad->frameliptowalkframe[LIPT_WA][framelip];
              pchr->action.now = ACTION_WA;
            }
          }
          else
          {
            if ( speed < pchr->spd_run )
            {
              pchr->action.next = ACTION_WB;
              if ( pchr->action.now != ACTION_WB )
              {
                pchr->anim.next = pmad->frameliptowalkframe[LIPT_WB][framelip];
                pchr->action.now = ACTION_WB;
              }
            }
            else
            {
              pchr->action.next = ACTION_WC;
              if ( pchr->action.now != ACTION_WC )
              {
                pchr->anim.next = pmad->frameliptowalkframe[LIPT_WC][framelip];
                pchr->action.now = ACTION_WC;
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
  CHR_REF currentcharacter = MAXCHR, lastcharacter = MAXCHR, tmpchr = MAXCHR;
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
  currentcharacter = MAXCHR;
  snprintf( newloadname, sizeof( newloadname ), "%s%s/%s", modname, CData.gamedat_dir, CData.spawn_file );
  fileread = fs_fileOpen( PRI_FAIL, "setup_characters()", newloadname, "r" );
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

      ChrList[lastcharacter].money += money;
      if ( ChrList[lastcharacter].money > MAXMONEY )  ChrList[lastcharacter].money = MAXMONEY;
      if ( ChrList[lastcharacter].money < 0 )  ChrList[lastcharacter].money = 0;
      ChrList[lastcharacter].aistate.content = content;
      ChrList[lastcharacter].passage = passage;
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
            ChrList[lastcharacter].aistate.alert |= ALERT_GRABBED;                       // Make spellbooks change

            // fake that it was grabbed by the left hand
            ChrList[lastcharacter].attachedto = VALIDATE_CHR(currentcharacter);  // Make grab work
            ChrList[lastcharacter].inwhichslot = SLOT_INVENTORY;
            let_character_think( lastcharacter, 1.0f );                     // Empty the grabbed messages

            // restore the proper attachment and slot variables
            ChrList[lastcharacter].attachedto = MAXCHR;                          // Fix grab
            ChrList[lastcharacter].inwhichslot = SLOT_NONE;
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
            if ( CapList[ChrList[lastcharacter].model].importslot == localslot[tnc] )
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
      if ( !ChrList[lastcharacter].isplayer )
      {
        // Let the character gain levels
        level -= 1;
        while ( ChrList[lastcharacter].experiencelevel < level && ChrList[lastcharacter].experience < MAXXP )
        {
          give_experience( lastcharacter, 100, XP_DIRECT );
        }
      }
      if ( ghost )  // Outdated, should be removed.
      {
        // Make the character a ghost !!!BAD!!!  Can do with enchants
        ChrList[lastcharacter].alpha_fp8 = 128;
        ChrList[lastcharacter].bumpstrength = CapList[ChrList[lastcharacter].model].bumpstrength * FP8_TO_FLOAT( ChrList[lastcharacter].alpha_fp8 );
        ChrList[lastcharacter].light_fp8 = 255;
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
  if ( !VALID_PLA( player ) || INBITS_NONE == PlaList[player].device ) return;

  // Make life easier
  character = pla_get_character( player );
  device    = PlaList[player].device;

  // Clear the player's latch buffers
  PlaList[player].latch.b = 0;
  PlaList[player].latch.x *= mous.sustain;
  PlaList[player].latch.y *= mous.sustain;

  // Mouse routines
  if ( HAS_SOME_BITS( device, INBITS_MOUS ) && mous.on )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_mouse_is_pressed( CONTROL_CAMERA ) )   // Don't allow movement in camera control mode
    {
      inputx = 0;
      inputy = 0;
      dist = mous.dlatch.x * mous.dlatch.x + mous.dlatch.y * mous.dlatch.y;

      if ( dist > 0.0 )
      {
        dist = sqrt( dist );
        inputx = ( float ) mous.dlatch.x / ( mous.sense + dist );
        inputy = ( float ) mous.dlatch.y / ( mous.sense + dist );
      }
      if ( CData.autoturncamera == 255 && control_mouse_is_pressed( CONTROL_CAMERA ) == 0 )  inputx = 0;

      turncos = ((Uint16)GCamera.turn_lr) >> 2;
      turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
      newx = ( inputx * turntosin[turncos] + inputy * turntosin[turnsin] );
      newy = (-inputx * turntosin[turnsin] + inputy * turntosin[turncos] );
    }

    PlaList[player].latch.x += newx * mous.cover * 5;
    PlaList[player].latch.y += newy * mous.cover * 5;

    // Read buttons
    if ( control_mouse_is_pressed( CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !ChrList[character].alive )
      {
        PlaList[player].latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        PlaList[player].latch.b |= LATCHBUTTON_JUMP;
      }
    };

    if ( control_mouse_is_pressed( CONTROL_LEFT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_LEFT;

    if ( control_mouse_is_pressed( CONTROL_LEFT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_mouse_is_pressed( CONTROL_LEFT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_RIGHT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_mouse_is_pressed( CONTROL_RIGHT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKRIGHT;
  }


  // Joystick A routines
  if ( HAS_SOME_BITS( device, INBITS_JOYA ) && joy[0].on )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_joy_is_pressed( 0, CONTROL_CAMERA ) )
    {
      inputx = joy[0].latch.x;
      inputy = joy[0].latch.y;
      dist = joy[0].latch.x * joy[0].latch.x + joy[0].latch.y * joy[0].latch.y;
      if ( dist > 1.0 )
      {
        dist = sqrt( dist );
        inputx /= dist;
        inputy /= dist;
      }
      if ( CData.autoturncamera == 255 && !control_joy_is_pressed( 0, CONTROL_CAMERA ) )  inputx = 0;

      turncos = ((Uint16)GCamera.turn_lr) >> 2;
      turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
      newx = ( inputx * turntosin[turncos] + inputy * turntosin[turnsin] );
      newy = ( -inputx * turntosin[turnsin] + inputy * turntosin[turncos] );
    }

    PlaList[player].latch.x += newx * mous.cover;
    PlaList[player].latch.y += newy * mous.cover;

    // Read buttons
    if ( control_joy_is_pressed( 0, CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !ChrList[character].alive )
      {
        PlaList[player].latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        PlaList[player].latch.b |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_joy_is_pressed( 0, CONTROL_LEFT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_LEFT;

    if ( control_joy_is_pressed( 0, CONTROL_LEFT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_joy_is_pressed( 0, CONTROL_LEFT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_joy_is_pressed( 0, CONTROL_RIGHT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_RIGHT;

    if ( control_joy_is_pressed( 0, CONTROL_RIGHT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_joy_is_pressed( 0, CONTROL_RIGHT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKRIGHT;
  }


  // Joystick B routines
  if ( HAS_SOME_BITS( device, INBITS_JOYB ) && joy[1].on )
  {
    // Movement
    newx = 0;
    newy = 0;
    if ( CData.autoturncamera == 255 || !control_joy_is_pressed( 1, CONTROL_CAMERA ) )
    {
      inputx = joy[1].latch.x;
      inputy = joy[1].latch.y;
      dist = joy[1].latch.x * joy[1].latch.x + joy[1].latch.y * joy[1].latch.y;
      if ( dist > 1.0 )
      {
        dist = sqrt( dist );
        inputx = joy[1].latch.x / dist;
        inputy = joy[1].latch.y / dist;
      }
      if ( CData.autoturncamera == 255 && !control_joy_is_pressed( 1, CONTROL_CAMERA ) )  inputx = 0;

      turncos = ((Uint16)GCamera.turn_lr) >> 2;
      turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
      newx = ( inputx * turntosin[turncos] + inputy * turntosin[turnsin] );
      newy = ( -inputx * turntosin[turnsin] + inputy * turntosin[turncos] );
    }

    PlaList[player].latch.x += newx * mous.cover;
    PlaList[player].latch.y += newy * mous.cover;

    // Read buttons
    if ( control_joy_is_pressed( 1, CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !ChrList[character].alive )
      {
        PlaList[player].latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        PlaList[player].latch.b |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_joy_is_pressed( 1, CONTROL_LEFT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_LEFT;

    if ( control_joy_is_pressed( 1, CONTROL_LEFT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_joy_is_pressed( 1, CONTROL_LEFT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_joy_is_pressed( 1, CONTROL_RIGHT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_RIGHT;

    if ( control_joy_is_pressed( 1, CONTROL_RIGHT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_joy_is_pressed( 1, CONTROL_RIGHT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKRIGHT;
  }

  // Keyboard routines
  if ( HAS_SOME_BITS( device, INBITS_KEYB ) && keyb.on )
  {
    // Movement
    newx = 0;
    newy = 0;
    inputx = inputy = 0;
    if ( control_key_is_pressed( KEY_RIGHT ) ) inputx += 1;
    if ( control_key_is_pressed( KEY_LEFT  ) ) inputx -= 1;
    if ( control_key_is_pressed( KEY_DOWN  ) ) inputy += 1;
    if ( control_key_is_pressed( KEY_UP    ) ) inputy -= 1;
    dist = inputx * inputx + inputy * inputy;
    if ( dist > 1.0 )
    {
      dist = sqrt( dist );
      inputx /= dist;
      inputy /= dist;
    }
    if ( CData.autoturncamera == 255 && numlocalpla == 1 )  inputx = 0;


    turncos = ((Uint16)GCamera.turn_lr) >> 2;
    turnsin = ( turncos + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
    newx = ( inputx * turntosin[turncos]  + inputy * turntosin[turnsin] );
    newy = ( -inputx * turntosin[turnsin] + inputy * turntosin[turncos] );

    PlaList[player].latch.x += newx * mous.cover;
    PlaList[player].latch.y += newy * mous.cover;

    // Read buttons
    if ( control_key_is_pressed( CONTROL_JUMP ) )
    {
      if (( respawnanytime && somelocalpladead ) || ( alllocalpladead && respawnvalid ) && VALID_CHR( character ) && !ChrList[character].alive )
      {
        PlaList[player].latch.b |= LATCHBUTTON_RESPAWN;
      }
      else
      {
        PlaList[player].latch.b |= LATCHBUTTON_JUMP;
      }
    }

    if ( control_key_is_pressed( CONTROL_LEFT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_LEFT;

    if ( control_key_is_pressed( CONTROL_LEFT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTLEFT;

    if ( control_key_is_pressed( CONTROL_LEFT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKLEFT;

    if ( control_key_is_pressed( CONTROL_RIGHT_USE ) )
      PlaList[player].latch.b |= LATCHBUTTON_RIGHT;

    if ( control_key_is_pressed( CONTROL_RIGHT_GET ) )
      PlaList[player].latch.b |= LATCHBUTTON_ALTRIGHT;

    if ( control_key_is_pressed( CONTROL_RIGHT_PACK ) )
      PlaList[player].latch.b |= LATCHBUTTON_PACKRIGHT;
  }

  dist = PlaList[player].latch.x * PlaList[player].latch.x + PlaList[player].latch.y * PlaList[player].latch.y;
  if ( dist > 1 )
  {
    dist = sqrt( dist );
    PlaList[player].latch.x /= dist;
    PlaList[player].latch.y /= dist;
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
float get_one_level( CHR_REF chr_ref )
{
  float level;
  Uint16 platform;

  // return a default for invalid characters
  if ( !VALID_CHR( chr_ref ) ) return bfalse;

  // return the cached value for pre-calculated characters
  if ( ChrList[chr_ref].levelvalid ) return ChrList[chr_ref].level;

  //get the base level
  ChrList[chr_ref].onwhichfan = mesh_get_fan( ChrList[chr_ref].pos );
  level = mesh_get_level( ChrList[chr_ref].onwhichfan, ChrList[chr_ref].pos.x, ChrList[chr_ref].pos.y, ChrList[chr_ref].waterwalk );

  // if there is a platform, choose whichever is higher
  platform = chr_get_onwhichplatform( chr_ref );
  if ( VALID_CHR( platform ) )
  {
    float ftmp = ChrList[platform].bmpdata.cv.z_max;
    level = MAX( level, ftmp );
  }

  ChrList[chr_ref].level      = level;
  ChrList[chr_ref].levelvalid = btrue;

  return ChrList[chr_ref].level;
};

//--------------------------------------------------------------------------------------------
void get_all_levels( void )
{
  CHR_REF chr_ref;

  // Initialize all the objects
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( chr_ref ) ) continue;

    ChrList[chr_ref].onwhichfan = INVALID_FAN;
    ChrList[chr_ref].levelvalid = bfalse;
  };

  // do the levels
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( chr_ref ) || ChrList[chr_ref].levelvalid ) continue;
    get_one_level( chr_ref );
  }

}


//--------------------------------------------------------------------------------------------
void make_onwhichfan( void )
{
  // ZZ> This function figures out which fan characters are on and sets their level
  CHR_REF chr_ref;
  int ripand;

  float  splashstrength = 1.0f, ripplesize = 1.0f, ripplestrength = 0.0f;
  bool_t is_inwater    = bfalse;
  bool_t is_underwater = bfalse;


  // Get levels every update
  get_all_levels();

  // Get levels every update
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( chr_ref ) ) continue;

    is_inwater = is_underwater = bfalse;
    splashstrength = 0.0f;
    ripplesize = 0.0f;
    if ( INVALID_FAN != ChrList[chr_ref].onwhichfan && mesh_has_some_bits( ChrList[chr_ref].onwhichfan, MESHFX_WATER ) )
    {
      splashstrength = ChrList[chr_ref].bmpdata.calc_size_big / 45.0f * ChrList[chr_ref].bmpdata.calc_size / 30.0f;
      if ( ChrList[chr_ref].vel.z > 0.0f ) splashstrength *= 0.5;
      splashstrength *= ABS( ChrList[chr_ref].vel.z ) / 10.0f;
      splashstrength *= ChrList[chr_ref].bumpstrength;
      if ( ChrList[chr_ref].pos.z < GWater.surfacelevel )
      {
        is_inwater = btrue;
      }

      ripplesize = ( ChrList[chr_ref].bmpdata.calc_size + ChrList[chr_ref].bmpdata.calc_size_big ) * 0.5f;
      if ( ChrList[chr_ref].bmpdata.cv.z_max < GWater.surfacelevel )
      {
        is_underwater = btrue;
      }

      // scale the ripple strength
      ripplestrength = - ( ChrList[chr_ref].bmpdata.cv.z_min - GWater.surfacelevel ) * ( ChrList[chr_ref].bmpdata.cv.z_max - GWater.surfacelevel );
      ripplestrength /= 0.75f * ChrList[chr_ref].bmpdata.calc_height * ChrList[chr_ref].bmpdata.calc_height;
      ripplestrength *= ripplesize / 37.5f * ChrList[chr_ref].bumpstrength;
      ripplestrength = MAX( 0.0f, ripplestrength );
    };

    // splash stuff
    if ( ChrList[chr_ref].inwater != is_inwater && splashstrength > 0.1f )
    {
      vect3 prt_pos = {ChrList[chr_ref].pos.x, ChrList[chr_ref].pos.y, GWater.surfacelevel + RAISE};
      Uint16 prt_index;

      // Splash
      prt_index = spawn_one_particle( splashstrength, prt_pos, 0, MAXMODEL, PRTPIP_SPLASH, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );

      // scale the size of the particle
      PrtList[prt_index].size_fp8 *= splashstrength;

      // scale the animation speed so that velocity appears the same
      if ( 0 != PrtList[prt_index].imageadd_fp8 && 0 != PrtList[prt_index].sizeadd_fp8 )
      {
        splashstrength = sqrt( splashstrength );
        PrtList[prt_index].imageadd_fp8 /= splashstrength;
        PrtList[prt_index].sizeadd_fp8  /= splashstrength;
      }
      else
      {
        PrtList[prt_index].imageadd_fp8 /= splashstrength;
        PrtList[prt_index].sizeadd_fp8  /= splashstrength;
      }


      ChrList[chr_ref].inwater = is_inwater;
      if ( GWater.iswater && is_inwater )
      {
        ChrList[chr_ref].aistate.alert |= ALERT_INWATER;
      }
    }
    else if ( is_inwater && ripplestrength > 0.0f )
    {
      // Ripples
      ripand = ((( int ) ChrList[chr_ref].vel.x ) != 0 ) | ((( int ) ChrList[chr_ref].vel.y ) != 0 );
      ripand = RIPPLEAND >> ripand;
      if ( 0 == ( wldframe&ripand ) )
      {
        vect3  prt_pos = {ChrList[chr_ref].pos.x, ChrList[chr_ref].pos.y, GWater.surfacelevel};
        Uint16 prt_index;

        prt_index = spawn_one_particle( ripplestrength, prt_pos, 0, MAXMODEL, PRTPIP_RIPPLE, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );

        // scale the size of the particle
        PrtList[prt_index].size_fp8 *= ripplesize;

        // scale the animation speed so that velocity appears the same
        if ( 0 != PrtList[prt_index].imageadd_fp8 && 0 != PrtList[prt_index].sizeadd_fp8 )
        {
          ripplesize = sqrt( ripplesize );
          PrtList[prt_index].imageadd_fp8 /= ripplesize;
          PrtList[prt_index].sizeadd_fp8  /= ripplesize;
        }
        else
        {
          PrtList[prt_index].imageadd_fp8 /= ripplesize;
          PrtList[prt_index].sizeadd_fp8  /= ripplesize;
        }
      }
    }

    // damage tile stuff
    if ( mesh_has_some_bits( ChrList[chr_ref].onwhichfan, MESHFX_DAMAGE ) && ChrList[chr_ref].pos.z <= GWater.surfacelevel + DAMAGERAISE )
    {
      Uint8 loc_damagemodifier;
      CHR_REF imount;

      // augment the rider's damage immunity with the mount's
      loc_damagemodifier = ChrList[chr_ref].damagemodifier_fp8[GTile_Dam.type];
      imount = chr_get_attachedto(chr_ref);
      if ( VALID_CHR(imount) )
      {
        Uint8 modbits1, modbits2, modshift1, modshift2;
        Uint8 tmp_damagemodifier;

        tmp_damagemodifier = ChrList[imount].damagemodifier_fp8[GTile_Dam.type];

        modbits1  = loc_damagemodifier & (~DAMAGE_SHIFT);
        modshift1 = loc_damagemodifier & DAMAGE_SHIFT;

        modbits2  = tmp_damagemodifier & (~DAMAGE_SHIFT);
        modshift2 = tmp_damagemodifier & DAMAGE_SHIFT;

        loc_damagemodifier = (modbits1 | modbits2) | MAX(modshift1, modshift2);
      }

      // DAMAGE_SHIFT means they're pretty well immune
      if ( !HAS_ALL_BITS(loc_damagemodifier, DAMAGE_SHIFT ) && !ChrList[chr_ref].invictus )  
      {
        if ( ChrList[chr_ref].damagetime == 0 )
        {
          PAIR ptemp = {GTile_Dam.amount, 1};
          damage_character( chr_ref, 32768, &ptemp, GTile_Dam.type, TEAM_DAMAGE, chr_get_aibumplast( chr_ref ), DAMFX_BLOC | DAMFX_ARMO );
          ChrList[chr_ref].damagetime = DELAY_DAMAGETILE;
        }

        if ( GTile_Dam.parttype != MAXPRTPIP && ( wldframe&GTile_Dam.partand ) == 0 )
        {
          spawn_one_particle( 1.0f, ChrList[chr_ref].pos,
                              0, MAXMODEL, GTile_Dam.parttype, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
        }

      }

      if ( ChrList[chr_ref].reaffirmdamagetype == GTile_Dam.type )
      {
        if (( wldframe&TILEREAFFIRMAND ) == 0 )
          reaffirm_attached_particles( chr_ref );
      }
    }


  }

}

//--------------------------------------------------------------------------------------------
bool_t remove_from_platform( CHR_REF object_ref )
{
  Uint16 platform;
  if ( !VALID_CHR( object_ref ) ) return bfalse;

  platform  = chr_get_onwhichplatform( object_ref );
  if ( !VALID_CHR( platform ) ) return bfalse;

  if ( ChrList[object_ref].weight > 0.0f )
    ChrList[platform].weight -= ChrList[object_ref].weight;

  ChrList[object_ref].onwhichplatform = MAXCHR;
  ChrList[object_ref].level           = ChrList[platform].level;

  if ( ChrList[object_ref].isplayer && CData.DevMode )
  {
    debug_message( 1, "removed %s(%s) from platform", ChrList[object_ref].name, CapList[ChrList[object_ref].model].classname );
  }


  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_to_platform( CHR_REF object_ref, Uint16 platform )
{
  remove_from_platform( object_ref );

  if ( !VALID_CHR( object_ref ) || !VALID_CHR( platform ) ) return
      bfalse;

  if ( !ChrList[platform].bmpdata.calc_is_platform )
    return bfalse;

  ChrList[object_ref].onwhichplatform  = platform;
  if ( ChrList[object_ref].weight > 0.0f )
  {
    ChrList[platform].holdingweight += ChrList[object_ref].weight;
  }

  ChrList[object_ref].jumpready  = btrue;
  ChrList[object_ref].jumpnumber = ChrList[object_ref].jumpnumberreset;

  ChrList[object_ref].level = ChrList[platform].bmpdata.cv.z_max;

  if ( ChrList[object_ref].isplayer )
  {
    debug_message( 1, "attached %s(%s) to platform", ChrList[object_ref].name, CapList[ChrList[object_ref].model].classname );
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
void create_bumplists()
{
  CHR_REF chr_ref, entry_ref;
  PRT_REF prt_ref;
  Uint32  fanblock;
  Uint8   hidestate;

  // Clear the lists
  for ( fanblock = 0; fanblock < bumplist.num_blocks; fanblock++ )
  {
    bumplist.valid             = bfalse;
    bumplist.num_chr[fanblock] = 0;
    bumplist.chr[fanblock]     = MAXCHR;
    bumplist.num_prt[fanblock] = 0;
    bumplist.prt[fanblock]     = MAXPRT;
  }

  // Fill 'em back up
  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    int ix, ix_min, ix_max;
    int iy, iy_min, iy_max;

    // ignore invalid characters and objects that are packed away
    if ( !VALID_CHR( chr_ref ) || chr_in_pack( chr_ref ) || ChrList[chr_ref].gopoof ) continue;

    // do not include hidden objects
    hidestate = CapList[ChrList[chr_ref].model].hidestate;
    if ( hidestate != NOHIDE && hidestate == ChrList[chr_ref].aistate.state ) continue;

    ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( ChrList[chr_ref].bmpdata.cv.x_min ) );
    ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( ChrList[chr_ref].bmpdata.cv.x_max ) );
    iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( ChrList[chr_ref].bmpdata.cv.y_min ) );
    iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( ChrList[chr_ref].bmpdata.cv.y_max ) );

    for ( ix = ix_min; ix <= ix_max; ix++ )
    {
      for ( iy = iy_min; iy <= iy_max; iy++ )
      {
        fanblock = mesh_convert_block( ix, iy );
        if ( INVALID_FAN == fanblock ) continue;

        // Insert before any other characters on the block
        entry_ref = bumplist.chr[fanblock];
        ChrList[chr_ref].bumpnext = entry_ref;
        bumplist.chr[fanblock]    = chr_ref;
        bumplist.num_chr[fanblock]++;
      }
    }
  };


  for ( prt_ref = 0; prt_ref < MAXPRT; prt_ref++ )
  {
    // ignore invalid particles
    if ( !VALID_PRT( prt_ref ) || PrtList[prt_ref].gopoof ) continue;

    fanblock = mesh_get_block( PrtList[prt_ref].pos );
    if ( INVALID_FAN == fanblock ) continue;

    // Insert before any other particles on the block
    entry_ref = bumplist.prt[fanblock];
    PrtList[prt_ref].bumpnext = entry_ref;
    bumplist.prt[fanblock] = prt_ref;
    bumplist.num_prt[fanblock]++;
  }

  bumplist.valid = btrue;
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
bool_t chr_is_inside( CHR_REF chra_ref, float lerp, CHR_REF chrb_ref, bool_t exclude_vert )
{
  // BB > Find whether an active point of chra_ref is "inside" chrb_ref's bounding volume.
  //      Abstraction of the old algorithm to see whether a character cpold be "above" another

  float ftmp;
  BData * ba, * bb;

  if( !VALID_CHR(chra_ref) || !VALID_CHR(chrb_ref) ) return bfalse;

  ba = &(ChrList[chra_ref].bmpdata);
  bb = &(ChrList[chrb_ref].bmpdata);

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
bool_t chr_do_collision( CHR_REF chra_ref, CHR_REF chrb_ref, bool_t exclude_height, CVolume * cv)
{
  // BB > use the bounding boxes to determine whether a collision has occurred.
  //      there are currently 3 levels of collision detection.
  //      level 1 - the basic square axis-aligned bounding box
  //      level 2 - the octagon bounding box calculated from the actual vertex positions
  //      level 3 - an "octree" of bounding bounding boxes calculated from the actual trianglr positions
  //  the level is chosen by the global variable chrcollisionlevel

  bool_t retval = bfalse;

  // set the minimum bumper level for object a
  if(ChrList[chra_ref].bmpdata.cv.level < 1)
  {
    md2_calculate_bumpers(chra_ref, 1);
  }

  // set the minimum bumper level for object b
  if(ChrList[chrb_ref].bmpdata.cv.level < 1)
  {
    md2_calculate_bumpers(chrb_ref, 1);
  }

  // find the simplest collision volume
  find_collision_volume( &(ChrList[chra_ref].pos), &(ChrList[chra_ref].bmpdata.cv), &(ChrList[chrb_ref].pos), &(ChrList[chrb_ref].bmpdata.cv), exclude_height, cv);

  if ( chrcollisionlevel>1 && retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(ChrList[chra_ref].bmpdata.cv.level < 2)
    {
      md2_calculate_bumpers(chra_ref, 2);
      was_refined = btrue;
    }

    // refine the bumper
    if(ChrList[chrb_ref].bmpdata.cv.level < 2)
    {
      md2_calculate_bumpers(chrb_ref, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(ChrList[chra_ref].pos), &(ChrList[chra_ref].bmpdata.cv), &(ChrList[chrb_ref].pos), &(ChrList[chrb_ref].bmpdata.cv), exclude_height, cv);
    };

    if(chrcollisionlevel>2 && retval)
    {
      was_refined = bfalse;

      // refine the bumper
      if(ChrList[chra_ref].bmpdata.cv.level < 3)
      {
        md2_calculate_bumpers(chra_ref, 3);
        was_refined = btrue;
      }

      // refine the bumper
      if(ChrList[chrb_ref].bmpdata.cv.level < 3)
      {
        md2_calculate_bumpers(chrb_ref, 3);
        was_refined = btrue;
      }

      assert(NULL != ChrList[chra_ref].bmpdata.cv_tree);
      assert(NULL != ChrList[chrb_ref].bmpdata.cv_tree);

      if(was_refined)
      {
        int cnt, tnc;
        CVolume cv3, tmp_cv;
        bool_t loc_retval;

        retval = bfalse;
        cv3.level = -1;
        for(cnt=0; cnt<8; cnt++)
        {
          if(-1 == (*ChrList[chra_ref].bmpdata.cv_tree)[cnt].level) continue;

          for(tnc=0; tnc<8; tnc++)
          {
            if(-1 == (*ChrList[chrb_ref].bmpdata.cv_tree)[cnt].level) continue;

            loc_retval = find_collision_volume( &(ChrList[chra_ref].pos), &((*ChrList[chra_ref].bmpdata.cv_tree)[cnt]), &(ChrList[chrb_ref].pos), &((*ChrList[chrb_ref].bmpdata.cv_tree)[cnt]), exclude_height, &tmp_cv);

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
bool_t prt_do_collision( CHR_REF chra_ref, PRT_REF prtb, bool_t exclude_height )
{
  bool_t retval = find_collision_volume( &(ChrList[chra_ref].pos), &(ChrList[chra_ref].bmpdata.cv), &(ChrList[prtb].pos), &(ChrList[prtb].bmpdata.cv), bfalse, NULL );

  if ( retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(ChrList[chra_ref].bmpdata.cv.level < 2)
    {
      md2_calculate_bumpers(chra_ref, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(ChrList[chra_ref].pos), &(ChrList[chra_ref].bmpdata.cv), &(ChrList[prtb].pos), &(ChrList[prtb].bmpdata.cv), bfalse, NULL );
    };
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void do_bumping( float dUpdate )
{
  // ZZ> This function sets handles characters hitting other characters or particles
  CHR_REF chra_ref, chrb_ref;
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
  for ( fanblock = 0; fanblock < bumplist.num_blocks; fanblock++ )
  {
    chrinblock = bumplist.num_chr[fanblock];
    prtinblock = bumplist.num_prt[fanblock];

    //// remove bad platforms
    //for ( cnt = 0, chra_ref = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra_ref );
    //      cnt++, chra_ref = chr_get_bumpnext( chra_ref ) )
    //{
    //  // detach character from invalid platforms
    //  chrb_ref  = chr_get_onwhichplatform( chra_ref );
    //  if ( VALID_CHR( chrb_ref ) )
    //  {
    //    if ( !chr_is_inside( chra_ref, 0.0f, chrb_ref, btrue ) ||
    //         ChrList[chrb_ref].bmpdata.cv.z_min  > ChrList[chrb_ref].bmpdata.cv.z_max - PLATTOLERANCE )
    //    {
    //      remove_from_platform( chra_ref );
    //    }
    //  }
    //};

    //// do attachments
    //for ( cnt = 0, chra_ref = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra_ref );
    //      cnt++, chra_ref = chr_get_bumpnext( chra_ref ) )
    //{
    //  // Do platforms (no interaction with held or mounted items)
    //  if ( chr_attached( chra_ref ) ) continue;

    //  for ( chrb_ref = chr_get_bumpnext( chra_ref ), tnc = cnt + 1;
    //        tnc < chrinblock && VALID_CHR( chrb_ref );
    //        tnc++, chrb_ref = chr_get_bumpnext( chrb_ref ) )
    //  {
    //    // do not put something on a platform that is being carried by someone
    //    if ( chr_attached( chrb_ref ) ) continue;

    //    // do not consider anything that is already a item/platform combo
    //    if ( ChrList[chra_ref].onwhichplatform == chrb_ref || ChrList[chrb_ref].onwhichplatform == chra_ref ) continue;

    //    if ( chr_is_inside( chra_ref, 0.0f, chrb_ref, btrue) )
    //    {
    //      // check for compatibility
    //      if ( ChrList[chrb_ref].bmpdata.calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( ChrList[chra_ref].pos.z > MAX( ChrList[chrb_ref].bmpdata.cv.z_min, ChrList[chrb_ref].bmpdata.cv.z_max - PLATTOLERANCE ) && ChrList[chra_ref].level < ChrList[chrb_ref].bmpdata.cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chra_ref, chrb_ref );
    //        }
    //      }
    //    }
    //
    //    if( chr_is_inside( chrb_ref, 0.0f, chra_ref, btrue) )
    //    {
    //      if ( ChrList[chra_ref].bmpdata.calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( ChrList[chrb_ref].pos.z > MAX( ChrList[chra_ref].bmpdata.cv.z_min, ChrList[chra_ref].bmpdata.cv.z_max - PLATTOLERANCE ) && ChrList[chrb_ref].level < ChrList[chra_ref].bmpdata.cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chrb_ref, chra_ref );
    //        }
    //      }
    //    }

    //  }
    //}

    //// Do mounting
    //for ( cnt = 0, chra_ref = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra_ref );
    //      cnt++, chra_ref = chr_get_bumpnext( chra_ref ) )
    //{
    //  if ( chr_attached( chra_ref ) ) continue;

    //  for ( chrb_ref = chr_get_bumpnext( chra_ref ), tnc = cnt + 1;
    //        tnc < chrinblock && VALID_CHR( chrb_ref );
    //        tnc++, chrb_ref = chr_get_bumpnext( chrb_ref ) )
    //  {

    //    // do not mount something that is being carried by someone
    //    if ( chr_attached( chrb_ref ) ) continue;

    //    if ( chr_is_inside( chra_ref, 0.0f, chrb_ref, btrue)   )
    //    {

    //      // Now see if either is on top the other like a platform
    //      if ( ChrList[chra_ref].pos.z > ChrList[chrb_ref].bmpdata.cv.z_max - PLATTOLERANCE && ChrList[chra_ref].pos.z < ChrList[chrb_ref].bmpdata.cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is A falling on B?
    //        if ( ChrList[chra_ref].vel.z < ChrList[chrb_ref].vel.z )
    //        {
    //          if ( ChrList[chra_ref].flyheight == 0 && ChrList[chra_ref].alive && MadList[ChrList[chra_ref].model].actionvalid[ACTION_MI] && !ChrList[chra_ref].isitem )
    //          {
    //            if ( ChrList[chrb_ref].alive && ChrList[chrb_ref].ismount && !chr_using_slot( chrb_ref, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chra_ref );
    //              if ( !attach_character_to_mount( chra_ref, chrb_ref, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                ChrList[chra_ref].aistate.alert |= ALERT_BUMPED;
    //                ChrList[chrb_ref].aistate.alert |= ALERT_BUMPED;
    //                ChrList[chra_ref].aistate.bumplast = chrb_ref;
    //                ChrList[chrb_ref].aistate.bumplast = chra_ref;
    //              };
    //            }
    //          }
    //        }
    //      }

    //    }

    //    if( chr_is_inside( chrb_ref, 0.0f, chra_ref, btrue)   )
    //    {
    //      if ( ChrList[chrb_ref].pos.z > ChrList[chra_ref].bmpdata.cv.z_max - PLATTOLERANCE && ChrList[chrb_ref].pos.z < ChrList[chra_ref].bmpdata.cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is B falling on A?
    //        if ( ChrList[chrb_ref].vel.z < ChrList[chra_ref].vel.z )
    //        {
    //          if ( ChrList[chrb_ref].flyheight == 0 && ChrList[chrb_ref].alive && MadList[ChrList[chrb_ref].model].actionvalid[ACTION_MI] && !ChrList[chrb_ref].isitem )
    //          {
    //            if ( ChrList[chra_ref].alive && ChrList[chra_ref].ismount && !chr_using_slot( chra_ref, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chrb_ref );
    //              if ( !attach_character_to_mount( chrb_ref, chra_ref, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                ChrList[chra_ref].aistate.alert |= ALERT_BUMPED;
    //                ChrList[chrb_ref].aistate.alert |= ALERT_BUMPED;
    //                ChrList[chra_ref].aistate.bumplast = chrb_ref;
    //                ChrList[chrb_ref].aistate.bumplast = chra_ref;
    //              };
    //            };
    //          }
    //        }
    //      }
    //    }
    //  }
    //}

    // do collisions
    for ( cnt = 0, chra_ref = bumplist.chr[fanblock];
          cnt < chrinblock && VALID_CHR( chra_ref );
          cnt++, chra_ref = chr_get_bumpnext( chra_ref ) )
    {
      float lerpa;
      lerpa = (ChrList[chra_ref].pos.z - ChrList[chra_ref].level) / PLATTOLERANCE;
      lerpa = CLIP(lerpa, 0, 1);

      apos = ChrList[chra_ref].pos;

      // don't do object-object collisions if they won't feel each other
      if ( ChrList[chra_ref].bumpstrength == 0.0f ) continue;

      // Do collisions (but not with attached items/characers)
      for ( chrb_ref = chr_get_bumpnext( chra_ref ), tnc = cnt + 1;
            tnc < chrinblock && VALID_CHR( chrb_ref );
            tnc++, chrb_ref = chr_get_bumpnext( chrb_ref ) )
      {
        CVolume cv;
        float lerpb;

        float bumpstrength = ChrList[chra_ref].bumpstrength * ChrList[chrb_ref].bumpstrength;

        // don't do object-object collisions if they won't feel eachother
        if ( bumpstrength == 0.0f ) continue;

        // do not collide with something you are already holding
        if ( chrb_ref == ChrList[chra_ref].attachedto || chra_ref == ChrList[chrb_ref].attachedto ) continue;

        // do not collide with a your platform
        if ( chrb_ref == ChrList[chra_ref].onwhichplatform || chra_ref == ChrList[chrb_ref].onwhichplatform ) continue;

        bpos = ChrList[chrb_ref].pos;

        lerpb = (ChrList[chrb_ref].pos.z - ChrList[chrb_ref].level) / PLATTOLERANCE;
        lerpb = CLIP(lerpb, 0, 1);

        if ( chr_do_collision( chra_ref, chrb_ref, bfalse, &cv) )
        {
          vect3 depth, ovlap, nrm, diffa, diffb;
          float ftmp, dotprod, pressure;
          float cr, m0, m1, psum, msum, udif, u0, u1, ln_cr;
          bool_t bfound;

          depth.x = (cv.x_max - cv.x_min);
          ovlap.x = depth.x / MIN(ChrList[chra_ref].bmpdata.cv.x_max - ChrList[chra_ref].bmpdata.cv.x_min, ChrList[chrb_ref].bmpdata.cv.x_max - ChrList[chrb_ref].bmpdata.cv.x_min);
          ovlap.x = CLIP(ovlap.x,-1,1);
          nrm.x = 1.0f / ovlap.x;

          depth.y = (cv.y_max - cv.y_min);
          ovlap.y = depth.y / MIN(ChrList[chra_ref].bmpdata.cv.y_max - ChrList[chra_ref].bmpdata.cv.y_min, ChrList[chrb_ref].bmpdata.cv.y_max - ChrList[chrb_ref].bmpdata.cv.y_min);
          ovlap.y = CLIP(ovlap.y,-1,1);
          nrm.y = 1.0f / ovlap.y;

          depth.z = (cv.z_max - cv.z_min);
          ovlap.z = depth.z / MIN(ChrList[chra_ref].bmpdata.cv.z_max - ChrList[chra_ref].bmpdata.cv.z_min, ChrList[chrb_ref].bmpdata.cv.z_max - ChrList[chrb_ref].bmpdata.cv.z_min);
          ovlap.z = CLIP(ovlap.z,-1,1);
          nrm.z = 1.0f / ovlap.z;

          nrm = Normalize(nrm);

          pressure = (depth.x / 30.0f) * (depth.y / 30.0f) * (depth.z / 30.0f);

          if(ovlap.x != 1.0)
          {
            diffa.x = ChrList[chra_ref].bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
            diffb.x = ChrList[chrb_ref].bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
          }
          else
          {
            diffa.x = ChrList[chra_ref].bmpdata.mids_lo.x - ChrList[chrb_ref].bmpdata.mids_lo.x;
            diffb.x =-diffa.x;
          }

          if(ovlap.y != 1.0)
          {
            diffa.y = ChrList[chra_ref].bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
            diffb.y = ChrList[chrb_ref].bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
          }
          else
          {
            diffa.y = ChrList[chra_ref].bmpdata.mids_lo.y - ChrList[chrb_ref].bmpdata.mids_lo.y;
            diffb.y =-diffa.y;
          }

          if(ovlap.y != 1.0)
          {
            diffa.z = ChrList[chra_ref].bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffa.z += (ChrList[chra_ref].bmpdata.mids_hi.z - ChrList[chra_ref].bmpdata.mids_lo.z) * lerpa;

            diffb.z = ChrList[chrb_ref].bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffb.z += (ChrList[chrb_ref].bmpdata.mids_hi.z - ChrList[chrb_ref].bmpdata.mids_lo.z) * lerpb;
          }
          else
          {
            diffa.z  = ChrList[chra_ref].bmpdata.mids_lo.z - ChrList[chrb_ref].bmpdata.mids_lo.z;
            diffa.z += (ChrList[chra_ref].bmpdata.mids_hi.z - ChrList[chra_ref].bmpdata.mids_lo.z) * lerpa;
            diffa.z -= (ChrList[chrb_ref].bmpdata.mids_hi.z - ChrList[chrb_ref].bmpdata.mids_lo.z) * lerpb;

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
          //cr = ChrList[chrb_ref].bumpdampen * ChrList[chra_ref].bumpdampen * bumpstrength * ovlap.z * ( nrm.x * nrm.x * ovlap.x + nrm.y * nrm.y * ovlap.y ) / ftmp;

          // determine a usable mass
          m0 = -1;
          m1 = -1;
          if ( ChrList[chra_ref].weight < 0 && ChrList[chrb_ref].weight < 0 )
          {
            m0 = m1 = 110.0f;
          }
          else if (ChrList[chra_ref].weight == 0 && ChrList[chrb_ref].weight == 0)
          {
            m0 = m1 = 1.0f;
          }
          else
          {
            m0 = (ChrList[chra_ref].weight == 0.0f) ? 1.0 : ChrList[chra_ref].weight;
            m1 = (ChrList[chrb_ref].weight == 0.0f) ? 1.0 : ChrList[chrb_ref].weight;
          }

          bfound = btrue;
          cr = ChrList[chrb_ref].bumpdampen * ChrList[chra_ref].bumpdampen;
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

            ChrList[chra_ref].accum_acc.x += (diffa.x * k  - ChrList[chra_ref].vel.x * gamma) * bumpstrength;
            ChrList[chra_ref].accum_acc.y += (diffa.y * k  - ChrList[chra_ref].vel.y * gamma) * bumpstrength;
            ChrList[chra_ref].accum_acc.z += (diffa.z * k  - ChrList[chra_ref].vel.z * gamma) * bumpstrength;
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

            ChrList[chrb_ref].accum_acc.x += (diffb.x * k  - ChrList[chrb_ref].vel.x * gamma) * bumpstrength;
            ChrList[chrb_ref].accum_acc.y += (diffb.y * k  - ChrList[chrb_ref].vel.y * gamma) * bumpstrength;
            ChrList[chrb_ref].accum_acc.z += (diffb.z * k  - ChrList[chrb_ref].vel.z * gamma) * bumpstrength;
          }


          //bfound = bfalse;
          //if (( ChrList[chra_ref].bmpdata.mids_lo.x - ChrList[chrb_ref].bmpdata.mids_lo.x ) * ( ChrList[chra_ref].vel.x - ChrList[chrb_ref].vel.x ) < 0.0f )
          //{
          //  u0 = ChrList[chra_ref].vel.x;
          //  u1 = ChrList[chrb_ref].vel.x;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  ChrList[chra_ref].vel.x = ( psum - m1 * udif * cr ) / msum;
          //  ChrList[chrb_ref].vel.x = ( psum + m0 * udif * cr ) / msum;

          //  //ChrList[chra_ref].bmpdata.mids_lo.x -= ChrList[chra_ref].vel.x*dUpdate;
          //  //ChrList[chrb_ref].bmpdata.mids_lo.x -= ChrList[chrb_ref].vel.x*dUpdate;

          //  bfound = btrue;
          //}



          //if (( ChrList[chra_ref].bmpdata.mids_lo.y - ChrList[chrb_ref].bmpdata.mids_lo.y ) * ( ChrList[chra_ref].vel.y - ChrList[chrb_ref].vel.y ) < 0.0f )
          //{
          //  u0 = ChrList[chra_ref].vel.y;
          //  u1 = ChrList[chrb_ref].vel.y;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  ChrList[chra_ref].vel.y = ( psum - m1 * udif * cr ) / msum;
          //  ChrList[chrb_ref].vel.y = ( psum + m0 * udif * cr ) / msum;

          //  //ChrList[chra_ref].bmpdata.mids_lo.y -= ChrList[chra_ref].vel.y*dUpdate;
          //  //ChrList[chrb_ref].bmpdata.mids_lo.y -= ChrList[chrb_ref].vel.y*dUpdate;

          //  bfound = btrue;
          //}

          //if ( ovlap.x > 0 && ovlap.z > 0 )
          //{
          //  ChrList[chra_ref].bmpdata.mids_lo.x += m1 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  ChrList[chrb_ref].bmpdata.mids_lo.x -= m0 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  bfound = btrue;
          //}

          //if ( ovlap.y > 0 && ovlap.z > 0 )
          //{
          //  ChrList[chra_ref].bmpdata.mids_lo.y += m1 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  ChrList[chrb_ref].bmpdata.mids_lo.y -= m0 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  bfound = btrue;
          //}

          if ( bfound )
          {
            //apos = ChrList[chra_ref].pos;
            ChrList[chra_ref].aistate.alert |= ALERT_BUMPED;
            ChrList[chrb_ref].aistate.alert |= ALERT_BUMPED;
            ChrList[chra_ref].aistate.bumplast = chrb_ref;
            ChrList[chrb_ref].aistate.bumplast = chra_ref;
          };
        }
      }
    };

    // Now check collisions with every bump particle in same area
    //for ( cnt = 0, chra_ref = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra_ref );
    //      cnt++, chra_ref = chr_get_bumpnext( chra_ref ) )
    //{
    //  IDSZ chridvulnerability, eveidremove;
    //  float chrbump = 1.0f;

    //  apos = ChrList[chra_ref].pos;
    //  chridvulnerability = CapList[ChrList[chra_ref].model].idsz[IDSZ_VULNERABILITY];
    //  chrbump = ChrList[chra_ref].bumpstrength;

    //  // Check for object-particle interaction
    //  for ( tnc = 0, prtb = bumplist.prt[fanblock];
    //        tnc < prtinblock && MAXPRT != prtb;
    //        tnc++ , prtb = prt_get_bumpnext( prtb ) )
    //  {
    //    float bumpstrength, prtbump;
    //    bool_t chr_is_vulnerable;

    //    CHR_REF prt_owner = prt_get_owner( prtb );
    //    CHR_REF prt_attached = prt_get_attachedtochr( prtb );

    //    pip = PrtList[prtb].pip;
    //    bpos = PrtList[prtb].pos;

    //    chr_is_vulnerable = !ChrList[chra_ref].invictus && ( IDSZ_NONE != chridvulnerability ) && CAP_INHERIT_IDSZ( PrtList[prtb].model, chridvulnerability );

    //    prtbump = PrtList[prtb].bumpstrength;
    //    bumpstrength = chr_is_vulnerable ? 1.0f : chrbump * prtbump;

    //    if ( 0.0f == bumpstrength ) continue;

    //    // First check absolute value diamond
    //    diff.x = ABS( apos.x - bpos.x );
    //    diff.y = ABS( apos.y - bpos.y );
    //    dist = diff.x + diff.y;
    //    if ( prt_do_collision( chra_ref, prtb, bfalse ) )
    //    {
    //      vect3 pvel;

    //      if ( MAXCHR != prt_get_attachedtochr( prtb ) )
    //      {
    //        pvel.x = ( PrtList[prtb].pos.x - PrtList[prtb].pos_old.x ) / dUpdate;
    //        pvel.y = ( PrtList[prtb].pos.y - PrtList[prtb].pos_old.y ) / dUpdate;
    //        pvel.z = ( PrtList[prtb].pos.z - PrtList[prtb].pos_old.z ) / dUpdate;
    //      }
    //      else
    //      {
    //        pvel = PrtList[prtb].vel;
    //      }

    //      if ( bpos.z > ChrList[chra_ref].bmpdata.cv.z_max + pvel.z && pvel.z < 0 && ChrList[chra_ref].bmpdata.calc_is_platform && !VALID_CHR( prt_attached ) )
    //      {
    //        // Particle is falling on A
    //        PrtList[prtb].accum_pos.z += ChrList[chra_ref].bmpdata.cv.z_max - PrtList[prtb].pos.z;

    //        PrtList[prtb].accum_vel.z = - (1.0f - PipList[pip].dampen * ChrList[chra_ref].bumpdampen) * PrtList[prtb].vel.z;

    //        PrtList[prtb].accum_acc.x += ( pvel.x - ChrList[chra_ref].vel.x ) * ( 1.0 - loc_platstick ) + ChrList[chra_ref].vel.x;
    //        PrtList[prtb].accum_acc.y += ( pvel.y - ChrList[chra_ref].vel.y ) * ( 1.0 - loc_platstick ) + ChrList[chra_ref].vel.y;
    //      }

    //      // Check reaffirmation of particles
    //      if ( prt_attached != chra_ref )
    //      {
    //        if ( ChrList[chra_ref].reloadtime == 0 )
    //        {
    //          if ( ChrList[chra_ref].reaffirmdamagetype == PrtList[prtb].damagetype && ChrList[chra_ref].damagetime == 0 )
    //          {
    //            reaffirm_attached_particles( chra_ref );
    //          }
    //        }
    //      }

    //      // Check for missile treatment
    //      if (( ChrList[chra_ref].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_SHIFT ) != DAMAGE_SHIFT ||
    //            MIS_NORMAL == ChrList[chra_ref].missiletreatment  ||
    //            VALID_CHR( prt_attached ) ||
    //            ( prt_owner == chra_ref && !PipList[pip].friendlyfire ) ||
    //            ( ChrList[chrmissilehandler[chra_ref]].mana_fp8 < ( ChrList[chra_ref].missilecost << 4 ) && !ChrList[chrmissilehandler[chra_ref]].canchannel ) )
    //      {
    //        if (( TeamList[PrtList[prtb].team].hatesteam[ChrList[chra_ref].team] || ( PipList[pip].friendlyfire && (( chra_ref != prt_owner && chra_ref != ChrList[prt_owner].attachedto ) || PipList[pip].onlydamagefriendly ) ) ) && !ChrList[chra_ref].invictus )
    //        {
    //          spawn_bump_particles( chra_ref, prtb );  // Catch on fire

    //          if (( PrtList[prtb].damage.ibase > 0 ) && ( PrtList[prtb].damage.irand > 0 ) )
    //          {
    //            if ( ChrList[chra_ref].damagetime == 0 && prt_attached != chra_ref && HAS_NO_BITS( PipList[pip].damfx, DAMFX_ARRO ) )
    //            {

    //              // Normal prtb damage
    //              if ( PipList[pip].allowpush )
    //              {
    //                float ftmp = 0.2;

    //                if ( ChrList[chra_ref].weight < 0 )
    //                {
    //                  ftmp = 0;
    //                }
    //                else if ( ChrList[chra_ref].weight != 0 )
    //                {
    //                  ftmp *= ( 1.0f + ChrList[chra_ref].bumpdampen ) * PrtList[prtb].weight / ChrList[chra_ref].weight;
    //                }

    //                ChrList[chra_ref].accum_vel.x += pvel.x * ftmp;
    //                ChrList[chra_ref].accum_vel.y += pvel.y * ftmp;
    //                ChrList[chra_ref].accum_vel.z += pvel.z * ftmp;

    //                PrtList[prtb].accum_vel.x += -ChrList[chra_ref].bumpdampen * pvel.x - PrtList[prtb].vel.x;
    //                PrtList[prtb].accum_vel.y += -ChrList[chra_ref].bumpdampen * pvel.y - PrtList[prtb].vel.y;
    //                PrtList[prtb].accum_vel.z += -ChrList[chra_ref].bumpdampen * pvel.z - PrtList[prtb].vel.z;
    //              }

    //              direction = RAD_TO_TURN( atan2( pvel.y, pvel.x ) );
    //              direction = 32768 + ChrList[chra_ref].turn_lr - direction;

    //              // Check all enchants to see if they are removed
    //              enchant = ChrList[chra_ref].firstenchant;
    //              while ( enchant != MAXENCHANT )
    //              {
    //                eveidremove = EveList[EncList[enchant].eve].removedbyidsz;
    //                temp = EncList[enchant].nextenchant;
    //                if ( eveidremove != IDSZ_NONE && CAP_INHERIT_IDSZ( PrtList[prtb].model, eveidremove ) )
    //                {
    //                  remove_enchant( enchant );
    //                }
    //                enchant = temp;
    //              }

    //              //Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
    //              //+1 (256) bonus for every 4 points of intelligence and/or wisdom above 14. Below 14 gives -1 instead!
    //              //Enemy IDAM spells damage is reduced by 1% per defender's wisdom, opposite for WDAM spells
    //              if ( PipList[pip].intdamagebonus )
    //              {
    //                PrtList[prtb].damage.ibase += (( ChrList[prt_owner].intelligence_fp8 - 3584 ) * 0.25 );    //First increase damage by the attacker
    //                if(!ChrList[chra_ref].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_INVERT || !ChrList[chra_ref].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_CHARGE)
    //                PrtList[prtb].damage.ibase -= (PrtList[prtb].damage.ibase * ( ChrList[chra_ref].wisdom_fp8 > 8 ));    //Then reduce it by defender
    //              }
    //              if ( PipList[pip].wisdamagebonus )  //Same with divine spells
    //              {
    //                PrtList[prtb].damage.ibase += (( ChrList[prt_owner].wisdom_fp8 - 3584 ) * 0.25 );
    //                if(!ChrList[chra_ref].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_INVERT || !ChrList[chra_ref].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_CHARGE)
    //                PrtList[prtb].damage.ibase -= (PrtList[prtb].damage.ibase * ( ChrList[chra_ref].intelligence_fp8 > 8 ));
    //              }

    //              //Force Pancake animation?
    //              if ( PipList[pip].causepancake )
    //              {
    //                vect3 panc;
    //                Uint16 rotate_cos, rotate_sin;
    //                float cv, sv;

    //                // just a guess
    //                panc.x = 0.25 * ABS( pvel.x ) * 2.0f / ( float )( 1 + ChrList[chra_ref].bmpdata.cv.x_max - ChrList[chra_ref].bmpdata.cv.x_min  );
    //                panc.y = 0.25 * ABS( pvel.y ) * 2.0f / ( float )( 1 + ChrList[chra_ref].bmpdata.cv.y_max - ChrList[chra_ref].bmpdata.cv.y_min );
    //                panc.z = 0.25 * ABS( pvel.z ) * 2.0f / ( float )( 1 + ChrList[chra_ref].bmpdata.cv.z_max - ChrList[chra_ref].bmpdata.cv.z_min );

    //                rotate_sin = ChrList[chra_ref].turn_lr >> 2;
    //                rotate_cos = ( rotate_sin + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;

    //                cv = turntosin[rotate_cos];
    //                sv = turntosin[rotate_sin];

    //                ChrList[chra_ref].pancakevel.x = - ( panc.x * cv - panc.y * sv );
    //                ChrList[chra_ref].pancakevel.y = - ( panc.x * sv + panc.y * cv );
    //                ChrList[chra_ref].pancakevel.z = -panc.z;
    //              }

    //              // Damage the character
    //              if ( chr_is_vulnerable )
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = PrtList[prtb].damage.ibase * 2.0f * bumpstrength;
    //                ptemp.irand = PrtList[prtb].damage.irand * 2.0f * bumpstrength;
    //                damage_character( chra_ref, direction, &ptemp, PrtList[prtb].damagetype, PrtList[prtb].team, prt_owner, PipList[pip].damfx );
    //                ChrList[chra_ref].aistate.alert |= ALERT_HITVULNERABLE;
    //                cost_mana( chra_ref, PipList[pip].manadrain*2, prt_owner );  //Do mana drain too
    //              }
    //              else
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = PrtList[prtb].damage.ibase * bumpstrength;
    //                ptemp.irand = PrtList[prtb].damage.irand * bumpstrength;

    //                damage_character( chra_ref, direction, &PrtList[prtb].damage, PrtList[prtb].damagetype, PrtList[prtb].team, prt_owner, PipList[pip].damfx );
    //                cost_mana( chra_ref, PipList[pip].manadrain, prt_owner );  //Do mana drain too
    //              }

    //              // Do confuse effects
    //              if ( HAS_NO_BITS( MadList[ChrList[chra_ref].model].framefx[ChrList[chra_ref].anim.next], MADFX_INVICTUS ) || HAS_SOME_BITS( PipList[pip].damfx, DAMFX_BLOC ) )
    //              {

    //                if ( PipList[pip].grogtime != 0 && CapList[ChrList[chra_ref].model].canbegrogged )
    //                {
    //                  ChrList[chra_ref].grogtime += PipList[pip].grogtime * bumpstrength;
    //                  if ( ChrList[chra_ref].grogtime < 0 )
    //                  {
    //                    ChrList[chra_ref].grogtime = -1;
    //                    debug_message( 1, "placing infinite grog on %s (%s)", ChrList[chra_ref].name, CapList[ChrList[chra_ref].model].classname );
    //                  }
    //                  ChrList[chra_ref].aistate.alert |= ALERT_GROGGED;
    //                }

    //                if ( PipList[pip].dazetime != 0 && CapList[ChrList[chra_ref].model].canbedazed )
    //                {
    //                  ChrList[chra_ref].dazetime += PipList[pip].dazetime * bumpstrength;
    //                  if ( ChrList[chra_ref].dazetime < 0 )
    //                  {
    //                    ChrList[chra_ref].dazetime = -1;
    //                    debug_message( 1, "placing infinite daze on %s (%s)", ChrList[chra_ref].name, CapList[ChrList[chra_ref].model].classname );
    //                  };
    //                  ChrList[chra_ref].aistate.alert |= ALERT_DAZED;
    //                }
    //              }

    //              // Notify the attacker of a scored hit
    //              if ( VALID_CHR( prt_owner ) )
    //              {
    //                ChrList[prt_owner].aistate.alert |= ALERT_SCOREDAHIT;
    //                ChrList[prt_owner].aistate.hitlast = chra_ref;
    //              }
    //            }

    //            if (( wldframe&31 ) == 0 && prt_attached == chra_ref )
    //            {
    //              // Attached prtb damage ( Burning )
    //              if ( PipList[pip].xyvel.ibase == 0 )
    //              {
    //                // Make character limp
    //                ChrList[chra_ref].vel.x = 0;
    //                ChrList[chra_ref].vel.y = 0;
    //              }
    //              damage_character( chra_ref, 32768, &PrtList[prtb].damage, PrtList[prtb].damagetype, PrtList[prtb].team, prt_owner, PipList[pip].damfx );
    //              cost_mana( chra_ref, PipList[pip].manadrain, prt_owner );  //Do mana drain too

    //            }
    //          }

    //          if ( PipList[pip].endbump )
    //          {
    //            if ( PipList[pip].bumpmoney )
    //            {
    //              if ( ChrList[chra_ref].cangrabmoney && ChrList[chra_ref].alive && ChrList[chra_ref].damagetime == 0 && ChrList[chra_ref].money != MAXMONEY )
    //              {
    //                if ( ChrList[chra_ref].ismount )
    //                {
    //                  CHR_REF irider = chr_get_holdingwhich( chra_ref, SLOT_SADDLE );

    //                  // Let mounts collect money for their riders
    //                  if ( VALID_CHR( irider ) )
    //                  {
    //                    ChrList[irider].money += PipList[pip].bumpmoney;
    //                    if ( ChrList[irider].money > MAXMONEY ) ChrList[irider].money = MAXMONEY;
    //                    if ( ChrList[irider].money <        0 ) ChrList[irider].money = 0;
    //                    PrtList[prtb].gopoof = btrue;
    //                  }
    //                }
    //                else
    //                {
    //                  // Normal money collection
    //                  ChrList[chra_ref].money += PipList[pip].bumpmoney;
    //                  if ( ChrList[chra_ref].money > MAXMONEY ) ChrList[chra_ref].money = MAXMONEY;
    //                  if ( ChrList[chra_ref].money < 0 ) ChrList[chra_ref].money = 0;
    //                  PrtList[prtb].gopoof = btrue;
    //                }
    //              }
    //            }
    //            else
    //            {
    //              // Only hit one character, not several
    //              PrtList[prtb].damage.ibase *= 1.0f - bumpstrength;
    //              PrtList[prtb].damage.irand *= 1.0f - bumpstrength;

    //              if ( PrtList[prtb].damage.ibase == 0 && PrtList[prtb].damage.irand <= 1 )
    //              {
    //                PrtList[prtb].gopoof = btrue;
    //              };
    //            }
    //          }
    //        }
    //      }
    //      else if ( prt_owner != chra_ref )
    //      {
    //        cost_mana( ChrList[chra_ref].missilehandler, ( ChrList[chra_ref].missilecost << 4 ), prt_owner );

    //        // Treat the missile
    //        switch ( ChrList[chra_ref].missiletreatment )
    //        {
    //          case MIS_DEFLECT:
    //            {
    //              // Use old position to find normal
    //              acc.x = PrtList[prtb].pos.x - pvel.x * dUpdate;
    //              acc.y = PrtList[prtb].pos.y - pvel.y * dUpdate;
    //              acc.x = ChrList[chra_ref].pos.x - acc.x;
    //              acc.y = ChrList[chra_ref].pos.y - acc.y;
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
    //                PrtList[prtb].accum_vel.x += -acc.x;
    //                PrtList[prtb].accum_vel.y += -acc.y;
    //              }
    //            }
    //            break;

    //          case MIS_REFLECT:
    //            {
    //              // Reflect it back in the direction it came
    //              PrtList[prtb].accum_vel.x += -2.0f * PrtList[prtb].vel.x;
    //              PrtList[prtb].accum_vel.y += -2.0f * PrtList[prtb].vel.y;
    //            };
    //            break;
    //        };


    //        // Change the owner of the missile
    //        if ( !PipList[pip].homing )
    //        {
    //          PrtList[prtb].team = ChrList[chra_ref].team;
    //          prt_owner = chra_ref;
    //        }
    //      }
    //    }
    //  }
    //}

    // do platform physics
    //for ( cnt = 0, chra_ref = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra_ref );
    //      cnt++, chra_ref = chr_get_bumpnext( chra_ref ) )
    //{
    //  // detach character from invalid platforms
    //  chrb_ref  = chr_get_onwhichplatform( chra_ref );
    //  if ( !VALID_CHR( chrb_ref ) ) continue;

    //  if ( ChrList[chra_ref].pos.z < ChrList[chrb_ref].bmpdata.cv.z_max + RAISE )
    //  {
    //    ChrList[chra_ref].pos.z = ChrList[chrb_ref].bmpdata.cv.z_max + RAISE;
    //    if ( ChrList[chra_ref].vel.z < ChrList[chrb_ref].vel.z )
    //    {
    //      ChrList[chra_ref].vel.z = - ( ChrList[chra_ref].vel.z - ChrList[chrb_ref].vel.z ) * ChrList[chra_ref].bumpdampen * ChrList[chrb_ref].bumpdampen + ChrList[chrb_ref].vel.z;
    //    };
    //  }

    //  ChrList[chra_ref].vel.x = ( ChrList[chra_ref].vel.x - ChrList[chrb_ref].vel.x ) * ( 1.0 - loc_platstick ) + ChrList[chrb_ref].vel.x;
    //  ChrList[chra_ref].vel.y = ( ChrList[chra_ref].vel.y - ChrList[chrb_ref].vel.y ) * ( 1.0 - loc_platstick ) + ChrList[chrb_ref].vel.y;
    //  ChrList[chra_ref].turn_lr += ( ChrList[chrb_ref].turn_lr - ChrList[chrb_ref].turn_lr_old ) * ( 1.0 - loc_platstick );
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
    ChrList[cnt].reloadtime -= dUpdate;
    if ( ChrList[cnt].reloadtime < 0 ) ChrList[cnt].reloadtime = 0;
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

      if ( ChrList[cnt].alive )
      {
        ChrList[cnt].mana_fp8 += ChrList[cnt].manareturn_fp8 >> MANARETURNSHIFT;
        if ( ChrList[cnt].mana_fp8 < 0 ) ChrList[cnt].mana_fp8 = 0;
        if ( ChrList[cnt].mana_fp8 > ChrList[cnt].manamax_fp8 ) ChrList[cnt].mana_fp8 = ChrList[cnt].manamax_fp8;

        ChrList[cnt].life_fp8 += ChrList[cnt].lifereturn;
        if ( ChrList[cnt].life_fp8 < 1 ) ChrList[cnt].life_fp8 = 1;
        if ( ChrList[cnt].life_fp8 > ChrList[cnt].lifemax_fp8 ) ChrList[cnt].life_fp8 = ChrList[cnt].lifemax_fp8;
      };

      if ( ChrList[cnt].grogtime > 0 )
      {
        ChrList[cnt].grogtime--;
        if ( ChrList[cnt].grogtime < 0 ) ChrList[cnt].grogtime = 0;

        if ( ChrList[cnt].grogtime == 0 )
        {
          debug_message( 1, "stat_return() - removing grog on %s (%s)", ChrList[cnt].name, CapList[ChrList[cnt].model].classname );
        };
      }

      if ( ChrList[cnt].dazetime > 0 )
      {
        ChrList[cnt].dazetime--;
        if ( ChrList[cnt].dazetime < 0 ) ChrList[cnt].dazetime = 0;
        if ( ChrList[cnt].grogtime == 0 )
        {
          debug_message( 1, "stat_return() - removing daze on %s (%s)", ChrList[cnt].name, CapList[ChrList[cnt].model].classname );
        };
      }

    }


    // Run through all the enchants as well
    for ( cnt = 0; cnt < MAXENCHANT; cnt++ )
    {
      bool_t kill_enchant = bfalse;
      if ( !EncList[cnt].on ) continue;

      if ( EncList[cnt].time == 0 )
      {
        kill_enchant = btrue;
      };

      if ( EncList[cnt].time > 0 ) EncList[cnt].time--;

      owner = EncList[cnt].owner;
      target = EncList[cnt].target;
      eve = EncList[cnt].eve;

      // Do drains
      if ( !kill_enchant && ChrList[owner].alive )
      {
        // Change life
        ChrList[owner].life_fp8 += EncList[cnt].ownerlife;
        if ( ChrList[owner].life_fp8 < 1 )
        {
          ChrList[owner].life_fp8 = 1;
          kill_character( owner, target );
        }
        if ( ChrList[owner].life_fp8 > ChrList[owner].lifemax_fp8 )
        {
          ChrList[owner].life_fp8 = ChrList[owner].lifemax_fp8;
        }
        // Change mana
        if ( !cost_mana( owner, -EncList[cnt].ownermana, target ) && EveList[eve].endifcantpay )
        {
          kill_enchant = btrue;
        }
      }
      else if ( !EveList[eve].stayifnoowner )
      {
        kill_enchant = btrue;
      }


      if ( !kill_enchant && EncList[cnt].on )
      {
        if ( ChrList[target].alive )
        {
          // Change life
          ChrList[target].life_fp8 += EncList[cnt].targetlife;
          if ( ChrList[target].life_fp8 < 1 )
          {
            ChrList[target].life_fp8 = 1;
            kill_character( target, owner );
          }
          if ( ChrList[target].life_fp8 > ChrList[target].lifemax_fp8 )
          {
            ChrList[target].life_fp8 = ChrList[target].lifemax_fp8;
          }

          // Change mana
          if ( !cost_mana( target, -EncList[cnt].targetmana, owner ) && EveList[eve].endifcantpay )
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

        if ( PrtList[cnt].pos.z < PITDEPTH && PipList[PrtList[cnt].pip].endwater )
        {
          PrtList[cnt].gopoof = btrue;
        }
      }



      // Kill any characters that fell in a pit...
      cnt = 0;
      while ( cnt < MAXCHR )
      {
        if ( ChrList[cnt].on && ChrList[cnt].alive && !chr_in_pack( cnt ) )
        {
          if ( !ChrList[cnt].invictus && ChrList[cnt].pos.z < PITDEPTH && !chr_attached( cnt ) )
          {
            // Got one!
            kill_character( cnt, MAXCHR );
            ChrList[cnt].vel.x = 0;
            ChrList[cnt].vel.y = 0;
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
    PlaList[cnt].valid = bfalse;
    PlaList[cnt].chr = 0;
    PlaList[cnt].latch.x = 0;
    PlaList[cnt].latch.y = 0;
    PlaList[cnt].latch.b = 0;
    PlaList[cnt].device = INBITS_NONE;
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
  CHR_REF chr_ref;

  bool_t willgetcaught;
  float newsize, fkeep;

  fkeep = pow( 0.9995, dUpdate );

  for ( chr_ref = 0; chr_ref < MAXCHR; chr_ref++ )
  {
    if ( !VALID_CHR( chr_ref ) || ChrList[chr_ref].sizegototime <= 0 ) continue;

    // Make sure it won't get caught in a wall
    willgetcaught = bfalse;
    if ( ChrList[chr_ref].sizegoto > ChrList[chr_ref].fat )
    {
      float x_min_save, x_max_save;
      float y_min_save, y_max_save;

      x_min_save = ChrList[chr_ref].bmpdata.cv.x_min;
      x_max_save = ChrList[chr_ref].bmpdata.cv.x_max;

      y_min_save = ChrList[chr_ref].bmpdata.cv.y_min;
      y_max_save = ChrList[chr_ref].bmpdata.cv.y_max;

      ChrList[chr_ref].bmpdata.cv.x_min -= 5;
      ChrList[chr_ref].bmpdata.cv.y_min -= 5;

      ChrList[chr_ref].bmpdata.cv.x_max += 5;
      ChrList[chr_ref].bmpdata.cv.y_max += 5;

      if ( 0 != __chrhitawall( chr_ref, NULL ) )
      {
        willgetcaught = btrue;
      }

      ChrList[chr_ref].bmpdata.cv.x_min = x_min_save;
      ChrList[chr_ref].bmpdata.cv.x_max = x_max_save;

      ChrList[chr_ref].bmpdata.cv.y_min = y_min_save;
      ChrList[chr_ref].bmpdata.cv.y_max = y_max_save;
    }


    // If it is getting caught, simply halt growth until later
    if ( willgetcaught ) continue;

    // Figure out how big it is
    ChrList[chr_ref].sizegototime -= dUpdate;
    if ( ChrList[chr_ref].sizegototime < 0 )
    {
      ChrList[chr_ref].sizegototime = 0;
    }

    if ( ChrList[chr_ref].sizegototime > 0 )
    {
      newsize = ChrList[chr_ref].fat * fkeep + ChrList[chr_ref].sizegoto * ( 1.0f - fkeep );
    }
    else if ( ChrList[chr_ref].sizegototime <= 0 )
    {
      newsize = ChrList[chr_ref].fat;
    }

    // Make it that big...
    ChrList[chr_ref].fat             = newsize;
    ChrList[chr_ref].bmpdata.shadow  = ChrList[chr_ref].bmpdata_save.shadow * newsize;
    ChrList[chr_ref].bmpdata.size    = ChrList[chr_ref].bmpdata_save.size * newsize;
    ChrList[chr_ref].bmpdata.sizebig = ChrList[chr_ref].bmpdata_save.sizebig * newsize;
    ChrList[chr_ref].bmpdata.height  = ChrList[chr_ref].bmpdata_save.height * newsize;
    ChrList[chr_ref].weight          = CapList[ChrList[chr_ref].model].weight * newsize * newsize * newsize;  // preserve density

    // Now come up with the magic number
    ChrList[chr_ref].scale = newsize;

    // calculate the bumpers
    make_one_character_matrix( chr_ref );
  }
}

//--------------------------------------------------------------------------------------------
void export_one_character_name( char *szSaveName, CHR_REF chr_ref )
{
  // ZZ> This function makes the naming.txt file for the chr_ref
  FILE* filewrite;
  int profile;

  // Can it export?
  profile = ChrList[chr_ref].model;
  filewrite = fs_fileOpen( PRI_FAIL, "export_one_character_name()", szSaveName, "w" );
  if ( NULL== filewrite )
  {
      log_error( "Error writing file (%s)\n", szSaveName );
      return;
  }

  convert_spaces( ChrList[chr_ref].name, sizeof( ChrList[chr_ref].name ), ChrList[chr_ref].name );
  fprintf( filewrite, ":%s\n", ChrList[chr_ref].name );
  fprintf( filewrite, ":STOP\n\n" );
  fs_fileClose( filewrite );
}

//--------------------------------------------------------------------------------------------
void export_one_character_profile( char *szSaveName, CHR_REF chr_ref )
{
  // ZZ> This function creates a data.txt file for the given chr_ref.
  //     it is assumed that all enchantments have been done away with
  FILE* filewrite;
  int profile;
  int damagetype, skin;
  char types[10] = "SCPHEFIZ";
  char codes[4];

  disenchant_character( chr_ref );

  // General stuff
  profile = ChrList[chr_ref].model;


  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( filewrite )
  {
    // Real general data
    fprintf( filewrite, "Slot number    : -1\n" );   // -1 signals a flexible load thing
    funderf( filewrite, "Class name     : ", CapList[profile].classname );
    ftruthf( filewrite, "Uniform light  : ", CapList[profile].uniformlit );
    fprintf( filewrite, "Maximum ammo   : %d\n", CapList[profile].ammomax );
    fprintf( filewrite, "Current ammo   : %d\n", CapList[chr_ref].ammo );
    fgendef( filewrite, "Gender         : ", CapList[chr_ref].gender );
    fprintf( filewrite, "\n" );



    // Object stats
    fprintf( filewrite, "Life color     : %d\n", CapList[chr_ref].lifecolor );
    fprintf( filewrite, "Mana color     : %d\n", CapList[chr_ref].manacolor );
    fprintf( filewrite, "Life           : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].lifemax_fp8 ) );
    fpairof( filewrite, "Life up        : ", &CapList[profile].lifeperlevel_fp8 );
    fprintf( filewrite, "Mana           : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].manamax_fp8 ) );
    fpairof( filewrite, "Mana up        : ", &CapList[profile].manaperlevel_fp8 );
    fprintf( filewrite, "Mana return    : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].manareturn_fp8 ) );
    fpairof( filewrite, "Mana return up : ", &CapList[profile].manareturnperlevel_fp8 );
    fprintf( filewrite, "Mana flow      : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].manaflow_fp8 ) );
    fpairof( filewrite, "Mana flow up   : ", &CapList[profile].manaflowperlevel_fp8 );
    fprintf( filewrite, "STR            : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].strength_fp8 ) );
    fpairof( filewrite, "STR up         : ", &CapList[profile].strengthperlevel_fp8 );
    fprintf( filewrite, "WIS            : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].wisdom_fp8 ) );
    fpairof( filewrite, "WIS up         : ", &CapList[profile].wisdomperlevel_fp8 );
    fprintf( filewrite, "INT            : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].intelligence_fp8 ) );
    fpairof( filewrite, "INT up         : ", &CapList[profile].intelligenceperlevel_fp8 );
    fprintf( filewrite, "DEX            : %4.2f\n", FP8_TO_FLOAT( ChrList[chr_ref].dexterity_fp8 ) );
    fpairof( filewrite, "DEX up         : ", &CapList[profile].dexterityperlevel_fp8 );
    fprintf( filewrite, "\n" );



    // More physical attributes
    fprintf( filewrite, "Size           : %4.2f\n", ChrList[chr_ref].sizegoto );
    fprintf( filewrite, "Size up        : %4.2f\n", CapList[profile].sizeperlevel );
    fprintf( filewrite, "Shadow size    : %d\n", CapList[profile].shadowsize );
    fprintf( filewrite, "Bump size      : %d\n", CapList[profile].bumpsize );
    fprintf( filewrite, "Bump height    : %d\n", CapList[profile].bumpheight );
    fprintf( filewrite, "Bump dampen    : %4.2f\n", CapList[profile].bumpdampen );
    fprintf( filewrite, "Weight         : %d\n", CapList[profile].weight < 0.0f ? 0xFF : ( Uint8 ) CapList[profile].weight );
    fprintf( filewrite, "Jump power     : %4.2f\n", CapList[profile].jump );
    fprintf( filewrite, "Jump number    : %d\n", CapList[profile].jumpnumber );
    fprintf( filewrite, "Sneak speed    : %d\n", CapList[profile].spd_sneak );
    fprintf( filewrite, "Walk speed     : %d\n", CapList[profile].spd_walk );
    fprintf( filewrite, "Run speed      : %d\n", CapList[profile].spd_run );
    fprintf( filewrite, "Fly to height  : %d\n", CapList[profile].flyheight );
    fprintf( filewrite, "Flashing AND   : %d\n", CapList[profile].flashand );
    fprintf( filewrite, "Alpha blending : %d\n", CapList[profile].alpha_fp8 );
    fprintf( filewrite, "Light blending : %d\n", CapList[profile].light_fp8 );
    ftruthf( filewrite, "Transfer blend : ", CapList[profile].transferblend );
    fprintf( filewrite, "Sheen          : %d\n", CapList[profile].sheen_fp8 );
    ftruthf( filewrite, "Phong mapping  : ", CapList[profile].enviro );
    fprintf( filewrite, "Texture X add  : %4.2f\n", CapList[profile].uoffvel / (float)UINT16_SIZE );
    fprintf( filewrite, "Texture Y add  : %4.2f\n", CapList[profile].voffvel / (float)UINT16_SIZE );
    ftruthf( filewrite, "Sticky butt    : ", CapList[profile].stickybutt );
    fprintf( filewrite, "\n" );



    // Invulnerability data
    ftruthf( filewrite, "Invictus       : ", CapList[profile].invictus );
    fprintf( filewrite, "NonI facing    : %d\n", CapList[profile].nframefacing );
    fprintf( filewrite, "NonI angle     : %d\n", CapList[profile].nframeangle );
    fprintf( filewrite, "I facing       : %d\n", CapList[profile].iframefacing );
    fprintf( filewrite, "I angle        : %d\n", CapList[profile].iframeangle );
    fprintf( filewrite, "\n" );



    // Skin defenses
    fprintf( filewrite, "Base defense   : " );
    for ( skin = 0; skin < MAXSKIN; skin++ ) { fprintf( filewrite, "%3d ", 255 - CapList[profile].defense_fp8[skin] ); }
    fprintf( filewrite, "\n" );

    for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
    {
      fprintf( filewrite, "%c damage shift :", types[damagetype] );
      for ( skin = 0; skin < MAXSKIN; skin++ ) { fprintf( filewrite, "%3d ", CapList[profile].damagemodifier_fp8[damagetype][skin]&DAMAGE_SHIFT ); };
      fprintf( filewrite, "\n" );
    }

    for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
    {
      fprintf( filewrite, "%c damage code  : ", types[damagetype] );
      for ( skin = 0; skin < MAXSKIN; skin++ )
      {
        codes[skin] = 'F';
        if ( CapList[profile].damagemodifier_fp8[damagetype][skin]&DAMAGE_CHARGE ) codes[skin] = 'C';
        if ( CapList[profile].damagemodifier_fp8[damagetype][skin]&DAMAGE_INVERT ) codes[skin] = 'T';
        if ( CapList[profile].damagemodifier_fp8[damagetype][skin]&DAMAGE_MANA )   codes[skin] = 'M';
        fprintf( filewrite, "%3c ", codes[skin] );
      }
      fprintf( filewrite, "\n" );
    }

    fprintf( filewrite, "Acceleration   : " );
    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      fprintf( filewrite, "%3.0f ", CapList[profile].maxaccel[skin]*80 );
    }
    fprintf( filewrite, "\n" );



    // Experience and level data
    fprintf( filewrite, "EXP for 2nd    : %d\n", CapList[profile].experienceforlevel[1] );
    fprintf( filewrite, "EXP for 3rd    : %d\n", CapList[profile].experienceforlevel[2] );
    fprintf( filewrite, "EXP for 4th    : %d\n", CapList[profile].experienceforlevel[3] );
    fprintf( filewrite, "EXP for 5th    : %d\n", CapList[profile].experienceforlevel[4] );
    fprintf( filewrite, "EXP for 6th    : %d\n", CapList[profile].experienceforlevel[5] );
    fprintf( filewrite, "Starting EXP   : %d\n", CapList[chr_ref].experience );
    fprintf( filewrite, "EXP worth      : %d\n", CapList[profile].experienceworth );
    fprintf( filewrite, "EXP exchange   : %5.3f\n", CapList[profile].experienceexchange );
    fprintf( filewrite, "EXPSECRET      : %4.2f\n", CapList[profile].experiencerate[0] );
    fprintf( filewrite, "EXPQUEST       : %4.2f\n", CapList[profile].experiencerate[1] );
    fprintf( filewrite, "EXPDARE        : %4.2f\n", CapList[profile].experiencerate[2] );
    fprintf( filewrite, "EXPKILL        : %4.2f\n", CapList[profile].experiencerate[3] );
    fprintf( filewrite, "EXPMURDER      : %4.2f\n", CapList[profile].experiencerate[4] );
    fprintf( filewrite, "EXPREVENGE     : %4.2f\n", CapList[profile].experiencerate[5] );
    fprintf( filewrite, "EXPTEAMWORK    : %4.2f\n", CapList[profile].experiencerate[6] );
    fprintf( filewrite, "EXPROLEPLAY    : %4.2f\n", CapList[profile].experiencerate[7] );
    fprintf( filewrite, "\n" );



    // IDSZ identification tags
    fprintf( filewrite, "IDSZ Parent    : [%s]\n", undo_idsz( CapList[profile].idsz[0] ) );
    fprintf( filewrite, "IDSZ Type      : [%s]\n", undo_idsz( CapList[profile].idsz[1] ) );
    fprintf( filewrite, "IDSZ Skill     : [%s]\n", undo_idsz( CapList[profile].idsz[2] ) );
    fprintf( filewrite, "IDSZ Special   : [%s]\n", undo_idsz( CapList[profile].idsz[3] ) );
    fprintf( filewrite, "IDSZ Hate      : [%s]\n", undo_idsz( CapList[profile].idsz[4] ) );
    fprintf( filewrite, "IDSZ Vulnie    : [%s]\n", undo_idsz( CapList[profile].idsz[5] ) );
    fprintf( filewrite, "\n" );



    // Item and damage flags
    ftruthf( filewrite, "Is an item     : ", CapList[profile].isitem );
    ftruthf( filewrite, "Is a mount     : ", CapList[profile].ismount );
    ftruthf( filewrite, "Is stackable   : ", CapList[profile].isstackable );
    ftruthf( filewrite, "Name known     : ", CapList[chr_ref].nameknown );
    ftruthf( filewrite, "Usage known    : ", CapList[profile].usageknown );
    ftruthf( filewrite, "Is exportable  : ", CapList[profile].cancarrytonextmodule );
    ftruthf( filewrite, "Requires skill : ", CapList[profile].needskillidtouse );
    ftruthf( filewrite, "Is platform    : ", CapList[profile].isplatform );
    ftruthf( filewrite, "Collects money : ", CapList[profile].cangrabmoney );
    ftruthf( filewrite, "Can open stuff : ", CapList[profile].canopenstuff );
    fprintf( filewrite, "\n" );



    // Other item and damage stuff
    fdamagf( filewrite, "Damage type    : ", CapList[profile].damagetargettype );
    factiof( filewrite, "Attack type    : ", CapList[profile].weaponaction );
    fprintf( filewrite, "\n" );



    // Particle attachments
    fprintf( filewrite, "Attached parts : %d\n", CapList[profile].attachedprtamount );
    fdamagf( filewrite, "Reaffirm type  : ", CapList[profile].attachedprtreaffirmdamagetype );
    fprintf( filewrite, "Particle type  : %d\n", CapList[profile].attachedprttype );
    fprintf( filewrite, "\n" );



    // Character hands
    ftruthf( filewrite, "Left valid     : ", CapList[profile].slotvalid[SLOT_LEFT] );
    ftruthf( filewrite, "Right valid    : ", CapList[profile].slotvalid[SLOT_RIGHT] );
    fprintf( filewrite, "\n" );



    // Particle spawning on attack
    ftruthf( filewrite, "Part on weapon : ", CapList[profile].attackattached );
    fprintf( filewrite, "Part type      : %d\n", CapList[profile].attackprttype );
    fprintf( filewrite, "\n" );



    // Particle spawning for GoPoof
    fprintf( filewrite, "Poof amount    : %d\n", CapList[profile].gopoofprtamount );
    fprintf( filewrite, "Facing add     : %d\n", CapList[profile].gopoofprtfacingadd );
    fprintf( filewrite, "Part type      : %d\n", CapList[profile].gopoofprttype );
    fprintf( filewrite, "\n" );



    // Particle spawning for blud
    ftruthf( filewrite, "Blud valid    : ", CapList[profile].bludlevel );
    fprintf( filewrite, "Part type      : %d\n", CapList[profile].bludprttype );
    fprintf( filewrite, "\n" );



    // Extra stuff
    ftruthf( filewrite, "Waterwalking   : ", CapList[profile].waterwalk );
    fprintf( filewrite, "Bounce dampen  : %5.3f\n", CapList[profile].dampen );
    fprintf( filewrite, "\n" );



    // More stuff
    fprintf( filewrite, "Life healing   : %5.3f\n", FP8_TO_FLOAT( CapList[profile].lifeheal_fp8 ) );
    fprintf( filewrite, "Mana cost      : %5.3f\n", FP8_TO_FLOAT( CapList[profile].manacost_fp8 ) );
    fprintf( filewrite, "Life return    : %d\n", CapList[profile].lifereturn_fp8 );
    fprintf( filewrite, "Stopped by     : %d\n", CapList[profile].stoppedby );

    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      STRING stmp;
      snprintf( stmp, sizeof( stmp ), "Skin %d name    : ", skin );
      funderf( filewrite, stmp, CapList[profile].skinname[skin] );
    };

    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      fprintf( filewrite, "Skin %d cost    : %d\n", skin, CapList[profile].skincost[skin] );
    };

    fprintf( filewrite, "STR dampen     : %5.3f\n", CapList[profile].strengthdampen );
    fprintf( filewrite, "\n" );



    // Another memory lapse
    ftruthf( filewrite, "No rider attak : ", btrue - CapList[profile].ridercanattack );
    ftruthf( filewrite, "Can be dazed   : ", CapList[profile].canbedazed );
    ftruthf( filewrite, "Can be grogged : ", CapList[profile].canbegrogged );
    fprintf( filewrite, "NOT USED       : 0\n" );
    fprintf( filewrite, "NOT USED       : 0\n" );
    ftruthf( filewrite, "Can see invisi : ", CapList[profile].canseeinvisible );
    fprintf( filewrite, "Kursed chance  : %d\n", ChrList[chr_ref].iskursed*100 );
    fprintf( filewrite, "Footfall sound : %d\n", CapList[profile].footfallsound );
    fprintf( filewrite, "Jump sound     : %d\n", CapList[profile].jumpsound );
    fprintf( filewrite, "\n" );


    // Expansions
    fprintf( filewrite, ":[GOLD] %d\n", CapList[chr_ref].money );

    if ( CapList[profile].skindressy&1 ) fprintf( filewrite, ":[DRES] 0\n" );
    if ( CapList[profile].skindressy&2 ) fprintf( filewrite, ":[DRES] 1\n" );
    if ( CapList[profile].skindressy&4 ) fprintf( filewrite, ":[DRES] 2\n" );
    if ( CapList[profile].skindressy&8 ) fprintf( filewrite, ":[DRES] 3\n" );
    if ( CapList[profile].resistbumpspawn ) fprintf( filewrite, ":[STUK] 0\n" );
    if ( CapList[profile].istoobig ) fprintf( filewrite, ":[PACK] 0\n" );
    if ( !CapList[profile].reflect ) fprintf( filewrite, ":[VAMP] 1\n" );
    if ( CapList[profile].alwaysdraw ) fprintf( filewrite, ":[DRAW] 1\n" );
    if ( CapList[profile].isranged ) fprintf( filewrite, ":[RANG] 1\n" );
    if ( CapList[profile].hidestate != NOHIDE ) fprintf( filewrite, ":[HIDE] %d\n", CapList[profile].hidestate );
    if ( CapList[profile].isequipment ) fprintf( filewrite, ":[EQUI] 1\n" );
    if ( CapList[chr_ref].bumpsizebig >= CapList[chr_ref].bumpsize ) fprintf( filewrite, ":[SQUA] 1\n" );
    if ( CapList[chr_ref].icon != CapList[profile].usageknown ) fprintf( filewrite, ":[ICON] %d\n", CapList[chr_ref].icon );
    if ( CapList[profile].forceshadow ) fprintf( filewrite, ":[SHAD] 1\n" );

    //Skill expansions
    if ( CapList[chr_ref].canseekurse )  fprintf( filewrite, ":[CKUR] 1\n" );
    if ( CapList[chr_ref].canusearcane ) fprintf( filewrite, ":[WMAG] 1\n" );
    if ( CapList[chr_ref].canjoust )     fprintf( filewrite, ":[JOUS] 1\n" );
    if ( CapList[chr_ref].canusedivine ) fprintf( filewrite, ":[HMAG] 1\n" );
    if ( CapList[chr_ref].candisarm )    fprintf( filewrite, ":[DISA] 1\n" );
    if ( CapList[chr_ref].canusetech )   fprintf( filewrite, ":[TECH] 1\n" );
    if ( CapList[chr_ref].canbackstab )  fprintf( filewrite, ":[STAB] 1\n" );
    if ( CapList[chr_ref].canuseadvancedweapons ) fprintf( filewrite, ":[AWEP] 1\n" );
    if ( CapList[chr_ref].canusepoison ) fprintf( filewrite, ":[POIS] 1\n" );
    if ( CapList[chr_ref].canread )  fprintf( filewrite, ":[READ] 1\n" );

    //General exported chr_ref information
    fprintf( filewrite, ":[PLAT] %d\n", CapList[profile].canuseplatforms );
    fprintf( filewrite, ":[SKIN] %d\n", ( ChrList[chr_ref].texture - MadList[profile].skinstart ) % MAXSKIN );
    fprintf( filewrite, ":[CONT] %d\n", ChrList[chr_ref].aistate.content );
    fprintf( filewrite, ":[STAT] %d\n", ChrList[chr_ref].aistate.state );
    fprintf( filewrite, ":[LEVL] %d\n", ChrList[chr_ref].experiencelevel );
    fs_fileClose( filewrite );
  }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin( char *szSaveName, CHR_REF chr_ref )
{
  // ZZ> This function creates a skin.txt file for the given chr_ref.
  FILE* filewrite;
  int profile;

  // General stuff
  profile = ChrList[chr_ref].model;

  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( NULL != filewrite )
  {
    fprintf( filewrite, "This file is used only by the import menu\n" );
    fprintf( filewrite, ": %d\n", ( ChrList[chr_ref].texture - MadList[profile].skinstart ) % MAXSKIN );
    fs_fileClose( filewrite );
  }
}

//--------------------------------------------------------------------------------------------
void calc_cap_experience( Uint16 profile )
{
  float statdebt, statperlevel;

  statdebt  = CapList[profile].life_fp8.ibase + CapList[profile].mana_fp8.ibase + CapList[profile].manareturn_fp8.ibase + CapList[profile].manaflow_fp8.ibase;
  statdebt += CapList[profile].strength_fp8.ibase + CapList[profile].wisdom_fp8.ibase + CapList[profile].intelligence_fp8.ibase + CapList[profile].dexterity_fp8.ibase;
  statdebt += ( CapList[profile].life_fp8.irand + CapList[profile].mana_fp8.irand + CapList[profile].manareturn_fp8.irand + CapList[profile].manaflow_fp8.irand ) * 0.5f;
  statdebt += ( CapList[profile].strength_fp8.irand + CapList[profile].wisdom_fp8.irand + CapList[profile].intelligence_fp8.irand + CapList[profile].dexterity_fp8.irand ) * 0.5f;

  statperlevel  = CapList[profile].lifeperlevel_fp8.ibase + CapList[profile].manaperlevel_fp8.ibase + CapList[profile].manareturnperlevel_fp8.ibase + CapList[profile].manaflowperlevel_fp8.ibase;
  statperlevel += CapList[profile].strengthperlevel_fp8.ibase + CapList[profile].wisdomperlevel_fp8.ibase + CapList[profile].intelligenceperlevel_fp8.ibase + CapList[profile].dexterityperlevel_fp8.ibase;
  statperlevel += ( CapList[profile].lifeperlevel_fp8.irand + CapList[profile].manaperlevel_fp8.irand + CapList[profile].manareturnperlevel_fp8.irand + CapList[profile].manaflowperlevel_fp8.irand ) * 0.5f;
  statperlevel += ( CapList[profile].strengthperlevel_fp8.irand + CapList[profile].wisdomperlevel_fp8.irand + CapList[profile].intelligenceperlevel_fp8.irand + CapList[profile].dexterityperlevel_fp8.irand ) * 0.5f;

  CapList[profile].experienceconst = 50.6f * ( FP8_TO_FLOAT( statdebt ) - 51.5 );
  CapList[profile].experiencecoeff = 26.3f * MAX( 1, FP8_TO_FLOAT( statperlevel ) );
};

//--------------------------------------------------------------------------------------------
int calc_chr_experience( Uint16 object, float level )
{
  Uint16 profile;

  if ( !VALID_CHR( object ) ) return 0;

  profile = ChrList[object].model;

  return level*level*CapList[profile].experiencecoeff + CapList[profile].experienceconst + 1;
};

//--------------------------------------------------------------------------------------------
float calc_chr_level( Uint16 object )
{
  Uint16 profile;
  float  level;

  if ( !VALID_CHR( object ) ) return 0.0f;

  profile = ChrList[object].model;

  level = ( ChrList[object].experience - CapList[profile].experienceconst ) / CapList[profile].experiencecoeff;
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
Uint16 object_generate_index( char *szLoadName )
{
  // ZZ > This reads the object slot in CData.data_file that the profile
  //      is assigned to.  Errors in this number may cause the program to abort

  FILE* fileread;
  int iobj = MAXMODEL;

  // Open the file
  fileread = fs_fileOpen( PRI_FAIL, "object_generate_index()", szLoadName, "r" );
  if ( fileread == NULL )
  {
    // The data file wasn't found
    log_error( "Data.txt could not be correctly read! (%s) \n", szLoadName );
    return iobj;
  }

  globalname = szLoadName;
  // Read in the iobj slot
  iobj = fget_next_int( fileread );
  if ( iobj < 0 )
  {
    if ( importobject < 0 )
    {
      log_error( "Object slot number %i is invalid. (%s) \n", iobj, szLoadName );
    }
    else
    {
      iobj = importobject;
    }
  }

  fs_fileClose(fileread);

  return iobj;
}


//--------------------------------------------------------------------------------------------
Uint16 load_one_cap( char *szObjectpath, Uint16 icap )
{
  // ZZ> This function fills a character profile with data from CData.data_file

  FILE* fileread;
  int skin, cnt;
  int iTmp;
  char cTmp;
  int damagetype, level, xptype;
  IDSZ idsz;
  CAP * pcap;
  STRING szLoadname;

  // generate the full path name of "data.txt"
  snprintf( szLoadname, sizeof( szLoadname ), "%s%s", szObjectpath, CData.data_file );

  // Open the file
  fileread = fs_fileOpen( PRI_FAIL, "object_generate_index()", szLoadname, "r" );
  if ( fileread == NULL )
  {
    // The data file wasn't found
    log_error( "Data.txt could not be correctly read! (%s) \n", szLoadname );
    return icap;
  }

  // make the notation "easier"
  pcap = CapList + icap;

  // skip over the slot information
  fget_next_int( fileread );

  // Read in the real general data
  fget_next_name( fileread, pcap->classname, sizeof( pcap->classname ) );

  // Make sure we don't load over an existing model
  if ( pcap->used )
  {
    log_error( "Character profile slot %i is already used. (%s)\n", icap, szLoadname );
  }

  // Light cheat
  pcap->uniformlit = fget_next_bool( fileread );

  // Ammo
  pcap->ammomax = fget_next_int( fileread );
  pcap->ammo = fget_next_int( fileread );

  // Gender
  pcap->gender = fget_next_gender( fileread );

  // Read in the icap stats
  pcap->lifecolor = fget_next_int( fileread );
  pcap->manacolor = fget_next_int( fileread );
  fget_next_pair_fp8( fileread, &pcap->life_fp8 );
  fget_next_pair_fp8( fileread, &pcap->lifeperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->mana_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manaperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manareturn_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manareturnperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manaflow_fp8 );
  fget_next_pair_fp8( fileread, &pcap->manaflowperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->strength_fp8 );
  fget_next_pair_fp8( fileread, &pcap->strengthperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->wisdom_fp8 );
  fget_next_pair_fp8( fileread, &pcap->wisdomperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->intelligence_fp8 );
  fget_next_pair_fp8( fileread, &pcap->intelligenceperlevel_fp8 );
  fget_next_pair_fp8( fileread, &pcap->dexterity_fp8 );
  fget_next_pair_fp8( fileread, &pcap->dexterityperlevel_fp8 );

  // More physical attributes
  pcap->size = fget_next_float( fileread );
  pcap->sizeperlevel = fget_next_float( fileread );
  pcap->shadowsize = fget_next_int( fileread );
  pcap->bumpsize = fget_next_int( fileread );
  pcap->bumpheight = fget_next_int( fileread );
  pcap->bumpdampen = fget_next_float( fileread );
  pcap->weight = fget_next_int( fileread );
  if ( pcap->weight == 255.0f ) pcap->weight = -1.0f;
  if ( pcap->weight ==   0.0f ) pcap->weight = 1.0f;

  pcap->bumpstrength = ( pcap->bumpsize > 0.0f ) ? 1.0f : 0.0f;
  if ( pcap->bumpsize   == 0.0f ) pcap->bumpsize   = 1.0f;
  if ( pcap->bumpheight == 0.0f ) pcap->bumpheight = 1.0f;
  if ( pcap->weight     == 0.0f ) pcap->weight     = 1.0f;

  pcap->jump = fget_next_float( fileread );
  pcap->jumpnumber = fget_next_int( fileread );
  pcap->spd_sneak = fget_next_int( fileread );
  pcap->spd_walk = fget_next_int( fileread );
  pcap->spd_run = fget_next_int( fileread );
  pcap->flyheight = fget_next_int( fileread );
  pcap->flashand = fget_next_int( fileread );
  pcap->alpha_fp8 = fget_next_int( fileread );
  pcap->light_fp8 = fget_next_int( fileread );
  if ( pcap->light_fp8 < 0xff )
  {
    pcap->alpha_fp8 = MIN( pcap->alpha_fp8, 0xff - pcap->light_fp8 );
  };

  pcap->transferblend = fget_next_bool( fileread );
  pcap->sheen_fp8 = fget_next_int( fileread );
  pcap->enviro = fget_next_bool( fileread );
  pcap->uoffvel = fget_next_float( fileread ) * (float)UINT16_MAX;
  pcap->voffvel = fget_next_float( fileread ) * (float)UINT16_MAX;
  pcap->stickybutt = fget_next_bool( fileread );


  // Invulnerability data
  pcap->invictus = fget_next_bool( fileread );
  pcap->nframefacing = fget_next_int( fileread );
  pcap->nframeangle = fget_next_int( fileread );
  pcap->iframefacing = fget_next_int( fileread );
  pcap->iframeangle = fget_next_int( fileread );
  // Resist burning and stuck arrows with nframe angle of 1 or more
  if ( pcap->nframeangle > 0 )
  {
    if ( pcap->nframeangle == 1 )
    {
      pcap->nframeangle = 0;
    }
  }


  // Skin defenses ( 4 skins )
  fgoto_colon( fileread );
  for ( skin = 0; skin < MAXSKIN; skin++ )
    { pcap->defense_fp8[skin] = 255 - fget_int( fileread ); };

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fgoto_colon( fileread );
    for ( skin = 0;skin < MAXSKIN;skin++ )
      { pcap->damagemodifier_fp8[damagetype][skin] = fget_int( fileread ); };
  }

  for ( damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++ )
  {
    fgoto_colon( fileread );
    for ( skin = 0; skin < MAXSKIN; skin++ )
    {
      cTmp = fget_first_letter( fileread );
      switch ( toupper( cTmp ) )
      {
        case 'T': pcap->damagemodifier_fp8[damagetype][skin] |= DAMAGE_INVERT; break;
        case 'C': pcap->damagemodifier_fp8[damagetype][skin] |= DAMAGE_CHARGE; break;
        case 'M': pcap->damagemodifier_fp8[damagetype][skin] |= DAMAGE_MANA;   break;
      };
    }
  }

  fgoto_colon( fileread );
  for ( skin = 0;skin < MAXSKIN;skin++ )
    { pcap->maxaccel[skin] = fget_float( fileread ) / 80.0; };


  // Experience and level data
  pcap->experienceforlevel[0] = 0;
  for ( level = 1; level < BASELEVELS; level++ )
    { pcap->experienceforlevel[level] = fget_next_int( fileread ); }

  fget_next_pair( fileread, &pcap->experience );
  pcap->experienceworth = fget_next_int( fileread );
  pcap->experienceexchange = fget_next_float( fileread );

  for ( xptype = 0; xptype < XP_COUNT; xptype++ )
    { pcap->experiencerate[xptype] = fget_next_float( fileread ) + 0.001f; }


  // IDSZ tags
  for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
    { pcap->idsz[cnt] = fget_next_idsz( fileread ); }


  // Item and damage flags
  pcap->isitem = fget_next_bool( fileread );
  pcap->ismount = fget_next_bool( fileread );
  pcap->isstackable = fget_next_bool( fileread );
  pcap->nameknown = fget_next_bool( fileread );
  pcap->usageknown = fget_next_bool( fileread );
  pcap->cancarrytonextmodule = fget_next_bool( fileread );
  pcap->needskillidtouse = fget_next_bool( fileread );
  pcap->isplatform = fget_next_bool( fileread );
  pcap->cangrabmoney = fget_next_bool( fileread );
  pcap->canopenstuff = fget_next_bool( fileread );



  // More item and damage stuff
  pcap->damagetargettype = fget_next_damage( fileread );
  pcap->weaponaction = fget_next_action( fileread );


  // Particle attachments
  pcap->attachedprtamount = fget_next_int( fileread );
  pcap->attachedprtreaffirmdamagetype = fget_next_damage( fileread );
  pcap->attachedprttype = fget_next_int( fileread );


  // Character hands
  pcap->slotvalid[SLOT_LEFT] = fget_next_bool( fileread );
  pcap->slotvalid[SLOT_RIGHT] = fget_next_bool( fileread );
  if ( pcap->ismount )
  {
    pcap->slotvalid[SLOT_SADDLE] = pcap->slotvalid[SLOT_LEFT];
    pcap->slotvalid[SLOT_LEFT]   = bfalse;
    //pcap->slotvalid[SLOT_RIGHT]  = bfalse;
  };



  // Attack order ( weapon )
  pcap->attackattached = fget_next_bool( fileread );
  pcap->attackprttype = fget_next_int( fileread );


  // GoPoof
  pcap->gopoofprtamount = fget_next_int( fileread );
  pcap->gopoofprtfacingadd = fget_next_int( fileread );
  pcap->gopoofprttype = fget_next_int( fileread );


  // Blud
  pcap->bludlevel = fget_next_blud( fileread );
  pcap->bludprttype = fget_next_int( fileread );


  // Stuff I forgot
  pcap->waterwalk = fget_next_bool( fileread );
  pcap->dampen = fget_next_float( fileread );


  // More stuff I forgot
  pcap->lifeheal_fp8 = fget_next_fixed( fileread );
  pcap->manacost_fp8 = fget_next_fixed( fileread );
  pcap->lifereturn_fp8 = fget_next_int( fileread );
  pcap->stoppedby = fget_next_int( fileread ) | MESHFX_IMPASS;

  for ( skin = 0;skin < MAXSKIN;skin++ )
    { fget_next_name( fileread, pcap->skinname[skin], sizeof( pcap->skinname[skin] ) ); };

  for ( skin = 0;skin < MAXSKIN;skin++ )
    { pcap->skincost[skin] = fget_next_int( fileread ); };

  pcap->strengthdampen = fget_next_float( fileread );



  // Another memory lapse
  pcap->ridercanattack = !fget_next_bool( fileread );
  pcap->canbedazed = fget_next_bool( fileread );
  pcap->canbegrogged = fget_next_bool( fileread );
  fget_next_int( fileread );   // !!!BAD!!! Life add
  fget_next_int( fileread );   // !!!BAD!!! Mana add
  pcap->canseeinvisible = fget_next_bool( fileread );
  pcap->kursechance = fget_next_int( fileread );

  iTmp = fget_next_int( fileread ); pcap->footfallsound = FIX_SOUND( iTmp );
  iTmp = fget_next_int( fileread ); pcap->jumpsound     = FIX_SOUND( iTmp );



  // Clear expansions...
  pcap->skindressy = bfalse;
  pcap->resistbumpspawn = bfalse;
  pcap->istoobig = bfalse;
  pcap->reflect = btrue;
  pcap->alwaysdraw = bfalse;
  pcap->isranged = bfalse;
  pcap->hidestate = NOHIDE;
  pcap->isequipment = bfalse;
  pcap->bumpsizebig = pcap->bumpsize * SQRT_TWO;
  pcap->money = 0;
  pcap->icon = pcap->usageknown;
  pcap->forceshadow = bfalse;
  pcap->skinoverride = NOSKINOVERRIDE;
  pcap->contentoverride = 0;
  pcap->stateoverride = 0;
  pcap->leveloverride = 0;
  pcap->canuseplatforms = !pcap->isplatform;

  //Reset Skill Expansions
  pcap->canseekurse = bfalse;
  pcap->canusearcane = bfalse;
  pcap->canjoust = bfalse;
  pcap->canusedivine = bfalse;
  pcap->candisarm = bfalse;
  pcap->canusetech = bfalse;
  pcap->canbackstab = bfalse;
  pcap->canusepoison = bfalse;
  pcap->canuseadvancedweapons = bfalse;

  // Read expansions
  while ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
    iTmp = fget_int( fileread );
    if ( MAKE_IDSZ( "GOLD" ) == idsz )  pcap->money = iTmp;
    else if ( MAKE_IDSZ( "STUK" ) == idsz )  pcap->resistbumpspawn = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "PACK" ) == idsz )  pcap->istoobig = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "VAMP" ) == idsz )  pcap->reflect = !INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "DRAW" ) == idsz )  pcap->alwaysdraw = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RANG" ) == idsz )  pcap->isranged = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "HIDE" ) == idsz )  pcap->hidestate = iTmp;
    else if ( MAKE_IDSZ( "EQUI" ) == idsz )  pcap->isequipment = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SQUA" ) == idsz )  pcap->bumpsizebig = pcap->bumpsize * 2.0f;
    else if ( MAKE_IDSZ( "ICON" ) == idsz )  pcap->icon = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SHAD" ) == idsz )  pcap->forceshadow = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "SKIN" ) == idsz )  pcap->skinoverride = iTmp % MAXSKIN;
    else if ( MAKE_IDSZ( "CONT" ) == idsz )  pcap->contentoverride = iTmp;
    else if ( MAKE_IDSZ( "STAT" ) == idsz )  pcap->stateoverride = iTmp;
    else if ( MAKE_IDSZ( "LEVL" ) == idsz )  pcap->leveloverride = iTmp;
    else if ( MAKE_IDSZ( "PLAT" ) == idsz )  pcap->canuseplatforms = INT_TO_BOOL( iTmp );
    else if ( MAKE_IDSZ( "RIPP" ) == idsz )  pcap->ripple = INT_TO_BOOL( iTmp );

    //Skill Expansions
    // [CKUR] Can it see kurses?
    else if ( MAKE_IDSZ( "CKUR" ) == idsz )  pcap->canseekurse  = INT_TO_BOOL( iTmp );
    // [WMAG] Can the character use arcane spellbooks?
    else if ( MAKE_IDSZ( "WMAG" ) == idsz )  pcap->canusearcane = INT_TO_BOOL( iTmp );
    // [JOUS] Can the character joust with a lance?
    else if ( MAKE_IDSZ( "JOUS" ) == idsz )  pcap->canjoust     = INT_TO_BOOL( iTmp );
    // [HMAG] Can the character use divine spells?
    else if ( MAKE_IDSZ( "HMAG" ) == idsz )  pcap->canusedivine = INT_TO_BOOL( iTmp );
    // [TECH] Able to use items technological items?
    else if ( MAKE_IDSZ( "TECH" ) == idsz )  pcap->canusetech   = INT_TO_BOOL( iTmp );
    // [DISA] Find and disarm traps?
    else if ( MAKE_IDSZ( "DISA" ) == idsz )  pcap->candisarm    = INT_TO_BOOL( iTmp );
    // [STAB] Backstab and murder?
    else if ( idsz == MAKE_IDSZ( "STAB" ) )  pcap->canbackstab  = INT_TO_BOOL( iTmp );
    // [AWEP] Profiency with advanced weapons?
    else if ( idsz == MAKE_IDSZ( "AWEP" ) )  pcap->canuseadvancedweapons = INT_TO_BOOL( iTmp );
    // [POIS] Use poison without err?
    else if ( idsz == MAKE_IDSZ( "POIS" ) )  pcap->canusepoison = INT_TO_BOOL( iTmp );
  }

  fs_fileClose( fileread );

  calc_cap_experience( icap );

  // tell everyone that we loaded correctly
  pcap->used = btrue;

  return icap;
}

//--------------------------------------------------------------------------------------------
int get_skin( char *filename )
{
  // ZZ> This function reads the skin.txt file...
  FILE* fileread;
  Uint8 skin;


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
      load_one_mad( filename, numloadplayer );

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
}

//--------------------------------------------------------------------------------------------
bool_t check_skills( int who, Uint32 whichskill )
{
  // ZF> This checks if the specified character has the required skill. Returns btrue if true
  // and bfalse if not. Also checks Skill expansions.
  bool_t result = bfalse;

  // First check the character Skill ID matches
  // Then check for expansion skills too.
  if ( CapList[ChrList[who].model].idsz[IDSZ_SKILL] == whichskill ) result = btrue;
  else if ( MAKE_IDSZ( "CKUR" ) == whichskill ) result = ChrList[who].canseekurse;
  else if ( MAKE_IDSZ( "WMAG" ) == whichskill ) result = ChrList[who].canusearcane;
  else if ( MAKE_IDSZ( "JOUS" ) == whichskill ) result = ChrList[who].canjoust;
  else if ( MAKE_IDSZ( "HMAG" ) == whichskill ) result = ChrList[who].canusedivine;
  else if ( MAKE_IDSZ( "DISA" ) == whichskill ) result = ChrList[who].candisarm;
  else if ( MAKE_IDSZ( "TECH" ) == whichskill ) result = ChrList[who].canusetech;
  else if ( MAKE_IDSZ( "AWEP" ) == whichskill ) result = ChrList[who].canuseadvancedweapons;
  else if ( MAKE_IDSZ( "STAB" ) == whichskill ) result = ChrList[who].canbackstab;
  else if ( MAKE_IDSZ( "POIS" ) == whichskill ) result = ChrList[who].canusepoison;
  else if ( MAKE_IDSZ( "READ" ) == whichskill ) result = ChrList[who].canread;

  return result;
}

//--------------------------------------------------------------------------------------------
int check_player_quest( char *whichplayer, IDSZ idsz )
{
  // ZF> This function checks if the specified player has the IDSZ in his or her quest.txt
  // and returns the quest level of that specific quest (Or -2 if it is not found, -1 if it is finished)

  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz = bfalse;
  Sint8 result = -2;
  Sint8 iTmp;

  //Always return "true" for [NONE] IDSZ checks
  if (idsz == IDSZ_NONE) result = -1;

  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return result;

  // Check each expansion
  while ( fgoto_colon_yesno( fileread ) && !foundidsz )
  {
    newidsz = fget_idsz( fileread );
    if ( newidsz == idsz )
    {
      foundidsz = btrue;
      iTmp = fget_int( fileread );  //Read value behind colon and IDSZ
      result = iTmp;
    }
  }

  fs_fileClose( fileread );

  return result;
}


//--------------------------------------------------------------------------------------------
bool_t add_quest_idsz( char *whichplayer, IDSZ idsz )
{
  // ZF> This function writes a IDSZ (With quest level 0) into a player quest.txt file, returns btrue if succeeded
  FILE *filewrite;
  STRING newloadname;

  // Only add quest IDSZ if it doesnt have it already
  if (check_player_quest(whichplayer, idsz) >= -1) return bfalse;

  // Try to open the file in read and append mode
  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
  filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "a" );
  if ( NULL == filewrite ) return bfalse;

  fprintf( filewrite, "\n:[%4s]: 0", undo_idsz( idsz ) );
  fs_fileClose( filewrite );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fcopy_line(FILE * fileread, FILE * filewrite)
{
  // BB > copy a line of arbitrary length, in chunks of length
  //      sizeof(linebuffer)

  char linebuffer[64];

  if(NULL == fileread || NULL == filewrite) return bfalse;
  if( feof(fileread) || feof(filewrite) ) return bfalse;

  fgets(linebuffer, sizeof(linebuffer), fileread);
  fputs(linebuffer, filewrite);
  while( strlen(linebuffer) == sizeof(linebuffer) )
  {
    fgets(linebuffer, sizeof(linebuffer), fileread);
    fputs(linebuffer, filewrite);
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
int modify_quest_idsz( char *whichplayer, IDSZ idsz, int adjustment )
{
  // ZF> This function increases or decreases a Quest IDSZ quest level by the amount determined in
  //     adjustment. It then returns the current quest level it now has.
  //     It returns -2 if failed and if the adjustment is 0, the quest is marked as beaten...

  FILE *filewrite, *fileread;
  STRING newloadname, copybuffer;
  bool_t foundidsz = bfalse;
  IDSZ newidsz;
  Sint8 NewQuestLevel = -2, QuestLevel;

  // Try to open the file in read/write mode
  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return NewQuestLevel;

  //Now check each expansion until we find correct IDSZ
  while ( fgoto_colon_yesno( fileread ) )
  {
    newidsz = fget_idsz( fileread );

    if ( newidsz == idsz )
    {
      foundidsz = btrue;
      QuestLevel = fget_int( fileread );

      if ( QuestLevel == -1 )					//Quest is already finished, do not modify
      {
        fs_fileClose( fileread );
        return NewQuestLevel;
      }

      // break out of the while loop
      break;
    }
  }

  if(foundidsz)
  {
    // modify the CData.quest_file

    char ctmp;

    //First close the file, rename it and reopen it for reading
    fs_fileClose( fileread );

    // create a "tmp_*" copy of the file
    snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
    snprintf( copybuffer, sizeof( copybuffer ), "%s/%s/tmp_%s", CData.players_dir, whichplayer, CData.quest_file);
    fs_copyFile( newloadname, copybuffer );

    // open the tmp file for reading and overwrite the original file
    fileread  = fs_fileOpen( PRI_NONE, NULL, copybuffer, "r" );
    filewrite = fs_fileOpen( PRI_NONE, NULL, newloadname, "w" );

    // read the tmp file line-by line
    while( !feof(fileread) )
    {
      ctmp = fgetc(fileread);
      ungetc(ctmp, fileread);

      if( ctmp == '/' )
      {
        // copy comments exactly

        fcopy_line(fileread, filewrite);
      }
      else if( fgoto_colon_yesno( fileread ) )
      {
        // scan the line for quest info
        newidsz = fget_idsz( fileread );
        QuestLevel = fget_int( fileread );

        // modify it
        if ( newidsz == idsz )
        {
          if(adjustment == 0) 
          {
            QuestLevel = -1;
          }
          else
          {
            QuestLevel += adjustment;
            if(QuestLevel < 0) QuestLevel = 0;
          }
          NewQuestLevel = QuestLevel;
        }

        // re-emit it
        fprintf(filewrite, "\n:[%s] %i", undo_idsz(idsz), QuestLevel);
      }
    }

    // get rid of the tmp file
    fs_fileClose( filewrite );
    fs_deleteFile( copybuffer );
  }

  fs_fileClose( fileread );

  return NewQuestLevel;
}

//--------------------------------------------------------------------------------------------
bool_t chr_attached( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return bfalse;

  ChrList[chr_ref].attachedto = VALIDATE_CHR( ChrList[chr_ref].attachedto );
  if(!VALID_CHR(chr_ref)) ChrList[chr_ref].inwhichslot = SLOT_NONE;

  return VALID_CHR( ChrList[chr_ref].attachedto );
};

//--------------------------------------------------------------------------------------------
bool_t chr_in_pack( CHR_REF chr_ref )
{
  CHR_REF inwhichpack = chr_get_inwhichpack( chr_ref );
  return VALID_CHR( inwhichpack );
}

//--------------------------------------------------------------------------------------------
bool_t chr_has_inventory( CHR_REF chr_ref )
{
  bool_t retval = bfalse;
  CHR_REF nextinpack = chr_get_nextinpack( chr_ref );

  if ( VALID_CHR( nextinpack ) )
  {
    retval = btrue;
  }
#if defined(_DEBUG) || !defined(NDEBUG)
  else
  {
    assert( ChrList[chr_ref].numinpack == 0 );
  }
#endif

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t chr_is_invisible( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return btrue;

  return FP8_MUL( ChrList[chr_ref].alpha_fp8, ChrList[chr_ref].light_fp8 ) <= INVISIBLE;
};

//--------------------------------------------------------------------------------------------
bool_t chr_using_slot( CHR_REF chr_ref, SLOT slot )
{
  CHR_REF inslot = chr_get_holdingwhich( chr_ref, slot );

  return VALID_CHR( inslot );
};


//--------------------------------------------------------------------------------------------
CHR_REF chr_get_nextinpack( CHR_REF chr_ref )
{
  CHR_REF nextinpack = MAXCHR;

  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  nextinpack = ChrList[chr_ref].nextinpack;
  if ( MAXCHR != nextinpack && !ChrList[chr_ref].on )
  {
    // this is an invalid configuration that may indicate a corrupted list
    nextinpack = ChrList[nextinpack].nextinpack;
    if ( VALID_CHR( nextinpack ) )
    {
      // the list is definitely corrupted
      assert( bfalse );
    }
  }
#endif

  ChrList[chr_ref].nextinpack = VALIDATE_CHR( ChrList[chr_ref].nextinpack );
  return ChrList[chr_ref].nextinpack;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_onwhichplatform( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].onwhichplatform = VALIDATE_CHR( ChrList[chr_ref].onwhichplatform );
  return ChrList[chr_ref].onwhichplatform;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_holdingwhich( CHR_REF chr_ref, SLOT slot )
{
  CHR_REF inslot;

  if ( !VALID_CHR( chr_ref ) || slot >= SLOT_COUNT ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  inslot = ChrList[chr_ref].holdingwhich[slot];
  if ( MAXCHR != inslot )
  {
    CHR_REF holder = ChrList[inslot].attachedto;

    if ( chr_ref != holder )
    {
      // invalid configuration
      assert( bfalse );
    }
  };
#endif

  ChrList[chr_ref].holdingwhich[slot] = VALIDATE_CHR( ChrList[chr_ref].holdingwhich[slot] );
  return ChrList[chr_ref].holdingwhich[slot];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_inwhichpack( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].inwhichpack = VALIDATE_CHR( ChrList[chr_ref].inwhichpack );
  return ChrList[chr_ref].inwhichpack;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_attachedto( CHR_REF chr_ref )
{
  CHR_REF holder;

  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)

  if( MAXCHR != ChrList[chr_ref].attachedto )
  {
    SLOT slot = ChrList[chr_ref].inwhichslot;
    if(slot != SLOT_INVENTORY)
    {
      assert(SLOT_NONE != slot);
      holder = ChrList[chr_ref].attachedto;
      assert( ChrList[holder].holdingwhich[slot] == chr_ref );
    };
  }
  else
  {
    assert(SLOT_NONE == ChrList[chr_ref].inwhichslot);
  };
#endif

  ChrList[chr_ref].attachedto = VALIDATE_CHR( ChrList[chr_ref].attachedto );
  if( !VALID_CHR( ChrList[chr_ref].attachedto ) ) ChrList[chr_ref].inwhichslot = SLOT_NONE;
  return ChrList[chr_ref].attachedto;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_bumpnext( CHR_REF chr_ref )
{
  CHR_REF bumpnext;

  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  bumpnext = ChrList[chr_ref].bumpnext;
  if ( bumpnext < MAXCHR && !ChrList[bumpnext].on && ChrList[bumpnext].bumpnext < MAXCHR )
  {
    // this is an invalid configuration
    assert( bfalse );
    return chr_get_bumpnext( bumpnext );
  }
#endif

  ChrList[chr_ref].bumpnext = VALIDATE_CHR( ChrList[chr_ref].bumpnext );
  return ChrList[chr_ref].bumpnext;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aitarget( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.target = VALIDATE_CHR( ChrList[chr_ref].aistate.target );
  return ChrList[chr_ref].aistate.target;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aiowner( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.owner = VALIDATE_CHR( ChrList[chr_ref].aistate.owner );
  return ChrList[chr_ref].aistate.owner;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aichild( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.child = VALIDATE_CHR( ChrList[chr_ref].aistate.child );
  return ChrList[chr_ref].aistate.child;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aiattacklast( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.attacklast = VALIDATE_CHR( ChrList[chr_ref].aistate.attacklast );
  return ChrList[chr_ref].aistate.attacklast;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aibumplast( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.bumplast = VALIDATE_CHR( ChrList[chr_ref].aistate.bumplast );
  return ChrList[chr_ref].aistate.bumplast;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aihitlast( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.hitlast = VALIDATE_CHR( ChrList[chr_ref].aistate.hitlast );
  return ChrList[chr_ref].aistate.hitlast;
};

//--------------------------------------------------------------------------------------------
CHR_REF team_get_sissy( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  TeamList[iteam].sissy = VALIDATE_CHR( TeamList[iteam].sissy );
  return TeamList[iteam].sissy;
};

//--------------------------------------------------------------------------------------------
CHR_REF team_get_leader( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  TeamList[iteam].leader = VALIDATE_CHR( TeamList[iteam].leader );
  return TeamList[iteam].leader;
};


//--------------------------------------------------------------------------------------------
CHR_REF pla_get_character( PLA_REF iplayer )
{
  if ( !VALID_PLA( iplayer ) ) return MAXCHR;

  PlaList[iplayer].chr = VALIDATE_CHR( PlaList[iplayer].chr );
  return PlaList[iplayer].chr;
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

  FREE( v->Vertices );
  FREE( v->Normals );
  FREE( v->Colors );
  FREE( v->Texture );
  FREE( v->Ambient );
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
  FREE(v);
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
bool_t chr_cvolume_reinit(CHR_REF chr_ref, CVolume * pcv)
{
  if(!VALID_CHR(chr_ref) || NULL == pcv) return bfalse;

  pcv->x_min = ChrList[chr_ref].pos.x - ChrList[chr_ref].bmpdata.size * ChrList[chr_ref].scale * ChrList[chr_ref].pancakepos.x;
  pcv->y_min = ChrList[chr_ref].pos.y - ChrList[chr_ref].bmpdata.size * ChrList[chr_ref].scale * ChrList[chr_ref].pancakepos.y;
  pcv->z_min = ChrList[chr_ref].pos.z;

  pcv->x_max = ChrList[chr_ref].pos.x + ChrList[chr_ref].bmpdata.size * ChrList[chr_ref].scale * ChrList[chr_ref].pancakepos.x;
  pcv->y_max = ChrList[chr_ref].pos.y + ChrList[chr_ref].bmpdata.size * ChrList[chr_ref].scale * ChrList[chr_ref].pancakepos.y;
  pcv->z_max = ChrList[chr_ref].pos.z + ChrList[chr_ref].bmpdata.height * ChrList[chr_ref].scale * ChrList[chr_ref].pancakepos.z;

  pcv->xy_min = -(ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y) - ChrList[chr_ref].bmpdata.sizebig * ChrList[chr_ref].scale * (ChrList[chr_ref].pancakepos.x + ChrList[chr_ref].pancakepos.y) * 0.5f;
  pcv->xy_max = -(ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y) + ChrList[chr_ref].bmpdata.sizebig * ChrList[chr_ref].scale * (ChrList[chr_ref].pancakepos.x + ChrList[chr_ref].pancakepos.y) * 0.5f;

  pcv->yx_min = -(-ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y) - ChrList[chr_ref].bmpdata.sizebig * ChrList[chr_ref].scale * (ChrList[chr_ref].pancakepos.x + ChrList[chr_ref].pancakepos.y) * 0.5f;
  pcv->yx_max = -(-ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y) + ChrList[chr_ref].bmpdata.sizebig * ChrList[chr_ref].scale * (ChrList[chr_ref].pancakepos.x + ChrList[chr_ref].pancakepos.y) * 0.5f;

  pcv->level = -1;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bdata_reinit(CHR_REF chr_ref, BData * pbd)
{
  if(!VALID_CHR(chr_ref) || NULL == pbd) return bfalse;

  pbd->calc_is_platform   = ChrList[chr_ref].isplatform;
  pbd->calc_is_mount      = ChrList[chr_ref].ismount;

  pbd->mids_lo = pbd->mids_hi = ChrList[chr_ref].pos;
  pbd->mids_hi.z += pbd->height * 0.5f;

  pbd->calc_size     = pbd->size    * ChrList[chr_ref].scale * (ChrList[chr_ref].pancakepos.x + ChrList[chr_ref].pancakepos.y) * 0.5f;
  pbd->calc_size_big = pbd->sizebig * ChrList[chr_ref].scale * (ChrList[chr_ref].pancakepos.x + ChrList[chr_ref].pancakepos.y) * 0.5f;

  chr_cvolume_reinit(chr_ref, &pbd->cv);

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers_0( CHR_REF chr_ref )
{
  int i;
  bool_t rv = bfalse;

  if( !VALID_CHR(chr_ref) ) return bfalse;

  // remove any level 3 info
  if(NULL != ChrList[chr_ref].bmpdata.cv_tree)
  {
    for(i=0; i<8; i++)
    {
      (*ChrList[chr_ref].bmpdata.cv_tree)[i].level = -1;
    }
  }

  rv = chr_bdata_reinit( chr_ref, &(ChrList[chr_ref].bmpdata) );

  ChrList[chr_ref].bmpdata.cv.level = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//bool_t md2_calculate_bumpers_1(CHR_REF chr_ref)
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
//  if( !VALID_CHR(chr_ref) ) return bfalse;
//  bd = &(ChrList[chr_ref].bmpdata);
//
//  imdl = ChrList[chr_ref].model;
//  if(!VALID_MDL(imdl) || !ChrList[chr_ref].matrixvalid )
//  {
//    set_default_bump_data( chr_ref );
//    return bfalse;
//  };
//
//  pmdl = MadList[imdl].md2_ptr;
//  if(NULL == pmdl)
//  {
//    set_default_bump_data( chr_ref );
//    return bfalse;
//  }
//
//  fl = md2_get_Frame(pmdl, ChrList[chr_ref].anim.last);
//  fc = md2_get_Frame(pmdl, ChrList[chr_ref].anim.next );
//  lerp = ChrList[chr_ref].anim.flip;
//
//  if(NULL==fl && NULL==fc)
//  {
//    set_default_bump_data( chr_ref );
//    return bfalse;
//  };
//
//  xdir.x = (ChrList[chr_ref].matrix).CNV(0,0);
//  xdir.y = (ChrList[chr_ref].matrix).CNV(0,1);
//  xdir.z = (ChrList[chr_ref].matrix).CNV(0,2);
//
//  ydir.x = (ChrList[chr_ref].matrix).CNV(1,0);
//  ydir.y = (ChrList[chr_ref].matrix).CNV(1,1);
//  ydir.z = (ChrList[chr_ref].matrix).CNV(1,2);
//
//  zdir.x = (ChrList[chr_ref].matrix).CNV(2,0);
//  zdir.y = (ChrList[chr_ref].matrix).CNV(2,1);
//  zdir.z = (ChrList[chr_ref].matrix).CNV(2,2);
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
//  bd->cv.x_min  = cv.x_min  + ChrList[chr_ref].pos.x;
//  bd->cv.y_min  = cv.y_min  + ChrList[chr_ref].pos.y;
//  bd->cv.z_min  = cv.z_min  + ChrList[chr_ref].pos.z;
//  bd->cv.xy_min = cv.xy_min + ( ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y);
//  bd->cv.yx_min = cv.yx_min + (-ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y);
//
//
//  bd->cv.x_max  = cv.x_max  + ChrList[chr_ref].pos.x;
//  bd->cv.y_max  = cv.y_max  + ChrList[chr_ref].pos.y;
//  bd->cv.z_max  = cv.z_max  + ChrList[chr_ref].pos.z;
//  bd->cv.xy_max = cv.xy_max + ( ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y);
//  bd->cv.yx_max = cv.yx_max + (-ChrList[chr_ref].pos.x + ChrList[chr_ref].pos.y);
//
//  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + ChrList[chr_ref].pos.x;
//  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + ChrList[chr_ref].pos.y;
//  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + ChrList[chr_ref].pos.z;
//
//  bd->mids_lo   = bd->mids_hi;
//  bd->mids_lo.z = cv.z_min  + ChrList[chr_ref].pos.z;
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
bool_t md2_calculate_bumpers_1(CHR_REF chr_ref)
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

  if( !VALID_CHR(chr_ref) ) return bfalse;
  bd = &(ChrList[chr_ref].bmpdata);

  imdl = ChrList[chr_ref].model;
  if(!VALID_MDL(imdl) || !ChrList[chr_ref].matrixvalid )
  {
    md2_calculate_bumpers_0( chr_ref );
    return bfalse;
  };

  xdir.x = (ChrList[chr_ref].matrix).CNV(0,0);
  xdir.y = (ChrList[chr_ref].matrix).CNV(0,1);
  xdir.z = (ChrList[chr_ref].matrix).CNV(0,2);

  ydir.x = (ChrList[chr_ref].matrix).CNV(1,0);
  ydir.y = (ChrList[chr_ref].matrix).CNV(1,1);
  ydir.z = (ChrList[chr_ref].matrix).CNV(1,2);

  zdir.x = (ChrList[chr_ref].matrix).CNV(2,0);
  zdir.y = (ChrList[chr_ref].matrix).CNV(2,1);
  zdir.z = (ChrList[chr_ref].matrix).CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = MadList[imdl].md2_ptr;
  if(NULL == pmdl)
  {
    md2_calculate_bumpers_0( chr_ref );
    return bfalse;
  }

  fl = md2_get_Frame(pmdl, ChrList[chr_ref].anim.last);
  fc = md2_get_Frame(pmdl, ChrList[chr_ref].anim.next );
  lerp = ChrList[chr_ref].anim.flip;

  if(NULL==fl && NULL==fc)
  {
    md2_calculate_bumpers_0( chr_ref );
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

  bd->cv.x_min  = cv.x_min;
  bd->cv.y_min  = cv.y_min;
  bd->cv.z_min  = cv.z_min;
  bd->cv.xy_min = cv.xy_min;
  bd->cv.yx_min = cv.yx_min;

  bd->cv.x_max  = cv.x_max;
  bd->cv.y_max  = cv.y_max;
  bd->cv.z_max  = cv.z_max;
  bd->cv.xy_max = cv.xy_max;
  bd->cv.yx_max = cv.yx_max;

  bd->mids_hi.x = (bd->cv.x_min + bd->cv.x_max) * 0.5f + ChrList[chr_ref].pos.x;
  bd->mids_hi.y = (bd->cv.y_min + bd->cv.y_max) * 0.5f + ChrList[chr_ref].pos.y;
  bd->mids_hi.z = (bd->cv.z_min + bd->cv.z_max) * 0.5f + ChrList[chr_ref].pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = bd->cv.z_min + ChrList[chr_ref].pos.z;

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
bool_t md2_calculate_bumpers_2(CHR_REF chr_ref, vect3 * vrt_ary)
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  vect3 xdir, ydir, zdir;
  Uint32 cnt;
  Uint32  vrt_count;
  bool_t  free_array = bfalse;

  CVolume cv;

  if( !VALID_CHR(chr_ref) ) return bfalse;
  bd = &(ChrList[chr_ref].bmpdata);

  imdl = ChrList[chr_ref].model;
  if(!VALID_MDL(imdl) || !ChrList[chr_ref].matrixvalid )
  {
    md2_calculate_bumpers_0( chr_ref );
    return bfalse;
  };

  xdir.x = (ChrList[chr_ref].matrix).CNV(0,0);
  xdir.y = (ChrList[chr_ref].matrix).CNV(0,1);
  xdir.z = (ChrList[chr_ref].matrix).CNV(0,2);

  ydir.x = (ChrList[chr_ref].matrix).CNV(1,0);
  ydir.y = (ChrList[chr_ref].matrix).CNV(1,1);
  ydir.z = (ChrList[chr_ref].matrix).CNV(1,2);

  zdir.x = (ChrList[chr_ref].matrix).CNV(2,0);
  zdir.y = (ChrList[chr_ref].matrix).CNV(2,1);
  zdir.z = (ChrList[chr_ref].matrix).CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = MadList[imdl].md2_ptr;
  if(NULL == pmdl)
  {
    md2_calculate_bumpers_0( chr_ref );
    return bfalse;
  }

  md2_blend_vertices(chr_ref, -1, -1);

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  if(NULL == vrt_ary)
  {
    vrt_ary = malloc(vrt_count * sizeof(vect3));
    if(NULL==vrt_ary)
    {
      return md2_calculate_bumpers_1( chr_ref );
    }
    free_array = btrue;
  }

  // transform the verts all at once, to reduce function calling overhead
  Transform3( 1.0f, 1.0f, &(ChrList[chr_ref].matrix), ChrList[chr_ref].vdata.Vertices, vrt_ary, vrt_count);

  cv.x_min  = cv.x_max  = vrt_ary[0].x;
  cv.y_min  = cv.y_max  = vrt_ary[0].y;
  cv.z_min  = cv.z_max  = vrt_ary[0].z;
  cv.xy_min = cv.xy_max = cv.x_min + cv.y_min;
  cv.yx_min = cv.yx_max = cv.x_min - cv.y_min;

  vrt_count = MadList[imdl].transvertices;
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

  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + ChrList[chr_ref].pos.x;
  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + ChrList[chr_ref].pos.y;
  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + ChrList[chr_ref].pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = cv.z_min + ChrList[chr_ref].pos.z;

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
    FREE( vrt_ary );
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers_3(CHR_REF chr_ref, CVolume_Tree * cv_tree)
{
  BData * bd;
  Uint16 imdl;
  MD2_Model * pmdl;
  Uint32 cnt, tnc;
  Uint32  tri_count, vrt_count;
  vect3 * vrt_ary;
  CVolume *pcv, cv_node[8];

  if( !VALID_CHR(chr_ref) ) return bfalse;
  bd = &(ChrList[chr_ref].bmpdata);

  imdl = ChrList[chr_ref].model;
  if(!VALID_MDL(imdl) || !ChrList[chr_ref].matrixvalid )
  {
    md2_calculate_bumpers_0( chr_ref );
    return bfalse;
  };

  pmdl = MadList[imdl].md2_ptr;
  if(NULL == pmdl)
  {
    md2_calculate_bumpers_0( chr_ref );
    return bfalse;
  }

  // allocate the array
  vrt_count = md2_get_numVertices(pmdl);
  vrt_ary   = malloc(vrt_count * sizeof(vect3));
  if(NULL==vrt_ary) return md2_calculate_bumpers_1( chr_ref );

  // make sure that we have the correct bounds
  if(bd->cv.level < 2)
  {
    md2_calculate_bumpers_2(chr_ref, vrt_ary);
  }

  if(NULL == cv_tree) return bfalse;

  // transform the verts all at once, to reduce function calling overhead
  Transform3( 1.0f, 1.0f, &(ChrList[chr_ref].matrix), ChrList[chr_ref].vdata.Vertices, vrt_ary, vrt_count);

  pcv = &(ChrList[chr_ref].bmpdata.cv);

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
    MD2_Triangle * pmd2_tri = md2_get_Triangle(pmdl, cnt);
    short * tri = pmd2_tri->vertexIndices;
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

  FREE( vrt_ary );

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers(CHR_REF chr_ref, int level)
{
  bool_t retval = bfalse;

  if(ChrList[chr_ref].bmpdata.cv.level >= level) return btrue;

  switch(level)
  {
    case 2:
      // the collision volume is an octagon, the ranges are calculated using the model's vertices
      retval = md2_calculate_bumpers_2(chr_ref, NULL);
      break;

    case 3:
      {
        // calculate the octree collision volume
        if(NULL == ChrList[chr_ref].bmpdata.cv_tree)
        {
          ChrList[chr_ref].bmpdata.cv_tree = calloc(1, sizeof(CVolume_Tree));
        };
        retval = md2_calculate_bumpers_3(chr_ref, ChrList[chr_ref].bmpdata.cv_tree);
      };
      break;

    case 1:
      // the collision volume is a simple axis-aligned bounding box, the range is calculated from the
      // md2's bounding box
      retval = md2_calculate_bumpers_1(chr_ref);

    default:
    case 0:
      // make the simplest estimation of the bounding box using the data in data.txt
      retval = md2_calculate_bumpers_0(chr_ref);
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
void cv_list_draw()
{
  Uint32 cnt;

  for(cnt=0; cnt<cv_list_count; cnt++)
  {
    draw_CVolume( &(cv_list[cnt]) );
  };
}

//--------------------------------------------------------------------------------------------
WP_LIST * wp_list_new(WP_LIST * w, vect3 * pos)
{
  w->tail = 0;
  w->head = 1;

  if(NULL == pos)
  {
    w->pos[0].x = w->pos[0].y = 0;
  }
  else
  {
    w->pos[0].x = pos->x;
    w->pos[0].y = pos->y;
  }

  return w;
}

AI_STATE * ai_state_new(AI_STATE * a, Uint16 ichr)
{
  int tnc;

  CHR * pchr;
  MAD * pmad;
  CAP * pcap;

  if(NULL == a) return NULL;

  memset(a, 0, sizeof(AI_STATE));

  if( !VALID_CHR_RANGE(ichr) ) return NULL;

  pchr = ChrList + ichr;

  if( !VALID_MDL_RANGE(pchr->model) ) return NULL;

  pmad = MadList + pchr->model;
  pcap = CapList + pchr->model;

  a->type    = pmad->ai;
  a->alert   = ALERT_SPAWNED;
  a->state   = pcap->stateoverride;
  a->content = pcap->contentoverride;
  a->target  = ichr;
  a->owner   = ichr;
  a->child   = ichr;
  a->time    = 0;

  tnc = 0;
  while ( tnc < MAXSTOR )
  {
    a->x[tnc] = 0;
    a->y[tnc] = 0;
    tnc++;
  }

  wp_list_new( &(a->wp), &(pchr->pos) );

  a->morphed = bfalse;

  a->latch.x = 0;
  a->latch.y = 0;
  a->latch.b = 0;
  a->turnmode = TURNMODE_VELOCITY;

  a->bumplast   = ichr;
  a->attacklast = MAXCHR;
  a->hitlast    = ichr;

  a->trgvel.x = 0;
  a->trgvel.y = 0;
  a->trgvel.z = 0;

  return a;
};


AI_STATE * ai_state_renew(AI_STATE * a, Uint16 ichr)
{
  int tnc;

  if(NULL == a) return NULL;

  if( !VALID_CHR_RANGE(ichr) )
  {
    memset(a, 0, sizeof(AI_STATE));
    return NULL;
  }

  a->alert = ALERT_NONE;
  a->target = ichr;
  a->time = 0;
  a->trgvel.x = 0;
  a->trgvel.y = 0;
  a->trgvel.z = 0;

  return a;
};

ACTION_INFO * action_info_new( ACTION_INFO * a)
{
  if(NULL == a) return NULL;

  a->ready = btrue;
  a->keep  = bfalse;
  a->loop  = bfalse;
  a->now   = ACTION_DA;
  a->next  = ACTION_DA;

  return a;
}

ANIM_INFO * anim_info_new( ANIM_INFO * a )
{
  if(NULL == a) return NULL;

  a->lip_fp8 = 0;
  a->flip    = 0.0f;
  a->next    = a->last = 0;

  return a;
};


//--------------------------------------------------------------------------------------------
void damage_character( CHR_REF chr_ref, Uint16 direction,
                       PAIR * pdam, DAMAGE damagetype, TEAM team,
                       Uint16 attacker, Uint16 effects )
{
  // ZZ> This function calculates and applies damage to a character.  It also
  //     sets alerts and begins actions.  Blocking and frame invincibility
  //     are done here too.  Direction is FRONT if the attack is coming head on,
  //     RIGHT if from the right, BEHIND if from the back, LEFT if from the
  //     left.

  int tnc;
  ACTION action;
  int damage, basedamage;
  Uint16 experience, model, left, right;
  AI_STATE * pstate;
  CHR * pchr;
  MAD * pmad;
  CAP * pcap;

  if( !VALID_CHR(chr_ref) ) return;

  pchr   = ChrList + chr_ref;
  pstate = &(pchr->aistate);

  if ( NULL == pdam ) return;
  if ( pchr->isplayer && CData.DevMode ) return;

  if ( pchr->alive && pdam->ibase >= 0 && pdam->irand >= 1 )
  {
    // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    damage = generate_unsigned( pdam );
    basedamage = damage;
    damage >>= ( pchr->damagemodifier_fp8[damagetype] & DAMAGE_SHIFT );


    // Allow charging (Invert damage to mana)
    if ( pchr->damagemodifier_fp8[damagetype]&DAMAGE_CHARGE )
    {
      pchr->mana_fp8 += damage;
      if ( pchr->mana_fp8 > pchr->manamax_fp8 )
      {
        pchr->mana_fp8 = pchr->manamax_fp8;
      }
      return;
    }

    // Mana damage (Deal damage to mana)
    if ( pchr->damagemodifier_fp8[damagetype]&DAMAGE_MANA )
    {
      pchr->mana_fp8 -= damage;
      if ( pchr->mana_fp8 < 0 )
      {
        pchr->mana_fp8 = 0;
      }
      return;
    }


    // Invert damage to heal
    if ( pchr->damagemodifier_fp8[damagetype]&DAMAGE_INVERT )
      damage = -damage;


    // Remember the damage type
    pstate->damagetypelast = damagetype;
    pstate->directionlast = direction;


    // Do it already
    if ( damage > 0 )
    {
      // Only damage if not invincible
      if ( pchr->damagetime == 0 && !pchr->invictus )
      {
        model = pchr->model;
        pmad = MadList + model;
        pcap = CapList + model;
        if ( HAS_SOME_BITS( effects, DAMFX_BLOC ) )
        {
          // Only damage if hitting from proper direction
          if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_INVICTUS ) )
          {
            // I Frame...
            direction -= pcap->iframefacing;
            left = ( ~pcap->iframeangle );
            right = pcap->iframeangle;

            // Check for shield
            if ( pchr->action.now >= ACTION_PA && pchr->action.now <= ACTION_PD )
            {
              // Using a shield?
              if ( pchr->action.now < ACTION_PC )
              {
                // Check left hand
                CHR_REF iholder = chr_get_holdingwhich( chr_ref, SLOT_LEFT );
                if ( VALID_CHR( iholder ) )
                {
                  left  = ~CapList[iholder].iframeangle;
                  right = CapList[iholder].iframeangle;
                }
              }
              else
              {
                // Check right hand
                CHR_REF iholder = chr_get_holdingwhich( chr_ref, SLOT_RIGHT );
                if ( VALID_CHR( iholder ) )
                {
                  left  = ~CapList[iholder].iframeangle;
                  right = CapList[iholder].iframeangle;
                }
              }
            }
          }
          else
          {
            // N Frame
            direction -= pcap->nframefacing;
            left = ( ~pcap->nframeangle );
            right = pcap->nframeangle;
          }
          // Check that direction
          if ( direction > left || direction < right )
          {
            damage = 0;
          }
        }



        if ( damage != 0 )
        {
          if ( HAS_SOME_BITS( effects, DAMFX_ARMO ) )
          {
            pchr->life_fp8 -= damage;
          }
          else
          {
            pchr->life_fp8 -= FP8_MUL( damage, pchr->defense_fp8 );
          }


          if ( basedamage > DAMAGE_MIN )
          {
            // Call for help if below 1/2 life
            if ( pchr->life_fp8 < ( pchr->lifemax_fp8 >> 1 ) ) //Zefz: Removed, because it caused guards to attack
              call_for_help( chr_ref );                    //when dispelling overlay spells (Faerie Light)

            // Spawn blud particles
            if ( pcap->bludlevel > BLUD_NONE && ( damagetype < DAMAGE_HOLY || pcap->bludlevel == BLUD_ULTRA ) )
            {
              spawn_one_particle( 1.0f, pchr->pos,
                                  pchr->turn_lr + direction, pchr->model, pcap->bludprttype,
                                  MAXCHR, GRIP_LAST, pchr->team, chr_ref, 0, MAXCHR );
            }
            // Set attack alert if it wasn't an accident
            if ( team == TEAM_DAMAGE )
            {
              pstate->attacklast = MAXCHR;
            }
            else
            {
              // Don't alert the chr_ref too much if under constant fire
              if ( pchr->carefultime == 0 )
              {
                // Don't let ichrs chase themselves...  That would be silly
                if ( attacker != chr_ref )
                {
                  pstate->alert |= ALERT_ATTACKED;
                  pstate->attacklast = attacker;
                  pchr->carefultime = DELAY_CAREFUL;
                }
              }
            }
          }


          // Taking damage action
          action = ACTION_HA;
          if ( pchr->life_fp8 < 0 )
          {
            // Character has died
            pchr->alive = bfalse;
            disenchant_character( chr_ref );
            pchr->action.keep = btrue;
            pchr->life_fp8 = -1;
            pchr->isplatform = btrue;
            pchr->bumpdampen /= 2.0;
            action = ACTION_KA;
            stop_sound(pchr->loopingchannel);    //Stop sound loops
            pchr->loopingchannel = -1;
            // Give kill experience
            experience = pcap->experienceworth + ( pchr->experience * pcap->experienceexchange );
            if ( VALID_CHR( attacker ) )
            {
              // Set target
              pstate->target = attacker;
              if ( team == TEAM_DAMAGE )  pstate->target = chr_ref;
              if ( team == TEAM_NULL )  pstate->target = chr_ref;
              // Award direct kill experience
              if ( TeamList[ChrList[attacker].team].hatesteam[pchr->team] )
              {
                give_experience( attacker, experience, XP_KILLENEMY );
              }

              // Check for hated
              if ( CAP_INHERIT_IDSZ( model, CapList[ChrList[attacker].model].idsz[IDSZ_HATE] ) )
              {
                give_experience( attacker, experience, XP_KILLHATED );
              }
            }

            // Clear all shop passages that it owned...
            tnc = 0;
            while ( tnc < numshoppassage )
            {
              if ( shopowner[tnc] == chr_ref )
              {
                shopowner[tnc] = NOOWNER;
              }
              tnc++;
            }

            // Let the other characters know it died
            tnc = 0;
            while ( tnc < MAXCHR )
            {
              if ( ChrList[tnc].on && ChrList[tnc].alive )
              {
                if ( ChrList[tnc].aistate.target == chr_ref )
                {
                  ChrList[tnc].aistate.alert |= ALERT_TARGETKILLED;
                }
                if ( !TeamList[ChrList[tnc].team].hatesteam[team] && TeamList[ChrList[tnc].team].hatesteam[pchr->team] )
                {
                  // All allies get team experience, but only if they also hate the dead guy's team
                  give_experience( tnc, experience, XP_TEAMKILL );
                }
              }
              tnc++;
            }

            // Check if it was a leader
            if ( team_get_leader( pchr->team ) == chr_ref )
            {
              // It was a leader, so set more alerts
              tnc = 0;
              while ( tnc < MAXCHR )
              {
                if ( ChrList[tnc].on && ChrList[tnc].team == pchr->team )
                {
                  // All folks on the leaders team get the alert
                  ChrList[tnc].aistate.alert |= ALERT_LEADERKILLED;
                }
                tnc++;
              }

              // The team now has no leader
              TeamList[pchr->team].leader = search_best_leader( pchr->team, chr_ref );
            }

            detach_character_from_mount( chr_ref, btrue, bfalse );
            action += ( rand() & 3 );
            play_action( chr_ref, action, bfalse );

            // Turn off all sounds if it's a player
            for ( tnc = 0; tnc < MAXWAVE; tnc++ )
            {
              //TODO Zefz: Do we need this? This makes all sounds a chr_ref makes stop when it dies...
              //           This may stop death sounds
              //stop_sound(pchr->model);
            }

            // Afford it one last thought if it's an AI
            TeamList[pchr->baseteam].morale--;
            pchr->team = pchr->baseteam;
            pstate->alert = ALERT_KILLED;
            pchr->sparkle = NOSPARKLE;
            pstate->time = 1;  // No timeout...
            let_character_think( chr_ref, 1.0f );
          }
          else
          {
            if ( basedamage > DAMAGE_MIN )
            {
              action += ( rand() & 3 );
              play_action( chr_ref, action, bfalse );

              // Make the chr_ref invincible for a limited time only
              if ( HAS_NO_BITS( effects, DAMFX_TIME ) )
                pchr->damagetime = DELAY_DAMAGE;
            }
          }
        }
        else
        {
          // Spawn a defend particle
          spawn_one_particle( pchr->bumpstrength, pchr->pos, pchr->turn_lr, MAXMODEL, PRTPIP_DEFEND, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
          pchr->damagetime = DELAY_DEFEND;
          pstate->alert |= ALERT_BLOCKED;
        }
      }
    }
    else if ( damage < 0 )
    {
      pchr->life_fp8 -= damage;
      if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;

      // Isssue an alert
      pstate->alert |= ALERT_HEALED;
      pstate->attacklast = attacker;
      if ( team != TEAM_DAMAGE )
      {
        pstate->attacklast = MAXCHR;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void kill_character( CHR_REF chr_ref, Uint16 killer )
{
  // ZZ> This function kills a chr_ref...  MAXCHR killer for accidental death

  Uint8 modifier;
  CHR * pchr;

  if( !VALID_CHR(chr_ref) ) return;

  pchr = ChrList + chr_ref;

  if ( !pchr->alive ) return;

  pchr->damagetime = 0;
  pchr->life_fp8 = 1;
  modifier = pchr->damagemodifier_fp8[DAMAGE_CRUSH];
  pchr->damagemodifier_fp8[DAMAGE_CRUSH] = 1;
  if ( VALID_CHR( killer ) )
  {
    PAIR ptemp = {512, 1};
    damage_character( chr_ref, 0, &ptemp, DAMAGE_CRUSH, ChrList[killer].team, killer, DAMFX_ARMO | DAMFX_BLOC );
  }
  else
  {
    PAIR ptemp = {512, 1};
    damage_character( chr_ref, 0, &ptemp, DAMAGE_CRUSH, TEAM_DAMAGE, chr_get_aibumplast( chr_ref ), DAMFX_ARMO | DAMFX_BLOC );
  }
  pchr->damagemodifier_fp8[DAMAGE_CRUSH] = modifier;

  // try something here.
  pchr->isplatform = btrue;
  pchr->ismount    = bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t wp_list_advance(WP_LIST * wl)
{
  // BB > return value of btrue means wp_list is empty

  bool_t retval = bfalse;

  if(NULL == wl) return retval;

  if( wl->tail != wl->head )
  {
    // advance the tail and let it wrap around
    wl->tail = (wl->tail + 1) % MAXWAY;
  }

  if ( wl->tail == wl->head )
  {
    retval = btrue;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t wp_list_add(WP_LIST * wl, float x, float y)
{
  // BB > add a point to the waypoint list. 
  //      returns bfalse if the list is full (?or should it advance the tail?)

  bool_t retval = bfalse;
  int    test;

  if(NULL == wl) return retval;

  test = (wl->head + 1) % MAXWAY;

  if(test == wl->tail) return bfalse;

  wl->pos[wl->head].x = x;
  wl->pos[wl->head].y = y;

  wl->head = test;

  return btrue;
};


//--------------------------------------------------------------------------------------------
BUMPLIST * bumplist_new(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  memset(b, 0, sizeof(BUMPLIST));

  return b;
};

//--------------------------------------------------------------------------------------------
void bumplist_delete(BUMPLIST * b)
{
  if(NULL == b) return;

  b->valid = bfalse;

  if(0 == b->num_blocks) return;

  b->num_blocks = 0;

  FREE(b->num_chr);
  FREE(b->chr);

  FREE(b->num_prt);
  FREE(b->prt);
}

//--------------------------------------------------------------------------------------------
BUMPLIST * bumplist_renew(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  bumplist_delete(b);
  return bumplist_new(b);
}

//--------------------------------------------------------------------------------------------
bool_t bumplist_allocate(BUMPLIST * b, int size)
{
  if(NULL == b) return bfalse;

  if(size <= 0)
  {
    bumplist_renew(b);
  }
  else
  {
    b->valid      = bfalse;
    b->num_blocks = size;

    b->num_chr = calloc(size, sizeof(Uint16));
    b->chr     = calloc(size, sizeof(Uint16));

    b->num_prt = calloc(size, sizeof(Uint16));
    b->prt     = calloc(size, sizeof(Uint16));
  }

  return btrue;
};