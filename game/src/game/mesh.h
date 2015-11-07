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
#include "egolib/Mesh/Info.hpp"

//--------------------------------------------------------------------------------------------
// external types
//--------------------------------------------------------------------------------------------

struct ego_mesh_info_t;
struct oglx_texture_t;

//--------------------------------------------------------------------------------------------

constexpr uint32_t GRIDS_X_MAX =
	MAP_TILE_MAX_X;
constexpr uint32_t GRIDS_Y_MAX =
	MAP_TILE_MAX_Y;
constexpr uint32_t GRIDS_MAX =
	GRIDS_X_MAX * GRIDS_Y_MAX;

#define VALID_GRID(PMPD, ID) ( (INVALID_TILE!=(ID)) && (NULL != (PMPD)) && (ID < (PMPD)->info.tiles_count) )

/// mesh physics
#define SLIDE                           0.04f         ///< Acceleration for steep hills
#define SLIDEFIX                        0.08f         ///< To make almost flat surfaces flat

//--------------------------------------------------------------------------------------------

typedef GLXvector3f normal_cache_t[4];
typedef std::array<float, 4> light_cache_t;

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
typedef Index<Uint32, IndexSystem::Tile,INVALID_BLOCK> TileIndex;

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

	struct Cache {
		/**
		 * @brief
		 *  Do Do the cache contents need an update?
		 */
		bool _needUpdate;
		/**
		 * @brief
		 *  The last frame in which the cache was updated.
		 * @remark
		 *  If negative, the cache contents are marked as "invalid".
		 */
		int _lastFrame;
		/**
		 * @brief
		 *  Construct this cache.
		 *  Its contents are marked as "invalid" and as "needing an update".
		 */
		Cache()
			: _lastFrame(-1), _needUpdate(true) {
		}
		/**
		 * @brief
		 *  Get if the cache contents need an update.
		 * @return
		 *  @a true if the cache contents an update, @a false otherwise
		 */
		bool getNeedUpdate() const { return _needUpdate; }
		/**
		 * @brief
		 *  Set if the cache contents need an update..
		 * @param needUpdate @a true if ache contents need an update, @a false otherwise
		 */
		void setNeedUpdate(bool needUpdate) { _needUpdate = needUpdate; }
		/**
		 * @brief
		 *  Set the last frame in which the cache was updated.
		 * @param lastFrame
		 *  the last frame in which the cache was updated.
		 *  If negative, the cache contents are marked as "invalid".
		 */
		void setLastFrame(int lastFrame) {
			_lastFrame = lastFrame;
		}
		/**
		 * @brief
		 *  Get the last frame in which the cache was updated.
		 * @return
		 *  the last frame in which the cache was updated.
		 *  If negative, the cache contents are marked as "invalid".
		 */
		int getLastFrame() const {
			return _lastFrame;
		}
		/**
		 * @brief
		 *  Get if the cache contents need an update.
		 * @param frame
		 *  the current frame
		 * @param framesSkip
		 *  the frame skip
		 * @return
		 * - If the cache contents are marked as "valid" and if
		 * - if the cache was updated at a frame frame such that
		 *   that frame + frameSkip >= thisFrame,
		 * then this method returns @a true, and @a false otherwise
		 */
		bool isValid(uint32_t thisFrame, uint32_t frameSkip = 0) {
			return (getLastFrame() >= 0 &&
				(uint32_t)getLastFrame() + frameSkip >= thisFrame);
		}
	};

    // tile corner lighting parameters
    normal_cache_t _ncache;                     ///< the normals at the corners of this tile
    
	struct LightingCache : Cache {
		LightingCache()
			: Cache(), _contents{ 0, 0, 0, 0 } {
		}
		light_cache_t _contents;

	};
	LightingCache _lightingCache;

	/// A cache for per-vertex lighting of a tile.
	struct VertexLightingCache : Cache {
		VertexLightingCache()
			: Cache(), _d1_cache{ 0,0,0,0 }, _d2_cache{ 0,0,0,0 } {
		}
		light_cache_t  _d1_cache; ///< the estimated change in the light at the corner of the tile
		light_cache_t  _d2_cache; ///< the estimated change in the light at the corner of the tile
	};
	/// The vertex lighting cache of this tile.
	VertexLightingCache _vertexLightingCache;

    // the bounding boc of this tile
    oct_bb_t       _oct;                        ///< the octagonal bounding box for this tile
	AABB2f         _aabb;

	/**
	 * @brief
	 *  Get if the tile has its fan rendering turned off.
	 * @return
	 *  @a true if the tile has its fan rendering turned off, @a false otherwise
	 */
	bool isFanOff() const {
		return MAP_FANOFF == _img;
	}
};

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

    float _edge_x; ///< Limits.
    float _edge_y;

    Uint32 *_tilestart;  ///< List of tiles  that start each row.

