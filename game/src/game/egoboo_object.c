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

namespace Ego {
	Uint32 Ego::Entities::spawnDepth = 0;
	Uint32 Ego::Entities::nextGUID = 0;
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Ego::Entity *Ego::Entity::ctor(void *child_data, bsp_type_t child_type, size_t child_index)
{
    BLANK_STRUCT_PTR(this);

    this->_name[0] = CSTR_END;
    this->state = Ego::Entity::State::Invalid;
    this->index = child_index;

    // Construct the BSP node for this entity.
    this->bsp_leaf.ctor(child_data, child_type, child_index);

    return this;
}

//--------------------------------------------------------------------------------------------
Ego::Entity *Ego::Entity::dtor()
{
    this->_name[0] = CSTR_END;
    this->state = Ego::Entity::State::Invalid;
    // Destruct the BSP node for this entity.
    this->bsp_leaf.dtor();
    BLANK_STRUCT_PTR(this);
    return this;
}
