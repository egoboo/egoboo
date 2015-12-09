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

        const Ego::Texture* get_ptr() const { return &const_cast<DeferredOpenGLTexture*>(this)->get(); }

        const Ego::Texture& get();

        void release();

        void setTextureSource(const std::string &filePath);

        //Type cast operator
        operator const Ego::Texture&() const {return const_cast<DeferredOpenGLTexture*>(this)->get();}

        /**
        * @return
        *   Get the filepath this deferred texture is pointing to
        **/
        const std::string& getFilePath() const;

        operator bool() const
        {
            return !_filePath.empty();
        }

    private:
        std::shared_ptr<Ego::Texture> _texture;
        std::string _filePath;
        bool _loaded;
    };
}
