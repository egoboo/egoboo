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

/// @file egolib/game/graphic_fan.c
/// @brief World mesh drawing.
/// @details

#include "egolib/game/graphic_fan.h"

#include "egolib/game/graphic.h"
#include "egolib/game/game.h"
#include "egolib/game/renderer_3d.h"
#include "egolib/game/mesh.h"
#include "egolib/game/Module/Module.hpp"
#include "egolib/FileFormats/Globals.hpp"

void animate_all_tiles( ego_mesh_t& mesh )
{
    bool small_tile_update = (g_animatedTilesState.elements[0].frame_add_old != g_animatedTilesState.elements[0].frame_add);
    bool big_tile_update = (g_animatedTilesState.elements[1].frame_add_old != g_animatedTilesState.elements[1].frame_add);
    // If there are no updates, do nothing.
    if (!small_tile_update && !big_tile_update) return;
    size_t numberOfTiles = mesh._tmem.getInfo().getTileCount();
    // Scan through all the animated tiles.
    for (const Index1D& element : mesh._fxlists.anm.elements)
    {
        // If the tile is out of range, then skip this tile.
        if (element.i() >= numberOfTiles) continue;
        // Otherwise animate the tile.
        animate_tile(mesh, element);
    }
}

bool animate_tile( ego_mesh_t& mesh, const Index1D& index )
{
    /// @author BB
    /// @details animate a given tile

	// do nothing if the tile is not animated
    if ( 0 == mesh.test_fx( index, MAPFX_ANIM ) )
    {
        return true;
    }

    // grab a pointer to the tile
	ego_tile_info_t& ptile = mesh.getTileInfo(index);

    uint16_t image = TILE_GET_LOWER_BITS( ptile._img ); // Tile image
    uint8_t type  = ptile._type;                       // Command type ( index to points in itile )

    uint16_t base_and, frame_add;
    // Animate the tiles
    if ( type >= tile_dict.offset )
    {
        // Big tiles
        base_and  = g_animatedTilesState.elements[1].base_and;     // Animation set
        frame_add = g_animatedTilesState.elements[1].frame_add;    // Animated image
    }
    else
    {
        // Small tiles
        base_and  = g_animatedTilesState.elements[0].base_and;          // Animation set
        frame_add = g_animatedTilesState.elements[0].frame_add;         // Animated image
    }

    uint16_t basetile = image & base_and;
    image = frame_add + basetile;

    // actually update the animated texture info
    return mesh.set_texture( index, image );
}
