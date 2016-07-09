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

int cmp_matrix_cache(const matrix_cache_t& lhs, const matrix_cache_t& rhs) {
    // handle problems with pointers
    if (&lhs == &rhs) {
        return 0;
    }

    // handle one of both if the matrix caches being invalid
    if (!lhs.valid && !rhs.valid) {
        return 0;
    } else if (!lhs.valid) {
        return 1;
    } else if (!rhs.valid) {
        return -1;
    }

    // handle differences in the type
    int itmp = lhs.type_bits - rhs.type_bits;
    if (0 != itmp) goto cmp_matrix_cache_end;

    //---- check for differences in the MAT_WEAPON data
    if (HAS_SOME_BITS(lhs.type_bits, MAT_WEAPON)) {
        itmp = (signed)REF_TO_INT(lhs.grip_chr.get()) - (signed)REF_TO_INT(rhs.grip_chr.get());
        if (0 != itmp) goto cmp_matrix_cache_end;

        itmp = (signed)lhs.grip_slot - (signed)rhs.grip_slot;
        if (0 != itmp) goto cmp_matrix_cache_end;

        for (int cnt = 0; cnt < GRIP_VERTS; cnt++) {
            itmp = (signed)lhs.grip_verts[cnt] - (signed)rhs.grip_verts[cnt];
            if (0 != itmp) goto cmp_matrix_cache_end;
        }

        // handle differences in the scale of our mount
        for (int cnt = 0; cnt < 3; cnt++) {
            float ftmp = lhs.grip_scale[cnt] - rhs.grip_scale[cnt];
            if (0.0f != ftmp) { itmp = sgn(ftmp); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the MAT_CHARACTER data
    if (HAS_SOME_BITS(lhs.type_bits, MAT_CHARACTER)) {
        // handle differences in the "Euler" rotation angles in 16-bit form
        for (int cnt = 0; cnt < 3; cnt++) {
            Facing ftmp = lhs.rotate[cnt] - rhs.rotate[cnt];
            if (Facing(0) != ftmp) { itmp = sgn(ftmp); goto cmp_matrix_cache_end; }
        }

        // handle differences in the translate vector
        for (int cnt = 0; cnt < 3; cnt++) {
            float ftmp = lhs.pos[cnt] - rhs.pos[cnt];
            if (0.0f != ftmp) { itmp = sgn(ftmp); goto cmp_matrix_cache_end; }
        }
    }

    //---- check for differences in the shared data
    if (HAS_SOME_BITS(lhs.type_bits, MAT_WEAPON) || HAS_SOME_BITS(lhs.type_bits, MAT_CHARACTER)) {
        // handle differences in our own scale
        for (int cnt = 0; cnt < 3; cnt++) {
            float ftmp = lhs.self_scale[cnt] - rhs.self_scale[cnt];
            if (0.0f != ftmp) { itmp = sgn(ftmp); goto cmp_matrix_cache_end; }
        }
    }

    // if it got here, the data is all the same
    itmp = 0;

cmp_matrix_cache_end:

    return sgn(itmp);
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
        Ego::Renderer::get().setWorldMatrix(pinst.ref.matrix);
	}
	else
	{
		Ego::Renderer::get().setWorldMatrix(pinst.matrix);
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
                    if (vertexIndex >= pinst.vrt_count) continue;
                    const GLvertex& pvrt = pinst.vrt_lst[vertexIndex];
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
        base_amb = (255 == pinst.light) ? 0 : (pinst.light * INV_FF<float>());
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
        Ego::Renderer::get().setWorldMatrix(pinst.ref.matrix);
    }
    else
    {
        Ego::Renderer::get().setWorldMatrix(pinst.matrix);
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
                    if (vertexIndex >= pinst.vrt_count) {
                        continue;
                    }
                    const GLvertex& pvrt = pinst.vrt_lst[vertexIndex];
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

gfx_rv MadRenderer::render(Camera& cam, const std::shared_ptr<Object> &pchr, GLXvector4f tint, const BIT_FIELD bits)
{
    /// @author ZZ
    /// @details This function picks the actual function to use

    gfx_rv retval;

    //Not visible?
    if (pchr->isHidden() || tint[AA] <= 0.0f || pchr->isInsideInventory()) {
        return gfx_fail;
    }

    if ( pchr->inst.enviro || HAS_SOME_BITS(bits, CHR_PHONG) )
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

    if (!pinst.ref.matrix_valid)
    {
        return gfx_error;
    }

    {
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);
        {
            auto& renderer = Ego::Renderer::get();
            // cull backward facing polygons
            // use couter-clockwise orientation to determine backfaces
            oglx_begin_culling(Ego::CullingMode::Back, MAD_REF_CULL);
            Ego::OpenGL::Utilities::isError();
            if (pinst.ref.alpha != 255 && pinst.ref.light == 255) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);
                GLXvector4f tint;
                chr_instance_t::get_tint(pinst, tint, CHR_ALPHA | CHR_REFLECT);

                // the previous call to chr_instance_update_lighting_ref() has actually set the
                // alpha and light for all vertices
                if (gfx_error == render(cam, pchr, tint, CHR_ALPHA | CHR_REFLECT)) {
                    retval = gfx_error;
                }
            }

            if (pinst.ref.light != 255) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);
                GLXvector4f tint;
                chr_instance_t::get_tint(pinst, tint, CHR_LIGHT | CHR_REFLECT);

                // the previous call to chr_instance_update_lighting_ref() has actually set the
                // alpha and light for all vertices
                if (gfx_error == MadRenderer::render(cam, pchr, tint, CHR_LIGHT | CHR_REFLECT)) {
                    retval = gfx_error;
                }
                Ego::OpenGL::Utilities::isError();
            }

            //Render shining effect on top of model
            if (pinst.ref.alpha == 0xFF && gfx.phongon && pinst.sheen > 0) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);
                GLXvector4f tint;
                chr_instance_t::get_tint(pinst, tint, CHR_PHONG | CHR_REFLECT);

                if (gfx_error == MadRenderer::render(cam, pchr, tint, CHR_PHONG | CHR_REFLECT)) {
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
            if (pinst.alpha < 255 && 255 == pinst.light) {
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
                chr_instance_t::get_tint(pinst, tint, CHR_ALPHA);

                if (render(cam, pchr, tint, CHR_ALPHA)) {
                    rendered = true;
                }
            }
            if (pinst.light < 255) {
                // light effects should show through transparent objects
                renderer.setCullingMode(Ego::CullingMode::None);

                // the alpha test can only mess us up here
                renderer.setAlphaTestEnabled(false);

                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                chr_instance_t::get_tint(pinst, tint, CHR_LIGHT);

                if (render(cam, pchr, tint, CHR_LIGHT)) {
                    rendered = true;
                }
            }

            // Render shining effect on top of model
            if (pinst.ref.alpha == 0xFF && gfx.phongon && pinst.sheen > 0) {
                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                GLXvector4f tint;
                chr_instance_t::get_tint(pinst, tint, CHR_PHONG);

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

            if (255 == pinst.alpha && 255 == pinst.light) {
                GLXvector4f tint;

                // allow the dont_cull_backfaces to keep solid objects from culling backfaces
                if (pinst.dont_cull_backfaces) {
                    // stop culling backward facing polugons
                    renderer.setCullingMode(Ego::CullingMode::None);
                } else {
                    // cull backward facing polygons
                    // use couter-clockwise orientation to determine backfaces
                    oglx_begin_culling(Ego::CullingMode::Back, MAD_NRM_CULL);            // GL_ENABLE_BIT | GL_POLYGON_BIT
                }

                chr_instance_t::get_tint(pinst, tint, CHR_SOLID);

                if (gfx_error == render(cam, pchr, tint, CHR_SOLID)) {
                    retval = gfx_error;
                }
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
    
    auto& inputSystem = InputSystem::get();
    // Draw the object bounding box as a part of the graphics debug mode F7.
    if (egoboo_config_t::get().debug_developerMode_enable.getValue() && inputSystem.keyboard.isKeyDown(SDLK_F7))
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
    if (InputSystem::get().keyboard.isKeyDown(SDLK_F6))
    {
        draw_chr_attached_grip( pchr );

        // Draw all the vertices of an object
        GL_DEBUG(glPointSize(5));
        draw_chr_verts(pchr, 0, pchr->inst.vrt_count);
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

    if ( vmin < 0 || ( size_t )vmin > pchr->inst.vrt_count ) return;
    if ( vmax < 0 || ( size_t )vmax > pchr->inst.vrt_count ) return;

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    Ego::Renderer::get().getTextureUnit().setActivated(nullptr);

	Ego::Renderer::get().setWorldMatrix(pchr->inst.matrix);
    GL_DEBUG( glBegin( GL_POINTS ) );
    {
        for ( cnt = vmin; cnt < vmax; cnt++ )
        {
            GL_DEBUG( glVertex3fv )( pchr->inst.vrt_lst[cnt].pos );
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

    Ego::Renderer::get().setWorldMatrix(pinst->matrix);

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

    vmin = ( int )pinst->vrt_count - ( int )slot_to_grip_offset(( slot_t )slot );
    vmax = vmin + GRIP_VERTS;

    if ( vmin >= 0 && vmax >= 0 && ( size_t )vmax <= pinst->vrt_count )
    {
		Vector3f src, dst, diff;

        GL_DEBUG( glBegin )( GL_LINES );
        {
            for ( cnt = 1; cnt < GRIP_VERTS; cnt++ )
            {
                src[kX] = pinst->vrt_lst[vmin].pos[XX];
                src[kY] = pinst->vrt_lst[vmin].pos[YY];
                src[kZ] = pinst->vrt_lst[vmin].pos[ZZ];

                diff[kX] = pinst->vrt_lst[vmin+cnt].pos[XX] - src[kX];
                diff[kY] = pinst->vrt_lst[vmin+cnt].pos[YY] - src[kY];
                diff[kZ] = pinst->vrt_lst[vmin+cnt].pos[ZZ] - src[kZ];

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
    if (self.lighting_frame_all < 0 || self.lighting_frame_all < 0) {
        force = true;
    }

    // has this already been calculated this update?
	if (!force && self.lighting_update_wld >= 0 && (Uint32)self.lighting_update_wld >= update_wld) {
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
    self.vrt_count = self.vrt_count;

    // interpolate the lighting for the origin of the object

	auto mesh = _currentModule->getMeshPointer();
	if (!mesh) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "nullptr == mesh");
	}

	lighting_cache_t global_light;
    GridIllumination::grid_lighting_interpolate(*mesh, global_light, Vector2f(pchr->getPosX(), pchr->getPosY()));

    // rotate the lighting data to body_centered coordinates
	lighting_cache_t loc_light;
	lighting_cache_t::lighting_project_cache(loc_light, global_light, self.matrix);

    //Low-pass filter to smooth lighting transitions?
    //self.color_amb = 0.9f * self.color_amb + 0.1f * (loc_light.hgh._lighting[LVEC_AMB] + loc_light.low._lighting[LVEC_AMB]) * 0.5f;
    //self.color_amb = (loc_light.hgh._lighting[LVEC_AMB] + loc_light.low._lighting[LVEC_AMB]) * 0.5f;
    self.color_amb = get_ambient_level();

    self.max_light = -255;
    self.min_light =  255;
    for (size_t cnt = 0; cnt < self.vrt_count; cnt++ )
    {
        float lite = 0.0f;

        GLvertex *pvert = self.vrt_lst + cnt;

        // a simple "height" measurement
        float hgt = pvert->pos[ZZ] * self.matrix( 3, 3 ) + self.matrix( 3, 3 );

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

gfx_rv chr_instance_t::update_bbox(chr_instance_t& self)
{
    // get the model. try to heal a bad model.
    if (!self.animationState.getModelDescriptor()) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
        return gfx_error;
    }

    const MD2_Frame &lastFrame = chr_instance_t::get_frame_lst(self);
    const MD2_Frame &nextFrame = chr_instance_t::get_frame_nxt(self);

    if (self.animationState.frame_nxt == self.animationState.frame_lst || self.animationState.flip == 0.0f) {
        self.bbox = lastFrame.bb;
    } else if (self.animationState.flip == 1.0f) {
        self.bbox = nextFrame.bb;
    } else {
        self.bbox = oct_bb_t::interpolate(lastFrame.bb, nextFrame.bb, self.animationState.flip);
    }

    return gfx_success;
}

gfx_rv chr_instance_t::needs_update(chr_instance_t& self, int vmin, int vmax, bool *verts_match, bool *frames_match)
{
    /// @author BB
    /// @details determine whether some specific vertices of an instance need to be updated
    ///                gfx_error   means that the function was passed invalid values
    ///                gfx_fail    means that the instance does not need to be updated
    ///                gfx_success means that the instance should be updated

	bool local_verts_match, local_frames_match;

    // ensure that the pointers point to something
    if ( NULL == verts_match ) verts_match  = &local_verts_match;
    if ( NULL == frames_match ) frames_match = &local_frames_match;

    // initialize the boolean pointers
    *verts_match  = false;
    *frames_match = false;

	vlst_cache_t *psave = &(self.save);

    // do we hace a valid mad?
    if (!self.animationState.getModelDescriptor()) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
        return gfx_error;
    }

    // check to see if the vlst_cache has been marked as invalid.
    // in this case, everything needs to be updated
	if (!psave->valid) {
		return gfx_success;
	}

    // get the last valid vertex from the chr_instance
    int maxvert = ((int)self.vrt_count) - 1;

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

	bool flips_match = (std::abs(psave->flip - self.animationState.flip) < FLIP_TOLERANCE);

    *frames_match = (self.animationState.frame_nxt == self.animationState.frame_lst && psave->frame_nxt == self.animationState.frame_nxt && psave->frame_lst == self.animationState.frame_lst ) ||
                    (flips_match && psave->frame_nxt == self.animationState.frame_nxt && psave->frame_lst == self.animationState.frame_lst);

    return (!(*verts_match) || !( *frames_match )) ? gfx_success : gfx_fail;
}

void chr_instance_t::interpolate_vertices_raw( GLvertex dst_ary[], const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip )
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

gfx_rv chr_instance_t::update_vertices(chr_instance_t& self, int vmin, int vmax, bool force)
{
	int maxvert;
    bool vertices_match, frames_match;
    float  loc_flip;

    gfx_rv retval;

    vlst_cache_t * psave;

    int vdirty1_min = -1, vdirty1_max = -1;
    int vdirty2_min = -1, vdirty2_max = -1;

    psave = &( self.save );

    if ( gfx_error == chr_instance_t::update_bbox( self ) )
    {
        return gfx_error;
    }

    // get the model
    if ( !self.animationState.getModelDescriptor() )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid mad" );
        return gfx_error;
    }
    std::shared_ptr<MD2Model> pmd2 = self.animationState.getModelDescriptor()->getMD2();

    // make sure we have valid data
    if (self.vrt_count != pmd2->getVertexCount())
    {
		Log::get().warn( "chr_instance_update_vertices() - character instance vertex data does not match its md2\n" );
        return gfx_error;
    }

    // get the vertex list size from the chr_instance
    maxvert = (( int )self.vrt_count ) - 1;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = maxvert;

    // are they in the right order?
    if ( vmax < vmin ) std::swap(vmax, vmin);

    // make sure that the vertices are within the max range
    vmin = Ego::Math::constrain( vmin, 0, maxvert );
    vmax = Ego::Math::constrain( vmax, 0, maxvert );

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
        retval = chr_instance_t::needs_update( self, vmin, vmax, &vertices_match, &frames_match );
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
    if ( self.animationState.frame_nxt >= frameList.size() || self.animationState.frame_lst >= frameList.size() )
    {
		Log::get().warn("%s:%d:%s: character instance frame is outside the range of its MD2\n", __FILE__, __LINE__, __FUNCTION__ );
        return gfx_error;
    }

    // grab the frame data from the correct model
    const MD2_Frame &nextFrame = frameList[self.animationState.frame_nxt];
    const MD2_Frame &lastFrame = frameList[self.animationState.frame_lst];

    // fix the flip for objects that are not animating
    loc_flip = self.animationState.flip;
    if ( self.animationState.frame_nxt == self.animationState.frame_lst ) loc_flip = 0.0f;

    // interpolate the 1st dirty region
    if ( vdirty1_min >= 0 && vdirty1_max >= 0 )
    {
		chr_instance_t::interpolate_vertices_raw(self.vrt_lst, lastFrame.vertexList, nextFrame.vertexList, vdirty1_min, vdirty1_max, loc_flip);
    }

    // interpolate the 2nd dirty region
    if ( vdirty2_min >= 0 && vdirty2_max >= 0 )
    {
		chr_instance_t::interpolate_vertices_raw(self.vrt_lst, lastFrame.vertexList, nextFrame.vertexList, vdirty2_min, vdirty2_max, loc_flip);
    }

    // update the saved parameters
    return chr_instance_t::update_vlst_cache(self, vmax, vmin, force, vertices_match, frames_match);
}

gfx_rv chr_instance_t::update_vlst_cache(chr_instance_t& self, int vmax, int vmin, bool force, bool vertices_match, bool frames_match)
{
    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // pinst->save.* values be tested and stored properly

	int maxvert = ((int)self.vrt_count) - 1;
	vlst_cache_t *psave = &(self.save);

    // the save_vmin and save_vmax is the most complex
    bool verts_updated = false;
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

    psave->frame_nxt = self.animationState.frame_nxt;
    psave->frame_lst = self.animationState.frame_lst;
    psave->flip      = self.animationState.flip;

    // store the last time there was an update to the animation
    bool frames_updated = false;
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

gfx_rv chr_instance_t::update_grip_verts(chr_instance_t& self, Uint16 vrt_lst[], size_t vrt_count )
{
    int vmin, vmax;
    Uint32 cnt;
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

        vmin = std::min<Uint16>( vmin, vrt_lst[cnt] );
        vmax = std::max<Uint16>( vmax, vrt_lst[cnt] );
        count++;
    }

    // if there are no valid points, there is nothing to do
    if ( 0 == count ) return gfx_fail;

    // force the vertices to update
    retval = chr_instance_t::update_vertices(self, vmin, vmax, true);

    return retval;
}

gfx_rv chr_instance_t::set_action(chr_instance_t& self, int action, bool action_ready, bool override_action)
{
	if (action < 0 || action > ACTION_COUNT) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, action, "invalid action range");
        return gfx_error;
    }

    // do we have a valid model?
	if (!self.animationState.getModelDescriptor()) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
        return gfx_error;
    }

    // is the chosen action valid?
	if (!self.animationState.getModelDescriptor()->isActionValid(action)) {
		return gfx_fail;
	}

    // are we going to check action_ready?
	if (!override_action && !self.action_ready) {
		return gfx_fail;
	}

    // save the old action
	int action_old = self.action_which;

    // set up the action
	self.action_which = action;
	self.action_next = ACTION_DA;
	self.action_ready = action_ready;

    // invalidate the vertex list if the action has changed
	if (action_old != action) {
		self.save.valid = false;
    }

    return gfx_success;
}

gfx_rv chr_instance_t::set_frame(chr_instance_t& self, int frame)
{
    if (self.action_which < 0 || self.action_which > ACTION_COUNT) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, self.action_which, "invalid action range");
        return gfx_error;
    }

    // do we have a valid model?
    if (!self.animationState.getModelDescriptor()) {
		gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
        return gfx_error;
    }

    // is the frame within the valid range for this action?
    if(!self.animationState.getModelDescriptor()->isFrameValid(self.action_which, frame)) return gfx_fail;

    // jump to the next frame
	self.animationState.flip = 0.0f;
	self.animationState.ilip = 0;
	self.animationState.frame_lst = self.animationState.frame_nxt;
	self.animationState.frame_nxt = frame;

	vlst_cache_t::test(self.save, &self);

    return gfx_success;
}

gfx_rv chr_instance_t::set_anim(chr_instance_t& self, int action, int frame, bool action_ready, bool override_action)
{
    gfx_rv retval = chr_instance_t::set_action(self, action, action_ready, override_action);
	if (gfx_success != retval) {
		return retval;
	}
    retval = chr_instance_t::set_frame(self, frame);
    return retval;
}

gfx_rv chr_instance_t::start_anim(chr_instance_t& self, int action, bool action_ready, bool override_action)
{
    if (action < 0 || action >= ACTION_COUNT) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, action, "invalid action range");
        return gfx_error;
    }
    if (!self.animationState.getModelDescriptor()) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
        return gfx_error;
    }
    return chr_instance_t::set_anim(self, action, self.animationState.getModelDescriptor()->getFirstFrame(action), action_ready, override_action);
}

