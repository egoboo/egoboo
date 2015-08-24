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

namespace Ego {
namespace Graphics {

#pragma push_macro("far")
#undef far
#pragma push_macro("FAR")
#undef FAR
#pragma push_macro("near")
#undef near
#pragma push_macro("NEAR")
#undef NEAR


Frustum::Frustum()
{
}

Frustum::~Frustum()
{
}

void Frustum::calculatePlanes(const Matrix4f4f& projection, const Matrix4f4f& view, const Matrix4f4f& world, Plane3f& left, Plane3f& right, Plane3f& bottom, Plane3f& top, Plane3f& near, Plane3f& far)
{
    calculatePlanes(projection * view * world, left, right, bottom, top, near, far);
}

void Frustum::calculatePlanes(const Matrix4f4f& projection, const Matrix4f4f& view, Plane3f& left, Plane3f& right, Plane3f& bottom, Plane3f& top, Plane3f& near, Plane3f& far)
{
    calculatePlanes(projection * view, left, right, bottom, top, near, far);
}

/**
 * @remark
 *	A point
 *	\f[
 *	\mathbf{p}=(x,y,z,w=1)
 *	\f]
 *	is transformed by a view to device (aka projection) or
 *	world to device (aka view + projection) transformation represented by a	\f$4 \times 4\f$
 *	matrix
 *	\f[
 *	\mathbf{T} =
 *	\left[\begin{matrix}
 *	r_0\\
 *	r_1\\
 *	r_2\\
 *	r_3\\
 *	\end{matrix}\right]
 *	\f]
 *	as follows
 *	\f[
 *	\mathbf{p}' = (x',y',z',w') = \mathbf{p} \cdot \mathbf{T} =
 *	\left[
 *	\mathbf{p} \cdot r_0
 *	\mathbf{p} \cdot r_1
 *	\mathbf{p} \cdot r_2
 *	\mathbf{p} \cdot r_3
 *	\right]
 *	\f]
 *	For not being clipped, the resulting point \f$\mathbf{p}'\f$ must fulfil the following
 *	properties
 *  <center><table>
 *  <tr><td>\f$-w' < x'\f$</td><td>inside half-space of left   clipping plane</td></tr>
 *  <tr><td>\f$x'  < w'\f$</td><td>inside half-space of right  clipping plane</td></tr>
 *  <tr><td>\f$-w' < y'\f$</td><td>inside half-space of bottom clipping plane</td></tr>
 *  <tr><td>\f$y' < w'\f$ </td><td>inside half-space of top    clipping plane</td></tr>
 *  <tr><td>\f$-w < z'\f$ </td><td>inside half-space of near   clipping plane</td></tr>
 *  <tr><td>\f$z' < w'\f$ </td><td>inside half-space of far    clipping plane</td></tr>
 *	</table></center>
 *  in order to be rendered.
 *@remark
 *	This relation allows for frustum plane extraction.
 *	Expanding those inequalities, we obtain
 *
 *	Left clipping plane:
 *	<table>
 *  <tr><td>\f{align*}
 *	               &\;-w' < x'
 *	\hookrightarrow&\;-(\mathbf{p} \cdot r_3) < (\mathbf{p} \cdot r_0)\\
 *  \hookrightarrow&\;0 < (\mathbf{p} \cdot r_0) + (\mathbf{p} \cdot r_3)\\ 
 *  \hookrightarrow&\;0 < \mathbf{p} \cdot (r_0 + r_3)\\
 *	\hookrightarrow&\;0 < (m_{0,0} + m_{3,0}) x + (m_{0,1} + m_{3,1}) y + (m_{0,2} + m_{3,2}) z + (m_{0,3} + m_{3,3}) w\\
 *	\hookrightarrow&\;0 < (m_{0,0} + m_{3,0}) x + (m_{0,1} + m_{3,1}) y + (m_{0,2} + m_{3,2}) z + (m_{0,3} + m_{3,3})
 *	\f}</td></tr>
 *	</table>
 *
 *	Right clipping plane
 *	<table>
 *  <tr><td>\f{align*}
 *	               &\;x' < w'
 *  \hookrightarrow&\;(\mathbf{p} \cdot r_0) < (\mathbf{p} \cdot r_3)\\
 *  \hookrightarrow&\;0 < (\mathbf{p} \cdot r_3) - (\mathbf{p} \cdot r_0)\\
 *	\hookrightarrow&\;0 < \mathbf{p} \cdot (r_3 - r_0)\\
 *	\hookrightarrow&\;0 < (m_{3,0} - m_{0,0}) x + (m_{3,1} - m_{0,1}) y + (m_{3,2} - m_{0,2}) z + (m_{3,3} - m_{0,3}) w\\
 *	\hookrightarrow&\;0 < (m_{3,0} - m_{0,0}) x + (m_{3,1} - m_{0,1}) y + (m_{3,2} - m_{0,2}) z + (m_{3,3} - m_{0,3})
 *	\f}</td></tr>
 *	</table>
 *
 *	Bottom clipping plane
 *	<table>
 *  <tr><td>\f{align*}
 *	               &\;-w' < y'\\
 *  \hookrightarrow&\;-(\mathbf{p} \cdot r_3) < (\mathbf{p} \cdot r_1)\\
 *  \hookrightarrow&\;0 < (\mathbf{p} \cdot r_1) + (\mathbf{p} \cdot r_3)\\
 *  \hookrightarrow&\;0 < \mathbf{p} + (r_1 + r_3)\\
 *  \hookrightarrow&\;0 < \mathbf{p} + (r_1 + r_3)\\
 *	\hookrightarrow&\;0 < (m_{1,0} + m_{3,0}) x + (m_{1,1} + m_{3,1}) y + (m_{1,2} + m_{3,2}) z + (m_{1,3} + m_{3,3}) w\\
 *	\hookrightarrow&\;0 < (m_{1,0} + m_{3,0}) x + (m_{1,1} + m_{3,1}) y + (m_{1,2} + m_{3,2}) z + (m_{1,3} + m_{3,3})
 *	\f}</td></tr>
 *	</table>
 *
 *	Top clipping plane
 *	<table>
 *  <tr><td>\f{align*}
 *	               &\;y' < w'\\
 *  \hookrightarrow&\;(\mathbf{p} \cdot r_1) < (\mathbf{p} \cdot r_3)\\
 *  \hookrightarrow&\;0 < (\mathbf{p} \cdot r_3) - (\mathbf{p} \cdot r_1)\\
 *  \hookrightarrow&\;0 < \mathbf{p} \cdot (r_3 - r_1)\\
 *	\hookrightarrow&\;0 < (m_{3,0} - m_{1,0}) x + (m_{3,1} - m_{1,1}) y + (m_{3,2} - m_{1,2}) z + (m_{3,3} - m_{1,3}) w\\
 *	\hookrightarrow&\;0 < (m_{3,0} - m_{1,0}) x + (m_{3,1} - m_{1,1}) y + (m_{3,2} - m_{1,2}) z + (m_{3,3} - m_{1,3})
 *	\f}</td></tr>
 *	</table>
 *
 *	Near clipping plane
 *	<table>
 *  <tr><td>\f{align*}
 *	           &-w < z'\\
 *  \rightarrow&-(\mathbf{p} \cdot r_3) < (\mathbf{p} \cdot r_2)\\
 *  \rightarrow&0 < (\mathbf{p} \cdot r_2) + (\mathbf{p} \cdot r_3)\\
 *  \rightarrow&0 < \mathbf{p} \cdot (r_2 + r_3)\\
 *  \rightarrow&0 < (m_{2,0} + m_{3,0}) x + (m_{2,1} + m_{3,1}) y + (m_{2,2} + m_{3,2}) z + (m_{2,3} + m_{3,3} w\\
 *  \rightarrow&0 < (m_{2,0} + m_{3,0}) x + (m_{2,1} + m_{3,1}) y + (m_{2,2} + m_{3,2}) z + (m_{2,3} + m_{3,3})
 *	\f}</td></tr>
 *	</table>
 *
 *	Far clipping plane
 *	<table>
 *  <tr><td>\f{align*}
 *	           &z' < w'\\
 *  \rightarrow&(\mathbf{p} \cdot r_2) < (\mathbf{p} \cdot r_3)\\
 *	\rightarrow&0 < (\mathbf{p} \cdot r_3) - (\mathbf{p} \cdot r_2)\\
 *	\rightarrow&0 < \mathbf{p} \cdot (r_3 - r_2)\\
 *	\rightarrow&0 < (m_{3,0} - m_{2,0}) x + (m_{3,1} - m_{2,1}) y + (m_{3,2} - m_{2,1}) z + (m_{3,3} - m_{2,3}) w\\
 *	\rightarrow&0 < (m_{3,0} - m_{2,0}) x + (m_{3,1} - m_{2,1}) y + (m_{3,2} - m_{2,1}) z + (m_{3,3} - m_{2,3})
 *	\f}</td></tr>
 *	</table>
 * @remark

 */
void Frustum::calculatePlanes(const Matrix4f4f& matrix, Plane3f& left, Plane3f& right, Plane3f& bottom, Plane3f& top, Plane3f& near, Plane3f& far)
{
    float a, b, c, d;

    // Compute the left clipping plane of the frustum.
    a = matrix(3, 0) + matrix(0, 0);
    b = matrix(3, 1) + matrix(0, 1);
    c = matrix(3, 2) + matrix(0, 2);
    d = matrix(3, 3) + matrix(0, 3);
    left = Plane3f(a, b, c, d);

    // Compute the right clipping plane of the frustum.
    a = matrix(3, 0) - matrix(0, 0);
    b = matrix(3, 1) - matrix(0, 1);
    c = matrix(3, 2) - matrix(0, 2);
    d = matrix(3, 3) - matrix(0, 3);
    right = Plane3f(a, b, c, d);

    // Compute the bottom clipping plane of the frustum.
    a = matrix(3, 0) + matrix(1, 0);
    b = matrix(3, 1) + matrix(1, 1);
    c = matrix(3, 2) + matrix(1, 2);
    d = matrix(3, 3) + matrix(1, 3);
    bottom = Plane3f(a, b, c, d);

    // Compute the top clipping plane of the frustum.
    a = matrix(3, 0) - matrix(1, 0);
    b = matrix(3, 1) - matrix(1, 1);
    c = matrix(3, 2) - matrix(1, 2);
    d = matrix(3, 3) - matrix(1, 3);
    top = Plane3f(a, b, c, d);

    // Compute the near clipping plane of the frustum.
    a = matrix(3, 0) - matrix(2, 0);
    b = matrix(3, 1) - matrix(2, 1);
    c = matrix(3, 2) - matrix(2, 2);
    d = matrix(3, 3) - matrix(2, 3);
    near = Plane3f(a, b, c, d);

    // Compute the far clipping plane of the frustum.
    a = matrix(3, 0) - matrix(2, 0);
    b = matrix(3, 1) - matrix(2, 1);
    c = matrix(3, 2) - matrix(2, 2);
    d = matrix(3, 3) - matrix(2, 3);
    far = Plane3f(a, b, c, d);
}

void Frustum::calculate(const Matrix4f4f& projection, const Matrix4f4f& view)
{
	Vector3f pt1;
	Vector3f vlook;

    // Compute the 6 frustum planes.
    {
		Frustum::calculatePlanes(projection, view,
                                 _planes[Planes::LEFT],
                                 _planes[Planes::RIGHT],
                                 _planes[Planes::BOTTOM],
                                 _planes[Planes::TOP],
                                 _planes[Planes::NEAR],
                                 _planes[Planes::FAR]);
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
        float dist = _planes[Planes::BACK].distance(_origin);

        // calculate the center of the sphere
        _sphere.setCenter(_origin + vlook * (dist * 0.5f));

        // the vector from p1 to the center of the sphere
		Vector3f vDiff = _sphere.getCenter() - pt1;

        // the radius becomes the length of this vector
		_sphere.setRadius(vDiff.length());
    }
}

Math::Relation Frustum::intersects(const bv_t& bv, bool doEnds) const {
	return intersects_aabb(bv.getAABB().getMin(), bv.getAABB().getMax(), doEnds);
}

Math::Relation Frustum::intersects(const Vector3f& point, const bool doEnds) const {
	// Handle optional parameters.
	int start = 0,
		end = doEnds ? Planes::END : Planes::SIDES_END;

	// Assume the point is inside the frustum.
	bool inside = true;

	// Scan through the frustum's planes:
	for (int i = start; i <= end; i++) {
		if (_planes[i].distance(point) <= 0.0f) {
			inside = false;
			break;
		}
	}

	return inside ? Math::Relation::inside : Math::Relation::outside;
}

Math::Relation Frustum::intersects(const Sphere3f& sphere, const bool doEnds) const {
	// In the case of the sphere radius being 0, the frustum - sphere (hence multiple
	// plane - sphere) intersection test reduces to a frustum - point (hence multiple
	// plane - point) intersection tests.
	if (sphere.getRadius() == 0.0f) {
		return intersects(sphere.getCenter(), doEnds);
	}

	// Assume the sphere is completely inside the frustum.
	Math::Relation result = Math::Relation::inside;

	// Handle optional parameters.
	int start = 0,
		end = doEnds ? Planes::END : Planes::SIDES_END;

	// scan each plane
	for (int i = start; i <= end; i++) {
		float dist = _planes[i].distance(sphere.getCenter());

		// If the sphere is completely behind the current plane, it is outside the frustum.
		if (dist <= -sphere.getRadius()) {
			result = Math::Relation::outside;
			break;
		// If it is not completely in front of the current plane, it intersects the frustum.
		} else if (dist < sphere.getRadius()) {
			result = Math::Relation::intersect;
		}
	}

	return result;
}

Math::Relation Frustum::intersects(const Cube3f& cube, const bool doEnds) const {
	// Assume the cube is inside the frustum.
	Math::Relation result = Math::Relation::inside;

	// Handle optional parameters.
	int start = 0,
		end = doEnds ? Planes::END : Planes::SIDES_END;

	for (int i = start; i <= end; i++) {
		const Plane3f& plane = _planes[i];
		Vector3f vmin, vmax;
		// find the most-positive and most-negative points of the aabb
		for (int j = 0; j < 3; j++) {
			if (plane.getNormal()[j] > 0.0f) {
				vmin[j] = cube.getCenter()[j] - cube.getSize();
				vmax[j] = cube.getCenter()[j] + cube.getSize();
			} else {
				vmin[j] = cube.getCenter()[j] + cube.getSize();
				vmax[j] = cube.getCenter()[j] - cube.getSize();
			}
		}

		// If the cube is completely on the negative half-space of the plane,
		// then it is completely outside.
		/// @todo This is wrong!
		if (plane.distance(vmin) > 0.0f) {
			result = Math::Relation::outside;
			break;
		}

		// if vmin is inside and vmax is outside, then it is not completely inside
		/// @todo This is wrong.
		if (plane.distance(vmax) >= 0.0f) {
			result = Math::Relation::intersect;
		}
	}

	return result;
}

Math::Relation Frustum::intersects(const AABB3f& aabb, bool doEnds) const {
    return intersects_aabb(aabb.getMin(), aabb.getMax(), doEnds);
}

Math::Relation Frustum::intersects_aabb(const Vector3f& mins, const Vector3f& maxs, bool doEnds) const {
    // Handle optional parameters.
    int start = 0,
        end = doEnds ? Planes::END : Planes::SIDES_END;

    // Assume the AABB is inside the frustum.
	Math::Relation result = Math::Relation::inside;

    // scan through the planes until something happens
    int i;
    for (i = start; i <= end; i++) {
		if (Math::Relation::outside == plane_intersects_aabb_max(_planes[i], mins, maxs)) {
			result = Math::Relation::outside;
            break;
        }

		if (Math::Relation::outside == plane_intersects_aabb_min(_planes[i], mins, maxs)) {
			result = Math::Relation::intersect;
            break;
        }
    }

    // Continue on if there is something to do.
	if (Math::Relation::intersect == result) {
        // If we are in geometry_intersect mode, we only need to check for
        // the geometry_outside condition.

        // This eliminates a geometry_inside == retval test in every iteration of the loop
        for ( /* nothing */; i <= end; i++) {
			if (Math::Relation::outside == plane_intersects_aabb_max(_planes[i], mins, maxs)) {
				result = Math::Relation::outside;
                break;
            }
        }
    }

    return result;
}

bool Frustum::intersects(const oct_bb_t& oct, const bool doEnds) const {
	AABB3f aabb = oct.toAABB();
	Math::Relation result = intersects_aabb(aabb.getMin(), aabb.getMax(), doEnds);

	return result > Math::Relation::outside;
}

#pragma pop_macro("NEAR")
#pragma pop_macro("near")
#pragma pop_macro("FAR")
#pragma pop_macro("far")

} // namespace Graphics
} // namespace Ego
