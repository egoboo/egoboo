#pragma once

//********************************************************************************************
//*
//*    This file is part of Cartman.
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
//*
//********************************************************************************************

#include <egolib/egolib.h>

#include "cartman_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_cartman_mpd;
struct s_select_lst;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define NEARLOW  0.0f //16.0f     // For autoweld
#define NEARHI 128.0f //112.0f        //
#define BARRIERHEIGHT 14.0f      //

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

float dist_from_border( struct s_cartman_mpd * pmesh, float x, float y );
int dist_from_edge( struct s_cartman_mpd * pmesh, int x, int y );
int nearest_vertex( struct s_cartman_mpd * pmesh, int x, int y, float nearx, float neary );

void fix_mesh( struct s_cartman_mpd * pmesh );
void fix_vertices( struct s_cartman_mpd * pmesh, int x, int y );
void fix_corners( struct s_cartman_mpd * pmesh, int x, int y );

void weld_0( struct s_cartman_mpd * pmesh, int x, int y );
void weld_1( struct s_cartman_mpd * pmesh, int x, int y );
void weld_2( struct s_cartman_mpd * pmesh, int x, int y );
void weld_3( struct s_cartman_mpd * pmesh, int x, int y );
void weld_cnt( struct s_cartman_mpd * pmesh, int x, int y, int cnt, Uint32 fan );

// functions taking a selection as an argument
void mesh_select_weld( const struct s_select_lst * plst );
void mesh_select_move( const struct s_select_lst * plst, float x, float y, float z );
void mesh_select_set_z_no_bound( const struct s_select_lst * plst, float z );
void mesh_select_jitter( const struct s_select_lst * plst );
void mesh_select_verts_connected( const struct s_select_lst * plst );

// select_lst_t extensions
struct s_select_lst * select_lst_add_rect( struct s_select_lst * plst, float x0, float y0, float z0, float x1, float y1, float z1, int mode );
struct s_select_lst * select_lst_remove_rect( struct s_select_lst * plst, float x0, float y0, float z0, float x1, float y1, float z1, int mode );

// mesh functions
void mesh_set_tile( struct s_cartman_mpd * pmesh, Uint16 tiletoset, Uint8 upper, Uint16 presser, Uint8 tx );
void move_mesh_z( struct s_cartman_mpd * pmesh, int z, Uint16 tiletype, Uint16 tileand );
void move_vert( struct s_cartman_mpd * pmesh, int vert, float x, float y, float z );
void raise_mesh( struct s_cartman_mpd * pmesh, Uint32 point_lst[], size_t point_count, float x, float y, int amount, int size );
void level_vrtz( struct s_cartman_mpd * pmesh );
void jitter_mesh( struct s_cartman_mpd * pmesh );
void flatten_mesh( struct s_cartman_mpd * pmesh, int y0 );
void clear_mesh( struct s_cartman_mpd * pmesh, Uint8 upper, Uint16 presser, Uint8 tx, Uint8 type );
void three_e_mesh( struct s_cartman_mpd * pmesh, Uint8 upper, Uint8 tx );
bool_t fan_is_floor( struct s_cartman_mpd * pmesh, int x, int y );
void   set_barrier_height( struct s_cartman_mpd * pmesh, int x, int y, int bits );
void   fix_walls( struct s_cartman_mpd * pmesh );
void   impass_edges( struct s_cartman_mpd * pmesh, int amount );

void mesh_replace_fx( struct s_cartman_mpd * pmesh, Uint16 fx_bits, Uint16 fx_mask, Uint8 fx_new );
void mesh_replace_tile( struct s_cartman_mpd * pmesh, int xfan, int yfan, int onfan, Uint8 tx, Uint8 upper, Uint8 fx, Uint8 type, Uint16 presser, bool_t tx_only, bool_t at_floor_level );
void mesh_set_fx( struct s_cartman_mpd * pmesh, int fan, Uint8 fx );
void mesh_move( struct s_cartman_mpd * pmesh, float dx, float dy, float dz );

// indecipherable legacy code
Uint8  tile_is_different( struct s_cartman_mpd * pmesh, int fan_x, int fan_y, Uint16 fx_bits, Uint16 fx_mask );
Uint16 trim_code( struct s_cartman_mpd * pmesh, int x, int y, Uint16 fx_bits );
Uint16 wall_code( struct s_cartman_mpd * pmesh, int x, int y, Uint16 fx_bits );
void   trim_mesh_tile( struct s_cartman_mpd * pmesh, Uint16 fx_bits, Uint16 fx_mask );
