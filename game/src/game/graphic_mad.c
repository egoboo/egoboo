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
#include "game/renderer_3d.h"
#include "game/game.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "egolib/Graphics/MD2Model.hpp"
#include "egolib/Graphics/ModelDescriptor.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Entities/_Include.hpp"
#include "game/CharacterMatrix.h"

//--------------------------------------------------------------------------------------------

// the flip tolerance is the default flip increment / 2
static constexpr float FLIP_TOLERANCE = 0.25f * 0.5f;

bool matrix_cache_t::isValid() const {
    return valid && matrix_valid;
}


//--------------------------------------------------------------------------------------------

struct Md2Vertex {
    struct {
        float x, y, z;
    } position;
    struct {
        float x, y, z;
    } normal;
    struct {
        float r, g, b, a;
    } colour;
    struct {
        float s, t;
    } texture;
};

struct Md2VertexBuffer {
    size_t size;
    Md2Vertex *vertices;
    Md2VertexBuffer(size_t size) : size(size), vertices(new Md2Vertex[size]) {
        // Intentionally empty.
    }
    Md2VertexBuffer() : Md2VertexBuffer(0) {
        // Intentionally empty.
    }
    ~Md2VertexBuffer() {
        if (nullptr != vertices) {
            delete[] vertices;
            vertices = nullptr;
        }
    }
    void ensureSize(size_t requiredSize) {
        if (requiredSize > size) {
            Md2Vertex *requiredVertices = new Md2Vertex[requiredSize];
            for (size_t i = 0; i < size; ++i) {
                requiredVertices[i] = vertices[i];
            }
            delete[] vertices;
            vertices = requiredVertices;
            size = requiredSize;
        }
    }
    void render(GLenum mode, size_t start, size_t length) {
        glBegin(mode); {
            for (size_t vertexIndex = start; vertexIndex < start + length; ++vertexIndex) {
                const auto& v = vertices[vertexIndex];
                glColor4f(v.colour.r, v.colour.g, v.colour.b, v.colour.a);
                glNormal3f(v.normal.x, v.normal.y, v.normal.z);
                glTexCoord2f(v.texture.s, v.texture.t);
                glVertex3f(v.position.x, v.position.y, v.position.z);
            }
        }
        glEnd();
    }
};

gfx_rv MadRenderer::render_enviro( Camera& cam, const std::shared_ptr<Object>& pchr, GLXvector4f tint, const BIT_FIELD bits )
{
	chr_instance_t& pinst = pchr->inst;

    if (!pinst.animationState.getModelDescriptor())
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, pchr->getObjRef().get(), "invalid mad" );
        return gfx_error;
    }
    const std::shared_ptr<MD2Model>& pmd2 = pchr->getProfile()->getModel()->getMD2();

    std::shared_ptr<const Ego::Texture> ptex = nullptr;
	if (HAS_SOME_BITS(bits, CHR_PHONG))
	{
		ptex = Ego::TextureManager::get().getTexture("mp_data/phong");
	}

	if (!GL_DEBUG(glIsEnabled)(GL_BLEND))
	{
		return gfx_fail;
	}

	if (nullptr == ptex)
	{
		ptex = pchr->getSkinTexture();
	}

    float uoffset = pinst.uoffset - float(cam.getTurnZ_turns());

	if (HAS_SOME_BITS(bits, CHR_REFLECT))
	{
        Ego::Renderer::get().setWorldMatrix(pinst.getReflectionMatrix());
	}
	else
	{
		Ego::Renderer::get().setWorldMatrix(pinst.getMatrix());
	}

    // Choose texture and matrix
	Ego::Renderer::get().getTextureUnit().setActivated(ptex.get());

    {
        Ego::OpenGL::PushAttrib pa(GL_CURRENT_BIT);
        {
            // Get the maximum number of vertices per command.
            size_t vertexBufferCapacity = 0;
            for (const MD2_GLCommand& glcommand : pmd2->getGLCommands()) {
                vertexBufferCapacity = std::max(vertexBufferCapacity, glcommand.data.size());
            }
            // Allocate a vertex buffer.
            Md2VertexBuffer vertexBuffer(vertexBufferCapacity);
            // Render each command
            for (const MD2_GLCommand &glcommand : pmd2->getGLCommands()) {
                // Pre-render this command.
                size_t vertexBufferSize = 0;
                for (const id_glcmd_packed_t& cmd : glcommand.data) {
                    uint16_t vertexIndex = cmd.index;
                    if (vertexIndex >= pinst.getVertexCount()) continue;
                    const GLvertex& pvrt = pinst.getVertex(vertexIndex);
                    auto& v = vertexBuffer.vertices[vertexBufferSize++];
                    v.position.x = pvrt.pos[XX];
                    v.position.y = pvrt.pos[YY];
                    v.position.z = pvrt.pos[ZZ];
                    v.normal.x = pvrt.nrm[XX];
                    v.normal.y = pvrt.nrm[YY];
                    v.normal.z = pvrt.nrm[ZZ];

                    // normalize the color so it can be modulated by the phong/environment map
                    v.colour.r = pvrt.color_dir * INV_FF<float>();
                    v.colour.g = pvrt.color_dir * INV_FF<float>();
                    v.colour.b = pvrt.color_dir * INV_FF<float>();
                    v.colour.a = 1.0f;

                    float cmax = std::max({v.colour.r, v.colour.g, v.colour.b});

                    if (cmax != 0.0f) {
                        v.colour.r /= cmax;
                        v.colour.g /= cmax;
                        v.colour.b /= cmax;
                    }

                    // apply the tint
                    v.colour.r *= tint[RR];
                    v.colour.g *= tint[GG];
                    v.colour.b *= tint[BB];
                    v.colour.a *= tint[AA];

                    v.texture.s = pvrt.env[XX] + uoffset;
                    v.texture.t = Ego::Math::constrain(cmax, 0.0f, 1.0f);

                    if (0 != (bits & CHR_PHONG)) {
                        // determine the phong texture coordinates
                        // the default phong is bright in both the forward and back directions...
                        v.texture.t = v.texture.t * 0.5f + 0.5f;
                    }
                }
                // Render this command.
                vertexBuffer.render(glcommand.glMode, 0, vertexBufferSize);
            }
        }
    }
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
        z = (pinst->_vertexList[cnt].pos[ZZ]) + pchr->matrix(3,2);

        // Figure out the fog coloring
        if(z < fogtop)
        {
            if(z < fogbottom)
            {
                pinst->_vertexList[cnt].specular = alpha;
            }
            else
            {
                z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f...  Amount of fog to keep
                red = fogred * z;
                grn = foggrn * z;
                blu = fogblu * z;
                fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                pinst->_vertexList[cnt].specular = fogspec;
            }
        }
        else
        {
            pinst->_vertexList[cnt].specular = 0;
        }
    }
}

else
{
    for (cnt = 0; cnt < pmad->transvertices; cnt++)
        pinst->_vertexList[cnt].specular = 0;
}

