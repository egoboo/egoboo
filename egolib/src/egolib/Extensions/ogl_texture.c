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

/// @file egolib/Extensions/ogl_texture.c
/// @ingroup _ogl_extensions_
/// @brief Implements OpenGL texture loading using SDL_image
/// @details Basic loading and managing OpenGL textures.
///   Uses SDL_image to load .tif, .png, .bmp, .dib, .xpm, and other formats into OpenGL texures

#include "egolib/Extensions/ogl_texture.h"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/SDL_GL_extensions.h"

#include "egolib/egoboo_setup.h"
#include "egolib/strutil.h"
#include "egolib/Image/Image.hpp"
#include "egolib/vfs.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define VALID_VALUE        0x32B04E67

#define VALID_TEXTURE( PTEX ) ( (NULL != (PTEX)) && (VALID_VALUE == (PTEX)->valid) )

static GLuint ErrorTexture_binding;
static bool ErrorTexture_created = false;

struct CErrorImage
{
private:
    const static size_t width = 2;
    const static size_t height = 2;
    GLubyte _bytes[height][width][4];
public:
    size_t getWidth() const throw()
        { return width; }
    
    size_t getHeight() const throw()
        { return height; }
    
    const GLubyte *getBytes() const throw()
        { return &(_bytes[0][0][0]); }
public:
    CErrorImage() throw()
    {
        for (size_t i = 0; i < height; i++)
        {
            for (size_t j = 0; j < width; j++)
            {
                if (0 == ((i & 0x1) ^ (j & 0x1)))
                {
                    _bytes[i][j][0] = (GLubyte)255;
                    _bytes[i][j][1] = (GLubyte)0;
                    _bytes[i][j][2] = (GLubyte)0;
                }
                else
                {
                    _bytes[i][j][0] = (GLubyte)0;
                    _bytes[i][j][1] = (GLubyte)255;
                    _bytes[i][j][2] = (GLubyte)255;
                }

                _bytes[i][j][3] = (GLubyte)255;
            }
        }
    }
};

static CErrorImage _errorImage;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ErrorTexture_create()
{
    if (!ErrorTexture_created)
    {
        // Clear the OpenGL error state.
        while (GL_NO_ERROR != glGetError())
        {
            /* Intentionally empty. */
        }
        glGenTextures(1, &ErrorTexture_binding);
        if (GL_NO_ERROR != glGetError())
        {
            throw std::runtime_error("unable to create error image");
        }
        try
        {
            ErrorImage_bind(GL_TEXTURE_2D, ErrorTexture_binding);
        }
        catch (...)
        {
            std::rethrow_exception(std::current_exception());
        }
        ErrorTexture_created = true;
    }
}

//--------------------------------------------------------------------------------------------
// Bind the given texture ID to the given target and sets its data to the data of the error image.
void ErrorImage_bind(GLenum target, GLuint id)
{
    while (GL_NO_ERROR != glGetError())
    { 
        /* Intentionally empty. */ 
    }
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


    glBindTexture(target, id);
    if (GL_NO_ERROR != glGetError())
    {
        throw std::runtime_error("unable to bind error image");
    }

    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // Create the image.
    if (target == GL_TEXTURE_1D)
    {
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, _errorImage.getWidth(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _errorImage.getBytes());
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _errorImage.getWidth(), _errorImage.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, _errorImage.getBytes());
    }

    glPopClientAttrib();
    if (GL_NO_ERROR != glGetError())
    {
        throw std::runtime_error("unable to bind error image");
    }

}

//--------------------------------------------------------------------------------------------
GLuint ErrorImage_get_binding()
{
    ErrorTexture_create();
    return ErrorTexture_binding;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oglx_texture_t *oglx_texture_t::ctor(oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    BLANK_STRUCT_PTR(self);

    self->base_valid = false;
    self->name[0] = '\0';
    self->imgW = self->imgH = 0;
    self->surface = nullptr;
    self->has_alpha = SDL_FALSE;



    // Only need one base binding per texture:
    // Do not need to ask for a new id, even if we change the texture data.
    GL_DEBUG(glGenTextures)(1, &(self->base.binding));

    // Set the validity flag.
    if (VALID_BINDING(self->base.binding) && !ERROR_IMAGE_BINDING(self->base.binding))
    {
        self->valid = VALID_VALUE;
    }
    else
    {
        self->valid = (GLuint)(~VALID_VALUE);
    }

    // Set to 2d texture by default.
    self->base.target = GL_TEXTURE_2D;

    // Set the image to be clamped in s and t.
    self->base.wrap_s = GL_CLAMP;
    self->base.wrap_t = GL_CLAMP;

    return self;
}

void oglx_texture_t::dtor(oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!VALID_TEXTURE(self)) return;

    // Mark as invalid.
    self->valid = (GLuint)(~VALID_VALUE);

    // Actually delete the OpenGL texture data.
    if (VALID_BINDING(self->base.binding))
    {
        GL_DEBUG(glDeleteTextures)(1, &(self->base.binding));
        self->base.binding = INVALID_GL_ID;
    }

    // Set the image to be clamped in s and t.
    self->base.wrap_s = GL_CLAMP;
    self->base.wrap_t = GL_CLAMP;

    // Reset the other data.
    self->imgH = self->imgW = self->base.width = self->base.height  = 0;
    self->name[0] = '\0';

    if (self->surface)
    {
        SDL_FreeSurface(self->surface);
        self->surface = nullptr;
    }
}

