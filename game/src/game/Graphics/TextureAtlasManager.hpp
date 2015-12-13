#pragma once

#include "IdLib/IdLib.hpp"
#include "egolib/egolib.h"

namespace Ego
{
namespace Graphics
{

class TextureAtlasManager : public Ego::Core::Singleton<TextureAtlasManager>
{
protected:
    // Befriend with the singleton to grant access to TextureAtlasManager::~TextureAtlasManager.
    using TheSingleton = Ego::Core::Singleton<TextureAtlasManager>;
    friend TheSingleton;
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

    void reinitialize();

    /// @brief Reupload all textures.
    void reupload();

    // @brief Decmiate all tiled textures of the current mesh.
    void decimate();

private:
    // decimate one tiled texture of a mesh
    void decimate(const Ego::Texture *src_tx, std::vector<std::shared_ptr<Ego::Texture>>& targetTextureList, int minification);

private:
    // the "small" textures
    std::vector<std::shared_ptr<Ego::Texture>> _smallTiles;

    // the "large" textures
    std::vector<std::shared_ptr<Ego::Texture>> _bigTiles;
};

} //namespace Graphics
} //namespace Ego
