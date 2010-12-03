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

/// @file mpd_BSP.c
/// @brief Implementation of functions for the mpd BSP
/// @details

#include "mpd_BSP.h"

#include "mesh.inl"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool_t _mpd_BSP_system_initialized = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

mpd_BSP_t mpd_BSP_root =
{
    OCT_BB_INIT_VALS, DYNAMIC_ARY_INIT_VALS, BSP_TREE_INIT_VALS
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void mpd_BSP_system_begin( ego_mpd_t * pmpd )
{
    if ( _mpd_BSP_system_initialized )
    {
        mpd_BSP_system_end();
    }

    if ( NULL != pmpd )
    {
        mpd_BSP_t * rv;

        // initialize the mesh's BSP structure with the mesh tiles
        rv = mpd_BSP_ctor( &mpd_BSP_root, pmpd );

        _mpd_BSP_system_initialized = ( NULL != rv );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void mpd_BSP_system_end()
{
    if ( _mpd_BSP_system_initialized )
    {
        mpd_BSP_dtor( &mpd_BSP_root );
    }

    _mpd_BSP_system_initialized = bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpd_BSP_t * mpd_BSP_ctor( mpd_BSP_t * pbsp, ego_mpd_t * pmesh )
{
    /// @details BB@> Create a new BSP tree for the mesh.
    //     These parameters duplicate the max resolution of the old system.

    int size_x, size_y;
    int depth;

    if ( NULL == pbsp ) return NULL;

    memset( pbsp, 0, sizeof( *pbsp ) );

    if ( NULL == pmesh ) return pbsp;
    size_x = pmesh->gmem.grids_x;
    size_y = pmesh->gmem.grids_y;

    // determine the number of bifurcations necessary to get cells the size of the "blocks"
    depth = CEIL( LOG( 0.5f * MAX( size_x, size_y ) ) / LOG( 2.0f ) );

    // make a 2D BSP tree with "max depth" depth
    BSP_tree_ctor( &( pbsp->tree ), 2, depth );

    mpd_BSP_alloc( pbsp, pmesh );

    return pbsp;
}

//--------------------------------------------------------------------------------------------
mpd_BSP_t * mpd_BSP_dtor( mpd_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return NULL;

    // free all allocated memory
    mpd_BSP_free( pbsp );

    // set the volume to zero
    oct_bb_ctor( &( pbsp->volume ) );

    return pbsp;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP_alloc( mpd_BSP_t * pbsp, ego_mpd_t * pmesh )
{
    Uint32 i;

    if ( NULL == pbsp || NULL == pmesh ) return bfalse;

    if ( 0 == pmesh->gmem.grid_count ) return bfalse;

    // allocate the BSP_leaf_t list, the containers for the actual tiles
    BSP_leaf_ary_ctor( &( pbsp->nodes ), pmesh->gmem.grid_count );
    if ( NULL == pbsp->nodes.ary ) return bfalse;

    // initialize the bounding volume size
    oct_bb_copy( &( pbsp->volume ), &( pmesh->tmem.tile_list[0].oct ) );

    // construct the BSP_leaf_t list
    for ( i = 1; i < pmesh->gmem.grid_count; i++ )
    {
        BSP_leaf_t      * pleaf = pbsp->nodes.ary + i;
        ego_tile_info_t * ptile = pmesh->tmem.tile_list + i;

        // add the bounding volume for this tile to the bounding volume for the mesh
        oct_bb_self_union( &( pbsp->volume ), &( ptile->oct ) );

        // let data type 1 stand for a tile, -1 is uninitialized
        BSP_leaf_ctor( pleaf, 2, pmesh->tmem.tile_list + i, 1 );
        pleaf->index = i;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP_free( mpd_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return bfalse;

    // deallocate the tree
    BSP_tree_dealloc( &( pbsp->tree ) );

    // deallocate the nodes
    BSP_leaf_ary_dtor( &( pbsp->nodes ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP_fill( mpd_BSP_t * pbsp )
{
    int tile;

    for ( tile = 0; tile < pbsp->nodes.top; tile++ )
    {
        ego_tile_info_t * pdata;
        BSP_leaf_t      * pleaf = pbsp->nodes.ary + tile;

        // do not deal with uninitialized nodes
        if ( pleaf->data_type < 0 ) continue;

        // grab the leaf data, assume that it points to the correct data structure
        pdata = ( ego_tile_info_t* ) pleaf->data;
        if ( NULL == pdata ) continue;

        // calculate the leaf's BSP_aabb_t
        BSP_aabb_from_oct_bb( &( pleaf->bbox ), &( pdata->oct ) );

        // insert the leaf
        BSP_tree_insert_leaf( &( pbsp->tree ), pleaf );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
int mpd_BSP_collide( mpd_BSP_t * pbsp, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst )
{
    /// @details BB@> fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    if ( NULL == pbsp || NULL == paabb ) return 0;

    if ( NULL == colst ) return 0;
    colst->top = 0;
    if ( 0 == colst->alloc ) return 0;

    // collide with any "infinite" nodes
    return BSP_tree_collide( &( pbsp->tree ), paabb, colst );
}

////--------------------------------------------------------------------------------------------
//bool_t mpd_BSP_insert_node( mpd_BSP_t * pbsp, BSP_leaf_t * pnode, int depth, int address_x[], int address_y[] )
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
//        pnode->next = ptree->infinite;
//        ptree->infinite = pnode;
//        retval = btrue;
//    }
//    else if ( 0 == depth )
//    {
//        // this can only happen if the tile should be in the root node list
//        pnode->next = ptree->root->nodes;
//        ptree->root->nodes = pnode;
//        retval = btrue;
//    }
//    else
//    {
//        // insert the node into the tree at this point
//        pbranch = ptree->root;
//        for ( i = 0; i < depth; i++ )
//        {
//            index = (( Uint32 )address_x[i] ) + ((( Uint32 )address_y[i] ) << 1 );
//
//            pbranch_new = BSP_tree_ensure_branch( ptree, pbranch, index );
//            if ( NULL == pbranch_new ) break;
//
//            pbranch = pbranch_new;
//        };
//
//        // insert the node in this branch
//        retval = BSP_tree_insert( ptree, pbranch, pnode, -1 );
//    };
//
//    return retval;
//}
//
