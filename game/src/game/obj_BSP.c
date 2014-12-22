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

/// @file game/obj_BSP.c
/// @brief Implementation of functions for the object BSP
/// @details

#include "game/obj_BSP.h"

#include "game/mesh_BSP.h"

#include "game/char.inl" /** @todo Remove. */
#include "game/particle.inl" /** @todo Remove. */
#include "egolib/bsp.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool obj_BSP_allocTree(obj_BSP_t *self, int dim, int depth);
static void obj_BSP_deallocTree(obj_BSP_t *self);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool obj_BSP_allocTree(obj_BSP_t *self, int dim, int depth)
{
	EGOBOO_ASSERT(NULL != self);
    BSP_tree_t *rv = BSP_tree_ctor(&(self->tree), dim, depth);
	return (NULL != rv);
}

//--------------------------------------------------------------------------------------------
static void obj_BSP_deallocTree(obj_BSP_t *self)
{
	EGOBOO_ASSERT(NULL != self);
    BSP_tree_dealloc(&(self->tree));
}

//--------------------------------------------------------------------------------------------
/// @brief Construct a BSP tree for game object.
/// @remark The used parameters duplicate the maximum resolution of the old system.
/// @author BB
/// @author MH
bool obj_BSP_ctor(obj_BSP_t *self, int bsp_dim, const mesh_BSP_t *mesh_bsp)
{
    int          cnt;
    int          mesh_dim, min_dim;
    float        bsp_size;
    BSP_tree_t * obj_tree;
    const BSP_tree_t * mesh_tree;

    if ( bsp_dim < 2 )
    {
        log_error( "obj_BSP_ctor() - cannot construct an object BSP with less than 2 dimensions\n" );
    }
    else if ( bsp_dim > 3 )
    {
        log_error( "obj_BSP_ctor() - cannot construct an object BSP with more than than 3 dimensions\n" );
    }

    if ( NULL == mesh_bsp ) return false;
    mesh_tree = &(mesh_bsp->tree);
    mesh_dim = mesh_bsp->tree.dimensions;

    if ( NULL == self ) return false;
    obj_tree = &(self->tree);

    BLANK_STRUCT_PTR(self)

    // allocate the data
	if (!obj_BSP_allocTree(self, bsp_dim, mesh_tree->max_depth)) return false;

    // find the maximum extent of the bsp
    bsp_size = 0.0f;
    min_dim = std::min( bsp_dim, mesh_dim );

    for ( cnt = 0; cnt < min_dim; cnt ++ )
    {
        float tmp_size = std::abs( mesh_tree->bsp_bbox.maxs.ary[cnt] - mesh_tree->bsp_bbox.mins.ary[cnt] );
        bsp_size = std::max( bsp_size, tmp_size );
    }

    // copy the volume from the mesh
    for ( int cnt = 0; cnt < min_dim; cnt++ )
    {
        // get the size
        obj_tree->bsp_bbox.mins.ary[cnt] = std::min( mesh_tree->bsp_bbox.mins.ary[cnt], mesh_tree->bbox.data.mins[cnt] );
        obj_tree->bsp_bbox.maxs.ary[cnt] = std::max( mesh_tree->bsp_bbox.maxs.ary[cnt], mesh_tree->bbox.data.maxs[cnt] );

        // make some extra space
        obj_tree->bsp_bbox.mins.ary[cnt] -= bsp_size * 0.25f;
        obj_tree->bsp_bbox.maxs.ary[cnt] += bsp_size * 0.25f;
    }

    // calculate a "reasonable size" for all dimensions that are not in the mesh
    for ( /* nothing */; cnt < bsp_dim; cnt++ )
    {
        obj_tree->bsp_bbox.mins.ary[cnt] = -bsp_size * 0.5f;
        obj_tree->bsp_bbox.maxs.ary[cnt] =  bsp_size * 0.5f;
    }

    if ( bsp_dim > 2 )
    {
        // make some extra special space in the z direction
        obj_tree->bsp_bbox.mins.ary[kZ] = std::min( -bsp_size, obj_tree->bsp_bbox.mins.ary[kZ] );
        obj_tree->bsp_bbox.maxs.ary[kZ] = std::max( bsp_size, obj_tree->bsp_bbox.maxs.ary[kZ] );
    }

    // calculate the mid positions
    for ( cnt = 0; cnt < bsp_dim; cnt++ )
    {
        obj_tree->bsp_bbox.mids.ary[cnt] = 0.5f * ( obj_tree->bsp_bbox.mins.ary[cnt] + obj_tree->bsp_bbox.maxs.ary[cnt] );
    }

    BSP_aabb_validate( &( obj_tree->bsp_bbox ) );

    return true;
}

