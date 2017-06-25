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

/// @file egolib/map_functions.h
/// @brief Definitions for mpd functionality ported from cartman and EgoMap
/// @details

#pragma once

#include "egolib/Math/_Include.hpp"

struct map_t;

    void map_generate_tile_twist_data(map_t& map);
    void map_generate_fan_type_data(map_t& map);
bool twist_to_normal(uint8_t twist, Vector3f& v, float slide);
uint8_t  cartman_calc_twist( int dx, int dy );
