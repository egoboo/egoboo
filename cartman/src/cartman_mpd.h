#pragma once

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
//********************************************************************************************

#include <egolib.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_mpd;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_tile_line_data;
typedef struct s_tile_line_data tile_line_data_t;

struct s_cartman_gl_command;
typedef struct s_cartman_gl_command cartman_gl_command_t;

struct s_cartman_mpd;
typedef struct s_cartman_mpd cartman_mpd_t;

struct s_cartman_mpd_create_info;
typedef struct s_cartman_mpd_create_info cartman_mpd_create_info_t;

struct s_tile_dictionary_lines;
typedef struct s_tile_dictionary_lines tile_dictionary_lines_t;

struct s_cartman_mpd_info;
typedef struct s_cartman_mpd_info cartman_mpd_info_t;

struct s_cartman_mpd_vertex;
typedef struct s_cartman_mpd_vertex cartman_mpd_vertex_t;

struct s_cartman_mpd_tile;
typedef struct s_cartman_mpd_tile cartman_mpd_tile_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CHAINEND 0xFFFFFFFF     // End of vertex chain
#define VERTEXUNUSED 0          // Check mesh.vrta to see if used

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))

#define TINYXY   4              // Plan tiles
#define SMALLXY 32              // Small tiles
#define BIGXY   (2 * SMALLXY)   // Big tiles

#define FIXNUM    4 // 4.129           // 4.150

#define TILE_BITS   7
#define TILE_ISIZE (1<<TILE_BITS)
#define TILE_MASK  (TILE_ISIZE - 1)
#define TILE_FSIZE ((float)TILE_ISIZE)
#define FOURNUM   ( TILE_FSIZE / (float)SMALLXY )          // Magic number

#define DEFAULT_TILE 62

#define TWIST_FLAT                      119
#define SLOPE 50            // Twist stuff

#define TILE_IS_FANOFF(XX)              ( MPD_FANOFF == (XX) )

// handle the upper and lower bits for the tile image
#define TILE_UPPER_SHIFT                8
#define TILE_LOWER_MASK                 ((1 << TILE_UPPER_SHIFT)-1)
#define TILE_UPPER_MASK                 (~TILE_LOWER_MASK)

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )
#define TILE_SET_BITS(HI,LO)            (TILE_SET_UPPER_BITS(HI) | TILE_GET_LOWER_BITS(LO))

#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )

#define DEFAULT_Z_SIZE ( 180 << 4 )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_cartman_mpd_create_info
{
    int tiles_x, tiles_y;
};

//--------------------------------------------------------------------------------------------
struct s_cartman_gl_command
{
    Uint8   numvertices;                // Number of vertices

    Uint8   ref[MPD_FAN_VERTICES_MAX];       // Lighting references

    int     x[MPD_FAN_VERTICES_MAX];         // Vertex texture posi
    int     y[MPD_FAN_VERTICES_MAX];         //

    float   u[MPD_FAN_VERTICES_MAX];         // Vertex texture posi
    float   v[MPD_FAN_VERTICES_MAX];         //

    int     count;                      // how many commands
    int     size[MPD_FAN_MAX];      // how many command entries
    int     vrt[MPD_FAN_ENTRIES_MAX];       // which vertex for each command entry
};

//--------------------------------------------------------------------------------------------
struct s_tile_line_data
{
    Uint32    count;
    Uint8     start[MPD_FAN_TYPE_MAX];
    Uint8     end[MPD_FAN_TYPE_MAX];
};

//--------------------------------------------------------------------------------------------
struct s_cartman_mpd_info
{
    int     tiles_x;                  // Size of mesh
    int     tiles_y;                  //
    size_t  tiles_count;
    size_t  vertex_count;

    float   edgex;            // Borders of mesh
    float   edgey;            //
    float   edgez;            //
};

cartman_mpd_info_t * cartman_mpd_info_ctor( cartman_mpd_info_t * );
cartman_mpd_info_t * cartman_mpd_info_dtor( cartman_mpd_info_t * );

bool_t cartman_mpd_info_init( cartman_mpd_info_t * pinfo, int vert_count, size_t tiles_x, size_t tiles_y );

