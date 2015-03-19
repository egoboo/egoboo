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

#include "cartman/cartman_map.h"

#include "cartman/cartman.h"
#include "cartman/cartman_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

cartman_mpd_t mesh;

size_t numwritten = 0;
size_t numattempt = 0;

tile_line_data_t tile_dict_lines[MAP_FAN_TYPE_MAX];

//--------------------------------------------------------------------------------------------

cartman_mpd_t *cartman_mpd_finalize( cartman_mpd_t * );

cartman_mpd_t *cartman_mpd_convert( cartman_mpd_t *, map_t * );
map_t *cartman_mpd_revert( map_t *, cartman_mpd_t * );

//--------------------------------------------------------------------------------------------

Uint8 cartman_mpd_calc_twist( int dx, int dy )
{
    Uint8 twist;

    // dx and dy should be from -7 to 8
    if ( dx < -7 ) dx = -7;
    if ( dx > 8 ) dx = 8;
    if ( dy < -7 ) dy = -7;
    if ( dy > 8 ) dy = 8;

    // Now between 0 and 15
    dx = dx + 7;
    dy = dy + 7;
    twist = ( dy << 4 ) + dx;

    return twist;
}

//--------------------------------------------------------------------------------------------

cartman_mpd_t::cartman_mpd_t() :
    vrt2(), vrt_free(MAP_VERTICES_MAX), vrt_at(0), info(),
    fan2(), fanstart2()
{
}

cartman_mpd_t::~cartman_mpd_t()
{
}

cartman_mpd_t *cartman_mpd_t::reset()
{
    for (auto& e : vrt2)
    {
        e.reset();
    }
    vrt_free = MAP_VERTICES_MAX;
    vrt_at = 0;

    info.reset();

    for (auto& e : fan2)
    {
        e.reset();
    }

    for (auto& e : fanstart2)
    {
        e = 0;
    }

    return this;
}

//--------------------------------------------------------------------------------------------

cartman_mpd_tile_t::cartman_mpd_tile_t() :
    type(0),
    tx_bits(MAP_FANOFF),
    twist(TWIST_FLAT),
    fx(MAPFX_WALL | MAPFX_IMPASS),
    vrtstart(MAP_FAN_ENTRIES_MAX)
{
}

void cartman_mpd_tile_t::reset()
{
    type = 0;
    tx_bits = MAP_FANOFF;
    twist = TWIST_FLAT;
    fx = MAPFX_WALL | MAPFX_IMPASS;
    vrtstart = MAP_FAN_ENTRIES_MAX;
}

//--------------------------------------------------------------------------------------------

Cartman::mpd_vertex_t::mpd_vertex_t() :
next(CHAINEND),
x(0.0f), y(0.0f), z(0.0f),
a(VERTEXUNUSED)
{}

Cartman::mpd_vertex_t::~mpd_vertex_t()
{
    x = 0.0f; y = 0.0f; z = 0.0f;
    a = VERTEXUNUSED;
    next = CHAINEND;
}


void Cartman::mpd_vertex_t::reset()
{
    x = 0.0f; y = 0.0f; z = 0.0f;
    a = VERTEXUNUSED;
    next = CHAINEND;
}

//--------------------------------------------------------------------------------------------

cartman_mpd_info_t::cartman_mpd_info_t() :
tiles_x(0), tiles_y(0),
tiles_count(0), vertex_count(0),
edgex(0), edgey(0), edgez(0)
{}

cartman_mpd_info_t::~cartman_mpd_info_t()
{}

void cartman_mpd_info_t::reset()
{
    tiles_x = 0;
    tiles_y = 0;
    tiles_count = 0;
    vertex_count = 0;
    edgex = edgey = edgez = 0;
}

