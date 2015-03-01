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

/// @file egolib/math/AABB.c
/// @brief Axis-aligned bounding boxes.
#include "egolib/math/AABB.h"
#include "egolib/bbox.h"

//--------------------------------------------------------------------------------------------
bool aabb_self_clear(aabb_t * psrc)
{
	/// @author BB
	/// @details Return this bounding box to an empty state.
	if (NULL == psrc) return false;

	for (size_t cnt = 0; cnt < 3; cnt++)
	{
		psrc->mins[cnt] = psrc->maxs[cnt] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool aabb_is_clear(const aabb_t * pdst)
{
	int cnt;
	bool retval;

	if (NULL == pdst) return true;

	// assume the best
	retval = true;

	// scan through the values
	for (cnt = 0; cnt < 3; cnt++)
	{
		if (0.0f != pdst->mins[cnt])
		{
			retval = false;
			break;
		}

		if (0.0f != pdst->maxs[cnt])
		{
			retval = false;
			break;
		}
	}

	return retval;
}

void aabb_t::from(const oct_bb_t& other)
{
    // The indices do not match up, so be careful.
	mins[kX] = other.mins[OCT_X];
	mins[kY] = other.mins[OCT_Y];
	mins[kZ] = other.mins[OCT_Z];

	maxs[kX] = other.maxs[OCT_X];
	maxs[kY] = other.maxs[OCT_Y];
	maxs[kZ] = other.maxs[OCT_Z];
}

void aabb_t::join(const aabb_t& other)
{
    for (size_t i = 0; i < 3; ++i)
    {
        mins[i] = std::min(mins[i], other.mins[i]);
        maxs[i] = std::max(maxs[i], other.maxs[i]);
    }
}

bool aabb_t::contains(const aabb_t& x, const aabb_t& y)
{
	for (size_t i = 0; i < 3; ++i)
	{
		if (y.maxs[i] > x.maxs[i]) return false;
		if (y.mins[i] < x.mins[i]) return false;
	}
	return true;
}

bool aabb_t::overlaps(const aabb_t& x, const aabb_t& y)
{
    for (size_t i = 0; i < 3; ++i)
    {
        if (y.maxs[i] < x.mins[i]) return false;
        if (y.mins[i] > x.maxs[i]) return false;
    }
    return true;
}
