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

bool waypoint_list_t::peek(waypoint_list_t& self, waypoint_t wp)
{
	int index;

	// is the list valid?
	if (self._tail >= MAXWAY) return false;

	// is the list is empty?
	if (0 == self._head) return false;

	if (self._tail > self._head)
	{
		// fix the tail
		self._tail = self._head;

		// we have passed the last waypoint
		// just tell them the previous waypoint
		index = self._tail - 1;
	}
	else if (self._tail == self._head)
	{
		// we have passed the last waypoint
		// just tell them the previous waypoint
		index = self._tail - 1;
	}
	else
	{
		// tell them the current waypoint
		index = self._tail;
	}

	wp[kX] = self._pos[index][kX];
	wp[kY] = self._pos[index][kY];
	wp[kZ] = self._pos[index][kZ];

	return true;
}

void waypoint_list_t::push(waypoint_list_t& self, int x, int y)
{
	// Add the waypoint.
	self._pos[self._head][kX] = x;
	self._pos[self._head][kY] = y;
	self._pos[self._head][kZ] = 0;

	// Do not let the list overflow.
	self._head++;
	if (self._head >= MAXWAY)
	{
		self._head = MAXWAY - 1;
	}
}

void waypoint_list_t::reset(waypoint_list_t& self)
{
	self._tail = 0;
}

void waypoint_list_t::clear(waypoint_list_t& self)
{
	self._tail = 0;
	self._head = 0;
}

bool waypoint_list_t::empty(waypoint_list_t& self)
{
	return 0 == self._head;
}

bool waypoint_list_t::finished(waypoint_list_t& self)
{
	if (0 == self._head) return true;

	return self._tail == self._head;
}

bool waypoint_list_t::advance(waypoint_list_t& self)
{
	bool retval;

	retval = false;
	if (self._tail > self._head)
	{
		// fix the tail
		self._tail = self._head;
	}
	else if (self._tail < self._head)
	{
		// advance the tail
		self._tail++;
		retval = true;
	}

	// clamp the tail to valid values
	if (self._tail >= MAXWAY) self._tail = MAXWAY - 1;

	return retval;
}