oglx_texture_t *oglx_texture_t::create()
{
    oglx_texture_t *self = static_cast<oglx_texture_t *>(malloc(sizeof(oglx_texture_t)));
    if (!self)
    {
        throw std::bad_alloc();
    }
    if (!oglx_texture_t::ctor(self))
    {
        free(self);
        throw std::bad_alloc();
    }
    return self;
}

void oglx_texture_t::destroy(oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    oglx_texture_t::dtor(self);
    free(self);
}

//--------------------------------------------------------------------------------------------
GLuint oglx_texture_t::convert(oglx_texture_t *self, SDL_Surface *image, Uint32 key)
{
    /// @author BB
    /// @details an oglx_texture_t wrapper for the SDL_GL_convert_surface() function

    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    // Make sure the old texture has been freed.
    oglx_texture_t::release(self);

    if (!image) return INVALID_GL_ID;

    /* set the color key, if valid */
    if (nullptr != image->format && nullptr != image->format->palette && INVALID_KEY != key)
    {
        SDL_SetColorKey(image, SDL_SRCCOLORKEY, key);
    }

    // Determine the correct power of two greater than or equal to the original image's size.
    self->base.binding = SDL_GL_convert_surface(self->base.binding, image, self->base.wrap_s, self->base.wrap_t);

    self->base.target = (( 1 == image->h) && (image->w > 1)) ? GL_TEXTURE_1D : GL_TEXTURE_2D;
    self->base.height = powerOfTwo(image->h);
    self->base.width  = powerOfTwo(image->w);

    // Set up some parameters for the format of the oglx_texture_t
    self->base_valid = true;
    self->surface    = image;
    self->imgW       = image->w;
    self->imgH       = image->h;
    strncpy(self->name, "SDL_Surface()", SDL_arraysize(self->name));

    return self->base.binding;
}

//--------------------------------------------------------------------------------------------
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
    if (!format->palette && 0x00 == format->Amask )
    {
        return SDL_FALSE;
    }

    // grab the info for scanning the surface
    //bit_mask = pformat->Rmask | pformat->Gmask | pformat->Bmask | pformat->Amask;
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
            Uint32 pixel = 0;
            for (int i = 0; i < bytesPerPixel; ++i)
            {
                if (pixel) pixel <<= 8;
                pixel |= *((uint8_t *)char_ptr);
                char_ptr++;
            }

            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);

            if (0xFF != a)
            {
                return SDL_TRUE;
            }
        }
        row_ptr += pitch;
    }

    return SDL_FALSE;
}

//--------------------------------------------------------------------------------------------
bool IMG_test_alpha_key(SDL_Surface *surface, Uint32 key)
{
    // test to see whether an image requires alpha blending
    if (NULL == surface) return SDL_FALSE;

    // save this alias
    SDL_PixelFormat *format = surface->format;

    // If there is no key specified (or the image has an alpha channel), use the basic version.
    if (INVALID_KEY == key || format->Aloss < 8)
    {
        return IMG_test_alpha(surface);
    }

    // if the surface is tagged as having an alpha value,
    // it is partially transparent
    if ( 0xff != format->alpha ) return SDL_TRUE;

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
            Uint32 pixel = (*ui32_ptr)&bitMask;

            if (pixel == key)
            {
                return SDL_TRUE;
            }
            else
            {
                Uint8 r, g, b, a;
                SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
                if (0xFF != a)
                {
                    return SDL_TRUE;
                }
            }

            // advance to the next entry
            char_ptr += bytesPerPixel;
        }
        row_ptr += pitch;
    }

    return SDL_FALSE;
}

