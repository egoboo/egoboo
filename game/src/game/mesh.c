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
#include "game/game.h"
#include "game/Module/Module.hpp"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

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
	Log::get().warn("%s:%d: %s\n", __FILE__, __LINE__, os.str().c_str());
}

//--------------------------------------------------------------------------------------------

tile_mem_t::tile_mem_t(const Ego::MeshInfo& info)
	: _tileList(info.getTileCount()), _info(info), _bbox() {
	// If the number of vertices exceeds the limits ...
	if (info.getVertexCount() > MAP_VERTICES_MAX) {
		// ... emit a warning.
		warnNumberOfVertices(__FILE__, __LINE__, info.getVertexCount());
	}
	// Set the mesh edge info.
	_edge_x = (info.getTileCountX() + 1) * Info<int>::Grid::Size();
	_edge_y = (info.getTileCountY() + 1) * Info<int>::Grid::Size();
	// Allocate the arrays.
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
	for (size_t i = 0; i < _info.getTileCount(); ++i) {
		get(i)._vrtstart = vertexIndex;

		uint8_t type = get(i)._type;

		// Throw away any remaining upper bits.
		type &= 0x3F;

		const tile_definition_t *def = dict.get(type);
		if (!def) continue;

		vertexIndex += def->numvertices;
	}

	if (vertexIndex != _info.getVertexCount()) {
		Log::get().warn("%s:%d: unexpected number of vertices %" PRIuZ " of %" PRIuZ "\n", __FILE__, __LINE__, \
			            vertexIndex, _info.getVertexCount());
	}
}

//--------------------------------------------------------------------------------------------

std::shared_ptr<ego_mesh_t> MeshLoader::convert(const map_t& source) const
{
    // Create a mesh.
    auto target = std::make_shared<ego_mesh_t>(Ego::MeshInfo(source._info.getVertexCount(), source._info.getTileCountX(), source._info.getTileCountY()));
	tile_mem_t& tmem_dst = target->_tmem;
	Ego::MeshInfo& info_dst = target->_info;

    // copy all the per-tile info
    for (Index1D cnt = 0; cnt < info_dst.getTileCount(); cnt++)
    {
        const tile_info_t& ptile_src = source._mem.tiles[cnt.i()];
        ego_tile_info_t& ptile_dst = tmem_dst.get(cnt);

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        ptile_dst._type = ptile_src.type;
        ptile_dst._img  = ptile_src.img;

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
		ptile_dst._base_fx = ptile_src.fx;
		ptile_dst._twist   = ptile_src.twist;

        // set the local fx flags
		ptile_dst._pass_fx = ptile_dst._base_fx;

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
        ego_tile_info_t& ptile_dst = tmem_dst.get(cnt);
        const map_vertex_t& pvrt_src = source._mem.vertices[vertex];

		ptile_dst._a = pvrt_src.a;
		ptile_dst._l = 0.0f;
    }

	return target;
}