gfx_rv chr_instance_t::increment_action(chr_instance_t& self)
{
    /// @author BB
    /// @details This function starts the next action for a character

	if (!self.animationState.getModelDescriptor()) {
		gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
        return gfx_error;
    }

    // get the correct action
	int action = self.animationState.getModelDescriptor()->getAction(self.action_next);

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    bool action_ready = ACTION_IS_TYPE( action, D ) || ACTION_IS_TYPE( action, W );

	return chr_instance_t::start_anim(self, action, action_ready, true);
}

gfx_rv chr_instance_t::increment_frame(chr_instance_t& self, const ObjectRef imount, const int mount_action)
{
    /// @author BB
    /// @details all the code necessary to move on to the next frame of the animation

    int frame_lst, frame_nxt;

    if ( !self.animationState.getModelDescriptor())
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mad" );
        return gfx_error;
    }

    // fix the ilip and flip
	self.animationState.ilip = self.animationState.ilip % 4;
	self.animationState.flip = fmod(self.animationState.flip, 1.0f);

    // Change frames
	frame_lst = self.animationState.frame_nxt;
	frame_nxt = self.animationState.frame_nxt + 1;

    // detect the end of the animation and handle special end conditions
	if (frame_nxt > self.animationState.getModelDescriptor()->getLastFrame(self.action_which))
    {
		if (self.action_keep)
        {
            // Freeze that animation at the last frame
            frame_nxt = frame_lst;

            // Break a kept action at any time
			self.action_ready = true;
        }
		else if (self.action_loop)
        {
            // Convert the action into a riding action if the character is mounted
            if (_currentModule->getObjectHandler().exists(imount))
            {
				chr_instance_t::start_anim(self, mount_action, true, true);
            }

            // set the frame to the beginning of the action
			frame_nxt = self.animationState.getModelDescriptor()->getFirstFrame(self.action_which);

            // Break a looped action at any time
			self.action_ready = true;
        }
        else
        {
            // Go on to the next action. don't let just anything interrupt it?
			chr_instance_t::increment_action(self);

            // chr_instance_increment_action() actually sets this value properly. just grab the new value.
			frame_nxt = self.animationState.frame_nxt;
        }
    }

	self.animationState.frame_lst = frame_lst;
	self.animationState.frame_nxt = frame_nxt;

	vlst_cache_t::test(self.save, &self);

    return gfx_success;
}