*/

//--------------------------------------------------------------------------------------------
gfx_rv MadRenderer::render_tex(Camera& camera, const std::shared_ptr<Object>& pchr, GLXvector4f tint, const BIT_FIELD bits)
{
    chr_instance_t& pinst = pchr->inst;

    if (!pinst.animationState.getModelDescriptor())
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
        return gfx_error;
    }

    const std::shared_ptr<MD2Model> &pmd2 = pchr->getProfile()->getModel()->getMD2();

    // To make life easier
    std::shared_ptr<const Ego::Texture> ptex = pchr->getSkinTexture();

    float uoffset = pinst.uoffset * INV_FFFF<float>();
    float voffset = pinst.voffset * INV_FFFF<float>();

    float base_amb = 0.0f;
    if (0 == (bits & CHR_LIGHT))
    {
        // Convert the "light" parameter to self-lighting for
        // every object that is not being rendered using CHR_LIGHT.
        base_amb = (0xFF == pinst.light) ? 0 : (pinst.light * INV_FF<float>());
    }

    // Get the maximum number of vertices per command.
    size_t vertexBufferCapacity = 0;
    for (const MD2_GLCommand& glcommand : pmd2->getGLCommands())
    {
        vertexBufferCapacity = std::max(vertexBufferCapacity, glcommand.data.size());
    }
    // Allocate a vertex buffer.
    Md2VertexBuffer vertexBuffer(vertexBufferCapacity);

    if (0 != (bits & CHR_REFLECT))
    {
        Ego::Renderer::get().setWorldMatrix(pinst.getReflectionMatrix());
    }
    else
    {
        Ego::Renderer::get().setWorldMatrix(pinst.getMatrix());
    }

    // Choose texture.
	Ego::Renderer::get().getTextureUnit().setActivated(ptex.get());

    {
        Ego::OpenGL::PushAttrib pa(GL_CURRENT_BIT);
        {
            // Render each command
            for (const MD2_GLCommand& glcommand : pmd2->getGLCommands()) {
                // Pre-render this command.
                size_t vertexBufferSize = 0;
                for (const id_glcmd_packed_t &cmd : glcommand.data) {
                    Uint16 vertexIndex = cmd.index;
                    if (vertexIndex >= pinst.getVertexCount()) {
                        continue;
                    }
                    const GLvertex& pvrt = pinst.getVertex(vertexIndex);
                    auto& v = vertexBuffer.vertices[vertexBufferSize++];
                    v.position.x = pvrt.pos[XX];
                    v.position.y = pvrt.pos[YY];
                    v.position.z = pvrt.pos[ZZ];
                    v.normal.x = pvrt.nrm[XX];
                    v.normal.y = pvrt.nrm[YY];
                    v.normal.z = pvrt.nrm[ZZ];

                    // Determine the texture coordinates.
                    v.texture.s = cmd.s + uoffset;
                    v.texture.t = cmd.t + voffset;

                    // Perform lighting.
                    if (HAS_NO_BITS(bits, CHR_LIGHT) && HAS_NO_BITS(bits, CHR_ALPHA)) {
                        // The directional lighting.
                        float fcol = pvrt.color_dir * INV_FF<float>();

                        v.colour.r = fcol;
                        v.colour.g = fcol;
                        v.colour.b = fcol;
                        v.colour.a = 1.0f;

                        // Ambient lighting.
                        if (HAS_NO_BITS(bits, CHR_PHONG)) {
                            // Convert the "light" parameter to self-lighting for
                            // every object that is not being rendered using CHR_LIGHT.

                            float acol = base_amb + pinst.color_amb * INV_FF<float>();

                            v.colour.r += acol;
                            v.colour.g += acol;
                            v.colour.b += acol;
                        }

                        // clip the colors
                        v.colour.r = Ego::Math::constrain(v.colour.r, 0.0f, 1.0f);
                        v.colour.g = Ego::Math::constrain(v.colour.g, 0.0f, 1.0f);
                        v.colour.b = Ego::Math::constrain(v.colour.b, 0.0f, 1.0f);

                        // tint the object
                        v.colour.r *= tint[RR];
                        v.colour.g *= tint[GG];
                        v.colour.b *= tint[BB];
                    } else {
                        // Set the basic tint.
                        v.colour.r = tint[RR];
                        v.colour.g = tint[GG];
                        v.colour.b = tint[BB];
                        v.colour.a = tint[AA];
                    }
                }
                // Render this command.
                vertexBuffer.render(glcommand.glMode, 0, vertexBufferSize);
            }
        }
    }
    return gfx_success;
}

/*
    // Do fog...
    if(fogon && pinst->light==255)
    {
        // The full fog value
*        alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

        for (cnt = 0; cnt < pmad->transvertices; cnt++)
        {
            // Figure out the z position of the vertex...  Not totally accurate
            z = (pinst->_vertexList[cnt].pos[ZZ]) + pchr->matrix(3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    pinst->_vertexList[cnt].specular = alpha;
                }
                else
                {
                    spek = pinst->_vertexList[cnt].specular & 255;
                    z = (z - fogbottom)/fogdistance;  // 0.0f to 1.0f...  Amount of old to keep
                    fogtokeep = 1.0f-z;  // 0.0f to 1.0f...  Amount of fog to keep
                    spek = spek * z;
                    red = (fogred * fogtokeep) + spek;
                    grn = (foggrn * fogtokeep) + spek;
                    blu = (fogblu * fogtokeep) + spek;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    pinst->_vertexList[cnt].specular = fogspec;
                }
            }
        }
    }
*/

