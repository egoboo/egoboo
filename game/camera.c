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
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "mathstuff.h"
#include "Frustum.h"
#include <assert.h>

vect3 campos = {0, 1500, 750};  // Camera position


//--------------------------------------------------------------------------------------------
void ortho_jitter( GLfloat xoff, GLfloat yoff )
{
  GLint viewport[4];
  GLfloat ortho[16];
  GLfloat scalex, scaley;

  glGetIntegerv( GL_VIEWPORT, viewport );
  /* this assumes that only a glOrtho() call has been applied to the projection matrix */
  glGetFloatv( GL_PROJECTION_MATRIX, ortho );

  scalex = ( 2.f / ortho[0] ) / viewport[2];
  scaley = ( 2.f / ortho[5] ) / viewport[3];
  glTranslatef( xoff * scalex, yoff * scaley, 0.f );
}

//--------------------------------------------------------------------------------------------
void frustum_jitter_fov( GLdouble nearval, GLdouble farval, GLdouble fov, GLdouble xoff, GLdouble yoff )
{
  GLfloat  scale;
  GLint    viewport[4];
  GLdouble hprime, wprime;
  GLdouble left, right, top, bottom;

  ATTRIB_PUSH( "frustum_jitter_fov", GL_TRANSFORM_BIT );
  {
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();

    glGetIntegerv( GL_VIEWPORT, viewport );

    //hprime = 2*nearval*tan(fov/2.0f);
    //wprime = viewport[2]*hprime/viewport[3];

    //scalex = wprime/viewport[2];
    //scaley = hprime/viewport[3];

    hprime = 2.0f * nearval * tan( fov * 0.5f );
    scale = hprime / viewport[3];
    wprime = scale * viewport[2];

    xoff *= 0.25;
    yoff *= 0.25;

    left   = -wprime / 2.0f;
    right  = wprime / 2.0f;
    top    = -hprime / 2.0f;
    bottom = hprime / 2.0f;

    glFrustum( left   - xoff * scale,
               right  - xoff * scale,
               top    - yoff * scale,
               bottom - yoff * scale,
               nearval, farval );

    glGetFloatv( GL_PROJECTION_MATRIX, mProjection.v );

    glPopMatrix();
  }
  ATTRIB_POP( "frustum_jitter_fov" );
}

//--------------------------------------------------------------------------------------------
void frustum_jitter( GLdouble left, GLdouble right,
                     GLdouble bottom, GLdouble top,
                     GLdouble nearval, GLdouble farval,
                     GLdouble xoff, GLdouble yoff )
{
  GLfloat scalex, scaley;
  GLint viewport[4];

  glGetIntegerv( GL_VIEWPORT, viewport );
  scalex = ( right - left ) / viewport[2];
  scaley = ( top - bottom ) / viewport[3];

  glFrustum( left - xoff * scalex,
             right - xoff * scalex,
             top - yoff * scaley,
             bottom - yoff * scaley,
             nearval, farval );
}

//--------------------------------------------------------------------------------------------
void camera_calc_turn_lr()
{
  // ZZ> This function makes the camera turn to face the character

  camturn_lr = vec_to_turn(camtrackpos.x - campos.x, camtrackpos.y - campos.y);
}

//--------------------------------------------------------------------------------------------
void screen_dump_matrix( GLMatrix a )
{
  int i, j;
  STRING buffer1 = {0};
  STRING buffer2 = {0};

  reset_messages();
  for ( j = 0; j < 4; j++ )
  {
    snprintf( buffer1, sizeof( buffer1 ), "  " );
    for ( i = 0; i < 4; i++ )
    {
      snprintf( buffer2, sizeof( buffer2 ), "%2.4f ", ( a ) _CNV( i, j ) );
      strncat( buffer1, buffer2, sizeof( buffer1 ) );
    };
    debug_message( 1, buffer1 );
    buffer1[0] = '\0';
  }
}

//--------------------------------------------------------------------------------------------
void stdout_dump_matrix( GLMatrix a )
{
  int i, j;

  for ( j = 0; j < 4; j++ )
  {
    fprintf( stdout, "  " );
    for ( i = 0; i < 4; i++ )
    {
      fprintf( stdout, "%2.4f ", ( a ) _CNV( i, j ) );
    };
    fprintf( stdout, "\n" );
  }
  fprintf( stdout, "\n" );
}