gfx_rv chr_instance_t::play_action(chr_instance_t& self, int action, bool action_ready)
{
    /// @author ZZ
    /// @details This function starts a generic action for a character
    if (!self.animationState.getModelDescriptor()) {
		gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
		return gfx_error;
	}

    return chr_instance_t::start_anim(self, self.animationState.getModelDescriptor()->getAction(action), action_ready, true);
}

void chr_instance_t::clear_cache(chr_instance_t& self)
{
    /// @author BB
    /// @details force chr_instance_update_vertices() recalculate the vertices the next time
    ///     the function is called

    self.save = vlst_cache_t();
    self.matrix_cache = matrix_cache_t();
    self.ref = chr_reflection_cache_t();

    self.lighting_update_wld = -1;
    self.lighting_frame_all  = -1;
}

ActionState::ActionState() :
    _action_ready(true), // Idiotic: This must be set at the beginning, script's spawn animations do not work!
    _action_which(ACTION_DA),
    _action_keep(false),
    _action_loop(false),
    _action_next(ACTION_DA) {
}

ActionState::~ActionState() {
}

AnimationState::AnimationState() :
    // model info
    _modelDescriptor(nullptr),
    // animation info
    _frame_nxt(0),
    _frame_lst(0),
    _ilip(0),
    _flip(0.0f),
    _rate(1.0f) {
}

