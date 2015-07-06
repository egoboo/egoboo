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

/// @file   egolib/Renderer/Renderer.hpp
/// @brief  Common interface of all textures.
/// @author Michael Heilmann

#include "egolib/Renderer/Texture.hpp"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/SDL_GL_extensions.h"
#include "egolib/Math/_Include.hpp"

#include "egolib/egoboo_setup.h"
#include "egolib/strutil.h"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Image/Image.hpp"
#include "egolib/vfs.h"

namespace Ego
{

Texture::Texture(const std::string& name,
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

TextureFilter Texture::getMinFilter() const
{
    return _minFilter;
}

void Texture::setMinFilter(TextureFilter minFilter)
{
    _minFilter = minFilter;
}

TextureFilter Texture::getMagFilter() const
{
    return _magFilter;
}

void Texture::setMagFilter(TextureFilter magFilter)
{
    _magFilter = magFilter;
}

TextureAddressMode Texture::getAddressModeS() const
{
    return _addressModeS;
}

void Texture::setAddressModeS(TextureAddressMode addressModeS)
{
    _addressModeS = addressModeS;
}

TextureAddressMode Texture::getAddressModeT() const
{
    return _addressModeT;
}

void Texture::setAddressModeT(TextureAddressMode addressModeT)
{
    _addressModeT = addressModeT;
}

int Texture::getSourceHeight() const
{
    return _sourceHeight;
}

int Texture::getSourceWidth() const
{
    return _sourceWidth;
}

int Texture::getWidth() const
{
    return _width;
}

int Texture::getHeight() const
{
    return _height;
}

bool Texture::hasAlpha() const
{
    return _hasAlpha;
}

void Texture::setName(const std::string& name)
{
    _name = name;
}

const std::string& Texture::getName() const
{
    return _name;
}

} // namespace Ego

bool IMG_test_alpha(SDL_Surface *surface)
{
    // test to see whether an image requires alpha blending

    if (!surface)
    {
        throw std::invalid_argument("nullptr == surface");
    }

    // Alias.
    SDL_PixelFormat *format = surface->format;

    // If the surface has a per-surface color key, it is partially transparent.
    Uint32 colorKey;
    int rslt = SDL_GetColorKey(surface, &colorKey);
    if (rslt < -1)
    {
        // If a value smaller than -1 is returned, an error occured.
        throw std::invalid_argument("SDL_GetColorKey failed");
    }
    else if (rslt >= 0)
    {
        // If a value greater or equal than 0 is returned, the surface has a color key.
        return true;
    }
    /*
    else if (rslt == -1)
    {
    // If a value of -1 is returned, the surface has no color key: Continue.
    }
    */

    // If it is an image without an alpha channel, then there is no alpha in the image.
    if (!format->palette && 0x00 == format->Amask)
    {
        return false;
    }

    // grab the info for scanning the surface
    Uint32 bitMask = format->Rmask | format->Gmask | format->Bmask | format->Amask;
    int bytesPerPixel = format->BytesPerPixel;
    int width = surface->w;
    int height = surface->h;
    int pitch = surface->pitch;

    const char *row_ptr = static_cast<const char *>(surface->pixels);
    for (int y = 0; y < height; ++y)
    {
        const char *char_ptr = row_ptr;
        for (int x = 0; x < width; ++x)
        {
            Uint32 *ui32_ptr = (Uint32 *)char_ptr;
            Uint32 pixel = (*ui32_ptr) & bitMask;
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);

            if (0xFF != a)
            {
                return true;
            }
            char_ptr += bytesPerPixel;
        }
        row_ptr += pitch;
    }

    return false;
}

