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

/// @file  egolib/math/AABB.h
/// @brief Axis-aligned bounding boxes.

#pragma once

#include "egolib/math/Sphere.h"

// Forward declaration.
struct oct_bb_t;

/**
 * @brief
 *	An axis-aligned bounding box ("AABB").
 * @remark
 *	The terms "the/an axis-aligned bounding box (object)" and "the/an AABB (object)" are synonyms.
 */
struct aabb_t
{
    /// @brief The minimum of the bounding box.
    fvec3_t mins;
    /// @brief The maximum of the bounding box.
    fvec3_t maxs;

	aabb_t() :
		mins(0.0f, 0.0f, 0.0f),
		maxs(0.0f, 0.0f, 0.0f)
	{
		//ctor
	}

    /**
     * @brief
     *  Get the minimum.
     * @return
     *  the minimum
     */
    const fvec3_t& getMin() const
    {
        return mins;
    }

    /**
     * @brief
     *  Get the maximum.
     * @return
     *  the maximum
     */
    const fvec3_t& getMax() const
    {
        return maxs;
    }

    /**
     * @brief
     *  Get the center.
     * @return
     *  the center
     */
    fvec3_t getCenter() const
    {
        return (mins + maxs) * 0.5f;
    }

	/**
	 * @brief
	 *	Construct this bounding box assigning it the default values of a bounding box.
	 * @return
	 *	a pointer to this bounding box on success, @a nullptr on failure
	 * @post
	 *	This bounding box was assigned the default values of a bounding box.
	 * @remark
	 *	The default values of a bounding box are the center of @a (0,0,0) and the size of @a 0 along all axes
	 */
	aabb_t *ctor()
	{
		for (size_t i = 0; i < 3; ++i)
		{
			mins[i] = maxs[i] = 0.0f;
		}
		return this;
	}
	/**
	 * @brief
	 *	Destruct this bounding box.
	 */
	void dtor()
	{
		for (size_t i = 0; i < 3; ++i)
		{
			mins[i] = maxs[i] = 0.0f;
		}
	}

    /**
     * @brief
     *  Assign the values of this axis-aligned bounding box
     *  such that it is the smallest AABB enclosing the given octagonal bounding box.
     * @param other
     *  the octagonal bounding box
     */
    void from(const oct_bb_t& other);

	/**
	 * @brief
	 *	Assign this bounding box the values of another bounding box.
	 * @param other
	 *	the other bounding box
	 * @post
	 *	This bounding box was assigned the values of the other bounding box.
	 */
	void assign(const aabb_t& other)
	{
		for (size_t cnt = 0; cnt < 3; cnt++)
		{
			mins[cnt] = other.mins[cnt];
			maxs[cnt] = other.maxs[cnt];
		}
	}
	/**
	 * @brief
	 *	Assign this bounding box the values of another bounding box.
	 * @param other
	 *	the other bounding box
	 * @return
	 *	this bounding box
	 * @post
	 *	This bounding box was assigned the values of the other bounding box.
	 */
	aabb_t& operator=(const aabb_t& other)
	{
		assign(other);
		return *this;
	}

    /**
     * @brief
     *	Assign this AABB the join if itself with another AABB.
     * @param other
     *	the other AABB
     * @post
     *	The result of the join was assigned to this AABB.
     */
    void join(const aabb_t& other);

    /**
     * @brief
     *	Get if an AABB contains another AABB.
     * @param x
     *	this AABB
     * @param y
     *	the other AABB
     * @return
     *	@a true if @a x contains @a y, @a false otherwise
     * @remark
     *  This function is <em>not</em> commutative.
     */
    static bool contains(const aabb_t& x, const aabb_t& y);

    /**
     * @brief
     *	Get if an AABB overlaps with another AABB.
     * @param x
     *	the AABB
     * @param y
     *	the other AABB
     * @return
     *	@a true if the AABBs overlap, @a false otherwise
     * @remark
     *  The function is commutative.
     */
    static bool overlaps(const aabb_t& x, const aabb_t& y);
};

/** @todo Remove this. */
bool aabb_self_clear(aabb_t *dst);

/** @todo Remove this. */
bool aabb_is_clear(const aabb_t *dst);


