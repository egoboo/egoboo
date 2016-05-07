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

#pragma once

#include "cartman/cartman_typedef.h"

//--------------------------------------------------------------------------------------------

struct cartman_mpd_t;
struct cartman_mpd_tile_t;
struct s_tile_definition_t;
struct select_lst_t;

//--------------------------------------------------------------------------------------------

// For autoweld
#define NEAR_TOLERANCE 1.0f
#define NEAR_LOW       (0.0f + NEAR_TOLERANCE)
#define NEAR_HGH       (Info<float>::Grid::Size() - NEAR_TOLERANCE)

#define BARRIERHEIGHT 14.0f      //

//--------------------------------------------------------------------------------------------

float dist_from_border( cartman_mpd_t * pmesh, float x, float y );
int dist_from_edge( cartman_mpd_t * pmesh, int mapx, int mapy );
int nearest_edge_vertex( cartman_mpd_t * pmesh, int mapx, int mapy, float nearx, float neary );

void fix_mesh( cartman_mpd_t * pmesh );
void fix_corners( cartman_mpd_t * pmesh );
void fix_edges( cartman_mpd_t * pmesh );
void fix_vertices( cartman_mpd_t * pmesh, int mapx, int mapy );
void weld_corner_verts( cartman_mpd_t * pmesh, int mapx, int mapy );

// functions taking a selection as an argument
// Weld selected vertices.
void mesh_select_weld(select_lst_t *plst);
// Move selected vertices.
void mesh_select_move(select_lst_t *plst, float x, float y, float z);
void mesh_select_set_z_no_bound( select_lst_t * plst, float z );
void mesh_select_jitter( select_lst_t * plst );
void mesh_select_verts_connected( select_lst_t& plst );

// Ensure any vertex in the specified rectangular area is in the specified selection list.
void select_lst_add_rect( select_lst_t& plst, float x0, float y0, float z0, float x1, float y1, float z1, int mode );
// Ensure no vertex in the specified rectangular area is in the specified selection list.
void select_lst_remove_rect( select_lst_t& plst, float x0, float y0, float z0, float x1, float y1, float z1, int mode );


struct MeshEditor {

	// mesh functions
	static void mesh_set_tile(cartman_mpd_t * pmesh, Uint16 tiletoset, Uint8 upper, Uint16 presser, Uint8 tx);
	static void move_mesh_z(cartman_mpd_t * pmesh, int z, Uint16 tiletype, Uint16 tileand);
	static void move_vert(cartman_mpd_t * pmesh, int vert, float x, float y, float z);
	static void raise_mesh(cartman_mpd_t * pmesh, Uint32 point_lst[], size_t point_count, float x, float y, int amount, int size);
	static void level_vrtz(cartman_mpd_t * pmesh);
	static void jitter_mesh(cartman_mpd_t * pmesh);
	static void flatten_mesh(cartman_mpd_t * pmesh, int y0);
	static void clear_mesh(cartman_mpd_t * pmesh, Uint8 upper, Uint16 presser, Uint8 tx, Uint8 type);
	static void three_e_mesh(cartman_mpd_t * pmesh, Uint8 upper, Uint8 tx);
	static bool fan_isPassableFloor(cartman_mpd_t * pmesh, int mapx, int mapy);
	static bool isImpassableWall(cartman_mpd_t * pmesh, int mapx, int mapy);
	static void set_barrier_height(cartman_mpd_t * pmesh, int mapx, int mapy);
	static void fix_walls(cartman_mpd_t * pmesh);
	static void impass_edges(cartman_mpd_t * pmesh, int amount);

	static void mesh_replace_fx(cartman_mpd_t * pmesh, Uint16 fx_bits, Uint16 fx_mask, Uint8 fx_new);
	static void mesh_replace_tile(cartman_mpd_t * pmesh, int xfan, int yfan, int onfan, Uint8 tx, Uint8 upper, Uint8 fx, Uint8 type, Uint16 presser, bool tx_only, bool at_floor_level);
	static void setFX(cartman_mpd_t *pmesh, int fan, uint8_t fx);
	static void mesh_move(cartman_mpd_t * pmesh, float dx, float dy, float dz);
};

// indecipherable legacy code
Uint8  tile_is_different( cartman_mpd_t * pmesh, int fan_x, int fan_y, Uint16 fx_bits, Uint16 fx_mask );
Uint16 trim_code( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits );
Uint16 wall_code( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits );
void   trim_mesh_tile( cartman_mpd_t * pmesh, Uint16 fx_bits, Uint16 fx_mask );
