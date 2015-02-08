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
	/* "mp_data/particle_trans", TRANSCOLOR */
    TX_PARTICLE_TRANS = 0,
	/* "mp_data/particle_light", INVALID_KEY */
    TX_PARTICLE_LIGHT,
	/* "mp_data/tile0",TRANSCOLOR */
    TX_TILE_0,
	/* "mp_data/tile1",TRANSCOLOR */
    TX_TILE_1,
	/* "mp_data/tile2",TRANSCOLOR */
    TX_TILE_2,
	/* "mp_data/tile3",TRANSCOLOR */
    TX_TILE_3,
	/* "mp_data/watertop", TRANSCOLOR */
    TX_WATER_TOP,
	/* "mp_data/waterlow", TRANSCOLOR */
    TX_WATER_LOW,
	/* "mp_data/phong", TRANSCOLOR */
    TX_PHONG,
	/* "mp_data/bars", INVALID_KEY vs. TRANSCOLOR */
    TX_BARS,
	/* "mp_data/blip", INVALID_KEY */
    TX_BLIP,
	/* "mp_data/plan", INVALID_KEY */
    TX_MAP,
	/* "mp_data/xpbar", TRANSCOLOR*/
    TX_XP_BAR,
	/* "mp_data/nullicon", INVALID_KEY */
    TX_ICON_NULL,           //Empty icon
    TX_FONT_BMP,            //Font bitmap
    TX_ICON_KEYB,           //Keyboard
    TX_ICON_MOUS,           //Mouse
    TX_ICON_JOYA,           //White joystick
    TX_ICON_JOYB,           //Black joystick
    TX_CURSOR,              //Mouse cursor
    TX_SKULL,               //Difficulity skull
    TX_SPECIAL_LAST
};



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//// The maximum number of textures.
#define TX_COUNT (2*(MAX_TEXTURE + MAX_ICON))

#define INVALID_TX_IDX TX_COUNT
#define INVALID_TX_REF ((TX_REF)INVALID_TX_IDX)

#define VALID_TX_RANGE(VAL) ( ((VAL)>=0) && ((VAL)<TX_COUNT) )
struct TextureManager : public _List<oglx_texture_t *, TX_COUNT>
{
	TextureManager();

	/**
	 * @brief
	 *	Get the texture manager.
	 * @return
	 *	the texture manager
	 * @pre
	 *	The texture manager must be started up.
	 * @warning
	 *	Shutting-down the texture manager will invalidate any pointers returned by calls to this method prior to shut-down.
	 */
	static TextureManager *getSingleton();
	
	/**
	 * @brief
	 *	Start-up the texture manager.
	 * @remark
	 *	If the texture manager is started-up, a call to this method is a no-op.
	 */
	static void startUp();
	/**
	 * @brief
	 *	Shut-down the texture manager.
	 * @remark
	 *	If the texture manager is not started-up, a call to this method is a no-op.
	 */
	static void shutDown();

	/**
	 * @brief
	 *	Acquire a texture index.
	 * @param itex
	 *	if this is the index of an existing texture, that texture is acquired
	 * @return
	 *	the texture index on success, #INVALID_TX_REF on failure
	 *
	 */
	TX_REF acquire(const TX_REF itex);
	/**
	 * @brief
	 *	Relinquish texture index.
	 * @param itex
	 *	the texture index
	 */
	bool relinquish(const TX_REF itex);

	/**
	 * @brief
	 *	Reload all textures.
	 */
	void reload_all();
	/**
	 * @brief
	 *	Release all textures.
	 */
	void release_all();

	TX_REF load(const char *filename, const TX_REF  itex_src, Uint32 key);
	oglx_texture_t *get_valid_ptr(const TX_REF itex);

private:
	void reset_freelist();
};