//--------------------------------------------------------------------------------------------
std::shared_ptr<ego_mesh_t> MeshLoader::operator()(const std::string& moduleName) const
{
	map_t map;
	// Load the map data.
	tile_dictionary_load_vfs("mp_data/fans.txt", tile_dict);
	if (!map.load("mp_data/level.mpd"))
	{
        Log::Entry entry(Log::Level::Error, __FILE__, __LINE__);
	    entry << "unable to load mesh of module `" << moduleName << "`" << Log::EndOfEntry;
        Log::get() << entry;
		throw Id::RuntimeErrorException(__FILE__, __LINE__, entry.getText());
	}
	// Create the mesh from map.
	std::shared_ptr<ego_mesh_t> mesh = convert(map);
	if (!mesh)
	{
        Log::Entry entry(Log::Level::Error, __FILE__, __LINE__);
        entry << "unable to convert mesh of module `" << moduleName << "`" << Log::EndOfEntry;
        Log::get() << entry;
		throw Id::RuntimeErrorException(__FILE__, __LINE__, entry.getText());
	}
	mesh->finalize();
	return mesh;
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

		twist_facing_x[cnt] = Facing((FACING_T)(-vec_to_facing(nrm[kZ], nrm[kY])));
		twist_facing_y[cnt] = Facing((FACING_T)(+vec_to_facing(nrm[kZ], nrm[kX])));

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
    _tmem._bbox = AxisAlignedBox3f(Point3f(_tmem._plst[0][XX], _tmem._plst[0][YY], _tmem._plst[0][ZZ]),
		                           Point3f(_tmem._plst[0][XX], _tmem._plst[0][YY], _tmem._plst[0][ZZ]));

	for (Index1D cnt = 0; cnt < _info.getTileCount(); cnt++)
	{
        ego_tile_info_t& ptile = _tmem.get(cnt);
        oct_bb_t& poct = ptile._oct;


        ptile._itile = cnt.i();

		tile_definition_t *pdef = tile_dict.get(ptile._type & 0x3F);
		if (NULL == pdef) continue;

		// Initialize the octagonal bounding box of the tile with the first vertex of the tile ...
        size_t mesh_vrt = _tmem.get(cnt)._vrtstart;
        oct_vec_v2_t ovec = oct_vec_v2_t(Vector3f(_tmem._plst[mesh_vrt][0], _tmem._plst[mesh_vrt][1],_tmem._plst[mesh_vrt][2]));
        poct = oct_bb_t(ovec);
        mesh_vrt++;
        
        // ... then add the other vertex of the tile to it.
        for (uint8_t i = 1, n = pdef->numvertices; i < n; i++, mesh_vrt++ )
        {
            ovec = oct_vec_v2_t(Vector3f(_tmem._plst[mesh_vrt][0],_tmem._plst[mesh_vrt][1],_tmem._plst[mesh_vrt][2]));
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
            oct_bb_t::self_grow(poct, ovec);
        }

        // Add the bounds of the tile to the bounds of the mesh.
        _tmem._bbox.join(poct.toAxisAlignedBox());
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::make_normals()
{
    // test for mesh

    // set the default normal for each fan, based on the calculated twist value
    for (Index1D fan0 = 0; fan0 < _tmem.getInfo().getTileCount(); fan0++ )
    {
        uint8_t twist = _tmem.get(fan0)._twist;

        _tmem._nlst[fan0.i()][XX] = g_meshLookupTables.twist_nrm[twist][kX];
        _tmem._nlst[fan0.i()][YY] = g_meshLookupTables.twist_nrm[twist][kY];
        _tmem._nlst[fan0.i()][ZZ] = g_meshLookupTables.twist_nrm[twist][kZ];
    }

	int      edge_is_crease[4];
	Vector3f nrm_lst[4], vec_sum;
	float    weight_lst[4];

    // find an "average" normal of each corner of the tile
    for (size_t iy = 0; iy < _info.getTileCountY(); iy++ )
    {
        for (size_t ix = 0; ix < _info.getTileCountX(); ix++ )
        {
            constexpr int ix_off[4] = {0, 1, 1, 0};
            constexpr int iy_off[4] = {0, 0, 1, 1};

            Index1D fan0 = getTileIndex(Index2D(ix, iy));
			if (!grid_is_valid(fan0)) {
				continue;
			}

            nrm_lst[0][kX] = _tmem._nlst[fan0.i()][XX];
            nrm_lst[0][kY] = _tmem._nlst[fan0.i()][YY];
            nrm_lst[0][kZ] = _tmem._nlst[fan0.i()][ZZ];

            // for each corner of this tile
            for (int i = 0; i < 4; i++ )
            {
                // the offset list needs to be shifted depending on what i is
                int dx, dy;
                size_t shift = ( 6 - i ) % 4;
                if ( 1 == ix_off[(4-shift) % 4] ){
                  dx = -1;  
                }  
                else {
                  dx = 0;  
                } 
                if ( 1 == iy_off[(4-shift) % 4] ){
                  dy = -1;  
                }  
                else{
                    dy = 0;
                } 

                int loc_ix_off[4];
                int loc_iy_off[4];
                for (int k = 0; k < 4; k++ )
                {
                    loc_ix_off[k] = ix_off[( 4-shift + k ) % 4 ] + dx;
                    loc_iy_off[k] = iy_off[( 4-shift + k ) % 4 ] + dy;
                }

                // cache the normals
                // nrm_lst[0] is already known.
                for (int j = 1; j < 4; j++ )
                {
                    int jx = static_cast<int>(ix) + loc_ix_off[j];
                    int jy = static_cast<int>(iy) + loc_iy_off[j];

                    Index1D fan1 = getTileIndex(Index2D(jx, jy));

                    if ( grid_is_valid( fan1 ) )
                    {
                        nrm_lst[j][kX] = _tmem._nlst[fan1.i()][XX];
                        nrm_lst[j][kY] = _tmem._nlst[fan1.i()][YY];
                        nrm_lst[j][kZ] = _tmem._nlst[fan1.i()][ZZ];

                        if ( nrm_lst[j][kZ] < 0 )
                        {
                            nrm_lst[j] = -nrm_lst[j];
                        }
                    }
                    else
                    {
                        nrm_lst[j] = Vector3f(0.0f, 0.0f, 1.0f);
                    }
                }

                // find the creases
                for (int j = 0; j < 4; j++ )
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
                for (int j = 1; j < 4; j++ )
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

BIT_FIELD ego_mesh_t::test_wall(const BIT_FIELD bits, const mesh_wall_data_t& data) const
{
	// if there is no interaction with the mesh or the mesh is empty, return 0.
	if (EMPTY_BIT_FIELD == bits || 0 == _info.getTileCount() || _tmem.getInfo().getTileCount() == 0) {
		return EMPTY_BIT_FIELD;
	}

	// The bit accumulator.
	BIT_FIELD pass = 0;

	// Detect out of bounds in the x- and/or y-direction.
	// In that case, return "wall" and "impassable".
	if ((data._i.min().x() < 0 || data._i.max().x() >= data._mesh->_info.getTileCountX()) ||
		(data._i.min().y() < 0 || data._i.max().y() >= data._mesh->_info.getTileCountY())) {
		pass = (MAPFX_IMPASS | MAPFX_WALL) & bits;
		g_meshStats.boundTests++;
	}
	if (EMPTY_BIT_FIELD != pass) {
		return pass;
	}

	for (int iy = data._i.min().y(); iy <= data._i.max().y(); ++iy) {
		for (int ix = data._i.min().x(); ix <= data._i.max().x(); ++ix) {
			Index1D tileIndex(ix + iy * data._mesh->_tmem.getInfo().getTileCountX());
			BIT_FIELD pass = data._mesh->getTileInfo(tileIndex).testFX(bits);
			if (EMPTY_BIT_FIELD != pass) {
				return pass;
			}
			g_meshStats.mpdfxTests++;
		}
	}

	return pass;
}
BIT_FIELD ego_mesh_t::test_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits) const {
	return test_wall(bits, mesh_wall_data_t(this, Circle2f(Point2f(pos[kX], pos[kY]), radius)));
}

float ego_mesh_t::get_pressure(const Vector3f& pos, float radius, const BIT_FIELD bits) const
{
    const float tile_area = Info<float>::Grid::Size() * Info<float>::Grid::Size();


    // deal with the optional parameters
    float loc_pressure = 0.0f;

    if (0 == bits) return 0;

    if ( 0 == _info.getTileCount() || _tmem.getInfo().getTileCount() == 0 ) return 0;

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
                Index1D itile = getTileIndex(Index2D(ix, iy));
                tile_valid = grid_is_valid( itile );
                if ( !tile_valid )
                {
                    is_blocked = true;
                }
                else
                {
                    is_blocked = 0 != _tmem.get(itile).testFX(bits);
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


	mesh_wall_data_t::mesh_wall_data_t(const ego_mesh_t *mesh,
		                               const AxisAlignedBox2f& f,
		                               const IndexRect& i)
		: _mesh(mesh), _f(f), _i(i)
	{
		if (nullptr == _mesh) {
			throw std::runtime_error("nullptr == mesh");
		}
	}

	mesh_wall_data_t::mesh_wall_data_t(const ego_mesh_t *mesh, const Circle2f& circle)
		: _mesh(mesh),
		  _f(leastClosure(circle)),
		  _i(Index2D(0, 0), Index2D(0, 0))
	{
		if (nullptr == mesh) {
			throw std::runtime_error("nullptr == mesh");
		}
		_mesh = mesh;
		// Limit the coordinate rectangle to be in bounds.
        {
            auto min = Point2f(std::max(_f.getMin().x(), 0.0f),
                               std::max(_f.getMin().y(), 0.0f));
            auto max = Point2f(std::min(_f.getMax().x(), _mesh->_tmem._edge_x),
                               std::min(_f.getMax().y(), _mesh->_tmem._edge_y));
            _f = AxisAlignedBox2f(min, max);
        }
        // Limit the index rectangle to be in bounds.
        {
            auto min = Index2D(std::floor(_f.getMin().x() / Info<float>::Grid::Size()),
                               std::floor(_f.getMin().y() / Info<float>::Grid::Size()));
            auto max = Index2D(std::floor(_f.getMax().x() / Info<float>::Grid::Size()),
                               std::floor(_f.getMax().y() / Info<float>::Grid::Size()));
            _i = IndexRect(min, max);
        }
	}

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
	_base_fx(0), _pass_fx(0), _a(0), _l(0), _cache_frame(-1), _twist(TWIST_FLAT)
{
    //ctor
}

GRID_FX_BITS ego_tile_info_t::testFX(const GRID_FX_BITS bits) const {
	return getFX() & bits;
}

GRID_FX_BITS ego_tile_info_t::getFX() const {
	return _pass_fx;
}

bool ego_tile_info_t::setFX(const GRID_FX_BITS bits) {
	// Save the old bits.
	GRID_FX_BITS oldBits = getFX();

	// Modify the bits.
	_pass_fx = bits;

	// Get the new bits.
	GRID_FX_BITS newBits = getFX();

	// Return if the bits were actually modified.
	return oldBits != newBits;
}

bool ego_tile_info_t::addFX(const GRID_FX_BITS bits) {
	// Save the old bits.
	GRID_FX_BITS oldBits = getFX();

	// Modify the bits.
	SET_BIT(_pass_fx, bits);

	// Get the new bits.
	GRID_FX_BITS newBits = getFX();

	// Return if the bits were actually modified.
	return oldBits != newBits;
}

bool ego_tile_info_t::removeFX(const GRID_FX_BITS bits) {
	// Save the old bits.
	GRID_FX_BITS oldBits = getFX();

	// Modify the bits.
	UNSET_BIT(_pass_fx, bits);

	// Get the new bits.
	GRID_FX_BITS newBits = getFX();

	// Return if the bits were actually modified.
	return oldBits != newBits;
}

//--------------------------------------------------------------------------------------------

mpdfx_list_ary_t::mpdfx_list_ary_t()
	: elements() {
}

mpdfx_list_ary_t::~mpdfx_list_ary_t() {
}

void mpdfx_list_ary_t::clear()
{
    elements.clear();
}

void mpdfx_list_ary_t::push_back(const Index1D& element)
{
    elements.push_back(element);
}

//--------------------------------------------------------------------------------------------

mpdfx_lists_t::mpdfx_lists_t(const Ego::MeshInfo& info) {
	sha.elements.reserve(info.getTileCount());
	drf.elements.reserve(info.getTileCount());
	anm.elements.reserve(info.getTileCount());
	wat.elements.reserve(info.getTileCount());
	wal.elements.reserve(info.getTileCount());
	imp.elements.reserve(info.getTileCount());
	dam.elements.reserve(info.getTileCount());
	slp.elements.reserve(info.getTileCount());

	// the list needs to be resynched
	dirty = true;
}

mpdfx_lists_t::~mpdfx_lists_t() {
	// No memory, hence nothing is stored, hence nothing is dirty.
	dirty = false;
}

void mpdfx_lists_t::reset()
{
    // Clear the lists.
    sha.clear();
    drf.clear();
    anm.clear();
    wat.clear();
    wal.clear();
    imp.clear();
    dam.clear();
    slp.clear();

    // Everything has been reset. Force it to recalculate.
	dirty = true;
}

int mpdfx_lists_t::push( GRID_FX_BITS fx_bits, size_t value )
{
    int retval = 0;

    if ( 0 == fx_bits ) return true;

    if ( HAS_NO_BITS( fx_bits, MAPFX_SHA ) )
    {
        sha.push_back(value);
        {
            retval++;
        }

    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_REFLECTIVE ) )
    {
        drf.push_back(value);
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_ANIM ) )
    {
        anm.push_back(value);
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WATER ) )
    {
        wat.push_back(value);
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WALL ) )
    {
        wal.push_back(value);
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_IMPASS ) )
    {
        imp.push_back(value);
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_DAMAGE ) )
    {
        dam.push_back(value);
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_SLIPPY ) )
    {
        slp.push_back(value);
        {
            retval++;
        }
    }

    return retval;
}

