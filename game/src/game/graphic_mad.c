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

/// @file  game/graphic_mad.c
/// @brief Character model drawing code.
/// @details

#include "egolib/bbox.h"
#include "game/graphic_mad.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/mad.h"
#include "game/game.h"
#include "game/input.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "game/Profiles/_Include.hpp"
#include "game/char.h"
#include "game/Graphics/MD2Model.hpp"

#include "game/Graphics/CameraSystem.hpp"

#include "game/Entities/ObjectHandler.hpp"
#include "game/Entities/EnchantHandler.hpp"
#include "game/Entities/ParticleHandler.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// the flip tolerance is the default flip increment / 2
static const float flip_tolerance = 0.25f * 0.5f;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void draw_chr_verts( Object * pchr, int vrt_offset, int verts );
static void _draw_one_grip_raw( chr_instance_t * pinst, mad_t * pmad, int slot );
static void draw_one_grip( chr_instance_t * pinst, mad_t * pmad, int slot );
static void draw_chr_grips( Object * pchr );
static void draw_chr_attached_grip( Object * pchr );
static void draw_chr_bbox( Object * pchr );

// these functions are only called by render_one_mad()
static gfx_rv render_one_mad_enviro( Camera& cam, const CHR_REF ichr, GLXvector4f tint, const BIT_FIELD bits );
static gfx_rv render_one_mad_tex( Camera& cam, const CHR_REF ichr, GLXvector4f tint, const BIT_FIELD bits );

// private chr_instance_t methods
static gfx_rv chr_instance_alloc( chr_instance_t * pinst, size_t vlst_size );
static gfx_rv chr_instance_free( chr_instance_t * pinst );
static gfx_rv chr_instance_update_vlst_cache( chr_instance_t * pinst, int vmax, int vmin, bool force, bool vertices_match, bool frames_match );
static gfx_rv chr_instance_needs_update( chr_instance_t * pinst, int vmin, int vmax, bool *verts_match, bool *frames_match );
static gfx_rv chr_instance_set_frame( chr_instance_t * pinst, int frame );
static void   chr_instance_clear_cache( chr_instance_t * pinst );
static void chr_instance_interpolate_vertices_raw(GLvertex dst_ary[], const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip);

// private vlst_cache_t methods
static vlst_cache_t * vlst_cache_init( vlst_cache_t * );
static gfx_rv         vlst_cache_test( vlst_cache_t *, chr_instance_t * );

// private chr_reflection_cache_t methods
static chr_reflection_cache_t * chr_reflection_cache_init( chr_reflection_cache_t * pcache );

// private matrix_cache_t methods
static matrix_cache_t * matrix_cache_init( matrix_cache_t * mcache );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_rv render_one_mad_enviro( Camera& cam, const CHR_REF character, GLXvector4f tint, const BIT_FIELD bits )
{
    /// @author ZZ
    /// @details This function draws an environment mapped model

    GLint matrix_mode[1];
    float  uoffset, voffset;

    Object          * pchr;
    mad_t          * pmad;
    std::shared_ptr<MD2Model> pmd2;
    chr_instance_t * pinst;
    oglx_texture_t   * ptex;

    if ( !_gameObjects.exists( character ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, character, "invalid character" );
        return gfx_error;
    }
    pchr  = _gameObjects.get( character );
    pinst = &( pchr->inst );

    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    if ( NULL == pmad->md2_ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL md2" );
        return gfx_error;
    }
    pmd2 = pmad->md2_ptr;

    ptex = NULL;
    if ( HAS_SOME_BITS( bits, CHR_PHONG ) )
    {
		ptex = TextureManager::get().get_valid_ptr((TX_REF)TX_PHONG);
    }

    if ( !GL_DEBUG( glIsEnabled )( GL_BLEND ) )
    {
        return gfx_fail;
    }

    if ( NULL == ptex )
    {
        ptex = TextureManager::get().get_valid_ptr( pinst->texture );
    }

    uoffset = pinst->uoffset - cam.getTurnZOne();
    voffset = pinst->voffset;

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();

    if ( HAS_SOME_BITS( bits, CHR_REFLECT ) )
    {
		Ego::Renderer::get().multiplyMatrix(pinst->ref.matrix);
    }
    else
    {
		Ego::Renderer::get().multiplyMatrix(pinst->matrix);
    }

    // Choose texture and matrix
    oglx_texture_t::bind( ptex );

    ATTRIB_PUSH( __FUNCTION__, GL_CURRENT_BIT );
    {
        GLXvector4f curr_color;

        GL_DEBUG( glGetFloatv )( GL_CURRENT_COLOR, curr_color );

        // Render each command
        for(const MD2_GLCommand &glcommand : pmd2->getGLCommands())
        {
            GL_DEBUG( glBegin )(glcommand.glMode);
            {
                for(const id_glcmd_packed_t& cmd : glcommand.data)
                {
                    GLfloat     cmax;
                    GLXvector4f col;
                    GLfloat     tex[2];
                    GLvertex   *pvrt;

                    uint16_t vertex = cmd.index;
                    if ( vertex >= pinst->vrt_count ) continue;

                    pvrt   = pinst->vrt_lst + vertex;

                    // normalize the color so it can be modulated by the phong/environment map
                    col[RR] = pvrt->color_dir * INV_FF;
                    col[GG] = pvrt->color_dir * INV_FF;
                    col[BB] = pvrt->color_dir * INV_FF;
                    col[AA] = 1.0f;

                    cmax = std::max( std::max( col[RR], col[GG] ), col[BB] );

                    if ( cmax != 0.0f )
                    {
                        col[RR] /= cmax;
                        col[GG] /= cmax;
                        col[BB] /= cmax;
                    }

                    // apply the tint
                    /// @todo not sure why curr_color is important, removing it fixes phong
                    col[RR] *= tint[RR];// * curr_color[RR];
                    col[GG] *= tint[GG];// * curr_color[GG];
                    col[BB] *= tint[BB];// * curr_color[BB];
                    col[AA] *= tint[AA];// * curr_color[AA];

                    tex[0] = pvrt->env[XX] + uoffset;
                    tex[1] = CLIP( cmax, 0.0f, 1.0f );

                    if ( 0 != ( bits & CHR_PHONG ) )
                    {
                        // determine the phong texture coordinates
                        // the default phong is bright in both the forward and back directions...
                        tex[1] = tex[1] * 0.5f + 0.5f;
                    }

                    GL_DEBUG( glColor4fv )( col );
                    GL_DEBUG( glNormal3fv )( pvrt->nrm );
                    GL_DEBUG( glTexCoord2fv )( tex );
                    GL_DEBUG( glVertex3fv )( pvrt->pos );
                }

            }
            GL_DEBUG_END();
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // restore the matrix mode
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );

    return gfx_success;
}

// Do fog...
/*
if(fogon && pinst->light==255)
{
    // The full fog value
    alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

    for (cnt = 0; cnt < pmad->transvertices; cnt++)
    {
        // Figure out the z position of the vertex...  Not totally accurate
        z = (pinst->vrt_lst[cnt].pos[ZZ]) + pchr->matrix(3,2);

        // Figure out the fog coloring
        if(z < fogtop)
        {
            if(z < fogbottom)
            {
                pinst->vrt_lst[cnt].specular = alpha;
            }
            else
            {
                z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f...  Amount of fog to keep
                red = fogred * z;
                grn = foggrn * z;
                blu = fogblu * z;
                fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                pinst->vrt_lst[cnt].specular = fogspec;
            }
        }
        else
        {
            pinst->vrt_lst[cnt].specular = 0;
        }
    }
}

else
{
    for (cnt = 0; cnt < pmad->transvertices; cnt++)
        pinst->vrt_lst[cnt].specular = 0;
}

*/

//--------------------------------------------------------------------------------------------
gfx_rv render_one_mad_tex(Camera& camera, const CHR_REF character, GLXvector4f tint, const BIT_FIELD bits)
{
    /// @author ZZ
    /// @details This function draws a model

    GLint matrix_mode;

    if (!_gameObjects.exists(character))
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, character, "invalid character");
        return gfx_error;
    }
    Object *pchr = _gameObjects.get(character);
    chr_instance_t *pinst = &(pchr->inst);

    if (!LOADED_MAD(pinst->imad))
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad");
        return gfx_error;
    }
    mad_t *pmad = MadStack.get_ptr(pinst->imad);

    if (!pmad->md2_ptr)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL md2");
        return gfx_error;
    }
    std::shared_ptr<MD2Model> pmd2 = pmad->md2_ptr;

    // To make life easier
    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(pinst->texture);

    float uoffset = pinst->uoffset * INV_FFFF;
    float voffset = pinst->voffset * INV_FFFF;

    float base_amb = 0.0f;
    if (0 == (bits & CHR_LIGHT))
    {
        // Convert the "light" parameter to self-lighting for
        // every object that is not being rendered using CHR_LIGHT.
        base_amb = (255 == pinst->light) ? 0 : (pinst->light * INV_FF);
    }

    // Get the maximum number of vertices per command.
    size_t vertexBufferCapacity = 0;
    for (const MD2_GLCommand& glcommand : pmd2->getGLCommands())
    {
        vertexBufferCapacity = std::max(vertexBufferCapacity, glcommand.data.size());
    }
    // Allocate a vertex buffer.
    struct Vertex
    {
        float px, py, pz;
        float nx, ny, nz;
        float r, g, b, a;
        float s, t;
    };
    auto vertexBuffer = std::unique_ptr<Vertex[]>(new Vertex[vertexBufferCapacity]);

    // save the matrix mode
    glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
    Ego::OpenGL::Utilities::isError();

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    Ego::OpenGL::Utilities::isError();
    if (0 != (bits & CHR_REFLECT))
    {
        Ego::Renderer::get().multiplyMatrix(pinst->ref.matrix);
    }
    else
    {
        Ego::Renderer::get().multiplyMatrix(pinst->matrix);
    }

    // Choose texture.
    oglx_texture_t::bind(ptex);

    glPushAttrib(GL_CURRENT_BIT);
    {
        // Render each command
        for (const MD2_GLCommand& glcommand : pmd2->getGLCommands())
        {
            // Pre-render this command.
            size_t vertexBufferSize = 0;
            for (const id_glcmd_packed_t &cmd : glcommand.data)
            {
                Uint16 vertexIndex = cmd.index;
                if (vertexIndex >= pinst->vrt_count)
                {
                    continue;
                }
                auto& v = vertexBuffer[vertexBufferSize++];
                GLvertex *pvrt = &(pinst->vrt_lst[vertexIndex]);
                v.px = pvrt->pos[XX];
                v.py = pvrt->pos[YY];
                v.pz = pvrt->pos[ZZ];
                v.nx = pvrt->nrm[XX];
                v.ny = pvrt->nrm[YY];
                v.nz = pvrt->nrm[ZZ];

                // Determine the texture coordinates.
                v.s = cmd.s + uoffset;
                v.t = cmd.t + voffset;

                // Perform lighting.
                if (HAS_NO_BITS(bits, CHR_LIGHT))
                {
                    // The directional lighting.
                    float fcol = pvrt->color_dir * INV_FF;

                    v.r = fcol;
                    v.g = fcol;
                    v.b = fcol;
                    v.a = 1.0f;

                    // Ambient lighting.
                    if (HAS_NO_BITS(bits, CHR_PHONG))
                    {
                        // Convert the "light" parameter to self-lighting for
                        // every object that is not being rendered using CHR_LIGHT.

                        float acol = base_amb + pinst->color_amb * INV_FF;

                        v.r += acol;
                        v.g += acol;
                        v.b += acol;
                    }

                    // clip the colors
                    v.r = Ego::Math::constrain(v.r, 0.0f, 1.0f);
                    v.g = Ego::Math::constrain(v.g, 0.0f, 1.0f);
                    v.b = Ego::Math::constrain(v.b, 0.0f, 1.0f);

                    // tint the object
                    v.r *= tint[RR];
                    v.g *= tint[GG];
                    v.b *= tint[BB];
                }
                else
                {
                    // Set the basic tint.
                    v.r = tint[RR];
                    v.g = tint[GG];
                    v.b = tint[BB];
                    v.a = tint[AA];
                }
            }
            // Render this command.
            glBegin(glcommand.glMode);
            {
                for (size_t vertexIndex = 0; vertexIndex < vertexBufferSize; ++vertexIndex)
                {
                    const auto& v = vertexBuffer[vertexIndex];
                    glColor4f(v.r, v.g, v.b, v.a);
                    glNormal3f(v.nx, v.ny, v.nz);
                    glTexCoord2f(v.s, v.t);
                    glVertex3f(v.px, v.py, v.pz);
                }
            }
            glEnd();
        }
    }
    glPopAttrib();

    // Restore the GL_MODELVIEW matrix
    glMatrixMode(GL_MODELVIEW);
    Ego::OpenGL::Utilities::isError();
    glPopMatrix();
    Ego::OpenGL::Utilities::isError();
    // restore the matrix mode
    glMatrixMode(matrix_mode);
    Ego::OpenGL::Utilities::isError();

    return gfx_success;
}

/*
    // Do fog...
    if(fogon && pinst->light==255)
    {
        // The full fog value
        alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

        for (cnt = 0; cnt < pmad->transvertices; cnt++)
        {
            // Figure out the z position of the vertex...  Not totally accurate
            z = (pinst->vrt_lst[cnt].pos[ZZ]) + pchr->matrix(3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    pinst->vrt_lst[cnt].specular = alpha;
                }
                else
                {
                    spek = pinst->vrt_lst[cnt].specular & 255;
                    z = (z - fogbottom)/fogdistance;  // 0.0f to 1.0f...  Amount of old to keep
                    fogtokeep = 1.0f-z;  // 0.0f to 1.0f...  Amount of fog to keep
                    spek = spek * z;
                    red = (fogred * fogtokeep) + spek;
                    grn = (foggrn * fogtokeep) + spek;
                    blu = (fogblu * fogtokeep) + spek;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    pinst->vrt_lst[cnt].specular = fogspec;
                }
            }
        }
    }
*/

//--------------------------------------------------------------------------------------------
gfx_rv render_one_mad( Camera& cam, const CHR_REF character, GLXvector4f tint, const BIT_FIELD bits )
{
    /// @author ZZ
    /// @details This function picks the actual function to use

    Object * pchr;
    gfx_rv retval;

    if ( !_gameObjects.exists( character ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, character, "invalid character" );
        return gfx_error;
    }
    pchr = _gameObjects.get( character );

    if ( pchr->is_hidden || _gameObjects.exists( pchr->inwhich_inventory ) ) return gfx_fail;

    if ( pchr->inst.enviro || HAS_SOME_BITS( bits, CHR_PHONG ) )
    {
        retval = render_one_mad_enviro( cam, character, tint, bits );
    }
    else
    {
        retval = render_one_mad_tex( cam, character, tint, bits );
    }

#if defined(DRAW_CHR_BBOX)
    // don't draw the debug stuff for reflections
    if ( 0 == ( bits & CHR_REFLECT ) )
    {
        draw_chr_bbox( pchr );
    }

    // the grips of all objects
    //draw_chr_attached_grip( pchr );

    // draw all the vertices of an object
    //GL_DEBUG( glPointSize( 5 ) );
    //draw_chr_verts( pchr, 0, pro_get_pmad(pchr->inst.imad)->md2_data.vertex_lst );
#endif

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_one_mad_ref( Camera& cam, const CHR_REF ichr )
{
    /// @author ZZ
    /// @details This function draws characters reflected in the floor

    Object * pchr;
    chr_instance_t * pinst;
    GLXvector4f tint;
    gfx_rv retval;

    if ( !_gameObjects.exists( ichr ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, ichr, "invalid character" );
        return gfx_error;
    }
    pchr = _gameObjects.get( ichr );
    pinst = &( pchr->inst );

    if ( pchr->is_hidden ) return gfx_fail;

    // assume the best
    retval = gfx_success;

    if ( !pinst->ref.matrix_valid )
    {
        if ( !chr_instance_apply_reflection_matrix( &( pchr->inst ), pchr->enviro.grid_level ) )
        {
            return gfx_error;
        }
    }

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT );
    {
        // cull backward facing polygons
        // use couter-clockwise orientation to determine backfaces
        oglx_begin_culling( GL_BACK, MAD_REF_CULL );            // GL_ENABLE_BIT | GL_POLYGON_BIT
        Ego::OpenGL::Utilities::isError();
        if ( pinst->ref.alpha != 255 && pinst->ref.light == 255 )
        {
            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );                        // GL_COLOR_BUFFER_BIT
            Ego::OpenGL::Utilities::isError();
            chr_instance_get_tint( pinst, tint, CHR_ALPHA | CHR_REFLECT );

            // the previous call to chr_instance_update_lighting_ref() has actually set the
            // alpha and light for all vertices
            if ( gfx_error == render_one_mad( cam, ichr, tint, CHR_ALPHA | CHR_REFLECT ) )
            {
                retval = gfx_error;
            }
        }

        if ( pinst->ref.light != 255 )
        {
            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );                        // GL_COLOR_BUFFER_BIT
            Ego::OpenGL::Utilities::isError();
            chr_instance_get_tint( pinst, tint, CHR_LIGHT | CHR_REFLECT );

            // the previous call to chr_instance_update_lighting_ref() has actually set the
            // alpha and light for all vertices
            if ( gfx_error == render_one_mad( cam, ichr, tint, CHR_LIGHT | CHR_REFLECT ) )
            {
                retval = gfx_error;
            }
            Ego::OpenGL::Utilities::isError();
        }

        if ( gfx.phongon && pinst->sheen > 0 )
        {
            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );
            Ego::OpenGL::Utilities::isError();
            chr_instance_get_tint( pinst, tint, CHR_PHONG | CHR_REFLECT );

            if ( gfx_error == render_one_mad( cam, ichr, tint, CHR_PHONG | CHR_REFLECT ) )
            {
                retval = gfx_error;
            }
            Ego::OpenGL::Utilities::isError();
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_one_mad_trans( Camera& cam, const CHR_REF ichr )
{
    /// @author ZZ
    /// @details This function dispatches the rendering of transparent characters
    ///               to the correct function. (this does not handle characer reflection)

    Object * pchr;
    chr_instance_t * pinst;
    GLXvector4f tint;
    bool rendered;

    if ( !_gameObjects.exists( ichr ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, ichr, "invalid character" );
        return gfx_error;
    }
    pchr = _gameObjects.get( ichr );
    pinst = &( pchr->inst );

    if ( pchr->is_hidden ) return gfx_fail;

    // there is an outside chance the object will not be rendered
    rendered = false;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT )
    {
        if ( pinst->alpha < 255 && 255 == pinst->light )
        {
            // most alpha effects will be messed up by
            // skipping backface culling, so don't

            // cull backward facing polygons
            // use clockwise orientation to determine backfaces
            oglx_begin_culling( GL_BACK, MAD_NRM_CULL );            // GL_ENABLE_BIT | GL_POLYGON_BIT

            // get a speed-up by not displaying completely transparent portions of the skin
            Ego::Renderer::get().setAlphaTestEnabled(true);
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                             // GL_COLOR_BUFFER_BIT

            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );      // GL_COLOR_BUFFER_BIT

            chr_instance_get_tint( pinst, tint, CHR_ALPHA );

            if ( render_one_mad( cam, ichr, tint, CHR_ALPHA ) )
            {
                rendered = true;
            }
        }
        if ( pinst->light < 255 )
        {
            // light effects should show through transparent objects
            oglx_end_culling();         // GL_ENABLE_BIT

            // the alpha test can only mess us up here
            Ego::Renderer::get().setAlphaTestEnabled(false);

            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG(glBlendFunc)(GL_ONE, GL_ONE);  // GL_COLOR_BUFFER_BIT

            chr_instance_get_tint( pinst, tint, CHR_LIGHT );

            if ( render_one_mad( cam, ichr, tint, CHR_LIGHT ) )
            {
                rendered = true;
            }
        }

        if ( gfx.phongon && pinst->sheen > 0 )
        {
            Ego::Renderer::get().setBlendingEnabled(true);
            GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE );    // GL_COLOR_BUFFER_BIT

            chr_instance_get_tint( pinst, tint, CHR_PHONG );

            if ( render_one_mad( cam, ichr, tint, CHR_PHONG ) )
            {
                rendered = true;
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return rendered ? gfx_success : gfx_fail;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_one_mad_solid( Camera& cam, const CHR_REF ichr )
{
    Object * pchr;
    chr_instance_t * pinst;
    gfx_rv retval = gfx_error;

    if ( !_gameObjects.exists( ichr ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, ichr, "invalid character" );
        return gfx_error;
    }
    pchr = _gameObjects.get( ichr );
    pinst = &( pchr->inst );

    if ( pchr->is_hidden ) return gfx_fail;

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT );
    {
        // do not display the completely transparent portion
        // this allows characters that have "holes" in their
        // textures to display the solid portions properly
        //
        // Objects with partially transparent skins should enable the [MODL] parameter "T"
        // which will enable the display of the partially transparent portion of the skin

        Ego::Renderer::get().setAlphaTestEnabled(true);
        GL_DEBUG( glAlphaFunc )( GL_EQUAL, 1.0f );             // GL_COLOR_BUFFER_BIT

        // can I turn this off?
        Ego::Renderer::get().setBlendingEnabled(true);
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );   // GL_COLOR_BUFFER_BIT

        if ( 255 == pinst->alpha && 255 == pinst->light )
        {
            GLXvector4f tint;

            // allow the dont_cull_backfaces to keep solid objects from culling backfaces
            if ( pinst->dont_cull_backfaces )
            {
                // stop culling backward facing polugons
                oglx_end_culling();         // GL_ENABLE_BIT
            }
            else
            {
                // cull backward facing polygons
                // use couter-clockwise orientation to determine backfaces
                oglx_begin_culling( GL_BACK, MAD_NRM_CULL );            // GL_ENABLE_BIT | GL_POLYGON_BIT
            }

            chr_instance_get_tint( pinst, tint, CHR_SOLID );

            if ( gfx_error == render_one_mad( cam, ichr, tint, CHR_SOLID ) )
            {
                retval = gfx_error;
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void draw_chr_bbox(Object *pchr)
{
    static const bool drawLeftSlot = true;
    static const bool drawRightSlot = true;
    static const bool drawCharacter = true;
    if (!ACTIVE_PCHR(pchr))
    {
        return;
    }
    // Draw the object bounding box as a part of the graphics debug mode F7.
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && SDL_KEYDOWN(keyb, SDLK_F7))
    {
        oglx_texture_t::bind(nullptr);
        {
            if (drawLeftSlot)
            {
                oct_bb_t bb;
                oct_bb_translate(&(pchr->slot_cv[SLOT_LEFT]), pchr->getPosition(), &bb);
                Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());
                render_oct_bb(&bb, true, true);
            }
            if (drawRightSlot)
            {
                oct_bb_t bb;
                oct_bb_translate(&(pchr->slot_cv[SLOT_RIGHT]), pchr->getPosition(), &bb);
                Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());
                render_oct_bb(&bb, true, true);
            }
            if (drawCharacter)
            {
                oct_bb_t bb;
                oct_bb_translate(&(pchr->chr_min_cv), pchr->getPosition(), &bb);
                Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());
                render_oct_bb(&bb, true, true);
            }
        }
    }

    //// The grips and vertrices of all objects.
    /*
    if (cfg.dev_mode && SDL_KEYDOWN(keyb, SDLK_F6))
    {
        draw_chr_attached_grip( pchr );

        // Draw all the vertices of an object
        GL_DEBUG(glPointSize(5));
        draw_chr_verts(pchr, 0, pchr->inst.vrt_count);
    }
    */
}

