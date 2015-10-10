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

/// @file egolib/FileFormats/map_file-v4.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/FileFormats/map_file-v4.h"

#include "egolib/log.h"
#include "egolib/strutil.h"

bool map_read_v4(vfs_FILE& file, map_t& map)
{
    // Alias.
    auto& mem = map._mem;

    // Load vertex a data
    for (map_vertex_t& vertex : mem.vertices)
    {
        vfs_read_Uint8(&file, &vertex.a);
    }

    return true;
}

bool map_write_v4(vfs_FILE& file, const map_t& map)
{
    const auto& mem = map._mem;

    for (const auto& vertex : mem.vertices)
    {
        vfs_write_Uint8(&file, vertex.a);
    }

    return true;
}