gfx_rv MadRenderer::render(Camera& cam, const std::shared_ptr<Object> &pchr, GLXvector4f tint, const BIT_FIELD bits)
{
    /// @author ZZ
    /// @details This function picks the actual function to use

    gfx_rv retval;

    //Not visible?
    if (pchr->isHidden() || tint[AA] <= 0.0f || pchr->isInsideInventory()) {
        return gfx_fail;
    }

    if ( pchr->getProfile()->isPhongMapped() || HAS_SOME_BITS(bits, CHR_PHONG) )
    {
        retval = render_enviro(cam, pchr, tint, bits);
    }
    else
    {
        retval = render_tex(cam, pchr, tint, bits);
    }

#if defined(_DEBUG)
    // don't draw the debug stuff for reflections
    if ( 0 == ( bits & CHR_REFLECT ) )
    {
        draw_chr_bbox(pchr);
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
gfx_rv MadRenderer::render_ref( Camera& cam, const std::shared_ptr<Object>& pchr)
{
	chr_instance_t& pinst = pchr->inst;

    //Does this object have a reflection?
    if(!pchr->getProfile()->hasReflection()) {
        return gfx_fail;        
    }

    if ( pchr->isHidden() ) return gfx_fail;

    // assume the best
	gfx_rv retval = gfx_success;

    {
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
        {
            auto& renderer = Ego::Renderer::get();
            // cull backward facing polygons
            // use couter-clockwise orientation to determine backfaces
            oglx_begin_culling(Ego::CullingMode::Back, MAD_REF_CULL);
            Ego::OpenGL::Utilities::isError();

            //Transparent
            if (pinst.getReflectionAlpha() != 0xFF && pinst.light == 0xFF) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

                GLXvector4f tint;
                pinst.getTint(tint, true, CHR_ALPHA);

                // the previous call to chr_instance_update_lighting_ref() has actually set the
                // alpha and light for all vertices
                if (gfx_error == render(cam, pchr, tint, CHR_ALPHA | CHR_REFLECT)) {
                    retval = gfx_error;
                }
            }

            //Glowing
            if (pinst.light != 0xFF) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pinst.getTint(tint, true, CHR_LIGHT);

                // the previous call to chr_instance_update_lighting_ref() has actually set the
                // alpha and light for all vertices
                if (gfx_error == MadRenderer::render(cam, pchr, tint, CHR_LIGHT)) {
                    retval = gfx_error;
                }
                Ego::OpenGL::Utilities::isError();
            }

            //Render shining effect on top of model
            if (pinst.getReflectionAlpha() == 0xFF && gfx.phongon && pinst.sheen > 0) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pinst.getTint(tint, true, CHR_PHONG);

                if (gfx_error == MadRenderer::render(cam, pchr, tint, CHR_PHONG)) {
                    retval = gfx_error;
                }
                Ego::OpenGL::Utilities::isError();
            }
        }
    }

    return retval;
}

gfx_rv MadRenderer::render_trans(Camera& cam, const std::shared_ptr<Object>& pchr)
{
	chr_instance_t& pinst = pchr->inst;

    if ( pchr->isHidden() ) return gfx_fail;

    // there is an outside chance the object will not be rendered
    bool rendered = false;

    {
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
        {
            auto& renderer = Ego::Renderer::get();

            if (pinst.alpha < 0xFF) {
                // most alpha effects will be messed up by
                // skipping backface culling, so don't

                // cull backward facing polygons
                // use clockwise orientation to determine backfaces
                oglx_begin_culling(Ego::CullingMode::Back, MAD_NRM_CULL);

                // get a speed-up by not displaying completely transparent portions of the skin
                renderer.setAlphaTestEnabled(true);
                renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);

                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::One);

                GLXvector4f tint;
                pinst.getTint(tint, false, CHR_ALPHA);

                if (render(cam, pchr, tint, CHR_ALPHA)) {
                    rendered = true;
                }
            }

            else if (pinst.light < 0xFF) {
                // light effects should show through transparent objects
                renderer.setCullingMode(Ego::CullingMode::None);

                // the alpha test can only mess us up here
                renderer.setAlphaTestEnabled(false);

                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pinst.getTint(tint, false, CHR_LIGHT);

                if (render(cam, pchr, tint, CHR_LIGHT)) {
                    rendered = true;
                }
            }

            // Render shining effect on top of model
            if (pinst.getReflectionAlpha() == 0xFF && gfx.phongon && pinst.sheen > 0) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pinst.getTint(tint, false, CHR_PHONG);

                if (render(cam, pchr, tint, CHR_PHONG)) {
                    rendered = true;
                }
            }
        }
    }

    return rendered ? gfx_success : gfx_fail;
}

