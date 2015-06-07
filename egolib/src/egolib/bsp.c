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

const size_t BSP_tree_t::Parameters::ALLOWED_DEPTH_MAX = std::numeric_limits<size_t>::max() - 1;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define INVALID_BSP_BRANCH_LIST(BL) ( (NULL == (BL)) || (NULL == (BL)->lst) || (0 == (BL)->lst_size) )

#define BRANCH_NODE_THRESHOLD 5

//--------------------------------------------------------------------------------------------
// special functions

static bool _generate_BSP_aabb_child(BSP_aabb_t& source, BSP::SubspaceIndex index, BSP_aabb_t& target);
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
		return 1 << dim; // ~ 1 * 2^dim
	}
};




//--------------------------------------------------------------------------------------------
// Generic BSP functions
//--------------------------------------------------------------------------------------------
bool _generate_BSP_aabb_child(BSP_aabb_t& source, BSP::SubspaceIndex index, BSP_aabb_t& target)
{
	// Valid source?
	if (!source.getDim()) return false;

	// Valid subspace index?
	size_t childCount = BSP::childCount(source.getDim());
	BSP::assertSubspaceIndexWithinBounds(__FILE__, __LINE__, index, childCount);

	// Make sure that the destination type matches the source type
	target.setDim(source.getDim(),false);

	// determine the bounds
	for (size_t i = 0, n = source.getDim(); i < n; ++i)
	{
		float maxval, minval;

		// Compute the shift for the sub-space bit of dimension i.
		// Have a look at how a sub-space index is computed to understand this.
		size_t shift = source.getDim() - 1 - i;

		// Test the sub-space bit. Dito.
		if (0 == (index & (1 << shift)))
		{
			minval = source.min()[i];
			maxval = source.mid()[i];
		}
		else
		{
			minval = source.mid()[i];
			maxval = source.max()[i];
		}

		target.min()[i] = minval;
		target.max()[i] = maxval;
		target.mid()[i] = 0.5f * (minval + maxval);
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
	const auto& leaf_min_ary = leafAABB->getMin();
	const auto& leaf_max_ary = leafAABB->getMax();

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

bool BSP_leaf_t::remove_link(BSP_leaf_t *L)
{
	if (NULL == L) return false;

	bool retval = false;

	if (L->_inserted)
	{
		L->_inserted = false;
		retval = true;
	}

	if (NULL != L->_next)
	{
		L->_next = NULL;
		retval = true;
	}

    L->_bbox = bv_t();

	return retval;
}

bool BSP_leaf_t::clear(BSP_leaf_t * L)
{
	if (!L) return false;

	L->_type = BSP_LEAF_NONE;
	L->_data = nullptr;

	BSP_leaf_t::remove_link(L);

	return true;
}

bool BSP_leaf_t::valid() const
{
	return _data && _type >= 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

BSP_branch_t::BSP_branch_t(size_t dim) : Shell(),
	parent(nullptr),
	unsorted(),
	children(dim),
	leaves(),
	bsp_bbox(dim),
	depth(0)
{
}

BSP_branch_t::~BSP_branch_t()
{
	// Set actual depth to 0 and parent to nullptr.
	depth = 0;
	parent = nullptr;
}

bool BSP_branch_t::unlink_all()
{
	bool retval = true;

	if (!unlink_parent())   retval = false;
	if (!unlink_children()) retval = false;
	if (!unlink_leaves())   retval = false;

	return retval;
}

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

bool BSP_branch_t::unlink_leaves()
{
	return BSP_branch_free_nodes(this, false);
}

bool BSP_branch_t::insert_leaf_list(BSP_leaf_t *leaf)
{
	if (!leaf) throw std::invalid_argument("nullptr == leaf");
	return leaves.push_front(*leaf);
}

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

bool BSP_branch_t::insert_branch(BSP::SubspaceIndex index, BSP_branch_t *branch)
{
	// B and B2 exist?
	if (!branch) throw std::invalid_argument("nullptr == branch");

	BSP_branch_list_t *children_ptr = &(children);

	// valid child list for B?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// Valid subspace index range?
	BSP::assertSubspaceIndexWithinBounds(__FILE__, __LINE__, index, children.lst_size);

	// We can't merge two branches at this time.
	if (children_ptr->lst[index])
	{
		return false;
	}

	EGOBOO_ASSERT(depth + 1 == branch->depth);

	// Insert the branch into the list.
	children_ptr->lst[index] = branch; branch->parent = this; children_ptr->inserted++;

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
	BSP_branch_update_depth_rec(branch, depth + 1);

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

bool BSP_branch_list_t::add_all_children(Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	if (!INVALID_BSP_BRANCH_LIST(this))
	{
		for (size_t i = 0, n = lst_size; i < n; ++i)
		{
			BSP_branch_t *child = lst[i];
			if (!child) continue;
			child->add_all_rec(collisions);
			if (collisions.full()) break;
		}

	}
	return !collisions.full();
}


bool BSP_branch_list_t::add_all_children(BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	if (!INVALID_BSP_BRANCH_LIST(this))
	{
		for (size_t i = 0, n = lst_size; i < n; ++i)
		{
			BSP_branch_t *child = lst[i];
			if (!child) continue;
			child->add_all_rec(test, collisions);
			if (collisions.full()) break;
		}

	}
	return !collisions.full();
}

bool BSP_branch_t::add_all_rec(Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	{
		leaves.add_all(collisions);
		unsorted.add_all(collisions);
	}
	{
		for (size_t i = 0, n = children.lst_size; i < n; ++i)
		{
			BSP_branch_t *child = children.lst[i];
			if (!child) continue;
			child->add_all_rec(collisions);
			if (collisions.full()) break;
		}

	}
	return !collisions.full();
}

bool BSP_branch_t::add_all_rec(BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	{
		leaves.add_all(test, collisions);
		unsorted.add_all(test, collisions);
	}
	{
		for (size_t i = 0, n = children.lst_size; i < n; ++i)
		{
			BSP_branch_t *child = children.lst[i];
			if (!child) continue;
			child->add_all_rec(test, collisions);
			if (collisions.full()) break;
		}

	}
	return !collisions.full();
}

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

void BSP_branch_t::collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	// Collide with the unsorted leaves.
	unsorted.collide(aabb,collisions);

	// Collide with the leaves leaves at this level of the tree.
	leaves.collide(aabb,collisions);

	// Collide with the child branches.
	children.collide(aabb, collisions);
}

void BSP_branch_t::collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	// Collide with the unsorted leaves.
	unsorted.collide(aabb,test,collisions);

	// Collide with the leaves at this level of the tree.
	leaves.collide(aabb,test,collisions);

	// Collide with the child branches.
	children.collide(aabb, test, collisions);
}

void BSP_branch_t::collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	// Collide with the unsorted leaves.
	unsorted.collide(frustum, collisions);

	// Collide with the leaves at this level of the tree.
	leaves.collide(frustum, collisions);

	// Colide with the child branches.
	children.collide(frustum, collisions);
}

void BSP_branch_t::collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	// Collide with the unsorted leaves.
	unsorted.collide(frustum,test,collisions);

	// Collide with the leaves at this level of the tree.
	leaves.collide(frustum,test,collisions);

	// Collide with the child branches.
	children.collide(frustum, test, collisions);
}

