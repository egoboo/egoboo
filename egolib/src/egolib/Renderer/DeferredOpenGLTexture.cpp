#include "DeferredOpenGLTexture.hpp"
#include "egolib/Graphics/TextureManager.hpp"
#include "egolib/fileutil.h"

namespace Ego
{

DeferredOpenGLTexture::DeferredOpenGLTexture() :
    _texture(),
    _filePath(),
    _loaded(false)
{
    //default ctor invalid texture
}


DeferredOpenGLTexture::DeferredOpenGLTexture(const std::string &filePath) :
    _texture(),
    _filePath(filePath),
    _loaded(false)
{
    //Do not load texture until its needed
}

const oglx_texture_t& DeferredOpenGLTexture::get()
{
    if(!_loaded) {
         if(_filePath.empty()) {
            throw std::logic_error("DeferredOpenGLTexture::get() on nullptr texture");
         }

        std::unordered_map<std::string, std::shared_ptr<oglx_texture_t>> &textureCache = TextureManager::get().getTextureCache();

        //Not loaded yet?
        auto result = textureCache.find(_filePath);
        if(result == textureCache.end()) {
            std::shared_ptr<oglx_texture_t> loadTexture = std::make_shared<oglx_texture_t>();
            ego_texture_load_vfs(loadTexture.get(), _filePath.c_str(), TRANSCOLOR);
            textureCache[_filePath] = loadTexture;
            _texture = loadTexture;
        }

        //Get cached texture
        else {
            _texture = (*result).second;
        }

        _loaded = true;
    }

    return *_texture.get();
}

void DeferredOpenGLTexture::release()
{
    _loaded = false;
    _texture.reset();
}

void DeferredOpenGLTexture::setTextureSource(const std::string &filePath)
{
    //Release any old source first
    release();

    _filePath = filePath;
}

const std::string& DeferredOpenGLTexture::getFilePath() const
{
    return _filePath;
}

} //Ego