#include "egoboo_utility.h"
#include "Mad.h"
#include "script.h"
#include "Log.h"

MAD MadList[MAXMODEL];

float  spek_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
float  spek_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
float  indextoenvirox[MD2LIGHTINDICES];                    // Environment map
float  lighttoenviroy[256];                                // Environment map
Uint32 lighttospek[MAXSPEKLEVEL][256];                     //

int     globalnumicon;                              // Number of icons
Uint16  madloadframe;                               // Where to load next

Uint16  skintoicon[MAXTEXTURE];                  // Skin to icon

//--------------------------------------------------------------------------------------------
ACTION action_number(char * szName)
{
  // ZZ> This function returns the number of the action in cFrameName, or
  //     it returns ACTION_INVALID if it could not find a match
  ACTION cnt;

  if(NULL==szName || '\0' == szName[0] || '\0' == szName[1]) return ACTION_INVALID;

  for ( cnt = 0; cnt < MAXACTION; cnt++ )
  {
    if ( 0 == strncmp(szName, cActionName[cnt], 2) ) return cnt;
  }

  return ACTION_INVALID;
}

//--------------------------------------------------------------------------------------------
Uint16 action_frame()
{
  // ZZ> This function returns the frame number in the third and fourth characters
  //     of cFrameName

  int number;
  sscanf( &cFrameName[2], "%d", &number );
  return number;
}

//--------------------------------------------------------------------------------------------
bool_t test_frame_name( char letter )
{
  // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
  //     of the frame name matches the input argument
  if ( cFrameName[4] == letter ) return btrue;
  if ( cFrameName[4] == 0 ) return bfalse;
  if ( cFrameName[5] == letter ) return btrue;
  if ( cFrameName[5] == 0 ) return bfalse;
  if ( cFrameName[6] == letter ) return btrue;
  if ( cFrameName[6] == 0 ) return bfalse;
  if ( cFrameName[7] == letter ) return btrue;
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct( Uint16 object, ACTION actiona, ACTION actionb )
{
  // ZZ> This function makes sure both actions are valid if either of them
  //     are valid.  It will copy start and ends to mirror the valid action.
  if ( MadList[object].actionvalid[actiona] == MadList[object].actionvalid[actionb] )
  {
    // They are either both valid or both invalid, in either case we can't help
  }
  else
  {
    // Fix the invalid one
    if ( !MadList[object].actionvalid[actiona] )
    {
      // Fix actiona
      MadList[object].actionvalid[actiona] = btrue;
      MadList[object].actionstart[actiona] = MadList[object].actionstart[actionb];
      MadList[object].actionend[actiona] = MadList[object].actionend[actionb];
    }
    else
    {
      // Fix actionb
      MadList[object].actionvalid[actionb] = btrue;
      MadList[object].actionstart[actionb] = MadList[object].actionstart[actiona];
      MadList[object].actionend[actionb] = MadList[object].actionend[actiona];
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_walk_frame( Uint16 object, LIPT lip_trans, ACTION action )
{
  // ZZ> This helps make walking look right
  int frame = 0;
  int framesinaction = MadList[object].actionend[action] - MadList[object].actionstart[action];

  while ( frame < 16 )
  {
    int framealong = 0;
    if ( framesinaction > 0 )
    {
      framealong = (( float )( frame * framesinaction ) / ( float ) MAXFRAMESPERANIM ) + 2;
      framealong %= framesinaction;
    }
    MadList[object].frameliptowalkframe[lip_trans][frame] = MadList[object].actionstart[action] + framealong;
    frame++;
  }
}

//--------------------------------------------------------------------------------------------
Uint16 get_framefx( char * szName )
{
  // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
  //     Drop timings
  Uint16 fx = 0;

  if(NULL == szName || '\0' == szName[0] || '\0' == szName[1] ) return 0;

  if ( test_frame_name( 'I' ) )
    fx |= MADFX_INVICTUS;
  if ( test_frame_name( 'L' ) )
  {
    if ( test_frame_name( 'A' ) )
      fx |= MADFX_ACTLEFT;
    if ( test_frame_name( 'G' ) )
      fx |= MADFX_GRABLEFT;
    if ( test_frame_name( 'D' ) )
      fx |= MADFX_DROPLEFT;
    if ( test_frame_name( 'C' ) )
      fx |= MADFX_CHARLEFT;
  }
  if ( test_frame_name( 'R' ) )
  {
    if ( test_frame_name( 'A' ) )
      fx |= MADFX_ACTRIGHT;
    if ( test_frame_name( 'G' ) )
      fx |= MADFX_GRABRIGHT;
    if ( test_frame_name( 'D' ) )
      fx |= MADFX_DROPRIGHT;
    if ( test_frame_name( 'C' ) )
      fx |= MADFX_CHARRIGHT;
  }
  if ( test_frame_name( 'S' ) )
    fx |= MADFX_STOP;
  if ( test_frame_name( 'F' ) )
    fx |= MADFX_FOOTFALL;
  if ( test_frame_name( 'P' ) )
    fx |= MADFX_POOF;

  return fx;
}

//--------------------------------------------------------------------------------------------
void make_framelip( Uint16 imdl, ACTION action )
{
  // ZZ> This helps make walking look right
  int frame, framesinaction;

  if ( MadList[imdl].actionvalid[action] )
  {
    framesinaction = MadList[imdl].actionend[action] - MadList[imdl].actionstart[action];
    frame = MadList[imdl].actionstart[action];
    while ( frame < MadList[imdl].actionend[action] )
    {
      MadList[imdl].framelip[frame] = ( frame - MadList[imdl].actionstart[action] ) * 15 / framesinaction;
      MadList[imdl].framelip[frame] = ( MadList[imdl].framelip[frame] ) % 16;
      frame++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_actions( Uint16 imdl )
{
  // ZZ> This function creates the iframe lists for each action based on the
  //     name of each md2 iframe in the model
  ACTION      action, lastaction;
  MD2_Model * pmd2;
  MAD       * pmad;

  int    iframe, framesinaction;
  int    iFrameCount;
  char * szName;

  if( imdl>=MAXMODEL || !MadList[imdl].used ) return;
  pmad = MadList + imdl;

  pmd2 = pmad->md2_ptr;
  if(NULL == pmd2) return;

  // Clear out all actions and reset to invalid
  action = 0;
  while ( action < MAXACTION )
  {
    pmad->actionvalid[action] = bfalse;
    action++;
  }

  iFrameCount = md2_get_numFrames(pmd2);
  if(0 == iFrameCount) return;

  // Set the primary dance action to be the first iframe, just as a default
  pmad->actionvalid[ACTION_DA] = btrue;
  pmad->actionstart[ACTION_DA] = 0;
  pmad->actionend[ACTION_DA]   = 1;


  // Now go huntin' to see what each iframe is, look for runs of same action
  szName = md2_get_Frame(pmd2, 0)->name;
  lastaction = action_number(szName);
  framesinaction = 0;
  for ( iframe = 0; iframe < iFrameCount; iframe++ )
  {
    MD2_Frame * pFrame = md2_get_Frame(pmd2, iframe);
    szName = pFrame->name;
    action = action_number(szName);
    if ( lastaction == action )
    {
      framesinaction++;
    }
    else
    {
      // Write the old action
      if ( lastaction < MAXACTION )
      {
        pmad->actionvalid[lastaction] = btrue;
        pmad->actionstart[lastaction] = iframe - framesinaction;
        pmad->actionend[lastaction]   = iframe;
      }
      framesinaction = 1;
      lastaction = action;
    }
    pmad->framefx[iframe] = get_framefx( szName );
  }

  // Write the old action
  if ( lastaction < MAXACTION )
  {
    pmad->actionvalid[lastaction] = btrue;
    pmad->actionstart[lastaction] = iframe - framesinaction;
    pmad->actionend[lastaction]   = iframe;
  }

  // Make sure actions are made valid if a similar one exists
  action_copy_correct( imdl, ACTION_DA, ACTION_DB );   // All dances should be safe
  action_copy_correct( imdl, ACTION_DB, ACTION_DC );
  action_copy_correct( imdl, ACTION_DC, ACTION_DD );
  action_copy_correct( imdl, ACTION_DB, ACTION_DC );
  action_copy_correct( imdl, ACTION_DA, ACTION_DB );
  action_copy_correct( imdl, ACTION_UA, ACTION_UB );
  action_copy_correct( imdl, ACTION_UB, ACTION_UC );
  action_copy_correct( imdl, ACTION_UC, ACTION_UD );
  action_copy_correct( imdl, ACTION_TA, ACTION_TB );
  action_copy_correct( imdl, ACTION_TC, ACTION_TD );
  action_copy_correct( imdl, ACTION_CA, ACTION_CB );
  action_copy_correct( imdl, ACTION_CC, ACTION_CD );
  action_copy_correct( imdl, ACTION_SA, ACTION_SB );
  action_copy_correct( imdl, ACTION_SC, ACTION_SD );
  action_copy_correct( imdl, ACTION_BA, ACTION_BB );
  action_copy_correct( imdl, ACTION_BC, ACTION_BD );
  action_copy_correct( imdl, ACTION_LA, ACTION_LB );
  action_copy_correct( imdl, ACTION_LC, ACTION_LD );
  action_copy_correct( imdl, ACTION_XA, ACTION_XB );
  action_copy_correct( imdl, ACTION_XC, ACTION_XD );
  action_copy_correct( imdl, ACTION_FA, ACTION_FB );
  action_copy_correct( imdl, ACTION_FC, ACTION_FD );
  action_copy_correct( imdl, ACTION_PA, ACTION_PB );
  action_copy_correct( imdl, ACTION_PC, ACTION_PD );
  action_copy_correct( imdl, ACTION_ZA, ACTION_ZB );
  action_copy_correct( imdl, ACTION_ZC, ACTION_ZD );
  action_copy_correct( imdl, ACTION_WA, ACTION_WB );
  action_copy_correct( imdl, ACTION_WB, ACTION_WC );
  action_copy_correct( imdl, ACTION_WC, ACTION_WD );
  action_copy_correct( imdl, ACTION_DA, ACTION_WD );   // All walks should be safe
  action_copy_correct( imdl, ACTION_WC, ACTION_WD );
  action_copy_correct( imdl, ACTION_WB, ACTION_WC );
  action_copy_correct( imdl, ACTION_WA, ACTION_WB );
  action_copy_correct( imdl, ACTION_JA, ACTION_JB );
  action_copy_correct( imdl, ACTION_JB, ACTION_JC );
  action_copy_correct( imdl, ACTION_DA, ACTION_JC );  // All jumps should be safe
  action_copy_correct( imdl, ACTION_JB, ACTION_JC );
  action_copy_correct( imdl, ACTION_JA, ACTION_JB );
  action_copy_correct( imdl, ACTION_HA, ACTION_HB );
  action_copy_correct( imdl, ACTION_HB, ACTION_HC );
  action_copy_correct( imdl, ACTION_HC, ACTION_HD );
  action_copy_correct( imdl, ACTION_HB, ACTION_HC );
  action_copy_correct( imdl, ACTION_HA, ACTION_HB );
  action_copy_correct( imdl, ACTION_KA, ACTION_KB );
  action_copy_correct( imdl, ACTION_KB, ACTION_KC );
  action_copy_correct( imdl, ACTION_KC, ACTION_KD );
  action_copy_correct( imdl, ACTION_KB, ACTION_KC );
  action_copy_correct( imdl, ACTION_KA, ACTION_KB );
  action_copy_correct( imdl, ACTION_MH, ACTION_MI );
  action_copy_correct( imdl, ACTION_DA, ACTION_MM );
  action_copy_correct( imdl, ACTION_MM, ACTION_MN );


  // Create table for doing transition from one type of walk to another...
  // Clear 'em all to start
  for ( iframe = 0; iframe < iFrameCount; iframe++ )
  {
    pmad->framelip[iframe] = 0;
  }

  // Need to figure out how far into action each iframe is
  make_framelip( imdl, ACTION_WA );
  make_framelip( imdl, ACTION_WB );
  make_framelip( imdl, ACTION_WC );

  // Now do the same, in reverse, for walking animations
  get_walk_frame( imdl, LIPT_DA, ACTION_DA );
  get_walk_frame( imdl, LIPT_WA, ACTION_WA );
  get_walk_frame( imdl, LIPT_WB, ACTION_WB );
  get_walk_frame( imdl, LIPT_WC, ACTION_WC );
}

//--------------------------------------------------------------------------------------------
void make_mad_equally_lit( Uint16 model )
{
  // ZZ> This function makes ultra low poly models look better
  int frame, vert;
  int iFrames, iVerts;
  MD2_Model * m;

  if(model > MAXMODEL || !MadList[model].used) return;

  m = MadList[model].md2_ptr;
  if(NULL == m) return;

  iFrames = md2_get_numFrames(m);
  iVerts  = md2_get_numVertices(m);

  for ( frame = 0; frame < iFrames; frame++ )
  {
    MD2_Frame * f = md2_get_Frame(m, frame);
    for ( vert = 0; vert < iVerts; vert++ )
    {
      f->vertices[vert].normal = EQUALLIGHTINDEX;
    }
  }

}

//--------------------------------------------------------------------------------------------
void check_copy( char* loadname, Uint16 object )
{
  // ZZ> This function copies a model's actions
  FILE *fileread;
  ACTION actiona, actionb;
  char szOne[16], szTwo[16];


  MadList[object].msg_start = 0;
  fileread = fs_fileOpen( PRI_NONE, NULL, loadname, "r" );
  if ( NULL != fileread )
  {
    while ( fget_next_string( fileread, szOne, sizeof( szOne ) ) )
    {
      actiona = what_action( szOne[0] );

      fget_string( fileread, szTwo, sizeof( szTwo ) );
      actionb = what_action( szTwo[0] );

      action_copy_correct( object, actiona, actionb );
      action_copy_correct( object, actiona + 1, actionb + 1 );
      action_copy_correct( object, actiona + 2, actionb + 2 );
      action_copy_correct( object, actiona + 3, actionb + 3 );
    }
    fs_fileClose( fileread );
  }
}

//--------------------------------------------------------------------------------------------
bool_t bbox_list_contract(BBOX_LIST *pnew)
{
  // BB > make the list as small as possible by removing dead nodes

  int i, j;

  // check for invalid lists
  if(NULL == pnew) return bfalse;

  // if there are no nodes, there is nothing to do
  if(0 == pnew->count) return btrue;

  for(i=0, j=0; i<pnew->count; i++)
  {
    if(pnew->list[i].mins.x == pnew->list[i].maxs.x) continue;

    if(i!=j)
    {
      memcpy(pnew->list + j, pnew->list + i, sizeof(AA_BBOX));
    }

    j++;
  }

  if(j < pnew->count)
  {
    bbox_list_realloc(pnew, j);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
// the most complete way of doing this would be to find the nodes of an octree
// at the given level that contain triangles.
// HOWEVER, the math on that is kicking my @$$, so I will do something simpler
bool_t mad_generate_bbox_list(BBOX_LIST *plist, MD2_Model * pmd2, int frame, int bbox_divisions)
{
  int ix, iy, iz;
  int i, j;
  int num_per_axis, bbox_alloc_count, bbox_count;

  AA_BBOX tmp_bb;

  int            vrt_count;
  MD2_Frame    * pframe;

  int            tri_count;
  md2_triangle * ptri_lst;

  // erase any existing data in the BBOX_LIST
  if(NULL == bbox_list_renew(plist)) return bfalse;

  // make sure we have valid values
  if(NULL == pmd2) return bfalse;

  // check the triangle list
  if(0 == pmd2->m_numTriangles || NULL == pmd2->m_triangles) return bfalse;
  tri_count = pmd2->m_numTriangles;
  ptri_lst  = pmd2->m_triangles;

  // check for valid frame
  if(0 == pmd2->m_numFrames || NULL == pmd2->m_frames || frame > pmd2->m_numFrames) return bfalse;
  pframe    = pmd2->m_frames + frame;
  vrt_count = pmd2->m_numVertices;

  // allocate a bbox list that has the required number of bboxes
  num_per_axis     = 1 << (  bbox_divisions);
  bbox_alloc_count = 1 << (3*bbox_divisions);
  bbox_list_alloc( plist, bbox_alloc_count );

  // initialize the data with "invalid" bboxes (i.e. they have zero volume)
  for(i=0; i<bbox_alloc_count; i++)
  {
    ix = (i >> (0*bbox_divisions)) & (num_per_axis-1);
    iy = (i >> (1*bbox_divisions)) & (num_per_axis-1);
    iz = (i >> (2*bbox_divisions)) & (num_per_axis-1);

    plist->list[i].mins.x = plist->list[i].maxs.x =  ix / (float)num_per_axis;
    plist->list[i].maxs.y = plist->list[i].mins.y =  iy / (float)num_per_axis;
    plist->list[i].maxs.z = plist->list[i].mins.z =  iz / (float)num_per_axis;
  }

  // for each triangle in the tri_list, calculate its bbox and,
  // generate a bbox in the plist for every "octree node" that it overlaps
  bbox_count = 0;
  for(i=0; i<tri_count; i++)
  {
    int ix_min,ix_max, iy_min,iy_max, iz_min,iz_max;

    // test the three vertices to make sure they are valid
    for(j=0;j<3;j++)
    {
      int ivrt = ptri_lst[i].vertexIndices[j];
      if(ivrt>vrt_count) break;

      if(0 == j)
      {
        tmp_bb.mins.x = tmp_bb.maxs.x = pframe->vertices[ivrt].x;
        tmp_bb.mins.y = tmp_bb.maxs.y = pframe->vertices[ivrt].y;
        tmp_bb.mins.z = tmp_bb.maxs.z = pframe->vertices[ivrt].z;
      }
      else
      {
        float ftmp;

        ftmp = pframe->vertices[ivrt].x;
        if(ftmp<tmp_bb.mins.x) tmp_bb.mins.x = ftmp;
        else if (ftmp>tmp_bb.maxs.x) tmp_bb.maxs.x = ftmp;

        ftmp = pframe->vertices[ivrt].y;
        if(ftmp<tmp_bb.mins.y) tmp_bb.mins.y = ftmp;
        else if (ftmp>tmp_bb.maxs.y) tmp_bb.maxs.y = ftmp;

        ftmp = pframe->vertices[ivrt].z;
        if(ftmp<tmp_bb.mins.z) tmp_bb.mins.z = ftmp;
        else if (ftmp>tmp_bb.maxs.z) tmp_bb.maxs.z = ftmp;
      }

    };

    // if a vertex is invalid, skip the triangle
    if(j<3) continue;


    // calculate the integer limits of the triangle's bbox
    ix_min = (tmp_bb.mins.x - pframe->bbmin[0]) / (pframe->bbmax[0]-pframe->bbmin[0]) * num_per_axis;
    ix_max = (tmp_bb.maxs.x - pframe->bbmin[0]) / (pframe->bbmax[0]-pframe->bbmin[0]) * num_per_axis;

    iy_min = (tmp_bb.mins.y - pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
    iy_max = (tmp_bb.maxs.y - pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;

    iz_min = (tmp_bb.mins.z - pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
    iz_max = (tmp_bb.maxs.z - pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;

    // insert valid nodes into the data
    for(ix=ix_min; ix<=ix_max; ix++)
    {
      for(iy=iy_min; iy<=iy_max; iy++)
      {
        for(iz=iz_min; iz<=iz_max; iz++)
        {

          // calculate the index from the integer positions
          int idx = ((ix & (num_per_axis-1)) << (0*bbox_divisions)) |
                    ((iy & (num_per_axis-1)) << (1*bbox_divisions)) |
                    ((iz & (num_per_axis-1)) << (2*bbox_divisions)) ;

          if( (plist->list[idx].mins.x == plist->list[idx].maxs.x) )
          {

            plist->list[idx].mins.x =  (ix + 0) / (float)num_per_axis;
            plist->list[idx].maxs.x =  (ix + 1) / (float)num_per_axis;

            plist->list[idx].mins.y =  (iy + 0) / (float)num_per_axis;
            plist->list[idx].maxs.y =  (iy + 1) / (float)num_per_axis;

            plist->list[idx].mins.z =  (iz + 0) / (float)num_per_axis;
            plist->list[idx].maxs.z =  (iz + 1) / (float)num_per_axis;

            bbox_count++;
          }
        }
      }
    }
  }

  if(bbox_count == bbox_alloc_count)
  {
    // this bbox is completely full. It doesn't help us at all
    bbox_list_delete(plist);
    return bfalse;
  }
  else if(bbox_count > bbox_alloc_count * 0.75f)
  {
    // this bbox is not efficient enough. It doesn't help us at all
    bbox_list_delete(plist);
    return bfalse;
  }
  else
  {
    return bbox_list_contract(plist);
  };
};

//--------------------------------------------------------------------------------------------
// need to find some way to collect the data into larger sized blocks
bool_t mad_simplify_bbox_list(int new_divisions, BBOX_LIST *pnew, BBOX_LIST *pold)
{
  int ix,ix_min,ix_max, iy,iy_min,iy_max, iz,iz_min,iz_max;
  int i;
  int bbox_count;
  int new_per_axis, new_alloc_count, old_divisions, old_per_axis;

  // erase any existing data in the BBOX_LIST
  if(NULL == bbox_list_renew(pnew)) return bfalse;

  // check for valid lists
  if(NULL == pold) return bfalse;

  if(0 == new_divisions)
  {

    if(0 != pold->count)
    {
      bbox_list_alloc( pnew, 1 );

      pnew->list[0].mins.x =  0;
      pnew->list[0].maxs.x =  1;
      pnew->list[0].mins.y =  0;
      pnew->list[0].maxs.y =  1;
      pnew->list[0].mins.z =  0;
      pnew->list[0].maxs.z =  1;
    }

    return btrue;
  }

  // coalece the old data
  old_divisions = new_divisions + 1;
  old_per_axis = 1 << old_divisions;

  // allocate a bbox list that has the required number of bboxes
  new_per_axis    = 1 << (    new_divisions);
  new_alloc_count = 1 << (3 * new_divisions);
  bbox_list_alloc( pnew, new_alloc_count );

  // insert valid nodes into the data
  bbox_count = 0;
  for(i=0; i<pold->count; i++)
  {
    AA_BBOX * pbb_old = pold->list + i;
    int idx_new;

    if (pbb_old->mins.x == pbb_old->maxs.x) continue;

    ix_min = pbb_old->mins.x * old_per_axis;
    ix_max = pbb_old->maxs.x * old_per_axis - 0.125f/old_per_axis;
    ix_min >>= 1;
    ix_max >>= 1;

    iy_min = pbb_old->mins.y * old_per_axis;
    iy_max = pbb_old->maxs.y * old_per_axis - 0.125f/old_per_axis/2;
    iy_min >>= 1;
    iy_min >>= 1;

    iz_min = pbb_old->mins.z * old_per_axis;
    iz_max = pbb_old->maxs.z * old_per_axis - 0.125f/old_per_axis/2;
    iz_min >>= 1;
    iz_max >>= 1;

    for(ix=ix_min; ix<=ix_max; ix++)
    {
      for(iy=iy_min; iy<=iy_max; iy++)
      {
        for(iz=iz_min; iz<=iz_max; iz++)
        {
          idx_new = ((ix & (new_per_axis-1)) << (0*new_divisions)) |
                    ((iy & (new_per_axis-1)) << (1*new_divisions)) |
                    ((iz & (new_per_axis-1)) << (2*new_divisions));

          if(pnew->list[idx_new].mins.x == pnew->list[idx_new].maxs.x)
          {

            pnew->list[idx_new].mins.x =  (ix + 0) / (float)new_per_axis;
            pnew->list[idx_new].maxs.x =  (ix + 1) / (float)new_per_axis;
            pnew->list[idx_new].mins.y =  (iy + 0) / (float)new_per_axis;
            pnew->list[idx_new].maxs.y =  (iy + 1) / (float)new_per_axis;
            pnew->list[idx_new].mins.z =  (iz + 0) / (float)new_per_axis;
            pnew->list[idx_new].maxs.z =  (iz + 1) / (float)new_per_axis;

            bbox_count++;
          }
        }
      }
    }
  };

  if(bbox_count == new_alloc_count)
  {
    // this bbox is completely full. It doesn't help us at all
    bbox_list_delete(pnew);
    return bfalse;
  }
  if(bbox_count > new_alloc_count * 0.75f)
  {
    // this bbox is not efficient enough. there's not enough reason to keep it around
    bbox_list_delete(pnew);
    return bfalse;
  }
  else
  {
    return bbox_list_contract(pnew);
  };
}


//--------------------------------------------------------------------------------------------
bool_t mad_delete_bbox_tree(MAD * pmad)
{
  int i;

  if(NULL == pmad) return bfalse;

  if(0 == pmad->bbox_frames || NULL == pmad->bbox_arrays) return btrue;

  for(i=0; i<pmad->bbox_frames; i++)
  {
    bbox_ary_delete(pmad->bbox_arrays + i);
  }

  free(pmad->bbox_arrays);
  pmad->bbox_arrays = NULL;

  pmad->bbox_frames = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mad_generate_bbox_tree(int max_level, MAD * pmad)
{
  int i,j;

  if(NULL == pmad) return bfalse;

  if(NULL == pmad->md2_ptr) return bfalse;

  // initialize the bbox_tree
  mad_delete_bbox_tree(pmad);

  // if there are no triangles, we are done
  if(0 == pmad->md2_ptr->m_numTriangles) return btrue;

  // read the frame count and allocate one BBOX_ARY per frame
  pmad->bbox_frames = pmad->md2_ptr->m_numFrames;
  pmad->bbox_arrays = calloc(pmad->bbox_frames, sizeof(BBOX_ARY));

  // go through every frame
  for(i=0; i<pmad->bbox_frames; i++)
  {
    // start with a nice clean bbox_ary
    bbox_ary_new(pmad->bbox_arrays + i);

    // allocate the lists for each level of detail
    bbox_ary_alloc(pmad->bbox_arrays + i, max_level + 1);

    // generate the most detailed bbox collection for this frame
    if(!mad_generate_bbox_list(pmad->bbox_arrays[i].list + max_level, pmad->md2_ptr, i, max_level))
    {
      // there was an error or the bbox list is completely full

      // deallocate everything
      mad_delete_bbox_tree(pmad);

      return bfalse;
    }

    // generate all of the remaining bbox collections by simplifying
    for(j=max_level-1; j>=0; j--)
    {
      if( !mad_simplify_bbox_list(j, pmad->bbox_arrays[i].list + j, pmad->bbox_arrays[i].list + j + 1) )
      {
        // there was an error or the bbox list is completely full
  
        // remove the last bbox list
        bbox_list_delete(pmad->bbox_arrays[i].list + j);

        return btrue;
      };
    };
  };

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BBOX_LIST * bbox_list_new(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;
  memset(lst, 0, sizeof(BBOX_LIST));
  return lst;
}

//--------------------------------------------------------------------------------------------
BBOX_LIST * bbox_list_delete(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;

  if(lst->count && NULL!=lst->list)
  {
    free(lst->list);
  }

  lst->count = 0;
  lst->list  = NULL;

  return lst;
}

//--------------------------------------------------------------------------------------------
BBOX_LIST * bbox_list_renew(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;

  bbox_list_delete(lst);
  return bbox_list_new(lst);
}

//--------------------------------------------------------------------------------------------
BBOX_LIST * bbox_list_alloc(BBOX_LIST * lst, int count)
{
  if(NULL == lst) return NULL;

  bbox_list_delete(lst);

  if(count>0)
  {
    lst->list = calloc(count, sizeof(AA_BBOX));
    if(NULL != lst->list)
    {
      lst->count = count;
    }
  }

  return lst;
}

//--------------------------------------------------------------------------------------------
BBOX_LIST * bbox_list_realloc(BBOX_LIST * lst, int count)
{
  // check for bad list
  if(NULL == lst) return NULL;

  // check for no change in the count
  if(count == lst->count) return lst;

  // check another dumb case
  if(count==0)
  {
    return bbox_list_delete(lst);
  }


  lst->list = realloc(lst->list, count * sizeof(AA_BBOX));
  if(NULL == lst->list)
  {
    lst->count = 0;
  }
  else
  {
    lst->count = count;
  }


  return lst;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BBOX_ARY * bbox_ary_new(BBOX_ARY * ary)
{
  if(NULL == ary) return NULL;
  memset(ary, 0, sizeof(BBOX_ARY));
  return ary;
}

//--------------------------------------------------------------------------------------------
BBOX_ARY * bbox_ary_delete(BBOX_ARY * ary)
{
  int i;

  if(NULL == ary) return NULL;

  if(NULL!=ary->list)
  {
    for(i=0; i<ary->count; i++)
    {
      bbox_list_delete(ary->list + i);
    }

    free(ary->list);
    ary->list = NULL;
  }

  ary->count = 0;
  ary->list = NULL;

  return ary;
}

//--------------------------------------------------------------------------------------------
BBOX_ARY * bbox_ary_renew(BBOX_ARY * ary)
{
  if(NULL == ary) return NULL;
  bbox_ary_delete(ary);
  return bbox_ary_new(ary);
}

//--------------------------------------------------------------------------------------------
BBOX_ARY * bbox_ary_alloc(BBOX_ARY * ary, int count)
{
  if(NULL == ary) return NULL;

  bbox_ary_delete(ary);

  if(count>0)
  {
    ary->list = calloc(count, sizeof(BBOX_LIST));
    if(NULL != ary->list)
    {
      ary->count = count;
    }
  }

  return ary;
}


//--------------------------------------------------------------------------------------------
MAD *  mad_new(MAD * pmad)
{
  if(NULL == pmad) return NULL;

  memset(pmad, 0, sizeof(MAD));

  return pmad;
}

//--------------------------------------------------------------------------------------------
MAD *  mad_delete(MAD * pmad)
{
  if(NULL == pmad) return NULL;

  if(NULL != pmad->md2_ptr)
  {
    md2_delete(pmad->md2_ptr);
    pmad->md2_ptr = NULL;
  }

  if(NULL != pmad->framelip)
  {
    free(pmad->framelip);
    pmad->framelip = NULL;
  };

  if(NULL != pmad->framefx)
  {
    free(pmad->framefx);
    pmad->framefx = NULL;
  };

  mad_delete_bbox_tree(pmad);

  return pmad;
};

//--------------------------------------------------------------------------------------------
MAD *  mad_renew(MAD * pmad)
{
  if(NULL == pmad) return NULL;

  mad_delete(pmad);

  return mad_new(pmad);
};

//---------------------------------------------------------------------------------------------
Uint16 load_one_mad( char * szObjectname, Uint16 imdl )
{
  // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
  //    int iFileHandleRead;

  STRING szLoadname;
  size_t iBytesRead = 0;
  int iFrames;
  MAD * pmad;

  if(!VALID_MDL(imdl)) return MAXMODEL;

  // make the notation "easier"
  pmad = MadList + imdl;

  // Make sure we don't load over an existing model
  if ( pmad->used )
  {
    log_error( "Model slot %i is already used. (%s)\n", imdl, szLoadname );
  }

  // make sure we have a clean MAD. More complicated because of dynamic allocation...
  if(NULL == mad_renew(pmad)) return MAXMODEL;

  // Make up a name for the model...  IMPORT\TEMP0000.OBJ
  strncpy( pmad->name, szObjectname, sizeof( pmad->name ) );

  // Load the AI script for this object
  snprintf( szLoadname, sizeof( szLoadname ), "%s%s", szObjectname, CData.script_file );

  // Create a reference to the one we just loaded
  pmad->ai = load_ai_script( szLoadname );
  if ( MAXAI == pmad->ai )
  {
    // use the default script
    pmad->ai = 0;
  }

  // Load the object model
  make_newloadname( szObjectname, "tris.md2", szLoadname );

#ifdef __unix__
  // unix is case sensitive, but sometimes this file is called tris.MD2
  if ( access( szLoadname, R_OK ) )
  {
    make_newloadname( szObjectname, "tris.MD2", szLoadname );
    // still no luck !
    if ( access( szObjectname, R_OK ) )
    {
      log_error( stderr, "ERROR: cannot open: %s\n", szLoadname );
    }
  }
#endif

  // load the actual md2 data
  pmad->md2_ptr = md2_load( szLoadname, NULL );
  if(NULL == pmad->md2_ptr) return MAXMODEL;

  // generate a bbox structure for each frame of the animation
  // this is dynamically allocated, mad_delete() or mad_renew() must be called
  // to make all of that go away
  mad_generate_bbox_tree(2, pmad);

  // Figure out how many vertices to transform
  pmad->vertices      = md2_get_numVertices( pmad->md2_ptr );
  pmad->transvertices = mad_calc_transvertices( pmad->md2_ptr );

  // allocate the extra animation data
  iFrames = md2_get_numFrames(pmad->md2_ptr);
  pmad->framelip = calloc(sizeof(Uint8),  iFrames);
  pmad->framefx  = calloc(sizeof(Uint16), iFrames);

  // tell everyone that we loaded correctly
  pmad->used = btrue;

  // Create the actions table for this object
  get_actions( imdl );

  // Copy entire actions to save frame space COPY.TXT
  snprintf( szLoadname, sizeof( szLoadname ), "%s%s", szObjectname, CData.copy_file );
  check_copy( szLoadname, imdl );

  return imdl;
}

//---------------------------------------------------------------------------------------------
void free_one_mad( Uint16 imdl )
{
  // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
  //    int iFileHandleRead;

  MAD * pmad;

  if(imdl > MAXMODEL) return;

  pmad = MadList + imdl;

  mad_delete(pmad);
}

////--------------------------------------------------------------------------------------------
//bool_t mad_generate_bbox_level(int bbox_divisions, int frame, MAD * pmad)
//{
//  int            i, bbox_count, vrt_count;
//  int            num_per_axis;
//  BBOX_LIST      bblist;
//  AA_BBOX        tmp_bb;
//
//  MD2_Model    * pmd2;
//
//  int            vrt_count;
//  MD2_Frame    * pframe;
//
//  int            tri_count;
//  md2_triangle * ptri_lst;
//
//  // check for valid MAD
//  if(NULL == pmad) return bfalse;
//
//  // check for valid MD2
//  if(NULL == pmad->md2_ptr) return bfalse;
//  pmd2 = pmad->md2_ptr->m_numFrames;
//
//  // check the triangle list
//  if(0 == pmd2->m_numTriangles || NULL == pmd2->m_triangles) return bfalse;
//  tri_count = pmd2->m_numTriangles;
//  ptri_lst  = pmd2->m_triangles;
//
//  // check for valid frame
//  if(0 == pmd2->m_numFrames || NULL == pmd2->m_frames || frame > pmd2->m_numFrames) return bfalse;
//  pframe    = pmd2->m_frames + frame;
//  vrt_count = pframe->vertices;
//
//  // allocate a bbox list that has the required number of bboxes
//  num_per_axis = 1 << (  bbox_divisions)
//  bbox_count   = 1 << (3*bbox_divisions);
//  bbox_list_alloc( &bblist, bbox_count );
//
//  // handle the special case
//  if(num_per_axis <= 1)
//  {
//    // just copy the pre-calculated values
//    memcpy(bblist.list[0].maxs.v, pframe->bbmax, 3*sizeof(float));
//    memcpy(bblist.list[0].mins.v, pframe->bbin, 3*sizeof(float));
//
//    return btrue;
//  }
//
//  for(i=0; i<bbox_count; i++)
//  {
//    int ix, iy, iz;
//
//    ix = (i >> 0*bbox_divisions) & (num_per_axis-1);
//    iy = (i >> 1*bbox_divisions) & (num_per_axis-1);
//    iz = (i >> 2*bbox_divisions) & (num_per_axis-1);
//
//    bblist.list[i].mins.x =  (ix + 0) / (float)num_per_axis;
//    bblist.list[i].maxs.x =  (ix + 1) / (float)num_per_axis;
//    bblist.list[i].mins.y =  (iy + 0) / (float)num_per_axis;
//    bblist.list[i].maxs.y =  (iy + 1) / (float)num_per_axis;
//    bblist.list[i].mins.z =  (iz + 0) / (float)num_per_axis;
//    bblist.list[i].maxs.z =  (iz + 1) / (float)num_per_axis;
//  }
//
//  // !!!! GAR !!! - I have to roll my own triangle-bbox-array collision code
//  //                it will probably be the most inefficient algorithm possible!
//
//  // now scan the triangle list
//  for(i=0;i<tri_count;i++)
//  {
//    int ix_min,ix_max, iy_min,iy_max, iz_min,iz_max;
//    float dx,dy,dz;
//    float max_comp;
//    vect3 uvec, vvec, ovec, nrm, ovvec;
//
//    // test the vertices to make sure they are valid
//    for(j=0;j<3;j++)
//    {
//      ivrt = ptri_lst[i].vertexIndices[v];
//      if(ivrt>vrt_count) break;
//      if(0 == j)
//      {
//        tmp_bb.mins.x = tmp_bb.maxs.x = pframe->vertices[ptri_lst[i].vertexIndices[j]].x;
//        tmp_bb.mins.y = tmp_bb.maxs.y = pframe->vertices[ptri_lst[i].vertexIndices[j]].y;
//        tmp_bb.mins.z = tmp_bb.maxs.z = pframe->vertices[ptri_lst[i].vertexIndices[j]].z;
//      }
//      else
//      {
//        float ftmp;
//
//        ftmp = pframe->vertices[ptri_lst[i].vertexIndices[j]].x;
//        if(ftmp<tmp_bb.mins.x) tmp_bb.mins.x = ftmp;
//        else if (ftmp>tmp_bb.maxs.x) tmp_bb.maxs.x = ftmp;
//
//        ftmp = pframe->vertices[ptri_lst[i].vertexIndices[j]].y;
//        if(ftmp<tmp_bb.mins.y) tmp_bb.mins.y = ftmp;
//        else if (ftmp>tmp_bb.maxs.y) tmp_bb.maxs.y = ftmp;
//
//        ftmp = pframe->vertices[ptri_lst[i].vertexIndices[j]].z;
//        if(ftmp<tmp_bb.mins.z) tmp_bb.mins.x = ftmp;
//        else if (ftmp>tmp_bb.maxs.z) tmp_bb.maxs.z = ftmp;
//      }
//
//    };
//
//    // if a vertex is invalid, skip the triangle
//    if(j!=2) continue;
//
//    dx = tmp_bb.maxs.x - tmp_bb.mins.x;
//    dy = tmp_bb.maxs.y - tmp_bb.mins.y;
//    dz = tmp_bb.maxs.z - tmp_bb.mins.z;
//
//    ovec.x = pframe->vertices[ptri_lst[i].vertexIndices[0]].x;
//    ovec.y = pframe->vertices[ptri_lst[i].vertexIndices[0]].y;
//    ovec.z = pframe->vertices[ptri_lst[i].vertexIndices[0]].z;
//
//    uvec.x = pframe->vertices[ptri_lst[i].vertexIndices[1]].x - ovec.x;
//    uvec.y = pframe->vertices[ptri_lst[i].vertexIndices[1]].y - ovec.y;
//    uvec.z = pframe->vertices[ptri_lst[i].vertexIndices[1]].z - ovec.z;
//
//    vvec.x = pframe->vertices[ptri_lst[i].vertexIndices[2]].x - ovec.x;
//    vvec.y = pframe->vertices[ptri_lst[i].vertexIndices[2]].y - ovec.y;
//    vvec.z = pframe->vertices[ptri_lst[i].vertexIndices[2]].z - ovec.z;
//
//    nrm   = CrossProduct(uvec, vvec);
//    ovvec = CrossProduct(ovec, vvec);
//
//    min_extent = MIN(MIN(dx,dy),dz);
//
//    // no point in searching outside the volume of the triangle
//    ix_min =     (tmp_bb.mins.x - pframe->bbmin[0]) / (pframe->bbmax[0]-pframe->bbmin[0]) * num_per_axis;
//    ix_max = 1 + (tmp_bb.maxs.x - pframe->bbmin[0]) / (pframe->bbmax[0]-pframe->bbmin[0]) * num_per_axis;
//
//    iy_min =     (tmp_bb.mins.y - pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//    iy_max = 1 + (tmp_bb.maxs.y - pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//
//    iz_min =     (tmp_bb.mins.z - pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//    iz_max = 1 + (tmp_bb.maxs.z - pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//
//    // The equation of a plane that intersects the origin is
//    //
//    //     pnorm . <x,y,z> = <0,0,0>.
//    //
//    // The parametric equation of a triangle is given by
//    //
//    //     <x,y,z> = u * uvec + v * vvec + ovec, 
//    //
//    // where u and v stay within the range [0,1], and uvec = p1 - p0, vvec = p2-p1, and ovec = p0, with
//    // p0, p1, and p2, being the corner points of the triangle
//    //
//    // Combining these two equations, we have
//    //
//    //     pnorm.x * (u * uvec.x + v * vvec.x + ovec.x) = 0
//    //     pnorm.y * (u * uvec.y + v * vvec.y + ovec.y) = 0
//    //     pnorm.z * (u * uvec.z + v * vvec.z + ovec.z) = 0
//
//    // divide the triangle along the axis with the largest component of the normal
//    if(dx == min_extent)
//    {
//      float minval = pframe->bbmin[0];
//      float maxval = pframe->bbmax[0];
//      float height;
//
//      for(i=ix_min; i<=ix_max; i++)
//      {
//        float u_min, u_max, u0,u1;
//        float y0,z0, y1,z1;
//
//        // the vector equation for the intersection of the triangle is
//        // <height,y,z> = u*uvec + v*vvec + ovec;
//        
//        // height-ovec.x = u*uvec.x + v*vvec.x
//        // y - ovec.y = u*uvec.y + v*vvec.y
//        // z - ovec.z = u*uvec.z + v*vvec.z
//
//        // u = (height-ovec.x)/uvec.x - v*vvec.x/uvec.x
//        // v = (height-ovec.x)/vvec.x - u*uvec.x/vvec.x
//        // both u and v are valid within the range [0,1], so
//        // min_u = max(0, (height-ovec.x)/uvec.x, (height-ovec.x)/uvec.x - vvec.x/uvec.x)
//        // max_u = min(1, (height-ovec.x)/uvec.x, (height-ovec.x)/uvec.x - vvec.x/uvec.x)
//        // if min_u >= max_u, then there is no intersection
//
//        height = (i + 0.5f)/(float)num_per_axis * (maxval-minval) + minval;
//
//        u0 = (height-ovec.x)/uvec.x;
//        u1 = u0 - vvec.x/uvec.x;
//
//        u_min = MAX(0, u0, u1);
//        u_max = MIN(1, u0, u1);
//
//        if(min_u >= max_u) continue;
//
//        // Now calculate the endpoints of the line
//
//        // y = ovec.y + u*uvec.y + v*vvec.y = -(ovec x vvec).z/vvec.x - u*(uvec x vvec).z/vvec.x + height*vvec.y/vvec.x
//        // z = ovec.z + u*uvec.z + v*vvec.z =  (ovec x vvec).y/vvec.x + u*(uvec x vvec).y/vvec.x + height*vvec.z/vvec.x
//
//        y0 = (-ovvec.x - u_min*nrm.z + height*vvec.y) / vvec.x;
//        z0 = ( ovvec.y + u_min*nrm.y + height*vvec.z) / vvec.x;
//
//        y1 = (-ovvec.x - u_max*nrm.z + height*vvec.y) / vvec.x;
//        z1 = ( ovvec.y + u_max*nrm.y + height*vvec.z) / vvec.x;
//
//
//        // now calculate which of the bboxes intersects this line
//        tmp_bb.mins.x = height;
//        tmp_bb.maxs.x = height;
//
//        iy_min = (MIN(y0,y1) + pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//        iy_max = (MAX(y0,y1) + pframe->bbmin[1]) / (pframe->bbmax[1]-pframe->bbmin[1]) * num_per_axis;
//
//        iz_min = (MIN(z0,z1) + pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//        iz_max = (MAX(z0,z1) + pframe->bbmin[2]) / (pframe->bbmax[2]-pframe->bbmin[2]) * num_per_axis;
//
//
//        for(j=iy_min, j<iy_max; j++)
//        {
//          for(k=iz_min, k<iz_max; k++)
//          {
//          }
//        }
//
//        height = i /(float)num_per_axis * (maxval-minval) + minval;
//
//        tmp_bb.mins.y = MIN(y0,y1);
//        tmp_bb.maxs.y = MAX(y0,y1);
//
//        tmp_bb.mins.z = MIN(z0,z1);
//        tmp_bb.maxs.z = MAX(z0,z1);
//
//        
//
//
//
//
//
//      }
//    }
//    else if(ABS(nrm.y) == max_comp)
//    {
//      // triangle is facing in the y direction, 
//    }
//    else
//    {
//      // triangle is facing in the z direction
//    }
//
//  }
//
//
//  // initialize the sises of the bboxes
//  pframe->vertices
//
//
//
//
//  // deallocate the list
//  bbox_list_delete( &bblist );
//}