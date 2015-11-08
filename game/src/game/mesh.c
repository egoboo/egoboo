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
}

void tile_mem_t::computeVertexIndices(const tile_dictionary_t& dict)
{
	size_t vertexIndex = 0;
	for (size_t i = 0; i < getTileCount(); ++i) {
		get(i)._vrtstart = vertexIndex;

		uint8_t type = get(i)._type;

		// Throw away any remaining upper bits.
		type &= 0x3F;

		const tile_definition_t *def = dict.get(type);
		if (!def) continue;

		vertexIndex += def->numvertices;
	}

	if (vertexIndex != getVertexCount()) {
		Log::warning("%s:%d: unexpected number of vertices %" PRIuZ " of %" PRIuZ "\n", __FILE__, __LINE__, vertexIndex, getVertexCount());
	}
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
        min_vrt_a = std::min(min_vrt_a, _gmem.get(TileIndex(i))._a);
    }

    /// @todo Use iterator.
    for (Uint32 i = 0; i < _info.getTileCount(); ++i)
    {
        _gmem.get(TileIndex(i))._a = 
			_gmem.get(TileIndex(i))._a - min_vrt_a;
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::recalc_twist()
{
    // recalculate the twist
    for (TileIndex fan = 0; fan.getI() < _info.getTileCount(); fan++)
    {
        uint8_t twist = get_fan_twist(fan);
        _gmem.get(fan)._twist = twist;
    }
}

bool ego_mesh_t::set_texture(const TileIndex& index, Uint16 image)
{
	if (!grid_is_valid(index)) {
		return false;
	}

    // Get the upper and lower bits for this tile image.
	uint16_t tile_value = _tmem.get(index)._img;
	uint16_t tile_lower = image & TILE_LOWER_MASK;
	uint16_t tile_upper = tile_value & TILE_UPPER_MASK;

    // Set the actual image.
    _tmem.get(index)._img = tile_upper | tile_lower;

    // Update the pre-computed texture info.
    return update_texture(index);
}

bool ego_mesh_t::update_texture(const TileIndex& index)
{
	if (!grid_is_valid(index)) {
		return false;
	}
	const ego_tile_info_t& ptile = _tmem.get(index);

    uint8_t type  = ptile._type & 0x3F;

	tile_definition_t *pdef = tile_dict.get( type );
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
	// (1) (Re)compute the vertex indices.
	_tmem.computeVertexIndices(tile_dict);
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
    for (TileIndex cnt(0); cnt < info_dst.getTileCount(); cnt++)
    {
        tile_info_t& ptile_src = source._mem.tiles[cnt.getI()];
        ego_tile_info_t& ptile_dst = tmem_dst.get(cnt);
        ego_grid_info_t& pgrid_dst = gmem_dst.get(cnt);

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        ptile_dst._type = ptile_src.type;
        ptile_dst._img  = ptile_src.img;

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        pgrid_dst._base_fx = ptile_src.fx;
        pgrid_dst._twist   = ptile_src.twist;

        // set the local fx flags
        pgrid_dst._pass_fx = pgrid_dst._base_fx;

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
        ego_grid_info_t& pgrid_dst = gmem_dst.get(cnt);
        const map_vertex_t& pvrt_src = source._mem.vertices[vertex];

        pgrid_dst._a = pvrt_src.a;
        pgrid_dst._l = 0.0f;
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

	// Allocate per-grid memory.
	_grid_list = new ego_grid_info_t[info.getTileCount()]();

	// Allocate the array for the tile start data.
	_tilestart = new Uint32[info.getTileCountY()];
	// Compute the tile start/block start data.
	// Compute look-up table for tile starts.
	for (int i = 0; i < info.getTileCountY(); i++)
	{
		_tilestart[i] = info.getTileCountX() * i;
	}
}

grid_mem_t::~grid_mem_t() {
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

	_edge_y = 0.0f;
	_edge_x = 0.0f;
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
        ego_tile_info_t& ptile = _tmem.get(cnt);
        oct_bb_t& poct = ptile._oct;


        ptile._itile = cnt.getI();
		uint8_t type = ptile._type & 0x3F;

		int tile_vrt;
		oct_vec_v2_t ovec;

		tile_definition_t *pdef = tile_dict.get(type);
		if (NULL == pdef) continue;

		size_t mesh_vrt = _tmem.get(cnt)._vrtstart;    // Number of vertices
		uint8_t vertices = pdef->numvertices;           // Number of vertices

		// initialize the bounding box
	    ovec = oct_vec_v2_t(Vector3f(_tmem._plst[mesh_vrt][0], _tmem._plst[mesh_vrt][1],_tmem._plst[mesh_vrt][2]));
        poct = oct_bb_t(ovec);
        mesh_vrt++;

        ptile._aabb._min = Vector2f(ptile._itile % _info.getTileCountX(), 
			                        ptile._itile % _info.getTileCountY()) * Info<float>::Grid::Size();
        ptile._aabb._max = Vector2f(ptile._aabb._min[OCT_X] + Info<float>::Grid::Size(), 
			                        ptile._aabb._min[OCT_Y] + Info<float>::Grid::Size());

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
        uint8_t twist = _gmem.get(fan0)._twist;

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

            TileIndex fan0 = getTileIndex(PointGrid(ix, iy));
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
                    int jx = ix + loc_ix_off[j];
                    int jy = iy + loc_iy_off[j];

                    TileIndex fan1 = getTileIndex(PointGrid(jx, jy));

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
float ego_mesh_interpolate_vertex(const ego_tile_info_t& info, const GLXvector3f& position)
{
	const oct_bb_t& boundingBox = info._oct;
	const light_cache_t& lightCache = info._lightingCache._contents;

	// Set the lighting to 0.
	float light = 0.0f;

    // Determine texture coordinates of the specified point.
    float u = (position[XX] - boundingBox._mins[OCT_X]) / (boundingBox._maxs[OCT_X] - boundingBox._mins[OCT_X]);
    float v = (position[YY] - boundingBox._mins[OCT_Y]) / (boundingBox._maxs[OCT_Y] - boundingBox._mins[OCT_Y]);

    // Interpolate the lighting at the four vertices of the tile.
	// to determine the final lighting at the specified point.
    float weightedSum = 0.0f;
    for (size_t i = 0; i < 4; ++i) {
		// Mix the u, v coordinate pairs (0,0),
		// (1,0), (1,1), and (0,1) using the
		// texture coordinates of the specified
		// point.
		static const float ix_off[4] = { 0.0f, 1.0f, 1.0f, 0.0f },
			               iy_off[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        float mix = grid_get_mix(ix_off[i], u, iy_off[i], v);
		
		weightedSum += mix;
        light += mix * lightCache[i];
    }

	// Normalize to the weighted sum.
    if (light > 0.0f && weightedSum > 0.0f) {
        light /= weightedSum;
        light = Ego::Math::constrain(light, 0.0f, 255.0f);
    } else {
        light = 0.0f;
    }
	return light;
}

#define BLAH_MIX_1(DU,UU) (4.0f/9.0f*((UU)-(-1+(DU)))*((2+(DU))-(UU)))
#define BLAH_MIX_2(DU,UU,DV,VV) (BLAH_MIX_1(DU,UU)*BLAH_MIX_1(DV,VV))

float grid_get_mix(float u0, float u, float v0, float v) {
	// Get the distance of u and v from u0 and v0.
	float du = u - u0, 
		  dv = v - v0;

	// If the absolute distance du or dv is greater than 1,
	// return 0.
	if (std::abs(du) > 1.0f || std::abs(dv) > 1.0f) {
		return 0.0f;
	}
	// The distances are within the bounds of [-1,+1] at this point.
	// The original formulas are
	// wt_u = (1.0f - du)*(1.0f + du)
	// wt_v = (1.0f - dv)*(1.0f + dv)
	// However, a term of the form
	// y = (1 - x) * (1 + x)
	// can be simplified to
	// y = (1 - x) * 1 + (1 - x) * x
	//   = (1 - x) + (1 - x) * x
	//   = 1 - x + x - x^2
	//   = 1 - x^2
	// Hence the original formulas become
	// wt_u = 1.0f - du * du)
	// wt_v = 1.0f - dv * dv
	float wt_u = 1.0f - du * du,
		  wt_v = 1.0f - dv * dv;

	return wt_u * wt_v;
}

BIT_FIELD ego_mesh_t::test_wall(const BIT_FIELD bits, mesh_wall_data_t& data) const
{
	// if there is no interaction with the mesh or the mesh is empty, return 0.
	if (EMPTY_BIT_FIELD == bits || 0 == _info.getTileCount() || _tmem.getTileCount() == 0) {
		return EMPTY_BIT_FIELD;
	}

	// The bit accumulator.
	BIT_FIELD pass = 0;

	// Detect out of bounds in the x- and/or y-direction.
	// In that case, return "wall" and "impassable".
	if ((data._i._min.getX() < 0 || data._i._min.getX() >= data._mesh->_info.getTileCountX()) ||
		(data._i._min.getY() < 0 || data._i._max.getY() >= data._mesh->_info.getTileCountY())) {
		pass = (MAPFX_IMPASS | MAPFX_WALL) & bits;
		g_meshStats.boundTests++;
	}
	if (EMPTY_BIT_FIELD != pass) {
		return pass;
	}

	for (int iy = data._i._min.getY(); iy <= data._i._max.getY(); ++iy) {
		for (int ix = data._i._min.getX(); ix <= data._i._max.getX(); ++ix) {
			TileIndex tileIndex(ix + iy * data._mesh->_gmem._grids_x);
			BIT_FIELD pass = ego_grid_info_t::test_all_fx(&(data._mesh->getGridInfo(tileIndex)), bits);
			if (EMPTY_BIT_FIELD != pass) {
				return pass;
			}
			g_meshStats.mpdfxTests++;
		}
	}

	return pass;
}
BIT_FIELD ego_mesh_t::test_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits) const {
	return test_wall(bits, mesh_wall_data_t(this, pos, radius));
}

float ego_mesh_t::get_pressure(const Vector3f& pos, float radius, const BIT_FIELD bits) const
{
    const float tile_area = Info<float>::Grid::Size() * Info<float>::Grid::Size();


    // deal with the optional parameters
    float loc_pressure = 0.0f;

    if (0 == bits) return 0;

    if ( 0 == _info.getTileCount() || _tmem.getTileCount() == 0 ) return 0;

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

			float area_ratio;
            float ovl_x_min, ovl_x_max;
            float ovl_y_min, ovl_y_max;

            float tx_min = ( ix + 0 ) * Info<float>::Grid::Size();
            float tx_max = ( ix + 1 ) * Info<float>::Grid::Size();

            if ( ix < 0 || ix >= _info.getTileCountX() )
            {
                tile_valid = false;
            }

            if ( tile_valid )
            {
                TileIndex itile = getTileIndex(PointGrid(ix, iy));
                tile_valid = grid_is_valid( itile );
                if ( !tile_valid )
                {
                    is_blocked = true;
                }
                else
                {
                    is_blocked = ( 0 != ego_grid_info_t::test_all_fx( &(_gmem.get(itile)), bits ) );
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

BIT_FIELD ego_mesh_t::hit_wall(const Vector3f& pos, float radius, const BIT_FIELD bits, Vector2f& nrm, float *pressure, mesh_wall_data_t& data) const {
	BIT_FIELD loc_pass;
	Uint32 pass;
	bool invalid;

	float  loc_pressure;

	bool needs_pressure = (NULL != pressure);
	bool needs_nrm = true;

	// deal with the optional parameters
	if (NULL == pressure) pressure = &loc_pressure;
	*pressure = 0.0f;

	nrm = Vector2f::zero();


	// ego_mesh_test_wall() clamps pdata->ix_* and pdata->iy_* to valid values

	pass = loc_pass = 0;
	nrm[kX] = nrm[kY] = 0.0f;
	for (int iy = data._i._min.getY(); iy <= data._i._max.getY(); iy++)
	{
		invalid = false;

		float ty_min = (iy + 0) * Info<float>::Grid::Size();
		float ty_max = (iy + 1) * Info<float>::Grid::Size();

		if (iy < 0 || iy >= _info.getTileCountY())
		{
			loc_pass |= (MAPFX_IMPASS | MAPFX_WALL);

			if (needs_nrm)
			{
				nrm[kY] += pos[kY] - (ty_max + ty_min) * 0.5f;
			}

			invalid = true;
			g_meshStats.boundTests++;
		}

		for (int ix = data._i._min.getX(); ix <= data._i._max.getX(); ix++)
		{
			float tx_min = (ix + 0) * Info<float>::Grid::Size();
			float tx_max = (ix + 1) * Info<float>::Grid::Size();

			if (ix < 0 || ix >= data._mesh->_info.getTileCountX())
			{
				loc_pass |= MAPFX_IMPASS | MAPFX_WALL;

				if (needs_nrm)
				{
					nrm[kX] += pos[kX] - (tx_max + tx_min) * 0.5f;
				}

				invalid = true;
				g_meshStats.boundTests++;
			}

			if (!invalid)
			{
				TileIndex itile = getTileIndex(PointGrid(ix, iy));
				if (grid_is_valid(itile))
				{
					BIT_FIELD mpdfx = ego_grid_info_t::get_all_fx(&(data._mesh->getGridInfo(itile)));
					bool is_blocked = HAS_SOME_BITS(mpdfx, bits);

					if (is_blocked)
					{
						SET_BIT(loc_pass, mpdfx);

						if (needs_nrm)
						{
							nrm[kX] += pos[kX] - (tx_max + tx_min) * 0.5f;
							nrm[kY] += pos[kY] - (ty_max + ty_min) * 0.5f;
						}
					}
				}
			}
		}
	}

	pass = loc_pass & bits;

	if (0 == pass)
	{
		// if there is no impact at all, there is no normal and no pressure
		nrm = Vector2f::zero();
		*pressure = 0.0f;
	}
	else
	{
		if (needs_nrm)
		{
			// special cases happen a lot. try to avoid computing the square root
			if (0.0f == nrm[kX] && 0.0f == nrm[kY])
			{
				// no normal does not mean no net pressure,
				// just that all the simplistic normal calculations balance
			}
			else if (0.0f == nrm[kX])
			{
				nrm[kY] = SGN(nrm[kY]);
			}
			else if (0.0f == nrm[kY])
			{
				nrm[kX] = SGN(nrm[kX]);
			}
			else
			{
				nrm.normalize();
			}
		}

		if (needs_pressure)
		{
			*pressure = get_pressure(pos, radius, bits);
		}
	}

	return pass;
}
BIT_FIELD ego_mesh_t::hit_wall( const Vector3f& pos, const float radius, const BIT_FIELD bits, Vector2f& nrm, float * pressure) const
{
	return hit_wall(pos, radius, bits, nrm, pressure, mesh_wall_data_t(this, pos, radius));
}

	mesh_wall_data_t::mesh_wall_data_t(const ego_mesh_t *mesh,
		const mesh_rect<float, CoordinateSystem::World>& f,
		const mesh_rect<int, CoordinateSystem::Grid>& i)
		: _mesh(mesh), _f(f), _i(i)
	{
		if (nullptr == _mesh) {
			throw std::runtime_error("nullptr == mesh");
		}
	}
	mesh_wall_data_t::mesh_wall_data_t(const ego_mesh_t *mesh, const Vector3f& pos, float radius)
		: _mesh(mesh),
		_f(pos, std::max(std::abs(radius), Info<float>::Grid::Size() * 0.5f)),
		_i(PointGrid(0, 0), PointGrid(0, 0))
	{
		if (nullptr == mesh) {
			throw std::runtime_error("nullptr == mesh");
		}
		_mesh = mesh;
		// Limit the values to be in-bounds.
		_f._min = PointWorld(std::max(_f._min.getX(), 0.0f),
			std::max(_f._min.getY(), 0.0f));
		_f._max = PointWorld(std::min(_f._max.getX(), _mesh->_gmem._edge_x),
			std::min(_f._max.getY(), _mesh->_gmem._edge_y));
		_i._min = PointGrid(std::floor(_f._min.getX() / Info<float>::Grid::Size()),
			std::floor(_f._min.getY() / Info<float>::Grid::Size()));
		_i._max = PointGrid(std::floor(_f._max.getX() / Info<float>::Grid::Size()),
			std::floor(_f._max.getY() / Info<float>::Grid::Size()));
	}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float ego_mesh_t::get_max_vertex_0(const PointGrid& point) const
{
    TileIndex itile = getTileIndex(point);
    if (TileIndex::Invalid == itile)
    {
        return 0.0f;
    }
    // get a pointer to the tile
    const ego_tile_info_t& ptile = _tmem.get(itile);

	size_t vstart = ptile._vrtstart;
	size_t vcount = std::min(static_cast<size_t>(4), _tmem.getVertexCount());

	size_t cnt;
	size_t ivrt = vstart;
    float zmax = _tmem._plst[ivrt][ZZ];
    for ( ivrt++, cnt = 1; cnt < vcount; ivrt++, cnt++ )
    {
        zmax = std::max( zmax, _tmem._plst[ivrt][ZZ] );
    }

    return zmax;
}

float ego_mesh_t::get_max_vertex_1( const PointGrid& point, float xmin, float ymin, float xmax, float ymax ) const
{
    static const int ix_off[4] = {1, 1, 0, 0};
    static const int iy_off[4] = {0, 1, 1, 0};

    TileIndex itile = getTileIndex( point );

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
// ego_tile_info_t
//--------------------------------------------------------------------------------------------
ego_tile_info_t::ego_tile_info_t() :
    _itile(0),
    _type(0),
    _img(0),
    _vrtstart(0),
    _fanoff(true),
    _ncache{0, 0, 0, 0},
	_lightingCache(),
	_vertexLightingCache(),
    _oct(),
    _aabb()
{
    //ctor
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
		GRID_FX_BITS fx = ego_grid_info_t::get_all_fx(&(gmem.get(i)));

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
    TileIndex tileRef = getTileIndex(point);

    // Everything outside the map bounds is wall and impassable.
    if (!grid_is_valid(tileRef))
    {
        return HAS_SOME_BITS((MAPFX_IMPASS | MAPFX_WALL), bits);
    }

    // Since we KNOW that this is in range, allow raw access to the data structure.
    GRID_FX_BITS fx = ego_grid_info_t::get_all_fx(&(_gmem.get(tileRef)));

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
    TileIndex tile = this->getTileIndex(point);
	if (!grid_is_valid(tile)) {
		return 0;
	}
	PointGrid gridPoint(static_cast<int>(point.getX()) % Info<int>::Grid::Size(),
                        static_cast<int>(point.getY()) % Info<int>::Grid::Size());

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

TileIndex ego_mesh_t::getTileIndex(const PointWorld& point) const
{
    if (point.getX() >= 0.0f && point.getX() < _gmem._edge_x && 
		point.getY() >= 0.0f && point.getY() < _gmem._edge_y)
    {
        PointGrid gridPoint(static_cast<int>(point.getX()) / Info<int>::Grid::Size(), 
                            static_cast<int>(point.getY()) / Info<int>::Grid::Size());
        return getTileIndex(gridPoint);
    }
    return TileIndex::Invalid;
}

TileIndex ego_mesh_t::getTileIndex(const PointGrid& point) const
{
    if (point.getX() < 0 || point.getX() >= _info.getTileCountX())
    {
        return TileIndex::Invalid;
    }
	if (point.getY() < 0 || point.getY() >= _info.getTileCountY())
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
    retval = ego_grid_info_t::sub_pass_fx(&(_gmem.get(itile)), flags );

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
    bool retval = ego_grid_info_t::add_pass_fx(&(_gmem.get(index)), flags);

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
    return ego_grid_info_t::test_all_fx(&(_gmem.get(index)), flags);
}

ego_tile_info_t& ego_mesh_t::getTileInfo(const TileIndex& index) {
	if (index.getI() >= _info.getTileCount()) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
	}
	return _tmem.get(index);
}

const ego_tile_info_t& ego_mesh_t::getTileInfo(const TileIndex& index) const {
    if (index.getI() >= _info.getTileCount()) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
    }
    return _tmem.get(index);
}

ego_grid_info_t& ego_mesh_t::getGridInfo(const TileIndex& index) {
	if (index.getI() >= _info.getTileCount()) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
	}
	return _gmem.get(index);
}

const ego_grid_info_t& ego_mesh_t::getGridInfo(const TileIndex& index) const {
    if (index.getI() >= _info.getTileCount()) {
		throw Id::RuntimeErrorException(__FILE__, __LINE__, "index out of bounds");
    }
    return _gmem.get(index);
}

Uint8 ego_mesh_t::get_twist(const TileIndex& index) const
{
    // Validate arguments.
    if (index >= _info.getTileCount())
    {
        return TWIST_FLAT;
    }
    return _gmem.get(index)._twist;
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
    const ego_tile_info_t& info = _tmem.get(tile);
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