//--------------------------------------------------------------------------------------------
void draw_chr_verts( Object * pchr, int vrt_offset, int verts )
{
    /// @author BB
    /// @details a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.

    GLint matrix_mode[1];

    mad_t * pmad;

    int vmin, vmax, cnt;

    if ( !ACTIVE_PCHR( pchr ) ) return;

    pmad = chr_get_pmad( GET_INDEX_PCHR( pchr ) );
    if ( NULL == pmad ) return;

    vmin = vrt_offset;
    vmax = vmin + verts;

    if ( vmin < 0 || ( size_t )vmin > pchr->inst.vrt_count ) return;
    if ( vmax < 0 || ( size_t )vmax > pchr->inst.vrt_count ) return;

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    oglx_texture_t::bind(nullptr);

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().multiplyMatrix(pchr->inst.matrix);
    GL_DEBUG( glBegin( GL_POINTS ) );
    {
        for ( cnt = vmin; cnt < vmax; cnt++ )
        {
            GL_DEBUG( glVertex3fv )( pchr->inst.vrt_lst[cnt].pos );
        }
    }
    GL_DEBUG_END();

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // restore the matrix mode
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );
}

//--------------------------------------------------------------------------------------------
void draw_one_grip( chr_instance_t * pinst, mad_t * pmad, int slot )
{
    GLint matrix_mode[1];


    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    oglx_texture_t::bind(nullptr);

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().multiplyMatrix(pinst->matrix);

    _draw_one_grip_raw( pinst, pmad, slot );

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // restore the matrix mode
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );
}

//--------------------------------------------------------------------------------------------
void _draw_one_grip_raw( chr_instance_t * pinst, mad_t * pmad, int slot )
{
    int vmin, vmax, cnt;

    float red[4] = {1, 0, 0, 1};
    float grn[4] = {0, 1, 0, 1};
    float blu[4] = {0, 0, 1, 1};
    float * col_ary[3];

    col_ary[0] = red;
    col_ary[1] = grn;
    col_ary[2] = blu;

    if ( NULL == pinst || NULL == pmad ) return;

    vmin = ( int )pinst->vrt_count - ( int )slot_to_grip_offset(( slot_t )slot );
    vmax = vmin + GRIP_VERTS;

    if ( vmin >= 0 && vmax >= 0 && ( size_t )vmax <= pinst->vrt_count )
    {
        fvec3_t   src, dst, diff;

        GL_DEBUG( glBegin )( GL_LINES );
        {
            for ( cnt = 1; cnt < GRIP_VERTS; cnt++ )
            {
                src.x = pinst->vrt_lst[vmin].pos[XX];
                src.y = pinst->vrt_lst[vmin].pos[YY];
                src.z = pinst->vrt_lst[vmin].pos[ZZ];

                diff.x = pinst->vrt_lst[vmin+cnt].pos[XX] - src.x;
                diff.y = pinst->vrt_lst[vmin+cnt].pos[YY] - src.y;
                diff.z = pinst->vrt_lst[vmin+cnt].pos[ZZ] - src.z;

                dst.x = src.x + 3 * diff.x;
                dst.y = src.y + 3 * diff.y;
                dst.z = src.z + 3 * diff.z;

                GL_DEBUG( glColor4fv )( col_ary[cnt-1] );

                GL_DEBUG( glVertex3fv )( src.v );
                GL_DEBUG( glVertex3fv )( dst.v );
            }
        }
        GL_DEBUG_END();
    }

	Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());
}

