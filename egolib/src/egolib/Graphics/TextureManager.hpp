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

/// @file egolib/Graphics/TextureManager.hpp
/// @brief the texture manager.

#pragma once

#include "egolib/typedef.h"
#include "egolib/Renderer/Renderer.hpp"

namespace Ego {

struct TextureManager : public id::singleton<TextureManager> {
protected:
	friend id::default_new_functor<TextureManager>;
    friend id::default_delete_functor<TextureManager>;
protected:
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
     *  Reupload all textures.
     */
    void reupload();

    /**
     * @brief
     *  Release all textures.
     */
    void release_all();

    /**
     * @brief
     *  Request a texture from the TextureHandler. If required, this function will load the texture
     *  first. This method is thread safe, if used by another thread that is not the OpenGL context
     *  thread, then it will block until the OpenGL context thread can load the texture for us.
     *  If the texture has already been loaded (even by other threads), that texture will be cached
     *  and this function will return it immediately.
     * @param filePath
     *  File path of the texture to load
     * @return
     *  The texture loaded by this texture manager. Could be the error texture if the specified
     *  path cannot be found.
     */
    const std::shared_ptr<Texture>& getTexture(const std::string &filePath);

    void updateDeferredLoading();

private:
    std::forward_list<std::shared_ptr<Texture>> _unload;
    std::unordered_map<std::string, std::shared_ptr<Texture>> _textureCache;

    std::mutex _deferredLoadingMutex;
    std::forward_list<std::string> _requestedLoadDeferredTextures;
    std::condition_variable _notifyDeferredLoadingComplete;
};

} // namespace Ego
