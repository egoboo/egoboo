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

#include "game/mesh_BSP.h"

//Forward declarations
namespace Ego {class Particle;}

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

bool prt_BSP_can_collide(const std::shared_ptr<Ego::Particle> &pprt);
bool chr_BSP_can_collide(const std::shared_ptr<Object> &pobj);
