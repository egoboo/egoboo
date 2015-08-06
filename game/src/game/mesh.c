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

#include "egolib/_math.h"
#include "egolib/bbox.h"
#include "game/mesh.h"
#include "game/graphic.h"
#include "game/egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// mesh initialization - not accessible by scripts
static void   ego_mesh_make_vrtstart( ego_mesh_t * mesh );

// some twist/normal functions
static bool ego_mesh_make_normals( ego_mesh_t * mesh );

static bool ego_mesh_convert( ego_mesh_t * pmesh_dst, map_t * pmesh_src );
static bool ego_mesh_make_bbox( ego_mesh_t * mesh );

static float grid_get_mix( float u0, float u, float v0, float v );



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mesh_t   mesh;

int mesh_mpdfx_tests = 0;
int mesh_bound_tests = 0;
int mesh_pressure_tests = 0;

fvec3_t   map_twist_nrm[256];
FACING_T  map_twist_facing_y[256];            // For surface normal of mesh
FACING_T  map_twist_facing_x[256];
fvec3_t   map_twist_vel[256];            // For sliding down steep hills
Uint8     map_twist_flat[256];

// variables to optimize calls to bind the textures
bool    mesh_tx_none   = false;
TX_REF    mesh_tx_image  = MESH_IMG_COUNT;
Uint8     mesh_tx_size   = 0xFF;

