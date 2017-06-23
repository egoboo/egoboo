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

/// @file egolib/FileFormats/map_file-v1.c
/// @brief Functions for raw read and write access to the .mpd file type
/// @details

#include "egolib/FileFormats/map_file-v1.h"

#include "egolib/Log/_Include.hpp"
#include "egolib/strutil.h"

bool map_read_v1(vfs_FILE& file, map_t& map)
{
    // Alias.
    auto& mem = map._mem;

    // Load tile data.
    for (auto& tile : mem.tiles)
    {
        uint32_t ui32_tmp;
        vfs_read_Uint32(file, &ui32_tmp);

        tile.type = Ego::Math::clipBits<8>( ui32_tmp >> 24 );
        tile.fx   = Ego::Math::clipBits<8>( ui32_tmp >> 16 );
        tile.img  = Ego::Math::clipBits<16>( ui32_tmp >>  0 );
    }

    return true;
}

bool map_write_v1(vfs_FILE& file, const map_t& map)
{
    // Alias.
    const auto& mem = map._mem;

    // Save tile data.
    for (const auto& tile : mem.tiles)
    {
        uint32_t ui32_tmp;
        ui32_tmp  = Ego::Math::clipBits<16>( tile.img ) <<  0;
        ui32_tmp |= Ego::Math::clipBits<8>( tile.fx ) << 16;
        ui32_tmp |= Ego::Math::clipBits<8>( tile.type ) << 24;

        vfs_write<uint32_t>(file, ui32_tmp);
    }

    return true;
}