AnimationState::~AnimationState() {
}

chr_instance_t::chr_instance_t() :
    // set the update frame to an invalid value
    update_frame(-1),

    matrix(Matrix4f4f::identity()),
    matrix_cache(),

    facing_z(0),

    alpha(0xFF),
    light(0),
    sheen(0),
    enviro(false),
    dont_cull_backfaces(false),

    colorshift(),

    uoffset(0),
    voffset(0),

    animationState(),

    // set the animation state
    action_ready(true),         // argh! this must be set at the beginning, script's spawn animations do not work!
    action_which(ACTION_DA),
    action_keep(false),
    action_loop(false),
    action_next(ACTION_DA),

    // lighting info
    color_amb(0),
    col_amb(),
    max_light(0),
    min_light(0),
    lighting_update_wld(-1),
    lighting_frame_all(-1),

    // linear interpolated frame vertices
    vrt_count(0),
    vrt_lst(nullptr),
    bbox(),

    // graphical optimizations
    indolist(false),
    save(),
    ref()
{
    // set the initial cache parameters
    chr_instance_t::clear_cache(*this);
}

chr_instance_t::~chr_instance_t() {
    if (vrt_lst) {
        delete[] vrt_lst;
        vrt_lst = nullptr;
    }
}

void chr_instance_t::dealloc(chr_instance_t& self)
{
	if (self.vrt_lst) {
		delete[] self.vrt_lst;
		self.vrt_lst = nullptr;
	}
    self.vrt_count = 0;
}

