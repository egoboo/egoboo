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

/// @file egolib/game/Graphics/TileList.cpp
/// @brief A list of tiles as used by the graphics system
/// @author Michael Heilmann

#include "egolib/game/Graphics/TileList.hpp"
#include "egolib/game/graphic.h"
#include "egolib/game/Core/GameEngine.hpp" //only for _currentModule
#include "egolib/game/Module/Module.hpp" //only for _currentModule

namespace Ego::Graphics {

TileList::TileList() :
	_all(), 
	_ref(), 
	_sha(), 
	_reflective(), 
	_nonReflective(), 
	_water(),

	_renderTiles(),
	_lastRenderTiles()
{
    try
    {
        _all.reserve(1024);
        _ref.reserve(1024);
        _sha.reserve(1024);
        _reflective.reserve(1024);
        _nonReflective.reserve(1024);
        _water.reserve(1024);
    }
    catch (...)
    {

    }
}

TileList::~TileList()
{}

void TileList::init()
{
	// Initialize the render list lists.
	_all.clear();
	_ref.clear();
	_sha.clear();
	_reflective.clear();
	_nonReflective.clear();
	_water.clear();
}

void TileList::reset()
{
	// Clear out the "in render list" flag for the old mesh.
	_lastRenderTiles = _renderTiles;
	_renderTiles.reset();

	// Re-initialize the renderlist.
	init();
}

gfx_rv TileList::insert(const Index1D& index, const ::Camera &cam)
{
	// check for a valid tile
	if (index >= getMesh()->_tmem.getInfo().getTileCount())
	{
		return gfx_fail;
	}
	ego_tile_info_t& ptile = getMesh()->_tmem.get(index);

    auto i2 = Grid::map<int>(index, (int)getMesh()->_info.getTileCountX());
	float dx = (i2.x() + Info<float>::Grid::Size() * 0.5f) - cam.getCenter()[kX];
	float dy = (i2.y() + Info<float>::Grid::Size() * 0.5f) - cam.getCenter()[kY];
	float distance = dx * dx + dy * dy;

	// Put each tile in basic list
	_all.emplace_back(index, distance);

	// Put each tile in one other list, for shadows and relections
	if (0 != ptile.testFX(MAPFX_SHA))
	{
		_sha.emplace_back(index, distance);
	}
	else
	{
		_ref.emplace_back(index, distance);
	}

	if (0 != ptile.testFX(MAPFX_REFLECTIVE))
	{
		_reflective.emplace_back(index, distance);
	}
	else
	{
		_nonReflective.emplace_back(index, distance);
	}

	if (0 != ptile.testFX(MAPFX_WATER))
	{
		_water.emplace_back(index, distance);
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
