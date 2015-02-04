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

/// @file game/graphic_texture.h

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Special Textures
enum e_global_tx_type
{
    TX_PARTICLE_TRANS = 0,
    TX_PARTICLE_LIGHT,
    TX_TILE_0,
    TX_TILE_1,
    TX_TILE_2,
    TX_TILE_3,
    TX_WATER_TOP,
    TX_WATER_LOW,
    TX_PHONG,
    TX_BARS,
    TX_BLIP,
    TX_MAP,
    TX_XP_BAR,
    TX_ICON_NULL,
    TX_SPECIAL_LAST
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// Color index of the transparent color in an 8-bit image,
/// or the rgb components of the transparent color in a 24-bit image.
/// Ignored in a 32 bit image.

#define TX_COUNT   (2*(MAX_TEXTURE + MAX_ICON))

#define INVALID_TX_IDX TX_COUNT
#define INVALID_TX_REF ((TX_REF)INVALID_TX_IDX)

#define VALID_TX_RANGE(VAL) ( ((VAL)>=0) && ((VAL)<TX_COUNT) )
struct TxListTy : public _List<oglx_texture_t *, TX_COUNT> {
	TxListTy() {
		update_guid = INVALID_UPDATE_GUID;
		used_count = 0;
		free_count = TX_COUNT;
		for (size_t i = 0, j = TX_SPECIAL_LAST; j < TX_COUNT; i++, j++)
		{
			free_ref[i] = j;
		}
		for (TX_REF i = 0; i < TX_COUNT; i++)
		{
			lst[i] = nullptr;
		}
	}
};
/// declare special arrays of textures
extern TxListTy TxList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void TxList_init_all();
void TxList_delete_all();
void TxList_release_all();
TX_REF TxList_get_free(const TX_REF itex);
bool TxList_free_one(const TX_REF  itex);
TX_REF TxList_load_one_vfs(const char *filename, const TX_REF  itex_src, Uint32 key);
oglx_texture_t * TxList_get_valid_ptr(const TX_REF itex);
void TxList_reload_all();
