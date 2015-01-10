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

/// @file  egolib/bsp_aabb.h
/// @brief BSP AABBs

#include "egolib/bsp_aabb.h"
#include "egolib/bbox.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool BSP_aabb_overlap_with_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr)
{
	/// @author BB
	/// @details Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
	///               than lhs_ptr, just check the lowest common dimensions.

	size_t cnt, min_dim;

	const float * rhs_mins, *rhs_maxs, *lhs_mins, *lhs_maxs;

	if (NULL == lhs_ptr || !lhs_ptr->valid) return false;
	if (NULL == rhs_ptr || !rhs_ptr->valid) return false;

	min_dim = std::min(rhs_ptr->dim, lhs_ptr->dim);
	if (0 == min_dim) return false;

	// the optomizer is supposed to do this stuff all by itself,
	// but isn't
	rhs_mins = rhs_ptr->mins.ary;
	rhs_maxs = rhs_ptr->maxs.ary;
	lhs_mins = lhs_ptr->mins.ary;
	lhs_maxs = lhs_ptr->maxs.ary;

	for (cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) < (*lhs_mins)) return false;
		if ((*rhs_mins) > (*lhs_maxs)) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_contains_BSP_aabb(const BSP_aabb_t * lhs_ptr, const BSP_aabb_t * rhs_ptr)
{
	/// @author BB
	/// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
	///               than lhs_ptr, just check the lowest common dimensions.

	size_t cnt, min_dim;

	const float * rhs_mins, *rhs_maxs, *lhs_mins, *lhs_maxs;

	if (NULL == lhs_ptr || !lhs_ptr->valid) return false;
	if (NULL == rhs_ptr || !rhs_ptr->valid) return false;

	min_dim = std::min(rhs_ptr->dim, lhs_ptr->dim);
	if (0 == min_dim) return false;

	// the optomizer is supposed to do this stuff all by itself,
	// but isn't
	rhs_mins = rhs_ptr->mins.ary;
	rhs_maxs = rhs_ptr->maxs.ary;
	lhs_mins = lhs_ptr->mins.ary;
	lhs_maxs = lhs_ptr->maxs.ary;

	for (cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) >(*lhs_maxs)) return false;
		if ((*rhs_mins) < (*lhs_mins)) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool BSP_aabb_overlap_with_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr)
{
	/// @author BB
	/// @details Do lhs_ptr and rhs_ptr overlap? If rhs_ptr has less dimensions
	///               than lhs_ptr, just check the lowest common dimensions.

	size_t cnt, min_dim;
#if 0
	const float * rhs_mins, *rhs_maxs, *lhs_mins, *lhs_maxs;
#endif
	if (NULL == lhs_ptr || !lhs_ptr->valid) return false;
	if (NULL == rhs_ptr /* || !rhs_ptr->valid */) return false;

	min_dim = std::min((size_t)3 /* rhs_ptr->dim */, lhs_ptr->dim);
	if (0 == min_dim) return false;

	// the optomizer is supposed to do this stuff all by itself,
	// but isn't
	const float *rhs_mins = rhs_ptr->mins.v;
	const float *rhs_maxs = rhs_ptr->maxs.v;
	const float *lhs_mins = lhs_ptr->mins.ary;
	const float *lhs_maxs = lhs_ptr->maxs.ary;

	for (cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) < (*lhs_mins)) return false;
		if ((*rhs_mins) > (*lhs_maxs)) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_contains_aabb(const BSP_aabb_t * lhs_ptr, const aabb_t * rhs_ptr)
{
	/// @author BB
	/// @details Is rhs_ptr contained within lhs_ptr? If rhs_ptr has less dimensions
	///               than lhs_ptr, just check the lowest common dimensions.

	size_t cnt, min_dim;
#if 0
	const float * rhs_mins, *rhs_maxs, *lhs_mins, *lhs_maxs;
#endif
	if (NULL == lhs_ptr || !lhs_ptr->valid) return false;
	if (NULL == rhs_ptr /* || !rhs_ptr->valid */) return false;

	min_dim = std::min((size_t)3 /* rhs_ptr->dim */, lhs_ptr->dim);
	if (0 == min_dim) return false;

	// the optimizer is supposed to do this stuff all by itself,
	// but isn't
	const float *rhs_mins = rhs_ptr->mins.v;
	const float *rhs_maxs = rhs_ptr->maxs.v;
	const float *lhs_mins = lhs_ptr->mins.ary;
	const float *lhs_maxs = lhs_ptr->maxs.ary;

	for (cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) >(*lhs_maxs)) return false;
		if ((*rhs_mins) < (*lhs_mins)) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_empty(const BSP_aabb_t *psrc)
{
	Uint32 cnt;

	if (NULL == psrc || 0 == psrc->dim || !psrc->valid) return true;

	for (cnt = 0; cnt < psrc->dim; cnt++)
	{
		if (psrc->maxs.ary[cnt] <= psrc->mins.ary[cnt])
			return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_invalidate(BSP_aabb_t * psrc)
{
	if (NULL == psrc) return false;

	// set it to valid
	psrc->valid = false;

	return true;
}

//--------------------------------------------------------------------------------------------
/**
* @brief
*	Set a BSP AABB to an empty state.
* @param self
*	the BSP AABB
*/
bool BSP_aabb_self_clear(BSP_aabb_t * psrc)
{
	if (NULL == psrc) return false;

	if (NULL == psrc->mins.ary || NULL == psrc->mids.ary || NULL == psrc->maxs.ary)
	{
		BSP_aabb_invalidate(psrc);
		return false;
	}

	for (size_t cnt = 0; cnt < psrc->dim; cnt++)
	{
		psrc->mins.ary[cnt] = psrc->mids.ary[cnt] = psrc->maxs.ary[cnt] = 0.0f;
	}

	return true;
}

bool BSP_aabb_ctor(BSP_aabb_t& self, size_t dim)
{
	// Allocate everything.
	return BSP_aabb_alloc(self, dim);
}

//--------------------------------------------------------------------------------------------
void BSP_aabb_dtor(BSP_aabb_t& self)
{
	// Deallocate everything.
	return BSP_aabb_dealloc(self);
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_alloc(BSP_aabb_t& self, size_t dim)
{
	if (!float_ary_ctor(&(self.mins), dim))
	{
		return false;
	}
	if (!float_ary_ctor(&(self.mids), dim))
	{
		float_ary_dtor(&(self.mins));
		return false;
	}
	if (!float_ary_ctor(&(self.maxs), dim))
	{
		float_ary_dtor(&(self.mids));
		float_ary_dtor(&(self.mins));
		return false;
	}

	EGOBOO_ASSERT(dim == self.mins.alloc && dim == self.mids.alloc && dim == self.maxs.alloc);

	self.dim = dim;

	// Set this BSP AABB to the empy state.
	for (size_t i = 0; i < self.dim; i++)
	{
		self.mins.ary[i] = self.mids.ary[i] = self.maxs.ary[i] = 0.0f;
	}

	// Set this BSP AABB to valid.
	self.valid = true;

	return true;
}

//--------------------------------------------------------------------------------------------
void BSP_aabb_dealloc(BSP_aabb_t& self)
{
	// Deallocate everything.
	float_ary_dtor(&(self.mins));
	float_ary_dtor(&(self.mids));
	float_ary_dtor(&(self.maxs));

	self.dim = 0;
	self.valid = false;
}

//--------------------------------------------------------------------------------------------
/**
* @brief
*	Convert an oct_bb_t to a BSP_aabb_t.
* @author
*	BB
*/
bool BSP_aabb_from_oct_bb(BSP_aabb_t *pdst, const oct_bb_t *psrc)
{
	Uint32 cnt;

	if (NULL == pdst || NULL == psrc) return false;

	BSP_aabb_invalidate(pdst);

	// This process is a little bit complicated because the
	// order to the OCT_* indices is optimized for a different test.
	if (1 == pdst->dim)
	{
		pdst->mins.ary[kX] = psrc->mins[OCT_X];

		pdst->maxs.ary[kX] = psrc->maxs[OCT_X];
	}
	else if (2 == pdst->dim)
	{
		pdst->mins.ary[kX] = psrc->mins[OCT_X];
		pdst->mins.ary[kY] = psrc->mins[OCT_Y];

		pdst->maxs.ary[kX] = psrc->maxs[OCT_X];
		pdst->maxs.ary[kY] = psrc->maxs[OCT_Y];
	}
	else if (pdst->dim >= 3)
	{
		pdst->mins.ary[kX] = psrc->mins[OCT_X];
		pdst->mins.ary[kY] = psrc->mins[OCT_Y];
		pdst->mins.ary[kZ] = psrc->mins[OCT_Z];

		pdst->maxs.ary[kX] = psrc->maxs[OCT_X];
		pdst->maxs.ary[kY] = psrc->maxs[OCT_Y];
		pdst->maxs.ary[kZ] = psrc->maxs[OCT_Z];

		// Blank any extended dimensions.
		for (cnt = 3; cnt < pdst->dim; cnt++)
		{
			pdst->mins.ary[cnt] = pdst->maxs.ary[cnt] = 0.0f;
		}
	}

	// Find the mid values.
	for (cnt = 0; cnt < pdst->dim; cnt++)
	{
		pdst->mids.ary[cnt] = 0.5f * (pdst->mins.ary[cnt] + pdst->maxs.ary[cnt]);
	}

	BSP_aabb_validate(pdst);

	return true;
}

bool BSP_aabb_validate(BSP_aabb_t *psrc)
{
	if (NULL == psrc)
	{
		return false;
	}

	// Set it to valid.
	psrc->valid = true;

	// Check to see if any dimension is inverted.
	for (size_t cnt = 0; cnt < psrc->dim; cnt++)
	{
		if (psrc->maxs.ary[cnt] < psrc->mids.ary[cnt])
		{
			psrc->valid = false;
			break;
		}
		if (psrc->maxs.ary[cnt] < psrc->mins.ary[cnt])
		{
			psrc->valid = false;
			break;
		}
		if (psrc->mids.ary[cnt] < psrc->mins.ary[cnt])
		{
			psrc->valid = false;
			break;
		}
	}

	return psrc->valid;
}

bool BSP_aabb_copy(BSP_aabb_t *pdst, const BSP_aabb_t *psrc)
{
	if (NULL == pdst)
	{
		return false;
	}
	
	if (NULL == psrc)
	{
		BSP_aabb_dtor(*pdst);
		return false;
	}

	// Ensure that both BSP AABBs have the same dimensions.
	if (pdst->dim != psrc->dim)
	{
		BSP_aabb_dealloc(*pdst);
		BSP_aabb_alloc(*pdst, psrc->dim);
	}

	for (size_t cnt = 0; cnt < psrc->dim; cnt++)
	{
		pdst->mins.ary[cnt] = psrc->mins.ary[cnt];
		pdst->mids.ary[cnt] = psrc->mids.ary[cnt];
		pdst->maxs.ary[cnt] = psrc->maxs.ary[cnt];
	}

	BSP_aabb_validate(pdst);

	return true;
}

bool BSP_aabb_self_union(BSP_aabb_t *pdst, const BSP_aabb_t *psrc)
{
	if (NULL == pdst)
	{
		return false;
	}

	if (NULL == psrc)
	{
		return BSP_aabb_validate(pdst);
	}

	size_t min_dim = std::min(psrc->dim, pdst->dim);

	for (size_t cnt = 0; cnt < min_dim; cnt++)
	{
		pdst->mins.ary[cnt] = std::min(pdst->mins.ary[cnt], psrc->mins.ary[cnt]);
		pdst->maxs.ary[cnt] = std::max(pdst->maxs.ary[cnt], psrc->maxs.ary[cnt]);

		pdst->mids.ary[cnt] = 0.5f * (pdst->mins.ary[cnt] + pdst->maxs.ary[cnt]);
	}

	return BSP_aabb_validate(pdst);
}