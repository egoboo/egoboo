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

/// @file egolib/game/Graphics/TileList.hpp
/// @brief A list of tiles as used by the graphics system
/// @author Michael Heilmann

#pragma once

#include "egolib/game/egoboo.h"
#include "egolib/game/mesh.h"
#include "egolib/game/Graphics/CameraSystem.hpp"

namespace Ego::Graphics {

struct ClippingEntry
{
private:
    Index1D index;    ///< The index of the tile.
    float distance;   ///< The distance of the tile.

public:
    ClippingEntry() :
        index(Index1D::Invalid), distance(std::numeric_limits<float>::infinity())
    {}

    ClippingEntry(const Index1D& index, float distance) :
        index(index), distance(distance)
    {}

    ClippingEntry(const ClippingEntry& other) :
        index(other.index), distance(other.distance)
    {}

    virtual ~ClippingEntry()
    {}

    ClippingEntry& operator=(const ClippingEntry& other)
    {
        index = other.index;
        distance = other.distance;
        return *this;
    }

    const Index1D& getIndex() const
    {
        return index;
    }

    float getDistance() const
    {
        return distance;
    }
};

/// Which tiles are to be drawn, arranged by MAPFX_* bits
struct TileList
{
	std::vector<ClippingEntry> _all;     ///< List of which to render, total
    std::vector<ClippingEntry> _ref;     ///< ..., is reflected in the floor
    std::vector<ClippingEntry> _sha;     ///< ..., is not reflected in the floor
	/// @brief Tiles reflecting entities i.e. "reflective" tiles.
    /// @remark Tiles on which the MAPFX_REFLECTIVE bit is set are added to this list.
    std::vector<ClippingEntry> _reflective;
	/// @brief Tiles not reflecting entities i.e. "non-reflective" tiles.
	/// @remark Tiles on which the MAPFX_REFLECTIVE bit is <em>not</em> set are added to this list.
    std::vector<ClippingEntry> _nonReflective;
	/// @brief Tiles which are water.
	/// @remark Tiles on which the MAPFX_WATER bit is set are added to this list.
    std::vector<ClippingEntry> _water;

	TileList();
	virtual ~TileList();
	void init();
	/// @brief Clear a render list
	void reset();
	/// @brief Insert a tile into this render list.
	/// @param index the tile index
	/// @param camera the camera
	gfx_rv insert(const Index1D& index, const ::Camera& camera);
	
	/// @brief Get mesh this render list is attached to.
	/// @return the mesh or @a nullptr
	/// @post If the render list is attached to a mesh, that mesh is returned.
	///       Otherwise a null pointer is returned.
	std::shared_ptr<ego_mesh_t> getMesh() const;

	/// @brief Insert a tile into this render list.
	/// @param the index of the tile to insert
	/// @param camera the camera
	gfx_rv add(const Index1D& index, ::Camera& camera);

	/// @brief check wheter a tile was rendered this render frame.
	/// @param index the index number of the tile
	/// @return true if the specified tile is currently in the render list for this render frame
	bool inRenderList(const Index1D& index) const;

private:
	std::bitset<MAP_TILE_MAX> _renderTiles;		//index of all tiles to be rendered
	std::bitset<MAP_TILE_MAX> _lastRenderTiles; //index of all tiles that were rendered last frame
};

}