bool BSP_branch_t::insert_branch_list_rec(BSP_tree_t *tree, BSP_leaf_t *leaf, BSP::SubspaceIndex index, size_t depth)
{
	// the leaf is a child of this branch

	// Validate arguments.
	if (!tree) throw std::invalid_argument("nullptr == tree");
	if (!leaf) throw std::invalid_argument("nullptr == leaf");

	BSP_branch_list_t *children_ptr = &(children);

	// a valid child list?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// Is the index within the correct range?
	BSP::assertSubspaceIndexWithinBounds(__FILE__, __LINE__, index, children.lst_size);

	// Try to get the child.
	BSP_branch_t *child = ensure_branch(tree, index);
	if (nullptr != child)
	{
		return false;
	}
	// Try to insert the leaf.
	if (!child->insert_leaf_rec(tree, leaf, depth))
	{
		return false;
	}

	if (0 == children.inserted)
	{
		children._bounds.set(leaf->_bbox);
	}
	else
	{
		children._bounds.add(leaf->_bbox);
	}
	children.inserted++;
	return true;
}

bool BSP_branch_t::insert_leaf_rec_1(BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth)
{
	if (!tree) throw std::invalid_argument("nullptr == tree");
	if (!leaf) throw std::invalid_argument("nullptr == leaf");

	// Not inserted anywhere, yet.

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
		BSP::SubspaceIndex index = _find_child_index(&(bsp_bbox), &(leaf->_bbox.getAABB()));

		// Insert the leaf in the right place.
		if (index == -2)
		{
			/* Do nothing, the leaf is too big. */
			log_warning("%s:%d: leaf not inserted as it is too big\n", __FILE__, __LINE__);
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
			if (insert_branch_list_rec(tree, leaf, index, depth))
			{
				return true;
			}
		}
	}

	return false;
}

