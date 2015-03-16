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

/// @file  egolib/Math/Cube.hpp
/// @brief Cubes.

#pragma once

#include "egolib/Math/Vector.hpp"

struct cube_t
{
	/**
	 * @brief
	 *	The center of the cube.
	 */
	fvec3_t center;
	/**
	 * @brief
	 *	The size of the cube.
	 */
	float size;
	/**
	 * @brief
	 *	Get the center of this cube.
	 * @return
	 *	the center of this cube
	 */
	const fvec3_t& getCenter() const
	{
		return center;
	}
	/**
	 * @brief
	 *	Get the size of this cube.
	 * @return
	 *	the size of this cube
	 */
	const float& getSize() const
	{
		return size;
	}
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
		center = other.center;
		size = other.size;
	}
	/**
	 * @brief
	 *	Get the minimum of this cube.
	 * @return
	 *	the minimum of this cube
	 */
	fvec3_t getMin() const
	{
		return fvec3_t(center.x - size, center.y - size, center.z - size);
	}
	/**
	 * @brief
	 *	Get the maximum of this cube.
	 * @return
	 *	the maximum of this cube
	 */
	fvec3_t getMax() const
	{
		return fvec3_t(center.x + size, center.y + size, center.z + size);
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
		assign(other);
		return *this;
	}
};
