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

/// @file  game/map_BSP.c
/// @brief BSPs for meshes.

#include "game/mesh_BSP.h"
#include "egolib/_math.h"
#include "egolib/bsp.h"
#include "egolib/bbox.h"
#include "game/obj_BSP.h"
#include "game/mesh.h"


static bool  mesh_BSP_insert(mesh_BSP_t * pbsp, ego_tile_info_t * ptile, int index);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mesh_BSP_t *mesh_BSP_t::ctor(mesh_BSP_t *self, const ego_mesh_t *mesh)
{
	EGOBOO_ASSERT(NULL != self && NULL != mesh);
    BLANK_STRUCT_PTR(self)

    // get the nominal physical size of the mesh
    float x_min = 0.0f;
    float x_max = mesh->gmem.edge_x;
    float y_min = 0.0f;
    float y_max = mesh->gmem.edge_y;
    float bsp_size = std::max( x_max - x_min, y_max - y_min );

    // determine the number of bifurcations necessary to get cells the size of the "blocks"
    int grids_x = mesh->gmem.grids_x;
    int grids_y = mesh->gmem.grids_y;
    int depth = CEIL(std::log( 0.5f * std::max( grids_x, grids_y ) ) / std::log( 2.0f ) );

    // make a 2D BSP tree with "max depth" depth
    // this automatically allocates all data
    self->tree.ctor(2, depth);

    // !!!!SET THE BSP SIZE HERE!!!!
    // enlarge it a bit
    self->tree.bsp_bbox.mins.ary[kX] = x_min - 0.25f * bsp_size;
    self->tree.bsp_bbox.maxs.ary[kX] = x_max + 0.25f * bsp_size;
    self->tree.bsp_bbox.mids.ary[kX] = 0.5f * (self->tree.bsp_bbox.mins.ary[kX] + self->tree.bsp_bbox.maxs.ary[kX]);

    self->tree.bsp_bbox.mins.ary[kY] = y_min - 0.25f * bsp_size;
    self->tree.bsp_bbox.maxs.ary[kY] = y_max + 0.25f * bsp_size;
    self->tree.bsp_bbox.mids.ary[kY] = 0.5f * (self->tree.bsp_bbox.mins.ary[kY] + self->tree.bsp_bbox.maxs.ary[kY]);

    // Initialize the volume.
	// @todo Error handling.
    oct_bb_ctor(&(self->volume));

    return self;
}

//--------------------------------------------------------------------------------------------
void mesh_BSP_t::dtor(mesh_BSP_t *self)
{
	if (nullptr == self)
	{
		return;
	}
    // Destruct the BSP tree.
    self->tree.dtor();
    BLANK_STRUCT_PTR(self)
}

mesh_BSP_t *mesh_BSP_new(const ego_mesh_t *mesh)
{
	mesh_BSP_t *self = (mesh_BSP_t *)malloc(sizeof(mesh_BSP_t));
	if (!self)
	{
		log_error("%s:%d: unable to allocate %zu Bytes\n", __FILE__, __LINE__, sizeof(mesh_BSP_t));
		return nullptr;
	}
	if (!mesh_BSP_t::ctor(self, mesh))
	{
		free(self);
		return nullptr;
	}
	return self;
}

void mesh_BSP_delete(mesh_BSP_t *self)
{
	EGOBOO_ASSERT(NULL != self);
	mesh_BSP_t::dtor(self);
	free(self);
}

//--------------------------------------------------------------------------------------------
bool mesh_BSP_fill(mesh_BSP_t *self, const ego_mesh_t *mesh)
{
    size_t cnt;

    size_t tcount;
    ego_tile_info_t * tlist, *ptile;

    // error trap
    if (NULL == self || NULL == mesh) return false;
    tcount = mesh->tmem.tile_count;
    tlist  = mesh->tmem.tile_list;

    // make sure the mesh is allocated
    if (0 == tcount || NULL == tlist) return false;

    // initialize the bsp volume
    // assumes tlist[0] is insterted
    oct_bb_copy(&(self->volume), &(tlist[0].oct));

    // insert each tile
    for (cnt = 0, ptile = tlist; cnt < tcount; cnt++, ptile++)
    {
        // try to insert the BSP
        if (mesh_BSP_insert(self, ptile, cnt ))
        {
            // add this tile's volume to the bsp's volume
            oct_bb_self_union(&(self->volume), &(ptile->oct));
        }
    }

    return self->count > 0;
}

size_t mesh_BSP_t::collide_aabb(const aabb_t *aabb, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const
{
    return tree.collide_aabb(aabb, test, collisions);
}


size_t mesh_BSP_t::collide_frustum(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, Ego::DynamicArray<BSP_leaf_t *>  *collisions) const
{
    return tree.collide_frustum(frustum, test, collisions);
}

/**
 * @brief
 *	Insert a tile's BSP_leaf_t into the BSP_tree_t.
 * @author
 *	BB
 */
static bool mesh_BSP_insert(mesh_BSP_t *self, ego_tile_info_t *ptile, int index)
{
    if (NULL == self || NULL == ptile) return false;
    BSP_tree_t *ptree = &(self->tree);

    // Grab the leaf from the tile.
    BSP_leaf_t *pleaf = &(ptile->bsp_leaf);

    // Make sure everything is kosher.
    if (ptile != (ego_tile_info_t *)(pleaf->data))
    {
        // Some kind of error. Re-initialize the data.
        pleaf->data      = ptile;
        pleaf->index     = index;
        pleaf->data_type = BSP_LEAF_TILE;
    };

    // Convert the octagonal bounding box to an AABB.
    bv_from_oct_bb(&(pleaf->bbox), &(ptile->oct));

    // Insert the leaf.
	bool retval = BSP_tree_insert_leaf(ptree, pleaf);

    if (retval)
    {
        // Log all successes.
        self->count++;
    }

    return retval;
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool mesh_BSP_can_collide(BSP_leaf_t *pleaf)
{
    // Make sure we have a character leaf.
    if (NULL == pleaf || NULL == pleaf->data || BSP_LEAF_TILE != pleaf->data_type)
    {
        return false;
    }
	ego_tile_info_t *ptile = (ego_tile_info_t *)(pleaf->data);

    if (TILE_IS_FANOFF(*ptile)) return false;

    return true;
}

/**
 * @brief
 *	A test function passed to BSP_*_collide_* functions to determine whether a leaf can be added to a collision list.
 * @author
 *	BB
 */
bool mesh_BSP_is_visible(BSP_leaf_t *pleaf)
{
    // Make sure we have a character leaf.
    if (NULL == pleaf || NULL == pleaf->data || BSP_LEAF_TILE != pleaf->data_type)
    {
        return false;
    }
	ego_tile_info_t *ptile = (ego_tile_info_t *)(pleaf->data);

    if (TILE_IS_FANOFF(*ptile)) return false;

    return true;
}
