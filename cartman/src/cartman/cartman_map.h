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

#include "egolib/egolib.h"

#include "cartman/cartman_typedef.h"

#include "egolib/FileFormats/map_tile_dictionary.h"

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

#define FOURNUM   ( TILE_FSIZE / (float)SMALLXY )          // Magic number

#define DEFAULT_TILE 62

#define TWIST_FLAT                      119
#define SLOPE 50            // Twist stuff

#define TILE_IS_FANOFF(XX)              ( MAP_FANOFF == (XX) )

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

#define CART_VALID_VERTEX_RANGE(IVRT) ( (CHAINEND != (IVRT)) && VALID_MPD_VERTEX_RANGE(IVRT) )


#define GRID_TO_POS( GRID ) ( (float)(GRID) / 3.0f * TILE_FSIZE )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct cartman_mpd_create_info_t
{
    int tiles_x, tiles_y;
};

//--------------------------------------------------------------------------------------------
struct tile_line_data_t
{
    Uint32    count;
    Uint8     start[MAP_FAN_TYPE_MAX];
    Uint8     end[MAP_FAN_TYPE_MAX];
};

//--------------------------------------------------------------------------------------------
struct cartman_mpd_info_t
{
    int     tiles_x;                  // Size of mesh
    int     tiles_y;                  //
    size_t  tiles_count;
    size_t  vertex_count;

    /**
     * @{
     * @brief
     *  The borders of the mesh.
     */
    float edgex, edgey, edgez;
    /**@}*/

    /**
     * @brief
     *  Construct this map information with its default values.
     */
    cartman_mpd_info_t();

    /**
     * @brief
     *  Destruct this map information.
     */
    virtual ~cartman_mpd_info_t();

    /**
     * @brief
     *  Reset this map information to its default values.
     */
    void reset();    

};



bool cartman_mpd_info_init(cartman_mpd_info_t *self, int vert_count, size_t tiles_x, size_t tiles_y);

//--------------------------------------------------------------------------------------------
namespace Cartman
{
    struct mpd_vertex_t
    {

        /**
         * @brief
         *  The next vertex in the fan of this vertex.
         * @default
         *  CHAINEND
         */
        Uint32 next;

        /**
         * @{
         * @brief
         *  The vertex position.
         * @remark
         *  @a z is sometimes referred to as the "elevation" of the vertex.
         * @default
         *  <tt>(0,0,0)</tt>
         * @todo
         *  Use a 3D vector type to represent the position.
         */
        float x, y, z;
        /** @} */

        /**
         * @brief
         *  The basic light of the vertex.
         * @remark
         *  If this is @a VERTEXUNUSED then the basic light is ignored.
         * @default
         *  @a VERTEXUNUSED
         */
        Uint8 a;

        /**
         * @brief
         *  Construct this vertex with its default values.
         */
        mpd_vertex_t();

        /**
         * @brief
         *  Destruct this vertex
         */
        virtual ~mpd_vertex_t();

        /**
         * @brief
         *  Reset this vertex to its default values.
         */
        void reset();

    };
}

/**
 * @brief
 *  Information about a tile.
 */
struct cartman_mpd_tile_t
{

    /**
     * @brief
     *  The fan type of the tile.
     * @default
     *  <tt>0</tt>
     */
    Uint8 type;

    /**
     * @brief
     *  The special effects flags of the tile.
     * @default
     *  <tt>MAPFX_WALL | MAPFX_IMPASS</tt>
     */
    Uint8 fx;

    /**
     * @brief
     *  The texture bits and special tile bits of the tile.
     * @default
     *  <tt>MAP_FANOFF</tt>
     */
    Uint16 tx_bits;

    /**
     * @brief
     *  The surface normal of this tile.
     * @default
     *  <tt>TWIST_FLAT</tt>
     */
    Uint8 twist;

    /**
     * @brief
     *  The index of the first vertex of this tile in the vertex array.
     * @default
     *  <tt>MAP_FAN_ENTRIES_MAX</tt>
     */
    Uint32 vrtstart;

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

};

//--------------------------------------------------------------------------------------------
struct cartman_mpd_t
{
    /**
     * @brief
     *  The number of free vertices.
     * @default
     *  <tt>MAP_VERTICES_MAX</tt>
     */
    Uint32 vrt_free;
    Uint32 vrt_at;                          // Current vertex check for new
    std::array<Cartman::mpd_vertex_t, MAP_VERTICES_MAX> vrt2;

    cartman_mpd_info_t   info;
    std::array<cartman_mpd_tile_t,MAP_TILE_MAX> fan2;

