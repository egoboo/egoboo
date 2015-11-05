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

/// @file egolib/FileFormats/map_tile_dictionary.h
/// @brief The tile definitions
/// @details

#pragma once

#include "egolib/typedef.h"
#include "egolib/FileFormats/map_fx.hpp"

//--------------------------------------------------------------------------------------------

/// A description of a tile type that allows some compression in the way vertices are stored in the mpd file
    struct tile_definition_t
    {
        tile_definition_t() :
            numvertices(0),
            ref(),
            grid_ix(),
            grid_iy(),
            u(),
            v(),
            command_count(0),
            command_entries(),
            command_verts()
        {
            ref.fill(0);
            grid_ix.fill(0);
            grid_iy.fill(0);
            u.fill(0);
            v.fill(0);
            command_entries.fill(0);
            command_verts.fill(0);
        }

        uint8_t         numvertices;                        ///< Number of vertices
        std::array<int, MAP_FAN_VERTICES_MAX> ref;          ///< encoded "position" of the vertex
        std::array<int, MAP_FAN_VERTICES_MAX> grid_ix;      ///< decoded x-index
        std::array<int, MAP_FAN_VERTICES_MAX> grid_iy;      ///< decoded y-index
        std::array<float, MAP_FAN_VERTICES_MAX> u;          ///< "horizontal" texture position
        std::array<float, MAP_FAN_VERTICES_MAX> v;          ///< "vertical" texture position

        uint8_t         command_count;                      ///< Number of commands
        std::array<uint8_t, MAP_FAN_MAX> command_entries;       ///< Entries in each command
        std::array<uint16_t, MAP_FAN_ENTRIES_MAX> command_verts; ///< Fansquare vertex list
    };

//--------------------------------------------------------------------------------------------

    /// A a dictionary of tile types
    struct tile_dictionary_t
    {
        tile_dictionary_t() :
            loaded(false),
            offset(0),
            def_count(0),
            def_lst()
        {
            //ctor
        }

        bool              loaded;
        size_t            offset;

        size_t            def_count;
        std::array<tile_definition_t, MAP_FAN_TYPE_MAX> def_lst;
    };

inline tile_definition_t* TILE_DICT_PTR(tile_dictionary_t& pdict, const uint8_t type)
{
    if(!pdict.loaded || type > pdict.def_count) {
        return nullptr;
    }
    return &pdict.def_lst[type];
}

//--------------------------------------------------------------------------------------------

bool tile_dictionary_load_vfs( const char * filename, tile_dictionary_t * pdict, int max_dict_size );