bool mpdfx_lists_t::synch( const tile_mem_t& tmem, bool force )
{
    if ( 0 == tmem.getInfo().getTileCount()) return true;

    // don't re-calculate unless it is necessary
    if ( !force && !dirty ) return true;

    // !!reset the counts!!
	reset();

    for (size_t i = 0; i < tmem.getInfo().getTileCount(); i++ )
    {
		push(tmem.get(i).getFX(), i);
    }

    // we're done calculating
	dirty = false;

    return true;
}

//--------------------------------------------------------------------------------------------

bool ego_mesh_t::tile_has_bits( const Index2D& i, const BIT_FIELD bits ) const
{
    // Figure out which tile we are on.
    Index1D j = getTileIndex(i);

    // Everything outside the map bounds is wall and impassable.
    if (!grid_is_valid(j))
    {
        return HAS_SOME_BITS((MAPFX_IMPASS | MAPFX_WALL), bits);
    }

    // Since we KNOW that this is in range, allow raw access to the data structure.
    GRID_FX_BITS fx = _tmem.get(j).getFX();

    return HAS_SOME_BITS(fx, bits);
}

bool ego_mesh_t::grid_is_valid(const Index1D& i) const
{
	g_meshStats.boundTests++;
    return _info.isValid(i);
}

Vector2f toWorldLT(const Index2D i) {
    return Vector2f((float)i.x(), (float)i.y()) * Info<float>::Grid::Size();
}

