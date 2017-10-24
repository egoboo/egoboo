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

/// @file     egolib/geometry.c
/// @brief   functions for manipulating geometric primitives
/// @details

#include "egolib/geometry.h"

#include "egolib/_math.h"
#include "egolib/frustum.h"

//--------------------------------------------------------------------------------------------
// axis aligned box functions
//--------------------------------------------------------------------------------------------

Ego::Math::Relation plane_intersects_aab_max(const Plane3f& plane, const Point3f& mins, const Point3f& maxs)
{
    // find the point-plane distance for the most-positive points of the aabb
    float dist = 0.0f;
    for (int j = 0; j < 3; j++)
    {
        float tmp = (plane.get_normal()[j] > 0.0f) ? maxs[j] : mins[j];
        dist += tmp * plane.get_normal()[j];
    }
    dist += plane.get_distance();

    if (dist > 0.0f)
    {
		return Ego::Math::Relation::inside;
    }
    else if (dist < 0.0f)
    {
		return Ego::Math::Relation::outside;
    }
    else
    {
		return Ego::Math::Relation::intersect;
    }
}

Ego::Math::Relation plane_intersects_aab_min(const Plane3f& plane, const Point3f& mins, const Point3f& maxs)
{
    // find the point-plane distance for the most-negative points of the aabb
    float dist = 0.0f;
    for (int j = 0; j < 3; j++)
    {
        float tmp = (plane.get_normal()[j] > 0.0f) ? mins[j] : maxs[j];
        dist += tmp * plane.get_normal()[j];
    }
    dist += plane.get_distance();

    if (dist > 0.0f)
    {
		return Ego::Math::Relation::inside;
    }
    else if (dist < 0.0f)
    {
		return Ego::Math::Relation::outside;
    }
    else
    {
		return Ego::Math::Relation::intersect;
    }
}

Ego::Math::Relation plane_intersects_aab(const Plane3f& plane, const AxisAlignedBox3f& aab)
{
	Ego::Math::Relation retval = Ego::Math::Relation::inside;
    const auto mins = aab.get_min(),
               maxs = aab.get_max();
	if (Ego::Math::Relation::outside == plane_intersects_aab_max(plane, mins, maxs))
    {
		retval = Ego::Math::Relation::outside;
        goto Done;
    }

	if (Ego::Math::Relation::outside == plane_intersects_aab_min(plane, mins, maxs))
    {
		retval = Ego::Math::Relation::intersect;
        goto Done;
    }

Done:

    return retval;
}

bool two_plane_intersection(Vector3f& p, Vector3f& d, const Plane3f& p0, const Plane3f& p1)
{
    // Compute \f$\vec{d} = \hat{n}_0 \times \hat{n}_1\f$
    const Vector3f &n0 = p0.get_normal();
    const Vector3f &n1 = p1.get_normal();
    d = cross(n0, n1);
    
    // If \f$\vec{v}\f$ is the zero vector, then the planes do not intersect.
    if (id::zero<Vector3f>() == d) {
        return false;
    }
    d = normalize(d).get_vector();
    if (0.0f != d[kZ])
    {
        p[kX] = (n0[kY] * n1[kW] - n0[kW] * n1[kY]) / d[kZ];
        p[kY] = (n0[kW] * n1[kX] - n0[kX] * n1[kW]) / d[kZ];
        p[kZ] = 0.0f;
    }
    else
    {
        throw std::runtime_error("not yet supported");
    }
    return true;
}

