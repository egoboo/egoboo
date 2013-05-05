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

/// @file AStar.h
/// @brief A* pathfinding.
/// @details A very ugly implementation of the AStar pathfinding agorithm.
///   There is lots of room for improvement.

#include "egoboo_typedef.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

struct s_ego_mesh;
struct s_waypoint_list;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

struct s_AStar_Node;
typedef struct s_AStar_Node AStar_Node_t;

//------------------------------------------------------------------------------
// A* pathfinding --------------------------------------------------------------
//------------------------------------------------------------------------------

struct s_AStar_Node
{
    float  weight;
    ego_bool closed;

    int    ix, iy;
    AStar_Node_t *parent;
};

//------------------------------------------------------------------------------
//Public functions
ego_bool AStar_find_path( struct s_ego_mesh *PMesh, Uint32 stoppedby, const int src_ix, const int src_iy, int dst_ix, int dst_iy );
ego_bool AStar_get_path( const int pos_x, const int dst_y, struct s_waypoint_list *plst );