float ego_mesh_t::getElevation(const Vector2f& p) const
{
    Index1D i1 = getTileIndex(p);
	if (!grid_is_valid(i1)) {
		return 0;
	}

    // Get the height of each fan corner.
    float z0 = _tmem._plst[_tmem.get(i1)._vrtstart + 0][ZZ];
    float z1 = _tmem._plst[_tmem.get(i1)._vrtstart + 1][ZZ];
    float z2 = _tmem._plst[_tmem.get(i1)._vrtstart + 2][ZZ];
    float z3 = _tmem._plst[_tmem.get(i1)._vrtstart + 3][ZZ];

    //Calculate where on the tile we are relative to top left corner of the tile (0,0)
    Vector2f posOnTile = Vector2f(static_cast<float>(static_cast<int>(p.x()) % Info<int>::Grid::Size()), 
                                  static_cast<float>(static_cast<int>(p.y()) % Info<int>::Grid::Size()));

    // Get the weighted height of each side.
    float zleft = (z0 * (Info<float>::Grid::Size() - posOnTile.y()) + z3 * posOnTile.y()) / Info<float>::Grid::Size();
    float zright = (z1 * (Info<float>::Grid::Size() - posOnTile.y()) + z2 * posOnTile.y()) / Info<float>::Grid::Size();
    float zdone = (zleft * (Info<float>::Grid::Size() - posOnTile.x()) + zright * posOnTile.x()) / Info<float>::Grid::Size();

    return zdone;
}

