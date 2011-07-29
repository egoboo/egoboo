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

#include "../egolib/bsp.inl"
#include "../egolib/log.h"

#include "../egolib/frustum.h"

// this include must be the absolute last include
#include "../egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define BRANCH_NODE_THRESHOLD 5

//--------------------------------------------------------------------------------------------
// special arrays
//--------------------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_ARY( BSP_leaf_ary, BSP_leaf_t );
IMPLEMENT_DYNAMIC_ARY( BSP_leaf_pary, BSP_leaf_t * );

IMPLEMENT_DYNAMIC_ARY( BSP_branch_ary, BSP_branch_t );
IMPLEMENT_DYNAMIC_ARY( BSP_branch_pary, BSP_branch_t * );

//--------------------------------------------------------------------------------------------
// special functions

static bool_t _generate_BSP_aabb_child( BSP_aabb_t * psrc, int index, BSP_aabb_t * pdst );
static int    _find_child_index( const BSP_aabb_t * pbranch_aabb, const aabb_t * pleaf_aabb );

//--------------------------------------------------------------------------------------------
// BSP_tree_t private functions

static BSP_branch_t * BSP_tree_alloc_branch( BSP_tree_t * t );

static bool_t BSP_tree_insert_infinite( BSP_tree_t * ptree, BSP_leaf_t * pleaf );

//static bool_t         BSP_tree_free_branch( BSP_tree_t * t, BSP_branch_t * B );
//static bool_t         BSP_tree_remove_used( BSP_tree_t * t, BSP_branch_t * B );
//static bool_t         BSP_tree_free_branches( BSP_tree_t * t );

//--------------------------------------------------------------------------------------------
// BSP_branch_t private functions

static bool_t BSP_branch_insert_leaf_rec( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth );
static bool_t BSP_branch_insert_branch_list_rec( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int index, int depth );
static bool_t BSP_branch_insert_leaf_list( BSP_branch_t * B, BSP_leaf_t * n );
static bool_t BSP_branch_insert_branch( BSP_branch_t * B, int index, BSP_branch_t * B2 );

