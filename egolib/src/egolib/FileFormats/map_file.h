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

#pragma once

#include "egolib/typedef.h"
#include "egolib/_math.h"
#include "egolib/FileFormats/map_tile_dictionary.h"
#include "egolib/Math/Vector.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// The size of a grid is an positive integral power of two.
// The grid info structure provides two static methods both
// returning a value of type @a Type: The method Size returns
// the grid size and the method Exponent returns the exponent
// to which 2 is raised to in order to compute the grid size.
template <typename Type>
struct Info;

template <>
struct Info<int> {
	struct Grid {
		/// @return the number bits by which 1 is shifted to the left in order to compute the grid size
		static constexpr int Bits() {
			return 7;
		}
		/// @return the exponent = bits
		static constexpr int Exponent() {
			return 7;
		}
		/// @return the grid size
		static constexpr int Size() {
			return 1 << Info<int>::Grid::Bits();
		}
		/// @return the mask for bitwise modulus
		static constexpr int Mask() {
			return Info<int>::Grid::Size() - 1;
		}
	};
	struct Block {
		/// @return the number bits by which 1 is shifted to the left in order to compute the block size
		static constexpr int Bits() {
			return 9;
		}
		/// @return the exponent = bits
		static constexpr int Exponent() {
			return 9;
		}
		/// @return the block size
		static constexpr int Size() {
			return 1 << Info<int>::Block::Exponent();
		}
		/// @return the mask for bitwise modulis
		static constexpr int Mask() {
			return Info<int>::Block::Size() - 1;
		}
	};
};

template <>
struct Info<float> {
	struct Grid {
		static float Exponent() {
			return (float)Info<int>::Grid::Exponent();
		}
		static float Size() {
			return (float)Info<int>::Grid::Size();
		}
	};
	struct Block {
		static float Exponent() {
			return (float)Info<int>::Block::Exponent();
		}
		static float Size() {
			return (float)Info<int>::Block::Size();
		}
	};
};


#define CURRENT_MAP_VERSION_LETTER 'D'

// mesh constants
#define MAP_ID_BASE 0x4D617041 // The string... MapA

/// The maximum number of tiles in x direction.
constexpr uint32_t MAP_TILE_MAX_X = 1024;
/// The maximum number of tiles in y direction.
constexpr uint32_t MAP_TILE_MAX_Y = 1024;
/// The maximum number of tiles.
constexpr uint32_t MAP_TILE_MAX = MAP_TILE_MAX_X * MAP_TILE_MAX_Y;
/// The maximum number of vertices.
constexpr uint32_t MAP_VERTICES_MAX = MAP_TILE_MAX*MAP_FAN_VERTICES_MAX;

// tile constants
#   define MAP_TILE_TYPE_MAX         256                     ///< Max number of tile images

#include "egolib/FileFormats/map_fx.hpp"

#   define VALID_MPD_TILE_RANGE(VAL)   ( ((size_t)(VAL)) < MAP_TILE_MAX )
#   define VALID_MPD_VERTEX_RANGE(VAL) ( ((size_t)(VAL)) < MAP_VERTICES_MAX )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  Temporary structure for reading and writing MPD files.
 */
struct map_info_t
{
private:
    /// The number of vertices.
	uint32_t _vertexCount;
    /// The number of tiles in the x direction.
	uint32_t _tileCountX;
    /// The number of tiles in the y direction.
	uint32_t _tileCountY;
public:
	/**
	 * @brief
	 *  Construct this map info for a map with 0 tiles along the x- and y-axis and 0 vertices.
	 */
	map_info_t();

	/**
	 * @brief
	 *  Construct this map info with the specified number of tiles along the x- and y-axis and the specified number of vertices.
	 * @param vertexCount
	 *  the number of vertices
	 * @param tileCountX
	 *  the number of tiles along the x-axis
	 * @param tileCountY
	 *  the number of tiles along the y-axis
	 * @remark
	 *  The number of vertices is set to a reasonable default value based on the number of tiles.
	 */
	map_info_t(uint32_t vertexCount, uint32_t tileCountX, uint32_t tileCountY);

	/**
	 * @brief
	 *  Copy construct this map info from another map info.
	 * @param other
	 *  the copy construction source
	 */
	map_info_t(const map_info_t& other);

	/**
	 * @brief
	 *  Assign this map info from another map info.
	 * @param other
	 *  the assignment source of the copy operation
	 */
	map_info_t& operator=(const map_info_t& other);
public:
    /**
     * @brief
     *  Get the number of vertices.
     * @return
     *  the number of vertices
     */
    uint32_t getVertexCount() const {
        return _vertexCount;
    }

