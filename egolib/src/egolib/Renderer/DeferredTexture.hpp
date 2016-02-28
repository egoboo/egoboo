#pragma once

#include "egolib/Renderer/Texture.hpp"

namespace Ego {
/**
 * @brief
 *  A texture with lazy loading. This means the texture will not
 *  be loaded into memory before it is required for rendering.
 */
class DeferredTexture {
public:
    DeferredTexture();

    DeferredTexture(const std::string &filePath);

    std::shared_ptr<const Texture> get_ptr() const { return get(); } //TODO: remove

    std::shared_ptr<const Texture> get() const;

    void release();

    void setTextureSource(const std::string &filePath);

    /**
     * @return
     *   Get the filepath this deferred texture is pointing to
     */
    const std::string& getFilePath() const;

    operator bool() const {
        return !_filePath.empty();
    }

private:
    mutable std::shared_ptr<Texture> _texture;
    mutable std::shared_ptr<Texture> _textureHD;
    mutable bool _loaded;
    mutable bool _loadedHD;
    std::string _filePath;
};

} // namespace Ego