bool IMG_test_alpha_key(SDL_Surface *surface, Uint32 key)
{
    // test to see whether an image requires alpha blending
    if (!surface)
    {
        return false;
    }
    // save this alias
    SDL_PixelFormat *format = surface->format;

    // If there is no key specified (or the image has an alpha channel), use the basic version.
    if (INVALID_KEY == key || format->Aloss < 8)
    {
        return IMG_test_alpha(surface);
    }

    // If the overall alpha marks the surface as not fully opaque,
    // it is partially transparent and hence requires alpha blending.
    uint8_t alpha;
    SDL_GetSurfaceAlphaMod(surface, &alpha);
    if (0xff != alpha)
    {
        return true;
    }

    // grab the info for scanning the surface
    Uint32 bitMask = format->Rmask | format->Gmask | format->Bmask | format->Amask;
    int bytesPerPixel = format->BytesPerPixel;
    int w = surface->w;
    int h = surface->h;
    int pitch = surface->pitch;

    const char *row_ptr = static_cast<const char *>(surface->pixels);
    for (int iy = 0; iy < h; iy++)
    {
        const char *char_ptr = row_ptr;
        for (int ix = 0; ix < w; ix++)
        {
            Uint32 *ui32_ptr = (Uint32 *)char_ptr;
            Uint32 pixel = (*ui32_ptr) & bitMask;

            if (pixel == key)
            {
                return true;
            }
            else
            {
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
                if (0xFF != a)
                {
                    return true;
                }
            }

            // advance to the next entry
            char_ptr += bytesPerPixel;
        }
        row_ptr += pitch;
    }

    return false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct CErrorTexture
{
    std::shared_ptr<SDL_Surface> _image;
    /// The OpenGL texture target of this error texture.
    GLenum _target;
    /// The OpenGL ID of this error texture.
    GLuint _id;
    char *_name;
    const char *getName() const
    {
        return _name;
    }
    int getSourceWidth() const
    {
        return _image->w;
    }
    int getSourceHeight() const
    {
        return _image->h;
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
    CErrorTexture(const char *name, GLenum target) :
        _image(ImageManager::get().getDefaultImage())
    {
        _name = strdup(name);
        if (!_name)
        {
            throw std::runtime_error("unable to create error texture");
        }
        _target = target;
        if (_target != GL_TEXTURE_1D && _target != GL_TEXTURE_2D)
        {
            free(_name);
            throw std::invalid_argument("invalid texture target");
        }
        while (GL_NO_ERROR != glGetError())
        {
            /* Nothing to do. */
        }
        glGenTextures(1, &_id);
        if (GL_NO_ERROR != glGetError())
        {
            free(_name);
            throw std::runtime_error("unable to create error texture");
        }
        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


        glBindTexture(target, _id);
        if (GL_NO_ERROR != glGetError())
        {
            free(_name);
            glDeleteTextures(1, &_id);
            throw std::runtime_error("unable to bind error texture");
        }

        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        if (GL_NO_ERROR != glGetError())
        {
            free(_name);
            glDeleteTextures(1, &_id);
            throw std::runtime_error("unable to set error texture parameters");
        }
        // Create the image.
        if (target == GL_TEXTURE_1D)
        {
            glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, _image->w, 0, GL_RGBA, GL_UNSIGNED_BYTE, _image->pixels);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _image->w, _image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, _image->pixels);
        }

        glPopClientAttrib();
        if (GL_NO_ERROR != glGetError())
        {
            free(_name);
            glDeleteTextures(1, &_id);
            throw std::runtime_error("unable to upload error image into error texture");
        }
    }
    // Destruct this error texture.
    ~CErrorTexture()
    {
        free(_name);
        glDeleteTextures(1, &_id);
    }
};

static CErrorTexture *_errorTexture1D = nullptr;
static CErrorTexture *_errorTexture2D = nullptr;

void initializeErrorTextures()
{
    if (!_errorTexture1D)
    {
        _errorTexture1D = new CErrorTexture("<error texture 1D>", GL_TEXTURE_1D);
    }
    if (!_errorTexture2D)
    {
        try
        {
            _errorTexture2D = new CErrorTexture("<error texture 2D>", GL_TEXTURE_2D);
        }
        catch (...)
        {
            delete _errorTexture1D;
            _errorTexture1D = nullptr;
            std::rethrow_exception(std::current_exception());
        }
    }
}

