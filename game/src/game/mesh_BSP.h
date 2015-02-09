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

/// @file game/mesh_BSP.h
/// @brief BSPs for meshes

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

// Forward declarations.
struct ego_mesh_t;
struct egolib_frustum_t;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	A BSP housing a mesh.
 */
struct mesh_BSP_t : public BSP_tree_t
{
	
	/**
	 * @brief
	 *	The parameters for creating a mesh BSP tree.
	 */
	class Parameters
	{

	public:

		/**
		* @brief
		*	Create the parameters for a mesh BSP tree.
		* @param mesh
		*	The mesh BSP tree on which the object BSP tree is based on
		* @throw std::invalid_argument
		*	if @a mesh is @a nullptr
		*/
		Parameters(const ego_mesh_t *mesh);

	public:
		/**
		 */

		/**
 		 * @brief
		 *	The maximum depth of the mesh BSP tree.
		 */
		size_t _maxDepth;

		/**
		 * @brief
		 *	The mesh the mesh BSP tree is based on.
		 */
		const ego_mesh_t *_mesh;

	};

	size_t count;
    oct_bb_t volume;

	/**
	 * @brief
	 *	Construct a mesh BSP tree.
	 * @param parameters
	 *	the parameters to construct the mesh BSP tree with
	 */
	mesh_BSP_t(const Parameters& parameters);

	/**
	 * @brief
	 *	Destuct a mesh BSP.
	 * @param self
	 *	the mesh BSP
	 */
	virtual ~mesh_BSP_t();

};

bool mesh_BSP_fill(mesh_BSP_t *self, const ego_mesh_t *mesh);
bool mesh_BSP_can_collide(BSP_leaf_t *leaf);
bool mesh_BSP_is_visible(BSP_leaf_t *leaf);
