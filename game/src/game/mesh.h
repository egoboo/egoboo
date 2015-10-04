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

/// @file game/mesh.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/lighting.h"

//--------------------------------------------------------------------------------------------
// external types
//--------------------------------------------------------------------------------------------

struct ego_mesh_info_t;
struct oglx_texture_t;

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
			return 1 << Info<int>::Grid::Exponent();
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

//--------------------------------------------------------------------------------------------

/// @todo max blocks in the x direction
/// max blocks in the y direction
#define GRID_BLOCKY_MAX (( MAP_TILE_MAX_Y >> (Info<int>::Block::Exponent()-Info<int>::Grid::Exponent()) )+1)  



#define VALID_GRID(PMPD, ID) ( (INVALID_TILE!=(ID)) && (NULL != (PMPD)) && (ID < (PMPD)->info.tiles_count) )

/// mesh physics
#define SLIDE                           0.04f         ///< Acceleration for steep hills
#define SLIDEFIX                        0.08f         ///< To make almost flat surfaces flat
#define TWIST_FLAT                      119

#define TILE_UPPER_SHIFT                8
#define TILE_LOWER_MASK                 ((1 << TILE_UPPER_SHIFT)-1)
#define TILE_UPPER_MASK                 (~TILE_LOWER_MASK)

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )


//--------------------------------------------------------------------------------------------

typedef GLXvector3f normal_cache_t[4];
typedef float       light_cache_t[4];

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *  An enumeration of coordinate system used by/for/together with meshes.
 */
enum class CoordinateSystem
{
    /**
     * @brief
     *  "world" coordinates.
     */
    World,
    /**
     * @brief
     *  "grid" coordinates.
     */
    Grid,
    /**
     * @brief
     *  "block" coordinates.
     */
    Block,
};

/**
 * @brief
 *  A template of a point.
 * @param _Type
 *  the type of the coordinate values of the point
 * @param _CoordinateSystem
 *  the coordinate system of the point
 */
template <typename _Type, CoordinateSystem _CoordinateSystem>
struct Point
{
private:

    _Type _x;

    _Type _y;

public:

    Point(const _Type& x, const _Type& y) :
        _x(x), _y(y)
    {}

    Point(const Point<_Type, _CoordinateSystem>& other) :
        _x(other._x), _y(other._y)
    {}

    Point<_Type, _CoordinateSystem>& operator=(const Point<_Type, _CoordinateSystem>& other)
    {
        _x = other._x;
        _y = other._y;
    }

    const _Type& getX() const
    {
        return _x;
    }

    const _Type& getY() const
    {
        return _y;
    }

};

typedef Point<int, CoordinateSystem::Block> PointBlock;
typedef Point<int, CoordinateSystem::Grid> PointGrid;
typedef Point<float, CoordinateSystem::World> PointWorld;

/**
 * @brief
 *  An enumeration of index systems used by/for/together with meshes.
 */
enum class IndexSystem
{
    /**
     * @brief
     *  "tile" indices.
     */
    Tile,

    /**
     * @brief
     *  "block" indices.
     */
    Block,
};

/**
 * @brief
 *  A template of an index.
 * @param _Type
 *  the type of the index value of the index
 * @param _IndexSystem
 *  the index type of the index
 * @param _InvalidIndex
 *   the index value for an invalid index of the index
 * @todo
 *  Because of (and as always just because of) Microsoft (and always only Microsoft) is
 *  incapable of providing proper C++ 11 support in time (unlike other compiler vendors)
 *  <tt>_Type _InvalidIndex = std::numeric_limits<_Type>::max()</tt> can't be used.
 */
template <typename _Type, IndexSystem _IndexSystem, _Type _InvalidIndex>
struct Index
{

private:

    _Type _i;

public:

    static const Index<_Type, _IndexSystem, _InvalidIndex> Invalid;

    Index() :
        _i(_InvalidIndex)
    {}

