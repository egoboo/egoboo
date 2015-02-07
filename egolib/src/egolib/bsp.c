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

namespace BSP
{
	/**
	 * @brief
	 *	Raise a "subspace index out of bounds" exception.
	 * @param file, line
	 *	the C++ source position associated with the error
	 * @param index
	 *	the index
	 * @param listSize
	 *	the list size
	 */
	void throwSubspaceIndexOutOfBounds(const char *file, int line, BSP::SubspaceIndex index, size_t listSize)
	{
		std::ostringstream msg;
		msg << file << ": " << line << ": "
			<< "subspace index " << index << " out of bounds [" << 0 << "," << listSize << ")";
		log_error("%s\n", msg.str().c_str());
		throw std::invalid_argument(msg.str());
	}
	void assertSubspaceIndexWithinBounds(const char *file, int line, BSP::SubspaceIndex index, size_t listSize)
	{
		if (index < 0 || index >= listSize)
		{
			throwSubspaceIndexOutOfBounds(file, line, index, listSize);
		}
	}

	/**
	 * @brief
	 *	The number of children for the given dimensionality.
	 * @return
	 *	the number of children (of a branch) for the given dimensionality
	 * @remark
	 *	For a dimensionality \f$d\f$ each branch may fork into (at most)
	 *	\f$2^d\f$ other branches. For instance, if \f$d = 3\f$ then
	 *	the branch forks in at most \f$2^3=8\f$ child branches.
	 */
	size_t childCount(size_t dim)
	{
		if (!dim) throw std::domain_error("invalid dimensionality");
		return 2 << dim; // ~ 2^dim
	}
};




