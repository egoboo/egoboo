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

#include "game/egoboo.h"
#include "game/lighting.h"
#include "egolib/Mesh/Info.hpp"

//--------------------------------------------------------------------------------------------
// external types
//--------------------------------------------------------------------------------------------

namespace Ego {
namespace OpenGL {
struct Texture;
} // namespace OpenGL
} // namespace Ego

//--------------------------------------------------------------------------------------------

constexpr uint32_t GRIDS_X_MAX =
	MAP_TILE_MAX_X;
constexpr uint32_t GRIDS_Y_MAX =
	MAP_TILE_MAX_Y;
constexpr uint32_t GRIDS_MAX =
	GRIDS_X_MAX * GRIDS_Y_MAX;

//--------------------------------------------------------------------------------------------

typedef GLXvector3f normal_cache_t[4];
typedef std::array<float, 4> light_cache_t;

//--------------------------------------------------------------------------------------------
typedef BIT_FIELD GRID_FX_BITS;
/// The data describing an Egoboo tile
class ego_tile_info_t
{
public:
    static const std::shared_ptr<ego_tile_info_t> NULL_TILE;

    ego_tile_info_t();
	~ego_tile_info_t() { _cache.init(); }

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

	/**
	 * @brief
	 *  Get if the tile has its fan rendering turned off.
	 * @return
	 *  @a true if the tile has its fan rendering turned off, @a false otherwise
	 */
	bool isFanOff() const {
		return MAP_FANOFF == _img;
	}

	GRID_FX_BITS testFX(const GRID_FX_BITS bits) const;

	/**
	* @brief
	*  Get the FX of this grid point.
	* @return
	*  the FX of this grid point
	*/
	GRID_FX_BITS getFX() const;

	/**
	* @brief
	*  Set the FX of this grid point.
	* @param fx
	*  the FX
	* @return
	*  @a true if the FX of this grid point changed,
	*  @a false otherwise
	*/
	bool setFX(const GRID_FX_BITS bits);

public:
	/**
	* @brief
	*  Add FX to this grid point.
	* @param fx
	*  the FX
	* @return
	*  @a true if the FX of this grid point changed,
	*  @a false otherwise
	*/
	bool addFX(const GRID_FX_BITS bits);
	/**
	* @brief
	*  Remove FX from this grid point.
	* @param fx
	*  the FX
	* @return
	*  @a true if the FX of this grid point changed,
	*  @a false otherwise
	*/
	bool removeFX(const GRID_FX_BITS bits);

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

	/// The vertex lighting cache of this tile.
	VertexLightingCache _vertexLightingCache;

    // the bounding boc of this tile
    oct_bb_t       _oct;                        ///< the octagonal bounding box for this tile

	/**
	* @brief
	*  The special effect flags in the MPD.
	* @warning
	*  Do not modify.
	*/
	GRID_FX_BITS _base_fx;

	/**
	* @brief
	*  The orientation of the file in the MPD.
	* @warning
	*  Do not modify.
	*/
	uint8_t _twist;

	// 
	GRID_FX_BITS    _pass_fx;                   ///< the working copy of base_fx, which might be modified by passages
												// the lighting info in the upper left hand corner of a grid
	uint8_t            _a, _l;                 ///< the raw mesh lighting... pretty much ignored
	lighting_cache_t _cache;                   ///< the per-grid lighting info
	int              _cache_frame;             ///< the last frame in which the cache was calculated

};

inline bool TILE_HAS_INVALID_IMAGE(const ego_tile_info_t& tileInfo) {
	return HAS_SOME_BITS(TILE_UPPER_MASK, tileInfo._img);
}

//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated mesh memory
struct tile_mem_t
{
private:
	std::vector<ego_tile_info_t> _tileList;   ///< tile command info
	Ego::MeshInfo _info;
public:
	float _edge_x; ///< Limits.
	float _edge_y;
    AxisAlignedBox3f _bbox;                 ///< bounding box for the entire mesh

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

public:
	ego_tile_info_t& get(const Index1D& i) {
        _info.assertValid(i);
		return _tileList[i.getI()];
	}
    const ego_tile_info_t& get(const Index1D& i) const {
        _info.assertValid(i);
		return _tileList[i.getI()];
    }

public:
	ego_tile_info_t& get(const Index2D& i) {
        _info.assertValid(i);
		return _tileList[_info.map(i).getI()];
	}
	const ego_tile_info_t& get(const Index2D& i) const {
        _info.assertValid(i);
		return _tileList[_info.map(i).getI()];
	}

public:
	const Ego::MeshInfo& getInfo() const {
		return _info;
	}