    Index(const _Type& i) :
        _i(i)
    {}

    Index(const Index<_Type, _IndexSystem, _InvalidIndex>& other) :
        _i(other._i)
    {}

    Index<_Type, _IndexSystem, _InvalidIndex>& operator=(const Index<_Type, _IndexSystem, _InvalidIndex>& other)
    {
        _i = other._i;
        return *this;
    }

    bool operator==(const Index<_Type, _IndexSystem, _InvalidIndex>& other) const
    {
        return _i == other._i;
    }

    bool operator!=(const Index<_Type, _IndexSystem, _InvalidIndex>& other) const
    {
        return _i != other._i;
    }

    bool operator<(const Index<_Type, _IndexSystem, _InvalidIndex>& other) const
    {
        return _i < other._i;
    }

    bool operator<=(const Index<_Type, _IndexSystem, _InvalidIndex>& other) const
    {
        return _i <= other._i;
    }

    bool operator>(const Index<_Type, _IndexSystem, _InvalidIndex>& other) const
    {
        return _i > other._i;
    }

    bool operator>=(const Index<_Type, _IndexSystem, _InvalidIndex>& other) const
    {
        return _i >= other._i;
    }

    const _Type& getI() const
    {
        return _i;
    }

    Index<_Type, _IndexSystem, _InvalidIndex>& operator++()
    {
        _i++;
        return *this;
    }
    Index<_Type, _IndexSystem, _InvalidIndex> operator++(int)
    {
        _Type j = _i;
        _i++;
        return Index<_Type,_IndexSystem,_InvalidIndex>(j);
    }

};

template <typename _Type, IndexSystem _IndexSystem, _Type _InvalidIndex>
const Index<_Type, _IndexSystem, _InvalidIndex> Index<_Type,_IndexSystem,_InvalidIndex>::Invalid;

/// @brief The index of a tile.
/// @todo UINT32_MAX is used because of Microsoft's Visual Studio 2013 lacking constexpr support
///       such that we could use std::numeric_limits<Uint32>::max().
typedef Index<Uint32, IndexSystem::Tile,UINT32_MAX> TileIndex;

/// @brief The index of a block.
/// @todo UINT32_MAX is used because of Microsoft's Visual Studio 2013 lacking constexpr support
///       such that we could use std::numeric_limits<Uint32>::max().
typedef Index<Uint32, IndexSystem::Block,UINT32_MAX> BlockIndex;

//--------------------------------------------------------------------------------------------

/// The data describing an Egoboo tile
class ego_tile_info_t
{
public:
    static const std::shared_ptr<ego_tile_info_t> NULL_TILE;

    ego_tile_info_t();

    const AABB2f& getAABB2D() const { return _aabb; }

public:
    // the "inherited" tile info
    size_t _itile;
    uint8_t _type;                             ///< Tile type
    uint16_t _img;                             ///< Get texture from this
    size_t _vrtstart;                          ///< Which vertex to start at

    // some extra flags
    bool _fanoff;                            ///< display this tile?

    // tile corner lighting parameters
    normal_cache_t _ncache;                     ///< the normals at the corners of this tile
    light_cache_t  _lcache;                     ///< the light at the corners of this tile
    bool           _request_lcache_update;      ///< has this tile been tagged for a lcache update?
    int            _lcache_frame;               ///< the last frame in which the lighting cache was updated

    // tile vertex lighting parameters
    bool           _request_clst_update;        ///< has this tile been tagged for a color list update?
    int            _clst_frame;                 ///< the last frame in which the color list was updated
    light_cache_t  _d1_cache;                   ///< the estimated change in the light at the corner of the tile
    light_cache_t  _d2_cache;                   ///< the estimated change in the light at the corner of the tile

    // the bounding boc of this tile
    oct_bb_t       _oct;                        ///< the octagonal bounding box for this tile
	AABB2f         _aabb;
};

