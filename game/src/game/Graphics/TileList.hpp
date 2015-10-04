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

#include "game/egoboo_typedef.h"
#include "game/mesh.h"
#include "game/Graphics/CameraSystem.hpp"

#define Ego_Graphics_TileList_enableCPP 0

namespace Ego {
namespace Graphics {

struct renderlist_lst_t
{
	struct element_t
	{
		Uint32 index;             ///< which tile
		float distance;          ///< how far it is
#if defined(Ego_Graphics_TileList_enableCPP) && 1 == Ego_Graphics_TileList_enableCPP
		element_t() :
			index(), distance(-1.0f)
		{}
		element_t(const element_t& other) :
			index(other.index), distance(other.distance)
		{}
		virtual ~element_t()
		{}
		element_t& operator=(const element_t& other)
		{
			index = other.index;
			distance = other.distance;
			return *this;
		}
#endif
	};
	/**
	* @brief
	*    The maximum capacity of a renderlist
	*    i.e. the maximum number of tiles in a render list
	*    i.e. the maximum number of tiles to draw.
	*/
	static const size_t CAPACITY = 1024;
	size_t size;              ///< how many in the list
	element_t lst[CAPACITY];  ///< the list

#if defined(Ego_Graphics_TileList_enableCPP) && 1 == Ego_Graphics_TileList_enableCPP
	renderlist_lst_t() :
		size(0), lst(),
		{}
		virtual ~renderlist_lst_t()
	{}
#endif

	static gfx_rv reset(renderlist_lst_t *self);
	static gfx_rv push(renderlist_lst_t *self, const TileIndex& index, float distance);
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
#if defined(Ego_Graphics_TileList_enableCPP) && 1 == Ego_Graphics_TileList_enableCPP
	virtual ~TileList() :
	{}
#endif
	TileList *init();
	/// @brief Clear a render list
	gfx_rv reset();
	/// @brief Insert a tile into this render list.
	/// @param index the tile index
	/// @param camera the camera
	gfx_rv insert(const TileIndex& index, const ::Camera& camera);
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
	gfx_rv add(const size_t index, ::Camera& camera);

	/**
	* @brief
	*	check wheter a tile was rendered this render frame
	* @param index
	*	the index number of the tile
	* @return
	*	true if the specified tile is currently in the render list for this render frame
	**/
	bool inRenderList(const TileIndex &index) const;

private:
	std::bitset<MAP_TILE_MAX> _renderTiles;		//index of all tiles to be rendered
	std::bitset<MAP_TILE_MAX> _lastRenderTiles; //index of all tiles that were rendered last frame
};

}
}
