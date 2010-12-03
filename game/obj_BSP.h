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

#include "bsp.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_chr;
struct s_prt_bundle;

struct s_mpd_BSP;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enum obj_BSP_type
{
    BSP_LEAF_NONE = -1,
    BSP_LEAF_CHR,
    BSP_LEAF_ENC,
    BSP_LEAF_PRT,
    BSP_LEAF_TILE
};

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

typedef struct s_obj_BSP obj_BSP_t;

bool_t obj_BSP_ctor( obj_BSP_t * pbsp, int dim, struct s_mpd_BSP * pmesh_bsp );
bool_t obj_BSP_dtor( obj_BSP_t * pbsp );

bool_t obj_BSP_alloc( obj_BSP_t * pbsp, int dim, int depth );
bool_t obj_BSP_free( obj_BSP_t * pbsp );

int    obj_BSP_collide( obj_BSP_t * pbsp, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern obj_BSP_t chr_BSP_root;
extern obj_BSP_t prt_BSP_root;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void obj_BSP_system_begin( struct s_mpd_BSP * pBSP );
void obj_BSP_system_end();

bool_t chr_BSP_insert( struct s_chr * pchr );
bool_t chr_BSP_fill();
bool_t chr_BSP_clear();

bool_t prt_BSP_insert( struct s_prt_bundle * pbdl_prt );
bool_t prt_BSP_fill();
bool_t prt_BSP_clear();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _obj_BSP_H