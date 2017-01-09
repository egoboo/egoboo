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

#include "egolib/Renderer/Texture.hpp"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/Extensions/ogl_include.h"
#include "egolib/Extensions/SDL_GL_extensions.h"
#include "egolib/Math/_Include.hpp"

#include "egolib/egoboo_setup.h"
#include "egolib/strutil.h"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Image/Image.hpp"
#include "egolib/Renderer/TextureSampler.hpp"
#include "egolib/vfs.h"

namespace Ego {

Texture::Texture(const String& name,
                 TextureType type, TextureAddressMode addressModeS, TextureAddressMode addressModeT,
                 int width, int height, int sourceWidth, int sourceHeight, std::shared_ptr<SDL_Surface> source,
                 bool hasAlpha) :
    _name(name),
    _type(type), _addressModeS(addressModeS),  _addressModeT(addressModeT),
    _width(width), _height(height), _sourceWidth(sourceWidth), _sourceHeight(sourceHeight), _source(source),
    _minFilter(g_ogl_textureParameters.textureFilter.minFilter), _magFilter(g_ogl_textureParameters.textureFilter.magFilter), _mipMapFilter(g_ogl_textureParameters.textureFilter.mipMapFilter),
    _hasAlpha(hasAlpha)
{}

Texture::~Texture()
{}

TextureType Texture::getType() const {
    return _type;
}

TextureFilter Texture::getMipMapFilter() const {
    return _mipMapFilter;
}

void Texture::setMipMapFilter(TextureFilter mipMapFilter) {
    _mipMapFilter = mipMapFilter;
}

TextureFilter Texture::getMinFilter() const {
    return _minFilter;
}

void Texture::setMinFilter(TextureFilter minFilter) {
    _minFilter = minFilter;
}

TextureFilter Texture::getMagFilter() const {
    return _magFilter;
}

void Texture::setMagFilter(TextureFilter magFilter) {
    _magFilter = magFilter;
}

TextureAddressMode Texture::getAddressModeS() const {
    return _addressModeS;
}

void Texture::setAddressModeS(TextureAddressMode addressModeS) {
    _addressModeS = addressModeS;
}

TextureAddressMode Texture::getAddressModeT() const {
    return _addressModeT;
}

void Texture::setAddressModeT(TextureAddressMode addressModeT) {
    _addressModeT = addressModeT;
}

int Texture::getSourceHeight() const {
    return _sourceHeight;
}

int Texture::getSourceWidth() const {
    return _sourceWidth;
}

int Texture::getWidth() const {
    return _width;
}

int Texture::getHeight() const {
    return _height;
}

bool Texture::hasAlpha() const {
    return _hasAlpha;
}

void Texture::setName(const String& name) {
    _name = name;
}

const Texture::String& Texture::getName() const {
    return _name;
}

} // namespace Ego

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct CErrorTexture
{
    template <typename T> using SharedPtr = std::shared_ptr<T>;
    using String = std::string;

    SharedPtr<SDL_Surface> image;
    /// The OpenGL texture target of this error texture.
    Ego::TextureType type;
    /// The OpenGL ID of this error texture.
    GLuint id;
    String name;
    GLuint getTextureID() const
    {
        return id;
    }
    const String& getName() const
    {
        return name;
    }
    int getSourceWidth() const
    {
        return image->w;
    }
    int getSourceHeight() const
    {
        return image->h;
    }
    int getWidth() const
    {
        return getSourceWidth();
    }
    int getHeight() const
    {
        return getSourceHeight();
    }
    // Construct this error texture.
    CErrorTexture(const String& name, Ego::TextureType type) :
        name(name), type(type), image(Ego::ImageManager::get().getDefaultImage())
    {
        namespace OpenGL = Ego::OpenGL;
        GLenum target_gl;
        switch (type) {
            case Ego::TextureType::_1D:
                target_gl = GL_TEXTURE_1D;
                break;
            case Ego::TextureType::_2D:
                target_gl = GL_TEXTURE_2D;
                break;
            default:
                throw Id::InvalidArgumentException(__FILE__, __LINE__, "invalid texture target");
        };
        OpenGL::Utilities::clearError();
        // (1) Create the OpenGL texture.
        glGenTextures(1, &id);
        if (OpenGL::Utilities::isError())
        {
            throw RuntimeErrorException(__FILE__, __LINE__, "unable to create error texture");
        }
        // (2) Bind the OpenGL texture.
        glBindTexture(target_gl, id);
        if (OpenGL::Utilities::isError())
        {
            glDeleteTextures(1, &id);
            throw RuntimeErrorException(__FILE__, __LINE__, "unable to bind error texture");
        }
        // (3) Set the texture parameters.
        Ego::TextureSampler sampler(Ego::TextureFilter::Nearest, Ego::TextureFilter::Nearest,
                                    Ego::TextureFilter::None, Ego::TextureAddressMode::Repeat,
                                    Ego::TextureAddressMode::Repeat, 1.0f);
        try {
            OpenGL::Utilities::setSampler(type, sampler);
        } catch (...) {
            glDeleteTextures(1, &id);
            std::rethrow_exception(std::current_exception());
        }

        // (4) Upload the image data.
        if (type == Ego::TextureType::_1D)
        {
            static const auto pfd = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>();
            OpenGL::Utilities::upload_1d(pfd, image->w, image->pixels);
        }
        else
        {
            static const auto pfd = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>();
            OpenGL::Utilities::upload_2d(pfd, image->w, image->h, image->pixels);
        }
        if (OpenGL::Utilities::isError())
        {
            glDeleteTextures(1, &id);
            throw RuntimeErrorException(__FILE__, __LINE__, "unable to upload error image into error texture");
        }
    }
    // Destruct this error texture.
    ~CErrorTexture()
    {
        glDeleteTextures(1, &id);
    }
};

