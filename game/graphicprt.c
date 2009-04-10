//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - graphicprc.c
 * Particle system drawing and management code.
 */

#include "egoboo.h"
#include "particle.h"
#include "graphic.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32  particletrans  EQ( 0x80 );
Uint32  antialiastrans  EQ( 0xC0 );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern void Begin3DMode();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_antialias_prt()
{
    GLVERTEX v[TOTALMAXPRT];
    GLVERTEX vtlist[4];
    Uint16 prt, pip, image, cnt, numparticle;
    Uint32  light;
    float size;
    Uint8 i;

    // Calculate the up and right vectors for billboarding.
    glVector vector_up, vector_right;
    Begin3DMode();
    vector_right.x = mView.v[0];
    vector_right.y = mView.v[4];
    vector_right.z = mView.v[8];
    vector_up.x = mView.v[1];
    vector_up.y = mView.v[5];
    vector_up.z = mView.v[9];

    // Original points
    numparticle = 0;
    cnt = 0;

    {
        while ( cnt < maxparticles )
        {
            if ( prtinview[cnt] && prtsize[cnt] != 0 )
            {
                v[numparticle].x = ( float ) prtxpos[cnt];
                v[numparticle].y = ( float ) prtypos[cnt];
                v[numparticle].z = ( float ) prtzpos[cnt];

                // [claforte] Aaron did a horrible hack here. Fix that ASAP.
                v[numparticle].color = cnt;  // Store an index in the color slot...
                numparticle++;
            }

            cnt++;
        }
    }

    {
        GLint depthfunc_save, alphafunc_save, alphablendsrc_save, alphablenddst_save, alphatestref_save  ;
        GLboolean depthmask_save, cullface_save, depthtest_save, alphatest_save, blend_save;

        GLTexture_Bind( txTexture + particletexture );

        depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
        glDepthMask( GL_FALSE );

        depthtest_save = glIsEnabled( GL_DEPTH_TEST );
        glEnable( GL_DEPTH_TEST );

        glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
        glDepthFunc( GL_LESS );

        alphatest_save = glIsEnabled( GL_ALPHA_TEST );
        glEnable( GL_ALPHA_TEST );

        glGetIntegerv( GL_ALPHA_TEST_FUNC, &alphafunc_save );
        glGetIntegerv( GL_ALPHA_TEST_REF, &alphatestref_save );
        glAlphaFunc( GL_GREATER, 0 );

        blend_save = glIsEnabled( GL_BLEND );
        glEnable( GL_BLEND );

        glGetIntegerv( GL_BLEND_SRC, &alphablendsrc_save );
        glGetIntegerv( GL_BLEND_DST, &alphablenddst_save  );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        cullface_save = glIsEnabled( GL_CULL_FACE );
        glDisable( GL_CULL_FACE );

        // Render each particle that was on
        for ( cnt = 0; cnt < maxparticles; cnt++ )
        {
            // Get the index from the color slot
            prt = ( Uint16 ) vtlist[cnt].color;
            pip = prtpip[prt];

            // Render solid ones twice...  For Antialias
            if ( prttype[prt] != PRTSOLIDSPRITE  ) continue;

            {
                float color_component = prtlight[prt] / 255.0f;
                light = ( 0xff000000 ) | ( prtlight[prt] << 16 ) | ( prtlight[prt] << 8 ) | ( prtlight[prt] );
                glColor4f( color_component, color_component, color_component, 1.0f );

                // Figure out the sprite's size based on distance
                size = FP8_TO_FLOAT( prtsize[prt] ) * 0.25f * 1.1f;  // [claforte] Fudge the value.

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
                image = FP8_TO_INT( prtimage[prt] + prtimagestt[prt] );

                vtlist[0].s = particleimageu[image][0];
                vtlist[0].t = particleimagev[image][0];

                vtlist[1].s = particleimageu[image][1];
                vtlist[1].t = particleimagev[image][0];

                vtlist[2].s = particleimageu[image][1];
                vtlist[2].t = particleimagev[image][1];

                vtlist[3].s = particleimageu[image][0];
                vtlist[3].t = particleimagev[image][1];

                // Go on and draw it
                glBegin( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        glTexCoord2f ( vtlist[i].s, vtlist[i].t );
                        glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
                    }
                }
                glEnd();

            }
        }

        //Restore values
        glDepthMask( depthmask_save );
        if (depthtest_save) glEnable( GL_DEPTH_TEST ); else glDisable( GL_DEPTH_TEST );

        glDepthFunc( depthfunc_save);
        if (alphatest_save) glEnable( GL_ALPHA_TEST ); else glDisable( GL_ALPHA_TEST );

        glAlphaFunc( alphafunc_save, alphatestref_save );
        if (blend_save) glEnable( GL_BLEND ); else glDisable( GL_BLEND );

        glBlendFunc( alphablendsrc_save, alphablenddst_save );
        if (cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
    }

}

//--------------------------------------------------------------------------------------------
void render_prt()
{
    // ZZ> This function draws the sprites for particle systems
    GLVERTEX v[TOTALMAXPRT];
    GLVERTEX vtlist[4];
    Uint16 cnt, prt, numparticle;
    Uint16 image;
    float size;
    Uint32  light;
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

    {
        while ( cnt < maxparticles )
        {
            if ( prtinview[cnt] && prtsize[cnt] != 0 )
            {
                v[numparticle].x = ( float ) prtxpos[cnt];
                v[numparticle].y = ( float ) prtypos[cnt];
                v[numparticle].z = ( float ) prtzpos[cnt];

                // [claforte] Aaron did a horrible hack here. Fix that ASAP.
                v[numparticle].color = cnt;  // Store an index in the color slot...
                numparticle++;
            }

            cnt++;
        }
    }

    //Draw antialiased sprites first
// if(antialiasing) render_antialias_prt();

    // Choose texture and matrix
    GLTexture_Bind( txTexture + particletexture );

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
            image = FP8_TO_INT( prtimage[prt] + prtimagestt[prt] ) ;

            vtlist[0].s = particleimageu[image][0];
            vtlist[0].t = particleimagev[image][0];

            vtlist[1].s = particleimageu[image][1];
            vtlist[1].t = particleimagev[image][0];

            vtlist[2].s = particleimageu[image][1];
            vtlist[2].t = particleimagev[image][1];

            vtlist[3].s = particleimageu[image][0];
            vtlist[3].t = particleimagev[image][1];

            glBegin( GL_TRIANGLE_FAN );
            {
                for ( i = 0; i < 4; i++ )
                {
                    glTexCoord2f ( vtlist[i].s, vtlist[i].t );
                    glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
                }
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

            glColor4f( color_component, color_component, color_component, alpha_component ); //[claforte] should use alpha_component instead of 0.5f?

            // [claforte] Fudge the value.
            size = ( float )prtsize[prt] * 0.00092;
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
            image = FP8_TO_INT( prtimage[prt] + prtimagestt[prt] ) ;
            light = ( 0xff000000 ) | ( prtlight[prt] << 16 ) | ( prtlight[prt] << 8 ) | ( prtlight[prt] );

            vtlist[0].s = particleimageu[image][0];
            vtlist[0].t = particleimagev[image][0];

            vtlist[1].s = particleimageu[image][1];
            vtlist[1].t = particleimagev[image][0];

            vtlist[2].s = particleimageu[image][1];
            vtlist[2].t = particleimagev[image][1];

            vtlist[3].s = particleimageu[image][0];
            vtlist[3].t = particleimagev[image][1];

            // Go on and draw it
            glBegin( GL_TRIANGLE_FAN );
            {
                for ( i = 0; i < 4; i++ )
                {
                    glTexCoord2f ( vtlist[i].s, vtlist[i].t );
                    glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
                }
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
            image = FP8_TO_INT( prtimage[prt] + prtimagestt[prt] ) ;

            vtlist[0].s = particleimageu[image][0];
            vtlist[0].t = particleimagev[image][0];

            vtlist[1].s = particleimageu[image][1];
            vtlist[1].t = particleimagev[image][0];

            vtlist[2].s = particleimageu[image][1];
            vtlist[2].t = particleimagev[image][1];

            vtlist[3].s = particleimageu[image][0];
            vtlist[3].t = particleimagev[image][1];

            // Go on and draw it
            glBegin( GL_TRIANGLE_FAN );
            {
                for ( i = 0; i < 4; i++ )
                {
                    glTexCoord2f ( vtlist[i].s, vtlist[i].t );
                    glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
                }
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
    GLVERTEX v[TOTALMAXPRT];
    GLVERTEX vtlist[4];
    Uint16 cnt, prt, numparticle;
    Uint16 image;
    float size;
    int startalpha;
    float level = 0.00f;
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

    while ( cnt < maxparticles )
    {
        if ( prtinview[cnt] && 0 != prtsize[cnt] && INVALID_TILE != prtonwhichfan[cnt] )
        {
            if ( meshfx[prtonwhichfan[cnt]] & MESHFX_DRAWREF )
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
    GLTexture_Bind( txTexture + particletexture );

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
            size = ( float )( prtsize[prt] ) * 0.00156f;

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
                image = FP8_TO_INT( prtimage[prt] + prtimagestt[prt] ) ;
                // light = (startalpha<<24)|usealpha;
                glColor4f( 1.0f, 1.0f, 1.0f, startalpha / 255.0f );

                vtlist[0].s = particleimageu[image][1];
                vtlist[0].t = particleimagev[image][1];

                vtlist[1].s = particleimageu[image][0];
                vtlist[1].t = particleimagev[image][1];

                vtlist[2].s = particleimageu[image][0];
                vtlist[2].t = particleimagev[image][0];

                vtlist[3].s = particleimageu[image][1];
                vtlist[3].t = particleimagev[image][0];

                glBegin( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        glTexCoord2fv ( &vtlist[i].s );
                        glVertex3fv ( &vtlist[i].x );
                    }
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
            startalpha = ( int )( 255 + v[cnt].z - level );
            if ( startalpha < 0 ) startalpha = 0;

            startalpha = ( startalpha | reffadeor ) >> ( 1 + prttype[prt] );  // Fix for Riva owners
            if ( startalpha > 255 ) startalpha = 255;
            if ( startalpha > 0 )
            {
                float color_component = prtlight[prt] / 16.0f;
                image = FP8_TO_INT( prtimage[prt] + prtimagestt[prt] ) ;
                glColor4f( color_component, color_component, color_component, startalpha / 255.0f );

                vtlist[0].s = particleimageu[image][1];
                vtlist[0].t = particleimagev[image][1];

                vtlist[1].s = particleimageu[image][0];
                vtlist[1].t = particleimagev[image][1];

                vtlist[2].s = particleimageu[image][0];
                vtlist[2].t = particleimagev[image][0];

                vtlist[3].s = particleimageu[image][1];
                vtlist[3].t = particleimagev[image][0];

                // Go on and draw it
                glBegin( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        glTexCoord2fv ( &vtlist[i].s );
                        glVertex3fv ( &vtlist[i].x );
                    }
                }
                glEnd();
            }
        }

        cnt++;
    }
}
