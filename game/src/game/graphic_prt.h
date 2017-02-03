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

/// @file game/graphic_prt.h

#pragma once

#include "game/egoboo.h"
#include "game/graphic.h"

//--------------------------------------------------------------------------------------------

// Forward declaration
struct prt_bundle_t;
namespace Ego { namespace Graphics { struct ObjectGraphics; } }

//--------------------------------------------------------------------------------------------

// dynamically calculate the particle texture coordinates
// support for computing the particle texture coordinates on the fly.
// currently, there ate two texture types: TX_PARTICLE_TRANS and TX_PARTICLE_LIGHT
void prt_set_texture_params(const std::shared_ptr<const Ego::Texture>& texture, uint8_t type);
float CALCULATE_PRT_U0(int IDX, int CNT);
float CALCULATE_PRT_U1(int IDX, int CNT);
float CALCULATE_PRT_V0(int IDX, int CNT);
float CALCULATE_PRT_V1(int IDX, int CNT);

namespace Ego {
namespace Graphics {

/// All the data necessary to display a particle.
struct ParticleGraphics
{
    bool valid;                ///< is the infor in this struct valid?

    // graphical optimizations
    bool         indolist;     ///< Has it been added yet?

    // basic info
    uint8_t  type;               ///< particle type
    uint32_t image_ref;          ///< which sub image within the texture?
    float    alpha;              ///< base alpha
    uint8_t  light;              ///< base self lighting

    // position info
	Vector3f  pos;
    float     size;
    float     scale;

    // billboard info
    prt_ori_t orientation;
	Vector3f  up;
	Vector3f  right;
	Vector3f  nrm;

    // lighting info
    float    famb;               ///< cached ambient light
    float    fdir;               ///< cached directional light

    float    fintens;            ///< current brightness
    float    falpha;             ///< current alpha

    // pre-compute some values for the reflected particle posisions
    bool ref_valid;
	Vector3f ref_up;
	Vector3f ref_right;
	Vector3f ref_pos;

    ParticleGraphics();
    void reset();
    static gfx_rv update(Camera& camera, const ParticleRef particle, Uint8 trans, bool do_lighting);
protected:
    static gfx_rv update_vertices(ParticleGraphics& inst, Camera& camera, Ego::Particle *pprt);
    static Matrix4f4f make_matrix(ParticleGraphics& inst);
    static gfx_rv update_lighting(ParticleGraphics& inst, Ego::Particle *pprt, Uint8 trans, bool do_lighting);
};

} // namespace Graphics
} // namespace Ego

struct ParticleGraphicsRenderer
{
    static gfx_rv render_one_prt_solid(const ParticleRef iprt);
    static gfx_rv render_one_prt_trans(const ParticleRef iprt);
    static gfx_rv render_one_prt_ref(const ParticleRef iprt);
    static void render_all_prt_bbox();
    static void render_prt_bbox(const std::shared_ptr<Ego::Particle> &bdl_prt);
    static void render_all_prt_attachment();
    static void prt_draw_attached_point(const std::shared_ptr<Ego::Particle> &bdl_prt);
private:
    static void draw_one_attachment_point(Ego::Graphics::ObjectGraphics& inst, int vrt_offset);
    static void calc_billboard_verts(Ego::VertexBuffer& vb, Ego::Graphics::ParticleGraphics& pinst, float size, bool do_reflect);
};

