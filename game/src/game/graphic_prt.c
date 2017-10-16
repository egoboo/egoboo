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

#include "game/graphic_prt.h"

#include "game/renderer_3d.h"
#include "game/game.h"
#include "game/lighting.h"
#include "game/Graphics/CameraSystem.hpp"
#include "egolib/Entities/_Include.hpp"
#include "game/CharacterMatrix.h"

float ParticleGraphicsRenderer::CALCULATE_PRT_U0(const Ego::Texture& texture, int CNT) {
    float w = texture.getSourceWidth();
    float wscale = w / static_cast<float>(texture.getWidth());
    return (((.05f + ((CNT)& 15)) / 16.0f)*wscale);
}

float ParticleGraphicsRenderer::CALCULATE_PRT_U1(const Ego::Texture& texture, int CNT)  {
    float w = texture.getSourceWidth();
    float wscale = w / static_cast<float>(texture.getWidth());
    return (((.95f + ((CNT)& 15)) / 16.0f)*wscale);
}

float ParticleGraphicsRenderer::CALCULATE_PRT_V0(const Ego::Texture& texture, int CNT)  {
    float w = texture.getSourceWidth();
    float h = texture.getSourceHeight();
    float hscale = h / static_cast<float>(texture.getHeight());
    return (((.05f + ((CNT) >> 4)) / 16.0f) * (w / h)*hscale);
}

float ParticleGraphicsRenderer::CALCULATE_PRT_V1(const Ego::Texture& texture, int CNT) {
    float w = texture.getSourceWidth();
    float h = texture.getSourceHeight();
    float hscale = h / static_cast<float>(texture.getHeight());
    return (((.95f + ((CNT) >> 4)) / 16.0f) * (w / h)*hscale);
}