inline bool TILE_IS_FANOFF(const std::shared_ptr<ego_tile_info_t>& tileInfo) {
	return MAP_FANOFF == tileInfo->_img;
}
inline bool TILE_IS_FANOFF(const ego_tile_info_t *tileInfo) {
	return MAP_FANOFF == tileInfo->_img;
}
inline bool TILE_HAS_INVALID_IMAGE(const ego_tile_info_t& tileInfo) {
	return HAS_SOME_BITS(TILE_UPPER_MASK, tileInfo._img);
}


//--------------------------------------------------------------------------------------------

typedef BIT_FIELD GRID_FX_BITS;

/// The data describing an Egoboo grid
struct ego_grid_info_t
{
    // MODIFY THESE FLAGS
    GRID_FX_BITS    _base_fx;                   ///< the special effects flags in the mpd
    GRID_FX_BITS    _pass_fx;                   ///< the working copy of base_fx, which might be modified by passages
    Uint8           _twist;                     ///< The orientation of the tile

    // the lighting info in the upper left hand corner of a grid
    Uint8            _a, _l;                   ///< the raw mesh lighting... pretty much ignored
    lighting_cache_t _cache;                   ///< the per-grid lighting info
    int              _cache_frame;             ///< the last frame in which the cache was calculated

	ego_grid_info_t();
	~ego_grid_info_t();
    static GRID_FX_BITS get_all_fx(const ego_grid_info_t *self);
    static GRID_FX_BITS test_all_fx(const ego_grid_info_t *self, const GRID_FX_BITS bits);
	static bool add_pass_fx(ego_grid_info_t *self, const GRID_FX_BITS bits);
	static bool sub_pass_fx(ego_grid_info_t *self, const GRID_FX_BITS bits);
	static bool set_pass_fx(ego_grid_info_t *self, const GRID_FX_BITS bits);
};

//--------------------------------------------------------------------------------------------

struct grid_mem_t
{
    int _grids_x;         ///< Size in grids
    int _grids_y;
    size_t _grid_count;   ///< How many grids.

    int _blocks_x;        ///< Size in blocks
    int _blocks_y;
    Uint32 _blocks_count; ///< Number of blocks (collision areas)

    float _edge_x; ///< Limits.
    float _edge_y;

    Uint32 *_blockstart; ///< List of blocks that start each row.
    Uint32 *_tilestart;  ///< List of tiles  that start each row.

protected:
    // the per-grid info
    ego_grid_info_t* _grid_list;                       ///< tile command info
public:
	grid_mem_t();
    ~grid_mem_t();
    bool alloc(const ego_mesh_info_t& info);
    void free();
    /**
     * @brief
     *  This function builds a look up table to ease calculating the fan number given an x,y pair.
     */
	void make_fanstart(const ego_mesh_info_t& info);

	ego_grid_info_t *get(const TileIndex& index)
	{
		// Validate arguments.
		if (TileIndex::Invalid == index)
		{
			return nullptr;
		}
		// Assert that the grids are allocated and the index is within bounds.
		if (!_grid_list || index.getI() >= _grid_count)
		{
			return nullptr;
		}
		return _grid_list + index.getI();
	}

    const ego_grid_info_t *get(const TileIndex& index) const
    {
        // Validate arguments.
        if (TileIndex::Invalid == index)
        {
            return nullptr;
        }
        // Assert that the grids are allocated and the index is within bounds.
        if (!_grid_list || index.getI() >= _grid_count)
        {
            return nullptr;
        }
        return _grid_list + index.getI();
    }
};

//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated mesh memory
struct tile_mem_t
{
private:
	std::vector<std::vector<std::shared_ptr<ego_tile_info_t>>> _tileList;   ///< tile command info
	size_t _tileCount;
public:
    AABB3f _bbox;                 ///< bounding box for the entire mesh

