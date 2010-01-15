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

#include <assert.h>

//BSP_tree_t * BSP_tree_create( size_t count );
//bool_t       BSP_tree_destroy( BSP_tree_t ** ptree );
//bool_t       BSP_tree_allocate( BSP_tree_t * t, size_t count, size_t dim );
//bool_t       BSP_tree_deallocate( BSP_tree_t * t );
//bool_t       BSP_tree_clear( BSP_tree_t * t );
//bool_t       BSP_tree_create_root( BSP_tree_t * t );
//bool_t       BSP_tree_free_nodes( BSP_tree_t * t );
//bool_t       BSP_tree_init_0( BSP_tree_t * t );
//BSP_tree_t * BSP_tree_init_1( BSP_tree_t * t, Sint32 dim, Sint32 depth);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BSP_node_t * BSP_node_create( void * data, int type )
{
    BSP_node_t * rv;

    rv = EGOBOO_NEW( BSP_node_t );
    if( NULL == rv ) return rv;

    return BSP_node_ctor( rv, data, type );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_node_destroy( BSP_node_t ** pnode )
{
    if( NULL == pnode || NULL == *pnode ) return bfalse;

    BSP_node_dtor( *pnode );

    EGOBOO_DELETE( *pnode );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_node_t * BSP_node_ctor( BSP_node_t * n, void * data, int type )
{
    if ( NULL == n ) return n;

    BSP_node_dtor( n );

    memset( n, 0, sizeof( BSP_node_t ) );

    if ( NULL == data ) return n;

    n->data_type = type;
    n->data      = data;

    return n;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_node_dtor( BSP_node_t * n )
{
    if ( NULL == n ) return bfalse;

    n->data_type = -1;
    n->data      = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_leaf_create( size_t count )
{
    BSP_leaf_t * rv;

    rv = EGOBOO_NEW(BSP_leaf_t);
    if( NULL == rv ) return rv;

    return BSP_leaf_ctor( rv, count );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_destroy( BSP_leaf_t ** pleaf )
{
    if( NULL == pleaf || NULL == *pleaf ) return bfalse;

    BSP_leaf_dtor( *pleaf );

    EGOBOO_DELETE(*pleaf);

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_leaf_create_ary( size_t ary_size, size_t count )
{
    Uint32 cnt;
    BSP_leaf_t * lst;

    lst = EGOBOO_NEW_ARY(BSP_leaf_t, ary_size);
    if( NULL == lst ) return lst;

    for( cnt = 0; cnt < ary_size; cnt++ )
    {
        BSP_leaf_ctor( lst + cnt, count );
    }

    return lst;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_destroy_ary( size_t ary_size, BSP_leaf_t ** lst )
{
    Uint32 cnt;

    if( NULL == lst || NULL == *lst || 0 == ary_size ) return bfalse;

    for( cnt = 0; cnt < ary_size; cnt++ )
    {
        BSP_leaf_dtor( (*lst) + cnt );
    }

    EGOBOO_DELETE_ARY(*lst);

    return btrue;
}


//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_leaf_ctor( BSP_leaf_t * L, size_t count )
{
    if ( NULL == L ) return L;

    BSP_leaf_dtor( L );

    memset( L, 0, sizeof( BSP_leaf_t ) );

    L->children = EGOBOO_NEW_ARY( BSP_leaf_t*, count );
    if ( NULL != L->children )
    {
        L->child_count = count;
    }

    return L;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_dtor( BSP_leaf_t * L )
{
    if ( NULL == L ) return bfalse;

    EGOBOO_DELETE( L->children );
    L->child_count = 0;

    return btrue;
}

////--------------------------------------------------------------------------------------------
//bool_t BSP_leaf_insert( BSP_leaf_t * L, BSP_node_t * n )
//{
//  if( NULL == L || NULL == n ) return bfalse;
//
//  n->next  = L->nodes;
//  L->nodes = n;
//
//  return btrue;
//}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//bool_t BSP_tree_allocate( BSP_tree_t * t, size_t count, size_t children )
//{
//  size_t i;
//
//  if(NULL == t) return bfalse;
//  if(NULL != t->leaf_list || t->leaf_count > 0) return bfalse;
//
//  // allocate the nodes
//  t->leaf_list = EGOBOO_NEW_ARY( BSP_leaf_t, count );
//  if(NULL == t->leaf_list) return bfalse;
//
//  // initialize the nodes
//  t->leaf_count = count;
//  for(i=0; i<count; i++)
//  {
//    BSP_leaf_ctor(t->leaf_list + i, children);
//  }
//
//  return btrue;
//}

//--------------------------------------------------------------------------------------------
//bool_t BSP_tree_deallocate( BSP_tree_t * t )
//{
//  size_t i;
//
//  if( NULL == t ) return bfalse;
//
//  if( NULL == t->leaf_list || 0 == t->leaf_count ) return btrue;
//
//  // delete the nodes
//  for(i=0; i<t->leaf_count; i++)
//  {
//    BSP_leaf_dtor(t->leaf_list + i);
//  }
//
//  EGOBOO_DELETE(t->leaf_list);
//  t->leaf_count = 0;
//
//  return btrue;
//}

//--------------------------------------------------------------------------------------------
BSP_tree_t * BSP_tree_ctor_default_0( BSP_tree_t * t )
{
    int i, j, k;
    int children, current_node;

    if ( NULL == t ) return NULL;

    if ( 0 == t->leaf_count || NULL == t->leaf_list ) return t;

    children = 2 << t->dimensions;
    t->leaf_list[0].parent = NULL;
    current_node = 0;
    for ( i = 0; i < t->depth; i++ )
    {
        int node_at_depth, free_node;

        node_at_depth = 1 << ( i * t->dimensions );
        free_node = BSP_tree_count_nodes( t->dimensions, i );
        for ( j = 0; j < node_at_depth; j++ )
        {
            for ( k = 0; k < children; k++ )
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
BSP_tree_t * BSP_tree_ctor_1( BSP_tree_t * t, Sint32 dim, Sint32 depth )
{
    Sint32 count, children;

    if ( NULL == t ) return t;

    BSP_tree_dtor( t );

    memset( t, 0, sizeof( BSP_tree_t ) );

    count = BSP_tree_count_nodes( dim, depth );
    if ( count < 0 ) return t;

    children = 2 << dim;
    if ( !BSP_tree_allocate( t, count, children ) ) return t;

    t->dimensions = dim;
    t->depth      = depth;

    BSP_tree_ctor_default_0( t );

    return t;
}

//--------------------------------------------------------------------------------------------
BSP_tree_t * BSP_tree_ctor_0( BSP_tree_t * t )
{
    if ( NULL == t ) return NULL;

    BSP_tree_init_0( t );

    return t;
}

//--------------------------------------------------------------------------------------------
BSP_tree_t * BSP_tree_dtor( BSP_tree_t * t )
{
    if ( NULL == t ) return NULL;

    BSP_tree_deallocate( t );

    return t;
}

//--------------------------------------------------------------------------------------------
//Sint32 BSP_tree_count_nodes(Sint32 dim, Sint32 depth)
//{
//  int itmp;
//  Sint32 node_count;
//  Uint32 numer, denom;
//
//  itmp = dim * (depth+1);
//  if( itmp > 31 ) return -1;
//
//  numer = (1 << itmp) - 1;
//  denom = (1 <<  dim) - 1;
//  node_count    = numer / denom;
//
//  return node_count;
//}

//--------------------------------------------------------------------------------------------
//bool_t BSP_tree_insert( BSP_tree_t * t, BSP_leaf_t * L, BSP_node_t * n, int index )
//{
//  bool_t retval;
//
//  if( NULL == t || NULL == L || NULL == n ) return bfalse;
//  if( index > (int)L->child_count ) return bfalse;
//
//  if( index >= 0 && NULL != L->children[index])
//  {
//    // inserting a node into the child
//    return BSP_leaf_insert(L->children[index], n);
//  }
//
//  retval = bfalse;
//  if(index < 0 || NULL == t->leaf_list)
//  {
//    // inserting a node into this leaf node
//    // this can either occur because someone requested it (index < 0)
//    // OR because there are no more free nodes
//    retval = BSP_leaf_insert(L, n);
//  }
//  else
//  {
//    // the requested L->children[index] slot is empty. grab a pre-allocated
//    // BSP_leaf_t from the free list in the BSP_tree_t structure an insert it in
//    // this child node
//
//    L->children[index] = t->leaf_list;
//    t->leaf_list = t->leaf_list->parent;           // parent is doubling for "next"
//
//    BSP_leaf_ctor( L->children[index], 2 << t->dimensions );
//
//    retval = BSP_leaf_insert(L->children[index], n);
//  }
//
//  return retval;
//}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int BSP_find_address_2d( oct_bb_t vmax, oct_bb_t vtile, size_t depth, int address_x[], int address_y[] )
{
    // BB> determine the address of the a node

    int i;
    float x_min, x_mid, x_max, y_min, y_mid, y_max;

    x_min = vmax.mins[OCT_X];
    x_max = vmax.maxs[OCT_X];

    y_min = vmax.mins[OCT_Y];
    y_max = vmax.maxs[OCT_Y];

    // determine the address of the node
    for ( i = 0; i < depth; i++ )
    {
        if (( vtile.mins[OCT_X] < x_min || vtile.mins[OCT_X] > x_max ) ||
            ( vtile.mins[OCT_Y] < y_min || vtile.mins[OCT_Y] > y_max ) )
        {
            // the bounding box is too big to fit at this level
            address_x[i] = -1;
            address_y[i] = -1;
            break;
        }

        x_mid = 0.5f * ( x_min + x_max );
        y_mid = 0.5f * ( y_min + y_max );

        if ( vtile.maxs[OCT_X] < x_mid )
        {
            address_x[i] = 0;
            x_max = x_mid;
        }
        else if ( vtile.mins[OCT_X] > x_mid )
        {
            address_x[i] = 1;
            x_min = x_mid;
        }
        else
        {
            address_x[i] = -1;
            break;
        }

        if ( vtile.maxs[OCT_Y] < y_mid )
        {
            address_y[i] = 0;
            y_max = y_mid;
        }
        else if ( vtile.mins[OCT_Y] > y_mid )
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

    if ( address_x[0] < 0 || address_y[0] < 0 ) i = -1;

    return i;
}

//--------------------------------------------------------------------------------------------
int BSP_find_address_3d( oct_bb_t vmax, oct_bb_t vtile, size_t depth, int address_x[], int address_y[], int address_z[] )
{
    // BB> determine the address of the a node

    int i;
    float x_min, x_mid, x_max;
    float y_min, y_mid, y_max;
    float z_min, z_mid, z_max;

    x_min = vmax.mins[OCT_X];
    x_max = vmax.maxs[OCT_X];

    y_min = vmax.mins[OCT_Y];
    y_max = vmax.maxs[OCT_Y];

    z_min = vmax.mins[OCT_Z];
    z_max = vmax.maxs[OCT_Z];

    // determine the address of the node
    for ( i = 0; i < depth; i++ )
    {
        if (( vtile.mins[OCT_X] < x_min || vtile.mins[OCT_X] > x_max ) ||
            ( vtile.mins[OCT_Y] < y_min || vtile.mins[OCT_Y] > y_max ) ||
            ( vtile.mins[OCT_Z] < z_min || vtile.mins[OCT_Z] > z_max ) )
        {
            // the bounding box is too big to fit at this level
            address_x[i] = -1;
            address_y[i] = -1;
            break;
        }

        x_mid = 0.5f * ( x_min + x_max );
        y_mid = 0.5f * ( y_min + y_max );
        z_mid = 0.5f * ( z_min + z_max );

        if ( vtile.maxs[OCT_X] < x_mid )
        {
            address_x[i] = 0;
            x_max = x_mid;
        }
        else if ( vtile.mins[OCT_X] > x_mid )
        {
            address_x[i] = 1;
            x_min = x_mid;
        }
        else
        {
            address_x[i] = -1;
            break;
        }

        if ( vtile.maxs[OCT_Y] < y_mid )
        {
            address_y[i] = 0;
            y_max = y_mid;
        }
        else if ( vtile.mins[OCT_Y] > y_mid )
        {
            address_y[i] = 1;
            y_min = y_mid;
        }
        else
        {
            address_y[i] = -1;
            break;
        }

        if ( vtile.maxs[OCT_Z] < z_mid )
        {
            address_z[i] = 0;
            z_max = z_mid;
        }
        else if ( vtile.mins[OCT_Z] > z_mid )
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

    if ( address_x[0] < 0 || address_y[0] < 0 || address_z[0] < 0 ) i = -1;

    return i;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t   BSP_leaf_insert( BSP_leaf_t * L, BSP_node_t * n )
{
    if (( NULL == L ) || ( NULL == n ) ) return bfalse;

    n->next  = L->nodes;
    L->nodes = n;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t BSP_tree_allocate( BSP_tree_t * t, size_t count, size_t dim )
{
    if ( NULL == t ) return bfalse;

    if ( NULL != t->leaf_list || t->leaf_count > 0 ) return bfalse;

    // allocate the nodes
    t->leaf_list = BSP_leaf_create_ary( count, dim );
    if ( NULL == t->leaf_list ) return bfalse;

    t->leaf_count = count;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_deallocate( BSP_tree_t * t )
{
    size_t i;

    if ( NULL == t ) return bfalse;
    if (( NULL == t ) ) return btrue;
    if ( NULL == t->leaf_list || 0 == t->leaf_count ) return btrue;

    // delete the nodes
    for ( i = 0; i < t->leaf_count; i++ )
    {
        BSP_leaf_dtor( t->leaf_list + i );
    }

    BSP_leaf_destroy( &( t->leaf_list ) );
    t->leaf_count = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_tree_t * BSP_tree_init_1( BSP_tree_t * t, Sint32 dim, Sint32 depth )
{
    Sint32 count;

    if ( NULL == t ) return t;
    BSP_tree_init_0( t );

    BSP_tree_clear( t );

    count = BSP_tree_count_nodes( dim, depth );
    if ( count < 0 ) return t;

    if ( !BSP_tree_allocate( t, count, dim ) ) return t;

    // set the maximum depth
    t->depth = depth;

    // initalize the tree, and create the root node
    BSP_tree_create_root( t );

    return t;
}

//--------------------------------------------------------------------------------------------
Sint32 BSP_tree_count_nodes( Sint32 dim, Sint32 depth )
{
    int itmp;
    Sint32 node_count;
    Uint32 numer, denom;

    itmp = dim * ( depth + 1 );
    if ( itmp > 31 ) return -1;

    numer = ( 1 << itmp ) - 1;
    denom = ( 1 <<  dim ) - 1;
    node_count    = numer / denom;

    return node_count;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_tree_get_free( BSP_tree_t * t )
{
    size_t i;
    BSP_leaf_t * ptmp = t->leaf_free;

    if ( NULL != ptmp )
    {
        t->leaf_free = ptmp->parent;

        // make sure that the data is cleared out
        ptmp->parent = NULL;
        assert( NULL == ptmp->nodes );
        ptmp->nodes  = NULL;
        for ( i = 0; i < ptmp->child_count; i++ )
        {
            if ( NULL != ptmp->children[i] )
            {
                ptmp->children[i]->parent = NULL;
            }
            ptmp->children[i] = NULL;
        }
    }

    return ptmp;
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_add_free( BSP_tree_t * t, BSP_leaf_t * L )
{
    size_t i;

    if ( NULL == t || NULL == L ) return bfalse;

    // remove any links to other leaves
    // assume the user knows what they are doing
    L->parent = NULL;
    for ( i = 0; i < L->child_count; i++ )
    {
        if ( NULL != L->children[i] )
        {
            L->children[i]->parent = NULL;
        }
        L->children[i] = NULL;
    }

    //remove any nodes (from this leaf only, not recursively)
    BSP_leaf_free_nodes( L, bfalse );

    // return it to the free list
    L->parent = t->leaf_free;
    t->leaf_free = L;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_init_0( BSP_tree_t * t )
{
    // BB > reset the tree to the "empty" state
    //      Assume we do not own the nodes or children

    size_t i, j;
    BSP_leaf_t * pleaf;

    // free any the nodes in the tree
    BSP_tree_free_nodes( t );

    // initialize the leaves.
    t->root      = NULL;
    t->leaf_free = NULL;
    for ( i = 0; i < t->leaf_count; i++ )
    {
        // grab a leaf off of the static list
        pleaf = t->leaf_list + i;

        // clear out any data in the leaf
        pleaf->parent = NULL;
        pleaf->nodes  = NULL;
        for ( j = 0; j < pleaf->child_count; j++ )
        {
            pleaf->children[j] = NULL;
        }

        // push it onto the "stack" through pleaf->parent
        pleaf->parent = t->leaf_free;
        t->leaf_free = pleaf;
    };

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_leaf_free_nodes( BSP_leaf_t * L, bool_t recursive )
{
    size_t i;
    BSP_node_t * pnode;

    if ( NULL == L ) return bfalse;

    if ( recursive )
    {
        // recursively clear out any nodes of the children
        for ( i = 0; i < L->child_count; i++ )
        {
            if ( NULL != L->children[i] )
            {
                BSP_leaf_free_nodes( L->children[i], btrue );
            }
        };
    }

    // free all nodes of this leaf
    while ( NULL != L->nodes )
    {
        // pop a node off the stack
        pnode = L->nodes;
        L->nodes = pnode->next;

        pnode->next = NULL;
    };

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_free_nodes( BSP_tree_t * t )
{
    BSP_node_t * pnode;

    if ( NULL == t ) return bfalse;

    // free all infinite nodes
    while ( NULL != t->infinite )
    {
        // pop a node off the stack
        pnode = t->infinite;
        t->infinite = pnode->next;

        pnode->next = NULL;
    };

    return BSP_leaf_free_nodes( t->root, btrue );
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_leaf_prune( BSP_tree_t * t, BSP_leaf_t * L )
{
    // BB> remove all leaves with no children. Do a depth first recursive search for efficiency

    size_t i;
    bool_t   empty;

    if ( NULL == L ) return bfalse;

    // prune all the children
    for ( i = 0; i < L->child_count; i++ )
    {
        BSP_leaf_prune( t, L->children[i] );
    }

    // do not remove the root node
    if ( L == t->root ) return btrue;

    // look to see if all children are free
    empty = btrue;
    for ( i = 0; i < L->child_count; i++ )
    {
        if ( NULL != L->children[i] ) empty = bfalse;
    }

    // check to see if there are any nodes attached to this leaf
    if ( NULL != L->nodes ) empty = bfalse;

    // if there are no children and no nodes, this leaf is useless
    if ( empty )
    {
        if ( NULL != L->parent )
        {
            bool_t   found = bfalse;
            // unlink the parent and return the node to the free list
            for ( i = 0; i < L->parent->child_count; i++ )
            {
                if ( L->parent->children[i] == L )
                {
                    L->parent->children[i] = NULL;
                    found = btrue;
                    break;
                }
            }
            assert( found );
        }
        BSP_tree_add_free( t, L );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_prune( BSP_tree_t * t )
{
    // BB> remove all leaves with no children.

    if ( NULL == t || NULL == t->root ) return bfalse;

    return BSP_leaf_prune( t, t->root );
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_create_root( BSP_tree_t * t )
{
    if ( !BSP_tree_init_0( t ) ) return bfalse;

    t->root = BSP_tree_get_free( t );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_tree_ensure_leaf( BSP_tree_t * t, BSP_leaf_t * L, int index )
{
    if (( NULL == t ) || ( NULL == L ) ) return NULL;
    if ( index < 0 || index > ( int )L->child_count ) return NULL;

    if ( NULL == L->children[index] )
    {
        L->children[index] = BSP_tree_get_free( t );
        L->children[index]->parent = L;
    }

    return L->children[index];
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_insert( BSP_tree_t * t, BSP_leaf_t * L, BSP_node_t * n, int index )
{
    bool_t retval;

    if (( NULL == t ) || ( NULL == L ) || ( NULL == n ) ) return bfalse;
    if ( index > ( int )L->child_count ) return bfalse;

    if ( index >= 0 && NULL != L->children[index] )
    {
        // inserting a node into the child
        return BSP_leaf_insert( L->children[index], n );
    }

    if ( index < 0 || NULL == t->leaf_free )
    {
        // inserting a node into this leaf node
        // this can either occur because someone requested it (index < 0)
        // OR because there are no more free nodes
        retval = BSP_leaf_insert( L, n );
    }
    else
    {
        // the requested L->children[index] slot is empty. grab a pre-allocated
        // BSP_leaf_t from the free list in the BSP_tree_t structure an insert it in
        // this child node

        L->children[index] = BSP_tree_get_free( t );
        L->children[index]->parent = L;

        BSP_leaf_ctor( L->children[index], t->dimensions );
        retval = BSP_leaf_insert( L->children[index], n );
    }

    // something went wrong ?
    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
BSP_node_t * BSP_node_init( BSP_node_t * n, void * data, int type )
{
    if ( NULL == n ) return n;

    n = BSP_node_create(data, type);

    if ( NULL == data ) return n;

    n->data_type = type;
    n->data      = data;

    return n;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_node_clear( BSP_node_t * n )
{
    if ( NULL == n ) return bfalse;
    if (( NULL == n ) ) return btrue;

    BSP_node_dtor( n );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t BSP_tree_clear( BSP_tree_t * t )
{
    return NULL != BSP_tree_dtor( t );
}
