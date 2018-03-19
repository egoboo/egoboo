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
#pragma once

#include "egolib/Math/Math.hpp"

#define GRIP_VERTS             4

/// Where an item is being held
enum slot_t : uint8_t
{
    SLOT_LEFT  = 0,
    SLOT_RIGHT,
    SLOT_COUNT
};

/// The vertex offsets for the various grips
enum grip_offset_t : uint8_t
{
    GRIP_ORIGIN    =               0,                ///< Spawn attachments at the center
    GRIP_LAST      =               1,                ///< Spawn particles at the last vertex
    GRIP_LEFT      = ( 1 * GRIP_VERTS ),             ///< Left weapon grip starts  4 from last
    GRIP_RIGHT     = ( 2 * GRIP_VERTS ),             ///< Right weapon grip starts 8 from last

    // aliases
    GRIP_INVENTORY =               GRIP_ORIGIN,
    GRIP_ONLY      =               GRIP_LEFT
};

/**
* @brief
*   Converts a slot position to a grip offset
**/
inline grip_offset_t slot_to_grip_offset( slot_t slot )
{
    return static_cast<grip_offset_t>((slot+1) * GRIP_VERTS);
}

/**
* @brief
*   Converts a grip offset to an Object slot position
**/
inline slot_t grip_offset_to_slot( grip_offset_t grip_off )
{
    if ( 0 != grip_off % GRIP_VERTS )
    {
        // this does not correspond to a valid slot
        // coerce it to the "default" slot
        return SLOT_LEFT;
    }
    else
    {
        // coerce the slot number to fit within the valid range
        int islot = Ego::Math::constrain<int>((static_cast<int>(grip_off) / GRIP_VERTS)-1, 0, SLOT_COUNT);

        return static_cast<slot_t>(islot);
    }
}
