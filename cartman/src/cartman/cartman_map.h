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

#pragma once

#include "cartman/Vertex.hpp"
#include "cartman/cartman_typedef.h"
#include "cartman/Tile.hpp"
#include "egolib/FileFormats/map_tile_dictionary.h"
#include "egolib/Mesh/Info.hpp"

//--------------------------------------------------------------------------------------------

#define TINYXY   4              // Plan tiles
#define SMALLXY 32              // Small tiles
#define BIGXY   (2 * SMALLXY)   // Big tiles

#define FOURNUM   ( Info<float>::Grid::Size() / (float)SMALLXY )          // Magic number

#define DEFAULT_TILE 62


#define TILE_IS_FANOFF(XX)              ( MAP_FANOFF == (XX) )

// handle the upper and lower bits for the tile image

#define TILE_SET_BITS(HI,LO)            (TILE_SET_UPPER_BITS(HI) | TILE_GET_LOWER_BITS(LO))
#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )
#define DEFAULT_Z_SIZE ( 180 << 4 )
#define CART_VALID_VERTEX_RANGE(IVRT) ( (CHAINEND != (IVRT)) && VALID_MPD_VERTEX_RANGE(IVRT) )
#define GRID_TO_POS( GRID ) ( (float)(GRID) / 3.0f * Info<float>::Grid::Size() )

//--------------------------------------------------------------------------------------------
struct cartman_mpd_create_info_t
{
    int tiles_x, tiles_y;
};

//--------------------------------------------------------------------------------------------
struct tile_line_data_t
{
    uint32_t  count;
    uint8_t   start[MAP_FAN_TYPE_MAX];
    uint8_t   end[MAP_FAN_TYPE_MAX];
};

//--------------------------------------------------------------------------------------------
struct cartman_mpd_info_t : public Ego::MeshInfo
{
private:
    /**
	 * @brief
	 *  The border of the mesh along the x-axis.
     */
	float _edgeX;
	/**
	 * @brief
	 *  The border of the mesh along the y-axis.
	 */
	float _edgeY;
	/**
	 * @brief
	 *  The border of the mesh along the z-axis.
	 */
	float _edgeZ;

public:
	/**
	 * @brief
	 *  Get the border of the mesh along the x-axis.
	 * @return
	 *  the border of the mesh along the x-axis
	 */
	float getEdgeX() const {
		return _edgeX;
	}
	/**
	 * @brief
	 *  Get the border of the mesh along the y-axis
	 * @return
	 *  the border of the mesh along the y-axis
	 */
	float getEdgeY() const {
		return _edgeY;
	}
	/**
	 * @brief
	 *  Get the border of the mesh along the z-axis
	 * @return
	 *  the border of the mesh along the z-axis
	 */
	float getEdgeZ() const {
		return _edgeZ;
	}

    /**
     * @brief
	 *  Construct this mesh information for a mesh with
	 *  0
	 *  tiles along the x- and y-axes.
     */
    cartman_mpd_info_t();
	/**
	 * @brief
	 *  Construct this mesh information for a mesh with
	 *  the specified number of tiles
	  * along the x- and y-axes.
	 */
	cartman_mpd_info_t(size_t tileCountX, size_t tileCountY);
	/**
	 * @brief
	 *  Construct this mesh information for a mesh with
	 *  the specified number of tiles
	 *  along the x- and y-axes and
	 *  the specified number of vetices.
	 */
	cartman_mpd_info_t(size_t vertexCount, size_t tileCountX, size_t tilCountY);
	/**
	 * @brief
	 *  Construct this mesh information for a mesh with
	 *  the specified number of tiles
	 *  along the x- and y-axes and
	 *  the specified edges.
	 */
	cartman_mpd_info_t(size_t tileCountX, size_t tileCountY, float edgeX, float edgeY, float edgeZ);
	/**
	 * @brief
	 *  Construct this mesh information for a mesh with
	 *  the specified number of tiles
	 *  along the x- and y-axes,
	 *  the specified number of vertices and
	 *  the specified edges.
	 */
	cartman_mpd_info_t(size_t vertexCount, size_t tileCountX, size_t tileCountY, float edgeX, float edgeY, float edgeZ);
	/**
	 * @brief
	 * Copy - construct this mesh information.
     * @param other
	 *  the construction source
	 */
	cartman_mpd_info_t(const cartman_mpd_info_t& other);

	/**
	 * @brief
	 *  Assign this mesh information from another mesh information.
	 * @param other
	 *  the assignment source
	 */
	cartman_mpd_info_t& operator=(const cartman_mpd_info_t& other);

    /**
     * @brief
     *  Destruct this map information.
     */
    virtual ~cartman_mpd_info_t();

    /**
     * @brief
     *  Reset this map information to its default values.
     */
	virtual void reset() override;    

	/**
	 * @brief
	 *  Set the vertex count.
	 * @param vertexCount
	 *  the vertex count
	 */
	void setVertexCount(size_t vertexCount) {
		_vertexCount = vertexCount;
	}

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
    uint32_t vrt_free;
    uint32_t vrt_at;                          // Current vertex check for new
    std::array<Cartman::mpd_vertex_t, MAP_VERTICES_MAX> vrt2;

    cartman_mpd_info_t   info;
    std::array<cartman_mpd_tile_t,MAP_TILE_MAX> fan2;

    /**
     * @brief
     *  Maps y-coordinates to "fan indices" i.e. indices into the array @a fan.
     * @default
     *  All y-coordinates are mapped to fan index @a 0.
     */
    std::array<uint32_t,MAP_TILE_MAX_Y> fanstart2;

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
    int get_ifan(const Index2D& index2d);

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
    cartman_mpd_tile_t *get_pfan(const Index2D& index2d);

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
    float get_level(const Index2D& index2d);

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
    int get_ivrt_xy(Index2D index2d, int index);

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
    mapx = std::floor(worldx / Info<float>::Grid::Size());
    mapy = std::floor(worldy / Info<float>::Grid::Size());
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
    worldx = mapx * Info<float>::Grid::Size() + tx * Info<float>::Grid::Size();
    worldy = mapy * Info<float>::Grid::Size() + ty * Info<float>::Grid::Size();
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
uint8_t cartman_mpd_get_fan_twist(cartman_mpd_t *self, uint32_t fan);

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