//--------------------------------------------------------------------------------------------
//void project_view()
//{
//  // ZZ> This function figures out where the corners of the view area
//  //     go when projected onto the plane of the mesh.  Used later for
//  //     determining which mesh fans need to be rendered
//
//  int cnt, tnc, extra[3];
//  float ztemp;
//  float numstep;
//  float zproject;
//  float xfin, yfin, zfin;
//  GLMatrix mTemp;
//
//  // Range
//  ztemp = campos.z;
//
//  // Topleft
//  mTemp = MatrixMult(RotateY(-rotmeshtopside * 0.5f * DEG_TO_RAD), mView);
//  mTemp = MatrixMult(RotateX(rotmeshup * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = campos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = campos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
//    zfin = 0;
//    cornerx[0] = xfin;
//    cornery[0] = yfin;
//    //dump_matrix(mTemp);
//  }
//
//  // Topright
//  mTemp = MatrixMult(RotateY(rotmeshtopside * 0.5f * DEG_TO_RAD), mView);
//  mTemp = MatrixMult(RotateX(rotmeshup * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = campos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = campos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
//    zfin = 0;
//    cornerx[1] = xfin;
//    cornery[1] = yfin;
//    //dump_matrix(mTemp);
//  }
//
//  // Bottomright
//  mTemp = MatrixMult(RotateY(rotmeshbottomside * 0.5f * DEG_TO_RAD), mView);
//  mTemp = MatrixMult(RotateX(-rotmeshdown * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = campos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = campos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
//    zfin = 0;
//    cornerx[2] = xfin;
//    cornery[2] = yfin;
//    //dump_matrix(mTemp);
//  }
//
//  // Bottomleft
//  mTemp = MatrixMult(RotateY(-rotmeshbottomside * 0.5f * DEG_TO_RAD), mView);
//  mTemp = MatrixMult(RotateX(-rotmeshdown * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = campos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = campos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
//    zfin = 0;
//    cornerx[3] = xfin;
//    cornery[3] = yfin;
//    //dump_matrix(mTemp);
//  }
//
//  // Get the extreme values
//  cornerlowx = cornerx[0];
//  cornerlowy = cornery[0];
//  cornerhighx = cornerx[0];
//  cornerhighy = cornery[0];
//  cornerlistlowtohighy[0] = 0;
//  cornerlistlowtohighy[3] = 0;
//
//  for (cnt = 0; cnt < 4; cnt++)
//  {
//    if (cornerx[cnt] < cornerlowx)
//      cornerlowx = cornerx[cnt];
//    if (cornery[cnt] < cornerlowy)
//    {
//      cornerlowy = cornery[cnt];
//      cornerlistlowtohighy[0] = cnt;
//    }
//    if (cornerx[cnt] > cornerhighx)
//      cornerhighx = cornerx[cnt];
//    if (cornery[cnt] > cornerhighy)
//    {
//      cornerhighy = cornery[cnt];
//      cornerlistlowtohighy[3] = cnt;
//    }
//  }
//
//  // Figure out the order of points
//  tnc = 0;
//  for (cnt = 0; cnt < 4; cnt++)
//  {
//    if (cnt != cornerlistlowtohighy[0] && cnt != cornerlistlowtohighy[3])
//    {
//      extra[tnc] = cnt;
//      tnc++;
//    }
//  }
//  cornerlistlowtohighy[1] = extra[1];
//  cornerlistlowtohighy[2] = extra[0];
//  if (cornery[extra[0]] < cornery[extra[1]])
//  {
//    cornerlistlowtohighy[1] = extra[0];
//    cornerlistlowtohighy[2] = extra[1];
//  }
//}
//
//--------------------------------------------------------------------------------------------
void make_camera_matrix()
{
  // ZZ> This function sets mView to the camera's location and rotation
  vect3 worldup = {0, 0, -gravity};

  if ( camswingamp > 0 )
  {
    camroll = turntosin[camswing & TRIGTABLE_MASK] * camswingamp * 5;
    mView = ViewMatrix( campos, camtrackpos, worldup, camroll );
  }
  else
  {
    mView = ViewMatrix( campos, camtrackpos, worldup, 0 );
  }

  frustum_jitter_fov( 10.0f, 20000.0f, DEG_TO_RAD*FOV, 2.0f*( float ) rand() / ( float ) RAND_MAX - 1.0f, 2.0f*( float ) rand() / ( float ) RAND_MAX - 1.0f );

  Frustum_CalculateFrustum( &gFrustum, mProjectionBig.v, mView.v );
}