//--------------------------------------------------------------------------------------------
void obj_BSP_dtor(obj_BSP_t *self)
{
	EGOBOO_ASSERT(NULL != self);

    // Deallocate everything.
    obj_BSP_deallocTree(self);

    // Run the destructors on all of the sub-objects.
    BSP_tree_dtor(&(self->tree ));
}

//--------------------------------------------------------------------------------------------
obj_BSP_t *obj_BSP_new(int dim, const mesh_BSP_t *mesh_bsp)
{
	EGOBOO_ASSERT(NULL != mesh_bsp);
	obj_BSP_t *self = (obj_BSP_t *)malloc(sizeof(obj_BSP_t));
	if (!self)
	{
		log_error("%s:%d: unable to allocate %zu Bytes\n",__FILE__,__LINE__,sizeof(obj_BSP_t));
		return NULL;
	}
	if (!obj_BSP_ctor(self, dim, mesh_bsp))
	{
		free(self);
		return NULL;
	}
	return self;
}

void obj_BSP_delete(obj_BSP_t *self)
{
	EGOBOO_ASSERT(NULL != self);
	obj_BSP_dtor(self);
	free(self);
}

/**
 * @brief
 *	Fill the collision list with references to tiles that the object volume may overlap.
 * @return
 *	return the number of collisions found
 * @author
 *	BB
 */
int obj_BSP_collide_aabb(const obj_BSP_t *self, const aabb_t *aabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst)
{
    if (NULL == self || NULL == aabb || NULL == colst) return 0;

    // Infinite nodes.
    return BSP_tree_collide_aabb(&(self->tree), aabb, ptest, colst);
}

/**
 * @brief
 *	Fill the collision list with references to tiles that the object volume may overlap.
 * @return
 *	the number of collisions found
 * @author BB
 */
