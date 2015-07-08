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
#include "game/Graphics/CameraSystem.hpp"
#include "game/Entities/_Include.hpp"

//--------------------------------------------------------------------------------------------


int ptex_w[2] = { 256, 256 };
int ptex_h[2] = { 256, 256 };
float ptex_wscale[2] = { 1.0f, 1.0f };
float ptex_hscale[2] = { 1.0f, 1.0f };

float CALCULATE_PRT_U0(int IDX, int CNT)  {
    return (((.05f + ((CNT)& 15)) / 16.0f)*ptex_wscale[IDX]);
}

float CALCULATE_PRT_U1(int IDX, int CNT)  {
    return (((.95f + ((CNT)& 15)) / 16.0f)*ptex_wscale[IDX]);
}

float CALCULATE_PRT_V0(int IDX, int CNT)  {
    return (((.05f + ((CNT) >> 4)) / 16.0f) * ((float)ptex_w[IDX] / (float)ptex_h[IDX])*ptex_hscale[IDX]);
}

float CALCULATE_PRT_V1(int IDX, int CNT) {
    return (((.95f + ((CNT) >> 4)) / 16.0f) * ((float)ptex_w[IDX] / (float)ptex_h[IDX])*ptex_hscale[IDX]);
}

int prt_get_texture_style(const TX_REF itex)
{
    int index;

    index = -1;
    switch (REF_TO_INT(itex))
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

void prt_set_texture_params(const TX_REF itex)
{
    int index = prt_get_texture_style(itex);
    if (index < 0)
    {
        return;
    }
    oglx_texture_t *ptex = TextureManager::get().get_valid_ptr(itex);
    if (!ptex)
    {
        return;
    }
    ptex_w[index] = ptex->getSourceWidth();
    ptex_h[index] = ptex->getSourceHeight();
    ptex_wscale[index] = (float)ptex->getSourceWidth() / (float)ptex->getWidth();
    ptex_hscale[index] = (float)ptex->getSourceHeight() / (float)ptex->getHeight();
}

//--------------------------------------------------------------------------------------------

Uint32 instance_update = (Uint32)~0;

//--------------------------------------------------------------------------------------------
static gfx_rv prt_instance_update(Camera& camera, const PRT_REF particle, Uint8 trans, bool do_lighting);
static void calc_billboard_verts(Ego::VertexBuffer& vb, prt_instance_t *pinst, float size, bool do_reflect);
static void draw_one_attachment_point(chr_instance_t *pinst, mad_t *pmad, int vrt_offset);
static void prt_draw_attached_point(prt_bundle_t *pbdl_prt);
static void render_prt_bbox(prt_bundle_t *pbdl_prt);
static gfx_rv prt_instance_update_vertices(Camera& camera, prt_instance_t * pinst, Ego::Particle * pprt);
static fmat_4x4_t prt_instance_make_matrix(prt_instance_t *pinst);
static gfx_rv prt_instance_update_lighting(prt_instance_t *pinst, Ego::Particle *pprt, Uint8 trans, bool do_lighting);

//--------------------------------------------------------------------------------------------

gfx_rv render_one_prt_solid(const PRT_REF iprt)
{
    /// @author BB
    /// @details Render the solid version of the particle

    const std::shared_ptr<Ego::Particle> &pprt = ParticleHandler::get()[iprt];
    if (pprt == nullptr || pprt->isTerminated())
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, iprt, "invalid particle");
        return gfx_error;
    }

    // if the particle is hidden, do not continue
    if (pprt->isHidden()) return gfx_fail;

    // if the particle instance data is not valid, do not continue
    if (!pprt->inst.valid) return gfx_fail;
    prt_instance_t *pinst = &(pprt->inst);

    // only render solid sprites
    if (SPRITE_SOLID != pprt->type) return gfx_fail;

    // billboard for the particle
    auto vb = std::make_shared<Ego::VertexBuffer>(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P3FT2F>());
    calc_billboard_verts(*vb, pinst, pinst->size, false);

    ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    {
        auto& renderer = Ego::Renderer::get();
        // Use the depth test to eliminate hidden portions of the particle
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(Ego::CompareFunction::Less);                                   // GL_DEPTH_BUFFER_BIT

        // enable the depth mask for the solid portion of the particles
        renderer.setDepthWriteEnabled(true);

        // draw draw front and back faces of polygons
        oglx_end_culling();        // GL_ENABLE_BIT

        // Since the textures are probably mipmapped or minified with some kind of
        // interpolation, we can never really turn blending off.
        renderer.setBlendingEnabled(true);
        GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);            // GL_ENABLE_BIT

        // only display the portion of the particle that is 100% solid
        renderer.setAlphaTestEnabled(true);
		renderer.setAlphaFunction(Ego::CompareFunction::Equal, 1.0f);  // GL_COLOR_BUFFER_BIT

        oglx_texture_t::bind(TextureManager::get().get_valid_ptr((TX_REF)TX_PARTICLE_TRANS));

        renderer.setColour(Ego::Math::Colour4f(pinst->fintens, pinst->fintens, pinst->fintens, 1.0f)); // GL_CURRENT_BIT

        renderer.render(*vb, Ego::PrimitiveType::TriangleFan, 0, 4);
    }
    ATTRIB_POP(__FUNCTION__);

    return gfx_success;
}

