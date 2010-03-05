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

/// @file graphic_prt.c
/// @brief Particle system drawing and management code.
/// @details

#include "graphic_prt.h"

#include "particle.inl"
#include "char.inl"
#include "profile.inl"

#include "game.h"
#include "texture.h"
#include "camera.h"
#include "input.h"
#include "lighting.h"

#include "egoboo_setup.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// dynamically calculate the particle texture coordinates

int   ptex_w[2] = {256, 256};
int   ptex_h[2] = {256, 256};
float ptex_wscale[2] = {1.0f, 1.0f};
float ptex_hscale[2] = {1.0f, 1.0f};

//--------------------------------------------------------------------------------------------
int prt_get_texture_style( const TX_REF by_reference itex )
{
    int index;

    index = -1;
    switch ( REF_TO_INT( itex ) )
    {
        case TX_PARTICLE_TRANS:
            index = 0;
            break;

        case TX_PARTICLE_LIGHT:
            index = 1;
            break;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
void prt_set_texture_params( const TX_REF by_reference itex )
{
    int index;
    oglx_texture_t * ptex;

    index = prt_get_texture_style( itex );
    if ( index < 0 ) return;

    ptex = TxTexture_get_ptr( itex );
    if ( NULL == ptex ) return;

    ptex_w[index] = ptex->imgW;
    ptex_h[index] = ptex->imgH;
    ptex_wscale[index] = ( float )ptex->imgW / ( float )ptex->base.width;
    ptex_hscale[index] = ( float )ptex->imgH / ( float )ptex->base.height;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// The data values necessary to sort particles by their position to the camera
struct s_prt_registry_entity
{
    PRT_REF index;
    float   dist;
};
typedef struct s_prt_registry_entity prt_registry_entity_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void prt_instance_update( camera_t * pcam, const PRT_REF by_reference particle, Uint8 trans, bool_t do_lighting );
static void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size, bool_t do_reflect );
static int  cmp_prt_registry_entity( const void * vlhs, const void * vrhs );

static void draw_one_attacment_point( chr_instance_t * pinst, mad_t * pmad, int vrt_offset );
static void prt_draw_attached_point( prt_t * pprt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32  instance_update = ( Uint32 )~0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int cmp_prt_registry_entity( const void * vlhs, const void * vrhs )
{
    const prt_registry_entity_t * lhs, * rhs;

    lhs = ( prt_registry_entity_t * ) vlhs;
    rhs = ( prt_registry_entity_t * ) vrhs;

    return lhs->dist - rhs->dist;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
size_t render_all_prt_begin( camera_t * pcam, prt_registry_entity_t reg[], size_t reg_count )
{
    fvec3_t vfwd, vcam;
    PRT_REF iprt;
    size_t  numparticle;

    prt_instance_update_all( pcam );

    vfwd = mat_getCamForward( pcam->mView );
    vcam = pcam->pos;

    // Original points
    numparticle = 0;
    for ( iprt = 0; iprt < maxparticles && numparticle < reg_count; iprt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if ( !DISPLAY_PRT( iprt ) ) continue;
        pprt = PrtList.lst + iprt;
        pinst = &( pprt->inst );

        if ( !pprt->inview || pprt->is_hidden ) continue;

        if ( pinst->size != 0 )
        {
            fvec3_t   vpos;
            float dist;

            vpos.x = pinst->pos.x - vcam.x;
            vpos.y = pinst->pos.y - vcam.y;
            vpos.z = pinst->pos.z - vcam.z;

            dist = fvec3_dot_product( vfwd.v, vpos.v );

            if ( dist > 0 )
            {
                reg[numparticle].index = REF_TO_INT( iprt );
                reg[numparticle].dist  = dist;
                numparticle++;
            }
        }
    }

    // sort the particles from close to far
    qsort( reg, numparticle, sizeof( prt_registry_entity_t ), cmp_prt_registry_entity );

    return numparticle;
}

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_solid( const PRT_REF by_reference iprt )
{
    /// @details BB@> Render the solid version of the particle

    GLvertex vtlist[4];
    int i;

    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !DISPLAY_PRT( iprt ) ) return bfalse;
    pprt = PrtList.lst + iprt;
    pinst = &( pprt->inst );

    // if the particle instance data is not valid, do not continue
    if ( !pinst->valid ) return bfalse;

    // only render solid sprites
    if ( SPRITE_SOLID != pprt->type ) return bfalse;

    // billboard for the particle
    calc_billboard_verts( vtlist, pinst, pinst->size, bfalse );

    ATTRIB_PUSH( "render_one_prt_solid", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
    {
        GL_DEBUG( glDepthMask )( GL_TRUE );           // GL_ENABLE_BIT

        GL_DEBUG( glDisable )( GL_CULL_FACE );        // GL_ENABLE_BIT
        GL_DEBUG( glDisable )( GL_DITHER );           // GL_ENABLE_BIT

        GL_DEBUG( glEnable )( GL_DEPTH_TEST );        // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LESS );           // GL_DEPTH_BUFFER_BIT

        GL_DEBUG( glDisable )( GL_BLEND );            // GL_ENABLE_BIT

        GL_DEBUG( glEnable )( GL_ALPHA_TEST );        // GL_ENABLE_BIT
        GL_DEBUG( glAlphaFunc )( GL_EQUAL, 1 );       // GL_COLOR_BUFFER_BIT

        oglx_texture_Bind( TxTexture_get_ptr(( TX_REF )TX_PARTICLE_TRANS ) ); // GL_CURRENT_BIT

        GL_DEBUG( glColor4f )( pinst->fintens, pinst->fintens, pinst->fintens, 1.0f );

        GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
        {
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                GL_DEBUG( glVertex3fv )( vtlist[i].pos );
            }
        }
        GL_DEBUG_END();
    }
    ATTRIB_POP( "render_one_prt_solid" );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void render_all_prt_solid( camera_t * pcam, prt_registry_entity_t reg[], size_t numparticle )
{
    /// @details BB@> do solid sprites first

    size_t cnt;
    PRT_REF prt;

    gfx_begin_3d( pcam );
    {
        // apply solid particles from near to far
        for ( cnt = 0; cnt < numparticle; cnt++ )
        {
            // Get the index from the color slot
            prt = reg[cnt].index;

            render_one_prt_solid( prt );
        }
    }
    gfx_end_3d();

}

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_trans( const PRT_REF by_reference iprt )
{
    /// @details BB@> do all kinds of transparent sprites next

    GLvertex vtlist[4];
    int i;
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !DISPLAY_PRT( iprt ) ) return bfalse;
    pprt = PrtList.lst + iprt;
    pinst = &( pprt->inst );

    // if the particle instance data is not valid, do not continue
    if ( !pinst->valid ) return bfalse;

    ATTRIB_PUSH( "render_one_prt_trans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
    {
        GL_DEBUG( glDepthMask )( GL_FALSE );        // GL_DEPTH_BUFFER_BIT - do not let transparent objects write into the depth buffer

        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );       // GL_DEPTH_BUFFER_BIT

        calc_billboard_verts( vtlist, pinst, pinst->size, bfalse );

        if ( SPRITE_SOLID == pprt->type )
        {
            // do the alpha blended edge ("anti-aliasing") of the solid particle

            // only display the alpha-edge of the particle
            GL_DEBUG( glEnable )( GL_ALPHA_TEST );        // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_LESS, 1.0f );     // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glColor4f )( pinst->fintens, pinst->fintens, pinst->fintens, 1.0f );  // GL_CURRENT_BIT

            pinst->texture_ref = TX_PARTICLE_TRANS;
            oglx_texture_Bind( TxTexture_get_ptr( pinst->texture_ref ) );

        }
        else if ( SPRITE_LIGHT == pprt->type )
        {
            // do the light sprites
            float intens = pinst->fintens * pinst->falpha;

            GL_DEBUG( glDisable )( GL_ALPHA_TEST );                         // GL_ENABLE_BIT

            GL_DEBUG( glEnable )( GL_BLEND );                               // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );                      // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glColor4f )( intens, intens, intens, 1.0f );          // GL_CURRENT_BIT

            pinst->texture_ref = TX_PARTICLE_LIGHT;
            oglx_texture_Bind( TxTexture_get_ptr( pinst->texture_ref ) );
        }
        else if ( SPRITE_ALPHA == pprt->type )
        {
            // do the transparent sprites

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );                            // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );                         // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

            GL_DEBUG( glColor4f )( pinst->fintens, pinst->fintens, pinst->fintens, pinst->falpha );  // GL_CURRENT_BIT

            pinst->texture_ref = TX_PARTICLE_TRANS;
            oglx_texture_Bind( TxTexture_get_ptr( pinst->texture_ref ) );
        }
        else
        {
            // unknown type
            return bfalse;
        }

        // Go on and draw it
        GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
        {
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                GL_DEBUG( glVertex3fv )( vtlist[i].pos );
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
    /// @details BB@> do all kinds of transparent sprites next

    int cnt;

    gfx_begin_3d( pcam );
    {
        // apply transparent particles from far to near
        for ( cnt = (( int )numparticle ) - 1; cnt >= 0; cnt-- )
        {
            // Get the index from the color slot
            render_one_prt_trans(( PRT_REF )reg[cnt].index );
        }
    }
    gfx_end_3d();
}

//--------------------------------------------------------------------------------------------
void render_prt( camera_t * pcam )
{
    /// @details ZZ@> This function draws the sprites for particle systems

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
    fvec3_t vfwd, vcam;
    PRT_REF cnt;
    size_t  numparticle;

    prt_instance_update_all( pcam );

    vfwd = mat_getCamForward( pcam->mView );
    vcam = pcam->pos;

    // Original points
    numparticle = 0;
    for ( cnt = 0; cnt < maxparticles && numparticle < reg_count; cnt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if ( !DISPLAY_PRT( cnt ) ) continue;
        pprt = PrtList.lst + cnt;
        pinst = &( pprt->inst );

        if ( !pprt->inview || pprt->is_hidden ) continue;

        if ( pinst->size != 0 )
        {
            fvec3_t   vpos;
            float dist;

            vpos = fvec3_sub( pinst->ref_pos.v, vcam.v );
            dist = fvec3_dot_product( vfwd.v, vpos.v );

            if ( dist > 0 )
            {
                reg[numparticle].index = REF_TO_INT( cnt );
                reg[numparticle].dist  = dist;
                numparticle++;
            }
        }
    }

    // sort the particles from close to far
    qsort( reg, numparticle, sizeof( prt_registry_entity_t ), cmp_prt_registry_entity );

    return numparticle;
}

//--------------------------------------------------------------------------------------------
bool_t render_one_prt_ref( const PRT_REF by_reference iprt )
{
    /// @details BB@> render one particle

    GLvertex vtlist[4];
    int startalpha;
    int i;
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !DISPLAY_PRT( iprt ) ) return bfalse;

    pprt = PrtList.lst + iprt;
    pinst = &( pprt->inst );
    if ( !pinst->valid ) return bfalse;

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    calc_billboard_verts( vtlist, pinst, pinst->size, btrue );

    // Fill in the rest of the data
    startalpha = 255 - ( pprt->enviro.floor_level - pinst->ref_pos.z );
    startalpha = CLIP( startalpha, 0, 255 );
    startalpha /= 2;

    //startalpha = ( startalpha | gfx.reffadeor ) >> 1;  // Fix for Riva owners
    //startalpha = CLIP(startalpha, 0, 255);

    if ( startalpha > 0 )
    {
        ATTRIB_PUSH( "render_one_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            GL_DEBUG( glDepthMask )( GL_FALSE );      // ENABLE_BIT

            GL_DEBUG( glEnable )( GL_DEPTH_TEST );    // ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_LEQUAL );     // GL_DEPTH_BUFFER_BIT

            GL_DEBUG( glDisable )( GL_CULL_FACE );    // ENABLE_BIT
            GL_DEBUG( glDisable )( GL_DITHER );       // ENABLE_BIT

            if ( SPRITE_LIGHT == pprt->type )
            {
                // do the light sprites
                float intens = startalpha * INV_FF * pinst->alpha * pinst->fintens;

                GL_DEBUG( glDisable )( GL_ALPHA_TEST );         // ENABLE_BIT

                GL_DEBUG( glEnable )( GL_BLEND );               // ENABLE_BIT
                GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );  // GL_COLOR_BUFFER_BIT

                GL_DEBUG( glColor4f )( intens, intens, intens, 1.0f );      // GL_CURRENT_BIT

                pinst->texture_ref = TX_PARTICLE_LIGHT;
                oglx_texture_Bind( TxTexture_get_ptr( pinst->texture_ref ) );
            }
            else if ( SPRITE_SOLID == pprt->type || SPRITE_ALPHA == pprt->type )
            {
                // do the transparent sprites

                float alpha = startalpha * INV_FF;
                if ( SPRITE_ALPHA == pprt->type )
                {
                    alpha *= pinst->falpha;
                }

                GL_DEBUG( glEnable )( GL_ALPHA_TEST );         // ENABLE_BIT
                GL_DEBUG( glAlphaFunc )( GL_GREATER, 0 );      // GL_COLOR_BUFFER_BIT

                GL_DEBUG( glEnable )( GL_BLEND );                                 // ENABLE_BIT
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

                GL_DEBUG( glColor4f )( pinst->fintens, pinst->fintens, pinst->fintens, alpha ); // GL_CURRENT_BIT

                pinst->texture_ref = TX_PARTICLE_TRANS;
                oglx_texture_Bind( TxTexture_get_ptr( pinst->texture_ref ) );
            }
            else
            {
                // unknown type
                return bfalse;
            }

            GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
            {
                for ( i = 0; i < 4; i++ )
                {
                    GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                    GL_DEBUG( glVertex3fv )( vtlist[i].pos );
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
    size_t cnt;
    PRT_REF prt;

    gfx_begin_3d( pcam );
    {
        // Render each particle that was on
        for ( cnt = 0; cnt < numparticle; cnt++ )
        {
            // Get the index from the color slot
            prt = reg[cnt].index;

            render_one_prt_ref(( PRT_REF )reg[cnt].index );
        }
    }
    gfx_end_3d();
}

//--------------------------------------------------------------------------------------------
void render_prt_ref( camera_t * pcam )
{
    /// @details ZZ@> This function draws sprites reflected in the floor

    prt_registry_entity_t reg[TOTAL_MAX_PRT];
    size_t numparticle;

    numparticle = render_all_prt_ref_begin( pcam, reg, TOTAL_MAX_PRT );
    render_all_prt_ref( pcam, reg, numparticle );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size, bool_t do_reflect )
{
    // Calculate the position of the four corners of the billboard
    // used to display the particle.

    int i, index;
    fvec3_t prt_pos, prt_up, prt_right;

    if ( NULL == vlst || NULL == pinst ) return;

    switch ( REF_TO_INT( pinst->texture_ref ) )
    {
        default:
        case TX_PARTICLE_TRANS:
            index = 0;
            break;

        case TX_PARTICLE_LIGHT:
            index = 1;
            break;
    }

    // use the pre-computed reflection parameters
    if ( do_reflect )
    {
        prt_pos   = pinst->ref_pos;
        prt_up    = pinst->ref_up;
        prt_right = pinst->ref_right;
    }
    else
    {
        prt_pos   = pinst->pos;
        prt_up    = pinst->up;
        prt_right = pinst->right;
    }

    for ( i = 0; i < 4; i++ )
    {
        vlst[i].pos[XX] = prt_pos.x;
        vlst[i].pos[YY] = prt_pos.y;
        vlst[i].pos[ZZ] = prt_pos.z;
    }

    vlst[0].pos[XX] += ( -prt_right.x - prt_up.x ) * size;
    vlst[0].pos[YY] += ( -prt_right.y - prt_up.y ) * size;
    vlst[0].pos[ZZ] += ( -prt_right.z - prt_up.z ) * size;

    vlst[1].pos[XX] += ( prt_right.x - prt_up.x ) * size;
    vlst[1].pos[YY] += ( prt_right.y - prt_up.y ) * size;
    vlst[1].pos[ZZ] += ( prt_right.z - prt_up.z ) * size;

    vlst[2].pos[XX] += ( prt_right.x + prt_up.x ) * size;
    vlst[2].pos[YY] += ( prt_right.y + prt_up.y ) * size;
    vlst[2].pos[ZZ] += ( prt_right.z + prt_up.z ) * size;

    vlst[3].pos[XX] += ( -prt_right.x + prt_up.x ) * size;
    vlst[3].pos[YY] += ( -prt_right.y + prt_up.y ) * size;
    vlst[3].pos[ZZ] += ( -prt_right.z + prt_up.z ) * size;

    vlst[0].tex[SS] = CALCULATE_PRT_U1( index, pinst->image_ref );
    vlst[0].tex[TT] = CALCULATE_PRT_V1( index, pinst->image_ref );

    vlst[1].tex[SS] = CALCULATE_PRT_U0( index, pinst->image_ref );
    vlst[1].tex[TT] = CALCULATE_PRT_V1( index, pinst->image_ref );

    vlst[2].tex[SS] = CALCULATE_PRT_U0( index, pinst->image_ref );
    vlst[2].tex[TT] = CALCULATE_PRT_V0( index, pinst->image_ref );

    vlst[3].tex[SS] = CALCULATE_PRT_U1( index, pinst->image_ref );
    vlst[3].tex[TT] = CALCULATE_PRT_V0( index, pinst->image_ref );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_all_prt_attachment()
{
    PRT_REF cnt;

    GL_DEBUG( glDisable )( GL_BLEND );

    for ( cnt = 0; cnt < maxparticles; cnt++ )
    {
        prt_draw_attached_point( PrtList.lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void draw_one_attacment_point( chr_instance_t * pinst, mad_t * pmad, int vrt_offset )
{
    /// @details BB@> a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.

    Uint32 vrt;
    GLboolean texture_1d_enabled, texture_2d_enabled;

    if ( NULL == pinst || NULL == pmad ) return;

    vrt = ( int )pinst->vrt_count - ( int )vrt_offset;

    if ( vrt < 0 || vrt >= pinst->vrt_count ) return;

    texture_1d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_1D );
    texture_2d_enabled = GL_DEBUG( glIsEnabled )( GL_TEXTURE_2D );

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    if ( texture_1d_enabled ) glDisable( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) glDisable( GL_TEXTURE_2D );

    GL_DEBUG( glPointSize )( 5 );

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
    GL_DEBUG( glMultMatrixf )( pinst->matrix.v );

    GL_DEBUG( glBegin( GL_POINTS ) );
    {
        glVertex3fv( pinst->vrt_lst[vrt].pos );
    }
    GL_DEBUG_END();

    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    if ( texture_1d_enabled ) glEnable( GL_TEXTURE_1D );
    if ( texture_2d_enabled ) glEnable( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void prt_draw_attached_point( prt_t * pprt )
{
    mad_t * pholder_mad;
    cap_t * pholder_cap;
    chr_t * pholder;

    if ( !DISPLAY_PPRT( pprt ) ) return;

    if ( !ACTIVE_CHR( pprt->attachedto_ref ) ) return;
    pholder = ChrList.lst + pprt->attachedto_ref;

    pholder_cap = pro_get_pcap( pholder->iprofile );
    if ( NULL == pholder_cap ) return;

    pholder_mad = chr_get_pmad( GET_REF_PCHR( pholder ) );
    if ( NULL == pholder_mad ) return;

    draw_one_attacment_point( &( pholder->inst ), pholder_mad, pprt->vrt_off );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void prt_instance_update_all( camera_t * pcam )
{
    PRT_REF iprt;

    if ( NULL == pcam ) pcam = PCamera;
    if ( NULL == pcam ) return;

    // only one update per frame
    if ( instance_update == update_wld ) return;
    instance_update = update_wld;

    for ( iprt = 0; iprt < maxparticles; iprt++ )
    {
        prt_t * pprt;
        prt_instance_t * pinst;

        if ( !DISPLAY_PRT( iprt ) ) continue;
        pprt = PrtList.lst + iprt;
        pinst = &( pprt->inst );

        if ( !pprt->inview || pprt->is_hidden || 0 == pprt->size )
        {
            pinst->valid = bfalse;
        }
        else
        {
            // calculate the "billboard" for this particle
            prt_instance_update( pcam, iprt, 255, btrue );
        }
    }
}

//--------------------------------------------------------------------------------------------
void prt_instance_update_vertices( camera_t * pcam, prt_instance_t * pinst, prt_t * pprt )
{
    pip_t * ppip;

    fvec3_t   vfwd, vup, vright;
    fvec3_t   vfwd_ref, vup_ref, vright_ref;

    if ( NULL == pcam || !DISPLAY_PPRT( pprt ) ) return;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return;
    ppip = PipStack.lst + pprt->pip_ref;

    pinst->type = pprt->type;

    pinst->image_ref = FP8_TO_INT( pprt->image + pprt->image_stt );

    // set the position
    pinst->pos         = pprt->pos;
    pinst->orientation = ppip->orientation;

    // calculate the billboard vectors for the reflecions
    pinst->ref_pos      = pprt->pos;
    pinst->ref_pos.z    = 2 * pprt->enviro.floor_level - pinst->pos.z;

    // get the vector from the camera to the particle
    vfwd = fvec3_sub( pinst->pos.v, pcam->pos.v );
    vfwd = fvec3_normalize( vfwd.v );

    vfwd_ref = fvec3_sub( pinst->ref_pos.v, pcam->pos.v );
    vfwd_ref = fvec3_normalize( vfwd_ref.v );

    // set the up and right vectors
    if ( ppip->rotatetoface && !ACTIVE_CHR( pprt->attachedto_ref ) && ( ABS( pprt->vel.x ) + ABS( pprt->vel.y ) + ABS( pprt->vel.z ) > 0 ) )
    {
        // the particle points along its direction of travel

        vup = pprt->vel;
        vup   = fvec3_normalize( vup.v );

        // get the correct "right" vector
        vright = fvec3_cross_product( vfwd.v, vup.v );
        vright = fvec3_normalize( vright.v );

        vup_ref    = vup;
        vright_ref = fvec3_cross_product( vfwd_ref.v, vup.v );
        vright_ref = fvec3_normalize( vright_ref.v );
    }
    else if ( ORIENTATION_B == pinst->orientation )
    {
        // use the camera up vector
        vup = pcam->vup;
        vup = fvec3_normalize( vup.v );

        // get the correct "right" vector
        vright = fvec3_cross_product( vfwd.v, vup.v );
        vright = fvec3_normalize( vright.v );

        vup_ref    = vup;
        vright_ref = fvec3_cross_product( vfwd_ref.v, vup.v );
        vright_ref = fvec3_normalize( vright_ref.v );
    }
    else if ( ORIENTATION_V == pinst->orientation )
    {
        // Using just the global up vector here is too harsh.
        // Smoothly interpolate the global up vector with the camera up vector
        // so that when the camera is looking straight down, the billboard's plane
        // is turned by 45 degrees to the camera (instead of 90 degrees which is invisible)

        float weight;
        fvec3_t vup_cam;

        // use the camera up vector
        vup_cam = pcam->vup;

        // use the global up vector
        vup.x = vup.y = 0;
        vup.z = 1;

        // adjust the vector so that the particle doesn't disappear if
        // you are viewing it from from the top or the bottom
        weight = 1.0f - ABS( vup_cam.z );
        if ( vup_cam.z < 0 ) weight *= -1;

        vup.x = vup.x + weight * vup_cam.x;
        vup.y = vup.y + weight * vup_cam.y;
        vup.z = vup.z + weight * vup_cam.z;
        vup = fvec3_normalize( vup.v );

        // get the correct "right" vector
        vright = fvec3_cross_product( vfwd.v, vup.v );
        vright = fvec3_normalize( vright.v );

        vright_ref = fvec3_cross_product( vfwd.v, vup_ref.v );
        vright_ref = fvec3_normalize( vright_ref.v );

        vup_ref    = vup;
        vright_ref = fvec3_cross_product( vfwd_ref.v, vup.v );
        vright_ref = fvec3_normalize( vright_ref.v );
    }
    else if ( ORIENTATION_H == pinst->orientation )
    {
        vup.x = vup.y = 0;
        vup.z = 1;

        // force right to be horizontal
        vright = fvec3_cross_product( vfwd.v, vup.v );

        // force "up" to be close to the camera forward, but horizontal
        vup = fvec3_cross_product( vup.v, vright.v );
        vup_ref = fvec3_cross_product( vup.v, vright_ref.v );

        // normalize them
        vright = fvec3_normalize( vright.v );
        vup    = fvec3_normalize( vup.v );

        vright_ref = vright;
        vup_ref    = vup;
    }
    else if ( ACTIVE_CHR( pprt->attachedto_ref ) )
    {
        chr_instance_t * cinst = chr_get_pinstance( pprt->attachedto_ref );

        if ( chr_matrix_valid( ChrList.lst + pprt->attachedto_ref ) )
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

            vup = fvec3_normalize( vup.v );
        }
        else
        {
            // use the camera directions?
            switch ( pinst->orientation )
            {
                case ORIENTATION_X: vup = pcam->vfw; break;
                case ORIENTATION_Y: vup = pcam->vrt; break;

                default:
                case ORIENTATION_Z: vup = pcam->vup; break;
            }
        }

        vup = fvec3_normalize( vup.v );

        // get the correct "right" vector
        vright = fvec3_cross_product( vfwd.v, vup.v );
        vright = fvec3_normalize( vright.v );

        vup_ref    = vup;
        vright_ref = fvec3_cross_product( vfwd_ref.v, vup.v );
        vright_ref = fvec3_normalize( vright_ref.v );
    }
    else
    {
        // use the camera up vector
        vup = pcam->vup;
        vup = fvec3_normalize( vup.v );

        // get the correct "right" vector
        vright = fvec3_cross_product( vfwd.v, vup.v );
        vright = fvec3_normalize( vright.v );

        vup_ref    = vup;
        vright_ref = fvec3_cross_product( vfwd_ref.v, vup.v );
        vright_ref = fvec3_normalize( vright_ref.v );
    }

    // calculate the actual vectors using the particle rotation
    if ( 0 == pprt->rotate )
    {
        pinst->up    = vup;
        pinst->right = vright;

        pinst->ref_up    = vup_ref;
        pinst->ref_right = vright_ref;
    }
    else
    {
        float sinval, cosval;
        Uint16 turn    = pprt->rotate >> 2;

        cosval = turntocos[turn & TRIG_TABLE_MASK ];
        sinval = turntosin[turn & TRIG_TABLE_MASK ];

        pinst->up.x    = vup.x * cosval - vright.x * sinval;
        pinst->up.y    = vup.y * cosval - vright.y * sinval;
        pinst->up.z    = vup.z * cosval - vright.z * sinval;

        pinst->right.x = vup.x * sinval + vright.x * cosval;
        pinst->right.y = vup.y * sinval + vright.y * cosval;
        pinst->right.z = vup.z * sinval + vright.z * cosval;

        pinst->ref_up.x    = vup_ref.x * cosval - vright_ref.x * sinval;
        pinst->ref_up.y    = vup_ref.y * cosval - vright_ref.y * sinval;
        pinst->ref_up.z    = vup_ref.z * cosval - vright_ref.z * sinval;

        pinst->ref_right.x = vup_ref.x * sinval + vright_ref.x * cosval;
        pinst->ref_right.y = vup_ref.y * sinval + vright_ref.y * cosval;
        pinst->ref_right.z = vup_ref.z * sinval + vright_ref.z * cosval;
    }

    // calculate the billboard normal
    pinst->nrm = fvec3_cross_product( pinst->right.v, pinst->up.v );

    // flip the normal so that the front front of the quad is toward the camera
    if ( fvec3_dot_product( vfwd.v, pinst->nrm.v ) < 0 )
    {
        pinst->nrm.x *= -1;
        pinst->nrm.y *= -1;
        pinst->nrm.z *= -1;
    }

    // Now we have to calculate the mirror-like reflection of the particles
    // this was a bit hard to figure. What happens is that the components of the
    // up and right vectors that are in the plane of the quad and closest to the world up are reversed.
    //
    // This is easy to think about in a couple of examples:
    // 1) If the quad is like a picture frame then whatever component (up or right)
    //    that actually points in the wodld up direction is reversed.
    //    This corresponds to the case where zdot == +/- 1 in the code below
    //
    // 2) If the particle is like a rug, then basically nothing happens since
    //    neither the up or right vectors point in the wodld up direction.
    //    This corresponds to ndot == 0 in the code below.
    //
    // This process does not affect the normal the length of the vector, or the
    // direction of the normal to the quad.

    {
        float zdot;  // the dot product between the up or the right vector and the world up
        float ndot;  // the dot product between either the up or the right vector
        float factor;
        fvec3_t world_up;

        // the normal sense of "up"
        world_up.x = world_up.y = 0;
        world_up.z = 1.0f;

        // the following statement could be optimized
        // since we know the only non-zero component of world_up is z
        ndot = fvec3_dot_product( pinst->nrm.v, world_up.v );

        // do nothing if the quad is basically horizontal
        if ( ndot < 1.0f - 1e-6 )
        {
            //---- do the right vector first
            {
                // the following statement could be optimized
                // since we know the only non-zero component of world_up is z
                zdot = fvec3_dot_product( pinst->ref_right.v, world_up.v );

                if ( ABS( zdot ) > 1e-6 )
                {
                    factor = zdot / ( 1.0f - ndot * ndot );

                    pinst->ref_right.x += 2.0f * factor * ( ndot * pinst->nrm.x - world_up.x );
                    pinst->ref_right.y += 2.0f * factor * ( ndot * pinst->nrm.y - world_up.y );
                    pinst->ref_right.z += 2.0f * factor * ( ndot * pinst->nrm.z - world_up.z );
                }
            }

            //---- do the up vector second
            {
                // the following statement could be optimized
                // since we know the only non-zero component of world_up is z
                zdot = fvec3_dot_product( pinst->ref_up.v, world_up.v );

                if ( ABS( zdot ) > 1e-6 )
                {
                    factor = zdot / ( 1.0f - ndot * ndot );

                    pinst->ref_up.x += 2.0f * factor * ( ndot * pinst->nrm.x - world_up.x );
                    pinst->ref_up.y += 2.0f * factor * ( ndot * pinst->nrm.y - world_up.y );
                    pinst->ref_up.z += 2.0f * factor * ( ndot * pinst->nrm.z - world_up.z );
                }
            }
        }
    }

    // set some particle dependent properties
    pinst->scale = 0.25f;
    switch ( pinst->type )
    {
        case SPRITE_SOLID: pinst->scale *= 0.9384f; break;
        case SPRITE_ALPHA: pinst->scale *= 0.9353f; break;
        case SPRITE_LIGHT: pinst->scale *= 1.5912f; break;
    }
    pinst->size = FP8_TO_FLOAT( pprt->size ) * pinst->scale;

    if ( cfg.dev_mode && SDLKEYDOWN( SDLK_F8 ) )
    {
        // a useful little mod to help with debugging particles
        // will make things like the bare-handed-attack particles visible
        pinst->size = MAX( 90, pinst->size );
    }

    // this instance is now completely valid
    pinst->valid = btrue;
}

//--------------------------------------------------------------------------------------------
fmat_4x4_t prt_instance_make_matrix( prt_instance_t * pinst )
{
    fmat_4x4_t mat = IdentityMatrix();

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
    Uint32 alpha;
    Sint16  self_light;
    lighting_cache_t global_light, loc_light;
    float amb, dir;
    fmat_4x4_t mat;

    if ( NULL == pinst || NULL == pprt ) return;

    // To make life easier
    alpha = trans;

    // interpolate the lighting for the origin of the object
    grid_lighting_interpolate( PMesh, &global_light, pinst->pos.x, pinst->pos.y );

    // rotate the lighting data to body_centered coordinates
    mat = prt_instance_make_matrix( pinst );
    lighting_project_cache( &loc_light, &global_light, mat );

    // determine the normal dependent amount of light
    lighting_evaluate_cache( &loc_light, pinst->nrm.v, pinst->pos.z, PMesh->tmem.bbox, &amb, &dir );

    // LIGHT-blended sprites automatically glow. ALPHA-blended and SOLID
    // sprites need to convert the light channel into additional alpha
    // lighting to make them "glow"
    self_light = 0;
    if ( SPRITE_LIGHT != pinst->type )
    {
        self_light  = ( 255 == pinst->light ) ? 0 : pinst->light;
    }

    // determine the ambient lighting
    pinst->famb = 0.9f * pinst->famb + 0.1f * ( self_light + amb );
    pinst->fdir = 0.9f * pinst->fdir + 0.1f * dir;

    // determine the overall lighting
    pinst->fintens = pinst->fdir * INV_FF;
    if ( do_lighting )
    {
        pinst->fintens += pinst->famb * INV_FF;
    }
    pinst->fintens = CLIP( pinst->fintens, 0.0f, 1.0f );

    // determine the alpha component
    pinst->falpha = ( alpha * INV_FF ) * ( pinst->alpha * INV_FF );
    pinst->falpha = CLIP( pinst->falpha, 0.0f, 1.0f );
}

//--------------------------------------------------------------------------------------------
void prt_instance_update( camera_t * pcam, const PRT_REF by_reference particle, Uint8 trans, bool_t do_lighting )
{
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !DISPLAY_PRT( particle ) ) return;
    pprt = PrtList.lst + particle;
    pinst = &( pprt->inst );

    // make sure that the vertices are interpolated
    prt_instance_update_vertices( pcam, pinst, pprt );

    // do the lighting
    prt_instance_update_lighting( pinst, pprt, trans, do_lighting );
}
