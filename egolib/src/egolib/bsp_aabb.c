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

BSP_aabb_t *BSP_aabb_t::ctor(size_t dim)
{
	// Construct the vectors.
	if (!mins.ctor(dim))
	{
		return NULL;
	}
	if (!mids.ctor(dim))
	{
		mins.dtor();
		return NULL;
	}
	if (!maxs.ctor(dim))
	{
		mids.dtor();
		mins.dtor();
		return NULL;
	}
	// Set the dimensionality.
	dim = dim;
	// Center the BSP AABB around the origin and set its size along all axes to zero.
	for (size_t index = 0; index < dim; ++index)
	{
		mins.ary[index] = mids.ary[index] = maxs.ary[index] = 0.0f;
	}
	// Mark this BSP AABB as valid.
	valid = true;

	return this;
}

//--------------------------------------------------------------------------------------------
void BSP_aabb_t::dtor()
{
	// Deallocate the vectors.
	mins.dtor();
	mids.dtor();
	maxs.dtor();
	// Set the dimenionality to zero.
	dim = 0;
	// Mark this BSP AABB as invalid.
	valid = false;
}

//--------------------------------------------------------------------------------------------
BSP_aabb_t *BSP_aabb_alloc(BSP_aabb_t *self, size_t dim)
{
	if (NULL == self)
	{
		return self;
	}
	// Set the dimensionality to @a 0.
	self->dim = 0;

	// Allocate the vectors.
	if (!self->mins.ctor(dim))
	{
		return NULL;
	}
	if (!self->mids.ctor(dim))
	{
		self->mins.dtor();
		return NULL;
	}
	if (!self->maxs.ctor(dim))
	{
		self->mids.dtor();
		self->mins.dtor();
		return NULL;
	}
	self->dim = dim;
	BSP_aabb_t::set_empty(self);
	BSP_aabb_validate(*self);
	return self;
}