void uninitializeErrorTextures()
{
    delete _errorTexture2D;
    _errorTexture2D = nullptr;
    delete _errorTexture1D;
    _errorTexture1D = nullptr;
}

GLuint get1DErrorTextureID()
{
    return _errorTexture1D->_id;
}

GLuint get2DErrorTextureID()
{
    return _errorTexture2D->_id;
}

bool isErrorTextureID(GLuint id)
{
    if(!_errorTexture1D || !_errorTexture2D) return false;
    return id == _errorTexture1D->_id
        || id == _errorTexture2D->_id;
}

oglx_texture_t::oglx_texture_t() :
    Ego::Texture
        (
            // The name of the texture is the error texture's.
            _errorTexture2D->getName(),
            // The texture is the 2D error texture.
            Ego::TextureType::_2D,
            // The texture coordinates of this texture are repeated along the s and t axes.
            Ego::TextureAddressMode::Repeat, Ego::TextureAddressMode::Repeat,
            // The size (width and height) of this texture is the size of the error image.
            _errorTexture2D->getWidth(), _errorTexture2D->getHeight(),
            // The size (width and height) the source of this texture is the size of the error image as well.
            _errorTexture2D->getSourceWidth(), _errorTexture2D->getSourceHeight(),
            // The error texture has no source.
            nullptr,
            // (The error texture has no alpha component).
            false
        ),
    // The OpenGL texture ID is the error texture's.
    _id(_errorTexture2D->_id)
{}

oglx_texture_t::~oglx_texture_t()
{
    release();
}

GLuint  oglx_texture_t::getTextureID() const
{
    return _id;
}

GLuint oglx_texture_t::load(const std::string& name, std::shared_ptr<SDL_Surface> source, Uint32 key)
{
    // Bind this texture to the backing error texture.
    release();

    // If no source is provided, keep this texture bound to the backing error texture.
    if (!source)
    {
        return INVALID_GL_ID;
    }

    // Convert the source into a format suited for OpenGL.
    std::shared_ptr<SDL_Surface> new_source;
    try
    {
        new_source = SDL_GL_convert(source);
    }
    catch (...)
    {
        return INVALID_GL_ID;
    }
    if (!new_source)
    {
        return INVALID_GL_ID;
    }
    // Generate a new OpenGL texture ID.
    Ego::OpenGL::Utilities::clearError();
    GLuint id;
    while (GL_NO_ERROR != glGetError())
    {
        /* Nothing to do. */
    }
    glGenTextures(1, &id);
    if (Ego::OpenGL::Utilities::isError())
    {
        return INVALID_GL_ID;
    }
    // Use default texture address mode.
    auto textureAddressModeS = Ego::TextureAddressMode::Repeat;
    auto textureAddressModeT = Ego::TextureAddressMode::Repeat;
    // Determine the texture type.
    auto type = ((1 == source->h) && (source->w > 1)) ? Ego::TextureType::_1D : Ego::TextureType::_2D;
    // Test if the image requires alpha blending.
    bool hasAlpha = IMG_test_alpha_key(new_source.get(), key);
    /* Set texture address mode and texture filtering. */
    Ego::OpenGL::Utilities::bind(id, type, textureAddressModeS, textureAddressModeT);
    /* Upload the texture data. */
    if (type == Ego::TextureType::_2D)
    {
        if (g_ogl_textureParameters.textureFilter.mipMapFilter > Ego::TextureFilter::None)
        {
            Ego::OpenGL::Utilities::upload_2d_mipmap(Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>(), new_source->w, new_source->h, new_source->pixels);
        }
        else
        {
            Ego::OpenGL::Utilities::upload_2d(Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>(), new_source->w, new_source->h, new_source->pixels);
        }
    }
    else if (type == Ego::TextureType::_1D)
    {
        Ego::OpenGL::Utilities::upload_1d(Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>(), new_source->w, new_source->pixels);
    }
    else
    {
        EGOBOO_ASSERT(0); /// @todo This code is in fact unreachable. Raise a std::runtime_error.
    }

    // Store the appropriate data.
    _addressModeS = textureAddressModeS;
    _addressModeT = textureAddressModeT;
    _type = type;
    _id = id;
    _width = new_source->w;
    _height = new_source->h;
    _source = source;
    _sourceWidth = source->w;
    _sourceHeight = source->h;
    _hasAlpha = hasAlpha;
    _name = name;

    return _id;
}

