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

/// @file egolib/bsp_leaf.c
/// @brief

#include "egolib/bsp_leaf.h"
#include "egolib/bbox.h"
#include "egolib/bv.h"
#include "egolib/frustum.h"
#include "egolib/log.h"

IMPLEMENT_DYNAMIC_ARY(BSP_leaf_ary, BSP_leaf_t);
IMPLEMENT_DYNAMIC_ARY(BSP_leaf_pary, BSP_leaf_t *);

//--------------------------------------------------------------------------------------------
// BSP_leaf_t
//--------------------------------------------------------------------------------------------

BSP_leaf_t *BSP_leaf_ctor(BSP_leaf_t *self, void *data, bsp_leaf_type_e type, int index)
{
	if (NULL == self) return NULL;

	BLANK_STRUCT_PTR(self)

	if (NULL == data) return NULL;

	self->data_type = type;
	self->data = data;
	self->index = index;

	bv_ctor(self->bbox);

	return self;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t *BSP_leaf_dtor(BSP_leaf_t *self)
{
	if (NULL == self) return self;

	self->inserted = false;
	self->data_type = BSP_LEAF_TYPE_NONE;
	self->data = NULL;

	return self;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_remove_link(BSP_leaf_t * L)
{
	bool retval;

	if (NULL == L) return false;

	retval = false;

	if (L->inserted)
	{
		L->inserted = false;
		retval = true;
	}

	if (NULL != L->next)
	{
		L->next = NULL;
		retval = true;
	}

	bv_self_clear(&(L->bbox));

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_clear(BSP_leaf_t * L)
{
	if (NULL == L) return false;

	L->data_type = BSP_LEAF_TYPE_NONE;
	L->data = NULL;

	BSP_leaf_remove_link(L);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_copy(BSP_leaf_t *dst, const BSP_leaf_t *src)
{
	if (NULL == dst) return false;

	if (NULL == src)
	{
		BLANK_STRUCT_PTR(dst);
	}
	else
	{
		memmove(dst, src, sizeof(*dst));
	}

	return true;
}


bool BSP_leaf_valid(BSP_leaf_t * L)
{
	if (NULL == L) return false;

	if (NULL == L->data) return false;
	if (L->data_type < 0) return false;

	return true;
}


//--------------------------------------------------------------------------------------------
// BSP_leaf_list_t
//--------------------------------------------------------------------------------------------
BSP_leaf_list_t *BSP_leaf_list_ctor(BSP_leaf_list_t * LL)
{
	if (NULL == LL) return LL;

	BLANK_STRUCT_PTR(LL)

		BSP_leaf_list_alloc(LL);

	bv_ctor(LL->bbox);

	return LL;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t * BSP_leaf_list_dtor(BSP_leaf_list_t *  LL)
{
	if (NULL == LL) return LL;

	BSP_leaf_list_dealloc(LL);

	BLANK_STRUCT_PTR(LL)

		return LL;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_alloc(BSP_leaf_list_t * LL)
{
	if (NULL == LL) return false;

	BSP_leaf_list_dealloc(LL);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_dealloc(BSP_leaf_list_t *  LL)
{
	if (NULL == LL) return false;

	return true;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t * BSP_leaf_list_clear(BSP_leaf_list_t * LL)
{
	// DYNAMIC ALLOCATION in LL->bbox, so do not use memset

	if (NULL == LL) return LL;

	bv_self_clear(&(LL->bbox));
	LL->count = 0;
	LL->lst = NULL;

	return LL;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_push_front(BSP_leaf_list_t *self, BSP_leaf_t *leaf)
{
	bool       retval;

	if (NULL == self || NULL == leaf)
	{
		log_error("%s: assertion `%s` failed\n", __FUNCTION__, "NULL != self");
		exit(EXIT_FAILURE);
	}

	if (leaf->inserted)
	{
		log_error("%s: trying to insert a BSP_leaf that is claiming to be part of a list already\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

	retval = false;

	if (EMPTY_BSP_LEAF_LIST(self))
	{
		// prepare the node
		leaf->next = NULL;

		// insert the node
		self->lst = leaf;
		self->count = 1;
		bv_copy(&(self->bbox), &(leaf->bbox));

		leaf->inserted = true;

		retval = true;
	}
	else
	{
		bool found = false;
		size_t cnt;
		BSP_leaf_t *leaf0;
		for (cnt = 0, leaf0 = self->lst;
			NULL != leaf0->next && cnt < self->count && !found;
			cnt++, leaf0 = leaf0->next)
		{
			// do not insert duplicates, or we have a big problem
			if (leaf == leaf0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			EGOBOO_ASSERT(NULL == leaf0->next);

			// prepare the node
			leaf->next = NULL;

			// insert the node at the end of the list
			leaf0->next = leaf;
			self->count = self->count + 1;
			bv_union(self->bbox, leaf->bbox);

			leaf->inserted = true;

			retval = true;
		}
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_reset(BSP_leaf_list_t * LL)
{
	/// @author BB
	/// @details Clear out the leaf list.

	size_t       cnt;
	BSP_leaf_t * ptmp;

	if (NULL == LL) return false;

	if (EMPTY_BSP_LEAF_LIST(LL))
	{
		LL->count = 0;
		LL->lst = NULL;
	}
	else
	{
		// unlink the nodes
		for (cnt = 0; NULL != LL->lst && cnt < LL->count; cnt++)
		{
			// pop a node off the list
			ptmp = LL->lst;
			LL->lst = ptmp->next;

			// clean up the node
			ptmp->inserted = false;
			ptmp->next = NULL;
		};
	}

	// did we have a problem with the LL->count?
	EGOBOO_ASSERT(NULL == LL->lst);

	// clear out the other data
	BSP_leaf_list_clear(LL);

	return true;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t *BSP_leaf_list_pop_front(BSP_leaf_list_t *self)
{
	// handle a bad list.
	/// @todo This should *never* happen.
	///       A debug assertion *must* suffice.
	if (NULL == self)
	{
		log_error("%s: assertion `%s` failed\n", __FUNCTION__, "NULL != self");
		return NULL;
	}

	// heal any bad lists
	/// @todo This should *never* happen.
	///       A debug assertion *must* suffice.
	///       Figure out if the current code
	///       enforces <tt>0 == self->count \f$\Leftrightarrow$ NULL == self->lst</tt>.
	if (NULL == self->lst)
	{
		self->count = 0;
	}
	/// @todo This should *never* happen.
	///       A debug assertion *must* suffice.
	///       Figure out if the code allows the inconsistency 0 == self->count && self->lst != NULL.
	if (0 == self->count)
	{
		self->lst = NULL;
	}

	// Handle an empty list.
	if (0 == self->count)
	{
		return NULL;
	}

	// Pop off the front leaf.
	BSP_leaf_t *leaf = self->lst;
	self->lst = leaf->next;
	self->count--;

	// Unlink it the front leaf.
	leaf->inserted = false;
	leaf->next = NULL;

	return leaf;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_collide_aabb(const BSP_leaf_list_t * LL, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst)
{
	/// @author BB
	/// @details check for collisions with the given node list

	size_t       cnt, lost_nodes;
	BSP_leaf_t * pleaf;
	bool       retval;

	// basic bounds checking
	if (NULL == paabb || DYNAMIC_ARY_INVALID(colst)) return false;

	// if the list is empty, there is nothing to do
	if (EMPTY_BSP_LEAF_LIST(LL)) return true;

	// NOTE: this has already been tested in the parent function
	//// we already have the bounding box of all the leafs
	//if ( !aabb_intersects_aabb( paabb, &(LL->bbox) ) )
	//{
	//    return false;
	//}

	// NOTE: this is already tested by DYNAMIC_ARY_INVALID( colst )
	//// if there is no more room in the colist, return false
	//colst_size = BSP_leaf_pary_get_size( colst );
	//if ( 0 == colst_size || BSP_leaf_pary_get_top( colst ) >= colst_size )
	//{
	//    return false;
	//}

	lost_nodes = 0;

	if (NULL != ptest)
	{
		// scan through every leaf
		for (cnt = 0, pleaf = LL->lst;
			cnt < LL->count && NULL != pleaf;
			cnt++, pleaf = pleaf->next)
		{
			bool do_insert;
			geometry_rv  geometry_test;
			bv_t * pleaf_bb = &(pleaf->bbox);

			// make sure the leaf is valid
			EGOBOO_ASSERT(pleaf->data_type > -1);

			// make sure the leaf is inserted
			if (!pleaf->inserted)
			{
				// hmmm.... what to do?
				log_warning("BSP_leaf_list_collide_aabb() - a node in a leaf list is claiming to not be inserted\n");
			}

			// test the geometry
			geometry_test = aabb_intersects_aabb(paabb, &(pleaf_bb->aabb));

			// determine what action to take
			do_insert = false;
			if (geometry_test > geometry_outside)
			{
				// we have a possible intersection
				do_insert = (*ptest)(pleaf);
			}

			if (do_insert)
			{
				if (!BSP_leaf_pary_push_back(colst, pleaf))
				{
					lost_nodes++;
				}
			}
		}
	}
	else
	{
		// scan through every leaf
		for (cnt = 0, pleaf = LL->lst;
			cnt < LL->count && NULL != pleaf;
			cnt++, pleaf = pleaf->next)
		{
			bool do_insert;
			geometry_rv  geometry_test;
			bv_t * pleaf_bb = &(pleaf->bbox);

			// make sure the leaf is valid
			EGOBOO_ASSERT(pleaf->data_type > -1);

			// make sure the leaf is inserted
			if (!pleaf->inserted)
			{
				// hmmm.... what to do?
				log_warning("BSP_leaf_list_collide_aabb() - a node in a leaf list is claiming to not be inserted\n");
			}

			// test the geometry
			geometry_test = aabb_intersects_aabb(paabb, &(pleaf_bb->aabb));

			// determine what action to take
			do_insert = false;
			if (geometry_test > geometry_outside)
			{
				do_insert = true;
			}

			if (do_insert)
			{
				if (!BSP_leaf_pary_push_back(colst, pleaf))
				{
					lost_nodes++;
				}
			}
		}
	}

	// warn the user if any nodes were rejected
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	// return false if we maxed out the colist
	retval = (BSP_leaf_pary_get_top(colst) < BSP_leaf_pary_get_size(colst));

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_collide_frustum(const BSP_leaf_list_t * LL, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst)
{
	/// @author BB
	/// @details check for collisions with the given node list

	size_t       cnt, colst_size, lost_nodes;
	BSP_leaf_t * pleaf;
	bool       retval;

	// basic bounds checking
	if (NULL == pfrust || DYNAMIC_ARY_INVALID(colst)) return false;

	// if the list is empty, there is nothing to do
	if (EMPTY_BSP_LEAF_LIST(LL)) return true;

	// we already have the bounding box of all the leafs
	if (!egolib_frustum_intersects_bv(pfrust, &(LL->bbox)))
	{
		return false;
	}

	// if there is no more room in the colist, return false
	colst_size = BSP_leaf_pary_get_size(colst);
	if (0 == colst_size || BSP_leaf_pary_get_top(colst) >= colst_size)
	{
		return false;
	}

	// assume the best
	lost_nodes = 0;

	// scan through every leaf
	for (cnt = 0, pleaf = LL->lst;
		cnt < LL->count && NULL != pleaf;
		cnt++, pleaf = pleaf->next)
	{
		bool do_insert;
		geometry_rv  geometry_test;
		bv_t * pleaf_bb = &(pleaf->bbox);

		// make sure the leaf is valid
		EGOBOO_ASSERT(pleaf->data_type > -1);

		// make sure the leaf is inserted
		if (!pleaf->inserted)
		{
			// hmmm.... what to do?
			log_warning("BSP_leaf_list_collide_aabb() - a node in a leaf list is claiming to not be inserted\n");
		}

		// test the geometry
		geometry_test = egolib_frustum_intersects_bv(pfrust, pleaf_bb);

		// determine what action to take
		do_insert = false;
		if (geometry_test > geometry_outside)
		{
			if (NULL == ptest)
			{
				do_insert = true;
			}
			else
			{
				// we have a possible intersection
				do_insert = (*ptest)(pleaf);
			}
		}

		if (do_insert)
		{
			if (!BSP_leaf_pary_push_back(colst, pleaf))
			{
				lost_nodes++;
			}
		}
	}

	// warn the user if any nodes were rejected
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	// return false if we maxed out the colist
	retval = (BSP_leaf_pary_get_top(colst) < BSP_leaf_pary_get_size(colst));

	return retval;
}
