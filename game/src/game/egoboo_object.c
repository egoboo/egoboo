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

namespace Ego
{

Uint32 Entities::spawnDepth = 0;
GUID Entities::nextGUID = 0;

Entity::Entity(void *child_data, bsp_type_t child_type, size_t child_index)
	: _guid(0), /// @todo should be EGO_GUID_INVALID
	  _allocated(false), on(false), turn_me_on(false), kill_me(false),
	  spawning(false), in_free_list(false), in_used_list(false),
	  // Things related to the updating of objects.
	  update_count(0), frame_count(0), _update_guid(0), /// @todo should be EGO_GUID_INVALID
	  state(State::Invalid),
	  _index(child_index) {
	_name[0] = CSTR_END;
	// Assign data to the BSP node.
	bsp_leaf.set(child_data, child_type, child_index);
}

void Entity::reset()
{
    this->_guid = 0; /// @todo Should be EGO_GUID_INVALID
    this->_allocated = false;
    this->on = false;
    this->turn_me_on = false;
    this->kill_me = false;
    this->spawning = false;
    this->in_free_list = false;
    this->in_used_list = false;
    /// Things related to the updating of objects.
    this->update_count = 0;
    this->frame_count = 0;
    this->_update_guid = 0; /// @todo Should be EGO_GUID_INVALID

    this->_name[0] = CSTR_END;
    this->state = State::Invalid;
}

Entity *Entity::dtor()
{
    this->_name[0] = CSTR_END;
    this->state = State::Invalid;
    // Assign data to the BSP node.
    this->bsp_leaf.set(nullptr, BSP_LEAF_NONE, 0);
    this->_guid = 0; /// @todo Should be EGO_GUID_INVALID
    this->_allocated = false;
    this->on = false;
    this->turn_me_on = false;
    this->kill_me = false;
    this->spawning = false;
    this->in_free_list = false;
    this->in_used_list = false;
    this->update_count = 0;
    this->frame_count = 0;
    this->_update_guid = 0; /// @todo Should be EGO_GUID_INVALID
    return this;
}

} // namespace Ego

