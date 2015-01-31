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

/// @file  game/bsp.h
/// @brief Global mesh, character and particle BSPs.
#pragma once

#include "game/obj_BSP.h"
#include "game/mesh_BSP.h"

/**
 * @brief
 *	Get the (global) mesh BSP.
 * @return
 *	the BSP
 * @pre
 *	the (global) mesh BSP was initialized
 */
mesh_BSP_t *getMeshBSP();

/**
 * @brief
 *	Initialize the (global) mesh BSP.
 * @param mesh
 *	the mesh used in initialization
 * @return
 *	@a true on success, @a false on failure
 * @remark
 *	If the BSPs were already initialized, they are re-initialized.
 */
bool mesh_BSP_system_begin(ego_mesh_t *mesh);

/**
 * @brief
 *	Uninitialize the (global) mesh BSP.
 * @remark
 *	If the mesh BSP is not initialized, this function is a noop.
 */
void mesh_BSP_system_end();

/**
 * @brief
 *	Get if the (global) mesh BSP is initialized.
 * @return
 *	@a true if the mesh BSP is initialized, @a false otherwise
 */
bool mesh_BSP_system_started();

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	Get the global BSP for the characters.
 * @return
 *	the BSP
 * @pre
 *	the global character BSP was initialized
 */
obj_BSP_t *getChrBSP();

/**
 * @brief
 *	Get the global BSP for the particles.
 * @return
 *	the BSP
 * @pre
 *	the global particle BSP was initialized 
 */
obj_BSP_t *getPrtBSP();

/**
 * @brief
 *	Initialize the (global) object (i.e. character and particle) BSPs.
 * @param mesh_bsp
 *	the mesh BSP used in initialization
 * @return
 *	@a true on success, @a false on failure
 * @post
 *	The object BSP is initialized.
 * @remark
 *	If the BSPs were already initialized, they are re-initialized.
 */
bool obj_BSP_system_begin(mesh_BSP_t * mesh_bsp);

/**
 * @brief
 *	Uninitialize the (global) object (i.e. character and particle) BSPs if they are initialized.
 * @remark
 *	If the object BSPs are not initialized, this function is a noop.
 * @post
 *	The object BSP is initialized.
 */
void obj_BSP_system_end();

/**
 * @brief
 *	Get if the (global) object (i.e. character and particle) BSPs are initialized.
 * @return
 *	@a true if the object BSPs are initialized, @a false otherwise
 */
bool obj_BSP_system_started();


bool chr_BSP_insert(chr_t * pchr);
bool chr_BSP_fill();
bool chr_BSP_clear();
bool chr_BSP_can_collide(BSP_leaf_t * pleaf);
bool chr_BSP_is_visible(BSP_leaf_t * pleaf);

bool prt_BSP_insert(s_prt_bundle * pbdl_prt);
bool prt_BSP_fill();
bool prt_BSP_clear();
bool prt_BSP_can_collide(BSP_leaf_t * pleaf);
bool prt_BSP_is_visible(BSP_leaf_t * pleaf);