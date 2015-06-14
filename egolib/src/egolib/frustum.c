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

#pragma push_macro("far")
#undef far
#pragma push_macro("FAR")
#undef FAR
#pragma push_macro("near")
#undef near
#pragma push_macro("NEAR")
#undef NEAR


egolib_frustum_t::egolib_frustum_t()
{
}

egolib_frustum_t::~egolib_frustum_t()
{
}

void egolib_frustum_t::calculatePlanes(const fmat_4x4_t& projection, const fmat_4x4_t& view, const fmat_4x4_t& world, plane_t& left, plane_t& right, plane_t& bottom, plane_t& top, plane_t& near, plane_t& far)
{
    calculatePlanes(projection * view * world, left, right, bottom, top, near, far);
}

void egolib_frustum_t::calculatePlanes(const fmat_4x4_t& projection, const fmat_4x4_t& view, plane_t& left, plane_t& right, plane_t& bottom, plane_t& top, plane_t& near, plane_t& far)
{
    calculatePlanes(projection * view, left, right, bottom, top, near, far);
}

void egolib_frustum_t::calculatePlanes(const fmat_4x4_t& matrix, plane_t& left, plane_t& right, plane_t& bottom, plane_t& top, plane_t& near, plane_t& far)
{
    float a, b, c, d;

    // Compute the left clipping plane of the frustum.
    a = matrix(3, 0) + matrix(0, 0);
    b = matrix(3, 1) + matrix(0, 1);
    c = matrix(3, 2) + matrix(0, 2);
    d = matrix(3, 3) + matrix(0, 3);
    left = plane_t(a, b, c, d);

    // Compute the right clipping plane of the frustum.
    a = matrix(3, 0) - matrix(0, 0);
    b = matrix(3, 1) - matrix(0, 1);
    c = matrix(3, 2) - matrix(0, 2);
    d = matrix(3, 3) - matrix(0, 3);
    right = plane_t(a, b, c, d);

    // Compute the bottom clipping plane of the frustum.
    a = matrix(3, 0) + matrix(1, 0);
    b = matrix(3, 1) + matrix(1, 1);
    c = matrix(3, 2) + matrix(1, 2);
    d = matrix(3, 3) + matrix(1, 3);
    bottom = plane_t(a, b, c, d);

    // Compute the top clipping plane of the frustum.
    a = matrix(3, 0) - matrix(1, 0);
    b = matrix(3, 1) - matrix(1, 1);
    c = matrix(3, 2) - matrix(1, 2);
    d = matrix(3, 3) - matrix(1, 3);
    top = plane_t(a, b, c, d);

    // Compute the near clipping plane of the frustum.
    a = matrix(3, 0) - matrix(2, 0);
    b = matrix(3, 1) - matrix(2, 1);
    c = matrix(3, 2) - matrix(2, 2);
    d = matrix(3, 3) - matrix(2, 3);
    near = plane_t(a, b, c, d);

    // Compute the far clipping plane of the frustum.
    a = matrix(3, 0) - matrix(2, 0);
    b = matrix(3, 1) - matrix(2, 1);
    c = matrix(3, 2) - matrix(2, 2);
    d = matrix(3, 3) - matrix(2, 3);
    far = plane_t(a, b, c, d);
}

void egolib_frustum_t::calculate(const fmat_4x4_t& projection, const fmat_4x4_t& view)
{
    fvec3_t pt1;
    fvec3_t vlook;

    // Compute the 6 frustum planes.
    {
        egolib_frustum_t::calculatePlanes(projection, view,
                                          _planes2[Planes::LEFT],
                                          _planes2[Planes::RIGHT],
                                          _planes2[Planes::BOTTOM],
                                          _planes2[Planes::TOP],
                                          _planes2[Planes::NEAR],
                                          _planes2[Planes::FAR]);
    }

    // Compute the origin.
    {
        // the origin of the frustum (should be the camera position)
        three_plane_intersection(_origin, _planes2[Planes::RIGHT],
			                              _planes2[Planes::LEFT],
									      _planes2[Planes::BOTTOM]);
    }

    // Compute the sphere.
    {
        // extract the view direction from the view matrix
        mat_getCamForward(view, vlook);

        // one far corner of the frustum
        three_plane_intersection(pt1, _planes2[Planes::TOP],
			                          _planes2[Planes::RIGHT],
									  _planes2[Planes::BACK]);

        // get the distance from the origin to the far plane
        float dist = _planes2[Planes::BACK].distance(_origin);

        // calculate the center of the sphere
        _sphere.setCenter(_origin + vlook * (dist * 0.5f));

        // the vector from p1 to the center of the sphere
        fvec3_t vDiff = _sphere.getCenter() - pt1;

        // the radius becomes the length of this vector
		_sphere.setRadius(vDiff.length());
    }
}

