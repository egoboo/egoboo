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

#pragma once

#include "egolib/typedef.h"

// Forward declarations.
class ego_mesh_t;

/// Data needed to specify a line-of-sight test
struct line_of_sight_info_t
{
    float x0, y0, z0;
    float x1, y1, z1;
    uint32_t stopped_by;

    ObjectRef collide_chr;
    uint32_t  collide_fx;
    int       collide_x;
    int       collide_y;

    static bool blocked(line_of_sight_info_t& self, std::shared_ptr<const ego_mesh_t> mesh);
    static bool with_mesh(line_of_sight_info_t& self, std::shared_ptr<const ego_mesh_t> mesh);
    static bool with_characters(line_of_sight_info_t& self);
};