static std::unique_ptr<CErrorTexture> _errorTexture1D = nullptr;
static std::unique_ptr<CErrorTexture> _errorTexture2D = nullptr;

namespace Ego {
namespace OpenGL {

void initializeErrorTextures()
{
    if (!_errorTexture1D)
    {
        _errorTexture1D = std::make_unique<CErrorTexture>("<error texture 1D>", TextureType::_1D);
    }
    if (!_errorTexture2D)
    {
        try
        {
            _errorTexture2D = std::make_unique<CErrorTexture>("<error texture 2D>", TextureType::_2D);
        }
        catch (...)
        {
            _errorTexture1D = nullptr;
            std::rethrow_exception(std::current_exception());
        }
    }
}

void uninitializeErrorTextures()
{
    _errorTexture2D = nullptr;
    _errorTexture1D = nullptr;
}

}
}

namespace Ego {
namespace OpenGL {

Texture::Texture() :
    Texture
        (
            // The OpenGL texture ID is the error texture's.
            _errorTexture2D->getTextureID(),
            // The name of the texture is the error texture's.
            _errorTexture2D->getName(),
            // The texture is the 2D error texture.
            TextureType::_2D,
            // The texture coordinates of this texture are repeated along the s and t axes.
            TextureAddressMode::Repeat, TextureAddressMode::Repeat,
            // The size (width and height) of this texture is the size of the error image.
            _errorTexture2D->getWidth(), _errorTexture2D->getHeight(),
            // The size (width and height) the source of this texture is the size of the error image as well.
            _errorTexture2D->getSourceWidth(), _errorTexture2D->getSourceHeight(),
            // The error texture has no source.
            nullptr,
            // (The error texture has no alpha component).
            false
        )
{}

Texture::Texture(GLuint id, const std::string& name,
                 TextureType type, TextureAddressMode addressModeS, TextureAddressMode addressModeT,
                 int width, int height, int sourceWidth, int sourceHeight, std::shared_ptr<SDL_Surface> source,
                 bool hasAlpha) :
    Ego::Texture
    (
        // The name of the texture is the error texture's.
        name,
        // The texture is the 2D error texture.
        type,
        // The texture coordinates of this texture are repeated along the s and t axes.
        addressModeS, addressModeT,
        // The size (width and height) of this texture is the size of the error image.
        width, height,
        // The size (width and height) the source of this texture is the size of the error image as well.
        sourceWidth, sourceHeight,
        // The error texture has no source.
        source,
        // (The error texture has no alpha component).
        hasAlpha
    ),
    _id(id)
{}

Texture::~Texture() {
    release();
}

GLuint Texture::getTextureID() const {
    return _id;
}

void Texture::load(const String& name, const SharedPtr<SDL_Surface>& surface, TextureType type, const TextureSampler& sampler) {
    // Bind this texture to the backing error texture.
    release();

    // If no surface is provided, keep this texture bound to the backing error texture.
    if (!surface) {
        throw Id::InvalidArgumentException(__FILE__, __LINE__, "nullptr == surface");
    }

    SharedPtr<SDL_Surface> newSurface = surface;

    // Convert to RGBA if the image has non-opaque alpha values or alpha modulation and convert to RGB otherwise.
    bool hasAlpha = Graphics::SDL::testAlpha(newSurface);
    const auto& pixelFormatDescriptor = hasAlpha ? PixelFormatDescriptor::get<PixelFormat::R8G8B8A8>()
                                                 : PixelFormatDescriptor::get<PixelFormat::R8G8B8>();
    newSurface = Graphics::SDL::convertPixelFormat(newSurface, pixelFormatDescriptor);

    // Convert to power of two.
    newSurface = Graphics::SDL::convertPowerOfTwo(newSurface);

    // (1)Generate a new OpenGL texture ID.
    Utilities::clearError();
    GLuint id;
    glGenTextures(1, &id);
    if (Utilities::isError()) {
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "glGenTextures failed");
    }
    // (2) Bind the new OpenGL texture ID.
    GLenum target_gl;
    switch (type) {
        case TextureType::_1D:
            target_gl = GL_TEXTURE_1D;
            break;
        case TextureType::_2D:
            target_gl = GL_TEXTURE_2D;
            break;
        default:
        {
            glDeleteTextures(1, &id);
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
        }
    };
    glBindTexture(target_gl, id);
    if (Utilities::isError()) {
        glDeleteTextures(1, &id);
        throw Id::RuntimeErrorException(__FILE__, __LINE__, "glBindTexture failed");
    }
    // (3) Set the texture sampler.
    try {
        Utilities::setSampler(type, sampler);
    } catch (...) {
        glDeleteTextures(1, &id);
        std::rethrow_exception(std::current_exception());
    }
    // (4) Upload the texture data.
    switch (type) {
        case TextureType::_2D:
        {
            if (TextureFilter::None != sampler.getMipMapFilter()) {
                Utilities::upload_2d_mipmap(pixelFormatDescriptor, newSurface->w, newSurface->h, newSurface->pixels);
            } else {
                Utilities::upload_2d(pixelFormatDescriptor, newSurface->w, newSurface->h, newSurface->pixels);
            }
        }
        break;
        case TextureType::_1D:
        {
            Utilities::upload_1d(pixelFormatDescriptor, newSurface->w, newSurface->pixels);
        }
        break;
        default:
        {
            glDeleteTextures(1, &id);
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
        }
        break;
    };

