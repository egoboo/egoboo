#pragma once

#include "egolib/Renderer/Texture.hpp"

namespace Ego
{
    /**
    * @brief
    *   A OpenGL texture with lazy loading. This means the texture will not
    *   be loaded into memory before it is required for rendering
    **/
    class DeferredOpenGLTexture
    {
    public:
        DeferredOpenGLTexture();

        DeferredOpenGLTexture(const std::string &filePath);

        const oglx_texture_t& get();

        void release();

        void setTextureSource(const std::string &filePath);

    private:
        std::shared_ptr<oglx_texture_t> _texture;
        std::string _filePath;
        bool _loaded;
    };
}