gfx_rv ParticleGraphicsRenderer::render_one_prt_solid(const ParticleRef iprt)
{
    /// @author BB
    /// @details Render the solid version of the particle

    const auto& pprt = ParticleHandler::get()[iprt];
    if (pprt == nullptr || pprt->isTerminated())
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid particle `" << iprt << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    // if the particle is hidden, do not continue
    if (pprt->isHidden()) return gfx_fail;

    // if the particle instance data is not valid, do not continue
    if (!pprt->inst.valid) return gfx_fail;
    auto& pinst = pprt->inst;

    // only render solid sprites
    if (SPRITE_SOLID != pprt->type) return gfx_fail;

    auto& renderer = Ego::Renderer::get();
    renderer.setWorldMatrix(Matrix4f4f::identity());
    {
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        {
            std::shared_ptr<const Ego::Texture> texture = nullptr;
            // Use the depth test to eliminate hidden portions of the particle
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(Ego::CompareFunction::Less);                                   // GL_DEPTH_BUFFER_BIT

            // enable the depth mask for the solid portion of the particles
            renderer.setDepthWriteEnabled(true);

            // draw draw front and back faces of polygons
            renderer.setCullingMode(Ego::CullingMode::None);

            // Since the textures are probably mipmapped or minified with some kind of
            // interpolation, we can never really turn blending off.
            renderer.setBlendingEnabled(true);
            renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

            // only display the portion of the particle that is 100% solid
            renderer.setAlphaTestEnabled(true);
            renderer.setAlphaFunction(Ego::CompareFunction::Equal, 1.0f);

            texture = ParticleHandler::get().getTransparentParticleTexture();
            renderer.getTextureUnit().setActivated(texture.get());

            renderer.setColour(Ego::Math::Colour4f(pinst.fintens, pinst.fintens, pinst.fintens, 1.0f));

            // billboard for the particle
            auto vd = Ego::VertexFormatFactory::get<Ego::VertexFormat::P3FT2F>();
            auto vb = std::make_shared<Ego::VertexBuffer>(4, vd.getVertexSize());
            calc_billboard_verts(*texture, *vb, pinst, pinst.size, false);

            renderer.render(*vb, vd, Ego::PrimitiveType::TriangleFan, 0, 4);
        }
    }

    return gfx_success;
}

gfx_rv ParticleGraphicsRenderer::render_one_prt_trans(const ParticleRef iprt)
{
    /// @author BB
    /// @details do all kinds of transparent sprites next

    const auto& pprt = ParticleHandler::get()[iprt];

    if (pprt == nullptr || pprt->isTerminated())
    {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid particle `" << iprt << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    // if the particle is hidden, do not continue
    if (pprt->isHidden()) return gfx_fail;

    // if the particle instance data is not valid, do not continue
    if (!pprt->inst.valid) return gfx_fail;
    auto& renderer = Ego::Renderer::get();
    auto& inst = pprt->inst;

    {
        renderer.setWorldMatrix(Matrix4f4f::identity());
        Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        {
            // Do not write into the depth buffer.
            renderer.setDepthWriteEnabled(false);

            // Enable depth test: Incoming fragment's depth value must be less or equal.
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

            // Draw front-facing and back-facing polygons.
            renderer.setCullingMode(Ego::CullingMode::None);

            Ego::Math::Colour4f particleColour;
            std::shared_ptr<const Ego::Texture> texture = nullptr;

            switch(pprt->type)
            {
                // Solid sprites.
                case SPRITE_SOLID:
                {
                    // Do the alpha blended edge ("anti-aliasing") of the solid particle.
                    // Only display the alpha-edge of the particle.
                    renderer.setAlphaTestEnabled(true);
                    renderer.setAlphaFunction(Ego::CompareFunction::Less, 1.0f);

                    renderer.setBlendingEnabled(true);
                    renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

                    particleColour = Ego::Math::Colour4f(inst.fintens, inst.fintens, inst.fintens, 1.0f);

                    texture = ParticleHandler::get().getTransparentParticleTexture();
                    renderer.getTextureUnit().setActivated(texture.get());
                }
                break;

                // Light sprites.
                case SPRITE_LIGHT:
                {
                    //Is particle invisible?
                    if(inst.fintens * inst.falpha <= 0.0f) {
                        return gfx_success;
                    }

                    renderer.setAlphaTestEnabled(false);
                    renderer.setBlendingEnabled(true);
                    renderer.setBlendFunction(Ego::BlendFunction::One, Ego::BlendFunction::One);

                    particleColour = Ego::Math::Colour4f(1.0f, 1.0f, 1.0f, inst.fintens * inst.falpha);

                    texture = ParticleHandler::get().getLightParticleTexture();
                    renderer.getTextureUnit().setActivated(texture.get());
                }
                break;

                // Transparent sprites.
                case SPRITE_ALPHA:
                {
                    //Is particle invisible?
                    if(inst.falpha <= 0.0f) {
                        return gfx_success;
                    }

                    // do not display the completely transparent portion
                    renderer.setAlphaTestEnabled(true);
                    renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);

                    renderer.setBlendingEnabled(true);
                    renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

                    particleColour = Ego::Math::Colour4f(inst.fintens, inst.fintens, inst.fintens, inst.falpha);

                    texture = ParticleHandler::get().getTransparentParticleTexture();
                    renderer.getTextureUnit().setActivated(texture.get());
                }
                break;

                // unknown type
                default:
                    return gfx_error;
                break;
            }

            auto vd = Ego::VertexFormatFactory::get<Ego::VertexFormat::P3FT2F>();
            auto vb = std::make_shared<Ego::VertexBuffer>(4, vd.getVertexSize());
            calc_billboard_verts(*texture, *vb, inst, inst.size, false);

            renderer.setColour(particleColour);

            // Go on and draw it
            renderer.render(*vb, vd, Ego::PrimitiveType::TriangleFan, 0, 4);
        }
    }

    return gfx_success;
}

