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

/// @file geometry.c
/// @brief
/// @details

#include "geometry.h"

#include "egoboo_frustum.h"

#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
// internal functions
//--------------------------------------------------------------------------------------------

static geometry_rv plane_intersects_aabb_min( const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs );
static geometry_rv plane_intersects_aabb_max( const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs );

//--------------------------------------------------------------------------------------------
// plane base
//--------------------------------------------------------------------------------------------
bool_t plane_base_normalize( plane_base_t * plane )
{
    // the vector < (*plane)[kX], (*plane)[kY], (*plane)[kZ] > is the normal to the plane.
    // Convert it to a unit normal vector, which makes (*plane)[kW] the distance to the origin.
    float magnitude2;

    if ( NULL == plane ) return bfalse;

    magnitude2 = fvec3_length_2(( *plane ) );

    if ( 0.0f == magnitude2 )
    {
        fvec4_self_clear(( *plane ) );
    }
    else
    {
        float magniude = SQRT( magnitude2 );

        fvec4_self_scale(( *plane ), 1.0f / magniude );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// point functions
//--------------------------------------------------------------------------------------------
float plane_point_distance( const plane_base_t plane, const point_base_t pos )
{
    // calculate the perpendicular from the point to the plane.
    // assumes a normalized plane.
    // negative numbers indicate that the point in in the "negative half-space" of the plane

    if ( NULL == plane || NULL == pos ) return 0.0f;

    return fvec3_dot_product( plane, pos ) + plane[kW];
}

//--------------------------------------------------------------------------------------------
geometry_rv point_intersects_aabb( const point_base_t pos, const fvec3_base_t corner1, const fvec3_base_t corner2 )
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

geometry_rv aabb_intersects_aabb( const aabb_t * lhs, const aabb_t * rhs )
{
    const int dimensions = 3;

    int         cnt;
    bool_t      not_inside = bfalse;
    geometry_rv retval     = geometry_error;

    if ( NULL == lhs || NULL == rhs ) return geometry_error;

    // assume rhs is inside lhs
    retval     = geometry_inside;
    not_inside = bfalse;

    // scan all the coordinates
    for ( cnt = 0; cnt < dimensions; cnt++ )
    {
        if ( rhs->mins[cnt] > lhs->maxs[cnt] )
        {
            retval = geometry_outside;
            not_inside = btrue;
            break;
        }
        else if ( rhs->maxs[cnt] < lhs->mins[cnt] )
        {
            retval = geometry_outside;
            not_inside = btrue;
            break;
        }
        else if ( !not_inside )
        {
            if ( rhs->maxs[cnt] > lhs->maxs[cnt] ||
                 rhs->mins[cnt] < lhs->maxs[cnt] )
            {
                // one of the sides is hanging over the edge
                retval = geometry_intersect;
                not_inside = btrue;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// plane functions
//--------------------------------------------------------------------------------------------
geometry_rv plane_intersects_aabb_max( const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs )
{
    int   j;
    float dist, tmp;

    // find the point-plane distance for the most-positive points of the aabb
    dist = 0.0f;
    for ( j = 0; j < 3; j++ )
    {
        tmp = ( plane[j] > 0.0f ) ? maxs[j] : mins[j];
        dist += tmp * plane[j];
    }
    dist += plane[3];

    return ( dist > 0.0f ) ? geometry_inside : geometry_outside;
}

//--------------------------------------------------------------------------------------------
geometry_rv plane_intersects_aabb_min( const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs )
{
    int   j;
    float dist, tmp;

    // find the point-plane distance for the most-negative points of the aabb
    dist = 0.0f;
    for ( j = 0; j < 3; j++ )
    {
        tmp = ( plane[j] > 0.0f ) ? mins[j] : maxs[j];
        dist += tmp * plane[j];
    }
    dist += plane[3];

    return ( dist > 0.0f ) ? geometry_inside : geometry_outside;
}

//--------------------------------------------------------------------------------------------
geometry_rv plane_intersects_aabb( const plane_base_t plane, const fvec3_base_t mins, const fvec3_base_t maxs )
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
bool_t two_plane_intersection( fvec3_base_t dst_pos, fvec3_base_t dst_dir, plane_base_t p0, plane_base_t p1 )
{
    bool_t retval = bfalse;

    if ( NULL == dst_pos || NULL == dst_dir ) return bfalse;
    if ( NULL == p0 || NULL == p1 ) return bfalse;

    // the direction of the intersection is given by the cross product of the normals
    fvec3_cross_product( dst_dir, p0, p1 );

    if ( fvec3_self_normalize( dst_dir ) < 0.0f )
    {
        retval = bfalse;
        fvec3_self_clear( dst_pos );
    }
    else
    {
        dst_pos[kX] = ( p0[kY] * p1[kW] - p0[kW] * p1[kY] ) / dst_dir[kZ];
        dst_pos[kY] = ( p0[kW] * p1[kX] - p0[kX] * p1[kW] ) / dst_dir[kZ];
        dst_pos[kZ] = 0.0f;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t three_plane_intersection( fvec3_base_t dst_pos, plane_base_t p0, plane_base_t p1, plane_base_t p2 )
{
    float det;
    float tmp;

    if ( NULL == dst_pos ) return bfalse;

    if ( NULL == p0 || NULL == p1 || NULL == p2 ) return bfalse;

    // the determinant of the matrix
    det =
        p0[kX] * ( p1[kY] * p2[kZ] - p1[kZ] * p2[kY] ) -
        p0[kY] * ( p1[kX] * p2[kZ] - p2[kX] * p1[kZ] ) +
        p0[kZ] * ( p1[kX] * p2[kY] - p2[kX] * p1[kY] );

    // check for system that is too close to being degenerate
    if ( ABS( det ) < 1e-6 ) return bfalse;

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

    return btrue;
}

//--------------------------------------------------------------------------------------------
// frustum functions
//--------------------------------------------------------------------------------------------
geometry_rv frustum_intersects_point( const frustum_base_t planes, const fvec3_base_t pos )
{
    // if the distance to the plane is negative for any plane, the point is outside the frustum

    int i;
    bool_t inside;

    // error trap
    if ( NULL == planes || NULL == pos ) return geometry_error;

    // assume the worst
    inside = btrue;

    // scan through the frustum's planes
    for ( i = 0; i < FRUST_PLANE_COUNT; i++ )
    {
        if ( plane_point_distance( planes[i], pos ) <= 0.0f )
        {
            inside = bfalse;
            break;
        }
    }

    return inside ? geometry_inside : geometry_outside;
}

//--------------------------------------------------------------------------------------------
geometry_rv frustum_intersects_sphere( const frustum_base_t planes, const fvec3_base_t pos, const float radius )
{
    // if the sphere is outside one plane farther than its radius, it is outside the frustum

    int i;

    geometry_rv retval;

    // error trap
    if ( NULL == planes || NULL == pos ) return geometry_error;

    // reduce lhs to a simpler function if possible
    if ( radius <= 0.0f )
    {
        return frustum_intersects_point( planes, pos );
    }

    // assume inside
    retval = geometry_inside;

    // scan each plane
    for ( i = 0; i < FRUST_PLANE_COUNT; i++ )
    {
        float dist = plane_point_distance( planes[i], pos );

        // if the sphere is completely outside one plane, it is outside the frustum
        if ( dist <= -radius )
        {
            retval = geometry_outside;
            break;
        }

        // the sphere must be partially inside.
        // if it is not completely inside one plane, it is not completely inside the frustum
        if ( dist < radius )
        {
            retval = geometry_intersect;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
geometry_rv frustum_intersects_cube( const frustum_base_t planes, const fvec3_base_t pos, const float size )
{
    int          i, j;
    geometry_rv     retval;
    fvec3_base_t vmin, vmax;

    if ( NULL == planes || NULL == pos ) return geometry_error;

    // assume that it is inside
    retval = geometry_inside;

    for ( i = 0; i < FRUST_PLANE_COUNT; i++ )
    {
        const plane_base_t * plane = planes + i;

        // find the most-positive and most-negative points of the aabb
        for ( j = 0; j < 3; j++ )
        {
            if (( *plane )[j] > 0.0f )
            {
                vmin[j] = pos[j] - size;
                vmax[j] = pos[j] + size;
            }
            else
            {
                vmin[j] = pos[j] + size;
                vmax[j] = pos[j] - size;
            }
        }

        // if it is completely outside any plane, then it is completely outside
        if ( plane_point_distance( *plane, vmin ) > 0.0f )
        {
            retval = geometry_outside;
            break;
        }

        // if vmin is inside and vmax is outside, then it is not completely inside
        if ( plane_point_distance( *plane, vmax ) >= 0.0f )
        {
            retval = geometry_intersect;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
geometry_rv frustum_intersects_aabb( const frustum_base_t planes, const fvec3_base_t mins, const fvec3_base_t maxs )
{
    int      i;
    geometry_rv retval;

    if ( NULL == planes || NULL == mins || NULL == maxs ) return geometry_error;

    // assume that it is inside
    retval = geometry_inside;

    // scan through the planes until something happens
    for ( i = 0; i < FRUST_PLANE_COUNT; i++ )
    {
        if ( geometry_outside == plane_intersects_aabb_max( planes[i], mins, maxs ) )
        {
            retval = geometry_outside;
            break;
        }

        if ( geometry_outside == plane_intersects_aabb_min( planes[i], mins, maxs ) )
        {
            retval = geometry_intersect;
            break;
        }
    }

    // continue on if there is something to do
    if ( geometry_intersect == retval )
    {
        // If we are in geometry_intersect mode, we only need to check for
        // the geometry_outside condition.
        // This eliminates a geometry_inside == retval test in every iteration of the loop
        for ( /* nothing */ ; i < FRUST_PLANE_COUNT; i++ )
        {
            if ( geometry_outside == plane_intersects_aabb_max( planes[i], mins, maxs ) )
            {
                retval = geometry_outside;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// sphere functions
//--------------------------------------------------------------------------------------------

geometry_rv sphere_intersects_sphere( const sphere_t * lhs, const sphere_t * rhs )
{
    geometry_rv retval = geometry_error;
    fvec3_t vdiff;
    float   dist2;

    // get the separating axis
    fvec3_sub( vdiff.v, lhs->origin.v, rhs->origin.v );

    // get the distance squared
    dist2 = fvec3_length_2( vdiff.v );

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

geometry_rv cone_intersects_point( const cone_t * K, const fvec3_base_t P )
{
    /// @desceiption determine whether a point is inside a cone

    fvec3_t dist_vec;
    float para_dist;

    geometry_rv retval = geometry_error;

    if ( NULL == K || NULL == P ) return geometry_error;

    // move the cone origin to the origin of coordinates
    fvec3_sub( dist_vec.v, P, K->origin.v );

    // project the point's position onto the cone's axis
    para_dist = fvec3_dot_product( K->axis.v, dist_vec.v );

    if ( para_dist < 0.0f )
    {
        retval = geometry_outside;
    }
    else
    {
        float dist_2, perp_dist_2, para_dist_2;

        // the square of the total distance
        dist_2 = fvec3_length_2( dist_vec.v );

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
            float test1, test2, test3;

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
            // the cosine of the of the angle between the point and the axis must be greater than the given sine
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
    // an approximation to the complete sphere-cone test.
    // currently, the spherical region around the origin of the cone has no special test and
    // a small volume behind the origin of the cone is not being culled.

    geometry_rv retval;
    bool_t done;

    float offset_length;
    fvec3_t offset_vec;

    // check to make sure the pointers are valid
    if ( NULL == K || NULL == S )
    {
        return geometry_error;
    }

    // cones with bad data are not useful
    if ( K->sin_2 < 0.0f || K->cos_2 > 1.0f )
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
    fvec3_scale( offset_vec.v, K->axis.v, offset_length );

    // assume the worst
    retval = geometry_error;
    done   = bfalse;

    // being instide the forward cone means that the sphere is
    // completely inside the original cone
    if ( !done )
    {
        cone_t  K_new;

        // Shift the origin by the offset in the positive sense.
        // If the center of the sphere is inside this cone, it must be competely inside the original cone
        memcpy( &K_new, K, sizeof( K_new ) );
        fvec3_add( K_new.origin.v, K->origin.v, offset_vec.v );

        switch ( cone_intersects_point( &K_new, S->origin.v ) )
        {
            case geometry_error:
                retval = geometry_error;
                done = btrue;
                break;

            case geometry_outside:
                /* do nothing */
                break;

            case geometry_intersect:
                retval = geometry_intersect;
                done = btrue;
                break;

            case geometry_inside:
                retval = geometry_inside;
                done = btrue;
                break;
        }
    }

    // test for intersection with the back cone.
    if ( !done )
    {
        cone_t  K_new;

        // Shift the origin by the offset in the negative sense.
        // If the center of the sphere is inside this cone, it must be intersecting the original cone.
        // Since it failed the other cone test it must, in fact, be merely intersecting the cone.
        memcpy( &K_new, K, sizeof( K_new ) );
        fvec3_sub( K_new.origin.v, K->origin.v, offset_vec.v );

        switch ( cone_intersects_point( &K_new, S->origin.v ) )
        {
            case geometry_error:
                retval = geometry_error;
                done = btrue;
                break;

            case geometry_outside:
                retval = geometry_outside;
                done = btrue;
                break;

            case geometry_intersect:
            case geometry_inside:
                retval = geometry_intersect;
                done = btrue;
                break;
        }
    }

    return !done ? geometry_error : retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t sphere_self_clear( sphere_t * ptr )
{
    if ( NULL == ptr ) return bfalse;

    fvec3_self_clear( ptr->origin.v );
    ptr->radius = -1.0f;

    return btrue;
}
