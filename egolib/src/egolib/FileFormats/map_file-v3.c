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

/// @file egolib/FileFormats/map_file-v3.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/FileFormats/map_file-v3.h"

#include "egolib/log.h"
#include "egolib/strutil.h"

bool map_read_v3(vfs_FILE *file, map_t *map)
{
    // Validate arguments.
    if (!map || !file)
    {
        return nullptr;
    }

    // Alias.
    auto& mem = map->_mem;

    // Load the x-coordinate of each vertex.
    for (auto& vertex : mem.vertices)
    {
        float ieee32_tmp;
        vfs_read_float(file, &ieee32_tmp);
        vertex.pos[kX] = ieee32_tmp;
    }

    // Load the y-coordinate of each vertex.
    for (auto& vertex : mem.vertices)
    {
        float ieee32_tmp;
        vfs_read_float(file, &ieee32_tmp);
        vertex.pos[kY] = ieee32_tmp;
    }

    // Load the z-coordinate of each vertex.
    for (auto& vertex : mem.vertices)
    {
        float ieee32_tmp;
        vfs_read_float(file, &ieee32_tmp);
        // Cartman scales the z-axis based off of a 4 bit fixed precision number.
        vertex.pos[kZ] = ieee32_tmp / 16.0f;
    }

    return true;
}

bool map_write_v3(vfs_FILE *file, const map_t *map)
{
    // Validate arguments.
    if (!map || !file)
    {
        return false;
    }

    // Alias.
    const auto& mem  = map->_mem;

    // Write the x-coordinate of each vertex.
    for (const auto& vertex : mem.vertices)
    {
        vfs_write_float(file, vertex.pos[kX]);
    }

    // Write the y-coordinate of each vertex.
    for (const auto& vertex : mem.vertices)
    {
        vfs_write_float(file, vertex.pos[kY]);
    }

    // Write the y-coordinate of each vertex.
    for (const auto& vertex : mem.vertices)
    {
        // Cartman scales the z-axis based off of a 4 bit fixed precision number.
        vfs_write_float(file, vertex.pos[kZ] * 16.0f);
    }

    return true;
}