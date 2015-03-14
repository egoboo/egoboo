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

#pragma once

#include "egolib/frustum.h"
#include "egolib/Math/AABB.h"

class BSP_leaf_t;

namespace BSP
{


	typedef bool (LeafTest)(BSP_leaf_t *);

	class LeafHolder
	{

	public:

		/**
		 * @brief
		 *	Remove all leaves from this leaf holder and its subordinate leaf holders (i.e. recursively).
		 * @return
		 *	the number of removed leaves
		 */
		virtual size_t removeAllLeaves() = 0;

	protected:

		virtual ~LeafHolder() { }

	};

	class Collider
	{

	public:

		/**
		 * @brief
		 *	Recursively search the collider and its subordinate colliders (i.e. recursively) for collisions with a frustum.
		 * @param frustum
		 *	the frustum
		 * @param collisions
		 *	the collision list
		 */
		virtual void collide(const egolib_frustum_t& frustum, Ego::DynamicArray<BSP_leaf_t *>& collisions) const = 0;

		/**
		 * @brief
		 *	Recursively search the collider and its subordinate colliders (i.e. recursively) for collisions with a frustum.
		 * @param frustum
		 *	the frustum
		 * @param test
		 *	a leaf test any colliding leaf has to pass in addition to get into the collision list or @a nullptr
		 * @param collisions
		 *	the collision list
		 */
		virtual void collide(const egolib_frustum_t& frustum, LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const = 0;
		
		/**
 		 * @brief
		 *	Recursively search the collider and its subordinate colliders (i.e. recursively) for collisions with an AABB.
		 * @param aabb
		 *	the AABB
		 * @param collisions
		 *	the collision list
	 	 */
		virtual void collide(const aabb_t& aabb, Ego::DynamicArray<BSP_leaf_t *>& collisions) const = 0;

		/**
		 * @brief
		 *	Recursively search the collider and its subordinate colliders (i.e. recursively) for collisions with an AABB.
		 * @param aabb
		 *	the AABB
		 * @param test
		 *	a leaf test any colliding leaf has to pass in addition to get into the collision list
		 * @param collisions
		 *	the collision list
		 */
		virtual void collide(const aabb_t& aabb, LeafTest& test, Ego::DynamicArray<BSP_leaf_t *>& collisions) const = 0;
		
	protected:

		virtual ~Collider() { }

	};
}