gfx_rv ParticleGraphicsRenderer::render_one_prt_ref(const ParticleRef iprt)
{
    /// @author BB
    /// @details render one particle
    const auto& pprt = ParticleHandler::get()[iprt];
    if(!pprt || pprt->isTerminated()) {
        Log::Entry e(Log::Level::Error, __FILE__, __LINE__);
        e << "invalid particle `" << iprt << "`" << Log::EndOfEntry;
        Log::get() << e;
        return gfx_error;
    }

    // if the particle is hidden, do not continue
    if (pprt->isHidden()) return gfx_fail;

    if (!pprt->inst.valid || !pprt->inst.ref_valid) return gfx_fail;
    auto& inst = pprt->inst;

    //Calculate the fadeoff factor depending on how high above the floor the particle is 
    float fadeoff = 255.0f - (pprt->enviro.floor_level - inst.ref_pos.z()); //255 - distance over ground
    fadeoff *= 0.5f;
    fadeoff = Ego::Math::constrain(fadeoff*id::fraction<float, 1, 255>(), 0.0f, 1.0f);

    auto& renderer = Ego::Renderer::get();
    if (fadeoff > 0.0f)
    {
        renderer.setWorldMatrix(Matrix4f4f::identity());
        {
            Ego::OpenGL::PushAttrib pa(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
            {
                Ego::Math::Colour4f particle_colour;
                std::shared_ptr<const Ego::Texture> texture = nullptr;

                // don't write into the depth buffer (disable glDepthMask for transparent objects)
                renderer.setDepthWriteEnabled(false); // ENABLE_BIT

                // do not draw hidden surfaces
                renderer.setDepthTestEnabled(true);
                renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

                // draw draw front and back faces of polygons
                renderer.setCullingMode(Ego::CullingMode::None);

                // do not display the completely transparent portion
                renderer.setAlphaTestEnabled(true);
                renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);

                renderer.setBlendingEnabled(true);
                renderer.setBlendFunction(Ego::BlendFunction::SourceAlpha, Ego::BlendFunction::OneMinusSourceAlpha);

                switch(pprt->type) 
                {
                    case SPRITE_LIGHT:
                    {
                        // do the light sprites
                        float alpha = fadeoff * inst.falpha;

                        //Nothing to draw?
                        if(alpha <= 0.0f) {
                            return gfx_fail;
                        }

                        particle_colour = Ego::Math::Colour4f(1.0f, 1.0f, 1.0f, alpha);

                        texture = ParticleHandler::get().getLightParticleTexture();
                        renderer.getTextureUnit().setActivated(texture.get());
                    }
                    break;

                    case SPRITE_SOLID:
                    case SPRITE_ALPHA:
                    {
                        float alpha = fadeoff;
                        if (SPRITE_ALPHA == pprt->type) {
                            alpha *= inst.falpha;

                            //Nothing to draw?
                            if(alpha <= 0.0f) {
                                return gfx_fail;
                            }
                        }

                        particle_colour = Ego::Math::Colour4f(inst.fintens, inst.fintens, inst.fintens, alpha);

                        texture = ParticleHandler::get().getTransparentParticleTexture();
                        renderer.getTextureUnit().setActivated(texture.get());
                    }
                    break;

                    // unknown type
                    default:
                        return gfx_fail;
                    break;
                }

                // Calculate the position of the four corners of the billboard
                // used to display the particle.
                auto vd = Ego::VertexFormatFactory::get<Ego::VertexFormat::P3FT2F>();
                auto vb = std::make_shared<Ego::VertexBuffer>(4, vd.getVertexSize());
                calc_billboard_verts(*texture, *vb, inst, inst.size, true);

                renderer.setColour(particle_colour); // GL_CURRENT_BIT

                renderer.render(*vb, vd, Ego::PrimitiveType::TriangleFan, 0, 4);
            }
        }
    }

    return gfx_success;
}