GLuint oglx_texture_t::load(std::shared_ptr<SDL_Surface> source, Uint32 key)
{
    std::ostringstream stream;
    stream << "<source " << static_cast<void *>(source.get()) << ">";
    return load(stream.str().c_str(), source, key);
}

void  oglx_texture_t::release()
{
    if (isErrorTextureID(_id))
    {
        return;
    }

    // Delete the OpenGL texture and assign the error texture.
    glDeleteTextures(1, &(_id));
    Ego::OpenGL::Utilities::isError();

    // Delete the source if it exists
    if (_source)
    {
        _source = nullptr;
    }

    // The texture is the 2D error texture.
    _type = Ego::TextureType::_2D;
    _id = _errorTexture2D->_id;

    // The texture coordinates of this texture are repeated along the s and t axes.
    _addressModeS = Ego::TextureAddressMode::Repeat;
    _addressModeT = Ego::TextureAddressMode::Repeat;

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

void oglx_texture_t::bind(const oglx_texture_t *texture)
{
    /// @author BB
    /// @details a oglx_texture_t wrapper for oglx_bind_to_tex_params() function

    if (!texture)
    {
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
    }
    else
    {
        auto anisotropy_enable = g_ogl_textureParameters.anisotropy_enable;
        auto anisotropy_level = g_ogl_textureParameters.anisotropy_level;
        Ego::OpenGL::Utilities::clearError();
        GLenum target_gl;
        switch (texture->_type)
        {
            case Ego::TextureType::_2D:
                glEnable(GL_TEXTURE_2D);
                glDisable(GL_TEXTURE_1D);
                target_gl = GL_TEXTURE_2D;
                break;
            case Ego::TextureType::_1D:
                glEnable(GL_TEXTURE_1D);
                glDisable(GL_TEXTURE_2D);
                target_gl = GL_TEXTURE_1D;
                break;
            default:
                throw std::runtime_error("unreachable code reached");
        }
        if (Ego::OpenGL::Utilities::isError())
        {
            return;
        }
        glBindTexture(target_gl, texture->_id);
        if (Ego::OpenGL::Utilities::isError())
        {
            return;
        }

        glTexParameteri(target_gl, GL_TEXTURE_WRAP_S, Ego::OpenGL::Utilities::toOpenGL(texture->_addressModeS));
        glTexParameteri(target_gl, GL_TEXTURE_WRAP_T, Ego::OpenGL::Utilities::toOpenGL(texture->_addressModeT));


        if (Ego::OpenGL::Utilities::isError())
        {
            return;
        }

        GLint minFilter_gl, magFilter_gl;
        Ego::OpenGL::Utilities::toOpenGL(texture->_minFilter, texture->_magFilter, texture->_mipMapFilter, minFilter_gl, magFilter_gl);
        glTexParameteri(target_gl, GL_TEXTURE_MIN_FILTER, minFilter_gl);
        glTexParameteri(target_gl, GL_TEXTURE_MAG_FILTER, magFilter_gl);
        if (Ego::OpenGL::Utilities::isError())
        {
            return;
        }


        if (GL_TEXTURE_2D == target_gl && g_ogl_caps.anisotropic_supported && anisotropy_enable && anisotropy_level >= 1.0f)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy_level);
        }

        if (Ego::OpenGL::Utilities::isError())
        {
            return;
        }
    }
    Ego::OpenGL::Utilities::isError();
}
