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

const oct_vec_v2_t oct_vec_v2_t::Zero = oct_vec_v2_t();

int oct_bb_t::to_points(const oct_bb_t& self, Vector4f pos[], size_t pos_count)
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
	ftmp = 0.5f * (self._maxs[OCT_XY] + self._maxs[OCT_YX]);  // the top point of the diamond
	if (ftmp <= self._maxs[OCT_Y])
    {
		val_x = 0.5f * (self._maxs[OCT_XY] - self._maxs[OCT_YX]);
        val_y = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_y = self._maxs[OCT_Y];

		val_x = self._maxs[OCT_Y] - self._maxs[OCT_YX];
		if (val_x < self._mins[OCT_X])
        {
			val_x = self._mins[OCT_X];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_x = self._maxs[OCT_XY] - self._maxs[OCT_Y];
		if (val_x > self._maxs[OCT_X])
        {
			val_x = self._maxs[OCT_X];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    //---- the points along the y_min edge
	ftmp = 0.5f * (self._mins[OCT_XY] + self._mins[OCT_YX]);  // the top point of the diamond
	if (ftmp >= self._mins[OCT_Y])
    {
		val_x = 0.5f * (self._mins[OCT_XY] - self._mins[OCT_YX]);
        val_y = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_y = self._mins[OCT_Y];

		val_x = self._mins[OCT_XY] - self._mins[OCT_Y];
		if (val_x < self._mins[OCT_X])
        {
			val_x = self._mins[OCT_X];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_x = self._mins[OCT_Y] - self._mins[OCT_YX];
		if (val_x > self._maxs[OCT_X])
        {
			val_x = self._maxs[OCT_X];
        }
        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    //---- the points along the x_max edge
	ftmp = 0.5f * (self._maxs[OCT_XY] - self._mins[OCT_YX]);  // the top point of the diamond
	if (ftmp <= self._maxs[OCT_X])
    {
		val_y = 0.5f * (self._maxs[OCT_XY] + self._mins[OCT_YX]);
        val_x = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_x = self._maxs[OCT_X];

		val_y = self._maxs[OCT_X] + self._mins[OCT_YX];
		if (val_y < self._mins[OCT_Y])
        {
			val_y = self._mins[OCT_Y];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_y = self._maxs[OCT_XY] - self._maxs[OCT_X];
		if (val_y > self._maxs[OCT_Y])
        {
			val_y = self._maxs[OCT_Y];
        }
        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    //---- the points along the x_min edge
	ftmp = 0.5f * (self._mins[OCT_XY] - self._maxs[OCT_YX]);  // the left point of the diamond
	if (ftmp >= self._mins[OCT_X])
    {
		val_y = 0.5f * (self._mins[OCT_XY] + self._maxs[OCT_YX]);
        val_x = ftmp;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }
    else
    {
		val_x = self._mins[OCT_X];

		val_y = self._mins[OCT_XY] - self._mins[OCT_X];
		if (val_y < self._mins[OCT_Y])
        {
			val_y = self._mins[OCT_Y];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

		val_y = self._maxs[OCT_YX] + self._mins[OCT_X];
		if (val_y > self._maxs[OCT_Y])
        {
			val_y = self._maxs[OCT_Y];
        }

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._maxs[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;

        pos[vcount][kX] = val_x;
        pos[vcount][kY] = val_y;
		pos[vcount][kZ] = self._mins[OCT_Z];
        pos[vcount][kW] = 0.0f;
        vcount++;
    }

    return vcount;
}

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
void oct_bb_t::points_to_oct_bb(oct_bb_t& self, const Vector4f points[], const size_t numberOfPoints)
{
    if (!points)
    {
        throw std::invalid_argument("nullptr == points");
    }
    if (!numberOfPoints)
    {
        throw std::invalid_argument("0 == numberOfPoints");
    }

    // Initialize the octagonal bounding box using the first point.
    oct_vec_v2_t otmp(Vector3f(points[0][kX], points[0][kY], points[0][kZ]));
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        self._mins[i] = self._maxs[i] = otmp[i];
    }

    // Join the octagonal bounding box (containing only the first point) with all other points.
    for (size_t i = 1; i < numberOfPoints; ++i)
    {
        otmp = oct_vec_v2_t(Vector3f(points[i][kX], points[i][kY], points[i][kZ]));

        for (size_t j = 0; j < OCT_COUNT; ++j)
        {
            self._mins[j] = std::min(self._mins[j], otmp[j]);
            self._maxs[j] = std::max(self._maxs[j], otmp[j]);
        }
    }

    self._empty = oct_bb_t::empty_raw(self);
}

egolib_rv oct_bb_t::downgrade(const oct_bb_t& psrc_bb, const bumper_t& bump_stt, const bumper_t& bump_base, oct_bb_t& pdst_bb)
{
	/// @author BB
	/// @details convert a level 1 bumper to an "equivalent" level 0 bumper

	if (&pdst_bb != &psrc_bb)
	{
		pdst_bb = psrc_bb;
	}

	if (0.0f == bump_stt.height)
	{
		pdst_bb._mins[OCT_Z] = pdst_bb._maxs[OCT_Z] = 0.0f;
	}
	else
	{
		// handle the vertical distortion the same as above
		pdst_bb._maxs[OCT_Z] = std::max(bump_base.height, psrc_bb._maxs[OCT_Z]);
	}

	// 0.0f == bump_stt.size is supposed to be shorthand for "this object doesn't interact
	// with anything", so we have to set all of the horizontal pdst_bb data to zero
	if (0.0f == bump_stt.size)
	{
		pdst_bb._mins[OCT_X] = pdst_bb._maxs[OCT_X] = 0.0f;
		pdst_bb._mins[OCT_Y] = pdst_bb._maxs[OCT_Y] = 0.0f;
		pdst_bb._mins[OCT_XY] = pdst_bb._maxs[OCT_XY] = 0.0f;
		pdst_bb._mins[OCT_YX] = pdst_bb._maxs[OCT_YX] = 0.0f;
	}

    pdst_bb._empty = oct_bb_t::empty_raw(pdst_bb);

	return rv_success;
}

egolib_rv oct_bb_t::downgrade(const oct_bb_t& psrc_bb, const bumper_t& bump_stt, const bumper_t& bump_base, bumper_t& pdst_bump)
{
    /// @author BB
    /// @details convert a level 1 bumper to an "equivalent" level 0 bumper

    float val1, val2, val3, val4;

        if ( 0.0f == bump_stt.height )
        {
            pdst_bump.height = 0.0f;
        }
        else
        {
            // have to use std::max here because the height can be distorted due
            // to make object-particle interactions easier (i.e. it allows you to
            // hit a grub bug with your hands)

            pdst_bump.height = std::max( bump_base.height, psrc_bb._maxs[OCT_Z] );
        }

        if ( 0.0f == bump_stt.size )
        {
            pdst_bump.size = 0.0f;
        }
        else
        {
            val1 = std::abs(psrc_bb._mins[OCT_X]);
			val2 = std::abs(psrc_bb._maxs[OCT_Y]);
			val3 = std::abs(psrc_bb._mins[OCT_Y]);
			val4 = std::abs(psrc_bb._maxs[OCT_Y]);
			pdst_bump.size = std::max({ val1, val2, val3, val4 });
        }

        if ( 0.0f == bump_stt.size_big )
        {
            pdst_bump.size_big = 0;
        }
        else
        {
			val1 = std::abs(psrc_bb._maxs[OCT_YX]);
			val2 = std::abs(psrc_bb._mins[OCT_YX]);
			val3 = std::abs(psrc_bb._maxs[OCT_XY]);
			val4 = std::abs(psrc_bb._mins[OCT_XY]);
			pdst_bump.size_big = std::max({ val1, val2, val3, val4 });
        }

		return rv_success;
}

void oct_bb_t::interpolate(const oct_bb_t& src1, const oct_bb_t& src2, oct_bb_t& dst, float flip)
{
	if (src1._empty && src2._empty) {
        dst = oct_bb_t();
    } else if (!src1._empty && 0.0f == flip) {
        oct_bb_t::copy(dst, src1);
    } else if (!src2._empty && 1.0f == flip) {
        oct_bb_t::copy(dst, src2);
    } else if (src1._empty || src2._empty) {
        dst = oct_bb_t();
    }

    for (size_t i = 0; i < (size_t)OCT_COUNT; ++i) {
        dst._mins[i] = src1._mins[i] + (src2._mins[i] - src1._mins[i]) * flip;
        dst._maxs[i] = src1._maxs[i] + (src2._maxs[i] - src1._maxs[i]) * flip;
    }

    dst._empty = oct_bb_t::empty_raw(dst);
}

//--------------------------------------------------------------------------------------------
void oct_bb_t::copy(oct_bb_t& self, const oct_bb_t& other)
{
    self._mins = other._mins;
    self._maxs = other._maxs;
    self._empty = other._empty;
    self._empty = oct_bb_t::empty_raw(self);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::validate(oct_bb_t& self)
{
    self._empty = oct_bb_t::empty_raw(self);
    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::empty_raw(const oct_bb_t& self)
{
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        if (self._mins[i] > self._maxs[i])
        {
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::empty(const oct_bb_t& self)
{
    if (self._empty ) return true;

    return oct_bb_t::empty_raw(self);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void oct_bb_t::validate_index(oct_bb_t& self, int index)
{
    if (oct_bb_t::empty_index(self, index))
    {
        self._empty = true;
    }
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::empty_index_raw(const oct_bb_t& self, int index)
{
    return (self._mins[index] >= self._maxs[index] );
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::empty_index(const oct_bb_t& self, int index)
{
	if (self._empty) {
		return true;
	}
	if (index < 0 || index >= OCT_COUNT) {
		return true;
	}
    return oct_bb_t::empty_index_raw(self, index);
}

//--------------------------------------------------------------------------------------------
void oct_bb_t::join(const oct_bb_t& other, int index)
{
    if (index < 0 || index >= OCT_COUNT)
    {
		throw std::runtime_error("index out of bounds");
    }

    // No simple cases, do the hard work.
    _mins[index] = std::min(_mins[index], other._mins[index]);
    _maxs[index] = std::max(_maxs[index], other._maxs[index] );

    oct_bb_t::validate_index(*this, index);
}

void oct_bb_t::join(const oct_bb_t& src1, const oct_bb_t& src2, oct_bb_t& dst)
{
	// @todo Obviously the author does not know how set union works.
	// no simple case, do the hard work
	for (size_t i = 0; i < (size_t)OCT_COUNT; ++i) {
		dst._mins[i] = std::min(src1._mins[i], src2._mins[i]);
		dst._maxs[i] = std::max(src1._maxs[i], src2._maxs[i]);
	}

    dst._empty = oct_bb_t::empty_raw(dst);
}

void oct_bb_t::join(const oct_vec_v2_t& v)
{
	// @todo Obviously the author does not know how set union works.
	for (size_t i = 0; i < OCT_COUNT; ++i)
	{
		_mins[i] = std::min(_mins[i], v[i]);
		_maxs[i] = std::max(_maxs[i], v[i]);
	}
    this->_empty = oct_bb_t::empty_raw(*this);
}

void oct_bb_t::join(const oct_bb_t& other)
{
	// @todo Obviously the author does not know how set union works.
	// No simple case, do the hard work.
	for (size_t i = 0; i < OCT_COUNT; ++i)
	{
		_mins[i] = std::min(_mins[i], other._mins[i]);
		_maxs[i] = std::max(_maxs[i], other._maxs[i]);
	}
    this->_empty = oct_bb_t::empty_raw(*this);
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::cut(const oct_bb_t& other, int index)
{
    if (index < 0 || index >= OCT_COUNT)
    {
		throw std::runtime_error("index out of bounds");
    }

	// @todo Obviously the author does not know how set intersection works.
    if (other._empty) {
        return rv_fail;
    }

    // No simple case. do the hard work.
    _mins[index]  = std::max(_mins[index], other._mins[index]);
    _maxs[index]  = std::min(_maxs[index], other._maxs[index]);

    oct_bb_t::validate_index(*this, index);
	return rv_success;
}

egolib_rv oct_bb_t::intersection(const oct_bb_t& src1, const oct_bb_t& src2, oct_bb_t& dst)
{
	/// @todo Obviously the author does not know how set intersection works.
    if (src1._empty && src2._empty) {
        dst = oct_bb_t();
        return rv_fail;
    }

    // no simple case. do the hard work
    for (size_t i = 0; i < (size_t)OCT_COUNT; ++i) {
        dst._mins[i]  = std::max(src1._mins[i], src2._mins[i]);
        dst._maxs[i]  = std::min(src1._maxs[i], src2._maxs[i]);
    }

    dst._empty = oct_bb_t::empty_raw(dst);
	return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv oct_bb_t::cut(const oct_bb_t& other)
{
	/// @todo Obviously the author does not know how set intersection works.
    if (other._empty) {
        return rv_fail;
    }

    // No simple case, do the hard work.
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        _mins[i] = std::max(_mins[i], other._mins[i]);
        _maxs[i] = std::min(_maxs[i], other._maxs[i]);
    }

    this->_empty = oct_bb_t::empty_raw(*this);
	return rv_success;
}

//--------------------------------------------------------------------------------------------
void oct_bb_t::translate(const oct_bb_t& src, const Vector3f& t, oct_bb_t& dst) {
    dst = src;
    dst.translate(t);
    dst._empty = oct_bb_t::empty_raw(dst);
}

void oct_bb_t::translate(const oct_bb_t& src, const oct_vec_v2_t& t, oct_bb_t& dst) {
    dst = src;
    dst.translate(oct_vec_v2_t(t));
    dst._empty = oct_bb_t::empty_raw(dst);
}

//--------------------------------------------------------------------------------------------
void oct_bb_t::self_grow(oct_bb_t& self, const oct_vec_v2_t& v)
{
    for (size_t i = 0; i < OCT_COUNT; ++i) 
    {
        self._mins[i] -= std::abs(v[i]);
        self._maxs[i] += std::abs(v[i]);
    }

    self._empty = oct_bb_t::empty_raw(self);
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::contains(const oct_bb_t& self, const oct_vec_v2_t& point)
{
    if (self._empty)
	{
        return false;
    }
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        if (point[i] < self._mins[i]) return false;
        if (point[i] > self._maxs[i]) return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------
bool oct_bb_t::contains(const oct_bb_t& self, const oct_bb_t& other)
{
    // If the right-hand side is empty ...
    if (other._empty)
    {
        // ... it is always contained in the left-hand side.
        return true;
    }
    // If the left-hand side is empty ...
    if (self._empty)
    {
        // ... it can not contain the right-hand side as the right hand-side is non-empty by the above.
        return false;
    }
    // At this point, the left-hand side as well as the right-hand side are non-empty.
    // Perform normal tests.
    for (size_t i = 0; i < OCT_COUNT; ++i)
    {
        if (other._maxs[i] > self._maxs[i]) return false;
        if (other._mins[i] < self._mins[i]) return false;
    }
    return true;
}
