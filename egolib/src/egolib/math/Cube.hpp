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

/// @file egolib/math/cube.h
/// @brief Cubes.

#pragma once

#include "egolib/vec.h"

struct cube_t
{
	/**
	 * @brief
	 *	The center of the cube.
	 * @todo
	 *	Rename to @a center.
	 */
	fvec3_t pos;
	/**
	 * @brief
	 *	The size of the cube.
	 */
	float size;
	/**
	 * @brief
	 *	Assign this cube the values of another cube.
	 * @param other
	 *	the other cube
	 * @post
	 *	This cube was assigned the values of the other cube.
	 */
	void assign(const cube_t& other)
	{
		pos = other.pos;
		size = other.size;
	}
	/**
	 * @brief
	 *	Assign this cube the values of another cube.
	 * @param other
	 *	the other cube
	 * @return
	 *	this cube
	 * @post
	 *	This cube was assigned the values of the other cube.
	 */
	cube_t& operator=(const cube_t& other)
	{

	}
};
