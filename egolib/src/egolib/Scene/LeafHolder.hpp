#pragma once

#include "egolib/frustum.h"
#include "egolib/math/AABB.h"

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