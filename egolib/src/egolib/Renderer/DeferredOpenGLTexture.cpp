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

const Ego::Texture& DeferredOpenGLTexture::get()
{
    if(!_loaded) {
         if(_filePath.empty()) {
            throw std::logic_error("DeferredOpenGLTexture::get() on nullptr texture");
         }

        _texture = TextureManager::get().getTexture(_filePath);
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