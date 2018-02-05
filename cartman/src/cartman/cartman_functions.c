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

#include "cartman/cartman_functions.h"

#include "cartman/cartman.h"
#include "cartman/cartman_map.h"
#include "cartman/cartman_select.h"
#include "cartman/cartman_math.h"
#include "cartman/View.hpp"
#include "egolib/FileFormats/Globals.hpp"

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

float dist_from_border( cartman_mpd_t& mesh, float x, float y )
{
    float x_dst, y_dst;

    if ( x < 0.0f ) x_dst = 0.0f;
    else if ( x > mesh.info.getEdgeX() * 0.5f ) x_dst = mesh.info.getEdgeX() - x;
    else if ( x > mesh.info.getEdgeX() ) x_dst = 0.0f;
    else x_dst = x;

    if ( y < 0.0f ) y_dst = 0.0f;
    else if ( y > mesh.info.getEdgeY() * 0.5f ) y_dst = mesh.info.getEdgeY() - y;
    else if ( y > mesh.info.getEdgeY() ) y_dst = 0.0f;
    else y_dst = x;

    return ( x < y ) ? x : y;
}

int dist_from_edge( cartman_mpd_t& mesh, Index2D index2d )
{
    if ( index2d.x() > ( mesh.info.getTileCountX() >> 1 ) )
        index2d.x() = mesh.info.getTileCountX() - index2d.x() - 1;
    if ( index2d.y() > ( mesh.info.getTileCountY() >> 1 ) )
        index2d.y() = mesh.info.getTileCountY() - index2d.y() - 1;

    if ( index2d.x() < index2d.y() )
        return index2d.x();

    return index2d.y();
}

//--------------------------------------------------------------------------------------------

void fix_corners( cartman_mpd_t& mesh )
{
    // ZZ> This function corrects corners across entire mesh

    // weld the corners in a checkerboard pattern
    for (size_t y = 0; y < mesh.info.getTileCountY(); y += 2 )
    {
        for ( size_t x = y & 1; x < mesh.info.getTileCountX(); x += 2 )
        {
            weld_corner_verts(mesh, Index2D(x, y));
        }
    }
}

void fix_edges(cartman_mpd_t& mesh) {
    // ZZ> This function seals the tile edges across the entire mesh

    // weld the edges of all tiles
    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            fix_vertices(mesh, *it);
    }
}

void fix_mesh( cartman_mpd_t& mesh )
{
    // ZZ> This function corrects corners across entire mesh
    fix_corners( mesh );
    fix_edges( mesh );
}

void fix_vertices( cartman_mpd_t& mesh, Index2D index2d )
{
    cartman_mpd_tile_t *pfan = mesh.get_pfan(index2d);
    if ( NULL == pfan ) return;

	tile_definition_t *pdef = tile_dict.get(pfan->type);
    if ( NULL == pdef ) return;

    for ( int cnt = 4; cnt < pdef->numvertices; cnt++ )
    {
        weld_edge_verts( mesh, pfan, pdef, cnt, index2d );
    }
}

//--------------------------------------------------------------------------------------------
void weld_corner_verts(cartman_mpd_t& mesh, Index2D index2d)
{
    int fan = mesh.get_ifan(index2d);
    if (!VALID_MPD_TILE_RANGE(fan))
    {
        return;
    }
    weld_TL(mesh, index2d);
    weld_TR(mesh, index2d);
    weld_BL(mesh, index2d);
    weld_BR(mesh, index2d);
}

//--------------------------------------------------------------------------------------------
void weld_TL( cartman_mpd_t& mesh, Index2D index2d )
{
	select_lst_t loc_lst;
    loc_lst.init( &mesh );

    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y()}, CORNER_TL));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() - 1, index2d.y()}, CORNER_TR));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() - 1, index2d.y() - 1}, CORNER_BR));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y() - 1}, CORNER_BL));

    mesh_select_weld( loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_TR( cartman_mpd_t& mesh, Index2D index2d )
{
	select_lst_t loc_lst;
    loc_lst.init( &mesh );

    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y()}, CORNER_TR));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y() - 1}, CORNER_BR));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() + 1, index2d.y() - 1}, CORNER_BL));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() + 1, index2d.y()}, CORNER_TL));

    mesh_select_weld( loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_BR( cartman_mpd_t& mesh, Index2D index2d )
{
    select_lst_t loc_lst;
    loc_lst.init( &mesh );

    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y()}, CORNER_BR));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() + 1, index2d.y()}, CORNER_BL));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() + 1, index2d.y() + 1}, CORNER_TL));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y() + 1}, CORNER_TR));

    mesh_select_weld( loc_lst );
}

//--------------------------------------------------------------------------------------------
void weld_BL( cartman_mpd_t& mesh, Index2D index2d )
{
	select_lst_t loc_lst;
    loc_lst.init( &mesh );

    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y()}, CORNER_BL));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x(), index2d.y() + 1}, CORNER_TL));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() - 1, index2d.y() + 1}, CORNER_TR));
    loc_lst.add(loc_lst.get_mesh()->get_ivrt_xy({index2d.x() - 1, index2d.y()}, CORNER_BR));

    mesh_select_weld( loc_lst );
}

