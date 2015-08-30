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
#include "egolib/Math/Vector.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#   define CURRENT_MAP_VERSION_LETTER 'D'

// mesh constants
#   define MAP_ID_BASE               0x4D617041 //'MapA'        ///< The string... MapA

/// The maximum number of tiles in x direction.
#define MAP_TILE_MAX_X 1024
/// The maximum number of tiles in y direction.
#define MAP_TILE_MAX_Y 1024
/// The maximum number of tiles.
#define MAP_TILE_MAX (MAP_TILE_MAX_X * MAP_TILE_MAX_Y)
/// The maximum number of vertices.
#define MAP_VERTICES_MAX (MAP_TILE_MAX*MAP_FAN_VERTICES_MAX)

static_assert(MAP_TILE_MAX_X <= UINT32_MAX, "MAP_TILE_MAX_X may not exceed UINT32_MAX");
static_assert(MAP_TILE_MAX_Y <= UINT32_MAX, "MAP_TILE_MAX_Y may not exceed UINT32_MAX");
static_assert(MAP_TILE_MAX <= UINT32_MAX, "MAP_TILE_MAX may not exceed UINT32_MAX");

#   define TILE_BITS   7
#   define TILE_ISIZE (1<<TILE_BITS)
#   define TILE_MASK  (TILE_ISIZE - 1)
#   define TILE_FSIZE ((float)TILE_ISIZE)

// tile constants
#   define MAP_TILE_TYPE_MAX         256                     ///< Max number of tile images

// special values
#   define MAP_FANOFF                0xFFFF                     ///< Don't draw the fansquare if tile = this

/// The bit flags for mesh tiles
    enum e_map_fx
    {
        MAPFX_REF             =       0,     ///< NOT USED
        ///< Egoboo v1.0 : "0 This tile is drawn 1st"

        MAPFX_SHA             = ( 1 << 0 ),  ///< 0 == (val & MAPFX_SHA) means that the tile is reflected in the floors
        ///< Egoboo v1.0: "0 This tile is drawn 2nd"
        ///< aicodes.txt : FXNOREFLECT

        MAPFX_REFLECTIVE      = ( 1 << 1 ),  ///< the tile reflects entities
        ///< Egoboo v1.0: "1 Draw reflection of characters"
        ///< aicodes.txt : FXDRAWREFLECT

        MAPFX_ANIM            = ( 1 << 2 ),  ///< Egoboo v1.0: "2 Animated tile ( 4 frame )"
        ///< aicodes.txt : FXANIM

        MAPFX_WATER           = ( 1 << 3 ),  ///< Egoboo v1.0: "3 Render water above surface ( Water details are set per module )"
        ///< aicodes.txt : FXWATER

        MAPFX_WALL            = ( 1 << 4 ),  ///< Egoboo v1.0: "4 Wall ( Passable by ghosts, particles )"
        ///< aicodes.txt : FXBARRIER

        MAPFX_IMPASS          = ( 1 << 5 ),  ///< Egoboo v1.0: "5 Impassable"
        ///< aicodes.txt : FXIMPASS

        MAPFX_DAMAGE          = ( 1 << 6 ),  ///< Egoboo v1.0: "6 Damage"
        ///< aicodes.txt : FXDAMAGE

        MAPFX_SLIPPY          = ( 1 << 7 )   ///< Egoboo v1.0: "7 Ice or normal"
        ///< aicodes.txt : FXSLIPPY
    };

#   define VALID_MPD_TILE_RANGE(VAL)   ( ((size_t)(VAL)) < MAP_TILE_MAX )
#   define VALID_MPD_VERTEX_RANGE(VAL) ( ((size_t)(VAL)) < MAP_VERTICES_MAX )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The basic parameters needed to create an mpd
struct map_info_t
{
    /// The number of vertices.
    Uint32 vertexCount;
    /// The number of tiles in the x direction.
    Uint32 tileCountX;
    /// The number of tiles in the y direction.
    Uint32 tileCountY;

    /**
     * @brief
     *  Get the number of vertices.
     * @return
     *  the number of vertices
     */
    Uint32 getVertexCount() const
    {
        return vertexCount;
    }

    /**
     * @brief
     *  Get the number of tiles in the x direction.
     * @return
     *  the number of tiles in the x direction
     */
    Uint32 getTileCountX() const
    {
        return tileCountX;
    }

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

    /**
     * @brief
     *  Default constructor.
     * @remark
     *  The default values are for an empty map.
     */
    map_info_t();

    /**
     * @brief
     *  Copy constructor.
     * @param other
     *  the source of the copy operation
     */
    map_info_t(const map_info_t& other);

    /**
     * @brief
     *  Assignment operator.
     * @param other
     *  the source of the copy operation
     */
    map_info_t& operator=(const map_info_t& other);

};

//--------------------------------------------------------------------------------------------

/// The information for an mpd tile.
struct tile_info_t
{
    Uint8   type;   ///< Tile type
    Uint16  img;    ///< Get texture from this
    Uint8   fx;     ///< Special effects flags
    Uint8   twist;
};

//--------------------------------------------------------------------------------------------

/// The information for a single mpd vertex.
struct map_vertex_t
{
	Vector3f pos; ///< Vertex position.
    Uint8 a;      ///< Vertex base light.
};

//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated memory in an mpd
struct map_mem_t
{
    std::vector<tile_info_t> tiles;
    std::vector<map_vertex_t> vertices;
    map_mem_t();
    map_mem_t(uint32_t tileCount,uint32_t vertexCount);
    virtual ~map_mem_t();
    void setInfo(uint32_t tileCount, uint32_t vertexCount);
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
    bool load(const char *name);

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
    bool save(const char *name) const;

};