static void warnNumberOfVertices(const char *file, int line, size_t numberOfVertices)
{
    log_warning("%s:%d: mesh has too many vertices - %" PRIuZ " requested, but maximum is %d\n", file, line, numberOfVertices, MAP_VERTICES_MAX);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mesh_info_t *ego_mesh_info_t::ctor(ego_mesh_info_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    self->tiles_x = 0; self->tiles_y = 0;
    self->tiles_count = 0;
    self->vertcount = 0;
    return self;
}

ego_mesh_info_t *ego_mesh_info_t::dtor(ego_mesh_info_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    self->tiles_x = 0; self->tiles_y = 0;
    self->tiles_count = 0;
    self->vertcount = 0;
    return self;
}

void ego_mesh_info_t::init(ego_mesh_info_t *self, int numvert, size_t tiles_x, size_t tiles_y)
{
    // Set the desired number of tiles.
    self->tiles_x = tiles_x;
    self->tiles_y = tiles_y;
    self->tiles_count = self->tiles_x * self->tiles_y;

    // Set the desired number of vertices.
    if (numvert < 0)
    {
        numvert = MAP_FAN_VERTICES_MAX * self->tiles_count;
    }
    self->vertcount = numvert;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------



tile_mem_t *tile_mem_t::ctor(tile_mem_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    BLANK_STRUCT_PTR(self);
    return self;
}

tile_mem_t *tile_mem_t::dtor(tile_mem_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    tile_mem_t::free(self);

    BLANK_STRUCT_PTR(self);

    return self;
}

bool tile_mem_t::alloc(tile_mem_t *self, const ego_mesh_info_t *info)
{
    if (!self || !info || 0 == info->vertcount)
    {
        return false;
    }

    // Free any memory already allocated.
    if (!tile_mem_t::free(self))
    {
        return false;
    }
    if (info->vertcount > MAP_VERTICES_MAX)
    {
        warnNumberOfVertices(__FILE__, __LINE__, info->vertcount);
        return false;
    }

    // Allocate per-vertex memory.
    try
    {
        self->plst = new GLXvector3f[info->vertcount];
        self->tlst = new GLXvector2f[info->vertcount];
        self->clst = new GLXvector3f[info->vertcount];
        self->nlst = new GLXvector3f[info->vertcount];
    }
    catch (std::bad_alloc& ex)
    {
        goto mesh_mem_alloc_fail;
    }
    // Allocate per-tile memory.
    self->tile_list = ego_tile_info_create_ary(info->tiles_count);
    if (!self->tile_list)
    {
        goto mesh_mem_alloc_fail;
    }
    self->vert_count = info->vertcount;
    self->tile_count = info->tiles_count;

    return true;

mesh_mem_alloc_fail:

    tile_mem_t::free(self);
    log_error("%s:%d: unable to allocate tile memory - reduce the maximum number of vertices" \
              " (check MAP_VERTICES_MAX)\n", __FILE__, __LINE__);

    return false;
}

bool tile_mem_t::free(tile_mem_t *self)
{
    if (!self)
    {
        return false;
    }
    // Free the vertex data.
    if (self->plst)
    {
        delete[] self->plst;
        self->plst = nullptr;
    }
    if (self->nlst)
    {
        delete[] self->nlst;
        self->nlst = nullptr;
    }
    if (self->clst)
    {
        delete[] self->clst;
        self->clst = nullptr;
    }
    if (self->tlst)
    {
        delete[] self->tlst;
        self->tlst = nullptr;
    }

    // Free the tile data.
    ego_tile_info_destroy_ary(self->tile_list, self->tile_count);
    self->tile_list = nullptr;

    // Set the vertex count to 0.
    self->vert_count = 0;
    // Set the tile count to 0.
    self->tile_count = 0;

    return true;
}


//--------------------------------------------------------------------------------------------
oglx_texture_t *ego_mesh_get_texture(Uint8 image, Uint8 size)
{
    oglx_texture_t * tx_ptr = nullptr;

    if (0 == size)
    {
        tx_ptr = TextureAtlasManager::get_sml(image);
    }
    else if (1 == size)
    {
        tx_ptr = TextureAtlasManager::get_big(image);
    }

    return tx_ptr;
}

//--------------------------------------------------------------------------------------------
void mesh_texture_invalidate()
{
    mesh_tx_image = MESH_IMG_COUNT;
    mesh_tx_size  = 0xFF;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * mesh_texture_bind( const ego_tile_info_t * ptile )
{
    Uint8  tx_image, tx_size;
    oglx_texture_t  * tx_ptr = NULL;
    bool needs_bind = false;

    // bind a NULL texture if we are in that mode
    if ( mesh_tx_none )
    {
        tx_ptr = NULL;
        needs_bind = true;

        mesh_texture_invalidate();
    }
    else if ( NULL == ptile )
    {
        tx_ptr = NULL;
        needs_bind = true;

        mesh_texture_invalidate();
    }
    else
    {
        tx_image = TILE_GET_LOWER_BITS( ptile->img );
        tx_size  = ( ptile->type < tile_dict.offset ) ? 0 : 1;

        if (( mesh_tx_image != tx_image ) || ( mesh_tx_size != tx_size ) )
        {
            tx_ptr = ego_mesh_get_texture( tx_image, tx_size );
            needs_bind = true;

            mesh_tx_image = tx_image;
            mesh_tx_size  = tx_size;
        }
    }

    if ( needs_bind )
    {
        oglx_texture_t::bind( tx_ptr );
        if (tx_ptr && tx_ptr->hasAlpha())
        {
            // MH: Enable alpha blending if the texture requires it.
            Ego::Renderer::get().setBlendingEnabled(true);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        }
    }

    return tx_ptr;
}

ego_mesh_t::ego_mesh_t() :
    info(),
    tmem(),
    gmem(),
    fxlists()
{
    tile_mem_t::ctor(&tmem);
    grid_mem_t::ctor(&gmem);
    ego_mesh_info_t::ctor(&info);
    mpdfx_lists_t::ctor(&fxlists);

    // global initialization
    ego_mesh_make_twist();
}

ego_mesh_t::~ego_mesh_t()
{
    tile_mem_t::dtor(&tmem);
    grid_mem_t::dtor(&gmem);
    ego_mesh_info_t::dtor(&info);
}

ego_mesh_t::ego_mesh_t(int tiles_x, int tiles_y) : ego_mesh_t()
{
    // intitalize the mesh info using the max number of vertices for each tile
    ego_mesh_info_t::init(&info, -1, tiles_x, tiles_y);

    // allocate the mesh memory
    tile_mem_t::alloc( &tmem, &info );
    grid_mem_t::alloc( &gmem, &info );
    mpdfx_lists_t::alloc( &fxlists, &info );    
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::remove_ambient()
{
   
    Uint8 min_vrt_a = 255;

    /// @todo Use iterator.
    for (Uint32 i = 0; i < this->info.tiles_count; ++i)
    {
        min_vrt_a = std::min(min_vrt_a, grid_mem_t::get(&(this->gmem),TileIndex(i))->a);
    }

    /// @todo Use iterator.
    for (Uint32 i = 0; i < this->info.tiles_count; ++i)
    {
        grid_mem_t::get(&(this->gmem),TileIndex(i))->a =
            grid_mem_t::get(&(this->gmem),TileIndex(i))->a - min_vrt_a;
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::recalc_twist()
{
	ego_mesh_info_t *pinfo = &(this->info);
	tile_mem_t *ptmem = &(this->tmem);
	grid_mem_t *pgmem = &(this->gmem);

    // recalculate the twist
    for (TileIndex fan = 0; fan.getI() < pinfo->tiles_count; fan++)
    {
        Uint8 twist = cartman_get_fan_twist(this, fan);
        grid_mem_t::get(pgmem,fan)->twist = twist;
    }
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_set_texture(ego_mesh_t *self, const TileIndex& index, Uint16 image)
{
    Uint16 tile_value, tile_upper, tile_lower;

    if (!ego_mesh_t::grid_is_valid(self, index)) return false;
    ego_tile_info_t *pointer = tile_mem_t::get(&(self->tmem),index);

    // Get the upper and lower bits for this tile image.
    tile_value = pointer->img;
    tile_lower = image      & TILE_LOWER_MASK;
    tile_upper = tile_value & TILE_UPPER_MASK;

    // Set the actual image.
    pointer->img = tile_upper | tile_lower;

    // Update the pre-computed texture info.
    return ego_mesh_update_texture(self, index);
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_update_texture(ego_mesh_t *self, const TileIndex& index)
{
    size_t mesh_vrt;
    int    tile_vrt;
    Uint16 vertices;
    Uint16 image;
    Uint8  type;

    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mesh_info_t * pinfo;
    ego_tile_info_t * ptile;
    tile_definition_t * pdef;

    ptmem = &(self->tmem);
    pgmem = &(self->gmem);
    pinfo = &(self->info);

    if (!ego_mesh_t::grid_is_valid(self,index)) return false;
    ptile = tile_mem_t::get(ptmem,index);

    image = TILE_GET_LOWER_BITS( ptile->img );
    type  = ptile->type & 0x3F;

    pdef = TILE_DICT_PTR( tile_dict, type );
    if ( NULL == pdef ) return false;

    mesh_vrt = ptile->vrtstart;
    vertices = pdef->numvertices;
    for ( tile_vrt = 0; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
    {
        ptmem->tlst[mesh_vrt][SS] = pdef->u[tile_vrt];
        ptmem->tlst[mesh_vrt][TT] = pdef->v[tile_vrt];
    }

    return true;
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::make_texture()
{
    ego_mesh_info_t *info = &(this->info);

    // Set the texture coordinate for every vertex.
    for (TileIndex index = 0; index < info->tiles_count; index++)
    {
        ego_mesh_update_texture(this, index);
    }
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_t::finalize( ego_mesh_t * mesh )
{
    if ( NULL == mesh ) return NULL;

    ego_mesh_make_vrtstart( mesh );
	mesh->remove_ambient();
	mesh->recalc_twist();
    ego_mesh_make_normals( mesh );
    ego_mesh_make_bbox( mesh );
	mesh->make_texture();

    // create some lists to make searching the mesh tiles easier
    mpdfx_lists_t::synch( &( mesh->fxlists ), &( mesh->gmem ), true );

    return mesh;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_convert( ego_mesh_t * pmesh_dst, map_t * pmesh_src )
{
    tile_mem_t * ptmem_dst;
    grid_mem_t * pgmem_dst;
    mpdfx_lists_t * plists_dst;
    ego_mesh_info_t * pinfo_dst;
    bool allocated_dst;

    if ( NULL == pmesh_src ) return false;
    map_mem_t  *pmem_src = &(pmesh_src->_mem);
    map_info_t *pinfo_src = &(pmesh_src->_info);

    // clear out all data in the destination mesh
    *pmesh_dst = ego_mesh_t();
    ptmem_dst  = &( pmesh_dst->tmem );
    pgmem_dst  = &( pmesh_dst->gmem );
    plists_dst = &( pmesh_dst->fxlists );
    pinfo_dst  = &( pmesh_dst->info );

    // set up the destination mesh from the source mesh
    ego_mesh_info_t::init( pinfo_dst, pinfo_src->vertexCount, pinfo_src->tileCountX, pinfo_src->tileCountY );

    allocated_dst = tile_mem_t::alloc( ptmem_dst, pinfo_dst );
    if ( !allocated_dst ) return false;

    allocated_dst = grid_mem_t::alloc( pgmem_dst, pinfo_dst );
    if ( !allocated_dst ) return false;

    allocated_dst = mpdfx_lists_t::alloc( plists_dst, pinfo_dst );
    if ( !allocated_dst ) return false;

    // copy all the per-tile info
    for (Uint32 cnt = 0; cnt < pinfo_dst->tiles_count; cnt++)
    {
        tile_info_t& ptile_src = pmem_src->tiles[cnt];
        ego_tile_info_t *ptile_dst = tile_mem_t::get(ptmem_dst,cnt);
        ego_grid_info_t *pgrid_dst = grid_mem_t::get(pgmem_dst,cnt);

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        // BLANK_STRUCT_PTR( ptile_dst )
        ptile_dst->type         = ptile_src.type;
        ptile_dst->img          = ptile_src.img;

        // do not BLANK_STRUCT_PTR() here, since these were constructed when they were allocated
        // BLANK_STRUCT_PTR( pgrid_dst )
        pgrid_dst->base_fx = ptile_src.fx;
        pgrid_dst->twist   = ptile_src.twist;

        // set the local fx flags
        pgrid_dst->pass_fx = pgrid_dst->base_fx;

        // lcache is set in the constructor
        // nlst is set in the constructor
    }

    // copy all the per-vertex info
    for (Uint32 cnt = 0; cnt < pinfo_src->vertexCount; cnt++ )
    {
        GLXvector3f     * ppos_dst = ptmem_dst->plst + cnt;
        GLXvector3f     * pcol_dst = ptmem_dst->clst + cnt;
        const map_vertex_t& pvrt_src = pmem_src->vertices[cnt];

        // copy all info from map_mem_t
        ( *ppos_dst )[XX] = pvrt_src.pos[kX];
        ( *ppos_dst )[YY] = pvrt_src.pos[kY];
        ( *ppos_dst )[ZZ] = pvrt_src.pos[kZ];

        // default color
        ( *pcol_dst )[RR] = ( *pcol_dst )[GG] = ( *pcol_dst )[BB] = 0.0f;

        // tlist is set below
    }

    // copy some of the pre-calculated grid lighting
    for (Uint32 cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        size_t vertex = tile_mem_t::get(ptmem_dst,cnt)->vrtstart;
        ego_grid_info_t *pgrid_dst = grid_mem_t::get(pgmem_dst,cnt);
        const map_vertex_t& pvrt_src = pmem_src->vertices[vertex];

        pgrid_dst->a = pvrt_src.a;
        pgrid_dst->l = 0.0f;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
ego_mesh_t * ego_mesh_load( const char *modname, ego_mesh_t * mesh )
{
    // trap bad module names
    if ( !VALID_CSTR( modname ) ) return mesh;

    // initialize the mesh
    {
        // clear and free any memory that has been allocated
        *mesh = ego_mesh_t();
    }

    // actually do the loading
    {
        map_t local_mpd;

        // load a raw mpd
        tile_dictionary_load_vfs( "mp_data/fans.txt", &tile_dict, -1 );
        if (!local_mpd.load("mp_data/level.mpd"))
        {
            return nullptr;
        }

        // convert it into a convenient version for Egoboo
        if (!ego_mesh_convert(mesh, &local_mpd))
        {
            return nullptr;
        }
    }

    // do some calculation to set up the mpd as a game mesh
    mesh = ego_mesh_t::finalize( mesh );

    return mesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
grid_mem_t *grid_mem_t::ctor(grid_mem_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    BLANK_STRUCT_PTR(self);
    return self;
}

grid_mem_t *grid_mem_t::dtor(grid_mem_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    grid_mem_t::free(self);
    BLANK_STRUCT_PTR(self);
    return self;
}

bool grid_mem_t::alloc(grid_mem_t *self, const ego_mesh_info_t *info)
{
    if (!self || !info || 0 == info->vertcount)
    {
        return false;
    }
    // Free any memory already allocated.
    if (!grid_mem_t::free(self))
    {
        return false;
    }
    if (info->vertcount > MAP_VERTICES_MAX)
    {
        warnNumberOfVertices(__FILE__, __LINE__, info->vertcount);
        return false;
    }

    // Set the desired block number of grids.
    self->grids_x = info->tiles_x;
    self->grids_y = info->tiles_y;
    self->grid_count = self->grids_x * self->grids_y;

    // Set the mesh edge info.
    self->edge_x = (self->grids_x + 1) * GRID_ISIZE;
    self->edge_y = (self->grids_y + 1) * GRID_ISIZE;

    // Set the desired blocknumber of blocks.
    // This only works if BLOCK_BITS = GRID_BITS + 2.
    self->blocks_x = (info->tiles_x >> 2);
    if (HAS_SOME_BITS(info->tiles_x, 0x03))
    {
        self->blocks_x++;
    }
    self->blocks_y = (info->tiles_y >> 2);
    if (HAS_SOME_BITS(info->tiles_y, 0x03))
    {
        self->blocks_y++;
    }
    self->blocks_count = self->blocks_x * self->blocks_y;

    // Allocate per-grid memory.
    self->grid_list  = ego_grid_info_create_ary(info->tiles_count);
    if (!self->grid_list)
    {
        goto grid_mem_alloc_fail;
    }
    // Allocate the array for the block start data.
    try
    {
        self->blockstart = new Uint32[self->blocks_y];
    }
    catch (std::bad_alloc& ex)
    {
        goto grid_mem_alloc_fail;
    }
    
    // Allocate the array for the tile start data.
    try
    {
        self->tilestart = new Uint32[info->tiles_y];
    }
    catch (std::bad_alloc& ex)
    {
        goto grid_mem_alloc_fail;
    }

    // Compute the tile start/block start data.
    grid_mem_t::make_fanstart(self, info);

    return true;

grid_mem_alloc_fail:

    grid_mem_t::free(self);
    log_error("%s:%d: unable to allocate grid memory - reduce the maximum number of vertices" \
              " (check MAP_VERTICES_MAX)\n",__FILE__,__LINE__);

    return false;
}

bool grid_mem_t::free(grid_mem_t *self)
{
    if (!self)
    {
        return false;
    }
    // Free the block start and tile start arrays.
    if (self->blockstart)
    {
        delete[] self->blockstart;
        self->blockstart = nullptr;
    }
    if (self->tilestart)
    {
        delete[] self->tilestart;
        self->tilestart = nullptr;
    }

    // Destroy the grid list.
    ego_grid_info_destroy_ary(self->grid_list, self->grid_count);
    self->grid_list = nullptr;

    // Blank this struct.
    BLANK_STRUCT_PTR(self);

    return true;
}

void grid_mem_t::make_fanstart(grid_mem_t *self, const ego_mesh_info_t *info)
{
    if (!self || !info)
    {
        return;
    }
    // Compute look-up table for tile starts.
    for (int i = 0; i < info->tiles_y; i++)
    {
        self->tilestart[i] = info->tiles_x * i;
    }

    // Calculate some of the block info
    if (self->blocks_x >= GRID_BLOCKY_MAX)
    {
        log_warning("%s:%d: number of mesh blocks in the x direction too large (%d out of %d).\n", __FILE__,__LINE__,\
                    self->blocks_x, GRID_BLOCKY_MAX);
    }

    if (self->blocks_y >= GRID_BLOCKY_MAX)
    {
        log_warning("%s:%d: number of mesh blocks in the y direction too large (%d out of %d).\n", __FILE__, __LINE__,\
                    self->blocks_y, GRID_BLOCKY_MAX);
    }

    // Compute look-up table for block starts.
    for (int i = 0; i < self->blocks_y; i++)
    {
        self->blockstart[i] = self->blocks_x * i;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ego_mesh_make_vrtstart( ego_mesh_t * mesh )
{
    size_t vert;
    Uint32 tile;

    ego_mesh_info_t * pinfo;
    tile_mem_t  * ptmem;
    tile_definition_t * pdef;

    if ( NULL == mesh ) return;

    pinfo = &( mesh->info );
    ptmem  = &( mesh->tmem );

    vert = 0;
    for ( tile = 0; tile < pinfo->tiles_count; tile++ )
    {
        Uint8 ttype;

        tile_mem_t::get(ptmem,tile)->vrtstart = vert;

        ttype = tile_mem_t::get(ptmem,tile)->type;

        // throw away any remaining upper bits
        ttype &= 0x3F;

        pdef = TILE_DICT_PTR( tile_dict, ttype );
        if ( NULL == pdef ) continue;

        vert += pdef->numvertices;
    }

    if ( vert != pinfo->vertcount )
    {
        log_warning( "ego_mesh_make_vrtstart() - unexpected number of vertices %" PRIuZ " of %" PRIuZ "\n", vert, pinfo->vertcount );
    }
}

//--------------------------------------------------------------------------------------------
void ego_mesh_make_twist()
{
    /// @author ZZ
    /// @details This function precomputes surface normals and steep hill acceleration for
    ///    the mesh

    int     cnt;
    float   gdot;
    fvec3_t grav = fvec3_t::zero();

    grav[kZ] = Physics::g_environment.gravity;

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        fvec3_t   gperp;    // gravity perpendicular to the mesh
        fvec3_t   gpara;    // gravity parallel      to the mesh (what pushes you)
        fvec3_t   nrm;

        twist_to_normal( cnt, nrm, 1.0f );

        map_twist_nrm[cnt] = nrm;

        map_twist_facing_x[cnt] = ( FACING_T )( - vec_to_facing( nrm[kZ], nrm[kY] ) );
        map_twist_facing_y[cnt] = vec_to_facing( nrm[kZ], nrm[kX] );

        // this is about 5 degrees off of vertical
        map_twist_flat[cnt] = false;
        if ( nrm[kZ] > 0.9945f )
        {
            map_twist_flat[cnt] = true;
        }

        // projection of the gravity parallel to the surface
        gdot = grav[kZ] * nrm[kZ];

        gperp = nrm * gdot;

        gpara = grav - gperp;

        map_twist_vel[cnt] = gpara;
    }
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_make_bbox( ego_mesh_t * mesh )
{
    /// @author BB
    /// @details set the bounding box for each tile, and for the entire mesh

    size_t mesh_vrt;
    int tile_vrt;
    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mesh_info_t * pinfo;
    tile_definition_t * pdef;

    if ( NULL == mesh ) return false;
    ptmem  = &( mesh->tmem );
    pgmem = &( mesh->gmem );
    pinfo = &( mesh->info );

    ptmem->bbox = aabb_t(fvec3_t(ptmem->plst[0][XX], ptmem->plst[0][YY], ptmem->plst[0][ZZ]), 
                         fvec3_t(ptmem->plst[0][XX], ptmem->plst[0][YY], ptmem->plst[0][ZZ]));

	for (TileIndex cnt = 0; cnt.getI() < mesh->info.tiles_count; cnt++)
	{
		Uint16 vertices;
		Uint8 type;
		oct_vec_v2_t ovec;

        ego_tile_info_t *ptile = tile_mem_t::get(ptmem, cnt);
        oct_bb_t& poct = ptile->oct;

		type = ptile->type;
		type &= 0x3F;

		pdef = TILE_DICT_PTR(tile_dict, type);
		if (NULL == pdef) continue;

		mesh_vrt = tile_mem_t::get(ptmem,cnt)->vrtstart;    // Number of vertices
		vertices = pdef->numvertices;                 // Number of vertices

		// initialize the bounding box
	    ovec = oct_vec_v2_t(fvec3_t(ptmem->plst[mesh_vrt][0], ptmem->plst[mesh_vrt][1],ptmem->plst[mesh_vrt][2]));
        poct = oct_bb_t(ovec);
        mesh_vrt++;

        // add the rest of the points into the bounding box
        for ( tile_vrt = 1; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
        {
            ovec.ctor(fvec3_t(ptmem->plst[mesh_vrt][0],ptmem->plst[mesh_vrt][1],ptmem->plst[mesh_vrt][2]));
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
        ptmem->bbox = aabb_t(fvec3_t(std::min(ptmem->bbox.getMin()[XX], poct._mins[XX]),
                                     std::min(ptmem->bbox.getMin()[YY], poct._mins[YY]),
                                     std::min(ptmem->bbox.getMin()[ZZ], poct._mins[ZZ])),
                             fvec3_t(std::max(ptmem->bbox.getMax()[XX], poct._maxs[XX]),
                                     std::max(ptmem->bbox.getMax()[YY], poct._maxs[YY]),
                                     std::max(ptmem->bbox.getMax()[ZZ], poct._maxs[ZZ])));
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_make_normals( ego_mesh_t * mesh )
{
    /// @author BB
    /// @details this function calculates a set of normals for the 4 corners
    ///               of a given tile. It is supposed to generate smooth normals for
    ///               most tiles, but where there is a creas (i.e. between the floor and
    ///               a wall) the normals should not be smoothed.

    int ix, iy;
    tile_mem_t * ptmem;
    grid_mem_t * pgmem;

    int     edge_is_crease[4];
    fvec3_t nrm_lst[4], vec_sum;
    float   weight_lst[4];

    // test for mesh
    if ( NULL == mesh ) return false;
    ptmem = &( mesh->tmem );
    pgmem = &( mesh->gmem );

    // set the default normal for each fan, based on the calculated twist value
    for (TileIndex fan0 = 0; fan0 < ptmem->tile_count; fan0++ )
    {
        Uint8 twist = grid_mem_t::get(pgmem,fan0)->twist;

        ptmem->nlst[fan0.getI()][XX] = map_twist_nrm[twist][kX];
        ptmem->nlst[fan0.getI()][YY] = map_twist_nrm[twist][kY];
        ptmem->nlst[fan0.getI()][ZZ] = map_twist_nrm[twist][kZ];
    }

    // find an "average" normal of each corner of the tile
    for ( iy = 0; iy < mesh->info.tiles_y; iy++ )
    {
        for ( ix = 0; ix < mesh->info.tiles_x; ix++ )
        {
            int ix_off[4] = {0, 1, 1, 0};
            int iy_off[4] = {0, 0, 1, 1};
            int i, j, k;

            TileIndex fan0 = mesh->get_tile_int(PointGrid(ix, iy));
            if ( !ego_mesh_t::grid_is_valid( mesh, fan0 ) ) continue;

            nrm_lst[0][kX] = ptmem->nlst[fan0.getI()][XX];
            nrm_lst[0][kY] = ptmem->nlst[fan0.getI()][YY];
            nrm_lst[0][kZ] = ptmem->nlst[fan0.getI()][ZZ];

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

                    TileIndex fan1 = mesh->get_tile_int(PointGrid(jx, jy));

                    if ( ego_mesh_t::grid_is_valid( mesh, fan1 ) )
                    {
                        nrm_lst[j][kX] = ptmem->nlst[fan1.getI()][XX];
                        nrm_lst[j][kY] = ptmem->nlst[fan1.getI()][YY];
                        nrm_lst[j][kZ] = ptmem->nlst[fan1.getI()][ZZ];

                        if ( nrm_lst[j][kZ] < 0 )
                        {
                            nrm_lst[j] = -nrm_lst[j];
                        }
                    }
                    else
                    {
                        nrm_lst[j] = fvec3_t(0, 0, 1);
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

                tile_mem_t::get(ptmem,fan0)->ncache[i][XX] = vec_sum[kX];
                tile_mem_t::get(ptmem,fan0)->ncache[i][YY] = vec_sum[kY];
                tile_mem_t::get(ptmem,fan0)->ncache[i][ZZ] = vec_sum[kZ];
            }
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------
bool grid_light_one_corner( const ego_mesh_t * mesh, const TileIndex& fan, float height, float nrm[], float * plight )
{
    bool             reflective;
    lighting_cache_t * lighting  = NULL;
    ego_grid_info_t  * pgrid = NULL;

    // valid parameters?
    if ( NULL == plight || mesh == nullptr )
    {
        // not updated
        return false;
    }

    // valid grid?
    pgrid = mesh->get_pgrid(fan);
    if ( NULL == pgrid )
    {
        // not updated
        return false;
    }

#if 0
    // ignore caching for now
    // max update speed is once per game frame
    if ( pgrid->cache_frame >= 0 && ( Uint32 )pgrid->cache_frame >= game_frame_all )
    {
        // not updated
        return false;
    }
#endif

    // get the grid lighting
    lighting = &( pgrid->cache );

    reflective = ( 0 != ego_grid_info_t::test_all_fx( pgrid, MAPFX_REFLECTIVE ) );

    // evaluate the grid lighting at this node
    if ( reflective )
    {
        float light_dir, light_amb;

        lighting_evaluate_cache( lighting, fvec3_t(nrm[0],nrm[1],nrm[2]), height, mesh->tmem.bbox, &light_amb, &light_dir );

        // make ambient light only illuminate 1/2
        ( *plight ) = light_amb + 0.5f * light_dir;
    }
    else
    {
        ( *plight ) = lighting_evaluate_cache( lighting, fvec3_t(nrm[0],nrm[1],nrm[2]), height, mesh->tmem.bbox, NULL, NULL );
    }

    // clip the light to a reasonable value
    ( *plight ) = CLIP(( *plight ), 0.0f, 255.0f );

    // update the cache frame
#if 0
    // figure out the correct way to cache *plight
    pgrid->cache_frame = game_frame_all;
#endif

    return true;
}

//--------------------------------------------------------------------------------------------
void ego_mesh_t::test_one_corner(GLXvector3f pos, float *pdelta)
{
    float loc_delta, low_delta, hgh_delta;
    float hgh_wt, low_wt;

    if ( NULL == pdelta ) pdelta = &loc_delta;

    // interpolate the lighting for the given corner of the mesh
	*pdelta = grid_lighting_test(this, pos, &low_delta, &hgh_delta);

    // determine the weighting
	hgh_wt = (pos[ZZ] - this->tmem.bbox.getMin()[kZ]) / (this->tmem.bbox.getMax()[kZ] - this->tmem.bbox.getMin()[kZ]);
    hgh_wt = CLIP( hgh_wt, 0.0f, 1.0f );
    low_wt = 1.0f - hgh_wt;

    *pdelta = low_wt * low_delta + hgh_wt * hgh_delta;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_t::light_one_corner(ego_tile_info_t * ptile, const bool reflective, const fvec3_t& pos, const fvec3_t& nrm, float * plight )
{
    lighting_cache_t grid_light;

    if ( NULL == ptile ) return false;

    // interpolate the lighting for the given corner of the mesh
    grid_lighting_interpolate( this, &grid_light, fvec2_t(pos[kX],pos[kY]) );

    if ( reflective )
    {
        float light_dir, light_amb;

        lighting_evaluate_cache( &grid_light, nrm, pos[ZZ], tmem.bbox, &light_amb, &light_dir );

        // make ambient light only illuminate 1/2
        ( *plight ) = light_amb + 0.5f * light_dir;
    }
    else
    {
        ( *plight ) = lighting_evaluate_cache( &grid_light, nrm, pos[ZZ], tmem.bbox, NULL, NULL );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_test_corners( ego_mesh_t * mesh, ego_tile_info_t * ptile, float threshold )
{
    bool retval;
    int corner;

    tile_mem_t      * ptmem;
    light_cache_t   * lcache;
    light_cache_t   * d1_cache;

    // validate the parameters
    if ( NULL == mesh || NULL == ptile ) return false;

    if ( threshold < 0.0f ) threshold = 0.0f;

    // get the normal and lighting cache for this tile
    ptmem    = &( mesh->tmem );
    lcache   = &( ptile->lcache );
    d1_cache = &( ptile->d1_cache );

    retval = false;
    for ( corner = 0; corner < 4; corner++ )
    {
        float            delta;
        float          * pdelta;
        float          * plight;
        GLXvector3f    * ppos;

        pdelta = ( *d1_cache ) + corner;
        plight = ( *lcache ) + corner;
        ppos   = ptmem->plst + ptile->vrtstart + corner;

        mesh->test_one_corner(*ppos, &delta);

        if ( 0.0f == *plight )
        {
            delta = 10.0f;
        }
        else
        {
            delta /= *plight;
            delta = CLIP( delta, 0.0f, 10.0f );
        }

        *pdelta += delta;

        if ( *pdelta > threshold )
        {
            retval = true;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_light_corners( ego_mesh_t * mesh, ego_tile_info_t * ptile, bool reflective, float mesh_lighting_keep )
{
    int corner;
    float max_delta;

    tile_mem_t      * ptmem;
    normal_cache_t  * ncache;
    light_cache_t   * lcache;
    light_cache_t   * d1_cache, * d2_cache;

    // check for valid pointers
    if ( NULL == mesh || NULL == ptile )
    {
        return -1.0f;
    }

    // if no update is requested, return an "error value"
    if ( !ptile->request_lcache_update )
    {
        return -1.0f;
    };

    // has the lighting already been calculated this frame?
    if ( ptile->lcache_frame >= 0 && ( Uint32 )ptile->lcache_frame >= game_frame_all )
    {
        return -1.0f;
    }

    // get the normal and lighting cache for this tile
    ptmem    = &( mesh->tmem );
    ncache   = &( ptile->ncache );
    lcache   = &( ptile->lcache );
    d1_cache = &( ptile->d1_cache );
    d2_cache = &( ptile->d2_cache );

    max_delta = 0.0f;
    for ( corner = 0; corner < 4; corner++ )
    {
        float light_new, light_old, delta, light_tmp;

        GLXvector3f    * pnrm;
        float          * plight;
        float          * pdelta1, * pdelta2;
        GLXvector3f    * ppos;

        pnrm    = ( *ncache ) + corner;
        plight  = ( *lcache ) + corner;
        pdelta1 = ( *d1_cache ) + corner;
        pdelta2 = ( *d2_cache ) + corner;
        ppos    = ptmem->plst + ptile->vrtstart + corner;

        light_new = 0.0f;
        mesh->light_one_corner( ptile, reflective, fvec3_t((*ppos)[0],(*ppos)[1],(*ppos)[2]),
			                                                 fvec3_t((*pnrm)[0],(*pnrm)[1],(*pnrm)[2]), &light_new );

        if ( *plight != light_new )
        {
            light_old = *plight;
            *plight = light_old * mesh_lighting_keep + light_new * ( 1.0f - mesh_lighting_keep );

            // measure the actual delta
            delta = std::abs( light_old - *plight );

            // measure the relative change of the lighting
            light_tmp = 0.5f * (std::abs(*plight) + std::abs(light_old));
            if ( 0.0f == light_tmp )
            {
                delta = 10.0f;
            }
            else
            {
                delta /= light_tmp;
                delta = CLIP( delta, 0.0f, 10.0f );
            }

            // add in the actual change this update
            *pdelta2 += std::abs( delta );

            // update the estimate to match the actual change
            *pdelta1 = *pdelta2;
        }

        max_delta = std::max( max_delta, *pdelta1 );
    }

    // un-mark the lcache
    ptile->request_lcache_update = false;
    ptile->lcache_frame        = game_frame_all;

    return max_delta;
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
    poct = &( ptile->oct );
    lc   = &( ptile->lcache );

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

//--------------------------------------------------------------------------------------------
BIT_FIELD ego_mesh_test_wall(const ego_mesh_t *mesh, const fvec3_t& pos, const float radius, const BIT_FIELD bits, mesh_wall_data_t *pdata)
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
    if ( NULL == mesh || 0 == mesh->info.tiles_count || 0 == mesh->tmem.tile_count ) return EMPTY_BIT_FIELD;
    pdata->pinfo = (ego_mesh_info_t *)&(mesh->info);
    pdata->tlist = tile_mem_t::get(&(mesh->tmem),0);
    pdata->glist = grid_mem_t::get(&(mesh->gmem),0);

    // make an alias for the radius
    loc_radius = radius;

    // set a minimum radius
    if ( 0.0f == loc_radius )
    {
        loc_radius = GRID_FSIZE * 0.5f;
    }

    // make sure it is positive
    loc_radius = std::abs( loc_radius );

    pdata->fx_min = pos[kX] - loc_radius;
    pdata->fx_max = pos[kX] + loc_radius;

    pdata->fy_min = pos[kY] - loc_radius;
    pdata->fy_max = pos[kY] + loc_radius;

    // make a large limit in case the pos is so large that it cannot be represented by an int
    pdata->fx_min = std::max( pdata->fx_min, -9.0f * mesh->gmem.edge_x );
    pdata->fx_max = std::min( pdata->fx_max, 10.0f * mesh->gmem.edge_x );

    pdata->fy_min = std::max( pdata->fy_min, -9.0f * mesh->gmem.edge_y );
    pdata->fy_max = std::min( pdata->fy_max, 10.0f * mesh->gmem.edge_y );

    // find an integer bound.
    // we need to know about out of range values below clamp these to valid values
	ego_irect_t bound;
    bound.xmin = std::floor( pdata->fx_min / GRID_FSIZE );
    bound.xmax = std::floor( pdata->fx_max / GRID_FSIZE );
    bound.ymin = std::floor( pdata->fy_min / GRID_FSIZE );
    bound.ymax = std::floor( pdata->fy_max / GRID_FSIZE );

    // limit the test values to be in-bounds
    pdata->fx_min = std::max( pdata->fx_min, 0.0f );
    pdata->fx_max = std::min( pdata->fx_max, mesh->gmem.edge_x );
    pdata->fy_min = std::max( pdata->fy_min, 0.0f );
    pdata->fy_max = std::min( pdata->fy_max, mesh->gmem.edge_y );

    pdata->ix_min = std::max( bound.xmin, 0 );
    pdata->ix_max = std::min( bound.xmax, mesh->info.tiles_x - 1 );
    pdata->iy_min = std::max( bound.ymin, 0 );
    pdata->iy_max = std::min( bound.ymax, mesh->info.tiles_y - 1 );

    // clear the bit accumulator
    pass = 0;

    // detect out of bounds in the y-direction
    if ( bound.ymin < 0 || bound.ymax >= pdata->pinfo->tiles_y )
    {
        pass = ( MAPFX_IMPASS | MAPFX_WALL ) & bits;
        mesh_bound_tests++;
    }
    if ( EMPTY_BIT_FIELD != pass ) return pass;

    // detect out of bounds in the x-direction
    if ( bound.xmin < 0 || bound.xmax >= pdata->pinfo->tiles_x )
    {
        pass = ( MAPFX_IMPASS | MAPFX_WALL ) & bits;
        mesh_bound_tests++;
    }
    if ( EMPTY_BIT_FIELD != pass ) return pass;

    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        // since we KNOW that this is in range, allow raw access to the data strucutre
        int irow = mesh->gmem.tilestart[iy];

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            int itile = ix + irow;

            // since we KNOW that this is in range, allow raw access to the data strucutre
            pass = ego_grid_info_t::test_all_fx( pdata->glist + itile, bits );
            if ( 0 != pass )
            {
                return pass;
            }

            mesh_mpdfx_tests++;
        }
    }

    return pass;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_t::get_pressure( const ego_mesh_t * mesh, const fvec3_t& pos, float radius, const BIT_FIELD bits )
{
    const float tile_area = GRID_FSIZE * GRID_FSIZE;

    int   ix_min, ix_max, iy_min, iy_max;
    float fx_min, fx_max, fy_min, fy_max, obj_area;
    int ix, iy;

    float  loc_pressure, loc_radius;

    const ego_mesh_info_t  * pinfo;
    const ego_tile_info_t * tlist;
    const ego_grid_info_t * glist;

    // deal with the optional parameters
    loc_pressure = 0.0f;

    if (0 == bits) return 0;

    if ( NULL == mesh || 0 == mesh->info.tiles_count || 0 == mesh->tmem.tile_count ) return 0;
    pinfo = &( mesh->info );
    tlist = tile_mem_t::get(&(mesh->tmem),0);
    glist = grid_mem_t::get(&(mesh->gmem),0);

    // make an alias for the radius
    loc_radius = radius;

    // set a minimum radius
    if ( 0.0f == loc_radius )
    {
        loc_radius = GRID_FSIZE * 0.5f;
    }

    // make sure it is positive
    loc_radius = std::abs( loc_radius );

    fx_min = pos[kX] - loc_radius;
    fx_max = pos[kX] + loc_radius;

    fy_min = pos[kY] - loc_radius;
    fy_max = pos[kY] + loc_radius;

    obj_area = ( fx_max - fx_min ) * ( fy_max - fy_min );

    ix_min = std::floor( fx_min / GRID_FSIZE );
    ix_max = std::floor( fx_max / GRID_FSIZE );

    iy_min = std::floor( fy_min / GRID_FSIZE );
    iy_max = std::floor( fy_max / GRID_FSIZE );

    for ( iy = iy_min; iy <= iy_max; iy++ )
    {
        float ty_min, ty_max;

        bool tile_valid = true;

        ty_min = ( iy + 0 ) * GRID_FSIZE;
        ty_max = ( iy + 1 ) * GRID_FSIZE;

        if ( iy < 0 || iy >= pinfo->tiles_y )
        {
            tile_valid = false;
        }

        for ( ix = ix_min; ix <= ix_max; ix++ )
        {
            bool is_blocked = false;
            float tx_min, tx_max;

            float area_ratio;
            float ovl_x_min, ovl_x_max;
            float ovl_y_min, ovl_y_max;

            tx_min = ( ix + 0 ) * GRID_FSIZE;
            tx_max = ( ix + 1 ) * GRID_FSIZE;

            if ( ix < 0 || ix >= pinfo->tiles_x )
            {
                tile_valid = false;
            }

            if ( tile_valid )
            {
                TileIndex itile = mesh->get_tile_int(PointGrid(ix, iy));
                tile_valid = ego_mesh_t::grid_is_valid( mesh, itile );
                if ( !tile_valid )
                {
                    is_blocked = true;
                }
                else
                {
                    is_blocked = ( 0 != ego_grid_info_t::test_all_fx( glist + itile.getI(), bits ) );
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

                mesh_pressure_tests++;
            }
        }
    }

    return loc_pressure;
}

//--------------------------------------------------------------------------------------------
fvec3_t ego_mesh_t::get_diff(const ego_mesh_t *mesh, const fvec3_t& pos, float radius, float center_pressure, const BIT_FIELD bits )
{
    /// @author BB
    /// @details determine the shortest "way out", but creating an array of "pressures"
    /// with each element representing the pressure when the object is moved in different directions
    /// by 1/2 a tile.

    const float jitter_size = GRID_FSIZE * 0.5f;
    float pressure_ary[9];
    float fx, fy;
    fvec3_t diff = fvec3_t::zero();
    float   sum_diff = 0.0f;
    float   dpressure;

    int cnt;

    // Find the pressure for the 9 points of jittering around the current position.
    pressure_ary[4] = center_pressure;
    for ( cnt = 0, fy = pos[kY] - jitter_size; fy <= pos[kY] + jitter_size; fy += jitter_size )
    {
        for ( fx = pos[kX] - jitter_size; fx <= pos[kX] + jitter_size; fx += jitter_size, cnt++ )
        {
            fvec3_t jitter_pos(fx,fy,0.0f);
            if (4 == cnt) continue;
            pressure_ary[cnt] = ego_mesh_t::get_pressure(mesh, jitter_pos, radius, bits);
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

                fvec2_t tmp(dist * fx, dist * fy);

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

//--------------------------------------------------------------------------------------------
BIT_FIELD ego_mesh_hit_wall( const ego_mesh_t * mesh, const fvec3_t& pos, const float radius, const BIT_FIELD bits, fvec2_t& nrm, float * pressure, mesh_wall_data_t * pdata )
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

    nrm = fvec2_t::zero();

    // if pdata is not NULL, someone has already run a version of mesh_test_wall
    if ( NULL == pdata )
    {
        pdata = &loc_data;

        // Do the simplest test.
        // Initializes the shared mesh_wall_data_t struct, so no need to do it again
        // Eliminates all cases of bad source data, so no need to test them again.
        if ( 0 == ego_mesh_test_wall( mesh, pos, radius, bits, pdata ) ) return 0;
    }

    // ego_mesh_test_wall() clamps pdata->ix_* and pdata->iy_* to valid values

    pass = loc_pass = 0;
    nrm[kX] = nrm[kY] = 0.0f;
    for ( iy = pdata->iy_min; iy <= pdata->iy_max; iy++ )
    {
        float ty_min, ty_max;

        invalid = false;

        ty_min = ( iy + 0 ) * GRID_FSIZE;
        ty_max = ( iy + 1 ) * GRID_FSIZE;

        if ( iy < 0 || iy >= pdata->pinfo->tiles_y )
        {
            loc_pass |= ( MAPFX_IMPASS | MAPFX_WALL );

            if ( needs_nrm )
            {
                nrm[kY] += pos[kY] - ( ty_max + ty_min ) * 0.5f;
            }

            invalid = true;
            mesh_bound_tests++;
        }

        for ( ix = pdata->ix_min; ix <= pdata->ix_max; ix++ )
        {
            float tx_min, tx_max;

            tx_min = ( ix + 0 ) * GRID_FSIZE;
            tx_max = ( ix + 1 ) * GRID_FSIZE;

            if ( ix < 0 || ix >= pdata->pinfo->tiles_x )
            {
                loc_pass |=  MAPFX_IMPASS | MAPFX_WALL;

                if ( needs_nrm )
                {
                    nrm[kX] += pos[kX] - ( tx_max + tx_min ) * 0.5f;
                }

                invalid = true;
                mesh_bound_tests++;
            }

            if ( !invalid )
            {
                TileIndex itile = mesh->get_tile_int(PointGrid(ix, iy));
                if ( ego_mesh_t::grid_is_valid( mesh, itile ) )
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
        nrm = fvec2_t::zero();
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
            *pressure = ego_mesh_t::get_pressure( mesh, pos, radius, bits );
        }
    }

    return pass;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float ego_mesh_get_max_vertex_0(const ego_mesh_t *self, const PointGrid& point)
{
    int type;
    Uint32 cnt;
    float zmax;
    size_t vcount, vstart, ivrt;

    if (!self) return 0.0f;

    TileIndex itile = self->get_tile_int(point);
    if (TileIndex::Invalid == itile)
    {
        return 0.0f;
    }
    // get a pointer to the tile
    ego_tile_info_t *ptile = tile_mem_t::get(&(self->tmem), itile);

    type   = ptile->type;
    vstart = ptile->vrtstart;
    vcount = std::min(static_cast<size_t>(4), self->tmem.vert_count);

    ivrt = vstart;
    zmax = self->tmem.plst[ivrt][ZZ];
    for ( ivrt++, cnt = 1; cnt < vcount; ivrt++, cnt++ )
    {
        zmax = std::max( zmax, self->tmem.plst[ivrt][ZZ] );
    }

    return zmax;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_get_max_vertex_1( const ego_mesh_t * mesh, const PointGrid& point, float xmin, float ymin, float xmax, float ymax )
{
    Uint32 cnt;
    int type;
    float zmax;
    size_t vcount, vstart, ivrt;

    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    if ( NULL == mesh ) return 0.0f;

    TileIndex itile = mesh->get_tile_int( point );

    if (TileIndex::Invalid == itile) return 0.0f;

    type   = tile_mem_t::get(&(mesh->tmem),itile)->type;
    vstart = tile_mem_t::get(&(mesh->tmem),itile)->vrtstart;
    vcount = std::min( (size_t)4, mesh->tmem.vert_count );

    zmax = -1e6;
    for ( ivrt = vstart, cnt = 0; cnt < vcount; ivrt++, cnt++ )
    {
        float fx, fy;
        GLXvector3f * pvert = mesh->tmem.plst + ivrt;

        // we are evaluating the height based on the grid, not the actual vertex positions
        fx = ( point.getX() + ix_off[cnt] ) * GRID_FSIZE;
        fy = ( point.getY() + iy_off[cnt] ) * GRID_FSIZE;

        if ( fx >= xmin && fx <= xmax && fy >= ymin && fy <= ymax )
        {
            zmax = std::max( zmax, ( *pvert )[ZZ] );
        }
    }

    if ( -1e6 == zmax ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
// ego_tile_info_t
//--------------------------------------------------------------------------------------------
ego_tile_info_t *ego_tile_info_t::ctor(ego_tile_info_t *self, int index)
{
    if (!self)
    {
        return nullptr;
    }
    BLANK_STRUCT_PTR(self);

    // Set the non-zero, non-null, non-false values.
    self->fanoff = true;
    self->inrenderlist_frame = -1;

    self->request_lcache_update = true;
    self->lcache_frame = -1;

    self->request_clst_update = false;
    self->clst_frame = -1;

    self->bsp_leaf.set(self, BSP_LEAF_TILE, index);

    return self;
}

ego_tile_info_t *ego_tile_info_t::dtor(ego_tile_info_t * self)
{
    if (!self)
    {
        return nullptr;
    }

    self = ego_tile_info_t::free(self);

    BLANK_STRUCT_PTR(self);

    return self;
}

ego_tile_info_t *ego_tile_info_t::free(ego_tile_info_t * self)
{
    if (!self)
    {
        return nullptr;
    }

    // Delete any dynamically allocated data.

    return self;
}

ego_tile_info_t *ego_tile_info_t::create(int index)
{
    ego_tile_info_t *self = EGOBOO_NEW(ego_tile_info_t);
    if (!self)
    {
        return nullptr;
    }
    return ego_tile_info_t::ctor(self, index);
}

ego_tile_info_t *ego_tile_info_t::destroy(ego_tile_info_t * self)
{
    if (!self)
    {
        return nullptr;
    }
    self = ego_tile_info_t::dtor(self);

    EGOBOO_DELETE(self);

    return self;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t *ego_tile_info_ctor_ary(ego_tile_info_t *self, size_t size)
{
    if (!self)
    {
        return nullptr;
    }
    for (size_t i = 0; i < size; i++)
    {
        ego_tile_info_t::ctor(self + i, i);
    }

    return self;
}

ego_tile_info_t * ego_tile_info_dtor_ary(ego_tile_info_t *self, size_t size)
{
    if (!self)
    {
        return nullptr;
    }
    for (size_t i = 0; i < size; ++i)
    {
        ego_tile_info_t::dtor(self + i);
    }

    return self;
}

ego_tile_info_t * ego_tile_info_create_ary(size_t size)
{
    ego_tile_info_t *self = EGOBOO_NEW_ARY(ego_tile_info_t, size);
    if (!self)
    {
        return nullptr;
    }
    return ego_tile_info_ctor_ary(self, size);
}

ego_tile_info_t *ego_tile_info_destroy_ary(ego_tile_info_t *self, size_t size)
{
    if (!self)
    {
        return nullptr;
    }
    ego_tile_info_ctor_ary(self, size);

    EGOBOO_DELETE_ARY(self);

    return self;
}

//--------------------------------------------------------------------------------------------
// ego_grid_info_t
//--------------------------------------------------------------------------------------------
ego_grid_info_t *ego_grid_info_t::ctor(ego_grid_info_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    BLANK_STRUCT_PTR(self);

    // Set to non-zero, non-null, non-false values.
    self->cache_frame = -1;
    self->twist = TWIST_FLAT;

    return self;
}

ego_grid_info_t *ego_grid_info_t::dtor(ego_grid_info_t * self)
{
    if (!self)
    {
        return nullptr;
    }

    ego_grid_info_t::free(self);

    BLANK_STRUCT_PTR(self);

    return self;
}

ego_grid_info_t *ego_grid_info_t::free(ego_grid_info_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    // Deallocate any dynamically allocated data (nothing to do yet).

    return self;
}

ego_grid_info_t *ego_grid_info_t::create()
{
    ego_grid_info_t *self = EGOBOO_NEW(ego_grid_info_t);
    if (!self)
    {
        return nullptr;
    }
    return ego_grid_info_t::ctor(self);
}

ego_grid_info_t *ego_grid_info_t::destroy(ego_grid_info_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    self = ego_grid_info_t::dtor(self);

    EGOBOO_DELETE(self);

    return self;
}

//--------------------------------------------------------------------------------------------
ego_grid_info_t *ego_grid_info_ctor_ary(ego_grid_info_t *self, size_t size)
{
    if (!self)
    {
        return nullptr;
    }
    for (size_t i = 0; i < size; i++)
    {
        ego_grid_info_t::ctor(self + i);
    }

    return self;
}

ego_grid_info_t *ego_grid_info_dtor_ary(ego_grid_info_t *self, size_t size)
{
    if (!self)
    {
        return nullptr;
    }

    for (size_t i = 0; i < size; ++i)
    {
        ego_grid_info_t::dtor(self + i);
    }

    return self;
}

ego_grid_info_t *ego_grid_info_create_ary(size_t size)
{
    ego_grid_info_t *self = EGOBOO_NEW_ARY(ego_grid_info_t, size);
    if (!self)
    {
        return nullptr;
    }
    return ego_grid_info_ctor_ary(self, size);
}

ego_grid_info_t *ego_grid_info_destroy_ary(ego_grid_info_t *self, size_t size)
{
    if (!self)
    {
        return nullptr;
    }

    self = ego_grid_info_dtor_ary(self, size);

    EGOBOO_DELETE_ARY(self);

    return self;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_update_water_level( ego_mesh_t * mesh )
{
    // BB>
    // TODO: WHEN we begin using the map_BSP for frustum culling, we need to
    // update the bounding box height for every single water tile and then re-insert them in the mpd
    // AT THE MOMENT, this is not done and the increased bounding height due to water is handled elsewhere

    if ( NULL == mesh ) return false;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpdfx_list_ary_t *mpdfx_list_ary_t::ctor(mpdfx_list_ary_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    BLANK_STRUCT_PTR(self);

    return self;
}

mpdfx_list_ary_t *mpdfx_list_ary_t::dtor(mpdfx_list_ary_t *self)
{
    if (!self)
    {
        return nullptr;
    }

    mpdfx_list_ary_t::dealloc(self);

    BLANK_STRUCT_PTR(self);

    return self;
}

mpdfx_list_ary_t *mpdfx_list_ary_t::alloc(mpdfx_list_ary_t *self, size_t size)
{
    if (!self)
    {
        return nullptr;
    }

    mpdfx_list_ary_t::dealloc(self);

    if (0 == size)
    {
        return self;
    }
    self->lst = EGOBOO_NEW_ARY(size_t, size);
    self->cnt = (!self->lst) ? 0 : size;
    self->idx = 0;
    return self;
}

mpdfx_list_ary_t *mpdfx_list_ary_t::dealloc(mpdfx_list_ary_t * self)
{
    if (!self)
    {
        return nullptr;
    }
    if (0 == self->cnt)
    {
        return self;
    }
    EGOBOO_DELETE_ARY(self->lst);
    self->cnt = 0;
    self->idx = 0;
    return self;
}

mpdfx_list_ary_t *mpdfx_list_ary_t::reset(mpdfx_list_ary_t *self)
{
    if (!self)
    {
        return nullptr;
    }
    self->idx = 0;
    return self;
}

bool mpdfx_list_ary_t::push(mpdfx_list_ary_t *self, size_t value)
{
    if (!self)
    {
        return false;
    }
    if (self->idx < self->cnt)
    {
        self->lst[self->idx] = value;
        self->idx++;
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
mpdfx_lists_t * mpdfx_lists_t::ctor( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return plst;

    BLANK_STRUCT_PTR( plst );

    mpdfx_list_ary_t::ctor( &( plst->sha ) );
    mpdfx_list_ary_t::ctor( &( plst->drf ) );
    mpdfx_list_ary_t::ctor( &( plst->anm ) );
    mpdfx_list_ary_t::ctor( &( plst->wat ) );
    mpdfx_list_ary_t::ctor( &( plst->wal ) );
    mpdfx_list_ary_t::ctor( &( plst->imp ) );
    mpdfx_list_ary_t::ctor( &( plst->dam ) );
    mpdfx_list_ary_t::ctor( &( plst->slp ) );

    return plst;
}

//--------------------------------------------------------------------------------------------
mpdfx_lists_t * mpdfx_lists_t::dtor( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return NULL;

    mpdfx_lists_t::dealloc( plst );

    mpdfx_list_ary_t::dtor( &( plst->sha ) );
    mpdfx_list_ary_t::dtor( &( plst->drf ) );
    mpdfx_list_ary_t::dtor( &( plst->anm ) );
    mpdfx_list_ary_t::dtor( &( plst->wat ) );
    mpdfx_list_ary_t::dtor( &( plst->wal ) );
    mpdfx_list_ary_t::dtor( &( plst->imp ) );
    mpdfx_list_ary_t::dtor( &( plst->dam ) );
    mpdfx_list_ary_t::dtor( &( plst->slp ) );

    BLANK_STRUCT_PTR( plst )

    return plst;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_t::alloc( mpdfx_lists_t * plst, const ego_mesh_info_t * pinfo )
{
    if ( NULL == plst || NULL == pinfo ) return false;

    // free any memory already allocated
    if ( !mpdfx_lists_t::dealloc( plst ) ) return false;

    if ( 0 == pinfo->tiles_count ) return true;

    mpdfx_list_ary_t::alloc( &( plst->sha ), pinfo->tiles_count );
    if ( NULL == plst->sha.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_t::alloc( &( plst->drf ), pinfo->tiles_count );
    if ( NULL == plst->drf.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_t::alloc( &( plst->anm ), pinfo->tiles_count );
    if ( NULL == plst->anm.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_t::alloc( &( plst->wat ), pinfo->tiles_count );
    if ( NULL == plst->wat.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_t::alloc( &( plst->wal ), pinfo->tiles_count );
    if ( NULL == plst->wal.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_t::alloc( &( plst->imp ), pinfo->tiles_count );
    if ( NULL == plst->imp.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_t::alloc( &( plst->dam ), pinfo->tiles_count );
    if ( NULL == plst->dam.lst ) goto mesh_mem_alloc_fail;

    mpdfx_list_ary_t::alloc( &( plst->slp ), pinfo->tiles_count );
    if ( NULL == plst->slp.lst ) goto mesh_mem_alloc_fail;

    // the list needs to be resynched
    plst->dirty = true;

    return true;

mesh_mem_alloc_fail:

    mpdfx_lists_t::dealloc( plst );

    log_error( "%s - cannot allocate mpdfx_lists_t for this mesh!\n", __FUNCTION__ );

    return false;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_t::dealloc( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return false;

    // free the memory
    mpdfx_list_ary_t::dealloc( &( plst->sha ) );
    mpdfx_list_ary_t::dealloc( &( plst->drf ) );
    mpdfx_list_ary_t::dealloc( &( plst->anm ) );
    mpdfx_list_ary_t::dealloc( &( plst->wat ) );
    mpdfx_list_ary_t::dealloc( &( plst->wal ) );
    mpdfx_list_ary_t::dealloc( &( plst->imp ) );
    mpdfx_list_ary_t::dealloc( &( plst->dam ) );
    mpdfx_list_ary_t::dealloc( &( plst->slp ) );

    // no memory, so nothing can be stored and nothing can be dirty
    plst->dirty = false;

    return true;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_t::reset( mpdfx_lists_t * plst )
{
    if ( NULL == plst ) return false;

    // free the memory
    mpdfx_list_ary_t::reset( &( plst->sha ) );
    mpdfx_list_ary_t::reset( &( plst->drf ) );
    mpdfx_list_ary_t::reset( &( plst->anm ) );
    mpdfx_list_ary_t::reset( &( plst->wat ) );
    mpdfx_list_ary_t::reset( &( plst->wal ) );
    mpdfx_list_ary_t::reset( &( plst->imp ) );
    mpdfx_list_ary_t::reset( &( plst->dam ) );
    mpdfx_list_ary_t::reset( &( plst->slp ) );

    // everything has been reset. force it to recalculate
    plst->dirty = true;

    return true;
}

//--------------------------------------------------------------------------------------------
int mpdfx_lists_t::push( mpdfx_lists_t * plst, GRID_FX_BITS fx_bits, size_t value )
{
    int retval = 0;

    if ( NULL == plst ) return false;

    if ( 0 == fx_bits ) return true;

    if ( HAS_NO_BITS( fx_bits, MAPFX_SHA ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->sha ), value ) )
        {
            retval++;
        }

    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_REFLECTIVE ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->drf ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_ANIM ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->anm ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WATER ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->wat ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_WALL ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->wal ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_IMPASS ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->imp ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_DAMAGE ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->dam ), value ) )
        {
            retval++;
        }
    }

    if ( HAS_ALL_BITS( fx_bits, MAPFX_SLIPPY ) )
    {
        if ( mpdfx_list_ary_t::push( &( plst->slp ), value ) )
        {
            retval++;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool mpdfx_lists_t::synch( mpdfx_lists_t * plst, const grid_mem_t * pgmem, bool force )
{
    size_t count, i;
    GRID_FX_BITS fx;

    if ( NULL == plst || NULL == pgmem ) return false;

    count =  pgmem->grid_count;
    if ( 0 == count ) return true;

    // don't re-calculate unless it is necessary
    if ( !force && !plst->dirty ) return true;

    // !!reset the counts!!
    mpdfx_lists_t::reset( plst );

    for ( i = 0; i < count; i++ )
    {
        fx = ego_grid_info_t::get_all_fx(grid_mem_t::get(pgmem,i));

        mpdfx_lists_t::push( plst, fx, i );
    }

    // we're done calculating
    plst->dirty = false;

    return true;
}


//--------------------------------------------------------------------------------------------
//Previously inlined
//--------------------------------------------------------------------------------------------
bool ego_mesh_tile_has_bits( const ego_mesh_t * mesh, const PointGrid& point, const BIT_FIELD bits )
{
	if (!mesh) {
		throw std::invalid_argument("nullptr == mesh");
	}
    // Figure out which tile we are on.
    TileIndex tileRef = mesh->get_tile_int(point);

    // Everything outside the map bounds is wall and impassable.
    if (!ego_mesh_t::grid_is_valid(mesh, tileRef))
    {
        return HAS_SOME_BITS((MAPFX_IMPASS | MAPFX_WALL), bits);
    }

    // Since we KNOW that this is in range, allow raw access to the data structure.
    GRID_FX_BITS fx = ego_grid_info_t::get_all_fx(grid_mem_t::get(&(mesh->gmem),tileRef));

    return HAS_SOME_BITS(fx, bits);
}

//--------------------------------------------------------------------------------------------
Uint32 ego_mesh_has_some_mpdfx( const BIT_FIELD mpdfx, const BIT_FIELD test )
{
    mesh_mpdfx_tests++;
    return HAS_SOME_BITS( mpdfx, test );
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_t::grid_is_valid(const ego_mesh_t *self, const TileIndex& index)
{
    if (!self)
    {
        return false;
    }
    mesh_bound_tests++;

    if (TileIndex::Invalid == index)
    {
        return false;
    }
    return index.getI() < self->info.tiles_count;
}

//--------------------------------------------------------------------------------------------
float ego_mesh_t::getElevation(const PointWorld& point) const
{
    TileIndex tile = this->get_grid(point);
    if (!ego_mesh_t::grid_is_valid(this, tile)) return 0;

    PointGrid gridPoint(static_cast<int>(point.getX()) & GRID_MASK,
                        static_cast<int>(point.getY()) & GRID_MASK);

    // Get the height of each fan corner.
    float z0 = tmem.plst[tile_mem_t::get(&tmem, tile)->vrtstart + 0][ZZ];
    float z1 = tmem.plst[tile_mem_t::get(&tmem, tile)->vrtstart + 1][ZZ];
    float z2 = tmem.plst[tile_mem_t::get(&tmem, tile)->vrtstart + 2][ZZ];
    float z3 = tmem.plst[tile_mem_t::get(&tmem, tile)->vrtstart + 3][ZZ];

    // Get the weighted height of each side.
    float zleft = (z0 * (GRID_FSIZE - gridPoint.getY()) + z3 * gridPoint.getY()) / GRID_FSIZE;
    float zright = (z1 * (GRID_FSIZE - gridPoint.getY()) + z2 * gridPoint.getY()) / GRID_FSIZE;
    float zdone = (zleft * (GRID_FSIZE - gridPoint.getX()) + zright * gridPoint.getX()) / GRID_FSIZE;

    return zdone;
}

//--------------------------------------------------------------------------------------------
BlockIndex ego_mesh_t::get_block(const PointWorld& point) const
{
	if (point.getX() >= 0.0f && point.getX() <= this->gmem.edge_x && point.getY() >= 0.0f && point.getY() <= this->gmem.edge_y)
    {
        PointBlock blockPoint(static_cast<int>(point.getX()) / BLOCK_ISIZE,
                              static_cast<int>(point.getY()) / BLOCK_ISIZE);
		return this->get_block_int(blockPoint);
    }

    return BlockIndex::Invalid;
}

TileIndex ego_mesh_t::get_grid(const PointWorld& point) const
{
    if (point.getX() >= 0.0f && point.getX() < this->gmem.edge_x && point.getY() >= 0.0f && point.getY() < this->gmem.edge_y)
    {
        // By the above, point.getX() and point.getY() are positive, hence the right shift is not a problem.
        // point.these are known to be positive, so >> is not a problem
        PointGrid gridPoint(static_cast<int>(point.getX()) >> GRID_BITS, 
                            static_cast<int>(point.getY()) >> GRID_BITS);
        return this->get_tile_int(gridPoint);
    }
    return TileIndex::Invalid;
}

BlockIndex ego_mesh_t::get_block_int(const PointBlock& point) const
{
    if (point.getX() < 0 || point.getX() >= this->gmem.blocks_x)
    {
        return BlockIndex::Invalid;
    }
    if (point.getY() < 0 || point.getY() >= this->gmem.blocks_y)
    {
        return BlockIndex::Invalid;
    }
    return point.getX() + this->gmem.blockstart[point.getY()];
}

TileIndex ego_mesh_t::get_tile_int(const PointGrid& point) const
{
    if (point.getX() < 0 || point.getX() >= this->info.tiles_x)
    {
        return TileIndex::Invalid;
    }
	if (point.getY() < 0 || point.getY() >= this->info.tiles_y)
    {
        return TileIndex::Invalid;
    }
	return point.getX() + this->gmem.tilestart[point.getY()];
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_clear_fx( ego_mesh_t * mesh, const TileIndex& itile, const BIT_FIELD flags )
{
    bool retval;

    // test for mesh
    if ( NULL == mesh ) return false;

    // test for invalid tile
    mesh_bound_tests++;
    if ( itile > mesh->info.tiles_count ) return false;

    mesh_mpdfx_tests++;
    retval = ego_grid_info_sub_pass_fx(grid_mem_t::get(&mesh->gmem,itile), flags );

    if ( retval )
    {
        mesh->fxlists.dirty = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool ego_mesh_add_fx(ego_mesh_t *self, const TileIndex& index, const BIT_FIELD flags)
{
    // Validate mesh.
    if (!self)
    {
        return false;
    }

    // Validate tile index.
    mesh_bound_tests++;
    if (index > self->info.tiles_count)
    {
        return false;
    }

    // Succeed only of something actually changed.
    mesh_mpdfx_tests++;
    bool retval = ego_grid_info_add_pass_fx(grid_mem_t::get(&(self->gmem),index), flags);

    if ( retval )
    {
        self->fxlists.dirty = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
Uint32 ego_mesh_t::test_fx(const ego_mesh_t *self, const TileIndex& index, const BIT_FIELD flags)
{
    // test for mesh
    if (!self) return 0;

    // test for a trivial value of flags
    if (EMPTY_BIT_FIELD == flags) return 0;

    // test for invalid tile
    mesh_bound_tests++;
    if (index > self->info.tiles_count)
    {
        return flags & ( MAPFX_WALL | MAPFX_IMPASS );
    }

    // if the tile is actually labelled as MAP_FANOFF, ignore it completely
    if (TILE_IS_FANOFF(tile_mem_t::get(&(self->tmem),index)))
    {
        return 0;
    }

    mesh_mpdfx_tests++;
    return ego_grid_info_t::test_all_fx(grid_mem_t::get(&(self->gmem),index), flags);
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t* ego_mesh_t::get_ptile(const TileIndex& index) const
{
    // Validate mesh and tile index.
    if (index.getI() >= info.tiles_count)
    {
        return nullptr;
    }

    // Get the tile info.
    return tile_mem_t::get(&tmem, index);
}

ego_grid_info_t* ego_mesh_t::get_pgrid(const TileIndex& index) const
{
    // Validate mesh and grid index.
    if (index.getI() >= info.tiles_count)
    {
        return nullptr;
    }

    // Get the grid info.
    return grid_mem_t::get(&gmem, index);
}

//--------------------------------------------------------------------------------------------
Uint8 ego_mesh_get_twist(ego_mesh_t *self, const TileIndex& index)
{
    // Validate arguments.
    if (!self || index >= self->info.tiles_count)
    {
        return TWIST_FLAT;
    }
    return grid_mem_t::get(&(self->gmem), index)->twist;
#if 0
    // Assert that the grids are allocated.
    if (!self->gmem.grid_list || index.getI() >= self->gmem.grid_count)
    {
        return TWIST_FLAT;
    }
    return self->gmem.grid_list[index].twist;
#endif
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GRID_FX_BITS ego_grid_info_t::get_all_fx(const ego_grid_info_t *self)
{
    if (!self) return MAPFX_WALL | MAPFX_IMPASS;

    return self->pass_fx;
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

//--------------------------------------------------------------------------------------------
bool ego_grid_info_add_pass_fx(ego_grid_info_t *self, const GRID_FX_BITS bits)
{
    GRID_FX_BITS old_bits, new_bits;

    if (!self) return false;

    // save the old bits
    old_bits = ego_grid_info_t::get_all_fx(self);

    // set the bits that we can modify
    SET_BIT(self->pass_fx, bits);

    // get the new bits
    new_bits = ego_grid_info_t::get_all_fx(self);

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

//--------------------------------------------------------------------------------------------
bool ego_grid_info_sub_pass_fx( ego_grid_info_t * pgrid, const GRID_FX_BITS bits )
{
    GRID_FX_BITS old_bits, new_bits;

    if ( NULL == pgrid ) return false;

    // save the old bits
    old_bits = ego_grid_info_t::get_all_fx( pgrid );

    // set the bits that we can modify
    UNSET_BIT( pgrid->pass_fx, bits );

    // get the new bits
    new_bits = ego_grid_info_t::get_all_fx( pgrid );

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

//--------------------------------------------------------------------------------------------
bool ego_grid_info_set_pass_fx( ego_grid_info_t * pgrid, const GRID_FX_BITS bits )
{
    GRID_FX_BITS old_bits, new_bits;

    if ( NULL == pgrid ) return false;

    // save the old bits
    old_bits = ego_grid_info_t::get_all_fx( pgrid );

    // set the bits that we can modify
    pgrid->pass_fx = bits;

    // get the new bits
    new_bits = ego_grid_info_t::get_all_fx( pgrid );

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

Uint8 cartman_get_fan_twist(const ego_mesh_t *self, const TileIndex& tile)
{
    // check for a valid tile
    if (TileIndex::Invalid == tile || tile > self->info.tiles_count)
    {
        return TWIST_FLAT;
    }
    ego_tile_info_t *info = tile_mem_t::get(&(self->tmem), tile);
    // if the tile is actually labelled as MAP_FANOFF, ignore it completely
    if (TILE_IS_FANOFF(info))
    {
        return TWIST_FLAT;
    }
    size_t vrtstart = info->vrtstart;

    float z0 = self->tmem.plst[vrtstart + 0][ZZ];
    float z1 = self->tmem.plst[vrtstart + 1][ZZ];
    float z2 = self->tmem.plst[vrtstart + 2][ZZ];
    float z3 = self->tmem.plst[vrtstart + 3][ZZ];

    float zx = CARTMAN_FIXNUM * (z0 + z3 - z1 - z2) / CARTMAN_SLOPE;
    float zy = CARTMAN_FIXNUM * (z2 + z3 - z0 - z1) / CARTMAN_SLOPE;

    return cartman_calc_twist(zx, zy);
}