gfx_rv MadRenderer::render_solid( Camera& cam, const std::shared_ptr<Object> &pchr )
{
	chr_instance_t& pinst = pchr->inst;

    if ( pchr->isHidden() ) return gfx_fail;

    //Only proceed if we are truly fully solid
    if (0xFF != pinst.alpha || 0xFF != pinst.light) {
        return gfx_success;
    }

    // assume the best
	gfx_rv retval = gfx_success;

    {
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
        {
            auto& renderer = Ego::Renderer::get();
            // do not display the completely transparent portion
            // this allows characters that have "holes" in their
            // textures to display the solid portions properly
            //
            // Objects with partially transparent skins should enable the [MODL] parameter "T"
            // which will enable the display of the partially transparent portion of the skin

            renderer.setAlphaTestEnabled(true);
            renderer.setAlphaFunction(Ego::CompareFunction::Equal, 1.0f);

            // can I turn this off?
            renderer.setBlendingEnabled(true);
            renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

            // allow the dont_cull_backfaces to keep solid objects from culling backfaces
            if (pchr->getProfile()->isDontCullBackfaces()) {
                // stop culling backward facing polugons
                renderer.setCullingMode(Ego::CullingMode::None);
            } else {
                // cull backward facing polygons
                // use couter-clockwise orientation to determine backfaces
                oglx_begin_culling(Ego::CullingMode::Back, MAD_NRM_CULL);            // GL_ENABLE_BIT | GL_POLYGON_BIT
            }

            GLXvector4f tint;
            pinst.getTint(tint, false, CHR_SOLID);

            if (gfx_error == render(cam, pchr, tint, CHR_SOLID)) {
                retval = gfx_error;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
#if _DEBUG
void MadRenderer::draw_chr_bbox(const std::shared_ptr<Object>& pchr)
{
    static constexpr bool drawLeftSlot = false;
    static constexpr bool drawRightSlot = false;
    static constexpr bool drawCharacter = true;
    
    // Draw the object bounding box as a part of the graphics debug mode F7.
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && Ego::Input::InputSystem::get().isKeyDown(SDLK_F7))
    {
        Ego::Renderer::get().setWorldMatrix(Matrix4f4f::identity());

        if (drawLeftSlot)
        {
            oct_bb_t bb;
            bb = oct_bb_t::translate(pchr->slot_cv[SLOT_LEFT], pchr->getPosition());
            Renderer3D::renderOctBB(bb, true, true);
        }
        if (drawRightSlot)
        {
            oct_bb_t bb;
            bb = oct_bb_t::translate(pchr->slot_cv[SLOT_RIGHT], pchr->getPosition());
            Renderer3D::renderOctBB(bb, true, true);
        }
        if (drawCharacter)
        {
            oct_bb_t bb;
            bb = oct_bb_t::translate(pchr->chr_min_cv, pchr->getPosition());
            Renderer3D::renderOctBB(bb, true, true);
        }
    }

    //// The grips and vertrices of all objects.
    if (Ego::Input::InputSystem::get().isKeyDown(SDLK_F6))
    {
        draw_chr_attached_grip( pchr );

        // Draw all the vertices of an object
        GL_DEBUG(glPointSize(5));
        draw_chr_verts(pchr, 0, pchr->inst.getVertexCount());
    }
}
#endif

#if _DEBUG
void MadRenderer::draw_chr_verts(const std::shared_ptr<Object>& pchr, int vrt_offset, int verts )
{
    /// @author BB
    /// @details a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.

    int vmin, vmax, cnt;

    vmin = vrt_offset;
    vmax = vmin + verts;

    if ( vmin < 0 || ( size_t )vmin > pchr->inst.getVertexCount() ) return;
    if ( vmax < 0 || ( size_t )vmax > pchr->inst.getVertexCount() ) return;

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    Ego::Renderer::get().getTextureUnit().setActivated(nullptr);

	Ego::Renderer::get().setWorldMatrix(pchr->inst.getMatrix());
    GL_DEBUG( glBegin( GL_POINTS ) );
    {
        for ( cnt = vmin; cnt < vmax; cnt++ )
        {
            GL_DEBUG( glVertex3fv )( pchr->inst.getVertex(cnt).pos );
        }
    }
    GL_DEBUG_END();
}
#endif

#if _DEBUG
void MadRenderer::draw_one_grip( chr_instance_t * pinst, int slot )
{
    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    Ego::Renderer::get().getTextureUnit().setActivated(nullptr);

    Ego::Renderer::get().setWorldMatrix(pinst->getMatrix());

    _draw_one_grip_raw( pinst, slot );
}

void MadRenderer::_draw_one_grip_raw( chr_instance_t * pinst, int slot )
{
    int vmin, vmax, cnt;

    float red[4] = {1, 0, 0, 1};
    float grn[4] = {0, 1, 0, 1};
    float blu[4] = {0, 0, 1, 1};
    float * col_ary[3];

    col_ary[0] = red;
    col_ary[1] = grn;
    col_ary[2] = blu;

    if ( NULL == pinst ) return;

    vmin = ( int )pinst->getVertexCount() - ( int )slot_to_grip_offset(( slot_t )slot );
    vmax = vmin + GRIP_VERTS;

    if ( vmin >= 0 && vmax >= 0 && ( size_t )vmax <= pinst->getVertexCount() )
    {
		Vector3f src, dst, diff;

        GL_DEBUG( glBegin )( GL_LINES );
        {
            for ( cnt = 1; cnt < GRIP_VERTS; cnt++ )
            {
                src[kX] = pinst->getVertex(vmin).pos[XX];
                src[kY] = pinst->getVertex(vmin).pos[YY];
                src[kZ] = pinst->getVertex(vmin).pos[ZZ];

                diff[kX] = pinst->getVertex(vmin+cnt).pos[XX] - src[kX];
                diff[kY] = pinst->getVertex(vmin+cnt).pos[YY] - src[kY];
                diff[kZ] = pinst->getVertex(vmin+cnt).pos[ZZ] - src[kZ];

                dst[kX] = src[kX] + 3 * diff[kX];
                dst[kY] = src[kY] + 3 * diff[kY];
                dst[kZ] = src[kZ] + 3 * diff[kZ];

                GL_DEBUG( glColor4fv )( col_ary[cnt-1] );

                GL_DEBUG( glVertex3f )( src[kX], src[kY], src[kZ] );
                GL_DEBUG( glVertex3f )( dst[kX], dst[kY], dst[kZ] );
            }
        }
        GL_DEBUG_END();
    }

	Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());
}

void MadRenderer::draw_chr_attached_grip(const std::shared_ptr<Object>& pchr)
{
    const std::shared_ptr<Object> &pholder = pchr->getHolder();
    if (!pholder || pholder->isTerminated()) return;

    draw_one_grip( &( pholder->inst ), pchr->inwhich_slot );
}
#endif

#if 0
void MadRenderer::draw_chr_grips( Object * pchr )
{
    mad_t * pmad;

    GLint matrix_mode[1];

    if ( !ACTIVE_PCHR( pchr ) ) return;

    const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(pchr->profile_ref);

    pmad = chr_get_pmad( GET_INDEX_PCHR( pchr ) );
    if ( NULL == pmad ) return;

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    Ego::Renderer::getTextureUnit().setActivated(nullptr);

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
#endif

//--------------------------------------------------------------------------------------------

void chr_instance_t::update_lighting_base(chr_instance_t& self, Object *pchr, bool force)
{
    /// @author BB
    /// @details determine the basic per-vertex lighting

    const int frame_skip = 1 << 2;
    const int frame_mask = frame_skip - 1;

	if (!pchr) {
		return;
	}

    // force this function to be evaluated the 1st time through
    if (self.lighting_frame_all < 0) {
        force = true;
    }

    // has this already been calculated this update?
	if (!force && self.lighting_update_wld >= 0 && static_cast<uint32_t>(self.lighting_update_wld) >= update_wld) {
		return;
	}
	self.lighting_update_wld = update_wld;

    // make sure the matrix is valid
    chr_update_matrix(pchr, true);

    // has this already been calculated in the last frame_skip frames?
	if (!force && self.lighting_frame_all >= 0 && static_cast<uint32_t>(self.lighting_frame_all) >= game_frame_all) {
		return;
	}

    // reduce the amount of updates to one every frame_skip frames, but dither
    // the updating so that not all objects update on the same frame
    self.lighting_frame_all = game_frame_all + ((game_frame_all + pchr->getObjRef().get()) & frame_mask);

	if (!self.animationState.getModelDescriptor()) {
		return;
	}

    // interpolate the lighting for the origin of the object

	auto mesh = _currentModule->getMeshPointer();
	if (!mesh) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
	}

	lighting_cache_t global_light;
    GridIllumination::grid_lighting_interpolate(*mesh, global_light, Vector2f(pchr->getPosX(), pchr->getPosY()));

    // rotate the lighting data to body_centered coordinates
	lighting_cache_t loc_light;
	lighting_cache_t::lighting_project_cache(loc_light, global_light, self.getMatrix());

    //Low-pass filter to smooth lighting transitions?
    //self.color_amb = 0.9f * self.color_amb + 0.1f * (loc_light.hgh._lighting[LVEC_AMB] + loc_light.low._lighting[LVEC_AMB]) * 0.5f;
    //self.color_amb = (loc_light.hgh._lighting[LVEC_AMB] + loc_light.low._lighting[LVEC_AMB]) * 0.5f;
    self.color_amb = get_ambient_level();

    self.max_light = -255;
    self.min_light =  255;
    for (size_t cnt = 0; cnt < self._vertexList.size(); cnt++ )
    {
        float lite = 0.0f;

        GLvertex *pvert = &self._vertexList[cnt];

        // a simple "height" measurement
        float hgt = pvert->pos[ZZ] * self.getMatrix()(3, 3) + self.getMatrix()(3, 3);

        if (pvert->nrm[0] == 0.0f && pvert->nrm[1] == 0.0f && pvert->nrm[2] == 0.0f)
        {
            // this is the "ambient only" index, but it really means to sum up all the light
            lite  = lighting_cache_t::lighting_evaluate_cache(loc_light, Vector3f(+1.0f,+1.0f,+1.0f), hgt, _currentModule->getMeshPointer()->_tmem._bbox, nullptr, nullptr);
            lite += lighting_cache_t::lighting_evaluate_cache(loc_light, Vector3f(-1.0f,-1.0f,-1.0f), hgt, _currentModule->getMeshPointer()->_tmem._bbox, nullptr, nullptr);

            // average all the directions
            lite /= 6.0f;
        }
        else
        {
            lite = lighting_cache_t::lighting_evaluate_cache( loc_light, Vector3f(pvert->nrm[0],pvert->nrm[1],pvert->nrm[2]), hgt, _currentModule->getMeshPointer()->_tmem._bbox, nullptr, nullptr);
        }

        pvert->color_dir = lite;

        self.max_light = std::max(self.max_light, pvert->color_dir);
        self.min_light = std::min(self.min_light, pvert->color_dir);
    }

    // ??coerce this to reasonable values in the presence of negative light??
    self.max_light = std::max(self.max_light, 0);
    self.min_light = std::max(self.min_light, 0);
}

oct_bb_t chr_instance_t::getBoundingBox() const
{
    //Beginning of a frame animation
    if (this->animationState.getTargetFrameIndex() == this->animationState.getSourceFrameIndex() || this->animationState.flip == 0.0f) {
        return getLastFrame().bb;
    } 

    //Finished frame animation
    if (this->animationState.flip == 1.0f) {
        return getNextFrame().bb;
    } 

    //We are middle between two animation frames
    return oct_bb_t::interpolate(getLastFrame().bb, getNextFrame().bb, this->animationState.flip);
}

gfx_rv chr_instance_t::needs_update(int vmin, int vmax, bool *verts_match, bool *frames_match)
{
	bool local_verts_match, local_frames_match;

    // ensure that the pointers point to something
    if ( NULL == verts_match ) verts_match  = &local_verts_match;
    if ( NULL == frames_match ) frames_match = &local_frames_match;

    // initialize the boolean pointers
    *verts_match  = false;
    *frames_match = false;

	vlst_cache_t *psave = &(this->save);

    // check to see if the vlst_cache has been marked as invalid.
    // in this case, everything needs to be updated
	if (!psave->valid) {
		return gfx_success;
	}

    // get the last valid vertex from the chr_instance
    int maxvert = ((int)this->_vertexList.size()) - 1;

    // check to make sure the lower bound of the saved data is valid.
    // it is initialized to an invalid value (psave->vmin = psave->vmax = -1)
	if (psave->vmin < 0 || psave->vmax < 0) {
		return gfx_success;
	}
    // check to make sure the upper bound of the saved data is valid.
	if (psave->vmin > maxvert || psave->vmax > maxvert) {
		return gfx_success;
	}
    // make sure that the min and max vertices are in the correct order
	if (vmax < vmin) {
		std::swap(vmax, vmin);
	}
    // test to see if we have already calculated this data
    *verts_match = (vmin >= psave->vmin) && (vmax <= psave->vmax);

	bool flips_match = (std::abs(psave->flip - this->animationState.flip) < FLIP_TOLERANCE);

    *frames_match = (this->animationState.getTargetFrameIndex() == this->animationState.getSourceFrameIndex() && psave->frame_nxt == this->animationState.getTargetFrameIndex() && psave->frame_lst == this->animationState.getSourceFrameIndex() ) ||
                    (flips_match && psave->frame_nxt == this->animationState.getTargetFrameIndex() && psave->frame_lst == this->animationState.getSourceFrameIndex());

    return (!(*verts_match) || !( *frames_match )) ? gfx_success : gfx_fail;
}

void chr_instance_t::interpolateVerticesRaw(const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip )
{
    /// raw indicates no bounds checking, so be careful

    int i;

    if ( 0.0f == flip )
    {
        for (size_t i = vmin; i <= vmax; i++)
        {
            GLvertex* dst = &_vertexList[i];
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
        for (size_t i = vmin; i <= vmax; i++ )
        {
            GLvertex* dst = &_vertexList[i];
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
        uint16_t vrta_lst, vrta_nxt;

        for (size_t i = vmin; i <= vmax; i++)
        {
            GLvertex* dst = &_vertexList[i];
            const MD2_Vertex &srcLast = lst_ary[i];
            const MD2_Vertex &srcNext = nxt_ary[i];

            dst->pos[XX] = srcLast.pos[kX] + ( srcNext.pos[kX] - srcLast.pos[kX] ) * flip;
            dst->pos[YY] = srcLast.pos[kY] + ( srcNext.pos[kY] - srcLast.pos[kY] ) * flip;
            dst->pos[ZZ] = srcLast.pos[kZ] + ( srcNext.pos[kZ] - srcLast.pos[kZ] ) * flip;
            dst->pos[WW] = 1.0f;

            dst->nrm[XX] = srcLast.nrm[kX] + ( srcNext.nrm[kX] - srcLast.nrm[kX] ) * flip;
            dst->nrm[YY] = srcLast.nrm[kY] + ( srcNext.nrm[kY] - srcLast.nrm[kY] ) * flip;
            dst->nrm[ZZ] = srcLast.nrm[kZ] + ( srcNext.nrm[kZ] - srcLast.nrm[kZ] ) * flip;

            vrta_lst = srcLast.normal;
            vrta_nxt = srcNext.normal;

            dst->env[XX] = indextoenvirox[vrta_lst] + ( indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst] ) * flip;
            dst->env[YY] = 0.5f * ( 1.0f + dst->nrm[ZZ] );
        }
    }
}

gfx_rv chr_instance_t::updateVertices(int vmin, int vmax, bool force)
{
    bool vertices_match, frames_match;
    float  loc_flip;

    int vdirty1_min = -1, vdirty1_max = -1;
    int vdirty2_min = -1, vdirty2_max = -1;

    // get the model
    const std::shared_ptr<MD2Model> &pmd2 = this->animationState.getModelDescriptor()->getMD2();

    // make sure we have valid data
    if (_vertexList.size() != pmd2->getVertexCount())
    {
		Log::get().warn( "chr_instance_update_vertices() - character instance vertex data does not match its md2\n" );
        return gfx_error;
    }

    // get the vertex list size from the chr_instance
    int maxvert = static_cast<int>(_vertexList.size()) - 1;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = maxvert;

    // are they in the right order?
    if ( vmax < vmin ) std::swap(vmax, vmin);

    // make sure that the vertices are within the max range
    vmin = Ego::Math::constrain(vmin, 0, maxvert);
    vmax = Ego::Math::constrain(vmax, 0, maxvert);

    if (force)
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
        gfx_rv retval = needs_update(vmin, vmax, &vertices_match, &frames_match );
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
            if ( vmin < this->save.vmin )
            {
                vdirty1_min = vmin;
                vdirty1_max = this->save.vmin - 1;
            }

            if ( vmax > this->save.vmax )
            {
                vdirty2_min = this->save.vmax + 1;
                vdirty2_max = vmax;
            }
        }
    }

    // make sure the frames are in the valid range
    const std::vector<MD2_Frame> &frameList = pmd2->getFrames();
    if ( this->animationState.getTargetFrameIndex() >= frameList.size() || this->animationState.getSourceFrameIndex() >= frameList.size() )
    {
		Log::get().warn("%s:%d:%s: character instance frame is outside the range of its MD2\n", __FILE__, __LINE__, __FUNCTION__ );
        return gfx_error;
    }

    // grab the frame data from the correct model
    const MD2_Frame &nextFrame = frameList[this->animationState.getTargetFrameIndex()];
    const MD2_Frame &lastFrame = frameList[this->animationState.getSourceFrameIndex()];

    // fix the flip for objects that are not animating
    loc_flip = this->animationState.flip;
    if ( this->animationState.getTargetFrameIndex() == this->animationState.getSourceFrameIndex() ) {
        loc_flip = 0.0f;
    }

    // interpolate the 1st dirty region
    if ( vdirty1_min >= 0 && vdirty1_max >= 0 )
    {
		interpolateVerticesRaw(lastFrame.vertexList, nextFrame.vertexList, vdirty1_min, vdirty1_max, loc_flip);
    }

    // interpolate the 2nd dirty region
    if ( vdirty2_min >= 0 && vdirty2_max >= 0 )
    {
		interpolateVerticesRaw(lastFrame.vertexList, nextFrame.vertexList, vdirty2_min, vdirty2_max, loc_flip);
    }

    // update the saved parameters
    return updateVertexCache(vmax, vmin, force, vertices_match, frames_match);
}

gfx_rv chr_instance_t::updateVertexCache(int vmax, int vmin, bool force, bool vertices_match, bool frames_match)
{
    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // this->save.* values be tested and stored properly

	int maxvert = ((int)this->_vertexList.size()) - 1;

    // the save_vmin and save_vmax is the most complex
    bool verts_updated = false;
    if ( force )
    {
        // to get here, either the specified range was outside the clean range or
        // the animation was updated. In any case, the only vertices that are
        // clean are in the range [vmin, vmax]

        this->save.vmin   = vmin;
        this->save.vmax   = vmax;
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
        this->save.vmin = vmin;
        this->save.vmax = vmax;
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

        if ( vmax >= this->save.vmin && vmin <= this->save.vmax )
        {
            // the old list [save_vmin, save_vmax] and the new list [vmin, vmax]
            // overlap, so we can merge them
            this->save.vmin = std::min( this->save.vmin, vmin );
            this->save.vmax = std::max( this->save.vmax, vmax );
            verts_updated = true;
        }
        else
        {
            // the old list and the new list are disjoint sets, so we are out of luck
            // save the set with the largest number of members
            if (( this->save.vmax - this->save.vmin ) >= ( vmax - vmin ) )
            {
                // obviously no change...
                //this->save.vmin = this->save.vmin;
                //this->save.vmax = this->save.vmax;
                verts_updated = true;
            }
            else
            {
                this->save.vmin = vmin;
                this->save.vmax = vmax;
                verts_updated = true;
            }
        }
    }
    else
    {
        // The only way to get here is to fail the vertices_match test, and fail the frames_match test

        // everything was dirty, so just save the new vertex list
        this->save.vmin = vmin;
        this->save.vmax = vmax;
        verts_updated = true;
    }

    this->save.frame_nxt = this->animationState.getTargetFrameIndex();
    this->save.frame_lst = this->animationState.getSourceFrameIndex();
    this->save.flip      = this->animationState.flip;

    // store the last time there was an update to the animation
    bool frames_updated = false;
    if ( !frames_match )
    {
        this->save.frame_wld = update_wld;
        frames_updated   = true;
    }

    // store the time of the last full update
    if ( 0 == vmin && maxvert == vmax )
    {
        this->save.vert_wld  = update_wld;
    }

    // mark the saved vlst_cache data as valid
    this->save.valid = true;

    return ( verts_updated || frames_updated ) ? gfx_success : gfx_fail;
}

gfx_rv chr_instance_t::update_grip_verts(chr_instance_t& self, uint16_t vrt_lst[], size_t vrt_count )
{
    int vmin, vmax;
    uint32_t cnt;
    size_t count;
    gfx_rv retval;

    if ( NULL == vrt_lst || 0 == vrt_count ) return gfx_fail;

    // count the valid attachment points
    vmin = 0xFFFF;
    vmax = 0;
    count = 0;
    for ( cnt = 0; cnt < vrt_count; cnt++ )
    {
        if ( 0xFFFF == vrt_lst[cnt] ) continue;

        vmin = std::min<uint16_t>( vmin, vrt_lst[cnt] );
        vmax = std::max<uint16_t>( vmax, vrt_lst[cnt] );
        count++;
    }

    // if there are no valid points, there is nothing to do
    if ( 0 == count ) return gfx_fail;

    // force the vertices to update
    retval = self.updateVertices(vmin, vmax, true);

    return retval;
}

gfx_rv chr_instance_t::setAction(const ModelAction action, const bool action_ready, const bool override_action)
{
    // is the chosen action valid?
	if (!animationState.getModelDescriptor()->isActionValid(action)) {
		return gfx_fail;
	}

    // are we going to check action_ready?
	if (!override_action && !actionState.action_ready) {
		return gfx_fail;
	}

    // save the old action
	int action_old = actionState.action_which;

    // set up the action
	actionState.action_which = action;
	actionState.action_next = ACTION_DA;
	actionState.action_ready = action_ready;

    // invalidate the vertex list if the action has changed
	if (action_old != action) {
		save.valid = false;
    }

    return gfx_success;
}

gfx_rv chr_instance_t::setFrame(int frame)
{
    if (this->actionState.action_which < 0 || this->actionState.action_which > ACTION_COUNT) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, this->actionState.action_which, "invalid action range");
        return gfx_error;
    }

    // is the frame within the valid range for this action?
    if(!this->animationState.getModelDescriptor()->isFrameValid(this->actionState.action_which, frame)) {
        return gfx_fail;
    }

    // jump to the next frame
	this->animationState.flip = 0.0f;
	this->animationState.ilip = 0;
	this->animationState.setSourceFrameIndex(this->animationState.getTargetFrameIndex());
	this->animationState.setTargetFrameIndex(frame);

	vlst_cache_t::test(this->save, this);

    return gfx_success;
}