Index1D ego_mesh_t::getTileIndex(const Vector2f& p) const
{
    if (p.x() >= 0.0f && p.x() < _tmem._edge_x && 
		p.y() >= 0.0f && p.y() < _tmem._edge_y)
    {
        // Map world coordinates to a tile index.
        // This function does not assume the point to be within the bounds of the mesh.
        // If a point is passed which is outside the bounds, the resulting index will
        // be invalid w.r.t. to the mesh.
        Index2D i2 = Index2D(static_cast<int>(p.x()) / Info<int>::Grid::Size(),
                             static_cast<int>(p.y()) / Info<int>::Grid::Size());

        return getTileIndex(i2);
    }
    return Index1D::Invalid;
}

Index1D ego_mesh_t::getTileIndex(const Index2D& i) const
{
    if (!_info.isValid(i)) {
        return Index1D::Invalid;
    }
	return _info.map(i);
}

bool ego_mesh_t::clear_fx( const Index1D& i, const BIT_FIELD flags )
{
	g_meshStats.boundTests++;
    if (!_info.isValid(i)) {
        return false;
    }
	g_meshStats.mpdfxTests++;

    if (_tmem.get(i).removeFX(flags)) {
        _fxlists.dirty = true;
        return true;
    } else {
        return false;
    }
}

