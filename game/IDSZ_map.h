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

/// @file IDSZ_map.h
/// @brief

#include "egoboo_typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
// CONSTANTS
//--------------------------------------------------------------------------------------------

#define IDSZ_NOT_FOUND           -1
#define MAX_IDSZ_MAP_SIZE         64

//--------------------------------------------------------------------------------------------
// struct s_IDSZ_node
//--------------------------------------------------------------------------------------------

/// The definition of a single IDSZ element in a IDSZ map
    struct s_IDSZ_node
    {
        IDSZ id;
        int  level;
    };
    typedef struct s_IDSZ_node IDSZ_node_t;

//--------------------------------------------------------------------------------------------
// PUBLIC FUNCTION PROTOTYPES
//--------------------------------------------------------------------------------------------

    void         idsz_map_init( IDSZ_node_t idsz_map[], const size_t idsz_map_len );
    egoboo_rv    idsz_map_add( IDSZ_node_t idsz_map[], const size_t idsz_map_len, const IDSZ idsz, const int level );

    IDSZ_node_t* idsz_map_get( const IDSZ_node_t pidsz_map[], const size_t idsz_map_len, const IDSZ idsz );
    IDSZ_node_t* idsz_map_iterate( const IDSZ_node_t pidsz_map[], const size_t idsz_map_len, int *iterator );
    egoboo_rv    idsz_map_copy( const IDSZ_node_t pcopy_from[], const size_t idsz_map_len, IDSZ_node_t pcopy_to[] );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _IDSZ_map_h
