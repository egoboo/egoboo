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

/// @file game/graphic_fan.h

#pragma once

#include "game/egoboo_typedef.h"
#include "game/graphic.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_mesh_t;
struct s_renderlist;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(MAP_CULL_RIGHT)
// this worked with the old camera
#    define MAP_REF_CULL   GL_CCW
#    define MAP_NRM_CULL   GL_CW
#else
// they had to be reversed with the new camera
#    define MAP_REF_CULL   GL_CW
#    define MAP_NRM_CULL   GL_CCW
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void animate_all_tiles( ego_mesh_t * pmesh );

gfx_rv render_fan( const ego_mesh_t *pmesh, const Uint32 fan );
gfx_rv render_hmap_fan( const ego_mesh_t *pmesh, const Uint32 itile );
gfx_rv render_water_fan( const ego_mesh_t *pmesh, const Uint32 fan, const Uint8 layer );

void animate_tiles();