//--------------------------------------------------------------------------------------------
struct s_cartman_mpd_vertex
{
    Uint32  next;   // Next vertex in fan
    float   x;      // Vertex position
    float   y;      //
    float   z;      // Vertex elevation
    Uint8   a;      // Vertex base light, VERTEXUNUSED == unused
};

cartman_mpd_vertex_t * cartman_mpd_vertex_ctor( cartman_mpd_vertex_t * );
cartman_mpd_vertex_t * cartman_mpd_vertex_dtor( cartman_mpd_vertex_t * );

bool_t cartman_mpd_vertex_ary_ctor( cartman_mpd_vertex_t ary[], size_t size );
bool_t cartman_mpd_vertex_ary_dtor( cartman_mpd_vertex_t ary[], size_t size );

//--------------------------------------------------------------------------------------------
struct s_cartman_mpd_tile
{
    Uint8   type;        // Tile fan type
    Uint8   fx;             // Rile special effects flags
    Uint16  tx_bits;        // Tile texture bits and special tile bits
    Uint8   twist;          // Surface normal
    Uint32  vrtstart;       // Which vertex to start at
};

cartman_mpd_tile_t * cartman_mpd_tile_ctor( cartman_mpd_tile_t * );
cartman_mpd_tile_t * cartman_mpd_tile_dtor( cartman_mpd_tile_t * );

bool_t cartman_mpd_tile_ary_ctor( cartman_mpd_tile_t ary[], size_t size );
bool_t cartman_mpd_tile_ary_dtor( cartman_mpd_tile_t ary[], size_t size );

//--------------------------------------------------------------------------------------------
struct s_cartman_mpd
{
    Uint32               vrt_free;                        // Number of free vertices
    Uint32               vrt_at;                          // Current vertex check for new
    cartman_mpd_vertex_t vrt[MPD_VERTICES_MAX];

    cartman_mpd_info_t   info;
    cartman_mpd_tile_t   fan[MPD_TILE_MAX];

    Uint32               fanstart[MPD_TILEY_MAX];   // Y to fan number
};

cartman_mpd_t * cartman_mpd_ctor( cartman_mpd_t * );
cartman_mpd_t * cartman_mpd_dtor( cartman_mpd_t * );
cartman_mpd_t * cartman_mpd_renew( cartman_mpd_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern cartman_mpd_t mesh;

extern size_t numwritten;
extern size_t numattempt;

extern tile_line_data_t tile_dict_lines[MPD_FAN_TYPE_MAX];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// loading/saving
cartman_mpd_t * cartman_mpd_load_vfs( /* const char *modname, */ cartman_mpd_t * pmesh );
cartman_mpd_t * cartman_mpd_save_vfs( /*const char *modname,*/ cartman_mpd_t * pmesh );
cartman_mpd_t * cartman_mpd_create( cartman_mpd_t * pmesh, int tiles_x, int tiles_y );

void cartman_mpd_make_twist();
void cartman_mpd_make_fanstart( cartman_mpd_t * pmesh );

void cartman_mpd_free_vertex_count( cartman_mpd_t * pmesh );
int  cartman_mpd_count_vertices( cartman_mpd_t * pmesh );

void cartman_mpd_free_vertices( cartman_mpd_t * pmesh );
int cartman_mpd_find_free_vertex( cartman_mpd_t * pmesh );
bool_t cartman_mpd_link_vertex( cartman_mpd_t * pmesh, int iparent, int child );

Uint8 cartman_mpd_get_fan_twist( cartman_mpd_t * pmesh, Uint32 fan );
float cartman_mpd_get_level( cartman_mpd_t * pmesh, float x, float y );
int cartman_mpd_get_vertex( cartman_mpd_t * pmesh, int x, int y, int num );
int cartman_mpd_get_fan( cartman_mpd_t * pmesh, int x, int y );

void cartman_mpd_remove_fan( cartman_mpd_t * pmesh, int fan );
int cartman_mpd_add_fan( cartman_mpd_t * pmesh, int fan, float x, float y );

void tile_dict_lines_add( int fantype, int start, int end );
void cartman_tile_dictionary_load_vfs();

// utility
Uint8 cartman_mpd_calc_twist( int x, int y );