bool three_plane_intersection(Point3f& dst_pos, const Plane3f& p0, const Plane3f& p1, const Plane3f& p2)
{
	Vector3f n0 = p0.get_normal(),
             n1 = p1.get_normal(),
             n2 = p2.get_normal();
    float d0 = p0.get_distance(),
          d1 = p1.get_distance(),
          d2 = p2.get_distance();
    // the determinant of the matrix
    float det =
        n0[kX] * (n1[kY] * n2[kZ] - n1[kZ] * n2[kY]) -
        n0[kY] * (n1[kX] * n2[kZ] - n2[kX] * n1[kZ]) +
        n0[kZ] * (n1[kX] * n2[kY] - n2[kX] * n1[kY]);

    // check for system that is too close to being degenerate
    if (std::abs(det) < 1e-6) return false;

    float tmp;

    // the x component
    tmp =
        d0 * (n1[kZ] * n2[kY] - n1[kY] * n2[kZ]) +
        d1 * (n0[kY] * n2[kZ] - n0[kZ] * n2[kY]) +
        d2 * (n0[kZ] * n1[kY] - n0[kY] * n1[kZ]);
    dst_pos[kX] = tmp / det;

    // the y component
    tmp =
        d0 * (n1[kX] * n2[kZ] - n1[kZ] * n2[kX]) +
        d1 * (n0[kZ] * n2[kX] - n0[kX] * n2[kZ]) +
        d2 * (n0[kX] * n1[kZ] - n0[kZ] * n1[kX]);
    dst_pos[kY] = tmp / det;

    // the z component
    tmp =
        d0 * (n1[kY] * n2[kX] - n1[kX] * n2[kY]) +
        d1 * (n0[kX] * n2[kY] - n0[kY] * n2[kX]) +
        d2 * (n0[kY] * n1[kX] - n0[kX] * n1[kY]);
    dst_pos[kZ] = tmp / det;

    return true;
}


//--------------------------------------------------------------------------------------------
// sphere functions
//--------------------------------------------------------------------------------------------

Ego::Math::Relation cone_intersects_point(const Cone3f& K, const Point3f& P)
{
    // If \f$\hat{d} \cdot \left(P-O\right) = \left|P-O\right|\cos \theta\f$ then the point
    // is on the cone and if \$\hat{d} \cdot \left(P-O\right) > \left|P-O\right|\cos \theta\f$
    // then it is in the cone. Otherwise it is outside the cone.
    // Compute \$t = P - O\f$.
	const auto t = P - K.get_origin();
    // Compute \$\left|t\right|\cos\theta\f$.
    const auto rhs = id::euclidean_norm(t) * std::cos(K.get_angle());
    // Compute \f$\hat{d} \cdot \left(t\right)\f$.
    const auto lhs = id::dot_product(K.get_axis(), t);
    if (lhs > rhs) {
        return Ego::Math::Relation::inside;
    } else if (lhs < rhs) {
        return Ego::Math::Relation::outside;
    } else {
        return Ego::Math::Relation::on;
    }
}

bool sphere_intersects_cone(const Sphere3f& S, const Cone3f& K) {
    // \$\sin\left(\theta\right)\f$
    const float s = std::sin(K.get_angle());
    // \f$\cos\left(\theta\right)\f$
    const float c = std::cos(K.get_angle());
    // The reciprocal of \f$\sin\left(\theta\right)\f$: \f$\frac{1}{\sin\left(\theta\right)}\f$.
    const float s_r = 1.0f / s;

    // Compute the forward cone.
    auto origin_p = K.get_origin() - K.get_axis() * (S.get_radius() * s_r);
    auto direction_p = K.get_axis();

    Vector3f t; float lhs, rhs;

    // Determine the relation of the center to the forward cone.
    t = S.get_center() - origin_p;
    rhs = id::euclidean_norm(t) * c;
    lhs = dot(direction_p, t);
    if (lhs < rhs) {
        // The center is outside the forward cone, the sphere is outside the cone.
        return false;
    }
    // (At this point, the sphere is either inside or on the forward cone).
    // Compute the backward cone.
    auto origin_m = K.get_origin();
    auto direction_m = -K.get_axis();
    // Determine the relation of the center to the backward cone.
    t = S.get_center() - origin_m;
    rhs = id::euclidean_norm(t) * s; // \f$\cos\left(90-\theta\right) = \sin(\theta)\f$
    lhs = dot(direction_m, t);
    if (lhs < rhs) {
        // The sphere does not intersect the backward cone is inside (on) the cone
        // if and only if \f$\left|C - V\right| < r\f$ (\f$\left|C - V\right | = r\f$).
        t = S.get_center() - K.get_origin();
        // We use the squared length and the squared radius.
        float l2 = id::squared_euclidean_norm(t), r2 = S.get_radius() * S.get_radius();
        return l2 <= r2;
    } else {
        // The sphere and the forward cone and the sphere and the backward cone intersect.
        // Hence, there sphere intersects the cone.
        return true;
    }
}
