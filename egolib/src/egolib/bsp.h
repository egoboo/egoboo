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

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

	// Forward declaration.
	struct egolib_frustum_t;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

    struct BSP_aabb_t;
    struct BSP_leaf_t;
    struct BSP_branch_t;
    struct BSP_branch_list_t;
    struct BSP_leaf_list_t;
    struct BSP_tree_t;

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
//--------------------------------------------------------------------------------------------

#include "egolib/bsp_aabb.h"


//--------------------------------------------------------------------------------------------
    struct BSP_leaf_t
    {
        bool inserted;

        BSP_leaf_t *next;
        bsp_type_t data_type;
        void *data;
        size_t index;
        bv_t bbox;
		static BSP_leaf_t *ctor(BSP_leaf_t * self, void * data, bsp_type_t type, int index);
		static void dtor(BSP_leaf_t * self);
    };


    bool BSP_leaf_clear( BSP_leaf_t * L );
    bool BSP_leaf_remove_link( BSP_leaf_t * L );
    bool BSP_leaf_copy( BSP_leaf_t * L_dst, const BSP_leaf_t * L_src );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    struct BSP_leaf_list_t
    {
        size_t count;
        BSP_leaf_t *lst;
        bv_t bbox;
		/**
		 * @brief
		 *	Construct this leaf list.
		 * @param self
		 *	this leaf list
		 */
		static BSP_leaf_list_t *ctor(BSP_leaf_list_t *self);
		/**
		 * @brief
		 *	Destruct this leaf list.
		 * @param self
		 *	this leaf list
		 */
		static BSP_leaf_list_t *dtor(BSP_leaf_list_t *self);
    };

	BSP_leaf_list_t *BSP_leaf_list_clear(BSP_leaf_list_t *self);
    bool BSP_leaf_list_alloc(BSP_leaf_list_t *self);
    bool BSP_leaf_list_dealloc(BSP_leaf_list_t *self);
    bool BSP_leaf_list_reset(BSP_leaf_list_t *self);

	/**
	 * @brief
	 *	Insert a leaf in the list, making sure there are no duplicates.
	 * @remark
	 *	Duplicates will cause loops in the list and make it impossible to traverse properly.
	 */
    bool BSP_leaf_list_push_front(BSP_leaf_list_t& self, BSP_leaf_t *leaf);
    BSP_leaf_t * BSP_leaf_list_pop_front(BSP_leaf_list_t * );

#define EMPTY_BSP_LEAF_LIST(LL) ( (NULL == (LL)) || (NULL == (LL)->lst) || (0 == (LL)->count) )

//--------------------------------------------------------------------------------------------
    struct BSP_branch_list_t
    {
        size_t lst_size;
        BSP_branch_t **lst;
        size_t inserted;
        bv_t bbox;
		static BSP_branch_list_t *ctor(BSP_branch_list_t *self, size_t dim);
		static BSP_branch_list_t *dtor(BSP_branch_list_t *self);
	};

    bool BSP_branch_list_clear_rec( BSP_branch_list_t * );

#if 0
	DECLARE_DYNAMIC_ARY(BSP_leaf_pary, BSP_leaf_t *)
#endif

	bool BSP_branch_list_collide_frustum(const BSP_branch_list_t * BL, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, Ego::DynamicArray< BSP_leaf_t * > * colst);
    bool BSP_branch_list_collide_aabb( const BSP_branch_list_t * BL, const aabb_t * paabb, BSP_leaf_test_t * ptest, Ego::DynamicArray< BSP_leaf_t * > * colst );

#define INVALID_BSP_BRANCH_LIST(BL) ( (NULL == (BL)) || (NULL == (BL)->lst) || (0 == (BL)->lst_size) )

//--------------------------------------------------------------------------------------------
struct BSP_branch_t
{
    BSP_branch_t *parent;       ///< the parent branch of this branch

    BSP_leaf_list_t unsorted;   ///< nodes that have not yet been sorted
    BSP_branch_list_t children; ///< the child branches of this branch
    BSP_leaf_list_t leaves;     ///< the leaves at this level

    BSP_aabb_t bsp_bbox;        //< the size of the node
    int depth;                  //< the actual depth of this branch

