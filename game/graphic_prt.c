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

/* Egoboo - graphic_prt.c
* Particle system drawing and management code.
*/

#include "graphic.h"

#include "particle.h"
#include "char.h"

#include "camera.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_prt_registry_entity
{
    int   index;
    float dist;
};
typedef struct s_prt_registry_entity prt_registry_entity_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void do_instance_update( camera_t * pcam );
static void prt_instance_upload( camera_t * pcam, prt_instance_t * pinst, prt_t * pprt );
static void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size );
static int cmp_prt_registry_entity(const void * vlhs, const void * vrhs);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32  particletrans   = 0x80;
Uint32  antialiastrans  = 0xC0;
Uint32  instance_update = (Uint32)~0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_billboard( camera_t * pcam, GLtexture * ptex, GLvector4 pos, float scale )
{
    Begin3DMode( pcam );
    {
        int i;
        GLvertex vtlist[4];
        float x1, y1;
        GLvector4 vector_up, vector_right;

        x1 = ( float ) GLtexture_GetImageWidth( ptex )  / ( float ) GLtexture_GetTextureWidth( ptex );
        y1 = ( float ) GLtexture_GetImageHeight( ptex )  / ( float ) GLtexture_GetTextureHeight( ptex );

        vector_right.x =  pcam->mView.CNV(0, 0) * x1 * scale;
        vector_right.y =  pcam->mView.CNV(1, 0) * x1 * scale;
        vector_right.z =  pcam->mView.CNV(2, 0) * x1 * scale;

        vector_up.x    = -pcam->mView.CNV(0, 1) * y1 * scale;
        vector_up.y    = -pcam->mView.CNV(1, 1) * y1 * scale;
        vector_up.z    = -pcam->mView.CNV(2, 1) * y1 * scale;

        // bottom left
        vtlist[0].x = pos.x + ( -vector_right.x - vector_up.x );
        vtlist[0].y = pos.y + ( -vector_right.y - vector_up.y );
        vtlist[0].z = pos.z + ( -vector_right.z - vector_up.z );
        vtlist[0].s = 0;
        vtlist[0].t = 0;

        // top left
        vtlist[1].x = pos.x + ( -vector_right.x + vector_up.x );
        vtlist[1].y = pos.y + ( -vector_right.y + vector_up.y );
        vtlist[1].z = pos.z + ( -vector_right.z + vector_up.z );
        vtlist[0].s = 0;
        vtlist[0].t = y1;

        // top right
        vtlist[2].x = pos.x + ( vector_right.x + vector_up.x );
        vtlist[2].y = pos.y + ( vector_right.y + vector_up.y );
        vtlist[2].z = pos.z + ( vector_right.z + vector_up.z );
        vtlist[0].s = x1;
        vtlist[0].t = y1;

        // bottom right
        vtlist[3].x = pos.x + ( vector_right.x - vector_up.x );
        vtlist[3].y = pos.y + ( vector_right.y - vector_up.y );
        vtlist[3].z = pos.z + ( vector_right.z - vector_up.z );
        vtlist[0].s = x1;
        vtlist[0].t = 0;

        // Go on and draw it
        glBegin( GL_QUADS );
        {
            for ( i = 0; i < 4; i++ )
            {
                glTexCoord2f ( vtlist[i].s, vtlist[i].t );
                glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
            }
        }
        glEnd();

    }
    End3DMode();

}

