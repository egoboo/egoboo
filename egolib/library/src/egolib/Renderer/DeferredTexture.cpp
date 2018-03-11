#include "egolib/Renderer/DeferredTexture.hpp"
#include "egolib/Graphics/TextureManager.hpp"
#include "egolib/fileutil.h"

namespace Ego {

DeferredTexture::DeferredTexture() :
    _texture(nullptr),
    _textureHD(nullptr),
    _loaded(false),
    _loadedHD(false),
    _filePath() {
    //default ctor invalid texture
}


DeferredTexture::DeferredTexture(const std::string &filePath) :
    _texture(nullptr),
    _textureHD(nullptr),
    _loaded(false),
    _loadedHD(false),
    _filePath(filePath) {
    //Do not load texture until its needed
}

std::shared_ptr<const Texture> DeferredTexture::get() const {
    if (!_loaded) {
        if (_filePath.empty()) {
            throw std::logic_error("DeferredTexture::get() on nullptr texture");
        }

        _texture = TextureManager::get().getTexture(_filePath);
        _loaded = true;
    }

    //Load and use the optional HD texture if it is available (else fall back to normal texture)
    if(egoboo_config_t::get().graphic_hd_textures_enable.getValue()) {

        if(!_loadedHD) {
            if(ego_texture_exists_vfs(_filePath + "_HD")) {
                _textureHD = TextureManager::get().getTexture(_filePath + "_HD");
            } 

            _loadedHD = true;            
        }

        if(_textureHD != nullptr) {
            return _textureHD; //Oh yeah HD!
        }
    }

    return _texture;
}

void DeferredTexture::release() {
    _loaded = false;
    _loadedHD = false;
    _texture.reset();
    _textureHD.reset();
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