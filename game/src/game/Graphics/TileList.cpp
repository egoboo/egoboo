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

/// @file game/Graphics/TileList.cpp
/// @brief A list of tiles as used by the graphics system
/// @author Michael Heilmann

#include "game/Graphics/TileList.hpp"
#include "game/graphic.h"

gfx_rv gfx_capture_mesh_tile(ego_tile_info_t * ptile)
{
	if (NULL == ptile)
	{
		return gfx_fail;
	}

	// Flag the tile as in the renderlist
	ptile->inrenderlist = true;

	// if the tile was not in the renderlist last frame, then we need to force a lighting update of this tile
	if (ptile->inrenderlist_frame < 0)
	{
		ptile->request_lcache_update = true;
		ptile->lcache_frame = -1;
	}
	else
	{
		Uint32 last_frame = (game_frame_all > 0) ? game_frame_all - 1 : 0;

		if ((Uint32)ptile->inrenderlist_frame < last_frame)
		{
			ptile->request_lcache_update = true;
		}
	}

	// make sure to cache the frame number of this update
	ptile->inrenderlist_frame = game_frame_all;

	return gfx_success;
}

namespace Ego {
namespace Graphics {

gfx_rv renderlist_lst_t::reset(renderlist_lst_t *self)
{
	if (nullptr == self)
	{
		return gfx_error;
	}
	self->size = 0;
	self->lst[0].index = TileIndex::Invalid.getI(); /// @todo index should be of type TileIndex.

	return gfx_success;
}

gfx_rv renderlist_lst_t::push(renderlist_lst_t *self, const TileIndex& index, float distance)
{
	if (!self)
	{
		return gfx_error;
	}

	if (self->size >= renderlist_lst_t::CAPACITY)
	{
		return gfx_fail;
	}
	self->lst[self->size].index = index.getI();
	self->lst[self->size].distance = distance;

	self->size++;

	return gfx_success;
}

TileList::TileList() :
	_mesh(nullptr), _all(), _ref(), _sha(), _reflective(), _nonReflective(), _water()
{}

TileList *TileList::init()
{
	// Initialize the render list lists.
	renderlist_lst_t::reset(&_all);
	renderlist_lst_t::reset(&_ref);
	renderlist_lst_t::reset(&_sha);
	renderlist_lst_t::reset(&_reflective);
	renderlist_lst_t::reset(&_nonReflective);
	renderlist_lst_t::reset(&_water);

	_mesh = nullptr;

	return this;
}

gfx_rv TileList::reset()
{
	if (!_mesh)
	{
		log_error("%s:%s:%d: tile list not attached to a mesh\n", __FILE__, __FUNCTION__, __LINE__);
		return gfx_error;
	}

	// Clear out the "in render list" flag for the old mesh.
	for (size_t i = 0; i < _all.size; ++i)
	{
		Uint32 fan = _all.lst[i].index;
		if (fan < _mesh->info.tiles_count)
		{
			_mesh->tmem.getTileList()[fan]->inrenderlist = false;
			_mesh->tmem.getTileList()[fan]->inrenderlist_frame = 0;
		}
	}

	// Re-initialize the renderlist.
	auto *mesh = _mesh;
	init();
	setMesh(mesh);

	return gfx_success;
}

gfx_rv TileList::insert(const TileIndex& index, const Camera &cam)
{
	if (!_mesh)
	{
		log_error("%s:%s:%d: tile list not attached to a mesh\n", __FILE__, __FUNCTION__, __LINE__);
		return gfx_error;
	}
	ego_mesh_t *mesh = _mesh;

	// check for a valid tile
	if (index >= mesh->gmem.grid_count)
	{
		return gfx_fail;
	}
	ego_grid_info_t *pgrid = grid_mem_t::get(&(mesh->gmem), index);
	if (!pgrid)
	{
		return gfx_fail;
	}

	// we can only accept so many tiles
	if (_all.size >= renderlist_lst_t::CAPACITY)
	{
		return gfx_fail;
	}

	int ix = index.getI() % mesh->info.tiles_x;
	int iy = index.getI() / mesh->info.tiles_x;
	float dx = (ix + TILE_FSIZE * 0.5f) - cam.getCenter()[kX];
	float dy = (iy + TILE_FSIZE * 0.5f) - cam.getCenter()[kY];
	float distance = dx * dx + dy * dy;

	// Put each tile in basic list
	renderlist_lst_t::push(&(_all), index, distance);

	// Put each tile in one other list, for shadows and relections
	if (0 != ego_grid_info_t::test_all_fx(pgrid, MAPFX_SHA))
	{
		renderlist_lst_t::push(&(_sha), index, distance);
	}
	else
	{
		renderlist_lst_t::push(&(_ref), index, distance);
	}

	if (0 != ego_grid_info_t::test_all_fx(pgrid, MAPFX_REFLECTIVE))
	{
		renderlist_lst_t::push(&(_reflective), index, distance);
	}
	else
	{
		renderlist_lst_t::push(&(_nonReflective), index, distance);
	}

	if (0 != ego_grid_info_t::test_all_fx(pgrid, MAPFX_WATER))
	{
		renderlist_lst_t::push(&(_water), index, distance);
	}

	return gfx_success;
}

ego_mesh_t *TileList::getMesh() const
{
	return _mesh;
}

void TileList::setMesh(ego_mesh_t *mesh)
{
	_mesh = mesh;
}

gfx_rv TileList::add(const std::vector<std::shared_ptr<ego_tile_info_t>> &tiles, Camera& camera)
{
	if (tiles.empty()) {
		return gfx_fail;
	}

	ego_mesh_t *mesh = getMesh();
	if (NULL == mesh)
	{
		log_error("%s:%s:%d: tile list not attached to a mesh\n", __FILE__, __FUNCTION__, __LINE__);
		return gfx_error;
	}

	// transfer valid pcolst entries to the renderlist
	for (size_t j = 0; j < tiles.size(); j++)
	{
		// Get fan index.
		TileIndex itile = tiles[j]->itile;

		// Get grid for tile index.
		ego_grid_info_t *pgrid = mesh->get_pgrid(itile);
		if (!pgrid) continue;

		if (gfx_error == gfx_capture_mesh_tile(tiles[j].get()))
		{
			return gfx_error;
		}

		if (gfx_error == insert(itile, camera))
		{
			return gfx_error;
		}
	}

	return gfx_success;
}

}
}