gfx_rv chr_instance_t::alloc(chr_instance_t& self, size_t vlst_size)
{
	chr_instance_t::dealloc(self);
	self.vrt_lst = new GLvertex[vlst_size]();
    self.vrt_count = vlst_size;
	return gfx_success;
}

gfx_rv chr_instance_t::set_mad(chr_instance_t& self, const std::shared_ptr<Ego::ModelDescriptor> &model)
{
    /// @author BB
    /// @details try to set the model used by the character instance.
    ///     If this fails, it leaves the old data. Just to be safe it
    ///     would be best to check whether the old modes is valid, and
    ///     if not, the data chould be set to safe values...

    bool updated = false;

	if (!model) {
		return gfx_fail;
	}

    if (self.animationState.getModelDescriptor() != model) {
        updated = true;
        self.animationState.setModelDescriptor(model);
    }

    // set the vertex size
    size_t vlst_size = self.animationState.getModelDescriptor()->getMD2()->getVertexCount();
    if (self.vrt_count != vlst_size) {
        updated = true;
		chr_instance_t::alloc(self, vlst_size);
    }

    // set the frames to frame 0 of this object's data
    if (0 != self.animationState.frame_nxt || 0 != self.animationState.frame_lst) {
        updated        = true;
        self.animationState.frame_lst = 0;
        self.animationState.frame_nxt = 0;

        // the vlst_cache parameters are not valid
        self.save.valid = false;
    }

    if (updated) {
        // update the vertex and lighting cache
        chr_instance_t::clear_cache(self);
        chr_instance_t::update_vertices(self, -1, -1, true);
    }

    return updated ? gfx_success : gfx_fail;
}

