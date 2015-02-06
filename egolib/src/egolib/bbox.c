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

/// @file egolib/bbox.c
/// @brief
/// @details

#include "egolib/bbox.h"
#include "egolib/_math.h"
#include "egolib/math/AABB.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_cv_point_data;
typedef struct s_cv_point_data cv_point_data_t;

//--------------------------------------------------------------------------------------------
// struct s_cv_point_data
//--------------------------------------------------------------------------------------------
struct s_cv_point_data
{
    s_cv_point_data() :
        inside(false),
        pos(0, 0, 0),
        rads(0.0f)
    {
        //ctor
    }

    bool  inside;
    fvec3_t   pos;
    float   rads;
};

static int cv_point_data_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
OVolume_t * OVolume__ctor( OVolume_t * pv )
{
    if ( NULL == pv ) return pv;

    pv->lod = -1;
    pv->needs_shape = true;
    pv->needs_position = true;
    oct_bb_t::ctor( &( pv->oct ) );

    return pv;
}

//--------------------------------------------------------------------------------------------
OVolume_t OVolume_merge( const OVolume_t * pv1, const OVolume_t * pv2 )
{
    OVolume_t rv;

    // construct the OVolume
    OVolume__ctor( &rv );

    if ( NULL == pv1 && NULL == pv2 )
    {
        return rv;
    }
    else if ( NULL == pv2 )
    {
        return *pv1;
    }
    else if ( NULL == pv1 )
    {
        return *pv2;
    }
    else
    {
        // check for uninitialized volumes
        if ( -1 == pv1->lod && -1 == pv2->lod )
        {
            return rv;
        }
        else if ( -1 == pv1->lod )
        {
            return *pv2;
        }
        else if ( -1 == pv2->lod )
        {
            return *pv1;
        };

        // merge the volumes
        oct_bb_union( &( pv1->oct ), &( pv2->oct ), &( rv.oct ) );

        // check for an invalid volume
        rv.lod = rv.oct.empty ? -1 : 1;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
OVolume_t OVolume_intersect( const OVolume_t * pv1, const OVolume_t * pv2 )
{
    OVolume_t rv;

    // construct the OVolume
    OVolume__ctor( &rv );

    if ( NULL == pv1 || NULL == pv2 )
    {
        return rv;
    }
    else
    {
        // check for uninitialized volumes
        if ( -1 == pv1->lod && -1 == pv2->lod )
        {
            return rv;
        }
        else if ( -1 == pv1->lod )
        {
            return *pv2;
        }
        else if ( -1 == pv2->lod )
        {
            return *pv1;
        }

        // intersect the volumes
        oct_bb_intersection_index( &( pv1->oct ), &( pv1->oct ), &( rv.oct ), OCT_X );
        if ( rv.oct.mins[OCT_X] >= rv.oct.maxs[OCT_X] ) return rv;

        oct_bb_intersection_index( &( pv1->oct ), &( pv1->oct ), &( rv.oct ), OCT_Y );
        if ( rv.oct.mins[OCT_Y] >= rv.oct.maxs[OCT_Y] ) return rv;

        oct_bb_intersection_index( &( pv1->oct ), &( pv1->oct ), &( rv.oct ), OCT_Z );
        if ( rv.oct.mins[OCT_Z] >= rv.oct.maxs[OCT_Z] ) return rv;

        if ( pv1->lod >= 0 && pv2->lod >= 0 )
        {
            oct_bb_intersection_index( &( pv1->oct ), &( pv1->oct ), &( rv.oct ), OCT_XY );
            if ( rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY] ) return rv;

            oct_bb_intersection_index( &( pv1->oct ), &( pv1->oct ), &( rv.oct ), OCT_YX );
            if ( rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX] ) return rv;
        }
        else if ( pv1->lod >= 0 )
        {
            rv.oct.mins[OCT_XY] = std::max( pv1->oct.mins[OCT_XY], pv2->oct.mins[OCT_X] + pv2->oct.mins[OCT_Y] );
            rv.oct.maxs[OCT_XY] = std::min( pv1->oct.maxs[OCT_XY], pv2->oct.maxs[OCT_X] + pv2->oct.maxs[OCT_Y] );
            if ( rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY] ) return rv;

            rv.oct.mins[OCT_YX] = std::max( pv1->oct.mins[OCT_YX], -pv2->oct.maxs[OCT_X] + pv2->oct.mins[OCT_Y] );
            rv.oct.maxs[OCT_YX] = std::min( pv1->oct.maxs[OCT_YX], -pv2->oct.mins[OCT_X] + pv2->oct.maxs[OCT_Y] );
            if ( rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX] ) return rv;
        }
        else if ( pv2->lod >= 0 )
        {
            rv.oct.mins[OCT_XY] = std::max( pv1->oct.mins[OCT_X] + pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_XY] );
            rv.oct.maxs[OCT_XY] = std::min( pv1->oct.maxs[OCT_X] + pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_XY] );
            if ( rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY] ) return rv;

            rv.oct.mins[OCT_YX] = std::max( -pv1->oct.maxs[OCT_X] + pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_YX] );
            rv.oct.maxs[OCT_YX] = std::min( -pv1->oct.mins[OCT_X] + pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_YX] );
            if ( rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX] ) return rv;
        }
        else
        {
            rv.oct.mins[OCT_XY] = std::max( pv1->oct.mins[OCT_X] + pv1->oct.mins[OCT_Y], pv2->oct.mins[OCT_X] + pv2->oct.mins[OCT_Y] );
            rv.oct.maxs[OCT_XY] = std::min( pv1->oct.maxs[OCT_X] + pv1->oct.maxs[OCT_Y], pv2->oct.maxs[OCT_X] + pv2->oct.maxs[OCT_Y] );
            if ( rv.oct.mins[OCT_XY] >= rv.oct.maxs[OCT_XY] ) return rv;

            rv.oct.mins[OCT_YX] = std::max( -pv1->oct.maxs[OCT_X] + pv1->oct.mins[OCT_Y], -pv2->oct.maxs[OCT_X] + pv2->oct.mins[OCT_Y] );
            rv.oct.maxs[OCT_YX] = std::min( -pv1->oct.mins[OCT_X] + pv1->oct.maxs[OCT_Y], -pv2->oct.mins[OCT_X] + pv2->oct.maxs[OCT_Y] );
            if ( rv.oct.mins[OCT_YX] >= rv.oct.maxs[OCT_YX] ) return rv;
        }

        if ( 0 == pv1->lod && 0 == pv2->lod )
        {
            rv.lod = 0;
        }
        else
        {
            rv.lod = std::min( pv1->lod, pv2->lod );
        }
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool OVolume_refine( OVolume_t * pov, fvec3_t * pcenter, float * pvolume )
{
    /// @author BB
    /// @details determine which of the 16 possible intersection points are within both
    ///     square and diamond bounding volumes

    bool invalid;
    int cnt, tnc, count;
    float  area, darea, volume;

    fvec3_t center, centroid;
    cv_point_data_t pd[16];

    if ( NULL == pov ) return false;

    invalid = false;
    if ( pov->oct.mins[OCT_X]  >= pov->oct.maxs[OCT_X] ) invalid = true;
    else if ( pov->oct.mins[OCT_Y]  >= pov->oct.maxs[OCT_Y] ) invalid = true;
    else if ( pov->oct.mins[OCT_Z]  >= pov->oct.maxs[OCT_Z] ) invalid = true;
    else if ( pov->oct.mins[OCT_XY] >= pov->oct.maxs[OCT_XY] ) invalid = true;
    else if ( pov->oct.mins[OCT_YX] >= pov->oct.maxs[OCT_YX] ) invalid = true;

    if ( invalid )
    {
        pov->lod = -1;
        if ( NULL != pvolume )( *pvolume ) = 0;
        return false;
    }

    // square points
    cnt = 0;
    pd[cnt].pos.x = pov->oct.maxs[OCT_X];
    pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

    cnt++;
    pd[cnt].pos.x = pov->oct.maxs[OCT_X];
    pd[cnt].pos.y = pov->oct.mins[OCT_Y];

    cnt++;
    pd[cnt].pos.x = pov->oct.mins[OCT_X];
    pd[cnt].pos.y = pov->oct.mins[OCT_Y];

    cnt++;
    pd[cnt].pos.x = pov->oct.mins[OCT_X];
    pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

    // diamond points
    cnt++;
    pd[cnt].pos.x = ( pov->oct.maxs[OCT_XY] - pov->oct.mins[OCT_YX] ) * 0.5f;
    pd[cnt].pos.y = ( pov->oct.maxs[OCT_XY] + pov->oct.mins[OCT_YX] ) * 0.5f;

    cnt++;
    pd[cnt].pos.x = ( pov->oct.mins[OCT_XY] - pov->oct.mins[OCT_YX] ) * 0.5f;
    pd[cnt].pos.y = ( pov->oct.mins[OCT_XY] + pov->oct.mins[OCT_YX] ) * 0.5f;

    cnt++;
    pd[cnt].pos.x = ( pov->oct.mins[OCT_XY] - pov->oct.maxs[OCT_YX] ) * 0.5f;
    pd[cnt].pos.y = ( pov->oct.mins[OCT_XY] + pov->oct.maxs[OCT_YX] ) * 0.5f;

    cnt++;
    pd[cnt].pos.x = ( pov->oct.maxs[OCT_XY] - pov->oct.maxs[OCT_YX] ) * 0.5f;
    pd[cnt].pos.y = ( pov->oct.maxs[OCT_XY] + pov->oct.maxs[OCT_YX] ) * 0.5f;

    // intersection points
    cnt++;
    pd[cnt].pos.x = pov->oct.maxs[OCT_X];
    pd[cnt].pos.y = pov->oct.maxs[OCT_X] + pov->oct.mins[OCT_YX];

    cnt++;
    pd[cnt].pos.x = pov->oct.mins[OCT_Y] - pov->oct.mins[OCT_YX];
    pd[cnt].pos.y = pov->oct.mins[OCT_Y];

    cnt++;
    pd[cnt].pos.x = -pov->oct.mins[OCT_Y] + pov->oct.mins[OCT_XY];
    pd[cnt].pos.y = pov->oct.mins[OCT_Y];

    cnt++;
    pd[cnt].pos.x = pov->oct.mins[OCT_X];
    pd[cnt].pos.y = -pov->oct.mins[OCT_X] + pov->oct.mins[OCT_XY];

    cnt++;
    pd[cnt].pos.x = pov->oct.mins[OCT_X];
    pd[cnt].pos.y = pov->oct.mins[OCT_X] + pov->oct.maxs[OCT_YX];

    cnt++;
    pd[cnt].pos.x = pov->oct.maxs[OCT_Y] - pov->oct.maxs[OCT_YX];
    pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

    cnt++;
    pd[cnt].pos.x = -pov->oct.maxs[OCT_Y] + pov->oct.maxs[OCT_XY];
    pd[cnt].pos.y = pov->oct.maxs[OCT_Y];

    cnt++;
    pd[cnt].pos.x = pov->oct.maxs[OCT_X];
    pd[cnt].pos.y = -pov->oct.maxs[OCT_X] + pov->oct.maxs[OCT_XY];

    // which points are outside both volumes
	center = fvec3_t::zero;
    count = 0;
    for ( cnt = 0; cnt < 16; cnt++ )
    {
        float ftmp;

        pd[cnt].inside = false;

        // check the box
        if ( pd[cnt].pos.x < pov->oct.mins[OCT_X] || pd[cnt].pos.x > pov->oct.maxs[OCT_X] ) continue;
        if ( pd[cnt].pos.y < pov->oct.mins[OCT_Y] || pd[cnt].pos.y > pov->oct.maxs[OCT_Y] ) continue;

        // check the diamond
        ftmp = pd[cnt].pos.x + pd[cnt].pos.y;
        if ( ftmp < pov->oct.mins[OCT_XY] || ftmp > pov->oct.maxs[OCT_XY] ) continue;

        ftmp = -pd[cnt].pos.x + pd[cnt].pos.y;
        if ( ftmp < pov->oct.mins[OCT_YX] || ftmp > pov->oct.maxs[OCT_YX] ) continue;

        // found a point
        center.x += pd[cnt].pos.x;
        center.y += pd[cnt].pos.y;
        count++;
        pd[cnt].inside = true;
    };

    if ( count < 3 ) return false;

    // find the centroid
    center.x *= 1.0f / ( float )count;
    center.y *= 1.0f / ( float )count;
    center.z *= 1.0f / ( float )count;

    // move the valid points to the beginning of the list
    for ( cnt = 0, tnc = 0; cnt < 16 && tnc < count; cnt++ )
    {
        if ( !pd[cnt].inside ) continue;

        // insert a valid point into the next available slot
        if ( tnc != cnt )
        {
            pd[tnc] = pd[cnt];
        }

        // record the Cartesian rotation angle relative to center
        pd[tnc].rads = ATAN2( pd[cnt].pos.y - center.y, pd[cnt].pos.x - center.x );
        tnc++;
    }

    // use qsort to order the points according to their rotation angle
    // relative to the centroid
    qsort(( void * )pd, count, sizeof( cv_point_data_t ), cv_point_data_cmp );

    // now we can use geometry to find the area of the planar collision area
	centroid = fvec3_t::zero;
    {
        fvec3_t diff1, diff2;
        oct_vec_t opd;

        oct_vec_ctor( opd, pd[0].pos );
        oct_bb_set_ovec( &( pov->oct ), opd );

        area = 0;
        for ( cnt = 1; cnt < count - 1; cnt++ )
        {
            tnc = cnt + 1;

            // optimize the bounding volume
            oct_vec_ctor( opd, pd[tnc].pos );
            oct_bb_self_sum_ovec( &( pov->oct ), opd );

            // determine the area for this element
            diff1.x = pd[cnt].pos.x - center.x;
            diff1.y = pd[cnt].pos.y - center.y;

            diff2.x = pd[tnc].pos.x - pd[cnt].pos.x;
            diff2.y = pd[tnc].pos.y - pd[cnt].pos.y;

            darea = diff1.x * diff2.y - diff1.y * diff2.x;

            // estimate the centroid
            area += darea;
            centroid.x += ( pd[cnt].pos.x + pd[tnc].pos.x + center.x ) / 3.0f * darea;
            centroid.y += ( pd[cnt].pos.y + pd[tnc].pos.y + center.y ) / 3.0f * darea;
        }

        diff1.x = pd[cnt].pos.x - center.x;
        diff1.y = pd[cnt].pos.y - center.y;

        diff2.x = pd[1].pos.x - pd[cnt].pos.x;
        diff2.y = pd[1].pos.y - pd[cnt].pos.y;

        darea = diff1.x * diff2.y - diff1.y * diff2.x;

        area += darea;
        centroid.x += ( pd[cnt].pos.x + pd[1].pos.x + center.x ) / 3.0f  * darea;
        centroid.y += ( pd[cnt].pos.y + pd[1].pos.y + center.y ) / 3.0f  * darea;
    }

    // is the volume valid?
    invalid = false;
    if ( pov->oct.mins[OCT_X]  >= pov->oct.maxs[OCT_X] ) invalid = true;
    else if ( pov->oct.mins[OCT_Y]  >= pov->oct.maxs[OCT_Y] ) invalid = true;
    else if ( pov->oct.mins[OCT_Z]  >= pov->oct.maxs[OCT_Z] ) invalid = true;
    else if ( pov->oct.mins[OCT_XY] >= pov->oct.maxs[OCT_XY] ) invalid = true;
    else if ( pov->oct.mins[OCT_YX] >= pov->oct.maxs[OCT_YX] ) invalid = true;

    if ( invalid )
    {
        pov->lod = -1;
        if ( NULL != pvolume )( *pvolume ) = 0;
        return false;
    }

    // determine the volume center
    if ( NULL != pcenter && std::abs( area ) > 0.0f )
    {
        ( *pcenter ).x = centroid.x / area;
        ( *pcenter ).y = centroid.y / area;
        ( *pcenter ).z = ( pov->oct.maxs[OCT_Z] + pov->oct.mins[OCT_Z] ) * 0.5f;
    }

    // determine the volume
    volume = ABS( area ) * ( pov->oct.maxs[OCT_Z] - pov->oct.mins[OCT_Z] );
    if ( NULL != pvolume )
    {
        ( *pvolume ) = volume;
    };

    return volume > 0.0f;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool CVolume_ctor( CVolume_t * pcv, const OVolume_t * pva, const OVolume_t * pvb )
{
    bool retval;
    CVolume_t cv;

    if ( pva->lod < 0 || pvb->lod < 0 ) return false;

    //---- reset the OVolume
    OVolume__ctor( &( pcv->ov ) );

    //---- do the preliminary collision test ----

    cv.ov = OVolume_intersect( pva, pvb );
    if ( cv.ov.lod < 0 )
    {
        return false;
    };

    //---- refine the collision volume ----

    cv.ov.lod = std::min( pva->lod, pvb->lod );
    retval = CVolume_refine( &cv );

    if ( NULL != pcv )
    {
        *pcv = cv;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool CVolume_refine( CVolume_t * pcv )
{
    /// @author BB
    /// @details determine which of the 16 possible intersection points are within both
    ///     square and diamond bounding volumes

    if ( NULL == pcv ) return false;

    if ( pcv->ov.oct.maxs[OCT_Z] <= pcv->ov.oct.mins[OCT_Z] )
    {
        pcv->ov.lod = -1;
        pcv->volume = 0;
        return false;
    }

    return OVolume_refine( &( pcv->ov ), &( pcv->center ), &( pcv->volume ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int cv_point_data_cmp( const void * pleft, const void * pright )
{
    int rv = 0;

    cv_point_data_t * pcv_left  = ( cv_point_data_t * )pleft;
    cv_point_data_t * pcv_right = ( cv_point_data_t * )pright;

    if ( pcv_left->rads < pcv_right->rads ) rv = -1;
    else if ( pcv_left->rads > pcv_right->rads ) rv = 1;

    return rv;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int oct_bb_to_points( const oct_bb_t * pbmp, fvec4_t   pos[], size_t pos_count )
{
    /// @author BB
    /// @details convert the corners of the level 1 bounding box to a point cloud
    ///      set pos[].w to zero for now, that the transform does not
    ///      shift the points while transforming them
    ///
    /// @note Make sure to set pos[].w to zero so that the bounding box will not be translated
    ///      then the transformation matrix is applied.
    ///
    /// @note The math for finding the corners of this bumper is not hard, but it is easy to make a mistake.
    ///      be careful if you modify anything.

    float ftmp;
    float val_x, val_y;

    int vcount = 0;

    if ( NULL == pbmp || NULL == pos || 0 == pos_count ) return 0;

    //---- the points along the y_max edge
    ftmp = 0.5f * ( pbmp->maxs[OCT_XY] + pbmp->maxs[OCT_YX] );  // the top point of the diamond
    if ( ftmp <= pbmp->maxs[OCT_Y] )
    {
        val_x = 0.5f * ( pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_YX] );
        val_y = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_y = pbmp->maxs[OCT_Y];

        val_x = pbmp->maxs[OCT_Y] - pbmp->maxs[OCT_YX];
        if ( val_x < pbmp->mins[OCT_X] )
        {
            val_x = pbmp->mins[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_x = pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_Y];
        if ( val_x > pbmp->maxs[OCT_X] )
        {
            val_x = pbmp->maxs[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the y_min edge
    ftmp = 0.5f * ( pbmp->mins[OCT_XY] + pbmp->mins[OCT_YX] );  // the top point of the diamond
    if ( ftmp >= pbmp->mins[OCT_Y] )
    {
        val_x = 0.5f * ( pbmp->mins[OCT_XY] - pbmp->mins[OCT_YX] );
        val_y = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_y = pbmp->mins[OCT_Y];

        val_x = pbmp->mins[OCT_XY] - pbmp->mins[OCT_Y];
        if ( val_x < pbmp->mins[OCT_X] )
        {
            val_x = pbmp->mins[OCT_X];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_x = pbmp->mins[OCT_Y] - pbmp->mins[OCT_YX];
        if ( val_x > pbmp->maxs[OCT_X] )
        {
            val_x = pbmp->maxs[OCT_X];
        }
        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the x_max edge
    ftmp = 0.5f * ( pbmp->maxs[OCT_XY] - pbmp->mins[OCT_YX] );  // the top point of the diamond
    if ( ftmp <= pbmp->maxs[OCT_X] )
    {
        val_y = 0.5f * ( pbmp->maxs[OCT_XY] + pbmp->mins[OCT_YX] );
        val_x = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_x = pbmp->maxs[OCT_X];

        val_y = pbmp->maxs[OCT_X] + pbmp->mins[OCT_YX];
        if ( val_y < pbmp->mins[OCT_Y] )
        {
            val_y = pbmp->mins[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_y = pbmp->maxs[OCT_XY] - pbmp->maxs[OCT_X];
        if ( val_y > pbmp->maxs[OCT_Y] )
        {
            val_y = pbmp->maxs[OCT_Y];
        }
        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    //---- the points along the x_min edge
    ftmp = 0.5f * ( pbmp->mins[OCT_XY] - pbmp->maxs[OCT_YX] );  // the left point of the diamond
    if ( ftmp >= pbmp->mins[OCT_X] )
    {
        val_y = 0.5f * ( pbmp->mins[OCT_XY] + pbmp->maxs[OCT_YX] );
        val_x = ftmp;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }
    else
    {
        val_x = pbmp->mins[OCT_X];

        val_y = pbmp->mins[OCT_XY] - pbmp->mins[OCT_X];
        if ( val_y < pbmp->mins[OCT_Y] )
        {
            val_y = pbmp->mins[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        val_y = pbmp->maxs[OCT_YX] + pbmp->mins[OCT_X];
        if ( val_y > pbmp->maxs[OCT_Y] )
        {
            val_y = pbmp->maxs[OCT_Y];
        }

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->maxs[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;

        pos[vcount].x = val_x;
        pos[vcount].y = val_y;
        pos[vcount].z = pbmp->mins[OCT_Z];
        pos[vcount].w = 0.0f;
        vcount++;
    }

    return vcount;
}

//--------------------------------------------------------------------------------------------
void points_to_oct_bb( oct_bb_t * pbmp, const fvec4_t pos[], const size_t pos_count )
{
    /// @author BB
    /// @details convert the new point cloud into a level 1 bounding box using a fvec4_t
    ///               array as the source
    oct_vec_t otmp;
    oct_vec_base_t pmins, pmaxs;

    if ( NULL == pbmp || NULL == pos || 0 == pos_count ) return;

    // resolve the pointers
    pmins = pbmp->mins;
    pmaxs = pbmp->maxs;

    // initialize using the first point
    oct_vec_ctor(otmp, fvec3_t(pos[0][kX],pos[0][kY],pos[0][kZ]));
    for ( Uint32 cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pmins[cnt] = pmaxs[cnt] = otmp[cnt];
    }

    // cycle through all other points
    for ( Uint32 cnt = 1; cnt < pos_count; cnt++ )
    {
        oct_vec_ctor( otmp, fvec3_t(pos[cnt][kX],pos[cnt][kY],pos[cnt][kZ]) );

        for ( Uint32 tnc = 0; tnc < OCT_COUNT; tnc++ )
        {
            pmins[tnc] = std::min( pmins[tnc], otmp[tnc] );
            pmaxs[tnc] = std::max( pmaxs[tnc], otmp[tnc] );
        }
    }

    oct_bb_validate( pbmp );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_downgrade( const oct_bb_t * psrc_bb, const bumper_t bump_stt, const bumper_t bump_base, bumper_t * pdst_bump, oct_bb_t * pdst_bb )
{
    /// @author BB
    /// @details convert a level 1 bumper to an "equivalent" level 0 bumper

    float val1, val2, val3, val4;

    // return if there is no source
    if ( NULL == psrc_bb ) return rv_error;

    //---- handle all of the pdst_bump data first
    if ( NULL != pdst_bump )
    {
        if ( 0.0f == bump_stt.height )
        {
            pdst_bump->height = 0.0f;
        }
        else
        {
            // have to use std::max here because the height can be distorted due
            // to make object-particle interactions easier (i.e. it allows you to
            // hit a grub bug with your hands)

            pdst_bump->height = std::max( bump_base.height, psrc_bb->maxs[OCT_Z] );
        }

        if ( 0.0f == bump_stt.size )
        {
            pdst_bump->size = 0.0f;
        }
        else
        {
            val1 = std::abs( psrc_bb->mins[OCT_X] );
			val2 = std::abs(psrc_bb->maxs[OCT_Y]);
			val3 = std::abs(psrc_bb->mins[OCT_Y]);
			val4 = std::abs(psrc_bb->maxs[OCT_Y]);
			pdst_bump->size = std::max({ val1, val2, val3, val4 });
        }

        if ( 0.0f == bump_stt.size_big )
        {
            pdst_bump->size_big = 0;
        }
        else
        {
			val1 = std::abs(psrc_bb->maxs[OCT_YX]);
			val2 = std::abs(psrc_bb->mins[OCT_YX]);
			val3 = std::abs(psrc_bb->maxs[OCT_XY]);
			val4 = std::abs(psrc_bb->mins[OCT_XY]);
            pdst_bump->size_big = std::max( std::max( val1, val2 ), std::max( val3, val4 ) );
        }
    }

    //---- handle all of the pdst_bb data second
    if ( NULL != pdst_bb )
    {
        // memcpy() can fail horribly if the domains overlap, so use memmove()
        if ( pdst_bb != psrc_bb )
        {
            memmove( pdst_bb, psrc_bb, sizeof( *pdst_bb ) );
        }

        if ( 0.0f == bump_stt.height )
        {
            pdst_bb->mins[OCT_Z] = pdst_bb->maxs[OCT_Z] = 0.0f;
        }
        else
        {
            // handle the vertical distortion the same as above
            pdst_bb->maxs[OCT_Z] = std::max( bump_base.height, psrc_bb->maxs[OCT_Z] );
        }

        // 0.0f == bump_stt.size is supposed to be shorthand for "this object doesn't interact
        // with anything", so we have to set all of the horizontal pdst_bb data to zero
        if ( 0.0f == bump_stt.size )
        {
            pdst_bb->mins[OCT_X ] = pdst_bb->maxs[OCT_X ] = 0.0f;
            pdst_bb->mins[OCT_Y ] = pdst_bb->maxs[OCT_Y ] = 0.0f;
            pdst_bb->mins[OCT_XY] = pdst_bb->maxs[OCT_XY] = 0.0f;
            pdst_bb->mins[OCT_YX] = pdst_bb->maxs[OCT_YX] = 0.0f;
        }
    }

    oct_bb_validate( pdst_bb );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_interpolate( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, float flip )
{
    int cnt;

    bool src1_empty, src2_empty;
    if ( NULL == pdst ) return rv_error;

    src1_empty = ( NULL == psrc1 || psrc1->empty );
    src2_empty = ( NULL == psrc2 || psrc2->empty );

    if ( src1_empty && src2_empty )
    {
        oct_bb_t::ctor( pdst );
        return rv_fail;
    }
    else if ( !src1_empty && 0.0f == flip )
    {
        return oct_bb_copy( pdst, psrc1 );
    }
    else if ( !src2_empty && 1.0f == flip )
    {
        return oct_bb_copy( pdst, psrc2 );
    }
    else if ( src1_empty || src2_empty )
    {
        oct_bb_t::ctor( pdst );
        return rv_fail;
    }

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] = psrc1->mins[cnt] + ( psrc2->mins[cnt] - psrc1->mins[cnt] ) * flip;
        pdst->maxs[cnt] = psrc1->maxs[cnt] + ( psrc2->maxs[cnt] - psrc1->maxs[cnt] ) * flip;
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
//inline
//--------------------------------------------------------------------------------------------

bool oct_vec_ctor(oct_vec_t ovec, const fvec3_t& pos)
{
	if (NULL == ovec)
	{
		return false;
	}
	ovec[OCT_X] = pos[kX];
	ovec[OCT_Y] = pos[kY];
	ovec[OCT_Z] = pos[kZ];
	ovec[OCT_XY] = pos[kX] + pos[kY];
	ovec[OCT_YX] = -pos[kX] + pos[kY];
	return true;
}

//--------------------------------------------------------------------------------------------
bool oct_vec_self_clear( oct_vec_t * ovec )
{
    int cnt;

    if ( NULL == ovec ) return false;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        ( *ovec )[cnt] = 0.0f;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool oct_vec_add_fvec3(const oct_vec_t osrc, const fvec3_t& fvec, oct_vec_t odst)
{
	if (NULL == odst)
	{
		return false;
	}
	oct_vec_ctor(odst, fvec);
	if (NULL != osrc)
	{
		for (size_t cnt = 0; cnt < OCT_COUNT; cnt++)
		{
			odst[cnt] += osrc[cnt];
		}
	}
	return true;
}

bool oct_vec_self_add_fvec3(oct_vec_t osrc, const fvec3_t& fvec)
{
	if (NULL == osrc)
	{
		return false;
	}
	oct_vec_t otmp;
	oct_vec_ctor(otmp, fvec);
	for (size_t cnt = 0; cnt < OCT_COUNT; cnt++)
	{
		osrc[cnt] += otmp[cnt];
	}
	return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oct_bb_t *oct_bb_t::ctor(oct_bb_t *self)
{
    if (!self) return nullptr;

	BLANK_STRUCT_PTR(self); /** @todo Remove this. */
    self->empty = true;

    return self;
}

void oct_bb_t::dtor(oct_bb_t *self)
{
	self->empty = true;
	BLANK_STRUCT_PTR(self); /** @todo Remove this. */
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_set_bumper( oct_bb_t * pobb, const bumper_t src )
{
    if ( NULL == pobb ) return rv_error;

    pobb->mins[OCT_X] = -src.size;
    pobb->maxs[OCT_X] =  src.size;

    pobb->mins[OCT_Y] = -src.size;
    pobb->maxs[OCT_Y] =  src.size;

    pobb->mins[OCT_XY] = -src.size_big;
    pobb->maxs[OCT_XY] =  src.size_big;

    pobb->mins[OCT_YX] = -src.size_big;
    pobb->maxs[OCT_YX] =  src.size_big;

    pobb->mins[OCT_Z] = -src.height;
    pobb->maxs[OCT_Z] =  src.height;

    return oct_bb_validate( pobb );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_copy( oct_bb_t * pdst, const oct_bb_t * psrc )
{
    if ( NULL == pdst ) return rv_error;

    if ( NULL == psrc )
    {
        oct_bb_t::ctor( pdst );
        return rv_success;
    }

    memmove( pdst, psrc, sizeof( *pdst ) );

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_validate( oct_bb_t * pobb )
{
    if ( NULL == pobb ) return rv_error;

    pobb->empty = oct_bb_empty_raw( pobb );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_empty_raw( const oct_bb_t * pbb )
{
    int cnt;
    bool rv = false;

    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        if ( pbb->mins[cnt] >= pbb->maxs[cnt] )
        {
            rv = true;
            break;
        }
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_empty( const oct_bb_t * pbb )
{
    if ( NULL == pbb || pbb->empty ) return true;

    return oct_bb_empty_raw( pbb );
}

//--------------------------------------------------------------------------------------------
egolib_rv  oct_bb_set_ovec( oct_bb_t * pobb, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pobb ) return rv_error;

    if ( NULL == ovec )
    {
        oct_bb_t::ctor( pobb ) ;
        return rv_fail;
    }

    // copy the data over
    for ( cnt = 0; cnt < OCT_COUNT; cnt ++ )
    {
        pobb->mins[cnt] = pobb->maxs[cnt] = ovec[cnt];
    }

    // this is true by the definition of this function
    pobb->empty = true;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oct_bb_t * oct_bb_ctor_index( oct_bb_t * pobb, int index )
{
    if ( NULL == pobb ) return NULL;

    if ( index >= 0 && index < OCT_COUNT )
    {
        pobb->mins[index] = pobb->maxs[index] = 0.0f;
        pobb->empty = true;
    }

    return pobb;
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_copy_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index )
{
    if ( NULL == pdst ) return rv_error;

    if ( NULL == psrc )
    {
        oct_bb_ctor_index( pdst, index );

        return rv_success;
    }

    pdst->mins[index] = psrc->mins[index];
    pdst->maxs[index] = psrc->maxs[index];

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_validate_index( oct_bb_t * pobb, int index )
{
    if ( NULL == pobb ) return rv_error;

    if ( oct_bb_empty_index( pobb, index ) )
    {
        pobb->empty = true;
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_empty_index_raw( const oct_bb_t * pbb, int index )
{
    return ( pbb->mins[index] >= pbb->maxs[index] );
}

//--------------------------------------------------------------------------------------------
bool oct_bb_empty_index( const oct_bb_t * pbb, int index )
{
    if ( NULL == pbb || pbb->empty ) return true;

    if ( index < 0 || index >= OCT_COUNT ) return true;

    return oct_bb_empty_index_raw( pbb, index );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_union_index( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst, int index )
{
    /// @author BB
    /// @details find the union of two oct_bb_t

    bool src1_empty, src2_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    src1_empty = ( NULL == psrc1 );
    src2_empty = ( NULL == psrc2 );

    if ( src1_empty && src2_empty )
    {
        oct_bb_ctor_index( pdst, index );
        return rv_fail;
    }
    else if ( src2_empty )
    {
        oct_bb_copy_index( pdst, psrc1, index );
        return rv_success;
    }
    else if ( src1_empty )
    {
        oct_bb_copy_index( pdst, psrc2, index );
        return rv_success;
    }

    // no simple case, do the hard work

    pdst->mins[index]  = std::min( psrc1->mins[index],  psrc2->mins[index] );
    pdst->maxs[index]  = std::max( psrc1->maxs[index],  psrc2->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_intersection_index( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, int index )
{
    /// @author BB
    /// @details find the intersection of two oct_bb_t

    bool src1_empty, src2_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    src1_empty = ( NULL == psrc1 || psrc1->empty );
    src2_empty = ( NULL == psrc2 || psrc2->empty );

    if ( src1_empty && src2_empty )
    {
        oct_bb_ctor_index( pdst, index );
        return rv_success;
    }

    // no simple case. do the hard work

    pdst->mins[index]  = std::max( psrc1->mins[index],  psrc2->mins[index] );
    pdst->maxs[index]  = std::min( psrc1->maxs[index],  psrc2->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
egolib_rv  oct_bb_self_union_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index )
{
    /// @author BB
    /// @details find the union of two oct_bb_t

    bool src_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    src_empty = ( NULL == psrc );

    if ( src_empty )
    {
        // !!!! DO NOTHING !!!!
        return rv_success;
    }

    // no simple case, do the hard work

    pdst->mins[index] = std::min(pdst->mins[index], psrc->mins[index]);
    pdst->maxs[index]  = std::max( pdst->maxs[index],  psrc->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_self_intersection_index( oct_bb_t * pdst, const oct_bb_t * psrc, int index )
{
    /// @author BB
    /// @details find the intersection of two oct_bb_t

    bool src_empty;

    if ( NULL == pdst ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    src_empty = ( NULL == psrc || psrc->empty );

    if ( src_empty )
    {
        return rv_fail;
    }

    // no simple case. do the hard work

    pdst->mins[index]  = std::max( pdst->mins[index],  psrc->mins[index] );
    pdst->maxs[index]  = std::min( pdst->maxs[index],  psrc->maxs[index] );

    return oct_bb_validate_index( pdst, index );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_union( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst )
{
    /// @author BB
    /// @details find the union of two oct_bb_t

    bool src1_null, src2_null;
    int cnt;

    if ( NULL == pdst ) return rv_error;

    src1_null = ( NULL == psrc1 );
    src2_null = ( NULL == psrc2 );

    if ( src1_null && src2_null )
    {
        oct_bb_t::ctor( pdst );
        return rv_fail;
    }
    else if ( src2_null )
    {
        return oct_bb_copy( pdst, psrc1 );
    }
    else if ( src1_null )
    {
        return oct_bb_copy( pdst, psrc2 );
    }

    // no simple case, do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = std::min( psrc1->mins[cnt],  psrc2->mins[cnt] );
        pdst->maxs[cnt]  = std::max( psrc1->maxs[cnt],  psrc2->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_intersection( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst )
{
    /// @author BB
    /// @details find the intersection of two oct_bb_t

    bool src1_empty, src2_empty;
    int cnt;

    if ( NULL == pdst ) return rv_error;

    src1_empty = ( NULL == psrc1 || psrc1->empty );
    src2_empty = ( NULL == psrc2 || psrc2->empty );

    if ( src1_empty && src2_empty )
    {
        oct_bb_t::ctor( pdst );
        return rv_fail;
    }

    // no simple case. do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = std::max( psrc1->mins[cnt],  psrc2->mins[cnt] );
        pdst->maxs[cnt]  = std::min( psrc1->maxs[cnt],  psrc2->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_self_union( oct_bb_t * pdst, const oct_bb_t * psrc )
{
    /// @author BB
    /// @details find the union of two oct_bb_t

    bool src_null;
    int cnt;

    if ( NULL == pdst ) return rv_error;

    src_null = ( NULL == psrc );

    if ( src_null )
    {
        // !!!! DO NOTHING !!!!
        return rv_success;
    }

    // no simple case, do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = std::min( pdst->mins[cnt],  psrc->mins[cnt] );
        pdst->maxs[cnt]  = std::max( pdst->maxs[cnt],  psrc->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_self_intersection( oct_bb_t * pdst, const oct_bb_t * psrc )
{
    /// @author BB
    /// @details find the intersection of two oct_bb_t

    bool src_empty;
    int cnt;

    if ( NULL == pdst ) return rv_error;

    src_empty = ( NULL == psrc || psrc->empty );

    if ( src_empty )
    {
        return rv_fail;
    }

    // no simple case. do the hard work
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt]  = std::max( pdst->mins[cnt],  psrc->mins[cnt] );
        pdst->maxs[cnt]  = std::min( pdst->maxs[cnt],  psrc->maxs[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_add_fvec3(const oct_bb_t *psrc, const fvec3_t& vec, oct_bb_t *pdst)
{
	if (NULL == pdst) return rv_error;

	if (NULL == psrc)
	{
		oct_bb_t::ctor(pdst);
	}
	else
	{
		oct_bb_copy(pdst, psrc);
	}

	pdst->mins[OCT_X] += vec[kX];
	pdst->maxs[OCT_X] += vec[kX];

	pdst->mins[OCT_Y] += vec[kY];
	pdst->maxs[OCT_Y] += vec[kY];

	pdst->mins[OCT_XY] += vec[kX] + vec[kY];
	pdst->maxs[OCT_XY] += vec[kX] + vec[kY];

	pdst->mins[OCT_YX] += -vec[kX] + vec[kY];
	pdst->maxs[OCT_YX] += -vec[kX] + vec[kY];

	pdst->mins[OCT_Z] += vec[kZ];
	pdst->maxs[OCT_Z] += vec[kZ];

	return oct_bb_validate(pdst);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_self_add_fvec3(oct_bb_t * pdst, const fvec3_t& vec)
{
	if (NULL == pdst) return rv_error;

	pdst->mins[OCT_X] += vec[kX];
	pdst->maxs[OCT_X] += vec[kX];

	pdst->mins[OCT_Y] += vec[kY];
	pdst->maxs[OCT_Y] += vec[kY];

	pdst->mins[OCT_XY] += vec[kX] + vec[kY];
	pdst->maxs[OCT_XY] += vec[kX] + vec[kY];

	pdst->mins[OCT_YX] += -vec[kX] + vec[kY];
	pdst->maxs[OCT_YX] += -vec[kX] + vec[kY];

	pdst->mins[OCT_Z] += vec[kZ];
	pdst->maxs[OCT_Z] += vec[kZ];

	return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_add_ovec( const oct_bb_t * psrc, const oct_vec_t ovec, oct_bb_t * pdst )
{
    /// @author BB
    /// @details shift the bounding box by the vector ovec

    int cnt;

    if ( NULL == pdst ) return rv_error;

    if ( NULL == psrc )
    {
        oct_bb_t::ctor( pdst );
    }
    else
    {
        oct_bb_copy( pdst, psrc );
    }

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] += ovec[cnt];
        pdst->maxs[cnt] += ovec[cnt];
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_self_add_ovec( oct_bb_t * pdst, const oct_vec_t ovec )
{
    /// @author BB
    /// @details shift the bounding box by the vector ovec

    int cnt;

    if ( NULL == pdst ) return rv_error;

    if ( NULL == ovec ) return rv_success;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] += ovec[cnt];
        pdst->maxs[cnt] += ovec[cnt];
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv  oct_bb_self_sum_ovec( oct_bb_t * pdst, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pdst ) return rv_fail;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] = std::min( pdst->mins[cnt], ovec[cnt] );
        pdst->maxs[cnt] = std::max( pdst->maxs[cnt], ovec[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv  oct_bb_self_grow( oct_bb_t * pdst, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pdst ) return rv_error;
    if ( NULL == ovec ) return rv_error;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        pdst->mins[cnt] = pdst->mins[cnt] - ABS( ovec[cnt] );
        pdst->maxs[cnt] = pdst->maxs[cnt] + ABS( ovec[cnt] );
    }

    return oct_bb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
bool oct_bb_point_inside( const oct_bb_t * pobb, const oct_vec_t ovec )
{
    int cnt;

    if ( NULL == pobb || pobb->empty ) return false;

    if ( NULL == ovec ) return false;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        if ( ovec[cnt] < pobb->mins[cnt] ) return false;
        if ( ovec[cnt] > pobb->maxs[cnt] ) return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_lhs_contains_rhs( const oct_bb_t * plhs, const oct_bb_t * prhs )
{
    int cnt;

    if ( NULL == plhs || plhs->empty ) return false;

    if ( NULL == prhs ) return false;

    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        if ( prhs->maxs[cnt] > plhs->maxs[cnt] ) return false;
        if ( prhs->mins[cnt] < plhs->mins[cnt] ) return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_get_mids( const oct_bb_t * pbb, oct_vec_t mids )
{
    if ( NULL == pbb || NULL == mids ) return false;

    if ( oct_bb_empty( pbb ) )
    {
        memmove( mids, pbb->maxs, sizeof( oct_vec_t ) );
    }
    else
    {
        int cnt;

        for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
        {
            mids[cnt] = 0.5f * ( pbb->mins[cnt] + pbb->maxs[cnt] );
        }
    }

    return true;
}
