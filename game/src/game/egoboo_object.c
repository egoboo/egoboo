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

/// @file game/egoboo_object.c
/// @brief Implementation of Egoboo "object" control routines
/// @details

#include "game/egoboo_object.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint32 ego_object_spawn_depth = 0;
Uint32 ego_object_guid = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Ego::Entity *ego_object_ctor(Ego::Entity *pbase, void *child_data, int child_type, size_t child_index)
{
    if ( NULL == pbase ) return pbase;

    BLANK_STRUCT_PTR( pbase )

    pbase->_name[0] = CSTR_END;
    pbase->state    = Ego::Entity::State::Invalid;
    pbase->index    = child_index;

    // initialize the bsp node for this character
    BSP_leaf_ctor( &( pbase->bsp_leaf ), child_data, child_type, child_index );

    return pbase;
}

//--------------------------------------------------------------------------------------------
Ego::Entity *ego_object_dtor(Ego::Entity *pbase)
{
    if ( NULL == pbase ) return pbase;

    BLANK_STRUCT_PTR( pbase )

    pbase->_name[0] = CSTR_END;
    pbase->state    = Ego::Entity::State::Invalid;

    // initialize the bsp node for this character
    BSP_leaf_dtor( &( pbase->bsp_leaf ) );

    return pbase;
}