    /**
     * @brief
     *  Maps y-coordinates to "fan indices" i.e. indices into the array @a fan.
     * @default
     *  All y-coordinates are mapped to fan index @a 0.
     */
    std::array<Uint32,MAP_TILE_MAX_Y> fanstart2;

    /**
     * @brief
     *  Construct this mesh.
     */
    cartman_mpd_t();

    /**
     * @brief
     *  Destruct this mesh.
     */
    virtual ~cartman_mpd_t();

    /**
     * @brief
     *  Reset this mesh.
     */
    cartman_mpd_t *reset();

    /**
     * @brief
     *  Allocates the vertices needed for a fan.
     * @param ifan
     *  the fan index
     * @param x, y
     *  the position in world coordinates
     * @return
     *  the index of the firt vertex of the fan on success, a negative value on failure
     * @todo
     *  Rename to <tt>addFan</tt>.
     */
    int add_ifan(int ifan, float x, float y);

    /**
     * @brief
     *  Allocates the vertices needed for a fan.
     * @param pfan
     *  the fan
     * @param x, y
     *  the position in world coordinates
     * @return
     *  the index of the firt vertex of the fan on success, a negative value on failure
     * @todo
     *  Rename to <tt>addFan</tt>.
     */
    int add_pfan(cartman_mpd_tile_t *pfan, float x, float y);

    /**
     * @brief
     *  Removes a fan's vertices from usage and sets the fan to not be drawn.
     * @param ifan
     *  the fan index
     * @todo
     *  Rename to <tt>removeFan</tt>.
     */
    void remove_ifan(int ifan);

    /**
     * @brief
     *  Removes a fan's vertices from usage and sets the fan to not be drawn.
     * @param pfan
     *  the fan
     * @todo
     *  Rename to <tt>removeFan</tt>.
     */
    void remove_pfan(cartman_mpd_tile_t *pfan);

    /**
     * @brief
     *  Get the fan index at a point.
     * @param mapx, mapy
     *  the point in map coordinates
     * @return
     *  the fan index if it exists at the point, @a -1 otherwise
     */
    int get_ifan(int mapx, int mapy);

    /**
     * @brief
     *  Get the fan at a point.
     * @param mapx, mapy
     *  the point in map coordinates
     * @retun
     *  the fan if it exists at the point, @a nullptr otherwise
     * @remark
     *  A call
     *  @code
     *  get_pfan(mapx,mapy)
     *  @endcode
     *  is conceptually equivalent to
     *  @code
     *  get_pfan(get_ifan(mapx,mapy))
     *  @endcode
     */
    cartman_mpd_tile_t *get_pfan(int mapx, int mapy);

    /**
     * @brief
     *  Get the fan at a fan index.
     * @param ifan
     *  the fan index
     * @return
     *  the fan if the fan index is within bounds, @a nullptr otherwise
     */
    cartman_mpd_tile_t *get_pfan(int ifan);

    /**
     * @brief
     *  Get the index of a free vertex.
     * @return
     *  the index of a free vertex, @a -1 if none was found
     */
    int find_free_vertex();

    /**
     * @brief
     *  Get the elevation at a point.
     * @param x,y
     *  the point in map coordinates
     * @return
     *  the elevation at the point.
     *  In particular, the elevation is @a 0 if the point is outside the map bounds.
     */
    float get_level(int mapx, int mapy);

    /**
     * @brief
     *  Get the elevation at a point.
     * @param x,y
     *  the point in world coordinates
     * @return
     *  the elevation at the point.
     *  In particular, the elevation is @a 0 if the point is outside the map bounds.
     */
    float get_level(float x, float y);

    /**
     * @brief
     *  Get the vertex index of a vertex in a fan.
     * @param mapx, mapy
     *  a point in map coordinates
     * @param index
     *  the fan-relative vertex index
     * @return
     *  the vertex index if it exists, @a -1 otherwise.
     * @remark
     *  A call
     *  @code
     *  get_ivrt(mapx,mapy,index)
     *  @endcode
     *  is conceptually equivalent to
     *  @code
     *  get_ivrt(get_fan(mapx,mapy),index)
     *  @endcode
     */
    int get_ivrt_xy(int mapx, int mapy, int index);

    /**
     * @brief
     *  Get the vertex index of a vertex in a fan.
     * @param ifan
     *  the fan index
     * @param index
     *  the fan-relative vertex index
     * @return
     *  the vertex index if it exists, @a -1 otherwise.
     */
    int get_ivrt_fan(int ifan, int index);

    /**
     * @brief
     *  Get the vertex index of a vertex in a fan.
     * @param pfan
     *  the fan
     * @param index
     *  the fan-relative vertex index
     * @return
     *  the vertex index if it exists, @a -1 otherwise.
     */
    int get_ivrt_pfan(cartman_mpd_tile_t *pfan, int index);