bool ego_mesh_t::add_fx(const Index1D& i, const BIT_FIELD flags)
{
    // Validate tile index.
	g_meshStats.boundTests++;
    if (!_info.isValid(i)) {
        return false;
    }

    // Succeed only of something actually changed.
	g_meshStats.mpdfxTests++;
    bool retval = _tmem.get(i).addFX(flags);

    if ( retval )
    {
        _fxlists.dirty = true;
    }

    return retval;
}

Uint32 ego_mesh_t::test_fx(const Index1D& i, const BIT_FIELD flags) const
{
    // test for a trivial value of flags
    if (EMPTY_BIT_FIELD == flags) return 0;

    // test for invalid tile
	g_meshStats.boundTests++;
    if (!_info.isValid(i)) {
        return flags & (MAPFX_WALL | MAPFX_IMPASS);
    }

    // if the tile is actually labelled as MAP_FANOFF, ignore it completely
    if (_tmem.get(i).isFanOff())
    {
        return 0;
    }

	g_meshStats.mpdfxTests++;
    return _tmem.get(i).testFX(flags);
}

ego_tile_info_t& ego_mesh_t::getTileInfo(const Index1D& i) {
    _info.assertValid(i);
	return _tmem.get(i);
}

const ego_tile_info_t& ego_mesh_t::getTileInfo(const Index1D& i) const {
    _info.assertValid(i);
    return _tmem.get(i);
}

uint8_t ego_mesh_t::get_twist(const Index1D& i) const
{
    if (!_info.isValid(i)) {
        return TWIST_FLAT;
    }
    return _tmem.get(i)._twist;
}