bool BSP_branch_t::insert_leaf_rec(BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth)
{
	if (!tree) throw std::invalid_argument("nullptr == tree");
	if (!leaf) throw std::invalid_argument("nullptr == leaf");

	BSP_leaf_list_t *unsorted_ptr = &(unsorted);
	BSP_leaf_list_t *leaves_ptr = &(leaves);

	// nothing has been done with the pleaf pointer yet
	bool handled = false;

	// count the nodes in different categories
	size_t sibling_leaves = leaves_ptr->getCount();
	size_t unsorted_leaves = unsorted_ptr->getCount();
	size_t leaves_at_this_level = sibling_leaves + unsorted_leaves;

	// Is list of unsorted leaves overflowing?
	if (unsorted_leaves > 0 && leaves_at_this_level >= BRANCH_NODE_THRESHOLD)
	{
		// Scan through the list and insert everyone in the next lower branch.
		BSP_leaf_t *tmp_leaf;
		int inserted;

		// If you just sort the whole list, you could end up with every single element in
		// the same node at some large depth (if the nodes are strange that way). Since,
		// REDUCING the depth of the NSPT is the entire point, we have to limit this in
		// some way...
		size_t target_unsorted = (sibling_leaves > BRANCH_NODE_THRESHOLD) ? 0 : BRANCH_NODE_THRESHOLD - sibling_leaves;
		size_t min_unsorted = target_unsorted >> 1;

		if (unsorted_ptr->getCount() > min_unsorted)
		{
			tmp_leaf = unsorted.pop_front();
			inserted = 0;
			do
			{
				if (insert_leaf_rec_1(tree, tmp_leaf, depth + 1))
				{
					inserted++;
				}

				// Break out BEFORE popping off another pointer.
				// Otherwise, we lose drop a node by mistake.
				if (unsorted_ptr->getCount() <= min_unsorted) break;

				tmp_leaf = unsorted.pop_front();
			} while (NULL != tmp_leaf);

			// Clear the old bounds.
			unsorted_ptr->_bounds.clear();

			if (unsorted_ptr->getCount() > 0)
			{
				// generate the correct bbox for any remaining nodes
				size_t cnt = 0;
				for (tmp_leaf = unsorted_ptr->lst;
					 nullptr != tmp_leaf && cnt < unsorted_ptr->getCount();
					 tmp_leaf = tmp_leaf->_next, cnt++)
				{
					unsorted_ptr->_bounds.add(tmp_leaf->_bbox);
				}
			}
		}
	}

	// Re-calculate the values for this branch.
	sibling_leaves = leaves_ptr->getCount();
	unsorted_leaves = unsorted_ptr->getCount();
	leaves_at_this_level = sibling_leaves + unsorted_leaves;

	// Defer sorting any new leaves, if possible.
	if (!handled && leaves_at_this_level < BRANCH_NODE_THRESHOLD)
	{
		handled = unsorted.push_front(*leaf);
		if (!handled) log_warning("%s:%d: leaf not inserted\n", __FILE__, __LINE__);
	}

	// Recursively insert.
	if (!handled)
	{
		// keep track of the tree depth
		handled = insert_leaf_rec_1(tree, leaf, depth + 1);
		/* @todo If the leaf is not handled, verify that it is handled by the caller. */
		/*if (!handled) log_warning("%s:%d: leaf not inserted\n", __FILE__, __LINE__);*/
	}

	// Keep a record of the highest depth.
	if (handled && depth > tree->depth)
	{
		tree->depth = depth;
	}

	return handled;
}

