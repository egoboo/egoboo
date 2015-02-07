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

/// @file game/obj_BSP.h

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

class GameObject;
struct s_prt_bundle;
struct mesh_BSP_t;

/**
 * @brief
 *	A BSP tree for objects.
 */
struct obj_BSP_t
{
	/**
	* @brief
	*	The parameters for creating an object BSP tree.
	*/
	class Parameters
	{
	public:
		/**
		 * @brief
		 *	The allowed minimum dimensionality of an object BSP tree.
		 */
		static const size_t ALLOWED_DIM_MIN = 2;
		/**
		 * @brief
		 *	The allowed maximum dimensionality of an object BSP tree.
		 */
		static const size_t ALLOWED_DIM_MAX = 3;
		/**
		 * @brief
		 *	Create parameters for an object BSP tree.
		 * @param dim
		 *	the desired dimensionality of the object BSP tree
		 * @param meshBSP
		 *	The mesh BSP tree on which the object BSP tree is based on
		 * @throw std::domain_error
		 *	if the desired dimensionality is smaller than @a 2 or greater than @a 3.
		 * @throw std::invalid_argument
		 *	if @a meshBSP is @a nullptr
		 */
		Parameters(size_t dim, const mesh_BSP_t *meshBSP);
	public:
		/**
		 * @brief
		 *	The desired dimensionality of the object BSP tree.
		 */
		size_t _dim;
		/**
		 * @brief
		 *	The mesh BSP tree on which the object BSP tree is based on.
		 */
		const mesh_BSP_t *_meshBSP;
	};

    /**
	 * @brief
	 *	The number of characters in this obj_BSP.
	 * @todo
	 *	The type should be @a size_t.
	 */
    size_t count;

    /**
	 * The BSP tree of characters for character-character and character-particle interactions.
	 */
    BSP_tree_t tree;

	/**
	 * @brief
	 *	Construct this object BSP tree.
	 * @param parameters
	 *	the parameters to construct the object BSP tree with
	 */
	obj_BSP_t(const Parameters& parameters);
	
	/**
	 * @brief
	 *	Destruct this object BSP tree.
	 */
	virtual ~obj_BSP_t();

	/**
	 * @brief
	 *	Fill the collision list with references to tiles that the object volume may overlap.
	 * @return
	 *	return the number of collisions in @a collisions
	 */
	void collide(const aabb_t *aabb, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;

	/**
	 * @brief
	 *	Fill the collision list with references to tiles that the object volume may overlap.
	 * @return
	 *	the number of collisions in @a collisions
	 */
	void collide(const egolib_frustum_t *frustum, BSP::LeafTest *test, Ego::DynamicArray<BSP_leaf_t *> *collisions) const;

};