protected:
    // the per-grid info
    ego_grid_info_t* _grid_list;                       ///< tile command info
public:
	grid_mem_t(const Ego::MeshInfo& info);
    ~grid_mem_t();

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
	std::vector<ego_tile_info_t> _tileList;   ///< tile command info
	size_t _vertexCount;
	size_t _tileCountX;
	size_t _tileCountY;
	size_t _tileCount;
public:
    AABB3f _bbox;                 ///< bounding box for the entire mesh

	std::unique_ptr<GLXvector3f[]> _plst;                 ///< the position list
    std::unique_ptr<GLXvector2f[]> _tlst;                 ///< the texture coordinate list
    std::unique_ptr<GLXvector3f[]> _nlst;                 ///< the normal list
    std::unique_ptr<GLXvector3f[]> _clst;                 ///< the color list (for lighting the mesh)

	tile_mem_t(const Ego::MeshInfo& info);
	~tile_mem_t();

	/**
	 * @brief (Re)compute the vertex indices of the tiles infos.
	 * @param dict the tile dictionary to compute the vertex indices over 
	 */
	void computeVertexIndices(const tile_dictionary_t& dict);

	ego_tile_info_t& get(const TileIndex& index)
	{
		// Assert that the index is within bounds.
		if (TileIndex::Invalid == index) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		}
		return getTile(index.getI());
	}

    const ego_tile_info_t& get(const TileIndex& index) const
    {
        // Assert that the index is within bounds.
        if (TileIndex::Invalid == index) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
        }
        return getTile(index.getI());
    }

	ego_tile_info_t& getTile(const size_t x, const size_t y)
	{
		if (y >= _tileCountY) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		}
		if (x >= _tileCountX) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		}
		//Retrieve the tile and return it
		return _tileList[y * _tileCountX + x];
	}

	const ego_tile_info_t& getTile(const size_t x, const size_t y) const
	{
		if (y >= _tileCountY) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		}
		if (x >= _tileCountX) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		}

		//Retrieve the tile and return it
		return _tileList[y * _tileCountX + x];
	}

	ego_tile_info_t& getTile(const size_t index)
	{
		if (index >= _tileCount) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		}
		return _tileList[index];
	}

    const ego_tile_info_t& getTile(const size_t index) const
    {
		if (index >= _tileCount) {
			throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
		}
		return _tileList[index];
    }

	size_t getTileCountX() const { return _tileCountX; }
	size_t getTileCountY() const { return _tileCountY; }
    size_t getTileCount() const { return _tileCount;}
	size_t getVertexCount() const { return _vertexCount; }

    std::vector<ego_tile_info_t>& getAllTiles() { return _tileList; }

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

	mpdfx_lists_t(const Ego::MeshInfo& info);
	~mpdfx_lists_t();
    void reset();
    int push(GRID_FX_BITS fx_bits, size_t value);
    bool synch(const grid_mem_t& other, bool force);
};

//--------------------------------------------------------------------------------------------

// struct for caching fome values for wall collisions
/// MH: This seems to be used like an iterator.
struct mesh_wall_data_t
{
	int   ix_min, ix_max, iy_min, iy_max;
	float fx_min, fx_max, fy_min, fy_max;

