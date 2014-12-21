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
/// @brief global character and particle BSPs
#pragma once

#include "game/obj_BSP.h"
#include "game/mesh_BSP.h"

// Forward declaration.
struct ego_mesh;
struct s_prt_bundle;

/**
 * @brief
 *	Global BSP for the mesh.
 * @todo
 *	Should be <tt>mesh_BSP_T *p_mesh_BSP_root</tt>.
 */
extern mesh_BSP_t mesh_BSP_root;

/**
 * @brief
 *	Initialize the (global) mesh BSP.
 * @param mesh
 *	the mesh used in initialization
 * @todo
 *	Document return value.
 */
egolib_rv mesh_BSP_system_begin(ego_mesh_t *mesh);

/**
 * @brief
 *	Uninitialize the (global) mesh BSP.
 * @remark
 *	If the mesh BSP is not initialized, this function is a noop.
 */
egolib_rv mesh_BSP_system_end(void);

/**
 * @brief
 *	Get if the (global) mesh BSP is initialized.
 * @return
 *	@a true if the mesh BSP is initialized, @a false otherwise
 */
bool mesh_BSP_system_started(void);

//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	Global BSP for the characters.
 * @todo
 *	Should be <tt>chr_BSP_T *p_chr_BSP_root</tt>.
 */
extern obj_BSP_t chr_BSP_root;

/**
 * @brief
 *	Global BSP for the particles.
 * @todo
 *	Should be <tt>obj_BSP_T *p_prt_BSP_root</tt>.
 */
extern obj_BSP_t prt_BSP_root;

/**
 * @brief
 *	Initialize the (global) object (i.e. character and particle) BSPs.
 * @param meshBSP
 *	the mesh BSP used in initialization
 */
void obj_BSP_system_begin(struct s_mpd_BSP * pBSP);

/**
 * @brief
 *	Uninitialize the (global) object (i.e. character and particle) BSPs if they are initialized.
 * @remark
 *	If the object BSPs are not initialized, this function is a noop.
 */
void obj_BSP_system_end();

/**
* @brief
*	Get if the (global) object (i.e. character and particle) BSPs are initialized.
* @return
*	@a true if the object BSPs are initialized, @a false otherwise
*/
bool obj_BSP_system_started();


bool chr_BSP_insert(struct s_chr * pchr);
bool chr_BSP_fill();
bool chr_BSP_clear();
bool chr_BSP_can_collide(BSP_leaf_t * pleaf);
bool chr_BSP_is_visible(BSP_leaf_t * pleaf);

bool prt_BSP_insert(struct s_prt_bundle * pbdl_prt);
bool prt_BSP_fill();
bool prt_BSP_clear();
bool prt_BSP_can_collide(BSP_leaf_t * pleaf);
bool prt_BSP_is_visible(BSP_leaf_t * pleaf);