//--------------------------------------------------------------------------------------------
int cmp_prt_registry_entity(const void * vlhs, const void * vrhs)
{
    const prt_registry_entity_t * lhs, * rhs;

    lhs = (prt_registry_entity_t *) vlhs;
    rhs = (prt_registry_entity_t *) vrhs;

    return lhs->dist - rhs->dist;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_prt( camera_t * pcam )
{
    // ZZ> This function draws the sprites for particle systems

    prt_registry_entity_t reg[TOTALMAXPRT];
    GLvector3 vfwd, vcam;
    GLvertex vtlist[4];
    int cnt, numparticle;
    Uint16 prt;
    int i;

    do_instance_update( pcam );

    vfwd = mat_getCamForward( pcam->mView );
    vcam.x = pcam->x;
    vcam.y = pcam->y;
    vcam.z = pcam->z;

    // Original points
    numparticle = 0;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_t * pprt = PrtList + cnt;
        prt_instance_t * pinst = &(pprt->inst);

        if ( !pprt->on || !pprt->inview ) continue;

        if( pprt->is_hidden || INVALID_TILE == pprt->onwhichfan ) continue;

        if ( pinst->size != 0 )
        {
            GLvector3 vpos;
            float dist;

            vpos.x = pinst->pos.x - vcam.x;
            vpos.y = pinst->pos.y - vcam.y;
            vpos.z = pinst->pos.z - vcam.z;

            dist = VDotProduct( vfwd, vpos );

            if ( dist > 0 )
            {
                reg[numparticle].index = cnt;
                reg[numparticle].dist  = dist;
                numparticle++;
            }
        }
    }

    // sort the particles from close to far
    qsort( reg, numparticle, sizeof(prt_registry_entity_t), cmp_prt_registry_entity );

    Begin3DMode( pcam );
    {
        //----------------------------
        // DO SOLID SPRITES FIRST
        glPushAttrib( GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
        {
            GLtexture_Bind( txTexture + TX_PARTICLE_TRANS );

            glDisable( GL_CULL_FACE );
            glDisable( GL_DITHER );

            glEnable( GL_DEPTH_TEST );
            glDepthFunc( GL_LESS );

            // apply solid particles from near to far
            for ( cnt = 0; cnt < numparticle; cnt++ )
            {
                prt_t * pprt;
                prt_instance_t * pinst;

                // Get the index from the color slot
                prt = reg[cnt].index;

                pprt = PrtList + prt;
                pinst = &(pprt->inst);
                if (!pinst->valid) continue;

                // Draw sprites this round
                if ( PRTSOLIDSPRITE != PrtList[prt].type ) continue;

                // billboard for the particle
                calc_billboard_verts( vtlist, pinst, pinst->size );

                //--------------------
                // Render the solid version of the particle

                // Calculate the position of the four corners of the billboard
                // used to display the particle.

                glDepthMask( GL_TRUE );

                glDisable( GL_BLEND );

                glEnable( GL_ALPHA_TEST );
                glAlphaFunc( GL_EQUAL, 1 );

                glColor4f( pinst->color_component, pinst->color_component, pinst->color_component, 1.0f );

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
        glPopAttrib();

        //----------------------------
        // DO ALL KINDS OF TRANSPARENT SPRITES NEXT
        glPushAttrib( GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
        {
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );

            glEnable( GL_DEPTH_TEST );
            glDepthFunc( GL_LEQUAL );

            // apply transparent particles from far to near
            for ( cnt = numparticle - 1; cnt >= 0; cnt-- )
            {
                prt_t * pprt;
                prt_instance_t * pinst;

                // Get the index from the color slot
                prt = reg[cnt].index;

                pprt = PrtList + prt;
                pinst = &(pprt->inst);
                if (!pinst->valid) continue;

                if ( PRTSOLIDSPRITE == PrtList[prt].type )
                {
                    // do the alpha blended edge of the solid particle

                    glEnable( GL_ALPHA_TEST );
                    glAlphaFunc( GL_LESS, 1 );

                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                    glColor4f( pinst->color_component, pinst->color_component, pinst->color_component, pinst->alpha_component );

                    GLtexture_Bind( txTexture + TX_PARTICLE_TRANS );
                }
                else if ( PRTLIGHTSPRITE == PrtList[prt].type )
                {
                    // do the light sprites

                    glDisable( GL_ALPHA_TEST );

                    glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_COLOR );
                    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

                    GLtexture_Bind( txTexture + TX_PARTICLE_LIGHT );
                }
                else if ( PRTALPHASPRITE == PrtList[prt].type )
                {
                    // do the transparent sprites

                    glEnable( GL_ALPHA_TEST );
                    glAlphaFunc( GL_GREATER, 0 );

                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                    glColor4f( pinst->color_component, pinst->color_component, pinst->color_component, pinst->alpha_component / 10.0f );

                    GLtexture_Bind( txTexture + TX_PARTICLE_TRANS );
                }
                else
                {
                    // unknown type
                    continue;
                }

                // Calculate the position of the four corners of the billboard
                // used to display the particle.
                calc_billboard_verts( vtlist, pinst, pinst->size );

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
        glPopAttrib();

    }
    End3DMode();


}

//--------------------------------------------------------------------------------------------
void render_refprt( camera_t * pcam )
{
    // ZZ> This function draws sprites reflected in the floor

    prt_registry_entity_t reg[TOTALMAXPRT];
    GLvector3 vfwd, vcam;
    GLvertex vtlist[4];
    int prt, numparticle;
    Uint16 cnt;
    float size;
    int startalpha;
    float level = 0.00f;
    int i;

    do_instance_update( pcam );

    vfwd = mat_getCamForward( pcam->mView );
    vcam.x = pcam->x;
    vcam.y = pcam->y;
    vcam.z = pcam->z;

    // Original points
    numparticle = 0;
    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_t * pprt = PrtList + cnt;
        prt_instance_t * pinst = &(pprt->inst);

        if ( !pprt->on || !pprt->inview || INVALID_TILE == pprt->onwhichfan ) continue;

        if ( pinst->size != 0 )
        {
            GLvector3 vpos;
            float dist;

            vpos.x = pprt->xpos - vcam.x;
            vpos.y = pprt->ypos - vcam.y;
            vpos.z = pprt->zpos - vcam.z;

            dist = VDotProduct( vfwd, vpos );

            if ( dist > 0 )
            {
                reg[numparticle].index = cnt;
                reg[numparticle].dist  = dist;
                numparticle++;
            }
        }
    }

    // sort the particles from close to far
    qsort( reg, numparticle, sizeof(prt_registry_entity_t), cmp_prt_registry_entity );

    // Choose texture.
    GLtexture_Bind( txTexture + TX_PARTICLE_TRANS );

    glDisable( GL_CULL_FACE );
    glDisable( GL_DITHER );

    // Render each particle that was on
    for ( cnt = 0; cnt < numparticle; cnt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        // Get the index from the color slot
        prt = reg[cnt].index;

        pprt = PrtList + prt;
        pinst = &(pprt->inst);
        if (!pinst->valid) continue;

        // Draw lights this round
        if ( PRTLIGHTSPRITE != PrtList[prt].type ) continue;

        size = pinst->size * 1.5f;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        calc_billboard_verts( vtlist, pinst, size );

        // Fill in the rest of the data
        startalpha = ( int )( 255 + pprt->zpos - level );
        if ( startalpha < 0 ) startalpha = 0;

        startalpha = ( startalpha | reffadeor ) >> 1;  // Fix for Riva owners
        if ( startalpha > 255 ) startalpha = 255;
        if ( startalpha > 0 )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, startalpha * INV_FF );

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

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Render each particle that was on
    for ( cnt = 0; cnt < numparticle; cnt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        // Get the index from the color slot
        prt = reg[cnt].index;

        pprt = PrtList + prt;
        pinst = &(pprt->inst);
        if (!pinst->valid) continue;

        // Draw solid and transparent sprites this round
        if ( PRTLIGHTSPRITE == PrtList[prt].type ) continue;

        // Figure out the sprite's size based on distance
        size = ( float )( PrtList[prt].size ) * 0.00092f;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        calc_billboard_verts( vtlist, pinst, size );

        // Fill in the rest of the data
        startalpha = ( int )( 255 + pprt->zpos - level );
        if ( startalpha < 0 ) startalpha = 0;

        startalpha = ( startalpha | reffadeor ) >> ( 1 + PrtList[prt].type );  // Fix for Riva owners
        if ( startalpha > 255 ) startalpha = 255;
        if ( startalpha > 0 )
        {
            float color_component = pinst->color_component / 16.0f;

            glColor4f( color_component, color_component, color_component, startalpha * INV_FF );

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
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void do_instance_update( camera_t * pcam )
{
    int cnt;

    if ( NULL == pcam ) pcam = &gCamera;
    if ( NULL == pcam ) return;

    // only one update per frame
    if ( instance_update == frame_wld ) return;
    instance_update = frame_wld;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_t * pprt = PrtList + cnt;
        prt_instance_t * pinst = &(pprt->inst);

        if ( !pprt->on || !pprt->inview || INVALID_TILE == pprt->onwhichfan || pprt->size == 0 )
        {
            pinst->valid = bfalse;
        }
        else
        {
            // calculate the "billboard" for this particle
            prt_instance_upload( pcam, pinst, pprt );
        }
    }
};

//--------------------------------------------------------------------------------------------
void prt_instance_upload( camera_t * pcam, prt_instance_t * pinst, prt_t * pprt )
{
    pip_t * ppip;

    Uint16 turn;
    GLvector3 vfwd, vup, vright;

    if ( NULL == pprt || !pprt->on ) return;

    if ( !VALID_PIP( pprt->pip ) ) return;
    ppip = PipList + pprt->pip;

    pinst->type = pprt->type;

    pinst->color_component = pprt->light * INV_FF;

    pinst->size  = FP8_TO_FLOAT( pprt->size ) * 0.25f;
    pinst->image = FP8_TO_INT( pprt->image + pprt->imagestt );

    // set the position
    pinst->pos.x = pprt->xpos;
    pinst->pos.y = pprt->ypos;
    pinst->pos.z = pprt->zpos;

    pinst->orientation = ppip->orientation;

    // get the vector from the camera to the particle
    vfwd.x = pinst->pos.x - pcam->x;
    vfwd.y = pinst->pos.y - pcam->y;
    vfwd.z = pinst->pos.z - pcam->z;
    vfwd = VNormalize( vfwd );

    // set the up and right vectors
    if( ORIENTATION_B == pinst->orientation )
    {
        // use the camera up vector
        vup = mat_getCamUp( pcam->mView );
        vup = VNormalize( vup );

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }
    else if ( VALID_CHR( pprt->attachedtocharacter ) )
    {
        chr_instance_t * cinst = &(ChrList[pprt->attachedtocharacter].inst);

        if ( cinst->matrixvalid )
        {
            // use the character matrix to orient the particle
            // assume that the particle "up" is in the z-direction in the object's
            // body fixed axes. should work for the gonnes & such

            switch( pinst->orientation )
            {
                case ORIENTATION_X: vup = mat_getChrForward( cinst->matrix ); break;
                case ORIENTATION_Y: vup = mat_getChrRight( cinst->matrix ); break;

                default:
                case ORIENTATION_Z: vup = mat_getChrUp( cinst->matrix ); break;
            }

            vup = VNormalize( vup );
        }
        else
        {
            // use the camera directions?
            switch( pinst->orientation )
            {
                case ORIENTATION_X: vup = mat_getCamForward( pcam->mView ); break;
                case ORIENTATION_Y: vup = mat_getCamRight( pcam->mView ); break;

                default:
                case ORIENTATION_Z: vup = mat_getCamUp( pcam->mView ); break;
            }
        }

        vup = VNormalize( vup );

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }
    else if ( ppip->rotatetoface && (ABS( pprt->xvel ) + ABS( pprt->yvel ) + ABS( pprt->zvel ) > 0) )
    {
        // the particle points along its direction of travel

        vup.x = pprt->xvel;
        vup.y = pprt->yvel;
        vup.z = pprt->zvel;
        vup = VNormalize( vup );

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }
    else if( ORIENTATION_V == pinst->orientation )
    {
        // use the camera up vector
        vup.x = vup.y = 0;
        vup.z = 1;

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }
    else if( ORIENTATION_H == pinst->orientation )
    {
        vup.x = vup.y = 0;
        vup.z = 1;

        // force right to be horizontal
        vright = VCrossProduct( vfwd, vup ); 

        // force "up" to be close to the camera forward, but horizontal
        vup = VCrossProduct( vup, vright );

        // notmalize them
        vright = VNormalize( vright );
        vright = VNormalize( vup );
    }
    else
    {
        // use the camera up vector
        vup = mat_getCamUp( pcam->mView );
        vup = VNormalize( vup );

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }

    // calculate the actual vectors using the particle rotation
    turn = pprt->rotate >> 2;
    pinst->up.x    = vup.x * turntocos[turn] - vright.x * turntosin[turn];
    pinst->up.y    = vup.y * turntocos[turn] - vright.y * turntosin[turn];
    pinst->up.z    = vup.z * turntocos[turn] - vright.z * turntosin[turn];

    pinst->right.x = vup.x * turntosin[turn] + vright.x * turntocos[turn];
    pinst->right.y = vup.y * turntosin[turn] + vright.y * turntocos[turn];
    pinst->right.z = vup.z * turntosin[turn] + vright.z * turntocos[turn];

    //pinst->up    = vup;
    //pinst->right = vright;

    // set some particle dependent properties
    pinst->alpha_component = 1.0f;
    switch ( pinst->type )
    {
        case PRTSOLIDSPRITE: break;
        case PRTALPHASPRITE: pinst->alpha_component = particletrans * INV_FF; break;
        case PRTLIGHTSPRITE: pinst->size *= 1.5; break;
    }

    pinst->valid = btrue;
}

//--------------------------------------------------------------------------------------------
void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size )
{
    // Calculate the position of the four corners of the billboard
    // used to display the particle.

    if ( NULL == vlst || NULL == pinst ) return;

    vlst[0].x = pinst->pos.x + ( -pinst->right.x - pinst->up.x ) * size;
    vlst[0].y = pinst->pos.y + ( -pinst->right.y - pinst->up.y ) * size;
    vlst[0].z = pinst->pos.z + ( -pinst->right.z - pinst->up.z ) * size;

    vlst[1].x = pinst->pos.x + (  pinst->right.x - pinst->up.x ) * size;
    vlst[1].y = pinst->pos.y + (  pinst->right.y - pinst->up.y ) * size;
    vlst[1].z = pinst->pos.z + (  pinst->right.z - pinst->up.z ) * size;

    vlst[2].x = pinst->pos.x + (  pinst->right.x + pinst->up.x ) * size;
    vlst[2].y = pinst->pos.y + (  pinst->right.y + pinst->up.y ) * size;
    vlst[2].z = pinst->pos.z + (  pinst->right.z + pinst->up.z ) * size;

    vlst[3].x = pinst->pos.x + ( -pinst->right.x + pinst->up.x ) * size;
    vlst[3].y = pinst->pos.y + ( -pinst->right.y + pinst->up.y ) * size;
    vlst[3].z = pinst->pos.z + ( -pinst->right.z + pinst->up.z ) * size;

    vlst[0].s = sprite_list_u[pinst->image][1];
    vlst[0].t = sprite_list_v[pinst->image][1];

    vlst[1].s = sprite_list_u[pinst->image][0];
    vlst[1].t = sprite_list_v[pinst->image][1];

    vlst[2].s = sprite_list_u[pinst->image][0];
    vlst[2].t = sprite_list_v[pinst->image][0];

    vlst[3].s = sprite_list_u[pinst->image][1];
    vlst[3].t = sprite_list_v[pinst->image][0];
}