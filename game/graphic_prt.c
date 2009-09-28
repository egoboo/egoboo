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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - graphic_prt.c
* Particle system drawing and management code.
*/

#include "graphic.h"

#include "particle.h"
#include "char.h"
#include "game.h"
#include "texture.h"

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
static void prt_instance_update( camera_t * pcam, Uint16 particle, Uint8 trans, bool_t do_lighting );
static void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size, float level, bool_t do_reflect );
static int cmp_prt_registry_entity(const void * vlhs, const void * vrhs);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32  instance_update = (Uint32)~0;

//--------------------------------------------------------------------------------------------
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
size_t render_all_prt_begin( camera_t * pcam, prt_registry_entity_t reg[], size_t reg_count )
{
    GLvector3 vfwd, vcam;
    int cnt;
    size_t numparticle;

    update_all_prt_instance( pcam );

    vfwd = mat_getCamForward( pcam->mView );
    vcam = pcam->pos;

    // Original points
    numparticle = 0;
    for ( cnt = 0; cnt < maxparticles && numparticle < reg_count; cnt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if( !ACTIVE_PRT(cnt) ) continue;
        pprt = PrtList.lst + cnt;
        pinst = &(pprt->inst);

        if ( !pprt->inview || pprt->is_hidden ) continue;

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

    return numparticle;
}

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_solid( Uint16 iprt )
{
    // BB > Render the solid version of the particle

    GLvertex vtlist[4];
    int i;

    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !ACTIVE_PRT(iprt) ) return bfalse;
    pprt = PrtList.lst + iprt;
    pinst = &(pprt->inst);

    // if the particle instance data is not valid, do not continue
    if ( !pinst->valid ) return bfalse;

    // only render solid sprites
    if ( SPRITE_SOLID != pprt->type ) return bfalse;

    // billboard for the particle
    calc_billboard_verts( vtlist, pinst, pinst->size, pprt->floor_level, bfalse );

    ATTRIB_PUSH( "render_one_prt_solid", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
    {
        GL_DEBUG(glDepthMask)(GL_TRUE );

        GL_DEBUG(glDisable)(GL_CULL_FACE );
        GL_DEBUG(glDisable)(GL_DITHER );

        GL_DEBUG(glEnable)(GL_DEPTH_TEST );
        GL_DEBUG(glDepthFunc)(GL_LESS );

        GL_DEBUG(glDisable)(GL_BLEND );

        GL_DEBUG(glEnable)(GL_ALPHA_TEST );
        GL_DEBUG(glAlphaFunc)(GL_EQUAL, 1 );

        oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_TRANS ) );

        GL_DEBUG(glColor4f)(pinst->fintens, pinst->fintens, pinst->fintens, 1.0f );

        GL_DEBUG(glBegin)(GL_TRIANGLE_FAN );
        {
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG(glTexCoord2fv)( vtlist[i].tex );
                GL_DEBUG(glVertex3fv)( vtlist[i].pos );
            }
        }
        GL_DEBUG_END();
    }
    ATTRIB_POP( "render_one_prt_solid");

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_all_prt_solid( camera_t * pcam, prt_registry_entity_t reg[], size_t numparticle )
{
    // BB > do solid sprites first

    Uint32 cnt;
    Uint16 prt;

    Begin3DMode( pcam );
    {
        // apply solid particles from near to far
        for ( cnt = 0; cnt < numparticle; cnt++ )
        {
            // Get the index from the color slot
            prt = reg[cnt].index;

            render_one_prt_solid( reg[cnt].index );
        }
    }
    End3DMode();

}

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_trans( Uint16 iprt )
{
    // BB> do all kinds of transparent sprites next

    GLvertex vtlist[4];
    int i;
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !ACTIVE_PRT(iprt) ) return bfalse;
    pprt = PrtList.lst + iprt;
    pinst = &(pprt->inst);

    // if the particle instance data is not valid, do not continue
    if ( !pinst->valid ) return bfalse;

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    calc_billboard_verts( vtlist, pinst, pinst->size, pprt->floor_level, bfalse );

    ATTRIB_PUSH( "render_one_prt_trans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
    {
        GL_DEBUG(glDepthMask)(GL_FALSE );

        GL_DEBUG(glEnable)(GL_BLEND );
        GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // some default

        GL_DEBUG(glEnable)(GL_DEPTH_TEST );
        GL_DEBUG(glDepthFunc)(GL_LEQUAL );

        if ( SPRITE_SOLID == PrtList.lst[iprt].type )
        {
            // do the alpha blended edge of the solid particle

            GL_DEBUG(glEnable)(GL_ALPHA_TEST );
            GL_DEBUG(glAlphaFunc)(GL_LESS, 1 );

            GL_DEBUG(glEnable)(GL_BLEND );
            GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            GL_DEBUG(glColor4f)( pinst->fintens, pinst->fintens, pinst->fintens, 1.0f );

            oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_TRANS ) );
        }
        else if ( SPRITE_LIGHT == PrtList.lst[iprt].type )
        {
            // do the light sprites

            GL_DEBUG(glDisable)(GL_ALPHA_TEST );

            GL_DEBUG(glEnable)( GL_BLEND );
            GL_DEBUG(glBlendFunc)( GL_ONE, GL_ONE_MINUS_SRC_COLOR );
            GL_DEBUG(glColor4f)( pinst->falpha, pinst->falpha, pinst->falpha, 1.0f );

            oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_LIGHT ) );
        }
        else if ( SPRITE_ALPHA == PrtList.lst[iprt].type )
        {
            // do the transparent sprites

            GL_DEBUG(glEnable)(GL_ALPHA_TEST );
            GL_DEBUG(glAlphaFunc)(GL_GREATER, 0 );

            GL_DEBUG(glEnable)(GL_BLEND );
            GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            GL_DEBUG(glColor4f)(pinst->fintens, pinst->fintens, pinst->fintens, pinst->falpha );

            oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_TRANS ) );
        }
        else
        {
            // unknown type
            return bfalse;
        }

        // Go on and draw it
        GL_DEBUG(glBegin)(GL_TRIANGLE_FAN );
        {
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG(glTexCoord2fv)(vtlist[i].tex );
                GL_DEBUG(glVertex3fv)(vtlist[i].pos );
            }
        }
        GL_DEBUG_END();
    }
    ATTRIB_POP( "render_one_prt_trans" );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_all_prt_trans( camera_t * pcam, prt_registry_entity_t reg[], size_t numparticle )
{
    // BB> do all kinds of transparent sprites next

    int cnt;

    Begin3DMode( pcam );
    {
        // apply transparent particles from far to near
        for ( cnt = numparticle - 1; cnt >= 0; cnt-- )
        {
            // Get the index from the color slot
            render_one_prt_trans( reg[cnt].index );
        }
    }
    End3DMode();
}

