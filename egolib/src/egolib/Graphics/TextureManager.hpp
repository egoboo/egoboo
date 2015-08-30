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


public:

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

    /**
    * @brief
    *   Request a texture from the TextureHandler. If required, this function will load the texture
    *   first. This method is thread safe, if used by another thread that is not the OpenGL context
    *   thread, then it will block until the OpenGL context thread can load the texture for us.
    *   If the texture has already been loaded (even by other threads), that texture will be cached
    *   and this function will return it immediately.
    * @param filePath
    *   File path of the texture to load
    * @return
    *   The oglx_texture_t loaded by this TextureManager. Could be the error texture if the specified
    *   path cannot be found. 
    **/
    const std::shared_ptr<oglx_texture_t>& getTexture(const std::string &filePath);

    void updateDeferredLoading();

private:
	std::forward_list<std::shared_ptr<oglx_texture_t>> _unload;
    std::unordered_map<std::string, std::shared_ptr<oglx_texture_t>> _textureCache;

    std::mutex _deferredLoadingMutex;
    std::forward_list<std::string> _requestedLoadDeferredTextures;
    std::condition_variable _notifyDeferredLoadingComplete;
};