gfx_rv chr_instance_t::startAnimation(const ModelAction action, const bool action_ready, const bool override_action)
{
    gfx_rv retval = setAction(action, action_ready, override_action);
    if ( rv_success != retval ) return retval;

    retval = setFrame(animationState.getModelDescriptor()->getFirstFrame(action));
    if ( rv_success != retval ) return retval;

    return gfx_success;
}

gfx_rv chr_instance_t::incrementAction()
{
    // get the correct action
	ModelAction action = animationState.getModelDescriptor()->getAction(actionState.action_next);

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    // @note ZF> Can't use ACTION_IS_TYPE(action, D) because of GCC compile warning
    bool action_ready = action < ACTION_DD || ACTION_IS_TYPE(action, W);

	return startAnimation(action, action_ready, true);
}

gfx_rv chr_instance_t::incrementFrame(const ObjectRef imount, const ModelAction mount_action)
{
    // fix the ilip and flip
	this->animationState.ilip = this->animationState.ilip % 4;
	this->animationState.flip = fmod(this->animationState.flip, 1.0f);

    // Change frames
	int frame_lst = this->animationState.getTargetFrameIndex();
	int frame_nxt = this->animationState.getTargetFrameIndex() + 1;

    // detect the end of the animation and handle special end conditions
	if (frame_nxt > this->animationState.getModelDescriptor()->getLastFrame(this->actionState.action_which))
    {
		if (this->actionState.action_keep)
        {
            // Freeze that animation at the last frame
            frame_nxt = frame_lst;

            // Break a kept action at any time
			this->actionState.action_ready = true;
        }
		else if (this->actionState.action_loop)
        {
            // Convert the action into a riding action if the character is mounted
            if (_currentModule->getObjectHandler().exists(imount))
            {
				startAnimation(mount_action, true, true);
            }

            // set the frame to the beginning of the action
			frame_nxt = this->animationState.getModelDescriptor()->getFirstFrame(this->actionState.action_which);

            // Break a looped action at any time
			this->actionState.action_ready = true;
        }
        else
        {
            // Go on to the next action. don't let just anything interrupt it?
			incrementAction();

            // incrementAction() actually sets this value properly. just grab the new value.
			frame_nxt = this->animationState.getTargetFrameIndex();
        }
    }

	this->animationState.setSourceFrameIndex(frame_lst);
	this->animationState.setTargetFrameIndex(frame_nxt);

	vlst_cache_t::test(this->save, this);

    return gfx_success;
}

