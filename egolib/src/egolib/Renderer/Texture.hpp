//********************************************************************************************
//*
//*    This file is part of the opengl extensions library. This library is
//*    distributed with Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file   egolib/Renderer/Texture.hpp
/// @brief  Common interface of all texture object encapsulations.
/// @author Michael Heilmann

#pragma once

#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_extensions.h"
#include "egolib/Renderer/TextureFilter.hpp"
#include "egolib/Renderer/TextureAddressMode.hpp"
#include "egolib/Renderer/TextureType.hpp"

#include "egolib/typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define TRANSCOLOR 0
#define INVALID_KEY ((Uint32)(~0))

namespace Ego
{

class Texture
{

protected:

    /**
     * @brief
     *  The texture type.
     */
    Ego::TextureType _type;

    /**
     * @brief
     *  The texture address mode along the s-axis.
     */
    Ego::TextureAddressMode _addressModeS;

    /**
     * @brief
     *  The texture address mode along the t-axis.
     */
    Ego::TextureAddressMode _addressModeT;

    /**
     * @brief
     *  The name of the texture.
     * @remark
     *  Usually the pathname of the texture descriptor file,
     *  for dynamic textures and special textures a name
     *  that can not be used as a pathname.
     */
    std::string _name;

    /**
     * @brief
     *  The width, in pixels, of the source of this texture.
     * @remark
     *  This value might differ for technical reasons from the width of the texture.
     */
    int _sourceWidth;

    /**
     * @brief
     *  The height, in pixels, of the source of this texture.
     * @remark
     *  This value might differ for technical reasons from the height of the texture.
     */
    int _sourceHeight;

    /**
     * @brief
     *  The width, in pixels, of the the texture.
     */
    int _width;

    /**
     * @brief
     *  The height, in pixels, of the texture.
     */
    int _height;

    /**
     * @brief
     *  @a true if this texture has an alpha component, @a false otherwise.
     */
    bool _hasAlpha;

public:

    /**
     * @brief
     *  A pointer to the source of the texture if available, a null pointer otherwise.
     */
    std::shared_ptr<SDL_Surface> _source;

public:

    /**
     * @brief
     *  Construct this texture.
     * @remark
     *  Intentionally protected.
     */
    Texture(const std::string& name,
            TextureType type, TextureAddressMode addressModeS, TextureAddressMode addressModeT,
            int width, int height, int _sourceWidth, int _sourceHeight, std::shared_ptr<SDL_Surface> source,
            bool hasAlpha);

    /**
     * @brief
     *  Destruct this texture.
     * @remark
     *  Intentionally protected.
     */
    virtual ~Texture();

    /**
     * @brief
     *  Get the texture address mode of this texture along the s-axis.
     * @return
     *  the texture address mode of this texture along the s-axis.
     */
    Ego::TextureAddressMode getAddressModeS() const;

    /**
     * @brief
     *  Get the texture address mode of this texture along the t-axis.
     * @return
     *  the texture address mode of this texture along the t-axis.
     */
    Ego::TextureAddressMode getAddressModeT() const;

    /**
     * @brief
     *  Get the width, in pixels, of this texture.
     * @param self
     *  this texture
     * @return
     *  the width, in pixels of this texture
     */
    int getWidth() const;

    /**
     * @brief
     *  Get the height, in pixels, of this texture.
     * @return
     *  the height, in pixels of this texture
     */
    int getHeight() const;

    /**
     * @brief
     *  Get the width, in pixels, of the source of this texture.
     * @return
     *  the width, in pixels, of the source of this texture
     * @remark
     *  This value might differ for technical reasons from the width of the texture.
     */
    int getSourceWidth() const;

    /**
     * @brief
     *  Get the height, in pixels, of the source of this texture.
     * @return
     *  the height, in pixels, of the source of this texture
     * @remark
     *  This value might differ for technical reasons from the height of the texture.
     */
    int getSourceHeight() const;

    /**
     * @brief
     *  Get if this texture has an alpha component.
     * @return
     *  @a true if this texture has an alpha component,
     *  @a false otherwise
     */
    bool hasAlpha() const;

    /**
     * @brief
     *  Set the name of this texture.
     * @param name
     *  the name
     */
    void setName(const std::string& name);

    /**
     * @brief
     *  Get the name of this texture.
     * @return
     *  the name of this texture
     */
    const std::string& getName() const;

};

} // namespace Ego

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    /// An encapsulation of the OpenGL texture state.
    struct oglx_texture_t : public Ego::Texture
    {


    protected:

        /**
         * @brief
         *  The OpenGL texture ID.
         * @remark
         *  At any point, a texture has a valid OpenGL texture ID assigned, <em>unless</em> resources were lost.
         */
        GLuint  _id;

    public:
        GLuint load(const std::string& name, std::shared_ptr<SDL_Surface> surface, Uint32 key = INVALID_KEY);
        GLuint load(std::shared_ptr<SDL_Surface> image, Uint32 key = INVALID_KEY);

        /**
         * @brief
         *  Delete backing image, delete OpenGL ID, assign OpenGL ID of the error texture, assign no backing image.
         * @param self
         *  this texture
         */
        void release();

    public:
        /**
        * @brief
        *  Construct this texture.
        * @post
        *  This texture is bound to the backing error texture.
        */
        oglx_texture_t();

        /**
        * @brief
        *  Destruct this texture.
        */
        virtual ~oglx_texture_t();

        static void bind(oglx_texture_t *texture);

    public:
        GLuint getTextureID() const;

    };

    /**
     * @brief
     *  Initialize the error textures.
     * @todo
     *  Move into texture manager.
     */
    void initializeErrorTextures();
    /**
     * @brief
     *  Uninitialize the error textures.
     * @todo
     *  Move into texture manager.
     */
    void uninitializeErrorTextures();
    GLuint get2DErrorTextureID();
    GLuint get1DErrorTextureID();
    bool isErrorTextureID(GLuint id);