	const Ego::MeshInfo *pinfo;
	const ego_grid_info_t *glist;
};


/// Egoboo's representation of the .mpd mesh file
class ego_mesh_t
{
public:
	/**
	 * @brief
	 *  Construct a mesh of the specified mesh info.
	 * @param info
	 *  the mesh info
	 */
    ego_mesh_t(const Ego::MeshInfo& info = Ego::MeshInfo());

    ~ego_mesh_t();

    Ego::MeshInfo _info;
    tile_mem_t _tmem;
    grid_mem_t _gmem;
    mpdfx_lists_t _fxlists;

    Vector3f get_diff(const Vector3f& pos, float radius, float center_pressure, const BIT_FIELD bits);
    float get_pressure(const Vector3f& pos, float radius, const BIT_FIELD bits) const;
	/// @brief Remove extra ambient light in the lightmap.
    void remove_ambient();
	void recalc_twist();
    void finalize();

    /// @brief Get the grid index of the grid at a given point (world coordinates).
    /// @param point the point (world coordinates)
    /// @return the grid index of the grid at the given point if there is a grid at that point,
    ///         #INVALID_TILE otherwise
    TileIndex getTileIndex(const PointWorld& point) const;

    /// @brief Get the tile index of the tile at a given point (grid coordinates).
    /// @param point the point (grid coordinates)
    /// @return the tile index of the tile at the given point if there is a tile at that point,
    ///         #INVALID_TILE otherwise
    TileIndex getTileIndex(const PointGrid& point) const;

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
	ego_tile_info_t& get_ptile(const TileIndex& index);
	const ego_tile_info_t& get_ptile(const TileIndex& index) const;

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
	bool set_texture(const TileIndex& tile, Uint16 image);
	bool update_texture(const TileIndex& tile);

	uint8_t get_fan_twist(const TileIndex& tile) const;
	float get_max_vertex_0(const PointGrid& point) const;
	float get_max_vertex_1(const PointGrid& point, float xmin, float ymin, float xmax, float ymax) const;

private:
	// mesh initialization - not accessible by scripts
	/// Calculate a set of normals for the 4 corner of a given tile.
	/// It is supposed to generate smooth normals for most tiles, but where there is a creas
	/// (i.e. between the floor and a wall) the normals should not be smoothed.
	// some twist/normal functions
	void make_normals();
	/// Set the bounding box for each tile, and for the entire mesh
	void make_bbox();

};

/// Some look-up tables for meshes (and independent of the particular mesh).
/// Contains precomputed surface normals and steep hill acceleration.
/// @todo This should be in map, not in mesh.
struct MeshLookupTables {
	Vector3f twist_nrm[256];
	/// For surface normal of the mesh.
	FACING_T twist_facing_y[256];
	/// For surface normal of the mesh.
	FACING_T twist_facing_x[256];
	/// Precomputed velocity (acceleration?) for sliding (down?) steep hills.
	Vector3f twist_vel[256];
	/// Is (something) flat?
	bool twist_flat[256];
	MeshLookupTables();
};

extern MeshLookupTables g_meshLookupTables;

//--------------------------------------------------------------------------------------------

/** Per-mesh test statistics. */
struct MeshStats {
	/** The number of MPD-FX tests performed. */
	int mpdfxTests;
	/** The number of bound tests performed. */
	int boundTests;
	/* The number of pressure tests performed. */
	int pressureTests;
	/** The mesh statistics. */
	MeshStats()
		: mpdfxTests(0), boundTests(0), pressureTests(0) {
	}
};

// Those are statistics. Move into per-mesh statistics.
extern MeshStats g_meshStats;

//--------------------------------------------------------------------------------------------

/// loading/saving
std::shared_ptr<ego_mesh_t> LoadMesh(const std::string& moduleName);

float ego_mesh_interpolate_vertex(const ego_tile_info_t& info, const GLXvector3f& position);

Uint32 ego_mesh_has_some_mpdfx(const BIT_FIELD mpdfx, const BIT_FIELD test);
