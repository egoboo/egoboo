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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Call this every time the camera moves to update the frustum
static void frustum_calculate(frustum_base_t pf, const fmat_4x4_t& proj, const fmat_4x4_t& modl);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void frustum_calculate(frustum_base_t planes, const fmat_4x4_t& projection, const fmat_4x4_t& view)
{
    float clip[16];        // This will hold the clipping planes

	//fmat_4x4_t _clip = projection * view;
    mat_Multiply(clip, projection.v, view.v);

    // This will extract the FRUST_PLANE_RIGHT side of the frustum
    planes[FRUST_PLANE_RIGHT][kX] = clip[ 3] - clip[ 0];
    planes[FRUST_PLANE_RIGHT][kY] = clip[ 7] - clip[ 4];
    planes[FRUST_PLANE_RIGHT][kZ] = clip[11] - clip[ 8];
    planes[FRUST_PLANE_RIGHT][kW] = clip[15] - clip[12];
    plane_base_normalize(planes + FRUST_PLANE_RIGHT);

    // This will extract the FRUST_PLANE_LEFT side of the frustum
    planes[FRUST_PLANE_LEFT][kX] = clip[ 3] + clip[ 0];
    planes[FRUST_PLANE_LEFT][kY] = clip[ 7] + clip[ 4];
    planes[FRUST_PLANE_LEFT][kZ] = clip[11] + clip[ 8];
    planes[FRUST_PLANE_LEFT][kW] = clip[15] + clip[12];
    plane_base_normalize(planes + FRUST_PLANE_LEFT);

    // This will extract the FRUST_PLANE_BOTTOM side of the frustum
    planes[FRUST_PLANE_BOTTOM][kX] = clip[ 3] + clip[ 1];
    planes[FRUST_PLANE_BOTTOM][kY] = clip[ 7] + clip[ 5];
    planes[FRUST_PLANE_BOTTOM][kZ] = clip[11] + clip[ 9];
    planes[FRUST_PLANE_BOTTOM][kW] = clip[15] + clip[13];
    plane_base_normalize(planes + FRUST_PLANE_BOTTOM);

    // This will extract the FRUST_PLANE_TOP side of the frustum
    planes[FRUST_PLANE_TOP][kX] = clip[ 3] - clip[ 1];
    planes[FRUST_PLANE_TOP][kY] = clip[ 7] - clip[ 5];
    planes[FRUST_PLANE_TOP][kZ] = clip[11] - clip[ 9];
    planes[FRUST_PLANE_TOP][kW] = clip[15] - clip[13];
    plane_base_normalize(planes + FRUST_PLANE_TOP);

    // This will extract the FRUST_PLANE_BACK side of the frustum
    planes[FRUST_PLANE_BACK][kX] = clip[ 3] - clip[ 2];
    planes[FRUST_PLANE_BACK][kY] = clip[ 7] - clip[ 6];
    planes[FRUST_PLANE_BACK][kZ] = clip[11] - clip[10];
    planes[FRUST_PLANE_BACK][kW] = clip[15] - clip[14];
    plane_base_normalize(planes + FRUST_PLANE_BACK);

    // This will extract the FRUST_PLANE_FRONT side of the frustum
    planes[FRUST_PLANE_FRONT][kX] = clip[ 3] + clip[ 2];
    planes[FRUST_PLANE_FRONT][kY] = clip[ 7] + clip[ 6];
    planes[FRUST_PLANE_FRONT][kZ] = clip[11] + clip[10];
    planes[FRUST_PLANE_FRONT][kW] = clip[15] + clip[14];
    plane_base_normalize(planes + FRUST_PLANE_FRONT);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

egolib_rv egolib_frustum_calculate(egolib_frustum_t *self, const fmat_4x4_t& projection, const fmat_4x4_t& view)
{
    fvec3_t pt1;
    fvec3_t vlook, vfar;

    if (!self) return rv_error;

    //---- construct the basic frustum
    {
        frustum_calculate(self->data, projection, view);
    }

    //---- construct the camera location
    {
        // the origin of the frustum (should be the camera position)
        three_plane_intersection(self->origin, self->data[FRUST_PLANE_RIGHT],
			                                   self->data[FRUST_PLANE_LEFT],
											   self->data[FRUST_PLANE_BOTTOM]);
    }

    //---- construct the sphere
    {
        // extract the view direction from the view matrix
        mat_getCamForward(view, vlook);

        // one far corner of the frustum
        three_plane_intersection(pt1, self->data[FRUST_PLANE_TOP],
			                          self->data[FRUST_PLANE_RIGHT],
									  self->data[FRUST_PLANE_BACK]);

        // get the distance from the origin to the far plane
        float dist = plane_point_distance(self->data[FRUST_PLANE_BACK], self->origin.v);

        // calculate the center of the sphere
		self->sphere.origin = self->origin + vlook * (dist * 0.5f);

        // the vector from p1 to the center of the sphere
        fvec3_t vDiff = self->sphere.origin - pt1;

        // the radius becomes the length of this vector
		self->sphere.radius = vDiff.length();
    }

    //---- construct the cone
    {
        float cos_half_fov;

		self->cone.origin = self->origin;
		self->cone.axis = vlook;

        // the vector from the origin to the far corner
        vfar = pt1 - self->cone.origin;

        // the cosine between the view direction and the
        cos_half_fov = vfar.dot(vlook) / vfar.length();

        // calculate the required trig functions
        self->cone.cos_2 = cos_half_fov * cos_half_fov;
        self->cone.sin_2 = 1.0f - self->cone.cos_2;
        self->cone.inv_sin = 1.0f / std::sqrt(self->cone.sin_2);
    }

    return rv_success;
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

//--------------------------------------------------------------------------------------------
geometry_rv egolib_frustum_t::intersects_point(const fvec3_t& point, const bool doEnds) const
{
	// Handle optional parameters.
	int i_stt, i_end;
	if (doEnds)
	{
		i_stt = 0;
		i_end = FRUST_PLANE_END;
	}
	else
	{
		i_stt = 0;
		i_end = FRUST_SIDES_END;
	}

	// Assume the point is inside the frustum.
	bool inside = true;

	// Scan through the frustum's planes:
	for (int i = i_stt; i <= i_end; i++)
	{
		if (plane_point_distance(data[i], point.v) <= 0.0f)
		{
			inside = false;
			break;
		}
	}

	return inside ? geometry_inside : geometry_outside;
}

//--------------------------------------------------------------------------------------------
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
		i_end = FRUST_PLANE_END;
	}
	else
	{
		i_stt = 0;
		i_end = FRUST_SIDES_END;
	}

	// scan each plane
	for (int i = i_stt; i <= i_end; i++)
	{
		float dist = plane_point_distance(this->data[i], sphere.origin.v);

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

//--------------------------------------------------------------------------------------------
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
		i_end = FRUST_PLANE_END;
	}
	else
	{
		i_stt = 0;
		i_end = FRUST_SIDES_END;
	}

	for (int i = i_stt; i <= i_end; i++)
	{
		const plane_base_t *plane = this->data + i;

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

//--------------------------------------------------------------------------------------------
geometry_rv egolib_frustum_t::intersects_aabb(const fvec3_t& mins, const fvec3_t& maxs, const bool doEnds) const
{
	// Handle optional parameters.
	int i_stt, i_end;
	if (doEnds)
	{
		i_stt = 0;
		i_end = FRUST_PLANE_END;
	}
	else
	{
		i_stt = 0;
		i_end = FRUST_SIDES_END;
	}

	// Assume the AABB is inside the frustum.
	geometry_rv retval = geometry_inside;

	// scan through the planes until something happens
	int i;
	for (i = i_stt; i <= i_end; i++)
	{
		if (geometry_outside == plane_intersects_aabb_max(this->data[i], mins, maxs))
		{
			retval = geometry_outside;
			break;
		}

		if (geometry_outside == plane_intersects_aabb_min(this->data[i], mins, maxs))
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
			if (geometry_outside == plane_intersects_aabb_max(this->data[i], mins, maxs))
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