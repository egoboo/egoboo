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
struct mesh_BSP_t
{
    size_t         count;
    oct_bb_t       volume;
    BSP_tree_t     tree;
};

#if 0
#define MESH_BSP_INIT \
    {\
        0,                 /* count  */ \
        OCT_BB_INIT_VALS,  /* volume */ \
        BSP_TREE_INIT_VALS /* tree   */ \
    }
#endif

/** @todo Should return @a bool. */
mesh_BSP_t *mesh_BSP_ctor(mesh_BSP_t *self, const ego_mesh_t *mesh);
/** @todo Should return @a void. */
mesh_BSP_t *mesh_BSP_dtor(mesh_BSP_t *self);
 
/**
 * @brief
 *	Create a new mesh BSP for a mesh.
 * @param mesh
 *	the mesh used in initialization
 * @return
 *	the mesh BSP on success, @a NULL failure
 * @author
 *	BB
 * @author
 *	MH
 * @details
 *	These parameters duplicate the max resolution of the old system.
 */
mesh_BSP_t *mesh_BSP_new(const ego_mesh_t *mesh);

/**
 * @brief
 *	Delete a mesh BSP.
 * @param self
 *	the mesh BSP
 */
void mesh_BSP_delete(mesh_BSP_t *mesh_bsp);

bool        mesh_BSP_alloc(mesh_BSP_t *self);
bool        mesh_BSP_free(mesh_BSP_t *self);
bool        mesh_BSP_fill(mesh_BSP_t *self, const ego_mesh_t *mesh);

bool        mesh_BSP_can_collide( BSP_leaf_t * pleaf );
bool        mesh_BSP_is_visible( BSP_leaf_t * pleaf );

int         mesh_BSP_collide_aabb( const mesh_BSP_t * pbsp, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
int         mesh_BSP_collide_frustum( const mesh_BSP_t * pbsp, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );