#include "egoboo_utility.h"
#include "Mad.h"

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
void get_actions( Uint16 mdl )
{
  // ZZ> This function creates the frame lists for each action based on the
  //     name of each md2 frame in the model
  int frame, framesinaction;
  ACTION action, lastaction;
  int         iFrameCount;
  MD2_Model * m;
  char      * szName;

  if( mdl>=MAXMODEL || !MadList[mdl].used ) return;

  m = MadList[mdl]._md2;
  if(NULL == m) return;

  // Clear out all actions and reset to invalid
  action = 0;
  while ( action < MAXACTION )
  {
    MadList[mdl].actionvalid[action] = bfalse;
    action++;
  }

  iFrameCount = md2_get_numFrames(m);
  if(0 == iFrameCount) return;

  // Set the primary dance action to be the first frame, just as a default
  MadList[mdl].actionvalid[ACTION_DA] = btrue;
  MadList[mdl].actionstart[ACTION_DA] = 0;
  MadList[mdl].actionend[ACTION_DA]   = 1;


  // Now go huntin' to see what each frame is, look for runs of same action
  szName = md2_get_Frame(m, 0)->name;
  lastaction = action_number(szName);
  framesinaction = 0;
  for ( frame = 0; frame < iFrameCount; frame++ )
  {
    MD2_Frame * pFrame = md2_get_Frame(m, frame);
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
        MadList[mdl].actionvalid[lastaction] = btrue;
        MadList[mdl].actionstart[lastaction] = frame - framesinaction;
        MadList[mdl].actionend[lastaction]   = frame;
      }
      framesinaction = 1;
      lastaction = action;
    }
    MadList[mdl].framefx[frame] = get_framefx( szName );
  }

  // Write the old action
  if ( lastaction < MAXACTION )
  {
    MadList[mdl].actionvalid[lastaction] = btrue;
    MadList[mdl].actionstart[lastaction] = frame - framesinaction;
    MadList[mdl].actionend[lastaction]   = frame;
  }

  // Make sure actions are made valid if a similar one exists
  action_copy_correct( mdl, ACTION_DA, ACTION_DB );   // All dances should be safe
  action_copy_correct( mdl, ACTION_DB, ACTION_DC );
  action_copy_correct( mdl, ACTION_DC, ACTION_DD );
  action_copy_correct( mdl, ACTION_DB, ACTION_DC );
  action_copy_correct( mdl, ACTION_DA, ACTION_DB );
  action_copy_correct( mdl, ACTION_UA, ACTION_UB );
  action_copy_correct( mdl, ACTION_UB, ACTION_UC );
  action_copy_correct( mdl, ACTION_UC, ACTION_UD );
  action_copy_correct( mdl, ACTION_TA, ACTION_TB );
  action_copy_correct( mdl, ACTION_TC, ACTION_TD );
  action_copy_correct( mdl, ACTION_CA, ACTION_CB );
  action_copy_correct( mdl, ACTION_CC, ACTION_CD );
  action_copy_correct( mdl, ACTION_SA, ACTION_SB );
  action_copy_correct( mdl, ACTION_SC, ACTION_SD );
  action_copy_correct( mdl, ACTION_BA, ACTION_BB );
  action_copy_correct( mdl, ACTION_BC, ACTION_BD );
  action_copy_correct( mdl, ACTION_LA, ACTION_LB );
  action_copy_correct( mdl, ACTION_LC, ACTION_LD );
  action_copy_correct( mdl, ACTION_XA, ACTION_XB );
  action_copy_correct( mdl, ACTION_XC, ACTION_XD );
  action_copy_correct( mdl, ACTION_FA, ACTION_FB );
  action_copy_correct( mdl, ACTION_FC, ACTION_FD );
  action_copy_correct( mdl, ACTION_PA, ACTION_PB );
  action_copy_correct( mdl, ACTION_PC, ACTION_PD );
  action_copy_correct( mdl, ACTION_ZA, ACTION_ZB );
  action_copy_correct( mdl, ACTION_ZC, ACTION_ZD );
  action_copy_correct( mdl, ACTION_WA, ACTION_WB );
  action_copy_correct( mdl, ACTION_WB, ACTION_WC );
  action_copy_correct( mdl, ACTION_WC, ACTION_WD );
  action_copy_correct( mdl, ACTION_DA, ACTION_WD );   // All walks should be safe
  action_copy_correct( mdl, ACTION_WC, ACTION_WD );
  action_copy_correct( mdl, ACTION_WB, ACTION_WC );
  action_copy_correct( mdl, ACTION_WA, ACTION_WB );
  action_copy_correct( mdl, ACTION_JA, ACTION_JB );
  action_copy_correct( mdl, ACTION_JB, ACTION_JC );
  action_copy_correct( mdl, ACTION_DA, ACTION_JC );  // All jumps should be safe
  action_copy_correct( mdl, ACTION_JB, ACTION_JC );
  action_copy_correct( mdl, ACTION_JA, ACTION_JB );
  action_copy_correct( mdl, ACTION_HA, ACTION_HB );
  action_copy_correct( mdl, ACTION_HB, ACTION_HC );
  action_copy_correct( mdl, ACTION_HC, ACTION_HD );
  action_copy_correct( mdl, ACTION_HB, ACTION_HC );
  action_copy_correct( mdl, ACTION_HA, ACTION_HB );
  action_copy_correct( mdl, ACTION_KA, ACTION_KB );
  action_copy_correct( mdl, ACTION_KB, ACTION_KC );
  action_copy_correct( mdl, ACTION_KC, ACTION_KD );
  action_copy_correct( mdl, ACTION_KB, ACTION_KC );
  action_copy_correct( mdl, ACTION_KA, ACTION_KB );
  action_copy_correct( mdl, ACTION_MH, ACTION_MI );
  action_copy_correct( mdl, ACTION_DA, ACTION_MM );
  action_copy_correct( mdl, ACTION_MM, ACTION_MN );


  // Create table for doing transition from one type of walk to another...
  // Clear 'em all to start
  for ( frame = 0; frame < iFrameCount; frame++ )
    MadList[mdl].framelip[frame] = 0;

  // Need to figure out how far into action each frame is
  make_framelip( mdl, ACTION_WA );
  make_framelip( mdl, ACTION_WB );
  make_framelip( mdl, ACTION_WC );

  // Now do the same, in reverse, for walking animations
  get_walk_frame( mdl, LIPT_DA, ACTION_DA );
  get_walk_frame( mdl, LIPT_WA, ACTION_WA );
  get_walk_frame( mdl, LIPT_WB, ACTION_WB );
  get_walk_frame( mdl, LIPT_WC, ACTION_WC );
}

//--------------------------------------------------------------------------------------------
void make_mad_equally_lit( Uint16 model )
{
  // ZZ> This function makes ultra low poly models look better
  int frame, vert;
  int iFrames, iVerts;
  MD2_Model * m;

  if(model > MAXMODEL || !MadList[model].used) return;

  m = MadList[model]._md2;
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

