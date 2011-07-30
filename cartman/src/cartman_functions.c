//********************************************************************************************
//*
//*    This file is part of Cartman.
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

#include "cartman_functions.h"

#include "cartman.h"
#include "cartman_mpd.h"
#include "cartman_select.h"

#include "cartman_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enum
{
    CORNER_TL,
    CORNER_TR,
    CORNER_BL,
    CORNER_BR,
    CORNER_COUNT
};

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
int dist_from_edge( cartman_mpd_t * pmesh, int x, int y )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    if ( x > ( pmesh->info.tiles_x >> 1 ) )
        x = pmesh->info.tiles_x - x - 1;
    if ( y > ( pmesh->info.tiles_y >> 1 ) )
        y = pmesh->info.tiles_y - y - 1;

    if ( x < y )
        return x;

    return y;
}

//--------------------------------------------------------------------------------------------
void fix_mesh( cartman_mpd_t * pmesh )
{
    // ZZ> This function corrects corners across entire mesh
    int mapx, mapy;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            //      fix_corners( pmesh, pmesh, mapx, mapy);
            fix_vertices( pmesh, mapx, mapy );
        }
    }
}

//--------------------------------------------------------------------------------------------
void fix_vertices( cartman_mpd_t * pmesh, int x, int y )
{
    int fan;
    int cnt;

    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    fix_corners( pmesh,  x, y );

    fan = cartman_mpd_get_fan( pmesh, x, y );

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;
    pfan = pmesh->fan + fan;

    if ( pfan->type >= MPD_FAN_TYPE_MAX ) return;
    pdef = tile_dict + pfan->type;

    for ( cnt = 4; cnt < pdef->numvertices; cnt++ )
    {
        weld_cnt( pmesh,  x, y, cnt, fan );
    }
}

//--------------------------------------------------------------------------------------------
void fix_corners( cartman_mpd_t * pmesh, int x, int y )
{
    int fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;

    weld_0( pmesh, x, y );
    weld_1( pmesh, x, y );
    weld_2( pmesh, x, y );
    weld_3( pmesh, x, y );
}

