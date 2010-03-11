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

#include "egoboo.h"
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_ARY( BSP_leaf_ary, BSP_leaf_t );
IMPLEMENT_DYNAMIC_ARY( BSP_leaf_pary, BSP_leaf_t * );

IMPLEMENT_DYNAMIC_ARY( BSP_branch_ary, BSP_branch_t );
IMPLEMENT_DYNAMIC_ARY( BSP_branch_pary, BSP_branch_t * );

//--------------------------------------------------------------------------------------------
// private functions

static BSP_branch_t * BSP_tree_alloc_branch( BSP_tree_t * t );
static bool_t         BSP_tree_free_branch( BSP_tree_t * t, BSP_branch_t * B );
static bool_t         BSP_tree_remove_used( BSP_tree_t * t, BSP_branch_t * B );

static bool_t         BSP_tree_insert_leaf_rec( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth );
static bool_t         BSP_tree_free_branches( BSP_tree_t * t );

static bool_t BSP_leaf_list_insert( BSP_leaf_t ** lst, size_t * pcount, BSP_leaf_t * n );
static bool_t BSP_leaf_list_clear( BSP_leaf_t ** lst, size_t * pcount );
static bool_t BSP_leaf_list_collide( BSP_leaf_t * leaf_lst, size_t leaf_count, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if defined(__cplusplus)
s_BSP_aabb::s_BSP_aabb() { memset( this, 0, sizeof( *this ) ); };
s_BSP_aabb::s_BSP_aabb( size_t dim ) { BSP_aabb_ctor( this, dim ); };
s_BSP_aabb::~s_BSP_aabb()  { BSP_aabb_dtor( this ); };
#endif

BSP_aabb_t * BSP_aabb_ctor( BSP_aabb_t * pbb, size_t dim )
{
    if ( NULL == pbb ) return NULL;

    // initialize the memory
    memset( pbb, 0, sizeof( *pbb ) );

    float_ary_ctor( &( pbb->mins ), dim );
    float_ary_ctor( &( pbb->mids ), dim );
    float_ary_ctor( &( pbb->maxs ), dim );

    if ( 0 == pbb->mins.alloc || 0 == pbb->mids.alloc || 0 == pbb->maxs.alloc )
    {
        BSP_aabb_dtor( pbb );
    }
    else
    {
        pbb->dim = dim;
    }

    return pbb;
}

//--------------------------------------------------------------------------------------------
BSP_aabb_t * BSP_aabb_dtor( BSP_aabb_t * pbb )
{
    if ( NULL == pbb ) return NULL;

    // deallocate everything
    float_ary_dtor( &( pbb->mins ) );
    float_ary_dtor( &( pbb->mids ) );
    float_ary_dtor( &( pbb->maxs ) );

    // wipe it
    memset( pbb, 0, sizeof( *pbb ) );

    return pbb;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_empty( BSP_aabb_t * psrc )
{
    int cnt;

    if ( NULL == psrc || 0 == psrc->dim ) return btrue;

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        if ( psrc->maxs.ary[cnt] <= psrc->mins.ary[cnt] ) return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_clear( BSP_aabb_t * psrc )
{
    /// @details BB@> Return this bounding box to an empty state.

    int cnt;

    if( NULL == psrc ) return bfalse;
    if( NULL == psrc->mins.ary || NULL == psrc->mids.ary || NULL == psrc->maxs.ary ) return bfalse;

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        psrc->mins.ary[cnt] = psrc->mids.ary[cnt] = psrc->maxs.ary[cnt] = 0.0f;
    }
    
    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_lhs_contains_rhs( BSP_aabb_t * psrc1, BSP_aabb_t * psrc2 )
{
    /// @details BB@> Is psrc2 contained within psrc1? If psrc2 has less dimensions
    ///               than psrc1, just check the lowest common dimensions.

    int cnt;

    if ( NULL == psrc1 || NULL == psrc2 ) return bfalse;

    if ( psrc2->dim > psrc1->dim ) return bfalse;

    for ( cnt = 0; cnt < psrc2->dim; cnt++ )
    {
        if ( psrc2->mins.ary[cnt] < psrc1->mins.ary[cnt] ) return bfalse;
        if ( psrc2->maxs.ary[cnt] > psrc1->maxs.ary[cnt] ) return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_overlap( BSP_aabb_t * psrc1, BSP_aabb_t * psrc2 )
{
    /// @details BB@> Do psrc1 and psrc2 overlap? If psrc2 has less dimensions
    ///               than psrc1, just check the lowest common dimensions.

    int cnt;

    if ( NULL == psrc1 || NULL == psrc2 ) return bfalse;

    if ( psrc2->dim > psrc1->dim ) return bfalse;

    for ( cnt = 0; cnt < psrc2->dim; cnt++ )
    {
        if ( psrc2->maxs.ary[cnt] < psrc1->mins.ary[cnt] ) return bfalse;
        if ( psrc2->mins.ary[cnt] > psrc1->maxs.ary[cnt] ) return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_from_oct_bb( BSP_aabb_t * pdst, oct_bb_t * psrc )
{
    /// @details BB@> do an automatic conversion from an oct_bb_t to a BSP_aabb_t

    int cnt;

    if ( NULL == pdst || NULL == psrc ) return bfalse;

    if ( 0 == pdst->dim ) return bfalse;

    // this process is a little bit complicated because the
    // order to the OCT_* indices is optimized for a different test.
    if ( 1 == pdst->dim )
    {
        pdst->mins.ary[0] = psrc->mins[OCT_X];

        pdst->maxs.ary[0] = psrc->maxs[OCT_X];
    }
    else if ( 2 == pdst->dim )
    {
        pdst->mins.ary[0] = psrc->mins[OCT_X];
        pdst->mins.ary[1] = psrc->mins[OCT_Y];

        pdst->maxs.ary[0] = psrc->maxs[OCT_X];
        pdst->maxs.ary[1] = psrc->maxs[OCT_Y];
    }
    else if ( pdst->dim >= 3 )
    {
        pdst->mins.ary[0] = psrc->mins[OCT_X];
        pdst->mins.ary[1] = psrc->mins[OCT_Y];
        pdst->mins.ary[2] = psrc->mins[OCT_Z];

        pdst->maxs.ary[0] = psrc->maxs[OCT_X];
        pdst->maxs.ary[1] = psrc->maxs[OCT_Y];
        pdst->maxs.ary[2] = psrc->maxs[OCT_Z];

        // blank any extended dimensions
        for ( cnt = 3; cnt < pdst->dim; cnt++ )
        {
            pdst->mins.ary[cnt] = pdst->maxs.ary[cnt] = 0.0f;
        }
    }

    // find the mid values
    for ( cnt = 0; cnt < pdst->dim; cnt++ )
    {
        pdst->mids.ary[cnt] = 0.5f * ( pdst->mins.ary[cnt] + pdst->maxs.ary[cnt] );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if defined(__cplusplus)
s_BSP_leaf::s_BSP_leaf() { memset( this, 0, sizeof( *this ) ); } ;
s_BSP_leaf::~s_BSP_leaf() { BSP_leaf_dtor( this ); };
#endif

BSP_leaf_t * BSP_leaf_create( int dim, void * data, int type )
{
    BSP_leaf_t * rv;

    rv = EGOBOO_NEW( BSP_leaf_t );
    if ( NULL == rv ) return rv;

    return BSP_leaf_ctor( rv, dim, data, type );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_destroy( BSP_leaf_t ** ppleaf )
{
    if ( NULL == ppleaf || NULL == *ppleaf ) return bfalse;

    BSP_leaf_dtor( *ppleaf );

    EGOBOO_DELETE( *ppleaf );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_leaf_ctor( BSP_leaf_t * L, int dim, void * data, int type )
{
    if ( NULL == L ) return L;

    memset( L, 0, sizeof( *L ) );

    BSP_aabb_ctor( &( L->bbox ), dim );

    if ( NULL == data ) return L;

    L->data_type = type;
    L->data      = data;

    return L;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_dtor( BSP_leaf_t * L )
{
    if ( NULL == L ) return bfalse;

    L->data_type = -1;
    L->data      = NULL;

    BSP_aabb_dtor( &( L->bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if defined(__cplusplus)
s_BSP_branch::s_BSP_branch()           { memset( this, 0, sizeof( *this ) ); }
s_BSP_branch::s_BSP_branch( size_t dim ) { BSP_branch_ctor( this, dim ); }
s_BSP_branch::~s_BSP_branch()          { BSP_branch_dtor( this ); }
#endif

BSP_branch_t * BSP_branch_create( size_t dim )
{
    BSP_branch_t * rv;

    rv = EGOBOO_NEW( BSP_branch_t );
    if ( NULL == rv ) return rv;

    return BSP_branch_ctor( rv, dim );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_destroy( BSP_branch_t ** ppbranch )
{
    if ( NULL == ppbranch || NULL == *ppbranch ) return bfalse;

    BSP_branch_dtor( *ppbranch );

    EGOBOO_DELETE( *ppbranch );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_branch_create_ary( size_t ary_size, size_t dim )
{
    int cnt;
    BSP_branch_t * lst;

    lst = EGOBOO_NEW_ARY( BSP_branch_t, ary_size );
    if ( NULL == lst ) return lst;

    for ( cnt = 0; cnt < ary_size; cnt++ )
    {
        BSP_branch_ctor( lst + cnt, dim );
    }

    return lst;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_destroy_ary( size_t ary_size, BSP_branch_t ** lst )
{
    int cnt;

    if ( NULL == lst || NULL == *lst || 0 == ary_size ) return bfalse;

    for ( cnt = 0; cnt < ary_size; cnt++ )
    {
        BSP_branch_dtor(( *lst ) + cnt );
    }

    EGOBOO_DELETE_ARY( *lst );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_branch_ctor( BSP_branch_t * B, size_t dim )
{
    int child_count;

    if ( NULL == B ) return B;

    memset( B, 0, sizeof( *B ) );

    // determine the number of children from the number of dimensions
    child_count = 2 << ( dim - 1 );

    // allocate the child list
    B->child_lst = EGOBOO_NEW_ARY( BSP_branch_t*, child_count );
    if ( NULL != B->child_lst )
    {
        int cnt;
        for ( cnt = 0; cnt < child_count; cnt++ ) B->child_lst[cnt] = NULL;
        B->child_count = child_count;
    }

    // allocate the branch's bounding box
    BSP_aabb_ctor( &( B->bbox ), dim );

    return B;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_dtor( BSP_branch_t * B )
{
    if ( NULL == B ) return bfalse;

    EGOBOO_DELETE_ARY( B->child_lst );
    B->child_count = 0;

    // deallocate the branch's bounding box
    BSP_aabb_dtor( &( B->bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_unlink( BSP_branch_t * B )
{
    // remove any links to other leaves
    // assume the user knows what they are doing

    Uint32 i;

    if ( NULL == B ) return bfalse;

    // remove any nodes (from this branch only, not recursively)
    BSP_branch_free_nodes( B, bfalse );

    // completely unlink the branch
    B->parent   = NULL;
    B->node_lst = NULL;
    for ( i = 0; i < B->child_count; i++ )
    {
        if ( NULL != B->child_lst[i] )
        {
            B->child_lst[i]->parent = NULL;
        }
        B->child_lst[i] = NULL;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_insert_leaf( BSP_branch_t * B, BSP_leaf_t * n )
{
    return BSP_leaf_list_insert( &( B->node_lst ), &( B->node_count ), n );
}

//--------------------------------------------------------------------------------------------
bool_t  BSP_branch_insert_branch( BSP_branch_t * B, int index, BSP_branch_t * B2 )
{
    if ( NULL == B || index < 0 || index >= B->child_count ) return bfalse;

    if ( NULL == B2 ) return bfalse;

    if ( NULL != B->child_lst[ index ] )
    {
        return bfalse;
    }

    EGOBOO_ASSERT( B->depth + 1 == B2->depth );

    B->child_lst[ index ] = B2;
    B2->parent            = B;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_clear_nodes( BSP_branch_t * B, bool_t recursive )
{
    int cnt;

    if ( NULL == B ) return bfalse;

    if ( recursive )
    {
        // recursively clear out any nodes in the child_lst
        for ( cnt = 0; cnt < B->child_count; cnt++ )
        {
            if ( NULL == B->child_lst[cnt] ) continue;

            BSP_branch_clear_nodes( B->child_lst[cnt], btrue );
        };
    }

    // clear the node list
    B->node_count = 0;
    B->node_lst   = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_free_nodes( BSP_branch_t * B, bool_t recursive )
{
    int cnt;

    if ( NULL == B ) return bfalse;

    if ( recursive )
    {
        // recursively clear out any nodes in the child_lst
        for ( cnt = 0; cnt < B->child_count; cnt++ )
        {
            if ( NULL == B->child_lst[cnt] ) continue;

            BSP_branch_free_nodes( B->child_lst[cnt], btrue );
        };
    }

    // free all nodes of this branch
    BSP_leaf_list_clear( &( B->node_lst ), &( B->node_count ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_prune( BSP_tree_t * t, BSP_branch_t * B, bool_t recursive )
{
    /// @details BB@> remove all leaves with no child_lst. Do a depth first recursive search for efficiency

    size_t i;
    bool_t   retval;

    if ( NULL == B ) return bfalse;

    // prune all of the children 1st
    if ( recursive )
    {
        // prune all the children
        for ( i = 0; i < B->child_count; i++ )
        {
            BSP_branch_prune( t, B->child_lst[i], btrue );
        }
    }

    // do not remove the root node
    if ( B == t->root ) return btrue;

    retval = btrue;
    if ( BSP_branch_empty( B ) )
    {
        if ( NULL != B->parent )
        {
            bool_t found = bfalse;

            // unlink the parent and return the node to the free list
            for ( i = 0; i < B->parent->child_count; i++ )
            {
                if ( B->parent->child_lst[i] == B )
                {
                    B->parent->child_lst[i] = NULL;
                    found = btrue;
                    break;
                }
            }
            EGOBOO_ASSERT( found );
        }

        retval = BSP_branch_pary_push_back( &( t->branch_free ), B );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_add_all_nodes( BSP_branch_t * pbranch, BSP_leaf_pary_t * colst )
{
    int          cnt;
    size_t       colst_size;
    BSP_leaf_t * ptmp;

    if ( NULL == pbranch || NULL == colst ) return bfalse;

    colst_size = BSP_leaf_pary_get_size( colst );
    if ( 0 == colst_size || BSP_leaf_pary_get_top( colst ) >= colst_size ) return bfalse;

    // add any nodes in the node_lst
    for ( cnt = 0, ptmp = pbranch->node_lst; NULL != ptmp && cnt < pbranch->node_count; ptmp = ptmp->next, cnt++ )
    {
        if ( !BSP_leaf_pary_push_back( colst, ptmp ) ) break;
    }

    // if there is no more room. stop
    if ( BSP_leaf_pary_get_top( colst ) >= colst_size ) return bfalse;

    // add all nodes from all children
    for ( cnt = 0; cnt < pbranch->child_count; cnt++ )
    {
        BSP_branch_t * pchild = pbranch->child_lst[cnt];
        if ( NULL == pchild ) continue;

        BSP_branch_add_all_nodes( pchild, colst );

        if ( BSP_leaf_pary_get_top( colst ) >= colst_size ) break;
    }

    return ( BSP_leaf_pary_get_top( colst ) < colst_size );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_empty( BSP_branch_t * pbranch )
{
    int    cnt;
    bool_t empty;

    if ( NULL == pbranch ) return bfalse;

    // look to see if all children are free
    empty = btrue;
    for ( cnt = 0; cnt < pbranch->child_count; cnt++ )
    {
        if ( NULL != pbranch->child_lst[cnt] )
        {
            empty = bfalse;
            break;
        }
    }

    // check to see if there are any nodes in this branch's node_lst
    if ( NULL != pbranch->node_lst )
    {
        empty = bfalse;
    }

    return empty;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_collide( BSP_branch_t * pbranch, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst )
{
    /// @details BB@> Recursively search the BSP tree for collisions with the paabb
    //      Return bfalse if we need to break out of the recursive search for any reson.

    int          cnt;
    BSP_aabb_t * pbranch_bb;

    // if the collision list doesn't exist, stop
    if ( NULL == colst || 0 == colst->alloc || colst->top >= colst->alloc ) return bfalse;

    // if the branch doesn't exist, stop
    if ( NULL == pbranch ) return bfalse;
    pbranch_bb = &( pbranch->bbox );

    // is the branch completely contained by the test aabb?
    if ( BSP_aabb_lhs_contains_rhs( paabb, pbranch_bb ) )
    {
        // add every single node under this branch
        return BSP_branch_add_all_nodes( pbranch, colst );
    }

    // return if the object does not intersect the branch
    if ( !BSP_aabb_overlap( paabb, pbranch_bb ) )
    {
        // the branch and the object do not overlap at all.
        // do nothing.
        return bfalse;
    }

    // check the node_lst
    BSP_leaf_list_collide( pbranch->node_lst, pbranch->node_count, paabb, colst );

    // check for collisions with all child_lst branches
    for ( cnt = 0; cnt < pbranch->child_count; cnt++ )
    {
        BSP_branch_t * pchild = pbranch->child_lst[cnt];

        // scan all the child_lst
        if ( NULL == pchild ) continue;

        BSP_branch_collide( pchild, paabb, colst );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if defined(__cplusplus)
s_BSP_tree::s_BSP_tree() { memset( this, 0, sizeof( *this ) ); }
s_BSP_tree::s_BSP_tree( Sint32 dim, Sint32 depth ) { BSP_tree_ctor( this, dim, depth ); }
s_BSP_tree::~s_BSP_tree() { BSP_tree_dtor( this ); }
#endif

bool_t BSP_tree_init_0( BSP_tree_t * t )
{
    /// @details BB@> reset the tree to the "empty" state. Assume we do not own the node_lst or child_lst.

    size_t i;
    BSP_branch_t * pbranch;

    // free any the nodes in the tree
    BSP_tree_free_nodes( t, bfalse );

    // initialize the leaves.
    t->branch_free.top = 0;
    t->branch_used.top = 0;
    for ( i = 0; i < t->branch_all.alloc; i++ )
    {
        // grab a branch off of the static list
        pbranch = t->branch_all.ary + i;

        if ( NULL == pbranch ) continue;

        // completely unlink the branch
        BSP_branch_unlink( pbranch );

        // push it onto the "stack"
        BSP_branch_pary_push_back( &( t->branch_free ), pbranch );
    };

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_tree_t * BSP_tree_ctor( BSP_tree_t * t, Sint32 dim, Sint32 depth )
{
    int node_count, cnt;

    if ( NULL == t ) return t;

    memset( t, 0, sizeof( *t ) );

    node_count = BSP_tree_count_nodes( dim, depth );
    if ( node_count < 0 ) return t;

    if ( !BSP_tree_alloc( t, node_count, dim ) ) return t;

    t->depth = depth;

    //initialize the free list
    t->branch_free.top = 0;
    t->branch_used.top = 0;
    for ( cnt = 0; cnt < t->branch_all.alloc; cnt++ )
    {
        BSP_branch_pary_push_back( &( t->branch_free ), t->branch_all.ary + cnt );
    }

    return t;
}

//--------------------------------------------------------------------------------------------
BSP_tree_t * BSP_tree_dtor( BSP_tree_t * t )
{
    if ( NULL == t ) return NULL;

    BSP_tree_free( t );

    return t;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_alloc( BSP_tree_t * t, size_t count, size_t dim )
{
    int cnt;

    if ( NULL == t ) return bfalse;

    if ( NULL != t->branch_all.ary || t->branch_all.alloc > 0 ) return bfalse;

    // re-initialize the variables
    t->dimensions = 0;

    // allocate the branches
    BSP_branch_ary_ctor( &( t->branch_all ), count );
    if ( NULL == t->branch_all.ary || 0 == t->branch_all.alloc ) return bfalse;

    // initialize the array branches
    for ( cnt = 0; cnt < count; cnt++ )
    {
        BSP_branch_ctor( t->branch_all.ary + cnt, dim );
    }

    // allocate the aux arrays
    BSP_branch_pary_ctor( &( t->branch_used ), count );
    BSP_branch_pary_ctor( &( t->branch_free ), count );

    // initialize the root bounding box
    BSP_aabb_ctor( &( t->bbox ), dim );

    // set the variables
    t->dimensions = dim;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_free( BSP_tree_t * t )
{
    size_t i;

    if ( NULL == t ) return bfalse;

    if ( NULL == t->branch_all.ary || 0 == t->branch_all.alloc ) return btrue;

    // destruct the branches
    for ( i = 0; i < t->branch_all.alloc; i++ )
    {
        BSP_branch_dtor( t->branch_all.ary + i );
    }

    // deallocate the branches
    BSP_branch_ary_dtor( &( t->branch_all ) );

    // deallocate the aux arrays
    BSP_branch_pary_dtor( &( t->branch_used ) );
    BSP_branch_pary_dtor( &( t->branch_free ) );

    // deallocate the root bounding box
    BSP_aabb_dtor( &( t->bbox ) );

    return btrue;
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
bool_t BSP_tree_remove_used( BSP_tree_t * t, BSP_branch_t * B )
{
    int cnt;

    if ( NULL == t || 0 == t->branch_used.top ) return bfalse;

    if ( NULL == B ) return bfalse;

    // scan the used list for the branch
    for ( cnt = 0; cnt < t->branch_used.top; cnt++ )
    {
        if ( B == t->branch_used.ary[cnt] ) break;
    }

    // did we find the branch in the used list?
    if ( cnt == t->branch_used.top ) return bfalse;

    // reduce the size of the list
    t->branch_used.top--;

    // move the branch that we found to the top of the list
    SWAP( BSP_branch_t *, t->branch_used.ary[cnt], t->branch_used.ary[t->branch_used.top] );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_tree_alloc_branch( BSP_tree_t * t )
{
    BSP_branch_t ** pB = NULL, * B = NULL;

    if ( NULL == t ) return NULL;

    // grab the top branch
    pB = BSP_branch_pary_pop_back( &( t->branch_free ) );
    if ( NULL == pB ) return NULL;

    B = *pB;
    if ( NULL == B ) return NULL;

    // add it to the used list
    BSP_branch_pary_push_back( &( t->branch_used ), B );

    return B;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_free_branch( BSP_tree_t * t, BSP_branch_t * B )
{
    bool_t retval;

    if ( NULL == t || NULL == B ) return bfalse;

    retval = bfalse;
    if ( BSP_tree_remove_used( t, B ) )
    {
        // add it to the used list
        retval =  BSP_branch_pary_push_back( &( t->branch_free ), B );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_tree_get_free( BSP_tree_t * t )
{
    BSP_branch_t *  B;

    // try to get a branch from our pre-allocated list. do all necessary book-keeping
    B = BSP_tree_alloc_branch( t );
    if ( NULL == B ) return NULL;

    if ( NULL != B )
    {
        // make sure that this branch does not have data left over
        // from its last use
        EGOBOO_ASSERT( NULL == B->node_lst );

        // make sure that the data is cleared out
        BSP_branch_unlink( B );
    }

    return B;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_add_free( BSP_tree_t * t, BSP_branch_t * B )
{
    if ( NULL == t || NULL == B ) return bfalse;

    // remove any links to other leaves
    BSP_branch_unlink( B );

    return BSP_tree_free_branch( t, B );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_free_all( BSP_tree_t * t )
{
    if ( !BSP_tree_free_nodes( t, bfalse ) ) return bfalse;

    if ( !BSP_tree_free_branches( t ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_clear_nodes( BSP_tree_t * t, bool_t recursive )
{
    if ( NULL == t ) return bfalse;

    if ( recursive )
    {
        BSP_branch_clear_nodes( t->root, btrue );
    }

    // free the infinite nodes of the tree
    t->infinite_count = 0;
    t->infinite = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_free_nodes( BSP_tree_t * t, bool_t recursive )
{
    if ( NULL == t ) return bfalse;

    if ( recursive )
    {
        BSP_branch_free_nodes( t->root, btrue );
    }

    // free the infinite nodes of the tree
    BSP_leaf_list_clear( &( t->infinite ), &( t->infinite_count ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_free_branches( BSP_tree_t * t )
{
    int cnt;

    if ( NULL == t ) return bfalse;

    // transfer all the "used" branches back to the "free" branches
    for ( cnt = 0; cnt < t->branch_used.top; cnt++ )
    {
        // grab a used branch
        BSP_branch_t * pbranch = t->branch_used.ary[cnt];
        if ( NULL == pbranch ) continue;

        // completely unlink the branch
        BSP_branch_unlink( pbranch );

        // return the branch to the free list
        BSP_branch_pary_push_back( &( t->branch_free ), pbranch );
    }

    // reset the used list
    t->branch_used.top = 0;

    // remove the tree root
    t->root = NULL;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t   BSP_tree_prune( BSP_tree_t * t )
{
    /// @details BB@> remove all leaves with no child_lst or node_lst.

    int cnt;

    if ( NULL == t || NULL == t->root ) return bfalse;

    // search through all allocated branches. This will not catch all of the
    // empty branches every time, but it should catch quite a few
    for ( cnt = 0; cnt < t->branch_used.top; cnt++ )
    {
        BSP_tree_prune_branch( t, cnt );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_tree_ensure_root( BSP_tree_t * t )
{
    int cnt;

    BSP_branch_t * proot;

    if ( NULL == t ) return NULL;

    if ( NULL != t->root ) return t->root;

    proot = BSP_tree_get_free( t );
    if ( NULL == proot ) return NULL;

    // make sure that it is unlinked
    BSP_branch_unlink( proot );

    // copy the tree bounding box to the root node
    for ( cnt = 0; cnt < t->dimensions; cnt++ )
    {
        proot->bbox.mins.ary[cnt] = t->bbox.mins.ary[cnt];
        proot->bbox.mids.ary[cnt] = t->bbox.mids.ary[cnt];
        proot->bbox.maxs.ary[cnt] = t->bbox.maxs.ary[cnt];
    }

    // fix the depth
    proot->depth = 0;

    // assign the root to the tree
    t->root = proot;

    return proot;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_tree_ensure_branch( BSP_tree_t * t, BSP_branch_t * B, int index )
{
    BSP_branch_t * pbranch;

    if (( NULL == t ) || ( NULL == B ) ) return NULL;
    if ( index < 0 || ( signed )index > ( signed )B->child_count ) return NULL;

    // grab any existing value
    pbranch = B->child_lst[index];

    // if this branch doesn't exist, create it and insert it properly.
    if ( NULL == pbranch )
    {
        // grab a free branch
        pbranch = BSP_tree_get_free( t );

        if ( NULL != pbranch )
        {
            // make sure that it is unlinked
            BSP_branch_unlink( pbranch );

            // generate its bounding box
            pbranch->depth = B->depth + 1;
            BSP_generate_aabb( &( B->bbox ), index, &( pbranch->bbox ) );

            // insert it in the correct position
            BSP_branch_insert_branch( B, index, pbranch );
        }
    }

    return pbranch;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_insert( BSP_tree_t * t, BSP_branch_t * B, BSP_leaf_t * n, int index )
{
    bool_t retval;

    if (( NULL == t ) || ( NULL == B ) || ( NULL == n ) ) return bfalse;
    if (( signed )index > ( signed )B->child_count ) return bfalse;

    if ( index >= 0 && NULL != B->child_lst[index] )
    {
        // inserting a node into the child
        return BSP_branch_insert_leaf( B->child_lst[index], n );
    }

    if ( index < 0 || 0 == t->branch_free.top )
    {
        // inserting a node into this branch node
        // this can either occur because someone requested it (index < 0)
        // OR because there are no more free nodes
        retval = BSP_branch_insert_leaf( B, n );
    }
    else
    {
        // the requested B->child_lst[index] slot is empty. grab a pre-allocated
        // BSP_branch_t from the free list in the BSP_tree_t structure an insert it in
        // this child node

        BSP_branch_t * pbranch = BSP_tree_ensure_branch( t, B, index );

        retval = BSP_branch_insert_leaf( pbranch, n );
    }

    // something went wrong ?
    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_insert_infinite( BSP_tree_t * ptree, BSP_leaf_t * pleaf )
{
    return BSP_leaf_list_insert( &( ptree->infinite ), &( ptree->infinite_count ), pleaf );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_insert_leaf( BSP_tree_t * ptree, BSP_leaf_t * pleaf )
{
    bool_t retval;

    if ( NULL == ptree || NULL == pleaf ) return bfalse;

    if ( !BSP_aabb_lhs_contains_rhs( &( ptree->bbox ), &( pleaf->bbox ) ) )
    {
        // put the leaf at the head of the intinite list
        retval = BSP_tree_insert_infinite( ptree, pleaf );
    }
    else
    {
        BSP_branch_t * proot = BSP_tree_ensure_root( ptree );

        retval = BSP_tree_insert_leaf_rec( ptree, proot, pleaf, 0 );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_insert_leaf_rec( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth )
{
    /// @details BB@> recursively insert a leaf in a tree of BSP_branch_t*. Get new branches using the
    ///              BSP_tree_get_free() function to allocate any new branches that are needed.

    int    cnt, index;
    bool_t retval, fail;

    BSP_branch_t * pchild;

    if ( NULL == ptree || NULL == pbranch || NULL == pleaf ) return bfalse;

    // keep track of the tree depth
    depth++;

    // don't go too deep
    if ( depth > ptree->depth )
    {
        // insert the node under this branch
        return BSP_branch_insert_leaf( pbranch, pleaf );
    }

    //---- determine which child the leaf needs to go under
    /// @note This function is not optimal, since we encode the comparisons
    /// in the 32-bit integer, indes, and then may have to decimate index to construct
    /// the child  branch's bounding by calling BSP_generate_aabb().
    /// The reason that it is done this way is that we would have to be dynamically
    /// allocating and deallocating memory every time this function is called, otherwise. Big waste of time.
    fail  = bfalse;
    index = 0;
    for ( cnt = 0; cnt < ptree->dimensions; cnt++ )
    {
        if ( pleaf->bbox.mins.ary[cnt] >= pbranch->bbox.mins.ary[cnt] && pleaf->bbox.maxs.ary[cnt] <= pbranch->bbox.mids.ary[cnt] )
        {
            index <<= 1;
            index |= 0;
        }
        else if ( pleaf->bbox.mins.ary[cnt] >= pbranch->bbox.mids.ary[cnt] && pleaf->bbox.maxs.ary[cnt] <= pbranch->bbox.maxs.ary[cnt] )
        {
            index <<= 1;
            index |= 1;
        }
        else if ( pleaf->bbox.mins.ary[cnt] >= pbranch->bbox.mins.ary[cnt] && pleaf->bbox.maxs.ary[cnt] <= pbranch->bbox.maxs.ary[cnt] )
        {
            // this leaf belongs at this node
            break;
        }
        else
        {
            // this leaf is actually bigger than this branch
            fail = btrue;
            break;
        }
    }

    if ( fail )
    {
        // we cannot place this the node under this branch
        return bfalse;
    }

    //---- insert the leaf in the right place
    retval = bfalse;
    if ( cnt < ptree->dimensions )
    {
        // place this node at this index
        retval = BSP_branch_insert_leaf( pbranch, pleaf );
    }
    else if ( index < pbranch->child_count )
    {
        bool_t created;

        pchild = pbranch->child_lst[index];

        created = bfalse;
        if ( NULL == pchild )
        {
            pchild = BSP_tree_ensure_branch( ptree, pbranch, index );

            created = btrue;
        }

        EGOBOO_ASSERT( pchild->depth == pbranch->depth + 1 );

        if ( NULL != pchild )
        {
            // insert the leaf
            retval = BSP_tree_insert_leaf_rec( ptree, pchild, pleaf, depth );
        }
    }

    // not necessary, but best to be thorough
    depth--;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_prune_branch( BSP_tree_t * t, int cnt )
{
    /// @details BB@> an optimized version of iterating through the t->branch_used list
    //                and then calling BSP_branch_prune() on the empty branch. In the old method,
    //                the t->branch_used list was searched twice to find each empty branch. This
    //                function does it only once.

    size_t i;
    bool_t remove;

    BSP_branch_t * B;

    if ( NULL == t || cnt < 0 || cnt >= t->branch_used.top ) return bfalse;

    B = t->branch_used.ary[ cnt ];
    if ( NULL == B ) return bfalse;

    // do not remove the root node
    if ( B == t->root ) return btrue;

    remove = bfalse;
    if ( BSP_branch_empty( B ) )
    {
        if ( NULL != B->parent )
        {
            bool_t found = bfalse;

            // unlink this node from its parent
            for ( i = 0; i < B->parent->child_count; i++ )
            {
                if ( B->parent->child_lst[i] == B )
                {
                    // remove it from the parent's list of children
                    B->parent->child_lst[i] = NULL;

                    // blank out the parent
                    B->parent = NULL;

                    // let them know that we found ourself
                    found = btrue;

                    break;
                }
            }

            // not finding yourself is an error
            EGOBOO_ASSERT( found );
        }

        remove = btrue;
    }

    if ( remove )
    {
        // reduce the size of the list
        t->branch_used.top--;

        // set B's data to "safe" values
        B->parent = NULL;
        for( i = 0; i < B->child_count; i++ ) B->child_lst[i] = NULL;
        B->node_count = 0;
        B->node_lst = NULL;
        B->depth = -1;
        BSP_aabb_clear( &(B->bbox) );

        // move the branch that we found to the top of the list
        SWAP( BSP_branch_t *, t->branch_used.ary[cnt], t->branch_used.ary[t->branch_used.top] );

        // add the branch to the free list
        BSP_branch_pary_push_back( &( t->branch_free ), B );
    }

    return remove;
}

//--------------------------------------------------------------------------------------------
int BSP_tree_collide( BSP_tree_t * tree, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst )
{
    /// @details BB@> fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == tree || NULL == paabb ) return 0;

    if ( NULL == colst ) return 0;
    colst->top = 0;
    if ( 0 == colst->alloc ) return 0;

    // collide with any "infinite" nodes
    BSP_leaf_list_collide( tree->infinite, tree->infinite_count, paabb, colst );

    // collise with the rest of the tree
    BSP_branch_collide( tree->root, paabb, colst );

    return colst->top;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_insert( BSP_leaf_t ** lst, size_t * pcount, BSP_leaf_t * n )
{
    /// @details BB@> Insert a leaf in the list, making sure there are no duplicates.
    ///               Duplicates will cause loops in the list and make it impossible to
    ///               traverse properly.

    int cnt;
    BSP_leaf_t * ptmp;

    if ( NULL == lst || NULL == pcount || NULL == n ) return bfalse;

    if ( NULL == *lst )
    {
        *lst    = n;
        *pcount = 1;
        n->next = NULL;

        return btrue;
    }

    for ( cnt = 0, ptmp = *lst;
          NULL != ptmp->next && cnt < *pcount;
          cnt++, ptmp = ptmp->next )
    {
        // do not insert duplicates, or we have a big problem
        if ( n == ptmp )
        {
            return bfalse;
        }
    }

    EGOBOO_ASSERT( NULL == ptmp->next );
    n->next = NULL;
    ptmp->next = n;

    ( *pcount )++;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_clear( BSP_leaf_t ** lst, size_t * pcount )
{
    /// @details BB@> Clear out the leaf list.

    int cnt;
    BSP_leaf_t * ptmp;

    if ( NULL == lst || NULL == pcount ) return bfalse;

    if ( 0 == *pcount )
    {
        EGOBOO_ASSERT( NULL == *lst );
        return btrue;
    }

    for ( cnt = 0;  NULL != *lst && cnt < *pcount; cnt++ )
    {
        // pop a node off the stack
        ptmp = *lst;
        *lst = ptmp->next;

        // clean up the node
        ptmp->next = NULL;
    };

    EGOBOO_ASSERT( NULL == *lst );

    *pcount = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_collide( BSP_leaf_t * leaf_lst, size_t leaf_count, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst )
{
    /// @details BB@> check for collisions with the given node list

    int          cnt;
    BSP_leaf_t * pleaf;
    bool_t       retval;
    size_t       colst_size;

    if ( NULL == leaf_lst || 0 == leaf_count ) return bfalse;
    if ( NULL == paabb ) return bfalse;

    colst_size = BSP_leaf_pary_get_size( colst );
    if ( 0 == colst_size || BSP_leaf_pary_get_top( colst ) >= colst_size )
        return bfalse;

    for ( cnt = 0, pleaf = leaf_lst;
          cnt < leaf_count && NULL != pleaf;
          cnt++, pleaf = pleaf->next )
    {
        BSP_aabb_t * pleaf_bb = &( pleaf->bbox );

        EGOBOO_ASSERT( pleaf->data_type >= 0 );

        if ( BSP_aabb_overlap( paabb, pleaf_bb ) )
        {
            // we have a possible intersection
            if ( !BSP_leaf_pary_push_back( colst, pleaf ) ) break;
        }
    }

    retval = ( BSP_leaf_pary_get_top( colst ) < colst_size );

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t BSP_generate_aabb( BSP_aabb_t * psrc, int index, BSP_aabb_t * pdst )
{
    int cnt, tnc, child_lst;

    // valid source?
    if ( NULL == psrc || psrc->dim <= 0 ) return bfalse;

    // valid destination?
    if ( NULL == pdst ) return bfalse;

    // valid index?
    child_lst = 2 << ( psrc->dim - 1 );
    if ( index < 0 || index >= child_lst ) return bfalse;

    // make sure that the destination type matches the source type
    if ( pdst->dim != psrc->dim )
    {
        BSP_aabb_dtor( pdst );
        BSP_aabb_ctor( pdst, psrc->dim );
    }

    // determine the bounds
    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        float maxval, minval;

        tnc = (( int )psrc->dim ) - 1 - cnt;

        if ( 0 == ( index & ( 1 << tnc ) ) )
        {
            minval = psrc->mins.ary[cnt];
            maxval = psrc->mids.ary[cnt];
        }
        else
        {
            minval = psrc->mids.ary[cnt];
            maxval = psrc->maxs.ary[cnt];
        }

        pdst->mins.ary[cnt] = minval;
        pdst->maxs.ary[cnt] = maxval;
        pdst->mids.ary[cnt] = 0.5f * ( minval + maxval );
    }

    return btrue;
}
