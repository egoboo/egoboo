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
#include "egolib/_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Call this every time the camera moves to update the frustum
static void frustum_calculate(frustum_base_t pf, const fmat_4x4_base_t proj, const fmat_4x4_base_t modl);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void frustum_calculate(frustum_base_t planes, const fmat_4x4_base_t proj, const fmat_4x4_base_t modl)
{
    float clip[16];        // This will hold the clipping planes

    mat_Multiply(clip, proj, modl);

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

egolib_rv egolib_frustum_calculate(egolib_frustum_t * pf, const fmat_4x4_base_t proj, const fmat_4x4_base_t modl)
{
    fvec3_t pt1;
    fvec3_t vlook, vfar;

    if ( NULL == pf || NULL == proj || NULL == modl ) return rv_error;

    //---- construct the basic frustum
    {
        frustum_calculate(pf->data, proj, modl);
    }

    //---- construct the camera location
    {
        // the origin of the frustum (should be the camera position)
        three_plane_intersection(pf->origin.v, pf->data[FRUST_PLANE_RIGHT], pf->data[FRUST_PLANE_LEFT], pf->data[FRUST_PLANE_BOTTOM]);
    }

    //---- construct the sphere
    {
        fvec3_t vDiff, vtmp;
        float dist;

        // extract the view direction from the modelview matrix
        mat_getCamForward(modl, vlook);

        // one far corner of the frustum
        three_plane_intersection(pt1.v, pf->data[FRUST_PLANE_TOP], pf->data[FRUST_PLANE_RIGHT], pf->data[FRUST_PLANE_BACK]);

        // get the distance from the origin to the far plane
        dist = plane_point_distance(pf->data[FRUST_PLANE_BACK], pf->origin.v);

        // calculate the center of the sphere
		fvec3_add(pf->sphere.origin.v, pf->origin.v, fvec3_scale(vtmp.v, vlook.v, dist * 0.5f));

        // the vector between the center of the sphere and and pt1
        vDiff = fvec3_sub(pf->sphere.origin, pt1);

        // the radius becomes the length of this vector
        pf->sphere.radius = fvec3_length(vDiff);
    }

    //---- construct the cone
    {
        float cos_half_fov;

		pf->cone.origin = pf->origin;
#if 0
        fvec3_base_copy( pf->cone.origin.v, pf->origin.v );
#endif
		pf->cone.axis = vlook;
#if 0
        fvec3_base_copy( pf->cone.axis.v, vlook.v );
#endif
        // the vector from the origin to the far corner
        vfar = fvec3_sub(pt1, pf->cone.origin);

        // the cosine between the view direction and the
        cos_half_fov = fvec3_dot_product(vfar, vlook) / fvec3_length(vfar);

        // calculate the required trig functions
        pf->cone.cos_2 = cos_half_fov * cos_half_fov;
        pf->cone.sin_2 = 1.0f - pf->cone.cos_2;
        pf->cone.inv_sin = 1.0f / std::sqrt(pf->cone.sin_2);
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
geometry_rv egolib_frustum_intersects_bv(const egolib_frustum_t * self,const bv_t *bv)
{
    /// @author BB
    /// @details determine the geometric relationship between the given frustum a bounding volume.
    ///
    /// @notes the return values deal with the aabb being inside the frustum or not
    ///
    /// geometry_error     - obvious
    /// geometry_outside   - the bounding volume is outside the frustum
    /// geometry_intersect - the bounding volume and the frustum partially overlap
    /// geometry_inside    - the bounding volume is completely inside the frustum

    geometry_rv retval = geometry_error;
    bool finished;

    if ( NULL == self || NULL == bv ) return geometry_error;

    finished = false;

    /// @todo PF@> Something's wrong; this returns intersect/inside for tiles not actually being
    ///            in the frustum, just force the complete calc for now.
#if 0
    if ( !finished )
    {
        intersect_rv = point_intersects_aabb( self->origin.v, bv->aabb.mins, bv->aabb.maxs );

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
        intersect_rv = cone_intersects_sphere( &( self->cone ), &( bv->sphere ) );

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
#endif

    if ( !finished )
    {
        // do the complete calculation. whatever it returns is what it is.
        // do not check the front and back of the frustum
        retval = frustum_intersects_aabb( self->data, bv->aabb.mins, bv->aabb.maxs, false );
        finished = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
