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

/// @file egolib/FileFormats/map_file-v2.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/FileFormats/map_file-v2.h"

#include "egolib/log.h"
#include "egolib/strutil.h"

bool map_read_v2(vfs_FILE& file, map_t& map)
{
    // Alias.
    auto& mem = map._mem;

    // Load twist data.
    for (auto& tile : mem.tiles)
    {
        vfs_read_Uint8(&file, &tile.twist);
    }

    return true;
}

bool map_write_v2(vfs_FILE& file, const map_t& map)
{
    // Alias.
    const auto& mem = map._mem;

    // Write twist data.
    for (const auto& tile : mem.tiles)
    {
        vfs_write<Uint8>(file, tile.twist);
    }

    return true;
}