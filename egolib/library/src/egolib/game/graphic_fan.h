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

/// @file egolib/game/graphic_fan.h

#pragma once

#include "egolib/game/egoboo.h"
#include "egolib/game/graphic.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

class ego_mesh_t;
namespace Ego::Graphics
{
	struct TileList;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(MAP_CULL_RIGHT)
// this worked with the old camera
#    define MAP_REF_CULL   idlib::winding_mode::anti_clockwise
#    define MAP_NRM_CULL   idlib::winding_mode::clockwise
#else
// they had to be reversed with the new camera
#    define MAP_REF_CULL   idlib::winding_mode::clockwise
#    define MAP_NRM_CULL   idlib::winding_mode::anti_clockwise
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void animate_all_tiles(ego_mesh_t& mesh);
bool animate_tile(ego_mesh_t& mesh, const Index1D& index);
