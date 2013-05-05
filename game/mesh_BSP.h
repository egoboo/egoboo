#pragma once

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

/// @file mesh_BSP.h

#include "egoboo_typedef.h"

#include "../egolib/bsp.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

struct s_ego_mesh;
struct s_frustum;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

struct s_mpd_BSP;
typedef struct s_mpd_BSP mesh_BSP_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// the BSP structure housing the mesh
struct s_mpd_BSP
{
    size_t         count;
    oct_bb_t       volume;
    BSP_tree_t     tree;
};

#define MAP_BSP_INIT \
    {\
        0,                 /* count  */ \
        OCT_BB_INIT_VALS,  /* volume */ \
        BSP_TREE_INIT_VALS /* tree   */ \
    }

mesh_BSP_t * mesh_BSP_ctor( mesh_BSP_t * pbsp, const struct s_ego_mesh * pmesh );
mesh_BSP_t * mesh_BSP_dtor( mesh_BSP_t * pbsp );
bool_t       mesh_BSP_alloc( mesh_BSP_t * pbsp );
bool_t       mesh_BSP_free( mesh_BSP_t * pbsp );
bool_t       mesh_BSP_fill( mesh_BSP_t * pbsp, const struct s_ego_mesh * pmpd );

bool_t       mesh_BSP_can_collide( BSP_leaf_t * pleaf );
bool_t       mesh_BSP_is_visible( BSP_leaf_t * pleaf );

int          mesh_BSP_collide_aabb( const mesh_BSP_t * pbsp, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
int          mesh_BSP_collide_frustum( const mesh_BSP_t * pbsp, const struct s_egolib_frustum * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern mesh_BSP_t mesh_BSP_root;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

egolib_rv mesh_BSP_system_begin( struct s_ego_mesh * pmpd );
egolib_rv mesh_BSP_system_end( void );
bool_t    mesh_BSP_system_started( void );
