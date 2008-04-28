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

int     numfreechr = 0;         // For allocation
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

  model = ChrList[character].model;
  inext = ChrList[character].anim.next;
  ilast = ChrList[character].anim.last;
  flip = ChrList[character].anim.flip;

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  pmdl  = MadList[model].md2_ptr;
  plast = md2_get_Frame(pmdl, ilast);
  pnext = md2_get_Frame(pmdl, inext);

  for ( cnt = 0; cnt < MadList[model].transvertices; cnt++ )
  {
    z = pnext->vertices[cnt].z + (pnext->vertices[cnt].z - plast->vertices[cnt].z) * flip;

    if ( z < low )
    {
      ChrList[character].vrtar_fp8[cnt] =
      ChrList[character].vrtag_fp8[cnt] =
      ChrList[character].vrtab_fp8[cnt] = valuelow;
    }
    else if ( z > high )
    {
      ChrList[character].vrtar_fp8[cnt] =
      ChrList[character].vrtag_fp8[cnt] =
      ChrList[character].vrtab_fp8[cnt] = valuehigh;
    }
    else
    {
      float ftmp = (float)( z - low ) / (float)( high - low );
      ChrList[character].vrtar_fp8[cnt] =
      ChrList[character].vrtag_fp8[cnt] =
      ChrList[character].vrtab_fp8[cnt] = valuelow + (valuehigh - valuelow) * ftmp;
    }
  }
}

//--------------------------------------------------------------------------------------------
void flash_character( CHR_REF character, Uint8 value )
{
  // ZZ> This function sets a character's lighting
  int cnt;
  Uint16 model = ChrList[character].model;

  assert( MAXMODEL != VALIDATE_MDL( model ) );

  cnt = 0;
  while ( cnt < MadList[model].transvertices )
  {
    ChrList[character].vrtar_fp8[cnt] =
      ChrList[character].vrtag_fp8[cnt] =
        ChrList[character].vrtab_fp8[cnt] = value;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void add_to_dolist( CHR_REF ichr )
{
  // This function puts a character in the list
  int fan;

  if ( !VALID_CHR( ichr ) || ChrList[ichr].indolist ) return;

  fan = ChrList[ichr].onwhichfan;
  //if ( mesh_in_renderlist( fan ) )
  {
    //ChrList[ichr].lightspek_fp8 = Mesh[meshvrtstart[fan]].vrtl_fp8;
    dolist[numdolist] = ichr;
    ChrList[ichr].indolist = btrue;
    numdolist++;


    // Do flashing
    if (( allframe & ChrList[ichr].flashand ) == 0 && ChrList[ichr].flashand != DONTFLASH )
    {
      flash_character( ichr, 255 );
    }
    // Do blacking
    if (( allframe&SEEKURSEAND ) == 0 && localseekurse && ChrList[ichr].iskursed )
    {
      flash_character( ichr, 0 );
    }
  }
  //else
  //{
  //  Uint16 model = ChrList[ichr].model;
  //  assert( MAXMODEL != VALIDATE_MDL( model ) );

  //  // Double check for large/special objects
  //  if ( CapList[model].alwaysdraw )
  //  {
  //    dolist[numdolist] = ichr;
  //    ChrList[ichr].indolist = btrue;
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
  // ZZ> This function GOrder.s the dolist based on distance from camera,
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
    if ( ChrList[character].light_fp8 != 255 || ChrList[character].alpha_fp8 != 255 )
    {
      // This makes stuff inside an invisible character visible...
      // A key inside a Jellcube, for example
      dist[cnt] = 0x7fffffff;
    }
    else
    {
      dist[cnt] = ABS( ChrList[character].pos.x - GCamera.pos.x ) + ABS( ChrList[character].pos.y - GCamera.pos.y );
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
    ChrList[character].indolist = bfalse;
    cnt++;
  }
  numdolist = 0;


  // Now fill it up again
  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( ChrList[cnt].on && !chr_in_pack( cnt ) )
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
          ChrList[character].pos = ChrList[cnt].pos;
          ChrList[character].pos_old = ChrList[cnt].pos_old;  // Copy olds to make SendMessageNear work
          character  = chr_get_nextinpack( character );
        }
      }
    }
    else
    {
      // Keep in hand weapons with character
      if ( ChrList[character].matrixvalid && ChrList[cnt].matrixvalid )
      {
        ChrList[cnt].pos.x = ChrList[cnt].matrix _CNV( 3, 0 );
        ChrList[cnt].pos.y = ChrList[cnt].matrix _CNV( 3, 1 );
        ChrList[cnt].pos.z = ChrList[cnt].matrix _CNV( 3, 2 );
      }
      else
      {
        ChrList[cnt].pos.x = ChrList[character].pos.x;
        ChrList[cnt].pos.y = ChrList[character].pos.y;
        ChrList[cnt].pos.z = ChrList[character].pos.z;
      }
      ChrList[cnt].turn_lr = ChrList[character].turn_lr;

      // Copy this stuff ONLY if it's a weapon, not for mounts
      if ( ChrList[character].transferblend && ChrList[cnt].isitem )
      {
        if ( ChrList[character].alpha_fp8 != 255 )
        {
          Uint16 model = ChrList[cnt].model;
          assert( MAXMODEL != VALIDATE_MDL( model ) );
          ChrList[cnt].alpha_fp8 = ChrList[character].alpha_fp8;
          ChrList[cnt].bumpstrength = CapList[model].bumpstrength * FP8_TO_FLOAT( ChrList[cnt].alpha_fp8 );
        }
        if ( ChrList[character].light_fp8 != 255 )
        {
          ChrList[cnt].light_fp8 = ChrList[character].light_fp8;
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

  mat_old = ChrList[cnt].matrix;
  ChrList[cnt].matrixvalid = bfalse;

  if ( ChrList[cnt].overlay )
  {
    // Overlays are kept with their target...
    tnc = chr_get_aitarget( cnt );

    if ( VALID_CHR( tnc ) )
    {
      ChrList[cnt].pos.x = ChrList[tnc].matrix _CNV( 3, 0 );
      ChrList[cnt].pos.y = ChrList[tnc].matrix _CNV( 3, 1 );
      ChrList[cnt].pos.z = ChrList[tnc].matrix _CNV( 3, 2 );

      ChrList[cnt].matrix = ChrList[tnc].matrix;

      ChrList[cnt].matrix _CNV( 0, 0 ) *= ChrList[cnt].pancakepos.x;
      ChrList[cnt].matrix _CNV( 1, 0 ) *= ChrList[cnt].pancakepos.x;
      ChrList[cnt].matrix _CNV( 2, 0 ) *= ChrList[cnt].pancakepos.x;

      ChrList[cnt].matrix _CNV( 0, 1 ) *= ChrList[cnt].pancakepos.y;
      ChrList[cnt].matrix _CNV( 1, 1 ) *= ChrList[cnt].pancakepos.y;
      ChrList[cnt].matrix _CNV( 2, 1 ) *= ChrList[cnt].pancakepos.y;

      ChrList[cnt].matrix _CNV( 0, 2 ) *= ChrList[cnt].pancakepos.z;
      ChrList[cnt].matrix _CNV( 1, 2 ) *= ChrList[cnt].pancakepos.z;
      ChrList[cnt].matrix _CNV( 2, 2 ) *= ChrList[cnt].pancakepos.z;

      ChrList[cnt].matrixvalid = btrue;

      recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[cnt].matrix));
    }
  }
  else
  {
    ChrList[cnt].matrix = ScaleXYZRotateXYZTranslate( ChrList[cnt].scale * ChrList[cnt].pancakepos.x, ChrList[cnt].scale * ChrList[cnt].pancakepos.y, ChrList[cnt].scale * ChrList[cnt].pancakepos.z,
                     ChrList[cnt].turn_lr >> 2,
                     (( Uint16 )( ChrList[cnt].mapturn_ud + 32768 ) ) >> 2,
                     (( Uint16 )( ChrList[cnt].mapturn_lr + 32768 ) ) >> 2,
                     ChrList[cnt].pos.x, ChrList[cnt].pos.y, ChrList[cnt].pos.z );

    ChrList[cnt].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[cnt].matrix));
  }

  if(ChrList[cnt].matrixvalid && recalc_bumper)
  {
    // invalidate the cached bumper data
    ChrList[cnt].bmpdata.cv.level = -1;
  };

  return ChrList[cnt].matrixvalid;
}

