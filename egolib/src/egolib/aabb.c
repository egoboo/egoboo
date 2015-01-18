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
/// @file egolib/aabb.c
/// @brief axis-aligned bounding boxes
#include "egolib/aabb.h"
#include "egolib/bbox.h"

//--------------------------------------------------------------------------------------------
aabb_t *aabb_ctor(aabb_t& self)
{
	for (size_t i = 0; i < 3; ++i)
	{
		self.mins[i] = self.maxs[i] = 0.0f;
	}
	return &self;
}

//--------------------------------------------------------------------------------------------
aabb_t *aabb_dtor(aabb_t& self)
{
	for (size_t i = 0; i < 3; ++i)
	{
		self.mins[i] = self.maxs[i] = 0.0f;
	}
	return &self;
}

//--------------------------------------------------------------------------------------------

bool aabb_copy(aabb_t& dst, const aabb_t& src)
{
	for (size_t cnt = 0; cnt < 3; cnt++)
	{
		dst.mins[cnt] = src.mins[cnt];
		dst.maxs[cnt] = src.maxs[cnt];
	}

	return true;
}

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

//--------------------------------------------------------------------------------------------
bool aabb_from_oct_bb(aabb_t * dst, const oct_bb_t * src)
{
	if (NULL == dst) return false;

	if (NULL == src)
	{
		BLANK_STRUCT_PTR(dst);
	}
	else
	{
		// the indices do not match up, so be careful
		dst->mins[kX] = src->mins[OCT_X];
		dst->mins[kY] = src->mins[OCT_Y];
		dst->mins[kZ] = src->mins[OCT_Z];

		dst->maxs[kX] = src->maxs[OCT_X];
		dst->maxs[kY] = src->maxs[OCT_Y];
		dst->maxs[kZ] = src->maxs[OCT_Z];
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool aabb_lhs_contains_rhs(const aabb_t& self, const aabb_t& other)
{
	// The optimizer is supposed to do this stuff all by itself, but isn't.
	const float *rhs_mins = other.mins + 0;
	const float *rhs_maxs = other.maxs + 0;
	const float *lhs_mins = self.mins + 0;
	const float *lhs_maxs = self.maxs + 0;

	for (size_t cnt = 0; cnt < 3; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) >(*lhs_maxs)) return false;
		if ((*rhs_mins) < (*lhs_mins)) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
void aabb_self_union(aabb_t& self, const aabb_t& other)
{
	for (size_t cnt = 0; cnt < 3; cnt++)
	{
		self.mins[cnt] = std::min(self.mins[cnt], other.mins[cnt]);
		self.maxs[cnt] = std::max(self.maxs[cnt], other.maxs[cnt]);
	}
}

//--------------------------------------------------------------------------------------------
bool aabb_overlap(const aabb_t& self, const aabb_t& other)
{
	// The optimizer is supposed to do this stuff all by itself, but isn't.
	const float *rhs_mins = other.mins + 0;
	const float *rhs_maxs = other.maxs + 0;
	const float *lhs_mins = self.mins + 0;
	const float *lhs_maxs = self.maxs + 0;

	for (size_t cnt = 0; cnt < 3; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) < (*lhs_mins)) return false;
		if ((*rhs_mins) > (*lhs_maxs)) return false;
	}

	return true;
}