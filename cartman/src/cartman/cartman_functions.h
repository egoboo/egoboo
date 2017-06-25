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
#include "egolib/Mesh/Info.hpp"

//--------------------------------------------------------------------------------------------

// For autoweld
#define NEAR_TOLERANCE 1.0f
#define NEAR_LOW       (0.0f + NEAR_TOLERANCE)
#define NEAR_HGH       (Info<float>::Grid::Size() - NEAR_TOLERANCE)

#define BARRIERHEIGHT 14.0f      //

//--------------------------------------------------------------------------------------------

float dist_from_border( cartman_mpd_t& mesh, float x, float y );
int dist_from_edge( cartman_mpd_t& mesh, Index2D index2d );
int nearest_edge_vertex( cartman_mpd_t& mesh, Index2D index2d, float nearx, float neary );

void fix_mesh( cartman_mpd_t& mesh );
void fix_corners( cartman_mpd_t& mesh );
void fix_edges( cartman_mpd_t& mesh );
void fix_vertices( cartman_mpd_t& mesh, Index2D index2d );

void weld_TL(cartman_mpd_t& mesh, Index2D index2d);
void weld_TR(cartman_mpd_t& mesh, Index2D index2d);
void weld_BR(cartman_mpd_t& mesh, Index2D index2d);
void weld_BL(cartman_mpd_t& mesh, Index2D index2d);
void weld_corner_verts( cartman_mpd_t& mesh, Index2D index2d );
void weld_edge_verts(cartman_mpd_t& mesh, cartman_mpd_tile_t * pfan, tile_definition_t * pdef, int cnt, Index2D index2d);

// functions taking a selection as an argument
// Weld selected vertices.
void mesh_select_weld(select_lst_t& plst);
// Move selected vertices.
void mesh_select_move(select_lst_t& plst, float x, float y, float z);
void mesh_select_set_z_no_bound( select_lst_t& plst, float z );
void mesh_select_jitter( select_lst_t& plst );
void mesh_select_verts_connected( select_lst_t& plst );

// Ensure any vertex in the specified rectangular area is in the specified selection list.
void select_lst_add_rect( select_lst_t& plst, const Vector3f& a, const Vector3f& b, int mode );
// Ensure no vertex in the specified rectangular area is in the specified selection list.
void select_lst_remove_rect( select_lst_t& plst, const Vector3f& a, const Vector3f& b, int mode );


struct MeshEditor {

	// mesh functions
	static void mesh_set_tile(cartman_mpd_t& mesh, uint16_t tiletoset, uint8_t upper, uint16_t presser, uint8_t tx);
	static void move_mesh_z(cartman_mpd_t& mesh, int z, uint16_t tiletype, uint16_t tileand);
	static void move_vert(cartman_mpd_t& mesh, int vert, float x, float y, float z);
	static void raise_mesh(cartman_mpd_t& mesh, uint32_t point_lst[], size_t point_count, float x, float y, int amount, int size);
	static void level_vrtz(cartman_mpd_t& mesh);
	static void jitter_mesh(cartman_mpd_t& mesh);
	static void flatten_mesh(cartman_mpd_t& mesh, int y0);
	static void clear_mesh(cartman_mpd_t& mesh, uint8_t upper, uint16_t presser, uint8_t tx, uint8_t type);
	static void three_e_mesh(cartman_mpd_t& mesh, uint8_t upper, uint8_t tx);
	static bool fan_isPassableFloor(cartman_mpd_t& mesh, const Index2D& index2d);
	static bool isImpassableWall(cartman_mpd_t& mesh, const Index2D& index2d);
	static void set_barrier_height(cartman_mpd_t& mesh, const Index2D& index2d);
	static void fix_walls(cartman_mpd_t& mesh);
	static void impass_edges(cartman_mpd_t& mesh, int amount);

	static void mesh_replace_fx(cartman_mpd_t& mesh, uint16_t fx_bits, uint16_t fx_mask, uint8_t fx_new);
	static void mesh_replace_tile(cartman_mpd_t& mesh, int xfan, int yfan, int onfan, uint8_t tx, uint8_t upper, uint8_t fx, uint8_t type, uint16_t presser, bool tx_only, bool at_floor_level);
	static void setFX(cartman_mpd_t& mesh, int fan, uint8_t fx);
	static void mesh_move(cartman_mpd_t& mesh, float dx, float dy, float dz);
};

// indecipherable legacy code
uint8_t  tile_is_different( cartman_mpd_t& mesh, Index2D index2d, uint16_t fx_bits, uint16_t fx_mask );
uint16_t trim_code( cartman_mpd_t& mesh, const Index2D& index2d, uint16_t fx_bits );
uint16_t wall_code( cartman_mpd_t& mesh, const Index2D& index2d, uint16_t fx_bits );
void   trim_mesh_tile( cartman_mpd_t& mesh, uint16_t fx_bits, uint16_t fx_mask );
