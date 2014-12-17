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

/// @file game/obj_BSP.h

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
    /// the number of objects in thhis obj_BSP
    int          count;

    /// the BSP of characters for character-character and character-particle interactions
    BSP_tree_t   tree;
};

#define OBJ_BSP_INIT_VALS { 0, BSP_TREE_INIT_VALS }

bool obj_BSP_ctor( obj_BSP_t * pbsp, int dim, const struct s_mpd_BSP * pmesh_bsp );
bool obj_BSP_dtor( obj_BSP_t * pbsp );

bool obj_BSP_alloc( obj_BSP_t * pbsp, int dim, int depth );
bool obj_BSP_free( obj_BSP_t * pbsp );

int obj_BSP_collide_aabb( const obj_BSP_t * pbsp, const aabb_t * paabb, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );
int obj_BSP_collide_frustum( const obj_BSP_t * pbsp, const struct s_egolib_frustum * pfrust, BSP_leaf_test_t * ptest, BSP_leaf_pary_t * colst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern obj_BSP_t chr_BSP_root;
extern obj_BSP_t prt_BSP_root;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void obj_BSP_system_begin( struct s_mpd_BSP * pBSP );
void obj_BSP_system_end( void );

bool chr_BSP_insert( struct s_chr * pchr );
bool chr_BSP_fill( void );
bool chr_BSP_clear( void );
bool chr_BSP_can_collide( BSP_leaf_t * pleaf );
bool chr_BSP_is_visible( BSP_leaf_t * pleaf );

bool prt_BSP_insert( struct s_prt_bundle * pbdl_prt );
bool prt_BSP_fill( void );
bool prt_BSP_clear( void );
bool prt_BSP_can_collide( BSP_leaf_t * pleaf );
bool prt_BSP_is_visible( BSP_leaf_t * pleaf );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _obj_BSP_H
