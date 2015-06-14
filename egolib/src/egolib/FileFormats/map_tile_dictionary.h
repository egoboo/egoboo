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

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_tile_definition;
    typedef struct s_tile_definition tile_definition_t;

    struct s_tile_dictionary;
    typedef struct s_tile_dictionary tile_dictionary_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// fan constants
#   define MAP_FAN_VERTICES_MAX      16                         ///< Fansquare vertices
#   define MAP_FAN_TYPE_MAX          64                         ///< Number of fansquare command types
#   define MAP_FAN_MAX               4                          ///< Draw up to 4 fans
#   define MAP_FAN_ENTRIES_MAX       32                         ///< Fansquare command list size
#   define MAP_FAN_SIZE_MAX          32                         ///< Max trigs in each command

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A description of a tile type that allows some compression in the way vertices are stored in the mpd file
    struct s_tile_definition
    {
        Uint8           numvertices;                        ///< Number of vertices
        int             ref[MAP_FAN_VERTICES_MAX];          ///< encoded "position" of the vertex
        int             grid_ix[MAP_FAN_VERTICES_MAX];      ///< decoded x-index
        int             grid_iy[MAP_FAN_VERTICES_MAX];      ///< decoded y-index

        float           u[MAP_FAN_VERTICES_MAX];            ///< "horizontal" texture position
        float           v[MAP_FAN_VERTICES_MAX];            ///< "vertical" texture position

        Uint8           command_count;                      ///< Number of commands
        Uint8           command_entries[MAP_FAN_MAX];       ///< Entries in each command
        Uint16          command_verts[MAP_FAN_ENTRIES_MAX]; ///< Fansquare vertex list
    };

//--------------------------------------------------------------------------------------------

    /// A a dictionary of tile types
    struct s_tile_dictionary
    {
        bool              loaded;
        size_t            offset;

        size_t            def_count;
        tile_definition_t def_lst[MAP_FAN_TYPE_MAX];
    };

#   define TILE_DICTIONARY_INIT { false, 0, 0 }

inline tile_definition_t* TILE_DICT_PTR(tile_dictionary_t& pdict, const uint8_t type)
{
    if(!pdict.loaded || type > pdict.def_count) {
        return nullptr;
    }
    return pdict.def_lst + type;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    bool tile_dictionary_load_vfs( const char * filename, tile_dictionary_t * pdict, int max_dict_size );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern tile_dictionary_t tile_dict;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif
