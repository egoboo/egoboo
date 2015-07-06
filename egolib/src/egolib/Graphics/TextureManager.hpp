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

/// @file  egolib/Graphics/TextureManager.hpp
/// @brief the texture manager.

#pragma once

#include "egolib/typedef.h"
#include "egolib/Renderer/Renderer.hpp"

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
    /* "mp_data/blip" */
    TX_BLIP,
    /* "mp_data/plan" */
    TX_MAP,
    /* "mp_data/xpbar", TRANSCOLOR*/
    TX_XP_BAR,
    /* "mp_data/nullicon" */
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

inline bool VALID_TX_RANGE(const TX_REF ref)
{
    return ref < TEXTURES_MAX;
}

struct TextureManager : public Ego::Core::Singleton <TextureManager>
{
protected:
    // Befriend with the singleton to grant access to TextureManager::~TextureManager.
    using TheSingleton = Ego::Core::Singleton<TextureManager>;
    friend TheSingleton;

    /**
     * @brief
     *  The list of texture objects.
     */
    oglx_texture_t *_lst[TEXTURES_MAX];

    /**
     * @brief
     *  The set of free texture references.
     */
    std::unordered_set<TX_REF> _free;

    /**
     * @brief
     *  Construct this texture manager.
     * @remark
     *  Intentionally protected.
     */
    TextureManager();

    /**
     * @brief
     *  Destruct this texture manager.
     * @remark
     *  Intentionally protected.
     */
    virtual ~TextureManager();

    /**
     * @brief
     *  Mark all textures as free.
     */
    void freeAll();

public:

    /**
     * @brief
     *  Acquire a texture reference.
     * @param ref
     *  if not equal to #INVALID_TX_REF, this texture reference is acquired
     * @return
     *  the texture reference on success, #INVALID_TX_REF on failure
     */
    TX_REF acquire(const TX_REF ref);

    /**
     * @brief
     *  Relinquish texture reference.
     * @param ref
     *  the texture reference
     */
    bool relinquish(const TX_REF ref);

    /**
     * @brief
     *	Reload all textures from their surfaces.
     */
    void reload_all();

    /**
     * @brief
     *	Release all textures.
     */
    void release_all();

    TX_REF load(const char *filename, const TX_REF ref, Uint32 key = INVALID_KEY);
    oglx_texture_t *get_valid_ptr(const TX_REF ref);

    inline std::unordered_map<std::string, std::shared_ptr<oglx_texture_t>>& getTextureCache() { return _textureCache; }

private:
    std::unordered_map<std::string, std::shared_ptr<oglx_texture_t>> _textureCache;
};
