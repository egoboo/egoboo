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

/// @file map_functions.c
/// @brief mpd functionality ported from cartman and EgoMap
/// @details

#include "map_functions.h"

#include "file_formats/map_file.h"

//--------------------------------------------------------------------------------------------
// Generic functions
//--------------------------------------------------------------------------------------------
bool_t twist_to_normal( Uint8 twist, float v[], float slide )
{
    int ix, iy;
    float dx, dy;
    float nx, ny, nz, nz2;
    float diff_xy;

    if ( NULL == v ) return bfalse;

    diff_xy = 128.0f / slide;

    ix = ( twist >> 0 ) & 0x0f;
    iy = ( twist >> 4 ) & 0x0f;
    ix -= 7;
    iy -= 7;

    dx = -ix / ( float )CARTMAN_FIXNUM * ( float )CARTMAN_SLOPE;
    dy = iy / ( float )CARTMAN_FIXNUM * ( float )CARTMAN_SLOPE;

    // determine the square of the z normal
    nz2 =  diff_xy * diff_xy / ( dx * dx + dy * dy + diff_xy * diff_xy );

    // determine the z normal
    nz = 0.0f;
    if ( nz2 > 0.0f )
    {
        nz = SQRT( nz2 );
    }

    nx = - dx * nz / diff_xy;
    ny = - dy * nz / diff_xy;

    v[0] = nx;
    v[1] = ny;
    v[2] = nz;

    return btrue;
}

