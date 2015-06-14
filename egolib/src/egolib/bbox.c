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
#include "egolib/Math/AABB.hpp"

//--------------------------------------------------------------------------------------------
int oct_bb_to_points(const oct_bb_t& self, fvec4_t pos[], size_t pos_count)
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

    if (!pos || 0 == pos_count ) return 0;

    //---- the points along the y_max edge
	ftmp = 0.5f * (self.maxs[OCT_XY] + self.maxs[OCT_YX]);  // the top point of the diamond
	if (ftmp <= self.maxs[OCT_Y])
    {
		val_x = 0.5f * (self.maxs[OCT_XY] - self.maxs[OCT_YX]);
        val_y = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_y = self.maxs[OCT_Y];

		val_x = self.maxs[OCT_Y] - self.maxs[OCT_YX];
		if (val_x < self.mins[OCT_X])
        {
			val_x = self.mins[OCT_X];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_x = self.maxs[OCT_XY] - self.maxs[OCT_Y];
		if (val_x > self.maxs[OCT_X])
        {
			val_x = self.maxs[OCT_X];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    //---- the points along the y_min edge
	ftmp = 0.5f * (self.mins[OCT_XY] + self.mins[OCT_YX]);  // the top point of the diamond
	if (ftmp >= self.mins[OCT_Y])
    {
		val_x = 0.5f * (self.mins[OCT_XY] - self.mins[OCT_YX]);
        val_y = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_y = self.mins[OCT_Y];

		val_x = self.mins[OCT_XY] - self.mins[OCT_Y];
		if (val_x < self.mins[OCT_X])
        {
			val_x = self.mins[OCT_X];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_x = self.mins[OCT_Y] - self.mins[OCT_YX];
		if (val_x > self.maxs[OCT_X])
        {
			val_x = self.maxs[OCT_X];
        }
        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    //---- the points along the x_max edge
	ftmp = 0.5f * (self.maxs[OCT_XY] - self.mins[OCT_YX]);  // the top point of the diamond
	if (ftmp <= self.maxs[OCT_X])
    {
		val_y = 0.5f * (self.maxs[OCT_XY] + self.mins[OCT_YX]);
        val_x = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_x = self.maxs[OCT_X];

		val_y = self.maxs[OCT_X] + self.mins[OCT_YX];
		if (val_y < self.mins[OCT_Y])
        {
			val_y = self.mins[OCT_Y];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_y = self.maxs[OCT_XY] - self.maxs[OCT_X];
		if (val_y > self.maxs[OCT_Y])
        {
			val_y = self.maxs[OCT_Y];
        }
        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    //---- the points along the x_min edge
	ftmp = 0.5f * (self.mins[OCT_XY] - self.maxs[OCT_YX]);  // the left point of the diamond
	if (ftmp >= self.mins[OCT_X])
    {
		val_y = 0.5f * (self.mins[OCT_XY] + self.maxs[OCT_YX]);
        val_x = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_x = self.mins[OCT_X];

		val_y = self.mins[OCT_XY] - self.mins[OCT_X];
		if (val_y < self.mins[OCT_Y])
        {
			val_y = self.mins[OCT_Y];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_y = self.maxs[OCT_YX] + self.mins[OCT_X];
		if (val_y > self.maxs[OCT_Y])
        {
			val_y = self.maxs[OCT_Y];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self.mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    return vcount;
}

//--------------------------------------------------------------------------------------------
/**
 * @brief
 *  Set the values of this octagonal bounding box such that it encloses the specified point cloud.
 * @param self
 *  this octagonal bounding box
 * @param points
 *  an array of points
 * @param numberOfPoints
 *  the number of points in the array
 */
void points_to_oct_bb(oct_bb_t *self, const fvec4_t points[], const size_t numberOfPoints)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!points)
    {
        throw std::invalid_argument("nullptr == points");
    }
    if (!numberOfPoints)
    {
        throw std::invalid_argument("0 == numberOfPoints");
    }

    // Initialize the octagonal bounding box using the first point.
    oct_vec_v2_t otmp(fvec3_t(points[0][kX], points[0][kY], points[0][kZ]));
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        self->mins[i] = self->maxs[i] = otmp[i];
    }

    // Join the octagonal bounding box (containing only the first point) with all other points.
    for (size_t i = 1; i < numberOfPoints; ++i)
    {
        otmp.ctor(fvec3_t(points[i][kX], points[i][kY], points[i][kZ]));

        for (size_t j = 0; j < OCT_COUNT; ++j)
        {
            self->mins[j] = std::min(self->mins[j], otmp[j]);
            self->maxs[j] = std::max(self->maxs[j], otmp[j]);
        }
    }

    oct_bb_t::validate(self);
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
            *pdst_bb = *psrc_bb;
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

        oct_bb_t::validate(pdst_bb);
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::interpolate(const oct_bb_t& src1, const oct_bb_t& src2, oct_bb_t& dst, float flip)
{
	if (src1.empty && src2.empty) {
        oct_bb_t::ctor(&dst);
        return rv_fail;
    } else if (!src1.empty && 0.0f == flip) {
        return oct_bb_copy(&dst, &src1);
    } else if (!src2.empty && 1.0f == flip) {
        return oct_bb_copy(&dst, &src2);
    } else if (src1.empty || src2.empty) {
        oct_bb_t::ctor(&dst);
        return rv_fail;
    }

    for (size_t i = 0; i < (size_t)OCT_COUNT; ++i) {
        dst.mins[i] = src1.mins[i] + (src2.mins[i] - src1.mins[i]) * flip;
        dst.maxs[i] = src1.maxs[i] + (src2.maxs[i] - src1.maxs[i]) * flip;
    }

    return oct_bb_t::validate(&dst);
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
bool oct_vec_add_fvec3(const oct_vec_v2_t& osrc, const fvec3_t& fvec, oct_vec_v2_t& odst)
{
    odst.ctor(fvec);
	{
		for (size_t cnt = 0; cnt < OCT_COUNT; cnt++)
		{
			odst[cnt] += osrc[cnt];
		}
	}
	return true;
}
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oct_bb_t *oct_bb_t::ctor(oct_bb_t *self)
{
    if (!self) return nullptr;

    self->mins.setZero();
    self->maxs.setZero();
    self->empty = true;

    return self;
}

void oct_bb_t::dtor(oct_bb_t *self)
{
	self->empty = true;
    self->maxs.setZero();
    self->mins.setZero();
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_copy(oct_bb_t *self, const oct_bb_t *other)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!other)
    {
        throw std::invalid_argument("nullptr == other");
    }
    self->mins = other->mins;
    self->maxs = other->maxs;
    self->empty = other->empty;
    return oct_bb_t::validate(self);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::validate(oct_bb_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    self->empty = oct_bb_t::empty_raw(self);
    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::empty_raw(const oct_bb_t *self)
{
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        if (self->mins[i] > self->maxs[i])
        {
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_empty( const oct_bb_t * pbb )
{
    if ( NULL == pbb || pbb->empty ) return true;

    return oct_bb_t::empty_raw( pbb );
}

//--------------------------------------------------------------------------------------------
void oct_bb_set_ovec(oct_bb_t *self, const oct_vec_v2_t& v)
{
    if (!self) throw std::invalid_argument("nullptr == self");
    self->mins = v;
    self->maxs = v;
    // The octagonal bounding box is not empty.
    self->empty = false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_validate_index(oct_bb_t *self, int index)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (oct_bb_empty_index(self, index))
    {
        self->empty = true;
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
egolib_rv oct_bb_t::join(const oct_bb_t& other, int index)
{
    if (index < 0 || index >= OCT_COUNT)
    {
        return rv_error;
    }

    // No simple cases, do the hard work.
    mins[index] = std::min(mins[index], other.mins[index]);
    maxs[index] = std::max(maxs[index], other.maxs[index] );

    return oct_bb_validate_index(this, index);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::cut(const oct_bb_t& other, int index)
{
    if (index < 0 || index >= OCT_COUNT)
    {
        return rv_error;
    }

    if (other.empty) /// @todo Obviously the author does not know how set intersection works.
    {
        return rv_fail;
    }

    // No simple case. do the hard work.
    mins[index]  = std::max(mins[index], other.mins[index]);
    maxs[index]  = std::min(maxs[index], other.maxs[index]);

    return oct_bb_validate_index(this, index);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_join( const oct_bb_t * psrc1, const oct_bb_t  * psrc2, oct_bb_t * pdst )
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

    return oct_bb_t::validate( pdst );
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

    return oct_bb_t::validate( pdst );
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::join(const oct_vec_v2_t& v)
{
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        mins[i] = std::min(mins[i], v[i]);
        maxs[i] = std::max(maxs[i], v[i]);
    }
    return oct_bb_t::validate(this);
}

egolib_rv oct_bb_t::join(const oct_bb_t& other)
{
    // No simple case, do the hard work.
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        mins[i] = std::min(mins[i], other.mins[i]);
        maxs[i] = std::max(maxs[i], other.maxs[i]);
    }

    return oct_bb_t::validate(this);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::cut(const oct_bb_t& other)
{
    if (other.empty) /// @todo Obviously the author does not know how set intersection works.
    {
        return rv_fail;
    }

    // No simple case, do the hard work.
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        mins[i] = std::max(mins[i], other.mins[i]);
        maxs[i] = std::min(maxs[i], other.maxs[i]);
    }

    return oct_bb_t::validate(this);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::translate(const oct_bb_t& src, const fvec3_t& t, oct_bb_t& dst) {
    dst = src;
    dst.translate(t);
	return oct_bb_t::validate(&dst);
}

egolib_rv oct_bb_t::translate(const oct_bb_t& src, const oct_vec_v2_t& t, oct_bb_t& dst) {
    dst = src;
    dst.translate(oct_vec_v2_t(t));
    return oct_bb_t::validate(&dst);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_self_grow(oct_bb_t *self, const oct_vec_v2_t& v)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        self->mins[i] -= std::abs(v[i]);
        self->maxs[i] += std::abs(v[i]);
    }

    return oct_bb_t::validate(self);
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::contains(const oct_bb_t *self, const oct_vec_v2_t& point)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (self->empty)
    {
        return false;
    }
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        if (point[i] < self->mins[i]) return false;
        if (point[i] > self->maxs[i]) return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::contains(const oct_bb_t *self, const oct_bb_t *other)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!other)
    {
        throw std::invalid_argument("nullptr == other");
    }
    // If the right-hand side is empty ...
    if (other->empty)
    {
        // ... it is always contained in the left-hand side.
        return true;
    }
    // If the left-hand side is empty ...
    if (self->empty)
    {
        // ... it can not contain the right-hand side as the right hand-side is non-empty by the above.
        return false;
    }
    // At this point, the left-hand side as well as the right-hand side are non-empty.
    // Perform normal tests.
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        if (other->maxs[i] > self->maxs[i]) return false;
        if (other->mins[i] < self->mins[i]) return false;
    }
    return true;
}
