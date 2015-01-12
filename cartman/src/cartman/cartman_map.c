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

#include "cartman_map.h"

#include "cartman.h"
#include "cartman_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

cartman_mpd_t mesh = { 0, 0 };

size_t numwritten = 0;
size_t numattempt = 0;

tile_line_data_t tile_dict_lines[MAP_FAN_TYPE_MAX];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static cartman_mpd_t * cartman_mpd_finalize( cartman_mpd_t * );

static cartman_mpd_t * cartman_mpd_convert( cartman_mpd_t *, map_t * );
static map_t * cartman_mpd_revert( map_t *, cartman_mpd_t * );

//--------------------------------------------------------------------------------------------
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
//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_ctor( cartman_mpd_t * ptr )
{
    if ( NULL == ptr ) ptr = &mesh;

    BLANK_STRUCT_PTR( ptr );

    cartman_mpd_vertex_ary_ctor( ptr->vrt, SDL_arraysize( ptr->vrt ) );
    ptr->vrt_free = SDL_arraysize( ptr->vrt );
    ptr->vrt_at   = 0;

    cartman_mpd_info_ctor( &( ptr->info ) );

    cartman_mpd_tile_ary_ctor( ptr->fan, SDL_arraysize( ptr->fan ) );

    return ptr;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_dtor( cartman_mpd_t * ptr )
{
    if ( NULL == ptr ) ptr = &mesh;

    cartman_mpd_vertex_ary_dtor( ptr->vrt, SDL_arraysize( ptr->vrt ) );
    ptr->vrt_free = 0;
    ptr->vrt_at   = 0;

    cartman_mpd_info_ctor( &( ptr->info ) );

    cartman_mpd_tile_ary_dtor( ptr->fan, SDL_arraysize( ptr->fan ) );

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_renew( cartman_mpd_t * ptr )
{
    ptr = cartman_mpd_dtor( ptr );
    ptr = cartman_mpd_ctor( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cartman_mpd_tile_t * cartman_mpd_tile_ctor( cartman_mpd_tile_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    ptr->tx_bits  = MAP_FANOFF;
    ptr->twist    = TWIST_FLAT;
    ptr->fx       = MAPFX_WALL | MAPFX_IMPASS;
    ptr->vrtstart = MAP_FAN_ENTRIES_MAX;

    return ptr;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_tile_t * cartman_mpd_tile_dtor( cartman_mpd_tile_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool cartman_mpd_vertex_ary_ctor( cartman_mpd_vertex_t ary[], size_t size )
{
    size_t cnt;

    if ( NULL == ary || 0 == size ) return false;

    for ( cnt = 0; cnt < size; cnt++ )
    {
        cartman_mpd_vertex_ctor( ary + cnt );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool cartman_mpd_vertex_ary_dtor( cartman_mpd_vertex_t ary[], size_t size )
{
    size_t cnt;

    if ( NULL == ary || 0 == size ) return false;

    for ( cnt = 0; cnt < size; cnt++ )
    {
        cartman_mpd_vertex_ctor( ary + cnt );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool cartman_mpd_tile_ary_ctor( cartman_mpd_tile_t ary[], size_t size )
{
    size_t cnt;

    if ( NULL == ary || 0 == size ) return false;

    for ( cnt = 0; cnt < size; cnt++ )
    {
        cartman_mpd_tile_ctor( ary + cnt );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
bool cartman_mpd_tile_ary_dtor( cartman_mpd_tile_t ary[], size_t size )
{
    size_t cnt;

    if ( NULL == ary || 0 == size ) return false;

    for ( cnt = 0; cnt < size; cnt++ )
    {
        cartman_mpd_tile_ctor( ary + cnt );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cartman_mpd_vertex_t * cartman_mpd_vertex_ctor( cartman_mpd_vertex_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    ptr->a    = VERTEXUNUSED;
    ptr->next = CHAINEND;

    return ptr;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_vertex_t * cartman_mpd_vertex_dtor( cartman_mpd_vertex_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cartman_mpd_info_t * cartman_mpd_info_ctor( cartman_mpd_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_info_t * cartman_mpd_info_dtor( cartman_mpd_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    BLANK_STRUCT_PTR( ptr );

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool cartman_mpd_info_init( cartman_mpd_info_t * pinfo, int vert_count, size_t tiles_x, size_t tiles_y )
{

    // handle default values
    if ( vert_count < 0 )
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
cartman_mpd_t * cartman_mpd_load_vfs( /* const char *modname, */ cartman_mpd_t * pmesh )
{
    // trap bad module names
    //if ( !VALID_CSTR( modname ) ) return pmesh;

    // make sure we have the tile dictionary
    cartman_tile_dictionary_load_vfs();

    // initialize the mesh
    {
        // create a new mesh if we are passed a NULL pointer
        if ( NULL == pmesh )
        {
            pmesh = cartman_mpd_ctor( pmesh );
        }

        if ( NULL == pmesh ) return pmesh;

        // free any memory that has been allocated
        pmesh = cartman_mpd_renew( pmesh );
    }

    // actually do the loading
    {
        map_t  local_mpd, * pmpd;

        // load a raw mpd
        map_ctor( &local_mpd );
        pmpd = map_load( vfs_resolveReadFilename( "mp_data/level.mpd" ), &local_mpd );

        // convert it into a convenient version for cartman
        pmesh = cartman_mpd_convert( pmesh, pmpd );

        // delete the now useless mpd data
        map_dtor( &local_mpd );
    }

    // do some calculation to set up the mpd as a game mesh
    pmesh = cartman_mpd_finalize( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_save_vfs( /*const char *modname,*/ cartman_mpd_t * pmesh )
{
    if ( NULL == pmesh /*|| INVALID_CSTR( modname )*/ ) return NULL;

    // make sure we have the tile dictionary
    cartman_tile_dictionary_load_vfs();

    // actually do the saving
    {
        map_t  local_mpd, * pmpd;

        // load a raw mpd
        pmpd = map_ctor( &local_mpd );

        // convert it into a convenient version for Egoboo
        pmpd = cartman_mpd_revert( pmpd, pmesh );
        pmpd = map_save( vfs_resolveReadFilename( "mp_data/level.mpd" ), pmpd );

        // delete the now useless mpd data
        map_dtor( &local_mpd );
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_make_fanstart( cartman_mpd_t * pmesh )
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( cnt = 0; cnt < pmesh->info.tiles_y; cnt++ )
    {
        pmesh->fanstart[cnt] = pmesh->info.tiles_x * cnt;
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
        pmesh->fan[fan].twist = cartman_mpd_get_fan_twist( pmesh, fan );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_create( cartman_mpd_t * pmesh, int tiles_x, int tiles_y )
{
    // ZZ> This function makes the mesh
    int mapx, mapy, fan, tile;
    int x, y;

    if ( NULL == pmesh ) pmesh = &mesh;

    cartman_mpd_free_vertices( pmesh );

    pmesh->info.tiles_x = tiles_x;
    pmesh->info.tiles_y = tiles_y;

    pmesh->info.edgex = pmesh->info.tiles_x * TILE_ISIZE;
    pmesh->info.edgey = pmesh->info.tiles_y * TILE_ISIZE;
    pmesh->info.edgez = 180 << 4;

    fan = 0;
    tile = 0;
    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        y = mapy * TILE_ISIZE;
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            x = mapx * TILE_ISIZE;

            pmesh->fan[fan].type = 0;
            pmesh->fan[fan].tx_bits = ((( mapx & 1 ) + ( mapy & 1 ) ) & 1 ) + DEFAULT_TILE;

            fan++;
        }
    }

    cartman_mpd_make_fanstart( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int cartman_mpd_get_ifan( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    int ifan = -1;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( mapy >= 0 && mapy < pmesh->info.tiles_y && mapy < MAP_TILEY_MAX )
    {
        if ( mapx >= 0 && mapx < pmesh->info.tiles_x && mapx < MAP_TILEY_MAX )
        {
            ifan = pmesh->fanstart[mapy] + mapx;
        }
    }

    return ifan;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_tile_t * cartman_mpd_get_pfan( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    int ifan = cartman_mpd_get_ifan( pmesh, mapx, mapy );

    return CART_MPD_FAN_PTR( pmesh, ifan );
}

//--------------------------------------------------------------------------------------------
cartman_mpd_vertex_t * cartman_mpd_get_pvrt_ivrt( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, int ivrt )
{
    cartman_mpd_vertex_t * pvrt = NULL;

    if ( NULL == pmesh || NULL == pfan || !CART_VALID_VERTEX_RANGE( ivrt ) ) return NULL;

    // get the raw vertex
    pvrt = pmesh->vrt + ivrt;

    // if it is marked as unused return a blank vertex
    // do not blank out the value of ivrt in case the caller
    // wants to make the vertex USED
    if ( VERTEXUNUSED == pvrt->a )
    {
        pvrt = NULL;
    }

    return pvrt;
}

//--------------------------------------------------------------------------------------------
cartman_mpd_vertex_t * cartman_mpd_get_pvrt_idx( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, int idx, int * ivrt_ptr )
{
    int loc_ivrt;

    cartman_mpd_vertex_t * pvrt = NULL;

    // optional parameters
    if ( NULL == ivrt_ptr ) ivrt_ptr = &loc_ivrt;

    *ivrt_ptr = cartman_mpd_get_ivrt_pfan( pmesh, pfan, idx );

    pvrt = cartman_mpd_get_pvrt_ivrt( pmesh, pfan, *ivrt_ptr );

    return pvrt;
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_free_vertex_count( cartman_mpd_t * pmesh )
{
    // ZZ> This function counts the unused vertices and sets pmesh->vrt_free
    int cnt, num;

    if ( NULL == pmesh ) pmesh = &mesh;

    num = 0;
    for ( cnt = 0; cnt < MAP_VERTICES_MAX; cnt++ )
    {
        if ( VERTEXUNUSED == pmesh->vrt[cnt].a )
        {
            num++;
        }
    }

    pmesh->vrt_free = num;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_count_vertices( cartman_mpd_t * pmesh )
{
    int fan, mapx, mapy, cnt, num, totalvert;
    Uint32 vert;

    if ( NULL == pmesh ) pmesh = &mesh;

    totalvert = 0;
    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_ifan( pmesh, mapx, mapy );
            if ( !VALID_MPD_TILE_RANGE( fan ) ) continue;

            num = tile_dict.def_lst[pmesh->fan[fan].type].numvertices;

            for ( cnt = 0, vert = pmesh->fan[fan].vrtstart;
                  cnt < num && CHAINEND != vert;
                  cnt++, vert = pmesh->vrt[vert].next )
            {
                totalvert++;
            }
        }
    }

    return totalvert;
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_free_vertices( cartman_mpd_t * pmesh )
{
    // ZZ> This function sets all vertices to unused
    int cnt;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( cnt = 0; cnt < MAP_VERTICES_MAX; cnt++ )
    {
        pmesh->vrt[cnt].a    = VERTEXUNUSED;
        pmesh->vrt[cnt].next = CHAINEND;
    }

    pmesh->vrt_at = 0;
    pmesh->vrt_free = MAP_VERTICES_MAX;
}

//--------------------------------------------------------------------------------------------
bool cartman_mpd_link_vertex( cartman_mpd_t * pmesh, int iparent, int ichild )
{
    cartman_mpd_vertex_t * pparent, * pchild;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( !CART_VALID_VERTEX_RANGE( iparent ) ) return false;
    pparent = pmesh->vrt + iparent;

    if ( !CART_VALID_VERTEX_RANGE( ichild ) ) return false;
    pchild = pmesh->vrt + ichild;

    pparent->next = ichild;
    pchild->next  = CHAINEND;

    return true;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_find_free_vertex( cartman_mpd_t * pmesh )
{
    // ZZ> This function returns the index of a free vertex, -1 if none were found.

    int cnt;

    bool found;
    cartman_mpd_vertex_t * vrt_lst;

    if ( NULL == pmesh ) pmesh = &mesh;
    vrt_lst = pmesh->vrt;

    if ( pmesh->vrt_free <= 0 ) return false;

    for ( cnt = 0;
          cnt < MAP_VERTICES_MAX && VERTEXUNUSED != pmesh->vrt[pmesh->vrt_at].a;
          cnt++ )
    {
        pmesh->vrt_at++;

        if ( !CART_VALID_VERTEX_RANGE( pmesh->vrt_at ) )
        {
            pmesh->vrt_at = 0;
        }
    }

    found = false;
    if ( VERTEXUNUSED == pmesh->vrt[pmesh->vrt_at].a )
    {
        pmesh->vrt[pmesh->vrt_at].a = 1;
        found = true;
    }

    return found ? pmesh->vrt_at : -1;
}

//--------------------------------------------------------------------------------------------
Uint8 cartman_mpd_get_fan_twist( cartman_mpd_t * pmesh, Uint32 fan )
{
    int zx, zy;
    int vt0, vt1, vt2, vt3;
    Uint8 twist;

    vt0 = pmesh->fan[fan].vrtstart;
    vt1 = pmesh->vrt[vt0].next;
    vt2 = pmesh->vrt[vt1].next;
    vt3 = pmesh->vrt[vt2].next;

    zx = ( pmesh->vrt[vt0].z + pmesh->vrt[vt3].z - pmesh->vrt[vt1].z - pmesh->vrt[vt2].z ) / SLOPE;
    zy = ( pmesh->vrt[vt2].z + pmesh->vrt[vt3].z - pmesh->vrt[vt0].z - pmesh->vrt[vt1].z ) / SLOPE;

    twist = cartman_mpd_calc_twist( zx, zy );

    return twist;
}

//--------------------------------------------------------------------------------------------
float cartman_mpd_get_level( cartman_mpd_t * pmesh, float x, float y )
{
    int mapx, mapy;
    int v0, v1, v2, v3; // the vertex of each fan corner
    float z0, z1, z2, z3; // the height of each fan corner
    float zleft, zright;   // Weighted height of each side
    float zdone = 0;

    cartman_mpd_vertex_t * vlst   = NULL;
    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    if ( x < 0.0f || x >= pmesh->info.edgex ) return zdone;
    mapx = FLOOR( x / TILE_FSIZE );

    if ( y < 0.0f || y >= pmesh->info.edgey ) return zdone;
    mapy = FLOOR( y / TILE_FSIZE );

    pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
    if ( NULL == pfan ) return 0;

    // get the vertices and heights
    v0 = pfan->vrtstart;
    z0 = CART_VALID_VERTEX_RANGE( v0 ) ? vlst[v0].z : 0;

    if ( CART_VALID_VERTEX_RANGE( v0 ) )
    {
        v1 = vlst[v0].next;
        z1 = CART_VALID_VERTEX_RANGE( v1 ) ? vlst[v1].z : 0;
    }
    else
    {
        v1 = CHAINEND;
        z1 = 0;
    }

    if ( CART_VALID_VERTEX_RANGE( v1 ) )
    {
        v2 = vlst[v1].next;
        z2 = CART_VALID_VERTEX_RANGE( v2 ) ? vlst[v2].z : 0;
    }
    else
    {
        v2 = CHAINEND;
        z2 = 0;
    }

    if ( CART_VALID_VERTEX_RANGE( v2 ) )
    {
        v3 = vlst[v2].next;
        z3 = CART_VALID_VERTEX_RANGE( v3 ) ? vlst[v3].z : 0;
    }
    else
    {
        v3 = CHAINEND;
        z3 = 0;
    }

    zleft  = ( z0 * ( TILE_ISIZE - mapy ) + z3 * mapy ) / TILE_FSIZE;
    zright = ( z1 * ( TILE_ISIZE - mapy ) + z2 * mapy ) / TILE_FSIZE;
    zdone  = ( zleft * ( TILE_ISIZE - mapx ) + zright * mapx ) / TILE_FSIZE;

    return zdone;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_free_vertex_list( cartman_mpd_t * pmesh, int list[], size_t size )
{
    int ivrt;

    if ( NULL == pmesh || NULL == list || 0 == size ) return -1;

    for (size_t cnt = 0; cnt < size; cnt++ )
    {
        ivrt = list[cnt];
        if ( !CART_VALID_VERTEX_RANGE( ivrt ) ) break;

        pmesh->vrt[ivrt].a    = VERTEXUNUSED;
        pmesh->vrt[ivrt].next = CHAINEND;
    }

    return size;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_allocate_vertex_list( cartman_mpd_t * pmesh, int list[], size_t size, size_t count )
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
        int vert = cartman_mpd_find_free_vertex( pmesh );
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
            cartman_mpd_vertex_ctor( pmesh->vrt + list[cnt] );
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
        pmesh->vrt[parent].a = 1;

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
int cartman_mpd_allocate_verts( cartman_mpd_t * pmesh, size_t count )
{
    int allocate_rv;
    int vertexlist[MAP_FAN_VERTICES_MAX + 1];

    // size must be less than MAP_FAN_VERTICES_MAX
    if ( count > MAP_FAN_VERTICES_MAX ) return -1;

    allocate_rv = cartman_mpd_allocate_vertex_list( pmesh, vertexlist, SDL_arraysize( vertexlist ), count );
    if ( allocate_rv < 0 ) return -1;

    return vertexlist[0];
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_add_fan_verts( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan )
{
    // ZZ> This function allocates the vertices needed for a fan_idx

    int vertexlist[MAP_FAN_VERTICES_MAX + 1];

    int vert_count;

    Uint8 fan_type;

    tile_definition_t    * pdef;
    cartman_mpd_vertex_t * vrt_list;

    if ( NULL == pfan ) return -1;

    // grab the mesh
    if ( NULL == pmesh ) pmesh = &mesh;
    vrt_list = pmesh->vrt + 0;

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

    cartman_mpd_allocate_vertex_list( pmesh, vertexlist, SDL_arraysize( vertexlist ), vert_count );

    // set the vertex posisions
    pfan->vrtstart = vertexlist[0];

    return pfan->vrtstart;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_add_pfan( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, float x, float y )
{
    // ZZ> This function allocates the vertices needed for a fan_idx

    int cnt;
    int vert_count;
    Uint32 vertex;

    int start_vertex;

    tile_definition_t    * pdef;
    cartman_mpd_vertex_t * pvrt;

    // grab the mesh
    if ( NULL == pmesh ) pmesh = &mesh;

    // check the pfan index
    if ( NULL == pfan )
    {
        log_warning( "%s - tried to add NULL fan pointer\n", __FUNCTION__ );
        goto cartman_mpd_add_fan_fail;
    }

    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef )
    {
        log_warning( "%s - invalid fan type %d\n", __FUNCTION__, pfan->type );
        goto cartman_mpd_add_fan_fail;
    }

    vert_count = pdef->numvertices;
    if ( 0 == vert_count )
    {
        log_warning( "%s - undefined fan type %d\n", __FUNCTION__, pfan->type );
        goto cartman_mpd_add_fan_fail;
    }
    else if ( vert_count > MAP_FAN_VERTICES_MAX )
    {
        log_warning( "%s - too many vertices in fan type %d\n", __FUNCTION__, pfan->type );
        goto cartman_mpd_add_fan_fail;
    }

    // allocate the verts for this fan
    start_vertex = cartman_mpd_add_fan_verts( pmesh, pfan );
    if ( start_vertex < 0 )
    {
        log_warning( "%s - could not allocate vertices for the fan\n", __FUNCTION__ );
        goto cartman_mpd_add_fan_fail;
    }

    // initialize the vertices
    pvrt = NULL;
    for ( cnt = 0, vertex = pfan->vrtstart;
          cnt < vert_count && CHAINEND != vertex;
          cnt++, vertex = pvrt->next )
    {
        pvrt = pmesh->vrt + vertex;

        pvrt->x = x + GRID_TO_POS( pdef->grid_ix[cnt] );
        pvrt->y = y + GRID_TO_POS( pdef->grid_iy[cnt] );
        pvrt->z = 0.0f;
    }

    return pfan->vrtstart;

cartman_mpd_add_fan_fail:

    return -1;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_add_ifan( cartman_mpd_t * pmesh, int ifan, float x, float y )
{
    // ZZ> This function allocates the vertices needed for a fan_idx

    return cartman_mpd_add_pfan( pmesh, CART_MPD_FAN_PTR( pmesh, ifan ), x, y );
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_remove_pfan( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan )
{
    int cnt, vert;
    Uint32 numvert;

    tile_definition_t * pdef;

    if ( NULL == pmesh || NULL == pfan ) return;

    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef ) return;

    numvert = pdef->numvertices;

    for ( cnt = 0, vert = pfan->vrtstart;
          cnt < numvert && CHAINEND != vert;
          cnt++, vert = pmesh->vrt[vert].next )
    {
        pmesh->vrt[vert].a = VERTEXUNUSED;
        pmesh->vrt_free++;
    }

    pfan->type     = 0;
    pfan->fx       = MAPFX_SHA;
    pfan->vrtstart = CHAINEND;
    pfan->fx       = MAPFX_WALL | MAPFX_IMPASS;
    pfan->tx_bits  = MAP_FANOFF;
    pfan->twist    = TWIST_FLAT;
}

//--------------------------------------------------------------------------------------------
void cartman_mpd_remove_ifan( cartman_mpd_t * pmesh, int fan )
{
    // ZZ> This function removes a fan's vertices from usage and sets the fan
    //     to not be drawn

    cartman_mpd_remove_pfan( pmesh, CART_MPD_FAN_PTR( pmesh, fan ) );
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_get_ivrt_pfan( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, int index )
{
    // ZZ> This function gets a vertex number or -1

    int vert, cnt;
    tile_definition_t * pdef;

    if ( NULL == pmesh ) pmesh = &mesh;

    // assume the worst
    vert = -1;

    // find the fan
    if ( NULL == pfan ) return -1;

    // find the tile definition
    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef ) return -1;

    // is it a valid vertex index?
    if ( index >= pdef->numvertices ) return -1;

    // find the actual vertex number
    vert = pfan->vrtstart;
    for ( cnt = 0; cnt < index; cnt++ )
    {
        vert = pmesh->vrt[vert].next;
        if ( CHAINEND == vert )
        {
            vert = -1;
        }
    }

    return vert;
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_get_ivrt_fan( cartman_mpd_t * pmesh, int fan, int index )
{
    // ZZ> This function gets a vertex number or -1

    cartman_mpd_tile_t * pfan;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return -1;

    return cartman_mpd_get_ivrt_pfan( pmesh, pfan, index );
}

//--------------------------------------------------------------------------------------------
int cartman_mpd_get_ivrt_xy( cartman_mpd_t * pmesh, int mapx, int mapy, int index )
{
    // ZZ> This function gets a vertex number or -1

    int fan;

    // find the fan
    fan = cartman_mpd_get_ifan( pmesh, mapx, mapy );
    if ( fan < 0 ) return -1;

    return cartman_mpd_get_ivrt_fan( pmesh, fan, index );
}

//--------------------------------------------------------------------------------------------
cartman_mpd_t * cartman_mpd_convert( cartman_mpd_t * pmesh_dst, map_t * pmesh_src )
{
    int itile_src, ifan_dst, ivrt_src;

    map_mem_t    * pmem_src;
    map_info_t   * pinfo_src;
    tile_info_t  * fan_ary_src;
    map_vertex_t * vrt_ary_src;

    cartman_mpd_info_t   * pinfo_dst;
    cartman_mpd_tile_t   * fan_ary_dst;
    cartman_mpd_vertex_t * vrt_ary_dst;

    if ( NULL == pmesh_src ) return NULL;
    pmem_src = &( pmesh_src->mem );
    pinfo_src = &( pmesh_src->info );
    fan_ary_src = pmem_src->tile_list;
    vrt_ary_src = pmem_src->vlst;

    // clear out all data in the destination mesh
    if ( NULL == cartman_mpd_renew( pmesh_dst ) ) return NULL;
    pinfo_dst = &( pmesh_dst->info );
    fan_ary_dst  = pmesh_dst->fan;
    vrt_ary_dst  = pmesh_dst->vrt;

    // set up the destination mesh from the source mesh
    cartman_mpd_info_init( pinfo_dst, pinfo_src->vertcount, pinfo_src->tiles_x, pinfo_src->tiles_y );

    // copy all the per-tile info
    for ( itile_src = 0; itile_src < pinfo_dst->tiles_count; itile_src++ )
    {
        tile_info_t        * ptile_src = fan_ary_src + itile_src;
        cartman_mpd_tile_t * pfan_dst  = fan_ary_dst + itile_src;

        pfan_dst->type     = ptile_src->type;
        pfan_dst->tx_bits  = ptile_src->img;
        pfan_dst->fx       = ptile_src->fx;
        pfan_dst->twist    = ptile_src->twist;
    }

    // store the vertices in the vertex chain for editing
    for ( ifan_dst = 0, ivrt_src = 0; ifan_dst < pinfo_dst->tiles_count; ifan_dst++ )
    {
        int ivrt_dst, cnt;
        int vert_count, allocate_rv;

        tile_definition_t    * pdef     = NULL;

        map_vertex_t         * pvrt_src = NULL;
        cartman_mpd_vertex_t * pvrt_dst = NULL;
        cartman_mpd_tile_t   * pfan_dst  = NULL;

        // use the data that was transferred to the destination fan
        pfan_dst  = fan_ary_dst + ifan_dst;

        // check for valid fan type
        pdef = TILE_DICT_PTR( tile_dict, pfan_dst->type );
        if ( NULL == pdef )
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
            pvrt_dst = vrt_ary_dst + ivrt_dst;

            pvrt_dst->x = pvrt_src->pos.x;
            pvrt_dst->y = pvrt_src->pos.y;
            pvrt_dst->z = pvrt_src->pos.z;
            pvrt_dst->a = std::max( pvrt_src->a, (Uint8)1 );  // force a != VERTEXUNUSED
        };
    }

    return pmesh_dst;

cartman_mpd_convert_fail:

    cartman_mpd_renew( pmesh_dst );

    return NULL;
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
	size_t cnt;
    int ivrt_dst, itile;

    cartman_mpd_info_t   * pinfo_src;
    cartman_mpd_tile_t   * fan_ary_src;
    cartman_mpd_vertex_t * vrt_ary_src;

    map_mem_t    * pmem_dst;
    map_info_t   * pinfo_dst;
    tile_info_t  * fan_ary_dst;
    map_vertex_t * vrt_ary_dst;
    map_info_t     loc_info_dst;

    if ( NULL == pmesh_src ) return NULL;
    pinfo_src = &( pmesh_src->info );
    fan_ary_src  = pmesh_src->fan;
    vrt_ary_src  = pmesh_src->vrt;

    // clear out all data in the destination mesh
    if ( NULL == map_renew( pmesh_dst ) ) return NULL;
    pmem_dst = &( pmesh_dst->mem );
    pinfo_dst = &( pmesh_dst->info );
    fan_ary_dst = pmem_dst->tile_list;
    vrt_ary_dst = pmem_dst->vlst;

    // make sure we have an accurate vertex count
    pinfo_src->vertex_count = cartman_mpd_count_vertices( pmesh_src );

    // allocate the correct size for the destination mesh
    loc_info_dst.tiles_x   = pinfo_src->tiles_x;
    loc_info_dst.tiles_y   = pinfo_src->tiles_y;
    loc_info_dst.vertcount = pinfo_src->vertex_count;
    map_init( pmesh_dst, &loc_info_dst );

    // revert the tile information
    for (cnt = 0; cnt < pinfo_src->tiles_count; cnt++ )
    {
        tile_info_t        * ptile_dst = fan_ary_dst + cnt;
        cartman_mpd_tile_t * pfan_src  = fan_ary_src + cnt;

        ptile_dst->type   = pfan_src->type;
        ptile_dst->img    = pfan_src->tx_bits;
        ptile_dst->fx     = pfan_src->fx;
        ptile_dst->twist  = pfan_src->twist;
    }

    // revert the vertex information
    for ( itile = 0, ivrt_dst = 0; itile < pinfo_src->tiles_count; itile++ )
    {
        int tnc, vert_count, ivrt_src;
        cartman_mpd_tile_t   * pfan_src;
        cartman_mpd_vertex_t * pvrt_src;
        map_vertex_t         * pvrt_dst;
        tile_definition_t    * pdef;

        // grab the source fan
        pfan_src = fan_ary_src + itile;

        // is the type valid?
        pdef = TILE_DICT_PTR( tile_dict, pfan_src->type );
        if ( NULL == pdef )
        {
            log_warning( "%s - invalid fan type %d used in the mesh\n", __FUNCTION__, pfan_src->type );
            goto cartman_mpd_revert_fail;
        }

        // is the vertex_count valid?
        vert_count = pdef->numvertices;
        if ( 0 == vert_count )
        {
            log_warning( "%s - undefined fan type %d used in the mesh\n", __FUNCTION__, pfan_src->type );
        }
        else if ( vert_count > MAP_FAN_VERTICES_MAX )
        {
            log_warning( "%s - too many vertices %d used in tile type %d\n", __FUNCTION__, vert_count, pfan_src->type );
            goto cartman_mpd_revert_fail;
        }

        // is the initial vertex valid?
        if ( !CART_VALID_VERTEX_RANGE( pfan_src->vrtstart ) )
        {
            log_warning( "%s - vertex %d is outside of valid vertex range\n", __FUNCTION__, pfan_src->vrtstart );
            goto cartman_mpd_revert_fail;
        }

        pvrt_src = NULL;
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
            pvrt_src = vrt_ary_src + ivrt_src;

            // check for VERTEXUNUSED
            if ( VERTEXUNUSED == pvrt_src->a )
            {
                log_warning( "%s - vertex %d of tile %d is marked as unused\n", __FUNCTION__, tnc, itile );
                goto cartman_mpd_revert_fail;
            }

            // grab the destination vertex
            pvrt_dst = vrt_ary_dst + ivrt_dst;

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