    /**@{*/
    /**
     * @brief
     *  Get the vertex at a vertex index.
     * @param ivrt
     *  the vertex index
     * @return
     *  the vertex if the vertex index is within bounds, @a nullptr otherwise
     */
    Cartman::mpd_vertex_t *get_vertex(int ivrt);
    const Cartman::mpd_vertex_t *get_vertex(int ivrt) const;
    /**@}*/

    static Cartman::mpd_vertex_t *get_pvrt_idx(cartman_mpd_t *self, cartman_mpd_tile_t *pfan, int idx, int *ivrt_ptr);
    static Cartman::mpd_vertex_t *get_pvrt_ivrt(cartman_mpd_t *self, cartman_mpd_tile_t *pfan, int ivrt);


public:

    /**
     * @brief
     *  Re-count used vertices.
     * @return
     *  the number of used vertices.
     */
    int count_used_vertices();

protected:
    /**
     * @brief
     *  Re-count unused vertices and update self->vrt_free.
     */
    void free_vertex_count();
};



/// @todo Removet his, use cartman_mpd_t::get_vertex(int).
#define CART_MPD_VERTEX_PTR(PMESH, IVERTEX) (!(PMESH) ? nullptr : (PMESH)->get_vertex(IVERTEX))

/// @todo Remove this, use cartman_mpd_t::get_pfan(int).
#define CART_MPD_FAN_PTR(PMESH, IFAN)       (!(PMESH) ? nullptr : (PMESH)->get_pfan(IFAN))

/**
 * @brief
 *  Project a point from world to map coordinates.
 * @param worldx, worldy
 *  the point in world coordinates
 * @param mapx, mapy
 *  the point in map coordinates
 * @remark
 *  Many world coordinate points are be projected on the same map coordinate.
 */
inline void worldToMap(float worldx, float worldy, int& mapx, int& mapy)
{
    mapx = FLOOR(worldx / TILE_FSIZE);
    mapy = FLOOR(worldy / TILE_FSIZE);
}

/**
 * @brief
 *  Project a point from map coordinates to world coordinates.
 * @param mapx, mapy
 *  the point in map coordinates
 * @param worldx, worldy
 *  the point in world coordinates
 * @param tx, ty
 *   see remarks
 * @remark
 *   As a point in map coordinates maps to a rectangle of points in world coordinates;
 *   to select a distinct point from the rectangle, the paramters @a tx and @a ty are
 *   used.
 */
inline void mapToWorld(int mapx, int mapy, float& worldx, float& worldy, float tx,float ty)
{
    worldx = mapx * TILE_FSIZE + tx * TILE_FSIZE;
    worldy = mapy * TILE_FSIZE + ty * TILE_FSIZE;
}





int cartman_mpd_free_vertex_list(cartman_mpd_t *self, int list[], size_t size);
int cartman_mpd_allocate_vertex_list(cartman_mpd_t *self, int list[], size_t size, size_t count);
void cartman_mpd_make_fanstart(cartman_mpd_t *self);


/**
 * @brief
 *  Set all vertices to unused.
 */
void cartman_mpd_free_vertices(cartman_mpd_t *self);

bool cartman_mpd_link_vertex(cartman_mpd_t *self, int iparent, int child);
Uint8 cartman_mpd_get_fan_twist(cartman_mpd_t *self, Uint32 fan);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern cartman_mpd_t mesh;

extern size_t numwritten;
extern size_t numattempt;

/// @todo This global has to be merged into the map.
extern tile_line_data_t tile_dict_lines[MAP_FAN_TYPE_MAX];

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  Load a Cartman map from the map file.
 * @param self
 *  the Cartman map to write the data to
 */
cartman_mpd_t *cartman_mpd_load_vfs(cartman_mpd_t *self);
/**
 * @brief
 *  Save a Cartman map to the map file.
 * @param self
 *  the Cartman map to read the data from
 */
cartman_mpd_t *cartman_mpd_save_vfs(cartman_mpd_t *self);
/**
 * @brief
 *  Create an empty Cartman map.
 * @param self
 *  the Cartman map to write the data to
 * @param tiles_x, tiles_y
 *  the size, in tiles, of the map along the x- and y-axes
 */
cartman_mpd_t *cartman_mpd_create(cartman_mpd_t *self, int tiles_x, int tiles_y);



void cartman_mpd_make_twist();
void tile_dict_lines_add( int fantype, int start, int end );
void cartman_tile_dictionary_load_vfs();

// utility
Uint8 cartman_mpd_calc_twist( int dx, int dy );
