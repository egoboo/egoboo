#pragma once

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

/// @file bsp.h
/// @details

#include "../egolib/frustum.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

    struct s_egolib_frustum;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

    struct s_BSP_aabb;
    typedef struct s_BSP_aabb BSP_aabb_t;

    struct s_BSP_leaf;
    typedef struct s_BSP_leaf BSP_leaf_t;

    struct s_BSP_branch;
    typedef struct s_BSP_branch BSP_branch_t;

    struct s_BSP_branch_list;
    typedef struct s_BSP_branch_list BSP_branch_list_t;

    struct s_BSP_leaf_list;
    typedef struct s_BSP_leaf_list BSP_leaf_list_t;

    struct s_BSP_tree;
    typedef struct s_BSP_tree BSP_tree_t;

//--------------------------------------------------------------------------------------------
// BSP types
//--------------------------------------------------------------------------------------------

    typedef bool_t ( BSP_leaf_test_t )( BSP_leaf_t * );

//--------------------------------------------------------------------------------------------
// known BSP types
//--------------------------------------------------------------------------------------------

    enum e_bsp_type
    {
        BSP_LEAF_NONE = -1,
        BSP_LEAF_CHR,
        BSP_LEAF_ENC,
        BSP_LEAF_PRT,
        BSP_LEAF_TILE
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_BSP_aabb
    {
        bool_t      valid;
        size_t      dim;

        float_ary_t mins;
        float_ary_t mids;
        float_ary_t maxs;
    };

#define BSP_AABB_INIT_VALS                         \
    {                                                  \
        bfalse,                /* bool_t      valid */ \
        0,                     /* size_t      dim   */ \
        DYNAMIC_ARY_INIT_VALS, /* float_ary_t mins  */ \
        DYNAMIC_ARY_INIT_VALS, /* float_ary_t mids  */ \
        DYNAMIC_ARY_INIT_VALS  /* float_ary_t maxs  */ \
    }

    BSP_aabb_t * BSP_aabb_ctor( BSP_aabb_t * pbb, size_t dim );
    BSP_aabb_t * BSP_aabb_dtor( BSP_aabb_t * pbb );

    BSP_aabb_t * BSP_aabb_alloc( BSP_aabb_t * pbb, size_t dim );
    BSP_aabb_t * BSP_aabb_dealloc( BSP_aabb_t * pbb );

    bool_t       BSP_aabb_empty( const BSP_aabb_t * pbb );
    bool_t       BSP_aabb_self_clear( BSP_aabb_t * pbb );

    bool_t       BSP_aabb_from_oct_bb( BSP_aabb_t * pdst, const oct_bb_t * psrc );

    bool_t       BSP_aabb_validate( BSP_aabb_t * pbb );
    bool_t       BSP_aabb_invalidate( BSP_aabb_t * pbb );
    bool_t       BSP_aabb_copy( BSP_aabb_t * pdst, const BSP_aabb_t * psrc );

    bool_t       BSP_aabb_self_union( BSP_aabb_t * pdst, const BSP_aabb_t * psrc );

//--------------------------------------------------------------------------------------------
    struct s_BSP_leaf
    {
        bool_t              inserted;

        struct s_BSP_leaf * next;
        int                 data_type;
        void              * data;
        size_t              index;

        ego_aabb_t          bbox;
    };

    DECLARE_DYNAMIC_ARY( BSP_leaf_ary, BSP_leaf_t )
    DECLARE_DYNAMIC_ARY( BSP_leaf_pary, BSP_leaf_t * )

    BSP_leaf_t * BSP_leaf_ctor( BSP_leaf_t * L, void * data, int type, int index );
    BSP_leaf_t * BSP_leaf_dtor( BSP_leaf_t * L );
    bool_t       BSP_leaf_clear( BSP_leaf_t * L );
    bool_t       BSP_leaf_unlink( BSP_leaf_t * L );
    bool_t       BSP_leaf_copy( BSP_leaf_t * L_dst, const BSP_leaf_t * L_src );

// OBSOLETE
//BSP_leaf_t * BSP_leaf_create( void * data, int type, int index );
//bool_t       BSP_leaf_destroy( BSP_leaf_t ** pL );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    struct s_BSP_leaf_list
    {
        size_t          count;
        BSP_leaf_t    * lst;

        ego_aabb_t      bbox;
    };

#define BSP_LEAF_LIST_INIT_VALS                       \
    {                                                     \
        0,                   /* size_t          count */  \
        NULL,                /* BSP_leaf_t    * lst   */  \
        AABB_INIT_VALS       /* aabb_t          bbox  */  \
    }

    BSP_leaf_list_t * BSP_leaf_list_ctor( BSP_leaf_list_t * );
    BSP_leaf_list_t * BSP_leaf_list_dtor( BSP_leaf_list_t * );
    BSP_leaf_list_t * BSP_leaf_list_clear( BSP_leaf_list_t * );

    bool_t BSP_leaf_list_alloc( BSP_leaf_list_t * );
    bool_t BSP_leaf_list_dealloc( BSP_leaf_list_t * );
    bool_t BSP_leaf_list_reset( BSP_leaf_list_t * );

    bool_t       BSP_leaf_list_push_front( BSP_leaf_list_t *, BSP_leaf_t * n );
    BSP_leaf_t * BSP_leaf_list_pop_front( BSP_leaf_list_t * );

#define EMPTY_BSP_LEAF_LIST(LL) ( (NULL == (LL)) || (NULL == (LL)->lst) || (0 == (LL)->count) )

//--------------------------------------------------------------------------------------------
    struct s_BSP_branch_list
    {
        size_t          lst_size;
        BSP_branch_t ** lst;

        size_t          inserted;
        ego_aabb_t      bbox;
    };

    BSP_branch_list_t * BSP_branch_list_ctor( BSP_branch_list_t *, size_t dim );
    BSP_branch_list_t * BSP_branch_list_dtor( BSP_branch_list_t * );

    bool_t BSP_branch_list_alloc( BSP_branch_list_t *, size_t dim );
    bool_t BSP_branch_list_dealloc( BSP_branch_list_t * );
    bool_t BSP_branch_list_clear_rec( BSP_branch_list_t * );

    bool_t BSP_branch_list_collide_frustum( const BSP_branch_list_t * BL, const struct s_egolib_frustum * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
    bool_t BSP_branch_list_collide_aabb( const BSP_branch_list_t * BL, const struct s_aabb * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );

#define INVALID_BSP_BRANCH_LIST(BL) ( (NULL == (BL)) || (NULL == (BL)->lst) || (0 == (BL)->lst_size) )

//--------------------------------------------------------------------------------------------
    struct s_BSP_branch
    {
        BSP_branch_t  * parent;                 //< the parent branch of this branch

        BSP_leaf_list_t   unsorted;             //< nodes that have not yet been sorted
        BSP_branch_list_t children;             //< the child branches of this branch
        BSP_leaf_list_t   nodes;                //< the nodes at this level

        BSP_aabb_t      bsp_bbox;               //< the size of the node
        int             depth;                  //< the actual depth of this branch
    };

    DECLARE_DYNAMIC_ARY( BSP_branch_ary, BSP_branch_t )
    DECLARE_DYNAMIC_ARY( BSP_branch_pary, BSP_branch_t * )

    BSP_branch_t * BSP_branch_ctor( BSP_branch_t * B, size_t dim );
    BSP_branch_t * BSP_branch_dtor( BSP_branch_t * B );
    bool_t         BSP_branch_alloc( BSP_branch_t * B, size_t dim );
    bool_t         BSP_branch_dealloc( BSP_branch_t * B );

    bool_t         BSP_branch_empty( const BSP_branch_t * pbranch );

    bool_t         BSP_branch_clear( BSP_branch_t * B, bool_t recursive );
    bool_t         BSP_branch_free_nodes( BSP_branch_t * B, bool_t recursive );
    bool_t         BSP_branch_unlink_all( BSP_branch_t * B );
    bool_t         BSP_branch_unlink_parent( BSP_branch_t * B );
    bool_t         BSP_branch_unlink_children( BSP_branch_t * B );
    bool_t         BSP_branch_unlink_nodes( BSP_branch_t * B );
    bool_t         BSP_branch_update_depth_rec( BSP_branch_t * B, int depth );

    bool_t         BSP_branch_add_all_rec( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
    bool_t         BSP_branch_add_all_nodes( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
    bool_t         BSP_branch_add_all_unsorted( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
    bool_t         BSP_branch_add_all_children( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );

// OBSOLETE
//BSP_branch_t * BSP_branch_create( size_t dim );
//bool_t         BSP_branch_destroy( BSP_branch_t ** ppbranch );
//BSP_branch_t * BSP_branch_create_ary( size_t ary_size, size_t dim );
//bool_t         BSP_branch_destroy_ary( size_t ary_size, BSP_branch_t ** ppbranch );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    struct s_BSP_tree
    {
        size_t dimensions;             ///< the number of dimensions used by the tree
        int    max_depth;              ///< the maximum depth that the BSP supports

        BSP_branch_ary_t  branch_all;  ///< the list of pre-allocated branches
        BSP_branch_pary_t branch_used; ///< a linked list of all used pre-allocated branches
        BSP_branch_pary_t branch_free; ///< a linked list of all free pre-allocated branches

        BSP_branch_t    * finite;      ///< the root node of the ordinary BSP tree
        BSP_leaf_list_t   infinite;    ///< all nodes which do not fit inside the BSP tree

        int               depth;       ///< the maximum actual depth of the tree
        ego_aabb_t        bbox;        ///< the actual size of everything in the tree
        BSP_aabb_t        bsp_bbox;    ///< the root-size of the tree
    };

#define BSP_TREE_INIT_VALS                                                   \
    {                                                                        \
        0,                         /* size_t              dimensions     */  \
        0,                         /* int                 max_depth      */  \
        DYNAMIC_ARY_INIT_VALS,     /* BSP_branch_ary_t    branch_all     */  \
        DYNAMIC_ARY_INIT_VALS,     /* BSP_branch_pary_t   branch_used    */  \
        DYNAMIC_ARY_INIT_VALS,     /* BSP_branch_pary_t   branch_free    */  \
        NULL,                      /* BSP_branch_t      * root           */  \
        BSP_LEAF_LIST_INIT_VALS,   /* BSP_leaf_list_t     infinite       */  \
        0,                         /* int                 depth          */  \
        EGO_AABB_INIT_VALS,        /* ego_aabb_t          bbox           */  \
        BSP_AABB_INIT_VALS         /* BSP_aabb_t          bsp_bbox       */  \
    }

    BSP_tree_t * BSP_tree_ctor( BSP_tree_t * t, Sint32 dim, Sint32 depth );
    BSP_tree_t * BSP_tree_dtor( BSP_tree_t * t );
    bool_t       BSP_tree_alloc( BSP_tree_t * t, size_t count, size_t dim );
    bool_t       BSP_tree_dealloc( BSP_tree_t * t );

    bool_t         BSP_tree_clear_rec( BSP_tree_t * t );
    bool_t         BSP_tree_prune( BSP_tree_t * t );
    BSP_branch_t * BSP_tree_get_free( BSP_tree_t * t );
    BSP_branch_t * BSP_tree_ensure_root( BSP_tree_t * t );
    BSP_branch_t * BSP_tree_ensure_branch( BSP_tree_t * t, BSP_branch_t * B, int index );
    Sint32         BSP_tree_count_nodes( Sint32 dim, Sint32 depth );
    bool_t         BSP_tree_insert_leaf( BSP_tree_t * ptree, BSP_leaf_t * pleaf );
    bool_t         BSP_tree_prune_branch( BSP_tree_t * t, size_t cnt );

    int            BSP_tree_collide_aabb( const BSP_tree_t * tree, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
    int            BSP_tree_collide_frustum( const BSP_tree_t * tree, const struct s_egolib_frustum * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );

// OBSOLETE
//BSP_tree_t * BSP_tree_create( size_t count );
//bool_t       BSP_tree_destroy( BSP_tree_t ** ptree );
//bool_t       BSP_tree_init_0( BSP_tree_t * t );
//BSP_tree_t * BSP_tree_init_1( BSP_tree_t * t, Sint32 dim, Sint32 depth );
//bool_t         BSP_tree_insert( BSP_tree_t * t, BSP_branch_t * B, BSP_leaf_t * n, int index );
//bool_t         BSP_tree_free_nodes( BSP_tree_t * t, bool_t recursive );
//bool_t         BSP_tree_free_all( BSP_tree_t * t );
//bool_t         BSP_tree_add_free( BSP_tree_t * t, BSP_branch_t * B );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_bsp_h
