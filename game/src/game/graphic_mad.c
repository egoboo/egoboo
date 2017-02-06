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

#include "game/graphic_mad.h"

#include "game/renderer_3d.h"
#include "game/lighting.h"
#include "game/graphic.h"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Entities/_Include.hpp"

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
    std::vector<Md2Vertex> vertices;
    Md2VertexBuffer(size_t size) : vertices() {
        vertices.resize(size);
        // Intentionally empty.
    }
    Md2VertexBuffer() : Md2VertexBuffer(0) {
        // Intentionally empty.
    }
    ~Md2VertexBuffer() {
        //dtor
    }

    void ensureSize(size_t requiredSize) {
        if (requiredSize > vertices.size()) {
            vertices.resize(requiredSize);
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

gfx_rv ObjectGraphicsRenderer::render_enviro( Camera& cam, const std::shared_ptr<Object>& pchr, GLXvector4f tint, const BIT_FIELD bits )
{
    if (!pchr->inst.getModelDescriptor())
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid mad `"<< pchr->getObjRef() << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }
    const auto& pmd2 = pchr->getProfile()->getModel()->getMD2();
    auto& renderer = Ego::Renderer::get();

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

    float uoffset = pchr->inst.uoffset - float(cam.getTurnZ_turns());

	if (HAS_SOME_BITS(bits, CHR_REFLECT))
	{
        renderer.setWorldMatrix(pchr->inst.getReflectionMatrix());
	}
	else
	{
		renderer.setWorldMatrix(pchr->inst.getMatrix());
	}

    // Choose texture and matrix
	renderer.getTextureUnit().setActivated(ptex.get());

    {
        Ego::OpenGL::PushAttrib pa(GL_CURRENT_BIT);
        {
            // Get the maximum number of vertices per command.
            size_t vertexBufferCapacity = 0;
            for (const auto& glcommand : pmd2->getGLCommands()) {
                vertexBufferCapacity = std::max(vertexBufferCapacity, glcommand.data.size());
            }
            // Allocate a vertex buffer.
            Md2VertexBuffer vertexBuffer(vertexBufferCapacity);
            // Render each command
            for (const auto& glcommand : pmd2->getGLCommands()) {
                // Pre-render this command.
                size_t vertexBufferSize = 0;
                for (const id_glcmd_packed_t& cmd : glcommand.data) {
                    uint16_t vertexIndex = cmd.index;
                    if (vertexIndex >= pchr->inst.getVertexCount()) continue;
                    const GLvertex& pvrt = pchr->inst.getVertex(vertexIndex);
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
if(fogon && pchr->inst->light==255)
{
    // The full fog value
    alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

    for (cnt = 0; cnt < pmad->transvertices; cnt++)
    {
        // Figure out the z position of the vertex...  Not totally accurate
        z = (pchr->inst->_vertexList[cnt].pos[ZZ]) + pchr->matrix(3,2);

        // Figure out the fog coloring
        if(z < fogtop)
        {
            if(z < fogbottom)
            {
                pchr->inst->_vertexList[cnt].specular = alpha;
            }
            else
            {
                z = 1.0f - ((z - fogbottom)/fogdistance);  // 0.0f to 1.0f...  Amount of fog to keep
                red = fogred * z;
                grn = foggrn * z;
                blu = fogblu * z;
                fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                pchr->inst->_vertexList[cnt].specular = fogspec;
            }
        }
        else
        {
            pchr->inst->_vertexList[cnt].specular = 0;
        }
    }
}

else
{
    for (cnt = 0; cnt < pmad->transvertices; cnt++)
        pchr->inst->_vertexList[cnt].specular = 0;
}

*/

//--------------------------------------------------------------------------------------------
gfx_rv ObjectGraphicsRenderer::render_tex(Camera& camera, const std::shared_ptr<Object>& pchr, GLXvector4f tint, const BIT_FIELD bits)
{
    if (!pchr->inst.getModelDescriptor())
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid mad `" << pchr->getObjRef() << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    const std::shared_ptr<MD2Model> &pmd2 = pchr->getProfile()->getModel()->getMD2();

    // To make life easier
    std::shared_ptr<const Ego::Texture> ptex = pchr->getSkinTexture();

    float uoffset = pchr->inst.uoffset * INV_FFFF<float>();
    float voffset = pchr->inst.voffset * INV_FFFF<float>();

    float base_amb = 0.0f;
    if (0 == (bits & CHR_LIGHT))
    {
        // Convert the "light" parameter to self-lighting for
        // every object that is not being rendered using CHR_LIGHT.
        base_amb = (0xFF == pchr->inst.light) ? 0 : (pchr->inst.light * INV_FF<float>());
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
        Ego::Renderer::get().setWorldMatrix(pchr->inst.getReflectionMatrix());
    }
    else
    {
        Ego::Renderer::get().setWorldMatrix(pchr->inst.getMatrix());
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
                    if (vertexIndex >= pchr->inst.getVertexCount()) {
                        continue;
                    }
                    const GLvertex& pvrt = pchr->inst.getVertex(vertexIndex);
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

                            float acol = base_amb + pchr->inst.getAmbientColour() * INV_FF<float>();

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
    if(fogon && pchr->inst->light==255)
    {
        // The full fog value
*        alpha = 0xff000000 | (fogred<<16) | (foggrn<<8) | (fogblu);

        for (cnt = 0; cnt < pmad->transvertices; cnt++)
        {
            // Figure out the z position of the vertex...  Not totally accurate
            z = (pchr->inst->_vertexList[cnt].pos[ZZ]) + pchr->matrix(3,2);

            // Figure out the fog coloring
            if(z < fogtop)
            {
                if(z < fogbottom)
                {
                    pchr->inst->_vertexList[cnt].specular = alpha;
                }
                else
                {
                    spek = pchr->inst->_vertexList[cnt].specular & 255;
                    z = (z - fogbottom)/fogdistance;  // 0.0f to 1.0f...  Amount of old to keep
                    fogtokeep = 1.0f-z;  // 0.0f to 1.0f...  Amount of fog to keep
                    spek = spek * z;
                    red = (fogred * fogtokeep) + spek;
                    grn = (foggrn * fogtokeep) + spek;
                    blu = (fogblu * fogtokeep) + spek;
                    fogspec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                    pchr->inst->_vertexList[cnt].specular = fogspec;
                }
            }
        }
    }
*/

gfx_rv ObjectGraphicsRenderer::render(Camera& cam, const std::shared_ptr<Object> &pchr, GLXvector4f tint, const BIT_FIELD bits)
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
gfx_rv ObjectGraphicsRenderer::render_ref( Camera& cam, const std::shared_ptr<Object>& pchr)
{
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
            if (pchr->inst.getReflectionAlpha() != 0xFF && pchr->inst.light == 0xFF) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

                GLXvector4f tint;
                pchr->inst.getTint(tint, true, CHR_ALPHA);

                if (gfx_error == render(cam, pchr, tint, CHR_ALPHA | CHR_REFLECT)) {
                    retval = gfx_error;
                }
            }

            //Glowing
            if (pchr->inst.light != 0xFF) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pchr->inst.getTint(tint, true, CHR_LIGHT);

                if (gfx_error == ObjectGraphicsRenderer::render(cam, pchr, tint, CHR_LIGHT)) {
                    retval = gfx_error;
                }
                Ego::OpenGL::Utilities::isError();
            }

            //Render shining effect on top of model
            if (pchr->inst.getReflectionAlpha() == 0xFF && gfx.phongon && pchr->inst.sheen > 0) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pchr->inst.getTint(tint, true, CHR_PHONG);

                if (gfx_error == ObjectGraphicsRenderer::render(cam, pchr, tint, CHR_PHONG)) {
                    retval = gfx_error;
                }
                Ego::OpenGL::Utilities::isError();
            }
        }
    }

    return retval;
}

