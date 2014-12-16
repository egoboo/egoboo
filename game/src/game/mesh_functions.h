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

/// @file game/map_functions.h
/// @brief Definitions for mpd functionality ported from cartman and EgoMap
///
/// @details

#include "egolib/map_functions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ego_mesh;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CARTMAN_SLOPE             50                        ///< increments for terrain slope

//--------------------------------------------------------------------------------------------
// Translated Cartman functions
//--------------------------------------------------------------------------------------------

Uint8 cartman_get_fan_twist( const struct s_ego_mesh * pmesh, Uint32 tile );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _map_functions_h
