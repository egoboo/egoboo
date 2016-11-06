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
#include "game/Core/GameEngine.hpp" //only for _currentModule
#include "game/Module/Module.hpp" //only for _currentModule

namespace Ego {
namespace Graphics {

void renderlist_lst_t::reset() {
	size = 0;
	lst[0]._index = Index1D::Invalid;
}

bool renderlist_lst_t::push(const Index1D& index, float distance) {
	if (size >= renderlist_lst_t::CAPACITY) {
		return false;
	}
	lst[size++] = element_t(index, distance);
	return true;
}

TileList::TileList() :
	_all(), 
	_ref(), 
	_sha(), 
	_reflective(), 
	_nonReflective(), 
	_water(),

	_renderTiles(),
	_lastRenderTiles()
{}

TileList::~TileList()
{}

void TileList::init()
{
	// Initialize the render list lists.
	_all.reset();
	_ref.reset();
	_sha.reset();
	_reflective.reset();
	_nonReflective.reset();
	_water.reset();
}

gfx_rv TileList::reset()
{
	// Clear out the "in render list" flag for the old mesh.
	_lastRenderTiles = _renderTiles;
	_renderTiles.reset();

	// Re-initialize the renderlist.
	init();

	return gfx_success;
}

gfx_rv TileList::insert(const Index1D& index, const ::Camera &cam)
{
	// check for a valid tile
	if (index >= getMesh()->_tmem.getInfo().getTileCount())
	{
		return gfx_fail;
	}
	ego_tile_info_t& ptile = getMesh()->_tmem.get(index);

	// we can only accept so many tiles
	if (_all.size >= renderlist_lst_t::CAPACITY)
	{
		return gfx_fail;
	}

    auto i2 = Grid::map<int>(index, (int)getMesh()->_info.getTileCountX());
	float dx = (i2.x() + Info<float>::Grid::Size() * 0.5f) - cam.getCenter()[kX];
	float dy = (i2.y() + Info<float>::Grid::Size() * 0.5f) - cam.getCenter()[kY];
	float distance = dx * dx + dy * dy;

	// Put each tile in basic list
	_all.push(index, distance);

	// Put each tile in one other list, for shadows and relections
	if (0 != ptile.testFX(MAPFX_SHA))
	{
		_sha.push(index, distance);
	}
	else
	{
		_ref.push(index, distance);
	}

	if (0 != ptile.testFX(MAPFX_REFLECTIVE))
	{
		_reflective.push(index, distance);
	}
	else
	{
		_nonReflective.push(index, distance);
	}

	if (0 != ptile.testFX(MAPFX_WATER))
	{
		_water.push(index, distance);
	}

	return gfx_success;
}

std::shared_ptr<ego_mesh_t> TileList::getMesh() const
{
	return _currentModule->getMeshPointer();
}

gfx_rv TileList::add(const Index1D& index, ::Camera& camera)
{
	_renderTiles[index.i()] = true;

	// if the tile was not in the renderlist last frame, then we need to force a lighting update of this tile
	if(!_lastRenderTiles[index.i()]) {
		ego_tile_info_t& tile = getMesh()->_tmem.get(index);
		tile._lightingCache.setNeedUpdate(true);
		tile._lightingCache.setLastFrame(-1);
	}

	if (gfx_error == insert(index, camera))
	{
		return gfx_error;
	}

	return gfx_success;
}

bool TileList::inRenderList(const Index1D& index) const
{
	if(index == Index1D::Invalid) return false;
	return _renderTiles[index.i()];
}

}
}