//--------------------------------------------------------------------------------------------
bool cartman_mpd_info_init(cartman_mpd_info_t * pinfo, int vert_count, size_t tiles_x, size_t tiles_y)
{

    // handle default values
    if (vert_count < 0)
    {
        vert_count = MAP_FAN_VERTICES_MAX * pinfo->tiles_count;
    }

    // set the desired number of tiles
    pinfo->tiles_x = tiles_x;
    pinfo->tiles_y = tiles_y;
    pinfo->tiles_count = pinfo->tiles_x * pinfo->tiles_y;
    pinfo->vertex_count = vert_count;

    pinfo->edgex = pinfo->tiles_x * TILE_ISIZE;
    pinfo->edgey = pinfo->tiles_y * TILE_ISIZE;
    pinfo->edgez = DEFAULT_Z_SIZE;

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void cartman_mpd_make_fanstart( cartman_mpd_t * pmesh )
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( cnt = 0; cnt < pmesh->info.tiles_y; cnt++ )
    {
        pmesh->fanstart2[cnt] = pmesh->info.tiles_x * cnt;
    }
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_make_twist( cartman_mpd_t * pmesh )
{
    Uint32 fan, numfan;

    if ( NULL == pmesh ) pmesh = &mesh;

    numfan = pmesh->info.tiles_x * pmesh->info.tiles_y;
    for ( fan = 0; fan < numfan; fan++ )
    {
        pmesh->fan2[fan].twist = cartman_mpd_get_fan_twist( pmesh, fan );
    }
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int cartman_mpd_t::get_ifan(int mapx, int mapy)
{
    if (mapy >= 0 && mapy < info.tiles_y && mapy < MAP_TILEY_MAX)
    {
        if (mapx >= 0 && mapx < info.tiles_x && mapx < MAP_TILEY_MAX)
        {
            return fanstart2[mapy] + mapx;
        }
    }

    return -1;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_tile_t *cartman_mpd_t::get_pfan(int mapx, int mapy)
{
    int ifan = get_ifan(mapx, mapy);
    return get_pfan(ifan);
}

cartman_mpd_tile_t *cartman_mpd_t::get_pfan(int ifan)
{
    if (!VALID_MPD_TILE_RANGE(ifan))
    {
        return nullptr;
    }
    return &(fan2[ifan]);
}

//--------------------------------------------------------------------------------------------
Cartman::mpd_vertex_t *cartman_mpd_t::get_pvrt_ivrt(cartman_mpd_t *pmesh, cartman_mpd_tile_t *pfan, int ivrt)
{
    if (!pmesh || !pfan || !CART_VALID_VERTEX_RANGE(ivrt))
    {
        return nullptr;
    }
    // Get the vertex.
    Cartman::mpd_vertex_t *vertex = pmesh->get_vertex(ivrt);

    // If it is marked as unused return @a nullptr.
    // Do not blank out the value of ivrt in case
    // the caller wants to make the vertex used.
    if (VERTEXUNUSED == vertex->a)
    {
        return nullptr;
    }

    return vertex;
}

//--------------------------------------------------------------------------------------------
Cartman::mpd_vertex_t * cartman_mpd_t::get_pvrt_idx( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, int idx, int * ivrt_ptr )
{
    int loc_ivrt;

    if (!pmesh)
    {
        throw std::invalid_argument("nullptr == self");
    }

    // optional parameters
    if ( NULL == ivrt_ptr ) ivrt_ptr = &loc_ivrt;

    *ivrt_ptr = pmesh->get_ivrt_pfan(pfan, idx);

    Cartman::mpd_vertex_t *pvrt = cartman_mpd_t::get_pvrt_ivrt(pmesh, pfan, *ivrt_ptr);

    return pvrt;
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_t::free_vertex_count()
{
    int num = 0;
    for (auto& e : vrt2)
    {
        if (VERTEXUNUSED == e.a)
        {
            num++;
        }
    }

    vrt_free = num;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_t::count_used_vertices()
{
    int totalvert = 0;
    for (int mapy = 0; mapy < info.tiles_y; mapy++)
    {
        for (int mapx = 0; mapx < info.tiles_x; mapx++)
        {
            int ifan = get_ifan(mapx, mapy);
            if (!VALID_MPD_TILE_RANGE(ifan))
            {
                continue;
            }
            int num = tile_dict.def_lst[fan2[ifan].type].numvertices;

            int cnt;
            Uint32 vert;
            for (cnt = 0, vert = fan2[ifan].vrtstart;
                 cnt < num && CHAINEND != vert;
                 cnt++, vert = vrt2[vert].next)
            {
                totalvert++;
            }
        }
    }

    return totalvert;
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_free_vertices(cartman_mpd_t *self)
{
    if (!self)
    {
        self = &mesh; /// @todo Bad!
    }
    for (size_t i = 0; i < MAP_VERTICES_MAX; ++i)
    {
        self->vrt2[i].a = VERTEXUNUSED;
        self->vrt2[i].next = CHAINEND;
    }

    self->vrt_at = 0;
    self->vrt_free = MAP_VERTICES_MAX;
}

Cartman::mpd_vertex_t *cartman_mpd_t::get_vertex(int ivrt)
{
    if (!CART_VALID_VERTEX_RANGE(ivrt))
    {
        return nullptr;
    }
    return &(vrt2[ivrt]);
}

const Cartman::mpd_vertex_t *cartman_mpd_t::get_vertex(int ivrt) const
{
    if (!CART_VALID_VERTEX_RANGE(ivrt))
    {
        return nullptr;
    }
    return &(vrt2[ivrt]);
}

//--------------------------------------------------------------------------------------------
bool cartman_mpd_link_vertex(cartman_mpd_t *self, int iparent, int ichild)
{
    if (!self)
    {
        self = &mesh;
    }

    Cartman::mpd_vertex_t *pparent = self->get_vertex(iparent),
                          *pchild = self->get_vertex(ichild);

    if (!pparent || !pchild)
    {
        return false;
    }

    pparent->next = ichild;
    pchild->next  = CHAINEND;

    return true;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_t::find_free_vertex()
{
    if (vrt_free <= 0)
    {
        return -1;
    }

    for (int cnt = 0;
         cnt < MAP_VERTICES_MAX && VERTEXUNUSED != vrt2[vrt_at].a;
         cnt++)
    {
        vrt_at++;

        if (!CART_VALID_VERTEX_RANGE(vrt_at))
        {
            vrt_at = 0;
        }
    }

    bool found = false;
    if (VERTEXUNUSED == vrt2[vrt_at].a)
    {
        vrt2[vrt_at].a = 1;
        found = true;
    }

    return found ? vrt_at : -1;
}

//--------------------------------------------------------------------------------------------
Uint8 cartman_mpd_get_fan_twist( cartman_mpd_t * pmesh, Uint32 fan )
{
    int vt0 = pmesh->fan2[fan].vrtstart;
    int vt1 = pmesh->vrt2[vt0].next;
    int vt2 = pmesh->vrt2[vt1].next;
    int vt3 = pmesh->vrt2[vt2].next;

    int zx = ( pmesh->vrt2[vt0].z + pmesh->vrt2[vt3].z - pmesh->vrt2[vt1].z - pmesh->vrt2[vt2].z )
           / SLOPE;
    int zy = ( pmesh->vrt2[vt2].z + pmesh->vrt2[vt3].z - pmesh->vrt2[vt0].z - pmesh->vrt2[vt1].z )
           / SLOPE;

    Uint8 twist = cartman_mpd_calc_twist( zx, zy );

    return twist;
}

//--------------------------------------------------------------------------------------------

float cartman_mpd_t::get_level(int mapx, int mapy)
{
    float zleft, zright;   // Weighted height of each side.
    float zdone = 0.0f;

    cartman_mpd_tile_t *pfan = get_pfan(mapx, mapy);
    if (!pfan) return zdone;

    int v0, v1, v2, v3;   // The vertex index of each fan corner
    float z0, z1, z2, z3; // The elevation of the vertex of each fan corner.
    // Get the vertex index (v0 to v3) and the elevation (z0 to z3) of each fan corner.
    v0 = pfan->vrtstart;

    z0 = CART_VALID_VERTEX_RANGE(v0) ? vrt2[v0].z : 0;

    if (CART_VALID_VERTEX_RANGE(v0))
    {
        v1 = vrt2[v0].next;
        z1 = CART_VALID_VERTEX_RANGE(v1) ? vrt2[v1].z : 0;
    }
    else
    {
        v1 = CHAINEND;
        z1 = 0;
    }

    if (CART_VALID_VERTEX_RANGE(v1))
    {
        v2 = vrt2[v1].next;
        z2 = CART_VALID_VERTEX_RANGE(v2) ? vrt2[v2].z : 0;
    }
    else
    {
        v2 = CHAINEND;
        z2 = 0;
    }

    if (CART_VALID_VERTEX_RANGE(v2))
    {
        v3 = vrt2[v2].next;
        z3 = CART_VALID_VERTEX_RANGE(v3) ? vrt2[v3].z : 0;
    }
    else
    {
        v3 = CHAINEND;
        z3 = 0;
    }

    zleft = (z0 * (TILE_ISIZE - mapy) + z3 * mapy) / TILE_FSIZE;
    zright = (z1 * (TILE_ISIZE - mapy) + z2 * mapy) / TILE_FSIZE;
    zdone = (zleft * (TILE_ISIZE - mapx) + zright * mapx) / TILE_FSIZE;

    return zdone;
}

float cartman_mpd_t::get_level(float x, float y)
{
    float zdone = 0.0f;
    if (x < 0.0f || x >= info.edgex) return zdone;
    if (y < 0.0f || y >= info.edgey) return zdone;
    int mapx, mapy;
    worldToMap(x, y, mapx, mapy);
    return get_level(mapx, mapy);
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_free_vertex_list(cartman_mpd_t *self, int list[], size_t size)
{
    if (!self || !list || !size)
    {
        return -1;
    }
    for (size_t i = 0; i < size; ++i)
    {
        int ivrt = list[i];
        if (!CART_VALID_VERTEX_RANGE(ivrt))
        {
            break;
        }
        self->vrt2[ivrt].a = VERTEXUNUSED;
        self->vrt2[ivrt].next = CHAINEND;
    }
    return size;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_allocate_vertex_list(cartman_mpd_t * pmesh, int list[], size_t size, size_t count)
{
	size_t cnt, valid_verts;
	int allocated = 0;
    Uint32 vrt_at_old = 0, vrt_free_old = MAP_VERTICES_MAX;

    bool alloc_error = false;

    // the list must be valid
    if ( NULL == list || 0 == size ) return -1;

    // size must be one greater than the max number to allocate
    if ( count + 1 > size ) return -1;

    // grab the mesh
    if ( NULL == pmesh ) pmesh = &mesh;
    vrt_at_old   = pmesh->vrt_at;
    vrt_free_old = pmesh->vrt_free;

    // try to allocate the vertices
    alloc_error = false;
    for (cnt = 0, valid_verts = 0; cnt < count; cnt++, valid_verts++ )
    {
        int vert = pmesh->find_free_vertex();
        if ( vert < 0 )
        {
            alloc_error = true;
            break;
        }

        list[cnt] = vert;
    }

    // handle allocation errors
    if ( alloc_error )
    {
        // reset the pmesh memory
        pmesh->vrt_at   = vrt_at_old;
        pmesh->vrt_free = vrt_free_old;
        for ( cnt = 0; cnt < valid_verts; cnt++ )
        {
            pmesh->vrt2[list[cnt]].reset();
        }

        // tell the caller we failed
        list[0] = CHAINEND;
        allocated = -1;
    }
    else
    {
        int parent, child;

        // allocated all the vertices, so adjust the free count
        pmesh->vrt_free -= valid_verts;

        // finish the list
        list[cnt] = CHAINEND;

        // tell the caller we succeeded
        allocated = valid_verts;

        // set the start vertex
        parent = list[0];
        pmesh->vrt2[parent].a = 1;

        // link the remaining vertices
        for ( cnt = 1; cnt < valid_verts; cnt++ )
        {
            parent = list[cnt-1];
            child  = list[cnt];
            cartman_mpd_link_vertex( pmesh, parent, child );
        }
    }

    return allocated;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_allocate_verts(cartman_mpd_t *self, size_t count)
{
    int vertexlist[MAP_FAN_VERTICES_MAX + 1];

    // count must be less than MAP_FAN_VERTICES_MAX
    if (count > MAP_FAN_VERTICES_MAX)
    {
        return -1;
    }
    int result = cartman_mpd_allocate_vertex_list(self, vertexlist, SDL_arraysize(vertexlist), count);
    if (result < 0)
    {
        return -1;
    }
    return vertexlist[0];
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_add_fan_verts(cartman_mpd_t *self, cartman_mpd_tile_t *pfan)
{
    // ZZ> This function allocates the vertices needed for a fan_idx

    int vertexlist[MAP_FAN_VERTICES_MAX + 1];

    int vert_count;

    Uint8 fan_type;

    tile_definition_t    * pdef;
    //Cartman::mpd_vertex_t * vrt_list;

    if ( NULL == pfan ) return -1;

    // grab the mesh
    if (!self) self = &mesh;
    //vrt_list = self->vrt2[0];

    fan_type = pfan->type;

    // get the tile definition
    pdef = TILE_DICT_PTR( tile_dict, fan_type );
    if ( NULL == pdef )
    {
        log_warning( "%s - tried to add invalid fan_idx type %d\n", __FUNCTION__, fan_type );
    }

    // check the vertex count
    vert_count = pdef->numvertices;
    if ( 0 == vert_count )
    {
        log_warning( "%s - tried to add undefined fan_idx type %d\n", __FUNCTION__, fan_type );
    }
    if ( vert_count > MAP_FAN_VERTICES_MAX )
    {
        log_error( "%s - fan_idx type %d is defined with too many vertices %d\n", __FUNCTION__, fan_type, vert_count );
    }

    cartman_mpd_allocate_vertex_list(self, vertexlist, SDL_arraysize(vertexlist), vert_count);

    // set the vertex posisions
    pfan->vrtstart = vertexlist[0];

    return pfan->vrtstart;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_t::add_pfan(cartman_mpd_tile_t *pfan, float x, float y)
{
    // Check the fan.
    if (!pfan)
    {
        log_warning("%s - tried to add null fan pointer\n", __FUNCTION__);
        return -1;
    }

    tile_definition_t *pdef = TILE_DICT_PTR(tile_dict, pfan->type);
    if (!pdef)
    {
        log_warning("%s - invalid fan type %d\n", __FUNCTION__, pfan->type);
        return -1;
    }

    int vert_count = pdef->numvertices;
    if (0 == vert_count)
    {
        log_warning("%s - undefined fan type %d\n", __FUNCTION__, pfan->type);
        return -1;
    }
    else if (vert_count > MAP_FAN_VERTICES_MAX)
    {
        log_warning("%s - too many vertices in fan type %d\n", __FUNCTION__, pfan->type);
        return -1;
    }

    // allocate the verts for this fan
    int start_vertex = cartman_mpd_add_fan_verts(this, pfan);
    if (start_vertex < 0)
    {
        log_warning("%s - could not allocate vertices for fan\n", __FUNCTION__);
        return -1;
    }

    // Initialize the vertices.
    Cartman::mpd_vertex_t *pvrt = nullptr;
    int cnt;
    Uint32 vertex;
    for ( cnt = 0, vertex = pfan->vrtstart;
          cnt < vert_count && CHAINEND != vertex;
          cnt++, vertex = pvrt->next )
    {
        pvrt = get_vertex(vertex);

        pvrt->x = x + GRID_TO_POS( pdef->grid_ix[cnt] );
        pvrt->y = y + GRID_TO_POS( pdef->grid_iy[cnt] );
        pvrt->z = 0.0f;
    }

    return pfan->vrtstart;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_t::add_ifan(int ifan, float x, float y)
{
    return add_pfan(CART_MPD_FAN_PTR(this, ifan), x, y);
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_t::remove_pfan(cartman_mpd_tile_t *pfan)
{
    if (!pfan) return;

    tile_definition_t *pdef = TILE_DICT_PTR(tile_dict, pfan->type);
    if (!pdef) return;

    Uint32 numvert = pdef->numvertices;

    for (int cnt = 0, vert = pfan->vrtstart;
         cnt < numvert && CHAINEND != vert;
         cnt++, vert = this->vrt2[vert].next )
    {
        this->vrt2[vert].a = VERTEXUNUSED;
        this->vrt_free++;
    }

    pfan->type     = 0;
    pfan->fx       = MAPFX_SHA;
    pfan->vrtstart = CHAINEND;
    pfan->fx       = MAPFX_WALL | MAPFX_IMPASS;
    pfan->tx_bits  = MAP_FANOFF;
    pfan->twist    = TWIST_FLAT;
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_t::remove_ifan(int ifan)
{
    remove_pfan(CART_MPD_FAN_PTR(this, ifan));
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_t::get_ivrt_pfan(cartman_mpd_tile_t *pfan, int index)
{
    // find the fan
    if ( NULL == pfan ) return -1;

    // find the tile definition
    tile_definition_t *pdef = TILE_DICT_PTR(tile_dict, pfan->type);
    if ( NULL == pdef ) return -1;

    // is it a valid vertex index?
    if ( index >= pdef->numvertices ) return -1;

    // find the actual vertex number
    int vert = pfan->vrtstart;
    for (int cnt = 0; cnt < index; cnt++ )
    {
        vert = vrt2[vert].next;
        if ( CHAINEND == vert )
        {
            vert = -1;
        }
    }

    return vert;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_t::get_ivrt_fan(int ifan, int index)
{
    cartman_mpd_tile_t *pfan = get_pfan(ifan);
    if (!pfan) return -1;

    return get_ivrt_pfan(pfan, index);
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_t::get_ivrt_xy(int mapx, int mapy, int index)
{
    // find the fan
    int ifan = get_ifan(mapx, mapy);
    if (ifan < 0) return -1;

    return get_ivrt_fan(ifan, index);
}

//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_convert( cartman_mpd_t * pmesh_dst, map_t * pmesh_src )
{
    int itile_src, ifan_dst, ivrt_src;

    map_mem_t    * pmem_src;
    map_info_t   * pinfo_src;
    tile_info_t  * fan_ary_src;
    map_vertex_t * vrt_ary_src;

    if ( NULL == pmesh_src ) return NULL;
    pmem_src = &( pmesh_src->mem );
    pinfo_src = &( pmesh_src->info );
    fan_ary_src = pmem_src->tile_list;
    vrt_ary_src = pmem_src->vlst;

    // Reset the destination mesh.
    if (!pmesh_dst->reset())
    {
        return nullptr;
    }

    // set up the destination mesh from the source mesh
    cartman_mpd_info_init(&(pmesh_dst->info), pinfo_src->vertcount, pinfo_src->tiles_x, pinfo_src->tiles_y);

    // copy all the per-tile info
    for ( itile_src = 0; itile_src < pmesh_dst->info.tiles_count; itile_src++ )
    {
        tile_info_t        * ptile_src = fan_ary_src + itile_src;
        cartman_mpd_tile_t * pfan_dst  = &(pmesh_dst->fan2[itile_src]);

        pfan_dst->type     = ptile_src->type;
        pfan_dst->tx_bits  = ptile_src->img;
        pfan_dst->fx       = ptile_src->fx;
        pfan_dst->twist    = ptile_src->twist;
    }

    // store the vertices in the vertex chain for editing
    for (ifan_dst = 0, ivrt_src = 0; ifan_dst < pmesh_dst->info.tiles_count; ifan_dst++)
    {
        int ivrt_dst, cnt;
        int vert_count, allocate_rv;

        map_vertex_t         * pvrt_src = NULL;
        Cartman::mpd_vertex_t * pvrt_dst = NULL;

        // use the data that was transferred to the destination fan
        cartman_mpd_tile_t *pfan_dst = &(pmesh_dst->fan2[ifan_dst]);

        // check for valid fan type
        tile_definition_t *pdef = TILE_DICT_PTR(tile_dict, pfan_dst->type);
        if (!pdef)
        {
            log_warning( "%s - invalid fan type in fan # %d\n", __FUNCTION__, ifan_dst );
            goto cartman_mpd_convert_fail;
        }

        // get an appropriate number of vertices from the tile definition
        if ( 0 == pdef->numvertices )
        {
            log_warning( "%s - undefined fan type %d in fan # %d\n", __FUNCTION__, pfan_dst->type, ifan_dst );
            vert_count = 4;
        }
        else
        {
            vert_count = pdef->numvertices;
        }

        // check for valid vertex count
        if ( vert_count > MAP_FAN_VERTICES_MAX )
        {
            log_warning( "%s - too many vertices in fan type %d in fan # %d\n", __FUNCTION__, pfan_dst->type, ifan_dst );
            goto cartman_mpd_convert_fail;
        }

        // allocate the vertices
        allocate_rv = cartman_mpd_allocate_verts( pmesh_dst, vert_count );
        if ( -1 == allocate_rv )
        {
            log_warning( "%s - could not allocate enough vertices for the mesh at fan # %d\n", __FUNCTION__, ifan_dst );
            goto cartman_mpd_convert_fail;
        }

        // set the fan's vertex start position
        pfan_dst->vrtstart = allocate_rv;

        // fill in the vertex values
        for ( cnt = 0, ivrt_dst = pfan_dst->vrtstart, pvrt_dst = NULL;
              cnt < vert_count;
              cnt++, ivrt_dst = pvrt_dst->next, ivrt_src++ )
        {
            if ( CHAINEND == ivrt_dst )
            {
                log_warning( "%s - unexpected CHAINEND in tile %d vertex %d\n.", __FUNCTION__, ifan_dst, ivrt_dst );
                goto cartman_mpd_convert_fail;
            }

            pvrt_src = vrt_ary_src + ivrt_src;
            pvrt_dst = &(pmesh_dst->vrt2[ivrt_dst]);

            pvrt_dst->x = pvrt_src->pos.x;
            pvrt_dst->y = pvrt_src->pos.y;
            pvrt_dst->z = pvrt_src->pos.z;
            pvrt_dst->a = std::max( pvrt_src->a, (Uint8)1 );  // force a != VERTEXUNUSED
        };
    }

    return pmesh_dst;

cartman_mpd_convert_fail:

    pmesh_dst->reset();

    return nullptr;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_finalize( cartman_mpd_t * pmesh )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    cartman_mpd_make_fanstart( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * cartman_mpd_revert( map_t * pmesh_dst, cartman_mpd_t * pmesh_src )
{
    if ( NULL == pmesh_src ) return NULL;
    cartman_mpd_info_t *pinfo_src = &(pmesh_src->info);

    // clear out all data in the destination mesh
    if ( NULL == map_renew( pmesh_dst ) ) return NULL;

    // make sure we have an accurate vertex count
    pinfo_src->vertex_count = pmesh_src->count_used_vertices();

    // allocate the correct size for the destination mesh
    map_info_t loc_info_dst;
    loc_info_dst.tiles_x   = pinfo_src->tiles_x;
    loc_info_dst.tiles_y   = pinfo_src->tiles_y;
    loc_info_dst.vertcount = pinfo_src->vertex_count;
    map_init( pmesh_dst, &loc_info_dst );
    
    map_mem_t *pmem_dst = &(pmesh_dst->mem);
    map_info_t *pinfo_dst = &(pmesh_dst->info);
    tile_info_t *fan_ary_dst = pmem_dst->tile_list;
    map_vertex_t *vrt_ary_dst = pmem_dst->vlst;

    // revert the tile information
    for (size_t cnt = 0; cnt < pinfo_src->tiles_count; cnt++ )
    {
        tile_info_t *fan_ary_dst = pmem_dst->tile_list;
        tile_info_t *ptile_dst = fan_ary_dst + cnt;
        cartman_mpd_tile_t *pfan_src = &(pmesh_src->fan2[cnt]);

        ptile_dst->type   = pfan_src->type;
        ptile_dst->img    = pfan_src->tx_bits;
        ptile_dst->fx     = pfan_src->fx;
        ptile_dst->twist  = pfan_src->twist;
    }

    // revert the vertex information
    for (int itile = 0, ivrt_dst = 0; itile < pinfo_src->tiles_count; itile++ )
    {
        // grab the source fan
        cartman_mpd_tile_t *pfan_src = &(pmesh_src->fan2[itile]);

        // is the type valid?
        tile_definition_t *pdef = TILE_DICT_PTR(tile_dict, pfan_src->type);
        if ( NULL == pdef )
        {
            log_warning("%s:%d: invalid fan type %d used in the mesh\n", __FILE__, __LINE__, pfan_src->type );
            goto cartman_mpd_revert_fail;
        }

        // is the vertex_count valid?
        int vert_count = pdef->numvertices;
        if ( 0 == vert_count )
        {
            log_warning("%s:%d: undefined fan type %d used in the mesh\n", __FILE__,__LINE__, pfan_src->type );
        }
        else if ( vert_count > MAP_FAN_VERTICES_MAX )
        {
            log_warning("%s:%d: too many vertices %d used in tile type %d\n", __FILE__,__LINE__, vert_count, pfan_src->type );
            goto cartman_mpd_revert_fail;
        }

        // is the initial vertex valid?
        if ( !CART_VALID_VERTEX_RANGE( pfan_src->vrtstart ) )
        {
            log_warning("%s:%d: vertex %d is outside of valid vertex range\n", __FILE__, __LINE__, pfan_src->vrtstart );
            goto cartman_mpd_revert_fail;
        }

        Cartman::mpd_vertex_t *pvrt_src = NULL;
        int tnc, ivrt_src;
        for ( tnc = 0, ivrt_src = pfan_src->vrtstart;
              tnc < vert_count;
              tnc++, ivrt_src = pvrt_src->next, ivrt_dst++ )
        {
            // check for a bad CHAINEND
            if ( CHAINEND == ivrt_src )
            {
                log_warning( "%s - vertex %d of tile %d is marked as unused\n", __FUNCTION__, tnc, itile );
                goto cartman_mpd_revert_fail;
            }

            // grab the src pointer
            pvrt_src = &(pmesh_src->vrt2[ivrt_src]);

            // check for VERTEXUNUSED
            if ( VERTEXUNUSED == pvrt_src->a )
            {
                log_warning( "%s - vertex %d of tile %d is marked as unused\n", __FUNCTION__, tnc, itile );
                goto cartman_mpd_revert_fail;
            }

            // grab the destination vertex
            map_vertex_t *pvrt_dst = vrt_ary_dst + ivrt_dst;

            pvrt_dst->pos.x = pvrt_src->x;
            pvrt_dst->pos.y = pvrt_src->y;
            pvrt_dst->pos.z = pvrt_src->z;
            pvrt_dst->a     = pvrt_src->a;
        }
    }

    return pmesh_dst;

cartman_mpd_revert_fail:

    // deallocate any dynamic memory
    map_renew( pmesh_dst );

    return NULL;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void tile_dict_lines_add( int fantype, int start, int end )
{
    // ZZ> This function adds a line to the vertex schematic
    size_t cnt;
    tile_line_data_t * plines;

    if ( fantype < 0 || fantype >= MAP_FAN_TYPE_MAX ) return;
    plines = tile_dict_lines + fantype;

    if ( plines->count >= tile_dict.def_count ) return;

    // Make sure line isn't already in list
    for ( cnt = 0; cnt < plines->count; cnt++ )
    {
        if (( plines->start[cnt] == start && plines->end[cnt] == end ) ||
            ( plines->end[cnt] == start && plines->start[cnt] == end ) )
        {
            return;
        }
    }

    // Add it in
    cnt = plines->count;
    plines->start[cnt] = start;
    plines->end[cnt]   = end;
    plines->count++;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_t *cartman_mpd_load_vfs(cartman_mpd_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    // Load the tile dictionary.
    cartman_tile_dictionary_load_vfs();

    // Reset the mesh.
    if (!self->reset())
    {
        return nullptr;
    }

    // Create a raw map.
    map_t local;
    if (!map_ctor(&local))
    {
        return nullptr;
    }

    // Load the raw map.
    if (!map_load("mp_data/level.mpd", &local))
    {
        map_dtor(&local);
        return nullptr;
    }

    // Convert the raw map into a Cartman map.
    if (!cartman_mpd_convert(self, &local))
    {
        map_dtor(&local);
        return nullptr;
    }

    map_dtor(&local);

    // Do finishing calculations.
    if (!cartman_mpd_finalize(self))
    {
        self->reset();
        return nullptr;
    }

    return self;
}

cartman_mpd_t *cartman_mpd_save_vfs(cartman_mpd_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    // Load the tile dictionary.
    /// @todo Why the hell do you load the tile dictionary?
    cartman_tile_dictionary_load_vfs();

    // Construct a raw map.
    map_t local;
    if (map_ctor(&local))
    {
        return nullptr;
    }

    // Convert the map map into a raw map.
    if (!cartman_mpd_revert(&local, self))
    {
        map_dtor(&local);
        return nullptr;
    }

    // Save the raw map.
    if (!map_save("mp_data/level.mpd", &local))
    {
        map_dtor(&local);
        return nullptr;
    }

    map_dtor(&local);

    return self;
}

cartman_mpd_t *cartman_mpd_create(cartman_mpd_t *self, int tiles_x, int tiles_y)
{
    if (!self)
    {
        self = &mesh; ///< @todo Bad!
    }
    cartman_mpd_free_vertices(self);

    self->info.tiles_x = tiles_x;
    self->info.tiles_y = tiles_y;

    self->info.edgex = self->info.tiles_x * TILE_ISIZE;
    self->info.edgey = self->info.tiles_y * TILE_ISIZE;
    self->info.edgez = 180 << 4;

    for (int mapy = 0, fan = 0; mapy < self->info.tiles_y; mapy++)
    {
        int y = mapy * TILE_ISIZE;
        for (int mapx = 0; mapx < self->info.tiles_x; mapx++)
        {
            int x = mapx * TILE_ISIZE;

            self->fan2[fan].type = 0;
            self->fan2[fan].tx_bits = (((mapx & 1) + (mapy & 1)) & 1) + DEFAULT_TILE;

            fan++;
        }
    }

    cartman_mpd_make_fanstart(self);

    return self;
}
//--------------------------------------------------------------------------------------------
void cartman_tile_dictionary_load_vfs()
{
    tile_definition_t * pdef;

    tile_dictionary_load_vfs( "mp_data/fans.txt", &tile_dict, -1 );

    // generate the lines for each fan type
    for ( size_t fantype = 0;  fantype < tile_dict.def_count; fantype++ )
    {
        int entry, command;

        pdef = TILE_DICT_PTR( tile_dict, fantype );
        if ( NULL == pdef || 0 == pdef->numvertices ) continue;

        for ( command = 0, entry = 0; command < pdef->command_count; command++ )
        {
            int cnt, inow, ilast, fancenter;
            int command_size;

            command_size = pdef->command_entries[command];

            // convert the fan data into lines representing the fan edges
            fancenter = pdef->command_verts[entry++];
            inow      = pdef->command_verts[entry++];
            for ( cnt = 2; cnt < command_size; cnt++, entry++ )
            {
                ilast = inow;
                inow = pdef->command_verts[entry];

                tile_dict_lines_add( fantype, fancenter, inow );
                tile_dict_lines_add( fantype, fancenter, ilast );
                tile_dict_lines_add( fantype, ilast,     inow );
            }
        }
    }
}
