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

obj_BSP_t chr_BSP_root = { 0, BSP_TREE_INIT_VALS };
obj_BSP_t prt_BSP_root = { 0, BSP_TREE_INIT_VALS };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void obj_BSP_system_begin( mpd_BSP_t * pBSP )
{
    /// @details BB@> initialize the obj_BSP list and load up some intialization files
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
    /// @details BB@> initialize the obj_BSP list and load up some intialization files
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
bool_t obj_BSP_ctor( obj_BSP_t * pbsp, int dim, mpd_BSP_t * pmesh_bsp )
{
    /// @details BB@> Create a new BSP tree for game objects.
    //     These parameters duplicate the max resolution of the old system.

    int          cnt;
    float        bsp_size;
    BSP_tree_t * t;

    if ( dim < 2 )
    {
        log_error( "obj_BSP_ctor() - cannot construct an object BSP with less than 2 dimensions\n" );
    }
    else if ( dim > 3 )
    {
        log_error( "obj_BSP_ctor() - cannot construct an object BSP with more than than 3 dimensions\n" );
    }

    if ( NULL == pbsp || NULL == pmesh_bsp ) return bfalse;

    memset( pbsp, 0, sizeof( *pbsp ) );

    // allocate the data
    obj_BSP_alloc( pbsp, dim, pmesh_bsp->tree.depth );

    t = &( pbsp->tree );

    //---- copy the volume from the mesh
    if ( dim > 0 )
    {
        t->bbox.mins.ary[kX] = pmesh_bsp->volume.mins[OCT_X];
        t->bbox.mins.ary[kY] = pmesh_bsp->volume.mins[OCT_Y];
    }

    if ( dim > 1 )
    {
        t->bbox.maxs.ary[kX] = pmesh_bsp->volume.maxs[OCT_X];
        t->bbox.maxs.ary[kY] = pmesh_bsp->volume.maxs[OCT_Y];
    }

    if ( dim > 2 )
    {
        // make some extra space in the z direction
        bsp_size = MAX( ABS( t->bbox.mins.ary[OCT_X] ), ABS( t->bbox.maxs.ary[OCT_X] ) );
        bsp_size = MAX( bsp_size, MAX( ABS( t->bbox.mins.ary[OCT_Y] ), ABS( t->bbox.maxs.ary[OCT_Y] ) ) );
        bsp_size = MAX( bsp_size, MAX( ABS( t->bbox.mins.ary[OCT_Z] ), ABS( t->bbox.maxs.ary[OCT_Z] ) ) );

        t->bbox.mins.ary[kZ] = -bsp_size * 2;
        t->bbox.maxs.ary[kZ] =  bsp_size * 2;
    }

    // calculate the mid positions
    for ( cnt = 0; cnt < dim; cnt++ )
    {
        t->bbox.mids.ary[cnt] = 0.5f * ( t->bbox.mins.ary[cnt] + t->bbox.maxs.ary[cnt] );
    }

    BSP_aabb_validate( &( t->bbox ) );

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
    /// @details BB@> insert a character's BSP_leaf_t into the BSP_tree_t

    bool_t       retval;
    BSP_leaf_t * pleaf;
    BSP_tree_t * ptree;

    bool_t can_be_reaffirmed;
    bool_t can_grab_money;
    bool_t can_use_platforms;
    bool_t can_collide;

    bool_t requires_chr_chr;
    bool_t requires_chr_prt;

    ptree = &( chr_BSP_root.tree );

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // no interactions with hidden objects
    if ( pchr->is_hidden )
        return bfalse;

    // no interactions with packed objects
    if ( pchr->pack.is_packed )
        return bfalse;

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

    pleaf = &( pchr->bsp_leaf );
    if ( pchr != ( chr_t * )( pleaf->data ) )
    {
        // some kind of error. re-initialize the data.
        pleaf->data      = pchr;
        pleaf->index     = GET_INDEX_PCHR( pchr );
        pleaf->data_type = BSP_LEAF_CHR;
    };

    retval = bfalse;
    if ( !oct_bb_empty( &( pchr->chr_max_cv ) ) )
    {
        oct_bb_t tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr, 0.0f, 1.0f, &tmp_oct );

        // convert the bounding box
        BSP_aabb_from_oct_bb( &( pleaf->bbox ), &tmp_oct );

        // insert the leaf
        retval = BSP_tree_insert_leaf( ptree, pleaf );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t prt_BSP_insert( prt_bundle_t * pbdl_prt )
{
    /// @details BB@> insert a particle's BSP_leaf_t into the BSP_tree_t

    bool_t       retval;
    BSP_leaf_t * pleaf;
    BSP_tree_t * ptree;

    prt_t *loc_pprt;
    pip_t *loc_ppip;

    oct_bb_t tmp_oct;

    // Each one of these tests allows one MORE reason to include the particle, not one less.
    // Removed bump particles. We have another loop that can detect these, and there
    // is no reason to fill up the BSP with particles like coins.
    bool_t       has_enchant;
    bool_t       does_damage;
    bool_t       does_status_effect;
    bool_t       does_special_effect;
    bool_t       can_push;

    ptree = &( prt_BSP_root.tree );

    if ( NULL == pbdl_prt || NULL == pbdl_prt->prt_ptr ) return bfalse;
    loc_pprt = pbdl_prt->prt_ptr;
    loc_ppip = pbdl_prt->pip_ptr;

    // is the particle in-game?
    if ( !INGAME_PRT_PBASE( loc_pprt ) || loc_pprt->is_hidden || loc_pprt->is_ghost ) return bfalse;

    // Make this optional? Is there any reason to fail if the particle has no profile reference?
    has_enchant = bfalse;
    if ( loc_ppip->spawnenchant )
    {
        if ( !LOADED_PRO( loc_pprt->profile_ref ) )
        {
            pro_t * ppro = ProList.lst + loc_pprt->profile_ref;
            has_enchant = LOADED_EVE( ppro->ieve );
        }
    }

    // any possible damage?
    does_damage         = ( ABS( loc_pprt->damage.base ) + ABS( loc_pprt->damage.rand ) ) > 0;

    // the other possible status effects
    // do not require damage
    does_status_effect  = ( 0 != loc_ppip->grog_time ) || ( 0 != loc_ppip->daze_time ) || ( 0 != loc_ppip->lifedrain ) || ( 0 != loc_ppip->manadrain );

    // these are not implemented yet
    does_special_effect = loc_ppip->cause_pancake || loc_ppip->cause_roll;

    // according to v1.0, only particles that cause damage can push
    can_push            = does_damage && loc_ppip->allowpush;

    // particles with no effect
    if ( !can_push && !has_enchant && !does_damage && !does_status_effect && !does_special_effect ) return bfalse;

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
    BSP_aabb_from_oct_bb( &( pleaf->bbox ), &tmp_oct );

    retval = BSP_tree_insert_leaf( ptree, pleaf );

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t chr_BSP_clear()
{
    CHR_REF ichr;

    // unlink all the BSP nodes
    BSP_tree_clear( &( chr_BSP_root.tree ), btrue );

    // unlink all used character nodes
    for ( ichr = 0; ichr < MAX_CHR; ichr++ )
    {
        ChrList.lst[ichr].bsp_leaf.next = NULL;
        ChrList.lst[ichr].bsp_leaf.inserted = bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t prt_BSP_clear()
{
    PRT_REF iprt;

    // unlink all the BSP nodes
    BSP_tree_clear( &( prt_BSP_root.tree ), btrue );

    // unlink all used particle nodes
    prt_BSP_root.count = 0;
    for ( iprt = 0; iprt < MAX_PRT; iprt++ )
    {
        PrtList.lst[iprt].bsp_leaf.next = NULL;
        PrtList.lst[iprt].bsp_leaf.inserted = bfalse;
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
        pchr->onwhichplatform_ref   = ( CHR_REF )MAX_CHR;
        pchr->targetplatform_ref   = ( CHR_REF )MAX_CHR;
        pchr->targetplatform_level = -1e32;

        // try to insert the character
        if ( chr_BSP_insert( pchr ) )
        {
            chr_BSP_root.count++;
        }
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
        if ( prt_BSP_insert( &prt_bdl ) )
        {
            prt_BSP_root.count++;
        }
    }
    PRT_END_LOOP()

    return btrue;
}

//--------------------------------------------------------------------------------------------
int obj_BSP_collide( obj_BSP_t * pbsp, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst )
{
    /// @details BB@> fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == pbsp || NULL == paabb || NULL == colst ) return 0;

    // infinite nodes
    return BSP_tree_collide( &( pbsp->tree ), paabb, colst );
}

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
//    /// @details BB@> Recursively search the BSP tree for collisions with the pvobj
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
//    /// @details BB@> check for collisions with the given node list
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
