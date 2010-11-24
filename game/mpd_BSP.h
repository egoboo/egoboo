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

struct s_ego_mpd;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// the BSP structure housing the mesh
struct s_mpd_BSP
{
    oct_bb_t       volume;
    BSP_leaf_ary_t nodes;
    BSP_tree_t     tree;
};
typedef struct s_mpd_BSP mpd_BSP_t;

mpd_BSP_t * mpd_BSP_ctor( mpd_BSP_t * pbsp, struct s_ego_mpd * pmesh );
mpd_BSP_t * mpd_BSP_dtor( mpd_BSP_t * );
bool_t      mpd_BSP_alloc( mpd_BSP_t * pbsp, struct s_ego_mpd * pmesh );
bool_t      mpd_BSP_free( mpd_BSP_t * pbsp );

bool_t      mpd_BSP_fill( mpd_BSP_t * pbsp );

int         mpd_BSP_collide( mpd_BSP_t * pbsp, BSP_aabb_t * paabb, BSP_leaf_pary_t * colst );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern mpd_BSP_t mpd_BSP_root;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void mpd_BSP_system_begin( struct s_ego_mpd * pmpd );
void mpd_BSP_system_end();
