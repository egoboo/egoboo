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
#include "egolib/Scene/Bounds.hpp"
#include "egolib/Scene/LeafHolder.hpp"

//--------------------------------------------------------------------------------------------
// forward declarations
//--------------------------------------------------------------------------------------------
class BSP_aabb_t;
class BSP_leaf_t;
class BSP_branch_t;
class BSP_branch_list_t;
class BSP_leaf_list_t;
class BSP_tree_t;

namespace BSP
{
	/**
	 * @brief
	 *	Indicates the relation between an entity and a branch.
	 *	If -2, the entity is larger than the bounds of the branch and if -1 the entity belongs to this branch.
	 *	Positive values range from 0 to 2^d-1 where d is the dimensionality and indicate the index of the
	 *	child branchat which the entity must be deferred to.
	 */
	typedef int SubspaceIndex;

}

//--------------------------------------------------------------------------------------------
// BSP types
//--------------------------------------------------------------------------------------------



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


	bool assign(const BSP_leaf_t& other);

	static bool clear(BSP_leaf_t *self);
	static bool remove_link(BSP_leaf_t *self);
	bool valid() const;


protected:
	bool inserted;

	friend class BSP_leaf_list_t;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
class BSP_leaf_list_t : public BSP::Collider, BSP::LeafHolder, Ego::Core::NonCopyable
{
protected:

    size_t count;

public:

	BSP_leaf_t *lst;

	/**
	 * @brief
	 *	The bounds of everything in this leaf list.
	 */
	BSP::Bounds<bv_t> _bounds;

	/**
	* @brief
	*	Get the size, in leaves, of this leaf list.
	* @return
	*	the size, in leaves, of this leaf list.
	*/
	size_t getCount() const
	{
		return count;
	}