gfx_rv ObjectGraphicsRenderer::render_trans(Camera& cam, const std::shared_ptr<Object>& pchr)
{
    if ( pchr->isHidden() ) return gfx_fail;

    // there is an outside chance the object will not be rendered
    bool rendered = false;

    {
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
        {
            auto& renderer = Ego::Renderer::get();

            if (pchr->inst.alpha < 0xFF) {
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
                pchr->inst.getTint(tint, false, CHR_ALPHA);

                if (render(cam, pchr, tint, CHR_ALPHA)) {
                    rendered = true;
                }
            }

            else if (pchr->inst.light < 0xFF) {
                // light effects should show through transparent objects
                renderer.setCullingMode(Ego::CullingMode::None);

                // the alpha test can only mess us up here
                renderer.setAlphaTestEnabled(false);

                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pchr->inst.getTint(tint, false, CHR_LIGHT);

                if (render(cam, pchr, tint, CHR_LIGHT)) {
                    rendered = true;
                }
            }

            // Render shining effect on top of model
            if (pchr->inst.getReflectionAlpha() == 0xFF && gfx.phongon && pchr->inst.sheen > 0) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                pchr->inst.getTint(tint, false, CHR_PHONG);

                if (render(cam, pchr, tint, CHR_PHONG)) {
                    rendered = true;
                }
            }
        }
    }

    return rendered ? gfx_success : gfx_fail;
}

gfx_rv ObjectGraphicsRenderer::render_solid( Camera& cam, const std::shared_ptr<Object> &pchr )
{
    if ( pchr->isHidden() ) return gfx_fail;

    //Only proceed if we are truly fully solid
    if (0xFF != pchr->inst.alpha || 0xFF != pchr->inst.light) {
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
            pchr->inst.getTint(tint, false, CHR_SOLID);

            if (gfx_error == render(cam, pchr, tint, CHR_SOLID)) {
                retval = gfx_error;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
#if _DEBUG
void ObjectGraphicsRenderer::draw_chr_bbox(const std::shared_ptr<Object>& pchr)
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
void ObjectGraphicsRenderer::draw_chr_verts(const std::shared_ptr<Object>& pchr, int vrt_offset, int verts )
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
void ObjectGraphicsRenderer::draw_one_grip( Ego::Graphics::ObjectGraphics *pinst, int slot )
{
    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    Ego::Renderer::get().getTextureUnit().setActivated(nullptr);

    Ego::Renderer::get().setWorldMatrix(pinst->getMatrix());

    _draw_one_grip_raw( pinst, slot );
}

void ObjectGraphicsRenderer::_draw_one_grip_raw( Ego::Graphics::ObjectGraphics *pinst, int slot )
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

void ObjectGraphicsRenderer::draw_chr_attached_grip(const std::shared_ptr<Object>& pchr)
{
    const auto& pholder = pchr->getHolder();
    if (!pholder || pholder->isTerminated()) return;

    draw_one_grip( &( pholder->inst ), pchr->inwhich_slot );
}
#endif

#if 0
void ObjectGraphicsRenderer::draw_chr_grips( Object * pchr )
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
