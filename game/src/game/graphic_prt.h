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

//--------------------------------------------------------------------------------------------

// dynamically calculate the particle texture coordinates
// support for computing the particle texture coordinates on the fly.
// currently, there ate two texture types: TX_PARTICLE_TRANS and TX_PARTICLE_LIGHT
extern int ptex_w[2];
extern int ptex_h[2];
extern float ptex_wscale[2];
extern float ptex_hscale[2];
void prt_set_texture_params(const std::shared_ptr<const Ego::Texture>& texture, uint8_t type);
float CALCULATE_PRT_U0(int IDX, int CNT);
float CALCULATE_PRT_U1(int IDX, int CNT);
float CALCULATE_PRT_V0(int IDX, int CNT);
float CALCULATE_PRT_V1(int IDX, int CNT);

/// All the data necessary to display a particle.
struct prt_instance_t
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

    prt_instance_t() :
        valid(false),
        indolist(false),

        // basic info
        type(0),
        image_ref(0),
        alpha(0.0f),
        light(0),

        // position info
        pos(Vector3f::zero()),
        size(0.0f),
        scale(0.0f),

        // billboard info
        orientation(prt_ori_t::ORIENTATION_B),
        up(Vector3f::zero()),
        right(Vector3f::zero()),
        nrm(Vector3f::zero()),

        // lighting info
        famb(0.0f),
        fdir(0.0f),

        fintens(0.0f),
        falpha(0.0f),

        // pre-compute some values for the reflected particle posisions
        ref_valid(false),
        ref_up(Vector3f::zero()),
        ref_right(Vector3f::zero()),
        ref_pos(Vector3f::zero())
    {
        //ctor   
    }

    void reset()
    {
        valid = false;

        // graphical optimizations
        indolist = false;

        // basic info
        type = 0;
        image_ref = 0;
        alpha = 0.0f;
        light = 0;

        // position info
        pos = Vector3f::zero();
        size = 0.0f;
        scale = 0.0f;

        // billboard info
        orientation = prt_ori_t::ORIENTATION_B;
        up = Vector3f::zero();
        right = Vector3f::zero();
        nrm = Vector3f::zero();

        // lighting info
        famb = 0.0f;
        fdir = 0.0f;

        fintens = 0.0f;
        falpha = 0.0f;

        // pre-compute some values for the reflected particle posisions
        ref_valid = false;
        ref_up = Vector3f::zero();
        ref_right = Vector3f::zero();
        ref_pos = Vector3f::zero();
    }

    static gfx_rv update_vertices(prt_instance_t& inst, Camera& camera, Ego::Particle *pprt);
    static Matrix4f4f make_matrix(prt_instance_t& inst);
    static gfx_rv update_lighting(prt_instance_t& inst, Ego::Particle *pprt, Uint8 trans, bool do_lighting);
};

gfx_rv render_one_prt_solid(const ParticleRef iprt);
gfx_rv render_one_prt_trans(const ParticleRef iprt);
gfx_rv render_one_prt_ref(const ParticleRef iprt);
void render_all_prt_bbox();
void render_all_prt_attachment();
gfx_rv update_all_prt_instance(Camera& cam);