void ParticleGraphicsRenderer::calc_billboard_verts(const Ego::Texture& texture, Ego::VertexBuffer& vb, Ego::Graphics::ParticleGraphics& inst, float size, bool do_reflect)
{
    // Calculate the position and texture coordinates of the four corners of the billboard used to display the particle.

    if (vb.getNumberOfVertices() < 4)
    {
        throw std::runtime_error("vertex buffer too small");
    }
    /// @todo Add a compare by equality for vertex format descriptors and assert the vertex buffer has the required format.
    struct Vertex
    {
        float x, y, z;
        float s, t;
    };

    int i;
	Vector3f prt_pos, prt_up, prt_right;

    // use the pre-computed reflection parameters
    if (do_reflect)
    {
        prt_pos = inst.ref_pos;
        prt_up = inst.ref_up;
        prt_right = inst.ref_right;
    }
    else
    {
        prt_pos = inst.pos;
        prt_up = inst.up;
        prt_right = inst.right;
    }

    Vertex *v = static_cast<Vertex *>(vb.lock());

    for (i = 0; i < 4; i++)
    {
        v[i].x = prt_pos[kX];
        v[i].y = prt_pos[kY];
        v[i].z = prt_pos[kZ];
    }

    // Considered as left bottom.
    // Hence expand to the left and the bottom.
    v[0].x += (-prt_right[kX] - prt_up[kX]) * size;
    v[0].y += (-prt_right[kY] - prt_up[kY]) * size;
    v[0].z += (-prt_right[kZ] - prt_up[kZ]) * size;

    // Considered as right bottom.
    // Hence expand to the right and the bottom.
    v[1].x += (prt_right[kX] - prt_up[kX]) * size;
    v[1].y += (prt_right[kY] - prt_up[kY]) * size;
    v[1].z += (prt_right[kZ] - prt_up[kZ]) * size;

    // Considered as right top.
    // Hence expand to the right and the top.
    v[2].x += (prt_right[kX] + prt_up[kX]) * size;
    v[2].y += (prt_right[kY] + prt_up[kY]) * size;
    v[2].z += (prt_right[kZ] + prt_up[kZ]) * size;

    // Considered as left top.
    // Hence expand to the left and the top.
    v[3].x += (-prt_right[kX] + prt_up[kX]) * size;
    v[3].y += (-prt_right[kY] + prt_up[kY]) * size;
    v[3].z += (-prt_right[kZ] + prt_up[kZ]) * size;

    v[0].s = CALCULATE_PRT_U1(texture, inst.image_ref);
    v[0].t = CALCULATE_PRT_V1(texture, inst.image_ref);

    v[1].s = CALCULATE_PRT_U0(texture, inst.image_ref);
    v[1].t = CALCULATE_PRT_V1(texture, inst.image_ref);

    v[2].s = CALCULATE_PRT_U0(texture, inst.image_ref);
    v[2].t = CALCULATE_PRT_V0(texture, inst.image_ref);

    v[3].s = CALCULATE_PRT_U1(texture, inst.image_ref);
    v[3].t = CALCULATE_PRT_V0(texture, inst.image_ref);

    vb.unlock();
}

void ParticleGraphicsRenderer::render_all_prt_attachment()
{
    Ego::Renderer::get().setBlendingEnabled(false);

    for(const auto& particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;
        prt_draw_attached_point(particle);
    }
}

void ParticleGraphicsRenderer::render_all_prt_bbox()
{
    for(const auto& particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;
        render_prt_bbox(particle);
    }
}

void ParticleGraphicsRenderer::draw_one_attachment_point(Ego::Graphics::ObjectGraphics& inst, int vrt_offset)
{
    /// @author BB
    /// @details a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.
    uint32_t vrt = (int)inst.getVertexCount() - (int)vrt_offset;

    if (vrt >= inst.getVertexCount()) return;

    auto& renderer = Ego::Renderer::get();
    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    renderer.getTextureUnit().setActivated(nullptr);
    renderer.setPointSize(5);
    renderer.setViewMatrix(Matrix4f4f::identity());
    renderer.setWorldMatrix(inst.getMatrix());
    GL_DEBUG(glBegin(GL_POINTS));
    {
        GL_DEBUG(glVertex3fv)(inst.getVertex(vrt).pos);
    }
    GL_DEBUG_END();
}

void ParticleGraphicsRenderer::prt_draw_attached_point(const std::shared_ptr<Ego::Particle>& particle)
{
    if (!particle->isAttached()) {
        return;
    }

    draw_one_attachment_point(particle->getAttachedObject()->inst, particle->attachedto_vrt_off);
}

void ParticleGraphicsRenderer::render_prt_bbox(const std::shared_ptr<Ego::Particle>& particle)
{    
    // only draw bullets
    //if ( 50 != loc_ppip->vel_hrz_pair.base ) return;

    // draw the object bounding box as a part of the graphics debug mode F7
    if ((egoboo_config_t::get().debug_developerMode_enable.getValue() && Ego::Input::InputSystem::get().isKeyDown(SDLK_F7)))
    {
        // copy the bounding volume
        oct_bb_t tmp_bb = particle->prt_max_cv;

        // determine the expanded collision volumes for both objects
        oct_bb_t exp_bb;
        phys_expand_oct_bb(tmp_bb, particle->getVelocity(), 0, 1, exp_bb);

        // shift the source bounding boxes to be centered on the given positions
        auto loc_bb = id::translate(exp_bb, particle->getPosition());

        Ego::Renderer::get().getTextureUnit().setActivated(nullptr);
        Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());
        Renderer3D::renderOctBB(loc_bb, true, true, Ego::Math::Colour4f::red(), Ego::Math::Colour4f::yellow());
    }
}