    // Store the appropriate data.
    _addressModeS = sampler.getAddressModeS();
    _addressModeT = sampler.getAddressModeT();
    _type = type;
    _id = id;
    _width = newSurface->w;
    _height = newSurface->h;
    _source = surface;
    _sourceWidth = surface->w;
    _sourceHeight = surface->h;
    _hasAlpha = hasAlpha;
    _name = name;
}

bool Texture::load(const String& name, const SharedPtr<SDL_Surface>& source) {
    // Determine the texture sampler.
    TextureSampler sampler(g_ogl_textureParameters.textureFilter.minFilter,
                           g_ogl_textureParameters.textureFilter.magFilter,
                           g_ogl_textureParameters.textureFilter.mipMapFilter,
                           TextureAddressMode::Repeat, TextureAddressMode::Repeat,
                           g_ogl_textureParameters.anisotropy_level);
    // Determine the texture type.
    auto type = ((1 == source->h) && (source->w > 1)) ? TextureType::_1D : TextureType::_2D;
    load(name, source, type, sampler);
    return true;
}

bool Texture::load(const std::shared_ptr<SDL_Surface>& source) {
    std::ostringstream stream;
    stream << "<source " << static_cast<void *>(source.get()) << ">";
    return load(stream.str(), source);
}

void  Texture::release() {
    if (isDefault())
    {
        return;
    }

    // Delete the OpenGL texture and assign the error texture.
    glDeleteTextures(1, &(_id));
    Utilities::isError();

    // Delete the source if it exists
    if (_source)
    {
        _source = nullptr;
    }

    if(!_errorTexture2D) {
        return;
    }

    // The texture is the 2D error texture.
    _type = TextureType::_2D;
    _id = _errorTexture2D->getTextureID();

    // The texture coordinates of this texture are repeated along the s and t axes.
    _addressModeS = TextureAddressMode::Repeat;
    _addressModeT = TextureAddressMode::Repeat;

    // The size (width and height) of this texture is the size of the error image.
    _width = _errorTexture2D->getWidth();
    _height = _errorTexture2D->getHeight();

    // The size (width and height) the source of this texture is the size of the error image as well.
    _sourceWidth = _errorTexture2D->getSourceWidth();
    _sourceHeight = _errorTexture2D->getSourceHeight();

    // The texture has the empty string as its name and the source is not available.
    _name = _errorTexture2D->getName();
    
    // (The error texture has no alpha component).
    _hasAlpha = false;
}

bool Texture::isDefault() const {
    if (!_errorTexture1D || !_errorTexture2D) return false;
    return getTextureID() == _errorTexture1D->getTextureID()
        || getTextureID() == _errorTexture2D->getTextureID();
}

} // namespace OpenGL
} // namespace Ego
