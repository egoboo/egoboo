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

/// @file  egolib/bsp_aabb.h
/// @brief BSP AABBs

#pragma once

#include "egolib/Math/_Include.hpp"
#include "egolib/bbox.h"
#include "egolib/DynamicArray.hpp"

/**
 * @brief
 *	An n-dimensional axis-aligned bounding box.
 */
class BSP_aabb_t
{
private:
	/**
	 * @brief
	 *	Is this bounding box assigned the empty set?
	 */
	bool _empty;
	/**
	 * @brief
	 *	A pointer to an array of <tt>_dim * 3</tt> elements.
	 *	The first (last) @a _dim values are the coordinates of the
	 *	smallest (greatest) point enclosed by this bounding box.
	 *	The remaining @a _dim values in the middle are the midpoint
	 *	of the bounding box.
	 */
	float *_values;
	/**
	 * @brief
	 *	The dimensionality of this BSP AABB.
	 */
	size_t _dim;

public:

	/**
	 * @brief
	 *	Get the dimensionality of this BSP AABB.
	 * @return
	 *	the dimensionality of this BSP AABB.
	 */
	size_t getDim() const
	{
		return _dim;
	}
	/**
	 * @brief
	 *	Set the dimensionality of this BSP AABB.
	 * @param newDim
	 *	the new dimensionality
     * @param preserve
     *  if @a true, preserve existing values up to the minimum of the old and the new dimensionality
	 */
	void setDim(size_t newDim,bool preserve);

	/**
	 * @brief
	 *	Construct this BSP AABB.
	 * @param dim
	 *	the dimensionality
	 * @post
	 *	A non-empty BSP AABB with its center at the origin an extend of zero along all axes is constructed.
	 *	The BSP AABB is not empty.
	 */
	BSP_aabb_t(size_t dim);
	/**
	 * @brief
	 *	Destruct this BSP ABB.
	 */
	~BSP_aabb_t();
	/**
	 * @brief
	 *	Get if this bounding box is assigned the empty set.
	 * @param self
	 *	this bounding box
	 * @return
	 *	@a true if this bounding box is empty, @a false otherwise
	 */
	bool empty() const;
	/**
	 * @brief
	 *	Assign this BSP AABB the empty set.
	 * @post
	 *	This BSP AABB is empty i.e. BSP_aabb_t::empty() is @a true.
	 */
	void clear();
	/**
	 * @brief
	 *	Get if this BSP AABB and another BSP AABB overlap.
	 * @param other
	 *	the other BSP AABB
	 * @return
	 *	@a true if this BSP AABB and the other BSP AABB overlap, @a false otherwise
	 * @remark
	 *	If the dimensionality of this BSP AABB and the other BSP AABB are different,
	 *		check the lowest common dimensionality.
	 */
	bool overlaps(const BSP_aabb_t& other) const;
	/**
	 * @brief
	 *	Get if this BSP AABB and another BSP AABB overlap.
	 * @param other
	 *	the other BSP AABB
	 * @return
	 *	@a true if this BSP AABB and the AABB overlap, @a false otherwise.
	 * @remark
	 *	If the dimensionality of the BSP AABB and the other BSP AABB are different,
	 *		check the lowest common dimensionality.
	 */
	bool contains(const BSP_aabb_t& other) const;
	/**
	 * @brief
	 *	Get if this BSP AABB and an AABB overlap.
	 * @param other
	 *	the AABB
	 * @return
	 *	@a true if this BSP AABB and the AABB overlap, @a false otherwise
	 * @remark
	 *	If the dimensionality of this BSP AABB and the AABB are different,
	 *		check the lowest common dimensionality.
	 */
	bool overlaps(const aabb_t& other) const;
	/**
	 * @brief
	 *	Get if this BSP AABB contains an AABB.
	 * @param other
	 *	the AABB
	 * @return
	 *	@a true if this BSP AABB contains the AABB, @a false otherwise
	 * @remark
	 *	If the dimensionality of this BSP AABB and the AABB are different,
	 *		check the lowest common dimensionality.
	 */
	bool contains(const aabb_t& other) const;
	const float *min() const;
	const float *mid() const;
	const float *max() const;
	float *min();
	float *mid();
	float *max();
	void add(const BSP_aabb_t& other);
	void set(const BSP_aabb_t& other);
	/**
	 * @brief
	 *	Convert from an OctBB to a BSP AABB.
	 * @param other
	 *	the OctBB
	 * @warning
	 *	If the dimensionality of the BSP AABB is smaller than the dimensionality of the OctBB,
	 *	exceeding dimensions are dropped!
	 */
	void set(const oct_bb_t& source);

	/**
	* @brief
	*	Assignment operator, handles safe copying of allocated pointers
	**/
	BSP_aabb_t& operator=(const BSP_aabb_t& other)
	{
        set(other);
		return *this;
	}

	//Disable copying class
	BSP_aabb_t(const BSP_aabb_t& copy) = delete;
};
