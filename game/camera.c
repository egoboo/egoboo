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

#include "camera.h"
#include "Frustum.h"
#include "input.h"
#include "char.h"

#include "egoboo_math.h"
#include "egoboo.h"

#include <assert.h>

CAMERA GCamera;

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

    glGetFloatv( GL_PROJECTION_MATRIX, GCamera.mProjection.v );

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

  GCamera.turn_lr = vec_to_turn(GCamera.trackpos.x - GCamera.pos.x, GCamera.trackpos.y - GCamera.pos.y);
}

//--------------------------------------------------------------------------------------------
void screen_dump_matrix( matrix_4x4 a )
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
void stdout_dump_matrix( matrix_4x4 a )
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
void make_camera_matrix()
{
  // ZZ> This function sets GCamera.mView to the camera's location and rotation
  vect3 worldup = {0, 0, -gravity};

  if ( GCamera.swingamp > 0 )
  {
    GCamera.roll = turntosin[GCamera.swing & TRIGTABLE_MASK] * GCamera.swingamp * 5;
    GCamera.mView = ViewMatrix( GCamera.pos, GCamera.trackpos, worldup, GCamera.roll );
  }
  else
  {
    GCamera.mView = ViewMatrix( GCamera.pos, GCamera.trackpos, worldup, 0 );
  }

  frustum_jitter_fov( 10.0f, 20000.0f, DEG_TO_RAD*FOV, 2.0f*( float ) rand() / ( float ) RAND_MAX - 1.0f, 2.0f*( float ) rand() / ( float ) RAND_MAX - 1.0f );

  Frustum_CalculateFrustum( &gFrustum, GCamera.mProjectionBig.v, GCamera.mView.v );
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

  fkeep = pow( GCamera.sustain, dUpdate );
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
    if ( !VALID_PLA( cnt ) || INBITS_NONE == PlaList[cnt].device ) continue;

    character = pla_get_character( cnt );
    if ( VALID_CHR( character ) && ChrList[character].alive )
    {
      CHR_REF attachedto = chr_get_attachedto( character );

      if ( VALID_CHR( attachedto ) )
      {
        // The character is mounted
        pos.x += ChrList[attachedto].pos.x;
        pos.y += ChrList[attachedto].pos.y;
        pos.z += ChrList[attachedto].pos.z + 0.9 * ChrList[character].bmpdata.calc_height;

        level += ChrList[attachedto].level;

        vel.x += ChrList[attachedto].vel.x;
        vel.y += ChrList[attachedto].vel.y;
        vel.z += ChrList[attachedto].vel.z;
      }
      else
      {
        // The character is on foot
        pos.x += ChrList[character].pos.x;
        pos.y += ChrList[character].pos.y;
        pos.z += ChrList[character].pos.z + 0.9 * ChrList[character].bmpdata.calc_height;

        level += ChrList[character].level;

        vel.x += ChrList[character].vel.x;
        vel.y += ChrList[character].vel.y;
        vel.z += ChrList[character].vel.z;
      };

      locoalive++;
    }
  }

  if ( locoalive == 0 )
  {
    pos = GCamera.trackpos;
    vel = GCamera.trackvel;
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

  GCamera.trackpos.x = GCamera.trackpos.x * fkeep + pos.x * fnew;
  GCamera.trackpos.y = GCamera.trackpos.y * fkeep + pos.y * fnew;
  GCamera.trackpos.z = GCamera.trackpos.z * fkeep + pos.z * fnew;

  GCamera.tracklevel = GCamera.tracklevel * fkeep + level * fnew;

  GCamera.trackvel.x = GCamera.trackvel.x * fkeep + vel.x * fnew;
  GCamera.trackvel.y = GCamera.trackvel.y * fkeep + vel.y * fnew;
  GCamera.trackvel.z = GCamera.trackvel.z * fkeep + vel.z * fnew;

  GCamera.zgoto      = GCamera.zgoto * fkeep + GCamera.zadd * fnew;
  GCamera.zadd       = GCamera.zadd  * fkeep + GCamera.zaddgoto * fnew;
  GCamera.pos.z      = GCamera.pos.z * fkeep + GCamera.zgoto    * fnew;
  GCamera.turnadd   *= fkeep;

  // Camera controls
  if ( CData.autoturncamera == 255 && numlocalpla >= 1 )
  {
    if ( mous.on && !control_mouse_is_pressed( CONTROL_CAMERA ) )
      GCamera.turnadd -= ( mous.dlatch.x * .5 ) * dUpdate / numlocalpla;

    if ( keyb.on )
    {
      if ( control_key_is_pressed( KEY_CAMERA_LEFT ) )
        GCamera.turnadd += CAMKEYTURN * dUpdate / numlocalpla;

      if ( control_key_is_pressed( KEY_CAMERA_RIGHT ) )
        GCamera.turnadd -= CAMKEYTURN * dUpdate / numlocalpla;
    };


    if ( joy[0].on && !control_joy_is_pressed( 0, CONTROL_CAMERA ) )
      GCamera.turnadd -= joy[0].latch.x * CAMJOYTURN * dUpdate / numlocalpla;

    if ( joy[1].on && !control_joy_is_pressed( 1, CONTROL_CAMERA ) )
      GCamera.turnadd -= joy[1].latch.x * CAMJOYTURN * dUpdate / numlocalpla;
  }

  if ( numlocalpla >= 1 )
  {
    if ( mous.on )
    {
      if ( control_mouse_is_pressed( CONTROL_CAMERA ) )
      {
        GCamera.turnadd += ( mous.dlatch.x / 3.0 ) * dUpdate / numlocalpla;
        GCamera.zaddgoto += ( float ) mous.dlatch.y / 3.0 * dUpdate / numlocalpla;
        if ( GCamera.zaddgoto < MINZADD )  GCamera.zaddgoto = MINZADD;
        if ( GCamera.zaddgoto > MAXZADD )  GCamera.zaddgoto = MAXZADD;
        doturntime = DELAY_TURN;  // Sticky turn...
      }
    }

    // JoyA camera controls
    if ( joy[0].on )
    {
      if ( control_joy_is_pressed( 0, CONTROL_CAMERA ) )
      {
        GCamera.turnadd  += joy[0].latch.x * CAMJOYTURN * dUpdate / numlocalpla;
        GCamera.zaddgoto += joy[0].latch.y * CAMJOYTURN * dUpdate / numlocalpla;
        if ( GCamera.zaddgoto < MINZADD )  GCamera.zaddgoto = MINZADD;
        if ( GCamera.zaddgoto > MAXZADD )  GCamera.zaddgoto = MAXZADD;
        doturntime = DELAY_TURN;  // Sticky turn...
      }
    }

    // JoyB camera controls
    if ( joy[1].on )
    {
      if ( control_joy_is_pressed( 1, CONTROL_CAMERA ) )
      {
        GCamera.turnadd  += joy[1].latch.x * CAMJOYTURN * dUpdate / numlocalpla;
        GCamera.zaddgoto += joy[1].latch.y * CAMJOYTURN * dUpdate / numlocalpla;
        if ( GCamera.zaddgoto < MINZADD )  GCamera.zaddgoto = MINZADD;
        if ( GCamera.zaddgoto > MAXZADD )  GCamera.zaddgoto = MAXZADD;
        doturntime = DELAY_TURN;  // Sticky turn...
      }
    }

    // Keyboard camera controls
    if ( keyb.on )
    {
      if ( control_key_is_pressed( KEY_CAMERA_LEFT ) )
      {
        GCamera.turnadd += CAMKEYTURN * dUpdate / numlocalpla;
        doturntime = DELAY_TURN;  // Sticky turn...
      }

      if ( control_key_is_pressed( KEY_CAMERA_RIGHT ) )
      {
        GCamera.turnadd -= CAMKEYTURN * dUpdate / numlocalpla;
        doturntime = DELAY_TURN;  // Sticky turn...
      }

      if ( control_key_is_pressed( KEY_CAMERA_IN ) )
      {
        GCamera.zaddgoto -= CAMKEYTURN * dUpdate / numlocalpla;
      }

      if ( control_key_is_pressed( KEY_CAMERA_OUT ) )
      {
        GCamera.zaddgoto += CAMKEYTURN * dUpdate / numlocalpla;
      }

      if ( GCamera.zaddgoto < MINZADD )  GCamera.zaddgoto = MINZADD;
      if ( GCamera.zaddgoto > MAXZADD )  GCamera.zaddgoto = MAXZADD;
    }
  }

  // Do distance effects for overlay and background
  if ( CData.render_overlay )
  {
    // Do fg distance effect
    GWater.layeru[0] += vel.x * GWater.layerdistx[0] * dUpdate;
    GWater.layerv[0] += vel.y * GWater.layerdisty[0] * dUpdate;
  }

  if ( CData.render_background )
  {
    // Do bg distance effect
    GWater.layeru[1] += vel.x * GWater.layerdistx[1] * dUpdate;
    GWater.layerv[1] += vel.y * GWater.layerdisty[1] * dUpdate;
  }

  // Get ready to scroll...
  move.x = GCamera.pos.x - GCamera.trackpos.x;
  move.y = GCamera.pos.y - GCamera.trackpos.y;
  ftmp = move.x * move.x + move.y * move.y;
  if ( ftmp > 0 )
  {
    ftmp = sqrt( ftmp );
    move.x *= GCamera.zoom / ftmp;
    move.y *= GCamera.zoom / ftmp;
  }
  else
  {
    move.x = GCamera.zoom;
    move.y = 0;
  }
  turnsin = (( Uint16 ) GCamera.turnadd * 10 ) & TRIGTABLE_MASK;
  turncos = ( turnsin + TRIGTABLE_SHIFT ) & TRIGTABLE_MASK;
  GCamera.pos.x = ( move.x * turntosin[turncos] + move.y * turntosin[turnsin] ) + GCamera.trackpos.x;
  GCamera.pos.y = ( -move.x * turntosin[turnsin] + move.y * turntosin[turncos] ) + GCamera.trackpos.y;

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

  GCamera.swing = 0;
  GCamera.pos.x = 0;
  GCamera.pos.y = 0;
  GCamera.pos.z = 800;
  GCamera.zoom = 1000;
  GCamera.trackvel.x = 0;
  GCamera.trackvel.y = 0;
  GCamera.trackvel.z = 0;
  GCamera.centerpos.x = GCamera.pos.x;
  GCamera.centerpos.y = GCamera.pos.y;
  GCamera.trackpos.x = GCamera.pos.x;
  GCamera.trackpos.y = GCamera.pos.y;
  GCamera.trackpos.z = 0;
  GCamera.turnadd = 0;
  GCamera.tracklevel = 0;
  GCamera.zadd = 800;
  GCamera.zaddgoto = 800;
  GCamera.zgoto = 800;
  GCamera.turn_lr     = 8192;
  GCamera.turn_lr_one = GCamera.turn_lr / (float)(1<<16);
  GCamera.roll = 0;

  GCamera.mProjection = ProjectionMatrix( 1.0f, 20000.0f, FOV );

  // calculate a second, larger, projection matrix
  fov2 = atan( tan(DEG_TO_RAD * FOV * 0.5f) * 1.5f) *2.0f * RAD_TO_DEG;
  GCamera.mProjectionBig = ProjectionMatrix( 1.0f, 20000.0f, fov2 );

  save = CData.autoturncamera;
  CData.autoturncamera = btrue;

  for ( cnt = 0; cnt < 32; cnt++ )
  {
    move_camera( 1.0 );
    GCamera.centerpos.x = GCamera.trackpos.x;
    GCamera.centerpos.y = GCamera.trackpos.y;
  }

  CData.autoturncamera = save;
  doturntime = 0;
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
//  matrix_4x4 mTemp;
//
//  // Range
//  ztemp = GCamera.pos.z;
//
//  // Topleft
//  mTemp = MatrixMult(RotateY(-rotmeshtopside * 0.5f * DEG_TO_RAD), GCamera.mView);
//  mTemp = MatrixMult(RotateX(rotmeshup * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = GCamera.pos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = GCamera.pos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
//    zfin = 0;
//    cornerx[0] = xfin;
//    cornery[0] = yfin;
//    //dump_matrix(mTemp);
//  }
//
//  // Topright
//  mTemp = MatrixMult(RotateY(rotmeshtopside * 0.5f * DEG_TO_RAD), GCamera.mView);
//  mTemp = MatrixMult(RotateX(rotmeshup * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = GCamera.pos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = GCamera.pos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
//    zfin = 0;
//    cornerx[1] = xfin;
//    cornery[1] = yfin;
//    //dump_matrix(mTemp);
//  }
//
//  // Bottomright
//  mTemp = MatrixMult(RotateY(rotmeshbottomside * 0.5f * DEG_TO_RAD), GCamera.mView);
//  mTemp = MatrixMult(RotateX(-rotmeshdown * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = GCamera.pos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = GCamera.pos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
//    zfin = 0;
//    cornerx[2] = xfin;
//    cornery[2] = yfin;
//    //dump_matrix(mTemp);
//  }
//
//  // Bottomleft
//  mTemp = MatrixMult(RotateY(-rotmeshbottomside * 0.5f * DEG_TO_RAD), GCamera.mView);
//  mTemp = MatrixMult(RotateX(-rotmeshdown * 0.5f * DEG_TO_RAD), mTemp);
//  zproject = (mTemp)_CNV(2, 2);        //2,2
//  // Camera must look down
//  if (zproject < 0)
//  {
//    numstep = -ztemp / zproject;
//    xfin = GCamera.pos.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
//    yfin = GCamera.pos.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
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