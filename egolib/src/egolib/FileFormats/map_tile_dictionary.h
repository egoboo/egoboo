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
        struct vertex_t
        {
            int ref;          ///< encoded x-/y-index of the vertex
            int grid_ix;      ///< decoded x-index
            int grid_iy;      ///< decoded y-index

            float u;            ///< "horizontal" texture position
            float v;            ///< "vertical" texture position
        };
        Uint8           numvertices;                    ///< The number of vertices
        vertex_t        vertices[MAP_FAN_VERTICES_MAX]; ///< The vertices.

        Uint8           command_count;                      ///< Number of commands
        Uint8           command_entries[MAP_FAN_MAX];       ///< Entries in each command
        Uint16          command_verts[MAP_FAN_ENTRIES_MAX]; ///< Fansquare vertex list
    };

//--------------------------------------------------------------------------------------------

    /// A a dictionary of tile types
    struct tile_dictionary_t
    {
        bool              loaded;
        size_t            offset;

        size_t            def_count;
        tile_definition_t def_lst[MAP_FAN_TYPE_MAX];

		inline const tile_definition_t* get(const uint8_t type) const
		{
			if (!loaded || type > def_count) {
				return nullptr;
			}
			return &(def_lst[type]);
		}

		inline tile_definition_t* get(const uint8_t type)
		{
			if (!loaded || type > def_count) {
				return nullptr;
			}
			return &(def_lst[type]);
		}
    };

//--------------------------------------------------------------------------------------------

bool tile_dictionary_load_vfs( const std::string& filename, tile_dictionary_t& dict, int max_dict_size );