//--------------------------------------------------------------------------------------------
void draw_chr_attached_grip( Object * pchr )
{
    mad_t * pholder_mad;
    Object * pholder;

    if ( !ACTIVE_PCHR( pchr ) ) return;

    if ( !_gameObjects.exists( pchr->attachedto ) ) return;
    pholder = _gameObjects.get( pchr->attachedto );

    pholder_mad = chr_get_pmad( GET_INDEX_PCHR( pholder ) );
    if ( NULL == pholder_mad ) return;

    draw_one_grip( &( pholder->inst ), pholder_mad, pchr->inwhich_slot );
}

//--------------------------------------------------------------------------------------------
void draw_chr_grips( Object * pchr )
{
    mad_t * pmad;

    GLint matrix_mode[1];

    if ( !ACTIVE_PCHR( pchr ) ) return;

    const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(pchr->profile_ref);

    pmad = chr_get_pmad( GET_INDEX_PCHR( pchr ) );
    if ( NULL == pmad ) return;

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    oglx_texture_t::bind(nullptr);

    // save the matrix mode
    GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPushMatrix )();
	Ego::Renderer::get().multiplyMatrix(pchr->inst.matrix);

    if ( profile->isSlotValid(SLOT_LEFT) )
    {
        _draw_one_grip_raw( &( pchr->inst ), pmad, SLOT_LEFT );
    }

    if ( profile->isSlotValid(SLOT_RIGHT) )
    {
        _draw_one_grip_raw( &( pchr->inst ), pmad, SLOT_RIGHT );
    }

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG( glMatrixMode )( GL_MODELVIEW );
    GL_DEBUG( glPopMatrix )();

    // restore the matrix mode
    GL_DEBUG( glMatrixMode )( matrix_mode[0] );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void chr_instance_update_lighting_base( chr_instance_t * pinst, Object * pchr, bool force )
{
    /// @author BB
    /// @details determine the basic per-vertex lighting

    const int frame_skip = 1 << 2;
    const int frame_mask = frame_skip - 1;

    Uint16 cnt;

    lighting_cache_t global_light, loc_light;

    GLvertex * vrt_lst;

    mad_t * pmad;

    if ( NULL == pinst || NULL == pchr ) return;
    vrt_lst = pinst->vrt_lst;

    // force this function to be evaluated the 1st time through
    if ( pinst->lighting_frame_all < 0 || pinst->lighting_frame_all < 0 )
    {
        force = true;
    }

    // has this already been calculated this update?
    if ( !force && pinst->lighting_update_wld >= 0 && ( Uint32 )pinst->lighting_update_wld >= update_wld ) return;
    pinst->lighting_update_wld = update_wld;

    // make sure the matrix is valid
    chr_update_matrix( pchr, true );

    // has this already been calculated in the last frame_skip frames?
    if ( !force && pinst->lighting_frame_all >= 0 && ( Uint32 )pinst->lighting_frame_all >= game_frame_all ) return;

    // reduce the amount of updates to one every frame_skip frames, but dither
    // the updating so that not all objects update on the same frame
    pinst->lighting_frame_all = game_frame_all + (( game_frame_all + pchr->getCharacterID() ) & frame_mask );

    if ( !LOADED_MAD( pinst->imad ) ) return;
    pmad = MadStack.get_ptr( pinst->imad );
    pinst->vrt_count = pinst->vrt_count;

    // interpolate the lighting for the origin of the object
    grid_lighting_interpolate( PMesh, &global_light, fvec2_t(pchr->getPosX(), pchr->getPosY()) );

    // rotate the lighting data to body_centered coordinates
    lighting_project_cache(&loc_light, &global_light, pinst->matrix);

    pinst->color_amb = 0.9f * pinst->color_amb + 0.1f * ( loc_light.hgh.lighting[LVEC_AMB] + loc_light.low.lighting[LVEC_AMB] ) * 0.5f;

    pinst->max_light = -255;
    pinst->min_light =  255;
    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        Sint16 lite;

        GLvertex * pvert = pinst->vrt_lst + cnt;

        // a simple "height" measurement
        float hgt = pvert->pos[ZZ] * pinst->matrix.CNV( 3, 3 ) + pinst->matrix.CNV( 3, 3 );

        if ( pvert->nrm[0] == 0.0f && pvert->nrm[1] == 0.0f && pvert->nrm[2] == 0.0f )
        {
            // this is the "ambient only" index, but it really means to sum up all the light
            lite  = lighting_evaluate_cache( &loc_light, fvec3_t(+1.0f,+1.0f,+1.0f), hgt, PMesh->tmem.bbox, NULL, NULL );
            lite += lighting_evaluate_cache( &loc_light, fvec3_t(-1.0f,-1.0f,-1.0f), hgt, PMesh->tmem.bbox, NULL, NULL );

            // average all the directions
            lite /= 6;
        }
        else
        {
            lite  = lighting_evaluate_cache( &loc_light, fvec3_t(pvert->nrm[0],pvert->nrm[1],pvert->nrm[2]), hgt, PMesh->tmem.bbox, NULL, NULL );
        }

        pvert->color_dir = 0.9f * pvert->color_dir + 0.1f * lite;

        pinst->max_light = std::max( pinst->max_light, pvert->color_dir );
        pinst->min_light = std::min( pinst->min_light, pvert->color_dir );
    }

    // ??coerce this to reasonable values in the presence of negative light??
    if ( pinst->max_light < 0 ) pinst->max_light = 0;
    if ( pinst->min_light < 0 ) pinst->min_light = 0;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_update_bbox( chr_instance_t * pinst )
{
    mad_t       *pmad;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    // get the model. try to heal a bad model.
    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    const MD2_Frame &lastFrame = chr_instnce_get_frame_lst(pinst);
    const MD2_Frame &nextFrame = chr_instnce_get_frame_nxt(pinst);

    if ( pinst->frame_nxt == pinst->frame_lst || pinst->flip == 0.0f )
    {
        pinst->bbox = lastFrame.bb;
    }
    else if ( pinst->flip == 1.0f )
    {
        pinst->bbox = nextFrame.bb;
    }
    else
    {
        oct_bb_interpolate(&lastFrame.bb, &nextFrame.bb, &pinst->bbox, pinst->flip );
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_needs_update( chr_instance_t * pinst, int vmin, int vmax, bool *verts_match, bool *frames_match )
{
    /// @author BB
    /// @details determine whether some specific vertices of an instance need to be updated
    ///                gfx_error   means that the function was passed invalid values
    ///                gfx_fail    means that the instance does not need to be updated
    ///                gfx_success means that the instance should be updated

    bool local_verts_match, flips_match, local_frames_match;

    vlst_cache_t * psave;

    mad_t * pmad;
    int maxvert;

    // ensure that the pointers point to something
    if ( NULL == verts_match ) verts_match  = &local_verts_match;
    if ( NULL == frames_match ) frames_match = &local_frames_match;

    // initialize the boolean pointers
    *verts_match  = false;
    *frames_match = false;

    // do we have a valid instance?
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }
    psave = &( pinst->save );

    // do we hace a valid mad?
    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    // check to see if the vlst_cache has been marked as invalid.
    // in this case, everything needs to be updated
    if ( !psave->valid ) return gfx_success;

    // get the last valid vertex from the chr_instance
    maxvert = (( int )pinst->vrt_count ) - 1;

    // check to make sure the lower bound of the saved data is valid.
    // it is initialized to an invalid value (psave->vmin = psave->vmax = -1)
    if ( psave->vmin < 0 || psave->vmax < 0 ) return gfx_success;

    // check to make sure the upper bound of the saved data is valid.
    if ( psave->vmin > maxvert || psave->vmax > maxvert ) return gfx_success;

    // make sure that the min and max vertices are in the correct order
    if ( vmax < vmin ) SWAP( int, vmax, vmin );

    // test to see if we have already calculated this data
    *verts_match = ( vmin >= psave->vmin ) && ( vmax <= psave->vmax );

    flips_match = (std::abs(psave->flip - pinst->flip) < flip_tolerance);

    *frames_match = ( pinst->frame_nxt == pinst->frame_lst && psave->frame_nxt == pinst->frame_nxt && psave->frame_lst == pinst->frame_lst ) ||
                    ( flips_match && psave->frame_nxt == pinst->frame_nxt && psave->frame_lst == pinst->frame_lst );

    return ( !( *verts_match ) || !( *frames_match ) ) ? gfx_success : gfx_fail;
}

//--------------------------------------------------------------------------------------------
void chr_instance_interpolate_vertices_raw( GLvertex dst_ary[], const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip )
{
    /// raw indicates no bounds checking, so be careful

    int i;

    GLvertex     * dst;

    if ( 0.0f == flip )
    {
        for ( i = vmin; i <= vmax; i++ )
        {
            dst     = dst_ary + i;
            const MD2_Vertex &srcLast = lst_ary[i];

			dst->pos[XX] = srcLast.pos[kX];
			dst->pos[YY] = srcLast.pos[kY];
			dst->pos[ZZ] = srcLast.pos[kZ];
            dst->pos[WW] = 1.0f;

			dst->nrm[XX] = srcLast.nrm[kX];
			dst->nrm[YY] = srcLast.nrm[kY];
			dst->nrm[ZZ] = srcLast.nrm[kZ];

            dst->env[XX] = indextoenvirox[srcLast.normal];
            dst->env[YY] = 0.5f * ( 1.0f + dst->nrm[ZZ] );
        }
    }
    else if ( 1.0f == flip )
    {
        for ( i = vmin; i <= vmax; i++ )
        {
            dst     = dst_ary + i;
            const MD2_Vertex &srcNext = nxt_ary[i];

			dst->pos[XX] = srcNext.pos[kX];
			dst->pos[YY] = srcNext.pos[kY];
			dst->pos[ZZ] = srcNext.pos[kZ];
            dst->pos[WW] = 1.0f;

            dst->nrm[XX] = srcNext.nrm[kX];
            dst->nrm[YY] = srcNext.nrm[kY];
            dst->nrm[ZZ] = srcNext.nrm[kZ];

            dst->env[XX] = indextoenvirox[srcNext.normal];
            dst->env[YY] = 0.5f * ( 1.0f + dst->nrm[ZZ] );
        }
    }
    else
    {
        Uint16 vrta_lst, vrta_nxt;

        for ( i = vmin; i <= vmax; i++ )
        {
            dst     = dst_ary + i;
            const MD2_Vertex &srcLast = lst_ary[i];
            const MD2_Vertex &srcNext = nxt_ary[i];

            dst->pos[XX] = srcLast.pos.x + ( srcNext.pos.x - srcLast.pos.x ) * flip;
            dst->pos[YY] = srcLast.pos.y + ( srcNext.pos.y - srcLast.pos.y ) * flip;
            dst->pos[ZZ] = srcLast.pos.z + ( srcNext.pos.z - srcLast.pos.z ) * flip;
            dst->pos[WW] = 1.0f;

            dst->nrm[XX] = srcLast.nrm.x + ( srcNext.nrm.x - srcLast.nrm.x ) * flip;
            dst->nrm[YY] = srcLast.nrm.y + ( srcNext.nrm.y - srcLast.nrm.y ) * flip;
            dst->nrm[ZZ] = srcLast.nrm.z + ( srcNext.nrm.z - srcLast.nrm.z ) * flip;

            vrta_lst = srcLast.normal;
            vrta_nxt = srcNext.normal;

            dst->env[XX] = indextoenvirox[vrta_lst] + ( indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst] ) * flip;
            dst->env[YY] = 0.5f * ( 1.0f + dst->nrm[ZZ] );
        }
    }
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_update_vertices( chr_instance_t * pinst, int vmin, int vmax, bool force )
{
	int maxvert;
    bool vertices_match, frames_match;
    float  loc_flip;

    gfx_rv retval;

    vlst_cache_t * psave;

    mad_t       * pmad;
    std::shared_ptr<MD2Model> pmd2;

    int vdirty1_min = -1, vdirty1_max = -1;
    int vdirty2_min = -1, vdirty2_max = -1;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }
    psave = &( pinst->save );

    if ( gfx_error == chr_instance_update_bbox( pinst ) )
    {
        return gfx_error;
    }

    // get the model
    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    if ( NULL == pmad->md2_ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL md2" );
        return gfx_error;
    }
    pmd2 = pmad->md2_ptr;

    // make sure we have valid data
    if (pinst->vrt_count != pmd2->getVertexCount())
    {
        log_error( "chr_instance_update_vertices() - character instance vertex data does not match its md2\n" );
    }

    // get the vertex list size from the chr_instance
    maxvert = (( int )pinst->vrt_count ) - 1;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = maxvert;

    // are they in the right order?
    if ( vmax < vmin ) SWAP( int, vmax, vmin );

    // make sure that the vertices are within the max range
    vmin = CLIP( vmin, 0, maxvert );
    vmax = CLIP( vmax, 0, maxvert );

    if ( force )
    {
        // force an update of vertices

        // select a range that encompases the requested vertices and the saved vertices
        // if this is the 1st update, the saved vertices may be set to invalid values, as well

        // grab the dirty vertices
        vdirty1_min = vmin;
        vdirty1_max = vmax;

        // force the routine to update
        vertices_match = false;
        frames_match   = false;
    }
    else
    {
        // do we need to update?
        retval = chr_instance_needs_update( pinst, vmin, vmax, &vertices_match, &frames_match );
        if ( gfx_error == retval ) return gfx_error;            // gfx_error == retval means some pointer or reference is messed up
        if ( gfx_fail  == retval ) return gfx_success;          // gfx_fail  == retval means we do not need to update this round

        if ( !frames_match )
        {
            // the entire frame is dirty
            vdirty1_min = vmin;
            vdirty1_max = vmax;
        }
        else
        {
            // grab the dirty vertices
            if ( vmin < psave->vmin )
            {
                vdirty1_min = vmin;
                vdirty1_max = psave->vmin - 1;
            }

            if ( vmax > psave->vmax )
            {
                vdirty2_min = psave->vmax + 1;
                vdirty2_max = vmax;
            }
        }
    }

    // make sure the frames are in the valid range
    const std::vector<MD2_Frame> &frameList = pmd2->getFrames();
    if ( pinst->frame_nxt >= frameList.size() || pinst->frame_lst >= frameList.size() )
    {
        log_error( "chr_instance_update_vertices() - character instance frame is outside the range of its md2\n" );
    }

    // grab the frame data from the correct model
    const MD2_Frame &nextFrame = frameList[pinst->frame_nxt];
    const MD2_Frame &lastFrame = frameList[pinst->frame_lst];

    // fix the flip for objects that are not animating
    loc_flip = pinst->flip;
    if ( pinst->frame_nxt == pinst->frame_lst ) loc_flip = 0.0f;

    // interpolate the 1st dirty region
    if ( vdirty1_min >= 0 && vdirty1_max >= 0 )
    {
        chr_instance_interpolate_vertices_raw( pinst->vrt_lst, lastFrame.vertexList, nextFrame.vertexList, vdirty1_min, vdirty1_max, loc_flip );
    }

    // interpolate the 2nd dirty region
    if ( vdirty2_min >= 0 && vdirty2_max >= 0 )
    {
        chr_instance_interpolate_vertices_raw( pinst->vrt_lst, lastFrame.vertexList, nextFrame.vertexList, vdirty2_min, vdirty2_max, loc_flip );
    }

    // update the saved parameters
    return chr_instance_update_vlst_cache( pinst, vmax, vmin, force, vertices_match, frames_match );
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_update_vlst_cache( chr_instance_t * pinst, int vmax, int vmin, bool force, bool vertices_match, bool frames_match )
{
    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // pinst->save.* values be tested and stored properly

    bool verts_updated, frames_updated;
    int    maxvert;

    vlst_cache_t * psave;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }
    maxvert = (( int )pinst->vrt_count ) - 1;
    psave   = &( pinst->save );

    // the save_vmin and save_vmax is the most complex
    verts_updated = false;
    if ( force )
    {
        // to get here, either the specified range was outside the clean range or
        // the animation was updated. In any case, the only vertices that are
        // clean are in the range [vmin, vmax]

        psave->vmin   = vmin;
        psave->vmax   = vmax;
        verts_updated = true;
    }
    else if ( vertices_match && frames_match )
    {
        // everything matches, so there is nothing to do
    }
    else if ( vertices_match )
    {
        // The only way to get here is to fail the frames_match test, and pass vertices_match

        // This means that all of the vertices were SUPPOSED TO BE updated,
        // but only the ones in the range [vmin, vmax] actually were.
        psave->vmin = vmin;
        psave->vmax = vmax;
        verts_updated = true;
    }
    else if ( frames_match )
    {
        // The only way to get here is to fail the vertices_match test, and pass frames_match test

        // There was no update to the animation,  but there was an update to some of the vertices
        // The clean verrices should be the union of the sets of the vertices updated this time
        // and the oned updated last time.
        //
        // If these ranges are disjoint, then only one of them can be saved. Choose the larger set

        if ( vmax >= psave->vmin && vmin <= psave->vmax )
        {
            // the old list [save_vmin, save_vmax] and the new list [vmin, vmax]
            // overlap, so we can merge them
            psave->vmin = std::min( psave->vmin, vmin );
            psave->vmax = std::max( psave->vmax, vmax );
            verts_updated = true;
        }
        else
        {
            // the old list and the new list are disjoint sets, so we are out of luck
            // save the set with the largest number of members
            if (( psave->vmax - psave->vmin ) >= ( vmax - vmin ) )
            {
                // obviously no change...
                psave->vmin = psave->vmin;
                psave->vmax = psave->vmax;
                verts_updated = true;
            }
            else
            {
                psave->vmin = vmin;
                psave->vmax = vmax;
                verts_updated = true;
            }
        }
    }
    else
    {
        // The only way to get here is to fail the vertices_match test, and fail the frames_match test

        // everything was dirty, so just save the new vertex list
        psave->vmin = vmin;
        psave->vmax = vmax;
        verts_updated = true;
    }

    psave->frame_nxt = pinst->frame_nxt;
    psave->frame_lst = pinst->frame_lst;
    psave->flip      = pinst->flip;

    // store the last time there was an update to the animation
    frames_updated = false;
    if ( !frames_match )
    {
        psave->frame_wld = update_wld;
        frames_updated   = true;
    }

    // store the time of the last full update
    if ( 0 == vmin && maxvert == vmax )
    {
        psave->vert_wld  = update_wld;
    }

    // mark the saved vlst_cache data as valid
    psave->valid = true;

    return ( verts_updated || frames_updated ) ? gfx_success : gfx_fail;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_update_grip_verts( chr_instance_t * pinst, Uint16 vrt_lst[], size_t vrt_count )
{
    int vmin, vmax;
    Uint32 cnt;
    size_t count;
    gfx_rv retval;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( NULL == vrt_lst || 0 == vrt_count ) return gfx_fail;

    // count the valid attachment points
    vmin = 0xFFFF;
    vmax = 0;
    count = 0;
    for ( cnt = 0; cnt < vrt_count; cnt++ )
    {
        if ( 0xFFFF == vrt_lst[cnt] ) continue;

        vmin = std::min<Uint16>( vmin, vrt_lst[cnt] );
        vmax = std::max<Uint16>( vmax, vrt_lst[cnt] );
        count++;
    }

    // if there are no valid points, there is nothing to do
    if ( 0 == count ) return gfx_fail;

    // force the vertices to update
    retval = chr_instance_update_vertices( pinst, vmin, vmax, true );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_action( chr_instance_t * pinst, int action, bool action_ready, bool override_action )
{
    int action_old;
    mad_t * pmad;

    // did we get a bad pointer?
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    // is the action in the valid range?
    if ( action < 0 || action > ACTION_COUNT )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, action, "invalid action range" );
        return gfx_error;
    }

    // do we have a valid model?
    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    // is the chosen action valid?
    if ( !pmad->action_valid[ action ] ) return gfx_fail;

    // are we going to check action_ready?
    if ( !override_action && !pinst->action_ready ) return gfx_fail;

    // save the old action
    action_old = pinst->action_which;

    // set up the action
    pinst->action_which = action;
    pinst->action_next  = ACTION_DA;
    pinst->action_ready = action_ready;

    // invalidate the vertex list if the action has changed
    if ( action_old != action )
    {
        pinst->save.valid = false;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_frame( chr_instance_t * pinst, int frame )
{
    mad_t * pmad;

    // did we get a bad pointer?
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    // is the action in the valid range?
    if ( pinst->action_which < 0 || pinst->action_which > ACTION_COUNT )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->action_which, "invalid action range" );
        return gfx_error;
    }

    // do we have a valid model?
    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    // is the current action valid?
    if ( !pmad->action_valid[ pinst->action_which ] ) return gfx_fail;

    // is the frame within the valid range for this action?
    if ( frame < pmad->action_stt[ pinst->action_which ] ) return gfx_fail;
    if ( frame > pmad->action_end[ pinst->action_which ] ) return gfx_fail;

    // jump to the next frame
    pinst->flip      = 0.0f;
    pinst->ilip      = 0;
    pinst->frame_lst = pinst->frame_nxt;
    pinst->frame_nxt = frame;

    vlst_cache_test( &( pinst->save ), pinst );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_anim( chr_instance_t * pinst, int action, int frame, bool action_ready, bool override_action )
{
    gfx_rv retval;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    retval = chr_instance_set_action( pinst, action, action_ready, override_action );
    if ( gfx_success != retval ) return retval;

    retval = chr_instance_set_frame( pinst, frame );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_start_anim( chr_instance_t * pinst, int action, bool action_ready, bool override_action )
{
    mad_t * pmad;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( action < 0 || action >= ACTION_COUNT )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, action, "invalid action range" );
        return gfx_error;
    }

    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    return chr_instance_set_anim( pinst, action, pmad->action_stt[action], action_ready, override_action );
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_increment_action( chr_instance_t * pinst )
{
    /// @author BB
    /// @details This function starts the next action for a character

    gfx_rv retval;

    int     action, action_old;
    bool  action_ready;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    // save the old action
    action_old = pinst->action_which;

    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }

    // get the correct action
    action = mad_get_action_ref( pinst->imad, pinst->action_next );

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    action_ready = ACTION_IS_TYPE( action, D ) || ACTION_IS_TYPE( action, W );

    retval = chr_instance_start_anim( pinst, action, action_ready, true );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_increment_frame( chr_instance_t * pinst, mad_t * pmad, const CHR_REF imount, const int mount_action )
{
    /// @author BB
    /// @details all the code necessary to move on to the next frame of the animation

    int frame_lst, frame_nxt;

    if ( NULL == pmad )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mad" );
        return gfx_error;
    }

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    // fix the ilip and flip
    pinst->ilip = pinst->ilip % 4;
    pinst->flip = fmod( pinst->flip, 1.0f );

    // Change frames
    frame_lst = pinst->frame_nxt;
    frame_nxt = pinst->frame_nxt + 1;

    // detect the end of the animation and handle special end conditions
    if ( frame_nxt > pmad->action_end[pinst->action_which] )
    {
        if ( pinst->action_keep )
        {
            // Freeze that animation at the last frame
            frame_nxt = frame_lst;

            // Break a kept action at any time
            pinst->action_ready = true;
        }
        else if ( pinst->action_loop )
        {
            // Convert the action into a riding action if the character is mounted
            if ( _gameObjects.exists( imount ) )
            {
                chr_instance_start_anim( pinst, mount_action, true, true );
            }

            // set the frame to the beginning of the action
            frame_nxt = pmad->action_stt[pinst->action_which];

            // Break a looped action at any time
            pinst->action_ready = true;
        }
        else
        {
            // make sure that the frame_nxt points to a valid frame in this action
            frame_nxt = pmad->action_end[pinst->action_which];

            // Go on to the next action. don't let just anything interrupt it?
            chr_instance_increment_action( pinst );

            // chr_instance_increment_action() actually sets this value properly. just grab the new value.
            frame_nxt = pinst->frame_nxt;
        }
    }

    pinst->frame_lst = frame_lst;
    pinst->frame_nxt = frame_nxt;

    vlst_cache_test( &( pinst->save ), pinst );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_play_action( chr_instance_t * pinst, int action, bool action_ready )
{
    /// @author ZZ
    /// @details This function starts a generic action for a character
    mad_t * pmad;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( !LOADED_MAD( pinst->imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pinst->imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( pinst->imad );

    action = mad_get_action_ref( pinst->imad, action );

    return chr_instance_start_anim( pinst, action, action_ready, true );
}

//--------------------------------------------------------------------------------------------
void chr_instance_clear_cache( chr_instance_t * pinst )
{
    /// @author BB
    /// @details force chr_instance_update_vertices() recalculate the vertices the next time
    ///     the function is called

    vlst_cache_init( &( pinst->save ) );

    matrix_cache_init( &( pinst->matrix_cache ) );

    chr_reflection_cache_init( &( pinst->ref ) );

    pinst->lighting_update_wld = -1;
    pinst->lighting_frame_all  = -1;
}

//--------------------------------------------------------------------------------------------
chr_instance_t * chr_instance_dtor( chr_instance_t * pinst )
{
    if ( NULL == pinst ) return pinst;

    chr_instance_free( pinst );

    EGOBOO_ASSERT( NULL == pinst->vrt_lst );

    BLANK_STRUCT_PTR( pinst )

    return pinst;
}

//--------------------------------------------------------------------------------------------
chr_instance_t * chr_instance_ctor( chr_instance_t * pinst )
{
    Uint32 cnt;

    if ( NULL == pinst ) return pinst;

    BLANK_STRUCT_PTR( pinst )

    // model parameters
    pinst->imad = INVALID_MAD_REF;
    pinst->vrt_count = 0;

    // set the initial cache parameters
    chr_instance_clear_cache( pinst );

    // Set up initial fade in lighting
    pinst->color_amb = 0;
    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        pinst->vrt_lst[cnt].color_dir = 0;
    }

    // clear out the matrix cache
    matrix_cache_init( &( pinst->matrix_cache ) );

    // the matrix should never be referenced if the cache is not valid,
    // but it never pays to have a 0 matrix...
	pinst->matrix = fmat_4x4_t::identity();

    // set the animation state
    pinst->rate         = 1.0f;
    pinst->action_next  = ACTION_DA;
    pinst->action_ready = true;                     // argh! this must be set at the beginning, script's spawn animations do not work!
    pinst->frame_lst    = 0;
    pinst->frame_nxt    = 0;

    // the vlst_cache parameters are not valid
    pinst->save.valid = false;

    // set the update frame to an invalid value
    pinst->update_frame        = -1;
    pinst->lighting_update_wld = -1;
    pinst->lighting_frame_all  = -1;

    return pinst;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_free( chr_instance_t * pinst )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    EGOBOO_DELETE_ARY( pinst->vrt_lst );
    pinst->vrt_count = 0;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_alloc( chr_instance_t * pinst, size_t vlst_size )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    chr_instance_free( pinst );

    if ( 0 == vlst_size ) return gfx_success;

    pinst->vrt_lst = EGOBOO_NEW_ARY( GLvertex, vlst_size );
    if ( NULL != pinst->vrt_lst )
    {
        pinst->vrt_count = vlst_size;
    }

    return ( NULL != pinst->vrt_lst ) ? gfx_success : gfx_fail;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_mad( chr_instance_t * pinst, const MAD_REF imad )
{
    /// @author BB
    /// @details try to set the model used by the character instance.
    ///     If this fails, it leaves the old data. Just to be safe it
    ///     would be best to check whether the old modes is valid, and
    ///     if not, the data chould be set to safe values...

    mad_t * pmad;
    bool updated = false;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( !LOADED_MAD( imad ) ) return gfx_fail;
    pmad = MadStack.get_ptr( imad );

    if ( NULL == pmad->md2_ptr )
    {
        log_error( "Invalid pmad instance spawn. (Slot number %i)\n", imad );

        return gfx_fail;
    }

    if ( pinst->imad != imad )
    {
        updated = true;
        pinst->imad = imad;
    }

    // set the vertex size
    size_t vlst_size = pmad->md2_ptr->getVertexCount();
    if ( pinst->vrt_count != vlst_size )
    {
        updated = true;
        chr_instance_alloc( pinst, vlst_size );
    }

    // set the frames to frame 0 of this object's data
    if ( 0 != pinst->frame_nxt || 0 != pinst->frame_lst )
    {
        updated          = true;
        pinst->frame_lst = 0;
        pinst->frame_nxt = 0;

        // the vlst_cache parameters are not valid
        pinst->save.valid = false;
    }

    if ( updated )
    {
        // update the vertex and lighting cache
        chr_instance_clear_cache( pinst );
        chr_instance_update_vertices( pinst, -1, -1, true );
    }

    return updated ? gfx_success : gfx_fail;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_update_ref( chr_instance_t * pinst, float grid_level, bool need_matrix )
{
    int startalpha;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( need_matrix )
    {
        // reflect the ordinary matrix
        chr_instance_apply_reflection_matrix( pinst, grid_level );
    }

    startalpha = 255;
    if ( pinst->ref.matrix_valid )
    {
        float pos_z;

        // determine the reflection alpha
        pos_z = grid_level - pinst->ref.matrix.CNV( 3, 2 );
        if ( pos_z < 0.0f ) pos_z = 0.0f;

        startalpha -= 2.0f * pos_z;
        startalpha *= 0.5f;
        startalpha = CLIP( startalpha, 0, 255 );
    }

    pinst->ref.alpha = ( pinst->alpha * startalpha * INV_FF );
    pinst->ref.light = ( 255 == pinst->light ) ? 255 : ( pinst->light * startalpha * INV_FF );

    pinst->ref.redshift = pinst->redshift + 1;
    pinst->ref.grnshift = pinst->grnshift + 1;
    pinst->ref.blushift = pinst->blushift + 1;

    pinst->ref.sheen    = pinst->sheen >> 1;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_spawn( chr_instance_t * pinst, const PRO_REF profile, const int skin )
{
    Sint8 greensave = 0, redsave = 0, bluesave = 0;

    SKIN_T  loc_skin;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    // Remember any previous color shifts in case of lasting enchantments
    greensave = pinst->grnshift;
    redsave   = pinst->redshift;
    bluesave  = pinst->blushift;

    // clear the instance
    chr_instance_ctor( pinst );

    const std::shared_ptr<ObjectProfile> &pobj = ProfileSystem::get().getProfile(profile);
    if(!pobj) {
        return gfx_fail;
    }

    loc_skin = 0;
    if ( skin >= 0 )
    {
        loc_skin = skin % SKINS_PEROBJECT_MAX;
    }

    // lighting parameters
    chr_instance_set_texture( pinst, pobj->getSkin(loc_skin) );
    pinst->enviro    = pobj->isPhongMapped();
    pinst->alpha     = pobj->getAlpha();
    pinst->light     = pobj->getLight();
    pinst->sheen     = pobj->getSheen();
    pinst->grnshift  = greensave;
    pinst->redshift  = redsave;
    pinst->blushift  = bluesave;
    pinst->dont_cull_backfaces = pobj->isDontCullBackfaces();

    // model parameters
    chr_instance_set_mad( pinst, pobj->getModelRef() );

    // set the initial action, all actions override it
    chr_instance_play_action( pinst, ACTION_DA, true );

    // upload these parameters to the reflection cache, but don't compute the matrix
    chr_instance_update_ref( pinst, 0, false );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD chr_instance_get_framefx( chr_instance_t * pinst )
{
    return chr_instnce_get_frame_nxt(pinst).framefx;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_frame_full( chr_instance_t * pinst, int frame_along, int ilip, const MAD_REF mad_override )
{
    MAD_REF imad;
    mad_t * pmad;
    int     frame_stt, frame_end, frame_count;

    int    new_nxt;

    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    // handle optional parameters
    if ( VALID_MAD_RANGE( mad_override ) )
    {
        imad = mad_override;
    }
    else
    {
        imad = pinst->imad;
    }

    if ( !LOADED_MAD( imad ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, imad, "invalid mad" );
        return gfx_error;
    }
    pmad = MadStack.get_ptr( imad );

    // we have to have a valid action range
    if ( pinst->action_which > ACTION_COUNT ) return gfx_fail;

    // try to heal a bad action
    if ( pinst->action_which != pmad->action_map[pinst->action_which] )
    {
        pinst->action_which = pmad->action_map[pinst->action_which];
    }

    // reject the action if it is cannot be made valid
    if ( pinst->action_which == ACTION_COUNT ) return gfx_fail;

    // get some frame info
    frame_stt   = pmad->action_stt[pinst->action_which];
    frame_end   = pmad->action_end[pinst->action_which];
    frame_count = 1 + ( frame_end - frame_stt );

    // try to heal an out of range value
    frame_along %= frame_count;

    // get the next frames
    new_nxt = frame_stt + frame_along;
    new_nxt = std::min( new_nxt, frame_end );

    pinst->frame_nxt  = new_nxt;
    pinst->ilip       = ilip;
    pinst->flip       = ilip * 0.25f;

    // set the validity of the cache
    vlst_cache_test( &( pinst->save ), pinst );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_action_keep( chr_instance_t * pinst, bool val )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    pinst->action_keep = val;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_action_ready( chr_instance_t * pinst, bool val )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    pinst->action_ready = val;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_action_loop( chr_instance_t * pinst, bool val )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    pinst->action_loop = val;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_action_next( chr_instance_t * pinst, int val )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( val < 0 || val > ACTION_COUNT ) return gfx_fail;

    pinst->action_next = val;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_remove_interpolation( chr_instance_t * pinst )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( pinst->frame_lst != pinst->frame_nxt )
    {
        pinst->frame_lst = pinst->frame_nxt;
        pinst->ilip      = 0;
        pinst->flip      = 0.0f;

        vlst_cache_test( &( pinst->save ), pinst );
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
const MD2_Frame& chr_instnce_get_frame_nxt(chr_instance_t * pinst)
{
    mad_t * pmad = MadStack.get_ptr( pinst->imad );
    if ( pinst->frame_nxt > pmad->md2_ptr->getFrames().size() )
    {
        log_error( "chr_instnce_get_frame_nxt() - invalid frame %d/%lu\n", pinst->frame_nxt, pmad->md2_ptr->getFrames().size() );
    }

    return pmad->md2_ptr->getFrames()[pinst->frame_nxt];
}

//--------------------------------------------------------------------------------------------
const MD2_Frame& chr_instnce_get_frame_lst(chr_instance_t * pinst)
{
    mad_t * pmad = MadStack.get_ptr( pinst->imad );
    if ( pinst->frame_lst > pmad->md2_ptr->getFrames().size() )
    {
        log_error( "chr_instnce_get_frame_lst() - invalid frame %d/%lu\n", pinst->frame_lst, pmad->md2_ptr->getFrames().size() );
    }

    return pmad->md2_ptr->getFrames()[pinst->frame_lst];
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_update_one_lip( chr_instance_t * pinst )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    pinst->ilip += 1;
    pinst->flip = 0.25f * pinst->ilip;

    vlst_cache_test( &( pinst->save ), pinst );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_update_one_flip( chr_instance_t * pinst, float dflip )
{
    if ( NULL == pinst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL instance" );
        return gfx_error;
    }

    if ( 0.0f == dflip ) return gfx_fail;

    // update the lips
    pinst->flip += dflip;
    pinst->ilip  = (( int )FLOOR( pinst->flip * 4 ) ) % 4;

    vlst_cache_test( &( pinst->save ), pinst );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
float chr_instance_get_remaining_flip( chr_instance_t * pinst )
{
    float remaining = 0.0f;

    if ( NULL == pinst ) return 0.0f;

    remaining = ( pinst->ilip + 1 ) * 0.25f - pinst->flip;

    return remaining;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
chr_reflection_cache_t * chr_reflection_cache_init( chr_reflection_cache_t * pcache )
{
    if ( NULL == pcache ) return pcache;

    BLANK_STRUCT_PTR( pcache )

    pcache->alpha = 127;
    pcache->light = 255;

    return pcache;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
vlst_cache_t * vlst_cache_init( vlst_cache_t * pcache )
{
    if ( NULL == pcache ) return NULL;

    BLANK_STRUCT_PTR( pcache )

    pcache->vmin = -1;
    pcache->vmax = -1;

    return pcache;
}

//--------------------------------------------------------------------------------------------
gfx_rv vlst_cache_test( vlst_cache_t * pcache, chr_instance_t * pinst )
{
    if ( NULL == pcache )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL cache" );
        return gfx_error;
    }

    if ( !pcache->valid ) return gfx_success;

    if ( NULL == pinst )
    {
        pcache->valid = false;
        return gfx_success;
    }

    if ( pinst->frame_lst != pcache->frame_nxt )
    {
        pcache->valid = false;
    }

    if ( pinst->frame_lst != pcache->frame_lst )
    {
        pcache->valid = false;
    }

    if (( pinst->frame_lst != pcache->frame_lst )  && std::abs( pinst->flip - pcache->flip ) > flip_tolerance )
    {
        pcache->valid = false;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
matrix_cache_t * matrix_cache_init( matrix_cache_t * mcache )
{
    /// @author BB
    /// @details clear out the matrix cache data

    int cnt;

    if ( NULL == mcache ) return mcache;

    BLANK_STRUCT_PTR( mcache )

    mcache->type_bits = MAT_UNKNOWN;
    mcache->grip_chr  = INVALID_CHR_REF;
    for ( cnt = 0; cnt < GRIP_VERTS; cnt++ )
    {
        mcache->grip_verts[cnt] = 0xFFFF;
    }

    mcache->rotate.x = 0;
    mcache->rotate.y = 0;
    mcache->rotate.z = 0;

    return mcache;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_set_texture( chr_instance_t * pinst, const TX_REF itex )
{
    if (!pinst)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == pinst");
        return gfx_error;
    }

    // grab the texture
    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(itex);

    // get the transparency info from the texture
    pinst->skin_has_transparency = false;
    if (ptex)
    {
        pinst->skin_has_transparency = oglx_texture_t::hasAlpha(ptex);
    }

    // set the texture index
    pinst->texture = itex;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
bool chr_instance_apply_reflection_matrix( chr_instance_t * pinst, float grid_level )
{
    /// @author BB
    /// @details Generate the extra data needed to display a reflection for this character

    if ( NULL == pinst ) return false;

    // invalidate the current matrix
    pinst->ref.matrix_valid = false;

    // actually flip the matrix
    if ( pinst->matrix_cache.valid )
    {
        pinst->ref.matrix = pinst->matrix;

        pinst->ref.matrix.CNV( 0, 2 ) = -pinst->ref.matrix.CNV( 0, 2 );
        pinst->ref.matrix.CNV( 1, 2 ) = -pinst->ref.matrix.CNV( 1, 2 );
        pinst->ref.matrix.CNV( 2, 2 ) = -pinst->ref.matrix.CNV( 2, 2 );
        pinst->ref.matrix.CNV( 3, 2 ) = 2 * grid_level - pinst->ref.matrix.CNV( 3, 2 );

        pinst->ref.matrix_valid = true;

        // fix the reflection
        chr_instance_update_ref( pinst, grid_level, false );
    }

    return pinst->ref.matrix_valid;
}

//--------------------------------------------------------------------------------------------
void chr_instance_get_tint( chr_instance_t * pinst, GLfloat * tint, const BIT_FIELD bits )
{
    int i;
    float weight_sum;
    GLXvector4f local_tint;

    int local_alpha;
    int local_light;
    int local_sheen;
    int local_redshift;
    int local_grnshift;
    int local_blushift;

    if ( NULL == tint ) tint = local_tint;

    if ( HAS_SOME_BITS( bits, CHR_REFLECT ) )
    {
        // this is a reflection, use the reflected parameters
        local_alpha    = pinst->ref.alpha;
        local_light    = pinst->ref.light;
        local_sheen    = pinst->ref.sheen;
        local_redshift = pinst->ref.redshift;
        local_grnshift = pinst->ref.grnshift;
        local_blushift = pinst->ref.blushift;
    }
    else
    {
        // this is NOT a reflection, use the normal parameters
        local_alpha    = pinst->alpha;
        local_light    = pinst->light;
        local_sheen    = pinst->sheen;
        local_redshift = pinst->redshift;
        local_grnshift = pinst->grnshift;
        local_blushift = pinst->blushift;
    }

    // modify these values based on local character abilities
    local_alpha = get_alpha( local_alpha, local_stats.seeinvis_mag );
    local_light = get_light( local_light, local_stats.seedark_mag );

    // clear out the tint
    weight_sum = 0;
    for ( i = 0; i < 4; i++ ) tint[i] = 0;

    if ( HAS_SOME_BITS( bits, CHR_SOLID ) )
    {
        // solid characters are not blended onto the canvas
        // the alpha channel is not important
        weight_sum += 1.0f;

        tint[RR] += 1.0f / ( 1 << local_redshift );
        tint[GG] += 1.0f / ( 1 << local_grnshift );
        tint[BB] += 1.0f / ( 1 << local_blushift );
        tint[AA] += 1.0f;
    }

    if ( HAS_SOME_BITS( bits, CHR_ALPHA ) )
    {
        // alpha characters are blended onto the canvas using the alpha channel
        // the alpha channel is not important
        weight_sum += 1.0f;

        tint[RR] += 1.0f / ( 1 << local_redshift );
        tint[GG] += 1.0f / ( 1 << local_grnshift );
        tint[BB] += 1.0f / ( 1 << local_blushift );
        tint[AA] += local_alpha * INV_FF;
    }

    if ( HAS_SOME_BITS( bits, CHR_LIGHT ) )
    {
        // alpha characters are blended onto the canvas by adding their color
        // the more black the colors, the less visible the character
        // the alpha channel is not important

        weight_sum += 1.0f;

        if ( local_light < 255 )
        {
            tint[RR] += local_light * INV_FF / ( 1 << local_redshift );
            tint[GG] += local_light * INV_FF / ( 1 << local_grnshift );
            tint[BB] += local_light * INV_FF / ( 1 << local_blushift );
        }

        tint[AA] += 1.0f;
    }

    if ( HAS_SOME_BITS( bits, CHR_PHONG ) )
    {
        // phong is essentially the same as light, but it is the
        // sheen that sets the effect

        float amount;

        weight_sum += 1.0f;

        amount = ( CLIP( local_sheen, 0, 15 ) << 4 ) / 240.0f;

        tint[RR] += amount;
        tint[GG] += amount;
        tint[BB] += amount;
        tint[AA] += 1.0f;
    }

    // average the tint
    if ( weight_sum != 0.0f && weight_sum != 1.0f )
    {
        for ( i = 0; i < 4; i++ )
        {
            tint[i] /= weight_sum;
        }
    }
}


