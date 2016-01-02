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

/// @file egolib/AI/AStar.h
/// @brief A* pathfinding.
/// @details A very ugly implementation of the A* pathfinding agorithm.
///          There is lots of room for improvement.

#pragma once

#include "egolib/AI/WaypointList.h"

// Forward declarations.
class ego_mesh_t;
struct waypoint_list_t;

/// Implementation of A* pathfinding algorithm.
class AStar {

public:
    struct Node {
        Node(int x, int y, float setWeight, const std::shared_ptr<Node>& setParent) :
            ix(x),
            iy(y),
            weight(setWeight),
            parent(setParent)
        {
            //ctor
        }

        float weight;
        int ix, iy;
        std::shared_ptr<Node> parent;
    };

public:
    AStar();
    bool find_path(const std::shared_ptr<const ego_mesh_t>& mesh, uint32_t stoppedBy, const int src_ix, const int src_iy, int dst_ix, int dst_iy);
    bool get_path(const int pos_x, const int dst_y, waypoint_list_t& wplst);

private:
    static constexpr size_t MAX_ASTAR_NODES = 512;   ///< Maximum number of nodes to explore
    static constexpr size_t MAX_ASTAR_PATH = 128;    ///< Maximum length of the final path (before pruning)

    std::shared_ptr<Node> final_node;
    std::shared_ptr<Node> start_node;

private:
    std::shared_ptr<Node> add_node(const int x, const int y, const std::shared_ptr<AStar::Node> &parent, float weight, std::priority_queue<std::shared_ptr<Node>>& openNodes);
    void reset();
};

extern AStar g_astar;