    std::vector<ego_tile_info_t>& getAllTiles() { return _tileList; }

};

//--------------------------------------------------------------------------------------------

struct mpdfx_list_ary_t
{
    std::vector<Index1D> elements;
	mpdfx_list_ary_t();
	~mpdfx_list_ary_t();
    void clear();
    void push_back(const Index1D& element);
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
    bool synch(const tile_mem_t& other, bool force);
};

//--------------------------------------------------------------------------------------------

class ego_mesh_t;

/// struct for caching fome values for wall collisions
/// MH: This seems to be used like an iterator.
struct mesh_wall_data_t {
	AxisAlignedBox2f _f;
	IndexRect _i;
	const ego_mesh_t *_mesh;
	mesh_wall_data_t(const ego_mesh_t *mesh,
		             const AxisAlignedBox2f& f,
		             const IndexRect& i);
	mesh_wall_data_t(const ego_mesh_t *mesh, const Circle2f& circle);
private:
    // Get \f$cl\left(\left\langle c, r' \right\rangle\right)\$ where
    // \f$\left\langle c, r \right\rangle\f$ is the specified
    // circle and \f$r' = \max\left(r,\frac{1}{2}g\right)\f$
    // and \f$g\f$ is the grid size.
    // @todo This can be replaced by
    // creating the circles \f$\langle c, r \rangle> \cup
    // \langle c, \frac{1}{2}g \rangle\f$.
    static AxisAlignedBox2f leastClosure(const Circle2f& circle) {
        auto circle0 = Circle2f(circle.getCenter(),
                                std::max(circle.getRadius(), Info<float>::Grid::Size() * 0.5f));
        static const Ego::Math::Closure<AxisAlignedBox2f, Circle2f> closure{};
        return closure(circle0);
    }
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
    Index1D getTileIndex(const Vector2f& point) const;

    /// @brief Get the tile index of the tile at a given point (grid coordinates).
    /// @param point the point (grid coordinates)
    /// @return the tile index of the tile at the given point if there is a tile at that point,
    ///         #INVALID_TILE otherwise
    Index1D getTileIndex(const Index2D& i) const;

    bool grid_is_valid(const Index1D& i) const;

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
	ego_tile_info_t& getTileInfo(const Index1D& i);
	const ego_tile_info_t& getTileInfo(const Index1D& i) const;

    Uint32 test_fx(const Index1D& i, const BIT_FIELD flags) const;

	bool clear_fx(const Index1D& i, const BIT_FIELD flags);
	bool add_fx(const Index1D& i, const BIT_FIELD flags);
	Uint8 get_twist(const Index1D& i) const;

	/// @todo @a pos and @a radius should be passed as a sphere.
	BIT_FIELD hit_wall(const Vector3f& pos, float radius, const BIT_FIELD bits, Vector2f& nrm, float *pressure, const mesh_wall_data_t& data) const;
	BIT_FIELD hit_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits, Vector2f& nrm, float *pressure) const;
	/// @todo @a pos and @a radius should be passed as a sphere.
	BIT_FIELD test_wall(const BIT_FIELD bits, const mesh_wall_data_t& data) const;
	BIT_FIELD test_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits) const;

	/**
	 * @brief
	 *  Get the precise height of the mesh at a given point (world coordinates).
	 * @param p
	 *	the point (world coordinates)
	 * @param waterwalk
	 *	if @a true and the fan is watery, then the height returned is the water level
	 * @return
	 *  the precise height of the mesh at the given point if there is a height at that point,
	 *  0 otherwise
	 */
	float getElevation(const Vector2f& p, bool waterwalk) const;
	/**
	 * @brief
	 *  Get the precise height of the mesh at a given point (world coordinates).
	 * @param p
	 *  the point (world coordinates)
	 * @return
	 *  the precise height of the mesh at the given point if there is a height at that point,
	 *  0 otherwise
	 **/
	float getElevation(const Vector2f& p) const;

	bool tile_has_bits(const Index2D& i, const BIT_FIELD bits) const;

	void make_texture();
	bool set_texture(const Index1D& i, Uint16 image);
	bool update_texture(const Index1D& i);

	uint8_t get_fan_twist(const Index1D& i) const;
	float get_max_vertex_0(const Index2D& i) const;
	float get_max_vertex_1(const Index2D& i, float xmin, float ymin, float xmax, float ymax) const;

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
	Facing twist_facing_y[256];
	/// For surface normal of the mesh.
	Facing twist_facing_x[256];
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
struct MeshLoader {
    std::shared_ptr<ego_mesh_t> operator()(const std::string& moduleName) const;
private:
    std::shared_ptr<ego_mesh_t> convert(const map_t& source) const;
};
