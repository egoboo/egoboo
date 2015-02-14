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

/// @file egolib/frustum.c
/// @brief a frustum culling package
/// @details The frustum_calculate() function was inspired by Ben Humphrey (DigiBen at http://www.gametutorials.com), who was inspired by
///          Mark Morely (through the now vanished tutorial at http://www.markmorley.com/opengl/frustumculling.html)

#include "egolib/frustum.h"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/_math.h"

void egolib_frustum_t::calculate(base_t planes, const fmat_4x4_t& projection, const fmat_4x4_t& view)
{
    float clip[16];        // This will hold the clipping planes

	//fmat_4x4_t _clip = projection * view;
    mat_Multiply(clip, projection.v, view.v);

    // This will extract the right side of the frustum.
    planes[Planes::RIGHT][kX] = clip[ 3] - clip[ 0];
    planes[Planes::RIGHT][kY] = clip[ 7] - clip[ 4];
    planes[Planes::RIGHT][kZ] = clip[11] - clip[ 8];
    planes[Planes::RIGHT][kW] = clip[15] - clip[12];
    plane_base_normalize(planes + Planes::RIGHT);

    // This will extract the left side of the frustum.
    planes[Planes::LEFT][kX] = clip[ 3] + clip[ 0];
    planes[Planes::LEFT][kY] = clip[ 7] + clip[ 4];
    planes[Planes::LEFT][kZ] = clip[11] + clip[ 8];
    planes[Planes::LEFT][kW] = clip[15] + clip[12];
    plane_base_normalize(planes + Planes::LEFT);

    // This will extract the bottom side of the frustum.
    planes[Planes::BOTTOM][kX] = clip[ 3] + clip[ 1];
    planes[Planes::BOTTOM][kY] = clip[ 7] + clip[ 5];
    planes[Planes::BOTTOM][kZ] = clip[11] + clip[ 9];
    planes[Planes::BOTTOM][kW] = clip[15] + clip[13];
    plane_base_normalize(planes + Planes::BOTTOM);

    // This will extract the top side of the frustum.
    planes[Planes::TOP][kX] = clip[ 3] - clip[ 1];
    planes[Planes::TOP][kY] = clip[ 7] - clip[ 5];
    planes[Planes::TOP][kZ] = clip[11] - clip[ 9];
    planes[Planes::TOP][kW] = clip[15] - clip[13];
    plane_base_normalize(planes + Planes::TOP);

    // This will extract the back side of the frustum.
    planes[Planes::BACK][kX] = clip[ 3] - clip[ 2];
    planes[Planes::BACK][kY] = clip[ 7] - clip[ 6];
    planes[Planes::BACK][kZ] = clip[11] - clip[10];
    planes[Planes::BACK][kW] = clip[15] - clip[14];
    plane_base_normalize(planes + Planes::BACK);

    // This will extract the front side of the frustum.
    planes[Planes::FRONT][kX] = clip[ 3] + clip[ 2];
    planes[Planes::FRONT][kY] = clip[ 7] + clip[ 6];
    planes[Planes::FRONT][kZ] = clip[11] + clip[10];
    planes[Planes::FRONT][kW] = clip[15] + clip[14];
    plane_base_normalize(planes + Planes::FRONT);
}

void egolib_frustum_t::calculate(const fmat_4x4_t& projection, const fmat_4x4_t& view)
{
    fvec3_t pt1;
    fvec3_t vlook, vfar;

    // Compute the 6 frustum planes.
    {
        egolib_frustum_t::calculate(_planes, projection, view);
    }

    // Compute the origin.
    {
        // the origin of the frustum (should be the camera position)
        three_plane_intersection(_origin, _planes[Planes::RIGHT],
			                              _planes[Planes::LEFT],
									      _planes[Planes::BOTTOM]);
    }

    // Compute the sphere.
    {
        // extract the view direction from the view matrix
        mat_getCamForward(view, vlook);

        // one far corner of the frustum
        three_plane_intersection(pt1, _planes[Planes::TOP],
			                          _planes[Planes::RIGHT],
									  _planes[Planes::BACK]);

        // get the distance from the origin to the far plane
        float dist = plane_point_distance(_planes[Planes::BACK], _origin.v);

        // calculate the center of the sphere
		_sphere.origin = _origin + vlook * (dist * 0.5f);

        // the vector from p1 to the center of the sphere
        fvec3_t vDiff = _sphere.origin - pt1;

        // the radius becomes the length of this vector
		_sphere.radius = vDiff.length();
    }

    // Compute the cone.
    {
        float cos_half_fov;

		_cone.origin = _origin;
		_cone.axis = vlook;

        // the vector from the origin to the far corner
        vfar = pt1 - _cone.origin;

        // the cosine between the view direction and the
        cos_half_fov = vfar.dot(vlook) / vfar.length();

        // calculate the required trig functions
        _cone.cos_2 = cos_half_fov * cos_half_fov;
        _cone.sin_2 = 1.0f - _cone.cos_2;
        _cone.inv_sin = 1.0f / std::sqrt(_cone.sin_2);
    }
}

geometry_rv egolib_frustum_t::intersects_bv(const bv_t *bv,bool doEnds) const
{
	// Validate arguments.
	if (nullptr == bv)
	{
		return geometry_error;
	}
	return intersects_aabb(bv->aabb.mins, bv->aabb.maxs, doEnds);
}