BSP_branch_t *BSP_branch_t::ensure_branch(BSP_tree_t *tree, BSP::SubspaceIndex index)
{
	if (!tree) throw std::invalid_argument("nullptr == tree");
	BSP::assertSubspaceIndexWithinBounds(__FILE__, __LINE__, index, children.lst_size);

	// If not child exists yet ...
	BSP_branch_t *child = children.lst[index];

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
			child->depth = depth + 1;
			_generate_BSP_aabb_child(bsp_bbox, index, child->bsp_bbox);

			// ... and insert it in the correct position.
			insert_branch(index, child);
		}
	}

	return child;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

BSP_tree_t::BSP_tree_t(const Parameters& parameters) : 
	finite(nullptr),
	infinite(),
	_used(nullptr), _nused(0),
	_free(nullptr), _nfree(0),
	_parameters(parameters),
	depth(0),
	_bounds(),
	bsp_bbox(parameters.getDim())
{
	// Load the singly-linked list of unused branches with preallocated branches.
	for (size_t index = 0; index < parameters.getMaxNodes(); ++index)
	{
		BSP_branch_t *branch = new BSP_branch_t(parameters.getDim());
		branch->_next = _free; _free = branch; _nfree++;
	}
}

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

	// Destroy all shells in the free list.
	while (_free)
	{
		Shell *shell = _free; _free = _free->_next; _nfree--;
		delete shell;
	}
}

BSP_tree_t::Parameters::Parameters(size_t dim, size_t maxDepth) :
	_maxChildNodes(1 << dim),
	_maxNodes(0),
	_dim(dim),
	_maxDepth(maxDepth)
{
	// Validate dimensionality.
	if (dim < BSP_tree_t::Parameters::ALLOWED_DIM_MIN)
	{
		log_error("%s: %d: specified dimensionality %" PRIuZ " is smaller than allowed minimum dimensionality %" PRIuZ "\n", \
			      __FILE__, __LINE__, dim, BSP_tree_t::Parameters::ALLOWED_DIM_MIN);
		throw std::domain_error("dimensionality out of range");
	}
	if (dim > BSP_tree_t::Parameters::ALLOWED_DEPTH_MAX)
	{
		log_error("%s:%d: specified dimensionality %" PRIuZ " is greater than allowed maximum dimensionality %" PRIuZ "\n", \
			__FILE__, __LINE__, dim, BSP_tree_t::Parameters::ALLOWED_DIM_MAX);
		throw std::domain_error("dimensionality out of range");
	}
	// Validate maximum depth.
	if (maxDepth > BSP_tree_t::Parameters::ALLOWED_DEPTH_MAX)
	{
		log_error("%s: %d: specified maximum depth %" PRIuZ " is greater than allowed maximum depth %" PRIuZ "\n", \
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
	_maxChildNodes = 1 << dim; // ~ 1 * 2^dim ~ 2^dim
}

BSP_tree_t::Parameters::Parameters(const Parameters& other) :
	_maxChildNodes(other._maxChildNodes),
	_maxNodes(other._maxNodes),
	_dim(other._dim),
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
	BSP_branch_t *branch = static_cast<BSP_branch_t *>(shell);
	if (!branch) throw std::runtime_error("invalid BSP tree");
	// The branch may not have a parent and must be empty.
	if (branch->parent || !branch->empty()) throw std::runtime_error("invalid BSP tree");
	return branch;
}


//--------------------------------------------------------------------------------------------
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
		const static bool ForceInfinite = true;
	}
}

