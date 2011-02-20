#pragma once

///
/// @file
/// @brief A* pathfinding.
/// @details A very ugly implementation of the AStar pathfinding agorithm.
///   There is lots of room for improvement.

#include "egoboo_typedef.h"

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
bool_t AStar_find_path( ego_mpd_t *PMesh, Uint32 stoppedby, const int src_ix, const int src_iy, const int dst_ix, const int dst_iy );
bool_t AStar_get_path( const int src_ix, const int src_iy, const int dst_ix, const int dst_iy, waypoint_list_t *plst );
