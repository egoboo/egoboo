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

/// @file egolib/bsp.h
/// @details

#pragma once

#include "egolib/DynamicArray.hpp"
#include "egolib/frustum.h"
#include "egolib/bv.h"
#include "egolib/platform.h"
#include "egolib/bsp_aabb.h"

//--------------------------------------------------------------------------------------------
// forward declarations
//--------------------------------------------------------------------------------------------
class BSP_aabb_t;
class BSP_leaf_t;
class BSP_branch_t;
class BSP_branch_list_t;
class BSP_leaf_list_t;
class BSP_tree_t;

//--------------------------------------------------------------------------------------------
// BSP types
//--------------------------------------------------------------------------------------------

    typedef bool ( BSP_leaf_test_t )( BSP_leaf_t * );

//--------------------------------------------------------------------------------------------
// known BSP types
//--------------------------------------------------------------------------------------------
enum bsp_type_t
{
    BSP_LEAF_NONE = -1,
    BSP_LEAF_CHR,
    BSP_LEAF_ENC,
    BSP_LEAF_PRT,
    BSP_LEAF_TILE
};

//--------------------------------------------------------------------------------------------
class BSP_leaf_t
{
public:
	BSP_leaf_t() :
		next(nullptr),
		data_type(BSP_LEAF_NONE),
		data(nullptr),
		index(0),
		bbox(),
		inserted(false)
	{
		//ctor
	}

    BSP_leaf_t *next;
    bsp_type_t data_type;
    void *data;
    size_t index;
    bv_t bbox;

	/**
	 * @brief
	 *	Is this leaf in a leaf list.
	 * @return
	 *	@a true if this leaf is in a leaf list, @a false otherwise
	 */
	bool isInList() const
	{
		return inserted;
	}

	BSP_leaf_t *ctor(void *data, bsp_type_t type, size_t index);
	void dtor();

	static bool clear(BSP_leaf_t * L);
	static bool remove_link(BSP_leaf_t * L);
	bool assign(const BSP_leaf_t& other);

protected:
	bool inserted;

	friend class BSP_leaf_list_t;
};

//inline
bool BSP_leaf_valid(const BSP_leaf_t *self);


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
class BSP_leaf_list_t
{
public:
    size_t count;

    BSP_leaf_t *lst;

    bv_t bbox;

	/**
	 * @brief
	 *	Get if this leaf list is empty.
	 * @return
	 *	@a true if this leaf list is empty, @a false otherwise
	 */
	bool empty() const
	{
		return 0 == count;
	}
	/**
	 * @brief
	 *	Construct this leaf list.
	 */
	BSP_leaf_list_t();
	/**
	 * @brief
	 *	Destruct this leaf list.
	 */
	~BSP_leaf_list_t();
	/**
	 * @brief
	 *	Insert a leaf in the list.
	 * @pre
	 *	The leaf must not be in a leaf list (checked).
	 * @post
	 *	The leaf is in this leaf list.
	 */
	bool push_front(BSP_leaf_t *leaf);

	/**
	 * @brief
	 *	Pop the head of this leaf list.
	 * @return
	 *	the head of the leaf list if the leaf list is not empty, @a nullptr otherwise
	 * @warning
	 *	This function does not update the bounding box of this leaf list.
	 */
	BSP_leaf_t *pop_front();

	/**
	 * @brief
	 *	Clear the leaf list.
	 * @warning
	 *	This function does update the bounding box of this leaf list.
	 */
	void clear();

