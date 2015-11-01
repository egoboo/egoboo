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

/// @file game/mesh.c
/// @brief Functions for creating, reading, and writing Egoboo's .mpd mesh file
/// @details

#include "game/mesh.h"
#include "game/lighting.h"
#include "game/physics.h"
#include "game/Physics/PhysicalConstants.hpp"
#include "game/graphic.h"
#include "egolib/FileFormats/Globals.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


static std::shared_ptr<ego_mesh_t> ego_mesh_convert(map_t& source);

static float grid_get_mix( float u0, float u, float v0, float v );

//Static class variables
const std::shared_ptr<ego_tile_info_t> ego_tile_info_t::NULL_TILE = nullptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

MeshStats g_meshStats;

static void warnNumberOfVertices(const char *file, int line, size_t numberOfVertices)
{
	std::ostringstream os;
	os << "mesh has too many vertices - " << numberOfVertices << " requested, "
	   << "but maximum is " << MAP_VERTICES_MAX;
	Log::warning("%s:%d: %s\n", __FILE__, __LINE__, os.str().c_str());
}

//--------------------------------------------------------------------------------------------

tile_mem_t::tile_mem_t(const Ego::MeshInfo& info)
	: _tileList(info.getTileCount()),
	  _tileCount(info.getTileCount()), _bbox(), _tileCountX(info.getTileCountX()),
	  _tileCountY(info.getTileCountY()), _vertexCount(info.getVertexCount()) {
	_plst = std::make_unique<GLXvector3f[]>(info.getVertexCount());
	_tlst = std::make_unique<GLXvector2f[]>(info.getVertexCount());
	_clst = std::make_unique<GLXvector3f[]>(info.getVertexCount());
	_nlst = std::make_unique<GLXvector3f[]>(info.getVertexCount());
}

tile_mem_t::~tile_mem_t() {
	// Free the vertex data.
	if (_plst)
	{
		_plst = nullptr;
	}
	if (_nlst)
	{
		_nlst = nullptr;
	}
	if (_clst)
	{
		_clst = nullptr;
	}
	if (_tlst)
	{
		_tlst = nullptr;
	}

	// Free the tile data.
	_tileList.clear();
}

//--------------------------------------------------------------------------------------------

ego_mesh_t::ego_mesh_t(const Ego::MeshInfo& mesh_info)
	: _info(mesh_info), _tmem(mesh_info), _gmem(mesh_info), _fxlists(mesh_info) {
}

ego_mesh_t::~ego_mesh_t() {
}


