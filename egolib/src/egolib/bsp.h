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
	BSP_leaf_list_t() :
		count(0),
		lst(nullptr),
		bbox()
	{

	}

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
	 * @param self
	 *	this leaf list
	 */
	BSP_leaf_list_t *ctor();
	/**
	 * @brief
	 *	Destruct this leaf list.
	 * @param self
	 *	this leaf list
	 */
	void dtor();
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

	BSP_branch_list_t() :
		lst_size(0),
		lst(nullptr),
		inserted(0),
		bbox()
	{
		//ctor
	}

	BSP_branch_list_t *ctor(size_t dim);
	BSP_branch_list_t *dtor();

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

	static bool clear_rec(BSP_branch_list_t * self);
	bool collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
	bool collide(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;
};

//--------------------------------------------------------------------------------------------
/// Element of a shotgun allocator (simplified).
class Shell {
public:
	/// The next element in the singly-linked list of shells.
	/// There are two lists: Used and unused.
	Shell *_next;
	Shell() : _next(nullptr) { }
};
class BSP_branch_t : public Shell
{
public:
	BSP_branch_t() :
		parent(nullptr),
		unsorted(),
		children(),
		leaves(),
		bsp_bbox(),
		depth(0),
		Shell()
	{
		//ctor
	}
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

	/**
	 * @brief
	 *	Create a branch.
	 * @param dimensionality
	 *	the dimensionality of this branch
	 * @return
	 *	a pointer to the branch on success, @a nullptr on failure
	 */
	static BSP_branch_t *create(size_t dimensionality);
	/**
	 * @brief
	 *	Destroy a branch.
	 * @param self
	 *	the branch
	 */
	static void destroy(BSP_branch_t *branch);

protected:
	/**
	 * @brief
	 *	Construct this branch.
	 * @param dimensionality
	 *	the dimensionality of this branch
	 * @return
	 *	a pointer to this branch on success, @a nullptr on failure
	 */
	BSP_branch_t *ctor(size_t dimensionality);
	
	/**
	 * @brief
	 *	Destruct this branch.
	 */
	void dtor();
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
	bool empty() const;

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

	static BSP_branch_t *ensure_branch(BSP_branch_t *self, BSP_tree_t *tree, size_t index);

	static bool add_all_rec(const BSP_branch_t *self, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions);
	static bool add_all_children(const BSP_branch_t *self, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions);

};

/**
 * @brief
 *	Clear this branch.
 * @param self
 *	this branch
 * @param recursive
 *	if @a true, recursively clear this branch
 */
bool BSP_branch_clear(BSP_branch_t *self, bool recursive);
/**
 * @brief
 *	Remove all children of this branch.
 * @param self
 *	this branch
 * @param recursive
 *	if @a true, recursively remove all children
 */
bool BSP_branch_free_nodes(BSP_branch_t *self, bool recursive);

bool BSP_branch_update_depth_rec(BSP_branch_t *self, int depth);

bool BSP_branch_insert_leaf_rec(BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth);
bool BSP_branch_insert_branch_list_rec(BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int index, int depth);
bool BSP_branch_insert_leaf_list(BSP_branch_t * B, BSP_leaf_t * n);
bool BSP_branch_insert_branch(BSP_branch_t * B, size_t index, BSP_branch_t * B2);
int BSP_branch_insert_leaf_rec_1(BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth);


#if 0
/**
 * @brief
 *	The interface of a (limited) repository of branches.
 */
class BranchRepository
{
public:
	/**
	 * @brief
	 *	Acquire a branch from this repository.
	 * @return
	 *	a pointer to the branch on success, @a nullptr on failure
	 */
	BSP_branch_t *acquire();
	/**
	 * @brief
	 *	Relinquish a branch to this repository.
	 * @param branch
	 *	a pointer to the branch
	 */
	void relinquish(BSP_branch_t *branch);
};
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/**
 * @brief
 *	A BSP tree.
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
		 *	The number of branches (aka nodes) in the BSP tree.
		 *	\[
		 *	\frac{(dimensionality^(maximumDepth + 1) - 1}{dimensionality - 1}
		 *	\]
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

	BSP_tree_t();
#if 0
	/**
	 * @brief
	 *    The minimum dimensionality of a BSP tree.
	 *    ("binary" should already suggest that this is the minimum dimensionality).
	 */
	static const size_t DIM_MIN = 2;
	/**
	 * @brief
	 *	The maximum depth of a BSP tree.
	 */
	static const size_t DEPTH_MAX = SIZE_MAX - 1;
#endif

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
	 *  Construct a BSP tree.
	 * @param parameters
	 *	the parameters for creating the BSP tree
	 * @return
	 *	the BSP tree on success, @a nullptr on failure
	 */
	BSP_tree_t *ctor(const Parameters& parameters);
	/**
	 * @brief
	 *     Destruct a BSP tree.
	 * @param self
	 *    the BSP tree
	 */
	void dtor();

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

	/**
	 * @brief
	 *	Prune non-root, non-empty branches of this BSP tree.
	 * @return
	 *	the number of branches pruned
	 */
	size_t prune();

	static bool insert_leaf(BSP_tree_t *self, BSP_leaf_t *leaf);


	void clear_rec();
};


