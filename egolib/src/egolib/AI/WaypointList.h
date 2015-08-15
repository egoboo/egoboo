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

/// @file egolib/AI/WaypointList.h
/// @brief implementation of waypoint lists

#pragma once

#include "egolib/typedef.h"

/// The maximum number of waypoints in a waypoint list.
#define MAXWAY 8

/// A waypoint.
typedef float waypoint_t[3];

struct waypoint_list_t
{
	int          tail;         ///< Which waypoint
	int          head;         ///< Where to stick next
	waypoint_t   pos[MAXWAY];  ///< Waypoint
};

bool waypoint_list_peek(waypoint_list_t& self, waypoint_t wp);
/**
 * @brief
 * Append a waypoint to a waypoint list.
 *	If the capacity of the waypoint list is reached, the last waypoint in the list is overwritten.
 * @param self
 *	the waypoint list
 * @param x, y
 *	the waypoint
 */
void waypoint_list_push(waypoint_list_t& self, int x, int y);
void waypoint_list_reset(waypoint_list_t& self);
void waypoint_list_clear(waypoint_list_t& self);
bool waypoint_list_empty(waypoint_list_t& self);
bool waypoint_list_finished(waypoint_list_t& self);
bool waypoint_list_advance(waypoint_list_t& self);
