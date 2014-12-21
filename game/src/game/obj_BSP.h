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

#include "egolib/bsp.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

struct s_chr;
struct s_prt_bundle;
struct s_egolib_frustum;
struct s_mpd_BSP;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

struct s_obj_BSP;
typedef struct s_obj_BSP obj_BSP_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the BSP structure housing the object
struct s_obj_BSP
{
    /// the number of objects in this obj_BSP.
	/// @todo This should be unsigned.
    int count;

    /// the BSP of characters for character-character and character-particle interactions
    BSP_tree_t   tree;
};

/** @todo Remove this. obj_BSP_ctor must be used. */
#define OBJ_BSP_INIT_VALS { 0, BSP_TREE_INIT_VALS }

bool obj_BSP_ctor( obj_BSP_t * pbsp, int dim, const struct s_mpd_BSP * pmesh_bsp );
bool obj_BSP_dtor( obj_BSP_t * pbsp );

bool obj_BSP_alloc( obj_BSP_t * pbsp, int dim, int depth );
bool obj_BSP_free( obj_BSP_t * pbsp );

int obj_BSP_collide_aabb( const obj_BSP_t * pbsp, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
int obj_BSP_collide_frustum( const obj_BSP_t * pbsp, const egolib_frustum_t * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );