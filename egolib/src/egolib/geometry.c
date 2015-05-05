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

//--------------------------------------------------------------------------------------------
// plane base
//--------------------------------------------------------------------------------------------
bool plane_base_normalize( plane_base_t * plane )
{
    // the vector < (*plane)[kX], (*plane)[kY], (*plane)[kZ] > is the normal to the plane.
    // Convert it to a unit normal vector, which makes (*plane)[kW] the distance to the origin.
    float magnitude2;

    if ( NULL == plane ) return false;

    magnitude2 = plane_get_normal(*plane).length();

    if ( 0.0f == magnitude2 )
    {
        fvec4_self_clear(( *plane ) );
    }
    else
    {
        float magniude = std::sqrt( magnitude2 );

        fvec4_self_scale(( *plane ), 1.0f / magniude );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
// point functions
//--------------------------------------------------------------------------------------------
float plane_point_distance(const plane_base_t plane, const fvec3_t& pos)
{
    // calculate the perpendicular from the point to the plane.
    // assumes a normalized plane.
    // negative numbers indicate that the point in in the "negative half-space" of the plane

    if ( NULL == plane) return 0.0f;

	/// @todo Use plane_t::get_normal(). Use point3_t::tovec3().
    return fvec3_t(plane[0],plane[1],plane[2]).dot(pos) + plane[kW];
}

//--------------------------------------------------------------------------------------------
geometry_rv point_intersects_aabb( const point_base_t pos, const fvec3_t& corner1, const fvec3_t& corner2 )
{
    int cnt;
    geometry_rv retval = geometry_inside;

    // assume inside
    retval = geometry_inside;

    // scan the points
    for ( cnt = 0; cnt < 3; cnt++ )
    {
        // don't assume one corner's points are always smaller
        if ( corner1[cnt] <= corner2[cnt] )
        {
            if ( pos[cnt] < corner1[cnt] || pos[cnt] > corner2[cnt] )
            {
                retval = geometry_outside;
                break;
            }
        }
        else
        {
            if ( pos[cnt] < corner2[cnt] || pos[cnt] > corner1[cnt] )
            {
                retval = geometry_outside;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// aabb functions
//--------------------------------------------------------------------------------------------

geometry_rv aabb_intersects_aabb(const aabb_t& lhs, const aabb_t& rhs)
{
    const size_t dimensions = 3;

    // Assume RHS is inside LHS.
	geometry_rv retval = geometry_inside;
    bool not_inside = false;

    // Scan all the coordinates.
    for (size_t cnt = 0; cnt < dimensions; ++cnt)
    {
        if (rhs.mins[cnt] > lhs.maxs[cnt])
        {
            return geometry_outside;
        }
        else if (rhs.maxs[cnt] < lhs.mins[cnt])
        {
            return geometry_outside;
        }
        else if (!not_inside)
        {
            if (rhs.maxs[cnt] > lhs.maxs[cnt] ||
                rhs.mins[cnt] < lhs.maxs[cnt])
            {
                // one of the sides is hanging over the edge
                retval = geometry_intersect;
                not_inside = true;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// plane functions
//--------------------------------------------------------------------------------------------
geometry_rv plane_intersects_aabb_max( const plane_base_t plane, const fvec3_t& mins, const fvec3_t& maxs )
{
    int   j;
    float dist, tmp;

    geometry_rv retval = geometry_error;

    // find the point-plane distance for the most-positive points of the aabb
    dist = 0.0f;
    for ( j = 0; j < 3; j++ )
    {
        tmp = ( plane[j] > 0.0f ) ? maxs[j] : mins[j];
        dist += tmp * plane[j];
    }
    dist += plane[3];

    if ( dist > 0.0f )
    {
        retval = geometry_inside;
    }
    else if ( dist < 0.0f )
    {
        retval = geometry_outside;
    }
    else
    {
        retval = geometry_intersect;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
geometry_rv plane_intersects_aabb_min(const plane_base_t plane, const fvec3_t& mins, const fvec3_t& maxs)
{
    int   j;
    float dist, tmp;

    geometry_rv retval = geometry_error;

    // find the point-plane distance for the most-negative points of the aabb
    dist = 0.0f;
    for ( j = 0; j < 3; j++ )
    {
        tmp = ( plane[j] > 0.0f ) ? mins[j] : maxs[j];
        dist += tmp * plane[j];
    }
    dist += plane[3];

    if ( dist > 0.0f )
    {
        retval = geometry_inside;
    }
    else if ( dist < 0.0f )
    {
        retval = geometry_outside;
    }
    else
    {
        retval = geometry_intersect;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
geometry_rv plane_intersects_aabb(const plane_base_t plane, const fvec3_t& mins, const fvec3_t& maxs)
{
    geometry_rv retval = geometry_inside;

    if ( geometry_outside == plane_intersects_aabb_max( plane, mins, maxs ) )
    {
        retval = geometry_outside;
        goto plane_intersects_aabb_done;
    }

    if ( geometry_outside == plane_intersects_aabb_min( plane, mins, maxs ) )
    {
        retval = geometry_intersect;
        goto plane_intersects_aabb_done;
    }

plane_intersects_aabb_done:

    return retval;
}

//--------------------------------------------------------------------------------------------
fvec3_t plane_get_normal(const plane_base_t self)
{
	return fvec3_t(self[kX],self[kY],self[kZ]);
}
bool two_plane_intersection(fvec3_t& dst_pos, fvec3_t& dst_dir, const plane_base_t p0, const plane_base_t p1)
{
    bool retval = false;

    if ( NULL == p0 || NULL == p1 ) return false;

    // the direction of the intersection is given by the cross product of the normals
    dst_dir = plane_get_normal(p0).cross(plane_get_normal(p1));

    if (dst_dir.normalize() < 0.0f)
    {
        retval = false;
        dst_pos = fvec3_t::zero();
    }
    else
    {
        dst_pos[kX] = ( p0[kY] * p1[kW] - p0[kW] * p1[kY] ) / dst_dir[kZ];
        dst_pos[kY] = ( p0[kW] * p1[kX] - p0[kX] * p1[kW] ) / dst_dir[kZ];
        dst_pos[kZ] = 0.0f;

        retval = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool three_plane_intersection( fvec3_t& dst_pos, const plane_base_t p0, const plane_base_t p1, const plane_base_t p2 )
{
    float det;
    float tmp;

    if ( NULL == p0 || NULL == p1 || NULL == p2 ) return false;

    // the determinant of the matrix
    det =
        p0[kX] * ( p1[kY] * p2[kZ] - p1[kZ] * p2[kY] ) -
        p0[kY] * ( p1[kX] * p2[kZ] - p2[kX] * p1[kZ] ) +
        p0[kZ] * ( p1[kX] * p2[kY] - p2[kX] * p1[kY] );

    // check for system that is too close to being degenerate
    if ( std::abs( det ) < 1e-6 ) return false;

    // the x component
    tmp =
        p0[kW] * ( p1[kZ] * p2[kY] - p1[kY] * p2[kZ] ) +
        p1[kW] * ( p0[kY] * p2[kZ] - p0[kZ] * p2[kY] ) +
        p2[kW] * ( p0[kZ] * p1[kY] - p0[kY] * p1[kZ] );
    dst_pos[kX] = tmp / det;

    // the y component
    tmp =
        p0[kW] * ( p1[kX] * p2[kZ] - p1[kZ] * p2[kX] ) +
        p1[kW] * ( p0[kZ] * p2[kX] - p0[kX] * p2[kZ] ) +
        p2[kW] * ( p0[kX] * p1[kZ] - p0[kZ] * p1[kX] );
    dst_pos[kY] = tmp / det;

    // the z component
    tmp =
        p0[kW] * ( p1[kY] * p2[kX] - p1[kX] * p2[kY] ) +
        p1[kW] * ( p0[kX] * p2[kY] - p0[kY] * p2[kX] ) +
        p2[kW] * ( p0[kY] * p1[kX] - p0[kX] * p1[kY] );
    dst_pos[kZ] = tmp / det;

    return true;
}


//--------------------------------------------------------------------------------------------
// sphere functions
//--------------------------------------------------------------------------------------------

geometry_rv sphere_intersects_sphere(const sphere_t *lhs, const sphere_t *rhs)
{
    geometry_rv retval = geometry_error;
    fvec3_t vdiff;
    float   dist2;

    // get the separating axis
    vdiff = lhs->origin - rhs->origin;

    // get the distance squared
    dist2 = vdiff.length_2();

    if ( rhs->radius < lhs->radius )
    {
        if ( dist2 < lhs->radius * lhs->radius )
        {
            retval = geometry_inside;
        }
    }

    if ( geometry_inside != retval )
    {
        // get the sum of the radii
        float fRadiiSum = lhs->radius + rhs->radius;

        // if the distance between the centers is less than the sum
        // of the radii, then we have an intersection
        // we calculate lhs using the squared lengths for speed
        if ( dist2 < fRadiiSum * fRadiiSum )
        {
            retval = geometry_intersect;
        }
        else
        {
            retval = geometry_outside;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// cone functions
//--------------------------------------------------------------------------------------------

geometry_rv cone_intersects_point(const cone_t *K, const fvec3_t& P)
{
    /// @brief determine whether a point is inside a cone
    geometry_rv retval = geometry_error;

    if (NULL == K) return geometry_error;

    // move the cone origin to the origin of coordinates
    fvec3_t dist_vec = P - K->origin;

    // project the point's position onto the cone's axis
    float para_dist = K->axis.dot(dist_vec);

    if ( para_dist < 0.0f )
    {
        retval = geometry_outside;
    }
    else
    {
        float dist_2, perp_dist_2, para_dist_2;

        // the square of the total distance
        dist_2 = dist_vec.length_2();

        // the square of the parallel distance
        para_dist_2 = para_dist * para_dist;

        // the scuare of the perpendicular distance
        perp_dist_2 = dist_2 - para_dist_2;

        if ( perp_dist_2 < 0.0f || K->sin_2 < 0.0f )
        {
            retval = geometry_error;
        }
        else if ( 0.0f == perp_dist_2 && K->sin_2 > 0.0f )
        {
            retval = geometry_inside;
        }
        else
        {
			float test3;

            // the test for being inside:
            //     the perp distance from the axis (squared, perp_dist_2) must be less than
            //     the radius of the cone at that distance along the cone (squared, dist_2 * K->sin_2)
            //test1 = dist_2 * K->sin_2 - perp_dist_2;

            // alternate test 1 for being inside:
            // the sine of the angle between the point and the axis must be less than the given sine
            // sqrt( 1.0f - para_dist_2 / dist_2 ) < sqrt( K->sin_2 )
            // 1.0f - para_dist_2 / dist_2 < K->sin_2
            // para_dist_2 / dist_2 > 1.0f - K->sin_2
            // para_dist_2 > dist_2 * (1.0f - K->sin_2)
            //test2 = para_dist_2 - dist_2 * (1.0f - K->sin_2);

            // alternate test 2 for being inside:
            // the cosine of the of the angle between the point and the axis must be greater than the given cosine
            // sqrt( para_dist_2 / dist_2 ) > sqrt( K->cos_2 )
            // para_dist_2 > dist_2 * K->cos_2
            test3 = para_dist_2 - dist_2 * K->cos_2;

            // is the point inside the cone?
            if ( test3 > 0.0f )
            {
                retval = geometry_inside;
            }
            else if ( 0.0f == test3 )
            {
                retval = geometry_intersect;
            }
            else
            {
                retval = geometry_outside;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
geometry_rv cone_intersects_sphere( const cone_t * K, const sphere_t * S )
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

    geometry_rv retval = geometry_error;
    bool done;

    float offset_length;
    fvec3_t offset_vec;

    // check to make sure the pointers are valid
    if ( NULL == K || NULL == S )
    {
        return geometry_error;
    }

    // cones with bad data are not useful
    if ( K->sin_2 <= 0.0f || K->sin_2 > 1.0f || K->cos_2 >= 1.0f )
    {
        return geometry_error;
    }

    // uninitialized/inverted spheres give no useful information
    if ( S->radius < 0.0f )
    {
        return geometry_error;
    }

    // this is the distance that the origin of the cone must be moved to
    // produce a new cone with an offset equal to the radius of the sphere
    offset_length = S->radius * K->inv_sin;
	offset_vec = K->axis * offset_length;

    // assume the worst
    retval = geometry_error;
    done   = false;

    // being instide the forward cone means that the sphere is
    // completely inside the original cone
    if ( !done )
    {
        cone_t  K_new;

        // Shift the origin by the offset in the positive sense.
        memcpy( &K_new, K, sizeof( K_new ) );
        K_new.origin = K->origin + offset_vec;

        // If the center of the sphere is inside this cone, it must be competely inside the original cone
        switch ( cone_intersects_point( &K_new, S->origin ) )
        {
            case geometry_error:
                retval = geometry_error;
                done = true;
                break;

            case geometry_outside: // the center of the sphere is outside the foreward cone
                /* do nothing */
                break;

            case geometry_intersect: // the origin of the cone is exactly on the foreward cone
            case geometry_inside:    // the origin of the sphere is inside the foreward cone

                // the sphere is completely inside the original cone
                retval = geometry_inside;
                done = true;
                break;
        }
    }

    // test for intersection with the back cone.
    if ( !done )
    {
        cone_t K_new;

        // Shift the origin by the offset in the negative sense.
		K_new = *K;
        K_new.origin = K->origin - offset_vec;

        // If the center of the sphere is inside this cone, it must be intersecting the original cone.
        // Since it failed test with the foreward cone, it must be merely intersecting the cone.
        switch ( cone_intersects_point( &K_new, S->origin ) )
        {
            case geometry_error:
                retval = geometry_error;
                done = true;
                break;

            case geometry_outside:
                retval = geometry_outside;
                done = true;
                break;

            case geometry_intersect:
            case geometry_inside:
                retval = geometry_intersect;
                done = true;
                break;
        }
    }

    return !done ? geometry_error : retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

