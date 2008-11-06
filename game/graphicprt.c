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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "particle.h"


Uint32  particletrans  EQ( 0x80000000 );
Uint32  antialiastrans  EQ( 0xC0000000 );


extern void Begin3DMode();

void render_prt()
{
  // ZZ> This function draws the sprites for particle systems
  // D3DLVERTEX v[MAXPRT];
  // D3DTLVERTEX vt[MAXPRT];
  // D3DTLVERTEX vtlist[4];
  GLVERTEX v[MAXPRT];
  GLVERTEX vtlist[4];
  Uint16 cnt, prt, numparticle;
  Uint16 image;
//  float scale;
  float size;
  Uint32  light;
//  DWORD fogspec;
//  Uint8 red, grn, blu;
//  Uint16 rotate;
//  float sinsize, cossize;
//  float z;
  int i;

  // Calculate the up and right vectors for billboarding.
  glVector vector_up, vector_right;
  Begin3DMode();
  vector_right.x = mView.v[0];
  vector_right.y = mView.v[4];
  vector_right.z = mView.v[8];
  vector_up.x = mView.v[1];
  vector_up.y = mView.v[5];
  vector_up.z = mView.v[9];


  // Flat shade these babies
  // if(shading != GL_FLAT)
  // lpD3DDDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, GL_FLAT);

  // Original points
  numparticle = 0;
  cnt = 0;
  /*
  if(fogon)
  {
      // The full fog value
      fogspec = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);
      while(cnt < MAXPRT)
      {
          if(prtinview[cnt]&&prtsize[cnt]!=0)
          {
              v[numparticle].x = (D3DVALUE) prtxpos[cnt];
              v[numparticle].y = (D3DVALUE) prtypos[cnt];
              v[numparticle].z = (D3DVALUE) prtzpos[cnt];
              v[numparticle].color = cnt;  // Store an index in the color slot...


              // Figure out the fog coloring
              z = v[numparticle].z;
              if(z < fogtop)
              {
                  if(z < fogbottom)
                  {
                      v[numparticle].dcSpecular = fogspec;  // Full fog
                  }
                  else
                  {
                      z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f... Amount of fog to keep
                      red = (fogred * z);
                      grn = (foggrn * z);
                      blu = (fogblu * z);
                      light = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                      v[numparticle].dcSpecular = light;
                  }
              }
              else
              {
                  v[numparticle].dcSpecular = 0;  // No fog
              }


              v[numparticle].dwReserved = 0;
              numparticle++;
          }
          cnt++;
      }
  }
  else
  */
  {
    while ( cnt < MAXPRT )
    {
      if ( prtinview[cnt] && prtsize[cnt] != 0 )
      {
        v[numparticle].x = ( float ) prtxpos[cnt];
        v[numparticle].y = ( float ) prtypos[cnt];
        v[numparticle].z = ( float ) prtzpos[cnt];

        // [claforte] Aaron did a horrible hack here. Fix that ASAP.
        v[numparticle].color = cnt;  // Store an index in the color slot...
        // v[numparticle].dcSpecular = 0;  // No fog
        // v[numparticle].dwReserved = 0;
        numparticle++;
      }
      cnt++;
    }
  }

  // Choose texture and matrix
  // lpD3DDDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, txTexture[particletexture].GetHandle());
  glBindTexture ( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[particletexture] ) );
  // lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
  // Make new ones so we can index them and not transform 'em each time
  // transform_vertices(numparticle, v, vt);


  glDisable( GL_CULL_FACE );
  glDisable( GL_DITHER );

  // DO SOLID SPRITES FIRST
  // Render each particle that was on
  cnt = 0;
  while ( cnt < numparticle )
  {
    // Get the index from the color slot
    prt = ( Uint16 ) v[cnt].color;

    // Draw sprites this round
    if ( prttype[prt] == PRTSOLIDSPRITE )
    {
      float color_component = prtlight[prt] / 255.0f;
      light = ( 0xff000000 ) | ( prtlight[prt] << 16 ) | ( prtlight[prt] << 8 ) | ( prtlight[prt] );
      glColor4f( color_component, color_component, color_component, 1.0f );

      // [claforte] Fudge the value.
      size = ( float )( prtsize[prt] ) * 0.00092f;

      // Calculate the position of the four corners of the billboard
      // used to display the particle.
      vtlist[0].x = v[cnt].x + ( ( -vector_right.x - vector_up.x ) * size );
      vtlist[0].y = v[cnt].y + ( ( -vector_right.y - vector_up.y ) * size );
      vtlist[0].z = v[cnt].z + ( ( -vector_right.z - vector_up.z ) * size );
      vtlist[1].x = v[cnt].x + ( ( vector_right.x - vector_up.x ) * size );
      vtlist[1].y = v[cnt].y + ( ( vector_right.y - vector_up.y ) * size );
      vtlist[1].z = v[cnt].z + ( ( vector_right.z - vector_up.z ) * size );
      vtlist[2].x = v[cnt].x + ( ( vector_right.x + vector_up.x ) * size );
      vtlist[2].y = v[cnt].y + ( ( vector_right.y + vector_up.y ) * size );
      vtlist[2].z = v[cnt].z + ( ( vector_right.z + vector_up.z ) * size );
      vtlist[3].x = v[cnt].x + ( ( -vector_right.x + vector_up.x ) * size );
      vtlist[3].y = v[cnt].y + ( ( -vector_right.y + vector_up.y ) * size );
      vtlist[3].z = v[cnt].z + ( ( -vector_right.z + vector_up.z ) * size );

      // Fill in the rest of the data
      image = ( ( Uint16 )( prtimage[prt] + prtimagestt[prt] ) ) >> 8;

      // vtlist[0].dcColor = light;
      // vtlist[0].dcSpecular = vt[cnt].dcSpecular;
      vtlist[0].s = particleimageu[image][0];
      vtlist[0].t = particleimagev[image][0];

      // vtlist[1].dcColor = light;
      // vtlist[1].dcSpecular = vt[cnt].dcSpecular;
      vtlist[1].s = particleimageu[image][1];
      vtlist[1].t = particleimagev[image][0];

      // vtlist[2].dcColor = light;
      // vtlist[2].dcSpecular = vt[cnt].dcSpecular;
      vtlist[2].s = particleimageu[image][1];
      vtlist[2].t = particleimagev[image][1];

      // vtlist[3].dcColor = light;
      // vtlist[3].dcSpecular = vt[cnt].dcSpecular;
      vtlist[3].s = particleimageu[image][0];
      vtlist[3].t = particleimagev[image][1];

      glBegin( GL_TRIANGLE_FAN );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2f ( vtlist[i].s, vtlist[i].t );
        glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
      }
      glEnd();
    }
    cnt++;
  }

  // DO TRANSPARENT SPRITES NEXT
  glDepthMask( GL_FALSE );
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


  // Render each particle that was on
  cnt = 0;
  while ( cnt < numparticle )
  {
    // Get the index from the color slot
    prt = ( Uint16 ) v[cnt].color;

    // Draw transparent sprites this round
    if ( prttype[prt] != PRTLIGHTSPRITE )  // Render solid ones twice...  For Antialias
    {
      float color_component = prtlight[prt] / 255.0f;
      float alpha_component;

      // Figure out the sprite's size based on distance
      if ( prttype[prt] == PRTSOLIDSPRITE )
        alpha_component = antialiastrans / 255.0f;
      else
        alpha_component = particletrans / 255.0f;

      glColor4f( color_component, color_component, color_component, 0.5f );//[claforte] should use alpha_component instead of 0.5f?

      // [claforte] Fudge the value.
      // REMOVE:size = scale / 850.0f;
      size = ( float )prtsize[prt] * 0.000917f;

      if ( prttype[prt] == PRTSOLIDSPRITE )
        size += 2.0f; // for antialiasing.

      // Calculate the position of the four corners of the billboard
      // used to display the particle.
      vtlist[0].x = v[cnt].x + ( ( -vector_right.x - vector_up.x ) * size );
      vtlist[0].y = v[cnt].y + ( ( -vector_right.y - vector_up.y ) * size );
      vtlist[0].z = v[cnt].z + ( ( -vector_right.z - vector_up.z ) * size );
      vtlist[1].x = v[cnt].x + ( ( vector_right.x - vector_up.x ) * size );
      vtlist[1].y = v[cnt].y + ( ( vector_right.y - vector_up.y ) * size );
      vtlist[1].z = v[cnt].z + ( ( vector_right.z - vector_up.z ) * size );
      vtlist[2].x = v[cnt].x + ( ( vector_right.x + vector_up.x ) * size );
      vtlist[2].y = v[cnt].y + ( ( vector_right.y + vector_up.y ) * size );
      vtlist[2].z = v[cnt].z + ( ( vector_right.z + vector_up.z ) * size );
      vtlist[3].x = v[cnt].x + ( ( -vector_right.x + vector_up.x ) * size );
      vtlist[3].y = v[cnt].y + ( ( -vector_right.y + vector_up.y ) * size );
      vtlist[3].z = v[cnt].z + ( ( -vector_right.z + vector_up.z ) * size );

      // Fill in the rest of the data
      image = ( ( Uint16 )( prtimage[prt] + prtimagestt[prt] ) ) >> 8;
      light = ( 0xff000000 ) | ( prtlight[prt] << 16 ) | ( prtlight[prt] << 8 ) | ( prtlight[prt] );

      // vtlist[0].dcColor = light;
      // vtlist[0].dcSpecular = vt[cnt].dcSpecular;
      vtlist[0].s = particleimageu[image][0];
      vtlist[0].t = particleimagev[image][0];

      // vtlist[1].dcColor = light;
      // vtlist[1].dcSpecular = vt[cnt].dcSpecular;
      vtlist[1].s = particleimageu[image][1];
      vtlist[1].t = particleimagev[image][0];

      // vtlist[2].dcColor = light;
      // vtlist[2].dcSpecular = vt[cnt].dcSpecular;
      vtlist[2].s = particleimageu[image][1];
      vtlist[2].t = particleimagev[image][1];

      // vtlist[3].dcColor = light;
      // vtlist[3].dcSpecular = vt[cnt].dcSpecular;
      vtlist[3].s = particleimageu[image][0];
      vtlist[3].t = particleimagev[image][1];

      // Go on and draw it
      glBegin( GL_TRIANGLE_FAN );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2f ( vtlist[i].s, vtlist[i].t );
        glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
      }
      glEnd();
    }
    cnt++;
  }

  // GLASS DONE LAST
  glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
  glBlendFunc( GL_ONE, GL_ONE );

  // Render each particle that was on
  cnt = 0;
  while ( cnt < numparticle )
  {
    // Get the index from the color slot
    prt = ( Uint16 ) v[cnt].color;

    // Draw lights this round
    if ( prttype[prt] == PRTLIGHTSPRITE )
    {
      // [claforte] Fudge the value.
      size = ( float )prtsize[prt] * 0.00156f;

      // Calculate the position of the four corners of the billboard
      // used to display the particle.
      vtlist[0].x = v[cnt].x + ( ( -vector_right.x - vector_up.x ) * size );
      vtlist[0].y = v[cnt].y + ( ( -vector_right.y - vector_up.y ) * size );
      vtlist[0].z = v[cnt].z + ( ( -vector_right.z - vector_up.z ) * size );
      vtlist[1].x = v[cnt].x + ( ( vector_right.x - vector_up.x ) * size );
      vtlist[1].y = v[cnt].y + ( ( vector_right.y - vector_up.y ) * size );
      vtlist[1].z = v[cnt].z + ( ( vector_right.z - vector_up.z ) * size );
      vtlist[2].x = v[cnt].x + ( ( vector_right.x + vector_up.x ) * size );
      vtlist[2].y = v[cnt].y + ( ( vector_right.y + vector_up.y ) * size );
      vtlist[2].z = v[cnt].z + ( ( vector_right.z + vector_up.z ) * size );
      vtlist[3].x = v[cnt].x + ( ( -vector_right.x + vector_up.x ) * size );
      vtlist[3].y = v[cnt].y + ( ( -vector_right.y + vector_up.y ) * size );
      vtlist[3].z = v[cnt].z + ( ( -vector_right.z + vector_up.z ) * size );


      // Fill in the rest of the data
      image = ( ( Uint16 )( prtimage[prt] + prtimagestt[prt] ) ) >> 8;

      // vtlist[0].dcColor = light;
      // vtlist[0].dcSpecular = vt[cnt].dcSpecular;
      vtlist[0].s = particleimageu[image][0];
      vtlist[0].t = particleimagev[image][0];

      // vtlist[1].dcColor = light;
      // vtlist[1].dcSpecular = vt[cnt].dcSpecular;
      vtlist[1].s = particleimageu[image][1];
      vtlist[1].t = particleimagev[image][0];

      // vtlist[2].dcColor = light;
      // vtlist[2].dcSpecular = vt[cnt].dcSpecular;
      vtlist[2].s = particleimageu[image][1];
      vtlist[2].t = particleimagev[image][1];

      // vtlist[3].dcColor = light;
      // vtlist[3].dcSpecular = vt[cnt].dcSpecular;
      vtlist[3].s = particleimageu[image][0];
      vtlist[3].t = particleimagev[image][1];

      // Go on and draw it
      glBegin( GL_TRIANGLE_FAN );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2f ( vtlist[i].s, vtlist[i].t );
        glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
      }
      glEnd();
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void render_refprt()
{
  // ZZ> This function draws sprites reflected in the floor
  GLVERTEX v[MAXPRT];
  GLVERTEX vtlist[4];
  Uint16 cnt, prt, numparticle;
  Uint16 image;
//  float scale;
  float size;
//  DWORD light;
  int startalpha;
  // Uint32  usealpha = 0x00ffffff;
  int level;
//  Uint16 rotate;
//  float sinsize, cossize;
  int i;

  // Calculate the up and right vectors for billboarding.
  glVector vector_up, vector_right;
  vector_right.x = mView.v[0];
  vector_right.y = mView.v[4];
  vector_right.z = mView.v[8];
  vector_up.x = mView.v[1];
  vector_up.y = mView.v[5];
  vector_up.z = mView.v[9];

  // Original points
  numparticle = 0;
  cnt = 0;
  while ( cnt < MAXPRT )
  {
    if ( prtinview[cnt] && prtsize[cnt] != 0 )
    {
      if ( meshfx[prtonwhichfan[cnt]] & MESHFXDRAWREF )
      {
        level = prtlevel[cnt];
        v[numparticle].x = ( float ) prtxpos[cnt];
        v[numparticle].y = ( float ) prtypos[cnt];
        v[numparticle].z = ( float ) - prtzpos[cnt] + level + level;
        if ( prtattachedtocharacter[cnt] != MAXCHR )
        {
          v[numparticle].z += RAISE + RAISE;
        }
        v[numparticle].color = cnt;  // Store an index in the color slot...
        numparticle++;
      }
    }
    cnt++;
  }

  // Choose texture.
  glBindTexture ( GL_TEXTURE_2D, GLTexture_GetTextureID( &txTexture[particletexture] ) );

  glDisable( GL_CULL_FACE );
  glDisable( GL_DITHER );

  // Render each particle that was on
  cnt = 0;
  while ( cnt < numparticle )
  {
    // Get the index from the color slot
    prt = ( Uint16 ) v[cnt].color;

    // Draw lights this round
    if ( prttype[prt] == PRTLIGHTSPRITE )
    {
      size = ( float )( prtsize[prt] ) * 0.00156f; // 0.00092f;

      // Calculate the position of the four corners of the billboard
      // used to display the particle.
      vtlist[0].x = v[cnt].x + ( ( -vector_right.x - vector_up.x ) * size );
      vtlist[0].y = v[cnt].y + ( ( -vector_right.y - vector_up.y ) * size );
      vtlist[0].z = v[cnt].z + ( ( -vector_right.z - vector_up.z ) * size );
      vtlist[1].x = v[cnt].x + ( ( vector_right.x - vector_up.x ) * size );
      vtlist[1].y = v[cnt].y + ( ( vector_right.y - vector_up.y ) * size );
      vtlist[1].z = v[cnt].z + ( ( vector_right.z - vector_up.z ) * size );
      vtlist[2].x = v[cnt].x + ( ( vector_right.x + vector_up.x ) * size );
      vtlist[2].y = v[cnt].y + ( ( vector_right.y + vector_up.y ) * size );
      vtlist[2].z = v[cnt].z + ( ( vector_right.z + vector_up.z ) * size );
      vtlist[3].x = v[cnt].x + ( ( -vector_right.x + vector_up.x ) * size );
      vtlist[3].y = v[cnt].y + ( ( -vector_right.y + vector_up.y ) * size );
      vtlist[3].z = v[cnt].z + ( ( -vector_right.z + vector_up.z ) * size );

      // Fill in the rest of the data
      startalpha = ( int )( 255 + v[cnt].z - level );
      if ( startalpha < 0 ) startalpha = 0;
      startalpha = ( startalpha | reffadeor ) >> 1;  // Fix for Riva owners
      if ( startalpha > 255 ) startalpha = 255;
      if ( startalpha > 0 )
      {
        image = ( ( Uint16 )( prtimage[prt] + prtimagestt[prt] ) ) >> 8;
        // light = (startalpha<<24)|usealpha;
        glColor4f( 1.0f, 1.0f, 1.0f, startalpha / 255.0f );

        // vtlist[0].dcSpecular = vt[cnt].dcSpecular;
        vtlist[0].s = particleimageu[image][1];
        vtlist[0].t = particleimagev[image][1];

        // vtlist[1].dcSpecular = vt[cnt].dcSpecular;
        vtlist[1].s = particleimageu[image][0];
        vtlist[1].t = particleimagev[image][1];

        // vtlist[2].dcSpecular = vt[cnt].dcSpecular;
        vtlist[2].s = particleimageu[image][0];
        vtlist[2].t = particleimagev[image][0];

        // vtlist[3].dcSpecular = vt[cnt].dcSpecular;
        vtlist[3].s = particleimageu[image][1];
        vtlist[3].t = particleimagev[image][0];

        glBegin( GL_TRIANGLE_FAN );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv ( &vtlist[i].s );
          glVertex3fv ( &vtlist[i].x );
        }
        glEnd();

      }
    }
    cnt++;
  }

  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


  // Render each particle that was on
  cnt = 0;
  while ( cnt < numparticle )
  {
    // Get the index from the color slot
    prt = ( Uint16 ) v[cnt].color;

    // Draw solid and transparent sprites this round
    if ( prttype[prt] != PRTLIGHTSPRITE )
    {
      // Figure out the sprite's size based on distance
      size = ( float )( prtsize[prt] ) * 0.00156f; // 0.00092f;

      // Calculate the position of the four corners of the billboard
      // used to display the particle.
      vtlist[0].x = v[cnt].x + ( ( -vector_right.x - vector_up.x ) * size );
      vtlist[0].y = v[cnt].y + ( ( -vector_right.y - vector_up.y ) * size );
      vtlist[0].z = v[cnt].z + ( ( -vector_right.z - vector_up.z ) * size );
      vtlist[1].x = v[cnt].x + ( ( vector_right.x - vector_up.x ) * size );
      vtlist[1].y = v[cnt].y + ( ( vector_right.y - vector_up.y ) * size );
      vtlist[1].z = v[cnt].z + ( ( vector_right.z - vector_up.z ) * size );
      vtlist[2].x = v[cnt].x + ( ( vector_right.x + vector_up.x ) * size );
      vtlist[2].y = v[cnt].y + ( ( vector_right.y + vector_up.y ) * size );
      vtlist[2].z = v[cnt].z + ( ( vector_right.z + vector_up.z ) * size );
      vtlist[3].x = v[cnt].x + ( ( -vector_right.x + vector_up.x ) * size );
      vtlist[3].y = v[cnt].y + ( ( -vector_right.y + vector_up.y ) * size );
      vtlist[3].z = v[cnt].z + ( ( -vector_right.z + vector_up.z ) * size );

      // Fill in the rest of the data
      startalpha = ( int )( 255 + v[cnt].z - level );
      if ( startalpha < 0 ) startalpha = 0;
      startalpha = ( startalpha | reffadeor ) >> ( 1 + prttype[prt] );  // Fix for Riva owners
      if ( startalpha > 255 ) startalpha = 255;
      if ( startalpha > 0 )
      {
        float color_component = prtlight[prt] / 16.0f;
        image = ( ( Uint16 )( prtimage[prt] + prtimagestt[prt] ) ) >> 8;
        glColor4f( color_component, color_component, color_component, startalpha / 255.0f );

        // vtlist[0].dcSpecular = vt[cnt].dcSpecular;
        vtlist[0].s = particleimageu[image][1];
        vtlist[0].t = particleimagev[image][1];

        // vtlist[1].dcSpecular = vt[cnt].dcSpecular;
        vtlist[1].s = particleimageu[image][0];
        vtlist[1].t = particleimagev[image][1];

        // vtlist[2].dcSpecular = vt[cnt].dcSpecular;
        vtlist[2].s = particleimageu[image][0];
        vtlist[2].t = particleimagev[image][0];

        // vtlist[3].dcSpecular = vt[cnt].dcSpecular;
        vtlist[3].s = particleimageu[image][1];
        vtlist[3].t = particleimagev[image][0];

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv ( &vtlist[i].s );
          glVertex3fv ( &vtlist[i].x );
        }
        glEnd();
      }
    }
    cnt++;
  }
}