geometry_rv egolib_frustum_t::intersects_point(const fvec3_t& point, const bool doEnds) const
{
	// Handle optional parameters.
	int i_stt, i_end;
	if (doEnds)
	{
		i_stt = 0;
		i_end = Planes::END;
	}
	else
	{
		i_stt = 0;
		i_end = Planes::SIDES_END;
	}

	// Assume the point is inside the frustum.
	bool inside = true;

	// Scan through the frustum's planes:
	for (int i = i_stt; i <= i_end; i++)
	{
		if (plane_point_distance(_planes[i], point.v) <= 0.0f)
		{
			inside = false;
			break;
		}
	}

	return inside ? geometry_inside : geometry_outside;
}

geometry_rv egolib_frustum_t::intersects_sphere(const sphere_t& sphere, const bool doEnds) const
{
	/// @todo The radius of a sphere shall preserve the invariant to be non-negative.
	/// The test below would then reduce to radius == 0.0f. In that case, the simple
	/// frustum - center test is sufficient.
	if (sphere.radius <= 0.0f)
	{
		return this->intersects_point(sphere.origin, doEnds);
	}

	// Assume the sphere is completely inside the frustum.
	geometry_rv retval = geometry_inside;

	// Handle optional parameters.
	int i_stt, i_end;
	if (doEnds)
	{
		i_stt = 0;
		i_end = Planes::END;
	}
	else
	{
		i_stt = 0;
		i_end = Planes::SIDES_END;
	}

	// scan each plane
	for (int i = i_stt; i <= i_end; i++)
	{
		float dist = plane_point_distance(_planes[i], sphere.origin.v);

		// If the sphere is completely behind the current plane, it is outside the frustum.
		if (dist <= -sphere.radius)
		{
			retval = geometry_outside;
			break;
		}
		// If it is not completely in front of the current plane, it intersects the frustum.
		else if (dist < sphere.radius)
		{
			retval = geometry_intersect;
		}
	}

	return retval;
}

geometry_rv egolib_frustum_t::intersects_cube(const fvec3_t& center, const float size, const bool doEnds) const
{
	fvec3_base_t vmin, vmax;

	// Assume the cube is inside the frustum.
	geometry_rv retval = geometry_inside;

	// Handle optional parameters.
	int i_stt, i_end;
	if (doEnds)
	{
		i_stt = 0;
		i_end = Planes::END;
	}
	else
	{
		i_stt = 0;
		i_end = Planes::SIDES_END;
	}

	for (int i = i_stt; i <= i_end; i++)
	{
		const plane_base_t *plane = _planes + i;

		// find the most-positive and most-negative points of the aabb
		for (int j = 0; j < 3; j++)
		{
			if ((*plane)[j] > 0.0f)
			{
				vmin[j] = center[j] - size;
				vmax[j] = center[j] + size;
			}
			else
			{
				vmin[j] = center[j] + size;
				vmax[j] = center[j] - size;
			}
		}

		// If the cube is completely on the negative half-space of the plane,
		// then it is completely outside.
		/// @todo This is wrong!
		if (plane_point_distance(*plane, vmin) > 0.0f)
		{
			retval = geometry_outside;
			break;
		}

		// if vmin is inside and vmax is outside, then it is not completely inside
		/// @todo This is wrong.
		if (plane_point_distance(*plane, vmax) >= 0.0f)
		{
			retval = geometry_intersect;
		}
	}

	return retval;
}

geometry_rv egolib_frustum_t::intersects_aabb(const fvec3_t& mins, const fvec3_t& maxs, const bool doEnds) const
{
	// Handle optional parameters.
	int i_stt, i_end;
	if (doEnds)
	{
		i_stt = 0;
		i_end = Planes::END;
	}
	else
	{
		i_stt = 0;
		i_end = Planes::SIDES_END;
	}

	// Assume the AABB is inside the frustum.
	geometry_rv retval = geometry_inside;

	// scan through the planes until something happens
	int i;
	for (i = i_stt; i <= i_end; i++)
	{
		if (geometry_outside == plane_intersects_aabb_max(_planes[i], mins, maxs))
		{
			retval = geometry_outside;
			break;
		}

		if (geometry_outside == plane_intersects_aabb_min(_planes[i], mins, maxs))
		{
			retval = geometry_intersect;
			break;
		}
	}

	// continue on if there is something to do
	if (geometry_intersect == retval)
	{
		// If we are in geometry_intersect mode, we only need to check for
		// the geometry_outside condition.

		// This eliminates a geometry_inside == retval test in every iteration of the loop
		for ( /* nothing */; i <= i_end; i++)
		{
			if (geometry_outside == plane_intersects_aabb_max(_planes[i], mins, maxs))
			{
				retval = geometry_outside;
				break;
			}
		}
	}

	return retval;
}

bool egolib_frustum_t::intersects_oct(const oct_bb_t *oct, const bool doEnds) const
{
	if (nullptr == oct)
	{
		return false;
	}
	bool retval = false;
	aabb_t aabb;
	if (aabb_from_oct_bb(&aabb, oct))
	{
		geometry_rv frustum_rv = this->intersects_aabb(aabb.mins, aabb.maxs, doEnds);

		retval = (frustum_rv > geometry_outside);
	}

	return retval;
}
