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

/// @file egolib/game/graphic_prt.h

#pragma once

#include "egolib/game/egoboo.h"

//--------------------------------------------------------------------------------------------

// Forward declaration
class Camera;
namespace Ego {
class Particle;
namespace Graphics { 
struct ParticleGraphics;
class ObjectGraphics;
} }

//--------------------------------------------------------------------------------------------


struct ParticleGraphicsRenderer
{
    // dynamically calculate the particle texture coordinates
    // support for computing the particle texture coordinates on the fly.
    // currently, there ate two texture types: TX_PARTICLE_TRANS and TX_PARTICLE_LIGHT
    static float CALCULATE_PRT_U0(const Ego::Texture& texture, int CNT);
    static float CALCULATE_PRT_U1(const Ego::Texture& texture, int CNT);
    static float CALCULATE_PRT_V0(const Ego::Texture& texture, int CNT);
    static float CALCULATE_PRT_V1(const Ego::Texture& texture, int CNT);
    static gfx_rv render_one_prt_solid(const ParticleRef iprt);
    static gfx_rv render_one_prt_trans(const ParticleRef iprt);
    static gfx_rv render_one_prt_ref(const ParticleRef iprt);
    static void render_all_prt_bbox();
    static void render_prt_bbox(const std::shared_ptr<Ego::Particle> &bdl_prt);
    static void render_all_prt_attachment();
    static void prt_draw_attached_point(const std::shared_ptr<Ego::Particle> &bdl_prt);
private:
    static void draw_one_attachment_point(Ego::Graphics::ObjectGraphics& inst, int vrt_offset);
    static void calc_billboard_verts(const Ego::Texture& texture, Ego::VertexBuffer& vb, Ego::Graphics::ParticleGraphics& pinst, float size, bool do_reflect);
};