//--------------------------------------------------------------------------------------------
void move_camera( float dUpdate )
{
  // ZZ> This function moves the camera
  int cnt, locoalive;  // Used in rts remove? -> int band,
  vect3 pos, vel, move;
  float level;
  CHR_REF character;
  Uint16 turnsin, turncos;
  float ftmp;
  float fkeep, fnew;

  fkeep = pow( camsustain, dUpdate );
  fnew  = 1.0 - fkeep;

  if ( CData.autoturncamera )
  {
    doturntime = 255;
  }
  else if ( doturntime > dUpdate )
  {
    doturntime -= dUpdate;
  }
  else
  {
    doturntime = 0;
  }

  pos.x = pos.y = pos.z = 0;
  vel.x = vel.y = vel.z = 0;
  level = 0;
  locoalive = 0;
  for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
  {
    if ( !VALID_PLA( cnt ) || INBITS_NONE == pladevice[cnt] ) continue;

    character = pla_get_character( cnt );
    if ( VALID_CHR( character ) && chralive[character] )
    {
      CHR_REF attachedto = chr_get_attachedto( character );

      if ( VALID_CHR( attachedto ) )
      {
        // The character is mounted
        pos.x += chrpos[attachedto].x;
        pos.y += chrpos[attachedto].y;
        pos.z += chrpos[attachedto].z + 0.9 * chrbumpheight[character];

        level += chrlevel[attachedto];

        vel.x += chrvel[attachedto].x;
        vel.y += chrvel[attachedto].y;
        vel.z += chrvel[attachedto].z;
      }
      else
      {
        // The character is on foot
        pos.x += chrpos[character].x;
        pos.y += chrpos[character].y;
        pos.z += chrpos[character].z + 0.9 * chrbumpheight[character];

        level += chrlevel[character];

        vel.x += chrvel[character].x;
        vel.y += chrvel[character].y;
        vel.z += chrvel[character].z;
      };

      locoalive++;
    }
  }

  if ( locoalive == 0 )
  {
    pos = camtrackpos;
    vel = camtrackvel;
  }
  else if ( locoalive > 1 )
  {
    pos.x /= locoalive;
    pos.y /= locoalive;
    pos.z /= locoalive;

    level /= locoalive;

    vel.x /= locoalive;
    vel.y /= locoalive;
    vel.z /= locoalive;
  }

  camtrackpos.x = camtrackpos.x * fkeep + pos.x * fnew;
  camtrackpos.y = camtrackpos.y * fkeep + pos.y * fnew;
  camtrackpos.z = camtrackpos.z * fkeep + pos.z * fnew;

  camtracklevel = camtracklevel * fkeep + level * fnew;

  camtrackvel.x = camtrackvel.x * fkeep + vel.x * fnew;
  camtrackvel.y = camtrackvel.y * fkeep + vel.y * fnew;
  camtrackvel.z = camtrackvel.z * fkeep + vel.z * fnew;

  camzgoto      = camzgoto * fkeep + camzadd * fnew;
  camzadd       = camzadd  * fkeep + camzaddgoto * fnew;
  campos.z      = campos.z * fkeep + camzgoto    * fnew;
  camturnadd   *= fkeep;

  // Camera controls
  if ( CData.autoturncamera == 255 && numlocalpla >= 1 )
  {
    if ( mouseon && !control_mouse_is_pressed( CONTROL_CAMERA ) )
      camturnadd -= ( mousedx * .5 ) * dUpdate / numlocalpla;

    if ( keyon )
    {
      if ( control_key_is_pressed( KEY_CAMERA_LEFT ) )
        camturnadd += CAMKEYTURN * dUpdate / numlocalpla;

      if ( control_key_is_pressed( KEY_CAMERA_RIGHT ) )
        camturnadd -= CAMKEYTURN * dUpdate / numlocalpla;
    };


    if ( joyaon && !control_joya_is_pressed( CONTROL_CAMERA ) )
      camturnadd -= joyax * CAMJOYTURN * dUpdate / numlocalpla;

    if ( joybon && !control_joyb_is_pressed( CONTROL_CAMERA ) )
      camturnadd -= joybx * CAMJOYTURN * dUpdate / numlocalpla;
  }

  if ( numlocalpla >= 1 )
  {
    if ( mouseon )
    {
      if ( control_mouse_is_pressed( CONTROL_CAMERA ) )
      {
        camturnadd += ( mousedx / 3.0 ) * dUpdate / numlocalpla;
        camzaddgoto += ( float ) mousedy / 3.0 * dUpdate / numlocalpla;
        if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
        if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
        doturntime = DELAY_TURN;  // Sticky turn...
      }
    }

    // JoyA camera controls
    if ( joyaon )
    {
      if ( control_joya_is_pressed( CONTROL_CAMERA ) )
      {
        camturnadd  += joyax * CAMJOYTURN * dUpdate / numlocalpla;
        camzaddgoto += joyay * CAMJOYTURN * dUpdate / numlocalpla;
        if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
        if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
        doturntime = DELAY_TURN;  // Sticky turn...
      }
    }

    // JoyB camera controls
    if ( joybon )
    {
      if ( control_joyb_is_pressed( CONTROL_CAMERA ) )
      {
        camturnadd  += joybx * CAMJOYTURN * dUpdate / numlocalpla;
        camzaddgoto += joyby * CAMJOYTURN * dUpdate / numlocalpla;
        if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
        if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
        doturntime = DELAY_TURN;  // Sticky turn...
      }
    }

    // Keyboard camera controls
    if ( keyon )
    {
      if ( control_key_is_pressed( KEY_CAMERA_LEFT ) )
      {
        camturnadd += CAMKEYTURN * dUpdate / numlocalpla;
        doturntime = DELAY_TURN;  // Sticky turn...
      }

      if ( control_key_is_pressed( KEY_CAMERA_RIGHT ) )
      {
        camturnadd -= CAMKEYTURN * dUpdate / numlocalpla;
        doturntime = DELAY_TURN;  // Sticky turn...
      }

      if ( control_key_is_pressed( KEY_CAMERA_IN ) )
      {
        camzaddgoto -= CAMKEYTURN * dUpdate / numlocalpla;
      }

      if ( control_key_is_pressed( KEY_CAMERA_OUT ) )
      {
        camzaddgoto += CAMKEYTURN * dUpdate / numlocalpla;
      }

      if ( camzaddgoto < MINZADD )  camzaddgoto = MINZADD;
      if ( camzaddgoto > MAXZADD )  camzaddgoto = MAXZADD;
    }
  }

  // Do distance effects for overlay and background
  if ( CData.render_overlay )
  {
    // Do fg distance effect
    waterlayeru[0] += vel.x * waterlayerdistx[0] * dUpdate;
    waterlayerv[0] += vel.y * waterlayerdisty[0] * dUpdate;
  }

  if ( CData.render_background )
  {
    // Do bg distance effect
    waterlayeru[1] += vel.x * waterlayerdistx[1] * dUpdate;
    waterlayerv[1] += vel.y * waterlayerdisty[1] * dUpdate;
  }

  // Get ready to scroll...
  move.x = campos.x - camtrackpos.x;
  move.y = campos.y - camtrackpos.y;
  ftmp = move.x * move.x + move.y * move.y;
  if ( ftmp > 0 )
  {
    ftmp = sqrt( ftmp );
    move.x *= camzoom / ftmp;
    move.y *= camzoom / ftmp;
  }
  else
  {
    move.x = camzoom;
    move.y = 0;
  }
  turnsin = (( Uint16 ) camturnadd * 10 ) & TRIGTABLE_MASK;
  turncos = ( turnsin + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
  campos.x = ( move.x * turntosin[turncos] + move.y * turntosin[turnsin] ) + camtrackpos.x;
  campos.y = ( -move.x * turntosin[turnsin] + move.y * turntosin[turncos] ) + camtrackpos.y;

  // Finish up the camera
  camera_calc_turn_lr();
  make_camera_matrix();
}

//--------------------------------------------------------------------------------------------
void reset_camera()
{
  // ZZ> This function makes sure the camera starts in a suitable position
  int cnt, save;
  float fov2;

  camswing = 0;
  campos.x = 0;
  campos.y = 0;
  campos.z = 800;
  camzoom = 1000;
  camtrackvel.x = 0;
  camtrackvel.y = 0;
  camtrackvel.z = 0;
  camcenterpos.x = campos.x;
  camcenterpos.y = campos.y;
  camtrackpos.x = campos.x;
  camtrackpos.y = campos.y;
  camtrackpos.z = 0;
  camturnadd = 0;
  camtracklevel = 0;
  camzadd = 800;
  camzaddgoto = 800;
  camzgoto = 800;
  camturn_lr     = 8192;
  camturn_lr_one = camturn_lr / (float)(1<<16);
  camroll = 0;

  mProjection = ProjectionMatrix( 1.0f, 20000.0f, FOV );

  // calculate a second, larger, projection matrix
  fov2 = atan( tan(DEG_TO_RAD * FOV * 0.5f) * 1.5f) *2.0f * RAD_TO_DEG;
  mProjectionBig = ProjectionMatrix( 1.0f, 20000.0f, fov2 );

  save = CData.autoturncamera;
  CData.autoturncamera = btrue;

  for ( cnt = 0; cnt < 32; cnt++ )
  {
    move_camera( 1.0 );
    camcenterpos.x = camtrackpos.x;
    camcenterpos.y = camtrackpos.y;
  }

  CData.autoturncamera = save;
  doturntime = 0;
}