void chr_instance_t::update_ref(chr_instance_t& self, const Vector3f &position, bool need_matrix)
{
    const float meshElevation = _currentModule->getMeshPointer()->getElevation(Vector2f(position.x(), position.y()));

    // reflect the ordinary matrix
    if (need_matrix && self.matrix_cache.valid) {

        self.ref.matrix = self.matrix;
        self.ref.matrix(2, 0) = -self.ref.matrix(0, 2);
        self.ref.matrix(2, 1) = -self.ref.matrix(1, 2);
        self.ref.matrix(2, 2) = -self.ref.matrix(2, 2);
        self.ref.matrix(2, 3) = 2.0f * meshElevation - position.z();
        self.ref.matrix_valid = true;
    }

    // determine the reflection alpha based on altitude above the mesh
    const float altitudeAboveGround = std::max(0.0f, position.z() - meshElevation);
    float alpha = 255.0f - altitudeAboveGround;
    alpha *= 0.5f;
    alpha = Ego::Math::constrain(alpha, 0.0f, 255.0f);

	self.ref.alpha = (self.alpha * alpha * INV_FF<float>());
	self.ref.light = (255 == self.light) ? 255 : (self.light * alpha * INV_FF<float>());

    self.ref.colorshift = colorshift_t(self.colorshift.red + 1, self.colorshift.green + 1, self.colorshift.blue + 1);

	self.ref.sheen = self.sheen >> 1;
}

