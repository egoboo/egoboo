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

/// @file egolib/bsp.c
/// @brief
/// @details

#include "egolib/bsp.h"

#include "egolib/log.h"

#include "egolib/frustum.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define INVALID_BSP_BRANCH_LIST(BL) ( (NULL == (BL)) || (NULL == (BL)->lst) || (0 == (BL)->lst_size) )

#define BRANCH_NODE_THRESHOLD 5

//--------------------------------------------------------------------------------------------
// special functions

static bool _generate_BSP_aabb_child(BSP_aabb_t * psrc, int index, BSP_aabb_t * pdst);
static int _find_child_index(const BSP_aabb_t * pbranch_aabb, const aabb_t * pleaf_aabb);

//--------------------------------------------------------------------------------------------
// Generic BSP functions
//--------------------------------------------------------------------------------------------
bool _generate_BSP_aabb_child(BSP_aabb_t * psrc, int index, BSP_aabb_t * pdst)
{
	signed tnc, child_count;

	// Valid source?
	if (!psrc || psrc->dim <= 0) return false;

	// Valid destination?
	if (!pdst) return false;

	// valid index?
	child_count = 2 << (psrc->dim - 1);
	if (index < 0 || index >= child_count) return false;

	// make sure that the destination type matches the source type
	if (pdst->dim != psrc->dim)
	{
		BSP_aabb_dealloc(pdst);
		BSP_aabb_alloc(pdst,psrc->dim);
	}

	// determine the bounds
	for (size_t cnt = 0; cnt < psrc->dim; cnt++)
	{
		float maxval, minval;

		tnc = psrc->dim - 1 - cnt;

		if (0 == (index & (1 << tnc)))
		{
			minval = psrc->mins.ary[cnt];
			maxval = psrc->mids.ary[cnt];
		}
		else
		{
			minval = psrc->mids.ary[cnt];
			maxval = psrc->maxs.ary[cnt];
		}

		pdst->mins.ary[cnt] = minval;
		pdst->maxs.ary[cnt] = maxval;
		pdst->mids.ary[cnt] = 0.5f * (minval + maxval);
	}

	return true;
}

//--------------------------------------------------------------------------------------------
/**
 * @brief
 *	Determine the child index this leaf needs to go under.
 * @param branchAABB
 *	the axis-aligned n-dimensional bounding box of the branch
 * @param leafAABB
 *	the axis-aligned 3-dimensional bounding box of the leaf
 * @return
 *	- @a -1 if the leaf belongs into this branch i.e. completely fits into this branch's bounding box.
 *	- @a -2 if the leaf is in fact bigger than this branch's bounding box.
 *	- @a n where <tt>0 <= n <= 2^dim-1</tt>: if the leaf belongs into the child branch @a n.
 */
