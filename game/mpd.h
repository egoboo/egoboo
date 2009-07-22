#pragma once

// ********************************************************************************************
// *
// *    This file is part of Egoboo.
// *
// *    Egoboo is free software: you can redistribute it and/or modify it
// *    under the terms of the GNU General Public License as published by
// *    the Free Software Foundation, either version 3 of the License, or
// *    (at your option) any later version.
// *
// *    Egoboo is distributed in the hope that it will be useful, but
// *    WITHOUT ANY WARRANTY; without even the implied warranty of
// *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// *    General Public License for more details.
// *
// *    You should have received a copy of the GNU General Public License
// *    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
// *
// ********************************************************************************************

#include "egoboo_typedef.h"
#include "egoboo_math.h"

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
#define TILE_BITS     7
#define TILE_SIZE     ((float)(1<<(TILE_BITS)))

#define MAPID                     0x4470614d                   // The string... MapD
#define MESH_MAXTOTALVERTRICES    1024*100
#define MAXMESHFAN                (512*512)                  // Terrain mesh size
#define MAXMESHTILEY              1024                       // Max tiles in y direction
#define MAXMESHVERTICES           16                         // Fansquare vertices
#define MAXMESHTYPE               64                         // Number of fansquare command types
#define MAXMESHCOMMAND            4                          // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES     32                         // Fansquare command list size
#define MAXMESHCOMMANDSIZE        32                         // Max trigs in each command
#define MAXTILETYPE               256                        // Max number of tile images
#define FANOFF                    0xFFFF                     // Don't draw the fansquare if tile = this

#define CARTMAN_FIXNUM            4.125 // 4.150             // Magic number
#define CARTMAN_SLOPE             50                        // increments for terrain slope

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))

#define VALID_TILE(PMPD, ID) ( (INVALID_TILE!=(ID)) && (NULL != (PMPD)) && (ID < (PMPD)->info.tiles_count) )

enum e_mpd_fx
{
    MPDFX_REF             =       0,                // 0 This tile is drawn 1st
    MPDFX_SHA             = (1 << 0),               // 0 This tile is drawn 2nd
    MPDFX_DRAWREF         = (1 << 1),               // 1 Draw reflection of characters
    MPDFX_ANIM            = (1 << 2),               // 2 Animated tile ( 4 frame )
    MPDFX_WATER           = (1 << 3),               // 3 Render water above surface ( Water details are set per module )
    MPDFX_WALL            = (1 << 4),               // 4 Wall ( Passable by ghosts, particles )
    MPDFX_IMPASS          = (1 << 5),               // 5 Impassable
    MPDFX_DAMAGE          = (1 << 6),               // 6 Damage
    MPDFX_SLIPPY          = (1 << 7),               // 7 Ice or normal
};

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
struct s_mpd_info
{
    size_t          vertcount;                         // For malloc

    int             tiles_x;                          // Size in tiles
    int             tiles_y;
};
typedef struct s_mpd_info mpd_info_t;

// --------------------------------------------------------------------------------------------
struct s_tile_info
{
    Uint8   type;                              // Tile type
    Uint16  img;                               // Get texture from this
    Uint8   fx;                                 // Special effects flags
    Uint8   twist;
    Uint32  vrtstart;                           // Which vertex to start at

    bool_t  inrenderlist;
};
typedef struct s_tile_info tile_info_t;

// --------------------------------------------------------------------------------------------
struct s_mpd_vertex
{
    GLvector3  pos;                               // Vertex position
    Uint8      a;                                 // Vertex base light
};
typedef struct s_mpd_vertex mpd_vertex_t;

// --------------------------------------------------------------------------------------------
struct s_mpd_mem
{
    size_t          tile_count;                       // Number of tiles
    tile_info_t *   tile_list;                        // Tile info

    size_t          vcount;                           // number of vertices
    mpd_vertex_t *  vlst;                             // list of vertices
};
typedef struct s_mpd_mem mpd_mem_t;

// --------------------------------------------------------------------------------------------
struct s_mpd
{
    mpd_info_t info;
    mpd_mem_t   mem;
};
typedef struct s_mpd mpd_t;

// --------------------------------------------------------------------------------------------
struct s_tile_definition
{
    Uint8           numvertices;                // Number of vertices
    float           u[MAXMESHVERTICES];         // Vertex texture posi
    float           v[MAXMESHVERTICES];

    Uint8           command_count;                        // Number of commands
    Uint8           command_entries[MAXMESHCOMMAND];      // Entries in each command
    Uint16          command_verts[MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
};
typedef struct s_tile_definition tile_definition_t;

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
extern tile_definition_t tile_dict[MAXMESHTYPE];

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
// the raw mpd loader
mpd_t *      mpd_load( const char *modname, mpd_t * pmesh );

mpd_t *      mpd_new( mpd_t * pmesh );
mpd_t *      mpd_delete( mpd_t * pmesh );
bool_t       mpd_free( mpd_t * pmesh );

bool_t twist_to_normal( Uint8 twist, float v[], float slide );
Uint8  cartman_get_twist(int x, int y);