gfx_rv chr_instance_t::spawn(chr_instance_t& self, const PRO_REF profileID, const int skin)
{
    // Remember any previous color shifts in case of lasting enchantments
    colorshift_t colorshift_save = self.colorshift;

    // clear the instance
    self = chr_instance_t();

    const std::shared_ptr<ObjectProfile> &profile = ProfileSystem::get().getProfile(profileID);
    if (!profile) {
        return gfx_fail;
    }

    // lighting parameters
	self.enviro = profile->isPhongMapped();
	self.alpha = profile->getAlpha();
	self.light = profile->getLight();
	self.sheen = profile->getSheen();
    self.colorshift = colorshift_save;
	self.dont_cull_backfaces = profile->isDontCullBackfaces();

    // model parameters
    chr_instance_t::set_mad(self, profile->getModel());

    // set the initial action, all actions override it
    chr_instance_t::play_action(self, ACTION_DA, true);

    // upload these parameters to the reflection cache, but don't compute the matrix
    chr_instance_t::update_ref(self, Vector3f::zero(), false);

    return gfx_success;
}

BIT_FIELD chr_instance_t::get_framefx(const chr_instance_t& self)
{
    return chr_instance_t::get_frame_nxt(self).framefx;
}

gfx_rv chr_instance_t::set_frame_full(chr_instance_t& self, int frame_along, int ilip, const std::shared_ptr<Ego::ModelDescriptor>& mad_override)
{
    // handle optional parameters
	std::shared_ptr<Ego::ModelDescriptor> imad;
	if (mad_override) {
		imad = mad_override;
	} else {
        imad = self.animationState.getModelDescriptor();
    }

	if (!imad) {
		gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid mad");
		return gfx_error;
	}

    // we have to have a valid action range
	if (self.action_which > ACTION_COUNT) {
		return gfx_fail;
	}

    // try to heal a bad action
    self.action_which = imad->getAction(self.action_which);

    // reject the action if it is cannot be made valid
	if (self.action_which == ACTION_COUNT) {
		return gfx_fail;
	}

    // get some frame info
    int frame_stt   = imad->getFirstFrame(self.action_which);
    int frame_end   = imad->getLastFrame(self.action_which);
    int frame_count = 1 + ( frame_end - frame_stt );

    // try to heal an out of range value
    frame_along %= frame_count;

    // get the next frames
    int new_nxt = frame_stt + frame_along;
    new_nxt = std::min(new_nxt, frame_end);

    self.animationState.frame_nxt = new_nxt;
    self.animationState.ilip      = ilip;
    self.animationState.flip      = ilip * 0.25f;

    // set the validity of the cache
	vlst_cache_t::test(self.save, &self);

    return gfx_success;
}

void chr_instance_t::set_action_keep(chr_instance_t& self, bool val) {
	self.action_keep = val;
}

void chr_instance_t::set_action_ready(chr_instance_t& self, bool val) {
    self.action_ready = val;
}

void chr_instance_t::set_action_loop(chr_instance_t& self, bool val) {
    self.action_loop = val;
}

gfx_rv chr_instance_t::set_action_next(chr_instance_t& self, int val) {
	if (val < 0 || val > ACTION_COUNT) {
		return gfx_fail;
	}

    self.action_next = val;

    return gfx_success;
}

void chr_instance_t::remove_interpolation(chr_instance_t& self)
{
    if (self.animationState.frame_lst != self.animationState.frame_nxt ) {
		self.animationState.frame_lst = self.animationState.frame_nxt;
		self.animationState.ilip = 0;
		self.animationState.flip = 0.0f;

		vlst_cache_t::test(self.save, &self);
    }
}

const MD2_Frame& chr_instance_t::get_frame_nxt(const chr_instance_t& self)
{
	if (self.animationState.frame_nxt > self.animationState.getModelDescriptor()->getMD2()->getFrames().size())
    {
		std::ostringstream os;
		os << __FILE__ << ":" << __LINE__ << ": invalid frame " << self.animationState.frame_nxt << "/" << self.animationState.getModelDescriptor()->getMD2()->getFrames().size() << std::endl;
		Log::get().error("%s",os.str().c_str());
		throw std::runtime_error(os.str());
    }

	return self.animationState.getModelDescriptor()->getMD2()->getFrames()[self.animationState.frame_nxt];
}

