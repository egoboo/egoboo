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

#include "cartman_functions.h"

#include "cartman.h"
#include "cartman_map.h"
#include "cartman_select.h"

#include "cartman_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enum
{
    CORNER_TL,
    CORNER_TR,
    CORNER_BR,
    CORNER_BL,
    CORNER_COUNT
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void weld_TL( cartman_mpd_t * pmesh, int mapx, int mapy );
static void weld_TR( cartman_mpd_t * pmesh, int mapx, int mapy );
static void weld_BR( cartman_mpd_t * pmesh, int mapx, int mapy );
static void weld_BL( cartman_mpd_t * pmesh, int mapx, int mapy );

static void weld_edge_verts( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, tile_definition_t * pdef, int cnt, int mapx, int mapy );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float dist_from_border( cartman_mpd_t * pmesh, float x, float y )
{
    float x_dst, y_dst;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( x < 0.0f ) x_dst = 0.0f;
    else if ( x > pmesh->info.edgex * 0.5f ) x_dst = pmesh->info.edgex - x;
    else if ( x > pmesh->info.edgex ) x_dst = 0.0f;
    else x_dst = x;

    if ( y < 0.0f ) y_dst = 0.0f;
    else if ( y > pmesh->info.edgey * 0.5f ) y_dst = pmesh->info.edgey - y;
    else if ( y > pmesh->info.edgey ) y_dst = 0.0f;
    else y_dst = x;

    return ( x < y ) ? x : y;
}

//--------------------------------------------------------------------------------------------
int dist_from_edge( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    if ( mapx > ( pmesh->info.tiles_x >> 1 ) )
        mapx = pmesh->info.tiles_x - mapx - 1;
    if ( mapy > ( pmesh->info.tiles_y >> 1 ) )
        mapy = pmesh->info.tiles_y - mapy - 1;

    if ( mapx < mapy )
        return mapx;

    return mapy;
}

//--------------------------------------------------------------------------------------------
void fix_corners( cartman_mpd_t * pmesh )
{
    // ZZ> This function corrects corners across entire mesh

    int mapx, mapy;

    if ( NULL == pmesh ) pmesh = &mesh;

    // weld the corners in a checkerboard pattern
    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy += 2 )
    {
        for ( mapx = mapy & 1; mapx < pmesh->info.tiles_x; mapx += 2 )
        {
            weld_corner_verts( pmesh, mapx, mapy );
        }
    }
}

//--------------------------------------------------------------------------------------------
void fix_edges( cartman_mpd_t * pmesh )
{
    // ZZ> This function seals the tile edges across the entire mesh

    int mapx, mapy;

    if ( NULL == pmesh ) pmesh = &mesh;

    // weld the edges of all tiles
    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fix_vertices( pmesh, mapx, mapy );
        }
    }
}

//--------------------------------------------------------------------------------------------
void fix_mesh( cartman_mpd_t * pmesh )
{
    // ZZ> This function corrects corners across entire mesh

    if ( NULL == pmesh ) pmesh = &mesh;

    fix_corners( pmesh );
    fix_edges( pmesh );
}

//--------------------------------------------------------------------------------------------
void fix_vertices( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    int cnt;

    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
    if ( NULL == pfan ) return;

    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef ) return;

    for ( cnt = 4; cnt < pdef->numvertices; cnt++ )
    {
        weld_edge_verts( pmesh, pfan, pdef, cnt, mapx, mapy );
    }
}

//--------------------------------------------------------------------------------------------
void weld_corner_verts( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    int fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    fan = cartman_mpd_get_ifan( pmesh, mapx, mapy );
    if ( !VALID_MPD_TILE_RANGE( fan ) ) return;

    weld_TL( pmesh, mapx, mapy );
    weld_TR( pmesh, mapx, mapy );
    weld_BL( pmesh, mapx, mapy );
    weld_BR( pmesh, mapx, mapy );
}

