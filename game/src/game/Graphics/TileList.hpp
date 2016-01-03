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

/// @file game/Graphics/TileList.hpp
/// @brief A list of tiles as used by the graphics system
/// @author Michael Heilmann

#pragma once

#include "game/egoboo.h"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"

namespace Ego {
namespace Graphics {

struct renderlist_lst_t
{
	struct element_t
	{
		Index1D _index;  ///< The index of the tile.
		float _distance;   ///< The distance of the tile.
		element_t() :
			_index(), _distance(-1.0f)
		{}
		element_t(const Index1D& index, float distance) :
			_index(index), _distance(distance)
		{}
		element_t(const element_t& other) :
			_index(other._index), _distance(other._distance)
		{}
		virtual ~element_t()
		{}
		element_t& operator=(const element_t& other) {
			_index = other._index;
			_distance = other._distance;
			return *this;
		}
	};
	/**
	* @brief
	*    The maximum capacity of a renderlist
	*    i.e. the maximum number of tiles in a render list
	*    i.e. the maximum number of tiles to draw.
	*/
	static const size_t CAPACITY = 1024;
	size_t size;                          ///< The number of entries.
	std::array<element_t, CAPACITY> lst;  ///< The entries.

	renderlist_lst_t() :
		size(0), lst()
	{}
	
	virtual ~renderlist_lst_t()
	{}

	void reset();
	/**
	 * @brief Add an entry
	 * @param index the index of the tile
	 * @param distance the distance of the tile
	 * @return @a true if the entry was added, @a false otherwise
	 */
	bool push(const Index1D& index, float distance);
};

/// Which tiles are to be drawn, arranged by MAPFX_* bits
struct TileList
{
	std::shared_ptr<ego_mesh_t> _mesh;
	renderlist_lst_t _all;     ///< List of which to render, total
	renderlist_lst_t _ref;     ///< ..., is reflected in the floor
	renderlist_lst_t _sha;     ///< ..., is not reflected in the floor
	/**
	 * @brief
	 *	Tiles reflecting entities i.e. "reflective" tiles.
	 * @remark
	 *	Tiles on which the MAPFX_REFLECTIVE bit is set are added to this list.
	 */
	renderlist_lst_t _reflective;
	/**
	 * @brief
	 *	Tiles not reflecting entities i.e. "non-reflective" tiles.
	 * @remark
	 *	Tiles on which the MAPFX_REFLECTIVE bit is <em>not</em> set are added to this list.
	 */
	renderlist_lst_t _nonReflective;
	/**
	 * @brief
	 *	Tiles which are water.
	 * @remark
	 * 	Tiles on which the MAPFX_WATER bit is set are added to this list.
	 */
	renderlist_lst_t _water;

	TileList();
	virtual ~TileList();
	void init();
	/// @brief Clear a render list
	gfx_rv reset();
	/// @brief Insert a tile into this render list.
	/// @param index the tile index
	/// @param camera the camera
	gfx_rv insert(const Index1D& index, const ::Camera& camera);
	/// @brief Get mesh this render list is attached to.
	/// @return the mesh or @a nullptr
	/// @post If the render list is attached to a mesh, that mesh is returned.
	///       Otherwise a null pointer is returned.
	std::shared_ptr<ego_mesh_t> getMesh() const;
	/// @brief Set mesh this render list is attached to.
	/// @param mesh the mesh or @a nullptr
	/// @post If @a mesh is not a null pointer, then this render list is attached to that mesh.
	///       Otherwise it is detached.
	void setMesh(std::shared_ptr<ego_mesh_t> mesh);
	/// @brief Insert a tile into this render list.
	/// @param the index of the tile to insert
	/// @param camera the camera
	gfx_rv add(const Index1D& index, ::Camera& camera);

	/**
	* @brief
	*	check wheter a tile was rendered this render frame
	* @param index
	*	the index number of the tile
	* @return
	*	true if the specified tile is currently in the render list for this render frame
	**/
	bool inRenderList(const Index1D& index) const;

private:
	std::bitset<MAP_TILE_MAX> _renderTiles;		//index of all tiles to be rendered
	std::bitset<MAP_TILE_MAX> _lastRenderTiles; //index of all tiles that were rendered last frame
};

}
}