	/**
	 * @brief
	 *	Add all leaves in this leaf list.
	 * @param collisions
	 *	a leave list to which the leaves are added to
	 */
	bool add_all(Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
	/**
	 * @brief
	 *	Add all leaves in this leaf list (filtered).
	 * @param test
	 *	a test each leave must pass before it is added to the collisions list
	 * @param collisions
	 *	a leave list to which the leaves are added to (if they pass the test)
	 */
	bool add_all(BSP_leaf_test_t& test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
	bool collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const;
	bool collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
};

//--------------------------------------------------------------------------------------------
class BSP_branch_list_t
{
public:

	BSP_branch_list_t(size_t dim);
	~BSP_branch_list_t();

	/**
	 * @brief
	 *	The number of elements in the array pointed to by @a list.
	 */
    size_t lst_size;
    BSP_branch_t **lst;
	/**
	 * @brief
	 *	The number of non-null pointer elements in the array pointed to by @a lst.
	 */
    size_t inserted;
    bv_t bbox;

	bool clear_rec();
	bool collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
	bool collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
};

//--------------------------------------------------------------------------------------------
/// Element of a shotgun allocator (simplified).
class Shell
{
public:
	/// The next element in the singly-linked list of shells.
	/// There are two lists: Used and unused.
	Shell *_next;
	Shell() : _next(nullptr) { }
};
class BSP_branch_t : public Shell
{
public:
	/**
	 * @brief
	 *	The parent branch of this branch.
	 */
    BSP_branch_t *parent;

	/**
	 * @brief
	 *	A list of leaves that have not yet been sorted.
	 */
    BSP_leaf_list_t unsorted;
    BSP_branch_list_t children; ///< the child branches of this branch
    BSP_leaf_list_t leaves;     ///< the leaves at this level

    BSP_aabb_t bsp_bbox;        //< the size of the node
	/**
	 * @brief
	 *	The depth of this branch.
	 *	The root branch has a depth of @a 0.
	 */
    int depth;

public:
	/**
	 * @brief
	 *	Construct this branch.
	 * @param dim
	 *	the dimensionality of this branch
	 * @return
	 *	a pointer to this branch on success, @a nullptr on failure
	 */
	BSP_branch_t(size_t dim);
	
	/**
	 * @brief
	 *	Destruct this branch.
	 */
	~BSP_branch_t();
public:
	/**
	 * @brief
	 *	Get if this branch is empty.
	 * @return
	 *	@a true if this branch is empty, @a false otherwise
	 * @remark
	 *	A branch is considered as empty if
	 *	- it has no leaves,
	 *	- no unsorted leaves and
	 *	- no child branches.
	 */
	bool isEmpty() const;

	/**
	 * @brief
	 *	Recursively search the BSP tree for collisions with an AABB.
	 * @return
	 *	@a false if we need to break out of the recursive search for any reason.
	 */
	bool collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
	
	/**
	 * @brief
	 *	Recursively search the BSP tree for collisions with a frustum.
	 * @return
	 *	@a false if we need to break out of the recursive search for any reason.
	 */
	bool collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;

	/**
	 * @brief
	 *	Unlink any children and leaves (from this branch only, NOT recursively);
	 *	unlink this branch from its parent and the parent from this branch.
	 */
	bool unlink_all();

	/**
	 * @brief
	 *	Unlink this branch from its parent and the parent from this branch.
	 * @remark
	 *	If this branch has no parent, a call to this function is a no-op.
	 */
	bool unlink_parent();
	
	/**
	 * @brief
	 *	Unlink any children (from this branch only, NOT recursively).
	 */
	bool unlink_children();
	
	/**
	 * @brief
	 *	Unlink any leaves (from this branch only, NOT recursively).
	 */
	bool unlink_leaves();

	/**
	 * @brief
	 *	Clear this branch.
	 * @param recursive
	 *	if @a true, recursively clear this branch
	 */
	bool clear(bool recursive);

	

	static bool add_all_rec(const BSP_branch_t *self, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions);
	static bool add_all_children(const BSP_branch_t *self, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions);

	/**
	 * @brief
	 *	Recursively insert a leaf in branch.
	 * @remark
	 *	Get new branches using BSP_tree_t::createBranch on @a tree if required.
	 * @remark
	 *	This function attempts to prevent overflowing the "unsorted" list.
	 */
	static bool insert_leaf_rec(BSP_branch_t *self, BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth);
protected:
	static BSP_branch_t *ensure_branch(BSP_branch_t *self, BSP_tree_t *tree, size_t index);
	/**
	 * @brief
	 *	Recursively insert a leaf in branch.
	 * @remark
	 *	Get new branches using BSP_tree_t::createBranch() on @a tree if required.
	 * @return
	 *	@a -1 if an error occured. @a 0 if the leaf was inserted into this branch
	 *	and @a 1 if the leaf was (scheduled to be) inserted into a child branch of
	 *	this branch.
	 */
	int insert_leaf_rec_1(BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth);
	/**
	 * @brief
	 *	Recursively insert a leaf in branch list.
	 * @remark
	 *	Get new branches using BSP_tree_t::createBranch() on @a tree if required.
	 * @todo
	 *	@a index should be of type size_t.
	 * @todo
	 *	Move most of this code into BSP_branch_list_t.
	 */
	static bool insert_branch_list_rec(BSP_branch_t *self, BSP_tree_t *tree, BSP_leaf_t *leaf, int index, size_t depth);

	/**
	 * @brief
	 *	Insert a leaf (unconditionally) into the "leaves" list of this branch.
	 * @param leaf
	 *	the leaf
	 * @return
	 *	@a true if the leaf was inserted, @a false otherwise
	 */
	bool insert_leaf_list(BSP_leaf_t *leaf);

};


/**
 * @brief
 *	Remove all children of this branch.
 * @param self
 *	this branch
 * @param recursive
 *	if @a true, recursively remove all children
 */
bool BSP_branch_free_nodes(BSP_branch_t *self, bool recursive);

/// @todo
/// @a depth should be of type @a size_t.
bool BSP_branch_update_depth_rec(BSP_branch_t *self, size_t depth);


bool BSP_branch_insert_branch(BSP_branch_t *self, size_t index, BSP_branch_t *branch);


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/**
 * @brief
 *	A space partitioning tree (SPT).
 * @todo
 *	Rename to SPT.
 */
class BSP_tree_t
{
public:
	/**
	 * @brief
	 *	The parameters for creating a BSP tree.
	 */
	class Parameters
	{
	public:
		/**
		 * @brief
		 * The allowed minimum dimensionality of a BSP tree.
		 * ("binary" should already suggest that this is the minimum dimensionality).
		 */
		static const size_t ALLOWED_DIM_MIN = 2;
		/**
		 * @brief
		 *	The allowed maximum dimensionality of a BSP tree.
		 */
		static const size_t ALLOWED_DIM_MAX = 8;
		/**
		 * @brief
		 *	The allowed maximum depth of a BSP tree.
		 */
		static const size_t ALLOWED_DEPTH_MAX = SIZE_MAX - 1;
		/**
		 * @brief
		 *	Create parameters for a BSP tree
		 * @param dim
		 *	the desired dimensionality of the BSP tree
		 * @param maxDepth
		 *	the desired maximum depth of the BSP tree
		 * @throw std::domain_error
		 *	if the desired dimensionality is smaller than the allowed minimum dimensionality
		 *	or greater than the allowed maximum dimensionality
		 * @throw std::domain_error
		 *	if the desired maximum depth is greater than the allowed maximum depth
		 * @remark
		 *	The maximum number of nodes is computed by
		 */
		Parameters(size_t dim, size_t maxDepth);
	public:
		/**
		 * @brief
		 *	Each node has an \f$d\f$-dimensional bounding box. A point is tested
		 *	against each dimension of the bounding box i.e. \f$n\f$ tests are
		 *	performed. The outcome of single test is either that the node goes
		 *	into one branch for being on the "right" or the "left side".
		 *	Subsequently, there must be \f$2^d\f$ branches fo each node.
		 *	That is, we have a \f$2^d\f$-ary tree. By the general rule
		 *	for the number of nodes for a complete \f$k\f$-ary tree of height \f$h\f$
		 *	\f[
		 *	n = \frac{k^{h+1}}{k-1}
		 *	\f]
		 *	we obtain for a \f$2^d\f$-ary tree a number of nodes of
		 *	\f[
		 *	n =& \frac{(2^d)^{h+1}}{2^d - 1}\\
		 *	  =& \frac{2^{d \cdot (h+1)}{2^d - 1}
		 *	\f]
		 *	\f$n\f$ is called here the number of branches.
		 */
		size_t _numBranches;
		/**
		 * @brief
		 *	The dimensionality of the BSP tree.
		 */
		size_t _dim;
		/**
		 * @brief
		 *	The maximum depth of the BSP tree.
		 */
		size_t _maxDepth;
	};

	/** 
	 * @brief
	 *  Construct this BSP tree.
	 * @param parameters
	 *	the parameters for creating the BSP tree
	 */
	BSP_tree_t(const Parameters& parameters);
	/**
	 * @brief
	 *	Destruct this BSP tree.
	 */
	virtual ~BSP_tree_t();

	/**
	 * @brief
	 *    The number of dimensions used by this BSP tree.
	 */
	size_t dimensions;
	/**
	 * @brief
	 *    The maximum depth this BSP tree supports.
	 */
	size_t max_depth;

	/**
	 * @brief
	 *    The maximum depth of this BSP tree actually has.
	 * @remark
	 *    The maximum depth (of a BSP tree) is the maximum of the lengths of its branches.
	 * @todo
	 *    Should be of type @a size_t.
	 */
	int depth;

	Shell *_used; ///< A singly-linked list of used branches.
	                     ///< The branches are chained up using their "Shell::_next" pointer.
	size_t _nused;
	Shell *_free; ///< A singly-linked list of free branches.
	                     ///< The banches are chained up using their "Shell::_next" pointer.
	size_t _nfree;

    BSP_branch_t *finite;      ///< the root node of the ordinary BSP tree
    BSP_leaf_list_t infinite;  ///< all nodes which do not fit inside the BSP tree

    bv_t bbox;           ///< the actual size of everything in the tree
    BSP_aabb_t bsp_bbox; ///< the root-size of the tree

	/**
	 * @brief
	 *	Fill the collision list with references to objects that the AABB may overlap.
	 * @param aabb
	 *	the AABB
	 * @param test
	 *	a leaf test to filter the leaves
	 * @param collisions
	 *	the collision list
	 * @return
	 *	the new number of leaves in @a collisions
	 */
	size_t collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
	
	/**
	 * @brief
	 *	Fill the collision list with references to leaves that the frustum may overlap.
	 * @param frustum
	 *	the frustum
	 * @param test
	 *	a leaf test to filter the leaves
	 * @param collisions
	 *	the collision list
	 * @return
	 *	the new number of leaves in @a collisions
	 */
	size_t collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;

	/**
	 * @brief
	 *	Prune non-root, non-empty branches of this BSP tree.
	 * @return
	 *	the number of branches pruned
	 */
	size_t prune();

	bool insert_leaf(BSP_leaf_t *leaf);


	void clear_rec();

protected:
	friend class BSP_branch_t; ///< To grant access to BSP_tree_t::createBranch().
	/**
	* @brief
	*	Ensure that the root node exists.
	* @return
	*	the root node on success, @a nullptr on failure
	* @remark
	*	The root node has a depth of @a 0.
	*/
	BSP_branch_t *ensure_root();

	/// @brief Create a branch.
	/// @return the free branch on success, @a nullptr on failure
	/// @post
	///	A branch returned by this function is empty, has no parent, and has a depth of @a 0.
	BSP_branch_t *createBranch();
};