//--------------------------------------------------------------------------------------------
void weld_TL( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init( &loc_lst, pmesh );

    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy, CORNER_TL ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx - 1, mapy, CORNER_TR ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx - 1, mapy - 1, CORNER_BR ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy - 1, CORNER_BL ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_TR( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init( &loc_lst, pmesh );

    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy, CORNER_TR ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy - 1, CORNER_BR ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx + 1, mapy - 1, CORNER_BL ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx + 1, mapy, CORNER_TL ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_BR( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init( &loc_lst, pmesh );

    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy, CORNER_BR ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx + 1, mapy, CORNER_BL ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx + 1, mapy + 1, CORNER_TL ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy + 1, CORNER_TR ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_BL( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init( &loc_lst, pmesh );

    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy, CORNER_BL ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx, mapy + 1, CORNER_TL ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx - 1, mapy + 1, CORNER_TR ) );
    select_lst_add( &loc_lst, cartman_mpd_get_ivrt_xy( loc_lst.pmesh, mapx - 1, mapy, CORNER_BR ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
int get_fan_vertex_by_coord( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, int grid_ix, int grid_iy, int ext_verts[] )
{
    int cnt, ivrt, idx, gx, gy;

    int loc_verts[16];
    int * vert_lst = NULL;

    tile_definition_t    * pdef;
    cartman_mpd_vertex_t * pvrt;

    // catch bad parameters
    if ( NULL == pmesh || NULL == pfan ) return -1;
    if ( grid_ix < 0 || grid_ix >= 4 ) return -1;
    if ( grid_iy < 0 || grid_iy >= 4 ) return -1;

    // get the tile definition
    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef ) return -1;

    // handle optional parameters
    vert_lst = ( NULL != ext_verts ) ? ext_verts : loc_verts;

    // blank out the vert_lst[] array
    for ( cnt = 0; cnt < 16; cnt++ )
    {
        vert_lst[cnt] = -1;
    }

    // store the vertices in the vrt_lst[]
    for ( cnt = 0, ivrt = pfan->vrtstart; cnt < pdef->numvertices && CHAINEND != ivrt; cnt++, ivrt = pmesh->vrt[ivrt].next )
    {
        // find the array index for this vertex
        gx = pdef->grid_ix[cnt];
        gy = pdef->grid_iy[cnt];
        idx = gx | ( gy << 2 );

        // get a pointer to the vertex data
        pvrt = CART_MPD_VERTEX_PTR( pmesh, ivrt );

        // blank out any bad data
        if ( NULL == pvrt || VERTEXUNUSED == pvrt->a )
        {
            ivrt = -1;
        }

        // store the vertex in an array
        vert_lst[idx] = ivrt;
    }

    // grab the vertex number from the vert_lst[]
    idx = grid_ix | ( grid_iy << 2 );
    return vert_lst[idx];
}

//--------------------------------------------------------------------------------------------
bool_t interpolate_coord( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, int grid_ix, int grid_iy, fvec3_base_t vec, int ext_verts[] )
{
    // set the coordinates of the given vertex to the interpolated position along the edge of the fan

    int loc_verts[16];
    int * vert_lst = NULL;

    bool_t retval, is_edge_x, is_edge_y;
    int cnt, ivrt, idx;

    float   vweight = 0.0f;
    fvec3_t vsum    = ZERO_VECT3;

    cartman_mpd_vertex_t * pvrt;

    // if we were not passed ext_verts, no one has pre-calculated the
    // array of vertices for this tile, yet.
    if ( NULL == ext_verts )
    {
        // we choose local storage for the vertices
        vert_lst = loc_verts;

        // try to find the given vertex
        ivrt = get_fan_vertex_by_coord( pmesh, pfan, grid_ix, grid_iy, vert_lst );

        // grab a pointer to the actual vertex
        pvrt = CART_MPD_VERTEX_PTR( pmesh, ivrt );

        // if it exists, we are done
        if ( NULL != pvrt )
        {
            vec[kX] = pvrt->x;
            vec[kY] = pvrt->y;
            vec[kZ] = pvrt->z;

            return btrue;
        }
    }
    else
    {
        // use the pre-calculated vertex list
        vert_lst = ext_verts;
    }

    is_edge_x = ( 0 == grid_ix ) || ( 3 == grid_ix );
    is_edge_y = ( 0 == grid_iy ) || ( 3 == grid_iy );

    // is the vertex a corner? All corners should exist.
    if ( is_edge_x && is_edge_y )
    {
        log_warning( "%s - something is wrong with the vertices.\n", __FUNCTION__ );
        return bfalse;
    }

    // assume the worst
    retval = bfalse;

    if ( is_edge_x )
    {
        cartman_mpd_vertex_t * pvrt_min = NULL;
        cartman_mpd_vertex_t * pvrt_max = NULL;

        int grid_min = -1, grid_max = -1;

        // get the vertices next to the unknown one on this edge
        for ( cnt = 0; cnt < 4; cnt++ )
        {
            idx = grid_ix | ( cnt << 2 );
            ivrt = vert_lst[idx];

            if ( ivrt < 0 ) continue;

            pvrt = CART_MPD_VERTEX_PTR( pmesh, ivrt );
            if ( NULL == pvrt || VERTEXUNUSED == pvrt->a ) continue;

            if (( cnt < grid_iy ) && (( -1 == grid_min ) || ( cnt > grid_min ) ) )
            {
                grid_min = cnt;
                pvrt_min = pvrt;
            }

            if (( cnt > grid_iy ) && (( -1 == grid_max ) || ( cnt < grid_max ) ) )
            {
                grid_max = cnt;
                pvrt_max = pvrt;
            }
        }

        if ( NULL == pvrt_min || NULL == pvrt_max ||
             -1 == grid_min || -1 == grid_max )
        {
            retval = bfalse;
        }
        else
        {
            float fmax = ( float )( grid_iy - grid_min ) / ( float )( grid_max - grid_min );
            float fmin = 1.0f - fmax;

            vec[kX] = fmax * pvrt_max->x + fmin * pvrt_min->x;
            vec[kY] = fmax * pvrt_max->y + fmin * pvrt_min->y;
            vec[kZ] = fmax * pvrt_max->z + fmin * pvrt_min->z;

            retval = btrue;
        }
    }
    else if ( is_edge_y )
    {
        cartman_mpd_vertex_t * pvrt_min = NULL;
        cartman_mpd_vertex_t * pvrt_max = NULL;

        int grid_min = -1, grid_max = -1;

        // get the vertices next to the unknown one on this edge
        for ( cnt = 0; cnt < 4; cnt++ )
        {
            idx = cnt | ( grid_iy << 2 );
            ivrt = vert_lst[idx];

            if ( ivrt < 0 ) continue;

            pvrt = CART_MPD_VERTEX_PTR( pmesh, ivrt );
            if ( NULL == pvrt || VERTEXUNUSED == pvrt->a ) continue;

            if (( cnt < grid_ix ) && (( -1 == grid_min ) || ( cnt > grid_min ) ) )
            {
                grid_min = cnt;
                pvrt_min = pvrt;
            }

            if (( cnt > grid_ix ) && (( -1 == grid_max ) || ( cnt < grid_max ) ) )
            {
                grid_max = cnt;
                pvrt_max = pvrt;
            }
        }

        if ( NULL == pvrt_min || NULL == pvrt_max ||
             -1 == grid_min || -1 == grid_max )
        {
            retval = bfalse;
        }
        else
        {
            float fmax = ( float )( grid_ix - grid_min ) / ( float )( grid_max - grid_min );
            float fmin = 1.0f - fmax;

            vec[kX] = fmax * pvrt_max->x + fmin * pvrt_min->x;
            vec[kY] = fmax * pvrt_max->y + fmin * pvrt_min->y;
            vec[kZ] = fmax * pvrt_max->z + fmin * pvrt_min->z;

            retval = btrue;
        }
    }
    else
    {
        // interpolate using all known points
        int gx, gy;

        for ( idx = 0; idx < 16; idx++ )
        {
            gx = idx & 3;
            gy = ( idx >> 2 ) & 3;

            ivrt = vert_lst[idx];
            pvrt = CART_MPD_VERTEX_PTR( pmesh, ivrt );
            if ( NULL != pvrt && VERTEXUNUSED != pvrt->a )
            {
                float weight = exp( - SQR( gx - grid_ix ) - SQR( gy - grid_iy ) );

                vsum.x += pvrt->x * weight;
                vsum.y += pvrt->y * weight;
                vsum.z += pvrt->z * weight;
                vweight += weight;
            }
        }

        if ( vweight <= 0.0f )
        {
            retval = bfalse;
        }
        else
        {
            vec[kX] = vsum.x / vweight;
            vec[kY] = vsum.y / vweight;
            vec[kZ] = vsum.z / vweight;

            retval = btrue;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int select_lst_add_fan_vertex( select_lst_t * plst, int mapx, int mapy, int grid_ix, int grid_iy, int fake_vert )
{
    int vert_lst[16];

    fvec3_t vtmp;

    int ivrt;

    cartman_mpd_tile_t   * pfan = NULL;
    cartman_mpd_vertex_t * pvrt = NULL;

    pfan = cartman_mpd_get_pfan( plst->pmesh, mapx, mapy );
    if ( NULL == pfan ) return -1;

    ivrt = get_fan_vertex_by_coord( plst->pmesh, pfan, grid_ix, grid_iy, vert_lst );
    if ( ivrt < 0 )
    {
        ivrt = fake_vert;

        pvrt = CART_MPD_VERTEX_PTR( plst->pmesh, ivrt );
        if ( NULL != pvrt )
        {
            if ( interpolate_coord( plst->pmesh, pfan, grid_ix, grid_iy, vtmp.v, vert_lst ) )
            {
                pvrt->x = vtmp.x;
                pvrt->y = vtmp.y;
                pvrt->z = vtmp.z;
            }
        }
    }

    // make sure that the vertex exists
    if ( NULL == pvrt )
    {
        pvrt = CART_MPD_VERTEX_PTR( plst->pmesh, ivrt );
        if ( NULL == pvrt )
        {
            ivrt = -1;
        }
    }

    // select_lst_add() only adds valid verts, so don't worry about a bad value in ivrt
    select_lst_add( plst, ivrt );

    return ivrt;
}

//--------------------------------------------------------------------------------------------
void weld_edge_verts( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, tile_definition_t * pdef, int cnt, int mapx, int mapy )
{
    int fake_edge_count = 0;
    int fake_edge_verts[8 + 1];

    int    grid_ix, grid_iy;
    bool_t is_edge_x, is_edge_y;
    int    allocate_rv;

    if ( NULL == pmesh || NULL == pfan || NULL == pdef ) return;

    // allocate some fake edge verts just in case
    allocate_rv = cartman_mpd_allocate_vertex_list( pmesh, fake_edge_verts, SDL_arraysize( fake_edge_verts ), 8 );
    if ( allocate_rv < 0 ) return;

    // alias for the grid location
    grid_ix = pdef->grid_ix[cnt];
    grid_iy = pdef->grid_iy[cnt];

    // is this an edge?
    is_edge_x = ( 0 == grid_ix ) || ( 3 == grid_ix );
    is_edge_y = ( 0 == grid_iy ) || ( 3 == grid_iy );

    if ( is_edge_x || is_edge_y )
    {
        int added_vert;
        select_lst_t loc_lst = SELECT_LST_INIT;

        select_lst_init( &loc_lst, pmesh );

        // add the point on this fan
        pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
        if ( NULL != pfan )
        {
            int ivrt;
            cartman_mpd_vertex_t * pvrt;

            pvrt = cartman_mpd_get_pvrt_idx( pmesh, pfan, cnt, &ivrt );
            if ( NULL != pvrt )
            {
                select_lst_add( &loc_lst, ivrt );
            }
        }

        if ( 0 == grid_ix )
        {
            added_vert = select_lst_add_fan_vertex( &loc_lst, mapx - 1, mapy, 3 - grid_ix, grid_iy, fake_edge_verts[fake_edge_count] );

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        if ( 3 == grid_ix )
        {
            added_vert = select_lst_add_fan_vertex( &loc_lst, mapx + 1, mapy, 3 - grid_ix, grid_iy, fake_edge_verts[fake_edge_count] );

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        if ( 0 == grid_iy )
        {
            added_vert = select_lst_add_fan_vertex( &loc_lst, mapx, mapy - 1, grid_ix, 3 - grid_iy, fake_edge_verts[fake_edge_count] );

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        if ( 3 == grid_iy )
        {
            added_vert = select_lst_add_fan_vertex( &loc_lst, mapx, mapy + 1, grid_ix, 3 - grid_iy, fake_edge_verts[fake_edge_count] );

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        mesh_select_weld( &loc_lst );
    }

    // free all allocated vertices
    cartman_mpd_free_vertex_list( pmesh, fake_edge_verts, SDL_arraysize( fake_edge_verts ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_add_rect( select_lst_t * plst, float x0, float y0, float z0, float x1, float y1, float z1, int mode )
{
    // ZZ> This function checks the rectangular selection

    Uint32 ivrt;
    cartman_mpd_vertex_t * pvrt;

    float xmin, ymin, zmin;
    float xmax, ymax, zmax;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    plst = select_lst_synch_mesh( plst, &mesh );
    if ( NULL == plst ) return plst;

    if ( NULL == plst->pmesh ) return plst;
    pmesh = plst->pmesh;

    // get the vertex list
    vlst = pmesh->vrt;

    // if the selection is empty, we're done
    if ( x0 == x1 || y0 == y1 || z0 == z1 ) return plst;

    // make sure that the selection is ordered properly
    if ( x0 < x1 ) { xmin = x0; xmax = x1; }
    else { xmin = x1; xmax = x0; };
    if ( y0 < y1 ) { ymin = y0; ymax = y1; }
    else { ymin = y1; ymax = y0; };
    if ( z0 < z1 ) { zmin = z0; zmax = z1; }
    else { zmin = z1; zmax = z0; };

    if ( mode == WINMODE_VERTEX )
    {
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MAP_VERTICES_MAX; ivrt++, pvrt++ )
        {
            if ( VERTEXUNUSED == pvrt->a ) continue;

            if ( pvrt->x >= xmin && pvrt->x <= xmax &&
                 pvrt->y >= ymin && pvrt->y <= ymax &&
                 pvrt->z >= zmin && pvrt->z <= zmax )
            {
                plst = select_lst_add( plst, ivrt );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MAP_VERTICES_MAX; ivrt++, pvrt++ )
        {
            if ( VERTEXUNUSED == pvrt->a ) continue;

            if ( pvrt->x >= xmin && pvrt->x <= xmax &&
                 pvrt->y >= ymin && pvrt->y <= ymax &&
                 pvrt->z >= zmin && pvrt->z <= zmax )
            {
                plst = select_lst_add( plst, ivrt );
            }
        }
    }

    return plst;
}

//--------------------------------------------------------------------------------------------
select_lst_t * select_lst_remove_rect( select_lst_t * plst, float x0, float y0, float z0, float x1, float y1, float z1, int mode )
{
    // ZZ> This function checks the rectangular selection, and removes any fans
    //     in the selection area

    Uint32 ivrt;
    cartman_mpd_vertex_t * pvrt;

    float xmin, ymin, zmin;
    float xmax, ymax, zmax;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    plst = select_lst_synch_mesh( plst, &mesh );
    if ( NULL == plst ) return plst;

    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get the vertex list
    vlst = pmesh->vrt;

    // if the selection is empty, we're done
    if ( x0 == x1 || y0 == y1 || z0 == z1 ) return plst;

    // make sure that the selection is ordered properly
    if ( x0 < x1 ) { xmin = x0; xmax = x1; }
    else { xmin = x1; xmax = x0; };
    if ( y0 < y1 ) { ymin = y0; ymax = y1; }
    else { ymin = y1; ymax = y0; };
    if ( z0 < z1 ) { zmin = z0; zmax = z1; }
    else { zmin = z1; zmax = z0; };

    if ( mode == WINMODE_VERTEX )
    {
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MAP_VERTICES_MAX; ivrt++, pvrt++ )
        {
            if ( VERTEXUNUSED == pvrt->a ) continue;

            if ( pvrt->x >= xmin && pvrt->x <= xmax &&
                 pvrt->y >= ymin && pvrt->y <= ymax &&
                 pvrt->z >= zmin && pvrt->z <= zmax )
            {
                plst = select_lst_remove( plst, ivrt );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MAP_VERTICES_MAX; ivrt++, pvrt++ )
        {
            if ( VERTEXUNUSED == pvrt->a ) continue;

            if ( pvrt->x >= xmin && pvrt->x <= xmax &&
                 pvrt->y >= ymin && pvrt->y <= ymax &&
                 pvrt->z >= zmin && pvrt->z <= zmax )
            {
                plst = select_lst_remove( plst, ivrt );
            }
        }
    }

    return plst;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int nearest_edge_vertex( cartman_mpd_t * pmesh, int mapx, int mapy, float nearx, float neary )
{
    // ZZ> This function gets a vertex number or -1
    int ivrt, bestvert, cnt;
    int num;

    float grid_fx, grid_fy;
    float prox_x, prox_y, dist_abs, bestprox;

    // aliases
    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
    if ( NULL == pfan ) return -1;

    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef ) return -1;

    // assume the worst
    bestvert = -1;

    num = pdef->numvertices;
    if ( num > 4 )
    {
        ivrt = pfan->vrtstart;

        // skip over the 4 corner vertices
        for ( cnt = 0; cnt < 4 && CHAINEND != ivrt; cnt++ )
        {
            ivrt = vlst[ivrt].next;
        }

        bestprox = 9000;
        for ( cnt = 4; cnt < num && CHAINEND != ivrt; cnt++ )
        {
            // where is this point in the "grid"?
            grid_fx = GRID_TO_POS( pdef->grid_ix[cnt] );
            grid_fy = GRID_TO_POS( pdef->grid_iy[cnt] );

            prox_x = grid_fx - nearx;
            prox_y = grid_fy - neary;
            dist_abs = ABS( prox_x ) + ABS( prox_y );

            if ( dist_abs < bestprox )
            {
                bestvert = ivrt;
                bestprox = dist_abs;
            }

            ivrt = vlst[ivrt].next;
            if ( CHAINEND == ivrt ) break;
        }
    }

    return bestvert;
}

//--------------------------------------------------------------------------------------------
void mesh_select_move( const select_lst_t * plst, float x, float y, float z )
{
    int ivrt, cnt;
    float newx, newy, newz;

    // aliases
    cartman_mpd_t * pmesh = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == plst ) return;

    // get the proper mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get an alias
    vlst = pmesh->vrt;

    // limit the movement by the bounds of the mesh
    for ( cnt = 0; cnt < plst->count; cnt++ )
    {
        ivrt = plst->which[cnt];

        newx = vlst[ivrt].x + x;
        newy = vlst[ivrt].y + y;
        newz = vlst[ivrt].z + z;

        if ( newx < 0 )  x = 0 - vlst[ivrt].x;
        if ( newx > pmesh->info.edgex ) x = pmesh->info.edgex - vlst[ivrt].x;
        if ( newy < 0 )  y = 0 - vlst[ivrt].y;
        if ( newy > pmesh->info.edgey ) y = pmesh->info.edgey - vlst[ivrt].y;
        if ( newz < 0 )  z = 0 - vlst[ivrt].z;
        if ( newz > pmesh->info.edgez ) z = pmesh->info.edgez - vlst[ivrt].z;
    }

    // do the movement
    for ( cnt = 0; cnt < plst->count; cnt++ )
    {
        ivrt = plst->which[cnt];

        newx = vlst[ivrt].x + x;
        newy = vlst[ivrt].y + y;
        newz = vlst[ivrt].z + z;

        if ( newx < 0 )  newx = 0;
        if ( newx > pmesh->info.edgex )  newx = pmesh->info.edgex;
        if ( newy < 0 )  newy = 0;
        if ( newy > pmesh->info.edgey )  newy = pmesh->info.edgey;
        if ( newz < 0 )  newz = 0;
        if ( newz > pmesh->info.edgez )  newz = pmesh->info.edgez;

        vlst[ivrt].x = newx;
        vlst[ivrt].y = newy;
        vlst[ivrt].z = newz;
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_set_z_no_bound( const select_lst_t * plst, float z )
{
    int vert, cnt;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == plst ) return;

    // get the mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get vertex list alias
    vlst = pmesh->vrt;

    for ( cnt = 0; cnt < plst->count; cnt++ )
    {
        vert = plst->which[cnt];
        if ( vert < 0 || vert > pmesh->info.vertex_count ) continue;

        vlst[vert].z = z;
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_jitter( const select_lst_t * plst )
{
    int cnt;
    Uint32 vert;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;

    if ( NULL == plst ) return;

    // get the mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    for ( cnt = 0; cnt < plst->count; cnt++ )
    {
        vert = plst->which[cnt];

        move_vert( plst->pmesh,  vert, ( rand() % 3 ) - 1, ( rand() % 3 ) - 1, 0 );
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_verts_connected( const select_lst_t * plst )
{
    int vert, cnt, tnc, mapx, mapy;
    Uint8 select_vertsfan;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    cartman_mpd_info_t   * pinfo  = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;
    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( NULL == plst ) plst = select_lst_default();

    // get the mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get vertex list alias
    pinfo = &( pmesh->info );
    vlst  = pmesh->vrt;

    for ( mapy = 0; mapy < pinfo->tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pinfo->tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            pdef = TILE_DICT_PTR( tile_dict, pfan->type );
            if ( NULL == pdef ) continue;

            select_vertsfan = bfalse;

            for ( cnt = 0, vert = pfan->vrtstart;
                  cnt < pdef->numvertices;
                  cnt++, vert = vlst[vert].next )
            {
                for ( tnc = 0; tnc < plst->count; tnc++ )
                {
                    if ( plst->which[tnc] == vert )
                    {
                        select_vertsfan = btrue;
                        break;
                    }
                }
            }

            if ( select_vertsfan )
            {
                for ( cnt = 0, vert = pfan->vrtstart;
                      cnt < pdef->numvertices;
                      cnt++, vert = vlst[vert].next )
                {
                    select_lst_add( plst, vert );
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_weld( const select_lst_t * plst )
{
    // ZZ> This function welds the highlighted vertices

    int cnt;
    float sum_x, sum_y, sum_z, sum_a;
    float avg_x, avg_y, avg_z, avg_a;
    Uint32 vert;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == plst ) return;

    // get the mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get vertex list alias
    vlst = pmesh->vrt;

    if ( plst->count > 1 )
    {
        sum_x = 0.0f;
        sum_y = 0.0f;
        sum_z = 0.0f;
        sum_a = 0.0f;

        for ( cnt = 0; cnt < plst->count; cnt++ )
        {
            vert = plst->which[cnt];
            if ( CHAINEND == vert ) break;

            sum_x += vlst[vert].x;
            sum_y += vlst[vert].y;
            sum_z += vlst[vert].z;
            sum_a += vlst[vert].a;
        }

        if ( plst->count > 1 )
        {
            avg_x = sum_x / plst->count;
            avg_y = sum_y / plst->count;
            avg_z = sum_z / plst->count;
            avg_a = sum_a / plst->count;
        }
        else
        {
            avg_x = sum_x;
            avg_y = sum_y;
            avg_z = sum_z;
            avg_a = sum_a;
        }

        for ( cnt = 0; cnt < plst->count; cnt++ )
        {
            vert = plst->which[cnt];
            if ( CHAINEND == vert ) break;

            vlst[vert].x = avg_x;
            vlst[vert].y = avg_y;
            vlst[vert].z = avg_z;
            vlst[vert].a = CLIP( avg_a, 1, 255 );
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_tile( cartman_mpd_t * pmesh, Uint16 tiletoset, Uint8 upper, Uint16 presser, Uint8 tx )
{
    // ZZ> This function sets one tile type to another

    int mapx, mapy;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            if ( TILE_IS_FANOFF( pfan->tx_bits ) ) continue;

            if ( tiletoset == pfan->tx_bits )
            {
                int tx_bits;

                tx_bits = TILE_SET_UPPER_BITS( upper );
                switch ( presser )
                {
                    case 0:
                        tx_bits |= tx & 0xFF;
                        break;

                    case 1:
                        tx_bits |= ( tx & 0xFE ) + ( rand() & 1 );
                        break;

                    case 2:
                        tx_bits |= ( tx & 0xFC ) + ( rand() & 3 );
                        break;

                    case 3:
                        tx_bits |= ( tx & 0xF8 ) + ( rand() & 7 );
                        break;

                    default:
                        tx_bits = pfan->tx_bits;
                }
                pfan->tx_bits = tx_bits;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_mesh_z( cartman_mpd_t * pmesh, int z, Uint16 tiletype, Uint16 tileand )
{
    int vert, cnt, newz, mapx, mapy;

    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    tiletype = tiletype & tileand;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            pdef = TILE_DICT_PTR( tile_dict, pfan->type );
            if ( NULL == pdef ) continue;

            if ( tiletype == ( pfan->tx_bits&tileand ) )
            {
                vert = pfan->vrtstart;
                for ( cnt = 0; cnt < pdef->numvertices; cnt++ )
                {
                    newz = pmesh->vrt[vert].z + z;
                    if ( newz < 0 )  newz = 0;
                    if ( newz > pmesh->info.edgez ) newz = pmesh->info.edgez;
                    pmesh->vrt[vert].z = newz;

                    vert = pmesh->vrt[vert].next;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_vert( cartman_mpd_t * pmesh, int vert, float x, float y, float z )
{
    int newx, newy, newz;

    if ( NULL == pmesh ) pmesh = &mesh;

    newx = pmesh->vrt[vert].x + x;
    newy = pmesh->vrt[vert].y + y;
    newz = pmesh->vrt[vert].z + z;

    if ( newx < 0 )  newx = 0;
    if ( newx > pmesh->info.edgex )  newx = pmesh->info.edgex;

    if ( newy < 0 )  newy = 0;
    if ( newy > pmesh->info.edgey )  newy = pmesh->info.edgey;

    if ( newz < -pmesh->info.edgez )  newz = -pmesh->info.edgez;
    if ( newz > pmesh->info.edgez )  newz = pmesh->info.edgez;

    pmesh->vrt[vert].x = newx;
    pmesh->vrt[vert].y = newy;
    pmesh->vrt[vert].z = newz;
}

//--------------------------------------------------------------------------------------------
void raise_mesh( cartman_mpd_t * pmesh, Uint32 point_lst[], size_t point_cnt, float x, float y, int amount, int size )
{
    int disx, disy, dis, cnt, newamount;
    Uint32 vert;

    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == point_lst || 0 == point_cnt ) return;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    for ( cnt = 0; cnt < point_cnt; cnt++ )
    {
        vert = point_lst[cnt];
        if ( !CART_VALID_VERTEX_RANGE( vert ) ) break;

        disx = vlst[vert].x - x;
        disy = vlst[vert].y - y;
        dis = sqrt(( float )( disx * disx + disy * disy ) );

        newamount = abs( amount ) - (( dis << 1 ) >> size );
        if ( newamount < 0 ) newamount = 0;
        if ( amount < 0 )  newamount = -newamount;

        move_vert( pmesh,  vert, 0, 0, newamount );
    }
}

//--------------------------------------------------------------------------------------------
void level_vrtz( cartman_mpd_t * pmesh )
{
    int mapx, mapy, cnt;
    Uint32 vert;

    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            pdef = TILE_DICT_PTR( tile_dict, pfan->type );
            if ( NULL == pdef ) continue;

            vert = pfan->vrtstart;
            for ( cnt = 0; cnt < pdef->numvertices; cnt++ )
            {
                vlst[vert].z = 0;
                vert = vlst[vert].next;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void jitter_mesh( cartman_mpd_t * pmesh )
{
    int mapx, mapy, num, cnt;
    Uint32 vert;

    select_lst_t loc_lst = SELECT_LST_INIT;

    // aliases
    cartman_mpd_vertex_t * vlst = NULL;
    tile_definition_t    * pdef = NULL;
    cartman_mpd_tile_t   * pfan = NULL;

    // grab the correct mesh
    if ( NULL == pmesh ) pmesh = &mesh;

    // get the alias
    vlst = pmesh->vrt;

    // initialize the local selection
    select_lst_init( &loc_lst, pmesh );

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            pdef = TILE_DICT_PTR( tile_dict, pfan->type );
            if ( NULL == pdef ) continue;

            num = pdef->numvertices;

            // clear the selection
            select_lst_clear( &loc_lst );

            // add all the tile vertices
            for ( cnt = 0, vert = pfan->vrtstart;
                  cnt < num;
                  cnt++, vert = vlst[vert].next )
            {
                select_lst_add( &loc_lst, vert );
            }

            // jitter the tile verts
            mesh_select_move( &loc_lst, ( rand()&7 ) - 3, ( rand()&7 ) - 3, ( rand()&63 ) - 32 );
        }
    }
}

//--------------------------------------------------------------------------------------------
void flatten_mesh( cartman_mpd_t * pmesh, int y0 )
{
    int mapx, mapy, num, cnt;
    Uint32 vert;
    int height;

    cartman_mpd_tile_t   * pfan = NULL;
    cartman_mpd_vertex_t * vlst = NULL;
    tile_definition_t    * pdef = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    height = ( 780 - ( y0 ) ) * 4;
    if ( height < 0 )  height = 0;
    if ( height > pmesh->info.edgez ) height = pmesh->info.edgez;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            pdef = TILE_DICT_PTR( tile_dict, pfan->type );
            if ( NULL == pdef ) continue;

            num = pdef->numvertices;

            vert = pfan->vrtstart;
            for ( cnt = 0; cnt < num; cnt++ )
            {
                float ftmp = vlst[vert].z - height;

                if ( ABS( ftmp ) < 50 )
                {
                    vlst[vert].z = height;
                }

                vert = vlst[vert].next;
            }
        }
    }

}

//--------------------------------------------------------------------------------------------
void clear_mesh( cartman_mpd_t * pmesh, Uint8 upper, Uint16 presser, Uint8 tx, Uint8 type )
{
    int mapx, mapy;
    int loc_type = type;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( !TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {

        for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
        {
            for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
            {
                int tx_bits;

                pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
                if ( NULL == pfan ) continue;

                cartman_mpd_remove_pfan( pmesh, pfan );

                tx_bits = TILE_SET_UPPER_BITS( upper );
                switch ( presser )
                {
                    case 0:
                        tx_bits |= tx & 0xFF;
                        break;
                    case 1:
                        tx_bits |= ( tx & 0xFE ) | ( rand() & 1 );
                        break;
                    case 2:
                        tx_bits |= ( tx & 0xFC ) | ( rand() & 3 );
                        break;
                    case 3:
                        tx_bits |= ( tx & 0xF8 ) | ( rand() & 7 );
                        break;
                    default:
                        tx_bits = pfan->tx_bits;
                        break;
                }
                pfan->tx_bits = tx_bits;

                loc_type = type;
                if ( loc_type <= 1 ) loc_type = rand() & 1;
                if ( loc_type == 32 || loc_type == 33 ) loc_type = 32 + ( rand() & 1 );
                pfan->type = loc_type;

                cartman_mpd_add_pfan( pmesh, pfan, mapx * TILE_ISIZE, mapy * TILE_ISIZE );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void three_e_mesh( cartman_mpd_t * pmesh, Uint8 upper, Uint8 tx )
{
    // ZZ> Replace all 3F tiles with 3E tiles...

    int mapx, mapy;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {
        return;
    }

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            if ( 0x3F == pfan->tx_bits )
            {
                pfan->tx_bits = 0x3E;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t fan_is_floor( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    cartman_mpd_tile_t * pfan;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
    if ( NULL == pfan ) return bfalse;

    return !HAS_BITS( pfan->fx, ( MAPFX_WALL | MAPFX_IMPASS ) );
}

//--------------------------------------------------------------------------------------------
bool_t fan_is_wall( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    cartman_mpd_tile_t * pfan;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
    if ( NULL == pfan ) return btrue;

    return HAS_BITS( pfan->fx, ( MAPFX_WALL | MAPFX_IMPASS ) );
}

//--------------------------------------------------------------------------------------------
#define BARRIER_FUNC(XX) ( 1.0f - (1.0f - (XX)) * (1.0f - (XX)) * (1.0f - (XX)) )

void set_barrier_height( cartman_mpd_t * pmesh, int mapx, int mapy )
{
    Uint32 vert, vert_count;
    int cnt;
    bool_t noedges, nocorners;
    float bestprox, prox, tprox, max_hgt, min_hgt;

    float corner_hgt[4];

    bool_t floor_mx, floor_px, floor_my, floor_py;
    bool_t floor_mxmy, floor_mxpy, floor_pxmy, floor_pxpy;

    cartman_mpd_tile_t   * pfan   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    // does the fan exist?
    pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
    if ( NULL == pfan ) return;

    // fan is defined?
    pdef = TILE_DICT_PTR( tile_dict, pfan->type );
    if ( NULL == pdef || 0 == pdef->numvertices ) return;
    vert_count = pdef->numvertices;

    // must not be a floor
    if ( fan_is_floor( pmesh,  mapx, mapy ) ) return;

    // find other walls around this tile
    floor_mx = fan_is_floor( pmesh,  mapx - 1, mapy );
    floor_px = fan_is_floor( pmesh,  mapx + 1, mapy );
    floor_my = fan_is_floor( pmesh,  mapx, mapy - 1 );
    floor_py = fan_is_floor( pmesh,  mapx, mapy + 1 );
    noedges  = !floor_mx && !floor_px && !floor_my && !floor_py;

    floor_mxmy = fan_is_floor( pmesh,  mapx - 1, mapy - 1 );
    floor_mxpy = fan_is_floor( pmesh,  mapx - 1, mapy + 1 );
    floor_pxmy = fan_is_floor( pmesh,  mapx + 1, mapy - 1 );
    floor_pxpy = fan_is_floor( pmesh,  mapx + 1, mapy + 1 );
    nocorners = !floor_mxmy && !floor_mxpy && !floor_pxmy && !floor_pxpy;

    // if it is completely surrounded by walls, there's nothing to do
    if ( noedges && nocorners ) return;

    // initialize the min/max values
    vert       = pfan->vrtstart;
    corner_hgt[0] = vlst[vert].z;
    min_hgt = corner_hgt[0];
    max_hgt = corner_hgt[0];
    vert = vlst[vert].next;

    // iterate through the corners
    for ( cnt = 1; cnt < 4 && CHAINEND != vert; vert = vlst[vert].next, cnt++ )
    {
        corner_hgt[cnt] = vlst[vert].z;
        min_hgt = MIN( min_hgt, corner_hgt[cnt] );
        max_hgt = MAX( max_hgt, corner_hgt[cnt] );
    }

    // correct all vertices
    for ( cnt = 0, vert = pfan->vrtstart; cnt < vert_count && CHAINEND != vert; cnt++, vert = vlst[vert].next )
    {
        float ftmp, weight;
        float vsum, wsum;

        vsum = 0.0f;
        wsum = 0.0f;

        if ( !noedges )
        {
            if ( !floor_mx )
            {
                weight = ( float )( pdef->grid_iy[cnt] ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_BL] + ( 1.0f - weight ) * corner_hgt[CORNER_TL];
                ftmp  -= min_hgt;

                weight = ( float )( 3 - pdef->grid_ix[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_px )
            {
                weight = ( float )( pdef->grid_iy[cnt] ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_BR] + ( 1.0f - weight ) * corner_hgt[CORNER_TR];
                ftmp  -= min_hgt;

                weight = ( float )( pdef->grid_ix[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_my )
            {
                weight = ( float )( 3 - pdef->grid_ix[cnt] ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_TL] + ( 1.0f - weight ) * corner_hgt[CORNER_TR];
                ftmp  -= min_hgt;

                weight = ( float )( 3 - pdef->grid_iy[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_py )
            {
                weight = ( float )( pdef->grid_ix[cnt] ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_BR] + ( 1.0f - weight ) * corner_hgt[CORNER_BL];
                ftmp  -= min_hgt;

                weight = ( float )( pdef->grid_iy[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }
        }

        if ( !nocorners )
        {
            if ( !floor_pxpy )
            {
                ftmp = corner_hgt[CORNER_BR] - min_hgt;

                weight = ( float )MAX( pdef->grid_ix[cnt], pdef->grid_iy[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_pxmy )
            {
                ftmp = corner_hgt[CORNER_TR] - min_hgt;

                weight = ( float )MIN( pdef->grid_ix[cnt], 3 - pdef->grid_iy[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_mxpy )
            {
                ftmp = corner_hgt[CORNER_BL] - min_hgt;

                weight = ( float )MIN( 3 - pdef->grid_ix[cnt], pdef->grid_iy[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_mxmy )
            {
                ftmp = corner_hgt[CORNER_TL] - min_hgt;

                weight = ( float )MIN( 3 - pdef->grid_ix[cnt], 3 - pdef->grid_iy[cnt] ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }
        }

        if ( 0.0f == wsum )
        {
            vsum = vlst[vert].z - min_hgt;
            wsum = 1.0f;
        }

        //scale = window_lst[mdata.window_id].surfacey - (mdata.rect_y0);
        //bestprox = bestprox * scale * BARRIERHEIGHT / window_lst[mdata.window_id].surfacey;

        //if (bestprox > pmesh->info.edgez) bestprox = pmesh->info.edgez;
        //if (bestprox < 0) bestprox = 0;

        if ( wsum > 0.0f )
        {
            ftmp = vsum / wsum;
        }
        else
        {
            ftmp = 0.0f;
        }

        // interpolate a value
        vlst[vert].z = ftmp + min_hgt;
    }
}

//--------------------------------------------------------------------------------------------
void fix_walls( cartman_mpd_t * pmesh )
{
    int mapx, mapy;

    if ( NULL == pmesh ) pmesh = &mesh;

    // make sure the corners are correct
    fix_corners( pmesh );

    // adjust the wall-icity of all non-corner vertices
    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            set_barrier_height( pmesh, mapx, mapy );
        }
    }
}

//--------------------------------------------------------------------------------------------
void impass_edges( cartman_mpd_t * pmesh, int amount )
{
    int mapx, mapy;
    cartman_mpd_tile_t * pfan;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            if ( dist_from_edge( pmesh,  mapx, mapy ) < amount )
            {
                pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
                if ( NULL == pfan ) continue;

                pfan->fx |= MAPFX_IMPASS;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_replace_fx( cartman_mpd_t * pmesh, Uint16 fx_bits, Uint16 fx_mask, Uint8 fx_new )
{
    // ZZ> This function sets the fx for a group of tiles

    int mapx, mapy;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    // trim away any bits that can't be matched
    fx_bits &= fx_mask;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            if ( fx_bits == ( pfan->tx_bits&fx_mask ) )
            {
                pfan->fx = fx_new;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 tile_is_different( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits, Uint16 fx_mask )
{
    // ZZ> bfalse if of same set, btrue if different

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
    if ( NULL == pfan ) return bfalse;

    if ( fx_mask == 0xC0 )
    {
        if ( pfan->tx_bits >= ( MAPFX_WALL | MAPFX_IMPASS ) )
        {
            return bfalse;
        }
    }

    if ( fx_bits == ( pfan->tx_bits&fx_mask ) )
    {
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 trim_code( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    Uint16 code;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( tile_is_different( pmesh,  mapx, mapy - 1, fx_bits, 0xF0 ) )
    {
        // Top
        code = 0;
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xF0 ) )
        {
            // Left
            code = 8;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xF0 ) )
        {
            // Right
            code = 9;
        }
        return code;
    }

    if ( tile_is_different( pmesh,  mapx, mapy + 1, fx_bits, 0xF0 ) )
    {
        // Bottom
        code = 1;
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xF0 ) )
        {
            // Left
            code = 10;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xF0 ) )
        {
            // Right
            code = 11;
        }
        return code;
    }

    if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xF0 ) )
    {
        // Left
        code = 2;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xF0 ) )
    {
        // Right
        code = 3;
        return code;
    }

    if ( tile_is_different( pmesh,  mapx + 1, mapy + 1, fx_bits, 0xF0 ) )
    {
        // Bottom Right
        code = 4;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy + 1, fx_bits, 0xF0 ) )
    {
        // Bottom Left
        code = 5;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx + 1, mapy - 1, fx_bits, 0xF0 ) )
    {
        // Top Right
        code = 6;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy - 1, fx_bits, 0xF0 ) )
    {
        // Top Left
        code = 7;
        return code;
    }

    // unknown
    code = 255;

    return code;
}

//--------------------------------------------------------------------------------------------
Uint16 wall_code( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    Uint16 code;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( tile_is_different( pmesh,  mapx, mapy - 1, fx_bits, 0xC0 ) )
    {
        // Top
        code = ( rand() & 2 ) + 20;
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xC0 ) )
        {
            // Left
            code = 48;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xC0 ) )
        {
            // Right
            code = 50;
        }

        return code;
    }

    if ( tile_is_different( pmesh,  mapx, mapy + 1, fx_bits, 0xC0 ) )
    {
        // Bottom
        code = ( rand() & 2 );
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xC0 ) )
        {
            // Left
            code = 52;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xC0 ) )
        {
            // Right
            code = 54;
        }
        return code;
    }

    if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xC0 ) )
    {
        // Left
        code = ( rand() & 2 ) + 16;
        return code;
    }

    if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xC0 ) )
    {
        // Right
        code = ( rand() & 2 ) + 4;
        return code;
    }

    if ( tile_is_different( pmesh,  mapx + 1, mapy + 1, fx_bits, 0xC0 ) )
    {
        // Bottom Right
        code = 32;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy + 1, fx_bits, 0xC0 ) )
    {
        // Bottom Left
        code = 34;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx + 1, mapy - 1, fx_bits, 0xC0 ) )
    {
        // Top Right
        code = 36;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy - 1, fx_bits, 0xC0 ) )
    {
        // Top Left
        code = 38;
        return code;
    }

    // unknown
    code = 255;

    return code;
}

//--------------------------------------------------------------------------------------------
void trim_mesh_tile( cartman_mpd_t * pmesh, Uint16 fx_bits, Uint16 fx_mask )
{
    // ZZ> This function trims walls and floors and tops automagically

    int mapx, mapy, code;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    fx_bits = fx_bits & fx_mask;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            if ( fx_bits == ( pfan->tx_bits&fx_mask ) )
            {
                if ( fx_mask == 0xC0 )
                {
                    code = wall_code( pmesh,  mapx, mapy, fx_bits );
                }
                else
                {
                    code = trim_code( pmesh,  mapx, mapy, fx_bits );
                }

                if ( code != 255 )
                {
                    pfan->tx_bits = fx_bits + code;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_replace_tile( cartman_mpd_t * pmesh, int _xfan, int _yfan, int _onfan, Uint8 _tx, Uint8 _upper, Uint8 _fx, Uint8 _type, Uint16 _presser, bool_t tx_only, bool_t at_floor_level )
{
    cart_vec_t pos[CORNER_COUNT];
    int tx_bits;
    int vert;

    cartman_mpd_tile_t   * pfan   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    pfan = CART_MPD_FAN_PTR( pmesh, _onfan );
    if ( NULL == pfan ) return;

    if ( !tx_only )
    {
        if ( !at_floor_level )
        {
            // Save corner positions
            vert = pfan->vrtstart;
            pos[CORNER_TL][kX] = vlst[vert].x;
            pos[CORNER_TL][kY] = vlst[vert].y;
            pos[CORNER_TL][kZ] = vlst[vert].z;

            vert = vlst[vert].next;
            pos[CORNER_TR][kX] = vlst[vert].x;
            pos[CORNER_TR][kY] = vlst[vert].y;
            pos[CORNER_TR][kZ] = vlst[vert].z;

            vert = vlst[vert].next;
            pos[CORNER_BR][kX] = vlst[vert].x;
            pos[CORNER_BR][kY] = vlst[vert].y;
            pos[CORNER_BR][kZ] = vlst[vert].z;

            vert = vlst[vert].next;
            pos[CORNER_BL][kX] = vlst[vert].x;
            pos[CORNER_BL][kY] = vlst[vert].y;
            pos[CORNER_BL][kZ] = vlst[vert].z;
        }
        cartman_mpd_remove_ifan( pmesh, _onfan );
    }

    // set the texture info
    tx_bits = TILE_SET_UPPER_BITS( _upper );
    switch ( _presser )
    {
        case 0:
            tx_bits |= _tx & 0xFF;
            break;
        case 1:
            tx_bits |= ( _tx & 0xFE ) | ( rand() & 1 );
            break;
        case 2:
            tx_bits |= ( _tx & 0xFC ) | ( rand() & 3 );
            break;
        case 3:
            tx_bits |= ( _tx & 0xF8 ) | ( rand() & 7 );
            break;
        default:
            tx_bits = pfan->tx_bits;
            break;
    };
    pfan->tx_bits = tx_bits;

    if ( !tx_only )
    {
        pfan->type = _type;
        cartman_mpd_add_ifan( pmesh, _onfan, _xfan * TILE_ISIZE, _yfan * TILE_ISIZE );
        pfan->fx = _fx;

        if ( 0 /*!at_floor_level*/ )
        {
            // Return corner positions
            vert = pfan->vrtstart;
            vlst[vert].x = pos[CORNER_TL][kX];
            vlst[vert].y = pos[CORNER_TL][kY];
            vlst[vert].z = pos[CORNER_TL][kZ];

            vert = vlst[vert].next;
            vlst[vert].x = pos[CORNER_TR][kX];
            vlst[vert].y = pos[CORNER_TR][kY];
            vlst[vert].z = pos[CORNER_TR][kZ];

            vert = vlst[vert].next;
            vlst[vert].x = pos[CORNER_BR][kX];
            vlst[vert].y = pos[CORNER_BR][kY];
            vlst[vert].z = pos[CORNER_BR][kZ];

            vert = vlst[vert].next;
            vlst[vert].x = pos[CORNER_BL][kX];
            vlst[vert].y = pos[CORNER_BL][kY];
            vlst[vert].z = pos[CORNER_BL][kZ];
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_fx( cartman_mpd_t * pmesh, int fan, Uint8 fx )
{
    cartman_mpd_tile_t * pfan;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return;

    pmesh->fan[fan].fx = fx;
}

//--------------------------------------------------------------------------------------------
void mesh_move( cartman_mpd_t * pmesh, float dx, float dy, float dz )
{
    Uint32 vert;
    int mapx, mapy, cnt;

    cartman_mpd_tile_t * pfan   = NULL;
    tile_definition_t  * pdef    = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            int count;

            pfan = cartman_mpd_get_pfan( pmesh, mapx, mapy );
            if ( NULL == pfan ) continue;

            pdef = TILE_DICT_PTR( tile_dict, pfan->type );
            if ( NULL == pdef ) continue;

            count = pdef->numvertices;

            for ( cnt = 0, vert = pfan->vrtstart;
                  cnt < count && CHAINEND != vert;
                  cnt++, vert = pmesh->vrt[vert].next )
            {
                move_vert( pmesh,  vert, dx, dy, dz );
            }
        }
    }
}
