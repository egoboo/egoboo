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

/// @file game/AI/WaypointList.c
/// @brief implementation of waypoint lists

#include "game/AI/WaypointList.h"

bool waypoint_list_peek(waypoint_list_t *plst, waypoint_t wp)
{
	int index;

	// is the list valid?
	if (NULL == plst || plst->tail >= MAXWAY) return false;

	// is the list is empty?
	if (0 == plst->head) return false;

	if (plst->tail > plst->head)
	{
		// fix the tail
		plst->tail = plst->head;

		// we have passed the last waypoint
		// just tell them the previous waypoint
		index = plst->tail - 1;
	}
	else if (plst->tail == plst->head)
	{
		// we have passed the last waypoint
		// just tell them the previous waypoint
		index = plst->tail - 1;
	}
	else
	{
		// tell them the current waypoint
		index = plst->tail;
	}

	wp[kX] = plst->pos[index][kX];
	wp[kY] = plst->pos[index][kY];
	wp[kZ] = plst->pos[index][kZ];

	return true;
}

bool waypoint_list_push(waypoint_list_t *self, int x, int y)
{
	if (NULL == self) return false;

	// Add the waypoint.
	self->pos[self->head][kX] = x;
	self->pos[self->head][kY] = y;
	self->pos[self->head][kZ] = 0;

	// Do not let the list overflow.
	self->head++;
	if (self->head >= MAXWAY)
	{
		self->head = MAXWAY - 1;
	}
	return true;
}

bool waypoint_list_reset(waypoint_list_t * plst)
{
	/// @author BB
	/// @details reset the waypoint list to the beginning

	if (NULL == plst) return false;

	plst->tail = 0;

	return true;
}

bool waypoint_list_clear(waypoint_list_t * plst)
{
	/// @author BB
	/// @details Clear out all waypoints

	if (NULL == plst) return false;

	plst->tail = 0;
	plst->head = 0;

	return true;
}

bool waypoint_list_empty(waypoint_list_t * plst)
{
	if (NULL == plst) return true;

	return 0 == plst->head;
}

bool waypoint_list_finished(waypoint_list_t * plst)
{
	if (NULL == plst || 0 == plst->head) return true;

	return plst->tail == plst->head;
}

bool waypoint_list_advance(waypoint_list_t * plst)
{
	bool retval;

	if (NULL == plst) return false;

	retval = false;
	if (plst->tail > plst->head)
	{
		// fix the tail
		plst->tail = plst->head;
	}
	else if (plst->tail < plst->head)
	{
		// advance the tail
		plst->tail++;
		retval = true;
	}

	// clamp the tail to valid values
	if (plst->tail >= MAXWAY) plst->tail = MAXWAY - 1;

	return retval;
}