/* Egoboo - camera.c
 * Various functions related to how the game camera works.
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
#include "camera.h"

//--------------------------------------------------------------------------------------------
void camera_look_at( float x, float y )
{
  // ZZ> This function makes the camera turn to face the character
  camzgoto = camzadd;
  if ( doturntime != 0 )
  {
    camturnleftright = ( 1.5f * PI ) - ATAN2( y - camy, x - camx );  // xgg
  }
}

void dump_matrix( glMatrix a )
{
  int i; int j;

  for ( j = 0;j < 4;j++ )
  {
    printf( "  " );
    for ( i = 0;i < 4;i++ )
      printf( "%f ", ( a )_CNV( i, j ) );
    printf( "\n" );
  }
}

//--------------------------------------------------------------------------------------------
void project_view()
{
  // ZZ> This function figures out where the corners of the view area
  //     go when projected onto the plane of the mesh.  Used later for
  //     determining which mesh fans need to be rendered

  int cnt, tnc, extra[2];
  float ztemp;
  float numstep;
  float zproject;
  float xfin, yfin, zfin;
  glMatrix mTemp;

  // Range
  ztemp = ( camz );

  // Topleft
  // printf("DIAG: In project_view\n");
  // printf("DIAG: dumping mView\n"); dump_matrix(mView);
  // printf("cam xyz,zoom = %f %f %f %f\n",camx,camy,camz,camzoom);

  mTemp = MatrixMult( RotateY( -rotmeshtopside * PI / 360 ), mView );
  mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
  zproject = ( mTemp )_CNV( 2, 2 );             //2,2
  // Camera must look down
  if ( zproject < 0 )
  {
    numstep = -ztemp / zproject;
    xfin = camx + ( numstep * ( mTemp )_CNV( 0, 2 ) );  // xgg      //0,2
    yfin = camy + ( numstep * ( mTemp )_CNV( 1, 2 ) );    //1,2
    zfin = 0;
    cornerx[0] = xfin;
    cornery[0] = yfin;
    // printf("Camera TL: %f %f\n",xfin,yfin);
    // dump_matrix(mTemp);
  }

  // Topright
  mTemp = MatrixMult( RotateY( rotmeshtopside * PI / 360 ), mView );
  mTemp = MatrixMult( RotateX( rotmeshup * PI / 360 ), mTemp );
  zproject = ( mTemp )_CNV( 2, 2 );             //2,2
  // Camera must look down
  if ( zproject < 0 )
  {
    numstep = -ztemp / zproject;
    xfin = camx + ( numstep * ( mTemp )_CNV( 0, 2 ) );  // xgg      //0,2
    yfin = camy + ( numstep * ( mTemp )_CNV( 1, 2 ) );    //1,2
    zfin = 0;
    cornerx[1] = xfin;
    cornery[1] = yfin;
    // printf("Camera TR: %f %f\n",xfin,yfin);
    // dump_matrix(mTemp);
  }

  // Bottomright
  mTemp = MatrixMult( RotateY( rotmeshbottomside * PI / 360 ), mView );
  mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
  zproject = ( mTemp )_CNV( 2, 2 );             //2,2
  // Camera must look down
  if ( zproject < 0 )
  {
    numstep = -ztemp / zproject;
    xfin = camx + ( numstep * ( mTemp )_CNV( 0, 2 ) );  // xgg      //0,2
    yfin = camy + ( numstep * ( mTemp )_CNV( 1, 2 ) );    //1,2
    zfin = 0;
    cornerx[2] = xfin;
    cornery[2] = yfin;
    // printf("Camera BR: %f %f\n",xfin,yfin);
    // dump_matrix(mTemp);
  }

  // Bottomleft
  mTemp = MatrixMult( RotateY( -rotmeshbottomside * PI / 360 ), mView );
  mTemp = MatrixMult( RotateX( -rotmeshdown * PI / 360 ), mTemp );
  zproject = ( mTemp )_CNV( 2, 2 );             //2,2
  // Camera must look down
  if ( zproject < 0 )
  {
    numstep = -ztemp / zproject;
    xfin = camx + ( numstep * ( mTemp )_CNV( 0, 2 ) );  // xgg      //0,2
    yfin = camy + ( numstep * ( mTemp )_CNV( 1, 2 ) );    //1,2
    zfin = 0;
    cornerx[3] = xfin;
    cornery[3] = yfin;
    // printf("Camera BL: %f %f\n",xfin,yfin);
    // dump_matrix(mTemp);
  }

  // Get the extreme values
  cornerlowx = cornerx[0];
  cornerlowy = cornery[0];
  cornerhighx = cornerx[0];
  cornerhighy = cornery[0];
  cornerlistlowtohighy[0] = 0;
  cornerlistlowtohighy[3] = 0;

  for ( cnt = 0; cnt < 4; cnt++ )
  {
    if ( cornerx[cnt] < cornerlowx )
      cornerlowx = cornerx[cnt];
    if ( cornery[cnt] < cornerlowy )
    {
      cornerlowy = cornery[cnt];
      cornerlistlowtohighy[0] = cnt;
    }
    if ( cornerx[cnt] > cornerhighx )
      cornerhighx = cornerx[cnt];
    if ( cornery[cnt] > cornerhighy )
    {
      cornerhighy = cornery[cnt];
      cornerlistlowtohighy[3] = cnt;
    }
  }

  // Figure out the order of points
  tnc = 0;
  for ( cnt = 0; cnt < 4; cnt++ )
  {
    if ( cnt != cornerlistlowtohighy[0] && cnt != cornerlistlowtohighy[3] )
    {
      extra[tnc] = cnt;
      tnc++;
    }
  }
  cornerlistlowtohighy[1] = extra[1];
  cornerlistlowtohighy[2] = extra[0];
  if ( cornery[extra[0]] < cornery[extra[1]] )
  {
    cornerlistlowtohighy[1] = extra[0];
    cornerlistlowtohighy[2] = extra[1];
  }
}

//--------------------------------------------------------------------------------------------
void make_camera_matrix()
{
  // ZZ> This function sets mView to the camera's location and rotation
  mView = mViewSave;
  mView = MatrixMult( Translate( camx, -camy, camz ), mView );  // xgg
  if ( camswingamp > 0.001f )
  {
    camroll = turntosin[camswing] * camswingamp;
    mView = MatrixMult( RotateY( camroll ), mView );
  }
  mView = MatrixMult( RotateZ( camturnleftright ), mView );
  mView = MatrixMult( RotateX( camturnupdown ), mView );
  // lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &mView);
//        glMatrixMode(GL_MODELVIEW);
///        glLoadMatrixf(mView.v);
}

//--------------------------------------------------------------------------------------------
void adjust_camera_angle( float height )
{
  // ZZ> This function makes the camera look downwards as it is raised up
  float percentmin, percentmax;


  if ( height < MINZADD )  height = MINZADD;
  percentmax = ( height - MINZADD ) / ( float )( MAXZADD - MINZADD );
  percentmin = 1.0f - percentmax;

  camturnupdown = ( ( MINUPDOWN * percentmin ) + ( MAXUPDOWN * percentmax ) );
  camzoom = ( MINZOOM * percentmin ) + ( MAXZOOM * percentmax );
}

//--------------------------------------------------------------------------------------------
void move_camera()
{
  // ZZ> This function moves the camera
  Uint16 cnt;
  Sint16 locoalive;
  float x, y, z, level, newx, newy, movex, movey;
  Uint16 character, turnsin, turncos;

  // printf("DIAG: In move_camera\n");
  // dump_matrix(mView);

  if ( autoturncamera )
    doturntime = 255;
  else if ( doturntime != 0 )
    doturntime--;

  x = 0;
  y = 0;
  z = 0;
  level = 0;
  locoalive = 0;

  for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
  {
    if ( plavalid[cnt] && pladevice[cnt] != INPUTNONE )
    {
      character = plaindex[cnt];
      if ( chralive[character] )
      {
        if ( chrattachedto[character] == MAXCHR )
        {
          // The character is on foot
          x += chrxpos[character];
          y += chrypos[character];
          z += chrzpos[character];
          level += chrlevel[character];
        }
        else
        {
          // The character is mounted
          x += chrxpos[chrattachedto[character]];
          y += chrypos[chrattachedto[character]];
          z += chrzpos[chrattachedto[character]];
          level += chrlevel[chrattachedto[character]];
        }
        locoalive++;
      }
    }
  }

  if ( locoalive > 0 )
  {
    x = x / locoalive;
    y = y / locoalive;
    z = z / locoalive;
    level = level / locoalive;
  }
  else
  {
    x = camtrackx;
    y = camtracky;
    z = camtrackz;
  }

  camtrackxvel = -camtrackx;
  camtrackyvel = -camtracky;
  camtrackzvel = -camtrackz;
  camtrackx = ( camtrackx + x ) / 2.0f;
  camtracky = ( camtracky + y ) / 2.0f;
  camtrackz = ( camtrackz + z ) / 2.0f;
  camtracklevel = ( camtracklevel + level ) / 2.0f;


  camturnadd = camturnadd * camsustain;
  camzadd = ( camzadd * 3.0f + camzaddgoto ) / 4.0f;
  camz = ( camz * 3.0f + camzgoto ) / 4.0f;
  // Camera controls
  if ( autoturncamera == 255 && numlocalpla == 1 )
  {
    if ( mouseon )
      if ( !control_mouse_is_pressed( MOS_CAMERA ) )
        camturnadd -= ( mousex * 0.5f );
    if ( keyon )
      camturnadd += ( control_key_is_pressed( KEY_LEFT ) - control_key_is_pressed( KEY_RIGHT ) ) * ( CAMKEYTURN );
    if ( joyaon )
      if ( !control_joya_is_pressed( JOA_CAMERA ) )
        camturnadd -= joyax * CAMJOYTURN;
    if ( joybon )
      if ( !control_joyb_is_pressed( JOB_CAMERA ) )
        camturnadd -= joybx * CAMJOYTURN;
  }
  else
  {
    if ( mouseon )
    {
      if ( control_mouse_is_pressed( MOS_CAMERA ) )
      {
        camturnadd += ( mousex / 3.0f );
        camzaddgoto += ( float ) mousey / 3.0f;
        if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
        if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
        doturntime = TURNTIME;  // Sticky turn...
      }
    }
    // JoyA camera controls
    if ( joyaon )
    {
      if ( control_joya_is_pressed( JOA_CAMERA ) )
      {
        camturnadd += joyax * CAMJOYTURN;
        camzaddgoto += joyay * CAMJOYTURN;
        if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
        if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
        doturntime = TURNTIME;  // Sticky turn...
      }
    }
    // JoyB camera controls
    if ( joybon )
    {
      if ( control_joyb_is_pressed( JOB_CAMERA ) )
      {
        camturnadd += joybx * CAMJOYTURN;
        camzaddgoto += joyby * CAMJOYTURN;
        if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
        if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
        doturntime = TURNTIME;  // Sticky turn...
      }
    }
  }
  // Keyboard camera controls
  if ( keyon )
  {
    if ( control_key_is_pressed( KEY_CAMERA_LEFT ) || control_key_is_pressed( KEY_CAMERA_RIGHT ) )
    {
      camturnadd += ( control_key_is_pressed( KEY_CAMERA_LEFT ) - control_key_is_pressed( KEY_CAMERA_RIGHT ) ) * CAMKEYTURN;
      doturntime = TURNTIME;  // Sticky turn...
    }
    if ( control_key_is_pressed( KEY_CAMERA_IN ) || control_key_is_pressed( KEY_CAMERA_OUT ) )
    {
      camzaddgoto += ( control_key_is_pressed( KEY_CAMERA_OUT ) - control_key_is_pressed( KEY_CAMERA_IN ) ) * CAMKEYTURN;
      if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
      if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
    }
  }
  camx -= ( float ) ( ( mView )_CNV( 0, 0 ) ) * camturnadd;  // xgg
  camy += ( float ) ( ( mView )_CNV( 1, 0 ) ) * -camturnadd;

  // Do distance effects for overlay and background
  camtrackxvel += camtrackx;
  camtrackyvel += camtracky;
  camtrackzvel += camtrackz;
  if ( overlayon )
  {
    // Do fg distance effect
    waterlayeru[0] += camtrackxvel * waterlayerdistx[0];
    waterlayerv[0] += camtrackyvel * waterlayerdisty[0];
  }
  if ( !clearson )
  {
    // Do bg distance effect
    waterlayeru[1] += camtrackxvel * waterlayerdistx[1];
    waterlayerv[1] += camtrackyvel * waterlayerdisty[1];
  }

  // Center on target for doing rotation...
  if ( doturntime != 0 )
  {
    camcenterx = camcenterx * 0.9f + camtrackx * 0.1f;
    camcentery = camcentery * 0.9f + camtracky * 0.1f;
  }

  // Create a tolerance area for walking without camera movement
  x = camtrackx - camx;
  y = camtracky - camy;
  newx = -( ( mView )_CNV( 0, 0 ) * x + ( mView )_CNV( 1, 0 ) * y ); // newx = -(mView(0,0) * x + mView(1,0) * y);
  newy = -( ( mView )_CNV( 0, 1 ) * x + ( mView )_CNV( 1, 1 ) * y ); // newy = -(mView(0,1) * x + mView(1,1) * y);

  // Get ready to scroll...
  movex = 0;
  movey = 0;

  // Adjust for camera height...
  z = ( TRACKXAREALOW  * ( MAXZADD - camzadd ) ) +
      ( TRACKXAREAHIGH * ( camzadd - MINZADD ) );
  z = z / ( MAXZADD - MINZADD );
  if ( newx < -z )
  {
    // Scroll left
    movex += ( newx + z );
  }
  if ( newx > z )
  {
    // Scroll right
    movex += ( newx - z );
  }

  // Adjust for camera height...
  z = ( TRACKYAREAMINLOW  * ( MAXZADD - camzadd ) ) +
      ( TRACKYAREAMINHIGH * ( camzadd - MINZADD ) );
  z = z / ( MAXZADD - MINZADD );

  if ( newy < z )
  {
    // Scroll down
    movey -= ( newy - z );
  }
  else
  {
    // Adjust for camera height...
    z = ( TRACKYAREAMAXLOW  * ( MAXZADD - camzadd ) ) +
        ( TRACKYAREAMAXHIGH * ( camzadd - MINZADD ) );
    z = z / ( MAXZADD - MINZADD );
    if ( newy > z )
    {
      // Scroll up
      movey -= ( newy - z );
    }
  }

  turnsin = (Uint16)( camturnleftrightone * TRIG_TABLE_SIZE ) & TRIG_TABLE_MASK;
  turncos = ( turnsin + TRIG_TABLE_OFFSET ) & TRIG_TABLE_MASK;
  camcenterx += ( movex * turntocos[turnsin] + movey * turntosin[turnsin] );
  camcentery += ( -movex * turntosin[turnsin] + movey * turntocos[turnsin] );

  // Finish up the camera
  camera_look_at( camcenterx, camcentery );
  camx = ( float ) camcenterx + ( camzoom * SIN( camturnleftright ) );
  camy = ( float ) camcentery + ( camzoom * COS( camturnleftright ) );

  adjust_camera_angle( camz );

  make_camera_matrix();
}

//--------------------------------------------------------------------------------------------
void reset_camera()
{
  // ZZ> This function makes sure the camera starts in a suitable position
  int cnt, save;

  camswing = 0;
  camx = meshedgex / 2;
  camy = meshedgey / 2;
  camz = 1500;
  camzoom = 1000;
  camtrackxvel = 0;
  camtrackyvel = 0;
  camtrackzvel = 1500;
  camcenterx = camx;
  camcentery = camy;
  camtrackx = camx;
  camtracky = camy;
  camtrackz = 1500;
  camturnadd = 0;
  camtracklevel = 0;
  camzadd = 1500;
  camzaddgoto = 1500;
  camzgoto = 1500;
  camturnleftright = ( float ) ( -PI / 4 );
  camturnleftrightone = ( float ) ( -PI / 4 ) / ( TWO_PI );
  camturnleftrightshort = 0;
  camturnupdown = ( float ) ( PI / 4 );
  camroll = 0;

  // Now move the camera towards the players
  mView = ZeroMatrix();

  save = autoturncamera;
  autoturncamera = btrue;

  for ( cnt = 0; cnt < 32; cnt++ )
  {
    move_camera();
    camcenterx = camtrackx;
    camcentery = camtracky;
  }

  autoturncamera = save;
  doturntime = 0;
}