uint8_t ego_mesh_t::get_fan_twist(const Index1D& i) const
{
    if (!_info.isValid(i)) {
        return TWIST_FLAT;
    }
    const ego_tile_info_t& info = _tmem.get(i);
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

float ego_mesh_t::get_max_vertex_0(const Index2D& i) const
{
	Index1D j = getTileIndex(i);
    if (!_info.isValid(j)) {
        return 0.0f;
    }

    const ego_tile_info_t& tile = _tmem.get(j);

	size_t vstart = tile._vrtstart;
	size_t vcount = std::min(static_cast<size_t>(4), _tmem.getInfo().getVertexCount());

	size_t cnt;
	size_t ivrt = vstart;
	float zmax = _tmem._plst[ivrt][ZZ];
	for (ivrt++, cnt = 1; cnt < vcount; ivrt++, cnt++)
	{
		zmax = std::max(zmax, _tmem._plst[ivrt][ZZ]);
	}

	return zmax;
}

float ego_mesh_t::get_max_vertex_1(const Index2D& i, float xmin, float ymin, float xmax, float ymax) const
{
	static const int ix_off[4] = { 1, 1, 0, 0 };
	static const int iy_off[4] = { 0, 1, 1, 0 };

	Index1D j = getTileIndex(i);

    if (!_info.isValid(j)) {
        return 0.0f;
    }

	size_t vstart = _tmem.get(j)._vrtstart;
	size_t vcount = std::min((size_t)4, _tmem.getInfo().getVertexCount());

	float zmax = -1e6;
	for (size_t ivrt = vstart, cnt = 0; cnt < vcount; ivrt++, cnt++)
	{
		GLXvector3f& vert = _tmem._plst[ivrt];

		// we are evaluating the height based on the grid, not the actual vertex positions
		float fx = (i.x() + ix_off[cnt]) * Info<float>::Grid::Size();
		float fy = (i.y() + iy_off[cnt]) * Info<float>::Grid::Size();

		if (fx >= xmin && fx <= xmax && fy >= ymin && fy <= ymax)
		{
			zmax = std::max(zmax, vert[ZZ]);
		}
	}

	if (-1e6 == zmax) zmax = 0.0f;

	return zmax;
}

Vector3f ego_mesh_t::get_diff(const Vector3f& pos, float radius, float center_pressure, const BIT_FIELD bits)
{
	/// @author BB
	/// @details determine the shortest "way out", but creating an array of "pressures"
	/// with each element representing the pressure when the object is moved in different directions
	/// by 1/2 a tile.

	const float jitter_size = Info<float>::Grid::Size() * 0.5f;
	std::array<float, 9> pressure_ary = {};
	float fx, fy;
	Vector3f diff = Vector3f::zero();
	float   sum_diff = 0.0f;
	float   dpressure;

	int cnt;

	// Find the pressure for the 9 points of jittering around the current position.
	pressure_ary[4] = center_pressure;
	for (cnt = 0, fy = pos[kY] - jitter_size; fy <= pos[kY] + jitter_size; fy += jitter_size)
	{
		for (fx = pos[kX] - jitter_size; fx <= pos[kX] + jitter_size; fx += jitter_size, cnt++)
		{
			Vector3f jitter_pos(fx, fy, 0.0f);
			if (4 == cnt) continue;
			pressure_ary[cnt] = get_pressure(jitter_pos, radius, bits);
		}
	}

	// Determine the "minimum number of tiles to move" to get into a clear area.
	diff[kX] = diff[kY] = 0.0f;
	sum_diff = 0.0f;
	for (cnt = 0, fy = -0.5f; fy <= 0.5f; fy += 0.5f)
	{
		for (fx = -0.5f; fx <= 0.5f; fx += 0.5f, cnt++)
		{
			if (4 == cnt) continue;

			dpressure = (pressure_ary[cnt] - center_pressure);

			// Find the maximal pressure gradient == the minimal distance to move.
			if (0.0f != dpressure)
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

BIT_FIELD ego_mesh_t::hit_wall(const Vector3f& pos, float radius, const BIT_FIELD bits, Vector2f& nrm, float *pressure, const mesh_wall_data_t& data) const {
	bool invalid;

	float  loc_pressure;

	bool needs_pressure = (NULL != pressure);
	bool needs_nrm = true;

	// deal with the optional parameters
	if (NULL == pressure) pressure = &loc_pressure;
	*pressure = 0.0f;

	nrm = Vector2f::zero();


	// ego_mesh_test_wall() clamps pdata->ix_* and pdata->iy_* to valid values

	BIT_FIELD loc_pass = 0;
	nrm[kX] = nrm[kY] = 0.0f;
	for (int iy = data._i.min().y(); iy <= data._i.max().y(); iy++)
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

		for (int ix = data._i.min().x(); ix <= data._i.max().x(); ix++)
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
				Index1D itile = getTileIndex(Index2D(ix, iy));
				if (grid_is_valid(itile))
				{
					BIT_FIELD mpdfx = data._mesh->getTileInfo(itile).getFX();
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

	uint32_t pass = loc_pass & bits;

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
				nrm[kY] = sgn(nrm[kY]);
			}
			else if (0.0f == nrm[kY])
			{
				nrm[kX] = sgn(nrm[kX]);
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

BIT_FIELD ego_mesh_t::hit_wall(const Vector3f& pos, const float radius, const BIT_FIELD bits, Vector2f& nrm, float * pressure) const
{
	return hit_wall(pos, radius, bits, nrm, pressure, mesh_wall_data_t(this, Circle2f(Point2f(pos[kX], pos[kY]), radius)));
}

ego_mesh_t::ego_mesh_t(const Ego::MeshInfo& mesh_info)
	: _info(mesh_info), _tmem(mesh_info), _fxlists(mesh_info) {
}

ego_mesh_t::~ego_mesh_t() {
}

void ego_mesh_t::remove_ambient() {
	/// @brief Remove ambient.
	uint8_t min_vrt_a = 255;
	for (Index1D i = 0; i < _info.getTileCount(); ++i) {
		min_vrt_a = std::min(min_vrt_a, _tmem.get(i)._a);
	}
	for (Index1D i = 0; i < _info.getTileCount(); ++i) {
		_tmem.get(i)._a = _tmem.get(i)._a - min_vrt_a;
	}
}

void ego_mesh_t::recalc_twist() {
	// @brief recalculate the twist.
	for (Index1D i = 0; i < _info.getTileCount(); ++i) {
		uint8_t twist = get_fan_twist(i);
		_tmem.get(i)._twist = twist;
	}
}

bool ego_mesh_t::set_texture(const Index1D& index1D, Uint16 image)
{
	if (!grid_is_valid(index1D)) {
		return false;
	}

	// Get the upper and lower bits for this tile image.
	uint16_t tile_value = _tmem.get(index1D)._img;
	uint16_t tile_lower = image & TILE_LOWER_MASK;
	uint16_t tile_upper = tile_value & TILE_UPPER_MASK;

	// Set the actual image.
	_tmem.get(index1D)._img = tile_upper | tile_lower;

	// Update the pre-computed texture info.
	return update_texture(index1D);
}

bool ego_mesh_t::update_texture(const Index1D& i)
{
	if (!grid_is_valid(i)) {
		return false;
	}
	const ego_tile_info_t& tile = _tmem.get(i);
	uint8_t type = tile._type & 0x3F;

	tile_definition_t *pdef = tile_dict.get(type);
	if (!pdef) return false;

	size_t mesh_vrt = tile._vrtstart;
	for (uint16_t tile_vrt = 0; tile_vrt < pdef->numvertices; tile_vrt++, mesh_vrt++) {
		_tmem._tlst[mesh_vrt][SS] = pdef->vertices[tile_vrt].u;
		_tmem._tlst[mesh_vrt][TT] = pdef->vertices[tile_vrt].v;
	}

	return true;
}

void ego_mesh_t::make_texture() {
	/// @brief Set the texture coordinate for every vertex.
	for (Index1D i = 0; i < _info.getTileCount(); ++i) {
		update_texture(i);
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
	_fxlists.synch(_tmem, true);
}

float ego_mesh_t::getElevation(const Vector2f& p, bool waterwalk) const
{
    const float floorElevation = getElevation(p);

    if (waterwalk && _currentModule->getWater()._surface_level > floorElevation && _currentModule->getWater()._is_water) {
        if (0 != test_fx(getTileIndex(p), MAPFX_WATER)) {
            return _currentModule->getWater()._surface_level;
        }
    }
    return floorElevation;
}