Ego::Math::Relation egolib_frustum_t::intersects_bv(const bv_t *bv, bool doEnds) const
{
	// Validate arguments.
	if (nullptr == bv)
	{
		return Ego::Math::Relation::error;
	}
	return intersects_aabb(bv->getAABB().getMin(), bv->getAABB().getMax(), doEnds);
}

Ego::Math::Relation egolib_frustum_t::intersects_point(const fvec3_t& point, const bool doEnds) const
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
		if (_planes2[i].distance(point) <= 0.0f)
		{
			inside = false;
			break;
		}
	}

	return inside ? Ego::Math::Relation::inside : Ego::Math::Relation::outside;
}

Ego::Math::Relation egolib_frustum_t::intersects_sphere(const sphere_t& sphere, const bool doEnds) const
{
	/// @todo The radius of a sphere shall preserve the invariant to be non-negative.
	/// The test below would then reduce to radius == 0.0f. In that case, the simple
	/// frustum - center test is sufficient.
	if (sphere.getRadius() == 0.0f)
	{
		return this->intersects_point(sphere.getCenter(), doEnds);
	}

	// Assume the sphere is completely inside the frustum.
	Ego::Math::Relation retval = Ego::Math::Relation::inside;

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
		float dist = _planes2[i].distance(sphere.getCenter());

		// If the sphere is completely behind the current plane, it is outside the frustum.
		if (dist <= -sphere.getRadius())
		{
			retval = Ego::Math::Relation::outside;
			break;
		}
		// If it is not completely in front of the current plane, it intersects the frustum.
		else if (dist < sphere.getRadius())
		{
			retval = Ego::Math::Relation::intersect;
		}
	}

	return retval;
}

Ego::Math::Relation egolib_frustum_t::intersects_cube(const fvec3_t& center, const float size, const bool doEnds) const
{
	// Assume the cube is inside the frustum.
	Ego::Math::Relation retval = Ego::Math::Relation::inside;

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
		const plane_t& plane = _planes2[i];
        fvec3_t vmin, vmax;
		// find the most-positive and most-negative points of the aabb
		for (int j = 0; j < 3; j++)
		{
			if (plane.getNormal()[j] > 0.0f)
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
		if (plane.distance(vmin) > 0.0f)
		{
			retval = Ego::Math::Relation::outside;
			break;
		}

		// if vmin is inside and vmax is outside, then it is not completely inside
		/// @todo This is wrong.
		if (plane.distance(vmax) >= 0.0f)
		{
			retval = Ego::Math::Relation::intersect;
		}
	}

	return retval;
}

Ego::Math::Relation egolib_frustum_t::intersects_aabb(const aabb_t& aabb, bool doEnds) const
{
    return intersects_aabb(aabb.getMin(), aabb.getMax(), doEnds);
}

Ego::Math::Relation egolib_frustum_t::intersects_aabb(const fvec3_t& mins, const fvec3_t& maxs, bool doEnds) const
{
    // Handle optional parameters.
    int i_stt = 0,
        i_end = doEnds ? Planes::END : Planes::SIDES_END;

    // Assume the AABB is inside the frustum.
	Ego::Math::Relation retval = Ego::Math::Relation::inside;

    // scan through the planes until something happens
    int i;
    for (i = i_stt; i <= i_end; i++)
    {
		if (Ego::Math::Relation::outside == plane_intersects_aabb_max(_planes2[i], mins, maxs))
        {
			retval = Ego::Math::Relation::outside;
            break;
        }

		if (Ego::Math::Relation::outside == plane_intersects_aabb_min(_planes2[i], mins, maxs))
        {
			retval = Ego::Math::Relation::intersect;
            break;
        }
    }

    // Continue on if there is something to do.
	if (Ego::Math::Relation::intersect == retval)
    {
        // If we are in geometry_intersect mode, we only need to check for
        // the geometry_outside condition.

        // This eliminates a geometry_inside == retval test in every iteration of the loop
        for ( /* nothing */; i <= i_end; i++)
        {
			if (Ego::Math::Relation::outside == plane_intersects_aabb_max(_planes2[i], mins, maxs))
            {
				retval = Ego::Math::Relation::outside;
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

	aabb_t aabb = oct->toAABB();
	Ego::Math::Relation frustum_rv = this->intersects_aabb(aabb.getMin(), aabb.getMax(), doEnds);

	return frustum_rv > Ego::Math::Relation::outside;
}

#pragma pop_macro("NEAR")
#pragma pop_macro("near")
#pragma pop_macro("FAR")
#pragma pop_macro("far")
