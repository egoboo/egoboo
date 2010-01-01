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

/// @file bsp.c
/// @brief
/// @details

#include "bsp.h"

#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE BSP_node_t * BSP_node_ctor( BSP_node_t * n, void * data, int type )
{
  if(NULL == n) return n;

  BSP_node_dtor( n );

  memset(n, 0, sizeof(BSP_node_t));

  if(NULL == data) return n;

  n->data_type = type;
  n->data      = data;

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_node_dtor( BSP_node_t * n )
{
  bool_t retval;

  if( NULL == n ) return bfalse;

  n->data_type = -1;
  n->data      = NULL;

  return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE BSP_leaf_t * BSP_leaf_ctor( BSP_leaf_t * L, size_t count )
{
  if(NULL == L) return L;

  BSP_leaf_dtor( L );

  memset(L, 0, sizeof(BSP_leaf_t));

  L->children = EGOBOO_NEW_ARY(BSP_leaf_t*, count);
  if(NULL != L->children)
  {
    L->child_count = count;
  }

  return L;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_leaf_dtor( BSP_leaf_t * L )
{
  bool_t retval;

  if( NULL == L ) return bfalse;

  EGOBOO_DELETE(L->children);
  L->child_count = 0;

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_leaf_insert( BSP_leaf_t * L, BSP_node_t * n )
{
  if( NULL == L || NULL == n ) return bfalse;

  n->next  = L->nodes;
  L->nodes = n;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_allocate( BSP_tree_t * t, size_t count, size_t children )
{
  size_t i;

  if(NULL == t) return bfalse;
  if(NULL != t->leaf_list || t->leaf_count > 0) return bfalse;

  // allocate the nodes
  t->leaf_list = EGOBOO_NEW_ARY( BSP_leaf_t, count );
  if(NULL == t->leaf_list) return bfalse;

  // initialize the nodes
  t->leaf_count = count;
  for(i=0; i<count; i++)
  {
    BSP_leaf_ctor(t->leaf_list + i, children);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_deallocate( BSP_tree_t * t )
{
  size_t i;

  if( NULL == t ) return bfalse;

  if( NULL == t->leaf_list || 0 == t->leaf_count ) return btrue;

  // delete the nodes
  for(i=0; i<t->leaf_count; i++)
  {
    BSP_leaf_dtor(t->leaf_list + i);
  }

  EGOBOO_DELETE(t->leaf_list);
  t->leaf_count = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE BSP_tree_t * BSP_tree_ctor_default(BSP_tree_t * t)
{
  int i, j, k;
  int children, current_node;

  if(NULL == t ) return NULL;

  if(0 == t->leaf_count || NULL == t->leaf_list) return t;

  children = 2 << t->dimensions;
  t->leaf_list[0].parent = NULL;
  current_node = 0;
  for(i=0; i<t->depth; i++)
  {
    int node_at_depth, free_node;

    node_at_depth = 1 << (i * t->dimensions);
    free_node = BSP_tree_count_nodes(t->dimensions, i);
    for(j=0; j<node_at_depth; j++)
    {
      for(k=0;k<children;k++)
      {
        t->leaf_list[free_node].parent = t->leaf_list + current_node;
        t->leaf_list[current_node].children[k] = t->leaf_list + free_node;
        free_node++;
      }
      current_node++;
    }
  }

  return t;
}

//--------------------------------------------------------------------------------------------
INLINE BSP_tree_t * BSP_tree_ctor( BSP_tree_t * t, Sint32 dim, Sint32 depth)
{
  Sint32 count, children;

  if(NULL == t) return t;

  BSP_tree_dtor( t );

  memset(t, 0, sizeof(BSP_tree_t));

  count = BSP_tree_count_nodes(dim, depth);
  if(count < 0) return t;

  children = 2 << dim;
  if( !BSP_tree_allocate(t, count, children) ) return t;

  t->dimensions = dim;
  t->depth      = depth;

  BSP_tree_ctor_default(t);

  return t;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_dtor( BSP_tree_t * t )
{
  bool_t retval;

  if( NULL == t ) return bfalse;

  retval = BSP_tree_deallocate( t );

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE Sint32 BSP_tree_count_nodes(Sint32 dim, Sint32 depth)
{
  int itmp;
  Sint32 node_count;
  Uint32 numer, denom;

  itmp = dim * (depth+1);
  if( itmp > 31 ) return -1;

  numer = (1 << itmp) - 1;
  denom = (1 <<  dim) - 1;
  node_count    = numer / denom;

  return node_count;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_insert( BSP_tree_t * t, BSP_leaf_t * L, BSP_node_t * n, int index )
{
  if( NULL == t || NULL == L || NULL == n ) return bfalse;
  if( index > (int)L->child_count ) return bfalse;

  if( index >= 0 && NULL != L->children[index])
  {
    // inserting a node into the child
    return BSP_leaf_insert(L->children[index], n);
  }

  if(index < 0 || NULL == t->leaf_list)
  {
    // inserting a node into this leaf node
    // this can either occur because someone requested it (index < 0)
    // OR because there are no more free nodes
    return BSP_leaf_insert(L, n);
  }
  else
  {
    // the requested L->children[index] slot is empty. grab a pre-allocated
    // BSP_leaf_t from the free list in the BSP_tree_t structure an insert it in
    // this child node

    L->children[index] = t->leaf_list;
    t->leaf_list = t->leaf_list->parent;           // parent is doubling for "next"

    BSP_leaf_ctor( L->children[index], 2 << t->dimensions );

    return BSP_leaf_insert(L->children[index], n);
  }

  // something went wrong
  return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int BSP_find_address_2d(OVolume_t * vmax, OVolume_t * vtile, size_t depth, int address_x[], int address_y[] )
{
  // BB> determine the address of the a node

  int i;
  float x_min, x_mid, x_max, y_min, y_mid, y_max;

  x_min = vmax->oct.mins[OCT_X];
  x_max = vmax->oct.maxs[OCT_X];

  y_min = vmax->oct.mins[OCT_Y];
  y_max = vmax->oct.maxs[OCT_Y];

  // determine the address of the node
  for(i=0; i<depth; i++)
  {
    if( (vtile->oct.mins[OCT_X] < x_min || vtile->oct.mins[OCT_X] > x_max) ||
        (vtile->oct.mins[OCT_Y] < y_min || vtile->oct.mins[OCT_Y] > y_max) )
    {
      // the bounding box is too big to fit at this level
      address_x[i] = -1;
      address_y[i] = -1;
      break;
    }

    x_mid = 0.5f * (x_min + x_max);
    y_mid = 0.5f * (y_min + y_max);

    if( vtile->oct.maxs[OCT_X] < x_mid )
    {
      address_x[i] = 0;
      x_max = x_mid;
    }
    else if( vtile->oct.mins[OCT_X] > x_mid )
    {
      address_x[i] = 1;
      x_min = x_mid;
    }
    else
    {
      address_x[i] = -1;
      break;
    }

    if( vtile->oct.maxs[OCT_Y] < y_mid )
    {
      address_y[i] = 0;
      y_max = y_mid;
    }
    else if( vtile->oct.mins[OCT_Y] > y_mid )
    {
      address_y[i] = 1;
      y_min = y_mid;
    }
    else
    {
      address_y[i] = -1;
      break;
    }
  }

  if(address_x[0] < 0 || address_y[0] < 0) i = -1;

  return i;
}

//--------------------------------------------------------------------------------------------
int BSP_find_address_3d(OVolume_t * vmax, OVolume_t * vtile, size_t depth, int address_x[], int address_y[], int address_z[] )
{
  // BB> determine the address of the a node

  int i;
  float x_min, x_mid, x_max;
  float y_min, y_mid, y_max;
  float z_min, z_mid, z_max;

  x_min = vmax->oct.mins[OCT_X];
  x_max = vmax->oct.maxs[OCT_X];

  y_min = vmax->oct.mins[OCT_Y];
  y_max = vmax->oct.maxs[OCT_Y];

  z_min = vmax->oct.mins[OCT_Z];
  z_max = vmax->oct.maxs[OCT_Z];

  // determine the address of the node
  for(i=0; i<depth; i++)
  {
    if( (vtile->oct.mins[OCT_X] < x_min || vtile->oct.mins[OCT_X] > x_max) ||
        (vtile->oct.mins[OCT_Y] < y_min || vtile->oct.mins[OCT_Y] > y_max) ||
        (vtile->oct.mins[OCT_Z] < z_min || vtile->oct.mins[OCT_Z] > z_max) )
    {
      // the bounding box is too big to fit at this level
      address_x[i] = -1;
      address_y[i] = -1;
      break;
    }

    x_mid = 0.5f * (x_min + x_max);
    y_mid = 0.5f * (y_min + y_max);
    z_mid = 0.5f * (z_min + z_max);

    if( vtile->oct.maxs[OCT_X] < x_mid )
    {
      address_x[i] = 0;
      x_max = x_mid;
    }
    else if( vtile->oct.mins[OCT_X] > x_mid )
    {
      address_x[i] = 1;
      x_min = x_mid;
    }
    else
    {
      address_x[i] = -1;
      break;
    }

    if( vtile->oct.maxs[OCT_Y] < y_mid )
    {
      address_y[i] = 0;
      y_max = y_mid;
    }
    else if( vtile->oct.mins[OCT_Y] > y_mid )
    {
      address_y[i] = 1;
      y_min = y_mid;
    }
    else
    {
      address_y[i] = -1;
      break;
    }

    if( vtile->oct.maxs[OCT_Z] < z_mid )
    {
      address_z[i] = 0;
      z_max = z_mid;
    }
    else if( vtile->oct.mins[OCT_Z] > z_mid )
    {
      address_z[i] = 1;
      z_min = z_mid;
    }
    else
    {
      address_z[i] = -1;
      break;
    }
  }

  if(address_x[0] < 0 || address_y[0] < 0 || address_z[0] < 0) i = -1;

  return i;
}