    // the per-vertex info to be presented to OpenGL
    size_t _vert_count;                 ///< number of vertices
    GLXvector3f *_plst;                 ///< the position list
    GLXvector2f *_tlst;                 ///< the texture coordinate list
    GLXvector3f *_nlst;                 ///< the normal list
    GLXvector3f *_clst;                 ///< the color list (for lighting the mesh)

	tile_mem_t();
	~tile_mem_t();
    void free();
    bool alloc(const ego_mesh_info_t& info);

    const std::shared_ptr<ego_tile_info_t>& get(const TileIndex& index) const
    {
        // Assert that the index is within bounds.
        if (TileIndex::Invalid == index)
        {
            return ego_tile_info_t::NULL_TILE;
        }
        return getTile(index.getI());
    }

	const std::shared_ptr<ego_tile_info_t>& getTile(const size_t x, const size_t y)
	{
		if (x >= _tileList.size()) return ego_tile_info_t::NULL_TILE;
		if (y >= _tileList[x].size()) return ego_tile_info_t::NULL_TILE;

		//Retrieve the tile and return it
		return _tileList[x][y];
	}

	const std::shared_ptr<ego_tile_info_t>& getTile(const size_t x, const size_t y) const
	{
		if (x >= _tileList.size()) return ego_tile_info_t::NULL_TILE;
		if (y >= _tileList[x].size()) return ego_tile_info_t::NULL_TILE;

		//Retrieve the tile and return it
		return _tileList[x][y];
	}

	const std::shared_ptr<ego_tile_info_t>& getTile(const size_t index)
	{
		if (index >= _tileCount) return ego_tile_info_t::NULL_TILE;

		//Extract X and Y positions
		size_t x = index % _tileList.size();
		size_t y = index / _tileList[x].size();

		//Retrieve the tile and return it
		return getTile(x, y);
	}

    const std::shared_ptr<ego_tile_info_t>& getTile(const size_t index) const
    {
        if(index >= _tileCount) return ego_tile_info_t::NULL_TILE;

        //Extract X and Y positions
        size_t x = index % _tileList.size();
        size_t y = index / _tileList[x].size();

        //Retrieve the tile and return it
        return getTile(x, y);
    }

    size_t getTileCount() const {return _tileCount;}

    std::vector<std::vector<std::shared_ptr<ego_tile_info_t>>>& getAllTiles() { return _tileList; }

};

//--------------------------------------------------------------------------------------------

struct mpdfx_list_ary_t
{
    size_t _cnt;
    size_t _idx;
    size_t *_lst;

	mpdfx_list_ary_t();
	~mpdfx_list_ary_t();
    void reset();
    bool push(size_t value);
	void alloc(size_t size);
	void dealloc();
};



//--------------------------------------------------------------------------------------------
struct mpdfx_lists_t
{
	// If @a true, the lists are constructed & allocated but are not synchronized with grid memory.
    bool dirty;

    mpdfx_list_ary_t sha;
    mpdfx_list_ary_t drf;
    mpdfx_list_ary_t anm;
    mpdfx_list_ary_t wat;
    mpdfx_list_ary_t wal;
    mpdfx_list_ary_t imp;
    mpdfx_list_ary_t dam;
    mpdfx_list_ary_t slp;

	mpdfx_lists_t();
	~mpdfx_lists_t();
    bool alloc(const ego_mesh_info_t& info);
    void dealloc();
    void reset();
    int push(GRID_FX_BITS fx_bits, size_t value);
    bool synch(const grid_mem_t& other, bool force);
};



//--------------------------------------------------------------------------------------------

/// The generic parameters describing an ego_mesh
struct ego_mesh_info_t
{
    size_t _vertcount;    ///< For malloc

