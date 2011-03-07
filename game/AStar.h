#pragma once

///
/// @file AStar.h
/// @brief A* pathfinding.
/// @details A very ugly implementation of the AStar pathfinding agorithm.
///   There is lots of room for improvement.

#include "egoboo_typedef.h"

//------------------------------------------------------------------------------
struct s_ego_mpd;
struct s_waypoint_list;

//------------------------------------------------------------------------------
// A* pathfinding --------------------------------------------------------------
//------------------------------------------------------------------------------
typedef struct sAStar_Node AStar_Node_t;

struct sAStar_Node
{
    float  weight;
    bool_t closed;

    int    ix, iy;    
    AStar_Node_t *parent;
};


//------------------------------------------------------------------------------
//Public functions
bool_t AStar_find_path( struct s_ego_mpd *PMesh, Uint32 stoppedby, const int src_ix, const int src_iy, int dst_ix, int dst_iy );
bool_t AStar_get_path( const int dst_x, const int dst_y, struct s_waypoint_list *plst );
