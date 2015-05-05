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

/// @file egolib/bv.c
/// @brief Convex bounding volumes consiting of spheres enclosing boxes.
#include "egolib/bv.h"

#include "egolib/bbox.h"

//--------------------------------------------------------------------------------------------
bool bv_self_clear(bv_t *self)
{
	EGOBOO_ASSERT(NULL != self);
	if (NULL == self) return false;

	aabb_self_clear(&(self->aabb));

	self->sphere.radius = 0.0f;
	self->sphere.origin = fvec3_t::zero();

	return true;
}

//--------------------------------------------------------------------------------------------
void bv_t::from(const oct_bb_t& other)
{
    if (!other.empty)
    {
        aabb.from(other);
        bv_validate(this);
    }
    else
    {
        bv_self_clear(this);
    }
}

//--------------------------------------------------------------------------------------------
bool bv_is_clear(const bv_t * pdst)
{
	bool retval;

	if (NULL == pdst) return true;

	retval = aabb_is_clear(&(pdst->aabb));

	if (!retval)
	{
		retval = (pdst->sphere.radius <= 0.0f);
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool bv_self_union(bv_t * pdst, const bv_t * psrc)
{
	if (NULL == pdst) return false;
	if (NULL == psrc) return true;
	if (pdst->sphere.radius < 0.0f)
	{
		bv_validate(pdst);
	}
	if (psrc->sphere.radius < 0.0f)
	{
		return true;
	}
	pdst->aabb.join(psrc->aabb);
	return true;
}

//--------------------------------------------------------------------------------------------
bool bv_t::contains(const bv_t *x, const bv_t *y)
{
	if (!x || !y) return false;
	return aabb_t::contains(x->aabb,y->aabb);
}

//--------------------------------------------------------------------------------------------
bool bv_t::overlaps(const bv_t *x, const bv_t *y)
{
	if (!x || !y) return false;
	return aabb_t::overlaps(x->aabb,y->aabb);
}

//--------------------------------------------------------------------------------------------
bool bv_validate(bv_t * rhs)
{
	int cnt;
	float radius_2;
	fvec3_t vtmp;

	if (NULL == rhs) return false;

	for (cnt = 0; cnt < 3; cnt++)
	{
		rhs->sphere.origin.v[cnt] = 0.5f * (rhs->aabb.mins[cnt] + rhs->aabb.maxs[cnt]);
	}

	radius_2 = (rhs->aabb.mins - rhs->sphere.origin).length_2();
	if (0.0f == radius_2)
	{
		rhs->sphere.radius = 0.0f;
	}
	else
	{
		rhs->sphere.radius = std::sqrt(radius_2);
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool bv_test(const bv_t * rhs)
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