#pragma once

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

/// @file lighting.h

#include "physics.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum
{
    LVEC_PX,               ///< light from +x
    LVEC_MX,               ///< light from -x

    LVEC_PY,               ///< light from +y
    LVEC_MY,               ///< light from -y

    LVEC_PZ,               ///< light from +z
    LVEC_MZ,               ///< light from -z

    LVEC_AMB,             ///< light from ambient

    LIGHTING_VEC_SIZE
};

typedef float lighting_vector_t[LIGHTING_VEC_SIZE];

void lighting_vector_evaluate( lighting_vector_t lvec, fvec3_base_t nrm, float * direct, float * amb );
void lighting_vector_sum( lighting_vector_t lvec, fvec3_base_t nrm, float direct, float ambient );

//--------------------------------------------------------------------------------------------
struct s_lighting_cache_base
{
    float             max_light;  ///< max amplitude of direct light
    float             max_delta;  ///< max change in the light amplitude
    lighting_vector_t lighting;   ///< light from +x,-x, +y,-y, +z,-z, ambient
};
typedef struct s_lighting_cache_base lighting_cache_base_t;

lighting_cache_base_t * lighting_cache_base_init( lighting_cache_base_t * pdata );
bool_t                  lighting_cache_base_max_light( lighting_cache_base_t * cache );
bool_t                  lighting_cache_base_blend( lighting_cache_base_t * cache, lighting_cache_base_t * cnew, float keep );

//--------------------------------------------------------------------------------------------
struct s_lighting_cache
{
    float                 max_light;              ///< max amplitude of direct light
    float                 max_delta;              ///< max change in amplitude of all light

    lighting_cache_base_t low;
    lighting_cache_base_t hgh;
};
typedef struct s_lighting_cache lighting_cache_t;

lighting_cache_t * lighting_cache_init( lighting_cache_t * pdata );
bool_t             lighting_cache_max_light( lighting_cache_t * cache );
bool_t             lighting_cache_blend( lighting_cache_t * cache, lighting_cache_t * cnew, float keep );
//--------------------------------------------------------------------------------------------
#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define TOTAL_MAX_DYNA                    64          // Absolute max number of dynamic lights

/// A definition of a single in-game dynamic light
struct s_dynalight
{
    float   distance;      ///< The distance from the center of the camera view
    fvec3_t pos;           ///< Light position
    float   level;         ///< Light intensity
    float   falloff;       ///< Light radius
};

typedef struct s_dynalight dynalight_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern float  light_a, light_d, light_nrm[3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t lighting_project_cache( lighting_cache_t * dst, lighting_cache_t * src, fmat_4x4_t mat );
bool_t lighting_cache_interpolate( lighting_cache_t * dst, lighting_cache_t * src[], float u, float v );
float lighting_cache_test( lighting_cache_t * src[], float u, float v, float * low_max_diff, float * hgh_max_diff );

float lighting_evaluate_cache( lighting_cache_t * src, fvec3_base_t nrm, float z, aabb_t bbox, float * light_amb, float * light_dir );

bool_t sum_dyna_lighting( dynalight_t * pdyna, lighting_vector_t lighting, fvec3_base_t nrm );
float  dyna_lighting_intensity( dynalight_t * pdyna, fvec3_base_t diff );