//--------------------------------------------------------------------------------------------
GLuint oglx_texture_t::load(oglx_texture_t *self, const char *name, SDL_Surface *image, Uint32 key)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (VALID_TEXTURE(self))
    {
        // Release any old texture.
        oglx_texture_t::release(self);
    }
    else
    {
        // Clean out any uninitialied data.
        self = oglx_texture_t::ctor(self);
        if (!self) return INVALID_GL_ID;
    }
    if (!image)
    {
        return INVALID_GL_ID;
    }

    // Test to see if the image requires alpha blanding.
    self->has_alpha = SDL_FALSE;
    if (INVALID_KEY == key)
    {
        self->has_alpha = IMG_test_alpha(image) ? SDL_TRUE : SDL_FALSE;
    }
    else
    {
        self->has_alpha = IMG_test_alpha_key(image, key) ? SDL_TRUE : SDL_FALSE;
    }

    // Upload the SDL_surface to OpenGL.
    GLuint retval = oglx_texture_t::convert(self, image, key);

    if (!VALID_BINDING(retval))
    {
        oglx_texture_t::dtor(self);
    }
    else
    {
        strncpy(self->name, name, SDL_arraysize(self->name));

        self->base.wrap_s = GL_REPEAT;
        self->base.wrap_t = GL_REPEAT;
    }

    return retval;
}

GLuint oglx_texture_t::load(oglx_texture_t *self, const char *filename, Uint32 key)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (VALID_TEXTURE(self))
    {
        // Release any old texture.
        oglx_texture_t::release(self);
    }
    else
    {
        // Clean out any uninitialied data.
        self = oglx_texture_t::ctor(self);
        if (!self)
        {
            return INVALID_GL_ID;
        }
    }

    SDL_Surface *image = IMG_Load_RW(vfs_openRWopsRead(filename), 1);
    return load(self, filename, image, key);
}

//--------------------------------------------------------------------------------------------
GLuint  oglx_texture_t::getTextureID(const oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    return !VALID_TEXTURE(self) ? INVALID_GL_ID : self->base.binding;
}

//--------------------------------------------------------------------------------------------
GLsizei  oglx_texture_t::getImageHeight(const oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    return !VALID_TEXTURE(self) ? 0 : self->imgH;
}

//--------------------------------------------------------------------------------------------
GLsizei  oglx_texture_t::getImageWidth(const oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    return !VALID_TEXTURE(self) ? 0 : self->imgW;
}

//--------------------------------------------------------------------------------------------
GLsizei  oglx_texture_t::getTextureWidth(const oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    return !VALID_TEXTURE(self) ? 0 : self->base.width;
}

//--------------------------------------------------------------------------------------------
GLsizei  oglx_texture_t::getTextureHeight(const oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    return !VALID_TEXTURE(self) ? 0 : self->base.height;
}

