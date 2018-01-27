#include "egolib/game/Graphics/TextureAtlasManager.hpp"
#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/game/Module/Module.hpp"
#include "egolib/Image/SDL_Image_Extensions.h"
#include "egolib/game/graphic.h" //only for MESH_IMG_COUNT constant

namespace Ego { namespace Graphics {

TextureAtlasManager::TextureAtlasManager() :
    _smallTiles(),
    _bigTiles() {
    //ctor        
}

TextureAtlasManager::~TextureAtlasManager() {

}

std::shared_ptr<Ego::Texture> TextureAtlasManager::getSmall(int index) const {
    if (index < 0 || index >= _smallTiles.size() || index >= MESH_IMG_COUNT) {
        return nullptr;
    }
    return _smallTiles[index];
}

std::shared_ptr<Ego::Texture> TextureAtlasManager::getBig(int index) const {
    if (index < 0 || index >= _bigTiles.size() || index >= MESH_IMG_COUNT) {
        return nullptr;
    }
    return _bigTiles[index];
}

void TextureAtlasManager::decimate(const std::shared_ptr<const Ego::Texture>& sourceTexture, std::vector<std::shared_ptr<Ego::Texture>>& targetTextureList, int minification) {
    static constexpr size_t SUB_TEXTURES = 8;

    if (!sourceTexture || !sourceTexture->m_source) {
        return;
    }

    // make an alias for the texture's SDL_Surface
    auto sourceImage = sourceTexture->m_source;

    // how large a step every time through the mesh?
    float stepX = static_cast<float>(sourceImage->w) / static_cast<float>(SUB_TEXTURES),
          stepY = static_cast<float>(sourceImage->h) / static_cast<float>(SUB_TEXTURES);

    SDL_Rect rectangle;
    rectangle.w = std::ceil(stepX * minification);
    rectangle.w = std::max<uint16_t>(1, rectangle.w);
    rectangle.h = std::ceil(stepY * minification);
    rectangle.h = std::max<uint16_t>(1, rectangle.h);

    size_t ix, iy;
    float x, y;

    // scan across the src_img
    for (iy = 0, y = 0.0f; iy < SUB_TEXTURES; iy++, y += stepY) {
        rectangle.y = std::floor(y);

        for (ix = 0, x = 0.0f; ix < SUB_TEXTURES; ix++, x += stepX) {
            rectangle.x = std::floor(x);

            // Create the destination texture.
            auto targetTexture = Renderer::get().createTexture();

            // Create the destination surface.
            const auto& pfd = pixel_descriptor::get<idlib::pixel_format::R8G8B8A8>();
            auto targetImage = ImageManager::get().createImage(rectangle.w, rectangle.h, pfd);
            if (!targetImage) {
                continue;
            }

            // Copy the pixels.
            Ego::blit(sourceImage.get(), Rectangle2f(Point2f(rectangle.x, rectangle.y), Point2f(rectangle.x + rectangle.w, rectangle.y + rectangle.h)), targetImage.get());

            // upload the SDL_Surface into OpenGL
            targetTexture->load(targetImage);
            targetTextureList.push_back(targetTexture);
        }
    }
}

void TextureAtlasManager::loadTileSet() {
    //Clear any old loaded data
    _smallTiles.clear();
    _bigTiles.clear();

    // Do the "small" textures.
    for (size_t i = 0; i < 4; ++i) {
        decimate(_currentModule->getTileTexture(i), _smallTiles, 1);
    }

    // Do the "big" textures.
    for (size_t i = 0; i < 4; ++i) {
        decimate(_currentModule->getTileTexture(i), _bigTiles, 2);
    }
}

void TextureAtlasManager::reupload() {
    for (std::shared_ptr<Ego::Texture>& texture : _smallTiles) {
        auto surface = texture->m_source;
        texture->load(surface);
    }

    for (std::shared_ptr<Ego::Texture>& texture : _bigTiles) {
        auto surface = texture->m_source;
        texture->load(surface);
    }
}

} } //namespace Ego::Graphics
