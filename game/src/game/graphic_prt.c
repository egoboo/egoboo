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

/// @file  game/graphic_prt.c
/// @brief Particle system drawing and management code.
/// @details

#include "egolib/bbox.h"
#include "game/graphic_prt.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/game.h"
#include "game/input.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "game/char.h"
#include "game/Profiles/_Include.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Entities/_Include.hpp"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_prt_registry_entity;
typedef struct s_prt_registry_entity prt_registry_entity_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// dynamically calculate the particle texture coordinates

int   ptex_w[2] = {256, 256};
int   ptex_h[2] = {256, 256};
float ptex_wscale[2] = {1.0f, 1.0f};
float ptex_hscale[2] = {1.0f, 1.0f};

//--------------------------------------------------------------------------------------------
int prt_get_texture_style( const TX_REF itex )
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
void prt_set_texture_params( const TX_REF itex )
{
    int index = prt_get_texture_style( itex );
    if (index < 0)
    {
        return;
    }
    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(itex);
    if (!ptex)
    {
        return;
    }
    ptex_w[index] = oglx_texture_t::getSourceWidth(ptex);
    ptex_h[index] = oglx_texture_t::getSourceHeight(ptex);
    ptex_wscale[index] = (float)oglx_texture_t::getSourceWidth(ptex) / (float)oglx_texture_t::getWidth(ptex);
    ptex_hscale[index] = (float)oglx_texture_t::getSourceHeight(ptex) / (float)oglx_texture_t::getHeight(ptex);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data values necessary to sort particles by their position to the camera
struct s_prt_registry_entity
{
    PRT_REF index;
    float   dist;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static gfx_rv prt_instance_update( std::shared_ptr<Camera> pcam, const PRT_REF particle, Uint8 trans, bool do_lighting );
static void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size, bool do_reflect );
static int  cmp_prt_registry_entity( const void * vlhs, const void * vrhs );

static void draw_one_attachment_point( chr_instance_t * pinst, mad_t * pmad, int vrt_offset );
static void prt_draw_attached_point( prt_bundle_t * pbdl_prt );

static void render_prt_bbox( prt_bundle_t * pbdl_prt );

static gfx_rv prt_instance_update_vertices( std::shared_ptr<Camera> pcam, prt_instance_t * pinst, prt_t * pprt );
static fmat_4x4_t prt_instance_make_matrix( prt_instance_t * pinst );
static gfx_rv prt_instance_update_lighting( prt_instance_t * pinst, prt_t * pprt, Uint8 trans, bool do_lighting );

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
gfx_rv render_one_prt_solid( const PRT_REF iprt )
{
    /// @author BB
    /// @details Render the solid version of the particle

    GLvertex vtlist[4];
    int i;

    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !DISPLAY_PRT( iprt ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, iprt, "invalid particle" );
        return gfx_error;
    }
    pprt = ParticleHandler::get().get_ptr( iprt );

    // if the particle is hidden, do not continue
    if ( pprt->is_hidden ) return gfx_fail;

    // if the particle instance data is not valid, do not continue
    if ( !pprt->inst.valid ) return gfx_fail;
    pinst = &( pprt->inst );

    // only render solid sprites
    if ( SPRITE_SOLID != pprt->type ) return gfx_fail;

    // billboard for the particle
    calc_billboard_verts( vtlist, pinst, pinst->size, false );

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
    {
        // Use the depth test to eliminate hidden portions of the particle
		Ego::Renderer::get().setDepthTestEnabled(true);
        Ego::Renderer::get().setDepthFunction(Ego::CompareFunction::Less);                                   // GL_DEPTH_BUFFER_BIT

        // enable the depth mask for the solid portion of the particles
        Ego::Renderer::get().setDepthWriteEnabled(true);

        // draw draw front and back faces of polygons
        oglx_end_culling();        // GL_ENABLE_BIT

        // Since the textures are probably mipmapped or minified with some kind of
        // interpolation, we can never really turn blending off.
        Ego::Renderer::get().setBlendingEnabled(true);
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );            // GL_ENABLE_BIT

        // only display the portion of the particle that is 100% solid
        Ego::Renderer::get().setAlphaTestEnabled(true);
        GL_DEBUG( glAlphaFunc )( GL_EQUAL, 1.0f );       // GL_COLOR_BUFFER_BIT

		oglx_texture_t::bind(TextureManager::get().get_valid_ptr((TX_REF)TX_PARTICLE_TRANS));

        GL_DEBUG( glColor4f )( pinst->fintens, pinst->fintens, pinst->fintens, 1.0f );  // GL_CURRENT_BIT

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
    ATTRIB_POP( __FUNCTION__ );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_one_prt_trans( const PRT_REF iprt )
{
    /// @author BB
    /// @details do all kinds of transparent sprites next

    GLvertex vtlist[4];
    int i;
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !DISPLAY_PRT( iprt ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, iprt, "invalid particle" );
        return gfx_error;
    }
    pprt = ParticleHandler::get().get_ptr( iprt );

    // if the particle is hidden, do not continue
    if ( pprt->is_hidden ) return gfx_fail;

    // if the particle instance data is not valid, do not continue
    if ( !pprt->inst.valid ) return gfx_fail;
    pinst = &( pprt->inst );

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
    {
        // Do not write into the depth buffer.
        Ego::Renderer::get().setDepthWriteEnabled(false);

        // Enable depth test: Incoming fragment's depth value must be less or equal.
		Ego::Renderer::get().setDepthTestEnabled(true);
        Ego::Renderer::get().setDepthFunction(Ego::CompareFunction::LessOrEqual);

        // Draw front-facing and back-facing polygons.
        oglx_end_culling();

        Ego::Math::Colour4f particleColour;
        bool drawParticle = false;
        // Solid sprites.
        if (SPRITE_SOLID == pprt->type)
        {
            // Do the alpha blended edge ("anti-aliasing") of the solid particle.
            // Only display the alpha-edge of the particle.
            Ego::Renderer::get().setAlphaTestEnabled(true);
            GL_DEBUG(glAlphaFunc)(GL_LESS, 1.0f);     // GL_COLOR_BUFFER_BIT

            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // GL_COLOR_BUFFER_BIT

            float fintens = pinst->fintens;
            particleColour = Ego::Math::Colour4f(fintens, fintens, fintens, 1.0f);

            pinst->texture_ref = TX_PARTICLE_TRANS;
			oglx_texture_t::bind(TextureManager::get().get_valid_ptr(pinst->texture_ref));

            drawParticle = true;
        }
        // Light sprites.
        else if (SPRITE_LIGHT == pprt->type)
        {
            Ego::Renderer::get().setAlphaTestEnabled(false);
            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG(glBlendFunc)(GL_ONE, GL_ONE);

            float fintens = pinst->fintens * pinst->falpha;
            particleColour = Ego::Math::Colour4f(fintens, fintens, fintens, 1.0f);

            pinst->texture_ref = TX_PARTICLE_LIGHT;
			oglx_texture_t::bind(TextureManager::get().get_valid_ptr(pinst->texture_ref));

            drawParticle = (fintens > 0.0f);
        }
        // Transparent sprites.
        else if (SPRITE_ALPHA == pprt->type)
        {
            // do not display the completely transparent portion
            Ego::Renderer::get().setAlphaTestEnabled(true);
            GL_DEBUG(glAlphaFunc)(GL_GREATER, 0.0f);                      // GL_COLOR_BUFFER_BIT

            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // GL_COLOR_BUFFER_BIT

            float fintens = pinst->fintens;
            float falpha = pinst->falpha;
            particleColour = Ego::Math::Colour4f(fintens, fintens, fintens, falpha);

            pinst->texture_ref = TX_PARTICLE_TRANS;
			oglx_texture_t::bind(TextureManager::get().get_valid_ptr(pinst->texture_ref));

            drawParticle = (falpha > 0.0f);
        }
        else
        {
            // unknown type
            return gfx_error;
        }

        if (drawParticle)
        {
            calc_billboard_verts(vtlist, pinst, pinst->size, false);

            Ego::Renderer::get().setColour(particleColour);

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
    }
    ATTRIB_POP( __FUNCTION__ );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

gfx_rv render_one_prt_ref( const PRT_REF iprt )
{
    /// @author BB
    /// @details render one particle

    GLvertex vtlist[4];
    int startalpha;
    int i;
    prt_t * pprt;
    prt_instance_t * pinst;

    if ( !DISPLAY_PRT( iprt ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, iprt, "invalid particle" );
        return gfx_error;
    }
    pprt = ParticleHandler::get().get_ptr( iprt );

    // if the particle is hidden, do not continue
    if ( pprt->is_hidden ) return gfx_fail;

    if ( !pprt->inst.valid || !pprt->inst.ref_valid ) return gfx_fail;
    pinst = &( pprt->inst );

    // Fill in the rest of the data. (make it match the case for characters)
    startalpha = 255;
    startalpha -= 2.0f * ( pprt->enviro.floor_level - pinst->ref_pos.z );
    startalpha *= 0.5f;
    startalpha = CLIP( startalpha, 0, 255 );

    //startalpha = ( startalpha | gfx.reffadeor ) >> 1;  // Fix for Riva owners
    //startalpha = CLIP(startalpha, 0, 255);

    if ( startalpha > 0 )
    {
        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
        {
            bool draw_particle;
            GLXvector4f particle_color;

            auto& renderer = Ego::Renderer::get();
            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );      // ENABLE_BIT

            // do not draw hidden surfaces
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

            // draw draw front and back faces of polygons
            oglx_end_culling();    // ENABLE_BIT

            draw_particle = false;
            if ( SPRITE_LIGHT == pprt->type )
            {
                // do the light sprites
                float intens = startalpha * INV_FF * pinst->falpha * pinst->fintens;

                renderer.setAlphaTestEnabled(false);

                renderer.setBlendingEnabled(true);
                GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );  // GL_COLOR_BUFFER_BIT

                particle_color[RR] = intens;
                particle_color[GG] = intens;
                particle_color[BB] = intens;
                particle_color[AA] = 1.0f;

                pinst->texture_ref = TX_PARTICLE_LIGHT;
                oglx_texture_t::bind(TextureManager::get().get_valid_ptr(pinst->texture_ref));

                draw_particle = intens > 0.0f;
            }
            else if ( SPRITE_SOLID == pprt->type || SPRITE_ALPHA == pprt->type )
            {
                // do the transparent sprites

                float alpha = startalpha * INV_FF;
                if ( SPRITE_ALPHA == pprt->type )
                {
                    alpha *= pinst->falpha;
                }

                auto& renderer = Ego::Renderer::get();
                // do not display the completely transparent portion
                renderer.setAlphaTestEnabled(true);
                GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );      // GL_COLOR_BUFFER_BIT

                renderer.setBlendingEnabled(true);
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

                particle_color[RR] = pinst->fintens;
                particle_color[GG] = pinst->fintens;
                particle_color[BB] = pinst->fintens;
                particle_color[AA] = alpha;

                pinst->texture_ref = TX_PARTICLE_TRANS;
                oglx_texture_t::bind(TextureManager::get().get_valid_ptr(pinst->texture_ref));

                draw_particle = alpha > 0.0f;
            }
            else
            {
                // unknown type
                return gfx_fail;
            }

            if ( draw_particle )
            {
                // Calculate the position of the four corners of the billboard
                // used to display the particle.
                calc_billboard_verts( vtlist, pinst, pinst->size, true );

                GL_DEBUG( glColor4fv )( particle_color );      // GL_CURRENT_BIT

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
        }
        ATTRIB_POP( __FUNCTION__ );

    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void calc_billboard_verts( GLvertex vlst[], prt_instance_t * pinst, float size, bool do_reflect )
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
    Ego::Renderer::get().setBlendingEnabled(false);

    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        prt_draw_attached_point( &prt_bdl );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void render_all_prt_bbox()
{
    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        render_prt_bbox( &prt_bdl );
    }
    PRT_END_LOOP();
}

//--------------------------------------------------------------------------------------------
void draw_one_attachment_point( chr_instance_t * pinst, mad_t * pmad, int vrt_offset )
{
    /// @author BB
    /// @details a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.

    Uint32 vrt;

    GLint matrix_mode[1];

    if ( NULL == pinst || NULL == pmad ) return;

    vrt = ( int )pinst->vrt_count - ( int )vrt_offset;

    if ( vrt < 0 || vrt >= pinst->vrt_count ) return;

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    oglx_texture_t::bind(nullptr);

    GL_DEBUG( glPointSize )( 5 );

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().multiplyMatrix(pinst->matrix);
    GL_DEBUG( glBegin( GL_POINTS ) );
    {
        GL_DEBUG( glVertex3fv )( pinst->vrt_lst[vrt].pos );
    }
    GL_DEBUG_END();

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // restore the matrix mode
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );
}

