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

    const Texture* get_ptr() const { return &const_cast<DeferredTexture*>(this)->get(); }

    const Texture& get();

    void release();

    void setTextureSource(const std::string &filePath);

    // Type cast operator
    operator const Texture&() const { return const_cast<DeferredTexture*>(this)->get(); }

    /**
     * @return
     *   Get the filepath this deferred texture is pointing to
     */
    const std::string& getFilePath() const;

    operator bool() const {
        return !_filePath.empty();
    }

private:
    std::shared_ptr<Texture> _texture;
    std::string _filePath;
    bool _loaded;
};

} // namespace Ego