size_t BSP_tree_t::prune()
{
	Shell **prev = &_used; 
	size_t count = 0;

	// Search through all used branches.
	// This will not catch all of the empty branches every time, but should catch quite a few.
	while (nullptr != *prev)
	{
		Shell *cur = *prev;
		BSP_branch_t *branch = static_cast<BSP_branch_t *>(cur);
		if (!branch) throw std::runtime_error("invalid BSP tree");
		if (branch == finite || !branch->empty()) // Do not remove root branches or non-empty branches.
		{
			prev = &cur->_next;
		}
		else
		{
			bool found = branch->unlink_parent();
			if (!found)
			{
				log_error("%s:%d: invalid BSP tree\n", __FILE__, __LINE__);
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


bool BSP_tree_t::insert_leaf(BSP_leaf_t *leaf)
{
	bool inserted;

	if (!leaf) return false;

	// If the leaf is NOT fully contained in the tree's bounding box ...
	if (!bsp_bbox.contains(leaf->_bbox.getAABB()) || BSP::Hacks::ForceInfinite)
	{
		// ... put the leaf at the head of the infinite list.
		inserted = infinite.push_front(*leaf);
		if (!inserted) throw std::runtime_error("capacity limit reached\n");
	}
	// Otherwise:
	else
	{
		BSP_branch_t *root = ensure_root();

		inserted = root->insert_leaf_rec(this, leaf, 0);
		if (!inserted)
		{
			// ... put the leaf at the head of the infinite list.
			inserted = infinite.push_front(*leaf);
			if (!inserted) throw std::runtime_error("capacity limit reached\n");
		}
	}

	// Make sure the bounding box matches the maximum extend of all leaves in the tree.
	_bounds.add(leaf->_bbox);
	return inserted;
}

void BSP_tree_t::collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	// Collide with any "infinite" nodes.
	infinite.collide(aabb, collisions);

	// Collide with the rest of the BSP tree.
	if (finite)
	{
		finite->collide(aabb, collisions);
	}
}

void BSP_tree_t::collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	// Collide with any "infinite" nodes.
	infinite.collide(aabb, test, collisions);

	// Collide with the rest of the BSP tree.
	if (finite)
	{
		finite->collide(aabb, test, collisions);
	}
}

void BSP_tree_t::collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	// Collide with any "infinite" nodes.
	infinite.collide(frustum,  collisions);

	// Collide with the rest of the BSP tree.
	if (finite)
	{
		finite->collide(frustum, collisions);
	}
}

void BSP_tree_t::collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
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
//--------------------------------------------------------------------------------------------

BSP_leaf_list_t::BSP_leaf_list_t() :
	count(0),
	lst(nullptr),
	_bounds()
{

}

BSP_leaf_list_t::~BSP_leaf_list_t()
{
	lst = nullptr;
	count = 0;
}

size_t BSP_leaf_list_t::removeAllLeaves()
{
	size_t oldCount = count;
	while (nullptr != lst)
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->_next;
		leaf->_next = nullptr;
		leaf->_inserted = false;
		count--;
	}
	_bounds.clear();
	return oldCount;
}

void BSP_leaf_list_t::clear()
{
	while (nullptr != lst)
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->_next;
		leaf->_next = nullptr;
		leaf->_inserted = false;
		count--;
	}
	_bounds.clear();
}

bool BSP_leaf_list_t::push_front(BSP_leaf_t& leaf)
{
	if (leaf.isInList())
	{
		log_warning("%s: %d: trying to insert a BSP_leaf that is claiming to be part of a list already\n",__FILE__,__LINE__);
		return false;
	}

	// Insert the leaf.
	leaf._next = lst; lst = &leaf;
	count++;
	leaf._inserted = true;
	// Add the leaf's bounding box to the bounds of this leaf list.
	_bounds.add(leaf._bbox);
	return true;
}

BSP_leaf_t *BSP_leaf_list_t::pop_front()
{
	if (nullptr == lst)
	{
		return nullptr;
	}
	else
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->_next;
		leaf->_next = nullptr;
		leaf->_inserted = false;
		count--;
		return leaf;
	}
}

