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

/// @file game/lighting.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_lighting_cache_base;
typedef struct s_lighting_cache_base lighting_cache_base_t;

struct s_lighting_cache;
typedef struct s_lighting_cache lighting_cache_t;

struct s_dynalight_data;
typedef struct s_dynalight_data dynalight_data_t;

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

void lighting_vector_evaluate( const lighting_vector_t lvec, const fvec3_t& nrm, float * direct, float * amb );
void lighting_vector_sum( lighting_vector_t lvec, const fvec3_t& nrm, const float direct, const float ambient );

//--------------------------------------------------------------------------------------------
struct s_lighting_cache_base
{
    float             max_light;  ///< max amplitude of direct light
    float             max_delta;  ///< max change in the light amplitude
    lighting_vector_t lighting;   ///< light from +x,-x, +y,-y, +z,-z, ambient
};

lighting_cache_base_t * lighting_cache_base_init( lighting_cache_base_t * pdata );
bool                  lighting_cache_base_max_light( lighting_cache_base_t * cache );
bool                  lighting_cache_base_blend( lighting_cache_base_t * cache, const lighting_cache_base_t * cnew, float keep );

//--------------------------------------------------------------------------------------------
struct s_lighting_cache
{
    float                 max_light;              ///< max amplitude of direct light
    float                 max_delta;              ///< max change in amplitude of all light

    lighting_cache_base_t low;
    lighting_cache_base_t hgh;
};

lighting_cache_t * lighting_cache_init( lighting_cache_t * pdata );
bool             lighting_cache_max_light( lighting_cache_t * cache );
bool             lighting_cache_blend( lighting_cache_t * cache, lighting_cache_t * cnew, float keep );

//--------------------------------------------------------------------------------------------
#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define TOTAL_MAX_DYNA                    64          // Absolute max number of dynamic lights

/// A definition of a single in-game dynamic light
struct s_dynalight_data
{
    float   distance;      ///< The distance from the center of the camera view
    fvec3_t pos;           ///< Light position
    float   level;         ///< Light intensity
    float   falloff;       ///< Light radius
};

dynalight_data_t * dynalight_data__init( dynalight_data_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern float   light_a,
               light_d;
extern fvec3_t light_nrm;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool lighting_project_cache( lighting_cache_t * dst, const lighting_cache_t * src, const fmat_4x4_base_t mat );
bool lighting_cache_interpolate( lighting_cache_t * dst, const lighting_cache_t * src[], const float u, const float v );
float lighting_cache_test( const lighting_cache_t * src[], const float u, const float v, float * low_max_diff, float * hgh_max_diff );

float lighting_evaluate_cache( const lighting_cache_t * src, const fvec3_t& nrm, const float z, const aabb_t bbox, float * light_amb, float * light_dir );

bool sum_dyna_lighting( const dynalight_data_t * pdyna, lighting_vector_t lighting, const fvec3_t& nrm );

/// @author BB
/// @details In the Aaron's lighting, the falloff function was
///                  light = (255 - r^2 / falloff) / 255.0f
///              this has a definite max radius for the light, rmax = sqrt(falloff*255),
///              which was good because we could have a definite range for a given light
///
///              This is not ideal because the light cuts off too abruptly. The new form of the
///              function is (in semi-maple notation)
///
///              f(n,r) = integral( (1+y)^n * y * (1-y)^n, y = -1 .. r )
///
///              this has the advantage that it forms a bell-shaped curve that approaches 0 smoothly
///              at r = -1 and r = 1. The lowest order term will always be quadratic in r, just like
///              Aaron's function. To eliminate terms like r^4 and higher order even terms, you can
///              various f(n,r) with different n's. But combining terms with larger and larger
///              n means that the left-over terms that make the function approach zero smoothly
///              will have higher and higher powers of r (more expensive) and the cutoff will
///              be sharper and sharper (which is against the whole point of this type of function).
///
///              Eliminating just the r^4 term gives the function
///                  f(y) = 1 - y^2 * ( 3.0f - y^4 ) / 2
///              to make it match Aaron's function best, you have to scale the function by
///                  y^2 = r^2 * 2 / 765 / falloff
///
///              I have previously tried rational polynomial functions like
///                  f(r) = k0 / (1 + k1 * r^2 ) + k2 / (1 + k3 * r^4 )
///              where the second term is to cancel make the function behave like Aaron's
///              at small r, and to make the function approximate same "size" of lighting area
///              as Aarons. An added benefit is that this function automatically has the right
///              "physics" behavior at large distances (falls off like 1/r^2). But that is the
///              exact problem because the infinite range means that it can potentally affect
///              the entire mesh, causing problems with computing a large number of lights
float  dyna_lighting_intensity( const dynalight_data_t * pdyna, const fvec3_t& diff );
