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

/// @file egolib/geometry.c
/// @brief
/// @details

#include "egolib/geometry.h"

#include "egolib/_math.h"
#include "egolib/_math.h"
#include "egolib/Math/Plane.hpp"
#include "egolib/Math/Sphere.h"
#include "egolib/frustum.h"

Ego::Math::Relation point_intersects_aabb(const Vector3f& point, const Vector3f& corner1, const Vector3f& corner2)
{
    // scan the points
    for (size_t cnt = 0; cnt < 3; cnt++ )
    {
        // don't assume one corner's points are always smaller
        if ( corner1[cnt] <= corner2[cnt] )
        {
            if (point[cnt] < corner1[cnt] || point[cnt] > corner2[cnt] )
            {
				return Ego::Math::Relation::outside;
            }
        }
        else
        {
            if (point[cnt] < corner2[cnt] || point[cnt] > corner1[cnt] )
            {
				return Ego::Math::Relation::outside;
            }
        }
    }

    // assume inside
    return Ego::Math::Relation::inside;
}

//--------------------------------------------------------------------------------------------
// aabb functions
//--------------------------------------------------------------------------------------------

Ego::Math::Relation aabb_intersects_aabb(const AABB3f& lhs, const AABB3f& rhs)
{
    const size_t dimensions = 3;

    // Assume RHS is inside LHS.
	Ego::Math::Relation retval = Ego::Math::Relation::inside;
    bool not_inside = false;

    // Scan all the coordinates.
    for (size_t cnt = 0; cnt < dimensions; ++cnt)
    {
        if (rhs.getMin()[cnt] > lhs.getMax()[cnt])
        {
			return Ego::Math::Relation::outside;
        }
        else if (rhs.getMax()[cnt] < lhs.getMin()[cnt])
        {
			return Ego::Math::Relation::outside;
        }
        else if (!not_inside)
        {
            if (rhs.getMax()[cnt] > lhs.getMax()[cnt] ||
                rhs.getMin()[cnt] < lhs.getMax()[cnt])
            {
                // one of the sides is hanging over the edge
				retval = Ego::Math::Relation::intersect;
                not_inside = true;
            }
        }
    }

    return retval;
}

Ego::Math::Relation plane_intersects_aabb_max(const Plane3f& plane, const Vector3f& mins, const Vector3f& maxs)
{
    int   j;
    float dist, tmp;

    // find the point-plane distance for the most-positive points of the aabb
    dist = 0.0f;
    for (j = 0; j < 3; j++)
    {
        tmp = (plane.getNormal()[j] > 0.0f) ? maxs[j] : mins[j];
        dist += tmp * plane.getNormal()[j];
    }
    dist += plane.getDistance();

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

Ego::Math::Relation plane_intersects_aabb_min(const Plane3f& plane, const Vector3f& mins, const Vector3f& maxs)
{
    int   j;
    float dist, tmp;

    // find the point-plane distance for the most-negative points of the aabb
    dist = 0.0f;
    for (j = 0; j < 3; j++)
    {
        tmp = (plane.getNormal()[j] > 0.0f) ? mins[j] : maxs[j];
        dist += tmp * plane.getNormal()[j];
    }
    dist += plane.getDistance();

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

Ego::Math::Relation plane_intersects_aabb(const Plane3f& plane, const Vector3f& mins, const Vector3f& maxs)
{
	Ego::Math::Relation retval = Ego::Math::Relation::inside;

	if (Ego::Math::Relation::outside == plane_intersects_aabb_max(plane, mins, maxs))
    {
		retval = Ego::Math::Relation::outside;
        goto plane_intersects_aabb_done;
    }

	if (Ego::Math::Relation::outside == plane_intersects_aabb_min(plane, mins, maxs))
    {
		retval = Ego::Math::Relation::intersect;
        goto plane_intersects_aabb_done;
    }

plane_intersects_aabb_done:

    return retval;
}

bool two_plane_intersection(Vector3f& p, Vector3f& d, const Plane3f& p0, const Plane3f& p1)
{
    // Compute \f$\vec{d} = \hat{n}_0 \times \hat{n}_1\f$
    const Vector3f &n0 = p0.getNormal();
    const Vector3f &n1 = p1.getNormal();
    d = n0.cross(n1);
    
    // If \f$\vec{v}\f$ is the zero vector, then the planes do not intersect.
    if (0 == d.normalize()) {
        return false;
    }
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

bool three_plane_intersection(Vector3f& dst_pos, const Plane3f& p0, const Plane3f& p1, const Plane3f& p2)
{
	Vector3f n0 = p0.getNormal(),
             n1 = p1.getNormal(),
             n2 = p2.getNormal();
    float d0 = p0.getDistance(),
          d1 = p1.getDistance(),
          d2 = p2.getDistance();
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

Ego::Math::Relation sphere_intersects_sphere(const Sphere3f& lhs, const Sphere3f& rhs)
{
    // Get the separating axis
    Vector3f vdiff = lhs.getCenter() - rhs.getCenter();

    // Get the distance squared.
    float dist2 = vdiff.length_2();

    if (rhs.getRadius() < lhs.getRadius())
    {
        if (dist2 < lhs.getRadius() * lhs.getRadius())
        {
			return Ego::Math::Relation::inside;
        }
    }

    // Get the sum of the radii.
    float fRadiiSum = lhs.getRadius() + rhs.getRadius();

    // If the distance between the centers is less than the sum of the radii,
    // then we have an intersection we calculate lhs using the squared lengths for speed.
    if (dist2 < fRadiiSum * fRadiiSum)
    {
		return Ego::Math::Relation::intersect;
    }
    else
    {
		return Ego::Math::Relation::outside;
    }
}

//--------------------------------------------------------------------------------------------
// cone functions
//--------------------------------------------------------------------------------------------

Ego::Math::Relation cone_intersects_point(const Cone3f& K, const Vector3f& P)
{
    // move the cone origin to the origin of coordinates
	Vector3f dist_vec = P - K.origin;

    // project the point's position onto the cone's axis
    float para_dist = K.axis.dot(dist_vec);

    if ( para_dist < 0.0f )
    {
		return Ego::Math::Relation::outside;
    }
    else
    {
        // the square of the total distance
        float dist_2 = dist_vec.length_2();

        // the square of the parallel distance
        float para_dist_2 = para_dist * para_dist;

        // the square of the perpendicular distance
        float perp_dist_2 = dist_2 - para_dist_2;

        if ( perp_dist_2 < 0.0f || K.sin_2 < 0.0f )
        {
			return Ego::Math::Relation::error;
        }
        else if ( 0.0f == perp_dist_2 && K.sin_2 > 0.0f )
        {
			return Ego::Math::Relation::inside;
        }
        else
        {
			float test3;

            // the test for being inside:
            //     the perp distance from the axis (squared, perp_dist_2) must be less than
            //     the radius of the cone at that distance along the cone (squared, dist_2 * K->sin_2)
            //test1 = dist_2 * K.sin_2 - perp_dist_2;

            // alternate test 1 for being inside:
            // the sine of the angle between the point and the axis must be less than the given sine
            // sqrt( 1.0f - para_dist_2 / dist_2 ) < sqrt( K.sin_2 )
            // 1.0f - para_dist_2 / dist_2 < K->sin_2
            // para_dist_2 / dist_2 > 1.0f - K->sin_2
            // para_dist_2 > dist_2 * (1.0f - K->sin_2)
            // test2 = para_dist_2 - dist_2 * (1.0f - K.sin_2);

            // alternate test 2 for being inside:
            // the cosine of the of the angle between the point and the axis must be greater than the given cosine
            // sqrt( para_dist_2 / dist_2 ) > sqrt( K.cos_2 )
            // para_dist_2 > dist_2 * K.cos_2
            test3 = para_dist_2 - dist_2 * K.cos_2;

            // is the point inside the cone?
            if ( test3 > 0.0f )
            {
				return Ego::Math::Relation::inside;
            }
            else if ( 0.0f == test3 )
            {
				return Ego::Math::Relation::intersect;
            }
            else
            {
				return Ego::Math::Relation::outside;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
Ego::Math::Relation cone_intersects_sphere(const Cone3f& K, const Sphere3f& S)
{
    /// @author BB
    ///
    /// @details  An approximation to the complete sphere-cone test.
    ///
    /// @notes
    /// Tests to remove some volume between the foreward and rear versions of the cone are not implemented,
    /// so this will over-estimate the cone volume. The smaller the opening angle, the worse the overestimation
    /// will be
    ///
    /// the return values deal with the sphere being inside the cone or not
    /// geometry_error     - obvious
    /// geometry_outside   - the aabb is outside the frustum
    /// geometry_intersect - the aabb and the frustum partially overlap
    /// geometry_inside    - the aabb is completely inside the frustum

	Ego::Math::Relation retval = Ego::Math::Relation::error;
    bool done;

    float offset_length;
	Vector3f offset_vec;

    // cones with bad data are not useful
	/// @todo Encapsulte cone such that this may not happen.
    if ( K.sin_2 <= 0.0f || K.sin_2 > 1.0f || K.cos_2 >= 1.0f )
    {
		throw std::runtime_error("invalid cone");
    }


    // this is the distance that the origin of the cone must be moved to
    // produce a new cone with an offset equal to the radius of the sphere
    offset_length = S.getRadius() * K.inv_sin;
	offset_vec = K.axis * offset_length;

    // assume the worst
    done   = false;

    // being instide the forward cone means that the sphere is
    // completely inside the original cone
    // Shift the origin by the offset in the positive sense.
	Cone3f K_new = K;
    K_new.origin = K.origin + offset_vec;

    // If the center of the sphere is inside this cone, it must be competely inside the original cone
    switch (cone_intersects_point(K_new, S.getCenter()))
    {
		case Ego::Math::Relation::outside: // the center of the sphere is outside the foreward cone
            /* do nothing */
            break;

		case Ego::Math::Relation::intersect: // the origin of the cone is exactly on the foreward cone
		case Ego::Math::Relation::inside:    // the origin of the sphere is inside the foreward cone

            // the sphere is completely inside the original cone
			retval = Ego::Math::Relation::inside;
            done = true;
            break;

        case Ego::Math::Relation::error:
            throw std::runtime_error("invalid cone");
    }

    // test for intersection with the back cone.
    if ( !done )
    {
        // Shift the origin by the offset in the negative sense.
		K_new = K;
        K_new.origin = K.origin - offset_vec;

        // If the center of the sphere is inside this cone, it must be intersecting the original cone.
        // Since it failed test with the foreward cone, it must be merely intersecting the cone.
        switch ( cone_intersects_point( K_new, S.getCenter() ) )
        {
			case Ego::Math::Relation::outside:
				retval = Ego::Math::Relation::outside;
                done = true;
                break;

			case Ego::Math::Relation::intersect:
			case Ego::Math::Relation::inside:
				retval = Ego::Math::Relation::intersect;
                done = true;
                break;

            case Ego::Math::Relation::error:
                throw std::runtime_error("invalid cone");
        }
    }

	return !done ? Ego::Math::Relation::error : retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