	/**
	 * @brief
	 *	Get if this leaf list is empty.
	 * @return
	 *	@a true if this leaf list is empty, @a false otherwise
	 */
	bool empty() const
	{
		return 0 == getCount();
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
	bool push_front(BSP_leaf_t& leaf);

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
	bool add_all(Ego::DynamicArray<BSP_leaf_t *>& collisions) const;
	/**
	 * @brief
	 *	Add all leaves in this leaf list (filtered).
	 * @param test
	 *	a test each leave must pass before it is added to the collisions list
	 * @param collisions
	 *	a leave list to which the leaves are added to (if they pass the test)
	 */
	bool add_all(BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const;

	// Override
	size_t removeAllLeaves() override;

	// Override
	void collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;
	
	// Override
	void collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

public:
	/**
	 * @brief
	 *	Classify this leaf list w.r.t. to its bounding volumen and an AABB.
	 * @param aabb
	 *	the AABB
	 * @return
	 *	the classification of this leaf list
	 * @todo
	 *	Make protected.
	 */
	geometry_rv classify(const aabb_t& aabb) const;
	/**
	 * @brief
	 *	Classify this leaf list w.r.t. to its bounding volumen and a frustum.
	 * @param frustum
	 *	the frustum
	 * @return
	 *	the classification of this leaf list
	 * @todo
	 *	Make protected.
	 */
	geometry_rv classify(const egolib_frustum_t& frustum) const;
};

//--------------------------------------------------------------------------------------------
/// A branch list in a BSP tree.
class BSP_branch_list_t : public BSP::Collider, BSP::LeafHolder
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
	/**
	 * @brief
	 *	The bounds of everything this branch list.
	 */
    BSP::Bounds<bv_t> _bounds;

	bool clear_rec();

	// Override
	size_t removeAllLeaves() override;

	// Override
	void collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	/**
	* @brief
	*	Add all leaves from branches of this branch list.
	* @param collisions
	*	traversal parameters
	*/
	bool add_all_children(Ego::DynamicArray<BSP_leaf_t *>& collisions) const;
	/**
	* @brief
	*	Add all leaves from branches of this branch list.
	* @param test, collisions
	*	traversal parameters
	*/
	bool add_all_children(BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const;

    //Disable copying class
    BSP_branch_list_t(const BSP_branch_list_t& copy) = delete;
    BSP_branch_list_t& operator=(const BSP_branch_list_t&) = delete;

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Element of a shotgun allocator (simplified).
class Shell
{
public:
	
	/// The next element in the singly-linked list of shells.
	/// There are two lists: Used and unused.
	Shell *_next;
	
	Shell() : _next(nullptr)
	{
	}

	virtual ~Shell()
	{
	}


    //Disable copying class
    Shell(const Shell& copy) = delete;
    Shell& operator=(const Shell&) = delete;
};

//--------------------------------------------------------------------------------------------
//// A branch in a BSP tree.
class BSP_branch_t : public Shell, BSP::Collider, BSP::LeafHolder
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
	 * @remark
	 *	The root branch has a depth of @a 0.
	 */
    size_t depth;

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

    //Disable copying class
    BSP_branch_t(const BSP_branch_t& copy) = delete;
    BSP_branch_t& operator=(const BSP_branch_t&) = delete;

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

	// Override
	size_t removeAllLeaves() override;

	// Override
	void collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;
	
	// Override
	void collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

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

protected:
	friend class BSP_branch_list_t;
	/**
	 * @brief
	 *	Add all leaves from this branch and from the child branches of this branch.
	 * @param test, collisions
	 *	traversal parameters
	 */
	bool add_all_rec(Ego::DynamicArray<BSP_leaf_t *>& collisions) const;
	bool add_all_rec(BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const;

public:
	/**
	 * @brief
	 *	Recursively insert a leaf in branch.
	 * @remark
	 *	Get new branches using BSP_tree_t::createBranch on @a tree if required.
	 * @remark
	 *	This function attempts to prevent overflowing the "unsorted" list.
	 */
	bool insert_leaf_rec(BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth);
protected:
	/**
	 * @brief
	 *	This will ensure that a branch exists in the branch list of this branch
	 *	that has a slightly smaller BSPAABB than this branch. This effectively
	 *	constructs the tree.
	 */
	BSP_branch_t *ensure_branch(BSP_tree_t *tree, BSP::SubspaceIndex index);
	/**
	 * @brief
	 *	Recursively insert a leaf in branch.
	 * @remark
	 *	Get new branches using BSP_tree_t::createBranch() on @a tree if required.
	 * @return
	 *	@a true if the leaf was inserted somewhere, @a false otherwise
	 */
	bool insert_leaf_rec_1(BSP_tree_t *tree, BSP_leaf_t *leaf, size_t depth);
	/**
	 * @brief
	 *	Recursively insert a leaf in branch list.
	 * @remark
	 *	Get new branches using BSP_tree_t::createBranch() on @a tree if required.
	 * @todo
	 *	Move most of this code into BSP_branch_list_t.
	 */
	bool insert_branch_list_rec(BSP_tree_t *tree, BSP_leaf_t *leaf, BSP::SubspaceIndex index, size_t depth);

	/**
	 * @brief
	 *	Insert a leaf (unconditionally) into the "leaves" list of this branch.
	 * @param leaf
	 *	the leaf
	 * @return
	 *	@a true if the leaf was inserted, @a false otherwise
	 */
	bool insert_leaf_list(BSP_leaf_t *leaf);

	/**
	 * @brief
	 *	Insert branch into branch list.
	 * @param index
	 *	the sub-space index where to insert the branch
	 * @param branch
	 *	the branch
	 */
	bool insert_branch(BSP::SubspaceIndex index, BSP_branch_t *branch);

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

///
bool BSP_branch_update_depth_rec(BSP_branch_t *self, size_t depth);



/**
 * @brief
 *	A space partitioning tree (SPT).
 * @todo
 *	Rename to SPT.
 */
class BSP_tree_t : public BSP::Collider, BSP::LeafHolder
{

public:

	/**
	 * @brief
	 *	The parameters of a tree.
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
        static const size_t ALLOWED_DEPTH_MAX;

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

		/**
		 * @brief
		 *	Copy constructor.
		 * @param other
		 *	the source
		 */
		Parameters(const Parameters& other);
		
		/**
		 * @brief
		 *	Assignment operator.
		 * @param other
		 *	the source
		 * @return
		 *	the target
		 */
		Parameters& operator=(const Parameters& other)
		{
			_maxChildNodes = other._maxChildNodes;
			_maxNodes = other._maxNodes;
			_dim = other._dim;
			_maxDepth = other._maxDepth;
			return *this;
		}
		
		/// @brief Get the maximum number of child nodes (of a node).
		/// @return the maximum number of child nodes
		size_t getMaxChildNodes() const
		{
			return _maxChildNodes;
		}

		/// @brief Get the maximum number of nodes (of a tree).
		/// @return the maximum number of nodes
		size_t getMaxNodes() const
		{
			return _maxNodes;
		}

		/// @brief Get the maximum depth (of a tree).
		/// @return the maximum depth
		size_t getMaxDepth() const
		{
			return _maxDepth;
		}

		/// @brief Get the dimensionality (of a tree).
		/// @return the dimensionality
		size_t getDim() const
		{
			return _dim;
		}

	private:

		/**
		 * @brief
		 *	The maximum number of child nodes (of a node).
		 * @remark
		 *	Each node may have at most \f$2^d\f$ child nodes
		 *	where \f$d\f$ is the dimensionality.
		 */
		size_t _maxChildNodes;

		/**
		 * @brief
		 *	The maximum number of nodes (of a tree).
		 * @remark
		 *	Each node may have at most \f$2^d\f$ child nodes (see above) where
		 *	\f$d\f$ is the dimensionality. That is, a tree is a \f$2^d\f$-ary
		 *	tree. By the general rule 
         *	\f{eqnarray*}{
         *	n = \frac{k^{h+1}}{k-1}
         *	\f}
         *  for the number of branches for a complete \f$k\f$-ary tree of height
         *  \f$h\f$ one obtains for a \f$k=2^d\f$-ary tree a number of nodes of
		 *	\f{eqnarray*}{
		 *	n =& \frac{(2^d)^{h+1}}{2^d - 1}\\
		 *	  =& \frac{2^{d \cdot (h+1)}}{2^d - 1}
		 *	\f}
		 */
		size_t _maxNodes;

		/**
		 * @brief
		 *	The dimensionality of a tree.
		 */
		size_t _dim;

		/**
		 * @brief
		 *	The maximum depth of a tree.
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

    //Disable copying class
    BSP_tree_t(const BSP_tree_t& copy) = delete;
    BSP_tree_t& operator=(const BSP_tree_t&) = delete;
	    
	/**
	 * @brief
	 *	Get the parameters of this BSP tree.
	 * @return
	 *	the parameters of this BSP tree.
	 */
	const Parameters& getParameters() const
	{
		return _parameters;
	}

	/**
	 * @brief
	 *	Get the bounds of the content of this BSP tree.
	 * @return
	 *	the bounds of the content of this BSP tree.
	 */
	const BSP::Bounds<bv_t>& getBounds() const
	{
		return _bounds;
	}

	/**
	 * @brief
	 *	Get the bounding box of this BSP tree.
	 * @return
	 *	the bounding box of this BSP tree
	 */
	const BSP_aabb_t& getBoundingBox() const
	{
		return bsp_bbox;
	}

	/**
	 * @brief
	 *	Prune non-root, non-empty branches of this BSP tree.
	 * @return
	 *	the number of branches pruned
	 */
	size_t prune();

	bool insert_leaf(BSP_leaf_t *leaf);

	// Override
	size_t removeAllLeaves() override;

	// Override
	void collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const aabb_t& aabb, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	// Override
	void collide(const egolib_frustum_t& frustum, BSP::LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const override;

	void getStats(size_t& free,size_t& used)
	{
		free = _nfree;
		used = _nused;
	}

protected:

	friend class BSP_branch_t; ///< To grant access to BSP_tree_t::createBranch().


	BSP_branch_t *finite;      ///< The root node of the ordinary BSP tree.
	BSP_leaf_list_t infinite;  ///< All leaves which do not fit inside the BSP tree.


	Shell *_used;  ///< A singly-linked list of used branches.
	               ///< The branches are chained up using their Shell::_next pointer.
	size_t _nused; ///< Number of branches chained in _used.

	Shell *_free;  ///< A singly-linked list of free branches.
	               ///< The branches are chained up using their Shell::next pointer.
	size_t _nfree; ///< Number of branches chained in _free.

	/**
	* @brief
	*	The parameters of this BSP tree.
	*/
	Parameters _parameters;

	/**
	* @brief
	*   The depth of this tree actually has.
	*/
	size_t depth;



	/**
	* @brief
	*	The bounds of everything in this tree.
	* @todo
	*	Use BSP::Bounds.
	* @todo
	*	Rename to @a _bounds.
	*/
	BSP::Bounds<bv_t> _bounds;

	BSP_aabb_t bsp_bbox; ///< the root-size of the tree


	/**
	* @brief
	*	Ensure that the root node exists.
	* @return
	*	the root node on success, @a nullptr on failure
	* @remark
	*	The root node has a depth of @a 0.
	*/
	BSP_branch_t *ensure_root();

	/**
	 * @brief
	 *	Create a branch.
	 * @return
	 *	the free branch on success, @a nullptr on failure
	 * @post
	 *	A branch returned by this function is empty, has no parent, and has a depth of @a 0.
	 */
	BSP_branch_t *createBranch();
};