int obj_BSP_collide_frustum(const obj_BSP_t *self, const egolib_frustum_t * frustum, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst)
{
    if (NULL == self || NULL == frustum || NULL == colst) return 0;

    // Infinite nodes.
    return BSP_tree_collide_frustum(&(self->tree), frustum, ptest, colst);
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool chr_BSP_can_collide(BSP_leaf_t * pchr_leaf)
{
    chr_t * pchr;

    bool can_be_reaffirmed;
    bool can_grab_money;
    bool can_use_platforms;
    bool can_collide;

    bool requires_chr_chr;
    bool requires_chr_prt;

    // make sure we have a character leaf
    if ( NULL == pchr_leaf || NULL == pchr_leaf->data || BSP_LEAF_CHR != pchr_leaf->data_type )
    {
        return false;
    }
    pchr = ( chr_t * )( pchr_leaf->data );

    if ( !ACTIVE_PCHR( pchr ) ) return false;

    // no interactions with hidden objects
    if ( pchr->is_hidden ) return false;

    // no interactions with packed objects
    if ( VALID_CHR_RANGE( pchr->inwhich_inventory ) ) return false;

    // generic flags for character interaction
    can_be_reaffirmed = ( pchr->reaffirm_damagetype < DAMAGE_COUNT );
    can_grab_money    = pchr->cangrabmoney;
    can_use_platforms = pchr->canuseplatforms;
    can_collide       = ( 0 != pchr->bump_stt.size ) && ( INVALID_CHR_REF == pchr->attachedto );

    // conditions for normal chr-chr interaction
    // platform tests are done elsewhere
    requires_chr_chr = can_collide /* || can_use_platforms */;

    // conditions for chr-prt interaction
    requires_chr_prt = can_be_reaffirmed /* || can_grab_money */;

    // even if an object does not interact with other characters,
    // it must still be inserted if it might interact with a particle
    if ( !requires_chr_chr && !requires_chr_prt ) return false;

    if ( oct_bb_empty( &( pchr->chr_max_cv ) ) ) return false;

    return true;
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool prt_BSP_can_collide(BSP_leaf_t * pprt_leaf)
{
    prt_t * pprt;
    pip_t * ppip;

    // Each one of these tests allows one MORE reason to include the particle, not one less.
    // Removed bump particles. We have another loop that can detect these, and there
    // is no reason to fill up the BSP with particles like coins.
    bool       has_enchant;
    bool       does_damage;
    bool       does_status_effect;
    bool       does_special_effect;
    bool       can_push;

    // make sure we have a character leaf
    if ( NULL == pprt_leaf || NULL == pprt_leaf->data || BSP_LEAF_PRT != pprt_leaf->data_type )
    {
        return false;
    }
    pprt = ( prt_t * )( pprt_leaf->data );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return false;
    ppip = PipStack_get_ptr( pprt->pip_ref );

    // is the particle in-game?
    if ( !INGAME_PPRT_BASE( pprt ) || pprt->is_hidden || pprt->is_ghost ) return false;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    has_enchant = false;
    if ( ppip->spawnenchant )
    {
        if ( !LOADED_PRO( pprt->profile_ref ) )
        {
            pro_t * ppro = ProList_get_ptr( pprt->profile_ref );
            has_enchant = LOADED_EVE( ppro->ieve );
        }
    }

    // any possible damage?
    does_damage         = ( ABS( pprt->damage.base ) + ABS( pprt->damage.rand ) ) > 0;

    // the other possible status effects
    // do not require damage
    does_status_effect  = ( 0 != ppip->grog_time ) || ( 0 != ppip->daze_time ) || ( 0 != ppip->lifedrain ) || ( 0 != ppip->manadrain );

    // these are not implemented yet
    does_special_effect = ppip->cause_pancake || ppip->cause_roll;

    // according to v1.0, only particles that cause damage can push
    can_push            = does_damage && ppip->allowpush;

    // particles with no effect
    if ( !can_push && !has_enchant && !does_damage && !does_status_effect && !does_special_effect ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool chr_BSP_is_visible(BSP_leaf_t * pchr_leaf)
{
    // make sure we have a character leaf
    if (NULL == pchr_leaf || NULL == pchr_leaf->data || BSP_LEAF_CHR != pchr_leaf->data_type)
    {
        return false;
    }
	chr_t *pchr = (chr_t *)(pchr_leaf->data);

    if (!ACTIVE_PCHR(pchr)) return false;

    // no interactions with hidden objects
    if (pchr->is_hidden) return false;

    // no interactions with packed objects
    if (VALID_CHR_RANGE(pchr->inwhich_inventory)) return false;

    return true;
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool prt_BSP_is_visible(BSP_leaf_t * pprt_leaf)
{
    // make sure we have a character leaf
    if (NULL == pprt_leaf || NULL == pprt_leaf->data || BSP_LEAF_PRT != pprt_leaf->data_type)
    {
        return false;
    }
	prt_t *pprt = (prt_t *)(pprt_leaf->data);

    // is the particle in-game?
    if (!INGAME_PPRT_BASE(pprt) || pprt->is_hidden) return false;

    // zero sized particles are not visible
    if (0 == pprt->size)
    {
        return false;
    }
    else if (pprt->inst.valid && pprt->inst.size <= 0.0f)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------

////--------------------------------------------------------------------------------------------
//bool obj_BSP_insert_leaf( obj_BSP_t * pbsp, BSP_leaf_t * pleaf, int depth, int address_x[], int address_y[], int address_z[] )
//{
//    int i;
//    bool retval;
//    Uint32 index;
//    BSP_branch_t * pbranch, * pbranch_new;
//    BSP_tree_t * ptree = &( pbsp->tree );
//
//    retval = false;
//    if ( depth < 0 )
//    {
//        // this can only happen if the node does not intersect the BSP bounding box
//        pleaf->next = ptree->infinite;
//        ptree->infinite = pleaf;
//        retval = true;
//    }
//    else if ( 0 == depth )
//    {
//        // this can only happen if the object should be in the root node list
//        pleaf->next = ptree->root->nodes;
//        ptree->root->nodes = pleaf;
//        retval = true;
//    }
//    else
//    {
//        // insert the node into the tree at this point
//        pbranch = ptree->root;
//        for ( i = 0; i < depth; i++ )
//        {
//            index = (( Uint32 )address_x[i] ) | ((( Uint32 )address_y[i] ) << 1 ) | ((( Uint32 )address_z[i] ) << 2 ) ;
//
//            pbranch_new = BSP_tree_ensure_branch( ptree, pbranch, index );
//            if ( NULL == pbranch_new ) break;
//
//            pbranch = pbranch_new;
//        };
//
//        // insert the node in this branch
//        retval = BSP_tree_insert( ptree, pbranch, pleaf, -1 );
//    }
//
//    return retval;
//}
//

////--------------------------------------------------------------------------------------------
//bool obj_BSP_collide_branch( BSP_branch_t * pbranch, oct_bb_t * pvbranch, oct_bb_t * pvobj, int_ary_t * colst )
//{
//    /// @author BB
/// @details Recursively search the BSP tree for collisions with the pvobj
//    //      Return false if we need to break out of the recursive search for any reson.
//
//    Uint32 i;
//    oct_bb_t    int_ov, tmp_ov;
//    float x_mid, y_mid, z_mid;
//    int address_x, address_y, address_z;
//
//    if ( NULL == colst ) return false;
//    if ( NULL == pvbranch || oct_bb_empty( *pvbranch ) ) return false;
//    if ( NULL == pvobj  || oct_bb_empty( *pvobj ) ) return false;
//
//    // return if the object does not intersect the branch
//    if ( !oct_bb_intersection( pvobj, pvbranch, &int_ov ) )
//    {
//        return false;
//    }
//
//    if ( !obj_BSP_collide_nodes( pbranch->nodes, pvobj, colst ) )
//    {
//        return false;
//    };
//
//    // check for collisions with any of the children
//    x_mid = ( pvbranch->maxs[OCT_X] + pvbranch->mins[OCT_X] ) * 0.5f;
//    y_mid = ( pvbranch->maxs[OCT_Y] + pvbranch->mins[OCT_Y] ) * 0.5f;
//    z_mid = ( pvbranch->maxs[OCT_Z] + pvbranch->mins[OCT_Z] ) * 0.5f;
//    for ( i = 0; i < pbranch->child_count; i++ )
//    {
//        // scan all the children
//        if ( NULL == pbranch->children[i] ) continue;
//
//        // create the volume of this node
//        address_x = i & ( 1 << 0 );
//        address_y = i & ( 1 << 1 );
//        address_z = i & ( 1 << 2 );
//
//        tmp_ov = *( pvbranch );
//
//        if ( 0 == address_x )
//        {
//            tmp_ov.maxs[OCT_X] = x_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_X] = x_mid;
//        }
//
//        if ( 0 == address_y )
//        {
//            tmp_ov.maxs[OCT_Y] = y_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_X] = y_mid;
//        }
//
//        if ( 0 == address_z )
//        {
//            tmp_ov.maxs[OCT_Z] = z_mid;
//        }
//        else
//        {
//            tmp_ov.mins[OCT_Z] = z_mid;
//        }
//
//        if ( oct_bb_intersection( pvobj, &tmp_ov, &int_ov ) )
//        {
//            // potential interaction with the child. go recursive!
//            bool ret = obj_BSP_collide_branch( pbranch->children[i], &( tmp_ov ), pvobj, colst );
//            if ( !ret ) return ret;
//        }
//    }
//
//    return true;
//}
//

////--------------------------------------------------------------------------------------------
//bool obj_BSP_collide_nodes( BSP_leaf_t leaf_lst[], oct_bb_t * pvobj, int_ary_t * colst )
//{
//    /// @author BB
//    /// @details check for collisions with the given node list
//
//    BSP_leaf_t * pleaf;
//    oct_bb_t    int_ov, * pnodevol;
//
//    if ( NULL == leaf_lst || NULL == pvobj ) return false;
//
//    if ( 0 == int_ary_get_size( colst ) || int_ary_get_top( colst ) >= int_ary_get_size( colst ) ) return false;
//
//    // check for collisions with any of the nodes of this branch
//    for ( pleaf = leaf_lst; NULL != pleaf; pleaf = pleaf->next )
//    {
//        if ( NULL == pleaf ) EGOBOO_ASSERT( false );
//
//        // get the volume of the node
//        pnodevol = NULL;
//        if ( BSP_LEAF_CHR == pleaf->data_type )
//        {
//            chr_t * pchr = ( chr_t* )pleaf->data;
//            pnodevol = &( pchr->chr_max_cv );
//        }
//        else if ( BSP_LEAF_PRT == pleaf->data_type )
//        {
//            prt_t * pprt = ( prt_t* )pleaf->data;
//            pnodevol = &( pprt->prt_max_cv );
//        }
//        else
//        {
//            continue;
//        }
//
//        if ( oct_bb_intersection( pvobj, pnodevol, &int_ov ) )
//        {
//            // we have a possible intersection
//            int_ary_push_back( colst, pleaf->index *(( BSP_LEAF_CHR == pleaf->data_type ) ? 1 : -1 ) );
//
//            if ( int_ary_get_top( colst ) >= int_ary_get_size( colst ) )
//            {
//                // too many nodes. break out of the search.
//                return false;
//            };
//        }
