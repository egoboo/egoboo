#pragma once

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

/// @file mesh.inl

#include "mesh.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

static INLINE float  mesh_get_level( const ego_mpd_t * pmesh, float pos_x, float pos_y );
static INLINE Uint32 mesh_get_block( const ego_mpd_t * pmesh, float pos_x, float pos_y );
static INLINE Uint32 mesh_get_grid( const ego_mpd_t * pmesh, float pos_x, float pos_y );

static INLINE Uint32 mesh_get_block_int( const ego_mpd_t * pmesh, int block_x, int block_y );
static INLINE Uint32 mesh_get_tile_int( const ego_mpd_t * pmesh, int grid_x,  int grid_y );

static INLINE Uint32 mesh_test_fx( const ego_mpd_t * pmesh, Uint32 itile, const BIT_FIELD flags );
static INLINE bool_t mesh_clear_fx( ego_mpd_t * pmesh, Uint32 itile, const BIT_FIELD flags );
static INLINE bool_t mesh_add_fx( ego_mpd_t * pmesh, Uint32 itile, const BIT_FIELD flags );

static INLINE Uint32 mesh_has_some_mpdfx( const BIT_FIELD mpdfx, const BIT_FIELD test );
static INLINE bool_t mesh_grid_is_valid( const ego_mpd_t *, Uint32 id );

static INLINE bool_t mesh_tile_has_bits( const ego_mpd_t *, const int ix, const int iy, const BIT_FIELD bits );

static INLINE ego_tile_info_t * mesh_get_ptile( const ego_mpd_t *, const Uint32 itile );
static INLINE ego_grid_info_t * mesh_get_pgrid( const ego_mpd_t *, const Uint32 itile );
static INLINE Uint8             mesh_get_twist( ego_mpd_t * pmesh, const Uint32 igrid );

static INLINE GRID_FX_BITS ego_grid_info_get_all_fx( const ego_grid_info_t * );
static INLINE GRID_FX_BITS ego_grid_info_test_all_fx( const ego_grid_info_t *, const GRID_FX_BITS bits );
static INLINE bool_t       ego_grid_info_add_pass_fx( ego_grid_info_t *, const GRID_FX_BITS bits );
static INLINE bool_t       ego_grid_info_sub_pass_fx( ego_grid_info_t *, const GRID_FX_BITS bits );
static INLINE bool_t       ego_grid_info_set_pass_fx( ego_grid_info_t *, const GRID_FX_BITS bits );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------

static INLINE bool_t mesh_tile_has_bits( const ego_mpd_t * pmesh, const int ix, const int iy, const BIT_FIELD bits )
{
    int itile;
    Uint8 fx;

    //figure out which tile we are on
    itile = mesh_get_tile_int( pmesh, ix, iy );

    //everything outside the map bounds is wall and impassable
    if ( !mesh_grid_is_valid( pmesh, itile ) )
    {
        return HAS_SOME_BITS(( MPDFX_IMPASS | MPDFX_WALL ), bits );
    }

    // since we KNOW that this is in range, allow raw access to the data strucutre
    fx = ego_grid_info_get_all_fx( pmesh->gmem.grid_list + itile );

    return HAS_SOME_BITS( fx, bits );
}

//--------------------------------------------------------------------------------------------
static INLINE Uint32 mesh_has_some_mpdfx( const BIT_FIELD mpdfx, const BIT_FIELD test )
{
    mesh_mpdfx_tests++;
    return HAS_SOME_BITS( mpdfx, test );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mesh_grid_is_valid( const ego_mpd_t * pmpd, Uint32 id )
{
    if ( NULL == pmpd ) return bfalse;

    mesh_bound_tests++;

    if ( INVALID_TILE == id ) return bfalse;

    return id < pmpd->info.tiles_count;
}

//--------------------------------------------------------------------------------------------
static INLINE float mesh_get_level( const ego_mpd_t * pmesh, float x, float y )
{
    /// @details ZZ@> This function returns the height of a point within a mesh fan, precisely

    Uint32 tile;
    int ix, iy;

    float z0, z1, z2, z3;         // Height of each fan corner
    float zleft, zright, zdone;   // Weighted height of each side

    tile = mesh_get_grid( pmesh, x, y );
    if ( !mesh_grid_is_valid( pmesh, tile ) ) return 0;

    ix = x;
    iy = y;

    ix &= GRID_MASK;
    iy &= GRID_MASK;

    z0 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 0 ][ZZ];
    z1 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 1 ][ZZ];
    z2 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 2 ][ZZ];
    z3 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 3 ][ZZ];

    zleft  = ( z0 * ( GRID_FSIZE - iy ) + z3 * iy ) / GRID_FSIZE;
    zright = ( z1 * ( GRID_FSIZE - iy ) + z2 * iy ) / GRID_FSIZE;
    zdone  = ( zleft * ( GRID_FSIZE - ix ) + zright * ix ) / GRID_FSIZE;

    return zdone;
}