//--------------------------------------------------------------------------------------------
BSP_aabb_t *BSP_aabb_dealloc(BSP_aabb_t *self)
{
	if (NULL == NULL)
	{
		return self;
	}
	// Deallocate the vectors.
	self->mins.dtor();
	self->mids.dtor();
	self->maxs.dtor();
	// Set the dimenionality to zero.
	self->dim = 0;
	// Mark this BSP AABB as invalid.
	self->valid = false;
	return self;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_from_oct_bb(BSP_aabb_t * pdst, const oct_bb_t * psrc)
{
	/// @author BB
	/// @details do an automatic conversion from an oct_bb_t to a BSP_aabb_t

	Uint32 cnt;

	if (NULL == pdst || NULL == psrc) return false;

	BSP_aabb_invalidate(*pdst);

	if (pdst->dim <= 0) return false;

	// this process is a little bit complicated because the
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

		// blank any extended dimensions
		for (cnt = 3; cnt < pdst->dim; cnt++)
		{
			pdst->mins.ary[cnt] = pdst->maxs.ary[cnt] = 0.0f;
		}
	}

	// find the mid values
	for (cnt = 0; cnt < pdst->dim; cnt++)
	{
		pdst->mids.ary[cnt] = 0.5f * (pdst->mins.ary[cnt] + pdst->maxs.ary[cnt]);
	}

	BSP_aabb_validate(*pdst);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_validate(BSP_aabb_t& src)
{
	// set it to valid
	src.valid = true;

	// check to see if any dimension is inverted
	for (size_t cnt = 0; cnt < src.dim; cnt++)
	{
		if (src.maxs.ary[cnt] < src.mids.ary[cnt])
		{
			src.valid = false;
			break;
		}
		if (src.maxs.ary[cnt] < src.mins.ary[cnt])
		{
			src.valid = false;
			break;
		}
		if (src.mids.ary[cnt] < src.mins.ary[cnt])
		{
			src.valid = false;
			break;
		}
	}

	return src.valid;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_copy(BSP_aabb_t * pdst, const BSP_aabb_t * psrc)
{
	size_t cnt;

	if (NULL == pdst) return false;

	if (NULL == psrc)
	{
		pdst->dtor();
		return false;
	}

	// ensure that they have the same dimensions
	if (pdst->dim != psrc->dim)
	{
		BSP_aabb_dealloc(pdst);
		BSP_aabb_alloc(pdst, psrc->dim);
	}

	for (cnt = 0; cnt < psrc->dim; cnt++)
	{
		pdst->mins.ary[cnt] = psrc->mins.ary[cnt];
		pdst->mids.ary[cnt] = psrc->mids.ary[cnt];
		pdst->maxs.ary[cnt] = psrc->maxs.ary[cnt];
	}

	BSP_aabb_validate(*pdst);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_self_union(BSP_aabb_t& dst, const BSP_aabb_t& src)
{
	size_t min_dim = std::min(src.dim, dst.dim);
	for (size_t cnt = 0; cnt < min_dim; cnt++)
	{
		dst.mins.ary[cnt] = std::min(dst.mins.ary[cnt], src.mins.ary[cnt]);
		dst.maxs.ary[cnt] = std::max(dst.maxs.ary[cnt], src.maxs.ary[cnt]);
		dst.mids.ary[cnt] = 0.5f * (dst.mins.ary[cnt] + dst.maxs.ary[cnt]);
	}
	return BSP_aabb_validate(dst);
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_t::is_empty(const BSP_aabb_t * psrc)
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
bool BSP_aabb_invalidate(BSP_aabb_t& src)
{
	// set it to valid
	src.valid = false;
	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_t::set_empty(BSP_aabb_t *self)
{
	if (NULL == self) return false;

	if (self->dim <= 0 || NULL == self->mins.ary || NULL == self->mids.ary || NULL == self->maxs.ary)
	{
		BSP_aabb_invalidate(*self);
		return false;
	}

	for (size_t index = 0; index < self->dim; ++index)
	{
		self->mins.ary[index] = self->mids.ary[index] = self->maxs.ary[index] = 0.0f;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool BSP_aabb_t::overlap_with_BSP_aabb(const BSP_aabb_t& other) const
{
	if (!this->valid || !other.valid)
	{
		return false;
	}
	size_t min_dim = std::min(this->dim, other.dim);
	if (0 == min_dim)
	{
		return false;
	}
	const float *other_mins = other.mins.ary;
	const float *other_maxs = other.maxs.ary;
	const float *self_mins  = this->mins.ary;
	const float *self_maxs  = this->maxs.ary;
	for (size_t cnt = 0; cnt < min_dim; cnt++, self_mins++, self_maxs++, other_mins++, other_maxs++)
	{
		if ((*other_maxs) < (*self_mins)) return false;
		if ((*other_mins) > (*self_maxs)) return false;
	}
	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_t::contains_BSP_aabb(const BSP_aabb_t& rhs_ptr) const
{
	if (!this->valid || !rhs_ptr.valid) return false;

	size_t min_dim = std::min(this->dim, rhs_ptr.dim);
	if (0 == min_dim) return false;

	// The optimizer is supposed to do this stuff all by itself, but isn't.
	const float *rhs_mins = rhs_ptr.mins.ary;
	const float *rhs_maxs = rhs_ptr.maxs.ary;
	const float *lhs_mins = this->mins.ary;
	const float *lhs_maxs = this->maxs.ary;

	for (size_t cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) >(*lhs_maxs)) return false;
		if ((*rhs_mins) < (*lhs_mins)) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool BSP_aabb_t::overlap_with_aabb(const aabb_t& rhs_ptr) const
{
	if (!this->valid) return false;

	size_t min_dim = std::min(this->dim,(size_t)3);
	if (0 == min_dim) return false;

	// The optimizer is supposed to do this stuff all by itself, but isn't.
	const float *rhs_mins = rhs_ptr.mins.v + 0;
	const float *rhs_maxs = rhs_ptr.maxs.v + 0;
	const float *lhs_mins = this->mins.ary;
	const float *lhs_maxs = this->maxs.ary;

	for (size_t cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) < (*lhs_mins)) return false;
		if ((*rhs_mins) > (*lhs_maxs)) return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_aabb_t::contains_aabb(const aabb_t& rhs_ptr) const
{
	if (!this->valid) return false;

	size_t min_dim = std::min(this->dim,(size_t)3);
	if (0 == min_dim) return false;

	// The optimizer is supposed to do this stuff all by itself, but isn't.
	const float *rhs_mins = rhs_ptr.mins.v + 0;
	const float *rhs_maxs = rhs_ptr.maxs.v + 0;
	const float *lhs_mins = this->mins.ary;
	const float *lhs_maxs = this->maxs.ary;

	for (size_t cnt = 0; cnt < min_dim; cnt++, rhs_mins++, rhs_maxs++, lhs_mins++, lhs_maxs++)
	{
		if ((*rhs_maxs) >(*lhs_maxs)) return false;
		if ((*rhs_mins) < (*lhs_mins)) return false;
	}

	return true;
}