//--------------------------------------------------------------------------------------------
void prt_draw_attached_point( prt_bundle_t * pbdl_prt )
{
    mad_t * pholder_mad;
    Object * pholder;

    prt_t * loc_pprt;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return;
    loc_pprt = pbdl_prt->prt_ptr;

    if ( !DISPLAY_PPRT( loc_pprt ) ) return;

    if ( !_gameObjects.exists( loc_pprt->attachedto_ref ) ) return;
    pholder = _gameObjects.get( loc_pprt->attachedto_ref );

    pholder_mad = chr_get_pmad( GET_INDEX_PCHR( pholder ) );
    if ( NULL == pholder_mad ) return;

    draw_one_attachment_point( &( pholder->inst ), pholder_mad, loc_pprt->attachedto_vrt_off );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_rv update_all_prt_instance( std::shared_ptr<Camera> pcam )
{
    gfx_rv retval;

    if ( nullptr == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    // only one update per frame
    if ( instance_update == update_wld ) return gfx_success;
    instance_update = update_wld;

    // assume the best
    retval = gfx_success;

    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        prt_instance_t * pinst;

        pinst = &( prt_bdl.prt_ptr->inst );

        // only do frame counting for particles that are fully activated!
        prt_bdl.prt_ptr->obj_base.frame_count++;

        if ( !prt_bdl.prt_ptr->inst.indolist )
        {
            pinst->valid     = false;
            pinst->ref_valid = false;
        }
        else
        {
            // calculate the "billboard" for this particle
            if ( gfx_error == prt_instance_update( pcam, iprt, 255, true ) )
            {
                retval = gfx_error;
            }
        }
    }
    PRT_END_LOOP();

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv prt_instance_update_vertices( std::shared_ptr<Camera> pcam, prt_instance_t * pinst, prt_t * pprt )
{
    pip_t * ppip;

    fvec3_t   vfwd, vup = fvec3_t(0.0f, 0.0f, 1.0f), vright;
    fvec3_t   vfwd_ref, vup_ref = fvec3_t(0.0f, 0.0f, 1.0f), vright_ref;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL particle instance" );
        return gfx_error;
    }
    pinst->valid     = false;
    pinst->ref_valid = false;

    if ( nullptr == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    if ( !DISPLAY_PPRT( pprt ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, GET_REF_PPRT( pprt ), "invalid particle" );
        return gfx_error;
    }

    if ( !LOADED_PIP( pprt->pip_ref ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pprt->pip_ref, "invalid pip" );
        return gfx_error;
    }
    ppip = PipStack.get_ptr( pprt->pip_ref );

    pinst->type = pprt->type;

    pinst->image_ref = (pprt->_image._start / EGO_ANIMATION_MULTIPLIER + pprt->_image._offset / EGO_ANIMATION_MULTIPLIER);


    // set the position
    prt_t::get_pos(pprt, pinst->pos);
    pinst->orientation = ppip->orientation;

    // calculate the billboard vectors for the reflecions
    prt_t::get_pos(pprt, pinst->ref_pos);
    pinst->ref_pos.z = 2 * pprt->enviro.floor_level - pinst->pos.z;

    // get the vector from the camera to the particle
    vfwd = pinst->pos - pcam->getPosition();
	vfwd.normalize();

	vfwd_ref = pinst->ref_pos - pcam->getPosition();
	vfwd_ref.normalize();

    // set the up and right vectors
    if (ppip->rotatetoface && !_gameObjects.exists(pprt->attachedto_ref) && (pprt->vel.length_abs() > 0))
    {
        // the particle points along its direction of travel

        vup = pprt->vel;
		vup.normalize();

        // get the correct "right" vector
		vright = vfwd.cross(vup);
		vright.normalize();

        vup_ref = vup;
        vright_ref = vfwd_ref.cross(vup);
		vright_ref.normalize();
    }
    else if ( ORIENTATION_B == pinst->orientation )
    {
        // use the camera up vector
        vup = pcam->getVUP();
		vup.normalize();

        // get the correct "right" vector
        vright = vfwd.cross(vup);
		vright.normalize();

        vup_ref = vup;
		vright_ref = vfwd_ref.cross(vup);
		vright_ref.normalize();
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
        vup_cam = pcam->getVUP();

        // use the global up vector
        vup.x = vup.y = 0;
        vup.z = 1;

        // adjust the vector so that the particle doesn't disappear if
        // you are viewing it from from the top or the bottom
        weight = 1.0f - std::abs(vup_cam.z);
        if ( vup_cam.z < 0 ) weight *= -1;

        vup.x = vup.x + weight * vup_cam.x;
        vup.y = vup.y + weight * vup_cam.y;
        vup.z = vup.z + weight * vup_cam.z;
		vup.normalize();

        // get the correct "right" vector
        vright = vfwd.cross(vup);
		vright.normalize();

        vright_ref = vfwd.cross(vup_ref);
		vright_ref.normalize();

        vup_ref = vup;
		vright_ref = vfwd_ref.cross(vup);
		vright_ref.normalize();
    }
    else if ( ORIENTATION_H == pinst->orientation )
    {
        fvec3_t vert = fvec3_t(0.0f, 0.0f, 1.0f);

        // force right to be horizontal
        vright = vfwd.cross(vert);

        // force "up" to be close to the camera forward, but horizontal
        vup = vert.cross(vright);
		vup_ref = vert.cross(vright_ref);

        // normalize them
		vright.normalize();
		vup.normalize();

        vright_ref = vright;
        vup_ref    = vup;
    }
    else if ( _gameObjects.exists( pprt->attachedto_ref ) )
    {
        chr_instance_t * cinst = chr_get_pinstance( pprt->attachedto_ref );

        if ( chr_matrix_valid( _gameObjects.get( pprt->attachedto_ref ) ) )
        {
            // use the character matrix to orient the particle
            // assume that the particle "up" is in the z-direction in the object's
            // body fixed axes. should work for the gonnes & such

            switch ( pinst->orientation )
            {
                case ORIENTATION_X: mat_getChrForward(cinst->matrix, vup); break;
                case ORIENTATION_Y: mat_getChrRight(cinst->matrix, vup);   break;
                default:
                case ORIENTATION_Z: mat_getChrUp(cinst->matrix, vup);      break;
            }

			vup.normalize();
        }
        else
        {
            // use the camera directions?
            switch ( pinst->orientation )
            {
                case ORIENTATION_X: vup = pcam->getVFW(); break;
                case ORIENTATION_Y: vup = pcam->getVRT(); break;

                default:
                case ORIENTATION_Z: vup = pcam->getVUP(); break;
            }
        }

		vup.normalize();

        // get the correct "right" vector
		vright = vfwd.cross(vup);
        vright.normalize();

        vup_ref = vup;
        vright_ref = vfwd_ref.cross(vup);
        vright_ref.normalize();
    }
    else
    {
        // use the camera up vector
        vup = pcam->getVUP();
		vup.normalize();

        // get the correct "right" vector
        vright = vfwd.cross(vup);
		vright.normalize();

        vup_ref = vup;
		vright_ref = vfwd_ref.cross(vup);
		vright_ref.normalize();
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
        float  sinval, cosval;
        TURN_T turn;

        turn = TO_TURN( pprt->rotate );
        cosval = turntocos[ turn ];
        sinval = turntosin[ turn ];

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
    pinst->nrm = pinst->right.cross(pinst->up);

    // flip the normal so that the front front of the quad is toward the camera
    if (vfwd.dot(pinst->nrm) < 0)
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
    //    neither the up or right vectors point in the world up direction.
    //    This corresponds to 0 == ndot in the code below.
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
        ndot = pinst->nrm.dot(world_up);

        // do nothing if the quad is basically horizontal
        if ( ndot < 1.0f - 1e-6 )
        {
            //---- do the right vector first
            {
                // the following statement could be optimized
                // since we know the only non-zero component of world_up is z
                zdot = pinst->ref_right.dot(world_up);

                if (std::abs(zdot) > 1e-6)
                {
                    factor = zdot / ( 1.0f - ndot * ndot );

                    pinst->ref_right += ((pinst->nrm * ndot) - world_up) * 2.0f * factor;
                }
            }

            //---- do the up vector second
            {
                // the following statement could be optimized
                // since we know the only non-zero component of world_up is z
                zdot = pinst->ref_up.dot(world_up);

                if ( std::abs( zdot ) > 1e-6 )
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
    pinst->scale = prt_t::get_scale( pprt );
    pinst->size  = FP8_TO_FLOAT( pprt->size ) * pinst->scale;

    // this instance is now completely valid
    pinst->valid     = true;
    pinst->ref_valid = true;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
fmat_4x4_t prt_instance_make_matrix( prt_instance_t * pinst )
{
    fmat_4x4_t mat = fmat_4x4_t::identity();

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
gfx_rv prt_instance_update_lighting( prt_instance_t * pinst, prt_t * pprt, Uint8 trans, bool do_lighting )
{
    Uint32 alpha;
    Sint16  self_light;
    lighting_cache_t global_light, loc_light;
    float amb, dir;
    fmat_4x4_t mat;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }
    if ( NULL == pprt )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL particle" );
        return gfx_error;
    }

    // To make life easier
    alpha = trans;

    // interpolate the lighting for the origin of the object
    grid_lighting_interpolate( PMesh, &global_light, fvec2_t(pinst->pos[kX],pinst->pos[kY]) );

    // rotate the lighting data to body_centered coordinates
    mat = prt_instance_make_matrix( pinst );
    lighting_project_cache( &loc_light, &global_light, mat );

    // determine the normal dependent amount of light
    lighting_evaluate_cache( &loc_light, pinst->nrm, pinst->pos.z, PMesh->tmem.bbox, &amb, &dir );

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

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv prt_instance_update( std::shared_ptr<Camera> pcam, const PRT_REF particle, Uint8 trans, bool do_lighting )
{
    prt_t          * pprt;
    prt_instance_t * pinst;
    gfx_rv        retval;

    if ( !DISPLAY_PRT( particle ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, particle, "invalid particle" );
        return gfx_error;
    }
    pprt = ParticleHandler::get().get_ptr( particle );
    pinst = &( pprt->inst );

    // assume the best
    retval = gfx_success;

    // make sure that the vertices are interpolated
    if ( gfx_error == prt_instance_update_vertices( pcam, pinst, pprt ) )
    {
        retval = gfx_error;
    }

    // do the lighting
    if ( gfx_error == prt_instance_update_lighting( pinst, pprt, trans, do_lighting ) )
    {
        retval = gfx_error;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void render_prt_bbox( prt_bundle_t * pbdl_prt )
{
    prt_t * loc_pprt;
    pip_t * loc_ppip;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    // only draw bullets
    //if ( 50 != loc_ppip->vel_hrz_pair.base ) return;

    if ( !DISPLAY_PPRT( loc_pprt ) ) return;

    // draw the object bounding box as a part of the graphics debug mode F7
    if ((egoboo_config_t::get().debug_developerMode_enable.getValue() && SDL_KEYDOWN(keyb, SDLK_F7)))
    {
        oct_bb_t loc_bb, tmp_bb, exp_bb;

        // copy the bounding volume
        tmp_bb = loc_pprt->prt_max_cv;

        // make sure that it has some minimum extent
        //for(cnt = 0; cnt < OCT_COUNT; cnt++ )
        //{
        //    tmp_bb.mins[cnt] = std::min( tmp_bb.mins[cnt], -1 );
        //    tmp_bb.maxs[cnt] = std::max( tmp_bb.mins[cnt],  1 );
        //}

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb(tmp_bb, loc_pprt->vel, 0, 1, exp_bb);

        // shift the source bounding boxes to be centered on the given positions
        oct_bb_translate( &exp_bb, loc_pprt->pos, &loc_bb );

        oglx_texture_t::bind(nullptr);
        {
            Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());

            render_oct_bb(&loc_bb, true, true);
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// EXAMPLE CODE THAT IS NOT USED
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void   render_all_particles( struct s_camera * pcam );
static size_t render_all_prt_begin( std::shared_ptr<Camera> pcam, prt_registry_entity_t reg[], size_t reg_count );
static void   render_all_prt_solid( std::shared_ptr<Camera> pcam, const prt_registry_entity_t reg[], const size_t numparticle );
static void   render_all_prt_trans( std::shared_ptr<Camera> pcam, const prt_registry_entity_t reg[], const size_t numparticle );

//--------------------------------------------------------------------------------------------
void render_all_particles( std::shared_ptr<Camera> pcam )
{
    /// @author ZZ
    /// @details This function draws the sprites for particle systems

    prt_registry_entity_t reg[PARTICLES_MAX];
    size_t numparticle;

    numparticle = render_all_prt_begin( pcam, reg, PARTICLES_MAX );

    render_all_prt_solid( pcam, reg, numparticle );
    render_all_prt_trans( pcam, reg, numparticle );
}

//--------------------------------------------------------------------------------------------
size_t render_all_prt_begin( std::shared_ptr<Camera> pcam,  prt_registry_entity_t reg[], size_t reg_count )
{
    fvec3_t vfwd, vcam;
    size_t  numparticle;

    update_all_prt_instance( pcam );

    mat_getCamForward(pcam->getView(), vfwd);
    vcam = pcam->getPosition();

    // Original points
    numparticle = 0;
    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        prt_instance_t * pinst;

        if ( numparticle >= reg_count ) break;

        pinst = &( prt_bdl.prt_ptr->inst );

        if ( !pinst->indolist ) continue;

        if ( 0 != pinst->size )
        {
            fvec3_t   vpos;
            float dist;

            vpos.x = pinst->pos.x - vcam.x;
            vpos.y = pinst->pos.y - vcam.y;
            vpos.z = pinst->pos.z - vcam.z;

            dist = vfwd.dot(vpos);

            if ( dist > 0 )
            {
                reg[numparticle].index = REF_TO_INT( prt_bdl.prt_ref );
                reg[numparticle].dist  = dist;
                numparticle++;
            }
        }
    }
    PRT_END_LOOP();

    // sort the particles from close to far
    qsort( reg, numparticle, sizeof( prt_registry_entity_t ), cmp_prt_registry_entity );

    return numparticle;
}

//--------------------------------------------------------------------------------------------
void render_all_prt_solid( std::shared_ptr<Camera> pcam, const prt_registry_entity_t reg[], const size_t numparticle )
{
    /// @author BB
    /// @details do solid sprites first

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
void render_all_prt_trans( std::shared_ptr<Camera> pcam, const prt_registry_entity_t reg[], const size_t numparticle )
{
    /// @author BB
    /// @details do all kinds of transparent sprites next

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
//--------------------------------------------------------------------------------------------
// EXAMPLE CODE THAT IS NOT USED
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void   render_all_prt_ref( std::shared_ptr<Camera> pcam, const prt_registry_entity_t reg[], const size_t numparticle );
static size_t render_all_prt_ref_begin( std::shared_ptr<Camera> pcam,  prt_registry_entity_t reg[],  size_t reg_count );
static void   render_prt_ref( std::shared_ptr<Camera> pcam );

//--------------------------------------------------------------------------------------------
void render_prt_ref( std::shared_ptr<Camera> pcam )
{
    /// @author ZZ
    /// @details This function draws sprites reflected in the floor

    prt_registry_entity_t reg[PARTICLES_MAX];
    size_t numparticle;

    numparticle = render_all_prt_ref_begin( pcam, reg, PARTICLES_MAX );
    render_all_prt_ref( pcam, reg, numparticle );
}

//--------------------------------------------------------------------------------------------
size_t render_all_prt_ref_begin( std::shared_ptr<Camera> pcam, prt_registry_entity_t reg[], size_t reg_count )
{
    fvec3_t vfwd, vcam;
    size_t  numparticle;

    update_all_prt_instance( pcam );

    mat_getCamForward(pcam->getView(), vfwd);
    vcam = pcam->getPosition();

    // Original points
    numparticle = 0;
    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        prt_instance_t * pinst;
        fvec3_t   vpos;
        float dist;

        if ( numparticle >= reg_count ) break;

        pinst = &( prt_bdl.prt_ptr->inst );

        if ( !pinst->indolist ) continue;

        vpos = pinst->ref_pos - vcam;
        dist = vfwd.dot(vpos);

        if ( dist > 0 )
        {
            reg[numparticle].index = REF_TO_INT( iprt );
            reg[numparticle].dist  = dist;
            numparticle++;
        }

    }
    PRT_END_LOOP();

    // sort the particles from close to far
    qsort( reg, numparticle, sizeof( prt_registry_entity_t ), cmp_prt_registry_entity );

    return numparticle;
}

//--------------------------------------------------------------------------------------------
void render_all_prt_ref( std::shared_ptr<Camera> pcam, const prt_registry_entity_t reg[], const size_t numparticle )
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