gfx_rv render_one_prt_trans(const PRT_REF iprt)
{
    /// @author BB
    /// @details do all kinds of transparent sprites next

    const std::shared_ptr<Ego::Particle> &pprt = ParticleHandler::get()[iprt];

    if (pprt == nullptr || pprt->isTerminated())
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, iprt, "invalid particle");
        return gfx_error;
    }

    // if the particle is hidden, do not continue
    if (pprt->isHidden()) return gfx_fail;

    // if the particle instance data is not valid, do not continue
    if (!pprt->inst.valid) return gfx_fail;
    prt_instance_t *pinst = &(pprt->inst);

    ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    {
        auto& renderer = Ego::Renderer::get();
        // Do not write into the depth buffer.
        renderer.setDepthWriteEnabled(false);

        // Enable depth test: Incoming fragment's depth value must be less or equal.
        renderer.setDepthTestEnabled(true);
        renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

        // Draw front-facing and back-facing polygons.
        oglx_end_culling();

        Ego::Math::Colour4f particleColour;
        bool drawParticle = false;
        // Solid sprites.
        if (SPRITE_SOLID == pprt->type)
        {
            // Do the alpha blended edge ("anti-aliasing") of the solid particle.
            // Only display the alpha-edge of the particle.
            renderer.setAlphaTestEnabled(true);
			renderer.setAlphaFunction(Ego::CompareFunction::Less, 1.0f);   // GL_COLOR_BUFFER_BIT

            renderer.setBlendingEnabled(true);
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
            renderer.setAlphaTestEnabled(false);
            renderer.setBlendingEnabled(true);
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
            renderer.setAlphaTestEnabled(true);
			renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);  // GL_COLOR_BUFFER_BIT

            renderer.setBlendingEnabled(true);
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
            auto vb = std::make_shared<Ego::VertexBuffer>(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P3FT2F>());
            calc_billboard_verts(*vb, pinst, pinst->size, false);

            renderer.setColour(particleColour);

            // Go on and draw it
            renderer.render(*vb, Ego::PrimitiveType::TriangleFan, 0, 4);
        }
    }
    ATTRIB_POP(__FUNCTION__);

    return gfx_success;
}