bool BSP_leaf_list_t::add_all(Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	size_t lost_leaves = 0;

	BSP_leaf_t *leaf;
	size_t cnt;
	for (cnt = 0, leaf = lst; nullptr != leaf && cnt < count; leaf = leaf->_next, ++cnt)
	{
		if (rv_success != collisions.push_back(leaf))
		{
			lost_leaves++;
		}
	}

	// Warn if any leaves were rejected.
	if (lost_leaves > 0)
	{
		log_warning("%s:%d: %" PRIuZ " leaves not added.\n", __FILE__, __LINE__, lost_leaves);
	}

	return (collisions.size() < collisions.capacity());
}

bool BSP_leaf_list_t::add_all(BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	size_t lost_leaves = 0;     // The number of lost leaves.
	size_t rejected_leaves = 0; // The number of rejected leaves.
	for (BSP_leaf_t *leaf = lst; nullptr != leaf; leaf = leaf->_next)
	{
		if ((*test)(leaf))
		{
			if (rv_success != collisions.push_back(leaf))
			{
				lost_leaves++;
			}
		}
		else
		{
			rejected_leaves++;
		}
	}

	// Warn if any leaves were lost.
	if (lost_leaves > 0)
	{
		log_warning("%s:%d: %" PRIuZ " nodes not added.\n", __FILE__, __LINE__, lost_leaves);
	}

	return (collisions.size() < collisions.capacity());
}

void BSP_leaf_list_t::collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry = classify(aabb);

	// If the AABB does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (geometry <= geometry_outside)
	{
		return;
	}
	// The leaf list is completey contained in the AABB. Add every single leaf.
	else if (geometry_inside == geometry)
	{
		add_all(collisions);
		return;
	}
	// The leaf list intersects with but is not completey contained in the frustum.
	// Test every single leaf.
	else if (geometry_intersect == geometry)
	{
		size_t lost_leaves = 0;  // The number of lost leaves.

		// Iterate over the leaves in the leaf list.
		for (BSP_leaf_t *leaf = lst; nullptr != leaf; leaf = leaf->_next)
		{
			// The leaf must be valid.
			EGOBOO_ASSERT(leaf->valid());
			// The leaf must be in a leaf list.
			EGOBOO_ASSERT(leaf->isInList());

			// Test the geometry.
			geometry_rv geometry_test = aabb_intersects_aabb(aabb, leaf->_bbox.getAABB());

			// determine what action to take
			if (geometry_test > geometry_outside)
			{
				if (rv_success != collisions.push_back(leaf))
				{
					lost_leaves++;
				}
			}
		}

		// Warn if any leaves were lost.
		if (lost_leaves > 0)
		{
			log_warning("%s:%d: %" PRIuZ " leaves not lost\n", __FILE__, __LINE__, lost_leaves);
		}
		return;
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		return;
	}
}

void BSP_leaf_list_t::collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry = classify(aabb);

	// If the AABB does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (geometry <= geometry_outside)
	{
		return;
	}
	// The leaf list is completey contained in the AABB. Add every single leaf.
	else if (geometry_inside == geometry)
	{
		add_all(test,collisions);
		return;
	}
	// The leaf list intersects with but is not completey contained in the frustum.
	// Test every single leaf.
	else if (geometry_intersect == geometry)
	{
		size_t lost_leaves = 0;  // The number of lost leaves.
		size_t rejected_leaves = 0; // The number of rejected leaves;

		// Iterate over the leaves in the leaf list.
		for (BSP_leaf_t *leaf = lst; nullptr != leaf; leaf = leaf->_next)
		{
			// The leaf must be valid.
			EGOBOO_ASSERT(leaf->valid());
			// The leaf must be in a leaf list.
			EGOBOO_ASSERT(leaf->isInList());

			// Test geometry.
			geometry_rv geometry_test = aabb_intersects_aabb(aabb, leaf->_bbox.getAABB());

			// Determine what action to take.
			if (geometry_test > geometry_outside)
			{
				if ((*test)(leaf))
				{
					if (rv_success != collisions.push_back(leaf))
					{
						lost_leaves++;
					}
				}
				else
				{
					rejected_leaves++;
				}
			}
		}

		// Warn if any leaves were lost.
		if (lost_leaves > 0)
		{
			log_warning("%s:%d: %" PRIuZ " leaves lost\n", __FILE__, __LINE__, lost_leaves);
		}
		return;
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		return;
	}
}