//--------------------------------------------------------------------------------------------
void weld_0( cartman_mpd_t * pmesh, int x, int y )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init(&loc_lst, pmesh);

    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y, 0 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x - 1, y, 1 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y - 1, 3 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x - 1, y - 1, 2 ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_1( cartman_mpd_t * pmesh, int x, int y )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init(&loc_lst, pmesh);

    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y, 1 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x + 1, y, 0 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y - 1, 2 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x + 1, y - 1, 3 ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_2( cartman_mpd_t * pmesh, int x, int y )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init(&loc_lst, pmesh);

    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y, 2 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x + 1, y, 3 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y + 1, 1 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x + 1, y + 1, 0 ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_3( cartman_mpd_t * pmesh, int x, int y )
{
    select_lst_t loc_lst = SELECT_LST_INIT;

    select_lst_init(&loc_lst, pmesh);

    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y, 3 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x - 1, y, 2 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y + 1, 0 ) );
    select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x - 1, y + 1, 1 ) );

    mesh_select_weld( &loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_cnt( cartman_mpd_t * pmesh, int x, int y, int cnt, Uint32 fan )
{
    cartman_mpd_tile_t   * pfan    = NULL;
    tile_definition_t    * pdef    = NULL;

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;
    pfan = pmesh->fan + fan;

    if ( pfan->type >= MPD_FAN_TYPE_MAX ) return;
    pdef = tile_dict + pfan->type;

    if ( pdef->u[cnt] * TILE_FSIZE  < NEARLOW + 1.0f ||
         pdef->v[cnt] * TILE_FSIZE  < NEARLOW + 1.0f ||
         pdef->u[cnt] * TILE_FSIZE  > NEARHI - 1.0f ||
         pdef->v[cnt] * TILE_FSIZE  > NEARHI - 1.0f )
    {
        select_lst_t loc_lst = SELECT_LST_INIT;

        select_lst_init(&loc_lst, pmesh);

        // add the center point
        select_lst_add( &loc_lst, cartman_mpd_get_vertex( loc_lst.pmesh, x, y, cnt ) );

        if ( pdef->u[cnt] * TILE_FSIZE < NEARLOW + 1 )
            select_lst_add( &loc_lst, nearest_vertex( loc_lst.pmesh,  x - 1, y, NEARHI, pdef->v[cnt] * TILE_FSIZE ) );

        if ( pdef->v[cnt] * TILE_FSIZE < NEARLOW + 1 )
            select_lst_add( &loc_lst, nearest_vertex( loc_lst.pmesh,  x, y - 1, pdef->u[cnt] * TILE_FSIZE, NEARHI ) );

        if ( pdef->u[cnt] * TILE_FSIZE > NEARHI - 1 )
            select_lst_add( &loc_lst, nearest_vertex( loc_lst.pmesh,  x + 1, y, NEARLOW, pdef->v[cnt] * TILE_FSIZE ) );

        if ( pdef->v[cnt] * TILE_FSIZE > NEARHI - 1 )
            select_lst_add( &loc_lst, nearest_vertex( loc_lst.pmesh,  x, y + 1, pdef->u[cnt] * TILE_FSIZE, NEARLOW ) );

        mesh_select_weld( &loc_lst );
    }
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
    if( NULL == plst ) return plst;

    if ( NULL == plst->pmesh ) return plst;
    pmesh = plst->pmesh;

    // get the vertex list
    vlst = pmesh->vrt;

    // if the selection is empty, we're done
    if( x0 == x1 || y0 == y1 || z0 == z1 ) return plst;

    // make sure that the selection is ordered properly
    if ( x0 < x1 ) { xmin = x0; xmax = x1; } else { xmin = x1; xmax = x0; };
    if ( y0 < y1 ) { ymin = y0; ymax = y1; } else { ymin = y1; ymax = y0; };
    if ( z0 < z1 ) { zmin = z0; zmax = z1; } else { zmin = z1; zmax = z0; };

    if ( mode == WINMODE_VERTEX )
    {
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MPD_VERTICES_MAX; ivrt++, pvrt++ )
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
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MPD_VERTICES_MAX; ivrt++, pvrt++ )
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
    if( NULL == plst ) return plst;

    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get the vertex list
    vlst = pmesh->vrt;

    // if the selection is empty, we're done
    if( x0 == x1 || y0 == y1 || z0 == z1 ) return plst;

    // make sure that the selection is ordered properly
    if ( x0 < x1 ) { xmin = x0; xmax = x1; } else { xmin = x1; xmax = x0; };
    if ( y0 < y1 ) { ymin = y0; ymax = y1; } else { ymin = y1; ymax = y0; };
    if ( z0 < z1 ) { zmin = z0; zmax = z1; } else { zmin = z1; zmax = z0; };

    if ( mode == WINMODE_VERTEX )
    {
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MPD_VERTICES_MAX; ivrt++, pvrt++ )
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
        for ( ivrt = 0, pvrt = vlst + 0; ivrt < MPD_VERTICES_MAX; ivrt++, pvrt++ )
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
int nearest_vertex( cartman_mpd_t * pmesh, int x, int y, float nearx, float neary )
{
    // ZZ> This function gets a vertex number or -1
    int ivrt, bestvert, cnt;
    int fan;
    int num;
    float prox, proxx, proxy, bestprox;

    // aliases
    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( fan < 0 || fan >= MPD_TILE_MAX ) return -1;
    pfan = pmesh->fan + fan;

    if ( pfan->type >= MPD_FAN_TYPE_MAX ) return -1;
    pdef = tile_dict + pfan->type;

    num = pdef->numvertices;

    bestvert = -1;
    if ( num > 4 )
    {
        ivrt = pfan->vrtstart;

        // skip over the 4 corner vertices
        for ( cnt = 0; cnt < 4; cnt++ )
        {
            ivrt = vlst[ivrt].next;
        }

        bestprox = 9000;
        for ( cnt = 4; cnt < num; cnt++ )
        {
            proxx = pdef->u[cnt] * TILE_FSIZE - nearx;
            proxy = pdef->v[cnt] * TILE_FSIZE - neary;
            prox = ABS( proxx ) + ABS( proxy );

            if ( prox < bestprox )
            {
                bestvert = ivrt;
                bestprox = prox;
            }

            ivrt = vlst[ivrt].next;
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

    if( NULL == plst ) return;

    // get the proper mesh
    pmesh = plst->pmesh;
    if( NULL == pmesh ) pmesh = &mesh;

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

    if( NULL == plst ) return;

    // get the mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get vertex list alias
    vlst = pmesh->vrt;

    for ( cnt = 0; cnt < plst->count; cnt++ )
    {
        vert = plst->which[cnt];
        if( vert < 0 || vert > pmesh->info.vertex_count ) continue;

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

    if( NULL == plst ) return;

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
    int fan;
    Uint8 select_vertsfan;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    cartman_mpd_info_t   * pinfo  = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;
    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;

    if( NULL == plst ) plst = select_lst_default();

    // get the mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get vertex list alias
    pinfo = &(pmesh->info);
    vlst  = pmesh->vrt;

    for ( mapy = 0; mapy < pinfo->tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pinfo->tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

            if ( pfan->type >= MPD_FAN_TYPE_MAX ) continue;
            pdef = tile_dict + pfan->type;

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
    int cnt, x, y, z, a;
    Uint32 vert;

    // aliases
    cartman_mpd_t        * pmesh  = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if( NULL == plst ) return;

    // get the mesh
    pmesh = plst->pmesh;
    if ( NULL == pmesh ) pmesh = &mesh;

    // get vertex list alias
    vlst = pmesh->vrt;

    if ( plst->count > 1 )
    {
        x = 0;
        y = 0;
        z = 0;
        a = 0;

        for ( cnt = 0; cnt < plst->count; cnt++ )
        {
            vert = plst->which[cnt];
            x += vlst[vert].x;
            y += vlst[vert].y;
            z += vlst[vert].z;
            a += vlst[vert].a;
        }
        x += cnt >> 1;  y += cnt >> 1;
        x = x / plst->count;
        y = y / plst->count;
        z = z / plst->count;
        a = a / plst->count;

        for ( cnt = 0; cnt < plst->count; cnt++ )
        {
            vert = plst->which[cnt];
            vlst[vert].x = x;
            vlst[vert].y = y;
            vlst[vert].z = z;
            vlst[vert].a = CLIP( a, 1, 255 );
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_tile( cartman_mpd_t * pmesh, Uint16 tiletoset, Uint8 upper, Uint16 presser, Uint8 tx )
{
    // ZZ> This function sets one tile type to another
    int fan;
    int mapx, mapy;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

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
    int fan;

    cartman_mpd_tile_t   * pfan   = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    tiletype = tiletype & tileand;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

            if ( pfan->type >= MPD_FAN_TYPE_MAX ) continue;
            pdef = tile_dict + pfan->type;

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
        if ( vert >= MPD_VERTICES_MAX ) break;

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
    int mapx, mapy, fan, cnt;
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
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

            if ( pfan->type >= MPD_FAN_TYPE_MAX ) continue;
            pdef = tile_dict + pfan->type;

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
    int mapx, mapy, fan, num, cnt;
    Uint32 vert;

    select_lst_t loc_lst = SELECT_LST_INIT;

    // aliases
    cartman_mpd_vertex_t * vlst    = NULL;

    // grab the correct mesh
    if ( NULL == pmesh ) pmesh = &mesh;

    // get the alias
    vlst = pmesh->vrt;

    // initialize the local selection
    select_lst_init(&loc_lst, pmesh);

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            cartman_mpd_tile_t   * pfan = NULL;

            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

            if ( pfan->type >= MPD_FAN_TYPE_MAX ) continue;
            num = tile_dict[pfan->type].numvertices;

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
    int mapx, mapy, fan, num, cnt;
    Uint32 vert;
    int height;

    cartman_mpd_tile_t   * pfan   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    height = ( 780 - ( y0 ) ) * 4;
    if ( height < 0 )  height = 0;
    if ( height > pmesh->info.edgez ) height = pmesh->info.edgez;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

            if ( pfan->type >= MPD_FAN_TYPE_MAX ) continue;
            num = tile_dict[pfan->type].numvertices;

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
    int fan;
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

                fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

                if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
                pfan = pmesh->fan + fan;

                cartman_mpd_remove_fan( pmesh, fan );

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

                cartman_mpd_add_fan( pmesh, fan, mapx * TILE_ISIZE, mapy * TILE_ISIZE );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void three_e_mesh( cartman_mpd_t * pmesh, Uint8 upper, Uint8 tx )
{
    // ZZ> Replace all 3F tiles with 3E tiles...

    int mapx, mapy;
    int fan;

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
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

            if ( 0x3F == pfan->tx_bits )
            {
                pfan->tx_bits = 0x3E;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t fan_is_floor( cartman_mpd_t * pmesh, int x, int y )
{
    int fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( fan < 0 || fan >= MPD_TILE_MAX ) return bfalse;

    return !HAS_BITS( pmesh->fan[fan].fx, ( MPDFX_WALL | MPDFX_IMPASS ) );
}

//--------------------------------------------------------------------------------------------
void set_barrier_height( cartman_mpd_t * pmesh, int x, int y, int bits )
{
    Uint32 fantype, fan, vert, vert_count;
    int cnt, noedges;
    float bestprox, prox, tprox, max_height, min_height;

    bool_t floor_mx, floor_px, floor_my, floor_py;
    bool_t floor_mxmy, floor_mxpy, floor_pxmy, floor_pxpy;

    cartman_mpd_tile_t   * pfan   = NULL;
    cartman_mpd_vertex_t * vlst   = NULL;
    tile_definition_t    * pdef   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;
    vlst = pmesh->vrt;

    fan = cartman_mpd_get_fan( pmesh, x, y );

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;
    pfan = pmesh->fan + fan;

    // bust be a MPDFX_WALL
    if ( !HAS_BITS( pfan->fx, bits ) ) return;

    floor_mx   = fan_is_floor( pmesh,  x - 1, y );
    floor_px   = fan_is_floor( pmesh,  x + 1, y );
    floor_my   = fan_is_floor( pmesh,  x, y - 1 );
    floor_py   = fan_is_floor( pmesh,  x, y + 1 );
    noedges = !( floor_mx || floor_px || floor_my || floor_py );

    floor_mxmy = fan_is_floor( pmesh,  x - 1, y - 1 );
    floor_mxpy = fan_is_floor( pmesh,  x - 1, y + 1 );
    floor_pxmy = fan_is_floor( pmesh,  x + 1, y - 1 );
    floor_pxpy = fan_is_floor( pmesh,  x + 1, y + 1 );

    // valid fantype?
    fantype = pfan->type;
    if ( fantype >= MPD_FAN_TYPE_MAX ) return;

    // fan is defined?
    pdef = tile_dict + fantype;
    if ( 0 == pdef->numvertices ) return;

    vert_count = pdef->numvertices;

    vert       = pfan->vrtstart;

    min_height = vlst[vert].z;
    max_height = vlst[vert].z;
    vert       = vlst[vert].next;
    for ( cnt = 1; cnt < vert_count; cnt++ )
    {
        min_height = MIN( min_height, vlst[vert].z );
        max_height = MAX( max_height, vlst[vert].z );
        vert       = vlst[vert].next;
    }

    vert = pfan->vrtstart;
    for ( cnt = 0; cnt < vert_count; cnt++ )
    {
        float ftmp;

        bestprox = NEARHI; // 2.0f / 3.0f * (NEARHI - NEARLOW);
        if ( floor_px )
        {
            prox = NEARHI - ( int )( pdef->u[cnt] * TILE_FSIZE );
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_py )
        {
            prox = NEARHI - ( int )( pdef->v[cnt] * TILE_FSIZE );
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_mx )
        {
            prox = ( int )( pdef->u[cnt] * TILE_FSIZE ) - NEARLOW;
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_my )
        {
            prox = ( int )( pdef->v[cnt] * TILE_FSIZE ) - NEARLOW;
            if ( prox < bestprox ) bestprox = prox;
        }

        if ( noedges )
        {
            // Surrounded by walls on all 4 sides, but it may be a corner piece
            if ( floor_pxpy )
            {
                prox  = NEARHI - ( int )( pdef->u[cnt] * TILE_FSIZE );
                tprox = NEARHI - ( int )( pdef->v[cnt] * TILE_FSIZE );
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_pxmy )
            {
                prox = NEARHI - ( int )( pdef->u[cnt] * TILE_FSIZE );
                tprox = ( int )( pdef->v[cnt] * TILE_FSIZE ) - NEARLOW;
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_mxpy )
            {
                prox = ( int )( pdef->u[cnt] * TILE_FSIZE ) - NEARLOW;
                tprox = NEARHI - ( int )( pdef->v[cnt] * TILE_FSIZE );
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_mxmy )
            {
                prox =  pdef->u[cnt] * TILE_FSIZE - NEARLOW;
                tprox =  pdef->v[cnt] * TILE_FSIZE - NEARLOW;
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
        }
        //scale = window_lst[mdata.window_id].surfacey - (mdata.rect_y0);
        //bestprox = bestprox * scale * BARRIERHEIGHT / window_lst[mdata.window_id].surfacey;

        //if (bestprox > pmesh->info.edgez) bestprox = pmesh->info.edgez;
        //if (bestprox < 0) bestprox = 0;

        ftmp = bestprox / TILE_ISIZE;
        ftmp = 1.0f - ftmp;
        ftmp *= ftmp * ftmp;
        ftmp = 1.0f - ftmp;

        vlst[vert].z = ftmp * ( max_height - min_height ) + min_height;
        vert = vlst[vert].next;
    }
}

//--------------------------------------------------------------------------------------------
void fix_walls( cartman_mpd_t * pmesh )
{
    int mapx, mapy;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            set_barrier_height( pmesh,  mapx, mapy, MPDFX_WALL | MPDFX_IMPASS );
        }
    }
}

//--------------------------------------------------------------------------------------------
void impass_edges( cartman_mpd_t * pmesh, int amount )
{
    int mapx, mapy;
    int fan;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            if ( dist_from_edge( pmesh,  mapx, mapy ) < amount )
            {
                fan = cartman_mpd_get_fan( pmesh, mapx, mapy );
                if ( fan >= 0 && fan < MPD_TILE_MAX )
                {
                    pmesh->fan[fan].fx |= MPDFX_IMPASS;
                }
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
            int fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

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
    int fan;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

    if ( fan < 0 || fan >= MPD_TILE_MAX ) return bfalse;
    pfan = pmesh->fan + fan;

    if ( fx_mask == 0xC0 )
    {
        if ( pfan->tx_bits >= ( MPDFX_WALL | MPDFX_IMPASS ) )
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
    int fan;
    int mapx, mapy, code;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    fx_bits = fx_bits & fx_mask;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

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

    if ( _onfan < 0 || _onfan >= MPD_TILE_MAX ) return;
    pfan = pmesh->fan + _onfan;

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
            pos[CORNER_BL][kX] = vlst[vert].x;
            pos[CORNER_BL][kY] = vlst[vert].y;
            pos[CORNER_BL][kZ] = vlst[vert].z;

            vert = vlst[vert].next;
            pos[CORNER_BR][kX] = vlst[vert].x;
            pos[CORNER_BR][kY] = vlst[vert].y;
            pos[CORNER_BR][kZ] = vlst[vert].z;
        }
        cartman_mpd_remove_fan( pmesh, _onfan );
    }

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
        cartman_mpd_add_fan( pmesh, _onfan, _xfan * TILE_ISIZE, _yfan * TILE_ISIZE );
        pfan->fx = _fx;
        if ( !at_floor_level )
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
            vlst[vert].x = pos[CORNER_BL][kX];
            vlst[vert].y = pos[CORNER_BL][kY];
            vlst[vert].z = pos[CORNER_BL][kZ];

            vert = vlst[vert].next;
            vlst[vert].x = pos[CORNER_BR][kX];
            vlst[vert].y = pos[CORNER_BR][kY];
            vlst[vert].z = pos[CORNER_BR][kZ];
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_fx( cartman_mpd_t * pmesh, int fan, Uint8 fx )
{
    if ( fan < 0 || fan >= MPD_TILE_MAX ) return;

    if ( NULL == pmesh ) pmesh = &mesh;

    pmesh->fan[fan].fx = fx;
}

//--------------------------------------------------------------------------------------------
void mesh_move( cartman_mpd_t * pmesh, float dx, float dy, float dz )
{
    int fan;
    Uint32 vert;
    int mapx, mapy, cnt;

    cartman_mpd_tile_t   * pfan   = NULL;

    if ( NULL == pmesh ) pmesh = &mesh;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            int count;

            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );

            if ( fan < 0 || fan >= MPD_TILE_MAX ) continue;
            pfan = pmesh->fan + fan;

            if ( pfan->type >= MPD_FAN_TYPE_MAX ) continue;
            count = tile_dict[pfan->type].numvertices;

            for ( cnt = 0, vert = pfan->vrtstart;
                  cnt < count && CHAINEND != vert;
                  cnt++, vert = pmesh->vrt[vert].next )
            {
                move_vert( pmesh,  vert, dx, dy, dz );
            }
        }
    }
}

