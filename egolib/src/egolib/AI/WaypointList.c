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

/// @file egolib/AI/WaypointList.c
/// @brief implementation of waypoint lists

#include "egolib/AI/WaypointList.h"
#include "egolib/Math/_Include.hpp"

bool waypoint_list_peek(waypoint_list_t *self, waypoint_t wp)
{
	int index;

	// is the list valid?
	if (NULL == self || self->tail >= MAXWAY) return false;

	// is the list is empty?
	if (0 == self->head) return false;

	if (self->tail > self->head)
	{
		// fix the tail
		self->tail = self->head;

		// we have passed the last waypoint
		// just tell them the previous waypoint
		index = self->tail - 1;
	}
	else if (self->tail == self->head)
	{
		// we have passed the last waypoint
		// just tell them the previous waypoint
		index = self->tail - 1;
	}
	else
	{
		// tell them the current waypoint
		index = self->tail;
	}

	wp[kX] = self->pos[index][kX];
	wp[kY] = self->pos[index][kY];
	wp[kZ] = self->pos[index][kZ];

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

bool waypoint_list_reset(waypoint_list_t *self)
{
	/// @author BB
	/// @details reset the waypoint list to the beginning

	if (NULL == self) return false;

	self->tail = 0;

	return true;
}

bool waypoint_list_clear(waypoint_list_t *self)
{
	/// @author BB
	/// @details Clear out all waypoints

	if (NULL == self) return false;

	self->tail = 0;
	self->head = 0;

	return true;
}

bool waypoint_list_empty(waypoint_list_t *self)
{
	if (NULL == self) return true;

	return 0 == self->head;
}

bool waypoint_list_finished(waypoint_list_t *self)
{
	if (NULL == self || 0 == self->head) return true;

	return self->tail == self->head;
}

bool waypoint_list_advance(waypoint_list_t *self)
{
	bool retval;

	if (NULL == self) return false;

	retval = false;
	if (self->tail > self->head)
	{
		// fix the tail
		self->tail = self->head;
	}
	else if (self->tail < self->head)
	{
		// advance the tail
		self->tail++;
		retval = true;
	}

	// clamp the tail to valid values
	if (self->tail >= MAXWAY) self->tail = MAXWAY - 1;

	return retval;
}
