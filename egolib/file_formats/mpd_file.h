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

#include "../egoboo_typedef.h"
#include "../egoboo_math.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_mpd_info;
    typedef struct s_mpd_info mpd_info_t;

    struct s_tile_info;
    typedef struct s_tile_info tile_info_t;

    struct s_mpd_vertex;
    typedef struct s_mpd_vertex mpd_vertex_t;

    struct s_mpd_mem;
    typedef struct s_mpd_mem mpd_mem_t;

    struct s_mpd;
    typedef struct s_mpd mpd_t;

    struct s_tile_definition;
    typedef struct s_tile_definition tile_definition_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// fan constants
#define MPD_FAN_VERTICES_MAX      16                         ///< Fansquare vertices
#define MPD_FAN_TYPE_MAX          64                         ///< Number of fansquare command types
#define MPD_FAN_MAX               4                          ///< Draw up to 4 fans
#define MPD_FAN_ENTRIES_MAX       32                         ///< Fansquare command list size
#define MPD_FAN_SIZE_MAX          32                         ///< Max trigs in each command

// mesh constants
#define MPD_ID                    0x4470614d                   ///< The string... MapD
#define MPD_TILEY_MAX             1024                       ///< Max tiles in y direction
#define MPD_TILE_MAX              (512*512)                  ///< Terrain mesh size
#define MPD_VERTICES_MAX          (MPD_TILE_MAX*MPD_FAN_VERTICES_MAX)

// tile constants
#define MPD_TILE_TYPE_MAX         256                        ///< Max number of tile images

// special values
#define MPD_FANOFF                0xFFFF                     ///< Don't draw the fansquare if tile = this

/// The bit flags for mesh tiles
    enum e_mpd_fx
    {
        MPDFX_REF             =       0,     ///< NOT USED
        ///< Egoboo v1.0 : "0 This tile is drawn 1st"

        MPDFX_SHA             = ( 1 << 0 ),  ///< 0 == (val & MPDFX_SHA) means that the tile is reflected in the floors
        ///< Egoboo v1.0: "0 This tile is drawn 2nd"
        ///< aicodes.txt : FXNOREFLECT

        MPDFX_DRAWREF         = ( 1 << 1 ),  ///< the tile reflects characters
        ///< Egoboo v1.0: "1 Draw reflection of characters"
        ///< aicodes.txt : FXDRAWREFLECT

        MPDFX_ANIM            = ( 1 << 2 ),  ///< Egoboo v1.0: "2 Animated tile ( 4 frame )"
        ///< aicodes.txt : FXANIM

        MPDFX_WATER           = ( 1 << 3 ),  ///< Egoboo v1.0: "3 Render water above surface ( Water details are set per module )"
        ///< aicodes.txt : FXWATER

        MPDFX_WALL            = ( 1 << 4 ),  ///< Egoboo v1.0: "4 Wall ( Passable by ghosts, particles )"
        ///< aicodes.txt : FXBARRIER

        MPDFX_IMPASS          = ( 1 << 5 ),  ///< Egoboo v1.0: "5 Impassable"
        ///< aicodes.txt : FXIMPASS

        MPDFX_DAMAGE          = ( 1 << 6 ),  ///< Egoboo v1.0: "6 Damage"
        ///< aicodes.txt : FXDAMAGE

        MPDFX_SLIPPY          = ( 1 << 7 )   ///< Egoboo v1.0: "7 Ice or normal"
        ///< aicodes.txt : FXSLIPPY
    };

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
        mpd_vertex_t *  vlst;                             ///< list of vertices
    };

//--------------------------------------------------------------------------------------------

/// The data describing a single mpd
    struct s_mpd
    {
        mpd_info_t info;
        mpd_mem_t   mem;
    };

//--------------------------------------------------------------------------------------------

/// A description of a tile type that allows some compression in the way vertices are stored in the mpd file
    struct s_tile_definition
    {
        Uint8           numvertices;                        ///< Number of vertices
        float           u[MPD_FAN_VERTICES_MAX];            ///< Vertex texture posi
        float           v[MPD_FAN_VERTICES_MAX];

        Uint8           command_count;                      ///< Number of commands
        Uint8           command_entries[MPD_FAN_MAX];       ///< Entries in each command
        Uint16          command_verts[MPD_FAN_ENTRIES_MAX]; ///< Fansquare vertex list
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern tile_definition_t tile_dict[MPD_FAN_TYPE_MAX];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the raw mpd loader
    mpd_t *      mpd_load( const char *modname, mpd_t * pmesh );
    mpd_t *      mpd_save( const char *modname, mpd_t * pmesh );

    mpd_t *      mpd_ctor( mpd_t * pmesh );
    mpd_t *      mpd_dtor( mpd_t * pmesh );
    mpd_t *      mpd_renew( mpd_t * pmesh );
    bool_t       mpd_free( mpd_t * pmesh );
    bool_t       mpd_init( mpd_t * pmesh, mpd_info_t * pinfo );

    void tile_dictionary_load_vfs( const char * filename, tile_definition_t dict[], size_t dict_size );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _mpd_file_h