//--------------------------------------------------------------------------------------------
map_t * map_generate_tile_twist_data( map_t * pmesh )
{
    /// @author BB
    /// @details generates twist data for all tiles from the bitmap

    size_t   mapx, mapy;
    size_t   tile_x, tile_y, itile;
    int      step_x, step_y;

    tile_info_t * tlst = NULL;

    // does the mesh exist?
    if ( NULL == pmesh ) return pmesh;
    tlst = pmesh->mem.tile_list;

    // are there tiles?
    if ( 0 == pmesh->mem.tile_count || NULL == pmesh->mem.tile_list ) return pmesh;

    step_x = 1;
    step_y = pmesh->info.tiles_y;
    for ( mapy = 0, tile_y = 0; mapy < pmesh->info.tiles_y; mapy++, tile_y += step_y )
    {
        for ( mapx = 0, tile_x = 0; mapx < pmesh->info.tiles_x; mapx++, tile_x += step_x )
        {
            int itile_mx, itile_px, itile_my, itile_py;
            float hgt_mx, hgt_px, hgt_my, hgt_py;

            itile = tile_x + tile_y;

            itile_mx = LAMBDA( mapx <= 0, -1, itile - step_x );
            if ( itile_mx < 0 )
            {
                hgt_mx = TILE_FSIZE;
            }
            else
            {
                hgt_mx = LAMBDA( HAS_SOME_BITS( tlst[itile_mx].fx, MAPFX_WALL | MAPFX_IMPASS ), TILE_FSIZE, 0.0f );
            }

            itile_px = LAMBDA( mapx >= pmesh->info.tiles_x - 1, -1, itile + step_x );
            if ( itile_px < 0 )
            {
                hgt_px = TILE_FSIZE;
            }
            else
            {
                hgt_px = LAMBDA( HAS_SOME_BITS( tlst[itile_px].fx, MAPFX_WALL | MAPFX_IMPASS ), TILE_FSIZE, 0.0f );
            }

            itile_my = LAMBDA( mapy <= 0, -1, itile - step_y );
            if ( itile_my < 0 )
            {
                hgt_my = TILE_FSIZE;
            }
            else
            {
                hgt_my = LAMBDA( HAS_SOME_BITS( tlst[itile_my].fx, MAPFX_WALL | MAPFX_IMPASS ), TILE_FSIZE, 0.0f );
            }

            itile_py = LAMBDA( mapy >= pmesh->info.tiles_y - 1, -1, itile + step_y );
            if ( itile_py < 0 )
            {
                hgt_py = TILE_FSIZE;
            }
            else
            {
                hgt_py = LAMBDA( HAS_SOME_BITS( tlst[itile_py].fx, MAPFX_WALL | MAPFX_IMPASS ), TILE_FSIZE, 0.0f );
            }

            // calculate the twist of this tile
            tlst[itile].twist = cartman_calc_twist(( hgt_px - hgt_mx ) / 8, ( hgt_py - hgt_my ) / 8 );
        }
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * map_generate_fan_type_data( map_t * pmesh )
{
    /// @author BB
    /// @details generates vertex data for all tiles from the bitmap

    size_t   mapx, mapy;
    size_t   tile_x, tile_y;
    int      step_x, step_y;
    bool_t   west_flat, north_flat, south_flat, east_flat;
    Uint32   wall_flags;
    Uint8    type;

    tile_info_t * tlst = NULL;

    // does the mesh exist?
    if ( NULL == pmesh ) return pmesh;
    tlst = pmesh->mem.tile_list;

    // are there tiles?
    if ( 0 == pmesh->mem.tile_count || NULL == pmesh->mem.tile_list ) return pmesh;

    step_x = 1;
    step_y = pmesh->info.tiles_y;
    for ( mapy = 0, tile_y = 0; mapy < pmesh->info.tiles_y; mapy++, tile_y += step_y )
    {
        for ( mapx = 0, tile_x = 0; mapx < pmesh->info.tiles_x; mapx++, tile_x += step_x )
        {
            int   itile, wall_count;
            int cnt, dx, dy;
            float tile_hgt[9];
            float vrt_hgt[16];

            itile = tile_x + tile_y;

            if ( HAS_NO_BITS( tlst[itile].fx, MAPFX_WALL | MAPFX_IMPASS ) )
            {
                // this tile is NOT a wall. Just alternate between type 1 and 2 in a checkerboard
                tlst[itile].type = (( tile_x & 1 ) + ( tile_y & 1 ) ) & 1;
                continue;
            }

            // get the heights of the surrounding tiles
            wall_count = 0;
            for ( cnt = 0, dy = -1; dy < 2; dy++ )
            {
                for ( dx = -1; dx < 2; dx++, cnt++ )
                {
                    if ( mapx + dx < 0 || mapx + dx >= pmesh->info.tiles_x ||
                         mapy + dy < 0 || mapy + dy >= pmesh->info.tiles_y )
                    {
                        itile = -1;
                    }
                    else
                    {
                        itile = itile + dx * step_x + dy * step_y;
                    }

                    if ( itile < 0 )
                    {
                        tile_hgt[cnt] = TILE_FSIZE;
                    }
                    else
                    {
                        tile_hgt[cnt] = LAMBDA( HAS_SOME_BITS( tlst[itile].fx, MAPFX_WALL | MAPFX_IMPASS ), TILE_FSIZE, 0.0f );
                    }

                    if ( tile_hgt[cnt] > 0.0f ) wall_count++;
                }
            }

            if ( 9 == wall_count )
            {
                // this tile is a wall surrounded by walls.
                // Just alternate between type 1 and 2 in a checkerboard, but opposite to the floors
                tlst[itile].type = (( tile_x & 1 ) + ( tile_y & 1 ) + 1 ) & 1;
                continue;
            }

            // set the north edge
            if ( tile_hgt[1] > 0.0f )
            {
                vrt_hgt[1] = vrt_hgt[2] = TILE_FSIZE;
            }

            // set the west edge
            if ( tile_hgt[3] > 0.0f )
            {
                vrt_hgt[4] = vrt_hgt[8] = TILE_FSIZE;
            }

            // set the west edge
            if ( tile_hgt[5] > 0.0f )
            {
                vrt_hgt[7] = vrt_hgt[11] = TILE_FSIZE;
            }

            // set the south edge
            if ( tile_hgt[7] > 0.0f )
            {
                vrt_hgt[13] = vrt_hgt[14] = TILE_FSIZE;
            }

            // set the northwest corner
            {
                float hsum = 0.0f, hwgt = 0.0f;
                if ( tile_hgt[0] > 0.0f )
                {
                    hsum += TILE_FSIZE;
                    hwgt += 1.0f;
                }
                else
                {
                    if ( tile_hgt[1] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                    if ( tile_hgt[3] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                }

                vrt_hgt[0] = hsum / hwgt;
            }

            // set the northeast corner
            {
                float hsum = 0.0f, hwgt = 0.0f;
                if ( tile_hgt[2] > 0.0f )
                {
                    hsum += TILE_FSIZE;
                    hwgt += 1.0f;
                }
                else
                {
                    if ( tile_hgt[1] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                    if ( tile_hgt[5] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                }

                vrt_hgt[3] = hsum / hwgt;
            }

            // set the southwest corner
            {
                float hsum = 0.0f, hwgt = 0.0f;
                if ( tile_hgt[6] > 0.0f )
                {
                    hsum += TILE_FSIZE;
                    hwgt += 1.0f;
                }
                else
                {
                    if ( tile_hgt[3] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                    if ( tile_hgt[7] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                }

                vrt_hgt[12] = hsum / hwgt;
            }

            // set the southwest corner
            {
                float hsum = 0.0f, hwgt = 0.0f;
                if ( tile_hgt[0] > 0.0f )
                {
                    hsum += TILE_FSIZE;
                    hwgt += 1.0f;
                }
                else
                {
                    if ( tile_hgt[5] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                    if ( tile_hgt[7] > 0.0f )
                    {
                        hsum += TILE_FSIZE;
                        hwgt += 1.0f;
                    }
                }

                vrt_hgt[15] = hsum / hwgt;
            }

            // check the west edge
            west_flat = bfalse;
            if ( vrt_hgt[12] == vrt_hgt[8] == vrt_hgt[4] == vrt_hgt[0] )
            {
                west_flat = btrue;
            }
            else if ( vrt_hgt[12] != vrt_hgt[0] )
            {
                west_flat = btrue;
            }

            // check the north edge
            north_flat = bfalse;
            if ( vrt_hgt[0] == vrt_hgt[1] == vrt_hgt[2] == vrt_hgt[3] )
            {
                north_flat = btrue;
            }
            else if ( vrt_hgt[0] != vrt_hgt[3] )
            {
                north_flat = btrue;
            }

            // check the east edge
            east_flat = bfalse;
            if ( vrt_hgt[3] == vrt_hgt[7] == vrt_hgt[11] == vrt_hgt[15] )
            {
                east_flat = btrue;
            }
            else if ( vrt_hgt[3] != vrt_hgt[15] )
            {
                east_flat = btrue;
            }

            // check the south edge
            south_flat = bfalse;
            if ( vrt_hgt[15] == vrt_hgt[14] == vrt_hgt[13] == vrt_hgt[12] )
            {
                south_flat = btrue;
            }
            else if ( vrt_hgt[15] != vrt_hgt[12] )
            {
                south_flat = btrue;
            }

            wall_flags = LAMBDA( west_flat,   0, 1 << 0 );
            wall_flags |= LAMBDA( north_flat, 0, 1 << 1 );
            wall_flags |= LAMBDA( east_flat,  0, 1 << 2 );
            wall_flags |= LAMBDA( south_flat, 0, 1 << 3 );

            // use the wall flags to get the type
            type = 0;
            switch ( wall_flags )
            {
                    // listed by complexity
                case  0: type =  5; break; // pillar/cross
                case  5: type =  8; break; // E-W wall/arch
                case 10: type =  9; break; // N-S wall/arch
                case  1: type = 12; break; // wall to the W
                case  2: type = 13; break; // wall to the N
                case  4: type = 14; break; // wall to the E
                case  8: type = 15; break; // wall to the S
                case  3: type = 17; break; // NW corner
                case  6: type = 18; break; // NE corner
                case  9: type = 16; break; // SW corner
                case 12: type = 19; break; // SE corner
                case  7: type = 22; break; // WNE T-junction
                case 11: type = 21; break; // SWN T-junction
                case 13: type = 20; break; // ESW T-junction
                case 14: type = 23; break; // NES T-junction
                case 15: type =  5; break; // arches
            }

            // calculate the twist of this tile
            tlst[itile].type = type;
        }
    }

    return pmesh;
}

//--------------------------------------------------------------------------------------------
map_t * map_generate_vertex_data( map_t * pmesh )
{
    /// @author BB
    /// @details generate vertices for an empty mesh
    /// @todo everything

    return pmesh;
}

//--------------------------------------------------------------------------------------------
// Cartman functions
//--------------------------------------------------------------------------------------------

Uint8 cartman_calc_twist( int x, int y )
{
    Uint8 twist;

    // x and y should be from -7 to 8
    if ( x < -7 ) x = -7;
    if ( x > 8 ) x = 8;
    if ( y < -7 ) y = -7;
    if ( y > 8 ) y = 8;

    // Now between 0 and 15
    x = x + 7;
    y = y + 7;
    twist = ( y << 4 ) + x;

    return twist;
}
