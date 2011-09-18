#pragma once

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

#include "../typedef.h"
#include "../_math.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_mpd_info;
    typedef struct s_mpd_info map_info_t;

    struct s_tile_info;
    typedef struct s_tile_info tile_info_t;

    struct s_mpd_vertex;
    typedef struct s_mpd_vertex map_vertex_t;

    struct s_mpd_mem;
    typedef struct s_mpd_mem map_mem_t;

    struct s_map;
    typedef struct s_map map_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#   define CURRENT_MAP_VERSION_LETTER 'D'

// mesh constants
#   define MAP_ID_BASE               'MapA'                   ///< The string... MapA
#   define MAP_TILEY_MAX             1024                       ///< Max tiles in y direction
#   define MAP_TILE_MAX              (512*512)                  ///< Terrain mesh size
#   define MAP_VERTICES_MAX          (MAP_TILE_MAX*MAP_FAN_VERTICES_MAX)

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

        MAPFX_DRAWREF         = ( 1 << 1 ),  ///< the tile reflects characters
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
    struct s_mpd_info
    {
        size_t          vertcount;                        ///< For malloc
        int             tiles_x;                          ///< Size in tiles
        int             tiles_y;
    };

//--------------------------------------------------------------------------------------------

/// The data describing a mpd tile
    struct s_tile_info
    {
        Uint8   type;                              ///< Tile type
        Uint16  img;                               ///< Get texture from this
        Uint8   fx;                                ///< Special effects flags
        Uint8   twist;
    };

//--------------------------------------------------------------------------------------------

/// The information for a single mpd vertex
    struct s_mpd_vertex
    {
        fvec3_t    pos;                               ///< Vertex position
        Uint8      a;                                 ///< Vertex base light
    };

//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated memory in an mpd
    struct s_mpd_mem
    {
        size_t          tile_count;                       ///< Number of tiles
        tile_info_t *   tile_list;                        ///< Tile info

        size_t          vcount;                           ///< number of vertices
        map_vertex_t *  vlst;                             ///< list of vertices
    };

//--------------------------------------------------------------------------------------------

/// The data describing a single mpd
    struct s_map
    {
        map_info_t info;
        map_mem_t   mem;
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the raw mpd loader
    map_t *      map_load( const char *modname, map_t * pmesh );
    map_t *      map_save( const char *modname, map_t * pmesh );

    map_t *      map_ctor( map_t * pmesh );
    map_t *      map_dtor( map_t * pmesh );
    map_t *      map_renew( map_t * pmesh );
    bool_t       map_free( map_t * pmesh );
    bool_t       map_init( map_t * pmesh, map_info_t * pinfo );
 
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _file_formats_map_file_h