gfx_rv render_one_prt_ref(const PRT_REF iprt)
{
    /// @author BB
    /// @details render one particle
    int startalpha;

    const std::shared_ptr<Ego::Particle>& pprt = ParticleHandler::get()[iprt];
    if(!pprt || pprt->isTerminated()) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, iprt, "invalid particle");
        return gfx_error;
    }

    // if the particle is hidden, do not continue
    if (pprt->isHidden()) return gfx_fail;

    if (!pprt->inst.valid || !pprt->inst.ref_valid) return gfx_fail;
    prt_instance_t *pinst = &(pprt->inst);

    // Fill in the rest of the data. (make it match the case for characters)
    startalpha = 255;
    startalpha -= 2.0f * (pprt->enviro.floor_level - pinst->ref_pos[kZ]);
    startalpha *= 0.5f;
    startalpha = CLIP(startalpha, 0, 255);

    //startalpha = ( startalpha | gfx.reffadeor ) >> 1;  // Fix for Riva owners
    //startalpha = CLIP(startalpha, 0, 255);

    if (startalpha > 0)
    {
        ATTRIB_PUSH(__FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
        {
            Ego::Colour4f particle_colour;

            auto& renderer = Ego::Renderer::get();
            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            renderer.setDepthWriteEnabled(false); // ENABLE_BIT

            // do not draw hidden surfaces
            renderer.setDepthTestEnabled(true);
            renderer.setDepthFunction(Ego::CompareFunction::LessOrEqual);

            // draw draw front and back faces of polygons
            oglx_end_culling();    // ENABLE_BIT

            bool draw_particle = false;
            if (SPRITE_LIGHT == pprt->type)
            {
                // do the light sprites
                float intens = startalpha * INV_FF * pinst->falpha * pinst->fintens;

                renderer.setAlphaTestEnabled(false);

                renderer.setBlendingEnabled(true);
                GL_DEBUG(glBlendFunc)(GL_ONE, GL_ONE);  // GL_COLOR_BUFFER_BIT

                particle_colour = Ego::Math::Colour4f(intens, intens, intens, 1.0f);

                pinst->texture_ref = TX_PARTICLE_TRANS;
                oglx_texture_t::bind(TextureManager::get().get_valid_ptr(pinst->texture_ref));

                draw_particle = intens > 0.0f;
            }
            else if (SPRITE_SOLID == pprt->type || SPRITE_ALPHA == pprt->type)
            {
                // do the transparent sprites

                float alpha = startalpha * INV_FF;
                if (SPRITE_ALPHA == pprt->type)
                {
                    alpha *= pinst->falpha;
                }

                auto& renderer = Ego::Renderer::get();
                // do not display the completely transparent portion
                renderer.setAlphaTestEnabled(true);
				renderer.setAlphaFunction(Ego::CompareFunction::Greater, 0.0f);  // GL_COLOR_BUFFER_BIT

                renderer.setBlendingEnabled(true);
                GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // GL_COLOR_BUFFER_BIT

                particle_colour = Ego::Math::Colour4f(pinst->fintens, pinst->fintens, pinst->fintens, alpha);

                pinst->texture_ref = TX_PARTICLE_TRANS;
                oglx_texture_t::bind(TextureManager::get().get_valid_ptr(pinst->texture_ref));

                draw_particle = alpha > 0.0f;
            }
            else
            {
                // unknown type
                return gfx_fail;
            }

            if (draw_particle)
            {
                // Calculate the position of the four corners of the billboard
                // used to display the particle.
                auto vb = std::make_shared<Ego::VertexBuffer>(4, Ego::VertexFormatDescriptor::get<Ego::VertexFormat::P3FT2F>());
                calc_billboard_verts(*vb, pinst, pinst->size, true);

                renderer.setColour(particle_colour); // GL_CURRENT_BIT

                renderer.render(*vb, Ego::PrimitiveType::TriangleFan, 0, 4);
            }
        }
        ATTRIB_POP(__FUNCTION__);

    }

    return gfx_success;
}