//--------------------------------------------------------------------------------------------
void render_prt( camera_t * pcam )
{
    // ZZ> This function draws the sprites for particle systems

    prt_registry_entity_t reg[TOTAL_MAX_PRT];
    size_t numparticle;

    numparticle = render_all_prt_begin( pcam, reg, TOTAL_MAX_PRT );

    render_all_prt_solid( pcam, reg, numparticle );
    render_all_prt_trans( pcam, reg, numparticle );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
size_t render_all_prt_ref_begin( camera_t * pcam, prt_registry_entity_t reg[], size_t reg_count )
{
    GLvector3 vfwd, vcam;
    Uint16 cnt;
    size_t numparticle;

    update_all_prt_instance( pcam );

    vfwd = mat_getCamForward( pcam->mView );
    vcam = pcam->pos;

    // Original points
    numparticle = 0;
    for ( cnt = 0; cnt < maxparticles && numparticle < reg_count; cnt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if( !ACTIVE_PRT(cnt) ) continue;
        pprt = PrtList.lst + cnt;
        pinst = &(pprt->inst);

        if ( !pprt->inview || pprt->is_hidden  ) continue;

        if ( pinst->size != 0 )
        {
            GLvector3 vpos;
            float dist;

            vpos = VSub( pinst->pos, vcam );
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

    return numparticle;
}

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_ref( Uint16 iprt )
{
    // BB > render one particle

    GLvertex vtlist[4];
    int startalpha;
    int i;
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !ACTIVE_PRT(iprt) ) return bfalse;

    pprt = PrtList.lst + iprt;
    pinst = &(pprt->inst);
    if (!pinst->valid) return bfalse;

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    calc_billboard_verts( vtlist, pinst, pinst->size, pprt->floor_level, btrue );

    // Fill in the rest of the data
    startalpha = 255 - (pprt->pos.z - pprt->floor_level) / 2.0f;
    startalpha = CLIP(startalpha, 0, 255);

    startalpha = ( startalpha | gfx.reffadeor ) >> 1;  // Fix for Riva owners
    startalpha = CLIP(startalpha, 0, 255);

    if ( startalpha > 0 )
    {
        ATTRIB_PUSH( "render_one_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
        {
            GL_DEBUG(glDisable)(GL_CULL_FACE );
            GL_DEBUG(glDisable)(GL_DITHER );

            if ( SPRITE_LIGHT == PrtList.lst[iprt].type )
            {
                // do the light sprites
                float alpha = startalpha * INV_FF * pinst->fintens / 2.0f;

                GL_DEBUG(glDisable)(GL_ALPHA_TEST );

                GL_DEBUG(glEnable)(GL_BLEND );
                GL_DEBUG(glBlendFunc)(GL_ONE, GL_ONE_MINUS_SRC_COLOR );
                GL_DEBUG(glColor4f)(alpha, alpha, alpha, 1.0f );

                oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_LIGHT ) );
            }
            else if ( SPRITE_SOLID == PrtList.lst[iprt].type || SPRITE_ALPHA == PrtList.lst[iprt].type )
            {
                // do the transparent sprites

                float alpha = pinst->falpha * pinst->fintens / 2.0f;

                GL_DEBUG(glEnable)(GL_ALPHA_TEST );
                GL_DEBUG(glAlphaFunc)(GL_GREATER, 0 );

                GL_DEBUG(glEnable)(GL_BLEND );
                GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                GL_DEBUG(glColor4f)(pinst->fintens, pinst->fintens, pinst->fintens, alpha );

                oglx_texture_Bind( TxTexture_get_ptr( TX_PARTICLE_TRANS ) );
            }
            else
            {
                // unknown type
                return bfalse;
            }

            GL_DEBUG(glBegin)(GL_TRIANGLE_FAN );
            {
                for ( i = 0; i < 4; i++ )
                {
                    GL_DEBUG(glTexCoord2fv)(vtlist[i].tex );
                    GL_DEBUG(glVertex3fv)(vtlist[i].pos );
                }
            }
            GL_DEBUG_END();
        }
        ATTRIB_POP( "render_one_prt_ref" );

    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_all_prt_ref( camera_t * pcam, prt_registry_entity_t reg[], size_t numparticle )
{
    Uint16 prt;
    Uint32 cnt;

    Begin3DMode( pcam );
    {
        // Render each particle that was on
        for ( cnt = 0; cnt < numparticle; cnt++ )
        {
            // Get the index from the color slot
            prt = reg[cnt].index;

            render_one_prt_ref( reg[cnt].index );
        }
    }
    End3DMode();
}

//--------------------------------------------------------------------------------------------
void render_prt_ref( camera_t * pcam )
{
    // ZZ> This function draws sprites reflected in the floor

    prt_registry_entity_t reg[TOTAL_MAX_PRT];
    size_t numparticle;

    numparticle = render_all_prt_ref_begin( pcam, reg, TOTAL_MAX_PRT );
    render_all_prt_ref( pcam, reg, numparticle );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void update_all_prt_instance( camera_t * pcam )
{
    int cnt;

    if ( NULL == pcam ) pcam = PCamera;
    if ( NULL == pcam ) return;

    // only one update per frame
    if ( instance_update == update_wld ) return;
    instance_update = update_wld;

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if( !ACTIVE_PRT(cnt) ) continue;
        pprt = PrtList.lst + cnt;
        pinst = &(pprt->inst);

        if ( !pprt->inview || pprt->is_hidden || 0 == pprt->size )
        {
            pinst->valid = bfalse;
        }
        else
        {
            // calculate the "billboard" for this particle
            prt_instance_update( pcam, cnt, 255, btrue );
        }
    }
}

//--------------------------------------------------------------------------------------------
void prt_instance_update_vertices( camera_t * pcam, prt_instance_t * pinst, prt_t * pprt )
{
    pip_t * ppip;

    GLvector3 vfwd, vup, vright;

    if ( NULL == pprt || !pprt->allocated || !pprt->active ) return;

    if ( !VALID_PIP( pprt->pip_ref ) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    pinst->type = pprt->type;

    pinst->image = FP8_TO_INT( pprt->image + pprt->imagestt );

    // set the position
    pinst->pos         = pprt->pos;
    pinst->orientation = ppip->orientation;

    // get the vector from the camera to the particle
    vfwd = VSub( pinst->pos, pcam->pos );
    vfwd = VNormalize( vfwd );

    // set the up and right vectors
    if ( ppip->rotatetoface && !ACTIVE_CHR( pprt->attachedto_ref ) && (ABS( pprt->vel.x ) + ABS( pprt->vel.y ) + ABS( pprt->vel.z ) > 0) )
    {
        // the particle points along its direction of travel

        vup = pprt->vel;
        vup   = VNormalize( vup );

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }
    else if ( ORIENTATION_B == pinst->orientation )
    {
        // use the camera up vector
        vup = mat_getCamUp( pcam->mView );
        vup = VNormalize( vup );

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }
    else if ( ACTIVE_CHR( pprt->attachedto_ref ) )
    {
        chr_instance_t * cinst = chr_get_pinstance(pprt->attachedto_ref);

        if ( cinst->matrixvalid )
        {
            // use the character matrix to orient the particle
            // assume that the particle "up" is in the z-direction in the object's
            // body fixed axes. should work for the gonnes & such

            switch ( pinst->orientation )
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
            switch ( pinst->orientation )
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
    else if ( ORIENTATION_V == pinst->orientation )
    {
        // use the camera up vector
        vup.x = vup.y = 0;
        vup.z = 1;

        // get the correct "right" vector
        vright = VCrossProduct( vfwd, vup );
        vright = VNormalize( vright );
    }
    else if ( ORIENTATION_H == pinst->orientation )
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
    if ( 0 == pprt->rotate )
    {
        pinst->up    = vup;
        pinst->right = vright;
    }
    else
    {
        Uint16 turn    = pprt->rotate >> 2;
        pinst->up.x    = vup.x * turntocos[turn & TRIG_TABLE_MASK ] - vright.x * turntosin[turn & TRIG_TABLE_MASK ];
        pinst->up.y    = vup.y * turntocos[turn & TRIG_TABLE_MASK ] - vright.y * turntosin[turn & TRIG_TABLE_MASK ];
        pinst->up.z    = vup.z * turntocos[turn & TRIG_TABLE_MASK ] - vright.z * turntosin[turn & TRIG_TABLE_MASK ];

        pinst->right.x = vup.x * turntosin[turn & TRIG_TABLE_MASK ] + vright.x * turntocos[turn & TRIG_TABLE_MASK ];
        pinst->right.y = vup.y * turntosin[turn & TRIG_TABLE_MASK ] + vright.y * turntocos[turn & TRIG_TABLE_MASK ];
        pinst->right.z = vup.z * turntosin[turn & TRIG_TABLE_MASK ] + vright.z * turntocos[turn & TRIG_TABLE_MASK ];
    }

    // calculate the billboard normal
    pinst->nrm = VCrossProduct( pinst->right, pinst->up );
    if ( VDotProduct(vfwd, pinst->nrm) < 0 )
    {
        pinst->nrm.x *= -1;
        pinst->nrm.y *= -1;
        pinst->nrm.z *= -1;
    }

    // set some particle dependent properties
    pinst->size  = FP8_TO_FLOAT( pprt->size ) * 0.25f;
    switch ( pinst->type )
    {
        case SPRITE_SOLID: pinst->size *= 0.9384f; break;
        case SPRITE_ALPHA: pinst->size *= 0.9353f; break;
        case SPRITE_LIGHT: pinst->size *= 1.5912f; break;
    }

    pinst->valid = btrue;
}

GLmatrix prt_inst_make_matrix( prt_instance_t * pinst )
{
    GLmatrix mat = IdentityMatrix();

    mat.CNV( 0, 1 ) = -pinst->up.x;
    mat.CNV( 1, 1 ) = -pinst->up.y;
    mat.CNV( 2, 1 ) = -pinst->up.z;

    mat.CNV( 0, 0 ) = pinst->right.x;
    mat.CNV( 1, 0 ) = pinst->right.y;
    mat.CNV( 2, 0 ) = pinst->right.z;

    mat.CNV( 0, 2 ) = pinst->nrm.x;
    mat.CNV( 1, 2 ) = pinst->nrm.y;
    mat.CNV( 2, 2 ) = pinst->nrm.z;

    return mat;
}

//--------------------------------------------------------------------------------------------
void prt_instance_update_lighting( prt_instance_t * pinst, prt_t * pprt, Uint8 trans, bool_t do_lighting )
{
    int    cnt;
    Uint32 alpha;
    Uint8  self_light;
    lighting_cache_t global_light, loc_light;
    float min_light, lite;
    GLmatrix mat;

    if ( NULL == pinst || NULL == pprt ) return;

    // To make life easier
    alpha = trans;

    // interpolate the lighting for the origin of the object
    interpolate_grid_lighting( PMesh, &global_light, pinst->pos );

    // rotate the lighting data to body_centered coordinates
    mat = prt_inst_make_matrix( pinst );
    project_lighting( &loc_light, &global_light, mat );

    // remove any "ambient" component from the lighting
    min_light = loc_light.max_light;
    for (cnt = 0; cnt < 6; cnt++)
    {
        min_light = MIN(min_light, loc_light.lighting_low[cnt]);
        min_light = MIN(min_light, loc_light.lighting_hgh[cnt]);
    }

    for (cnt = 0; cnt < 6; cnt++)
    {
        loc_light.lighting_low[cnt] -= min_light;
        loc_light.lighting_hgh[cnt] -= min_light;
    }

    // determine the ambient lighting
    self_light  = ( 255 == pinst->light ) ? 0 : pinst->light;
    pinst->famb = 0.9f * pinst->famb + 0.1f * (self_light + min_light);

    // determine the normal dependent amount of light
    lite = evaluate_lighting_cache( &loc_light, pinst->nrm.v, pinst->pos.z, PMesh->mmem.bbox, NULL, NULL );
    pinst->fdir = 0.9f * pinst->fdir + 0.1f * lite;

    // determine the overall lighting
    pinst->fintens = pinst->fdir * INV_FF;
    if ( do_lighting )
    {
        pinst->fintens += pinst->famb * INV_FF;
    }

    // determine the alpha component
    pinst->falpha = (alpha * INV_FF) * (pinst->alpha * INV_FF);
    pinst->falpha = CLIP(pinst->falpha, 0.0f, 1.0f);
}

//--------------------------------------------------------------------------------------------
void prt_instance_update( camera_t * pcam, Uint16 particle, Uint8 trans, bool_t do_lighting )
{
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !ACTIVE_PRT(particle) ) return;
    pprt = PrtList.lst + particle;
    pinst = &(pprt->inst);

    // make sure that the vertices are interpolated
    prt_instance_update_vertices( pcam, pinst, pprt );

    // do the lighting
    prt_instance_update_lighting( pinst, pprt, trans, do_lighting );
}

//--------------------------------------------------------------------------------------------
void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size, float level, bool_t do_reflect )
{
    // Calculate the position of the four corners of the billboard
    // used to display the particle.

    int i;

    if ( NULL == vlst || NULL == pinst ) return;

    for ( i = 0; i < 4; i++ )
    {
        vlst[i].pos[XX] = pinst->pos.x;
        vlst[i].pos[YY] = pinst->pos.y;
        vlst[i].pos[ZZ] = pinst->pos.z;
    }

    vlst[0].pos[XX] += ( -pinst->right.x - pinst->up.x ) * size;
    vlst[0].pos[YY] += ( -pinst->right.y - pinst->up.y ) * size;
    vlst[0].pos[ZZ] += ( -pinst->right.z - pinst->up.z ) * size;

    vlst[1].pos[XX] += (  pinst->right.x - pinst->up.x ) * size;
    vlst[1].pos[YY] += (  pinst->right.y - pinst->up.y ) * size;
    vlst[1].pos[ZZ] += (  pinst->right.z - pinst->up.z ) * size;

    vlst[2].pos[XX] += (  pinst->right.x + pinst->up.x ) * size;
    vlst[2].pos[YY] += (  pinst->right.y + pinst->up.y ) * size;
    vlst[2].pos[ZZ] += (  pinst->right.z + pinst->up.z ) * size;

    vlst[3].pos[XX] += ( -pinst->right.x + pinst->up.x ) * size;
    vlst[3].pos[YY] += ( -pinst->right.y + pinst->up.y ) * size;
    vlst[3].pos[ZZ] += ( -pinst->right.z + pinst->up.z ) * size;

    if ( do_reflect )
    {
        for ( i = 0; i < 4; i++ )
        {
            vlst[i].pos[ZZ] = 2 * level - vlst[i].pos[ZZ];
        }
    }

    vlst[0].tex[SS] = sprite_list_u[pinst->image][1];
    vlst[0].tex[TT] = sprite_list_v[pinst->image][1];

    vlst[1].tex[SS] = sprite_list_u[pinst->image][0];
    vlst[1].tex[TT] = sprite_list_v[pinst->image][1];

    vlst[2].tex[SS] = sprite_list_u[pinst->image][0];
    vlst[2].tex[TT] = sprite_list_v[pinst->image][0];

    vlst[3].tex[SS] = sprite_list_u[pinst->image][1];
    vlst[3].tex[TT] = sprite_list_v[pinst->image][0];
}
