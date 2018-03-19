#pragma once

#include "idlib/singleton.hpp"
#include <memory>
#include <vector>

namespace Ego { class Texture; }

namespace Ego::Graphics {

class TextureAtlasManager : public idlib::singleton<TextureAtlasManager> {
protected:
    friend idlib::default_new_functor<TextureAtlasManager>;
    friend idlib::default_delete_functor<TextureAtlasManager>;
    /**
     * @brief Construct this texture atlas manager.
     */
    TextureAtlasManager();
    /**
     * @brief Destruct this texture atlas manager.
     */
    virtual ~TextureAtlasManager();

public:
    std::shared_ptr<Ego::Texture> getSmall(int which) const;

    std::shared_ptr<Ego::Texture> getBig(int which) const;


    /// @brief Reupload all textures.
    void reupload();

    /**
     * @brief
     *  Decmiate all tiled textures of the current mesh.
     *  This turns a big texture tilemap into many smaller textures
     *  for each tile type (tile0.bmp, tile1.bmp etc.)
     */
    void loadTileSet();

private:
    // decimate one tiled texture of a mesh
    void decimate(const std::shared_ptr<const Ego::Texture>& src_tx, std::vector<std::shared_ptr<Ego::Texture>>& targetTextureList, int minification);

private:
    // the "small" textures
    std::vector<std::shared_ptr<Ego::Texture>> _smallTiles;

    // the "large" textures
    std::vector<std::shared_ptr<Ego::Texture>> _bigTiles;
};

} //namespace Ego::Graphics