void calc_billboard_verts(Ego::VertexBuffer& vb, prt_instance_t *pinst, float size, bool do_reflect)
{
    // Calculate the position and texture coordinates of the four corners of the billboard used to display the particle.

    if (!pinst)
    {
        throw std::invalid_argument("nullptr == pinst");
    }
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

    int i, index;
    fvec3_t prt_pos, prt_up, prt_right;

    switch (REF_TO_INT(pinst->texture_ref))
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
    if (do_reflect)
    {
        prt_pos = pinst->ref_pos;
        prt_up = pinst->ref_up;
        prt_right = pinst->ref_right;
    }
    else
    {
        prt_pos = pinst->pos;
        prt_up = pinst->up;
        prt_right = pinst->right;
    }

    Vertex *v = static_cast<Vertex *>(vb.lock());

    for (i = 0; i < 4; i++)
    {
        v[i].x = prt_pos[kX];
        v[i].y = prt_pos[kY];
        v[i].z = prt_pos[kZ];
    }

    v[0].x += (-prt_right[kX] - prt_up[kX]) * size;
    v[0].y += (-prt_right[kY] - prt_up[kY]) * size;
    v[0].z += (-prt_right[kZ] - prt_up[kZ]) * size;

    v[1].x += (prt_right[kX] - prt_up[kX]) * size;
    v[1].y += (prt_right[kY] - prt_up[kY]) * size;
    v[1].z += (prt_right[kZ] - prt_up[kZ]) * size;

    v[2].x += (prt_right[kX] + prt_up[kX]) * size;
    v[2].y += (prt_right[kY] + prt_up[kY]) * size;
    v[2].z += (prt_right[kZ] + prt_up[kZ]) * size;

    v[3].x += (-prt_right[kX] + prt_up[kX]) * size;
    v[3].y += (-prt_right[kY] + prt_up[kY]) * size;
    v[3].z += (-prt_right[kZ] + prt_up[kZ]) * size;

    v[0].s = CALCULATE_PRT_U1(index, pinst->image_ref);
    v[0].t = CALCULATE_PRT_V1(index, pinst->image_ref);

    v[1].s = CALCULATE_PRT_U0(index, pinst->image_ref);
    v[1].t = CALCULATE_PRT_V1(index, pinst->image_ref);

    v[2].s = CALCULATE_PRT_U0(index, pinst->image_ref);
    v[2].t = CALCULATE_PRT_V0(index, pinst->image_ref);

    v[3].s = CALCULATE_PRT_U1(index, pinst->image_ref);
    v[3].t = CALCULATE_PRT_V0(index, pinst->image_ref);

    vb.unlock();
}

void render_all_prt_attachment()
{
    Ego::Renderer::get().setBlendingEnabled(false);

    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;

        prt_bundle_t prt_bdl(particle.get());
        prt_draw_attached_point(&prt_bdl);
    }
}

void render_all_prt_bbox()
{
    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;

        prt_bundle_t prt_bdl(particle.get());
        render_prt_bbox(&prt_bdl);
    }
}

void draw_one_attachment_point(chr_instance_t *pinst, mad_t *pmad, int vrt_offset)
{
    /// @author BB
    /// @details a function that will draw some of the vertices of the given character.
    ///     The original idea was to use this to debug the grip for attached items.
    if (!pinst || !pmad)
    {
        return;
    }
    uint32_t vrt = (int)pinst->vrt_count - (int)vrt_offset;

    if (vrt >= pinst->vrt_count) return;

    // disable the texturing so all the points will be white,
    // not the texture color of the last vertex we drawn
    oglx_texture_t::bind(nullptr);

    GL_DEBUG(glPointSize)(5);

    // save the matrix mode
    GLint matrix_mode[1];
    GL_DEBUG(glGetIntegerv)(GL_MATRIX_MODE, matrix_mode);

    // store the GL_MODELVIEW matrix (this stack has a finite depth, minimum of 32)
    GL_DEBUG(glMatrixMode)(GL_MODELVIEW);
    GL_DEBUG(glPushMatrix)();
    Ego::Renderer::get().multiplyMatrix(pinst->matrix);
    GL_DEBUG(glBegin(GL_POINTS));
    {
        GL_DEBUG(glVertex3fv)(pinst->vrt_lst[vrt].pos);
    }
    GL_DEBUG_END();

    // Restore the GL_MODELVIEW matrix
    GL_DEBUG(glMatrixMode)(GL_MODELVIEW);
    GL_DEBUG(glPopMatrix)();

    // restore the matrix mode
    GL_DEBUG(glMatrixMode)(matrix_mode[0]);
}