    /**
     * @brief
     *  The size, in tiles, along the x-axis.
     * @todo
     *  Rename to @a sizeX. The type should be @a size_t.
     */
    int _tiles_x;
    /**
     * @brief
     *  The size, in tiles, along the y-axis.
     * @todo
     *  Rename to @a sizeY. The type should be @a size_t.
     */
    int _tiles_y;
    /**
     * @brief
     *  The number of tiles in the mesh.
     * @invariant
     *  <tt>size = sizeX * sizeY</tt>
     * @todo
     *  Rename to @a size. The type should be @a size_t.
     */
    uint32_t _tiles_count;

	ego_mesh_info_t();
	~ego_mesh_info_t();
    void reset(int numvert, size_t tiles_x, size_t tiles_y);
};

//--------------------------------------------------------------------------------------------

// struct for caching fome values for wall collisions
/// MH: This seems to be used like an iterator.
struct mesh_wall_data_t
{
	int   ix_min, ix_max, iy_min, iy_max;
	float fx_min, fx_max, fy_min, fy_max;

	const ego_mesh_info_t *pinfo;
	const ego_grid_info_t *glist;
};


/// Egoboo's representation of the .mpd mesh file
class ego_mesh_t
{
public:
    ego_mesh_t();
    ego_mesh_t(int tiles_x, int tiles_y);

    ~ego_mesh_t();

    ego_mesh_info_t _info;
    tile_mem_t _tmem;
    grid_mem_t _gmem;
    mpdfx_lists_t _fxlists;

    Vector3f get_diff(const Vector3f& pos, float radius, float center_pressure, const BIT_FIELD bits);
    float get_pressure(const Vector3f& pos, float radius, const BIT_FIELD bits) const;
	/// @brief Remove extra ambient light in the lightmap.
    void remove_ambient();
	void recalc_twist();
    void finalize();
    void test_one_corner(GLXvector3f pos, float *pdelta);
    
    bool light_one_corner(ego_tile_info_t *ptile, const bool reflective, const Vector3f& pos, const Vector3f& nrm, float * plight);



    /// @brief Get the block index of the block at a given point (world coordinates).
    /// @param point the point (world coordinates)
    /// @return the block index of the block at the given point if there is a block at that point,
    ///         #INVALID_BLOCK otherwise
    BlockIndex get_block(const PointWorld& point) const;

    /// @brief Get the grid index of the grid at a given point (world coordinates).
    /// @param point the point (world coordinates)
    /// @return the grid index of the grid at the given point if there is a grid at that point,
    ///         #INVALID_TILE otherwise
    TileIndex get_grid(const PointWorld& point) const;

    /// @brief Get the block index of the block at a given point (block coordinates).
    /// @param point the point (block coordinates)
    /// @return the block index of the block at the given point if there is a block at that point,
    ///         #INVALID_BLOCK otherwise
    BlockIndex get_block_int(const PointBlock& point) const;

    /// @brief Get the tile index of the tile at a given point (grid coordinates).
    /// @param point the point (grid coordinates)
    /// @return the tile index of the tile at the given point if there is a tile at that point,
    ///         #INVALID_TILE otherwise
    TileIndex get_tile_int(const PointGrid& point) const;

    bool grid_is_valid(const TileIndex& id) const;

    /**
     * @brief
     *  Get the tile information for at a tile index in a mesh.
     * @param self
     *  the mesh
     * @param index
     *  the tile index
     * @return
     *  a pointer to the tile information of the tile at the index in this mesh
     *  if the tiles are allocated and the index is within bounds, @a nullptr otherwise.
     */
    ego_tile_info_t *get_ptile(const TileIndex& index) const;

    /**
     * @brief
     *  Get the grid information for at a tile index in a mesh.
     * @param self
     *  the mesh
     * @param index
     *  the tile index
     * @return
     *  a pointer to the grid information of the tile at the index in this mesh
     *  if the grids are allocated and the index is within bounds, @a nullptr otherwise.
     */
	const ego_grid_info_t *get_pgrid(const TileIndex& index) const;
	ego_grid_info_t *get_pgrid(const TileIndex& index);