gfx_rv chr_instance_t::playAction(const ModelAction action, const bool action_ready)
{
    return startAnimation(animationState.getModelDescriptor()->getAction(action), action_ready, true);
}

void chr_instance_t::clearCache()
{
    /// @author BB
    /// @details force chr_instance_update_vertices() recalculate the vertices the next time
    ///     the function is called

    this->save = vlst_cache_t();
    this->matrix_cache = matrix_cache_t();

    this->lighting_update_wld = -1;
    this->lighting_frame_all  = -1;
}

chr_instance_t::chr_instance_t(const Object &object) :
    matrix_cache(),

    facing_z(0),

    alpha(0xFF),
    light(0),
    sheen(0),

    colorshift(),

    uoffset(0),
    voffset(0),

    animationState(),
    actionState(),

    // lighting info
    color_amb(0),
    col_amb(),
    max_light(0),
    min_light(0),
    lighting_update_wld(-1),
    lighting_frame_all(-1),

    // linear interpolated frame vertices
    _object(object),
    _vertexList(),
    _matrix(Matrix4f4f::identity()),
    _reflectionMatrix(Matrix4f4f::identity()),

    // graphical optimizations
    save()
{
    // initalize the character instance
    setObjectProfile(_object.getProfile());
}

chr_instance_t::~chr_instance_t() 
{
    //dtor
}

