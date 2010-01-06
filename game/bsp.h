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

#include "bbox.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_BSP_node
{
    struct s_BSP_node * next;
    int                 data_type;
    void              * data;
    int                 index;
};
typedef struct s_BSP_node BSP_node_t;

BSP_node_t * BSP_node_create( void * data, int type );
bool_t       BSP_node_destroy( BSP_node_t ** pnode );
BSP_node_t * BSP_node_ctor( BSP_node_t * t, void * data, int type );
bool_t       BSP_node_dtor( BSP_node_t * t );

//--------------------------------------------------------------------------------------------
struct s_BSP_leaf
{
    struct s_BSP_leaf  * parent;
    size_t               child_count;
    struct s_BSP_leaf ** children;
    BSP_node_t         * nodes;
};
typedef struct s_BSP_leaf BSP_leaf_t;

BSP_leaf_t * BSP_leaf_create( size_t count );
bool_t       BSP_leaf_destroy( BSP_leaf_t ** pleaf );
BSP_leaf_t * BSP_leaf_create_ary( size_t ary_size, size_t count );
bool_t       BSP_leaf_destroy_ary( size_t ary_size, BSP_leaf_t ** pleaf );
BSP_leaf_t * BSP_leaf_ctor( BSP_leaf_t * L, size_t size );
bool_t       BSP_leaf_dtor( BSP_leaf_t * L );
bool_t       BSP_leaf_insert( BSP_leaf_t * L, BSP_node_t * n );
bool_t       BSP_leaf_free_nodes( BSP_leaf_t * L, bool_t recursive );

//--------------------------------------------------------------------------------------------
struct s_BSP_tree
{
    int dimensions;
    int depth;

    size_t       leaf_count;
    BSP_leaf_t * leaf_list;
    BSP_leaf_t * leaf_free;

    BSP_leaf_t * root;

    BSP_node_t * infinite;
};
typedef struct s_BSP_tree BSP_tree_t;

BSP_tree_t * BSP_tree_create( size_t count );
bool_t       BSP_tree_destroy( BSP_tree_t ** ptree );

BSP_tree_t * BSP_tree_ctor_0( BSP_tree_t * t );
BSP_tree_t * BSP_tree_ctor_1( BSP_tree_t * t, Sint32 dim, Sint32 depth );
BSP_tree_t * BSP_tree_dtor( BSP_tree_t * t );
bool_t       BSP_tree_allocate( BSP_tree_t * t, size_t count, size_t dim );
bool_t       BSP_tree_deallocate( BSP_tree_t * t );
bool_t       BSP_tree_init_0( BSP_tree_t * t );
BSP_tree_t * BSP_tree_init_1( BSP_tree_t * t, Sint32 dim, Sint32 depth );

bool_t       BSP_tree_clear( BSP_tree_t * t );
bool_t       BSP_tree_free_nodes( BSP_tree_t * t );
bool_t       BSP_tree_prune( BSP_tree_t * t );
BSP_leaf_t * BSP_tree_get_free( BSP_tree_t * t );
bool_t       BSP_tree_add_free( BSP_tree_t * t, BSP_leaf_t * L );
bool_t       BSP_tree_create_root( BSP_tree_t * t );
BSP_leaf_t * BSP_tree_ensure_leaf( BSP_tree_t * t, BSP_leaf_t * L, int index );
bool_t       BSP_tree_insert( BSP_tree_t * t, BSP_leaf_t * L, BSP_node_t * n, int index );
Sint32       BSP_tree_count_nodes( Sint32 dim, Sint32 depth );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int BSP_find_address_2d( oct_bb_t vmax, oct_bb_t vtile, size_t depth, int address_x[], int address_y[] );
int BSP_find_address_3d( oct_bb_t vmax, oct_bb_t vtile, size_t depth, int address_x[], int address_y[], int address_z[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define Egoboo_bsp_h

