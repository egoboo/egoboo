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
#include "egolib/opengl/renderer.h"
#include "egolib/_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// use OpenGL to multiply matrices
static INLINE bool ogl_matrix_mult( GLXmatrix clip, const GLXmatrix proj, const GLXmatrix modl );

/// Call this every time the camera moves to update the frustum
static void frustum_calculate( frustum_base_t pf, const float proj[], const float modl[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE bool ogl_matrix_mult( GLXmatrix clip, const GLXmatrix proj, const GLXmatrix modl )
{
    // a helper function to harness opengl to do our matrix multiplications for us

    if ( NULL == clip )
    {
        // nothing to do
        return false;
    }

    if ( NULL == proj || NULL == modl )
    {
        // treat one of both matrices as the zero matrix
        memset( clip, 0, sizeof( float )*16 );
    }
    else
    {
        // use opengl to multiply the matrices so it is self-consistent
        GLint matrix_mode[1];

        // save the matrix mode
        GL_DEBUG( glGetIntegerv )( GL_MATRIX_MODE, matrix_mode );

        // do the work in the projection matrix stack
        glMatrixMode( GL_PROJECTION );
        glPushMatrix();

        // load the given projection matrix
        glLoadMatrixf( proj );

        // multiply model matrix with this
        glMultMatrixf( modl );

        // grab the resultant matrix and place it in clip
        glGetFloatv( GL_PROJECTION_MATRIX, ( GLfloat * )clip );

        // restore the old projection matrix
        glPopMatrix();

        // restore the matrix mode
        glMatrixMode( matrix_mode[0] );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void frustum_calculate( frustum_base_t planes, const float proj[], const float modl[] )
{
    float clip[16];        // This will hold the clipping planes

    ogl_matrix_mult( clip, proj, modl );

    // This will extract the FRUST_PLANE_RIGHT side of the frustum
    planes[FRUST_PLANE_RIGHT][kX] = clip[ 3] - clip[ 0];
    planes[FRUST_PLANE_RIGHT][kY] = clip[ 7] - clip[ 4];
    planes[FRUST_PLANE_RIGHT][kZ] = clip[11] - clip[ 8];
    planes[FRUST_PLANE_RIGHT][kW] = clip[15] - clip[12];
    plane_base_normalize( planes + FRUST_PLANE_RIGHT );

    // This will extract the FRUST_PLANE_LEFT side of the frustum
    planes[FRUST_PLANE_LEFT][kX] = clip[ 3] + clip[ 0];
    planes[FRUST_PLANE_LEFT][kY] = clip[ 7] + clip[ 4];
    planes[FRUST_PLANE_LEFT][kZ] = clip[11] + clip[ 8];
    planes[FRUST_PLANE_LEFT][kW] = clip[15] + clip[12];
    plane_base_normalize( planes + FRUST_PLANE_LEFT );

    // This will extract the FRUST_PLANE_BOTTOM side of the frustum
    planes[FRUST_PLANE_BOTTOM][kX] = clip[ 3] + clip[ 1];
    planes[FRUST_PLANE_BOTTOM][kY] = clip[ 7] + clip[ 5];
    planes[FRUST_PLANE_BOTTOM][kZ] = clip[11] + clip[ 9];
    planes[FRUST_PLANE_BOTTOM][kW] = clip[15] + clip[13];
    plane_base_normalize( planes + FRUST_PLANE_BOTTOM );

    // This will extract the FRUST_PLANE_TOP side of the frustum
    planes[FRUST_PLANE_TOP][kX] = clip[ 3] - clip[ 1];
    planes[FRUST_PLANE_TOP][kY] = clip[ 7] - clip[ 5];
    planes[FRUST_PLANE_TOP][kZ] = clip[11] - clip[ 9];
    planes[FRUST_PLANE_TOP][kW] = clip[15] - clip[13];
    plane_base_normalize( planes + FRUST_PLANE_TOP );

    // This will extract the FRUST_PLANE_BACK side of the frustum
    planes[FRUST_PLANE_BACK][kX] = clip[ 3] - clip[ 2];
    planes[FRUST_PLANE_BACK][kY] = clip[ 7] - clip[ 6];
    planes[FRUST_PLANE_BACK][kZ] = clip[11] - clip[10];
    planes[FRUST_PLANE_BACK][kW] = clip[15] - clip[14];
    plane_base_normalize( planes + FRUST_PLANE_BACK );

    // This will extract the FRUST_PLANE_FRONT side of the frustum
    planes[FRUST_PLANE_FRONT][kX] = clip[ 3] + clip[ 2];
    planes[FRUST_PLANE_FRONT][kY] = clip[ 7] + clip[ 6];
    planes[FRUST_PLANE_FRONT][kZ] = clip[11] + clip[10];
    planes[FRUST_PLANE_FRONT][kW] = clip[15] + clip[14];
    plane_base_normalize( planes + FRUST_PLANE_FRONT );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

egolib_rv egolib_frustum_calculate( egolib_frustum_t * pf, const float proj[], const float modl[] )
{
    fvec3_t pt1;
    fvec3_t vlook, vfar;

    if ( NULL == pf || NULL == proj || NULL == modl ) return rv_error;

    //---- construct the basic frustum
    {
        frustum_calculate( pf->data, proj, modl );
    }

    //---- construct the camera location
    {
        // the origin of the frustum (should be the camera position)
        three_plane_intersection( pf->origin.v, pf->data[FRUST_PLANE_RIGHT], pf->data[FRUST_PLANE_LEFT], pf->data[FRUST_PLANE_BOTTOM] );
    }

    //---- construct the sphere
    {
        fvec3_t vDiff, vtmp;
        float dist;

        // extract the view direction from the modelview matrix
        mat_getCamForward( modl, vlook.v );

        // one far corner of the frustum
        three_plane_intersection( pt1.v, pf->data[FRUST_PLANE_TOP], pf->data[FRUST_PLANE_RIGHT], pf->data[FRUST_PLANE_BACK] );

        // get the distance from the origin to the far plane
        dist = plane_point_distance( pf->data[FRUST_PLANE_BACK], pf->origin.v );

        // calculate the center of the sphere
        fvec3_add( pf->sphere.origin.v, pf->origin.v, fvec3_scale( vtmp.v, vlook.v, dist * 0.5f ) );

        // the vector between the center of the sphere and and pt1
        fvec3_sub( vDiff.v, pf->sphere.origin.v, pt1.v );

        // the radius becomes the length of this vector
        pf->sphere.radius = fvec3_length( vDiff.v );
    }

    //---- construct the cone
    {
        float cos_half_fov;

        fvec3_base_copy( pf->cone.origin.v, pf->origin.v );
        fvec3_base_copy( pf->cone.axis.v, vlook.v );

        // the vector from the origin to the far corner
        fvec3_sub( vfar.v, pt1.v, pf->cone.origin.v );

        // the cosine between the view direction and the
        cos_half_fov = fvec3_dot_product( vfar.v, vlook.v ) / fvec3_length( vfar.v );

        // calculate the required trig functions
        pf->cone.cos_2 = cos_half_fov * cos_half_fov;
        pf->cone.sin_2 = 1.0f - pf->cone.cos_2;
        pf->cone.inv_sin = 1.0f / SQRT( pf->cone.sin_2 );
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
geometry_rv egolib_frustum_intersects_ego_aabb( const egolib_frustum_t * pfrust, const ego_aabb_t * paabb )
{
    /// @author BB
    /// @details determine the geometric relationship between the given frustum and axis-aligned bounding box
    ///
    /// @notes the return values deal with the aabb being inside the frustum or not
    ///
    /// geometry_error     - obvious
    /// geometry_outside   - the aabb is outside the frustum
    /// geometry_intersect - the aabb and the frustum partially overlap
    /// geometry_inside    - the aabb is completely inside the frustum

    geometry_rv retval = geometry_error;
    geometry_rv intersect_rv;
    bool finished;

    if ( NULL == pfrust || NULL == paabb ) return geometry_error;

    finished = false;

    if ( !finished )
    {
        intersect_rv = point_intersects_aabb( pfrust->origin.v, paabb->data.mins, paabb->data.maxs );

        switch ( intersect_rv )
        {
            case geometry_error:
                // an error occured in this function
                retval = geometry_error;
                break;

            case geometry_outside:
                // the camera being outside the bounding volume means nothing by itself
                /* do nothing */
                break;

            case geometry_intersect:
                // this is not emitted by point_intersects_aabb() at this time
                // merge with the next case

            case geometry_inside:
                // The camera is inside the bounding box, so it must intersect with it.
                // As a speedup, don't try to further refine this answer.
                retval = geometry_intersect;
                finished = true;
                break;
        }
    }

    // ... this calculation takes too much time ...
    //if ( !finished )
    //{
    //    intersect_rv = sphere_intersects_sphere( &( pfrust->sphere ), &( paabb->sphere ) );

    //    switch ( intersect_rv )
    //    {
    //        case geometry_error:
    //            // an error occured in this function
    //            retval = intersect_rv;
    //            finished = true;
    //            break;

    //        case geometry_outside:
    //            // the sphere surrounding the camera frustum does not intersect the sphere surrounding the aabb
    //            retval = intersect_rv;
    //            finished = true;
    //            break;

    //        case geometry_intersect:
    //        case geometry_inside:
    //            /* do nothing */
    //            break;

    //    }
    //}

    if ( !finished )
    {
        intersect_rv = cone_intersects_sphere( &( pfrust->cone ), &( paabb->sphere ) );

        switch ( intersect_rv )
        {
            case geometry_error:
                // an error occured in this function
                retval = geometry_error;
                break;

            case geometry_outside:
                // the cone representing the camera frustum does not intersect the sphere surrounding the aabb
                // since the cone is bigger than the frustum, we are done
                retval = geometry_outside;
                finished = true;
                break;

            case geometry_intersect:
                retval = geometry_intersect;
                finished = true;
                break;

            case geometry_inside:
                retval = geometry_inside;
                finished = true;
                break;
        }
    }

    if ( !finished )
    {
        // do the complete calculation. whatever it returns is what it is.
        // do not check the front and back of the frustum
        retval = frustum_intersects_aabb( pfrust->data, paabb->data.mins, paabb->data.maxs, false );
        finished = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_aabb_t * ego_aabb_ctor( ego_aabb_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ego_aabb_self_clear( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_aabb_t * ego_aabb_dtor( ego_aabb_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    ego_aabb_self_clear( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_self_clear( ego_aabb_t * psrc )
{
    if ( NULL == psrc ) return false;

    aabb_self_clear( &( psrc->data ) );

    sphere_self_clear( &( psrc->sphere ) );

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_copy( ego_aabb_t * pdst, const ego_aabb_t * psrc )
{
    bool retval = false;

    if ( NULL == pdst )
    {
        retval = false;
    }
    else if ( NULL == psrc )
    {
        retval = ego_aabb_self_clear( pdst );
    }
    else
    {
        memmove( pdst, psrc, sizeof( *pdst ) );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_from_oct_bb( ego_aabb_t * dst, const oct_bb_t * src )
{
    bool retval;

    if ( NULL == dst ) return false;

    retval = aabb_from_oct_bb( &( dst->data ), src );

    if ( retval )
    {
        retval = ego_aabb_validate( dst );
    }
    else
    {
        ego_aabb_self_clear( dst );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_is_clear( const ego_aabb_t * pdst )
{
    bool retval;

    if ( NULL == pdst ) return true;

    retval = aabb_is_clear( &( pdst->data ) );

    if ( !retval )
    {
        retval = ( pdst->sphere.radius <= 0.0f );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_self_union( ego_aabb_t * pdst, const ego_aabb_t * psrc )
{
    bool retval = false;

    if ( NULL == pdst ) return false;
    if ( NULL == psrc ) return true;

    if ( pdst->sphere.radius < 0.0f )
    {
        ego_aabb_validate( pdst );
    }

    if ( psrc->sphere.radius < 0.0f )
    {
        return true;
    }

    retval = aabb_self_union( &( pdst->data ), &( psrc->data ) );
    if ( retval )
    {
        retval = ego_aabb_validate( pdst );
    }
    else
    {
        sphere_self_clear( &( pdst->sphere ) );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_lhs_contains_rhs( const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr )
{
    if ( NULL == lhs_ptr || NULL == rhs_ptr ) return false;

    ego_aabb_test( lhs_ptr );
    ego_aabb_test( rhs_ptr );

    return aabb_lhs_contains_rhs( &( lhs_ptr->data ), &( rhs_ptr->data ) );
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_overlap( const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr )
{
    if ( NULL == lhs_ptr || NULL == rhs_ptr ) return false;

    ego_aabb_test( lhs_ptr );
    ego_aabb_test( rhs_ptr );

    return aabb_overlap( &( lhs_ptr->data ), &( rhs_ptr->data ) );
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_validate( ego_aabb_t * rhs )
{
    int cnt;
    float radius_2;
    fvec3_t vtmp;

    if ( NULL == rhs ) return false;

    for ( cnt = 0; cnt < 3; cnt++ )
    {
        rhs->sphere.origin.v[cnt] = 0.5f * ( rhs->data.mins[cnt] + rhs->data.maxs[cnt] );
    }

    fvec3_sub( vtmp.v, rhs->data.mins, rhs->sphere.origin.v );
    radius_2 = fvec3_length_2( vtmp.v );
    if ( 0.0f == radius_2 )
    {
        rhs->sphere.radius = 0.0f;
    }
    else
    {
        rhs->sphere.radius = SQRT( radius_2 );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_test( const ego_aabb_t * rhs )
{
    bool retval;

    if ( NULL == rhs ) return false;

    if ( rhs->sphere.radius < 0.0f )
    {
        retval = false;
    }
    else
    {
        retval = true;
    }

    return retval;
}
