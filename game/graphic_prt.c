/* Egoboo - graphicprc.c
* Particle system drawing and management code.
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

#include "ogl_include.h"
#include "Log.h"
#include "Mesh.h"
#include "particle.h"
#include "camera.h"
#include "char.h"

#include "egoboo_math.h"
#include "egoboo.h"

#include <assert.h>

typedef enum prt_ori_t
{
  ori_v,
  ori_h,
  ori_p,
  ori_b
} PRT_ORI;

int particle_orientation[256] =
{
  ori_b, ori_b, ori_v, ori_v, ori_b, ori_b, ori_b, ori_b, ori_v, ori_b, ori_v, ori_b, ori_b, ori_v, ori_b, ori_b,
  ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_b, ori_b, ori_b, ori_b,
  ori_h, ori_h, ori_h, ori_h, ori_h, ori_h, ori_h, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p,
  ori_b, ori_b, ori_b, ori_v, ori_b, ori_b, ori_b, ori_b, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_h, ori_h, ori_v, ori_v,
  ori_v, ori_v, ori_v, ori_b, ori_v, ori_p, ori_p, ori_p, ori_b, ori_b, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_v, ori_b, ori_v, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_v, ori_v,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_v, ori_b, ori_p, ori_p, ori_p, ori_b, ori_b, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_p, ori_v, ori_v, ori_b, ori_b, ori_b, ori_p, ori_p, ori_p, ori_p, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_v, ori_v, ori_v, ori_v, ori_p, ori_p, ori_p, ori_p
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern void Begin3DMode();

GLint prt_attrib_open;
GLint prt_attrib_close;


void get_vectors( Uint16 prt, vect3 * vert, vect3 * horiz, float * dist )
{
  CHR_REF chr;
  Uint16 pip;
  float cossize, sinsize;
  Uint16 rotate;
  vect3 vector_right, vector_up;
  vect3 vec1, vec2;
  vect3 vect_out;

  PRT_ORI ori;
  Uint32 image;

  if ( !VALID_PRT( prt ) ) return;

  assert( NULL != vert  );
  assert( NULL != horiz );
  assert( NULL != dist  );

  chr = prt_get_attachedtochr( prt );
  pip = PrtList[prt].pip;

  rotate = 0;
  image = FP8_TO_INT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );
  ori = particle_orientation[image];

  // if the velocity is zero, convert the projectile to a billboard
  if (( ori == ori_p ) && ABS( PrtList[prt].vel.x ) + ABS( PrtList[prt].vel.y ) + ABS( PrtList[prt].vel.z ) < 0.1 )
  {
    ori = ori_b;
    rotate += 16384;
  }

  vec1.x = 0;
  vec1.y = 0;
  vec1.z = 1;

  vec2.x = GCamera.pos.x - PrtList[prt].pos.x;
  vec2.y = GCamera.pos.y - PrtList[prt].pos.y;
  vec2.z = GCamera.pos.z - PrtList[prt].pos.z;

  vect_out.x = GCamera.mView.v[ 2];
  vect_out.y = GCamera.mView.v[ 6];
  vect_out.z = GCamera.mView.v[10];

  *dist = DotProduct(vect_out, vec2);

  switch ( ori )
  {
      // the particle is standing like a tree
    case ori_v:
      {
        vector_right = Normalize( CrossProduct( vec1, vec2 ) );
        vector_up    = vec1;

        rotate += PrtList[prt].rotate + 8192;
      };
      break;

      // the particle is lying like a coin
    case ori_h:
      {
        vector_right = Normalize( CrossProduct( vec1, vec2 ) );
        vector_up    = Normalize( CrossProduct( vec1, vector_right ) );

        rotate += PrtList[prt].rotate - 24576;
      };
      break;

      // the particle is flying through the air (projectile motion)
    case ori_p:
      {
        vect3 vec_vel, vec1, vec2, vec3;

        CHR_REF prt_target = prt_get_target( prt );

        if ( ABS( PrtList[prt].vel.x ) + ABS( PrtList[prt].vel.y ) + ABS( PrtList[prt].vel.z ) > 0.0f )
        {
          vec_vel.x = PrtList[prt].vel.x;
          vec_vel.y = PrtList[prt].vel.y;
          vec_vel.z = PrtList[prt].vel.z;
        }
        else if ( VALID_CHR( prt_target ) && VALID_CHR( prt_target ) )
        {
          vec_vel.x = ChrList[prt_target].pos.x - PrtList[prt].pos.x;
          vec_vel.y = ChrList[prt_target].pos.y - PrtList[prt].pos.y;
          vec_vel.z = ChrList[prt_target].pos.z - PrtList[prt].pos.z;
        }
        else
        {
          vec_vel.x = 0;
          vec_vel.y = 0;
          vec_vel.z = 1;
        };

        vec2.x = GCamera.pos.x - PrtList[prt].pos.x;
        vec2.y = GCamera.pos.y - PrtList[prt].pos.y;
        vec2.z = GCamera.pos.z - PrtList[prt].pos.z;

        vec1 = Normalize( vec_vel );
        vec3 = CrossProduct( vec1, Normalize( vec2 ) );

        vector_right = vec3;
        vector_up    = vec1;

        rotate += PrtList[prt].rotate - 8192;
      }
      break;

      // normal billboarded particle
    default:
    case ori_b:
      {
        // this is the simple billboard
        //vector_right.x = GCamera.mView.v[0];
        //vector_right.y = GCamera.mView.v[4];
        //vector_right.z = GCamera.mView.v[8];

        //vector_up.x = GCamera.mView.v[1];
        //vector_up.y = GCamera.mView.v[5];
        //vector_up.z = GCamera.mView.v[9];

        vector_right = Normalize( CrossProduct( vec1, vec2 ) );
        vector_up    = Normalize( CrossProduct( vector_right, vec2 ) );

        rotate += PrtList[prt].rotate - 24576;
      };

      break;
  };

  if ( VALID_CHR( chr ) && PipList[pip].rotatewithattached )
  {
    rotate += ChrList[chr].turn_lr;
  }

  rotate >>= 2;
  sinsize = turntosin[rotate & TRIGTABLE_MASK];
  cossize = turntosin[( rotate+TRIGTABLE_SHIFT ) & TRIGTABLE_MASK];

  ( *horiz ).x = cossize * vector_right.x - sinsize * vector_up.x;
  ( *horiz ).y = cossize * vector_right.y - sinsize * vector_up.y;
  ( *horiz ).z = cossize * vector_right.z - sinsize * vector_up.z;

  ( *vert ).x  = sinsize * vector_right.x + cossize * vector_up.x;
  ( *vert ).y  = sinsize * vector_right.y + cossize * vector_up.y;
  ( *vert ).z  = sinsize * vector_right.z + cossize * vector_up.z;
};


//--------------------------------------------------------------------------------------------
void render_antialias_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_antialias_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Render each particle that was on

    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Render solid ones twice...  For Antialias
      if ( PrtList[prt].type != PRTTYPE_SOLID ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( PrtList[prt].lightr_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightg_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightb_fp8 ), FP8_TO_FLOAT( antialiastrans_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.25f * 1.1f;  // [claforte] Fudge the value.

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

        vtlist[0].tx.s = CALCULATE_PRT_U0( image );
        vtlist[0].tx.t = CALCULATE_PRT_V0( image );

        vtlist[1].tx.s = CALCULATE_PRT_U1( image );
        vtlist[1].tx.t = CALCULATE_PRT_V0( image );

        vtlist[2].tx.s = CALCULATE_PRT_U1( image );
        vtlist[2].tx.t = CALCULATE_PRT_V1( image );

        vtlist[3].tx.s = CALCULATE_PRT_U0( image );
        vtlist[3].tx.t = CALCULATE_PRT_V1( image );

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );  //[claforte] should use alpha_component instead of 0.5?
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();

      }
    }
  }
  ATTRIB_POP( "render_antialias_prt" );
};

//--------------------------------------------------------------------------------------------
void render_solid_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_solid_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // Render each particle that was on
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Draw sprites this round
      if ( PrtList[prt].type != PRTTYPE_SOLID ) continue;

      {
        GLvector color_component = { FP8_TO_FLOAT( PrtList[prt].lightr_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightg_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightb_fp8 ), 1};

        // [claforte] Fudge the value.
        size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.25f;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

        vtlist[0].tx.s = CALCULATE_PRT_U0( image );
        vtlist[0].tx.t = CALCULATE_PRT_V0( image );

        vtlist[1].tx.s = CALCULATE_PRT_U1( image );
        vtlist[1].tx.t = CALCULATE_PRT_V0( image );

        vtlist[2].tx.s = CALCULATE_PRT_U1( image );
        vtlist[2].tx.t = CALCULATE_PRT_V1( image );

        vtlist[3].tx.s = CALCULATE_PRT_U0( image );
        vtlist[3].tx.t = CALCULATE_PRT_V1( image );

        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};
//--------------------------------------------------------------------------------------------
void render_transparent_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_transparent_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Draw transparent sprites this round
      if ( PrtList[prt].type != PRTTYPE_ALPHA ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( PrtList[prt].lightr_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightg_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightb_fp8 ), FP8_TO_FLOAT( PrtList[prt].alpha_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.5f;  // [claforte] Fudge the value.

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

        vtlist[0].tx.s = CALCULATE_PRT_U0( image );
        vtlist[0].tx.t = CALCULATE_PRT_V0( image );

        vtlist[1].tx.s = CALCULATE_PRT_U1( image );
        vtlist[1].tx.t = CALCULATE_PRT_V0( image );

        vtlist[2].tx.s = CALCULATE_PRT_U1( image );
        vtlist[2].tx.t = CALCULATE_PRT_V1( image );

        vtlist[3].tx.s = CALCULATE_PRT_U0( image );
        vtlist[3].tx.t = CALCULATE_PRT_V1( image );

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );  //[claforte] should use alpha_component instead of 0.5?
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v  );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_light_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_light_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );




    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Draw lights this round
      if ( PrtList[prt].type != PRTTYPE_LIGHT ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( PrtList[prt].alpha_fp8 ), FP8_TO_FLOAT( PrtList[prt].alpha_fp8 ), FP8_TO_FLOAT( PrtList[prt].alpha_fp8 ), 1.0f};

        // [claforte] Fudge the value.
        size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.5f;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

        vtlist[0].tx.s = CALCULATE_PRT_U0( image );
        vtlist[0].tx.t = CALCULATE_PRT_V0( image );

        vtlist[1].tx.s = CALCULATE_PRT_U1( image );
        vtlist[1].tx.t = CALCULATE_PRT_V0( image );

        vtlist[2].tx.s = CALCULATE_PRT_U1( image );
        vtlist[2].tx.t = CALCULATE_PRT_V1( image );

        vtlist[3].tx.s = CALCULATE_PRT_U0( image );
        vtlist[3].tx.t = CALCULATE_PRT_V1( image );

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};


int cmp_particle_vertices( const void * pleft, const void * pright )
{
  // do a distance based comparison

  GLVertex *vleft = (GLVertex *)pleft, *vright = (GLVertex *)pright;

  if( NULL == vleft || NULL == vright ) return 0;

  if( vleft->col.r < vright->col.r )
  {
    return -1;
  }
  else if( vleft->col.r > vright->col.r )
  {
    return 1;
  }
  else
  {
    return 0;
  }

};

//--------------------------------------------------------------------------------------------
void sort_particles( GLVertex v[], int numparticle )
{
  // do the in-place quick sort

  if(NULL == v || 0 == numparticle) return;

  qsort(v, numparticle, sizeof(GLVertex), cmp_particle_vertices);
}

//--------------------------------------------------------------------------------------------
void render_particles()
{
  // ZZ> This function draws the sprites for particle systems

  GLVertex v[MAXPRT];
  Uint16 cnt, numparticle;

  if ( INVALID_TEXTURE == GLTexture_GetTextureID( &TxTexture[particletexture] ) ) return;

  // Original points
  numparticle = 0;
  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    if ( !VALID_PRT( cnt ) || !PrtList[cnt].inview || PrtList[cnt].gopoof || PrtList[cnt].size_fp8 == 0 ) continue;

    v[numparticle].pos.x = ( float ) PrtList[cnt].pos.x;
    v[numparticle].pos.y = ( float ) PrtList[cnt].pos.y;
    v[numparticle].pos.z = ( float ) PrtList[cnt].pos.z;

    // !!!!!PRE CALCULATE the billboard vectors so you only have to do it ONCE!!!!!!!
    get_vectors( cnt, &v[numparticle].up, &v[numparticle].rt, &v[numparticle].col.r );

    // [claforte] Aaron did a horrible hack here. Fix that ASAP.
    v[numparticle].color = cnt;  // Store an index in the color slot...
    numparticle++;
  }

  if ( 0 == numparticle ) return;

  // sort particles by distance
  sort_particles( v, numparticle );

  ATTRIB_PUSH( "render_particles", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT );
  {
    // Flat shade these babies
    glShadeModel( CData.shading );

    glDisable( GL_CULL_FACE );
    glDisable( GL_DITHER );

    // Choose texture
    GLTexture_Bind( &TxTexture[particletexture], CData.texturefilter );

    // DO ANTIALIAS SOLID SPRITES FIRST
    render_antialias_prt( numparticle, v );

    // DO SOLID SPRITES FIRST
    render_solid_prt( numparticle, v );

    // LIGHTS DONE LAST
    render_light_prt( numparticle, v );

    // DO TRANSPARENT SPRITES NEXT
    render_transparent_prt( numparticle, v );
  }
  glPopAttrib();

};

//--------------------------------------------------------------------------------------------
void render_antialias_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_antialias_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Render each particle that was on
    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Render solid ones twice...  For Antialias
      if ( PrtList[prt].type != PRTTYPE_SOLID ) continue;

      {

        GLvector color_component = {FP8_TO_FLOAT( PrtList[prt].lightr_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightg_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightb_fp8 ), FP8_TO_FLOAT( antialiastrans_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.25f * 1.1f;  // [claforte] Fudge the value.

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

        vtlist[0].tx.s = CALCULATE_PRT_U0( image );
        vtlist[0].tx.t = CALCULATE_PRT_V0( image );

        vtlist[1].tx.s = CALCULATE_PRT_U1( image );
        vtlist[1].tx.t = CALCULATE_PRT_V0( image );

        vtlist[2].tx.s = CALCULATE_PRT_U1( image );
        vtlist[2].tx.t = CALCULATE_PRT_V1( image );

        vtlist[3].tx.s = CALCULATE_PRT_U0( image );
        vtlist[3].tx.t = CALCULATE_PRT_V1( image );

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_solid_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_solid_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // Render each particle that was on
    glDepthMask( GL_TRUE );
    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Draw sprites this round
      if ( PrtList[prt].type != PRTTYPE_SOLID ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( PrtList[prt].lightr_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightg_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightb_fp8 ), 1};

        // [claforte] Fudge the value.
        size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.25f;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

        vtlist[0].tx.s = CALCULATE_PRT_U0( image );
        vtlist[0].tx.t = CALCULATE_PRT_V0( image );

        vtlist[1].tx.s = CALCULATE_PRT_U1( image );
        vtlist[1].tx.t = CALCULATE_PRT_V0( image );

        vtlist[2].tx.s = CALCULATE_PRT_U1( image );
        vtlist[2].tx.t = CALCULATE_PRT_V1( image );

        vtlist[3].tx.s = CALCULATE_PRT_U0( image );
        vtlist[3].tx.t = CALCULATE_PRT_V1( image );

        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_transparent_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_transparent_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Draw transparent sprites this round
      if ( PrtList[prt].type != PRTTYPE_ALPHA ) continue;

      {
        GLvector color_component = { FP8_TO_FLOAT( PrtList[prt].lightr_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightg_fp8 ), FP8_TO_FLOAT( PrtList[prt].lightb_fp8 ), FP8_TO_FLOAT( PrtList[prt].alpha_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.25f;  // [claforte] Fudge the value.

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
        vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
        vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

        vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
        vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
        vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

        vtlist[0].tx.s = CALCULATE_PRT_U0( image );
        vtlist[0].tx.t = CALCULATE_PRT_V0( image );

        vtlist[1].tx.s = CALCULATE_PRT_U1( image );
        vtlist[1].tx.t = CALCULATE_PRT_V0( image );

        vtlist[2].tx.s = CALCULATE_PRT_U1( image );
        vtlist[2].tx.t = CALCULATE_PRT_V1( image );

        vtlist[3].tx.s = CALCULATE_PRT_U0( image );
        vtlist[3].tx.t = CALCULATE_PRT_V1( image );

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );  //[claforte] should use alpha_component instead of 0.5?
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }

    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_light_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  GLVertex vtlist[4];
  Uint16 cnt, prt, chr, pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_light_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      GLvector color_component = { FP8_TO_FLOAT( PrtList[cnt].alpha_fp8 ), FP8_TO_FLOAT( PrtList[cnt].alpha_fp8 ), FP8_TO_FLOAT( PrtList[cnt].alpha_fp8 ), 1.0f};

      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( prt );
      pip = PrtList[prt].pip;

      // Draw lights this round
      if ( PrtList[prt].type != PRTTYPE_LIGHT ) continue;

      // [claforte] Fudge the value.
      size = FP8_TO_FLOAT( PrtList[prt].size_fp8 ) * 0.5f;

      // Calculate the position of the four corners of the billboard
      // used to display the particle.
      vtlist[0].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
      vtlist[0].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
      vtlist[0].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

      vtlist[1].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x - vrtlist[cnt].up.x ) * size );
      vtlist[1].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y - vrtlist[cnt].up.y ) * size );
      vtlist[1].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z - vrtlist[cnt].up.z ) * size );

      vtlist[2].pos.x = vrtlist[cnt].pos.x + (( vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
      vtlist[2].pos.y = vrtlist[cnt].pos.y + (( vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
      vtlist[2].pos.z = vrtlist[cnt].pos.z + (( vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

      vtlist[3].pos.x = vrtlist[cnt].pos.x + (( -vrtlist[cnt].rt.x + vrtlist[cnt].up.x ) * size );
      vtlist[3].pos.y = vrtlist[cnt].pos.y + (( -vrtlist[cnt].rt.y + vrtlist[cnt].up.y ) * size );
      vtlist[3].pos.z = vrtlist[cnt].pos.z + (( -vrtlist[cnt].rt.z + vrtlist[cnt].up.z ) * size );

      // Fill in the rest of the data
      image = FP8_TO_FLOAT( PrtList[prt].image_fp8 + PrtList[prt].imagestt_fp8 );

      vtlist[0].tx.s = CALCULATE_PRT_U0( image );
      vtlist[0].tx.t = CALCULATE_PRT_V0( image );

      vtlist[1].tx.s = CALCULATE_PRT_U1( image );
      vtlist[1].tx.t = CALCULATE_PRT_V0( image );

      vtlist[2].tx.s = CALCULATE_PRT_U1( image );
      vtlist[2].tx.t = CALCULATE_PRT_V1( image );

      vtlist[3].tx.s = CALCULATE_PRT_U0( image );
      vtlist[3].tx.t = CALCULATE_PRT_V1( image );

      // Go on and draw it
      glBegin( GL_TRIANGLE_FAN );
      glColor4fv( color_component.v );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2fv( vtlist[i].tx._v );
        glVertex3fv( vtlist[i].pos.v );
      }
      glEnd();
    }

  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_particle_reflections()
{
  // ZZ> This function draws the sprites for particle systems

  GLVertex v[MAXPRT];
  Uint16 cnt, numparticle;
  float level;

  if ( INVALID_TEXTURE == GLTexture_GetTextureID( &TxTexture[particletexture] ) )
    return;

  // Original points
  numparticle = 0;
  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    if ( !VALID_PRT( cnt ) || !PrtList[cnt].inview || PrtList[cnt].size_fp8 == 0 ) continue;

    if ( mesh_has_some_bits( PrtList[cnt].onwhichfan, MPDFX_SHINY ) )
    {
      level = PrtList[cnt].level;
      v[numparticle].pos.x = PrtList[cnt].pos.x;
      v[numparticle].pos.y = PrtList[cnt].pos.y;
      v[numparticle].pos.z = level + level - PrtList[cnt].pos.z;

      // !!!!!PRE CALCULATE the billboard vectors so you only have to do it ONCE!!!!!!!
      get_vectors( cnt, &v[numparticle].up, &v[numparticle].rt, &v[numparticle].col.r );

      v[numparticle].color = cnt;  // Store an index in the color slot...
      numparticle++;
    }
  }

  if ( 0 == numparticle ) return;

  // sort the particles by distance
  sort_particles( v, numparticle );


  ATTRIB_PUSH( "render_particle_reflections", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT );
  {
    // Flat shade these babies
    glShadeModel( CData.shading );

    // Choose texture and matrix
    GLTexture_Bind( &TxTexture[particletexture], CData.texturefilter );

    glDisable( GL_CULL_FACE );
    glDisable( GL_DITHER );

    // DO ANTIALIAS SOLID SPRITES FIRST
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_antialias_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );

    // DO SOLID SPRITES FIRST
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_solid_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );

    // DO TRANSPARENT SPRITES NEXT
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_transparent_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );

    // LIGHTS DONE LAST
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_light_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );
  }
  glPopAttrib();

}