const MD2_Frame& chr_instance_t::get_frame_lst(chr_instance_t& self)
{
	if (self.animationState.frame_lst > self.animationState.getModelDescriptor()->getMD2()->getFrames().size())
    {
		std::ostringstream os;
		os << __FILE__ << ":" << __LINE__ << ": invalid frame " << self.animationState.frame_lst << "/" << self.animationState.getModelDescriptor()->getMD2()->getFrames().size() << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

	return self.animationState.getModelDescriptor()->getMD2()->getFrames()[self.animationState.frame_lst];
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

float chr_instance_t::get_remaining_flip(chr_instance_t& self)
{
	return (self.animationState.ilip + 1) * 0.25f - self.animationState.flip;
}

void chr_instance_t::get_tint(chr_instance_t& self, GLfloat * tint, const BIT_FIELD bits)
{
	int i;
	float weight_sum;
	GLXvector4f local_tint;

	int local_alpha;
	int local_light;
	int local_sheen;
    colorshift_t local_colorshift;

	if (NULL == tint) tint = local_tint;

	if (HAS_SOME_BITS(bits, CHR_REFLECT))
	{
		// this is a reflection, use the reflected parameters
		local_alpha = self.ref.alpha;
		local_light = self.ref.light;
		local_sheen = self.ref.sheen;
        local_colorshift = self.ref.colorshift;
	}
	else
	{
		// this is NOT a reflection, use the normal parameters
		local_alpha = self.alpha;
		local_light = self.light;
		local_sheen = self.sheen;
        local_colorshift = self.colorshift;
	}

	// modify these values based on local character abilities
	local_alpha = get_alpha(local_alpha, local_stats.seeinvis_mag);
	local_light = get_light(local_light, local_stats.seedark_mag);

	// clear out the tint
	weight_sum = 0;
	for (i = 0; i < 4; i++) tint[i] = 0;

	if (HAS_SOME_BITS(bits, CHR_SOLID))
	{
		// solid characters are not blended onto the canvas
		// the alpha channel is not important
		weight_sum += 1.0f;

		tint[RR] += 1.0f / (1 << local_colorshift.red);
		tint[GG] += 1.0f / (1 << local_colorshift.green);
		tint[BB] += 1.0f / (1 << local_colorshift.blue);
		tint[AA] += 1.0f;
	}

	if (HAS_SOME_BITS(bits, CHR_ALPHA))
	{
		// alpha characters are blended onto the canvas using the alpha channel
		weight_sum += 1.0f;

		tint[RR] += 1.0f / (1 << local_colorshift.red);
		tint[GG] += 1.0f / (1 << local_colorshift.green);
		tint[BB] += 1.0f / (1 << local_colorshift.blue);
		tint[AA] += local_alpha * INV_FF<float>();
	}

	if (HAS_SOME_BITS(bits, CHR_LIGHT))
	{
		// alpha characters are blended onto the canvas by adding their color
		// the more black the colors, the less visible the character
		// the alpha channel is not important

		weight_sum += 1.0f;

		if (local_light < 255)
		{
			tint[RR] += local_light * INV_FF<float>() / (1 << local_colorshift.red);
			tint[GG] += local_light * INV_FF<float>() / (1 << local_colorshift.green);
			tint[BB] += local_light * INV_FF<float>() / (1 << local_colorshift.blue);
		}

		tint[AA] += 1.0f;
	}

	if (HAS_SOME_BITS(bits, CHR_PHONG))
	{
		// phong is essentially the same as light, but it is the
		// sheen that sets the effect

		float amount;

		weight_sum += 1.0f;

		amount = (Ego::Math::constrain(local_sheen, 0, 15) << 4) / 240.0f;

		tint[RR] += amount;
		tint[GG] += amount;
		tint[BB] += amount;
		tint[AA] += 1.0f;
	}

	// average the tint
	if (weight_sum != 0.0f && weight_sum != 1.0f)
	{
		for (i = 0; i < 4; i++)
		{
			tint[i] /= weight_sum;
		}
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

    if (instance->animationState.frame_lst != self.frame_nxt) {
        self.valid = false;
    }

    if (instance->animationState.frame_lst != self.frame_lst) {
        self.valid = false;
    }

    if ((instance->animationState.frame_lst != self.frame_lst)  && std::abs(instance->animationState.flip - self.flip) > FLIP_TOLERANCE) {
        self.valid = false;
    }

    return gfx_success;
}

void chr_instance_t::flash(chr_instance_t& self, uint8_t value)
{
	const float flash_val = value * INV_FF<float>();

	// flash the ambient color
	self.color_amb = flash_val;

	// flash the directional lighting
	self.color_amb = flash_val;
	for (size_t i = 0; i < self.vrt_count; ++i) {
		GLvertex *pv = &(self.vrt_lst[i]);
		pv->color_dir = flash_val;
	}
}

