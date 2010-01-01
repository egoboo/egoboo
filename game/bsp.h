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
struct sBSP_node
{
  struct sBSP_node * next;
  int                 data_type;
  void              * data;
};
typedef struct sBSP_node BSP_node_t;

//--------------------------------------------------------------------------------------------
struct sBSP_leaf
{
  struct sBSP_leaf  * parent;
  size_t               child_count;
  struct sBSP_leaf ** children;
  BSP_node_t           * nodes;
};
typedef struct sBSP_leaf BSP_leaf_t;

//--------------------------------------------------------------------------------------------
struct sBSP_tree
{
  int dimensions;
  int depth;

  size_t       leaf_count;
  BSP_leaf_t * leaf_list;

  BSP_leaf_t * root;
};
typedef struct sBSP_tree BSP_tree_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BSP_node_t * BSP_node_ctor( BSP_node_t * t, void * data, int type );
bool_t       BSP_node_dtor( BSP_node_t * t );

BSP_leaf_t * BSP_leaf_ctor( BSP_leaf_t * L, size_t size );
bool_t       BSP_leaf_dtor( BSP_leaf_t * L );
bool_t       BSP_leaf_insert( BSP_leaf_t * L, BSP_node_t * n );

BSP_tree_t * BSP_tree_ctor( BSP_tree_t * t, Sint32 dim, Sint32 depth);
bool_t       BSP_tree_dtor( BSP_tree_t * t );

Sint32       BSP_tree_count_nodes(Sint32 dim, Sint32 depth);

int BSP_find_address_2d(OVolume_t * vmax, OVolume_t * vtile, size_t depth, int address_x[], int address_y[] );
int BSP_find_address_3d(OVolume_t * vmax, OVolume_t * vtile, size_t depth, int address_x[], int address_y[], int address_z[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define Egoboo_bsp_h

