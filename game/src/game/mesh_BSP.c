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
mesh_BSP_t::Parameters::Parameters(const ego_mesh_t *mesh)
{
	if (!mesh)
	{
		log_error("%s:%d: no mesh provided\n", __FILE__, __LINE__);
		throw std::invalid_argument("no mesh provided");
	}
	_mesh = mesh;
	// Determine the number of bifurcations necessary to get cells the size of the "blocks".
	int grids_x = mesh->gmem.grids_x;
	int grids_y = mesh->gmem.grids_y;
	_maxDepth = std::ceil(std::log(0.5f * std::max(grids_x, grids_y)) / std::log(2.0f));
}
//--------------------------------------------------------------------------------------------
mesh_BSP_t::mesh_BSP_t(const Parameters& parameters) :
	BSP_tree_t(BSP_tree_t::Parameters(2,parameters._maxDepth)),
	count(0)
{
	// Get the size of the mesh.
	float x_min = 0.0f;
	float x_max = parameters._mesh->gmem.edge_x;
	float y_min = 0.0f;
	float y_max = parameters._mesh->gmem.edge_y;
	float size = std::max(x_max - x_min, y_max - y_min);
    // Enlarge that size a bit ...
    bsp_bbox.min()[kX] = x_min - 0.25f * size;
    bsp_bbox.max()[kX] = x_max + 0.25f * size;
    bsp_bbox.min()[kY] = y_min - 0.25f * size;
    bsp_bbox.max()[kY] = y_max + 0.25f * size;

	// ... and dompute the midpoints: We have the bounding box of our mesh BSP.
	bsp_bbox.mid()[kX] = 0.5f * (bsp_bbox.min()[kX] + bsp_bbox.max()[kX]);
    bsp_bbox.mid()[kY] = 0.5f * (bsp_bbox.min()[kY] + bsp_bbox.max()[kY]);

    // Initialize the volume.
	// @todo Error handling.
	oct_bb_t::ctor(&volume);
}

//--------------------------------------------------------------------------------------------
mesh_BSP_t::~mesh_BSP_t()
{
    // Destruct the volume.
	oct_bb_t::dtor(&volume);
	// Set the count to zero.
	count = 0;
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
    tlist  = tile_mem_t::get(&(mesh->tmem),0);

    // make sure the mesh is allocated
    if (0 == tcount || NULL == tlist) return false;

    // initialize the bsp volume
    // assumes tlist[0] is insterted
    self->volume = tlist[0].oct;

    // insert each tile
    for (cnt = 0, ptile = tlist; cnt < tcount; cnt++, ptile++)
    {
        // try to insert the BSP
        if (mesh_BSP_insert(self, ptile, cnt ))
        {
            // add this tile's volume to the bsp's volume
            self->volume.join(ptile->oct);
        }
    }

    return self->count > 0;
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
    pleaf->bbox = ptile->oct.toBV();

    // Insert the leaf.
	bool retval = self->insert_leaf(pleaf);

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

    if (TILE_IS_FANOFF(ptile)) return false;

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

    if (TILE_IS_FANOFF(ptile)) return false;

    return true;
}