//--------------------------------------------------------------------------------------------
// Generic BSP functions
//--------------------------------------------------------------------------------------------
bool _generate_BSP_aabb_child(BSP_aabb_t * psrc, BSP::SubspaceIndex index, BSP_aabb_t * pdst)
{
	signed tnc, child_count;

	// Valid source?
	if (!psrc || !psrc->getDim()) return false;

	// Valid destination?
	if (!pdst) return false;

	// Valid subspace index?
	child_count = BSP::childCount(psrc->getDim());
	BSP::assertSubspaceIndexWithinBounds(__FILE__, __LINE__, index, child_count);

	// make sure that the destination type matches the source type
	pdst->setDim(psrc->getDim());

	// determine the bounds
	for (size_t i = 0, n = psrc->getDim(); i < n; ++i)
	{
		float maxval, minval;

		tnc = psrc->getDim() - 1 - i;

		if (0 == (index & (1 << tnc))) /* Tests the sub-space bit. */
		{
			minval = psrc->min()[i];
			maxval = psrc->mid()[i];
		}
		else
		{
			minval = psrc->mid()[i];
			maxval = psrc->max()[i];
		}

		pdst->min()[i] = minval;
		pdst->max()[i] = maxval;
		pdst->mid()[i] = 0.5f * (minval + maxval);
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
BSP::SubspaceIndex _find_child_index(const BSP_aabb_t *branchAABB, const aabb_t *leafAABB)
{
	//---- determine which child the leaf needs to go under

	/// @note This function is not optimal, since we encode the comparisons
	/// in the 32-bit integer indices, and then may have to decimate index to construct
	/// the child  branch's bounding by calling _generate_BSP_aabb_child().
	/// The reason that it is done this way is that we would have to be dynamically
	/// allocating and deallocating memory every time this function is called, otherwise. Big waste of time.

	// Validate arguments.
	if (!branchAABB) throw std::invalid_argument("nullptr == branchAABB");
	if (!leafAABB) throw std::invalid_argument("nullptr == leafAABB");

	// Validate arguments.
	if (0 == branchAABB->getDim()) throw std::invalid_argument("invalid dimensionality of BSP AABB of branch");
	if (branchAABB->empty()) throw std::invalid_argument("empty BSP AABB of branch");

	// Get aliases to the minimum, median and maximum of the BSP AABB of the branch.
	const float *branch_min_ary = branchAABB->min();
	const float *branch_mid_ary = branchAABB->mid();
	const float *branch_max_ary = branchAABB->max();

	// Get aliases the the minimum and the maximum of the AABB of the leaf.
	const float *leaf_min_ary = leafAABB->mins.v;
	const float *leaf_max_ary = leafAABB->maxs.v;

	BSP::SubspaceIndex index = 0;
	size_t d = std::min(branchAABB->getDim(), (size_t)3);
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
				break;
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
		for (size_t i = 0, n = children.lst_size; i < n; ++i)
		{
			if (nullptr != children.lst[i])
			{
				children.lst[i]->parent = nullptr;
			}

			children.lst[i] = nullptr;
		}

	}

	children_ptr->_bounds.clear();

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
bool BSP_branch_update_depth_rec(BSP_branch_t *self, size_t depth)
{
	if (!self) return false;
	BSP_branch_list_t *children_ptr = &(self->children);

	self->depth = depth;

	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		for (size_t cnt = 0; cnt < children_ptr->lst_size; cnt++)
		{
			if (!children_ptr->lst[cnt]) continue;

			BSP_branch_update_depth_rec(children_ptr->lst[cnt], depth + 1);
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------
size_t BSP_branch_t::removeAllLeaves()
{
	size_t count = 0;
	if (!INVALID_BSP_BRANCH_LIST(&(children)))
	{
		// Recursively clear out any nodes in the child list
		for (size_t i = 0, n = children.lst_size; i < n; ++i)
		{
			if (!children.lst[i]) continue;
			count += children.lst[i]->removeAllLeaves();
		}
	}

	// Remove all leaves of this branch.
	count += leaves.removeAllLeaves();

	// Remove all unsorted leaves of this branch.
	count += unsorted.removeAllLeaves();

	return count;
}

bool BSP_branch_t::insert_branch(BSP_branch_t *self, BSP::SubspaceIndex index, BSP_branch_t *branch)
{
	// B and B2 exist?
	if (!self) throw std::invalid_argument("nullptr == self");
	if (!branch) throw std::invalid_argument("nullptr == branch");

	BSP_branch_list_t *children_ptr = &(self->children);

	// valid child list for B?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// Valid subspace index range?
	BSP::assertSubspaceIndexWithinBounds(__FILE__, __LINE__, index, self->children.lst_size);

	// We can't merge two branches at this time.
	if (children_ptr->lst[index])
	{
		return false;
	}

	EGOBOO_ASSERT(self->depth + 1 == branch->depth);

	// Insert the branch into the list.
	children_ptr->lst[index] = branch; branch->parent = self; children_ptr->inserted++;

	// Update the all bounds above branch.
	for (BSP_branch_t *ptmp = branch->parent; NULL != ptmp; ptmp = ptmp->parent)
	{
		BSP_branch_list_t * tmp_children_ptr = &(ptmp->children);

		// Add the bounds of the children.
		tmp_children_ptr->_bounds.add(branch->children._bounds);

		// Add the bounds of the leaves.
		tmp_children_ptr->_bounds.add(branch->leaves._bounds);

		// Add the bounds of the unsorted leaves.
		tmp_children_ptr->_bounds.add(branch->unsorted._bounds);
	}

	// update the depth B2 and all children
	BSP_branch_update_depth_rec(branch, self->depth + 1);

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
			for (size_t i = 0, n = children->lst_size; i < n; ++i)
			{
				if (!children->lst[i]) continue;

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
bool BSP_branch_t::add_all_children(BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	if (!INVALID_BSP_BRANCH_LIST(&(children)))
	{
		for (size_t i = 0, n = children.lst_size; i < n; ++i)
		{
			BSP_branch_t *child = children.lst[i];
			if (!child) continue;
			child->add_all_rec(test, collisions);
			if (collisions->full()) break;
		}

	}
	return !collisions->full();
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::add_all_rec(BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{

	if (test)
	{
		leaves.add_all(*test, collisions);
		unsorted.add_all(*test, collisions);
	}
	else
	{
		leaves.add_all(collisions);
		unsorted.add_all(collisions);
	}

	if (!INVALID_BSP_BRANCH_LIST(&(children)))
	{
		for (size_t i = 0, n = children.lst_size; i < n; ++i)
		{
			BSP_branch_t *child = children.lst[i];
			if (!child) continue;
			child->add_all_rec(test, collisions);
			if (collisions->full()) break;
		}

	}

	return !collisions->full();
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::empty() const
{
	// If the branch has leaves, it is not empty.
	if (!leaves.empty())
	{
		return false;
	}

	// If the branch has unsorted leaves, it is not empty.
	if (!unsorted.empty())
	{
		return false;
	}
	// If the branch has child branches, then it is not empty.
	if (!INVALID_BSP_BRANCH_LIST(&children))
	{
		for (size_t i = 0, n = children.lst_size; i < n; ++i)
		{
			if (nullptr != children.lst[i])
			{
				return false;
			}
		}
	}
	return true;
}

//--------------------------------------------------------------------------------------------
void BSP_branch_t::collide(const aabb_t *aabb, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
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
			geom_unsorted = unsorted_ptr->_bounds.intersects(*aabb);
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
			geom_nodes = leaves_ptr->_bounds.intersects(*aabb);
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
			geom_children = children_ptr->_bounds.intersects(*aabb);
		}
	}

	if (geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside)
	{
		// The branch and the object do not overlap at all. Do nothing.
		return;
	}

	switch (geom_unsorted)
	{
	case geometry_intersect:
		// The aabb and branch partially overlap. Test each item.
		unsorted_ptr->collide(aabb, test, collisions);
		retval = true;
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
		leaves_ptr->collide(aabb, test, collisions);
		retval = true;
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
		children.collide(aabb, test, collisions);
		retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		if (add_all_children(test, collisions)) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}
}

//--------------------------------------------------------------------------------------------
void BSP_branch_t::collide(const egolib_frustum_t *frustum, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
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
			geom_unsorted = unsorted_ptr->_bounds.intersects(*frustum);
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
			geom_nodes = leaves_ptr->_bounds.intersects(*frustum);
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
			geom_children = children_ptr->_bounds.intersects(*frustum);
		}
	}

	if (geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside)
	{
		// The branch and the object do not overlap at all. Do nothing.
		return;
	}

	switch (geom_unsorted)
	{
	case geometry_intersect:
		// The frustum and branch partially overlap. Test each item.
		unsorted_ptr->collide(frustum, test, collisions);
		retval = true;
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
		leaves_ptr->collide(frustum, test, collisions);
		retval = true;
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
		children.collide(frustum, test, collisions);
		retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		BSP_retval = add_all_children(test, collisions);
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::insert_branch_list_rec(BSP_branch_t *self, BSP_tree_t *tree, BSP_leaf_t *leaf, BSP::SubspaceIndex index, size_t depth)
{
	// the leaf is a child of this branch
	bool inserted_branch;

	// Validate arguments.
	if (!self) throw std::invalid_argument("nullptr == self");
	if (!tree) throw std::invalid_argument("nullptr == tree");
	if (!leaf) throw std::invalid_argument("nullptr == leaf");
#if 0
	if (!self || !tree || !leaf) return false;
#endif
	BSP_branch_list_t *children_ptr = &(self->children);

	// a valid child list?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// Is the index within the correct range?
	BSP::assertSubspaceIndexWithinBounds(__FILE__, __LINE__, index, self->children.lst_size);

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
		children_ptr->_bounds.set(leaf->bbox);
	}
	else
	{
		children_ptr->_bounds.add(leaf->bbox);
	}
	children_ptr->inserted++;
	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::insert_leaf_rec_1(BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth)
{
	int retval;

	bool inserted_leaf, inserted_branch;

	if (!tree || !leaf) return -1;

	// Not inserted anywhere, yet.
	bool inserted_here = false;
	bool inserted_children = false;

	// Don't go too deep.
	if (depth > tree->_parameters.getMaxDepth())
	{
		// Insert the leaf into the leaves of this branch.
		if (insert_leaf_list(leaf))
		{
			return true;
		}
	}
	else
	{
		// Determine which child the leaf needs to go under.
		BSP::SubspaceIndex index = _find_child_index(&(bsp_bbox), &(leaf->bbox.aabb));

		// Insert the leaf in the right place.
		if (index == -2)
		{
			/* Do nothing, the leaf is too big. */
			return false;
		}
		else if (-1 == index)
		{
			// The leaf belongs on this branch.
			if (insert_leaf_list(leaf))
			{
				return true;
			}
		}
		else
		{
			// The leaf belongs on child branches of this branch.
			if (BSP_branch_t::insert_branch_list_rec(this, tree, leaf, index, depth))
			{
				return true;
			}
		}
	}

	return false;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::insert_leaf_rec(BSP_branch_t *self, BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth)
{
	if (!self || !tree || !leaf) return false;

	BSP_leaf_list_t *unsorted_ptr = &(self->unsorted);
	BSP_leaf_list_t *leaves_ptr = &(self->leaves);

	// nothing has been done with the pleaf pointer yet
	bool handled = false;

	// count the nodes in different categories
	size_t sibling_nodes = leaves_ptr->count;
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
				if (self->insert_leaf_rec_1(tree, tmp_leaf, depth + 1))
				{
					inserted++;
				}

				// Break out BEFORE popping off another pointer.
				// Otherwise, we lose drop a node by mistake.
				if (unsorted_ptr->count <= min_unsorted) break;

				tmp_leaf = self->unsorted.pop_front();
			} while (NULL != tmp_leaf);

			// Clear the old bounds.
			unsorted_ptr->_bounds.clear();

			if (unsorted_ptr->count > 0)
			{
				// generate the correct bbox for any remaining nodes
				size_t cnt = 0;
				for (tmp_leaf = unsorted_ptr->lst;
					 nullptr != tmp_leaf && cnt < unsorted_ptr->count;
					 tmp_leaf = tmp_leaf->next, cnt++)
				{
					unsorted_ptr->_bounds.add(tmp_leaf->bbox);
				}
			}
		}
	}

	// Re-calculate the values for this branch.
	sibling_nodes = leaves_ptr->count;
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
		// keep track of the tree depth
		handled = self->insert_leaf_rec_1(tree, leaf, depth + 1);
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
	_parameters(parameters),
	depth(0),
	_free(nullptr), _nfree(0),
	_used(nullptr), _nused(0),
	finite(nullptr),
	infinite(),
	bbox(),
	bsp_bbox(parameters.getDim())
{
	// Load the singly-linked list of unused branches with preallocated branches.
	for (size_t index = 0; index < parameters.getMaxNodes(); ++index)
	{
		BSP_branch_t *branch = new BSP_branch_t(parameters.getDim());
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
	// Compute the number of nodes for a complete tree.
	size_t n = std::pow(2, dim * (maxDepth+1))-1;
	size_t d = std::pow(2, dim) - 1;
	_maxNodes = n / d;
	// Store the dimensionality.
	_dim = dim;
	// Store the maximum depth.
	_maxDepth = maxDepth;
	_maxChildNodes = 2 << dim;
}

BSP_tree_t::Parameters::Parameters(const Parameters& other)
	: _dim(other._dim),
	  _maxNodes(other._maxNodes),
	  _maxChildNodes(other._maxChildNodes),
	  _maxDepth(other._maxDepth)
{

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
		if (branch == finite || !branch->empty()) // Do not remove root branches or non-empty branches.
		{ 
			prev = &cur->_next;
		}
		else
		{
			bool found = branch->unlink_parent();
			if (!found)
			{
				log_error("%s:%d: invalid BSP tree\n",__FILE__,__LINE__);
				std::domain_error("invalid BSP tree");
			}
			branch->depth = 0;
			branch->bsp_bbox.clear();

			// Unlink this branch from the "used" list and ...
			*prev = cur->_next; _nused--;
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
	root->bsp_bbox.set(bsp_bbox);

	// Fix the depth.
	root->depth = 0;

	// Assign the root to the tree.
	finite = root;

	return root;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t *BSP_branch_t::ensure_branch(BSP_branch_t *self, BSP_tree_t *tree, BSP::SubspaceIndex index)
{
	if (!self) throw std::invalid_argument("nullptr == self");
	if (!tree) throw std::invalid_argument("nullptr == tree");
	BSP::assertSubspaceIndexWithinBounds(__FILE__,__LINE__,index,self->children.lst_size);

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
			BSP_branch_t::insert_branch(self, index, child);
		}
	}

	return child;
}
//--------------------------------------------------------------------------------------------
namespace BSP
{
	namespace Hacks
	{
		/**
		 * @warning
		 *	A hack that is active as long as the issues with the BSPs are not fixed.
		 *	It forces all leaves into the infinite leave list. If this is not done
		 *  massive clipping problems occur.
		 */
		const static bool ForceInfinit = true;
	}
}
bool BSP_tree_t::insert_leaf(BSP_leaf_t *leaf)
{
	bool retval;

	if (!leaf) return false;

	// If the leaf is NOT fully contained in the tree's bounding box ...
	if (!bsp_bbox.contains(leaf->bbox.aabb) || BSP::Hacks::ForceInfinit)
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

void BSP_tree_t::collide(const aabb_t *aabb, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	if (!aabb) throw std::invalid_argument("nullptr == aabb");
	if (!collisions) throw std::invalid_argument("nullptr == collisions");

	// Collide with any "infinite" nodes.
	infinite.collide(aabb, test, collisions);

	// Collide with the rest of the BSP tree.
	if (finite)
	{
		finite->collide(aabb, test, collisions);
	}
}

void BSP_tree_t::collide(const egolib_frustum_t *frustum, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	if (!frustum) throw std::invalid_argument("nullptr == frustum");
	if (!collisions) throw std::invalid_argument("nullptr == collisions");

	// Collide with any "infinite" nodes.
	infinite.collide(frustum, test, collisions);

	// Collide with the rest of the BSP tree.
	if (finite)
	{
		finite->collide(frustum, test, collisions);
	}
}

size_t BSP_tree_t::removeAllLeaves()
{
	size_t count = infinite.removeAllLeaves();
	if (finite)
	{
		count += finite->removeAllLeaves();
	}
	return count;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t::BSP_leaf_list_t() :
	count(0),
	lst(nullptr),
	_bounds()
{

}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t::~BSP_leaf_list_t()
{
	lst = nullptr;
	count = 0;
}

//--------------------------------------------------------------------------------------------
size_t BSP_leaf_list_t::removeAllLeaves()
{
	size_t oldCount = count;
	while (nullptr != lst)
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->next;
		leaf->next = nullptr;
		leaf->inserted = false;
		count--;
	}
	return oldCount;
}

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
	_bounds.clear();
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

	// Insert the leaf.
	leaf->next = lst; lst = leaf;
	count++;
	leaf->inserted = true;
	// Add the leaf's bounding box to the bounds of this leaf list.
	_bounds.add(leaf->bbox);
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

bool BSP_leaf_list_t::add_all(BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
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
void BSP_leaf_list_t::collide(const aabb_t *aabb, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
{
	// Validate arguments.
	if (!aabb) throw std::invalid_argument("nullptr == aabb");
	if (!collisions) throw std::invalid_argument("nullptr == collisions");

	// If this leaf list is empty, there is nothing to do.
	if (empty())
	{
		return;
	}
	// If the AABB does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (!_bounds.intersects(*aabb))
	{
		return;
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
}

//--------------------------------------------------------------------------------------------
void BSP_leaf_list_t::collide(const egolib_frustum_t *frustum, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	// Validate arguments.
	if (!frustum) throw std::invalid_argument("nullptr == frustum");
	if (!collisions) throw std::invalid_argument("nullptr == collisions");

	// If this leaf list is empty, there is nothing to do.
	if (empty())
	{
		return;
	}

	// If the frustum does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (!_bounds.intersects(*frustum))
	{
		return;
	}

	// If the collision list is full, return.
	if (collisions->full())
	{
		return;
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
}


//--------------------------------------------------------------------------------------------
// BSP_branch_list_t
//--------------------------------------------------------------------------------------------
BSP_branch_list_t::BSP_branch_list_t(size_t dim) :
	lst_size(0),
	lst(nullptr),
	inserted(0),
	_bounds()
{
	// Determine the number of children from the dimensionality:
	// If we have two dimensions then we have 2 children.
	size_t child_count = BSP::childCount(dim);
	this->lst = new BSP_branch_t *[child_count];
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
	delete[] this->lst;
	this->lst = nullptr;
	this->lst_size = 0;
	this->inserted = 0;
}

size_t BSP_branch_list_t::removeAllLeaves()
{
	size_t count = 0;
	for (size_t i = 0, n = lst_size; i < n; ++i)
	{
		if (!lst[i]) continue;
		count += lst[i]->removeAllLeaves();
	}
	return count;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_list_t::clear_rec()
{
	// Recursively clear out any children.
	for (size_t i = 0; i < lst_size; ++i)
	{
		if (!lst[i]) continue;
		lst[i]->clear(true);
		lst[i] = nullptr;
	}

	// Set the number of inserted children to 0.
	inserted = 0;

	// Clear out the size of the children.
	_bounds.clear();

	return true;
}

//--------------------------------------------------------------------------------------------
void BSP_branch_list_t::collide(const aabb_t *aabb, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	for (size_t i = 0, n = lst_size; i < n; ++i)
	{
		BSP_branch_t *child = lst[i];
		if (!child) continue;
		child->collide(aabb, test, collisions);
	}
}

void BSP_branch_list_t::collide(const egolib_frustum_t *frustum, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
	for (size_t i = 0, n = lst_size; i < n; ++i)
	{
		BSP_branch_t *child = lst[i];
		if (!child) continue;
		child->collide(frustum, test, collisions);
	}
}

//--------------------------------------------------------------------------------------------

bool BSP_leaf_valid(const BSP_leaf_t *self)
{
    if (!self) return false;
	if (!self->data || self->data_type < 0) return false;
    return true;
}