void BSP_leaf_list_t::collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry = classify(frustum);

	// If the frustum does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (geometry <= geometry_outside)
	{
		return;
	}
	// The leaf list is completey contained in the frustum. Add every single leaf.
	else if (geometry_inside == geometry)
	{
		add_all(collisions);
		return;
	}
	// The leaf list intersects with but is not completey contained in the frustum.
	// Test every single leaf.
	else if (geometry_intersect == geometry)
	{
		size_t lost_leaves = 0; // The number of lost leaves.

		// Scan through every leaf.
		for (BSP_leaf_t *leaf = lst; nullptr != leaf; leaf = leaf->_next)
		{
			// The leaf must be valid.
			EGOBOO_ASSERT(leaf->valid());
			// The leaf must be in a leaf list.
			EGOBOO_ASSERT(leaf->isInList());

			// Test the geometry
			geometry_rv geometry_test = frustum.intersects_bv(&(leaf->_bbox), true);

			// Determine what action to take.
			if (geometry_test > geometry_outside)
			{
				if (rv_success != collisions.push_back(leaf))
				{
					lost_leaves++;
				}
			}
		}

		// Warn if any nodes were lost.
		if (lost_leaves > 0)
		{
			log_warning("%s:%d: %" PRIuZ " leaves lost\n", __FILE__, __LINE__, lost_leaves);
		}
		return;
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		return;
	}
}

void BSP_leaf_list_t::collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry = classify(frustum);

	// If the frustum does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (geometry <= geometry_outside)
	{
		return;
	}
	// The leaf list is completey contained in the frustum. Add every single leaf.
	else if (geometry_inside == geometry)
	{
		add_all(test, collisions);
		return;
	}
	// The leaf list intersects with the frustum but is not completey contained in the frustum.
	// Test every single leaf.
	else if (geometry_intersect == geometry)
	{
		size_t rejected_leaves = 0; // The number of rejected leaves.
		size_t lost_leaves = 0; // The number of lost leaves.
		// Scan through every leaf.
		for (BSP_leaf_t *leaf = lst; nullptr != leaf; leaf = leaf->_next)
		{
			// The leaf must be valid.
			EGOBOO_ASSERT(leaf->valid());
			// The leaf must be in a leaf list.
			EGOBOO_ASSERT(leaf->isInList());

			// Test the geometry
			geometry_rv geometry_test = frustum.intersects_bv(&(leaf->_bbox), true);

			// Determine what action to take.
			if (geometry_test > geometry_outside)
			{
				if ((*test)(leaf))
				{
					if (rv_success != collisions.push_back(leaf))
					{
						lost_leaves++;
					}
				}
				else
				{
					rejected_leaves++;
				}
			}
		}

		// Warn if any nodes were lost.
		if (lost_leaves > 0)
		{
			log_warning("%s:%d: %" PRIuZ " leaves lost\n", __FILE__, __LINE__, lost_leaves);
		}
		return;
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		return;
	}
}

geometry_rv BSP_leaf_list_t::classify(const aabb_t& aabb) const
{
	if (0 == getCount())
	{
		return geometry_outside;
	}
	else if (1 == getCount())
	{
		// A speedup.
		// If there is only one entry, then the frustum-aabb collision test is
		// called twice for 2 bounding boxes of the same size.
		return geometry_intersect;
	}
	else
	{
		return _bounds.intersects(aabb);
	}
}