void prt_draw_attached_point(prt_bundle_t *pbdl_prt)
{
    if (!pbdl_prt)
    {
        return;
    }

    Ego::Particle *loc_pprt = pbdl_prt->_prt_ptr;
    if (loc_pprt == nullptr || loc_pprt->isTerminated())
    {
        return;
    }

    if (!loc_pprt->isAttached())
    {
        return;
    }
    const std::shared_ptr<Object>& pholder = loc_pprt->getAttachedObject();

    mad_t *pholder_mad = chr_get_pmad(pholder->getCharacterID());
    if (!pholder_mad)
    {
        return;
    }
    draw_one_attachment_point(&(pholder->inst), pholder_mad, loc_pprt->attachedto_vrt_off);
}

gfx_rv update_all_prt_instance(Camera& camera)
{
    // only one update per frame
    if (instance_update == update_wld) return gfx_success;
    instance_update = update_wld;

    // assume the best
    gfx_rv retval = gfx_success;

    for(const std::shared_ptr<Ego::Particle> &particle : ParticleHandler::get().iterator())
    {
        if(particle->isTerminated()) continue;
        
        prt_bundle_t prt_bdl(particle.get());

        prt_instance_t *pinst = &(prt_bdl._prt_ptr->inst);

        // only do frame counting for particles that are fully activated!
        prt_bdl._prt_ptr->frame_count++;

        if (!prt_bdl._prt_ptr->inst.indolist)
        {
            pinst->valid = false;
            pinst->ref_valid = false;
        }
        else
        {
            // calculate the "billboard" for this particle
            if (gfx_error == prt_instance_update(camera, particle->getParticleID(), 255, true))
            {
                retval = gfx_error;
            }
        }
    }
 
    return retval;
}

