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
Ego::Entity *Ego::Entity::ctor(Ego::Entity *self, void *child_data, bsp_type_t child_type, size_t child_index)
{
	if (NULL == self)
	{
		return self;
	}
    BLANK_STRUCT_PTR(self)

    self->_name[0] = CSTR_END;
    self->state = Ego::Entity::State::Invalid;
    self->index = child_index;

    // Construct the BSP node for this character.
    BSP_leaf_t::ctor(&(self->bsp_leaf), child_data, child_type, child_index);

    return self;
}

//--------------------------------------------------------------------------------------------
Ego::Entity *Ego::Entity::dtor(Ego::Entity *self)
{
	if (NULL == self)
	{
		return self;
	}
    self->_name[0] = CSTR_END;
    self->state = Ego::Entity::State::Invalid;
    // Destruct the BSP node for this character.
    BSP_leaf_t::dtor(&(self->bsp_leaf));
	BLANK_STRUCT_PTR(self)
	return self;
}