geometry_rv BSP_leaf_list_t::classify(const egolib_frustum_t& frustum) const
{
	if (0 == getCount())
	{
		return geometry_outside;
	}
	else if (1 == getCount())
	{
		// A speedup.
		// If there is only one entry, then the frustum-aabb collision test is
		// called twice for 2 bounding boxes of the same size.
		return geometry_intersect;
	}
	else
	{
		return _bounds.intersects(frustum);
	}
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

BSP_branch_list_t::BSP_branch_list_t(size_t dim) :
	lst_size(0),
	lst(nullptr),
	inserted(0),
	_bounds()
{
	// Determine the number of children from the dimensionality.
	size_t child_count = BSP::childCount(dim);
	this->lst = new BSP_branch_t *[child_count]();
	this->inserted = 0;
	this->lst_size = child_count;
}

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

void BSP_branch_list_t::collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry;

	if (0 == inserted)
	{
		geometry = geometry_outside;
	}
	else if (1 == inserted)
	{
		// A speedup.
		// If there is only one entry, then the AABB-AABB collision test is
		// called twice for 2 equivalent AABBs.
		geometry = geometry_intersect;
	}
	else
	{
		geometry = _bounds.intersects(aabb);
	}

	if (geometry <= geometry_outside)
	{
		// The branch list and the AABB do not overlap at all. Do nothing.
		return;
	}
	if (geometry_inside == geometry)
	{
		add_all_children(collisions);
	}
	else if (geometry_intersect == geometry)
	{
		for (size_t i = 0, n = lst_size; i < n; ++i)
		{
			BSP_branch_t *child = lst[i];
			if (!child) continue;
			child->collide(aabb, collisions);
		}
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
}

void BSP_branch_list_t::collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry;

	if (0 == inserted)
	{
		geometry = geometry_outside;
	}
	else if (1 == inserted)
	{
		// A speedup.
		// If there is only one entry, then the AABB-AABB collision test is
		// called twice for 2 equivalent AABBs.
		geometry = geometry_intersect;
	}
	else
	{
		geometry = _bounds.intersects(aabb);
	}

	if (geometry <= geometry_outside)
	{
		// The branch list and the AABB do not overlap at all. Do nothing.
		return;
	}

	if (geometry_inside == geometry)
	{
		add_all_children(test,collisions);
	}
	else if (geometry_intersect == geometry)
	{
		for (size_t i = 0, n = lst_size; i < n; ++i)
		{
			BSP_branch_t *child = lst[i];
			if (!child) continue;
			child->collide(aabb, test, collisions);
		}
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
}

void BSP_branch_list_t::collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry;

	if (0 == inserted)
	{
		geometry = geometry_outside;
	}
	else if (1 == inserted)
	{
		// A speedup.
		// If there is only one entry, then the frustum-AABB collision test is
		// called twice for 2 equivalent AABBs.
		geometry = geometry_intersect;
	}
	else
	{
		geometry = _bounds.intersects(frustum);
	}

	if (geometry <= geometry_outside)
	{
		// The branch list and the frustum do not overlap at all. Do nothing.
		return;
	}

	if (geometry_inside == geometry)
	{
		add_all_children(collisions);
	}
	else if (geometry_intersect == geometry)
	{
		for (size_t i = 0, n = lst_size; i < n; ++i)
		{
			BSP_branch_t *child = lst[i];
			if (!child) continue;
			child->collide(frustum, collisions);
		}
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
}

void BSP_branch_list_t::collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const
{
	geometry_rv geometry;

	if (0 == inserted)
	{
		geometry = geometry_outside;
	}
	else if (1 == inserted)
	{
		// A speedup.
		// If there is only one entry, then the frustum-AABB collision test is
		// called twice for 2 equivalent AABBs.
		geometry = geometry_intersect;
	}
	else
	{
		geometry = _bounds.intersects(frustum);
	}

	if (geometry <= geometry_outside)
	{
		// The branch list and the frustum do not overlap at all. Do nothing.
		return;
	}

	if (geometry_inside == geometry)
	{
		add_all_children(test, collisions);
	}
	else if (geometry_intersect == geometry)
	{
		for (size_t i = 0, n = lst_size; i < n; ++i)
		{
			BSP_branch_t *child = lst[i];
			if (!child) continue;
			child->collide(frustum, test, collisions);
		}
	}
	else
	{
		log_error("%s:%d: unreachable code reached\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
}