//--------------------------------------------------------------------------------------------
static INLINE Uint32 mesh_get_block( const ego_mpd_t * pmesh, float pos_x, float pos_y )
{
    Uint32 block = INVALID_BLOCK;

    if ( pos_x >= 0.0f && pos_x <= pmesh->gmem.edge_x && pos_y >= 0.0f && pos_y <= pmesh->gmem.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix /= BLOCK_ISIZE;
        iy /= BLOCK_ISIZE;

        block = mesh_get_block_int( pmesh, ix, iy );
    }

    return block;
}

//--------------------------------------------------------------------------------------------
static INLINE Uint32 mesh_get_grid( const ego_mpd_t * pmesh, float pos_x, float pos_y )
{
    Uint32 tile = INVALID_TILE;

    if ( pos_x >= 0.0f && pos_x < pmesh->gmem.edge_x && pos_y >= 0.0f && pos_y < pmesh->gmem.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        // these are known to be positive, so >> is not a problem
        ix >>= GRID_BITS;
        iy >>= GRID_BITS;

        tile = mesh_get_tile_int( pmesh, ix, iy );
    }

    return tile;
}

//--------------------------------------------------------------------------------------------
static INLINE Uint32 mesh_get_block_int( const ego_mpd_t * pmesh, int block_x, int block_y )
{
    if ( NULL == pmesh ) return INVALID_BLOCK;

    if ( block_x < 0 || block_x >= pmesh->gmem.blocks_x )  return INVALID_BLOCK;
    if ( block_y < 0 || block_y >= pmesh->gmem.blocks_y )  return INVALID_BLOCK;

    return block_x + pmesh->gmem.blockstart[block_y];
}