	/**
	 * @brief
	 *	Construct this branch.
	 * @param self
	 *	this branch
	 * @param dim
	 *	the dimensionality of this branch
	 * @return
	 *	@a self on success, @a NULL on failure
	 */
	static BSP_branch_t *ctor(BSP_branch_t *self, size_t dim);
	/**
	 * @brief
	 *	Destruct this branch.
	 * @param self
	 *	this branch
	 */
	static void dtor(BSP_branch_t *self);
};

/// @brief Get if this branch is empty.
/// @param self this branch
/// @return @a true on success, @a false on failure
bool BSP_branch_empty(const BSP_branch_t *self);
/// @brief Clear this branch.
/// @param self this branch
/// @param recursive if @a true, recursively clear this branch
bool BSP_branch_clear(BSP_branch_t *self, bool recursive);
bool BSP_branch_free_nodes( BSP_branch_t * B, bool recursive );
bool BSP_branch_unlink_all( BSP_branch_t * B );
bool BSP_branch_unlink_parent( BSP_branch_t * B );
bool BSP_branch_unlink_children( BSP_branch_t * B );
bool BSP_branch_unlink_nodes( BSP_branch_t * B );
bool BSP_branch_update_depth_rec( BSP_branch_t * B, int depth );

bool BSP_branch_add_all_rec(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, Ego::DynamicArray<BSP_leaf_t *>  *colst);
bool BSP_branch_add_all_nodes(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, Ego::DynamicArray<BSP_leaf_t *>  *colst);
bool BSP_branch_add_all_unsorted(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, Ego::DynamicArray<BSP_leaf_t *>  *colst);
bool BSP_branch_add_all_children(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, Ego::DynamicArray<BSP_leaf_t *>  *colst);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct BSP_tree_t
{
    size_t dimensions;          ///< the number of dimensions used by the tree
    int max_depth;              ///< the maximum depth that the BSP supports

	Ego::DynamicArray < BSP_branch_t>    branch_all;  ///< the list of pre-allocated branches
	Ego::DynamicArray < BSP_branch_t * > branch_used; ///< a linked list of all used pre-allocated branches
	Ego::DynamicArray < BSP_branch_t * > branch_free; ///< a linked list of all free pre-allocated branches

    BSP_branch_t *finite;      ///< the root node of the ordinary BSP tree
    BSP_leaf_list_t infinite;  ///< all nodes which do not fit inside the BSP tree

    int depth;           ///< the maximum actual depth of the tree
    bv_t bbox;           ///< the actual size of everything in the tree
    BSP_aabb_t bsp_bbox; ///< the root-size of the tree
};

BSP_tree_t *BSP_tree_ctor( BSP_tree_t * t, Sint32 dim, Sint32 depth );
BSP_tree_t *BSP_tree_dtor( BSP_tree_t * t );
bool BSP_tree_alloc( BSP_tree_t * t, size_t count, size_t dim );
bool BSP_tree_dealloc( BSP_tree_t * t );

bool BSP_tree_clear_rec( BSP_tree_t * t );
bool BSP_tree_prune( BSP_tree_t * t );
BSP_branch_t *BSP_tree_get_free( BSP_tree_t * t );
BSP_branch_t *BSP_tree_ensure_root( BSP_tree_t * t );
BSP_branch_t *BSP_tree_ensure_branch( BSP_tree_t * t, BSP_branch_t * B, int index );
Sint32 BSP_tree_count_nodes( Sint32 dim, Sint32 depth );
bool BSP_tree_insert_leaf( BSP_tree_t * ptree, BSP_leaf_t * pleaf );
bool BSP_tree_prune_branch( BSP_tree_t * t, size_t cnt );

size_t BSP_tree_collide_aabb( const BSP_tree_t * tree, const aabb_t * paabb, BSP_leaf_test_t * ptest, Ego::DynamicArray<BSP_leaf_t *> *colst );
size_t BSP_tree_collide_frustum(const BSP_tree_t * tree, const egolib_frustum_t * paabb, BSP_leaf_test_t * ptest, Ego::DynamicArray<BSP_leaf_t *> *colst);


//inline
bool BSP_leaf_valid(BSP_leaf_t *self);
