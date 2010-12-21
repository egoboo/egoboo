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

#include "egoboo_typedef.h"
#include "egoboo_math.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define GRID_BITS       7
#define GRID_ISIZE     (1<<(GRID_BITS))
#define GRID_FSIZE     ((float)GRID_ISIZE)
#define GRID_MASK      (GRID_ISIZE - 1)

#define MAPID                     0x4470614d                   ///< The string... MapD
#define MESH_MAXTOTALVERTRICES    1024*100
#define MAXMESHFAN                (512*512)                  ///< Terrain mesh size
#define MAXMESHTILEY              1024                       ///< Max tiles in y direction
#define MAXMESHVERTICES           16                         ///< Fansquare vertices
#define MAXMESHTYPE               64                         ///< Number of fansquare command types
#define MAXMESHCOMMAND            4                          ///< Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES     32                         ///< Fansquare command list size
#define MAXMESHCOMMANDSIZE        32                         ///< Max trigs in each command
#define MAXTILETYPE               256                        ///< Max number of tile images
#define FANOFF                    0xFFFF                     ///< Don't draw the fansquare if tile = this

#define CARTMAN_FIXNUM            4.125f ///< 4.150f             ///< Magic number
#define CARTMAN_SLOPE             50                        ///< increments for terrain slope

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))

#define VALID_GRID(PMPD, ID) ( (INVALID_TILE!=(ID)) && (NULL != (PMPD)) && (ID < (PMPD)->info.tiles_count) )

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
    typedef struct s_mpd_info mpd_info_t;

//--------------------------------------------------------------------------------------------

/// The data describing a mpd tile
    struct s_tile_info
    {
        Uint8   type;                              ///< Tile type
        Uint16  img;                               ///< Get texture from this
        Uint8   fx;                                ///< Special effects flags
        Uint8   twist;
    };
    typedef struct s_tile_info tile_info_t;

//--------------------------------------------------------------------------------------------

/// The information for a single mpd vertex
    struct s_mpd_vertex
    {
        fvec3_t    pos;                               ///< Vertex position
        Uint8      a;                                 ///< Vertex base light
    };
    typedef struct s_mpd_vertex mpd_vertex_t;

//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated memory in an mpd
    struct s_mpd_mem
    {
        size_t          tile_count;                       ///< Number of tiles
        tile_info_t *   tile_list;                        ///< Tile info

        size_t          vcount;                           ///< number of vertices
        mpd_vertex_t *  vlst;                             ///< list of vertices
    };
    typedef struct s_mpd_mem mpd_mem_t;

//--------------------------------------------------------------------------------------------

/// The data describing a single mpd
    struct s_mpd
    {
        mpd_info_t info;
        mpd_mem_t   mem;
    };
    typedef struct s_mpd mpd_t;

//--------------------------------------------------------------------------------------------

/// A description of a tile type that allows some compression in the way vertices are stored in the mpd file
    struct s_tile_definition
    {
        Uint8           numvertices;                ///< Number of vertices
        float           u[MAXMESHVERTICES];         ///< Vertex texture posi
        float           v[MAXMESHVERTICES];

        Uint8           command_count;                        ///< Number of commands
        Uint8           command_entries[MAXMESHCOMMAND];      ///< Entries in each command
        Uint16          command_verts[MAXMESHCOMMANDENTRIES]; ///< Fansquare vertex list
    };
    typedef struct s_tile_definition tile_definition_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    extern tile_definition_t tile_dict[MAXMESHTYPE];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the raw mpd loader
    mpd_t *      mpd_load( const char *modname, mpd_t * pmesh );

    mpd_t *      mpd_ctor( mpd_t * pmesh );
    mpd_t *      mpd_dtor( mpd_t * pmesh );
    bool_t       mpd_free( mpd_t * pmesh );

    bool_t twist_to_normal( Uint8 twist, float v[], float slide );
    Uint8  cartman_get_twist( int x, int y );

    void tile_dictionary_load_vfs( const char * filename, tile_definition_t dict[], size_t dict_size );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _mpd_file_h