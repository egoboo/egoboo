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
ego_aabb_t *ego_aabb_ctor(ego_aabb_t *self)
{
	EGOBOO_ASSERT(NULL != self);
	if (NULL == self) return self;

	ego_aabb_self_clear(self);

	return self;
}

//--------------------------------------------------------------------------------------------
ego_aabb_t *ego_aabb_dtor(ego_aabb_t *self)
{
	EGOBOO_ASSERT(NULL != self);
	if (NULL == self) return self;

	ego_aabb_self_clear(self);

	return self;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_self_clear(ego_aabb_t *self)
{
	EGOBOO_ASSERT(NULL != self);
	if (NULL == self) return false;

	aabb_self_clear(&(self->data));

	sphere_self_clear(&(self->sphere));

	return true;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_copy(ego_aabb_t * pdst, const ego_aabb_t * psrc)
{
	bool retval = false;

	if (NULL == pdst)
	{
		retval = false;
	}
	else if (NULL == psrc)
	{
		retval = ego_aabb_self_clear(pdst);
	}
	else
	{
		memmove(pdst, psrc, sizeof(*pdst));
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_from_oct_bb(ego_aabb_t * dst, const oct_bb_t * src)
{
	bool retval;

	if (NULL == dst) return false;

	retval = aabb_from_oct_bb(&(dst->data), src);

	if (retval)
	{
		retval = ego_aabb_validate(dst);
	}
	else
	{
		ego_aabb_self_clear(dst);
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_is_clear(const ego_aabb_t * pdst)
{
	bool retval;

	if (NULL == pdst) return true;

	retval = aabb_is_clear(&(pdst->data));

	if (!retval)
	{
		retval = (pdst->sphere.radius <= 0.0f);
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_self_union(ego_aabb_t * pdst, const ego_aabb_t * psrc)
{
	bool retval = false;

	if (NULL == pdst) return false;
	if (NULL == psrc) return true;

	if (pdst->sphere.radius < 0.0f)
	{
		ego_aabb_validate(pdst);
	}

	if (psrc->sphere.radius < 0.0f)
	{
		return true;
	}

	retval = aabb_self_union(&(pdst->data), &(psrc->data));
	if (retval)
	{
		retval = ego_aabb_validate(pdst);
	}
	else
	{
		sphere_self_clear(&(pdst->sphere));
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_lhs_contains_rhs(const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr)
{
	if (NULL == lhs_ptr || NULL == rhs_ptr) return false;

	ego_aabb_test(lhs_ptr);
	ego_aabb_test(rhs_ptr);

	return aabb_lhs_contains_rhs(&(lhs_ptr->data), &(rhs_ptr->data));
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_overlap(const ego_aabb_t * lhs_ptr, const ego_aabb_t * rhs_ptr)
{
	if (NULL == lhs_ptr || NULL == rhs_ptr) return false;

	ego_aabb_test(lhs_ptr);
	ego_aabb_test(rhs_ptr);

	return aabb_overlap(&(lhs_ptr->data), &(rhs_ptr->data));
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_validate(ego_aabb_t * rhs)
{
	int cnt;
	float radius_2;
	fvec3_t vtmp;

	if (NULL == rhs) return false;

	for (cnt = 0; cnt < 3; cnt++)
	{
		rhs->sphere.origin.v[cnt] = 0.5f * (rhs->data.mins[cnt] + rhs->data.maxs[cnt]);
	}

	fvec3_sub(vtmp.v, rhs->data.mins, rhs->sphere.origin.v);
	radius_2 = fvec3_length_2(vtmp.v);
	if (0.0f == radius_2)
	{
		rhs->sphere.radius = 0.0f;
	}
	else
	{
		rhs->sphere.radius = SQRT(radius_2);
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool ego_aabb_test(const ego_aabb_t * rhs)
{
	bool retval;

	if (NULL == rhs) return false;

	if (rhs->sphere.radius < 0.0f)
	{
		retval = false;
	}
	else
	{
		retval = true;
	}

	return retval;
}