int _find_child_index(const BSP_aabb_t *branchAABB, const aabb_t *leafAABB)
{
	//---- determine which child the leaf needs to go under

	/// @note This function is not optimal, since we encode the comparisons
	/// in the 32-bit integer indices, and then may have to decimate index to construct
	/// the child  branch's bounding by calling _generate_BSP_aabb_child().
	/// The reason that it is done this way is that we would have to be dynamically
	/// allocating and deallocating memory every time this function is called, otherwise. Big waste of time.
	int    index;

	const float *branch_min_ary, *branch_mid_ary, *branch_max_ary;

	// get aliases to the leaf aabb data
	if (NULL == leafAABB) return -2;
	const float *leaf_min_ary = leafAABB->mins.v;
	const float *leaf_max_ary = leafAABB->maxs.v;

	// get aliases to the branch aabb data
	if (NULL == branchAABB || 0 == branchAABB->dim || !branchAABB->valid)
		return -2;

	if (NULL == branchAABB->mins.ary || 0 == branchAABB->mins.cp) return -2;
	branch_min_ary = branchAABB->mins.ary;

	if (NULL == branchAABB->mids.ary || 0 == branchAABB->mids.cp) return -2;
	branch_mid_ary = branchAABB->mids.ary;

	if (NULL == branchAABB->maxs.ary || 0 == branchAABB->maxs.cp) return -2;
	branch_max_ary = branchAABB->maxs.ary;

	index = 0;
	size_t d = std::min(branchAABB->dim, (size_t)3);
	for (size_t i = 0; i < d; ++i)
	{
		index <<= 1;

		// Is the leaf within the left side of the AABB w.r.t. the dimension i? If so, do nothing.
		    if (leaf_min_ary[i] >= branch_min_ary[i] && leaf_max_ary[i] <= branch_mid_ary[i])
		{
			// the following code does nothing
			index |= 0;
		}
		// Is the leaf within the right side of the AABB w.r.t. the dimension i? If so, set one bit.
		else if (leaf_min_ary[i] >= branch_mid_ary[i] && leaf_max_ary[i] <= branch_max_ary[i])
		{
			index |= 1;
		}
		else if (leaf_min_ary[i] >= branch_min_ary[i] && leaf_max_ary[i] <= branch_max_ary[i])
		{
			// This leaf belongs at this branch.
			index = -1;
			break;
		}
		else
		{
			// This leaf is actually bigger than this branch.
			index = -2;
			break;
		}
	}

	return index;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t *BSP_leaf_t::ctor(void *data, bsp_type_t type, size_t index)
{
	this->next = nullptr;
	this->inserted = false;
	this->data_type = type;
	this->index = index;
	this->data = data;
	this->bbox.ctor();
	return this;
}

//--------------------------------------------------------------------------------------------
void BSP_leaf_t::dtor()
{
	this->bbox.dtor();
	this->data = nullptr;
	this->index = 0;
	this->data_type = BSP_LEAF_NONE;
	this->inserted = false;
	this->next = nullptr;
}
//--------------------------------------------------------------------------------------------
bool BSP_leaf_t::remove_link(BSP_leaf_t *L)
{
	if (NULL == L) return false;

	bool retval = false;

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
bool BSP_leaf_t::clear(BSP_leaf_t * L)
{
	if (NULL == L) return false;

	L->data_type = BSP_LEAF_NONE;
	L->data = NULL;

	BSP_leaf_t::remove_link(L);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_t::assign(const BSP_leaf_t& other)
{
	this->next = other.next;
	this->data_type = other.data_type;
	this->data = other.data;
	this->index = other.index;
	this->bbox.assign(other.bbox);
	return true;
}

//--------------------------------------------------------------------------------------------
// BSP_branch_t
//--------------------------------------------------------------------------------------------
#if 0
BSP_branch_t *BSP_branch_t::create(size_t dim)
{
	return new BSP_branch_t(dim);
}
#endif
#if 0
void BSP_branch_t::destroy(BSP_branch_t *self)
{
	delete self;
}
#endif
BSP_branch_t::BSP_branch_t(size_t dim) :
	parent(nullptr),
	unsorted(),
	children(dim),
	leaves(),
	bsp_bbox(dim),
	depth(0),
	Shell()
{
}

//--------------------------------------------------------------------------------------------
BSP_branch_t::~BSP_branch_t()
{
	// Set actual depth to 0 and parent to nullptr.
	depth = 0;
	parent = nullptr;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::unlink_all()
{
	bool retval = true;

	if (!unlink_parent())   retval = false;
	if (!unlink_children()) retval = false;
	if (!unlink_leaves())   retval = false;

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::unlink_parent()
{
	if (!parent) return true;
	BSP_branch_list_t *children = &(parent->children);

	// Unlink this branch from parent.
	parent = nullptr;

	bool found_self = false;
	// Unlink parent from this branch.
	if (!INVALID_BSP_BRANCH_LIST(children))
	{
		for (size_t i = 0; i < children->lst_size; i++)
		{
			if (this == children->lst[i])
			{
				children->lst[i] = nullptr;
				found_self = true;
			}
		}
	}

	// Re-count the parents children.
	// @todo Inefficient and stupid.
	children->inserted = 0;
	for (size_t i = 0; i < children->lst_size; i++)
	{
		if (!children->lst[i]) continue;
		children->inserted++;
	}

	return found_self;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::unlink_children()
{
	BSP_branch_list_t *children_ptr = &(children);

	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		for (size_t i = 0; i < children_ptr->lst_size; i++)
		{
			if (nullptr != children_ptr->lst[i])
			{
				children_ptr->lst[i]->parent = nullptr;
			}

			children_ptr->lst[i] = nullptr;
		}

	}

	bv_self_clear(&(children_ptr->bbox));

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::unlink_leaves()
{
	return BSP_branch_free_nodes(this, false);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::insert_leaf_list(BSP_leaf_t *leaf)
{
	if (!leaf) return false;
	return leaves.push_front(leaf);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_update_depth_rec(BSP_branch_t * B, size_t depth)
{
	if (NULL == B) return false;
	BSP_branch_list_t *children_ptr = &(B->children);

	B->depth = depth;

	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		for (size_t cnt = 0; cnt < children_ptr->lst_size; cnt++)
		{
			if (NULL == children_ptr->lst[cnt]) continue;

			BSP_branch_update_depth_rec(children_ptr->lst[cnt], depth + 1);
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_insert_branch(BSP_branch_t *B, size_t index, BSP_branch_t * B2)
{
	// B and B2 exist?
	if (NULL == B || NULL == B2) return false;
	BSP_branch_list_t *children_ptr = &(B->children);

	// valid child list for B?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// valid index range?
	if (index >= children_ptr->lst_size) return false;

	// we can't merge two branches at this time
	if (NULL != children_ptr->lst[index])
	{
		return false;
	}

	EGOBOO_ASSERT(B->depth + 1 == B2->depth);

	children_ptr->lst[index] = B2;
	B2->parent = B;

	// update the all bboxes above B2
	for (BSP_branch_t *ptmp = B2->parent; NULL != ptmp; ptmp = ptmp->parent)
	{
		BSP_branch_list_t * tmp_children_ptr = &(ptmp->children);

		// add in the size of B2->children
		if (bv_is_clear(&(tmp_children_ptr->bbox)))
		{
			tmp_children_ptr->bbox = B2->children.bbox;
		}
		else
		{
			bv_self_union(&(tmp_children_ptr->bbox), &(B2->children.bbox));
		}

		// add in the size of B2->nodes
		if (bv_is_clear(&(tmp_children_ptr->bbox)))
		{
			tmp_children_ptr->bbox = B2->leaves.bbox;
		}
		else
		{
			bv_self_union(&(tmp_children_ptr->bbox), &(B2->leaves.bbox));
		}

		// add in the size of B2->unsorted
		if (bv_is_clear(&(tmp_children_ptr->bbox)))
		{
			tmp_children_ptr->bbox = B2->unsorted.bbox;
		}
		else
		{
			bv_self_union(&(tmp_children_ptr->bbox), &(B2->unsorted.bbox));
		}
	}

	// update the depth B2 and all children
	BSP_branch_update_depth_rec(B2, B->depth + 1);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::clear(bool recursive)
{
	if (recursive)
	{
		children.clear_rec();
	}
	else
	{
		// Unlink any child branches.
		unlink_children();
	}

	// Clear the leaves.
	leaves.clear();

	// Clear the unsorted leaves.
	unsorted.clear();

	return true;
}

bool BSP_branch_free_nodes(BSP_branch_t *self, bool recursive)
{
	if (!self) return false;
	BSP_branch_list_t *children = &(self->children);

	if (recursive)
	{
		if (!INVALID_BSP_BRANCH_LIST(children))
		{
			// Recursively clear out any nodes in the child list
			for (size_t i = 0; i < children->lst_size; i++)
			{
				if (NULL == children->lst[i]) continue;

				BSP_branch_free_nodes(children->lst[i], true);
			}
		}
	}

	// Remove all leaves of this branch.
	self->leaves.clear();

	// Remove all unsorted leaves of this branch.
	self->unsorted.clear();

	return true;
}
//--------------------------------------------------------------------------------------------
bool BSP_branch_t::add_all_children(const BSP_branch_t *pbranch, BSP_leaf_test_t *ptest, Ego::DynamicArray<BSP_leaf_t *>  *colst)
{
	size_t cnt, colst_cp;

	const BSP_branch_list_t * children_ptr;

	if (NULL == pbranch) return false;
	children_ptr = &(pbranch->children);

	colst_cp = colst->capacity();

	// add all nodes from all children
	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		for (cnt = 0; cnt < children_ptr->lst_size; cnt++)
		{
			BSP_branch_t * pchild = children_ptr->lst[cnt];
			if (NULL == pchild) continue;

			BSP_branch_t::add_all_rec(pchild, ptest, colst);

			if (colst->size() >= colst_cp) break;
		}

	}

	return (colst->size() < colst_cp);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::add_all_rec(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, Ego::DynamicArray<BSP_leaf_t *>  *colst)
{
	size_t cnt, colst_cp;

	const BSP_branch_list_t * children_ptr;

	if (NULL == pbranch) return false;
	children_ptr = &(pbranch->children);

	colst_cp = colst->capacity();

	if (ptest)
	{
		pbranch->leaves.add_all(*ptest, colst);
		pbranch->unsorted.add_all(*ptest, colst);
	}
	else
	{
		pbranch->leaves.add_all(colst);
		pbranch->unsorted.add_all(colst);
	}

	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		// add all nodes from all children
		for (cnt = 0; cnt < children_ptr->lst_size; cnt++)
		{
			BSP_branch_t *child = children_ptr->lst[cnt];
			if (!child) continue;

			BSP_branch_t::add_all_rec(child, ptest, colst);

			if (colst->size() >= colst_cp) break;
		}

	}

	return (colst->size() < colst_cp);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::isEmpty() const
{
	// Assume the branch is empty.
	bool empty = true;

	// If the branch has leaves, it is not empty.
	if (!leaves.empty())
	{
		empty = false;
		goto BSP_branch_empty_exit;
	}

	// If the branch has unsorted leaves, it is not empty.
	if (!unsorted.empty())
	{
		empty = false;
		goto BSP_branch_empty_exit;
	}
	// If the branch has child branches, then it is not empty.
	if (!INVALID_BSP_BRANCH_LIST(&children))
	{
		for (size_t cnt = 0; cnt < children.lst_size; cnt++)
		{
			if (nullptr != children.lst[cnt])
			{
				empty = false;
				goto BSP_branch_empty_exit;
			}
		}
	}

BSP_branch_empty_exit:

	return empty;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	geometry_rv geom_children, geom_nodes, geom_unsorted;
	bool BSP_retval;

	// if the branch doesn't exist, stop
	const BSP_branch_list_t *children_ptr = &(children);
	const BSP_leaf_list_t *leaves_ptr = &(leaves);
	const BSP_leaf_list_t *unsorted_ptr = &(unsorted);

	// assume the worst
	bool retval = false;

	// check the unsorted nodes
	geom_unsorted = geometry_error;
	if (NULL != unsorted_ptr && NULL != unsorted_ptr->lst)
	{
		if (0 == unsorted_ptr->count)
		{
			geom_unsorted = geometry_outside;
		}
		else if (1 == unsorted_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the aabb-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_unsorted = geometry_intersect;
		}
		else
		{
			geom_unsorted = aabb_intersects_aabb(*aabb, unsorted_ptr->bbox.aabb);
		}
	}

	// check the nodes at this level of the tree
	geom_nodes = geometry_error;
	if (NULL != leaves_ptr && NULL != leaves_ptr->lst)
	{
		if (0 == leaves_ptr->count)
		{
			geom_nodes = geometry_outside;
		}
		else if (1 == leaves_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the aabb-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_nodes = geometry_intersect;
		}
		else
		{
			geom_nodes = aabb_intersects_aabb(*aabb, leaves_ptr->bbox.aabb);
		}
	}

	// check the nodes at all lower levels
	geom_children = geometry_error;
	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		if (0 == children_ptr->inserted)
		{
			geom_children = geometry_outside;
		}
		else if (1 == children_ptr->inserted)
		{
			// A speedup.
			// If there is only one entry, then the aabb-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_children = geometry_intersect;
		}
		else
		{
			geom_children = aabb_intersects_aabb(*aabb, children_ptr->bbox.aabb);
		}
	}

	if (geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside)
	{
		// the branch and the object do not overlap at all.
		// do nothing.
		return false;
	}

	switch (geom_unsorted)
	{
	case geometry_intersect:
		// The aabb and branch partially overlap. Test each item.
		BSP_retval = unsorted_ptr->collide(aabb, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		if (test)
		{
			BSP_retval = unsorted.add_all(*test, collisions);
		}
		else
		{
			BSP_retval = unsorted.add_all(collisions);
		}
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_nodes)
	{
	case geometry_intersect:
		// The aabb and branch partially overlap. Test each item.
		BSP_retval = leaves_ptr->collide(aabb, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single leave.
		if (test)
		{
			BSP_retval = leaves.add_all(*test, collisions);
		}
		else
		{
			BSP_retval = leaves.add_all(collisions);
		}
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_children)
	{
	case geometry_intersect:
		// The aabb and branch partially overlap. Test each item.
		BSP_retval = children.collide(aabb, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		BSP_retval = BSP_branch_t::add_all_children(this, test, collisions);
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	geometry_rv geom_children, geom_nodes, geom_unsorted;
	bool BSP_retval;

	const BSP_branch_list_t *children_ptr = &(children);
	const BSP_leaf_list_t *leaves_ptr = &(leaves);
	const BSP_leaf_list_t *unsorted_ptr = &(unsorted);

	// assume the worst
	bool retval = false;

	// check the unsorted nodes
	geom_unsorted = geometry_error;
	if (NULL != unsorted_ptr && NULL != unsorted_ptr->lst)
	{
		if (0 == unsorted_ptr->count)
		{
			geom_unsorted = geometry_outside;
		}
		else if (1 == unsorted_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the frustum-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_unsorted = geometry_intersect;
		}
		else
		{
			geom_unsorted = frustum->intersects_bv(&(unsorted_ptr->bbox), true);
		}
	}

	// check the nodes at this level of the tree
	geom_nodes = geometry_error;
	if (NULL != leaves_ptr && NULL != leaves_ptr->lst)
	{
		if (0 == leaves_ptr->count)
		{
			geom_nodes = geometry_outside;
		}
		else if (1 == leaves_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the frustum-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_nodes = geometry_intersect;
		}
		else
		{
			geom_nodes = frustum->intersects_bv(&(leaves_ptr->bbox), true);
		}
	}

	// check the nodes at all lower levels
	geom_children = geometry_error;
	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		if (0 == children_ptr->inserted)
		{
			geom_children = geometry_outside;
		}
		else if (1 == children_ptr->inserted)
		{
			// A speedup.
			// If there is only one entry, then the frustum-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_children = geometry_intersect;
		}
		else
		{
			geom_children = frustum->intersects_bv(&(children_ptr->bbox), true);
		}
	}

	if (geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside)
	{
		// the branch and the object do not overlap at all.
		// do nothing.
		return false;
	}

	switch (geom_unsorted)
	{
	case geometry_intersect:
		// The frustum and branch partially overlap. Test each item.
		BSP_retval = unsorted_ptr->collide(frustum, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		if (test)
		{
			BSP_retval = unsorted.add_all(*test, collisions);
		}
		else
		{
			BSP_retval = unsorted.add_all(collisions);
		}
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_nodes)
	{
	case geometry_intersect:
		// The frustum and branch partially overlap. Test each item.
		BSP_retval = leaves_ptr->collide(frustum, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single leave.
		if (test)
		{
			BSP_retval = leaves.add_all(*test, collisions);
		}
		else
		{
			BSP_retval = leaves.add_all(collisions);
		}
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_children)
	{
	case geometry_intersect:
		// The frustum and branch partially overlap. Test each item.
		BSP_retval = children.collide(frustum, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		BSP_retval = BSP_branch_t::add_all_children(this, test, collisions);
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::insert_branch_list_rec(BSP_branch_t *self, BSP_tree_t *tree, BSP_leaf_t *leaf, int index, size_t depth)
{
	// the leaf is a child of this branch
	bool inserted_branch;

	// Validate arguments.
	if (!self || !tree || !leaf) return false;
	
	BSP_branch_list_t *children_ptr = &(self->children);

	// a valid child list?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// Is the index within the correct range?
	if (index < 0 || (size_t)index >= children_ptr->lst_size) return false;

	// Try to get the child.
	BSP_branch_t *child = BSP_branch_t::ensure_branch(self, tree, index);
	if (nullptr != child)
	{
		return false;
	}
	// Try to insert the leaf.
	if (!BSP_branch_t::insert_leaf_rec(child, tree, leaf, depth))
	{
		return false;
	}

	if (0 == children_ptr->inserted)
	{
		children_ptr->bbox = leaf->bbox;
	}
	else
	{
		bv_self_union(&(children_ptr->bbox), &(leaf->bbox));
	}
	children_ptr->inserted++;
	return true;
}

//--------------------------------------------------------------------------------------------
int BSP_branch_t::insert_leaf_rec_1(BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth)
{
	int index, retval;

	bool inserted_leaf, inserted_branch;

	if (!tree || !leaf) return -1;

	// Not inserted anywhere, yet.
	bool inserted_here = false;
	bool inserted_children = false;

	// Don't go too deep.
	if (depth > tree->max_depth)
	{
		// Insert the leaf into the leaves of this branch.
		inserted_here = insert_leaf_list(leaf);
	}
	else
	{
		// Determine which child the leaf needs to go under.
		index = _find_child_index(&(bsp_bbox), &(leaf->bbox.aabb));

		// Insert the leaf in the right place.
		if (index < -1)
		{
			/* do nothing */
		}
		else if (-1 == index)
		{
			// The leaf belongs on this branch.
			inserted_here = insert_leaf_list(leaf);
		}
		else
		{
			// The leaf belongs on child branches of this branch.
			inserted_children = BSP_branch_t::insert_branch_list_rec(this, tree, leaf, index, depth);
		}
	}

	retval = -1;
	if (inserted_here && !inserted_children)
	{
		retval = 0;
	}
	else if (!inserted_here && inserted_children)
	{
		retval = 1;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::insert_leaf_rec(BSP_branch_t *self, BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth)
{
	if (!self || !tree || !leaf) return false;

	BSP_leaf_list_t *unsorted_ptr = &(self->unsorted);
	BSP_leaf_list_t *nodes_ptr = &(self->leaves);

	// nothing has been done with the pleaf pointer yet
	bool handled = false;

	// count the nodes in different categories
	size_t sibling_nodes = nodes_ptr->count;
	size_t unsorted_nodes = unsorted_ptr->count;
	size_t nodes_at_this_level = sibling_nodes + unsorted_nodes;

	// Is list of unsorted nodes overflowing?
	if (unsorted_nodes > 0 && nodes_at_this_level >= BRANCH_NODE_THRESHOLD)
	{
		// Scan through the list and insert everyone in the next lower branch.
		BSP_leaf_t *tmp_leaf;
		int inserted;

		// If you just sort the whole list, you could end up with every single element in
		// the same node at some large depth (if the nodes are strange that way). Since,
		// REDUCING the depth of the NSPT is the entire point, we have to limit this in
		// some way...
		size_t target_unsorted = (sibling_nodes > BRANCH_NODE_THRESHOLD) ? 0 : BRANCH_NODE_THRESHOLD - sibling_nodes;
		size_t min_unsorted = target_unsorted >> 1;

		if (unsorted_ptr->count > min_unsorted)
		{
			tmp_leaf = self->unsorted.pop_front();
			inserted = 0;
			do
			{
				if (self->insert_leaf_rec_1(tree, tmp_leaf, depth + 1) >= 0)
				{
					inserted++;
				}

				// Break out BEFORE popping off another pointer.
				// Otherwise, we lose drop a node by mistake.
				if (unsorted_ptr->count <= min_unsorted) break;

				tmp_leaf = self->unsorted.pop_front();
			} while (NULL != tmp_leaf);

			// now clear out the old bbox.
			bv_self_clear(&(unsorted_ptr->bbox));

			if (unsorted_ptr->count > 0)
			{
				// generate the correct bbox for any remaining nodes
				tmp_leaf = unsorted_ptr->lst;
				size_t cnt = 0;
				if (nullptr != tmp_leaf)
				{
					unsorted_ptr->bbox.aabb = tmp_leaf->bbox.aabb;
					tmp_leaf = tmp_leaf->next;
					cnt++;
				}

				for ( /* nothing */;
					nullptr != tmp_leaf && cnt < unsorted_ptr->count;
					tmp_leaf = tmp_leaf->next, cnt++)
				{
					aabb_self_union(unsorted_ptr->bbox.aabb, tmp_leaf->bbox.aabb);
				}

				bv_validate(&(unsorted_ptr->bbox));
			}
		}
	}

	// Re-calculate the values for this branch.
	sibling_nodes = nodes_ptr->count;
	unsorted_nodes = unsorted_ptr->count;
	nodes_at_this_level = sibling_nodes + unsorted_nodes;

	// Defer sorting any new leaves, if possible.
	if (!handled && nodes_at_this_level < BRANCH_NODE_THRESHOLD)
	{
		handled = self->unsorted.push_front(leaf);
	}

	// Recursively insert.
	if (!handled)
	{
		int where;

		// keep track of the tree depth
		where = self->insert_leaf_rec_1(tree, leaf, depth + 1);

		handled = (where >= 0);
	}

	// Keep a record of the highest depth.
	if (handled && depth > tree->depth)
	{
		tree->depth = depth;
	}

	return handled;
}

//--------------------------------------------------------------------------------------------
// BSP_tree_t
//--------------------------------------------------------------------------------------------
BSP_tree_t::BSP_tree_t(const Parameters& parameters) :
	dimensions(parameters._dim),
	max_depth(parameters._maxDepth),
	depth(0),
	_free(nullptr), _nfree(0),
	_used(nullptr), _nused(0),
	finite(nullptr),
	infinite(),
	bbox(),
	bsp_bbox(parameters._dim)
{
	// Load the singly-linked list of unused branches with preallocated branches.
	for (size_t index = 0; index < parameters._numBranches; ++index)
	{
		BSP_branch_t *branch = new BSP_branch_t(parameters._dim);
		if (!branch)
		{
			throw std::bad_alloc(); /* @todo Remove this. */
		}
		Shell *shell = (Shell *)branch;
		shell->_next = _free; _free = shell; _nfree++;
	}
}

//--------------------------------------------------------------------------------------------
BSP_tree_t::~BSP_tree_t()
{
	// Clear the roots.
	finite = nullptr;

	// As long as there are used branches ...
	while (_nused > 0)
	{
		// ... prune the tree.
		size_t pruned = prune();
		std::stringstream s;
		s << __FILE__ << ":" << __LINE__ << ": pruned: " << pruned << ", remaining: " << _nused << std::endl;
		log_info("%s", s.str().c_str());
	}

	// Destroy all branches in the free branch list.
	while (_free)
	{
		Shell *shell = _free; _free = _free->_next; _nfree--;
		BSP_branch_t *branch = (BSP_branch_t *)shell;
		delete branch;
	}
}

//--------------------------------------------------------------------------------------------
BSP_tree_t::Parameters::Parameters(size_t dim, size_t maxDepth)
{
	// Validate dimensionality.
	if (dim < BSP_tree_t::Parameters::ALLOWED_DIM_MIN)
	{
		log_error("%s: %d: specified dimensionality %zu is smaller than allowed minimum dimensionality %zu\n", \
			      __FILE__, __LINE__, dim, BSP_tree_t::Parameters::ALLOWED_DIM_MIN);
		throw std::domain_error("dimensionality out of range");
	}
	if (dim > BSP_tree_t::Parameters::ALLOWED_DEPTH_MAX)
	{
		log_error("%s:%d: specified dimensionality %zu is greater than allowed maximum dimensionality %zu\n", \
			__FILE__, __LINE__, dim, BSP_tree_t::Parameters::ALLOWED_DIM_MAX);
		throw std::domain_error("dimensionality out of range");
	}
	// Validate maximum depth.
	if (maxDepth > BSP_tree_t::Parameters::ALLOWED_DEPTH_MAX)
	{
		log_error("%s: %d: specified maximum depth %zu is greater than allowed maximum depth %zu\n", \
			      __FILE__, __LINE__, maxDepth, BSP_tree_t::Parameters::ALLOWED_DEPTH_MAX);
		throw std::domain_error("maximum depth out of range");
	}
	// Compute the number of branches for a complete tree.
	size_t n = std::pow(2, dim * (maxDepth+1))-1;
	size_t d = std::pow(2, dim) - 1;
	_numBranches = n / d;
	// Compute the number of branches to pre-allocated branches.
	_dim = dim;
	_maxDepth = maxDepth;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t *BSP_tree_t::createBranch()
{
	if (!_free)
	{
		log_error("%s:%d: no more free branches available\n", __FILE__, __LINE__);
		return nullptr;
	}
	Shell *shell = _free; _free = _free->_next; _nfree--;
	shell->_next = _used; _used = shell; _nused++;
	BSP_branch_t *branch = (BSP_branch_t *)shell;
	// The branch may not have a parent and must be empty.
	EGOBOO_ASSERT(nullptr == branch->parent && branch->empty());
	return branch;
}
//--------------------------------------------------------------------------------------------
void BSP_tree_t::clear_rec()
{
	// Clear all the branches, recursively.
	if (finite)
	{
		finite->clear(true);
	}

	// Clear the infinite leaves of the tree.
	infinite.clear();

	// Set the bbox to empty
	bv_self_clear(&(bbox));
}

//--------------------------------------------------------------------------------------------
size_t BSP_tree_t::prune()
{
	if (!finite) return 0;

	Shell **prev = &_used, *cur; size_t count = 0;
	// Search through all used branches.
	// This will not catch all of the empty branches every time, but should catch quite a few.
	while (nullptr != *prev)
	{
		Shell *cur = *prev;
		BSP_branch_t *branch = (BSP_branch_t *)cur;
		if (branch == finite || !branch->isEmpty()) // Do not remove root o non-empty branches.
		{ 
			prev = &cur->_next;
		}
		else
		{
			bool found = branch->unlink_parent();
			if (!found) {
				log_error("%s:%d: invalid BSP tree\n",__FILE__,__LINE__);
				std::domain_error("invalid BSP tree");
			}
			branch->depth = 0;
			BSP_aabb_t::set_empty(&(branch->bsp_bbox));

			// Unlink this branch from the "used" list and ...
			*prev = cur; _nused--;
			// ... add it to the "free" list.
			cur->_next = _free; _free = cur; _nfree++;
			count++;
		}
	}
	return count;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t *BSP_tree_t::ensure_root()
{
	if (finite) return finite;

	BSP_branch_t *root = createBranch();
	if (!root) return nullptr;

	// Copy the tree bounding box to the root node.
	for (size_t i = 0; i < dimensions; ++i)
	{
		root->bsp_bbox.mins.ary[i] = bsp_bbox.mins.ary[i];
		root->bsp_bbox.mids.ary[i] = bsp_bbox.mids.ary[i];
		root->bsp_bbox.maxs.ary[i] = bsp_bbox.maxs.ary[i];
	}

	// Fix the depth.
	root->depth = 0;

	// Assign the root to the tree.
	finite = root;

	return root;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t *BSP_branch_t::ensure_branch(BSP_branch_t *self, BSP_tree_t *tree, size_t index)
{
	if (!tree || !self) return nullptr;
	if (index > self->children.lst_size) return nullptr;

	// If not child exists yet ...
	BSP_branch_t *child = self->children.lst[index];

	// ... create it.
	if (nullptr == child)
	{
		// Get a free branch ...
		child = tree->createBranch();

		if (nullptr != child)
		{
			// ... make sure that it is unlinked ...
			child->unlink_all();

			// ... generate its bounding box ...
			child->depth = self->depth + 1;
			_generate_BSP_aabb_child(&(self->bsp_bbox), index, &(child->bsp_bbox));

			// ... and insert it in the correct position.
			BSP_branch_insert_branch(self, index, child);
		}
	}

	return child;
}
//--------------------------------------------------------------------------------------------
bool BSP_tree_t::insert_leaf(BSP_leaf_t *leaf)
{
	bool retval;

	if (!leaf) return false;

	// If the leaf is fully contained in the tree's bounding box ...
	if (!bsp_bbox.contains_aabb(leaf->bbox.aabb))
	{
		// ... put the leaf at the head of the infinite list.
		retval = infinite.push_front(leaf);
	}
	// Otherwise:
	else
	{
		BSP_branch_t *root = ensure_root();

		retval = BSP_branch_t::insert_leaf_rec(root, this, leaf, 0);
	}

	// Make sure the bounding box matches the maximum extend of all leaves in the tree.
	if (retval)
	{
		if (bv_is_clear(&(bbox)))
		{
			bbox = leaf->bbox;
		}
		else
		{
			bv_self_union(&(bbox), &(leaf->bbox));
		}
	}

	return retval;
}

//--------------------------------------------------------------------------------------------

size_t BSP_tree_t::collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	if (!aabb || !collisions)
	{
		return 0;
	}
	// Collide with any "infinite" nodes.
	infinite.collide(aabb, test, collisions);

	// Collide with the rest of the BSP tree.
	if (finite)
	{
		finite->collide(aabb, test, collisions);
	}
	return collisions->size();
}

//--------------------------------------------------------------------------------------------
size_t BSP_tree_t::collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	if (!frustum || !collisions)
	{
		return 0;
	}
	// Collide with any "infinite" nodes.
	infinite.collide(frustum, test, collisions);

	// Collide with the rest of the BSP tree.
	if (finite)
	{
		finite->collide(frustum, test, collisions);
	}
	return collisions->size();
}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t::BSP_leaf_list_t() :
	count(0),
	lst(nullptr),
	bbox()
{

}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t::~BSP_leaf_list_t()
{
	bbox.dtor();
	lst = nullptr;
	count = 0;
}

//--------------------------------------------------------------------------------------------
void BSP_leaf_list_t::clear()
{
	while (nullptr != lst)
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->next;
		leaf->next = nullptr;
		leaf->inserted = false;
		count--;
	}
	bv_self_clear(&bbox);
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_t::push_front(BSP_leaf_t *leaf)
{
	if (nullptr == leaf)
	{
		return false;
	}
	if (leaf->inserted)
	{
		log_warning("%s: %d: trying to insert a BSP_leaf that is claiming to be part of a list already\n",__FILE__,__LINE__);
		return false;
	}

	if (nullptr == lst)
	{
		// Insert the leaf.
		leaf->next = lst; lst = leaf;
		count++;
		leaf->inserted = true;
		// Use the leaf's bounding box as the bounding box of the leaf list.
		bbox = leaf->bbox;
	}
	else
	{
		// Insert the leaf at the beginning of the leaf list.
		leaf->next = lst; lst = leaf;
		count++;
		leaf->inserted = true;
		// Use the union of the leaf's bounding box and the old bounding box the leaf list
		// as the new bounding box of the leaf list.
		bv_self_union(&(bbox), &(leaf->bbox));
		return true;
	}
	return true;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t *BSP_leaf_list_t::pop_front()
{
	if (nullptr == lst)
	{
		return nullptr;
	}
	else
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->next;
		leaf->next = nullptr;
		leaf->inserted = false;
		count--;
		return leaf;
	}
}

bool BSP_leaf_list_t::add_all(Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
{
	size_t lost_nodes = 0;

	BSP_leaf_t *leaf;
	size_t cnt;
	for (cnt = 0, leaf = lst; nullptr != leaf && cnt < count; leaf = leaf->next, ++cnt)
	{
		if (rv_success != collisions->push_back(leaf))
		{
			lost_nodes++;
		}
	}

	// Warn if any nodes were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	return (collisions->size() < collisions->capacity());
}

bool BSP_leaf_list_t::add_all(BSP_leaf_test_t& test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
{
	size_t lost_nodes = 0;

	BSP_leaf_t *leaf;
	size_t cnt;
	for (cnt = 0, leaf = lst; nullptr != leaf && cnt < count; leaf = leaf->next, ++cnt)
	{
		if ((*test)(leaf))
		{
			if (rv_success != collisions->push_back(leaf))
			{
				lost_nodes++;
			}
		}
	}

	// Warn if any nodes were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	return (collisions->size() < collisions->capacity());
}
//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_t::collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
{
	// Validate arguments.
	if (!aabb || !collisions)
	{
		return false;
	}
	// If this leaf list is empty, there is nothing to do.
	if (empty())
	{
		return true;
	}
	// If the AABB does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (!aabb_intersects_aabb(*aabb, bbox.aabb))
	{
		return false;
	}

	size_t lost_nodes = 0;  // The number of lost nodes.
	BSP_leaf_t *leaf = lst; // The current leaf.
	if (nullptr != test)
	{
		// Iterate over the leaves in the leaf list.
		for (; nullptr != leaf; leaf = leaf->next)
		{
			// The leaf must have a valid type.
			EGOBOO_ASSERT(leaf->data_type > -1);
			// The leaf must be in a leaf list.
			EGOBOO_ASSERT(leaf->inserted);

			// Test geometry.
			geometry_rv geometry_test = aabb_intersects_aabb(*aabb, leaf->bbox.aabb);

			// Determine what action to take.
			bool do_insert = false;
			if (geometry_test > geometry_outside)
			{
				do_insert = (*test)(leaf);
			}

			if (do_insert)
			{
				if (rv_success != collisions->push_back(leaf))
				{
					lost_nodes++;
				}
			}
		}
	}
	else
	{

		// Iterate over the leaves in the leaf list.
		for (; nullptr != leaf; leaf = leaf->next)
		{
			// The leaf must have a valid type.
			EGOBOO_ASSERT(leaf->data_type > -1);
			// The leaf must be inserted.
			EGOBOO_ASSERT(leaf->inserted);

			// Test the geometry.
			geometry_rv geometry_test = aabb_intersects_aabb(*aabb, leaf->bbox.aabb);

			// determine what action to take
			bool do_insert = false;
			if (geometry_test > geometry_outside)
			{
				do_insert = true;
			}

			if (do_insert)
			{
				if (rv_success != collisions->push_back(leaf))
				{
					lost_nodes++;
				}
			}
		}
	}

	// Warn if any nodes were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	// Return false if the collision list is full.
	return !collisions->full();
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_t::collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	// Validate arguments.
	if (!frustum || !collisions)
	{
		return false;
	}

	// If this leaf list is empty, there is nothing to do.
	if (empty())
	{
		return true;
	}

	// If the frustum does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (!frustum->intersects_bv(&(bbox),true))
	{
		return false;
	}

	// If the collision list is full, return false.
	if (collisions->full())
	{
		return false;
	}

	size_t lost_nodes = 0; // The number of lost nodes.
	BSP_leaf_t *leaf = lst;  // The current leaf.
	// Scan through every leaf
	for (; nullptr != leaf; leaf = leaf->next)
	{
		// The leaf must have a valid type.
		EGOBOO_ASSERT(leaf->data_type > -1);
		// The leaf must be inserted.
		EGOBOO_ASSERT(leaf->inserted);

		// Test the geometry
		geometry_rv geometry_test = frustum->intersects_bv(&leaf->bbox, true);

		// Determine what action to take.
		bool do_insert = false;
		if (geometry_test > geometry_outside)
		{
			if (nullptr == test)
			{
				do_insert = true;
			}
			else
			{
				// we have a possible intersection
				do_insert = (*test)(leaf);
			}
		}

		if (do_insert)
		{
			if (rv_success != collisions->push_back(leaf))
			{
				lost_nodes++;
			}
		}
	}

	// Warn if any nodes were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	// Return false if the collision list is full.
	return !collisions->full();
}

//--------------------------------------------------------------------------------------------
// BSP_branch_list_t
//--------------------------------------------------------------------------------------------
BSP_branch_list_t::BSP_branch_list_t(size_t dim) :
	lst_size(0),
	lst(nullptr),
	inserted(0),
	bbox()
{
	// Determine the number of children from the dimensionality:
	// If we have two dimensions then we have 2 children.
	// @todo Is this correct? A branch forks of into dim children.
	size_t child_count = (0 == dim) ? 0 : (2 << (dim - 1));
	this->lst = EGOBOO_NEW_ARY(BSP_branch_t*, child_count);
	if (!this->lst)
	{
		throw std::bad_alloc();
	}
	this->inserted = 0;
	this->lst_size = child_count;
	for (size_t index = 0; index < child_count; ++index)
	{
		this->lst[index] = nullptr;
	}
}

//--------------------------------------------------------------------------------------------
BSP_branch_list_t::~BSP_branch_list_t()
{
	EGOBOO_DELETE_ARY(this->lst);
	this->lst = nullptr;
	this->lst_size = 0;
	this->inserted = 0;
	this->bbox.dtor();
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_list_t::clear_rec()
{
	// Recursively clear out any children.
	for (size_t cnt = 0; cnt < lst_size; cnt++)
	{
		if (!lst[cnt]) continue;
		lst[cnt]->clear(true);
		lst[cnt] = nullptr;
	}

	// Set the number of inserted children to 0.
	inserted = 0;

	// Clear out the size of the children.
	bv_self_clear(&(bbox));

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_list_t::collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	// Scan the child branches and collide with them recursively.
	size_t numberOfCollisions = 0;
	for (size_t i = 0; i < lst_size; ++i)
	{
		BSP_branch_t *child = lst[i];
		if (!child) continue;
		if (child->collide(aabb, test, collisions))
		{
			numberOfCollisions++;
		}
	}
	return numberOfCollisions > 0;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_list_t::collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	// Scan the child branches and collide with them recursively.
	size_t numberOfCollisions = 0;
	for (size_t i = 0; i < lst_size; ++i)
	{
		BSP_branch_t *child = lst[i];
		if (!child) continue;
		if (child->collide(frustum, test, collisions))
		{
			numberOfCollisions++;
		}
	}

	return numberOfCollisions > 0;
}

bool BSP_leaf_valid(const BSP_leaf_t *self)
{
    if (!self) return false;
	if (!self->data || self->data_type < 0) return false;
    return true;
}
