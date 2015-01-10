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

/// @file game/AStar.h
/// @brief A* pathfinding.
/// @details A very ugly implementation of the A* pathfinding agorithm.
///          There is lots of room for improvement.

#pragma once

#include "game/egoboo_typedef.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// Forward declarations.
struct ego_mesh_t;
struct waypoint_list_t;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#if 0
struct AStar_Node_t;
#endif

//------------------------------------------------------------------------------
// A* pathfinding --------------------------------------------------------------
//------------------------------------------------------------------------------

struct AStar_Node_t
{
    float  weight;
    bool closed;

    int    ix, iy;
    AStar_Node_t *parent;
};

//------------------------------------------------------------------------------
//Public functions
bool AStar_find_path( ego_mesh_t *mesh, Uint32 stoppedBy, const int src_ix, const int src_iy, int dst_ix, int dst_iy );
bool AStar_get_path( const int pos_x, const int dst_y, waypoint_list_t *plst );
