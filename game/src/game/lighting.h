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

#include "game/egoboo.h"

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

/**
 * @brief A lighting vector. Its components denote the directed and undirected (aka ambient) light
 * @todo Egoboo's lighting vectors do not denote the color of the light (yet).
 */
typedef std::array<float, LIGHTING_VEC_SIZE> LightingVector;

/**
 * @brief Evaluate a lighting vector w.r.t. to a given surface normal.
 * @param lvec the lighting vector
 * @param nrm the surface normal
 * @param [out] direct the resulting directed lighting
 * @param [out] amb the resulting undirected (aka ambient) lighting
 * @todo Properly describe what the resulting directed and undirected lighting are.
 */
void lighting_vector_evaluate( const LightingVector& lvec, const Vector3f& nrm, float& direct, float& amb);
void lighting_vector_sum(LightingVector& lvec, const Vector3f& nrm, const float direct, const float ambient);

//--------------------------------------------------------------------------------------------
struct lighting_cache_base_t
{
    lighting_cache_base_t() :
        _max_light(0.0f),
        _max_delta(0.0f),
        _lighting{}
    {
        _lighting.fill(0.0f);
    }

    float          _max_light;  ///< max amplitude of direct light
    float          _max_delta;  ///< max change in the light amplitude
    LightingVector _lighting;   ///< light from +x,-x, +y,-y, +z,-z, ambient

	void init();
    /// Recompute the maximal amplitude of the directed light.
	void max_light();
	// Blend another cache into this cache.
	static void blend(lighting_cache_base_t& self, const lighting_cache_base_t& other, float keep);
	static float evaluate(const lighting_cache_base_t& self, const Vector3f& nrm, float& amb);
};



//--------------------------------------------------------------------------------------------
struct lighting_cache_t
{
    lighting_cache_t() :
        _max_light(0.0f),
        _max_delta(0.0f),
        low(),
        hgh()
    {

    }

    float                 _max_light;              ///< max amplitude of direct light
    float                 _max_delta;              ///< max change in amplitude of all light

    lighting_cache_base_t low;
    lighting_cache_base_t hgh;

	void init();
    /// Recompute the maximal amplitude of the directed light.
	void max_light();
	/// Blend another cache into this cache.
	static void blend(lighting_cache_t& self, lighting_cache_t& other, float keep);

	static void lighting_project_cache(lighting_cache_t& dst, const lighting_cache_t& src, const Matrix4f4f& mat);
	static bool lighting_cache_interpolate(lighting_cache_t& dst, const std::array<const lighting_cache_t *, 4>& src, const float u, const float v);
	static float lighting_evaluate_cache(const lighting_cache_t& src, const Vector3f& nrm, const float z, const AABB3f& bbox, float *light_amb, float *light_dir);

};

//--------------------------------------------------------------------------------------------
#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define TOTAL_MAX_DYNA                    64          // Absolute max number of dynamic lights

/// A definition of a single in-game dynamic light
struct dynalight_data_t
{
    float    distance;      ///< The distance from the center of the camera view
	Vector3f pos;           ///< Light position
    float    level;         ///< Light intensity
    float    falloff;       ///< Light radius

	static void init(dynalight_data_t& self);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern float    light_a,
                light_d;
extern Vector3f light_nrm;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float lighting_cache_test( const lighting_cache_t * src[], const float u, const float v, float& low_max_diff, float& hgh_max_diff );


bool sum_dyna_lighting( const dynalight_data_t * pdyna, LightingVector& lighting, const Vector3f& nrm );

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
float  dyna_lighting_intensity( const dynalight_data_t * pdyna, const Vector3f& diff );