//--------------------------------------------------------------------------------------------
static INLINE Uint32 mesh_get_tile_int( const ego_mpd_t * pmesh, int grid_x,  int grid_y )
{
    if ( NULL == pmesh ) return INVALID_TILE;

    if ( grid_x < 0 || grid_x >= pmesh->info.tiles_x )  return INVALID_TILE;
    if ( grid_y < 0 || grid_y >= pmesh->info.tiles_y )  return INVALID_TILE;

    return grid_x + pmesh->gmem.tilestart[grid_y];
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mesh_clear_fx( ego_mpd_t * pmesh, Uint32 itile, const BIT_FIELD flags )
{
    // test for mesh
    if ( NULL == pmesh ) return bfalse;

    // test for invalid tile
    mesh_bound_tests++;
    if ( itile > pmesh->info.tiles_count ) return bfalse;

    return ego_grid_info_sub_pass_fx( pmesh->gmem.grid_list + itile, flags );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t mesh_add_fx( ego_mpd_t * pmesh, Uint32 itile, const BIT_FIELD flags )
{
    // test for mesh
    if ( NULL == pmesh ) return bfalse;

    // test for invalid tile
    mesh_bound_tests++;
    if ( itile > pmesh->info.tiles_count ) return bfalse;

    // succeed only of something actually changed
    return ego_grid_info_add_pass_fx( pmesh->gmem.grid_list + itile, flags );
}

//--------------------------------------------------------------------------------------------
static INLINE Uint32 mesh_test_fx( const ego_mpd_t * pmesh, Uint32 itile, const BIT_FIELD flags )
{
    // test for mesh
    if ( NULL == pmesh ) return 0;

    // test for invalid tile
    mesh_bound_tests++;
    if ( itile > pmesh->info.tiles_count )
    {
        return flags & ( MPDFX_WALL | MPDFX_IMPASS );
    }

    // if the tile is actually labelled as MPD_FANOFF, ignore it completely
    if ( TILE_IS_FANOFF( pmesh->tmem.tile_list[itile] ) )
    {
        return 0;
    }

    return ego_grid_info_test_all_fx( pmesh->gmem.grid_list + itile, flags );
}

//--------------------------------------------------------------------------------------------
static INLINE ego_tile_info_t * mesh_get_ptile( const ego_mpd_t * pmesh, const Uint32 itile )
{
    // valid parameters?
    if ( NULL == pmesh || itile >= pmesh->info.tiles_count ) return NULL;

    // a double check in case the tiles aren't allocated
    if ( NULL == pmesh->tmem.tile_list || itile >= pmesh->tmem.tile_count ) return NULL;

    return pmesh->tmem.tile_list + itile;
}

//--------------------------------------------------------------------------------------------
static INLINE ego_grid_info_t * mesh_get_pgrid( const ego_mpd_t * pmesh, const Uint32 igrid )
{
    // valid parameters?
    if ( NULL == pmesh || igrid >= pmesh->info.tiles_count ) return NULL;

    // a double check in case the grids aren't allocated
    if ( NULL == pmesh->gmem.grid_list || igrid >= pmesh->gmem.grid_count ) return NULL;

    return pmesh->gmem.grid_list + igrid;
}

//--------------------------------------------------------------------------------------------
static INLINE Uint8 mesh_get_twist( ego_mpd_t * pmesh, const Uint32 igrid )
{
    // valid parameters?
    if ( NULL == pmesh || igrid >= pmesh->info.tiles_count ) return TWIST_FLAT;

    // a double check in case the grids aren't allocated
    if ( NULL == pmesh->gmem.grid_list || igrid >= pmesh->gmem.grid_count ) return TWIST_FLAT;

    return pmesh->gmem.grid_list[igrid].twist;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE GRID_FX_BITS ego_grid_info_get_all_fx( const ego_grid_info_t * pgrid )
{
    if ( NULL == pgrid ) return MPDFX_WALL | MPDFX_IMPASS;

    return pgrid->wall_fx | pgrid->pass_fx;
}

//--------------------------------------------------------------------------------------------
static INLINE GRID_FX_BITS ego_grid_info_test_all_fx( const ego_grid_info_t * pgrid, const GRID_FX_BITS bits )
{
    GRID_FX_BITS grid_bits;

    if ( NULL == pgrid )
    {
        grid_bits = MPDFX_WALL | MPDFX_IMPASS;
    }
    else
    {
        grid_bits = ego_grid_info_get_all_fx( pgrid );
    }

    return grid_bits & bits;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t ego_grid_info_add_pass_fx( ego_grid_info_t * pgrid, const GRID_FX_BITS bits )
{
    GRID_FX_BITS old_bits, new_bits;

    if ( NULL == pgrid ) return bfalse;

    // save the old bits
    old_bits = ego_grid_info_get_all_fx( pgrid );

    // set the bits that we can modify
    SET_BIT( pgrid->pass_fx, bits );

    // get the new bits
    new_bits = ego_grid_info_get_all_fx( pgrid );

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t ego_grid_info_sub_pass_fx( ego_grid_info_t * pgrid, const GRID_FX_BITS bits )
{
    GRID_FX_BITS old_bits, new_bits;

    if ( NULL == pgrid ) return bfalse;

    // save the old bits
    old_bits = ego_grid_info_get_all_fx( pgrid );

    // set the bits that we can modify
    UNSET_BIT( pgrid->pass_fx, bits );

    // get the new bits
    new_bits = ego_grid_info_get_all_fx( pgrid );

    // let the caller know if they changed anything
    return old_bits != new_bits;
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t ego_grid_info_set_pass_fx( ego_grid_info_t * pgrid, const GRID_FX_BITS bits )
{
    GRID_FX_BITS old_bits, new_bits;

    if ( NULL == pgrid ) return bfalse;

    // save the old bits
    old_bits = ego_grid_info_get_all_fx( pgrid );

    // set the bits that we can modify
    pgrid->pass_fx = bits;

    // get the new bits
    new_bits = ego_grid_info_get_all_fx( pgrid );

    // let the caller know if they changed anything
    return old_bits != new_bits;
}