//--------------------------------------------------------------------------------------------
GLboolean oglx_texture_t::getSize(const oglx_texture_t *self, oglx_frect_t tx_rect, oglx_frect_t img_rect)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    GLboolean retval = GL_FALSE;

    {
        if ( NULL != tx_rect )
        {
            // calculate the texture rectangle
            tx_rect[0] = 0.0f;
            tx_rect[1] = 0.0f;
            tx_rect[2] = (0 == self->base.width) ? 1.0f : (GLfloat)self->imgW / (GLfloat)self->base.width;
            tx_rect[3] = (0 == self->base.height) ? 1.0f : (GLfloat)self->imgH / (GLfloat)self->base.height;

            // clamp the values
            if ( tx_rect[2] > 1.0f ) tx_rect[2] = 1.0f;
            if ( tx_rect[2] < 0.0f ) tx_rect[2] = 0.0f;

            if ( tx_rect[3] > 1.0f ) tx_rect[3] = 1.0f;
            if ( tx_rect[3] < 0.0f ) tx_rect[3] = 0.0f;

            retval = GL_TRUE;
        }

        if ( NULL != img_rect )
        {
            img_rect[0] = 0.0f;
            img_rect[1] = 0.0f;
            img_rect[2] = (GLfloat)self->imgW;
            img_rect[3] = (GLfloat)self->imgH;

            retval = GL_TRUE;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void  oglx_texture_t::release(oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!VALID_TEXTURE(self))
    {
        return;
    }

    // Delete any existing SDL surface.
    if (self->surface)
    {
        SDL_FreeSurface(self->surface);
        self->surface = nullptr;
    }

    // Try to get rid of any stored texture data for this texture.
    GL_DEBUG(glDeleteTextures)(1, &(self->base.binding));
    self->base.binding = INVALID_GL_ID;

    // Make sure the error texture exists.
    ErrorTexture_create();

    // Generate a new texture binding.
    GL_DEBUG(glGenTextures)(1, &(self->base.binding));

    // Bind the error texture instead of the old texture.
    ErrorImage_bind(self->base.target, self->base.binding);

    // Reset the other data
    self->imgW = self->base.width = _errorImage.getWidth();
    self->imgH = self->base.height = _errorImage.getHeight();
    strncpy(self->name, "ErrorImage", sizeof(self->name));

    // Set wrapping for s and t coordinates to clamped.
    self->base.wrap_s = GL_CLAMP;
    self->base.wrap_t = GL_CLAMP;
}

//--------------------------------------------------------------------------------------------
void oglx_texture_t::bind( oglx_texture_t *texture )
{
    /// @author BB
    /// @details a oglx_texture_t wrapper for oglx_bind_to_tex_params() function

    GLenum target;
    GLuint id;
    GLint wrap_s, wrap_t;

    // Make sure the error texture exists.
    ErrorTexture_create();

    // assume the texture is going to be the error texture
    target = GL_TEXTURE_2D;
    wrap_s = wrap_t = GL_REPEAT;
    id = ErrorTexture_binding;

    if ( NULL == texture )
    {
        // NULL texture means white blob
        id = INVALID_GL_ID;
    }
    else if ( VALID_TEXTURE( texture ) && VALID_BINDING( texture->base.binding ) )
    {
        // grab the info from the texture
        target = texture->base.target;
        id     = texture->base.binding;
        wrap_s = texture->base.wrap_s;
        wrap_t = texture->base.wrap_t;
    }

    // upload the texture
    id = oglx_bind_to_tex_params( id, target, wrap_s, wrap_t );

    // if the texture binding changed, upload the change.
    if ( VALID_TEXTURE( texture ) )
    {
        texture->base.binding = id;
    }
}

//--------------------------------------------------------------------------------------------
GLboolean oglx_texture_Valid(oglx_texture_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    return VALID_TEXTURE(self);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
GLuint oglx_bind_to_tex_params( GLuint binding, GLenum target, GLint wrap_s, GLint wrap_t )
{
    GLuint local_binding;

    // Make sure the error texture exists.
    ErrorTexture_create();

    // handle default parameters
    if ( wrap_s < 0 ) wrap_s = GL_REPEAT;
    if ( wrap_t < 0 ) wrap_t = GL_REPEAT;

    local_binding = VALID_BINDING( binding ) ? binding : ErrorTexture_binding;

    auto filt_type  = g_ogl_textureParameters.textureFiltering;
    auto anisotropy = g_ogl_textureParameters.anisotropyLevel;

    if ( !GL_DEBUG( glIsEnabled )( target ) )
    {
        GL_DEBUG( glEnable )( target );
    };

    if (filt_type >= Ego::TextureFilter::ANISOTROPIC)
    {
        // Anisotropic filtered!
        oglx_bind( target, local_binding, wrap_s, wrap_t, GL_LINEAR, GL_LINEAR, anisotropy );
    }
    else
    {
        switch ( filt_type )
        {
                // Unfiltered
            case Ego::TextureFilter::UNFILTERED:
                oglx_bind( target, local_binding, wrap_s, wrap_t, GL_NEAREST, GL_LINEAR, 0 );
                break;

                // Linear filtered
            case Ego::TextureFilter::LINEAR:
                oglx_bind( target, local_binding, wrap_s, wrap_t, GL_LINEAR, GL_LINEAR, 0 );
                break;

                // Bilinear interpolation
            case Ego::TextureFilter::MIPMAP:
                oglx_bind( target, local_binding, wrap_s, wrap_t, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR, 0 );
                break;

                // Bilinear interpolation
            case Ego::TextureFilter::BILINEAR:
                oglx_bind( target, local_binding, wrap_s, wrap_t, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, 0 );
                break;

                // Trilinear filtered (quality 1)
            case Ego::TextureFilter::TRILINEAR_1:
                oglx_bind( target, local_binding, wrap_s, wrap_t, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR, 0 );
                break;

                // Trilinear filtered (quality 2)
            case Ego::TextureFilter::TRILINEAR_2:
                oglx_bind( target, local_binding, wrap_s, wrap_t, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, 0 );
                break;
        };
    }

    return local_binding;
}