const GLvertex& chr_instance_t::getVertex(const size_t index) const
{
    return _vertexList[index];
}

bool chr_instance_t::setModel(const std::shared_ptr<Ego::ModelDescriptor> &model)
{
    bool updated = false;

    if (this->animationState.getModelDescriptor() != model) {
        updated = true;
        this->animationState.setModelDescriptor(model);
    }

    // set the vertex size
    size_t vlst_size = this->animationState.getModelDescriptor()->getMD2()->getVertexCount();
    if (this->_vertexList.size() != vlst_size) {
        updated = true;
        this->_vertexList.resize(vlst_size);
    }

    // set the frames to frame 0 of this object's data
    if (0 != this->animationState.getTargetFrameIndex() || 0 != this->animationState.getSourceFrameIndex()) {
        updated = true;
        this->animationState.setSourceFrameIndex(0);
        this->animationState.setTargetFrameIndex(0);

        // the vlst_cache parameters are not valid
        this->save.valid = false;
    }

    if (updated) {
        // update the vertex and lighting cache
        clearCache();
        updateVertices(-1, -1, true);
    }

    return updated;
}

const Matrix4f4f& chr_instance_t::getMatrix() const
{
    return _matrix;
}

const Matrix4f4f& chr_instance_t::getReflectionMatrix() const
{
    return _reflectionMatrix;
}

uint8_t chr_instance_t::getReflectionAlpha() const
{
    // determine the reflection alpha based on altitude above the mesh
    const float altitudeAboveGround = std::max(0.0f, _object.getPosZ() - _object.getFloorElevation());
    float alphaFade = (255.0f - altitudeAboveGround)*0.5f;
    alphaFade = Ego::Math::constrain(alphaFade, 0.0f, 255.0f);

    return this->alpha * alphaFade * INV_FF<float>();
}

void chr_instance_t::setObjectProfile(const std::shared_ptr<ObjectProfile> &profile)
{
    //Reset data
    // Remember any previous color shifts in case of lasting enchantments
    _matrix = Matrix4f4f::identity();
    _reflectionMatrix = Matrix4f4f::identity();
    facing_z = 0;
    uoffset = 0;
    voffset = 0;
    animationState = AnimationState();
    actionState = ActionState();
    color_amb = 0;
    col_amb = Vector4f::zero();
    max_light = 0;
    min_light = 0;
    _vertexList.clear();
    clearCache();

    // lighting parameters
	this->alpha = profile->getAlpha();
	this->light = profile->getLight();
	this->sheen = profile->getSheen();

    // model parameters
    setModel(profile->getModel());

    // set the initial action, all actions override it
    playAction(ACTION_DA, true);
}

BIT_FIELD chr_instance_t::getFrameFX() const
{
    return getNextFrame().framefx;
}

gfx_rv chr_instance_t::setFrameFull(int frame_along, int ilip)
{
    // handle optional parameters
	const std::shared_ptr<Ego::ModelDescriptor> &imad = this->animationState.getModelDescriptor();

    // we have to have a valid action range
	if (this->actionState.action_which > ACTION_COUNT) {
		return gfx_fail;
	}

    // try to heal a bad action
    this->actionState.action_which = imad->getAction(this->actionState.action_which);

    // reject the action if it is cannot be made valid
	if (this->actionState.action_which == ACTION_COUNT) {
		return gfx_fail;
	}

    // get some frame info
    int frame_stt   = imad->getFirstFrame(this->actionState.action_which);
    int frame_end   = imad->getLastFrame(this->actionState.action_which);
    int frame_count = 1 + ( frame_end - frame_stt );

    // try to heal an out of range value
    frame_along %= frame_count;

    // get the next frames
    int new_nxt = frame_stt + frame_along;
    new_nxt = std::min(new_nxt, frame_end);

    this->animationState.setTargetFrameIndex(new_nxt);
    this->animationState.ilip      = ilip;
    this->animationState.flip      = ilip * 0.25f;

    // set the validity of the cache
	vlst_cache_t::test(this->save, this);

    return gfx_success;
}

