//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//*
//********************************************************************************************

#pragma once

#include "egolib/platform.h"
#include "egolib/FileFormats/map_fx.hpp"

/**
 * @brief
 *  Information about a tile.
 * @remark
 *  Note:
 *  - A wall tile is not necessarily an impassable tile, an impassable tile is not necessarily a wall tile.
 *  - A floor tile is a tile that is no wall tile.
 */
struct cartman_mpd_tile_t
{

	/**
	 * @brief
	 *  The fan type of the tile.
	 * @default
	 *  <tt>0</tt>
	 */
	uint8_t type;

	/**
	 * @brief
	 *  The special effects flags of the tile.
	 * @default
	 *  <tt>MAPFX_WALL | MAPFX_IMPASS</tt>
	 */
	uint8_t fx;

	/**
	 * @brief
	 *  The texture bits and special tile bits of the tile.
	 * @default
	 *  <tt>MAP_FANOFF</tt>
	 */
	uint16_t tx_bits;

	/**
	 * @brief
	 *  The surface normal of this tile.
	 * @default
	 *  <tt>TWIST_FLAT</tt>
	 */
	uint8_t twist;

	/**
	 * @brief
	 *  The index of the first vertex of this tile in the vertex array.
	 * @default
	 *  <tt>MAP_FAN_ENTRIES_MAX</tt>
	 */
	uint32_t vrtstart;

	/**
	 * @brief
	 *  Construct this tile with its default values.
	 */
	cartman_mpd_tile_t();

	/**
	 * @brief
	 *  Reset this tile to its default values.
	 */
	void reset();

	/**
	 * @brief
	 *  Get if this tile has its fan rendering turned off.
	 * @return
	 *  @a true if this tile has its fan rendering turned off, @a false otherwise
	 */
	bool isFanOff() const {
		return MAP_FANOFF == tx_bits;
	}

	/**
	 * @brief
	 *  Set the FX of a tile.
	 * @param tile
	 *  the tile
	 * @param fx
	 *  the FX
	 */
	void setFX(uint8_t fx);
	/**
	 * @brief
	 *  Get the FX of a tile.
	 * @param tile
	 *  the tile
	 * @return
	 *  the FX of the file
	 */
	uint8_t getFX() const;
	/**
	 * @brief
	 *  Make tile impassable.
	 * @param tile
	 *  the tile
	 */
	void setImpassable();
	/**
	 * @brief
	 *  Get if a tile is impassable.
	 * @param tile
	 *  the tile
	 * @return
	 *  @a true if the tile is impassable, @a false otherwise
	 */
	bool isImpassable() const;
	/**
	 * @brief
	 *  Get if a tile is a passable floor tile.
	 * @param tile
	 *  the tile
	 * @return
	 *  @a true if the tile is a passable floor tile, @a false otherwise
	 */
	bool isPassableFloor() const;
	/**
	 * @brief
	 *  Get if tile is an impassable wall.
	 * @param tile
	 *  the tile
	 * @return
	 *  @a true if the tile is an impassable wall, @a false otherwise
	 */
	bool isImpassableWall() const;

};