static bool_t BSP_branch_prune( BSP_tree_t * t, BSP_branch_t * B, bool_t recursive );
static bool_t BSP_branch_collide_aabb( const BSP_branch_t * pbranch, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
static bool_t BSP_branch_collide_frustum( const BSP_branch_t * pbranch, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );

static int BSP_branch_insert_leaf_rec_1( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth );

//--------------------------------------------------------------------------------------------
// BSP_leaf_t private functions

static bool_t BSP_leaf_list_collide_aabb( const BSP_leaf_list_t * LL, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
static bool_t BSP_leaf_list_collide_frustum( const BSP_leaf_list_t * LL, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
static BSP_leaf_t * BSP_leaf_create( void * data, int type, int index );
static bool_t BSP_leaf_destroy( BSP_leaf_t ** ppleaf );

//--------------------------------------------------------------------------------------------
// Generic BSP functions
//--------------------------------------------------------------------------------------------
bool_t _generate_BSP_aabb_child( BSP_aabb_t * psrc, int index, BSP_aabb_t * pdst )
{
    size_t cnt;
    signed tnc, child_count;

    // valid source?
    if ( NULL == psrc || psrc->dim <= 0 ) return bfalse;

    // valid destination?
    if ( NULL == pdst ) return bfalse;

    // valid index?
    child_count = 2 << ( psrc->dim - 1 );
    if ( index < 0 || index >= child_count ) return bfalse;

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

        tnc = (( signed )psrc->dim ) - 1 - cnt;

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

//--------------------------------------------------------------------------------------------
int _find_child_index( const BSP_aabb_t * pbranch_aabb, const aabb_t * pleaf_aabb )
{
    //---- determine which child the leaf needs to go under

    /// @note This function is not optimal, since we encode the comparisons
    /// in the 32-bit integer indices, and then may have to decimate index to construct
    /// the child  branch's bounding by calling _generate_BSP_aabb_child().
    /// The reason that it is done this way is that we would have to be dynamically
    /// allocating and deallocating memory every time this function is called, otherwise. Big waste of time.

    size_t cnt;
    int    index;

    const float * branch_min_ary, * branch_mid_ary, * branch_max_ary;
    const float * leaf_min_ary, * leaf_max_ary;

    // get aliases to the leaf aabb data
    if ( NULL == pleaf_aabb ) return -2;
    leaf_min_ary = pleaf_aabb->mins;
    leaf_max_ary = pleaf_aabb->maxs;

    // get aliases to the branch aabb data
    if ( NULL == pbranch_aabb || 0 == pbranch_aabb->dim || !pbranch_aabb->valid )
        return -2;

    if ( NULL == pbranch_aabb->mins.ary || 0 == pbranch_aabb->mins.alloc ) return -2;
    branch_min_ary = pbranch_aabb->mins.ary;

    if ( NULL == pbranch_aabb->mids.ary || 0 == pbranch_aabb->mids.alloc ) return -2;
    branch_mid_ary = pbranch_aabb->mids.ary;

    if ( NULL == pbranch_aabb->maxs.ary || 0 == pbranch_aabb->maxs.alloc ) return -2;
    branch_max_ary = pbranch_aabb->maxs.ary;

    index = 0;
    for ( cnt = 0; cnt < pbranch_aabb->dim; cnt++ )
    {
        index <<= 1;

        if ( leaf_min_ary[cnt] >= branch_min_ary[cnt] && leaf_max_ary[cnt] <= branch_mid_ary[cnt] )
        {
            // the following code does nothing
            index |= 0;
        }
        else if ( leaf_min_ary[cnt] >= branch_mid_ary[cnt] && leaf_max_ary[cnt] <= branch_max_ary[cnt] )
        {
            index |= 1;
        }
        else if ( leaf_min_ary[cnt] >= branch_min_ary[cnt] && leaf_max_ary[cnt] <= branch_max_ary[cnt] )
        {
            // this leaf belongs at this node
            index = -1;
            break;
        }
        else
        {
            // this leaf is actually bigger than this branch
            index = -2;
            break;
        }
    }

    return index;
}

//--------------------------------------------------------------------------------------------
// BSP_aabb_t
//--------------------------------------------------------------------------------------------
BSP_aabb_t * BSP_aabb_ctor( BSP_aabb_t * pbb, size_t dim )
{
    if ( NULL == pbb ) return NULL;

    // initialize the memory
    BLANK_STRUCT_PTR( pbb )

    // allocate memory and clear it
    BSP_aabb_alloc( pbb, dim );

    return pbb;
}

//--------------------------------------------------------------------------------------------
BSP_aabb_t * BSP_aabb_dtor( BSP_aabb_t * pbb )
{
    if ( NULL == pbb ) return NULL;

    // deallocate everything
    pbb = BSP_aabb_dealloc( pbb );

    // wipe it
    BLANK_STRUCT_PTR( pbb )

    return pbb;
}

//--------------------------------------------------------------------------------------------
BSP_aabb_t * BSP_aabb_alloc( BSP_aabb_t * pbb, size_t dim )
{
    if ( NULL == pbb ) return pbb;

    pbb->dim = 0;

    float_ary_ctor( &( pbb->mins ), dim );
    float_ary_ctor( &( pbb->mids ), dim );
    float_ary_ctor( &( pbb->maxs ), dim );

    if ( dim != pbb->mins.alloc || dim != pbb->mids.alloc || dim != pbb->maxs.alloc )
    {
        BSP_aabb_dealloc( pbb );
    }
    else
    {
        pbb->dim = dim;
        BSP_aabb_self_clear( pbb );
    }

    BSP_aabb_validate( pbb );

    return pbb;
}

//--------------------------------------------------------------------------------------------
BSP_aabb_t * BSP_aabb_dealloc( BSP_aabb_t * pbb )
{
    if ( NULL == pbb ) return pbb;

    // deallocate everything
    float_ary_dtor( &( pbb->mins ) );
    float_ary_dtor( &( pbb->mids ) );
    float_ary_dtor( &( pbb->maxs ) );

    pbb->dim = 0;
    pbb->valid = bfalse;

    return pbb;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_empty( const BSP_aabb_t * psrc )
{
    Uint32 cnt;

    if ( NULL == psrc || 0 == psrc->dim  || !psrc->valid ) return btrue;

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        if ( psrc->maxs.ary[cnt] <= psrc->mins.ary[cnt] )
            return btrue;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_self_clear( BSP_aabb_t * psrc )
{
    /// @details BB@> Return this bounding box to an empty state.

    Uint32 cnt;

    if ( NULL == psrc ) return bfalse;

    if ( psrc->dim <= 0 || NULL == psrc->mins.ary || NULL == psrc->mids.ary || NULL == psrc->maxs.ary )
    {
        BSP_aabb_invalidate( psrc );
        return bfalse;
    }

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        psrc->mins.ary[cnt] = psrc->mids.ary[cnt] = psrc->maxs.ary[cnt] = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_from_oct_bb( BSP_aabb_t * pdst, const oct_bb_t * psrc )
{
    /// @details BB@> do an automatic conversion from an oct_bb_t to a BSP_aabb_t

    Uint32 cnt;

    if ( NULL == pdst || NULL == psrc ) return bfalse;

    BSP_aabb_invalidate( pdst );

    if ( pdst->dim <= 0 ) return bfalse;

    // this process is a little bit complicated because the
    // order to the OCT_* indices is optimized for a different test.
    if ( 1 == pdst->dim )
    {
        pdst->mins.ary[kX] = psrc->mins[OCT_X];

        pdst->maxs.ary[kX] = psrc->maxs[OCT_X];
    }
    else if ( 2 == pdst->dim )
    {
        pdst->mins.ary[kX] = psrc->mins[OCT_X];
        pdst->mins.ary[kY] = psrc->mins[OCT_Y];

        pdst->maxs.ary[kX] = psrc->maxs[OCT_X];
        pdst->maxs.ary[kY] = psrc->maxs[OCT_Y];
    }
    else if ( pdst->dim >= 3 )
    {
        pdst->mins.ary[kX] = psrc->mins[OCT_X];
        pdst->mins.ary[kY] = psrc->mins[OCT_Y];
        pdst->mins.ary[kZ] = psrc->mins[OCT_Z];

        pdst->maxs.ary[kX] = psrc->maxs[OCT_X];
        pdst->maxs.ary[kY] = psrc->maxs[OCT_Y];
        pdst->maxs.ary[kZ] = psrc->maxs[OCT_Z];

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

    BSP_aabb_validate( pdst );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_validate( BSP_aabb_t * psrc )
{
    size_t cnt;

    if ( NULL == psrc ) return bfalse;

    // set it to valid
    psrc->valid = btrue;

    // check to see if any dimension is inverted
    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        if ( psrc->maxs.ary[cnt] < psrc->mids.ary[cnt] )
        {
            psrc->valid = bfalse;
            break;
        }
        if ( psrc->maxs.ary[cnt] < psrc->mins.ary[cnt] )
        {
            psrc->valid = bfalse;
            break;
        }
        if ( psrc->mids.ary[cnt] < psrc->mins.ary[cnt] )
        {
            psrc->valid = bfalse;
            break;
        }
    }

    return psrc->valid;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_invalidate( BSP_aabb_t * psrc )
{
    if ( NULL == psrc ) return bfalse;

    // set it to valid
    psrc->valid = bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_copy( BSP_aabb_t * pdst, const BSP_aabb_t * psrc )
{
    size_t cnt;

    if ( NULL == pdst ) return bfalse;

    if ( NULL == psrc )
    {
        BSP_aabb_dtor( pdst );
        return bfalse;
    }

    // ensure that they have the same dimensions
    if ( pdst->dim != psrc->dim )
    {
        BSP_aabb_dealloc( pdst );
        BSP_aabb_alloc( pdst, psrc->dim );
    }

    for ( cnt = 0; cnt < psrc->dim; cnt++ )
    {
        pdst->mins.ary[cnt] = psrc->mins.ary[cnt];
        pdst->mids.ary[cnt] = psrc->mids.ary[cnt];
        pdst->maxs.ary[cnt] = psrc->maxs.ary[cnt];
    }

    BSP_aabb_validate( pdst );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_aabb_self_union( BSP_aabb_t * pdst, const BSP_aabb_t * psrc )
{
    size_t min_dim, cnt;

    if ( NULL == pdst ) return bfalse;

    if ( NULL == psrc )
    {
        return BSP_aabb_validate( pdst );
    }

    min_dim = MIN( psrc->dim, pdst->dim );

    for ( cnt = 0; cnt < min_dim; cnt++ )
    {
        pdst->mins.ary[cnt] = MIN( pdst->mins.ary[cnt], psrc->mins.ary[cnt] );
        pdst->maxs.ary[cnt] = MAX( pdst->maxs.ary[cnt], psrc->maxs.ary[cnt] );

        pdst->mids.ary[cnt] = 0.5f * ( pdst->mins.ary[cnt] + pdst->maxs.ary[cnt] );
    }

    return BSP_aabb_validate( pdst );
}

//--------------------------------------------------------------------------------------------
// BSP_leaf_t
//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_leaf_create( void * data, int type, int index )
{
    BSP_leaf_t * rv;

    rv = EGOBOO_NEW( BSP_leaf_t );
    if ( NULL == rv ) return rv;

    return BSP_leaf_ctor( rv, data, type, index );
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
BSP_leaf_t * BSP_leaf_ctor( BSP_leaf_t * L, void * data, int type, int index )
{
    if ( NULL == L ) return L;

    BLANK_STRUCT_PTR( L )

    if ( NULL == data ) return L;

    L->data_type = type;
    L->data      = data;
    L->index     = index;

    ego_aabb_ctor( &( L->bbox ) );

    return L;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_leaf_dtor( BSP_leaf_t * L )
{
    if ( NULL == L ) return L;

    L->inserted  = bfalse;
    L->data_type = -1;
    L->data      = NULL;

    return L;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_unlink( BSP_leaf_t * L )
{
    bool_t retval;

    if ( NULL == L ) return bfalse;

    retval = bfalse;

    if ( L->inserted )
    {
        L->inserted = bfalse;
        retval = btrue;
    }

    if ( NULL != L->next )
    {
        L->next = NULL;
        retval = btrue;
    }

    ego_aabb_self_clear( &( L->bbox ) );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_clear( BSP_leaf_t * L )
{
    if ( NULL == L ) return bfalse;

    L->data_type = BSP_LEAF_NONE;
    L->data      = NULL;

    BSP_leaf_unlink( L );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_copy( BSP_leaf_t * dst, const BSP_leaf_t * src )
{
    if ( NULL == dst ) return bfalse;

    if ( NULL == src )
    {
        BLANK_STRUCT_PTR( dst );
    }
    else
    {
        memmove( dst, src, sizeof( *dst ) );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// BSP_branch_t
//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_branch_ctor( BSP_branch_t * B, size_t dim )
{
    if ( NULL == B ) return B;

    BLANK_STRUCT_PTR( B )

    BSP_branch_alloc( B, dim );

    return B;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_branch_dtor( BSP_branch_t * B )
{
    if ( NULL == B ) return B;

    BSP_branch_dealloc( B );

    BLANK_STRUCT_PTR( B )

    return B;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_alloc( BSP_branch_t * B, size_t dim )
{
    bool_t child_rv, nodes_rv, unsorted_rv;

    if ( NULL == B ) return bfalse;

    // allocate the branch's children
    child_rv = BSP_branch_list_alloc( &( B->children ), dim );

    // allocate the branch's nodes
    nodes_rv = BSP_leaf_list_alloc( &( B->nodes ) );

    // allocate the branch's nodes
    unsorted_rv = BSP_leaf_list_alloc( &( B->unsorted ) );

    // allocate the branch's bounding box
    BSP_aabb_alloc( &( B->bsp_bbox ), dim );

    return child_rv && nodes_rv && unsorted_rv && ( dim == B->bsp_bbox.dim );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_dealloc( BSP_branch_t * B )
{
    if ( NULL == B ) return bfalse;

    // deallocate the list of children
    BSP_branch_list_dtor( &( B->children ) );

    // deallocate the branch's nodes
    BSP_leaf_list_dealloc( &( B->nodes ) );

    // deallocate the list of nodes
    BSP_leaf_list_dtor( &( B->nodes ) );

    // deallocate the branch's unsorted nodes
    BSP_leaf_list_dealloc( &( B->unsorted ) );

    // deallocate the list of unsorted nodes
    BSP_leaf_list_dtor( &( B->unsorted ) );

    // deallocate the bounding box
    BSP_aabb_dtor( &( B->bsp_bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_unlink_all( BSP_branch_t * B )
{
    bool_t retval = btrue;

    if ( NULL == B ) return bfalse;

    if ( !BSP_branch_unlink_parent( B ) ) retval = bfalse;

    if ( !BSP_branch_unlink_children( B ) ) retval = bfalse;

    if ( !BSP_branch_unlink_nodes( B ) ) retval = bfalse;

    return retval;
}
//--------------------------------------------------------------------------------------------
bool_t BSP_branch_unlink_parent( BSP_branch_t * B )
{
    // unlink this branch from its parent_ptr

    size_t i;
    bool_t found_self = bfalse;

    BSP_branch_t      * parent_ptr;
    BSP_branch_list_t * children_ptr;

    if ( NULL == B ) return bfalse;

    if ( NULL == B->parent ) return btrue;
    parent_ptr    = B->parent;
    children_ptr = &( parent_ptr->children );

    // unlink from the parent_ptr
    B->parent = NULL;

    // unlink the parent_ptr from us
    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        for ( i = 0; i < children_ptr->lst_size; i++ )
        {
            if ( B == children_ptr->lst[i] )
            {
                children_ptr->lst[i] = NULL;
                found_self = btrue;
            }
        }
    }

    // re-count the parent_ptr's children_ptr
    children_ptr->inserted = 0;
    for ( i = 0; i < children_ptr->lst_size; i++ )
    {
        if ( NULL == children_ptr->lst[i] ) continue;
        children_ptr->inserted++;
    }

    return found_self;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_unlink_children( BSP_branch_t * B )
{
    // unlink all children from this branch only, not recursively

    size_t i;
    BSP_branch_list_t * children_ptr;

    if ( NULL == B ) return bfalse;
    children_ptr = &( B->children );

    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        for ( i = 0; i < children_ptr->lst_size; i++ )
        {
            if ( NULL != children_ptr->lst[i] )
            {
                children_ptr->lst[i]->parent = NULL;
            }

            children_ptr->lst[i] = NULL;
        }

    }

    ego_aabb_self_clear( &( children_ptr->bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_unlink_nodes( BSP_branch_t * B )
{
    // Remove any nodes (from this branch only, NOT recursively).
    // This resets the B->nodes list.

    if ( NULL == B ) return bfalse;

    return BSP_branch_free_nodes( B, bfalse );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_insert_leaf_list( BSP_branch_t * B, BSP_leaf_t * n )
{
    bool_t retval;

    if ( NULL == B || NULL == n ) return bfalse;

    retval = BSP_leaf_list_push_front( &( B->nodes ), n );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_update_depth_rec( BSP_branch_t * B, int depth )
{
    size_t cnt;
    BSP_branch_list_t * children_ptr;

    if ( NULL == B || depth < 0 ) return bfalse;
    children_ptr = &( B->children );

    B->depth = depth;

    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        for ( cnt = 0; cnt < children_ptr->lst_size; cnt++ )
        {
            if ( NULL == children_ptr->lst[cnt] ) continue;

            BSP_branch_update_depth_rec( children_ptr->lst[cnt], depth + 1 );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_insert_branch( BSP_branch_t * B, int index, BSP_branch_t * B2 )
{
    BSP_branch_t * ptmp;
    BSP_branch_list_t * children_ptr;

    // B and B2 exist?
    if ( NULL == B || NULL == B2 ) return bfalse;
    children_ptr = &( B->children );

    // valid child list for B?
    if ( INVALID_BSP_BRANCH_LIST( children_ptr ) ) return bfalse;

    // valid index range?
    if ( index < 0 || ( size_t )index >= children_ptr->lst_size ) return bfalse;

    // we can't merge two branches at this time
    if ( NULL != children_ptr->lst[ index ] )
    {
        return bfalse;
    }

    EGOBOO_ASSERT( B->depth + 1 == B2->depth );

    children_ptr->lst[ index ] = B2;
    B2->parent                 = B;

    // update the all bboxes above B2
    for ( ptmp = B2->parent; NULL != ptmp; ptmp = ptmp->parent )
    {
        BSP_branch_list_t * tmp_children_ptr = &( ptmp->children );

        // add in the size of B2->children
        if ( ego_aabb_is_clear( &( tmp_children_ptr->bbox ) ) )
        {
            ego_aabb_copy( &( tmp_children_ptr->bbox ), &( B2->children.bbox ) );
        }
        else
        {
            ego_aabb_self_union( &( tmp_children_ptr->bbox ), &( B2->children.bbox ) );
        }

        // add in the size of B2->nodes
        if ( ego_aabb_is_clear( &( tmp_children_ptr->bbox ) ) )
        {
            ego_aabb_copy( &( tmp_children_ptr->bbox ), &( B2->nodes.bbox ) );
        }
        else
        {
            ego_aabb_self_union( &( tmp_children_ptr->bbox ), &( B2->nodes.bbox ) );
        }

        // add in the size of B2->unsorted
        if ( ego_aabb_is_clear( &( tmp_children_ptr->bbox ) ) )
        {
            ego_aabb_copy( &( tmp_children_ptr->bbox ), &( B2->unsorted.bbox ) );
        }
        else
        {
            ego_aabb_self_union( &( tmp_children_ptr->bbox ), &( B2->unsorted.bbox ) );
        }
    }

    // update the depth B2 and all children
    BSP_branch_update_depth_rec( B2, B->depth + 1 );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_clear( BSP_branch_t * B, bool_t recursive )
{

    if ( NULL == B ) return bfalse;

    if ( recursive )
    {
        BSP_branch_list_clear_rec( &( B->children ) );
    }
    else
    {
        // unlink any child nodes
        BSP_branch_unlink_children( B );
    }

    // clear the node list
    BSP_leaf_list_reset( &( B->nodes ) );

    // clear the unsorted list
    BSP_leaf_list_reset( &( B->unsorted ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_free_nodes( BSP_branch_t * B, bool_t recursive )
{
    size_t cnt;
    BSP_branch_list_t * children_ptr;

    if ( NULL == B ) return bfalse;
    children_ptr = &( B->children );

    if ( recursive )
    {
        if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
        {
            // recursively clear out any nodes in the children.lst
            for ( cnt = 0; cnt < children_ptr->lst_size; cnt++ )
            {
                if ( NULL == children_ptr->lst[cnt] ) continue;

                BSP_branch_free_nodes( children_ptr->lst[cnt], btrue );
            }
        }
    }

    // free all nodes of this branch
    BSP_leaf_list_reset( &( B->nodes ) );

    // free all unsorted nodes of this branch
    BSP_leaf_list_reset( &( B->unsorted ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_prune( BSP_tree_t * t, BSP_branch_t * B, bool_t recursive )
{
    /// @details BB@> remove all leaves with no children. Do a depth first recursive search for efficiency

    size_t i;
    bool_t   retval;
    BSP_branch_list_t * children_ptr;

    if ( NULL == B ) return bfalse;
    children_ptr = &( B->children );

    // prune all of the children 1st
    if ( recursive )
    {
        if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
        {
            // prune all the children
            for ( i = 0; i < children_ptr->lst_size; i++ )
            {
                BSP_branch_prune( t, children_ptr->lst[i], btrue );
            }
        }
    }

    // do not remove the root node
    if ( B == t->finite ) return btrue;

    retval = btrue;
    if ( BSP_branch_empty( B ) )
    {
        if ( NULL != B->parent )
        {
            bool_t found = bfalse;

            // unlink the parent and return the node to the free list
            for ( i = 0; i < B->parent->children.lst_size; i++ )
            {
                if ( B->parent->children.lst[i] == B )
                {
                    B->parent->children.lst[i] = NULL;
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
bool_t BSP_branch_add_all_nodes( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    size_t       cnt, colst_size;
    BSP_leaf_t * ptmp;

    const BSP_leaf_list_t * nodes_ptr;

    if ( NULL == pbranch ) return bfalse;
    nodes_ptr = &( pbranch->nodes );

    if ( DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    colst_size = BSP_leaf_pary_get_size( colst );

    if ( NULL != ptest )
    {
        // add any valid nodes in the nodes.lst
        for ( cnt = 0, ptmp = nodes_ptr->lst; NULL != ptmp && cnt < nodes_ptr->count; ptmp = ptmp->next, cnt++ )
        {
            if (( *ptest )( ptmp ) )
            {
                if ( !BSP_leaf_pary_push_back( colst, ptmp ) ) break;
            }
        }
    }
    else
    {
        // add any nodes in the nodes.lst
        for ( cnt = 0, ptmp = nodes_ptr->lst; NULL != ptmp && cnt < nodes_ptr->count; ptmp = ptmp->next, cnt++ )
        {
            if ( !BSP_leaf_pary_push_back( colst, ptmp ) ) break;
        }
    }

    return ( BSP_leaf_pary_get_top( colst ) < colst_size );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_add_all_unsorted( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    size_t       cnt, colst_size;
    BSP_leaf_t * ptmp;
    const BSP_leaf_list_t * unsorted_ptr;

    if ( NULL == pbranch ) return bfalse;
    unsorted_ptr = &( pbranch->unsorted );

    if ( DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    colst_size = BSP_leaf_pary_get_size( colst );

    if ( NULL != ptest )
    {
        // add any valid unsorted in the unsorted.lst
        for ( cnt = 0, ptmp = unsorted_ptr->lst; NULL != ptmp && cnt < unsorted_ptr->count; ptmp = ptmp->next, cnt++ )
        {
            if (( *ptest )( ptmp ) )
            {
                if ( !BSP_leaf_pary_push_back( colst, ptmp ) ) break;
            }
        }
    }
    else
    {
        // add any unsorted in the unsorted.lst
        for ( cnt = 0, ptmp = unsorted_ptr->lst; NULL != ptmp && cnt < unsorted_ptr->count; ptmp = ptmp->next, cnt++ )
        {
            if ( !BSP_leaf_pary_push_back( colst, ptmp ) ) break;
        }
    }

    return ( BSP_leaf_pary_get_top( colst ) < colst_size );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_add_all_children( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    size_t cnt, colst_size;

    const BSP_branch_list_t * children_ptr;

    if ( NULL == pbranch ) return bfalse;
    children_ptr = &( pbranch->children );

    if ( DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    colst_size = BSP_leaf_pary_get_size( colst );

    // add all nodes from all children
    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        for ( cnt = 0; cnt < children_ptr->lst_size; cnt++ )
        {
            BSP_branch_t * pchild = children_ptr->lst[cnt];
            if ( NULL == pchild ) continue;

            BSP_branch_add_all_rec( pchild, ptest, colst );

            if ( BSP_leaf_pary_get_top( colst ) >= colst_size ) break;
        }

    }

    return ( BSP_leaf_pary_get_top( colst ) < colst_size );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_add_all_rec( const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    size_t cnt, colst_size;

    const BSP_branch_list_t * children_ptr;

    if ( NULL == pbranch ) return bfalse;
    children_ptr = &( pbranch->children );

    if ( DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    colst_size = BSP_leaf_pary_get_size( colst );

    BSP_branch_add_all_nodes( pbranch, ptest, colst );
    BSP_branch_add_all_unsorted( pbranch, ptest, colst );

    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        // add all nodes from all children
        for ( cnt = 0; cnt < children_ptr->lst_size; cnt++ )
        {
            BSP_branch_t * pchild = children_ptr->lst[cnt];
            if ( NULL == pchild ) continue;

            BSP_branch_add_all_rec( pchild, ptest, colst );

            if ( BSP_leaf_pary_get_top( colst ) >= colst_size ) break;
        }

    }

    return ( BSP_leaf_pary_get_top( colst ) < colst_size );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_empty( const BSP_branch_t * pbranch )
{
    size_t cnt;
    bool_t empty;
    const BSP_branch_list_t * children_ptr;
    const BSP_leaf_list_t * nodes_ptr, * unsorted_ptr;

    if ( NULL == pbranch ) return bfalse;
    children_ptr = &( pbranch->children );
    unsorted_ptr = &( pbranch->unsorted );
    nodes_ptr    = &( pbranch->nodes );

    // assume the worst
    empty = btrue;

    // check to see if there are any nodes in this branch's nodes.lst
    if ( !EMPTY_BSP_LEAF_LIST( nodes_ptr ) )
    {
        empty = bfalse;
        goto BSP_branch_empty_exit;
    }

    // check to see if there are any nodes in this branch's unsorted.lst
    if ( !EMPTY_BSP_LEAF_LIST( unsorted_ptr ) )
    {
        empty = bfalse;
        goto BSP_branch_empty_exit;
    }

    // look to see if all children are free
    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        for ( cnt = 0; cnt < children_ptr->lst_size; cnt++ )
        {
            if ( NULL != children_ptr->lst[cnt] )
            {
                empty = bfalse;
                goto BSP_branch_empty_exit;
            }
        }
    }

BSP_branch_empty_exit:

    return empty;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_collide_aabb( const BSP_branch_t * pbranch, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @details BB@> Recursively search the BSP tree for collisions with the paabb
    //      Return bfalse if we need to break out of the recursive search for any reason.

    geometry_rv geom_children, geom_nodes, geom_unsorted;
    bool_t retval, BSP_retval;

    const BSP_leaf_list_t * nodes_ptr, * unsorted_ptr;
    const BSP_branch_list_t * children_ptr;

    // if the collision list doesn't exist, stop
    if ( DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    // if the branch doesn't exist, stop
    if ( NULL == pbranch ) return bfalse;
    children_ptr = &( pbranch->children );
    nodes_ptr = &( pbranch->nodes );
    unsorted_ptr = &( pbranch->unsorted );

    // assume the worst
    retval = bfalse;

    // check the unsorted nodes
    geom_unsorted = geometry_error;
    if ( NULL != unsorted_ptr && NULL != unsorted_ptr->lst )
    {
        if ( 0 == unsorted_ptr->count )
        {
            geom_unsorted = geometry_outside;
        }
        else if ( 1 == unsorted_ptr->count )
        {
            // A speedup.
            // If there is only one entry, then the aabb-aabb collision test is
            // called twice for 2 bounding boxes of the same size
            geom_unsorted = geometry_intersect;
        }
        else
        {
            geom_unsorted = aabb_intersects_aabb( paabb, &( unsorted_ptr->bbox.data ) );
        }
    }

    // check the nodes at this level of the tree
    geom_nodes = geometry_error;
    if ( NULL != nodes_ptr && NULL != nodes_ptr->lst )
    {
        if ( 0 == nodes_ptr->count )
        {
            geom_nodes = geometry_outside;
        }
        else if ( 1 == nodes_ptr->count )
        {
            // A speedup.
            // If there is only one entry, then the aabb-aabb collision test is
            // called twice for 2 bounding boxes of the same size
            geom_nodes = geometry_intersect;
        }
        else
        {
            geom_nodes = aabb_intersects_aabb( paabb, &( nodes_ptr->bbox.data ) );
        }
    }

    // check the nodes at all lower levels
    geom_children = geometry_error;
    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        if ( 0 == children_ptr->inserted )
        {
            geom_children = geometry_outside;
        }
        else if ( 1 == children_ptr->inserted )
        {
            // A speedup.
            // If there is only one entry, then the aabb-aabb collision test is
            // called twice for 2 bounding boxes of the same size
            geom_children = geometry_intersect;
        }
        else
        {
            geom_children = aabb_intersects_aabb( paabb, &( children_ptr->bbox.data ) );
        }
    }

    if ( geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside )
    {
        // the branch and the object do not overlap at all.
        // do nothing.
        return bfalse;
    }

    switch ( geom_unsorted )
    {
        case geometry_intersect:
            // The aabb and branch partially overlap. Test each item.
            BSP_retval = BSP_leaf_list_collide_aabb( unsorted_ptr, paabb, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        case geometry_inside:
            // The branch is completely contained by the test aabb. Add every single node.
            BSP_retval = BSP_branch_add_all_unsorted( pbranch, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        default:
            /* do nothing */
            break;
    }

    switch ( geom_nodes )
    {
        case geometry_intersect:
            // The aabb and branch partially overlap. Test each item.
            BSP_retval = BSP_leaf_list_collide_aabb( nodes_ptr, paabb, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        case geometry_inside:
            // The branch is completely contained by the test aabb. Add every single node.
            BSP_retval = BSP_branch_add_all_nodes( pbranch, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        default:
            /* do nothing */
            break;
    }

    switch ( geom_children )
    {
        case geometry_intersect:
            // The aabb and branch partially overlap. Test each item.
            BSP_retval = BSP_branch_list_collide_aabb( &( pbranch->children ), paabb, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        case geometry_inside:
            // The branch is completely contained by the test aabb. Add every single node.
            BSP_retval = BSP_branch_add_all_children( pbranch, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        default:
            /* do nothing */
            break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_collide_frustum( const BSP_branch_t * pbranch, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @details BB@> Recursively search the BSP tree for collisions with the paabb
    //      Return bfalse if we need to break out of the recursive search for any reason.

    geometry_rv geom_children, geom_nodes, geom_unsorted;
    bool_t retval, BSP_retval;

    const BSP_leaf_list_t * nodes_ptr, * unsorted_ptr;
    const BSP_branch_list_t * children_ptr;

    // if the collision list doesn't exist, stop
    if ( DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    // if the branch doesn't exist, stop
    if ( NULL == pbranch ) return bfalse;
    children_ptr = &( pbranch->children );
    nodes_ptr = &( pbranch->nodes );
    unsorted_ptr = &( pbranch->unsorted );

    // assume the worst
    retval = bfalse;

    // check the unsorted nodes
    geom_unsorted = geometry_error;
    if ( NULL != unsorted_ptr && NULL != unsorted_ptr->lst )
    {
        if ( 0 == unsorted_ptr->count )
        {
            geom_unsorted = geometry_outside;
        }
        else if ( 1 == unsorted_ptr->count )
        {
            // A speedup.
            // If there is only one entry, then the frustum-aabb collision test is
            // called twice for 2 bounding boxes of the same size
            geom_unsorted = geometry_intersect;
        }
        else
        {
            geom_unsorted = egolib_frustum_intersects_ego_aabb( pfrust, &( unsorted_ptr->bbox ) );
        }
    }

    // check the nodes at this level of the tree
    geom_nodes = geometry_error;
    if ( NULL != nodes_ptr && NULL != nodes_ptr->lst )
    {
        if ( 0 == nodes_ptr->count )
        {
            geom_nodes = geometry_outside;
        }
        else if ( 1 == nodes_ptr->count )
        {
            // A speedup.
            // If there is only one entry, then the frustum-aabb collision test is
            // called twice for 2 bounding boxes of the same size
            geom_nodes = geometry_intersect;
        }
        else
        {
            geom_nodes = egolib_frustum_intersects_ego_aabb( pfrust, &( nodes_ptr->bbox ) );
        }
    }

    // check the nodes at all lower levels
    geom_children = geometry_error;
    if ( !INVALID_BSP_BRANCH_LIST( children_ptr ) )
    {
        if ( 0 == children_ptr->inserted )
        {
            geom_children = geometry_outside;
        }
        else if ( 1 == children_ptr->inserted )
        {
            // A speedup.
            // If there is only one entry, then the frustum-aabb collision test is
            // called twice for 2 bounding boxes of the same size
            geom_children = geometry_intersect;
        }
        else
        {
            geom_children = egolib_frustum_intersects_ego_aabb( pfrust, &( children_ptr->bbox ) );
        }
    }

    if ( geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside )
    {
        // the branch and the object do not overlap at all.
        // do nothing.
        return bfalse;
    }

    switch ( geom_unsorted )
    {
        case geometry_intersect:
            // The frustum and branch partially overlap. Test each item.
            BSP_retval = BSP_leaf_list_collide_frustum( unsorted_ptr, pfrust, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        case geometry_inside:
            // The branch is completely contained by the test aabb. Add every single node.
            BSP_retval = BSP_branch_add_all_unsorted( pbranch, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        default:
            /* do nothing */
            break;
    }

    switch ( geom_nodes )
    {
        case geometry_intersect:
            // The frustum and branch partially overlap. Test each item.
            BSP_retval = BSP_leaf_list_collide_frustum( nodes_ptr, pfrust, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        case geometry_inside:
            // The branch is completely contained by the test aabb. Add every single node.
            BSP_retval = BSP_branch_add_all_nodes( pbranch, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        default:
            /* do nothing */
            break;
    }

    switch ( geom_children )
    {
        case geometry_intersect:
            // The frustum and branch partially overlap. Test each item.
            BSP_retval = BSP_branch_list_collide_frustum( &( pbranch->children ), pfrust, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        case geometry_inside:
            // The branch is completely contained by the test aabb. Add every single node.
            BSP_retval = BSP_branch_add_all_children( pbranch, ptest, colst );
            if ( BSP_retval ) retval = btrue;
            break;

        default:
            /* do nothing */
            break;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_insert_branch_list_rec( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int index, int depth )
{
    // the leaf is a child of this branch

    BSP_branch_t * pchild;
    bool_t inserted_branch;
    BSP_branch_list_t * children_ptr;

    // is there a leaf to insert?
    if ( NULL == pleaf ) return bfalse;

    // does the branch exist?
    if ( NULL == pbranch ) return bfalse;
    children_ptr = &( pbranch->children );

    // a valid child list?
    if ( INVALID_BSP_BRANCH_LIST( children_ptr ) ) return bfalse;

    // is the index within the correct range?
    if ( index < 0 || ( size_t )index >= children_ptr->lst_size ) return bfalse;

    // get the child. if it doesn't exist, create one
    pchild = BSP_tree_ensure_branch( ptree, pbranch, index );

    // try to insert the leaf
    inserted_branch = bfalse;
    if ( NULL != pchild )
    {
        // insert the leaf
        inserted_branch = BSP_branch_insert_leaf_rec( ptree, pchild, pleaf, depth );
    }

    // if it was inserted, update the bounding box
    if ( inserted_branch )
    {
        if ( 0 == children_ptr->inserted )
        {
            ego_aabb_copy( &( children_ptr->bbox ), &( pleaf->bbox ) );
        }
        else
        {
            ego_aabb_self_union( &( children_ptr->bbox ), &( pleaf->bbox ) );
        }

        children_ptr->inserted++;
    }

    return inserted_branch;
}

//--------------------------------------------------------------------------------------------
int BSP_branch_insert_leaf_rec_1( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth )
{
    int index, retval;

    bool_t inserted_leaf, inserted_branch;

    if ( NULL == ptree || NULL == pbranch || NULL == pleaf || depth < 0 ) return -1;

    // not inserted anywhere, yet
    inserted_leaf   = bfalse;
    inserted_branch = bfalse;

    // don't go too deep
    if ( depth > ptree->max_depth )
    {
        // insert the node under this branch
        inserted_branch = BSP_branch_insert_leaf_list( pbranch, pleaf );
    }
    else
    {
        //---- determine which child the leaf needs to go under
        index = _find_child_index( &( pbranch->bsp_bbox ), &( pleaf->bbox.data ) );

        //---- insert the leaf in the right place
        if ( index < -1 )
        {
            /* do nothing */
        }
        else if ( -1 == index )
        {
            // the leaf belongs on this branch
            inserted_leaf = BSP_branch_insert_leaf_list( pbranch, pleaf );
        }
        else
        {
            inserted_branch = BSP_branch_insert_branch_list_rec( ptree, pbranch, pleaf, index, depth );
        }
    }

    retval = -1;
    if ( inserted_leaf && !inserted_branch )
    {
        retval = 0;
    }
    else if ( !inserted_leaf && inserted_branch )
    {
        retval = 1;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_insert_leaf_rec( BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth )
{
    /// @details BB@> recursively insert a leaf in a tree of BSP_branch_t*. Get new branches using the
    ///              BSP_tree_get_free() function to allocate any new branches that are needed.

    bool_t handled = bfalse;
    size_t nodes_at_this_level, unsorted_nodes, sibling_nodes;

    BSP_leaf_list_t * unsorted_ptr;
    BSP_leaf_list_t * nodes_ptr;

    if ( NULL == ptree || NULL == pleaf ) return bfalse;

    if ( NULL == pbranch ) return bfalse;
    unsorted_ptr = &( pbranch->unsorted );
    nodes_ptr    = &( pbranch->nodes );

    // nothing has been done with the pleaf pointer yet
    handled = bfalse;

    // count the nodes in different categories
    sibling_nodes       = nodes_ptr->count;
    unsorted_nodes      = unsorted_ptr->count;
    nodes_at_this_level = sibling_nodes + unsorted_nodes;

    // is list of unsorted nodes overflowing?
    if ( unsorted_nodes > 0 && nodes_at_this_level >= BRANCH_NODE_THRESHOLD )
    {
        // scan through the list and insert everyone in the next lower branch

        BSP_leaf_t * tmp_leaf;
        int inserted;

        // if you just sort the whole list, you could end up with
        // every single element in the same node at some large depth
        // (if the nodes are strange that way).
        // Since, REDUCING the depth of the BSP is the entire point, we have to limit this in some way...
        size_t target_unsorted = ( sibling_nodes > BRANCH_NODE_THRESHOLD ) ? 0 : BRANCH_NODE_THRESHOLD - sibling_nodes;
        size_t min_unsorted = target_unsorted >> 1;

        if ( unsorted_ptr->count > min_unsorted )
        {
            tmp_leaf = BSP_leaf_list_pop_front( &( pbranch->unsorted ) );
            inserted = 0;
            do
            {
                if ( BSP_branch_insert_leaf_rec_1( ptree, pbranch, tmp_leaf, depth + 1 ) )
                {
                    inserted++;
                }

                // Break out BEFORE popping off another pointer.
                // Otherwise, we lose drop a node by mistake.
                if ( unsorted_ptr->count <= min_unsorted ) break;

                tmp_leaf = BSP_leaf_list_pop_front( &( pbranch->unsorted ) );
            }
            while ( NULL != tmp_leaf );

            // now clear out the old bbox.
            ego_aabb_self_clear( &( unsorted_ptr->bbox ) );

            if ( unsorted_ptr->count > 0 )
            {
                // generate the correct bbox for any remaining nodes

                size_t cnt;

                tmp_leaf = unsorted_ptr->lst;
                cnt = 0;
                if ( NULL != tmp_leaf )
                {
                    aabb_copy( &( unsorted_ptr->bbox.data ), &( tmp_leaf->bbox.data ) );
                    tmp_leaf = tmp_leaf->next;
                    cnt++;
                }

                for ( /* nothing */;
                                   NULL != tmp_leaf && cnt < unsorted_ptr->count;
                                   tmp_leaf = tmp_leaf->next, cnt++ )
                {
                    aabb_self_union( &( unsorted_ptr->bbox.data ), &( tmp_leaf->bbox.data ) );
                }

                ego_aabb_validate( &( unsorted_ptr->bbox ) );
            }
        }
    }

    // re-calculate the values for this branch
    sibling_nodes       = nodes_ptr->count;
    unsorted_nodes      = unsorted_ptr->count;
    nodes_at_this_level = sibling_nodes + unsorted_nodes;

    // defer sorting any new leaves, if possible
    if ( !handled && nodes_at_this_level < BRANCH_NODE_THRESHOLD )
    {
        handled = BSP_leaf_list_push_front( &( pbranch->unsorted ), pleaf );
    }

    // recursively insert
    if ( !handled )
    {
        int where;

        // keep track of the tree depth
        where = BSP_branch_insert_leaf_rec_1( ptree, pbranch, pleaf, depth + 1 );

        handled = ( where >= 0 );
    }

    // keep a record of the highest depth
    if ( handled && depth > ptree->depth )
    {
        ptree->depth = depth;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
// BSP_tree_t
//--------------------------------------------------------------------------------------------
BSP_tree_t * BSP_tree_ctor( BSP_tree_t * t, Sint32 req_dim, Sint32 req_depth )
{
    int    node_count;
    size_t cnt;

    if ( NULL == t ) return t;

    BLANK_STRUCT_PTR( t )

    ego_aabb_ctor( &( t->bbox ) );
    BSP_aabb_ctor( &( t->bsp_bbox ), req_dim );
    BSP_leaf_list_ctor( &( t->infinite ) );

    // these constructions are handled in BSP_tree_alloc()
    // BSP_branch_ary_ctor ( &(t->branch_all) );
    // BSP_branch_pary_ctor( &(t->branch_used) );
    // BSP_branch_pary_ctor( &(t->branch_free) );

    node_count = BSP_tree_count_nodes( req_dim, req_depth );
    if ( node_count < 0 ) return t;

    if ( !BSP_tree_alloc( t, node_count, req_dim ) ) return t;

    t->max_depth = req_depth;

    // initialize the free list
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

    BSP_tree_dealloc( t );

    return t;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_alloc( BSP_tree_t * t, size_t count, size_t dim )
{
    size_t cnt;

    if ( NULL == t ) return bfalse;

    if ( NULL != t->branch_all.ary || t->branch_all.alloc > 0 ) return bfalse;

    // re-initialize the variables
    t->dimensions = 0;

    // allocate the infinite node list
    BSP_leaf_list_alloc( &( t->infinite ) );

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
    BSP_aabb_ctor( &( t->bsp_bbox ), dim );

    // set the variables
    t->dimensions = dim;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_dealloc( BSP_tree_t * t )
{
    size_t i;

    if ( NULL == t ) return bfalse;

    if ( NULL == t->branch_all.ary || 0 == t->branch_all.alloc ) return btrue;

    // allocate the infinite node list
    BSP_leaf_list_dealloc( &( t->infinite ) );

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
    BSP_aabb_dtor( &( t->bsp_bbox ) );

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
        EGOBOO_ASSERT( NULL == B->nodes.lst );

        // make sure that the data is cleared out
        BSP_branch_unlink_all( B );
    }

    return B;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_clear_rec( BSP_tree_t * t )
{
    if ( NULL == t ) return bfalse;

    // clear all the branches, recursively
    BSP_branch_clear( t->finite, btrue );

    // free the infinite nodes of the tree
    BSP_leaf_list_reset( &( t->infinite ) );

    // set the bbox to empty
    ego_aabb_self_clear( &( t->bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_prune( BSP_tree_t * t )
{
    /// @details BB@> remove all leaves with no children.lst or nodes.lst.

    int cnt;

    if ( NULL == t || NULL == t->finite ) return bfalse;

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
    size_t         cnt;
    BSP_branch_t * proot;

    if ( NULL == t ) return NULL;

    if ( NULL != t->finite ) return t->finite;

    proot = BSP_tree_get_free( t );
    if ( NULL == proot ) return NULL;

    // make sure that it is unlinked
    BSP_branch_unlink_all( proot );

    // copy the tree bounding box to the root node
    for ( cnt = 0; cnt < t->dimensions; cnt++ )
    {
        proot->bsp_bbox.mins.ary[cnt] = t->bsp_bbox.mins.ary[cnt];
        proot->bsp_bbox.mids.ary[cnt] = t->bsp_bbox.mids.ary[cnt];
        proot->bsp_bbox.maxs.ary[cnt] = t->bsp_bbox.maxs.ary[cnt];
    }

    // fix the depth
    proot->depth = 0;

    // assign the root to the tree
    t->finite = proot;

    return proot;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_tree_ensure_branch( BSP_tree_t * t, BSP_branch_t * B, int index )
{
    BSP_branch_t * pbranch;

    if (( NULL == t ) || ( NULL == B ) ) return NULL;
    if ( index < 0 || ( size_t )index > B->children.lst_size ) return NULL;

    // grab any existing value
    pbranch = B->children.lst[index];

    // if this branch doesn't exist, create it and insert it properly.
    if ( NULL == pbranch )
    {
        // grab a free branch
        pbranch = BSP_tree_get_free( t );

        if ( NULL != pbranch )
        {
            // make sure that it is unlinked
            BSP_branch_unlink_all( pbranch );

            // generate its bounding box
            pbranch->depth = B->depth + 1;
            _generate_BSP_aabb_child( &( B->bsp_bbox ), index, &( pbranch->bsp_bbox ) );

            // insert it in the correct position
            BSP_branch_insert_branch( B, index, pbranch );
        }
    }

    return pbranch;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_insert_infinite( BSP_tree_t * ptree, BSP_leaf_t * pleaf )
{
    if ( NULL == ptree || NULL == pleaf ) return bfalse;

    return BSP_leaf_list_push_front( &( ptree->infinite ), pleaf );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_insert_leaf( BSP_tree_t * ptree, BSP_leaf_t * pleaf )
{
    bool_t retval;

    if ( NULL == ptree || NULL == pleaf ) return bfalse;

    if ( !BSP_aabb_contains_aabb( &( ptree->bsp_bbox ), &( pleaf->bbox.data ) ) )
    {
        // put the leaf at the head of the infinite list
        retval = BSP_tree_insert_infinite( ptree, pleaf );
    }
    else
    {
        BSP_branch_t * proot = BSP_tree_ensure_root( ptree );

        retval = BSP_branch_insert_leaf_rec( ptree, proot, pleaf, 0 );
    }

    // make sure the ptree->bbox matches the maximum extent of all objects in the tree
    if ( retval )
    {
        if ( ego_aabb_is_clear( &( ptree->bbox ) ) )
        {
            ego_aabb_copy( &( ptree->bbox ), &( pleaf->bbox ) );
        }
        else
        {
            ego_aabb_self_union( &( ptree->bbox ), &( pleaf->bbox ) );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_tree_prune_branch( BSP_tree_t * t, size_t cnt )
{
    /// @details BB@> an optimized version of iterating through the t->branch_used list
    ///                and then calling BSP_branch_prune() on the empty branch. In the old method,
    ///                the t->branch_used list was searched twice to find each empty branch. This
    ///                function does it only once.

    size_t i;
    bool_t remove;

    BSP_branch_t * B;

    if ( NULL == t || t->branch_used.top <= 0 || cnt >= ( size_t )t->branch_used.top ) return bfalse;

    B = t->branch_used.ary[ cnt ];
    if ( NULL == B ) return bfalse;

    // do not remove the root node
    if ( B == t->finite ) return btrue;

    remove = bfalse;
    if ( BSP_branch_empty( B ) )
    {
        bool_t found = BSP_branch_unlink_all( B );

        // not finding yourself is an error
        if ( !found )
        {
            EGOBOO_ASSERT( bfalse );
        }

        remove = btrue;
    }

    if ( remove )
    {
        // reduce the size of the list
        t->branch_used.top--;

        // set B's data to "safe" values
        // this has already been "unlinked", so some of this is redundant
        B->parent = NULL;
        for ( i = 0; i < B->children.lst_size; i++ )
        {
            B->children.lst[i] = NULL;
        }
        BSP_leaf_list_clear( &( B->nodes ) );
        BSP_leaf_list_clear( &( B->unsorted ) );
        B->depth = -1;
        BSP_aabb_self_clear( &( B->bsp_bbox ) );

        // move the branch that we found to the top of the list
        SWAP( BSP_branch_t *, t->branch_used.ary[cnt], t->branch_used.ary[t->branch_used.top] );

        // add the branch to the free list
        BSP_branch_pary_push_back( &( t->branch_free ), B );
    }

    return remove;
}

//--------------------------------------------------------------------------------------------
int BSP_tree_collide_aabb( const BSP_tree_t * tree, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @details BB@> fill the collision list with references to objects that the aabb may overlap.
    //      Return the number of collisions found.

    if ( NULL == tree || NULL == paabb ) return 0;

    if ( NULL == colst || 0 == colst->alloc ) return 0;

    // collide with any "infinite" nodes
    BSP_leaf_list_collide_aabb( &( tree->infinite ), paabb, ptest, colst );

    // collide with the rest of the tree
    BSP_branch_collide_aabb( tree->finite, paabb, ptest, colst );

    return colst->top;
}

//--------------------------------------------------------------------------------------------
int BSP_tree_collide_frustum( const BSP_tree_t * tree, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @details BB@> fill the collision list with references to objects that the frustum may overlap.
    //      Return the number of collisions found.

    if ( NULL == tree || NULL == pfrust ) return 0;

    if ( NULL == colst || 0 == colst->alloc ) return 0;

    // collide with any "infinite" nodes
    BSP_leaf_list_collide_frustum( &( tree->infinite ), pfrust, ptest, colst );

    // collide with the rest of the tree
    BSP_branch_collide_frustum( tree->finite, pfrust, ptest, colst );

    return colst->top;
}

//--------------------------------------------------------------------------------------------
// BSP_leaf_list_t
//--------------------------------------------------------------------------------------------
BSP_leaf_list_t * BSP_leaf_list_ctor( BSP_leaf_list_t * LL )
{
    if ( NULL == LL ) return LL;

    BLANK_STRUCT_PTR( LL )

    BSP_leaf_list_alloc( LL );

    ego_aabb_ctor( &( LL->bbox ) );

    return LL;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t * BSP_leaf_list_dtor( BSP_leaf_list_t *  LL )
{
    if ( NULL == LL ) return LL;

    BSP_leaf_list_dealloc( LL );

    BLANK_STRUCT_PTR( LL )

    return LL;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_alloc( BSP_leaf_list_t * LL )
{
    if ( NULL == LL ) return bfalse;

    BSP_leaf_list_dealloc( LL );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_dealloc( BSP_leaf_list_t *  LL )
{
    if ( NULL == LL ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t * BSP_leaf_list_clear( BSP_leaf_list_t * LL )
{
    // DYNAMIC ALLOCATION in LL->bbox, so do not use memset

    if ( NULL == LL ) return LL;

    ego_aabb_self_clear( &( LL->bbox ) );
    LL->count = 0;
    LL->lst = NULL;

    return LL;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_push_front( BSP_leaf_list_t * LL, BSP_leaf_t * n )
{
    /// @details BB@> Insert a leaf in the list, making sure there are no duplicates.
    ///               Duplicates will cause loops in the list and make it impossible to
    ///               traverse properly.

    bool_t       retval;
    size_t       cnt;
    BSP_leaf_t * pleaf;

    if ( NULL == LL || NULL == n ) return bfalse;

    if ( n->inserted )
    {
        // hmmm.... what to do?
        log_warning( "BSP_leaf_list_push_front() - trying to insert a BSP_leaf that is claiming to be part of a list already\n" );
    }

    retval = bfalse;

    if ( EMPTY_BSP_LEAF_LIST( LL ) )
    {
        // prepare the node
        n->next = NULL;

        // insert the node
        LL->lst     = n;
        LL->count   = 1;
        ego_aabb_copy( &( LL->bbox ), &( n->bbox ) );

        n->inserted = btrue;

        retval = btrue;
    }
    else
    {
        bool_t found = bfalse;

        for ( cnt = 0, pleaf = LL->lst;
              NULL != pleaf->next && cnt < LL->count && !found;
              cnt++, pleaf = pleaf->next )
        {
            // do not insert duplicates, or we have a big problem
            if ( n == pleaf )
            {
                found = btrue;
                break;
            }
        }

        if ( !found )
        {
            EGOBOO_ASSERT( NULL == pleaf->next );

            // prepare the node
            n->next = NULL;

            // insert the node at the end of the list
            pleaf->next = n;
            LL->count   = LL->count + 1;
            ego_aabb_self_union( &( LL->bbox ), &( n->bbox ) );

            n->inserted = btrue;

            retval = btrue;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_reset( BSP_leaf_list_t * LL )
{
    /// @details BB@> Clear out the leaf list.

    size_t       cnt;
    BSP_leaf_t * ptmp;

    if ( NULL == LL ) return bfalse;

    if ( EMPTY_BSP_LEAF_LIST( LL ) )
    {
        LL->count = 0;
        LL->lst = NULL;
    }
    else
    {
        // unlink the nodes
        for ( cnt = 0; NULL != LL->lst && cnt < LL->count; cnt++ )
        {
            // pop a node off the list
            ptmp    = LL->lst;
            LL->lst = ptmp->next;

            // clean up the node
            ptmp->inserted = bfalse;
            ptmp->next     = NULL;
        };
    }

    // did we have a problem with the LL->count?
    EGOBOO_ASSERT( NULL == LL->lst );

    // clear out the other data
    BSP_leaf_list_clear( LL );

    return btrue;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t * BSP_leaf_list_pop_front( BSP_leaf_list_t * LL )
{
    BSP_leaf_t * retval;

    // handle a bad list
    if ( NULL == LL ) return NULL;

    // heal any bad lists
    if ( NULL == LL->lst )
    {
        LL->count = 0;
    }
    if ( 0 == LL->count )
    {
        LL->lst = NULL;
    }

    // handle an empty list
    if ( 0 == LL->count ) return NULL;

    // pop off the top node
    retval  = LL->lst;
    LL->lst = retval->next;
    LL->count--;

    // unlink it so that we avoid a bunch of debugging errors
    retval->inserted = bfalse;
    retval->next     = NULL;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_collide_aabb( const BSP_leaf_list_t * LL, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @details BB@> check for collisions with the given node list

    size_t       cnt;
    BSP_leaf_t * pleaf;
    bool_t       retval;

    // basic bounds checking
    if ( NULL == paabb || DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    // if the list is empty, there is nothing to do
    if ( EMPTY_BSP_LEAF_LIST( LL ) ) return btrue;

    // NOTE: this has already been tested in the parent function
    //// we already have the bounding box of all the leafs
    //if ( !aabb_intersects_aabb( paabb, &(LL->bbox) ) )
    //{
    //    return bfalse;
    //}

    // NOTE: this is already tested by DYNAMIC_ARY_INVALID( colst )
    //// if there is no more room in the colist, return bfalse
    //colst_size = BSP_leaf_pary_get_size( colst );
    //if ( 0 == colst_size || BSP_leaf_pary_get_top( colst ) >= colst_size )
    //{
    //    return bfalse;
    //}

    // scan through every leaf
    for ( cnt = 0, pleaf = LL->lst;
          cnt < LL->count && NULL != pleaf;
          cnt++, pleaf = pleaf->next )
    {
        bool_t do_insert;
        geometry_rv  geometry_test;
        ego_aabb_t * pleaf_bb = &( pleaf->bbox );

        // make sure the leaf is valid
        EGOBOO_ASSERT( pleaf->data_type > -1 );

        // make sure the leaf is inserted
        if ( !pleaf->inserted )
        {
            // hmmm.... what to do?
            log_warning( "BSP_leaf_list_collide_aabb() - a node in a leaf list is claiming to not be inserted\n" );
        }

        // test the geometry
        geometry_test = aabb_intersects_aabb( paabb, &( pleaf_bb->data ) );

        // determine what action to take
        do_insert = bfalse;
        if ( geometry_test > geometry_outside )
        {
            if ( NULL == ptest )
            {
                do_insert = btrue;
            }
            else
            {
                // we have a possible intersection
                do_insert = ( *ptest )( pleaf );
            }
        }

        if ( do_insert )
        {
            if ( !BSP_leaf_pary_push_back( colst, pleaf ) ) break;
        }
    }

    // return false if we maxed out the colist
    retval = ( BSP_leaf_pary_get_top( colst ) < BSP_leaf_pary_get_size( colst ) );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_leaf_list_collide_frustum( const BSP_leaf_list_t * LL, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @details BB@> check for collisions with the given node list

    size_t       cnt;
    BSP_leaf_t * pleaf;
    bool_t       retval;

    // basic bounds checking
    if ( NULL == pfrust || DYNAMIC_ARY_INVALID( colst ) ) return bfalse;

    // if the list is empty, there is nothing to do
    if ( EMPTY_BSP_LEAF_LIST( LL ) ) return btrue;

    // NOTE: this has already been tested in the parent function
    //// we already have the bounding box of all the leafs
    //if ( !egolib_frustum_intersects_ego_aabb( pfrust, &(LL->bbox) ) )
    //{
    //    return bfalse;
    //}

    // NOTE: this is already tested by DYNAMIC_ARY_INVALID( colst )
    //// if there is no more room in the colist, return bfalse
    //colst_size = BSP_leaf_pary_get_size( colst );
    //if ( 0 == colst_size || BSP_leaf_pary_get_top( colst ) >= colst_size )
    //{
    //    return bfalse;
    //}

    // scan through every leaf
    for ( cnt = 0, pleaf = LL->lst;
          cnt < LL->count && NULL != pleaf;
          cnt++, pleaf = pleaf->next )
    {
        bool_t do_insert;
        geometry_rv  geometry_test;
        ego_aabb_t * pleaf_bb = &( pleaf->bbox );

        // make sure the leaf is valid
        EGOBOO_ASSERT( pleaf->data_type > -1 );

        // make sure the leaf is inserted
        if ( !pleaf->inserted )
        {
            // hmmm.... what to do?
            log_warning( "BSP_leaf_list_collide_aabb() - a node in a leaf list is claiming to not be inserted\n" );
        }

        // test the geometry
        geometry_test = egolib_frustum_intersects_ego_aabb( pfrust, pleaf_bb );

        // determine what action to take
        do_insert = bfalse;
        if ( geometry_test > geometry_outside )
        {
            if ( NULL == ptest )
            {
                do_insert = btrue;
            }
            else
            {
                // we have a possible intersection
                do_insert = ( *ptest )( pleaf );
            }
        }

        if ( do_insert )
        {
            if ( !BSP_leaf_pary_push_back( colst, pleaf ) ) break;
        }
    }

    // return false if we maxed out the colist
    retval = ( BSP_leaf_pary_get_top( colst ) < BSP_leaf_pary_get_size( colst ) );

    return retval;
}

//--------------------------------------------------------------------------------------------
// BSP_branch_list_t
//--------------------------------------------------------------------------------------------
BSP_branch_list_t * BSP_branch_list_ctor( BSP_branch_list_t * BL, size_t dim )
{
    if ( NULL == BL ) return BL;

    BLANK_STRUCT_PTR( BL )

    BSP_branch_list_alloc( BL, dim );

    ego_aabb_ctor( &( BL->bbox ) );

    return BL;
}

//--------------------------------------------------------------------------------------------
BSP_branch_list_t * BSP_branch_list_dtor( BSP_branch_list_t *  BL )
{
    if ( NULL == BL ) return BL;

    BSP_branch_list_dealloc( BL );

    BLANK_STRUCT_PTR( BL )

    return BL;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_list_alloc( BSP_branch_list_t * BL, size_t dim )
{
    // determine the number of children from the number of dimensions
    size_t child_count = ( 0 == dim ) ? 0 : ( 2 << ( dim - 1 ) );

    if ( NULL == BL ) return bfalse;

    BSP_branch_list_dealloc( BL );

    // allocate the child list
    BL->lst = EGOBOO_NEW_ARY( BSP_branch_t*, child_count );
    if ( NULL != BL->lst )
    {
        size_t cnt;
        for ( cnt = 0; cnt < child_count; cnt++ )
        {
            BL->lst[cnt] = NULL;
        }
        BL->lst_size = child_count;
    }

    return ( NULL != BL->lst ) && ( child_count == BL->lst_size );
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_list_dealloc( BSP_branch_list_t * BL )
{
    if ( NULL == BL ) return bfalse;

    EGOBOO_DELETE_ARY( BL->lst );
    BL->lst_size = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_list_clear_rec( BSP_branch_list_t * BL )
{
    size_t cnt;

    if ( NULL == BL ) return bfalse;

    // recursively clear out any nodes in the children.lst
    for ( cnt = 0; cnt < BL->lst_size; cnt++ )
    {
        if ( NULL == BL->lst[cnt] ) continue;

        BSP_branch_clear( BL->lst[cnt], btrue );
    };

    // reset the number of inserted children
    BL->inserted = 0;

    // clear out the size of the nodes
    ego_aabb_self_clear( &( BL->bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_list_collide_aabb( const BSP_branch_list_t * BL, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    size_t cnt;
    int collisions;
    BSP_branch_t * pchild;

    if ( NULL == BL ) return bfalse;

    // scan the child branches and collide with them recursively
    collisions = 0;
    for ( cnt = 0; cnt < BL->lst_size; cnt++ )
    {
        // scan all the children.lst
        pchild = BL->lst[cnt];
        if ( NULL == pchild ) continue;

        // collide with each pchild
        if ( BSP_branch_collide_aabb( pchild, paabb, ptest, colst ) )
        {
            collisions++;
        }
    }

    return collisions > 0;
}

//--------------------------------------------------------------------------------------------
bool_t BSP_branch_list_collide_frustum( const BSP_branch_list_t * BL, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    size_t cnt;
    int collisions;
    BSP_branch_t * pchild;

    if ( NULL == BL ) return bfalse;

    // scan the child branches and collide with them recursively
    collisions = 0;
    for ( cnt = 0; cnt < BL->lst_size; cnt++ )
    {
        // scan all the children.lst
        pchild = BL->lst[cnt];
        if ( NULL == pchild ) continue;

        // collide with each pchild
        if ( BSP_branch_collide_frustum( pchild, pfrust, ptest, colst ) )
        {
            collisions++;
        }
    }

    return collisions > 0;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------

//BSP_branch_t * BSP_branch_create( size_t dim )
//{
//    BSP_branch_t * rv;
//
//    rv = EGOBOO_NEW( BSP_branch_t );
//    if ( NULL == rv ) return rv;
//
//    return BSP_branch_ctor( rv, dim );
//}

////--------------------------------------------------------------------------------------------
//bool_t BSP_branch_destroy( BSP_branch_t ** ppbranch )
//{
//    if ( NULL == ppbranch || NULL == *ppbranch ) return bfalse;
//
//    BSP_branch_dtor( *ppbranch );
//
//    EGOBOO_DELETE( *ppbranch );
//
//    return btrue;
//}

////--------------------------------------------------------------------------------------------
//BSP_branch_t * BSP_branch_create_ary( size_t ary_size, size_t dim )
//{
//    size_t         cnt;
//    BSP_branch_t * lst;
//
//    lst = EGOBOO_NEW_ARY( BSP_branch_t, ary_size );
//    if ( NULL == lst ) return lst;
//
//    for ( cnt = 0; cnt < ary_size; cnt++ )
//    {
//        BSP_branch_ctor( lst + cnt, dim );
//    }
//
//    return lst;
//}

////--------------------------------------------------------------------------------------------
//bool_t BSP_branch_destroy_ary( size_t ary_size, BSP_branch_t ** lst )
//{
//    size_t cnt;
//
//    if ( NULL == lst || NULL == *lst || 0 == ary_size ) return bfalse;
//
//    for ( cnt = 0; cnt < ary_size; cnt++ )
//    {
//        BSP_branch_dtor(( *lst ) + cnt );
//    }
//
//    EGOBOO_DELETE_ARY( *lst );
//
//    return btrue;
//}

////--------------------------------------------------------------------------------------------
//bool_t BSP_tree_init_0( BSP_tree_t * t )
//{
//    /// @details BB@> reset the tree to the "empty" state. Assume we do not own the nodes.lst or children.lst.
//
//    size_t i;
//    BSP_branch_t * pbranch;
//
//    // free any the nodes in the tree
//    BSP_tree_free_nodes( t, bfalse );
//
//    // initialize the leaves.
//    t->branch_free.top = 0;
//    t->branch_used.top = 0;
//    for ( i = 0; i < t->branch_all.alloc; i++ )
//    {
//        // grab a branch off of the static list
//        pbranch = t->branch_all.ary + i;
//
//        if ( NULL == pbranch ) continue;
//
//        // completely unlink the branch
//        BSP_branch_unlink_all( pbranch );
//
//        // push it onto the "stack"
//        BSP_branch_pary_push_back( &( t->branch_free ), pbranch );
//    };
//
//    return btrue;
//}

////--------------------------------------------------------------------------------------------
//bool_t BSP_tree_add_free( BSP_tree_t * t, BSP_branch_t * B )
//{
//    if ( NULL == t || NULL == B ) return bfalse;
//
//    // remove any links to other leaves
//    BSP_branch_unlink_all( B );
//
//    return BSP_tree_free_branch( t, B );
//}

////--------------------------------------------------------------------------------------------
//bool_t BSP_tree_free_all( BSP_tree_t * t )
//{
//    if ( !BSP_tree_free_nodes( t, bfalse ) ) return bfalse;
//
//    if ( !BSP_tree_free_branches( t ) ) return bfalse;
//
//    return btrue;
//}

////--------------------------------------------------------------------------------------------
//bool_t BSP_tree_free_nodes( BSP_tree_t * t, bool_t recursive )
//{
//    if ( NULL == t ) return bfalse;
//
//    if ( recursive )
//    {
//        BSP_branch_free_nodes( t->root, btrue );
//    }
//
//    // free the infinite nodes of the tree
//    BSP_leaf_list_reset( &( t->infinite ) );
//
//    return btrue;
//}

//--------------------------------------------------------------------------------------------
//bool_t BSP_tree_insert( BSP_tree_t * t, BSP_branch_t * B, BSP_leaf_t * n, int index )
//{
//    bool_t retval;
//
//    if (( NULL == t ) || ( NULL == B ) || ( NULL == n ) ) return bfalse;
//    if (( signed )index > ( signed )B->children.lst_size ) return bfalse;
//
//    if ( index >= 0 && NULL != B->children.lst[index] )
//    {
//        // inserting a node into the child
//        retval = BSP_branch_insert_leaf_list( B->children.lst[index], n );
//    }
//    else if ( index < 0 || 0 == t->branch_free.top )
//    {
//        // inserting a node into this branch node
//        // this can either occur because someone requested it (index < 0)
//        // OR because there are no more free nodes
//        retval = BSP_branch_insert_leaf_list( B, n );
//    }
//    else
//    {
//        // the requested B->children.lst[index] slot is empty. grab a pre-allocated
//        // BSP_branch_t from the free list in the BSP_tree_t structure an insert it in
//        // this child node
//
//        BSP_branch_t * pbranch = BSP_tree_ensure_branch( t, B, index );
//
//        retval = BSP_branch_insert_leaf_list( pbranch, n );
//    }
//
//    // do some book keeping
//    if( retval )
//    {
//        if( 0 == B->children.inserted )
//        {
//            BSP_aabb_copy( &(B->children.bbox), &(n->bbox) );
//        }
//        else
//        {
//            BSP_aabb_self_union( &(B->children.bbox), &(n->bbox) );
//        }
//
//        B->children.inserted++;
//    }
//
//    // something went wrong ?
//    return retval;
//}

////--------------------------------------------------------------------------------------------
//bool_t BSP_tree_free_branch( BSP_tree_t * t, BSP_branch_t * B )
//{
//    bool_t retval;
//
//    if ( NULL == t || NULL == B ) return bfalse;
//
//    retval = bfalse;
//    if ( BSP_tree_remove_used( t, B ) )
//    {
//        // add it to the used list
//        retval =  BSP_branch_pary_push_back( &( t->branch_free ), B );
//    }
//
//    return retval;
//}
//

////--------------------------------------------------------------------------------------------
//bool_t BSP_tree_free_branches( BSP_tree_t * t )
//{
//    int cnt;
//
//    if ( NULL == t ) return bfalse;
//
//    // transfer all the "used" branches back to the "free" branches
//    for ( cnt = 0; cnt < t->branch_used.top; cnt++ )
//    {
//        // grab a used branch
//        BSP_branch_t * pbranch = t->branch_used.ary[cnt];
//        if ( NULL == pbranch ) continue;
//
//        // completely unlink the branch
//        BSP_branch_unlink_all( pbranch );
//
//        // return the branch to the free list
//        BSP_branch_pary_push_back( &( t->branch_free ), pbranch );
//    }
//
//    // reset the used list
//    t->branch_used.top = 0;
//
//    // remove the tree root
//    t->finite = NULL;
//
//    return btrue;
//}
//

////--------------------------------------------------------------------------------------------
//bool_t BSP_tree_remove_used( BSP_tree_t * t, BSP_branch_t * B )
//{
//    int cnt;
//
//    if ( NULL == t || 0 == t->branch_used.top ) return bfalse;
//
//    if ( NULL == B ) return bfalse;
//
//    // scan the used list for the branch
//    for ( cnt = 0; cnt < t->branch_used.top; cnt++ )
//    {
//        if ( B == t->branch_used.ary[cnt] ) break;
//    }
//
//    // did we find the branch in the used list?
//    if ( cnt == t->branch_used.top ) return bfalse;
//
//    // reduce the size of the list
//    t->branch_used.top--;
//
//    // move the branch that we found to the top of the list
//    SWAP( BSP_branch_t *, t->branch_used.ary[cnt], t->branch_used.ary[t->branch_used.top] );
//
//    return btrue;
//}
//