gfx_rv prt_instance_update_vertices(Camera& camera, prt_instance_t *pinst, Ego::Particle *pprt)
{
    if (!pinst)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == pinst");
        return gfx_error;
    }
    pinst->valid = false;
    pinst->ref_valid = false;

    if (pprt->isTerminated())
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, pprt->getParticleID(), "invalid particle");
        return gfx_error;
    }

    const std::shared_ptr<pip_t> &ppip = pprt->getProfile();

    pinst->type = pprt->type;

    pinst->image_ref = (pprt->_image._start / EGO_ANIMATION_MULTIPLIER + pprt->_image._offset / EGO_ANIMATION_MULTIPLIER);


    // Set the position.
    pinst->pos = pprt->getPosition();
    pinst->orientation = ppip->orientation;

    // Calculate the billboard vectors for the reflections.
    pinst->ref_pos = pprt->getPosition();
    pinst->ref_pos[kZ] = 2 * pprt->enviro.floor_level - pinst->pos[kZ];

    // get the vector from the camera to the particle
    fvec3_t vfwd = pinst->pos - camera.getPosition();
    vfwd.normalize();

    fvec3_t vfwd_ref = pinst->ref_pos - camera.getPosition();
    vfwd_ref.normalize();

    // Set the up and right vectors.
    fvec3_t vup = fvec3_t(0.0f, 0.0f, 1.0f), vright;
    fvec3_t vup_ref = fvec3_t(0.0f, 0.0f, 1.0f), vright_ref;
    if (ppip->rotatetoface && !pprt->isAttached() && (pprt->vel.length_abs() > 0))
    {
        // The particle points along its direction of travel.

        vup = pprt->vel;
        vup.normalize();

        // Get the correct "right" vector.
        vright = vfwd.cross(vup);
        vright.normalize();

        vup_ref = vup;
        vright_ref = vfwd_ref.cross(vup);
        vright_ref.normalize();
    }
    else if (ORIENTATION_B == pinst->orientation)
    {
        // Use the camera up vector.
        vup = camera.getVUP();
        vup.normalize();

        // Get the correct "right" vector.
        vright = vfwd.cross(vup);
        vright.normalize();

        vup_ref = vup;
        vright_ref = vfwd_ref.cross(vup);
        vright_ref.normalize();
    }
    else if (ORIENTATION_V == pinst->orientation)
    {
        // Using just the global up vector here is too harsh.
        // Smoothly interpolate the global up vector with the camera up vector
        // so that when the camera is looking straight down, the billboard's plane
        // is turned by 45 degrees to the camera (instead of 90 degrees which is invisible)

        // Use the camera up vector.
        fvec3_t vup_cam = camera.getVUP();

        // Use the global up vector.
        vup = fvec3_t(0, 0, 1);

        // Adjust the vector so that the particle doesn't disappear if
        // you are viewing it from from the top or the bottom.
        float weight = 1.0f - std::abs(vup_cam[kZ]);
        if (vup_cam[kZ] < 0) weight *= -1;

        vup += vup_cam * weight;
        vup.normalize();

        // Get the correct "right" vector.
        vright = vfwd.cross(vup);
        vright.normalize();

        vright_ref = vfwd.cross(vup_ref);
        vright_ref.normalize();

        vup_ref = vup;
        vright_ref = vfwd_ref.cross(vup);
        vright_ref.normalize();
    }
    else if (ORIENTATION_H == pinst->orientation)
    {
        fvec3_t vert = fvec3_t(0.0f, 0.0f, 1.0f);

        // Force right to be horizontal.
        vright = vfwd.cross(vert);

        // Force "up" to be close to the camera forward, but horizontal.
        vup = vert.cross(vright);
        //vup_ref = vert.cross(vright_ref); //TODO: JJ> is this needed?

        // Normalize them.
        vright.normalize();
        vup.normalize();

        vright_ref = vright;
        vup_ref = vup;
    }
    else if (pprt->isAttached())
    {
        chr_instance_t *cinst = &(pprt->getAttachedObject()->inst);

        if (chr_matrix_valid(pprt->getAttachedObject().get()))
        {
            // Use the character matrix to orient the particle.
            // Assume that the particle "up" is in the z-direction in the object's
            // body fixed axes. Should work for the gonnes & such.

            switch (pinst->orientation)
            {
                case ORIENTATION_X: vup = mat_getChrForward(cinst->matrix); break;
                case ORIENTATION_Y: vup = mat_getChrRight(cinst->matrix);   break;
                default:
                case ORIENTATION_Z: vup = mat_getChrUp(cinst->matrix);      break;
            }

            vup.normalize();
        }
        else
        {
            // Use the camera directions?
            switch (pinst->orientation)
            {
                case ORIENTATION_X: vup = camera.getVFW(); break;
                case ORIENTATION_Y: vup = camera.getVRT(); break;

                default:
                case ORIENTATION_Z: vup = camera.getVUP(); break;
            }
        }

        vup.normalize();

        // Get the correct "right" vector.
        vright = vfwd.cross(vup);
        vright.normalize();

        vup_ref = vup;
        vright_ref = vfwd_ref.cross(vup);
        vright_ref.normalize();
    }
    else
    {
        // Use the camera up vector.
        vup = camera.getVUP();
        vup.normalize();

        // Get the correct "right" vector.
        vright = vfwd.cross(vup);
        vright.normalize();

        vup_ref = vup;
        vright_ref = vfwd_ref.cross(vup);
        vright_ref.normalize();
    }

    // Calculate the actual vectors using the particle rotation.
    if (0 == pprt->rotate)
    {
        pinst->up = vup;
        pinst->right = vright;

        pinst->ref_up = vup_ref;
        pinst->ref_right = vright_ref;
    }
    else
    {
        TURN_T turn = TO_TURN(pprt->rotate);
        float cosval = turntocos[turn];
        float sinval = turntosin[turn];

        pinst->up = vup * cosval - vright * sinval;

        pinst->right = vup * sinval + vright * cosval;

        pinst->ref_up = vup_ref * cosval - vright_ref * sinval;

        pinst->ref_right = vup_ref * sinval + vright_ref * cosval;
    }

    // Calculate the billboard normal.
    pinst->nrm = pinst->right.cross(pinst->up);

    // Flip the normal so that the front front of the quad is toward the camera.
    if (vfwd.dot(pinst->nrm) < 0)
    {
        pinst->nrm *= -1;
    }

    // Now we have to calculate the mirror-like reflection of the particles.
    // This was a bit hard to figure. What happens is that the components of the
    // up and right vectors that are in the plane of the quad and closest to the world up are reversed.
    //
    // This is easy to think about in a couple of examples:
    // 1) If the quad is like a picture frame then whatever component (up or right)
    //    that actually points in the wodld up direction is reversed.
    //    This corresponds to the case where zdot == +/- 1 in the code below.
    //
    // 2) If the particle is like a rug, then basically nothing happens since
    //    neither the up or right vectors point in the world up direction.
    //    This corresponds to 0 == ndot in the code below.
    //
    // This process does not affect the normal the length of the vector, or the
    // direction of the normal to the quad.

    {
        // The normal sense of "up".
        fvec3_t world_up = fvec3_t(0, 0, 1);

        // The dot product between the normal vector and the world up vector:
        // The following statement could be optimized
        // since we know the only non-zero component of the world up vector is z.
        float ndot = pinst->nrm.dot(world_up);

        // Do nothing if the quad is basically horizontal.
        if (ndot < 1.0f - 1e-6)
        {
            // Do the right vector first.
            {
                // The dot product between the right vector and the world up:
                // The following statement could be optimized
                // since we know the only non-zero component of the world up vector is z.
                float zdot = pinst->ref_right.dot(world_up);

                if (std::abs(zdot) > 1e-6)
                {
                    float factor = zdot / (1.0f - ndot * ndot);
                    pinst->ref_right += ((pinst->nrm * ndot) - world_up) * 2.0f * factor;
                }
            }

            // Do the up vector second.
            {
                // The dot product between the up vector and the world up:
                // The following statement could be optimized
                // since we know the only non-zero component of the world up vector is z.
                float zdot = pinst->ref_up.dot(world_up);

                if (std::abs(zdot) > 1e-6)
                {
                    float factor = zdot / (1.0f - ndot * ndot);
                    pinst->ref_up += (pinst->nrm * ndot - world_up) * 2.0f * factor;
                }
            }
        }
    }

    // Set some particle dependent properties.
    pinst->scale = pprt->getScale();
    pinst->size = FP8_TO_FLOAT(pprt->size) * pinst->scale;

    // This instance is now completely valid.
    pinst->valid = true;
    pinst->ref_valid = true;

    return gfx_success;
}