void ego_mesh_t::remove_ambient()
{
   
    Uint8 min_vrt_a = 255;

    /// @todo Use iterator.
    for (Uint32 i = 0; i < _info.getTileCount(); ++i)
    {
        min_vrt_a = std::min(min_vrt_a, _gmem.get(TileIndex(i))->_a);
    }

    /// @todo Use iterator.
    for (Uint32 i = 0; i < _info.getTileCount(); ++i)
    {
        _gmem.get(TileIndex(i))->_a = 
			_gmem.get(TileIndex(i))->_a - min_vrt_a;
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::recalc_twist()
{
    // recalculate the twist
    for (TileIndex fan = 0; fan.getI() < _info.getTileCount(); fan++)
    {
        uint8_t twist = get_fan_twist(fan);
        _gmem.get(fan)->_twist = twist;
    }
}

bool ego_mesh_t::set_texture(const TileIndex& index, Uint16 image)
{
	if (!grid_is_valid(index)) {
		return false;
	}

    // Get the upper and lower bits for this tile image.
	uint16_t tile_value = _tmem.getTile(index.getI())._img;
	uint16_t tile_lower = image & TILE_LOWER_MASK;
	uint16_t tile_upper = tile_value & TILE_UPPER_MASK;

    // Set the actual image.
    _tmem.getTile(index.getI())._img = tile_upper | tile_lower;

    // Update the pre-computed texture info.
    return update_texture(index);
}

bool ego_mesh_t::update_texture(const TileIndex& index)
{
	if (!grid_is_valid(index)) {
		return false;
	}
	const ego_tile_info_t& ptile = _tmem.getTile(index.getI());

    uint8_t type  = ptile._type & 0x3F;

	tile_definition_t *pdef = TILE_DICT_PTR( tile_dict, type );
    if ( NULL == pdef ) return false;

    size_t mesh_vrt = ptile._vrtstart;
    for (uint16_t tile_vrt = 0; tile_vrt < pdef->numvertices; tile_vrt++, mesh_vrt++ )
    {
        _tmem._tlst[mesh_vrt][SS] = pdef->u[tile_vrt];
        _tmem._tlst[mesh_vrt][TT] = pdef->v[tile_vrt];
    }

    return true;
}

void ego_mesh_t::make_texture()
{
    // Set the texture coordinate for every vertex.
    for (TileIndex index = 0; index < _info.getTileCount(); ++index)
    {
        update_texture(index);
    }
}

void ego_mesh_t::finalize()
{
    make_vrtstart();
	remove_ambient();
	recalc_twist();
    make_normals();
    make_bbox();
	make_texture();

    // create some lists to make searching the mesh tiles easier
	_fxlists.synch(_gmem, true );
}

//--------------------------------------------------------------------------------------------
std::shared_ptr<ego_mesh_t> ego_mesh_convert(map_t& source)
{
    // clear out all data in the destination mesh
    auto target = std::make_shared<ego_mesh_t>(Ego::MeshInfo(source._info.getVertexCount(), source._info.getTileCountX(), source._info.getTileCountY()));
	tile_mem_t& tmem_dst = target->_tmem;
	grid_mem_t& gmem_dst = target->_gmem;
	Ego::MeshInfo& info_dst = target->_info;

    // copy all the per-tile info
    for (size_t cnt = 0; cnt < info_dst.getTileCount(); cnt++)
    {
        tile_info_t& ptile_src = source._mem.tiles[cnt];
        ego_tile_info_t& ptile_dst = tmem_dst.getTile(cnt);
        ego_grid_info_t *pgrid_dst = gmem_dst.get(cnt);

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        ptile_dst._type = ptile_src.type;
        ptile_dst._img  = ptile_src.img;

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        pgrid_dst->_base_fx = ptile_src.fx;
        pgrid_dst->_twist   = ptile_src.twist;

        // set the local fx flags
        pgrid_dst->_pass_fx = pgrid_dst->_base_fx;

        // lcache is set in the constructor
        // nlst is set in the constructor
    }

    // copy all the per-vertex info
    for (size_t cnt = 0; cnt < source._info.getVertexCount(); cnt++ )
    {
		GLXvector3f& ppos_dst = tmem_dst._plst[cnt];
        GLXvector3f& pcol_dst = tmem_dst._clst[cnt];
        const map_vertex_t& pvrt_src = source._mem.vertices[cnt];

        // copy all info from map_mem_t
        ppos_dst[XX] = pvrt_src.pos[kX];
        ppos_dst[YY] = pvrt_src.pos[kY];
        ppos_dst[ZZ] = pvrt_src.pos[kZ];

        // default color
        pcol_dst[RR] = pcol_dst[GG] = pcol_dst[BB] = 0.0f;

        // tlist is set below
    }

    // copy some of the pre-calculated grid lighting
    for (Uint32 cnt = 0; cnt < info_dst.getTileCount(); cnt++ )
    {
        size_t vertex = tmem_dst.get(cnt)._vrtstart;
        ego_grid_info_t *pgrid_dst = gmem_dst.get(cnt);
        const map_vertex_t& pvrt_src = source._mem.vertices[vertex];

        pgrid_dst->_a = pvrt_src.a;
        pgrid_dst->_l = 0.0f;
    }

	return target;
}

//--------------------------------------------------------------------------------------------
std::shared_ptr<ego_mesh_t> LoadMesh(const std::string& moduleName)
{
	map_t local_mpd;
	// Load the map data.
	tile_dictionary_load_vfs("mp_data/fans.txt", &tile_dict, -1);
	if (!local_mpd.load("mp_data/level.mpd"))
	{
		std::ostringstream os;
		os << "unable to load mesh of module `" << moduleName << "`";
		Log::error("%s\n", os.str().c_str());
		throw Id::RuntimeErrorException(__FILE__, __LINE__, os.str());
	}
	// Create the mesh from map.
	std::shared_ptr<ego_mesh_t> mesh = ego_mesh_convert(local_mpd);
	if (!mesh)
	{
		std::ostringstream os;
		os << "unable to convert mesh of module `" << moduleName << "`";
		Log::error("%s\n", os.str().c_str());
		throw Id::RuntimeErrorException(__FILE__, __LINE__, os.str());
	}
	mesh->finalize();
	return mesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

grid_mem_t::grid_mem_t(const Ego::MeshInfo& info) {
	// Free any memory already allocated.
	if (info.getVertexCount() > MAP_VERTICES_MAX) {
		warnNumberOfVertices(__FILE__, __LINE__, info.getVertexCount());
	}

	// Set the desired block number of grids.
	_grids_x = info.getTileCountX();
	_grids_y = info.getTileCountY();
	_grid_count = info.getTileCount();

	// Set the mesh edge info.
	_edge_x = (_grids_x + 1) * Info<int>::Grid::Size();
	_edge_y = (_grids_y + 1) * Info<int>::Grid::Size();

	// Set the desired blocknumber of blocks.
	// This only works if BLOCK_BITS = GRID_BITS + 2.
	_blocks_x = (info.getTileCountX() >> 2);
	if (HAS_SOME_BITS(info.getTileCountX(), 0x03))
	{
		_blocks_x++;
	}
	_blocks_y = (info.getTileCountY() >> 2);
	if (HAS_SOME_BITS(info.getTileCountY(), 0x03))
	{
		_blocks_y++;
	}
	_blocks_count = _blocks_x * _blocks_y;

	// Allocate per-grid memory.
	_grid_list = new ego_grid_info_t[info.getTileCount()]();
	// Allocate the array for the block start data.
	_blockstart = new Uint32[_blocks_y];

	// Allocate the array for the tile start data.
	_tilestart = new Uint32[info.getTileCountY()];
	// Compute the tile start/block start data.
	// Compute look-up table for tile starts.
	for (int i = 0; i < info.getTileCountY(); i++)
	{
		_tilestart[i] = info.getTileCountX() * i;
	}

	// Calculate some of the block info
	if (_blocks_x >= GRID_BLOCKY_MAX)
	{
		Log::warning("%s:%d: number of mesh blocks in the x direction too large (%d out of %d).\n", __FILE__, __LINE__, \
			         _blocks_x, GRID_BLOCKY_MAX);
	}

	if (_blocks_y >= GRID_BLOCKY_MAX)
	{
		Log::warning("%s:%d: number of mesh blocks in the y direction too large (%d out of %d).\n", __FILE__, __LINE__, \
			         _blocks_y, GRID_BLOCKY_MAX);
	}

	// Compute look-up table for block starts.
	for (int i = 0; i < _blocks_y; i++)
	{
		_blockstart[i] = _blocks_x * i;
	}
}

grid_mem_t::~grid_mem_t() {
	// Free the block start and tile start arrays.
	if (_blockstart)
	{
		delete[] _blockstart;
		_blockstart = nullptr;
	}
	if (_tilestart)
	{
		delete[] _tilestart;
		_tilestart = nullptr;
	}

	// Destroy the grid list.
	if (_grid_list) {
		delete[] _grid_list;
		_grid_list = nullptr;
	}


	_grids_y = 0;
	_grids_x = 0;
	_grid_count = 0;

	_blocks_y = 0;
	_blocks_x = 0;
	_blocks_count = 0;

	_edge_y = 0.0f;
	_edge_x = 0.0f;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ego_mesh_t::make_vrtstart()
{
    size_t vert = 0;
    for (uint32_t tile = 0; tile < _info.getTileCount(); tile++ )
    {
        _tmem.get(tile)._vrtstart = vert;

        uint8_t ttype = _tmem.get(tile)._type;

        // throw away any remaining upper bits
        ttype &= 0x3F;

		tile_definition_t *pdef = TILE_DICT_PTR( tile_dict, ttype );
        if ( NULL == pdef ) continue;

        vert += pdef->numvertices;
    }

    if ( vert != _info.getVertexCount() )
    {
		Log::warning( "%s:%d: unexpected number of vertices %" PRIuZ " of %" PRIuZ "\n", __FILE__, __LINE__, vert, _info.getVertexCount() );
    }
}

//--------------------------------------------------------------------------------------------
MeshLookupTables g_meshLookupTables;

MeshLookupTables::MeshLookupTables() {
	Vector3f grav = Vector3f::zero();

	grav[kZ] = Ego::Physics::g_environment.gravity;

	for (size_t cnt = 0; cnt < 256; cnt++)
	{
		Vector3f nrm;

		twist_to_normal(cnt, nrm, 1.0f);

		twist_nrm[cnt] = nrm;

		twist_facing_x[cnt] = (FACING_T)(-vec_to_facing(nrm[kZ], nrm[kY]));
		twist_facing_y[cnt] = vec_to_facing(nrm[kZ], nrm[kX]);

		// this is about 5 degrees off of vertical
		twist_flat[cnt] = false;
		if (nrm[kZ] > 0.9945f)
		{
			twist_flat[cnt] = true;
		}

		// projection of the gravity parallel to the surface
		float gdot = grav[kZ] * nrm[kZ];

		// Gravity perpendicular to the mesh.
		Vector3f gperp = nrm * gdot;

		// Gravity parallel to the mesh.
		Vector3f gpara = grav - gperp;

		twist_vel[cnt] = gpara;
	}
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::make_bbox()
{
    _tmem._bbox = AABB3f(Vector3f(_tmem._plst[0][XX], _tmem._plst[0][YY], _tmem._plst[0][ZZ]),
		                 Vector3f(_tmem._plst[0][XX], _tmem._plst[0][YY], _tmem._plst[0][ZZ]));

	for (TileIndex cnt = 0; cnt.getI() < _info.getTileCount(); cnt++)
	{
        ego_tile_info_t& ptile = _tmem.getTile(cnt.getI());
        oct_bb_t& poct = ptile._oct;


        ptile._itile = cnt.getI();
		uint8_t type = ptile._type & 0x3F;

		int tile_vrt;
		oct_vec_v2_t ovec;

		tile_definition_t *pdef = TILE_DICT_PTR(tile_dict, type);
		if (NULL == pdef) continue;

		size_t mesh_vrt = _tmem.get(cnt)._vrtstart;    // Number of vertices
		uint8_t vertices = pdef->numvertices;           // Number of vertices

		// initialize the bounding box
	    ovec = oct_vec_v2_t(Vector3f(_tmem._plst[mesh_vrt][0], _tmem._plst[mesh_vrt][1],_tmem._plst[mesh_vrt][2]));
        poct = oct_bb_t(ovec);
        mesh_vrt++;

        ptile._aabb._min = Vector2f(Info<float>::Grid::Size() * (ptile._itile % _info.getTileCountX()), Info<float>::Grid::Size() * (ptile._itile / _info.getTileCountY()));
        ptile._aabb._max = Vector2f(ptile._aabb._min[OCT_X] + Info<float>::Grid::Size(), ptile._aabb._min[OCT_Y] + Info<float>::Grid::Size());

        // add the rest of the points into the bounding box
        for ( tile_vrt = 1; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
        {
            ovec.ctor(Vector3f(_tmem._plst[mesh_vrt][0],_tmem._plst[mesh_vrt][1],_tmem._plst[mesh_vrt][2]));
            poct.join(ovec);
        }

		/// @todo: This test is not up-2-date anymore.
		///        If you join a bbox with an ovec, the box is never(!) empty.
		///        However, it still might be desirable not to have boxes with
		///        width, height and depth of one. Should be evaluated asap.
        // ensure that NO tile has zero volume.
        // if a tile is declared to have all the same height, it will accidentally be called "empty".
        if (poct._empty || (std::abs(poct._maxs[OCT_X] - poct._mins[OCT_X]) +
                            std::abs(poct._maxs[OCT_Y] - poct._mins[OCT_Y]) +
                            std::abs(poct._maxs[OCT_Z] - poct._mins[OCT_Z])) < std::numeric_limits<float>::epsilon())
        {
            ovec[OCT_X] = ovec[OCT_Y] = ovec[OCT_Z] = 0.1;
            ovec[OCT_XY] = ovec[OCT_YX] = Ego::Math::sqrtTwo<float>() * ovec[OCT_X];
            oct_bb_self_grow(poct, ovec);
        }

        // extend the mesh bounding box
        _tmem._bbox = AABB3f(Vector3f(std::min(_tmem._bbox.getMin()[XX], poct._mins[XX]),
                                      std::min(_tmem._bbox.getMin()[YY], poct._mins[YY]),
                                      std::min(_tmem._bbox.getMin()[ZZ], poct._mins[ZZ])),
                             Vector3f(std::max(_tmem._bbox.getMax()[XX], poct._maxs[XX]),
                                      std::max(_tmem._bbox.getMax()[YY], poct._maxs[YY]),
                                      std::max(_tmem._bbox.getMax()[ZZ], poct._maxs[ZZ])));
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::make_normals()
{
    // test for mesh

    // set the default normal for each fan, based on the calculated twist value
    for (TileIndex fan0 = 0; fan0 < _tmem.getTileCount(); fan0++ )
    {
        uint8_t twist = _gmem.get(fan0)->_twist;

        _tmem._nlst[fan0.getI()][XX] = g_meshLookupTables.twist_nrm[twist][kX];
        _tmem._nlst[fan0.getI()][YY] = g_meshLookupTables.twist_nrm[twist][kY];
        _tmem._nlst[fan0.getI()][ZZ] = g_meshLookupTables.twist_nrm[twist][kZ];
    }

	int      edge_is_crease[4];
	Vector3f nrm_lst[4], vec_sum;
	float    weight_lst[4];

    // find an "average" normal of each corner of the tile
    for (size_t iy = 0; iy < _info.getTileCountY(); iy++ )
    {
        for (size_t ix = 0; ix < _info.getTileCountX(); ix++ )
        {
            int ix_off[4] = {0, 1, 1, 0};
            int iy_off[4] = {0, 0, 1, 1};
            int i, j, k;

            TileIndex fan0 = get_tile_int(PointGrid(ix, iy));
			if (!grid_is_valid(fan0)) {
				continue;
			}

            nrm_lst[0][kX] = _tmem._nlst[fan0.getI()][XX];
            nrm_lst[0][kY] = _tmem._nlst[fan0.getI()][YY];
            nrm_lst[0][kZ] = _tmem._nlst[fan0.getI()][ZZ];

            // for each corner of this tile
            for ( i = 0; i < 4; i++ )
            {
                int dx, dy;
                int loc_ix_off[4];
                int loc_iy_off[4];

                // the offset list needs to be shifted depending on what i is
                j = ( 6 - i ) % 4;

                if ( 1 == ix_off[(4-j)%4] ) dx = -1; else dx = 0;
                if ( 1 == iy_off[(4-j)%4] ) dy = -1; else dy = 0;

                for ( k = 0; k < 4; k++ )
                {
                    loc_ix_off[k] = ix_off[( 4-j + k ) % 4 ] + dx;
                    loc_iy_off[k] = iy_off[( 4-j + k ) % 4 ] + dy;
                }

                // cache the normals
                // nrm_lst[0] is already known.
                for ( j = 1; j < 4; j++ )
                {
                    int jx, jy;

                    jx = ix + loc_ix_off[j];
                    jy = iy + loc_iy_off[j];

                    TileIndex fan1 = get_tile_int(PointGrid(jx, jy));

                    if ( grid_is_valid( fan1 ) )
                    {
                        nrm_lst[j][kX] = _tmem._nlst[fan1.getI()][XX];
                        nrm_lst[j][kY] = _tmem._nlst[fan1.getI()][YY];
                        nrm_lst[j][kZ] = _tmem._nlst[fan1.getI()][ZZ];

                        if ( nrm_lst[j][kZ] < 0 )
                        {
                            nrm_lst[j] = -nrm_lst[j];
                        }
                    }
                    else
                    {
                        nrm_lst[j] = Vector3f(0, 0, 1);
                    }
                }

                // find the creases
                for ( j = 0; j < 4; j++ )
                {
                    float vdot;
                    int m = ( j + 1 ) % 4;

                    vdot = nrm_lst[j].dot(nrm_lst[m]);

                    edge_is_crease[j] = (vdot < Ego::Math::invSqrtTwo<float>());

                    weight_lst[j] = nrm_lst[j].dot(nrm_lst[0]);
                }

                weight_lst[0] = 1.0f;
                if ( edge_is_crease[0] )
                {
                    // this means that there is a crease between tile 0 and 1
                    weight_lst[1] = 0.0f;
                }

                if ( edge_is_crease[3] )
                {
                    // this means that there is a crease between tile 0 and 3
                    weight_lst[3] = 0.0f;
                }

                if ( edge_is_crease[0] && edge_is_crease[3] )
                {
                    // this means that there is a crease between tile 0 and 1
                    // and a crease between tile 0 and 3, isolating tile 2
                    weight_lst[2] = 0.0f;
                }

                vec_sum = nrm_lst[0];
                for ( j = 1; j < 4; j++ )
                {
                    if ( weight_lst[j] > 0.0f )
                    {
                        vec_sum[kX] += nrm_lst[j][kX] * weight_lst[j];
                        vec_sum[kY] += nrm_lst[j][kY] * weight_lst[j];
                        vec_sum[kZ] += nrm_lst[j][kZ] * weight_lst[j];
                    }
                }

				vec_sum.normalize();

                _tmem.get(fan0)._ncache[i][XX] = vec_sum[kX];
                _tmem.get(fan0)._ncache[i][YY] = vec_sum[kY];
                _tmem.get(fan0)._ncache[i][ZZ] = vec_sum[kZ];
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_interpolate_vertex( tile_mem_t * pmem, ego_tile_info_t * ptile, float pos[], float * plight )
{
    int cnt;
    int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    float u, v, wt, weight_sum;
    float loc_light;

    oct_bb_t        * poct;
    light_cache_t   * lc;

    // check the parameters
    if ( NULL == pmem || NULL == ptile ) return false;

    // handle optional parameters
    if ( NULL == plight ) plight = &loc_light;

    ( *plight ) = 0.0f;

    // alias some variables
    poct = &( ptile->_oct );
    lc   = &( ptile->_lcache );

    // determine a u,v coordinate for the vertex
    u = ( pos[XX] - poct->_mins[OCT_X] ) / ( poct->_maxs[OCT_X] - poct->_mins[OCT_X] );
    v = ( pos[YY] - poct->_mins[OCT_Y] ) / ( poct->_maxs[OCT_Y] - poct->_mins[OCT_Y] );

    // average the cached data on the 4 corners of the mesh
    weight_sum = 0.0f;
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        wt = grid_get_mix( ix_off[cnt], u, iy_off[cnt], v );

        weight_sum += wt;
        ( *plight )  += wt * ( *lc )[cnt];
    }

    if (( *plight ) > 0.0f && weight_sum > 0.0f )
    {
        ( *plight ) /= weight_sum;
        ( *plight ) = CLIP(( *plight ), 0.0f, 255.0f );
    }
    else
    {
        ( *plight ) = 0.0f;
    }

    return true;
}

#define BLAH_MIX_1(DU,UU) (4.0f/9.0f*((UU)-(-1+(DU)))*((2+(DU))-(UU)))
#define BLAH_MIX_2(DU,UU,DV,VV) (BLAH_MIX_1(DU,UU)*BLAH_MIX_1(DV,VV))

float grid_get_mix( float u0, float u, float v0, float v )
{
    float wt_u, wt_v;
    float du = u - u0;
    float dv = v - v0;

    // du *= 1.0f;
    if ( std::abs( du ) > 1.0f ) return 0.0f;
    wt_u = ( 1.0f - du ) * ( 1.0f + du );

    // dv *= 1.0f;
    if ( std::abs( dv ) > 1.0f ) return 0.0f;
    wt_v = ( 1.0f - dv ) * ( 1.0f + dv );

    return wt_u * wt_v;
}

BIT_FIELD ego_mesh_t::test_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits, mesh_wall_data_t *pdata) const
{
    /// @author BB
    /// @details an abstraction of the functions of chr_hit_wall() and prt_hit_wall()

    mesh_wall_data_t loc_data;

    BIT_FIELD pass;
    int ix, iy;
    float loc_radius;

    // deal with the optional parameters
    if ( NULL == pdata ) pdata = &loc_data;

    // if there is no interaction with the mesh, return 0
    if ( EMPTY_BIT_FIELD == bits ) return EMPTY_BIT_FIELD;

    // if the mesh is empty, return 0
    if ( 0 == _info.getTileCount() || _tmem.getTileCount() == 0 ) return EMPTY_BIT_FIELD;
    pdata->pinfo = (Ego::MeshInfo *)&(_info);
    pdata->glist = _gmem.get(0);

    // make an alias for the radius
    loc_radius = radius;

    // set a minimum radius
    if ( 0.0f == loc_radius )
    {
        loc_radius = Info<float>::Grid::Size() * 0.5f;
    }

    // make sure it is positive
    loc_radius = std::abs( loc_radius );

    pdata->fx_min = pos[kX] - loc_radius;
    pdata->fx_max = pos[kX] + loc_radius;

    pdata->fy_min = pos[kY] - loc_radius;
    pdata->fy_max = pos[kY] + loc_radius;

    // make a large limit in case the pos is so large that it cannot be represented by an int
    pdata->fx_min = std::max( pdata->fx_min, -9.0f * _gmem._edge_x );
    pdata->fx_max = std::min( pdata->fx_max, 10.0f * _gmem._edge_x );

    pdata->fy_min = std::max( pdata->fy_min, -9.0f * _gmem._edge_y );
    pdata->fy_max = std::min( pdata->fy_max, 10.0f * _gmem._edge_y );

    // find an integer bound.
    // we need to know about out of range values below clamp these to valid values
	ego_irect_t bound;
    bound.xmin = std::floor( pdata->fx_min / Info<float>::Grid::Size());
    bound.xmax = std::floor( pdata->fx_max / Info<float>::Grid::Size());
    bound.ymin = std::floor( pdata->fy_min / Info<float>::Grid::Size());
    bound.ymax = std::floor( pdata->fy_max / Info<float>::Grid::Size());

    // limit the test values to be in-bounds
    pdata->fx_min = std::max( pdata->fx_min, 0.0f );
    pdata->fx_max = std::min( pdata->fx_max, _gmem._edge_x );
    pdata->fy_min = std::max( pdata->fy_min, 0.0f );
    pdata->fy_max = std::min( pdata->fy_max, _gmem._edge_y );

    pdata->ix_min = std::max( bound.xmin, 0 );
    pdata->ix_max = std::min( bound.xmax, int(_info.getTileCountX()) - 1 );
    pdata->iy_min = std::max( bound.ymin, 0 );
    pdata->iy_max = std::min( bound.ymax, int(_info.getTileCountY()) - 1 );

    // clear the bit accumulator
    pass = 0;

    // detect out of bounds in the y-direction
    if ( bound.ymin < 0 || bound.ymax >= pdata->pinfo->getTileCountY() )
    {
        pass = ( MAPFX_IMPASS | MAPFX_WALL ) & bits;
		g_meshStats.boundTests++;
    }
    if ( EMPTY_BIT_FIELD != pass ) return pass;

    // detect out of bounds in the x-direction
    if ( bound.xmin < 0 || bound.xmax >= pdata->pinfo->getTileCountX() )
    {
        pass = ( MAPFX_IMPASS | MAPFX_WALL ) & bits;
		g_meshStats.boundTests++;
    }
    if ( EMPTY_BIT_FIELD != pass ) return pass;

    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        // since we KNOW that this is in range, allow raw access to the data strucutre
        int irow = _gmem._tilestart[iy];

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            int itile = ix + irow;

            // since we KNOW that this is in range, allow raw access to the data strucutre
            pass = ego_grid_info_t::test_all_fx( &(pdata->glist[itile]), bits );
            if ( 0 != pass )
            {
                return pass;
            }

			g_meshStats.mpdfxTests++;
        }
    }

    return pass;
}

float ego_mesh_t::get_pressure(const Vector3f& pos, float radius, const BIT_FIELD bits) const
{
    const float tile_area = Info<float>::Grid::Size() * Info<float>::Grid::Size();


    // deal with the optional parameters
    float loc_pressure = 0.0f;

    if (0 == bits) return 0;

    if ( 0 == _info.getTileCount() || _tmem.getTileCount() == 0 ) return 0;
	const ego_grid_info_t *glist = _gmem.get(0);

    // make an alias for the radius
    float loc_radius = radius;

    // set a minimum radius
    if ( 0.0f == loc_radius )
    {
        loc_radius = Info<float>::Grid::Size() * 0.5f;
    }

    // make sure it is positive
    loc_radius = std::abs( loc_radius );

    float fx_min = pos[kX] - loc_radius;
    float fx_max = pos[kX] + loc_radius;

    float fy_min = pos[kY] - loc_radius;
    float fy_max = pos[kY] + loc_radius;

    float obj_area = ( fx_max - fx_min ) * ( fy_max - fy_min );

    int ix_min = std::floor( fx_min / Info<float>::Grid::Size());
    int ix_max = std::floor( fx_max / Info<float>::Grid::Size());

    int iy_min = std::floor( fy_min / Info<float>::Grid::Size());
    int iy_max = std::floor( fy_max / Info<float>::Grid::Size());

    for ( int iy = iy_min; iy <= iy_max; iy++ )
    {
        bool tile_valid = true;

        float ty_min = ( iy + 0 ) * Info<float>::Grid::Size();
        float ty_max = ( iy + 1 ) * Info<float>::Grid::Size();

        if ( iy < 0 || iy >= _info.getTileCountY() )
        {
            tile_valid = false;
        }

        for ( int ix = ix_min; ix <= ix_max; ix++ )
        {
            bool is_blocked = false;
            float tx_min, tx_max;

            float area_ratio;
            float ovl_x_min, ovl_x_max;
            float ovl_y_min, ovl_y_max;

            tx_min = ( ix + 0 ) * Info<float>::Grid::Size();
            tx_max = ( ix + 1 ) * Info<float>::Grid::Size();

            if ( ix < 0 || ix >= _info.getTileCountX() )
            {
                tile_valid = false;
            }

            if ( tile_valid )
            {
                TileIndex itile = get_tile_int(PointGrid(ix, iy));
                tile_valid = grid_is_valid( itile );
                if ( !tile_valid )
                {
                    is_blocked = true;
                }
                else
                {
                    is_blocked = ( 0 != ego_grid_info_t::test_all_fx( &(glist[itile.getI()]), bits ) );
                }
            }

            if ( !tile_valid )
            {
                is_blocked = true;
            }

            if ( is_blocked )
            {
                // hiting the mesh
                float min_area;

                // determine the area overlap of the tile with the
                // object's bounding box
                ovl_x_min = std::max( fx_min, tx_min );
                ovl_x_max = std::min( fx_max, tx_max );

                ovl_y_min = std::max( fy_min, ty_min );
                ovl_y_max = std::min( fy_max, ty_max );

                min_area = std::min( tile_area, obj_area );

                area_ratio = 0.0f;
                if ( ovl_x_min <= ovl_x_max && ovl_y_min <= ovl_y_max )
                {
                    if ( 0.0f == min_area )
                    {
                        area_ratio = 1.0f;
                    }
                    else
                    {
                        area_ratio  = ( ovl_x_max - ovl_x_min ) * ( ovl_y_max - ovl_y_min ) / min_area;
                    }
                }

                loc_pressure += area_ratio;

                g_meshStats.pressureTests++;
            }
        }
    }

    return loc_pressure;
}

//--------------------------------------------------------------------------------------------
Vector3f ego_mesh_t::get_diff(const Vector3f& pos, float radius, float center_pressure, const BIT_FIELD bits )
{
    /// @author BB
    /// @details determine the shortest "way out", but creating an array of "pressures"
    /// with each element representing the pressure when the object is moved in different directions
    /// by 1/2 a tile.

    const float jitter_size = Info<float>::Grid::Size() * 0.5f;
    float pressure_ary[9];
    float fx, fy;
    Vector3f diff = Vector3f::zero();
    float   sum_diff = 0.0f;
    float   dpressure;

    int cnt;

    // Find the pressure for the 9 points of jittering around the current position.
    pressure_ary[4] = center_pressure;
    for ( cnt = 0, fy = pos[kY] - jitter_size; fy <= pos[kY] + jitter_size; fy += jitter_size )
    {
        for ( fx = pos[kX] - jitter_size; fx <= pos[kX] + jitter_size; fx += jitter_size, cnt++ )
        {
            Vector3f jitter_pos(fx,fy,0.0f);
            if (4 == cnt) continue;
            pressure_ary[cnt] = get_pressure(jitter_pos, radius, bits);
        }
    }

    // Determine the "minimum number of tiles to move" to get into a clear area.
    diff[kX] = diff[kY] = 0.0f;
    sum_diff = 0.0f;
    for ( cnt = 0, fy = -0.5f; fy <= 0.5f; fy += 0.5f )
    {
        for ( fx = -0.5f; fx <= 0.5f; fx += 0.5f, cnt++ )
        {
            if (4 == cnt) continue;

            dpressure = ( pressure_ary[cnt] - center_pressure );

            // Find the maximal pressure gradient == the minimal distance to move.
            if ( 0.0f != dpressure )
            {
                float   dist = pressure_ary[4] / dpressure;

				Vector2f tmp(dist * fx, dist * fy);

                float weight = 1.0f / dist;

                diff[XX] += tmp[YY] * weight;
                diff[YY] += tmp[XX] * weight;
                sum_diff += std::abs(weight);
            }
        }
    }
    // normalize the displacement by dividing by the weight...
    // unnecessary if the following normalization is kept in
    //if( sum_diff > 0.0f )
    //{
    //    diff[kX] /= sum_diff;
    //    diff[kY] /= sum_diff;
    //}

    // Limit the maximum displacement to less than one tile.
    if (std::abs(diff[kX]) + std::abs(diff[kY]) > 0.0f)
    {
        float fmax = std::max(std::abs(diff[kX]), std::abs(diff[kY]));

        diff[kX] /= fmax;
        diff[kY] /= fmax;
    }

    return diff;
}

BIT_FIELD ego_mesh_t::hit_wall( const Vector3f& pos, const float radius, const BIT_FIELD bits, Vector2f& nrm, float * pressure, mesh_wall_data_t * pdata ) const
{
    /// @author BB
    /// @details an abstraction of the functions of chr_hit_wall() and prt_hit_wall()

    BIT_FIELD loc_pass;
    Uint32 pass;
    int ix, iy;
    bool invalid;

    float  loc_pressure;

    bool needs_pressure = ( NULL != pressure );
    bool needs_nrm = true;

    mesh_wall_data_t loc_data;

    // deal with the optional parameters
    if ( NULL == pressure ) pressure = &loc_pressure;
    *pressure = 0.0f;

    nrm = Vector2f::zero();

    // if pdata is not NULL, someone has already run a version of mesh_test_wall
    if ( NULL == pdata )
    {
        pdata = &loc_data;

        // Do the simplest test.
        // Initializes the shared mesh_wall_data_t struct, so no need to do it again
        // Eliminates all cases of bad source data, so no need to test them again.
        if ( 0 == test_wall( pos, radius, bits, pdata ) ) return 0;
    }

    // ego_mesh_test_wall() clamps pdata->ix_* and pdata->iy_* to valid values

    pass = loc_pass = 0;
    nrm[kX] = nrm[kY] = 0.0f;
    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        float ty_min, ty_max;

        invalid = false;

        ty_min = ( iy + 0 ) * Info<float>::Grid::Size();
        ty_max = ( iy + 1 ) * Info<float>::Grid::Size();

        if ( iy < 0 || iy >= pdata->pinfo->getTileCountY() )
        {
            loc_pass |= ( MAPFX_IMPASS | MAPFX_WALL );

            if ( needs_nrm )
            {
                nrm[kY] += pos[kY] - ( ty_max + ty_min ) * 0.5f;
            }

            invalid = true;
			g_meshStats.boundTests++;
        }

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            float tx_min, tx_max;

            tx_min = ( ix + 0 ) * Info<float>::Grid::Size();
            tx_max = ( ix + 1 ) * Info<float>::Grid::Size();

            if ( ix < 0 || ix >= pdata->pinfo->getTileCountX() )
            {
                loc_pass |=  MAPFX_IMPASS | MAPFX_WALL;

                if ( needs_nrm )
                {
                    nrm[kX] += pos[kX] - ( tx_max + tx_min ) * 0.5f;
                }

                invalid = true;
				g_meshStats.boundTests++;
            }

            if ( !invalid )
            {
                TileIndex itile = get_tile_int(PointGrid(ix, iy));
                if ( grid_is_valid( itile ) )
                {
                    BIT_FIELD mpdfx   = ego_grid_info_t::get_all_fx( pdata->glist + itile.getI() );
                    bool is_blocked = HAS_SOME_BITS( mpdfx, bits );

                    if ( is_blocked )
                    {
                        SET_BIT( loc_pass,  mpdfx );

                        if ( needs_nrm )
                        {
                            nrm[kX] += pos[kX] - ( tx_max + tx_min ) * 0.5f;
                            nrm[kY] += pos[kY] - ( ty_max + ty_min ) * 0.5f;
                        }
                    }
                }
            }
        }
    }

    pass = loc_pass & bits;

    if ( 0 == pass )
    {
        // if there is no impact at all, there is no normal and no pressure
        nrm = Vector2f::zero();
        *pressure = 0.0f;
    }
    else
    {
        if ( needs_nrm )
        {
            // special cases happen a lot. try to avoid computing the square root
            if ( 0.0f == nrm[kX] && 0.0f == nrm[kY] )
            {
                // no normal does not mean no net pressure,
                // just that all the simplistic normal calculations balance
            }
            else if ( 0.0f == nrm[kX] )
            {
                nrm[kY] = SGN( nrm[kY] );
            }
            else if ( 0.0f == nrm[kY] )
            {
                nrm[kX] = SGN( nrm[kX] );
            }
            else
            {
                nrm.normalize();
            }
        }

        if ( needs_pressure )
        {
            *pressure = get_pressure( pos, radius, bits );
        }
    }

    return pass;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float ego_mesh_t::get_max_vertex_0(const PointGrid& point) const
{
    Uint32 cnt;
    float zmax;
    size_t vcount, vstart, ivrt;

    TileIndex itile = get_tile_int(point);
    if (TileIndex::Invalid == itile)
    {
        return 0.0f;
    }
    // get a pointer to the tile
    const ego_tile_info_t& ptile = _tmem.getTile(itile.getI());

    vstart = ptile._vrtstart;
    vcount = std::min(static_cast<size_t>(4), _tmem.getVertexCount());

    ivrt = vstart;
    zmax = _tmem._plst[ivrt][ZZ];
    for ( ivrt++, cnt = 1; cnt < vcount; ivrt++, cnt++ )
    {
        zmax = std::max( zmax, _tmem._plst[ivrt][ZZ] );
    }

    return zmax;
}

float ego_mesh_t::get_max_vertex_1( const PointGrid& point, float xmin, float ymin, float xmax, float ymax ) const
{
    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    TileIndex itile = get_tile_int( point );

    if (TileIndex::Invalid == itile) return 0.0f;

    size_t vstart = _tmem.get(itile)._vrtstart;
    size_t vcount = std::min( (size_t)4, _tmem.getVertexCount() );

    float zmax = -1e6;
    for (size_t ivrt = vstart, cnt = 0; cnt < vcount; ivrt++, cnt++ )
    {
        GLXvector3f& vert = _tmem._plst[ivrt];

        // we are evaluating the height based on the grid, not the actual vertex positions
        float fx = ( point.getX() + ix_off[cnt] ) * Info<float>::Grid::Size();
        float fy = ( point.getY() + iy_off[cnt] ) * Info<float>::Grid::Size();

        if ( fx >= xmin && fx <= xmax && fy >= ymin && fy <= ymax )
        {
            zmax = std::max( zmax, vert[ZZ] );
        }
    }

    if ( -1e6 == zmax ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
// ego_grid_info_t
//--------------------------------------------------------------------------------------------
ego_grid_info_t::ego_grid_info_t()
	: _base_fx(0), _pass_fx(0), _a(0), _l(0), _cache_frame(-1), _twist(TWIST_FLAT)
{ }

ego_grid_info_t::~ego_grid_info_t() {
	lighting_cache_t::init(_cache);
}

//--------------------------------------------------------------------------------------------

mpdfx_list_ary_t::mpdfx_list_ary_t()
	: _cnt(0), _lst(nullptr), _idx(0) {
}

mpdfx_list_ary_t::~mpdfx_list_ary_t() {
    dealloc();

	_lst = nullptr;
	_idx = 0;
	_cnt = 0;
}

void mpdfx_list_ary_t::alloc(size_t size)
{
	dealloc();

    if (0 == size)
    {
        return;
    }
    _lst = new size_t[size];
    _cnt = size;
    _idx = 0;
}

void mpdfx_list_ary_t::dealloc()
{
    if (0 == _cnt)
    {
        return;
    }
	delete[] _lst;
	_lst = nullptr;
    _cnt = 0;
    _idx = 0;
}

void mpdfx_list_ary_t::reset()
{
    _idx = 0;
}

bool mpdfx_list_ary_t::push(size_t value)
{
    if (_idx < _cnt)
    {
        _lst[_idx] = value;
        _idx++;
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------

mpdfx_lists_t::mpdfx_lists_t(const Ego::MeshInfo& info) {
	sha.alloc(info.getTileCount());
	drf.alloc(info.getTileCount());
	anm.alloc(info.getTileCount());
	wat.alloc(info.getTileCount());
	wal.alloc(info.getTileCount());
	imp.alloc(info.getTileCount());
	dam.alloc(info.getTileCount());
	slp.alloc(info.getTileCount());

	// the list needs to be resynched
	dirty = true;
}

mpdfx_lists_t::~mpdfx_lists_t() {
	// free the memory
	sha.dealloc();
	drf.dealloc();
	anm.dealloc();
	wat.dealloc();
	wal.dealloc();
	imp.dealloc();
	dam.dealloc();
	slp.dealloc();

	// No memory, hence nothing is stored, hence nothing is dirty.
	dirty = false;
}

void mpdfx_lists_t::reset()
{
    // free the memory
    sha.reset();
    drf.reset();
    anm.reset();
    wat.reset();
    wal.reset();
    imp.reset();
    dam.reset();
    slp.reset();

    // everything has been reset. force it to recalculate
	dirty = true;
}

int mpdfx_lists_t::push( GRID_FX_BITS fx_bits, size_t value )
{
    int retval = 0;

    if ( 0 == fx_bits ) return true;

    if ( HAS_NO_BITS( fx_bits, MAPFX_SHA ) )
    {
        if ( sha.push(value) )
        {
            retval++;
        }

    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_REFLECTIVE ) )
    {
        if ( drf.push(value) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_ANIM ) )
    {
        if ( anm.push(value) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WATER ) )
    {
        if ( wat.push(value) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WALL ) )
    {
        if ( wal.push(value) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_IMPASS ) )
    {
        if ( imp.push(value) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_DAMAGE ) )
    {
        if ( dam.push(value) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_SLIPPY ) )
    {
        if ( slp.push(value) )
        {
            retval++;
        }
    }

    return retval;
}

bool mpdfx_lists_t::synch( const grid_mem_t& gmem, bool force )
{
    if ( 0 == gmem._grid_count ) return true;

    // don't re-calculate unless it is necessary
    if ( !force && !dirty ) return true;

    // !!reset the counts!!
	reset();

    for (size_t i = 0; i < gmem._grid_count; i++ )
    {
		GRID_FX_BITS fx = ego_grid_info_t::get_all_fx(gmem.get(i));

        push(fx, i);
    }

    // we're done calculating
	dirty = false;

    return true;
}


//--------------------------------------------------------------------------------------------
//Previously inlined
//--------------------------------------------------------------------------------------------
bool ego_mesh_t::tile_has_bits( const PointGrid& point, const BIT_FIELD bits ) const
{
    // Figure out which tile we are on.
    TileIndex tileRef = get_tile_int(point);

    // Everything outside the map bounds is wall and impassable.
    if (!grid_is_valid(tileRef))
    {
        return HAS_SOME_BITS((MAPFX_IMPASS | MAPFX_WALL), bits);
    }

    // Since we KNOW that this is in range, allow raw access to the data structure.
    GRID_FX_BITS fx = ego_grid_info_t::get_all_fx(_gmem.get(tileRef));

    return HAS_SOME_BITS(fx, bits);
}

Uint32 ego_mesh_has_some_mpdfx( const BIT_FIELD mpdfx, const BIT_FIELD test )
{
	g_meshStats.mpdfxTests++;
    return HAS_SOME_BITS( mpdfx, test );
}

bool ego_mesh_t::grid_is_valid(const TileIndex& index) const
{
	g_meshStats.boundTests++;

    if (TileIndex::Invalid == index)
    {
        return false;
    }
    return index.getI() < _info.getTileCount();
}

//--------------------------------------------------------------------------------------------
float ego_mesh_t::getElevation(const PointWorld& point) const
{
    TileIndex tile = this->get_grid(point);
	if (!grid_is_valid(tile)) {
		return 0;
	}
	PointGrid gridPoint(static_cast<int>(point.getX()) & Info<int>::Grid::Mask(),
                        static_cast<int>(point.getY()) & Info<int>::Grid::Mask());

    // Get the height of each fan corner.
    float z0 = _tmem._plst[_tmem.get(tile)._vrtstart + 0][ZZ];
    float z1 = _tmem._plst[_tmem.get(tile)._vrtstart + 1][ZZ];
    float z2 = _tmem._plst[_tmem.get(tile)._vrtstart + 2][ZZ];
    float z3 = _tmem._plst[_tmem.get(tile)._vrtstart + 3][ZZ];

    // Get the weighted height of each side.
    float zleft = (z0 * (Info<float>::Grid::Size() - gridPoint.getY()) + z3 * gridPoint.getY()) / Info<float>::Grid::Size();
    float zright = (z1 * (Info<float>::Grid::Size() - gridPoint.getY()) + z2 * gridPoint.getY()) / Info<float>::Grid::Size();
    float zdone = (zleft * (Info<float>::Grid::Size() - gridPoint.getX()) + zright * gridPoint.getX()) / Info<float>::Grid::Size();

    return zdone;
}

//--------------------------------------------------------------------------------------------
BlockIndex ego_mesh_t::get_block(const PointWorld& point) const
{
	if (point.getX() >= 0.0f && point.getX() <= _gmem._edge_x && point.getY() >= 0.0f && point.getY() <= _gmem._edge_y)
    {
        PointBlock blockPoint(static_cast<int>(point.getX()) / Info<int>::Block::Size(),
                              static_cast<int>(point.getY()) / Info<int>::Block::Size());
		return this->get_block_int(blockPoint);
    }

    return BlockIndex::Invalid;
}

TileIndex ego_mesh_t::get_grid(const PointWorld& point) const
{
    if (point.getX() >= 0.0f && point.getX() < this->_gmem._edge_x && point.getY() >= 0.0f && point.getY() < this->_gmem._edge_y)
    {
        // By the above, point.getX() and point.getY() are positive, hence the right shift is not a problem.
        // point.these are known to be positive, so >> is not a problem
        PointGrid gridPoint(static_cast<int>(point.getX()) >> Info<int>::Grid::Bits(), 
                            static_cast<int>(point.getY()) >> Info<int>::Grid::Bits());
        return this->get_tile_int(gridPoint);
    }
    return TileIndex::Invalid;
}

BlockIndex ego_mesh_t::get_block_int(const PointBlock& point) const
{
    if (point.getX() < 0 || point.getX() >= this->_gmem._blocks_x)
    {
        return BlockIndex::Invalid;
    }
    if (point.getY() < 0 || point.getY() >= this->_gmem._blocks_y)
    {
        return BlockIndex::Invalid;
    }
    return point.getX() + this->_gmem._blockstart[point.getY()];
}

TileIndex ego_mesh_t::get_tile_int(const PointGrid& point) const
{
    if (point.getX() < 0 || point.getX() >= this->_info.getTileCountX())
    {
        return TileIndex::Invalid;
    }
	if (point.getY() < 0 || point.getY() >= this->_info.getTileCountY())
    {
        return TileIndex::Invalid;
    }
	return point.getX() + this->_gmem._tilestart[point.getY()];
}

bool ego_mesh_t::clear_fx( const TileIndex& itile, const BIT_FIELD flags )
{
    bool retval;

    // test for invalid tile
	g_meshStats.boundTests++;
    if ( itile > _info.getTileCount() ) return false;

	g_meshStats.mpdfxTests++;
    retval = ego_grid_info_t::sub_pass_fx(_gmem.get(itile), flags );

    if ( retval )
    {
        _fxlists.dirty = true;
    }

    return retval;
}

bool ego_mesh_t::add_fx(const TileIndex& index, const BIT_FIELD flags)
{
    // Validate tile index.
	g_meshStats.boundTests++;
    if (index > _info.getTileCount())
    {
        return false;
    }

    // Succeed only of something actually changed.
	g_meshStats.mpdfxTests++;
    bool retval = ego_grid_info_t::add_pass_fx(_gmem.get(index), flags);

    if ( retval )
    {
        _fxlists.dirty = true;
    }

    return retval;
}

Uint32 ego_mesh_t::test_fx(const TileIndex& index, const BIT_FIELD flags) const
{
    // test for a trivial value of flags
    if (EMPTY_BIT_FIELD == flags) return 0;

    // test for invalid tile
	g_meshStats.boundTests++;
    if (index > _info.getTileCount())
    {
        return flags & ( MAPFX_WALL | MAPFX_IMPASS );
    }

    // if the tile is actually labelled as MAP_FANOFF, ignore it completely
    if (_tmem.get(index).isFanOff())
    {
        return 0;
    }

	g_meshStats.mpdfxTests++;
    return ego_grid_info_t::test_all_fx(_gmem.get(index), flags);
}

ego_tile_info_t& ego_mesh_t::get_ptile(const TileIndex& index)
{
	// Validate mesh and tile index.
	if (index.getI() >= _info.getTileCount())
	{
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
	}

	// Get the tile info.
	return _tmem.getTile(index.getI());
}

const ego_tile_info_t& ego_mesh_t::get_ptile(const TileIndex& index) const
{
    // Validate mesh and tile index.
    if (index.getI() >= _info.getTileCount())
    {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
    }

    // Get the tile info.
    return _tmem.getTile(index.getI());
}

ego_grid_info_t *ego_mesh_t::get_pgrid(const TileIndex& index)
{
	// Validate mesh and grid index.
	if (index.getI() >= _info.getTileCount())
	{
		return nullptr;
	}

	// Get the grid info.
	return _gmem.get(index);
}

const ego_grid_info_t *ego_mesh_t::get_pgrid(const TileIndex& index) const
{
    // Validate mesh and grid index.
    if (index.getI() >= _info.getTileCount())
    {
        return nullptr;
    }

    // Get the grid info.
    return _gmem.get(index);
}

Uint8 ego_mesh_t::get_twist(const TileIndex& index) const
{
    // Validate arguments.
    if (index >= _info.getTileCount())
    {
        return TWIST_FLAT;
    }
    return _gmem.get(index)->_twist;
}

//--------------------------------------------------------------------------------------------

GRID_FX_BITS ego_grid_info_t::get_all_fx(const ego_grid_info_t *self)
{
    if (!self) return MAPFX_WALL | MAPFX_IMPASS;

    return self->_pass_fx;
}

GRID_FX_BITS ego_grid_info_t::test_all_fx(const ego_grid_info_t *self, const GRID_FX_BITS bits)
{
    GRID_FX_BITS grid_bits;

    if (!self)
    {
        grid_bits = MAPFX_WALL | MAPFX_IMPASS;
    }
    else
    {
        grid_bits = ego_grid_info_t::get_all_fx(self);
    }

    return grid_bits & bits;
}

bool ego_grid_info_t::add_pass_fx(ego_grid_info_t *self, const GRID_FX_BITS bits)
{
    if (!self) return false;

    // save the old bits
	GRID_FX_BITS old_bits = ego_grid_info_t::get_all_fx(self);

    // set the bits that we can modify
    SET_BIT(self->_pass_fx, bits);

    // get the new bits
	GRID_FX_BITS new_bits = ego_grid_info_t::get_all_fx(self);

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

bool ego_grid_info_t::sub_pass_fx(ego_grid_info_t *self, const GRID_FX_BITS bits)
{
    if ( NULL == self) return false;

    // save the old bits
	GRID_FX_BITS old_bits = ego_grid_info_t::get_all_fx(self);

    // set the bits that we can modify
    UNSET_BIT(self->_pass_fx, bits );

    // get the new bits
	GRID_FX_BITS new_bits = ego_grid_info_t::get_all_fx(self);

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

bool ego_grid_info_t::set_pass_fx(ego_grid_info_t *self, const GRID_FX_BITS bits)
{
    if ( NULL == self) return false;

    // save the old bits
	GRID_FX_BITS old_bits = ego_grid_info_t::get_all_fx(self);

    // set the bits that we can modify
	self->_pass_fx = bits;

    // get the new bits
	GRID_FX_BITS new_bits = ego_grid_info_t::get_all_fx(self);

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

uint8_t ego_mesh_t::get_fan_twist(const TileIndex& tile) const
{
    // check for a valid tile
    if (TileIndex::Invalid == tile || tile > _info.getTileCount())
    {
        return TWIST_FLAT;
    }
    const ego_tile_info_t& info = _tmem.getTile(tile.getI());
    // if the tile is actually labelled as MAP_FANOFF, ignore it completely
	if (info.isFanOff())
    {
        return TWIST_FLAT;
    }
    size_t vrtstart = info._vrtstart;

    float z0 = _tmem._plst[vrtstart + 0][ZZ];
    float z1 = _tmem._plst[vrtstart + 1][ZZ];
    float z2 = _tmem._plst[vrtstart + 2][ZZ];
    float z3 = _tmem._plst[vrtstart + 3][ZZ];

    float zx = CARTMAN_FIXNUM * (z0 + z3 - z1 - z2) / CARTMAN_SLOPE;
    float zy = CARTMAN_FIXNUM * (z2 + z3 - z0 - z1) / CARTMAN_SLOPE;

    return cartman_calc_twist(zx, zy);
}