//--------------------------------------------------------------------------------------------
void free_one_character( CHR_REF ichr )
{
  // ZZ> This function sticks a ichr back on the free ichr stack
  int cnt;

  if ( !VALID_CHR( ichr ) ) return;

  fprintf( stdout, "free_one_character() - \n\tprofile == %d, CapList[profile].classname == \"%s\", index == %d\n", ChrList[ichr].model, CapList[ChrList[ichr].model].classname, ichr );

  //remove any collision volume octree
  if(NULL != ChrList[ichr].bmpdata.cv_tree)
  {
    free( ChrList[ichr].bmpdata.cv_tree );
    ChrList[ichr].bmpdata.cv_tree = NULL;
  }

  // add it to the free list
  freechrlist[numfreechr] = ichr;
  numfreechr++;

  // Remove from stat list
  if ( ChrList[ichr].staton )
  {
    ChrList[ichr].staton = bfalse;
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
  assert( MAXMODEL != VALIDATE_MDL( ChrList[ichr].model ) );
  if ( ChrList[ichr].alive && !CapList[ChrList[ichr].model].invictus )
  {
    TeamList[ChrList[ichr].baseteam].morale--;
  }

  cnt = 0;
  while ( cnt < MAXCHR )
  {
    if ( ChrList[cnt].on )
    {
      if ( ChrList[cnt].aitarget == ichr )
      {
        ChrList[cnt].alert |= ALERT_TARGETKILLED;
        ChrList[cnt].aitarget = cnt;
      }
      if ( team_get_leader( ChrList[cnt].team ) == ichr )
      {
        ChrList[cnt].alert |= ALERT_LEADERKILLED;
      }
    }
    cnt++;
  }

  if ( team_get_leader( ChrList[ichr].team ) == ichr )
  {
    TeamList[ChrList[ichr].team].leader = MAXCHR;
  }

  if( INVALID_CHANNEL != ChrList[ichr].loopingchannel )
  {
    stop_sound( ChrList[ichr].loopingchannel );
    ChrList[ichr].loopingchannel = INVALID_CHANNEL;
  };

  ChrList[ichr].on = bfalse;
  ChrList[ichr].alive = bfalse;
  ChrList[ichr].inwhichpack = MAXCHR;
  VData_Blended_Deallocate(&(ChrList[ichr].vdata));
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
    ChrList[cnt].freeme = btrue;
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
  PRT * pprt;
  CHR * pchr;


  // Check validity of attachment
  if ( !VALID_CHR( character ) || chr_in_pack( character ) || !VALID_PRT( particle ) )
  {
    PrtList[particle].gopoof = btrue;
    return;
  }

  pprt = PrtList + particle;
  pchr = ChrList + character;

  // Do we have a matrix???
  if ( pchr->matrixvalid )
  {
    // Transform the weapon grip from model to world space

    if ( vertoffset == GRIP_ORIGIN )
    {
      pprt->pos.x = pchr->matrix _CNV( 3, 0 );
      pprt->pos.y = pchr->matrix _CNV( 3, 1 );
      pprt->pos.z = pchr->matrix _CNV( 3, 2 );
    }
    else
    {
      Uint32 ilast, inext;
      MD2_Model * pmdl;
      MD2_Frame * plast, * pnext;

      model = pchr->model;
      inext = pchr->anim.next;
      ilast = pchr->anim.last;
      flip  = pchr->anim.flip;

      assert( MAXMODEL != VALIDATE_MDL( model ) );

      pmdl  = MadList[model].md2_ptr;
      plast = md2_get_Frame(pmdl, ilast);
      pnext = md2_get_Frame(pmdl, inext);

      //handle possible invalid values
      vertex = ( MadList[model].vertices >= vertoffset ) ? MadList[model].vertices - vertoffset : MadList[model].vertices - GRIP_LAST;

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
      Transform4_Full( &(pchr->matrix), &point, &nupoint, 1 );

      pprt->pos.x = nupoint.x;
      pprt->pos.y = nupoint.y;
      pprt->pos.z = nupoint.z;
    }
  }
  else
  {
    // No matrix, so just wing it...
    pprt->pos.x = pchr->pos.x;
    pprt->pos.y = pchr->pos.y;
    pprt->pos.z = pchr->pos.z;
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
  ChrList[ichr].matrixvalid = bfalse;

  // check that the mount is valid
  imount = chr_get_attachedto( ichr );
  if ( !VALID_CHR( imount ) )
  {
    ChrList[ichr].matrix = ZeroMatrix();
    return bfalse;
  }

  mat_old = ChrList[ichr].matrix;

  if(0xFFFF == ChrList[ichr].attachedgrip[0])
  {
    // Calculate weapon's matrix
    ChrList[ichr].matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, ChrList[ichr].turn_lr + ChrList[imount].turn_lr, ChrList[imount].pos.x, ChrList[imount].pos.y, ChrList[imount].pos.z);
    ChrList[ichr].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[ichr].matrix));
  }
  else if(0xFFFF == ChrList[ichr].attachedgrip[1])
  {
    // do the linear interpolation
    vertex = ChrList[ichr].attachedgrip[0];
    md2_blend_vertices(imount, vertex, vertex);

    // Calculate weapon's matrix
    ChrList[ichr].matrix = ScaleXYZRotateXYZTranslate( 1, 1, 1, 0, 0, ChrList[ichr].turn_lr + ChrList[imount].turn_lr, ChrList[imount].vdata.Vertices[vertex].x, ChrList[imount].vdata.Vertices[vertex].y, ChrList[imount].vdata.Vertices[vertex].z);
    ChrList[ichr].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[ichr].matrix));
  }
  else
  {
    GLvector point[GRIP_SIZE], nupoint[GRIP_SIZE];

    // do the linear interpolation
    vertex = ChrList[ichr].attachedgrip[0];
    md2_blend_vertices(imount, vertex, vertex+GRIP_SIZE);

    for ( cnt = 0; cnt < GRIP_SIZE; cnt++ )
    {
      point[cnt].x = ChrList[imount].vdata.Vertices[vertex+cnt].x;
      point[cnt].y = ChrList[imount].vdata.Vertices[vertex+cnt].y;
      point[cnt].z = ChrList[imount].vdata.Vertices[vertex+cnt].z;
      point[cnt].w = 1.0f;
    };

    // Do the transform
    Transform4_Full( &(ChrList[imount].matrix), point, nupoint, GRIP_SIZE );

    // Calculate weapon's matrix based on positions of grip points
    // chrscale is recomputed at time of attachment
    ChrList[ichr].matrix = FourPoints( nupoint[0], nupoint[1], nupoint[2], nupoint[3], 1.0 );
    ChrList[ichr].pos.x = (ChrList[ichr].matrix)_CNV(3,0);
    ChrList[ichr].pos.y = (ChrList[ichr].matrix)_CNV(3,1);
    ChrList[ichr].pos.z = (ChrList[ichr].matrix)_CNV(3,2);
    ChrList[ichr].matrixvalid = btrue;

    recalc_bumper = matrix_compare_3x3(&mat_old, &(ChrList[ichr].matrix));
  };

  if(ChrList[ichr].matrixvalid && recalc_bumper)
  {
    // invalidate the cached bumper data
    md2_calculate_bumpers(ichr, 0);
  };

  return ChrList[ichr].matrixvalid;
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
    ChrList[ichr].matrixvalid = bfalse;
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
      if ( ChrList[ichr].matrixvalid || !VALID_CHR( ichr ) ) continue;

      attached = chr_get_attachedto( ichr );
      if ( !VALID_CHR( attached ) ) continue;

      if ( !ChrList[attached].matrixvalid )
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

  for ( cnt = 0, charb = VALID_CHR( bumplist.chr[fanblock] ); cnt < bumplist.num_chr[fanblock] && VALID_CHR( charb ); cnt++, charb = chr_get_bumpnext( charb ) )
  {
    // don't find stupid stuff
    if ( !VALID_CHR( charb ) || 0.0f == ChrList[charb].bumpstrength ) continue;

    if ( !ChrList[charb].alive || ChrList[charb].invictus || chr_in_pack( charb ) ) continue;

    if ( charb == donttarget || charb == oldtarget ) continue;

    if ( allow_anyone || ( request_friends && !TeamList[team].hatesteam[ChrList[charb].team] ) || ( request_enemies && TeamList[team].hatesteam[ChrList[charb].team] ) )
    {
      local_distance = ABS( ChrList[charb].pos.x - prtx ) + ABS( ChrList[charb].pos.y - prty );
      if ( local_distance < g_search.bestdistance )
      {
        local_angle = facing - vec_to_turn( ChrList[charb].pos.x - prtx, ChrList[charb].pos.y - prty );
        if ( local_angle < g_search.bestangle || local_angle > ( 65535 - g_search.bestangle ) )
        {
          bfound = btrue;
          g_search.besttarget   = charb;
          g_search.bestdistance = local_distance;
          g_search.useangle     = local_angle;
          if ( local_angle  > 32767 )
            g_search.bestangle = UINT16_SIZE - local_angle;
          else
            g_search.bestangle = local_angle;
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
  g_search.besttarget   = MAXCHR;
  g_search.bestdistance = 9999;
  g_search.bestangle    = targetangle;
  done = bfalse;

  prt_search_target_in_block( block_x + 0, block_y + 0, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  if ( VALID_CHR( g_search.besttarget ) ) return g_search.besttarget;

  prt_search_target_in_block( block_x + 1, block_y + 0, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  prt_search_target_in_block( block_x - 1, block_y + 0, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  prt_search_target_in_block( block_x + 0, block_y + 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  prt_search_target_in_block( block_x + 0, block_y - 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  if ( VALID_CHR( g_search.besttarget ) ) return g_search.besttarget;

  btmp = prt_search_target_in_block( block_x + 1, block_y + 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  btmp = prt_search_target_in_block( block_x + 1, block_y - 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  btmp = prt_search_target_in_block( block_x - 1, block_y + 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );
  btmp = prt_search_target_in_block( block_x - 1, block_y - 1, prtx, prty, facing, request_friends, allow_anyone, team, donttarget, oldtarget );

  return g_search.besttarget;
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
      ChrList[numfreechr].bmpdata.cv_tree = NULL;

      // initialize the looping sounds
      ChrList[numfreechr].loopingchannel = INVALID_CHANNEL;
    }
    else if(NULL != ChrList[numfreechr].bmpdata.cv_tree)
    {
      // remove existing collision volume octree
      free( ChrList[numfreechr].bmpdata.cv_tree );
      ChrList[numfreechr].bmpdata.cv_tree = NULL;

      // silence all looping sounds
      if( INVALID_CHANNEL != ChrList[numfreechr].loopingchannel )
      {
        stop_sound( ChrList[numfreechr].loopingchannel );
        ChrList[numfreechr].loopingchannel = INVALID_CHANNEL;
      };
    }

    // reset some values
    ChrList[numfreechr].on = bfalse;
    ChrList[numfreechr].alive = bfalse;
    ChrList[numfreechr].staton = bfalse;
    ChrList[numfreechr].matrixvalid = bfalse;
    ChrList[numfreechr].model = MAXMODEL;
    VData_Blended_Deallocate(&(ChrList[numfreechr].vdata));
    ChrList[numfreechr].name[0] = '\0';

    // invalidate pack
    ChrList[numfreechr].numinpack = 0;
    ChrList[numfreechr].inwhichpack = MAXCHR;
    ChrList[numfreechr].nextinpack = MAXCHR;

    // invalidate attachmants
    ChrList[numfreechr].inwhichslot = SLOT_NONE;
    ChrList[numfreechr].attachedto = MAXCHR;
    for ( cnt = 0; cnt < SLOT_COUNT; cnt++ )
    {
      ChrList[numfreechr].holdingwhich[cnt] = MAXCHR;
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

  if ( !VALID_CHR( ichr ) || 0.0f == ChrList[ichr].bumpstrength ) return 0;

  pos.x = ( ChrList[ichr].bmpdata.cv.x_max + ChrList[ichr].bmpdata.cv.x_min ) * 0.5f;
  pos.y = ( ChrList[ichr].bmpdata.cv.y_max + ChrList[ichr].bmpdata.cv.y_min ) * 0.5f;
  pos.z =   ChrList[ichr].bmpdata.cv.z_min;

  size.x = ( ChrList[ichr].bmpdata.cv.x_max - ChrList[ichr].bmpdata.cv.x_min ) * 0.5f;
  size.y = ( ChrList[ichr].bmpdata.cv.y_max - ChrList[ichr].bmpdata.cv.y_min ) * 0.5f;
  size.z = ( ChrList[ichr].bmpdata.cv.z_max - ChrList[ichr].bmpdata.cv.z_min ) * 0.5f;

  retval = mesh_hitawall( pos, size.x, size.y, ChrList[ichr].stoppedby );

  if( 0!=retval && NULL!=norm )
  {
    vect3 pos2;

    VectorClear( norm->v );

    pos2.x = pos.x + ChrList[ichr].pos.x - ChrList[ichr].pos_old.x;
    pos2.y = pos.y + ChrList[ichr].pos.y - ChrList[ichr].pos_old.y;
    pos2.z = pos.z + ChrList[ichr].pos.z - ChrList[ichr].pos_old.z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, ChrList[ichr].stoppedby ) )
    {
      return 0;
    }

    pos2.x = pos.x;
    pos2.y = pos.y + ChrList[ichr].pos.y - ChrList[ichr].pos_old.y;
    pos2.z = pos.z + ChrList[ichr].pos.z - ChrList[ichr].pos_old.z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, ChrList[ichr].stoppedby ) )
    {
      norm->x = -SGN(ChrList[ichr].pos.x - ChrList[ichr].pos_old.x);
    }

    pos2.x = pos.x + ChrList[ichr].pos.x - ChrList[ichr].pos_old.x;
    pos2.y = pos.y;
    pos2.z = pos.z + ChrList[ichr].pos.z - ChrList[ichr].pos_old.z;

    if( 0 != mesh_hitawall( pos2, size.x, size.y, ChrList[ichr].stoppedby ) )
    {
      norm->y = -SGN(ChrList[ichr].pos.y - ChrList[ichr].pos_old.y);
    }

    pos2.x = pos.x + ChrList[ichr].pos.x - ChrList[ichr].pos_old.x;
    pos2.y = pos.y + ChrList[ichr].pos.y - ChrList[ichr].pos_old.y;
    pos2.z = pos.z;

    if( 0 != mesh_hitawall( pos, size.x, size.y, ChrList[ichr].stoppedby ) )
    {
      norm->z = -SGN(ChrList[ichr].pos.z - ChrList[ichr].pos_old.z);
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
  enchant = ChrList[character].firstenchant;
  while ( enchant < MAXENCHANT )
  {
    remove_enchant_value( enchant, ADDACCEL );
    enchant = EncList[enchant].nextenchant;
  }

  // Set the starting value
  assert( MAXMODEL != VALIDATE_MDL( ChrList[character].model ) );
  ChrList[character].maxaccel = CapList[ChrList[character].model].maxaccel[( ChrList[character].texture - MadList[ChrList[character].model].skinstart ) % MAXSKIN];

  // Put the acceleration enchants back on
  enchant = ChrList[character].firstenchant;
  while ( enchant < MAXENCHANT )
  {
    add_enchant_value( enchant, ADDACCEL, EncList[enchant].eve );
    enchant = EncList[enchant].nextenchant;
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
  if ( !ignorekurse && ChrList[ichr].iskursed && ChrList[imount].alive )
  {
    ChrList[ichr].alert |= ALERT_NOTDROPPED;
    return bfalse;
  }


  // Rip 'em apart
  _slot = ChrList[ichr].inwhichslot;
  if(_slot == SLOT_INVENTORY)
  {
    ChrList[ichr].attachedto = MAXCHR;
    ChrList[ichr].inwhichslot = SLOT_NONE;
  }
  else
  {
    assert(_slot != SLOT_NONE);
    assert(ichr == ChrList[imount].holdingwhich[_slot]);
    ChrList[ichr].attachedto = MAXCHR;
    ChrList[ichr].inwhichslot = SLOT_NONE;
    ChrList[imount].holdingwhich[_slot] = MAXCHR;
  }


  ChrList[ichr].scale = ChrList[ichr].fat; // * MadList[ChrList[ichr].model].scale * 4;


  // Run the falling animation...
  play_action( ichr, ACTION_JB + ( ChrList[ichr].inwhichslot % 2 ), bfalse );

  // Set the positions
  if ( ChrList[ichr].matrixvalid )
  {
    ChrList[ichr].pos.x = ChrList[ichr].matrix _CNV( 3, 0 );
    ChrList[ichr].pos.y = ChrList[ichr].matrix _CNV( 3, 1 );
    ChrList[ichr].pos.z = ChrList[ichr].matrix _CNV( 3, 2 );
  }
  else
  {
    ChrList[ichr].pos.x = ChrList[imount].pos.x;
    ChrList[ichr].pos.y = ChrList[imount].pos.y;
    ChrList[ichr].pos.z = ChrList[imount].pos.z;
  }



  // Make sure it's not dropped in a wall...
  if ( 0 != __chrhitawall( ichr, NULL ) )
  {
    ChrList[ichr].pos.x = ChrList[imount].pos.x;
    ChrList[ichr].pos.y = ChrList[imount].pos.y;
  }


  // Check for shop passages
  inshop = bfalse;
  if ( ChrList[ichr].isitem && numshoppassage != 0 && doshop )
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
      Uint16 model = ChrList[ichr].model;

      assert( MAXMODEL != VALIDATE_MDL( model ) );

      // Give the imount its money back, alert the shop iowner
      price = CapList[model].skincost[( ChrList[ichr].texture - MadList[model].skinstart ) % MAXSKIN];
      if ( CapList[model].isstackable )
      {
        price *= ChrList[ichr].ammo;
      }
      ChrList[imount].money += price;
      ChrList[iowner].money -= price;
      if ( ChrList[iowner].money < 0 )  ChrList[iowner].money = 0;
      if ( ChrList[imount].money > MAXMONEY )  ChrList[imount].money = MAXMONEY;

      ChrList[iowner].alert |= ALERT_SIGNALED;
      ChrList[iowner].message = price;  // Tell iowner how much...
      ChrList[iowner].messagedata = 0;  // 0 for buying an item
    }
  }

  // Make sure it works right
  ChrList[ichr].hitready = btrue;
  ChrList[ichr].alert   |= ALERT_DROPPED;
  if ( inshop )
  {
    // Drop straight down to avoid theft
    ChrList[ichr].vel.x = 0;
    ChrList[ichr].vel.y = 0;
  }
  else
  {
    Uint16 sin_dir = RANDIE;
    ChrList[ichr].accum_vel.x += ChrList[imount].vel.x + 0.5 * DROPXYVEL * turntosin[(( sin_dir>>2 ) + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];
    ChrList[ichr].accum_vel.y += ChrList[imount].vel.y + 0.5 * DROPXYVEL * turntosin[sin_dir>>2];
  }
  ChrList[ichr].accum_vel.z += DROPZVEL;


  // Turn looping off
  ChrList[ichr].action.loop = bfalse;


  // Reset the team if it is a imount
  if ( ChrList[imount].ismount )
  {
    ChrList[imount].team = ChrList[imount].baseteam;
    ChrList[imount].alert |= ALERT_DROPPED;
  }
  ChrList[ichr].team = ChrList[ichr].baseteam;
  ChrList[ichr].alert |= ALERT_DROPPED;


  // Reset transparency
  if ( ChrList[ichr].isitem && ChrList[imount].transferblend )
  {
    Uint16 model = ChrList[ichr].model;

    assert( MAXMODEL != VALIDATE_MDL( model ) );

    // Okay, reset transparency
    enchant = ChrList[ichr].firstenchant;
    while ( enchant < MAXENCHANT )
    {
      unset_enchant_value( enchant, SETALPHABLEND );
      unset_enchant_value( enchant, SETLIGHTBLEND );
      enchant = EncList[enchant].nextenchant;
    }

    ChrList[ichr].alpha_fp8 = CapList[model].alpha_fp8;
    ChrList[ichr].bumpstrength = CapList[model].bumpstrength * FP8_TO_FLOAT( ChrList[ichr].alpha_fp8 );
    ChrList[ichr].light_fp8 = CapList[model].light_fp8;
    enchant = ChrList[ichr].firstenchant;
    while ( enchant < MAXENCHANT )
    {
      set_enchant_value( enchant, SETALPHABLEND, EncList[enchant].eve );
      set_enchant_value( enchant, SETLIGHTBLEND, EncList[enchant].eve );
      enchant = EncList[enchant].nextenchant;
    }
  }

  // Set twist
  ChrList[ichr].mapturn_lr = 32768;
  ChrList[ichr].mapturn_ud = 32768;

  if ( ChrList[ichr].isplayer )
    debug_message( 1, "dismounted %s(%s) from (%s)", ChrList[ichr].name, CapList[ChrList[ichr].model].classname, CapList[ChrList[imount].model].classname );


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
  ChrList[ichr].hitready = bfalse;

  //make sure you're not trying to mount yourself!
  if ( ichr == imount )
    return bfalse;

  // make sure that neither is in someone's pack
  if ( chr_in_pack( ichr ) || chr_in_pack( imount ) )
    return bfalse;

  // Make sure the the slot is valid
  assert( MAXMODEL != VALIDATE_MDL( ChrList[imount].model ) );
  if ( SLOT_NONE == slot || !CapList[ChrList[imount].model].slotvalid[slot] )
    return bfalse;

  // Put 'em together
  assert(slot != SLOT_NONE);
  ChrList[ichr].inwhichslot = slot;
  ChrList[ichr].attachedto  = imount;
  ChrList[imount].holdingwhich[slot] = ichr;

  // handle the vertices
  {
    Uint16 model = ChrList[imount].model;
    Uint16 vrtoffset = slot_to_offset( slot );

    assert( MAXMODEL != VALIDATE_MDL( model ) );
    if ( MadList[model].vertices > vrtoffset && vrtoffset > 0 )
    {
      tnc = MadList[model].vertices - vrtoffset;
      ChrList[ichr].attachedgrip[0] = tnc;
      ChrList[ichr].attachedgrip[1] = tnc + 1;
      ChrList[ichr].attachedgrip[2] = tnc + 2;
      ChrList[ichr].attachedgrip[3] = tnc + 3;
    }
    else
    {
      ChrList[ichr].attachedgrip[0] = MadList[model].vertices - 1;
      ChrList[ichr].attachedgrip[1] = 0xFFFF;
      ChrList[ichr].attachedgrip[2] = 0xFFFF;
      ChrList[ichr].attachedgrip[3] = 0xFFFF;
    }
  }

  ChrList[ichr].jumptime = DELAY_JUMP * 4;


  // Run the held animation
  if ( ChrList[imount].bmpdata.calc_is_mount && slot == SLOT_SADDLE )
  {
    // Riding imount
    play_action( ichr, ACTION_MI, btrue );
    ChrList[ichr].action.loop = btrue;
  }
  else
  {
    play_action( ichr, ACTION_MM + slot, bfalse );
    if ( ChrList[ichr].isitem )
    {
      // Item grab
      ChrList[ichr].action.keep = btrue;
    }
  }

  // Set the team
  if ( ChrList[ichr].isitem )
  {
    ChrList[ichr].team = ChrList[imount].team;
    // Set the alert
    ChrList[ichr].alert |= ALERT_GRABBED;
  }
  else if ( ChrList[imount].bmpdata.calc_is_mount )
  {
    ChrList[imount].team = ChrList[ichr].team;
    // Set the alert
    if ( !ChrList[imount].isitem )
    {
      ChrList[imount].alert |= ALERT_GRABBED;
    }
  }

  // It's not gonna hit the floor
  ChrList[ichr].hitready = bfalse;

  if ( ChrList[ichr].isplayer )
    debug_message( 1, "mounted %s(%s) to (%s)", ChrList[ichr].name, CapList[ChrList[ichr].model].classname, CapList[ChrList[imount].model].classname );


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

  Uint16 item_mdl = ChrList[item].model;

  assert( MAXMODEL != VALIDATE_MDL( item_mdl ) );


  if ( CapList[item_mdl].isstackable )
  {
    Uint16 inpack_mdl;

    inpack = chr_get_nextinpack( character );
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

        if ( ChrList[inpack].ammomax != ChrList[item].ammomax )
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
static bool_t pack_push_front( Uint16 iitem, CHR_REF ichr )
{
  // make sure the item and character are valid
  if ( !VALID_CHR( iitem ) || !VALID_CHR( ichr ) ) return bfalse;

  // make sure the item is free to add
  if ( chr_attached( iitem ) || chr_in_pack( iitem ) ) return bfalse;

  // we cannot do packs within packs, so
  if ( chr_in_pack( ichr ) ) return bfalse;

  // make sure there is space for the item
  if ( ChrList[ichr].numinpack >= MAXNUMINPACK ) return bfalse;

  // insert at the front of the list
  ChrList[iitem].nextinpack  = chr_get_nextinpack( ichr );
  ChrList[ichr].nextinpack = iitem;
  ChrList[iitem].inwhichpack = ichr;
  ChrList[ichr].numinpack++;

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
  ChrList[ichr].numinpack--;

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
bool_t add_item_to_character_pack( Uint16 iitem, CHR_REF ichr )
{
  // ZZ> This function puts one ichr inside the other's pack
  Uint16 newammo, istack;

  // Make sure both objects exist
  if ( !VALID_CHR( ichr ) || !VALID_CHR( iitem ) ) return bfalse;

  // Make sure neither object is in a pack
  if ( chr_in_pack( ichr ) || chr_in_pack( iitem ) ) return bfalse;

  // make sure we the character IS NOT an item and the item IS an item
  if ( ChrList[ichr].isitem || !ChrList[iitem].isitem ) return bfalse;

  // make sure the item does not have an inventory of its own
  if ( chr_has_inventory( iitem ) ) return bfalse;

  istack = stack_in_pack( iitem, ichr );
  if ( VALID_CHR( istack ) )
  {
    // put out torches, etc.
    disaffirm_attached_particles( iitem );

    // We found a similar, stackable iitem in the pack
    if ( ChrList[iitem].nameknown || ChrList[istack].nameknown )
    {
      ChrList[iitem].nameknown = btrue;
      ChrList[istack].nameknown = btrue;
    }
    if ( CapList[ChrList[iitem].model].usageknown || CapList[ChrList[istack].model].usageknown )
    {
      CapList[ChrList[iitem].model].usageknown = btrue;
      CapList[ChrList[istack].model].usageknown = btrue;
    }
    newammo = ChrList[iitem].ammo + ChrList[istack].ammo;
    if ( newammo <= ChrList[istack].ammomax )
    {
      // All transfered, so kill the in hand iitem
      ChrList[istack].ammo = newammo;
      detach_character_from_mount( iitem, btrue, bfalse );
      ChrList[iitem].freeme = btrue;
    }
    else
    {
      // Only some were transfered,
      ChrList[iitem].ammo += ChrList[istack].ammo - ChrList[istack].ammomax;
      ChrList[istack].ammo = ChrList[istack].ammomax;
      ChrList[ichr].alert |= ALERT_TOOMUCHBAGGAGE;
    }
  }
  else
  {
    // Make sure we have room for another iitem
    if ( ChrList[ichr].numinpack >= MAXNUMINPACK )
    {
      ChrList[ichr].alert |= ALERT_TOOMUCHBAGGAGE;
      return bfalse;
    }

    // Take the item out of hand
    if ( detach_character_from_mount( iitem, btrue, bfalse ) )
    {
      ChrList[iitem].alert &= ~ALERT_DROPPED;
    }

    if ( pack_push_front( iitem, ichr ) )
    {
      // put out torches, etc.
      disaffirm_attached_particles( iitem );
      ChrList[iitem].alert |= ALERT_ATLASTWAYPOINT;

      // Remove the iitem from play
      ChrList[iitem].hitready    = bfalse;
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
  if ( ChrList[item].iskursed && ChrList[item].isequipped && !ignorekurse )
  {
    // Flag the last item as not removed
    ChrList[item].alert |= ALERT_NOTPUTAWAY;  // Doubles as IfNotTakenOut

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
    ChrList[item].alert &= ( ~ALERT_GRABBED );
    ChrList[item].alert |= ALERT_TAKENOUT;
    //ChrList[item].team   = ChrList[character].team;
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


  if ( ChrList[character].pos.z > -2 ) // Don't lose keys in pits...
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
        if ( CAP_INHERIT_IDSZ_RANGE( ChrList[item].model, testa, testz ) )
        {
          // We found a key...
          ChrList[item].inwhichpack = MAXCHR;
          ChrList[item].isequipped = bfalse;

          ChrList[lastitem].nextinpack = nextitem;
          ChrList[item].nextinpack = MAXCHR;
          ChrList[character].numinpack--;

          ChrList[item].hitready = btrue;
          ChrList[item].alert |= ALERT_DROPPED;

          direction = RANDIE;
          ChrList[item].turn_lr = direction + 32768;
          cosdir = direction + 16384;
          ChrList[item].level = ChrList[character].level;
          ChrList[item].pos.x = ChrList[character].pos.x;
          ChrList[item].pos.y = ChrList[character].pos.y;
          ChrList[item].pos.z = ChrList[character].pos.z;
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
void drop_all_items( CHR_REF character )
{
  // ZZ> This function drops all of a character's items
  Uint16 item, direction, cosdir, diradd;


  if ( !VALID_CHR( character ) ) return;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    detach_character_from_mount( chr_get_holdingwhich( character, _slot ), !ChrList[character].alive, bfalse );
  };

  if ( chr_has_inventory( character ) )
  {
    direction = ChrList[character].turn_lr + 32768;
    diradd = (float)UINT16_SIZE / ChrList[character].numinpack;
    while ( ChrList[character].numinpack > 0 )
    {
      item = get_item_from_character_pack( character, SLOT_NONE, !ChrList[character].alive );
      if ( detach_character_from_mount( item, btrue, btrue ) )
      {
        ChrList[item].hitready = btrue;
        ChrList[item].alert |= ALERT_DROPPED;
        ChrList[item].pos.x = ChrList[character].pos.x;
        ChrList[item].pos.y = ChrList[character].pos.y;
        ChrList[item].pos.z = ChrList[character].pos.z;
        ChrList[item].level = ChrList[character].level;
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

  model = ChrList[ichr].model;
  if ( !VALID_MDL( model ) ) return bfalse;

  // Make sure the character doesn't have something already, and that it has hands

  if ( chr_using_slot( ichr, slot ) || !CapList[model].slotvalid[slot] )
    return bfalse;

  // Make life easier
  grip  = slot_to_grip( slot );

  // !!!!base the grab distance off of the character size!!!!
  grab_width  = ( ChrList[ichr].bmpdata.calc_size_big + ChrList[ichr].bmpdata.calc_size ) / 2.0f * 1.5f;
  grab_height = ChrList[ichr].bmpdata.calc_height / 2.0f * 1.5f;

  // Do we have a matrix???
  if ( ChrList[ichr].matrixvalid )
  {
    // Transform the weapon grip from model to world space
    vertex = ChrList[ichr].attachedgrip[0];

    if(0xFFFF == vertex)
    {
      point.x = ChrList[ichr].pos.x;
      point.y = ChrList[ichr].pos.y;
      point.z = ChrList[ichr].pos.z;
      point.w = 1.0f;
    }
    else
    {
      point.x = ChrList[ichr].vdata.Vertices[vertex].x;
      point.y = ChrList[ichr].vdata.Vertices[vertex].y;
      point.z = ChrList[ichr].vdata.Vertices[vertex].z;
      point.w = 1.0f;
    }

    // Do the transform
    Transform4_Full( &(ChrList[ichr].matrix), &posa, &point, 1 );
  }
  else
  {
    // Just wing it
    posa.x = ChrList[ichr].pos.x;
    posa.y = ChrList[ichr].pos.y;
    posa.z = ChrList[ichr].pos.z;
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
    if ( !ChrList[ichr].canseeinvisible && chr_is_invisible( iobject ) ) continue;

    // if we can't pickpocket, don't mess with inventory items
    if ( !can_pickpocket && VALID_CHR( ipacker ) ) continue;

    // if we can't disarm, don't mess with held items
    if ( !can_disarm && VALID_CHR( iholder ) ) continue;

    // if we can't grab people, don't mess with them
    if ( !people && !ChrList[iobject].isitem ) continue;

    // get the target object position
    if ( !VALID_CHR( ipacker ) && !VALID_CHR(iholder) )
    {
      trg_strength_fp8     = ChrList[iobject].strength_fp8;
      trg_intelligence_fp8 = ChrList[iobject].intelligence_fp8;
      trg_team             = ChrList[iobject].team;

      posb = ChrList[iobject].pos;
    }
    else if ( VALID_CHR(iholder) )
    {
      trg_chr              = iholder;
      trg_strength_fp8     = ChrList[iholder].strength_fp8;
      trg_intelligence_fp8 = ChrList[iholder].intelligence_fp8;

      trg_team = ChrList[iholder].team;
      posb     = ChrList[iobject].pos;
    }
    else // must be in a pack
    {
      trg_chr              = ipacker;
      trg_strength_fp8     = ChrList[ipacker].strength_fp8;
      trg_intelligence_fp8 = ChrList[ipacker].intelligence_fp8;
      trg_team = ChrList[ipacker].team;
      posb     = ChrList[ipacker].pos;
      posb.z  += ChrList[ipacker].bmpdata.calc_height / 2;
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
      ballowed = ChrList[ichr].dexterity_fp8 >= trg_intelligence_fp8 && TeamList[ChrList[ichr].team].hatesteam[trg_team];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        ChrList[iholder].alert |= ALERT_ATTACKED;
        ChrList[iholder].aibumplast = ichr;
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
      ballowed = !ChrList[iobject].iskursed && ChrList[ichr].strength_fp8 > trg_strength_fp8 && TeamList[ChrList[ichr].team].hatesteam[trg_team];

      if ( !ballowed )
      {
        // if we fail, we get attacked
        ChrList[iholder].alert |= ALERT_ATTACKED;
        ChrList[iholder].aibumplast = ichr;
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
  if ( mesh_check( ChrList[minchr].pos.x, ChrList[minchr].pos.y ) )
  {
    if ( numshoppassage == 0 )
    {
      ballowed = btrue;
    }
    else if ( ChrList[minchr].isitem )
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
    if ( ChrList[ichr].isitem )
    {
      ballowed = btrue; // As in NetHack, Pets can shop for free =]
    }
    else
    {
      // Pay the shop owner, or don't allow grab...
      ChrList[owner].alert |= ALERT_SIGNALED;
      price = CapList[ChrList[minchr].model].skincost[( ChrList[minchr].texture - MadList[ChrList[minchr].model].skinstart ) % MAXSKIN];
      if ( CapList[ChrList[minchr].model].isstackable )
      {
        price *= ChrList[minchr].ammo;
      }
      ChrList[owner].message = price;  // Tell owner how much...
      if ( ChrList[ichr].money >= price )
      {
        // Okay to buy
        ChrList[ichr].money  -= price;  // Skin 0 cost is price
        ChrList[owner].money += price;
        if ( ChrList[owner].money > MAXMONEY )  ChrList[owner].money = MAXMONEY;

        ballowed = btrue;
        ChrList[owner].messagedata = 1;  // 1 for selling an item
      }
      else
      {
        // Don't allow purchase
        ChrList[owner].messagedata = 2;  // 2 for "you can't afford that"
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
    ChrList[minchr].accum_vel.z += DROPZVEL;
    ChrList[minchr].hitready = btrue;
    ChrList[minchr].alert |= ALERT_DROPPED;
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
  action = ChrList[ichr].action.now;
  // See if it's an unarmed attack...
  if ( !VALID_CHR( weapon ) )
  {
    weapon = ichr;
    spawngrip = slot_to_grip( slot );
  }


  if ( weapon != ichr && (( CapList[ChrList[weapon].model].isstackable && ChrList[weapon].ammo > 1 ) || ( action >= ACTION_FA && action <= ACTION_FD ) ) )
  {
    // Throw the weapon if it's stacked or a hurl animation
    pos.x = ChrList[ichr].pos.x;
    pos.y = ChrList[ichr].pos.y;
    pos.z = ChrList[ichr].pos.z;
    thrown = spawn_one_character( ChrList[ichr].pos, ChrList[weapon].model, ChrList[ichr].team, 0, ChrList[ichr].turn_lr, ChrList[weapon].name, MAXCHR );
    if ( VALID_CHR( thrown ) )
    {
      ChrList[thrown].iskursed = bfalse;
      ChrList[thrown].ammo = 1;
      ChrList[thrown].alert |= ALERT_THROWN;

      velocity = 0.0f;
      if ( ChrList[ichr].weight >= 0.0f )
      {
        velocity = ChrList[ichr].strength_fp8 / ( ChrList[thrown].weight * THROWFIX );
      };

      velocity += MINTHROWVELOCITY;
      if ( velocity > MAXTHROWVELOCITY )
      {
        velocity = MAXTHROWVELOCITY;
      }
      tTmp = ( 0x7FFF + ChrList[ichr].turn_lr ) >> 2;
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
        particle = spawn_one_particle( 1.0f, ChrList[weapon].pos, ChrList[ichr].turn_lr, ChrList[weapon].model, CapList[ChrList[weapon].model].attackprttype, weapon, spawngrip, ChrList[ichr].team, ichr, 0, MAXCHR );
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
                PrtList[particle].pos.x = ChrList[ichr].pos.x;
                PrtList[particle].pos.y = ChrList[ichr].pos.y;
              }
            }
          }
          else
          {
            // Attached particles get a strength bonus for reeling...
            if ( PipList[PrtList[particle].pip].causeknockback ) dampen = ( REELBASE + ( ChrList[ichr].strength_fp8 / REEL ) ) * 4; //Extra knockback?
            else dampen = REELBASE + ( ChrList[ichr].strength_fp8 / REEL );      // No, do normal

            PrtList[particle].accum_vel.x += -(1.0f - dampen) * PrtList[particle].vel.x;
            PrtList[particle].accum_vel.y += -(1.0f - dampen) * PrtList[particle].vel.y;
            PrtList[particle].accum_vel.z += -(1.0f - dampen) * PrtList[particle].vel.z;
          }

          // Initial particles get a strength bonus, which may be 0.00
          PrtList[particle].damage.ibase += ( ChrList[ichr].strength_fp8 * CapList[ChrList[weapon].model].strengthdampen );

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
  CHR_REF ichr;

  // poof all characters that have pending poof requests
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) || !ChrList[ichr].gopoof ) continue;

    // detach from any imount
    detach_character_from_mount( ichr, btrue, bfalse );

    // Drop all possesions
    for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
    {
      if ( chr_using_slot( ichr, _slot ) )
        detach_character_from_mount( chr_get_holdingwhich( ichr, _slot ), btrue, bfalse );
    };

    free_inventory( ichr );
    ChrList[ichr].freeme = btrue;
  };

  // free all characters that requested destruction last round
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) || !ChrList[ichr].freeme ) continue;
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
  bool_t   ready, allowedtoattack, watchtarget, grounded, dojumptimer;
  TURNMODE loc_turnmode;
  float ftmp, level;

  float horiz_friction, vert_friction;
  float loc_slippyfriction, loc_airfriction, loc_waterfriction, loc_noslipfriction;
  float loc_flydampen, loc_traction;
  float turnfactor;
  vect3 nrm = {0.0f, 0.0f, 0.0f};
  CHR * pchr;
  MAD * pmad;


  loc_airfriction    = airfriction;
  loc_waterfriction  = waterfriction;
  loc_slippyfriction = slippyfriction;
  loc_noslipfriction = noslipfriction;
  loc_flydampen      = pow( FLYDAMPEN     , dUpdate );

  // Move every character
  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) ) continue;

    pchr = ChrList + ichr;

    // Character's old location
    pchr->turn_lr_old = pchr->turn_lr;

    if ( chr_in_pack( ichr ) ) continue;

    // get the model
    imdl = VALIDATE_MDL( pchr->model );
    assert( MAXMODEL != imdl );
    pmad = MadList + pchr->model;

    // get the imount
    imount = chr_get_attachedto(ichr);

    // get the level
    level = pchr->level;

    // TURNMODE_VELOCITY acts strange when someone is mounted on a "bucking" imount, like the gelfeet
    loc_turnmode = pchr->turnmode;
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
        dvx = pchr->latch.x;
        dvy = pchr->latch.y;

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
        //maxvel = 1.5f * MAX(MAX(3,pchr->runspd), MAX(pchr->walkspd,pchr->sneakspd));
        pchr->trgvel.x = dvx * maxvel;
        pchr->trgvel.y = dvy * maxvel;
        pchr->trgvel.z = 0;

        if ( pchr->maxaccel > 0.0f )
        {
          dvx = ( pchr->trgvel.x - pchr->vel.x );
          dvy = ( pchr->trgvel.y - pchr->vel.y );

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
          CHR_REF ai_target = chr_get_aitarget( ichr );
          if ( VALID_CHR( ai_target ) && ichr != ai_target )
          {
            pchr->turn_lr = terp_dir( pchr->turn_lr, ChrList[ai_target].pos.x - pchr->pos.x, ChrList[ai_target].pos.y - pchr->pos.y, dUpdate * turnfactor );
          };
        }

        // Get direction from ACTUAL change in velocity
        if ( loc_turnmode == TURNMODE_VELOCITY )
        {
          if ( pchr->isplayer )
            pchr->turn_lr = terp_dir( pchr->turn_lr, pchr->trgvel.x, pchr->trgvel.y, dUpdate * turnfactor );
          else
            pchr->turn_lr = terp_dir( pchr->turn_lr, pchr->trgvel.x, pchr->trgvel.y, dUpdate * turnfactor / 4.0f );
        }

        // Otherwise make it spin
        else if ( loc_turnmode == TURNMODE_SPIN )
        {
          pchr->turn_lr += SPINRATE * dUpdate * turnfactor;
        }
      };

      // Character latches for generalized buttons
      if ( LATCHBUTTON_NONE != pchr->latch.b )
      {
        if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_JUMP ) && pchr->jumptime == 0.0f )
        {
          if ( detach_character_from_mount( ichr, btrue, btrue ) )
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
              if ( pchr->action.ready )    play_action( ichr, ACTION_JA, btrue );

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

        if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_ALTLEFT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_GRAB;
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

        if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_ALTRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_GRAB;
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

        if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_PACKLEFT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_PACK;
          item = chr_get_holdingwhich( ichr, SLOT_LEFT );
          if ( VALID_CHR( item ) )
          {
            if (( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
            {
              // The item couldn't be put away
              ChrList[item].alert |= ALERT_NOTPUTAWAY;
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

        if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_PACKRIGHT ) && pchr->action.ready && pchr->reloadtime == 0 )
        {
          pchr->reloadtime = DELAY_PACK;
          item = chr_get_holdingwhich( ichr, SLOT_RIGHT );
          if ( VALID_CHR( item ) )
          {
            if (( ChrList[item].iskursed || CapList[ChrList[item].model].istoobig ) && !CapList[ChrList[item].model].isequipment )
            {
              // The item couldn't be put away
              ChrList[item].alert |= ALERT_NOTPUTAWAY;
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

        if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_LEFT ) && pchr->reloadtime == 0 )
        {
          // Which weapon?
          weapon = chr_get_holdingwhich( ichr, SLOT_LEFT );
          if ( !VALID_CHR( weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = ichr;
          }
          action = CapList[ChrList[weapon].model].weaponaction;


          // Can it do it?
          allowedtoattack = btrue;
          if ( !pmad->actionvalid[action] || ChrList[weapon].reloadtime > 0 ||
               ( CapList[ChrList[weapon].model].needskillidtouse && !check_skills( ichr, CapList[ChrList[weapon].model].idsz[IDSZ_SKILL] ) ) )
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
              ChrList[weapon].alert |= ALERT_USED;
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
                  ChrList[imount].alert |= ALERT_USED;
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
                  cost_mana( ichr, ChrList[weapon].manacost, weapon );
                  // Check life healing
                  pchr->life_fp8 += ChrList[weapon].lifeheal;
                  if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;
                  ready = ( action == ACTION_PA );
                  action += rand() & 1;
                  play_action( ichr, action, ready );
                  if ( weapon != ichr )
                  {
                    // Make the weapon attack too
                    play_action( weapon, ACTION_MJ, bfalse );
                    ChrList[weapon].alert |= ALERT_USED;
                  }
                  else
                  {
                    // Flag for unarmed attack
                    pchr->alert |= ALERT_USED;
                  }
                }
              }
            }
          }
        }
        else if ( HAS_SOME_BITS( pchr->latch.b, LATCHBUTTON_RIGHT ) && pchr->reloadtime == 0 )
        {
          // Which weapon?
          weapon = chr_get_holdingwhich( ichr, SLOT_RIGHT );
          if ( !VALID_CHR( weapon ) )
          {
            // Unarmed means character itself is the weapon
            weapon = ichr;
          }
          action = CapList[ChrList[weapon].model].weaponaction + 2;


          // Can it do it?
          allowedtoattack = btrue;
          if ( !pmad->actionvalid[action] || ChrList[weapon].reloadtime > 0 ||
               ( CapList[ChrList[weapon].model].needskillidtouse && !check_skills( ichr, CapList[ChrList[weapon].model].idsz[IDSZ_SKILL] ) ) )
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
              ChrList[weapon].alert |= ALERT_USED;
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
                  ChrList[imount].alert |= ALERT_USED;
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
                  cost_mana( ichr, ChrList[weapon].manacost, weapon );
                  // Check life healing
                  pchr->life_fp8 += ChrList[weapon].lifeheal;
                  if ( pchr->life_fp8 > pchr->lifemax_fp8 )  pchr->life_fp8 = pchr->lifemax_fp8;
                  ready = ( action == ACTION_PC );
                  action += rand() & 1;
                  play_action( ichr, action, ready );
                  if ( weapon != ichr )
                  {
                    // Make the weapon attack too
                    play_action( weapon, ACTION_MJ, bfalse );
                    ChrList[weapon].alert |= ALERT_USED;
                  }
                  else
                  {
                    // Flag for unarmed attack
                    pchr->alert |= ALERT_USED;
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
          character_swipe( ichr, SLOT_LEFT );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_ACTRIGHT ) )
          character_swipe( ichr, SLOT_RIGHT );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_GRABLEFT ) )
          character_grab_stuff( ichr, SLOT_LEFT, bfalse );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_GRABRIGHT ) )
          character_grab_stuff( ichr, SLOT_RIGHT, bfalse );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_CHARLEFT ) )
          character_grab_stuff( ichr, SLOT_LEFT, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_CHARRIGHT ) )
          character_grab_stuff( ichr, SLOT_RIGHT, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_DROPLEFT ) )
          detach_character_from_mount( chr_get_holdingwhich( ichr, SLOT_LEFT ), bfalse, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_DROPRIGHT ) )
          detach_character_from_mount( chr_get_holdingwhich( ichr, SLOT_RIGHT ), bfalse, btrue );
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_POOF ) && !pchr->isplayer )
          pchr->gopoof = btrue;
        if ( HAS_SOME_BITS( pmad->framefx[pchr->anim.next], MADFX_FOOTFALL ) )
        {
          if ( INVALID_SOUND != CapList[imdl].footfallsound )
          {
            float volume = ( ABS( pchr->vel.x ) +  ABS( pchr->vel.y ) ) / CapList[imdl].sneakspd;
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
        if ( speed < pchr->sneakspd )
        {
          //                        pchr->action.next = ACTION_DA;
          // Do boredom
          pchr->boretime -= dUpdate;
          if ( pchr->boretime <= 0 ) pchr->boretime = 0;

          if ( pchr->boretime <= 0 )
          {
            pchr->alert |= ALERT_BORED;
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
          if ( speed < pchr->walkspd )
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
            if ( speed < pchr->runspd )
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
      ChrList[lastcharacter].aicontent = content;
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
            ChrList[lastcharacter].alert |= ALERT_GRABBED;                       // Make spellbooks change

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
float get_one_level( CHR_REF character )
{
  float level;
  Uint16 platform;

  // return a default for invalid characters
  if ( !VALID_CHR( character ) ) return bfalse;

  // return the cached value for pre-calculated characters
  if ( ChrList[character].levelvalid ) return ChrList[character].level;

  //get the base level
  ChrList[character].onwhichfan = mesh_get_fan( ChrList[character].pos );
  level = mesh_get_level( ChrList[character].onwhichfan, ChrList[character].pos.x, ChrList[character].pos.y, ChrList[character].waterwalk );

  // if there is a platform, choose whichever is higher
  platform = chr_get_onwhichplatform( character );
  if ( VALID_CHR( platform ) )
  {
    float ftmp = ChrList[platform].bmpdata.cv.z_max;
    level = MAX( level, ftmp );
  }

  ChrList[character].level      = level;
  ChrList[character].levelvalid = btrue;

  return ChrList[character].level;
};

//--------------------------------------------------------------------------------------------
void get_all_levels( void )
{
  CHR_REF character;

  // Initialize all the objects
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) ) continue;

    ChrList[character].onwhichfan = INVALID_FAN;
    ChrList[character].levelvalid = bfalse;
  };

  // do the levels
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( character ) || ChrList[character].levelvalid ) continue;
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
    if ( INVALID_FAN != ChrList[character].onwhichfan && mesh_has_some_bits( ChrList[character].onwhichfan, MESHFX_WATER ) )
    {
      splashstrength = ChrList[character].bmpdata.calc_size_big / 45.0f * ChrList[character].bmpdata.calc_size / 30.0f;
      if ( ChrList[character].vel.z > 0.0f ) splashstrength *= 0.5;
      splashstrength *= ABS( ChrList[character].vel.z ) / 10.0f;
      splashstrength *= ChrList[character].bumpstrength;
      if ( ChrList[character].pos.z < GWater.surfacelevel )
      {
        is_inwater = btrue;
      }

      ripplesize = ( ChrList[character].bmpdata.calc_size + ChrList[character].bmpdata.calc_size_big ) * 0.5f;
      if ( ChrList[character].bmpdata.cv.z_max < GWater.surfacelevel )
      {
        is_underwater = btrue;
      }

      // scale the ripple strength
      ripplestrength = - ( ChrList[character].bmpdata.cv.z_min - GWater.surfacelevel ) * ( ChrList[character].bmpdata.cv.z_max - GWater.surfacelevel );
      ripplestrength /= 0.75f * ChrList[character].bmpdata.calc_height * ChrList[character].bmpdata.calc_height;
      ripplestrength *= ripplesize / 37.5f * ChrList[character].bumpstrength;
      ripplestrength = MAX( 0.0f, ripplestrength );
    };

    // splash stuff
    if ( ChrList[character].inwater != is_inwater && splashstrength > 0.1f )
    {
      vect3 prt_pos = {ChrList[character].pos.x, ChrList[character].pos.y, GWater.surfacelevel + RAISE};
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


      ChrList[character].inwater = is_inwater;
      if ( GWater.iswater && is_inwater )
      {
        ChrList[character].alert |= ALERT_INWATER;
      }
    }
    else if ( is_inwater && ripplestrength > 0.0f )
    {
      // Ripples
      ripand = ((( int ) ChrList[character].vel.x ) != 0 ) | ((( int ) ChrList[character].vel.y ) != 0 );
      ripand = RIPPLEAND >> ripand;
      if ( 0 == ( wldframe&ripand ) )
      {
        vect3  prt_pos = {ChrList[character].pos.x, ChrList[character].pos.y, GWater.surfacelevel};
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
    if ( mesh_has_some_bits( ChrList[character].onwhichfan, MESHFX_DAMAGE ) && ChrList[character].pos.z <= GWater.surfacelevel + DAMAGERAISE )
    {
      Uint8 loc_damagemodifier;
      CHR_REF imount;

      // augment the rider's damage immunity with the mount's
      loc_damagemodifier = ChrList[character].damagemodifier_fp8[GTile_Dam.type];
      imount = chr_get_attachedto(character);
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

      if ( !HAS_ALL_BITS(loc_damagemodifier, DAMAGE_SHIFT ) && !ChrList[character].invictus )  // DAMAGE_SHIFT means they're pretty well immune
      {
        if ( ChrList[character].damagetime == 0 )
        {
          PAIR ptemp = {GTile_Dam.amount, 1};
          damage_character( character, 32768, &ptemp, GTile_Dam.type, TEAM_DAMAGE, chr_get_aibumplast( character ), DAMFX_BLOC | DAMFX_ARMO );
          ChrList[character].damagetime = DELAY_DAMAGETILE;
        }

        if ( GTile_Dam.parttype != MAXPRTPIP && ( wldframe&GTile_Dam.partand ) == 0 )
        {
          spawn_one_particle( 1.0f, ChrList[character].pos,
                              0, MAXMODEL, GTile_Dam.parttype, MAXCHR, GRIP_LAST, TEAM_NULL, MAXCHR, 0, MAXCHR );
        }

      }

      if ( ChrList[character].reaffirmdamagetype == GTile_Dam.type )
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

  if ( ChrList[object].weight > 0.0f )
    ChrList[platform].weight -= ChrList[object].weight;

  ChrList[object].onwhichplatform = MAXCHR;
  ChrList[object].level           = ChrList[platform].level;

  if ( ChrList[object].isplayer && CData.DevMode )
    debug_message( 1, "removel %s(%s) from platform", ChrList[object].name, CapList[ChrList[object].model].classname );


  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_to_platform( Uint16 object, Uint16 platform )
{
  remove_from_platform( object );

  if ( !VALID_CHR( object ) || !VALID_CHR( platform ) ) return
      bfalse;

  if ( !ChrList[platform].bmpdata.calc_is_platform )
    return bfalse;

  ChrList[object].onwhichplatform  = platform;
  if ( ChrList[object].weight > 0.0f )
    ChrList[platform].holdingweight += ChrList[object].weight;

  ChrList[object].jumpready  = btrue;
  ChrList[object].jumpnumber = ChrList[object].jumpnumberreset;

  ChrList[object].level = ChrList[platform].bmpdata.cv.z_max;

  if ( ChrList[object].isplayer )
    debug_message( 1, "attached %s(%s) to platform", ChrList[object].name, CapList[ChrList[object].model].classname );

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
  while ( fanblock < bumplist.num_blocks )
  {
    bumplist.num_chr[fanblock] = 0;
    bumplist.chr[fanblock]    = MAXCHR;
    bumplist.num_prt[fanblock] = 0;
    bumplist.prt[fanblock]    = MAXPRT;
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
    hide = CapList[ChrList[ichr].model].hidestate;
    if ( hide != NOHIDE && hide == ChrList[ichr].aistate ) continue;

    ix_min = MESH_FLOAT_TO_BLOCK( mesh_clip_x( ChrList[ichr].bmpdata.cv.x_min ) );
    ix_max = MESH_FLOAT_TO_BLOCK( mesh_clip_x( ChrList[ichr].bmpdata.cv.x_max ) );
    iy_min = MESH_FLOAT_TO_BLOCK( mesh_clip_y( ChrList[ichr].bmpdata.cv.y_min ) );
    iy_max = MESH_FLOAT_TO_BLOCK( mesh_clip_y( ChrList[ichr].bmpdata.cv.y_max ) );

    for ( ix = ix_min; ix <= ix_max; ix++ )
    {
      for ( iy = iy_min; iy <= iy_max; iy++ )
      {
        fanblock = mesh_convert_block( ix, iy );
        if ( INVALID_FAN == fanblock ) continue;

        // Insert before any other characters on the block
        entry = bumplist.chr[fanblock];
        ChrList[ichr].bumpnext = entry;
        bumplist.chr[fanblock] = ichr;
        bumplist.num_chr[fanblock]++;
      }
    }
  };


  for ( iprt = 0; iprt < MAXPRT; iprt++ )
  {
    // ignore invalid particles
    if ( !VALID_PRT( iprt ) || PrtList[iprt].gopoof ) continue;

    fanblock = mesh_get_block( PrtList[iprt].pos );
    if ( INVALID_FAN == fanblock ) continue;

    // Insert before any other particles on the block
    entry = bumplist.prt[fanblock];
    PrtList[iprt].bumpnext = entry;
    bumplist.prt[fanblock] = iprt;
    bumplist.num_prt[fanblock]++;
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

  ba = &(ChrList[chra].bmpdata);
  bb = &(ChrList[chrb].bmpdata);

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
  if(ChrList[chra].bmpdata.cv.level < 1)
  {
    md2_calculate_bumpers(chra, 1);
  }

  // set the minimum bumper level for object b
  if(ChrList[chrb].bmpdata.cv.level < 1)
  {
    md2_calculate_bumpers(chrb, 1);
  }

  // find the simplest collision volume
  find_collision_volume( &(ChrList[chra].pos), &(ChrList[chra].bmpdata.cv), &(ChrList[chrb].pos), &(ChrList[chrb].bmpdata.cv), exclude_height, cv);

  if ( chrcollisionlevel>1 && retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(ChrList[chra].bmpdata.cv.level < 2)
    {
      md2_calculate_bumpers(chra, 2);
      was_refined = btrue;
    }

    // refine the bumper
    if(ChrList[chrb].bmpdata.cv.level < 2)
    {
      md2_calculate_bumpers(chrb, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(ChrList[chra].pos), &(ChrList[chra].bmpdata.cv), &(ChrList[chrb].pos), &(ChrList[chrb].bmpdata.cv), exclude_height, cv);
    };

    if(chrcollisionlevel>2 && retval)
    {
      was_refined = bfalse;

      // refine the bumper
      if(ChrList[chra].bmpdata.cv.level < 3)
      {
        md2_calculate_bumpers(chra, 3);
        was_refined = btrue;
      }

      // refine the bumper
      if(ChrList[chrb].bmpdata.cv.level < 3)
      {
        md2_calculate_bumpers(chrb, 3);
        was_refined = btrue;
      }

      assert(NULL != ChrList[chra].bmpdata.cv_tree);
      assert(NULL != ChrList[chrb].bmpdata.cv_tree);

      if(was_refined)
      {
        int cnt, tnc;
        CVolume cv3, tmp_cv;
        bool_t loc_retval;

        retval = bfalse;
        cv3.level = -1;
        for(cnt=0; cnt<8; cnt++)
        {
          if(-1 == (*ChrList[chra].bmpdata.cv_tree)[cnt].level) continue;

          for(tnc=0; tnc<8; tnc++)
          {
            if(-1 == (*ChrList[chrb].bmpdata.cv_tree)[cnt].level) continue;

            loc_retval = find_collision_volume( &(ChrList[chra].pos), &((*ChrList[chra].bmpdata.cv_tree)[cnt]), &(ChrList[chrb].pos), &((*ChrList[chrb].bmpdata.cv_tree)[cnt]), exclude_height, &tmp_cv);

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
  bool_t retval = find_collision_volume( &(ChrList[chra].pos), &(ChrList[chra].bmpdata.cv), &(ChrList[prtb].pos), &(ChrList[prtb].bmpdata.cv), bfalse, NULL );

  if ( retval )
  {
    bool_t was_refined = bfalse;

    // refine the bumper
    if(ChrList[chra].bmpdata.cv.level < 2)
    {
      md2_calculate_bumpers(chra, 2);
      was_refined = btrue;
    }

    if(was_refined)
    {
      retval = find_collision_volume( &(ChrList[chra].pos), &(ChrList[chra].bmpdata.cv), &(ChrList[prtb].pos), &(ChrList[prtb].bmpdata.cv), bfalse, NULL );
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
  for ( fanblock = 0; fanblock < bumplist.num_blocks; fanblock++ )
  {
    chrinblock = bumplist.num_chr[fanblock];
    prtinblock = bumplist.num_prt[fanblock];

    //// remove bad platforms
    //for ( cnt = 0, chra = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  // detach character from invalid platforms
    //  chrb  = chr_get_onwhichplatform( chra );
    //  if ( VALID_CHR( chrb ) )
    //  {
    //    if ( !chr_is_inside( chra, 0.0f, chrb, btrue ) ||
    //         ChrList[chrb].bmpdata.cv.z_min  > ChrList[chrb].bmpdata.cv.z_max - PLATTOLERANCE )
    //    {
    //      remove_from_platform( chra );
    //    }
    //  }
    //};

    //// do attachments
    //for ( cnt = 0, chra = bumplist.chr[fanblock];
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
    //    if ( ChrList[chra].onwhichplatform == chrb || ChrList[chrb].onwhichplatform == chra ) continue;

    //    if ( chr_is_inside( chra, 0.0f, chrb, btrue) )
    //    {
    //      // check for compatibility
    //      if ( ChrList[chrb].bmpdata.calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( ChrList[chra].pos.z > MAX( ChrList[chrb].bmpdata.cv.z_min, ChrList[chrb].bmpdata.cv.z_max - PLATTOLERANCE ) && ChrList[chra].level < ChrList[chrb].bmpdata.cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chra, chrb );
    //        }
    //      }
    //    }
    //
    //    if( chr_is_inside( chrb, 0.0f, chra, btrue) )
    //    {
    //      if ( ChrList[chra].bmpdata.calc_is_platform )
    //      {
    //        // check for overlap in the z direction
    //        if ( ChrList[chrb].pos.z > MAX( ChrList[chra].bmpdata.cv.z_min, ChrList[chra].bmpdata.cv.z_max - PLATTOLERANCE ) && ChrList[chrb].level < ChrList[chra].bmpdata.cv.z_max )
    //        {
    //          // A is inside, coming from above
    //          attach_to_platform( chrb, chra );
    //        }
    //      }
    //    }

    //  }
    //}

    //// Do mounting
    //for ( cnt = 0, chra = bumplist.chr[fanblock];
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
    //      if ( ChrList[chra].pos.z > ChrList[chrb].bmpdata.cv.z_max - PLATTOLERANCE && ChrList[chra].pos.z < ChrList[chrb].bmpdata.cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is A falling on B?
    //        if ( ChrList[chra].vel.z < ChrList[chrb].vel.z )
    //        {
    //          if ( ChrList[chra].flyheight == 0 && ChrList[chra].alive && MadList[ChrList[chra].model].actionvalid[ACTION_MI] && !ChrList[chra].isitem )
    //          {
    //            if ( ChrList[chrb].alive && ChrList[chrb].ismount && !chr_using_slot( chrb, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chra );
    //              if ( !attach_character_to_mount( chra, chrb, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                ChrList[chra].alert |= ALERT_BUMPED;
    //                ChrList[chrb].alert |= ALERT_BUMPED;
    //                ChrList[chra].aibumplast = chrb;
    //                ChrList[chrb].aibumplast = chra;
    //              };
    //            }
    //          }
    //        }
    //      }

    //    }

    //    if( chr_is_inside( chrb, 0.0f, chra, btrue)   )
    //    {
    //      if ( ChrList[chrb].pos.z > ChrList[chra].bmpdata.cv.z_max - PLATTOLERANCE && ChrList[chrb].pos.z < ChrList[chra].bmpdata.cv.z_max + PLATTOLERANCE / 5 )
    //      {
    //        // Is B falling on A?
    //        if ( ChrList[chrb].vel.z < ChrList[chra].vel.z )
    //        {
    //          if ( ChrList[chrb].flyheight == 0 && ChrList[chrb].alive && MadList[ChrList[chrb].model].actionvalid[ACTION_MI] && !ChrList[chrb].isitem )
    //          {
    //            if ( ChrList[chra].alive && ChrList[chra].ismount && !chr_using_slot( chra, SLOT_SADDLE ) )
    //            {
    //              remove_from_platform( chrb );
    //              if ( !attach_character_to_mount( chrb, chra, SLOT_SADDLE ) )
    //              {
    //                // failed mount is a bump
    //                ChrList[chra].alert |= ALERT_BUMPED;
    //                ChrList[chrb].alert |= ALERT_BUMPED;
    //                ChrList[chra].aibumplast = chrb;
    //                ChrList[chrb].aibumplast = chra;
    //              };
    //            };
    //          }
    //        }
    //      }
    //    }
    //  }
    //}

    // do collisions
    for ( cnt = 0, chra = bumplist.chr[fanblock];
          cnt < chrinblock && VALID_CHR( chra );
          cnt++, chra = chr_get_bumpnext( chra ) )
    {
      float lerpa;
      lerpa = (ChrList[chra].pos.z - ChrList[chra].level) / PLATTOLERANCE;
      lerpa = CLIP(lerpa, 0, 1);

      apos = ChrList[chra].pos;

      // don't do object-object collisions if they won't feel each other
      if ( ChrList[chra].bumpstrength == 0.0f ) continue;

      // Do collisions (but not with attached items/characers)
      for ( chrb = chr_get_bumpnext( chra ), tnc = cnt + 1;
            tnc < chrinblock && VALID_CHR( chrb );
            tnc++, chrb = chr_get_bumpnext( chrb ) )
      {
        CVolume cv;
        float lerpb;

        float bumpstrength = ChrList[chra].bumpstrength * ChrList[chrb].bumpstrength;

        // don't do object-object collisions if they won't feel eachother
        if ( bumpstrength == 0.0f ) continue;

        // do not collide with something you are already holding
        if ( chrb == ChrList[chra].attachedto || chra == ChrList[chrb].attachedto ) continue;

        // do not collide with a your platform
        if ( chrb == ChrList[chra].onwhichplatform || chra == ChrList[chrb].onwhichplatform ) continue;

        bpos = ChrList[chrb].pos;

        lerpb = (ChrList[chrb].pos.z - ChrList[chrb].level) / PLATTOLERANCE;
        lerpb = CLIP(lerpb, 0, 1);

        if ( chr_do_collision( chra, chrb, bfalse, &cv) )
        {
          vect3 depth, ovlap, nrm, diffa, diffb;
          float ftmp, dotprod, pressure;
          float cr, m0, m1, psum, msum, udif, u0, u1, ln_cr;
          bool_t bfound;

          depth.x = (cv.x_max - cv.x_min);
          ovlap.x = depth.x / MIN(ChrList[chra].bmpdata.cv.x_max - ChrList[chra].bmpdata.cv.x_min, ChrList[chrb].bmpdata.cv.x_max - ChrList[chrb].bmpdata.cv.x_min);
          ovlap.x = CLIP(ovlap.x,-1,1);
          nrm.x = 1.0f / ovlap.x;

          depth.y = (cv.y_max - cv.y_min);
          ovlap.y = depth.y / MIN(ChrList[chra].bmpdata.cv.y_max - ChrList[chra].bmpdata.cv.y_min, ChrList[chrb].bmpdata.cv.y_max - ChrList[chrb].bmpdata.cv.y_min);
          ovlap.y = CLIP(ovlap.y,-1,1);
          nrm.y = 1.0f / ovlap.y;

          depth.z = (cv.z_max - cv.z_min);
          ovlap.z = depth.z / MIN(ChrList[chra].bmpdata.cv.z_max - ChrList[chra].bmpdata.cv.z_min, ChrList[chrb].bmpdata.cv.z_max - ChrList[chrb].bmpdata.cv.z_min);
          ovlap.z = CLIP(ovlap.z,-1,1);
          nrm.z = 1.0f / ovlap.z;

          nrm = Normalize(nrm);

          pressure = (depth.x / 30.0f) * (depth.y / 30.0f) * (depth.z / 30.0f);

          if(ovlap.x != 1.0)
          {
            diffa.x = ChrList[chra].bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
            diffb.x = ChrList[chrb].bmpdata.mids_lo.x - (cv.x_max + cv.x_min) * 0.5f;
          }
          else
          {
            diffa.x = ChrList[chra].bmpdata.mids_lo.x - ChrList[chrb].bmpdata.mids_lo.x;
            diffb.x =-diffa.x;
          }

          if(ovlap.y != 1.0)
          {
            diffa.y = ChrList[chra].bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
            diffb.y = ChrList[chrb].bmpdata.mids_lo.y - (cv.y_max + cv.y_min) * 0.5f;
          }
          else
          {
            diffa.y = ChrList[chra].bmpdata.mids_lo.y - ChrList[chrb].bmpdata.mids_lo.y;
            diffb.y =-diffa.y;
          }

          if(ovlap.y != 1.0)
          {
            diffa.z = ChrList[chra].bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffa.z += (ChrList[chra].bmpdata.mids_hi.z - ChrList[chra].bmpdata.mids_lo.z) * lerpa;

            diffb.z = ChrList[chrb].bmpdata.mids_lo.z - (cv.z_max + cv.z_min) * 0.5f;
            diffb.z += (ChrList[chrb].bmpdata.mids_hi.z - ChrList[chrb].bmpdata.mids_lo.z) * lerpb;
          }
          else
          {
            diffa.z  = ChrList[chra].bmpdata.mids_lo.z - ChrList[chrb].bmpdata.mids_lo.z;
            diffa.z += (ChrList[chra].bmpdata.mids_hi.z - ChrList[chra].bmpdata.mids_lo.z) * lerpa;
            diffa.z -= (ChrList[chrb].bmpdata.mids_hi.z - ChrList[chrb].bmpdata.mids_lo.z) * lerpb;

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
          //cr = ChrList[chrb].bumpdampen * ChrList[chra].bumpdampen * bumpstrength * ovlap.z * ( nrm.x * nrm.x * ovlap.x + nrm.y * nrm.y * ovlap.y ) / ftmp;

          // determine a usable mass
          m0 = -1;
          m1 = -1;
          if ( ChrList[chra].weight < 0 && ChrList[chrb].weight < 0 )
          {
            m0 = m1 = 110.0f;
          }
          else if (ChrList[chra].weight == 0 && ChrList[chrb].weight == 0)
          {
            m0 = m1 = 1.0f;
          }
          else
          {
            m0 = (ChrList[chra].weight == 0.0f) ? 1.0 : ChrList[chra].weight;
            m1 = (ChrList[chrb].weight == 0.0f) ? 1.0 : ChrList[chrb].weight;
          }

          bfound = btrue;
          cr = ChrList[chrb].bumpdampen * ChrList[chra].bumpdampen;
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

            ChrList[chra].accum_acc.x += (diffa.x * k  - ChrList[chra].vel.x * gamma) * bumpstrength;
            ChrList[chra].accum_acc.y += (diffa.y * k  - ChrList[chra].vel.y * gamma) * bumpstrength;
            ChrList[chra].accum_acc.z += (diffa.z * k  - ChrList[chra].vel.z * gamma) * bumpstrength;
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

            ChrList[chrb].accum_acc.x += (diffb.x * k  - ChrList[chrb].vel.x * gamma) * bumpstrength;
            ChrList[chrb].accum_acc.y += (diffb.y * k  - ChrList[chrb].vel.y * gamma) * bumpstrength;
            ChrList[chrb].accum_acc.z += (diffb.z * k  - ChrList[chrb].vel.z * gamma) * bumpstrength;
          }


          //bfound = bfalse;
          //if (( ChrList[chra].bmpdata.mids_lo.x - ChrList[chrb].bmpdata.mids_lo.x ) * ( ChrList[chra].vel.x - ChrList[chrb].vel.x ) < 0.0f )
          //{
          //  u0 = ChrList[chra].vel.x;
          //  u1 = ChrList[chrb].vel.x;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  ChrList[chra].vel.x = ( psum - m1 * udif * cr ) / msum;
          //  ChrList[chrb].vel.x = ( psum + m0 * udif * cr ) / msum;

          //  //ChrList[chra].bmpdata.mids_lo.x -= ChrList[chra].vel.x*dUpdate;
          //  //ChrList[chrb].bmpdata.mids_lo.x -= ChrList[chrb].vel.x*dUpdate;

          //  bfound = btrue;
          //}



          //if (( ChrList[chra].bmpdata.mids_lo.y - ChrList[chrb].bmpdata.mids_lo.y ) * ( ChrList[chra].vel.y - ChrList[chrb].vel.y ) < 0.0f )
          //{
          //  u0 = ChrList[chra].vel.y;
          //  u1 = ChrList[chrb].vel.y;

          //  psum = m0 * u0 + m1 * u1;
          //  udif = u1 - u0;

          //  ChrList[chra].vel.y = ( psum - m1 * udif * cr ) / msum;
          //  ChrList[chrb].vel.y = ( psum + m0 * udif * cr ) / msum;

          //  //ChrList[chra].bmpdata.mids_lo.y -= ChrList[chra].vel.y*dUpdate;
          //  //ChrList[chrb].bmpdata.mids_lo.y -= ChrList[chrb].vel.y*dUpdate;

          //  bfound = btrue;
          //}

          //if ( ovlap.x > 0 && ovlap.z > 0 )
          //{
          //  ChrList[chra].bmpdata.mids_lo.x += m1 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  ChrList[chrb].bmpdata.mids_lo.x -= m0 / ( m0 + m1 ) * ovlap.y * 0.5 * ovlap.z;
          //  bfound = btrue;
          //}

          //if ( ovlap.y > 0 && ovlap.z > 0 )
          //{
          //  ChrList[chra].bmpdata.mids_lo.y += m1 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  ChrList[chrb].bmpdata.mids_lo.y -= m0 / ( m0 + m1 ) * ovlap.x * 0.5f * ovlap.z;
          //  bfound = btrue;
          //}

          if ( bfound )
          {
            //apos = ChrList[chra].pos;
            ChrList[chra].alert |= ALERT_BUMPED;
            ChrList[chrb].alert |= ALERT_BUMPED;
            ChrList[chra].aibumplast = chrb;
            ChrList[chrb].aibumplast = chra;
          };
        }
      }
    };

    // Now check collisions with every bump particle in same area
    //for ( cnt = 0, chra = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  IDSZ chridvulnerability, eveidremove;
    //  float chrbump = 1.0f;

    //  apos = ChrList[chra].pos;
    //  chridvulnerability = CapList[ChrList[chra].model].idsz[IDSZ_VULNERABILITY];
    //  chrbump = ChrList[chra].bumpstrength;

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

    //    chr_is_vulnerable = !ChrList[chra].invictus && ( IDSZ_NONE != chridvulnerability ) && CAP_INHERIT_IDSZ( PrtList[prtb].model, chridvulnerability );

    //    prtbump = PrtList[prtb].bumpstrength;
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
    //        pvel.x = ( PrtList[prtb].pos.x - PrtList[prtb].pos_old.x ) / dUpdate;
    //        pvel.y = ( PrtList[prtb].pos.y - PrtList[prtb].pos_old.y ) / dUpdate;
    //        pvel.z = ( PrtList[prtb].pos.z - PrtList[prtb].pos_old.z ) / dUpdate;
    //      }
    //      else
    //      {
    //        pvel = PrtList[prtb].vel;
    //      }

    //      if ( bpos.z > ChrList[chra].bmpdata.cv.z_max + pvel.z && pvel.z < 0 && ChrList[chra].bmpdata.calc_is_platform && !VALID_CHR( prt_attached ) )
    //      {
    //        // Particle is falling on A
    //        PrtList[prtb].accum_pos.z += ChrList[chra].bmpdata.cv.z_max - PrtList[prtb].pos.z;

    //        PrtList[prtb].accum_vel.z = - (1.0f - PipList[pip].dampen * ChrList[chra].bumpdampen) * PrtList[prtb].vel.z;

    //        PrtList[prtb].accum_acc.x += ( pvel.x - ChrList[chra].vel.x ) * ( 1.0 - loc_platstick ) + ChrList[chra].vel.x;
    //        PrtList[prtb].accum_acc.y += ( pvel.y - ChrList[chra].vel.y ) * ( 1.0 - loc_platstick ) + ChrList[chra].vel.y;
    //      }

    //      // Check reaffirmation of particles
    //      if ( prt_attached != chra )
    //      {
    //        if ( ChrList[chra].reloadtime == 0 )
    //        {
    //          if ( ChrList[chra].reaffirmdamagetype == PrtList[prtb].damagetype && ChrList[chra].damagetime == 0 )
    //          {
    //            reaffirm_attached_particles( chra );
    //          }
    //        }
    //      }

    //      // Check for missile treatment
    //      if (( ChrList[chra].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_SHIFT ) != DAMAGE_SHIFT ||
    //            MIS_NORMAL == ChrList[chra].missiletreatment  ||
    //            VALID_CHR( prt_attached ) ||
    //            ( prt_owner == chra && !PipList[pip].friendlyfire ) ||
    //            ( ChrList[chrmissilehandler[chra]].mana_fp8 < ( ChrList[chra].missilecost << 4 ) && !ChrList[chrmissilehandler[chra]].canchannel ) )
    //      {
    //        if (( TeamList[PrtList[prtb].team].hatesteam[ChrList[chra].team] || ( PipList[pip].friendlyfire && (( chra != prt_owner && chra != ChrList[prt_owner].attachedto ) || PipList[pip].onlydamagefriendly ) ) ) && !ChrList[chra].invictus )
    //        {
    //          spawn_bump_particles( chra, prtb );  // Catch on fire

    //          if (( PrtList[prtb].damage.ibase > 0 ) && ( PrtList[prtb].damage.irand > 0 ) )
    //          {
    //            if ( ChrList[chra].damagetime == 0 && prt_attached != chra && HAS_NO_BITS( PipList[pip].damfx, DAMFX_ARRO ) )
    //            {

    //              // Normal prtb damage
    //              if ( PipList[pip].allowpush )
    //              {
    //                float ftmp = 0.2;

    //                if ( ChrList[chra].weight < 0 )
    //                {
    //                  ftmp = 0;
    //                }
    //                else if ( ChrList[chra].weight != 0 )
    //                {
    //                  ftmp *= ( 1.0f + ChrList[chra].bumpdampen ) * PrtList[prtb].weight / ChrList[chra].weight;
    //                }

    //                ChrList[chra].accum_vel.x += pvel.x * ftmp;
    //                ChrList[chra].accum_vel.y += pvel.y * ftmp;
    //                ChrList[chra].accum_vel.z += pvel.z * ftmp;

    //                PrtList[prtb].accum_vel.x += -ChrList[chra].bumpdampen * pvel.x - PrtList[prtb].vel.x;
    //                PrtList[prtb].accum_vel.y += -ChrList[chra].bumpdampen * pvel.y - PrtList[prtb].vel.y;
    //                PrtList[prtb].accum_vel.z += -ChrList[chra].bumpdampen * pvel.z - PrtList[prtb].vel.z;
    //              }

    //              direction = RAD_TO_TURN( atan2( pvel.y, pvel.x ) );
    //              direction = 32768 + ChrList[chra].turn_lr - direction;

    //              // Check all enchants to see if they are removed
    //              enchant = ChrList[chra].firstenchant;
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
    //                if(!ChrList[chra].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_INVERT || !ChrList[chra].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_CHARGE)
    //                PrtList[prtb].damage.ibase -= (PrtList[prtb].damage.ibase * ( ChrList[chra].wisdom_fp8 > 8 ));    //Then reduce it by defender
    //              }
    //              if ( PipList[pip].wisdamagebonus )  //Same with divine spells
    //              {
    //                PrtList[prtb].damage.ibase += (( ChrList[prt_owner].wisdom_fp8 - 3584 ) * 0.25 );
    //                if(!ChrList[chra].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_INVERT || !ChrList[chra].damagemodifier_fp8[PrtList[prtb].damagetype]&DAMAGE_CHARGE)
    //                PrtList[prtb].damage.ibase -= (PrtList[prtb].damage.ibase * ( ChrList[chra].intelligence_fp8 > 8 ));
    //              }

    //              //Force Pancake animation?
    //              if ( PipList[pip].causepancake )
    //              {
    //                vect3 panc;
    //                Uint16 rotate_cos, rotate_sin;
    //                float cv, sv;

    //                // just a guess
    //                panc.x = 0.25 * ABS( pvel.x ) * 2.0f / ( float )( 1 + ChrList[chra].bmpdata.cv.x_max - ChrList[chra].bmpdata.cv.x_min  );
    //                panc.y = 0.25 * ABS( pvel.y ) * 2.0f / ( float )( 1 + ChrList[chra].bmpdata.cv.y_max - ChrList[chra].bmpdata.cv.y_min );
    //                panc.z = 0.25 * ABS( pvel.z ) * 2.0f / ( float )( 1 + ChrList[chra].bmpdata.cv.z_max - ChrList[chra].bmpdata.cv.z_min );

    //                rotate_sin = ChrList[chra].turn_lr >> 2;
    //                rotate_cos = ( rotate_sin + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;

    //                cv = turntosin[rotate_cos];
    //                sv = turntosin[rotate_sin];

    //                ChrList[chra].pancakevel.x = - ( panc.x * cv - panc.y * sv );
    //                ChrList[chra].pancakevel.y = - ( panc.x * sv + panc.y * cv );
    //                ChrList[chra].pancakevel.z = -panc.z;
    //              }

    //              // Damage the character
    //              if ( chr_is_vulnerable )
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = PrtList[prtb].damage.ibase * 2.0f * bumpstrength;
    //                ptemp.irand = PrtList[prtb].damage.irand * 2.0f * bumpstrength;
    //                damage_character( chra, direction, &ptemp, PrtList[prtb].damagetype, PrtList[prtb].team, prt_owner, PipList[pip].damfx );
    //                ChrList[chra].alert |= ALERT_HITVULNERABLE;
    //                cost_mana( chra, PipList[pip].manadrain*2, prt_owner );  //Do mana drain too
    //              }
    //              else
    //              {
    //                PAIR ptemp;
    //                ptemp.ibase = PrtList[prtb].damage.ibase * bumpstrength;
    //                ptemp.irand = PrtList[prtb].damage.irand * bumpstrength;

    //                damage_character( chra, direction, &PrtList[prtb].damage, PrtList[prtb].damagetype, PrtList[prtb].team, prt_owner, PipList[pip].damfx );
    //                cost_mana( chra, PipList[pip].manadrain, prt_owner );  //Do mana drain too
    //              }

    //              // Do confuse effects
    //              if ( HAS_NO_BITS( MadList[ChrList[chra].model].framefx[ChrList[chra].anim.next], MADFX_INVICTUS ) || HAS_SOME_BITS( PipList[pip].damfx, DAMFX_BLOC ) )
    //              {

    //                if ( PipList[pip].grogtime != 0 && CapList[ChrList[chra].model].canbegrogged )
    //                {
    //                  ChrList[chra].grogtime += PipList[pip].grogtime * bumpstrength;
    //                  if ( ChrList[chra].grogtime < 0 )
    //                  {
    //                    ChrList[chra].grogtime = -1;
    //                    debug_message( 1, "placing infinite grog on %s (%s)", ChrList[chra].name, CapList[ChrList[chra].model].classname );
    //                  }
    //                  ChrList[chra].alert |= ALERT_GROGGED;
    //                }

    //                if ( PipList[pip].dazetime != 0 && CapList[ChrList[chra].model].canbedazed )
    //                {
    //                  ChrList[chra].dazetime += PipList[pip].dazetime * bumpstrength;
    //                  if ( ChrList[chra].dazetime < 0 )
    //                  {
    //                    ChrList[chra].dazetime = -1;
    //                    debug_message( 1, "placing infinite daze on %s (%s)", ChrList[chra].name, CapList[ChrList[chra].model].classname );
    //                  };
    //                  ChrList[chra].alert |= ALERT_DAZED;
    //                }
    //              }

    //              // Notify the attacker of a scored hit
    //              if ( VALID_CHR( prt_owner ) )
    //              {
    //                ChrList[prt_owner].alert |= ALERT_SCOREDAHIT;
    //                ChrList[prt_owner].aihitlast = chra;
    //              }
    //            }

    //            if (( wldframe&31 ) == 0 && prt_attached == chra )
    //            {
    //              // Attached prtb damage ( Burning )
    //              if ( PipList[pip].xyvel.ibase == 0 )
    //              {
    //                // Make character limp
    //                ChrList[chra].vel.x = 0;
    //                ChrList[chra].vel.y = 0;
    //              }
    //              damage_character( chra, 32768, &PrtList[prtb].damage, PrtList[prtb].damagetype, PrtList[prtb].team, prt_owner, PipList[pip].damfx );
    //              cost_mana( chra, PipList[pip].manadrain, prt_owner );  //Do mana drain too

    //            }
    //          }

    //          if ( PipList[pip].endbump )
    //          {
    //            if ( PipList[pip].bumpmoney )
    //            {
    //              if ( ChrList[chra].cangrabmoney && ChrList[chra].alive && ChrList[chra].damagetime == 0 && ChrList[chra].money != MAXMONEY )
    //              {
    //                if ( ChrList[chra].ismount )
    //                {
    //                  CHR_REF irider = chr_get_holdingwhich( chra, SLOT_SADDLE );

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
    //                  ChrList[chra].money += PipList[pip].bumpmoney;
    //                  if ( ChrList[chra].money > MAXMONEY ) ChrList[chra].money = MAXMONEY;
    //                  if ( ChrList[chra].money < 0 ) ChrList[chra].money = 0;
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
    //      else if ( prt_owner != chra )
    //      {
    //        cost_mana( ChrList[chra].missilehandler, ( ChrList[chra].missilecost << 4 ), prt_owner );

    //        // Treat the missile
    //        switch ( ChrList[chra].missiletreatment )
    //        {
    //          case MIS_DEFLECT:
    //            {
    //              // Use old position to find normal
    //              acc.x = PrtList[prtb].pos.x - pvel.x * dUpdate;
    //              acc.y = PrtList[prtb].pos.y - pvel.y * dUpdate;
    //              acc.x = ChrList[chra].pos.x - acc.x;
    //              acc.y = ChrList[chra].pos.y - acc.y;
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
    //              // Reflect it back in the direction it GCamera.e
    //              PrtList[prtb].accum_vel.x += -2.0f * PrtList[prtb].vel.x;
    //              PrtList[prtb].accum_vel.y += -2.0f * PrtList[prtb].vel.y;
    //            };
    //            break;
    //        };


    //        // Change the owner of the missile
    //        if ( !PipList[pip].homing )
    //        {
    //          PrtList[prtb].team = ChrList[chra].team;
    //          prt_owner = chra;
    //        }
    //      }
    //    }
    //  }
    //}

    // do platform physics
    //for ( cnt = 0, chra = bumplist.chr[fanblock];
    //      cnt < chrinblock && VALID_CHR( chra );
    //      cnt++, chra = chr_get_bumpnext( chra ) )
    //{
    //  // detach character from invalid platforms
    //  chrb  = chr_get_onwhichplatform( chra );
    //  if ( !VALID_CHR( chrb ) ) continue;

    //  if ( ChrList[chra].pos.z < ChrList[chrb].bmpdata.cv.z_max + RAISE )
    //  {
    //    ChrList[chra].pos.z = ChrList[chrb].bmpdata.cv.z_max + RAISE;
    //    if ( ChrList[chra].vel.z < ChrList[chrb].vel.z )
    //    {
    //      ChrList[chra].vel.z = - ( ChrList[chra].vel.z - ChrList[chrb].vel.z ) * ChrList[chra].bumpdampen * ChrList[chrb].bumpdampen + ChrList[chrb].vel.z;
    //    };
    //  }

    //  ChrList[chra].vel.x = ( ChrList[chra].vel.x - ChrList[chrb].vel.x ) * ( 1.0 - loc_platstick ) + ChrList[chrb].vel.x;
    //  ChrList[chra].vel.y = ( ChrList[chra].vel.y - ChrList[chrb].vel.y ) * ( 1.0 - loc_platstick ) + ChrList[chrb].vel.y;
    //  ChrList[chra].turn_lr += ( ChrList[chrb].turn_lr - ChrList[chrb].turn_lr_old ) * ( 1.0 - loc_platstick );
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
  CHR_REF ichr;
  bool_t willgetcaught;
  float newsize, fkeep;

  fkeep = pow( 0.9995, dUpdate );

  for ( ichr = 0; ichr < MAXCHR; ichr++ )
  {
    if ( !VALID_CHR( ichr ) || ChrList[ichr].sizegototime <= 0 ) continue;

    // Make sure it won't get caught in a wall
    willgetcaught = bfalse;
    if ( ChrList[ichr].sizegoto > ChrList[ichr].fat )
    {
      float x_min_save, x_max_save;
      float y_min_save, y_max_save;

      x_min_save = ChrList[ichr].bmpdata.cv.x_min;
      x_max_save = ChrList[ichr].bmpdata.cv.x_max;

      y_min_save = ChrList[ichr].bmpdata.cv.y_min;
      y_max_save = ChrList[ichr].bmpdata.cv.y_max;

      ChrList[ichr].bmpdata.cv.x_min -= 5;
      ChrList[ichr].bmpdata.cv.y_min -= 5;

      ChrList[ichr].bmpdata.cv.x_max += 5;
      ChrList[ichr].bmpdata.cv.y_max += 5;

      if ( 0 != __chrhitawall( ichr, NULL ) )
      {
        willgetcaught = btrue;
      }

      ChrList[ichr].bmpdata.cv.x_min = x_min_save;
      ChrList[ichr].bmpdata.cv.x_max = x_max_save;

      ChrList[ichr].bmpdata.cv.y_min = y_min_save;
      ChrList[ichr].bmpdata.cv.y_max = y_max_save;
    }


    // If it is getting caught, simply halt growth until later
    if ( willgetcaught ) continue;

    // Figure out how big it is
    ChrList[ichr].sizegototime -= dUpdate;
    if ( ChrList[ichr].sizegototime < 0 )
    {
      ChrList[ichr].sizegototime = 0;
    }

    if ( ChrList[ichr].sizegototime > 0 )
    {
      newsize = ChrList[ichr].fat * fkeep + ChrList[ichr].sizegoto * ( 1.0f - fkeep );
    }
    else if ( ChrList[ichr].sizegototime <= 0 )
    {
      newsize = ChrList[ichr].fat;
    }

    // Make it that big...
    ChrList[ichr].fat             = newsize;
    ChrList[ichr].bmpdata.shadow  = ChrList[ichr].bmpdata_save.shadow * newsize;
    ChrList[ichr].bmpdata.size    = ChrList[ichr].bmpdata_save.size * newsize;
    ChrList[ichr].bmpdata.sizebig = ChrList[ichr].bmpdata_save.sizebig * newsize;
    ChrList[ichr].bmpdata.height  = ChrList[ichr].bmpdata_save.height * newsize;
    ChrList[ichr].weight          = CapList[ChrList[ichr].model].weight * newsize * newsize * newsize;  // preserve density

    // Now come up with the magic number
    ChrList[ichr].scale = newsize;

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
  profile = ChrList[character].model;
  filewrite = fs_fileOpen( PRI_FAIL, "export_one_character_name()", szSaveName, "w" );
  if ( NULL== filewrite )
  {
      log_error( "Error writing file (%s)\n", szSaveName );
      return;
  }

  convert_spaces( ChrList[character].name, sizeof( ChrList[character].name ), ChrList[character].name );
  fprintf( filewrite, ":%s\n", ChrList[character].name );
  fprintf( filewrite, ":STOP\n\n" );
  fs_fileClose( filewrite );
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
  profile = ChrList[character].model;


  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( filewrite )
  {
    // Real general data
    fprintf( filewrite, "Slot number    : -1\n" );   // -1 signals a flexible load thing
    funderf( filewrite, "Class name     : ", CapList[profile].classname );
    ftruthf( filewrite, "Uniform light  : ", CapList[profile].uniformlit );
    fprintf( filewrite, "Maximum ammo   : %d\n", CapList[profile].ammomax );
    fprintf( filewrite, "Current ammo   : %d\n", CapList[character].ammo );
    fgendef( filewrite, "Gender         : ", CapList[character].gender );
    fprintf( filewrite, "\n" );



    // Object stats
    fprintf( filewrite, "Life color     : %d\n", CapList[character].lifecolor );
    fprintf( filewrite, "Mana color     : %d\n", CapList[character].manacolor );
    fprintf( filewrite, "Life           : %4.2f\n", FP8_TO_FLOAT( ChrList[character].lifemax_fp8 ) );
    fpairof( filewrite, "Life up        : ", &CapList[profile].lifeperlevel_fp8 );
    fprintf( filewrite, "Mana           : %4.2f\n", FP8_TO_FLOAT( ChrList[character].manamax_fp8 ) );
    fpairof( filewrite, "Mana up        : ", &CapList[profile].manaperlevel_fp8 );
    fprintf( filewrite, "Mana return    : %4.2f\n", FP8_TO_FLOAT( ChrList[character].manareturn_fp8 ) );
    fpairof( filewrite, "Mana return up : ", &CapList[profile].manareturnperlevel_fp8 );
    fprintf( filewrite, "Mana flow      : %4.2f\n", FP8_TO_FLOAT( ChrList[character].manaflow_fp8 ) );
    fpairof( filewrite, "Mana flow up   : ", &CapList[profile].manaflowperlevel_fp8 );
    fprintf( filewrite, "STR            : %4.2f\n", FP8_TO_FLOAT( ChrList[character].strength_fp8 ) );
    fpairof( filewrite, "STR up         : ", &CapList[profile].strengthperlevel_fp8 );
    fprintf( filewrite, "WIS            : %4.2f\n", FP8_TO_FLOAT( ChrList[character].wisdom_fp8 ) );
    fpairof( filewrite, "WIS up         : ", &CapList[profile].wisdomperlevel_fp8 );
    fprintf( filewrite, "INT            : %4.2f\n", FP8_TO_FLOAT( ChrList[character].intelligence_fp8 ) );
    fpairof( filewrite, "INT up         : ", &CapList[profile].intelligenceperlevel_fp8 );
    fprintf( filewrite, "DEX            : %4.2f\n", FP8_TO_FLOAT( ChrList[character].dexterity_fp8 ) );
    fpairof( filewrite, "DEX up         : ", &CapList[profile].dexterityperlevel_fp8 );
    fprintf( filewrite, "\n" );



    // More physical attributes
    fprintf( filewrite, "Size           : %4.2f\n", ChrList[character].sizegoto );
    fprintf( filewrite, "Size up        : %4.2f\n", CapList[profile].sizeperlevel );
    fprintf( filewrite, "Shadow size    : %d\n", CapList[profile].shadowsize );
    fprintf( filewrite, "Bump size      : %d\n", CapList[profile].bumpsize );
    fprintf( filewrite, "Bump height    : %d\n", CapList[profile].bumpheight );
    fprintf( filewrite, "Bump dampen    : %4.2f\n", CapList[profile].bumpdampen );
    fprintf( filewrite, "Weight         : %d\n", CapList[profile].weight < 0.0f ? 0xFF : ( Uint8 ) CapList[profile].weight );
    fprintf( filewrite, "Jump power     : %4.2f\n", CapList[profile].jump );
    fprintf( filewrite, "Jump number    : %d\n", CapList[profile].jumpnumber );
    fprintf( filewrite, "Sneak speed    : %d\n", CapList[profile].sneakspd );
    fprintf( filewrite, "Walk speed     : %d\n", CapList[profile].walkspd );
    fprintf( filewrite, "Run speed      : %d\n", CapList[profile].runspd );
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
    fprintf( filewrite, "Starting EXP   : %d\n", CapList[character].experience );
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
    ftruthf( filewrite, "Name known     : ", CapList[character].nameknown );
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
    fprintf( filewrite, "Kursed chance  : %d\n", ChrList[character].iskursed*100 );
    fprintf( filewrite, "Footfall sound : %d\n", CapList[profile].footfallsound );
    fprintf( filewrite, "Jump sound     : %d\n", CapList[profile].jumpsound );
    fprintf( filewrite, "\n" );


    // Expansions
    fprintf( filewrite, ":[GOLD] %d\n", CapList[character].money );

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
    if ( CapList[character].bumpsizebig >= CapList[character].bumpsize ) fprintf( filewrite, ":[SQUA] 1\n" );
    if ( CapList[character].icon != CapList[profile].usageknown ) fprintf( filewrite, ":[ICON] %d\n", CapList[character].icon );
    if ( CapList[profile].forceshadow ) fprintf( filewrite, ":[SHAD] 1\n" );

    //Skill expansions
    if ( CapList[character].canseekurse )  fprintf( filewrite, ":[CKUR] 1\n" );
    if ( CapList[character].canusearcane ) fprintf( filewrite, ":[WMAG] 1\n" );
    if ( CapList[character].canjoust )     fprintf( filewrite, ":[JOUS] 1\n" );
    if ( CapList[character].canusedivine ) fprintf( filewrite, ":[HMAG] 1\n" );
    if ( CapList[character].candisarm )    fprintf( filewrite, ":[DISA] 1\n" );
    if ( CapList[character].canusetech )   fprintf( filewrite, ":[TECH] 1\n" );
    if ( CapList[character].canbackstab )  fprintf( filewrite, ":[STAB] 1\n" );
    if ( CapList[character].canuseadvancedweapons ) fprintf( filewrite, ":[AWEP] 1\n" );
    if ( CapList[character].canusepoison ) fprintf( filewrite, ":[POIS] 1\n" );
    if ( CapList[character].canread )  fprintf( filewrite, ":[READ] 1\n" );

    //General exported character information
    fprintf( filewrite, ":[PLAT] %d\n", CapList[profile].canuseplatforms );
    fprintf( filewrite, ":[SKIN] %d\n", ( ChrList[character].texture - MadList[profile].skinstart ) % MAXSKIN );
    fprintf( filewrite, ":[CONT] %d\n", ChrList[character].aicontent );
    fprintf( filewrite, ":[STAT] %d\n", ChrList[character].aistate );
    fprintf( filewrite, ":[LEVL] %d\n", ChrList[character].experiencelevel );
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
  profile = ChrList[character].model;

  // Open the file
  filewrite = fs_fileOpen( PRI_NONE, NULL, szSaveName, "w" );
  if ( NULL != filewrite )
  {
    fprintf( filewrite, "This file is used only by the import menu\n" );
    fprintf( filewrite, ": %d\n", ( ChrList[character].texture - MadList[profile].skinstart ) % MAXSKIN );
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
  pcap->sneakspd = fget_next_int( fileread );
  pcap->walkspd = fget_next_int( fileread );
  pcap->runspd = fget_next_int( fileread );
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
int modify_quest_idsz( char *whichplayer, IDSZ idsz, int adjustment )
{
  // ZF> This function increases or decreases a Quest IDSZ quest level by the amount determined in
  // adjustment. It then returns the current quest level it now has.
  // It returns -2 if failed and if the adjustment is 0, the quest is marked as beaten...
 
  FILE *newfile, *fileread;
  STRING newloadname, copybuffer;
  bool_t foundidsz = bfalse;
  IDSZ newidsz;
  Sint8 NewQuestLevel = -2, QuestLevel;

  // Try to open the file in read/write mode
  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return NewQuestLevel;

  //Now check each expansion until we find correct IDSZ
  while ( fgoto_colon_yesno( fileread ) && !foundidsz )
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
	  else
	  {
		  //First close the file, rename it and reopen it for reading
		  fs_fileClose( fileread );
		  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
		  snprintf( copybuffer, sizeof( copybuffer ), "%s/%s/tmp_%s", CData.players_dir, whichplayer, CData.quest_file);
		  rename(newloadname, copybuffer); 
          fileread = fs_fileOpen( PRI_NONE, NULL, copybuffer, "r" );

		  //Then make the new Quest.txt and add the new modifications
		  snprintf( newloadname, sizeof( newloadname ), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file );
          newfile = fs_fileOpen( PRI_WARN, NULL, newloadname, "w" );
		  fprintf(newfile, "//This file keeps order of all the quests for the player\n");
		  fprintf(newfile, "//The number after the IDSZ shows the quest level. -1 means it is completed.\n");
          
		  //Now read the old quest file and copy each line to the new one
		  while ( fgoto_colon_yesno( fileread ))
		  {
			  newidsz = fget_idsz( fileread );
			  if ( newidsz != idsz )
			  {
				QuestLevel = fget_int( fileread );
				snprintf( copybuffer, sizeof( copybuffer ), "\n:[%s] %i",  undo_idsz(newidsz), QuestLevel );
                fprintf( newfile, copybuffer );
			  }
			  else	//This is where we actually modify the part we need
			  {
				  QuestLevel = fget_int( fileread );
				  if(adjustment == 0) NewQuestLevel = -1;
				  else 
				  {
					  NewQuestLevel = QuestLevel + adjustment;
					  if(NewQuestLevel < 0) NewQuestLevel = 0;
				  }
				  fprintf(newfile, "\n:[%s] %i", undo_idsz(idsz), NewQuestLevel);
			  }
		  }		  
		  fs_fileClose( newfile );
	  }
    }
  }
  fs_fileClose( fileread );
  
  //Delete the old quest file
  if(foundidsz)
  {
	snprintf( copybuffer, sizeof( copybuffer ), "%s/%s/tmp_%s", CData.players_dir, whichplayer, CData.quest_file);
	fs_deleteFile( copybuffer );
  }
  return NewQuestLevel;
}

//--------------------------------------------------------------------------------------------
bool_t chr_attached( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return bfalse;

  ChrList[ichr].attachedto = VALIDATE_CHR( ChrList[ichr].attachedto );
  if(!VALID_CHR(ichr)) ChrList[ichr].inwhichslot = SLOT_NONE;

  return VALID_CHR( ChrList[ichr].attachedto );
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
    assert( ChrList[ichr].numinpack == 0 );
  }
#endif

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t chr_is_invisible( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return btrue;

  return FP8_MUL( ChrList[ichr].alpha_fp8, ChrList[ichr].light_fp8 ) <= INVISIBLE;
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
  nextinpack = ChrList[ichr].nextinpack;
  if ( MAXCHR != nextinpack && !ChrList[ichr].on )
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

  ChrList[ichr].nextinpack = VALIDATE_CHR( ChrList[ichr].nextinpack );
  return ChrList[ichr].nextinpack;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_onwhichplatform( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].onwhichplatform = VALIDATE_CHR( ChrList[ichr].onwhichplatform );
  return ChrList[ichr].onwhichplatform;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_holdingwhich( CHR_REF ichr, SLOT slot )
{
  CHR_REF inslot;

  if ( !VALID_CHR( ichr ) || slot >= SLOT_COUNT ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  inslot = ChrList[ichr].holdingwhich[slot];
  if ( MAXCHR != inslot )
  {
    CHR_REF holder = ChrList[inslot].attachedto;

    if ( ichr != holder )
    {
      // invalid configuration
      assert( bfalse );
    }
  };
#endif

  ChrList[ichr].holdingwhich[slot] = VALIDATE_CHR( ChrList[ichr].holdingwhich[slot] );
  return ChrList[ichr].holdingwhich[slot];
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_inwhichpack( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].inwhichpack = VALIDATE_CHR( ChrList[ichr].inwhichpack );
  return ChrList[ichr].inwhichpack;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_attachedto( CHR_REF ichr )
{
  CHR_REF holder;

  if ( !VALID_CHR( ichr ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)

  if( MAXCHR != ChrList[ichr].attachedto )
  {
    SLOT slot = ChrList[ichr].inwhichslot;
    if(slot != SLOT_INVENTORY)
    {
      assert(SLOT_NONE != slot);
      holder = ChrList[ichr].attachedto;
      assert( ChrList[holder].holdingwhich[slot] == ichr );
    };
  }
  else
  {
    assert(SLOT_NONE == ChrList[ichr].inwhichslot);
  };
#endif

  ChrList[ichr].attachedto = VALIDATE_CHR( ChrList[ichr].attachedto );
  if( !VALID_CHR( ChrList[ichr].attachedto ) ) ChrList[ichr].inwhichslot = SLOT_NONE;
  return ChrList[ichr].attachedto;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_bumpnext( CHR_REF ichr )
{
  CHR_REF bumpnext;

  if ( !VALID_CHR( ichr ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  bumpnext = ChrList[ichr].bumpnext;
  if ( bumpnext < MAXCHR && !ChrList[bumpnext].on && ChrList[bumpnext].bumpnext < MAXCHR )
  {
    // this is an invalid configuration
    assert( bfalse );
    return chr_get_bumpnext( bumpnext );
  }
#endif

  ChrList[ichr].bumpnext = VALIDATE_CHR( ChrList[ichr].bumpnext );
  return ChrList[ichr].bumpnext;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aitarget( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].aitarget = VALIDATE_CHR( ChrList[ichr].aitarget );
  return ChrList[ichr].aitarget;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aiowner( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].aiowner = VALIDATE_CHR( ChrList[ichr].aiowner );
  return ChrList[ichr].aiowner;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aichild( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].aichild = VALIDATE_CHR( ChrList[ichr].aichild );
  return ChrList[ichr].aichild;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aiattacklast( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].aiattacklast = VALIDATE_CHR( ChrList[ichr].aiattacklast );
  return ChrList[ichr].aiattacklast;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aibumplast( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].aibumplast = VALIDATE_CHR( ChrList[ichr].aibumplast );
  return ChrList[ichr].aibumplast;
};

//--------------------------------------------------------------------------------------------
CHR_REF chr_get_aihitlast( CHR_REF ichr )
{
  if ( !VALID_CHR( ichr ) ) return MAXCHR;

  ChrList[ichr].aihitlast = VALIDATE_CHR( ChrList[ichr].aihitlast );
  return ChrList[ichr].aihitlast;
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

  pcv->x_min = ChrList[ichr].pos.x - ChrList[ichr].bmpdata.size * ChrList[ichr].scale * ChrList[ichr].pancakepos.x;
  pcv->y_min = ChrList[ichr].pos.y - ChrList[ichr].bmpdata.size * ChrList[ichr].scale * ChrList[ichr].pancakepos.y;
  pcv->z_min = ChrList[ichr].pos.z;

  pcv->x_max = ChrList[ichr].pos.x + ChrList[ichr].bmpdata.size * ChrList[ichr].scale * ChrList[ichr].pancakepos.x;
  pcv->y_max = ChrList[ichr].pos.y + ChrList[ichr].bmpdata.size * ChrList[ichr].scale * ChrList[ichr].pancakepos.y;
  pcv->z_max = ChrList[ichr].pos.z + ChrList[ichr].bmpdata.height * ChrList[ichr].scale * ChrList[ichr].pancakepos.z;

  pcv->xy_min = -(ChrList[ichr].pos.x + ChrList[ichr].pos.y) - ChrList[ichr].bmpdata.sizebig * ChrList[ichr].scale * (ChrList[ichr].pancakepos.x + ChrList[ichr].pancakepos.y) * 0.5f;
  pcv->xy_max = -(ChrList[ichr].pos.x + ChrList[ichr].pos.y) + ChrList[ichr].bmpdata.sizebig * ChrList[ichr].scale * (ChrList[ichr].pancakepos.x + ChrList[ichr].pancakepos.y) * 0.5f;

  pcv->yx_min = -(-ChrList[ichr].pos.x + ChrList[ichr].pos.y) - ChrList[ichr].bmpdata.sizebig * ChrList[ichr].scale * (ChrList[ichr].pancakepos.x + ChrList[ichr].pancakepos.y) * 0.5f;
  pcv->yx_max = -(-ChrList[ichr].pos.x + ChrList[ichr].pos.y) + ChrList[ichr].bmpdata.sizebig * ChrList[ichr].scale * (ChrList[ichr].pancakepos.x + ChrList[ichr].pancakepos.y) * 0.5f;

  pcv->level = -1;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_bdata_reinit(CHR_REF ichr, BData * pbd)
{
  if(!VALID_CHR(ichr) || NULL == pbd) return bfalse;

  pbd->calc_is_platform   = ChrList[ichr].isplatform;
  pbd->calc_is_mount      = ChrList[ichr].ismount;

  pbd->mids_lo = pbd->mids_hi = ChrList[ichr].pos;
  pbd->mids_hi.z += pbd->height * 0.5f;

  pbd->calc_size     = pbd->size    * ChrList[ichr].scale * (ChrList[ichr].pancakepos.x + ChrList[ichr].pancakepos.y) * 0.5f;
  pbd->calc_size_big = pbd->sizebig * ChrList[ichr].scale * (ChrList[ichr].pancakepos.x + ChrList[ichr].pancakepos.y) * 0.5f;

  chr_cvolume_reinit(ichr, &pbd->cv);

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t md2_calculate_bumpers_0( CHR_REF ichr )
{
  bool_t rv = bfalse;

  if( !VALID_CHR(ichr) ) return bfalse;

  ChrList[ichr].bmpdata.cv_tree = NULL;

  rv = chr_bdata_reinit( ichr, &(ChrList[ichr].bmpdata) );

  ChrList[ichr].bmpdata.cv.level = 0;

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
//  bd = &(ChrList[ichr].bmpdata);
//
//  imdl = ChrList[ichr].model;
//  if(!VALID_MDL(imdl) || !ChrList[ichr].matrixvalid )
//  {
//    set_default_bump_data( ichr );
//    return bfalse;
//  };
//
//  pmdl = MadList[imdl].md2_ptr;
//  if(NULL == pmdl)
//  {
//    set_default_bump_data( ichr );
//    return bfalse;
//  }
//
//  fl = md2_get_Frame(pmdl, ChrList[ichr].anim.last);
//  fc = md2_get_Frame(pmdl, ChrList[ichr].anim.next );
//  lerp = ChrList[ichr].anim.flip;
//
//  if(NULL==fl && NULL==fc)
//  {
//    set_default_bump_data( ichr );
//    return bfalse;
//  };
//
//  xdir.x = (ChrList[ichr].matrix)_CNV(0,0);
//  xdir.y = (ChrList[ichr].matrix)_CNV(0,1);
//  xdir.z = (ChrList[ichr].matrix)_CNV(0,2);
//
//  ydir.x = (ChrList[ichr].matrix)_CNV(1,0);
//  ydir.y = (ChrList[ichr].matrix)_CNV(1,1);
//  ydir.z = (ChrList[ichr].matrix)_CNV(1,2);
//
//  zdir.x = (ChrList[ichr].matrix)_CNV(2,0);
//  zdir.y = (ChrList[ichr].matrix)_CNV(2,1);
//  zdir.z = (ChrList[ichr].matrix)_CNV(2,2);
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
//  bd->cv.x_min  = cv.x_min  * 4.125 + ChrList[ichr].pos.x;
//  bd->cv.y_min  = cv.y_min  * 4.125 + ChrList[ichr].pos.y;
//  bd->cv.z_min  = cv.z_min  * 4.125 + ChrList[ichr].pos.z;
//  bd->cv.xy_min = cv.xy_min * 4.125 + ( ChrList[ichr].pos.x + ChrList[ichr].pos.y);
//  bd->cv.yx_min = cv.yx_min * 4.125 + (-ChrList[ichr].pos.x + ChrList[ichr].pos.y);
//
//
//  bd->cv.x_max  = cv.x_max  * 4.125 + ChrList[ichr].pos.x;
//  bd->cv.y_max  = cv.y_max  * 4.125 + ChrList[ichr].pos.y;
//  bd->cv.z_max  = cv.z_max  * 4.125 + ChrList[ichr].pos.z;
//  bd->cv.xy_max = cv.xy_max * 4.125 + ( ChrList[ichr].pos.x + ChrList[ichr].pos.y);
//  bd->cv.yx_max = cv.yx_max * 4.125 + (-ChrList[ichr].pos.x + ChrList[ichr].pos.y);
//
//  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f * 4.125 + ChrList[ichr].pos.x;
//  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f * 4.125 + ChrList[ichr].pos.y;
//  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f * 4.125 + ChrList[ichr].pos.z;
//
//  bd->mids_lo   = bd->mids_hi;
//  bd->mids_lo.z = cv.z_min * 4.125  + ChrList[ichr].pos.z;
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
  bd = &(ChrList[ichr].bmpdata);

  imdl = ChrList[ichr].model;
  if(!VALID_MDL(imdl) || !ChrList[ichr].matrixvalid )
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  };

  xdir.x = (ChrList[ichr].matrix)_CNV(0,0);
  xdir.y = (ChrList[ichr].matrix)_CNV(0,1);
  xdir.z = (ChrList[ichr].matrix)_CNV(0,2);

  ydir.x = (ChrList[ichr].matrix)_CNV(1,0);
  ydir.y = (ChrList[ichr].matrix)_CNV(1,1);
  ydir.z = (ChrList[ichr].matrix)_CNV(1,2);

  zdir.x = (ChrList[ichr].matrix)_CNV(2,0);
  zdir.y = (ChrList[ichr].matrix)_CNV(2,1);
  zdir.z = (ChrList[ichr].matrix)_CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = MadList[imdl].md2_ptr;
  if(NULL == pmdl)
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  }

  fl = md2_get_Frame(pmdl, ChrList[ichr].anim.last);
  fc = md2_get_Frame(pmdl, ChrList[ichr].anim.next );
  lerp = ChrList[ichr].anim.flip;

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

  bd->mids_hi.x = (bd->cv.x_min + bd->cv.x_max) * 0.5f + ChrList[ichr].pos.x;
  bd->mids_hi.y = (bd->cv.y_min + bd->cv.y_max) * 0.5f + ChrList[ichr].pos.y;
  bd->mids_hi.z = (bd->cv.z_min + bd->cv.z_max) * 0.5f + ChrList[ichr].pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = bd->cv.z_min + ChrList[ichr].pos.z;

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
  Uint32 cnt;
  Uint32  vrt_count;
  bool_t  free_array = bfalse;

  CVolume cv;

  if( !VALID_CHR(ichr) ) return bfalse;
  bd = &(ChrList[ichr].bmpdata);

  imdl = ChrList[ichr].model;
  if(!VALID_MDL(imdl) || !ChrList[ichr].matrixvalid )
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  };

  xdir.x = (ChrList[ichr].matrix)_CNV(0,0);
  xdir.y = (ChrList[ichr].matrix)_CNV(0,1);
  xdir.z = (ChrList[ichr].matrix)_CNV(0,2);

  ydir.x = (ChrList[ichr].matrix)_CNV(1,0);
  ydir.y = (ChrList[ichr].matrix)_CNV(1,1);
  ydir.z = (ChrList[ichr].matrix)_CNV(1,2);

  zdir.x = (ChrList[ichr].matrix)_CNV(2,0);
  zdir.y = (ChrList[ichr].matrix)_CNV(2,1);
  zdir.z = (ChrList[ichr].matrix)_CNV(2,2);

  bd->calc_is_platform  = bd->calc_is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  bd->calc_is_mount     = bd->calc_is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);

  pmdl = MadList[imdl].md2_ptr;
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
  Transform3(&(ChrList[ichr].matrix), ChrList[ichr].vdata.Vertices, vrt_ary, vrt_count);

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

  bd->mids_lo.x = (cv.x_min + cv.x_max) * 0.5f + ChrList[ichr].pos.x;
  bd->mids_lo.y = (cv.y_min + cv.y_max) * 0.5f + ChrList[ichr].pos.y;
  bd->mids_hi.z = (cv.z_min + cv.z_max) * 0.5f + ChrList[ichr].pos.z;

  bd->mids_lo   = bd->mids_hi;
  bd->mids_lo.z = cv.z_min + ChrList[ichr].pos.z;

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
  Uint32 cnt, tnc;
  Uint32  tri_count, vrt_count;
  vect3 * vrt_ary;
  CVolume *pcv, cv_node[8];

  if( !VALID_CHR(ichr) ) return bfalse;
  bd = &(ChrList[ichr].bmpdata);

  imdl = ChrList[ichr].model;
  if(!VALID_MDL(imdl) || !ChrList[ichr].matrixvalid )
  {
    md2_calculate_bumpers_0( ichr );
    return bfalse;
  };

  pmdl = MadList[imdl].md2_ptr;
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
  Transform3(&(ChrList[ichr].matrix), ChrList[ichr].vdata.Vertices, vrt_ary, vrt_count);

  pcv = &(ChrList[ichr].bmpdata.cv);

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

  if(ChrList[ichr].bmpdata.cv.level >= level) return btrue;

  switch(level)
  {
    case 2:
      // the collision volume is an octagon, the ranges are calculated using the model's vertices
      retval = md2_calculate_bumpers_2(ichr, NULL);
      break;

    case 3:
      {
        // calculate the octree collision volume
        if(NULL == ChrList[ichr].bmpdata.cv_tree)
        {
          ChrList[ichr].bmpdata.cv_tree = calloc(1, sizeof(CVolume_Tree));
        };
        retval = md2_calculate_bumpers_3(ichr, ChrList[ichr].bmpdata.cv_tree);
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
  Uint32 cnt;

  for(cnt=0; cnt<cv_list_count; cnt++)
  {
    draw_CVolume( &(cv_list[cnt]) );
  };
}