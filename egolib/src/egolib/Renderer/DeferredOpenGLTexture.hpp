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

        oglx_texture_t* get_ptr() { return const_cast<oglx_texture_t*>(&const_cast<DeferredOpenGLTexture*>(this)->get()); }
        const oglx_texture_t* get_ptr() const { return &const_cast<DeferredOpenGLTexture*>(this)->get(); }

        const oglx_texture_t& get();

        void release();

        void setTextureSource(const std::string &filePath);

        //Type cast operator
        operator const oglx_texture_t&() const {return const_cast<DeferredOpenGLTexture*>(this)->get();}

        /**
        * @return
        *   Get the filepath this deferred texture is pointing to
        **/
        const std::string& getFilePath() const;

    private:
        std::shared_ptr<oglx_texture_t> _texture;
        std::string _filePath;
        bool _loaded;
    };
}