    /**
     * @brief
     *  Get the number of tiles in the x direction.
     * @return
     *  the number of tiles in the x direction
     */
    uint32_t getTileCountX() const {
        return _tileCountX;
    }

	/**
	 * @brief
	 *  Get the number of tiles in the y direction.
	 * @return
	 *  the number of tiles in the y direction
	 */
	uint32_t getTileCountY() const {
		return _tileCountY;
	}

	/**
	 * @brief
	 *  Get the number of tiles
	 * @return
	 *  the number of tiles
	 */
	uint32_t getTileCount() const {
		return getTileCountX() * getTileCountY();
	}

public:
    /**
     * @brief
     *  Load creation parameters from a file.
     * @param file
     *  the source file
     */
    void load(vfs_FILE& file);

    /**
     * @brief
     *  Save creation parameters to a file.
     * @param file
     *  the target file
     */
    void save(vfs_FILE& file) const;

    /**
     * @brief
     *  Validate creation parameters.
     * @return
     *  @a true if the creation parameters are valid, @a false otherwise
     */
    bool validate() const;

    /**
     * @brief
     *  Reset this creation parameters to their default values.
     * @remark
     *  The default values are for an empty map.
     */
    void reset();

};

//--------------------------------------------------------------------------------------------

/// The information for an mpd tile.
struct tile_info_t
{
    uint8_t   type;   ///< Tile type
    uint16_t  img;    ///< Get texture from this
    uint8_t   fx;     ///< Special effects flags
    uint8_t   twist;
};

//--------------------------------------------------------------------------------------------

/// The information for a single mpd vertex.
struct map_vertex_t
{
	Vector3f pos; ///< Vertex position.
    uint8_t a;      ///< Vertex base light.
};

//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated memory in an mpd
struct map_mem_t
{
    std::vector<tile_info_t> tiles;
    std::vector<map_vertex_t> vertices;
    map_mem_t();
	/**
	 * @brief
	 *  Construct this map memory according to the specified map information.
	 * @param info
	 *  the map information
	 */
	map_mem_t(const map_info_t& info);

    virtual ~map_mem_t();
	/**
	 * @brief
	 *	Resize the map memory according to the specified map information.
	 * @param info
	 *  the map information
	 */
	void setInfo(const map_info_t& info);

	/**
	 * @brief
	 *  Get the tile at the specified index.
	 * @param i
	 *  the index
	 * @return
	 *  the tile at the specified index
	 * @throw std::runtime_error
	 *  if the index is greater than or equal to the number of tiles
	 */
	const tile_info_t& operator()(size_t i) const;

};

//--------------------------------------------------------------------------------------------

/// The data describing a single mpd
struct map_t
{
public:
    map_info_t _info;
    map_mem_t _mem;
public:

    /**
     * @brief
     *  Create this map.
     * @param info
     *  the map info. Default is an empty map.
     */
    map_t(const map_info_t& info = map_info_t());

    /**
     * @brief
     *  Destruct this map.
     */
    virtual ~map_t();

    /**
     * @brief
     *  Set the map info.
     * @param info
     *  the map info. Default is an empty map.
     */
    bool setInfo(const map_info_t& info = map_info_t());

    /**
     * @brief
     *  Load a map from a file.
     * @param file
     *  the file to load the map from
     */
    bool load(vfs_FILE& file);

    /**
     * @brief
     *  Load a map from a file.
     * @param name
     *  the name to load the map from
     */
    bool load(const std::string& name);

    /**
     * @brief
     *  Save a map to a file.
     * @param file
     *  the file to save the map to
     */
    bool save(vfs_FILE& file) const;

    /**
     * @brief
     *  Save a map to a file.
     * @param name
     *  the name to save the map to
     */
    bool save(const std::string& name) const;

	/**
	 * @brief
	 *  Get the index of a tile in a 1D tile array given its index in a 2D tile array.
	 * @param x, y
	 *  the 2D index of the tile
	 * @return
	 *  the 1D index of the tile \f$y \cdot w + x\f$ where \f$w\f$ is the width of the map
	 */
	size_t getTileIndex(int x, int y) const;

	/**
	 * @brief
	 *  Get the tile at the specified index.
	 * @param x, y
	 *  the index of the tile
	 * @return
	 *  the tile
	 * @throw std::runtime_error
	 *  if @a x (@a y) is greater than or equal to the size of the map along the x-axis (y-axis) 
	 */
	const tile_info_t& operator()(size_t x, size_t y) const;
	/**
	 * @brief
	 *  Get the tile at the specified index.
	 * @param i
	 *  the index of the tile
	 * @return
	 *  the tile
	 * @throw std::runtime_error
	 *  if @a i is greater than or equal to the size of the map
	 */
	const tile_info_t& operator()(size_t i) const;
};