//--------------------------------------------------------------------------------------------
int get_fan_vertex_by_coord( const cartman_mpd_t * pmesh, const cartman_mpd_tile_t * pfan, int grid_ix, int grid_iy, int ext_verts[] )
{
    int cnt, ivrt, idx, gx, gy;
    int loc_verts[16];
    const Cartman::mpd_vertex_t * pvrt;

    // catch bad parameters
    if ( NULL == pmesh || NULL == pfan ) return -1;
    if ( grid_ix < 0 || grid_ix >= 4 ) return -1;
    if ( grid_iy < 0 || grid_iy >= 4 ) return -1;

    // get the tile definition
	tile_definition_t *pdef = tile_dict.get(pfan->type);
    if ( NULL == pdef ) return -1;

    // handle optional parameters
    int *vert_lst = ( NULL != ext_verts ) ? ext_verts : loc_verts;

    // blank out the vert_lst[] array
    for ( cnt = 0; cnt < 16; cnt++ )
    {
        vert_lst[cnt] = -1;
    }

    // store the vertices in the vrt_lst[]
    for ( cnt = 0, ivrt = pfan->vrtstart; cnt < pdef->numvertices && CHAINEND != ivrt; cnt++, ivrt = pmesh->vrt2[ivrt].next )
    {
        // find the array index for this vertex
        gx = pdef->vertices[cnt].grid_ix;
        gy = pdef->vertices[cnt].grid_iy;
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
bool interpolate_coord( cartman_mpd_t * pmesh, cartman_mpd_tile_t * pfan, int grid_ix, int grid_iy, Vector3f& vec, int ext_verts[] )
{
    // set the coordinates of the given vertex to the interpolated position along the edge of the fan

    int loc_verts[16];
    int * vert_lst = NULL;

    bool retval, is_edge_x, is_edge_y;
    int cnt, ivrt, idx;

    float   vweight = 0.0f;
	Vector3f vsum    = id::zero<Vector3f>();

    Cartman::mpd_vertex_t * pvrt;

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

            return true;
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
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "something is wrong with the vertices", Log::EndOfEntry);
        return false;
    }

    // assume the worst
    retval = false;

    if ( is_edge_x )
    {
        Cartman::mpd_vertex_t * pvrt_min = NULL;
        Cartman::mpd_vertex_t * pvrt_max = NULL;

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
            retval = false;
        }
        else
        {
            float fmax = ( float )( grid_iy - grid_min ) / ( float )( grid_max - grid_min );
            float fmin = 1.0f - fmax;

            vec[kX] = fmax * pvrt_max->x + fmin * pvrt_min->x;
            vec[kY] = fmax * pvrt_max->y + fmin * pvrt_min->y;
            vec[kZ] = fmax * pvrt_max->z + fmin * pvrt_min->z;

            retval = true;
        }
    }
    else if ( is_edge_y )
    {
        Cartman::mpd_vertex_t * pvrt_min = NULL;
        Cartman::mpd_vertex_t * pvrt_max = NULL;

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
            retval = false;
        }
        else
        {
            float fmax = ( float )( grid_ix - grid_min ) / ( float )( grid_max - grid_min );
            float fmin = 1.0f - fmax;

            vec[kX] = fmax * pvrt_max->x + fmin * pvrt_min->x;
            vec[kY] = fmax * pvrt_max->y + fmin * pvrt_min->y;
            vec[kZ] = fmax * pvrt_max->z + fmin * pvrt_min->z;

            retval = true;
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
                float weight = expf( - id::sq( gx - grid_ix ) - id::sq( gy - grid_iy ) );
                vsum += Vector3f(pvrt->x, pvrt->y, pvrt->z) * weight;
                vweight += weight;
            }
        }

        if ( vweight <= 0.0f )
        {
            retval = false;
        }
        else
        {
            vec = vsum * (1.0f / vweight);
            retval = true;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int select_lst_add_fan_vertex( select_lst_t& plst, Index2D index2d, int grid_ix, int grid_iy, int fake_vert )
{
    int vert_lst[16];

	if (!plst.get_mesh()) {
		return -1;
	}
    cartman_mpd_tile_t *pfan = plst.get_mesh()->get_pfan(index2d);
    if (!pfan) return -1;

    int ivrt = get_fan_vertex_by_coord(plst.get_mesh(), pfan, grid_ix, grid_iy, vert_lst );
    Cartman::mpd_vertex_t *pvrt = nullptr;
    if ( ivrt < 0 )
    {
        ivrt = fake_vert;

        pvrt = CART_MPD_VERTEX_PTR(plst.get_mesh(), ivrt);
        if ( NULL != pvrt )
        {
			Vector3f vtmp;
            if ( interpolate_coord(plst.get_mesh(), pfan, grid_ix, grid_iy, vtmp, vert_lst ) )
            {
                pvrt->x = vtmp[kX];
                pvrt->y = vtmp[kY];
                pvrt->z = vtmp[kZ];
            }
        }
    }

    // make sure that the vertex exists
    if ( NULL == pvrt )
    {
        pvrt = CART_MPD_VERTEX_PTR( plst.get_mesh(), ivrt );
        if ( NULL == pvrt )
        {
            ivrt = -1;
        }
    }

    // select_lst_add() only adds valid verts, so don't worry about a bad value in ivrt
    plst.add( ivrt );

    return ivrt;
}

//--------------------------------------------------------------------------------------------
void weld_edge_verts( cartman_mpd_t& mesh, cartman_mpd_tile_t * pfan, tile_definition_t * pdef, int cnt, Index2D index2d )
{
    int fake_edge_count = 0;
    int fake_edge_verts[8 + 1];
    if (NULL == pfan || NULL == pdef ) return;

    // allocate some fake edge verts just in case
    int allocate_rv = cartman_mpd_allocate_vertex_list( &mesh, fake_edge_verts, SDL_arraysize( fake_edge_verts ), 8 );
    if ( allocate_rv < 0 ) return;

    // alias for the grid location
    int grid_ix = pdef->vertices[cnt].grid_ix;
    int grid_iy = pdef->vertices[cnt].grid_iy;

    // is this an edge?
    bool is_edge_x = ( 0 == grid_ix ) || ( 3 == grid_ix );
    bool is_edge_y = ( 0 == grid_iy ) || ( 3 == grid_iy );

    if ( is_edge_x || is_edge_y )
    {
        int added_vert;
		select_lst_t loc_lst;

        loc_lst.init( &mesh );

        // add the point on this fan
        pfan = mesh.get_pfan(index2d);
        if ( NULL != pfan )
        {
            int ivrt;
            Cartman::mpd_vertex_t *pvrt = cartman_mpd_t::get_pvrt_idx(&mesh, pfan, cnt, &ivrt);
            if ( NULL != pvrt )
            {
                loc_lst.add( ivrt );
            }
        }

        if ( 0 == grid_ix )
        {
            added_vert = select_lst_add_fan_vertex(loc_lst, {index2d.x() - 1, index2d.y()}, 3 - grid_ix, grid_iy, fake_edge_verts[fake_edge_count]);

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        if ( 3 == grid_ix )
        {
            added_vert = select_lst_add_fan_vertex(loc_lst, {index2d.x() + 1, index2d.y()}, 3 - grid_ix, grid_iy, fake_edge_verts[fake_edge_count]);

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        if ( 0 == grid_iy )
        {
            added_vert = select_lst_add_fan_vertex(loc_lst, {index2d.x(), index2d.y() - 1}, grid_ix, 3 - grid_iy, fake_edge_verts[fake_edge_count]);

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        if ( 3 == grid_iy )
        {
            added_vert = select_lst_add_fan_vertex(loc_lst, {index2d.x(), index2d.y() + 1}, grid_ix, 3 - grid_iy, fake_edge_verts[fake_edge_count]);

            // did the function use the "fake" vertex?
            if ( added_vert == fake_edge_verts[fake_edge_count] )
            {
                fake_edge_count++;
            }
        }

        mesh_select_weld( loc_lst );
    }

    // free all allocated vertices
    cartman_mpd_free_vertex_list( &mesh, fake_edge_verts, SDL_arraysize( fake_edge_verts ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void select_lst_add_rect( select_lst_t& plst, const Vector3f& a, const Vector3f& b, int mode )
{
    // ZZ> This function checks the rectangular selection

    uint32_t ivrt;

    float xmin, ymin, zmin;
    float xmax, ymax, zmax;

	plst.synch_mesh(&mesh);

	cartman_mpd_t *pmesh = plst.get_mesh();
    if (!pmesh) return;


    // if the selection is empty, we're done
    if ( a == b ) return;

    // make sure that the selection is ordered properly
    std::tie(xmin, xmax) = std::minmax(a.x(), b.x());
    std::tie(ymin, ymax) = std::minmax(a.y(), b.y());
    std::tie(zmin, zmax) = std::minmax(a.z(), b.z());

    if ( mode == WINMODE_VERTEX )
    {
        for ( ivrt = 0; ivrt < MAP_VERTICES_MAX; ivrt++)
        {
            Cartman::mpd_vertex_t& pvrt = pmesh->vrt2[ivrt];
            if (VERTEXUNUSED == pvrt.a) continue;

            
            if (pvrt.x >= xmin && pvrt.x <= xmax &&
                pvrt.y >= ymin && pvrt.y <= ymax &&
                pvrt.z >= zmin && pvrt.z <= zmax)
            {
                plst.add( ivrt );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
        for (ivrt = 0; ivrt < MAP_VERTICES_MAX; ivrt++)
        {
            Cartman::mpd_vertex_t& pvrt = pmesh->vrt2[ivrt];
            if (VERTEXUNUSED == pvrt.a) continue;

            if (pvrt.x >= xmin && pvrt.x <= xmax &&
                pvrt.y >= ymin && pvrt.y <= ymax &&
                pvrt.z >= zmin && pvrt.z <= zmax )
            {
                plst.add( ivrt );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void select_lst_remove_rect( select_lst_t& plst, const Vector3f& a, const Vector3f& b, int mode )
{
    float xmin, ymin, zmin;
    float xmax, ymax, zmax;

	plst.synch_mesh(&mesh);

	cartman_mpd_t *pmesh = plst.get_mesh();
    if (!pmesh) return;

    // If the selection is empty, we're done.
	if (a == b) {
		return;
	}
    // Make sure the coordinates of the selection are properly ordered.
    std::tie(xmin, xmax) = std::minmax(a.x(), b.x());
    std::tie(ymin, ymax) = std::minmax(a.y(), b.y());
    std::tie(zmin, zmax) = std::minmax(a.z(), b.z());

    if ( mode == WINMODE_VERTEX )
    {
		uint32_t ivrt;
        for ( ivrt = 0; ivrt < MAP_VERTICES_MAX; ivrt++)
        {
            Cartman::mpd_vertex_t& pvrt = pmesh->vrt2[ivrt];
            if ( VERTEXUNUSED == pvrt.a ) continue;

            if ( pvrt.x >= xmin && pvrt.x <= xmax &&
                 pvrt.y >= ymin && pvrt.y <= ymax &&
                 pvrt.z >= zmin && pvrt.z <= zmax )
            {
                plst.remove( ivrt );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
		uint32_t ivrt;
        for ( ivrt = 0; ivrt < MAP_VERTICES_MAX; ivrt++)
        {
            Cartman::mpd_vertex_t& pvrt = pmesh->vrt2[ivrt];
            if ( VERTEXUNUSED == pvrt.a ) continue;

            if ( pvrt.x >= xmin && pvrt.x <= xmax &&
                 pvrt.y >= ymin && pvrt.y <= ymax &&
                 pvrt.z >= zmin && pvrt.z <= zmax )
            {
                plst.remove( ivrt );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int nearest_edge_vertex( cartman_mpd_t& mesh, Index2D index2d, float nearx, float neary )
{
    // ZZ> This function gets a vertex number or -1
    int bestvert;
    float grid_fx, grid_fy;
    float prox_x, prox_y, dist_abs, bestprox;

    cartman_mpd_tile_t *pfan = mesh.get_pfan(index2d);
    if ( NULL == pfan ) return -1;

	tile_definition_t *pdef = tile_dict.get(pfan->type);
    if ( NULL == pdef ) return -1;

    // assume the worst
    bestvert = -1;

    int num = pdef->numvertices;
    if ( num > 4 )
    {
        uint32_t ivrt = pfan->vrtstart;

        // skip over the 4 corner vertices
        for ( int cnt = 0; cnt < 4 && CHAINEND != ivrt; cnt++ )
        {
            ivrt = mesh.vrt2[ivrt].next;
        }

        bestprox = 9000;
        for ( int cnt = 4; cnt < num && CHAINEND != ivrt; cnt++ )
        {
            // where is this point in the "grid"?
            grid_fx = GRID_TO_POS( pdef->vertices[cnt].grid_ix );
            grid_fy = GRID_TO_POS( pdef->vertices[cnt].grid_iy );

            prox_x = grid_fx - nearx;
            prox_y = grid_fy - neary;
			dist_abs = std::abs(prox_x) + std::abs(prox_y);

            if ( dist_abs < bestprox )
            {
                bestvert = ivrt;
                bestprox = dist_abs;
            }

            ivrt = mesh.vrt2[ivrt].next;
            if ( CHAINEND == ivrt ) break;
        }
    }

    return bestvert;
}

//--------------------------------------------------------------------------------------------
void mesh_select_move( select_lst_t& plst, float x, float y, float z )
{
    // get the proper mesh
	cartman_mpd_t *pmesh = plst.get_mesh();
    if (nullptr == pmesh) {
        throw id::runtime_error(__FILE__, __LINE__, "selection list has no mesh");
    }

    // limit the movement by the bounds of the mesh
    for ( int cnt = 0; cnt < plst.count(); cnt++ )
    {
        int ivrt = select_lst_t::at(plst, cnt);
        float newx = pmesh->vrt2[ivrt].x + x;
        float newy = pmesh->vrt2[ivrt].y + y;
        float newz = pmesh->vrt2[ivrt].z + z;

        if (newx < 0)  x = 0 - pmesh->vrt2[ivrt].x;
        if (newx > pmesh->info.getEdgeX()) x = pmesh->info.getEdgeX() - pmesh->vrt2[ivrt].x;
        if (newy < 0)  y = 0 - pmesh->vrt2[ivrt].y;
        if (newy > pmesh->info.getEdgeY()) y = pmesh->info.getEdgeY() - pmesh->vrt2[ivrt].y;
        if (newz < 0)  z = 0 - pmesh->vrt2[ivrt].z;
        if (newz > pmesh->info.getEdgeZ()) z = pmesh->info.getEdgeZ() - pmesh->vrt2[ivrt].z;
    }

    // do the movement
    for ( int cnt = 0; cnt < plst.count(); cnt++ )
    {
        int ivrt = select_lst_t::at(plst, cnt);

        float newx = pmesh->vrt2[ivrt].x + x;
        float newy = pmesh->vrt2[ivrt].y + y;
        float newz = pmesh->vrt2[ivrt].z + z;

		newx = Ego::Math::constrain(newx, 0.0f, pmesh->info.getEdgeX());
		newy = Ego::Math::constrain(newy, 0.0f, pmesh->info.getEdgeY());
		newz = Ego::Math::constrain(newz, 0.0f, pmesh->info.getEdgeZ());

        pmesh->vrt2[ivrt].x = newx;
        pmesh->vrt2[ivrt].y = newy;
        pmesh->vrt2[ivrt].z = newz;
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_set_z_no_bound( select_lst_t& plst, float z )
{
    // get the mesh
	cartman_mpd_t *pmesh = plst.get_mesh();
    if (nullptr == pmesh) {
        throw id::runtime_error(__FILE__, __LINE__, "selection list has no mesh");
    }

    for ( int cnt = 0; cnt < plst.count(); cnt++ )
    {
        uint32_t vert = select_lst_t::at(plst, cnt);
        if ( vert > pmesh->info.getVertexCount() ) continue;

        pmesh->vrt2[vert].z = z;
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_jitter(select_lst_t& plst) {
	cartman_mpd_t *pmesh = plst.get_mesh();
    if (nullptr == pmesh)         {
        throw id::runtime_error(__FILE__, __LINE__, "selection list has no mesh");
    }
	for (int i = 0; i < plst.count(); ++i) {
		int vertex = select_lst_t::at(plst, i);
        MeshEditor::move_vert(*pmesh,  vertex, Random::next(2) - 1, Random::next(2) - 1, 0);
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_verts_connected(select_lst_t& plst) {
	cartman_mpd_t *pmesh = plst.get_mesh();
    if (nullptr == pmesh) {
        throw id::runtime_error(__FILE__, __LINE__, "selection list has no mesh");
    }

    // get vertex list alias
	cartman_mpd_info_t& pinfo = pmesh->info;

    for (auto it = pinfo.begin(); it != pinfo.end(); ++it) {
            cartman_mpd_tile_t *pfan = pmesh->get_pfan(*it);
            if ( NULL == pfan ) continue;

			tile_definition_t *pdef = tile_dict.get(pfan->type);
            if ( NULL == pdef ) continue;

			bool select_vertsfan = false;

			uint32_t vert;
			int cnt;
            for ( cnt = 0, vert = pfan->vrtstart;
                  cnt < pdef->numvertices;
                  cnt++, vert = pmesh->vrt2[vert].next)
            {
                for ( int tnc = 0; tnc < plst.count(); tnc++ )
                {
                    if ( select_lst_t::at(plst,tnc) == vert )
                    {
                        select_vertsfan = true;
                        break;
                    }
                }
            }

            if ( select_vertsfan )
            {
                for ( int cnt = 0, vert = pfan->vrtstart;
                      cnt < pdef->numvertices;
                      cnt++, vert = pmesh->vrt2[vert].next)
                {
                    plst.add( vert );
                }
            }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_select_weld(select_lst_t& plst)
{
	cartman_mpd_t *pmesh = plst.get_mesh();
    if (nullptr == pmesh) {
        throw id::runtime_error(__FILE__, __LINE__, "selection list has no mesh");
    }

    if ( plst.count() > 1 )
    {
		float sum_x, sum_y, sum_z, sum_a;
        sum_x = 0.0f;
        sum_y = 0.0f;
        sum_z = 0.0f;
        sum_a = 0.0f;

        for ( int cnt = 0; cnt < plst.count(); cnt++ )
        {
            uint32_t vert = select_lst_t::at(plst,cnt);
            if ( CHAINEND == vert ) break;

            sum_x += pmesh->vrt2[vert].x;
            sum_y += pmesh->vrt2[vert].y;
            sum_z += pmesh->vrt2[vert].z;
            sum_a += pmesh->vrt2[vert].a;
        }

		float avg_x, avg_y, avg_z, avg_a;
        if ( plst.count() > 1 )
        {
            avg_x = sum_x / plst.count();
            avg_y = sum_y / plst.count();
            avg_z = sum_z / plst.count();
            avg_a = sum_a / plst.count();
        }
        else
        {
            avg_x = sum_x;
            avg_y = sum_y;
            avg_z = sum_z;
            avg_a = sum_a;
        }

        for ( int cnt = 0; cnt < plst.count(); cnt++ )
        {
            int vertex = select_lst_t::at(plst, cnt);
            if (CHAINEND == vertex) break;

            pmesh->vrt2[vertex].x = avg_x;
            pmesh->vrt2[vertex].y = avg_y;
            pmesh->vrt2[vertex].z = avg_z;
            pmesh->vrt2[vertex].a = Ego::Math::constrain(avg_a, 1.0f, 255.0f);
        }
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::mesh_set_tile( cartman_mpd_t& mesh, uint16_t tiletoset, uint8_t upper, uint16_t presser, uint8_t tx )
{
    // ZZ> This function sets one tile type to another
    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
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
                        tx_bits |= ( tx & 0xFE ) + Random::next(1);
                        break;

                    case 2:
                        tx_bits |= ( tx & 0xFC ) + Random::next(3);
                        break;

                    case 3:
                        tx_bits |= ( tx & 0xF8 ) + Random::next(7);
                        break;

                    default:
                        tx_bits = pfan->tx_bits;
                }
                pfan->tx_bits = tx_bits;
            }
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::move_mesh_z( cartman_mpd_t& mesh, int z, uint16_t tiletype, uint16_t tileand )
{
    tiletype = tiletype & tileand;

    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

			tile_definition_t *pdef = tile_dict.get(pfan->type);
            if ( NULL == pdef ) continue;

            if ( tiletype == ( pfan->tx_bits & tileand ) )
            {
                int vert = pfan->vrtstart;
                for ( int cnt = 0; cnt < pdef->numvertices; cnt++ )
                {
                    float newz = mesh.vrt2[vert].z + z;
					newz = Ego::Math::constrain(newz, 0.0f, mesh.info.getEdgeZ());
                    mesh.vrt2[vert].z = newz;

                    vert = mesh.vrt2[vert].next;
                }
            }
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::move_vert( cartman_mpd_t& mesh, int vert, float x, float y, float z )
{
    int newx = mesh.vrt2[vert].x + x;
    int newy = mesh.vrt2[vert].y + y;
    int newz = mesh.vrt2[vert].z + z;

	newx = Ego::Math::constrain(newx, 0, (int)mesh.info.getEdgeX());
	newy = Ego::Math::constrain(newy, 0, (int)mesh.info.getEdgeY());
	newz = Ego::Math::constrain(newz, -(int)mesh.info.getEdgeZ(), +(int)mesh.info.getEdgeZ());

    mesh.vrt2[vert].x = newx;
    mesh.vrt2[vert].y = newy;
    mesh.vrt2[vert].z = newz;
}

//--------------------------------------------------------------------------------------------
void MeshEditor::raise_mesh( cartman_mpd_t& mesh, uint32_t point_lst[], size_t point_cnt, float x, float y, int amount, int size )
{
    if ( NULL == point_lst || 0 == point_cnt ) return;

    for ( size_t cnt = 0; cnt < point_cnt; cnt++ )
    {
        uint32_t vert = point_lst[cnt];
        if ( !CART_VALID_VERTEX_RANGE( vert ) ) break;

        float disx = mesh.vrt2[vert].x - x;
        float disy = mesh.vrt2[vert].y - y;
        float dis  = std::sqrt( disx * disx + disy * disy );

        float newamount = std::abs( amount ) - ( 2.0f * dis ) / ((float)size);
        if ( newamount < 0 ) newamount = 0;
        if ( amount < 0 )    newamount = -newamount;

        move_vert( mesh,  vert, 0, 0, newamount );
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::level_vrtz( cartman_mpd_t& mesh )
{
    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

			tile_definition_t *pdef = tile_dict.get(pfan->type);
            if ( NULL == pdef ) continue;

            uint32_t vert = pfan->vrtstart;
            for ( int cnt = 0; cnt < pdef->numvertices; cnt++ )
            {
                mesh.vrt2[vert].z = 0;
                vert = mesh.vrt2[vert].next;
            }
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::jitter_mesh( cartman_mpd_t& mesh )
{
	select_lst_t loc_lst;

    // initialize the local selection
    loc_lst.init( &mesh );

    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

			tile_definition_t *pdef = tile_dict.get(pfan->type);
            if ( NULL == pdef ) continue;

            int num = pdef->numvertices;

            // clear the selection
            loc_lst.clear();

            // add all the tile vertices
			int cnt;
			uint32_t vert;
            for ( cnt = 0, vert = pfan->vrtstart;
                  cnt < num;
                  cnt++, vert = mesh.vrt2[vert].next)
            {
                loc_lst.add( vert );
            }

            // jitter the tile verts
            mesh_select_move( loc_lst, Random::next(-3, 4), Random::next(-3, 4), Random::next(-32, 31) );
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::flatten_mesh( cartman_mpd_t& mesh, int y0 )
{
    int height = ( 780 - ( y0 ) ) * 4;
	height = int(Ego::Math::constrain(float(height), 0.0f, mesh.info.getEdgeZ()));

    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

			tile_definition_t *pdef = tile_dict.get(pfan->type);
            if ( NULL == pdef ) continue;

            int num = pdef->numvertices;

            uint32_t vert = pfan->vrtstart;
            for ( int cnt = 0; cnt < num; cnt++ )
            {
                float ftmp = mesh.vrt2[vert].z - height;

				if (std::abs(ftmp) < 50)
                {
                    mesh.vrt2[vert].z = height;
                }

                vert = mesh.vrt2[vert].next;
            }
    }

}

//--------------------------------------------------------------------------------------------
void MeshEditor::clear_mesh( cartman_mpd_t& mesh, uint8_t upper, uint16_t presser, uint8_t tx, uint8_t type )
{
    int loc_type = type;

    if ( !TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {
        for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it)
        {
                cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
                if ( NULL == pfan ) continue;

                mesh.remove_pfan(pfan);

                int tx_bits = TILE_SET_UPPER_BITS( upper );
                switch ( presser )
                {
                    case 0:
                        tx_bits |= tx & 0xFF;
                        break;
                    case 1:
                        tx_bits |= ( tx & 0xFE ) | Random::next(1);
                        break;
                    case 2:
                        tx_bits |= ( tx & 0xFC ) | Random::next(3);
                        break;
                    case 3:
                        tx_bits |= ( tx & 0xF8 ) | Random::next(7);
                        break;
                    default:
                        tx_bits = pfan->tx_bits;
                        break;
                }
                pfan->tx_bits = tx_bits;

                loc_type = type;
                if ( loc_type <= 1 ) loc_type = Random::next(1);
                if ( loc_type == 32 || loc_type == 33 ) loc_type = 32 + Random::next(1);
                pfan->type = loc_type;

                mesh.add_pfan(pfan, (*it).x() * Info<int>::Grid::Size(), (*it).y() * Info<int>::Grid::Size());
        }
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::three_e_mesh( cartman_mpd_t& mesh, uint8_t upper, uint8_t tx )
{
    // ZZ> Replace all 3F tiles with 3E tiles...
    if ( TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {
        return;
    }

    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

            if ( 0x3F == pfan->tx_bits )
            {
                pfan->tx_bits = 0x3E;
            }
    }
}

//--------------------------------------------------------------------------------------------
bool MeshEditor::fan_isPassableFloor( cartman_mpd_t& mesh, const Index2D& index2d )
{
	cartman_mpd_tile_t *ptile = mesh.get_pfan(index2d);
    if (!ptile) return false;
	return ptile->isPassableFloor();
}

bool MeshEditor::isImpassableWall( cartman_mpd_t& mesh, const Index2D& index2d )
{
	cartman_mpd_tile_t *ptile = mesh.get_pfan(index2d);
    if (!ptile) return true;
	return ptile->isImpassableWall();
}

//--------------------------------------------------------------------------------------------
#define BARRIER_FUNC(XX) ( 1.0f - (1.0f - (XX)) * (1.0f - (XX)) * (1.0f - (XX)) )

void MeshEditor::set_barrier_height( cartman_mpd_t& mesh, const Index2D& index2d )
{
    float corner_hgt[4];

    // does the fan exist?
	cartman_mpd_tile_t *pfan = mesh.get_pfan(index2d);
    if ( NULL == pfan ) return;

    // fan is defined?
	tile_definition_t *pdef = tile_dict.get(pfan->type);
    if ( NULL == pdef || 0 == pdef->numvertices ) return;
    uint32_t vert_count = pdef->numvertices;

    // must not be a passable floor
    if ( fan_isPassableFloor( mesh,  index2d ) ) return;

    // find other walls around this tile
    bool floor_mx = fan_isPassableFloor(mesh, {index2d.x() - 1, index2d.y()});
    bool floor_px = fan_isPassableFloor(mesh, {index2d.x() + 1, index2d.y()});
    bool floor_my = fan_isPassableFloor(mesh, {index2d.x(), index2d.y() - 1});
    bool floor_py = fan_isPassableFloor(mesh, {index2d.x(), index2d.y() + 1});
    bool noedges  = !floor_mx && !floor_px && !floor_my && !floor_py;

    bool floor_mxmy = fan_isPassableFloor(mesh, {index2d.x() - 1, index2d.y() - 1});
    bool floor_mxpy = fan_isPassableFloor(mesh, {index2d.x() - 1, index2d.y() + 1});
    bool floor_pxmy = fan_isPassableFloor(mesh, {index2d.x() + 1, index2d.y() - 1});
    bool floor_pxpy = fan_isPassableFloor(mesh, {index2d.x() + 1, index2d.y() + 1});
    bool nocorners  = !floor_mxmy && !floor_mxpy && !floor_pxmy && !floor_pxpy;

    // if it is completely surrounded by walls, there's nothing to do
    if ( noedges && nocorners ) return;

    // initialize the min/max values
    uint32_t vert = pfan->vrtstart;
    corner_hgt[0] = mesh.vrt2[vert].z;
    float min_hgt = corner_hgt[0];
    float max_hgt = corner_hgt[0];
    vert = mesh.vrt2[vert].next;

    // iterate through the corners
    for (uint32_t cnt = 1; cnt < 4 && CHAINEND != vert; vert = mesh.vrt2[vert].next, cnt++)
    {
        corner_hgt[cnt] = mesh.vrt2[vert].z;
		min_hgt = std::min(min_hgt, corner_hgt[cnt]);
        max_hgt = std::max( max_hgt, corner_hgt[cnt] );
    }

    // correct all vertices
    for (uint32_t cnt = 0, vert = pfan->vrtstart; cnt < vert_count && CHAINEND != vert; cnt++, vert = mesh.vrt2[vert].next)
    {
        float ftmp, weight;

        float vsum = 0.0f;
        float wsum = 0.0f;

        if ( !noedges )
        {
            if ( !floor_mx )
            {
                weight = ( float )( pdef->vertices[cnt].grid_iy ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_BL] + ( 1.0f - weight ) * corner_hgt[CORNER_TL];
                ftmp  -= min_hgt;

                weight = ( float )( 3 - pdef->vertices[cnt].grid_ix ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_px )
            {
                weight = ( float )( pdef->vertices[cnt].grid_iy ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_BR] + ( 1.0f - weight ) * corner_hgt[CORNER_TR];
                ftmp  -= min_hgt;

                weight = ( float )( pdef->vertices[cnt].grid_ix ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_my )
            {
                weight = ( float )( 3 - pdef->vertices[cnt].grid_ix ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_TL] + ( 1.0f - weight ) * corner_hgt[CORNER_TR];
                ftmp  -= min_hgt;

                weight = ( float )( 3 - pdef->vertices[cnt].grid_iy ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_py )
            {
                weight = ( float )( pdef->vertices[cnt].grid_ix ) / 3.0f;
                ftmp   = weight * corner_hgt[CORNER_BR] + ( 1.0f - weight ) * corner_hgt[CORNER_BL];
                ftmp  -= min_hgt;

                weight = ( float )( pdef->vertices[cnt].grid_iy ) / 3.0f;
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

                weight = ( float )std::max( pdef->vertices[cnt].grid_ix, pdef->vertices[cnt].grid_iy ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_pxmy )
            {
                ftmp = corner_hgt[CORNER_TR] - min_hgt;

                weight = ( float )std::min( pdef->vertices[cnt].grid_ix, 3 - pdef->vertices[cnt].grid_iy ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_mxpy )
            {
                ftmp = corner_hgt[CORNER_BL] - min_hgt;

                weight = ( float )std::min( 3 - pdef->vertices[cnt].grid_ix, pdef->vertices[cnt].grid_iy ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }

            if ( !floor_mxmy )
            {
                ftmp = corner_hgt[CORNER_TL] - min_hgt;

                weight = ( float )std::min( 3 - pdef->vertices[cnt].grid_ix, 3 - pdef->vertices[cnt].grid_iy ) / 3.0f;
                ftmp *= BARRIER_FUNC( weight );

                vsum += weight * ftmp;
                wsum += weight;
            }
        }

        if ( 0.0f == wsum )
        {
            vsum = mesh.vrt2[vert].z - min_hgt;
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
        mesh.vrt2[vert].z = ftmp + min_hgt;
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::fix_walls( cartman_mpd_t& mesh )
{
    // make sure the corners are correct
    fix_corners( mesh );

    // adjust the wall-icity of all non-corner vertices
    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            set_barrier_height(mesh, *it);
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::impass_edges( cartman_mpd_t& mesh, int amount )
{
    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            if (dist_from_edge(mesh, *it) < amount)
            {
                cartman_mpd_tile_t *tile = mesh.get_pfan(*it);
                if (!tile) continue;
				tile->setImpassable();
            }
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::mesh_replace_fx( cartman_mpd_t& mesh, uint16_t fx_bits, uint16_t fx_mask, uint8_t fx_new )
{
    // ZZ> This function sets the fx for a group of tiles

    // trim away any bits that can't be matched
    fx_bits &= fx_mask;

    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

            if ( fx_bits == ( pfan->tx_bits&fx_mask ) )
            {
                pfan->fx = fx_new;
            }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
uint8_t tile_is_different( cartman_mpd_t& mesh, Index2D index2d, uint16_t fx_bits, uint16_t fx_mask )
{
    // ZZ> false if of same set, true if different
    cartman_mpd_tile_t *pfan = mesh.get_pfan(index2d);
    if ( NULL == pfan ) return false;

    if ( fx_mask == 0xC0 )
    {
        if ( pfan->tx_bits >= ( MAPFX_WALL | MAPFX_IMPASS ) )
        {
            return false;
        }
    }

    if ( fx_bits == ( pfan->tx_bits&fx_mask ) )
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------
uint16_t trim_code( cartman_mpd_t& mesh, const Index2D& index2d, uint16_t fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    uint16_t code;

    if (tile_is_different(mesh, {index2d.x(), index2d.y() - 1}, fx_bits, 0xF0))
    {
        // Top
        code = 0;
        if (tile_is_different(mesh, {index2d.x() - 1, index2d.y()}, fx_bits, 0xF0))
        {
            // Left
            code = 8;
        }
        if (tile_is_different(mesh, {index2d.x() + 1, index2d.y()}, fx_bits, 0xF0))
        {
            // Right
            code = 9;
        }
        return code;
    }

    if (tile_is_different(mesh, {index2d.x(), index2d.y() + 1}, fx_bits, 0xF0))
    {
        // Bottom
        code = 1;
        if (tile_is_different(mesh, {index2d.x() - 1, index2d.y()}, fx_bits, 0xF0))
        {
            // Left
            code = 10;
        }
        if (tile_is_different(mesh, {index2d.x() + 1, index2d.y()}, fx_bits, 0xF0))
        {
            // Right
            code = 11;
        }
        return code;
    }

    if (tile_is_different(mesh, {index2d.x() - 1, index2d.y()}, fx_bits, 0xF0))
    {
        // Left
        code = 2;
        return code;
    }
    if (tile_is_different(mesh, {index2d.x() + 1, index2d.y()}, fx_bits, 0xF0))
    {
        // Right
        code = 3;
        return code;
    }

    if (tile_is_different(mesh, {index2d.x() + 1, index2d.y() + 1}, fx_bits, 0xF0))
    {
        // Bottom Right
        code = 4;
        return code;
    }
    if (tile_is_different(mesh, {index2d.x() - 1, index2d.y() + 1}, fx_bits, 0xF0))
    {
        // Bottom Left
        code = 5;
        return code;
    }
    if (tile_is_different(mesh, {index2d.x() + 1, index2d.y() - 1}, fx_bits, 0xF0))
    {
        // Top Right
        code = 6;
        return code;
    }
    if (tile_is_different(mesh, {index2d.x() - 1, index2d.y() - 1}, fx_bits, 0xF0))
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
uint16_t wall_code( cartman_mpd_t& mesh, const Index2D& index2d, uint16_t fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    uint16_t code;

    if (tile_is_different(mesh, {index2d.x(), index2d.y() - 1}, fx_bits, 0xC0))
    {
        // Top
        code = Random::next(1) * 2 + 20;
        if (tile_is_different(mesh, {index2d.x() - 1, index2d.y()}, fx_bits, 0xC0))
        {
            // Left
            code = 48;
        }
        if (tile_is_different(mesh, {index2d.x() + 1, index2d.y()}, fx_bits, 0xC0))
        {
            // Right
            code = 50;
        }

        return code;
    }

    if (tile_is_different(mesh, {index2d.x(), index2d.y() + 1}, fx_bits, 0xC0))
    {
        // Bottom
        code = Random::next(1) * 2;
        if (tile_is_different(mesh, {index2d.x() - 1, index2d.y()}, fx_bits, 0xC0))
        {
            // Left
            code = 52;
        }
        if (tile_is_different(mesh, {index2d.x() + 1, index2d.y()}, fx_bits, 0xC0))
        {
            // Right
            code = 54;
        }
        return code;
    }

    if (tile_is_different(mesh, {index2d.x() - 1, index2d.y()}, fx_bits, 0xC0))
    {
        // Left
        code = Random::next(1) * 2 + 16;
        return code;
    }

    if (tile_is_different(mesh, {index2d.x() + 1, index2d.y()}, fx_bits, 0xC0))
    {
        // Right
        code = Random::next(1) * 2 + 4;
        return code;
    }

    if (tile_is_different(mesh, {index2d.x() + 1, index2d.y() + 1}, fx_bits, 0xC0))
    {
        // Bottom Right
        code = 32;
        return code;
    }
    if (tile_is_different(mesh, {index2d.x() - 1, index2d.y() + 1}, fx_bits, 0xC0))
    {
        // Bottom Left
        code = 34;
        return code;
    }
    if (tile_is_different(mesh, {index2d.x() + 1, index2d.y() - 1}, fx_bits, 0xC0))
    {
        // Top Right
        code = 36;
        return code;
    }
    if (tile_is_different(mesh, {index2d.x() - 1, index2d.y() - 1}, fx_bits, 0xC0))
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
void trim_mesh_tile( cartman_mpd_t& mesh, uint16_t fx_bits, uint16_t fx_mask )
{
    // ZZ> This function trims walls and floors and tops automagically
    fx_bits = fx_bits & fx_mask;

    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            cartman_mpd_tile_t *pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

            if ( fx_bits == ( pfan->tx_bits&fx_mask ) )
            {
				int code;
                if ( fx_mask == 0xC0 )
                {
                    code = wall_code(mesh, *it, fx_bits);
                }
                else
                {
                    code = trim_code(mesh, *it, fx_bits);
                }

                if ( code != 255 )
                {
                    pfan->tx_bits = fx_bits + code;
                }
            }
    }
}

//--------------------------------------------------------------------------------------------
void MeshEditor::mesh_replace_tile(cartman_mpd_t& mesh, int _xfan, int _yfan, int _onfan, uint8_t _tx, uint8_t _upper, uint8_t _fx, uint8_t _type, uint16_t _presser, bool tx_only, bool at_floor_level)
{
    cart_vec_t pos[CORNER_COUNT];
    int tx_bits;

	cartman_mpd_tile_t *pfan = CART_MPD_FAN_PTR(&mesh, _onfan);
    if ( NULL == pfan ) return;

    if ( !tx_only )
    {
        if ( !at_floor_level )
        {
            // Save corner positions
            int vert = pfan->vrtstart;
            pos[CORNER_TL][kX] = mesh.vrt2[vert].x;
            pos[CORNER_TL][kY] = mesh.vrt2[vert].y;
            pos[CORNER_TL][kZ] = mesh.vrt2[vert].z;

            vert = mesh.vrt2[vert].next;
            pos[CORNER_TR][kX] = mesh.vrt2[vert].x;
            pos[CORNER_TR][kY] = mesh.vrt2[vert].y;
            pos[CORNER_TR][kZ] = mesh.vrt2[vert].z;

            vert = mesh.vrt2[vert].next;
            pos[CORNER_BR][kX] = mesh.vrt2[vert].x;
            pos[CORNER_BR][kY] = mesh.vrt2[vert].y;
            pos[CORNER_BR][kZ] = mesh.vrt2[vert].z;

            vert = mesh.vrt2[vert].next;
            pos[CORNER_BL][kX] = mesh.vrt2[vert].x;
            pos[CORNER_BL][kY] = mesh.vrt2[vert].y;
            pos[CORNER_BL][kZ] = mesh.vrt2[vert].z;
        }
        mesh.remove_ifan(_onfan);
    }

    // set the texture info
    tx_bits = TILE_SET_UPPER_BITS( _upper );
    switch ( _presser )
    {
        case 0:
            tx_bits |= _tx & 0xFF;
            break;
        case 1:
            tx_bits |= ( _tx & 0xFE ) | Random::next(1);
            break;
        case 2:
            tx_bits |= ( _tx & 0xFC ) | Random::next(3);
            break;
        case 3:
            tx_bits |= ( _tx & 0xF8 ) | Random::next(7);
            break;
        default:
            tx_bits = pfan->tx_bits;
            break;
    };
    pfan->tx_bits = tx_bits;

    if ( !tx_only )
    {
        pfan->type = _type;
        mesh.add_ifan(_onfan, _xfan * Info<int>::Grid::Size(), _yfan * Info<int>::Grid::Size());
        pfan->fx = _fx;

        if ( 0 /*!at_floor_level*/ )
        {
			int vert;
            // Return corner positions
            vert = pfan->vrtstart;
            mesh.vrt2[vert].x = pos[CORNER_TL][kX];
            mesh.vrt2[vert].y = pos[CORNER_TL][kY];
            mesh.vrt2[vert].z = pos[CORNER_TL][kZ];

            vert = mesh.vrt2[vert].next;
            mesh.vrt2[vert].x = pos[CORNER_TR][kX];
            mesh.vrt2[vert].y = pos[CORNER_TR][kY];
            mesh.vrt2[vert].z = pos[CORNER_TR][kZ];

            vert = mesh.vrt2[vert].next;
            mesh.vrt2[vert].x = pos[CORNER_BR][kX];
            mesh.vrt2[vert].y = pos[CORNER_BR][kY];
            mesh.vrt2[vert].z = pos[CORNER_BR][kZ];

            vert = mesh.vrt2[vert].next;
            mesh.vrt2[vert].x = pos[CORNER_BL][kX];
            mesh.vrt2[vert].y = pos[CORNER_BL][kY];
            mesh.vrt2[vert].z = pos[CORNER_BL][kZ];
        }
    }
}

//--------------------------------------------------------------------------------------------

void MeshEditor::setFX( cartman_mpd_t& mesh, int ifan, uint8_t fx )
{
    cartman_mpd_tile_t *tile = mesh.get_pfan(ifan);
    if (!tile) return;
	tile->setFX(fx);
}

//--------------------------------------------------------------------------------------------
void MeshEditor::mesh_move( cartman_mpd_t& mesh, float dx, float dy, float dz )
{
    uint32_t vert;
    int cnt;

    cartman_mpd_tile_t * pfan   = NULL;
    tile_definition_t  * pdef    = NULL;

    for (auto it = mesh.info.begin(); it != mesh.info.end(); ++it) {
            int count;

            pfan = mesh.get_pfan(*it);
            if ( NULL == pfan ) continue;

            pdef = tile_dict.get(pfan->type );
            if ( NULL == pdef ) continue;

            count = pdef->numvertices;

            for ( cnt = 0, vert = pfan->vrtstart;
                  cnt < count && CHAINEND != vert;
                  cnt++, vert = mesh.vrt2[vert].next )
            {
                MeshEditor::move_vert( mesh,  vert, dx, dy, dz );
            }
    }
}