fmat_4x4_t prt_instance_make_matrix(prt_instance_t *pinst)
{
    fmat_4x4_t mat = fmat_4x4_t::identity();

    mat(1, 0) = -pinst->up[kX];
    mat(1, 1) = -pinst->up[kY];
    mat(1, 2) = -pinst->up[kZ];

    mat(0, 0) = pinst->right[kX];
    mat(0, 1) = pinst->right[kY];
    mat(0, 2) = pinst->right[kZ];

    mat(2, 0) = pinst->nrm[kX];
    mat(2, 1) = pinst->nrm[kY];
    mat(2, 2) = pinst->nrm[kZ];

    return mat;
}

gfx_rv prt_instance_update_lighting(prt_instance_t *pinst, Ego::Particle *pprt, Uint8 trans, bool do_lighting)
{
    if (!pinst)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL instance");
        return gfx_error;
    }
    if (!pprt)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "NULL particle");
        return gfx_error;
    }

    // To make life easier
    Uint32 alpha = trans;

    // interpolate the lighting for the origin of the object
    lighting_cache_t global_light;
    grid_lighting_interpolate(_currentModule->getMeshPointer(), &global_light, fvec2_t(pinst->pos[kX], pinst->pos[kY]));

    // rotate the lighting data to body_centered coordinates
    fmat_4x4_t mat = prt_instance_make_matrix(pinst);
    lighting_cache_t loc_light;
    lighting_project_cache(&loc_light, &global_light, mat);

    // determine the normal dependent amount of light
    float amb, dir;
    lighting_evaluate_cache(&loc_light, pinst->nrm, pinst->pos[kZ], _currentModule->getMeshPointer()->tmem.bbox, &amb, &dir);

    // LIGHT-blended sprites automatically glow. ALPHA-blended and SOLID
    // sprites need to convert the light channel into additional alpha
    // lighting to make them "glow"
    Sint16 self_light = 0;
    if (SPRITE_LIGHT != pinst->type)
    {
        self_light = (255 == pinst->light) ? 0 : pinst->light;
    }

    // determine the ambient lighting
    pinst->famb = 0.9f * pinst->famb + 0.1f * (self_light + amb);
    pinst->fdir = 0.9f * pinst->fdir + 0.1f * dir;

    // determine the overall lighting
    pinst->fintens = pinst->fdir * INV_FF;
    if (do_lighting)
    {
        pinst->fintens += pinst->famb * INV_FF;
    }
    pinst->fintens = CLIP(pinst->fintens, 0.0f, 1.0f);

    // determine the alpha component
    pinst->falpha = (alpha * INV_FF) * (pinst->alpha * INV_FF);
    pinst->falpha = CLIP(pinst->falpha, 0.0f, 1.0f);

    return gfx_success;
}