void chr_instance_t::setActionKeep(bool val) {
	actionState.action_keep = val;
}

void chr_instance_t::setActionReady(bool val) {
    actionState.action_ready = val;
}

void chr_instance_t::setActionLooped(bool val) {
    actionState.action_loop = val;
}

void chr_instance_t::setNextAction(const ModelAction val) {
    actionState.action_next = val;
}

void chr_instance_t::removeInterpolation()
{
    if (this->animationState.getSourceFrameIndex() != this->animationState.getTargetFrameIndex() ) {
		this->animationState.setSourceFrameIndex(this->animationState.getTargetFrameIndex());
		this->animationState.ilip = 0;
		this->animationState.flip = 0.0f;

		vlst_cache_t::test(this->save, this);
    }
}

const MD2_Frame& chr_instance_t::getNextFrame() const
{
    return animationState.getTargetFrame();
}

const MD2_Frame& chr_instance_t::getLastFrame() const
{
    return animationState.getSourceFrame();
}

void chr_instance_t::update_one_lip(chr_instance_t& self) {
    self.animationState.ilip += 1;
    self.animationState.flip = 0.25f * self.animationState.ilip;

	vlst_cache_t::test(self.save, &self);
}

gfx_rv chr_instance_t::update_one_flip(chr_instance_t& self, float dflip)
{
	if (0.0f == dflip) {
		return gfx_fail;
	}

    // update the lips
    self.animationState.flip += dflip;
	self.animationState.ilip = ((int)std::floor(self.animationState.flip * 4)) % 4;

	vlst_cache_t::test(self.save, &self);

    return gfx_success;
}

float chr_instance_t::getRemainingFlip() const
{
	return (this->animationState.ilip + 1) * 0.25f - this->animationState.flip;
}

void chr_instance_t::getTint(GLXvector4f tint, const bool reflection, const int type)
{
	int local_alpha;
	int local_light;
	int local_sheen;
    colorshift_t local_colorshift;

	if (reflection)
	{
		// this is a reflection, use the reflected parameters
		local_alpha = getReflectionAlpha();

        if(this->light == 0xFF) {
            local_light = 0xFF;
        }
        else {
            local_light = this->light * local_alpha * INV_FF<float>();
        }

		local_sheen = this->sheen / 2; //half of normal sheen
        local_colorshift = colorshift_t(this->colorshift.red + 1, this->colorshift.green + 1, this->colorshift.blue + 1);
	}
	else
	{
		// this is NOT a reflection, use the normal parameters
		local_alpha = this->alpha;
		local_light = this->light;
		local_sheen = this->sheen;
        local_colorshift = this->colorshift;
	}

	// modify these values based on local character abilities
    if(local_stats.seeinvis_level > 0.0f) {
        local_alpha = std::max(local_alpha, SEEINVISIBLE);
    }
	local_light = get_light(local_light, local_stats.seedark_mag);

	// clear out the tint
    tint[RR] = 1.0f / (1 << local_colorshift.red);
    tint[GG] = 1.0f / (1 << local_colorshift.green);
    tint[BB] = 1.0f / (1 << local_colorshift.blue);
    tint[AA] = 1.0f;

    switch(type)
    {
        case CHR_LIGHT:
        case CHR_ALPHA:
            // alpha characters are blended onto the canvas using the alpha channel
            tint[AA] = local_alpha * INV_FF<float>();

            // alpha characters are blended onto the canvas by adding their color
            // the more black the colors, the less visible the character
            // the alpha channel is not important
            tint[RR] = local_light * INV_FF<float>() / (1 << local_colorshift.red);
            tint[GG] = local_light * INV_FF<float>() / (1 << local_colorshift.green);
            tint[BB] = local_light * INV_FF<float>() / (1 << local_colorshift.blue);
        break;

        case CHR_PHONG:
            // phong is essentially the same as light, but it is the
            // sheen that sets the effect
            float amount = (Ego::Math::constrain(local_sheen, 0, 15) << 4) / 240.0f;

            tint[RR] += tint[RR] * 0.5f + amount;
            tint[GG] += tint[GG] * 0.5f + amount;
            tint[BB] += tint[BB] * 0.5f + amount;

            tint[RR] /= 2.0f;
            tint[GG] /= 2.0f;
            tint[BB] /= 2.0f;
        break;
    }

}


gfx_rv vlst_cache_t::test(vlst_cache_t& self, chr_instance_t *instance)
{
	if (!self.valid) {
		return gfx_success;
	}

    if (!instance) {
        self.valid = false;
        return gfx_success;
    }

    if (instance->animationState.getSourceFrameIndex() != self.frame_nxt) {
        self.valid = false;
    }

    if (instance->animationState.getSourceFrameIndex() != self.frame_lst) {
        self.valid = false;
    }

    if ((instance->animationState.getSourceFrameIndex() != self.frame_lst)  && std::abs(instance->animationState.flip - self.flip) > FLIP_TOLERANCE) {
        self.valid = false;
    }

    return gfx_success;
}

void chr_instance_t::flash(uint8_t value)
{
	const float flash_val = value * INV_FF<float>();

	// flash the ambient color
	this->color_amb = flash_val;

	// flash the directional lighting
	for (size_t i = 0; i < _vertexList.size(); ++i) {
		_vertexList[i].color_dir = flash_val;
	}
}


size_t chr_instance_t::getVertexCount() const
{
    return _vertexList.size();
}

void chr_instance_t::flashVariableHeight(const uint8_t valuelow, const int16_t low, const uint8_t valuehigh, const int16_t high)
{
    for (size_t cnt = 0; cnt < _vertexList.size(); cnt++)
    {
        int16_t z = _vertexList[cnt].pos[ZZ];

        if ( z < low )
        {
            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] = valuelow;
        }
        else if ( z > high )
        {
            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] = valuehigh;
        }
        else if ( high != low )
        {
            uint8_t valuemid = ( valuehigh * ( z - low ) / ( high - low ) ) +
                             ( valuelow * ( high - z ) / ( high - low ) );

            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] =  valuemid;
        }
        else
        {
            // z == high == low
            uint8_t valuemid = ( valuehigh + valuelow ) * 0.5f;

            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] =  valuemid;
        }
    }
}

void chr_instance_t::setMatrix(const Matrix4f4f& matrix)
{
    //Set the normal model matrix
    _matrix = matrix;

    //Compute the reflected matrix as well
    _reflectionMatrix = matrix;
    _reflectionMatrix(2, 0) = -_reflectionMatrix(0, 2);
    _reflectionMatrix(2, 1) = -_reflectionMatrix(1, 2);
    _reflectionMatrix(2, 2) = -_reflectionMatrix(2, 2);
    _reflectionMatrix(2, 3) = 2.0f * _object.getFloorElevation() - _object.getPosZ();
}
