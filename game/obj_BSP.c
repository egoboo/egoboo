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

/// @file obj_BSP.c
/// @brief Implementation of functions for the object BSP
/// @details

#include "obj_BSP.h"

#include "mpd_BSP.h"

#include "char.inl"
#include "particle.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool_t _obj_BSP_system_initialized = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

obj_BSP_t chr_BSP_root = OBJ_BSP_INIT_VALS;
obj_BSP_t prt_BSP_root = OBJ_BSP_INIT_VALS;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void obj_BSP_system_begin( mpd_BSP_t * pBSP )
{
    /// @author BB
    /// @details initialize the obj_BSP list and load up some intialization files
    ///     necessary for the the obj_BSP loading code to work

    if ( _obj_BSP_system_initialized )
    {
        obj_BSP_system_end();
    }

    // use 2D BSPs for the moment
    obj_BSP_ctor( &chr_BSP_root, 2, pBSP );
    obj_BSP_ctor( &prt_BSP_root, 2, pBSP );

    // let the code know that everything is initialized
    _obj_BSP_system_initialized = btrue;
}

//--------------------------------------------------------------------------------------------
void obj_BSP_system_end()
{
    /// @author BB
    /// @details initialize the obj_BSP list and load up some intialization files
    ///     necessary for the the obj_BSP loading code to work

    if ( _obj_BSP_system_initialized )
    {
        // delete the object BSP data
        obj_BSP_dtor( &chr_BSP_root );
        obj_BSP_dtor( &prt_BSP_root );

        _obj_BSP_system_initialized = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t obj_BSP_alloc( obj_BSP_t * pbsp, int dim, int depth )
{
    // allocate make a 2D BSP tree

    BSP_tree_t * rv;

    if ( NULL == pbsp ) return bfalse;

    obj_BSP_free( pbsp );

    rv = BSP_tree_ctor( &( pbsp->tree ), dim, depth );

    return ( NULL != rv );
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_free( obj_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    BSP_tree_dealloc( &( pbsp->tree ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_ctor( obj_BSP_t * pbsp, int bsp_dim, const mpd_BSP_t * pmesh_bsp )
{
    /// @author BB
    /// @details Create a new BSP tree for game objects.
    ///     These parameters duplicate the max resolution of the old system.

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

    if ( NULL == pmesh_bsp ) return bfalse;
    mesh_tree = &( pmesh_bsp->tree );
    mesh_dim = pmesh_bsp->tree.dimensions;

    if ( NULL == pbsp ) return bfalse;
    obj_tree = &( pbsp->tree );

    BLANK_STRUCT_PTR( pbsp )

    // allocate the data
    obj_BSP_alloc( pbsp, bsp_dim, mesh_tree->max_depth );

    // find the maximum extent of the bsp
    bsp_size = 0.0f;
    min_dim = MIN( bsp_dim, mesh_dim );

    for ( cnt = 0; cnt < min_dim; cnt ++ )
    {
        float tmp_size = ABS( mesh_tree->bsp_bbox.maxs.ary[cnt] - mesh_tree->bsp_bbox.mins.ary[cnt] );
        bsp_size = MAX( bsp_size, tmp_size );
    }

    // copy the volume from the mesh
    for ( cnt = 0; cnt < min_dim; cnt++ )
    {
        // get the size
        obj_tree->bsp_bbox.mins.ary[cnt] = MIN( mesh_tree->bsp_bbox.mins.ary[cnt], mesh_tree->bbox.data.mins[cnt] );
        obj_tree->bsp_bbox.maxs.ary[cnt] = MAX( mesh_tree->bsp_bbox.maxs.ary[cnt], mesh_tree->bbox.data.maxs[cnt] );

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
        obj_tree->bsp_bbox.mins.ary[kZ] = MIN( -bsp_size, obj_tree->bsp_bbox.mins.ary[kZ] );
        obj_tree->bsp_bbox.maxs.ary[kZ] = MAX( bsp_size, obj_tree->bsp_bbox.maxs.ary[kZ] );
    }

    // calculate the mid positions
    for ( cnt = 0; cnt < bsp_dim; cnt++ )
    {
        obj_tree->bsp_bbox.mids.ary[cnt] = 0.5f * ( obj_tree->bsp_bbox.mins.ary[cnt] + obj_tree->bsp_bbox.maxs.ary[cnt] );
    }

    BSP_aabb_validate( &( obj_tree->bsp_bbox ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t obj_BSP_dtor( obj_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // deallocate everything
    obj_BSP_free( pbsp );

    // run the destructors on all of the sub-objects
    BSP_tree_dtor( &( pbsp->tree ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_BSP_insert( chr_t * pchr )
{
    /// @author BB
    /// @details insert a character's BSP_leaf_t into the BSP_tree_t

    bool_t       retval;
    BSP_leaf_t * pleaf;
    BSP_tree_t * ptree;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    ptree = &( chr_BSP_root.tree );

    // no interactions with hidden objects
    if ( pchr->is_hidden ) return bfalse;

    // heal the leaf if it needs it
    pleaf = &( pchr->bsp_leaf );
    if ( pchr != ( chr_t * )( pleaf->data ) )
    {
        // some kind of error. re-initialize the data.
        pleaf->data      = pchr;
        pleaf->index     = GET_INDEX_PCHR( pchr );
        pleaf->data_type = BSP_LEAF_CHR;
    };

    // do the insert
    retval = bfalse;
    if ( !oct_bb_empty( &( pchr->chr_max_cv ) ) )
    {
        oct_bb_t tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr, 0.0f, 1.0f, &tmp_oct );

        // convert the bounding box
        ego_aabb_from_oct_bb( &( pleaf->bbox ), &tmp_oct );

        // insert the leaf
        retval = BSP_tree_insert_leaf( ptree, pleaf );
    }

    if ( retval )
    {
        chr_BSP_root.count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_BSP_insert( prt_bundle_t * pbdl_prt )
{
    /// @author BB
    /// @details insert a particle's BSP_leaf_t into the BSP_tree_t

    bool_t       retval;
    BSP_leaf_t * pleaf;
    BSP_tree_t * ptree;

    prt_t *loc_pprt;

    oct_bb_t tmp_oct;

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;
    loc_pprt = pbdl_prt->prt_ptr;
    ptree = &( prt_BSP_root.tree );

    // is the particle in-game?
    if ( !INGAME_PPRT_BASE( loc_pprt ) || loc_pprt->is_hidden || loc_pprt->is_ghost ) return bfalse;

    // heal the leaf if necessary
    pleaf = &( loc_pprt->bsp_leaf );
    if ( loc_pprt != ( prt_t * )( pleaf->data ) )
    {
        // some kind of error. re-initialize the data.
        pleaf->data      = loc_pprt;
        pleaf->index     = GET_INDEX_PPRT( loc_pprt );
        pleaf->data_type = BSP_LEAF_PRT;
    };

    // use the object velocity to figure out where the volume that the object will occupy during this
    // update
    phys_expand_prt_bb( loc_pprt, 0.0f, 1.0f, &tmp_oct );

    // convert the bounding box
    ego_aabb_from_oct_bb( &( pleaf->bbox ), &tmp_oct );

    retval = BSP_tree_insert_leaf( ptree, pleaf );
    if ( retval )
    {
        prt_BSP_root.count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_BSP_clear()
{
    CHR_REF ichr;

    // unlink all the BSP nodes
    BSP_tree_clear_rec( &( chr_BSP_root.tree ) );
    chr_BSP_root.count = 0;

    // unlink all used character nodes
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        BSP_leaf_unlink( &( ChrList.lst[ichr].bsp_leaf ) );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_BSP_clear()
{
    PRT_REF iprt;

    // unlink all the BSP nodes
    BSP_tree_clear_rec( &( prt_BSP_root.tree ) );
    prt_BSP_root.count = 0;

    // unlink all used particle nodes
    for ( iprt = 0; iprt < MAX_PRT; iprt++ )
    {
        BSP_leaf_unlink( &( PrtList.lst[iprt].bsp_leaf ) );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_BSP_fill()
{
    // insert the characters
    chr_BSP_root.count = 0;
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        // reset a couple of things here
        pchr->holdingweight        = 0;
        pchr->onwhichplatform_ref  = ( CHR_REF )MAX_CHR;
        pchr->targetplatform_ref   = ( CHR_REF )MAX_CHR;
        pchr->targetplatform_level = -1e32;

        // try to insert the character
        chr_BSP_insert( pchr );
    }
    CHR_END_LOOP()

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_BSP_fill()
{
    // insert the particles
    prt_BSP_root.count = 0;
    PRT_BEGIN_LOOP_ACTIVE( iprt, prt_bdl )
    {
        // reset a couple of things here
        prt_bdl.prt_ptr->onwhichplatform_ref  = ( CHR_REF )MAX_CHR;
        prt_bdl.prt_ptr->targetplatform_ref   = ( CHR_REF )MAX_CHR;
        prt_bdl.prt_ptr->targetplatform_level = -1e32;

        // try to insert the particle
        prt_BSP_insert( &prt_bdl );
    }
    PRT_END_LOOP()

    return btrue;
}

//--------------------------------------------------------------------------------------------
int obj_BSP_collide_aabb( const obj_BSP_t * pbsp, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @author BB
    /// @details fill the collision list with references to tiles that the object volume may overlap.
    ///      Return the number of collisions found.

    if ( NULL == pbsp || NULL == paabb || NULL == colst ) return 0;

    // infinite nodes
    return BSP_tree_collide_aabb( &( pbsp->tree ), paabb, ptest, colst );
}

//--------------------------------------------------------------------------------------------
int obj_BSP_collide_frustum( const obj_BSP_t * pbsp, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst )
{
    /// @author BB
    /// @details fill the collision list with references to tiles that the object volume may overlap.
    ///      Return the number of collisions found.

    if ( NULL == pbsp || NULL == pfrust || NULL == colst ) return 0;

    // infinite nodes
    return BSP_tree_collide_frustum( &( pbsp->tree ), pfrust, ptest, colst );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t chr_BSP_can_collide( BSP_leaf_t * pchr_leaf )
{
    /// @author BB
    /// @details a test function passed to BSP_*_collide_* functions to determine whether a leaf
    ///               can be added to a collision list

    chr_t * pchr;

    bool_t can_be_reaffirmed;
    bool_t can_grab_money;
    bool_t can_use_platforms;
    bool_t can_collide;

    bool_t requires_chr_chr;
    bool_t requires_chr_prt;

    // make sure we have a character leaf
    if ( NULL == pchr_leaf || NULL == pchr_leaf->data || BSP_LEAF_CHR != pchr_leaf->data_type )
    {
        return bfalse;
    }
    pchr = ( chr_t * )( pchr_leaf->data );

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // no interactions with hidden objects
    if ( pchr->is_hidden ) return bfalse;

    // no interactions with packed objects
    if ( VALID_CHR_RANGE( pchr->inwhich_inventory ) ) return bfalse;

    // generic flags for character interaction
    can_be_reaffirmed = ( pchr->reaffirm_damagetype < DAMAGE_COUNT );
    can_grab_money    = pchr->cangrabmoney;
    can_use_platforms = pchr->canuseplatforms;
    can_collide       = ( 0 != pchr->bump_stt.size ) && ( MAX_CHR == pchr->attachedto );

    // conditions for normal chr-chr interaction
    // platform tests are done elsewhere
    requires_chr_chr = can_collide /* || can_use_platforms */;

    // conditions for chr-prt interaction
    requires_chr_prt = can_be_reaffirmed /* || can_grab_money */;

    // even if an object does not interact with other characters,
    // it must still be inserted if it might interact with a particle
    if ( !requires_chr_chr && !requires_chr_prt ) return bfalse;

    if ( oct_bb_empty( &( pchr->chr_max_cv ) ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_BSP_can_collide( BSP_leaf_t * pprt_leaf )
{
    /// @author BB
    /// @details a test function passed to BSP_*_collide_* functions to determine whether a leaf
    ///               can be added to a collision list

    prt_t * pprt;
    pip_t * ppip;

    // Each one of these tests allows one MORE reason to include the particle, not one less.
    // Removed bump particles. We have another loop that can detect these, and there
    // is no reason to fill up the BSP with particles like coins.
    bool_t       has_enchant;
    bool_t       does_damage;
    bool_t       does_status_effect;
    bool_t       does_special_effect;
    bool_t       can_push;

    // make sure we have a character leaf
    if ( NULL == pprt_leaf || NULL == pprt_leaf->data || BSP_LEAF_PRT != pprt_leaf->data_type )
    {
        return bfalse;
    }
    pprt = ( prt_t * )( pprt_leaf->data );

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack_get_ptr( pprt->pip_ref );

    // is the particle in-game?
    if ( !INGAME_PPRT_BASE( pprt ) || pprt->is_hidden || pprt->is_ghost ) return bfalse;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    has_enchant = bfalse;
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
    if ( !can_push && !has_enchant && !does_damage && !does_status_effect && !does_special_effect ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t chr_BSP_is_visible( BSP_leaf_t * pchr_leaf )
{
    /// @author BB
    /// @details a test function passed to BSP_*_collide_* functions to determine whether a leaf
    ///               can be added to a collision list

    chr_t * pchr;

    // make sure we have a character leaf
    if ( NULL == pchr_leaf || NULL == pchr_leaf->data || BSP_LEAF_CHR != pchr_leaf->data_type )
    {
        return bfalse;
    }
    pchr = ( chr_t * )( pchr_leaf->data );

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // no interactions with hidden objects
    if ( pchr->is_hidden ) return bfalse;

    // no interactions with packed objects
    if ( VALID_CHR_RANGE( pchr->inwhich_inventory ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_BSP_is_visible( BSP_leaf_t * pprt_leaf )
{
    /// @author BB
    /// @details a test function passed to BSP_*_collide_* functions to determine whether a leaf
    ///               can be added to a collision list

    prt_t * pprt;

    // make sure we have a character leaf
    if ( NULL == pprt_leaf || NULL == pprt_leaf->data || BSP_LEAF_PRT != pprt_leaf->data_type )
    {
        return bfalse;
    }
    pprt = ( prt_t * )( pprt_leaf->data );

    // is the particle in-game?
    if ( !INGAME_PPRT_BASE( pprt ) || pprt->is_hidden ) return bfalse;

    // zero sized particles are not visible
    if ( 0 == pprt->size )
    {
        return bfalse;
    }
    else if ( pprt->inst.valid && pprt->inst.size <= 0.0f )
    {
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------

////--------------------------------------------------------------------------------------------
//bool_t obj_BSP_insert_leaf( obj_BSP_t * pbsp, BSP_leaf_t * pleaf, int depth, int address_x[], int address_y[], int address_z[] )
//{
//    int i;
//    bool_t retval;
//    Uint32 index;
//    BSP_branch_t * pbranch, * pbranch_new;
//    BSP_tree_t * ptree = &( pbsp->tree );
//
//    retval = bfalse;
//    if ( depth < 0 )
//    {
//        // this can only happen if the node does not intersect the BSP bounding box
//        pleaf->next = ptree->infinite;
//        ptree->infinite = pleaf;
//        retval = btrue;
//    }
//    else if ( 0 == depth )
//    {
//        // this can only happen if the object should be in the root node list
//        pleaf->next = ptree->root->nodes;
//        ptree->root->nodes = pleaf;
//        retval = btrue;
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
//bool_t obj_BSP_collide_branch( BSP_branch_t * pbranch, oct_bb_t * pvbranch, oct_bb_t * pvobj, int_ary_t * colst )
//{
//    /// @author BB
/// @details Recursively search the BSP tree for collisions with the pvobj
//    //      Return bfalse if we need to break out of the recursive search for any reson.
//
//    Uint32 i;
//    oct_bb_t    int_ov, tmp_ov;
//    float x_mid, y_mid, z_mid;
//    int address_x, address_y, address_z;
//
//    if ( NULL == colst ) return bfalse;
//    if ( NULL == pvbranch || oct_bb_empty( *pvbranch ) ) return bfalse;
//    if ( NULL == pvobj  || oct_bb_empty( *pvobj ) ) return bfalse;
//
//    // return if the object does not intersect the branch
//    if ( !oct_bb_intersection( pvobj, pvbranch, &int_ov ) )
//    {
//        return bfalse;
//    }
//
//    if ( !obj_BSP_collide_nodes( pbranch->nodes, pvobj, colst ) )
//    {
//        return bfalse;
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
//            bool_t ret = obj_BSP_collide_branch( pbranch->children[i], &( tmp_ov ), pvobj, colst );
//            if ( !ret ) return ret;
//        }
//    }
//
//    return btrue;
//}
//

////--------------------------------------------------------------------------------------------
//bool_t obj_BSP_collide_nodes( BSP_leaf_t leaf_lst[], oct_bb_t * pvobj, int_ary_t * colst )
//{
//    /// @author BB
//    /// @details check for collisions with the given node list
//
//    BSP_leaf_t * pleaf;
//    oct_bb_t    int_ov, * pnodevol;
//
//    if ( NULL == leaf_lst || NULL == pvobj ) return bfalse;
//
//    if ( 0 == int_ary_get_size( colst ) || int_ary_get_top( colst ) >= int_ary_get_size( colst ) ) return bfalse;
//
//    // check for collisions with any of the nodes of this branch
//    for ( pleaf = leaf_lst; NULL != pleaf; pleaf = pleaf->next )
//    {
//        if ( NULL == pleaf ) EGOBOO_ASSERT( bfalse );
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
//                return bfalse;
//            };
//        }