    Uint32 test_fx(const TileIndex& index, const BIT_FIELD flags) const;

	bool clear_fx(const TileIndex& index, const BIT_FIELD flags);
	bool add_fx(const TileIndex& index, const BIT_FIELD flags);
	Uint8 get_twist(const TileIndex& index) const;

	/// @todo @a pos and @a radius should be passed as a sphere.
	BIT_FIELD hit_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits, Vector2f& nrm, float *pressure, mesh_wall_data_t *private_data) const;
	/// @todo @a pos and @a radius should be passed as a sphere.
	BIT_FIELD test_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits, mesh_wall_data_t *private_data) const;

	/**
	 * @brief
	 *  Get the precise height of the mesh at a given point (world coordinates).
	 * @param point
	 *	the point (world coordinates)
	 * @param waterwalk
	 *	if @a true and the fan is watery, then the height returned is the water level
	 * @return
	 *  the precise height of the mesh at the given point if there is a height at that point,
	 *  0 otherwise
	 */
	float getElevation(const PointWorld& point, bool waterwalk) const;
	/**
	 * @brief
	 *  Get the precise height of the mesh at a given point (world coordinates).
	 * @param point
	 *  the point (world coordinates)
	 * @return
	 *  the precise height of the mesh at the given point if there is a height at that point,
	 *  0 otherwise
	 **/
	float getElevation(const PointWorld& point) const;

	bool tile_has_bits(const PointGrid& point, const BIT_FIELD bits) const;

	void make_texture();
	static bool set_texture(ego_mesh_t *self, const TileIndex& tile, Uint16 image);
	static bool update_texture(ego_mesh_t *self, const TileIndex& tile);

};

float ego_mesh_get_max_vertex_0(const ego_mesh_t *self, const PointGrid& point);
float ego_mesh_get_max_vertex_1(const ego_mesh_t *self, const PointGrid& point, float xmin, float ymin, float xmax, float ymax);

//--------------------------------------------------------------------------------------------

extern Vector3f  map_twist_nrm[256];
extern FACING_T  map_twist_facing_y[256];              ///< For surface normal of mesh
extern FACING_T  map_twist_facing_x[256];
extern Vector3f  map_twist_vel[256];            ///< For sliding down steep hills
extern Uint8     map_twist_flat[256];

extern int mesh_mpdfx_tests;
extern int mesh_bound_tests;
extern int mesh_pressure_tests;

// variables to optimize calls to bind the textures
extern bool  mesh_tx_none;           ///< use blank textures?
extern TX_REF  mesh_tx_image;          ///< Last texture used
extern Uint8   mesh_tx_size;           ///< what size texture?

//--------------------------------------------------------------------------------------------

/// loading/saving
std::shared_ptr<ego_mesh_t> LoadMesh(const std::string& moduleName);

void   ego_mesh_make_twist();

bool ego_mesh_test_corners(ego_mesh_t *self, ego_tile_info_t *tile, float threshold);
float ego_mesh_light_corners(ego_mesh_t *self, ego_tile_info_t *tile, bool reflective, float mesh_lighting_keep);
bool ego_mesh_interpolate_vertex(tile_mem_t *self, ego_tile_info_t *tile, float pos[], float *plight);
bool grid_light_one_corner(const ego_mesh_t& self, const TileIndex& fan, float height, float nrm[], float *plight);

void mesh_texture_invalidate();
oglx_texture_t * mesh_texture_bind( const ego_tile_info_t * ptile );

Uint32 ego_mesh_has_some_mpdfx(const BIT_FIELD mpdfx, const BIT_FIELD test);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CARTMAN_SLOPE             50                        ///< increments for terrain slope

//--------------------------------------------------------------------------------------------
// Translated Cartman functions
//--------------------------------------------------------------------------------------------

Uint8 cartman_get_fan_twist(const ego_mesh_t *self, const TileIndex& tile);
