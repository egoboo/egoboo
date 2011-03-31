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

/// @file mpd_functions.c
/// @brief mpd functionality ported from cartman and EgoMap
/// @details 

#include "mpd_functions.h"

#include "mesh.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static Uint8 cartman_calc_twist( int x, int y );

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


//--------------------------------------------------------------------------------------------
Uint8 cartman_get_fan_twist( const ego_mpd_t * pmesh, Uint32 tile )
{
    size_t vrtstart;
    float z0, z1, z2, z3;
    float zx, zy;

    // check for a valid tile
    if ( INVALID_TILE == tile  || tile > pmesh->info.tiles_count ) return TWIST_FLAT;

    // if the tile is actually labelled as MPD_FANOFF, ignore it completely
    if ( TILE_IS_FANOFF( pmesh->tmem.tile_list[tile] ) ) return TWIST_FLAT;

    vrtstart = pmesh->tmem.tile_list[tile].vrtstart;

    z0 = pmesh->tmem.plst[vrtstart + 0][ZZ];
    z1 = pmesh->tmem.plst[vrtstart + 1][ZZ];
    z2 = pmesh->tmem.plst[vrtstart + 2][ZZ];
    z3 = pmesh->tmem.plst[vrtstart + 3][ZZ];

    zx = CARTMAN_FIXNUM * ( z0 + z3 - z1 - z2 ) / CARTMAN_SLOPE;
    zy = CARTMAN_FIXNUM * ( z2 + z3 - z0 - z1 ) / CARTMAN_SLOPE;

    return cartman_calc_twist( zx, zy );
}

