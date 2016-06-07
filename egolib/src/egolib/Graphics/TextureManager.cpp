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

/// @file  egolib/Graphics/TextureManager.cpp
/// @brief the texture manager.

#include "egolib/_math.h"
#include "egolib/fileutil.h"
#include "egolib/Graphics/TextureManager.hpp"
#include "egolib/Image/ImageManager.hpp"

/**
 * @brief
 *  Load an image into a texture.
 * @param [out] texture
 *  the texture to load the image in
 * @param filename
 *  the filename of the image <em>without</em> extension.
 * @param key
 *  ?
 * @post
 *  the texture is released and - if loading succeeds - the loaded with a new image.
 *  The filenames this function considers are all combinations of the specified
 *  filename concatenated with supported file extensions until one combination
 *  succeeds (i.e. the image was successfully loaded into the texture) or all
 *  combinations failed.
 */
static bool ego_texture_load_vfs(std::shared_ptr<Ego::Texture> texture, const char *filename);

static bool ego_texture_load_vfs(std::shared_ptr<Ego::Texture> texture, const char *filename) {
    // Get rid of any old data.
    texture->release();

    // Load the image.
    bool retval = false;

    // Try all different formats.
    for (const auto& loader : Ego::ImageManager::get()) {
        for (const auto& extension : loader.getExtensions()) {
            texture->release();
            // Build the full file name.
            std::string fullFilename = filename + extension;
            // Open the file.
            vfs_FILE *file = vfs_openRead(fullFilename);
            if (!file) {
                continue;
            }
            // Stream the surface.
            std::shared_ptr<SDL_Surface> surface = nullptr;
            try {
                surface = loader.load(file);
            } catch (...) {
                vfs_close(file);
                continue;
            }
            vfs_close(file);
            if (!surface) {
                continue;
            }
            // Create the texture from the surface.
            retval = texture->load(fullFilename.c_str(), surface);
            if (retval) {
                goto End;
            }
        }
    }
End:
    if (!retval) {
        Log::get().warn("unable to load texture: %s\n", vfs_resolveReadFilename(filename));
    }

    return retval;
}


//--------------------------------------------------------------------------------------------

namespace Ego {
TextureManager::TextureManager() :
    _deferredLoadingMutex(),
    _requestedLoadDeferredTextures(),
    _notifyDeferredLoadingComplete() {
    Ego::OpenGL::initializeErrorTextures();
}

TextureManager::~TextureManager() {
    _textureCache.clear();
    _unload.clear();
    Ego::OpenGL::uninitializeErrorTextures();
}

void TextureManager::release_all() {
    if (SDL_GL_GetCurrentContext() != nullptr) {
        // We are the main OpenGL context thread so we can destroy textures.
        _textureCache.clear();
        _unload.clear();
    } else {
        // We are not the main OpenGL context thread so we can not destroy textures.
        for (auto it = std::begin(_textureCache); it != std::end(_textureCache);) {
            _unload.push_front(it->second);
            it = _textureCache.erase(it);
        }
    }
}

void TextureManager::reupload() {
    // TODO
}

void TextureManager::updateDeferredLoading() {
    //If nothing to do, exit function immeadiately
    if (_requestedLoadDeferredTextures.empty()) return;

    //Load each texture that is required by another thread
    {
        std::lock_guard<std::mutex> lock(_deferredLoadingMutex);
        for (const std::string &filePath : _requestedLoadDeferredTextures) {
            std::shared_ptr<Ego::Texture> loadTexture = std::make_shared<Ego::OpenGL::Texture>();
            ego_texture_load_vfs(loadTexture, filePath.c_str());
            _textureCache[filePath] = loadTexture;
            //Log::get().debug("Deferred texture load: %s\n", filePath.c_str());
        }
        _requestedLoadDeferredTextures.clear();
    }

    //Notify all waiting threads that loading is complete
    _notifyDeferredLoadingComplete.notify_all();
}

const std::shared_ptr<Texture>& TextureManager::getTexture(const std::string &filePath) {
    //Not loaded yet?
    const auto &result = _textureCache.find(filePath);
    if (result == _textureCache.end()) {

        if (SDL_GL_GetCurrentContext() != nullptr) {
            //We are the main OpenGL context thread so we can load textures
            std::shared_ptr<Texture> loadTexture = std::make_shared<OpenGL::Texture>();
            ego_texture_load_vfs(loadTexture, filePath.c_str());
            _textureCache[filePath] = loadTexture;
        } else {
            //We cannot load textures, wait blocking for main thread to load it for us
            std::unique_lock<std::mutex> lock(_deferredLoadingMutex);
            _requestedLoadDeferredTextures.push_front(filePath);
            //Log::get().debug("Wait for deferred texture: %s\n", filePath.c_str());
            _notifyDeferredLoadingComplete.wait(lock, [this] {return _requestedLoadDeferredTextures.empty(); });
        }

        return _textureCache[filePath];
    }

    //Get cached texture
    else {
        return result->second;
    }
}

} // namespace Ego
