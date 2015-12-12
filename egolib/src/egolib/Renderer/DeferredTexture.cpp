#include "egolib/Renderer/DeferredTexture.hpp"
#include "egolib/Graphics/TextureManager.hpp"
#include "egolib/fileutil.h"

namespace Ego {

DeferredTexture::DeferredTexture() :
    _texture(),
    _filePath(),
    _loaded(false) {
    //default ctor invalid texture
}


DeferredTexture::DeferredTexture(const std::string &filePath) :
    _texture(),
    _filePath(filePath),
    _loaded(false) {
    //Do not load texture until its needed
}

const Texture& DeferredTexture::get() {
    if (!_loaded) {
        if (_filePath.empty()) {
            throw std::logic_error("DeferredTexture::get() on nullptr texture");
        }

        _texture = TextureManager::get().getTexture(_filePath);
        _loaded = true;
    }

    return *_texture.get();
}

void DeferredTexture::release() {
    _loaded = false;
    _texture.reset();
}

void DeferredTexture::setTextureSource(const std::string &filePath) {
    // Release any old source first
    release();

    _filePath = filePath;
}

const std::string& DeferredTexture::getFilePath() const {
    return _filePath;
}

} // namespace Ego