gfx_rv prt_instance_update(Camera& camera, const PRT_REF particle, Uint8 trans, bool do_lighting)
{
    const std::shared_ptr<Ego::Particle> &pprt = ParticleHandler::get()[particle];
    if(!pprt) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, particle, "invalid particle");
        return gfx_error;
    }

    prt_instance_t *pinst = &(pprt->inst);

    // assume the best
    gfx_rv retval = gfx_success;

    // make sure that the vertices are interpolated
    if (gfx_error == prt_instance_update_vertices(camera, pinst, pprt.get()))
    {
        retval = gfx_error;
    }

    // do the lighting
    if (gfx_error == prt_instance_update_lighting(pinst, pprt.get(), trans, do_lighting))
    {
        retval = gfx_error;
    }

    return retval;
}

void render_prt_bbox(prt_bundle_t *pbdl_prt)
{
    if (!pbdl_prt)
    {
        return;
    }
    Ego::Particle *loc_pprt = pbdl_prt->_prt_ptr;
    if (!loc_pprt || loc_pprt->isTerminated())
    {
        return;
    }
    
    std::shared_ptr<pip_t> loc_ppip = pbdl_prt->_pip_ptr;

    // only draw bullets
    //if ( 50 != loc_ppip->vel_hrz_pair.base ) return;

    // draw the object bounding box as a part of the graphics debug mode F7
    if ((egoboo_config_t::get().debug_developerMode_enable.getValue() && SDL_KEYDOWN(keyb, SDLK_F7)))
    {
        // copy the bounding volume
        oct_bb_t tmp_bb = loc_pprt->prt_max_cv;

        // determine the expanded collision volumes for both objects
        oct_bb_t exp_bb;
        phys_expand_oct_bb(tmp_bb, loc_pprt->vel, 0, 1, exp_bb);

        // shift the source bounding boxes to be centered on the given positions
        oct_bb_t loc_bb;
        oct_bb_t::translate(exp_bb, loc_pprt->pos, loc_bb);

        oglx_texture_t::bind(nullptr);
        Ego::Renderer::get().setColour(Ego::Math::Colour4f::white());
        render_oct_bb(&loc_bb, true, true);
    }
}
