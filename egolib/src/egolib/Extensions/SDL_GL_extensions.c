//********************************************************************************************
//*
//*    This file is part of the SDL extensions library. This library is
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

/// @file egolib/Extensions/SDL_GL_extensions.c
/// @ingroup _sdl_extensions_
/// @brief Implementation of the OpenGL extensions to SDL
/// @details

#include "egolib/Extensions/SDL_GL_extensions.h"
#include "egolib/Renderer/TextureFilter.hpp"
#include "egolib/Renderer/Texture.hpp"
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Math/_Include.hpp"
#include "egolib/Image/ImageManager.hpp"
#include "egolib/Graphics/PixelFormat.hpp"

SDL_Surface *SDL_GL_createSurface(int w, int h)
{

    // Expand the screen format to support alpha:
    // a) Copy the format of the main surface.
    SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);

    SDL_Surface *ret = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, format->BitsPerPixel,
                                            format->Rmask, format->Gmask, format->Bmask, format->Amask);
    SDL_FreeFormat(format);
    return ret;
}

//--------------------------------------------------------------------------------------------
bool SDL_GL_set_gl_mode(oglx_video_parameters_t * v)
{
    /// @author BB
    /// @details this function applies OpenGL settings. Must have a valid SDL_Surface to do any good.

    if (NULL == v || !SDL_WasInit(SDL_INIT_VIDEO)) return false;

    oglx_Get_Screen_Info(&g_ogl_caps);

    if (v->multisample_arb)
    {
        GL_DEBUG(glDisable)(GL_MULTISAMPLE);
        GL_DEBUG(glEnable)(GL_MULTISAMPLE_ARB);
    }
    else if (v->multisample)
    {
        GL_DEBUG(glEnable)(GL_MULTISAMPLE);
    }
    else
    {
        GL_DEBUG(glDisable)(GL_MULTISAMPLE);
        GL_DEBUG(glDisable)(GL_MULTISAMPLE_ARB);
    }

    // Enable perspective correction?
    GL_DEBUG(glHint)(GL_PERSPECTIVE_CORRECTION_HINT, v->perspective);

    // Enable dithering?
    if (v->dither) GL_DEBUG(glEnable)(GL_DITHER);
    else GL_DEBUG(glDisable)(GL_DITHER);

    // Enable Gouraud shading? (Important!)
    GL_DEBUG(glShadeModel)(v->shading);

    // Enable antialiasing?
    if (v->antialiasing)
    {
        GL_DEBUG(glEnable)(GL_LINE_SMOOTH);
        GL_DEBUG(glHint)(GL_LINE_SMOOTH_HINT, GL_NICEST);

        GL_DEBUG(glEnable)(GL_POINT_SMOOTH);
        GL_DEBUG(glHint)(GL_POINT_SMOOTH_HINT, GL_NICEST);

        GL_DEBUG(glDisable)(GL_POLYGON_SMOOTH);
        GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

        // PLEASE do not turn this on unless you use
        // @code
        // Ego::Renderer::get().setBlendingEnabled(true);
        // GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // @endcode
        // before every single draw command.
        //
        // GL_DEBUG(glEnable)(GL_POLYGON_SMOOTH);
        // GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        GL_DEBUG(glDisable)(GL_POINT_SMOOTH);
        GL_DEBUG(glDisable)(GL_LINE_SMOOTH);
        GL_DEBUG(glDisable)(GL_POLYGON_SMOOTH);
    }

    // Disable anisotropic filtering if it is not supported.
    v->anisotropy_enable &= g_ogl_caps.anisotropic_supported;
    // However, always bound the values to valid ranges.
    v->anisotropy_levels = Ego::Math::constrain(v->anisotropy_levels, 1.0f, g_ogl_caps.maxAnisotropy);
    if (v->anisotropy_enable && v->anisotropy_levels > 1.0f)
    {
        GL_DEBUG(glTexParameterf)(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, v->anisotropy_levels);
    };

    // fill mode
    GL_DEBUG(glPolygonMode)(GL_FRONT, GL_FILL);
    GL_DEBUG(glPolygonMode)(GL_BACK, GL_FILL);

    /* Disable OpenGL lighting */
    GL_DEBUG(glDisable)(GL_LIGHTING);

    return true;
}

//--------------------------------------------------------------------------------------------
void SDL_GL_report_mode(SDLX_video_parameters_t * retval)
{
    SDL_Window * surface = (NULL == retval) ? NULL : retval->surface;

    SDLX_report_mode(surface, retval);

    if (NULL != retval && retval->flags.opengl)
    {
        oglx_report_caps();
    }
}

//--------------------------------------------------------------------------------------------
SDLX_video_parameters_t * SDL_GL_set_mode(SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, oglx_video_parameters_t * gl_new, bool has_valid_mode)
{
    /// @author BB
    /// @details let SDL_GL try to set a new video mode.

    SDLX_video_parameters_t param_old;
    SDLX_video_parameters_t * retval = NULL;

    // initialize v_old and param_old
    if (has_valid_mode)
    {
        if (NULL == v_old)
        {
            SDLX_video_parameters_t::defaults(&param_old);
            v_old = &param_old;
        }
        else
        {
            memcpy(&param_old, v_old, sizeof(SDLX_video_parameters_t));
        }
    }
    else
    {
        v_old = NULL;
    }

    // use the sdl extensions to set the SDL video mode
    retval = SDLX_set_mode(v_old, v_new, has_valid_mode, false);

    if (NULL != retval)
    {
        // report on the success or failure to set the mode
        SDL_GL_report_mode(retval);

        // set the opengl parameters
        gl_new->multisample = GL_FALSE;
        gl_new->multisample_arb = GL_FALSE;
        if (NULL != retval->surface && retval->flags.opengl)
        {
            // correct the multisampling
            gl_new->multisample_arb = retval->gl_att.multi_samples > 1;

            SDL_GL_set_gl_mode(gl_new);
        }
    }

    return retval;
}

Uint32 SDL_GL_getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
            break;

        case 2:
            return *(Uint16 *)p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(Uint32 *)p;
            break;

        default:
            throw std::runtime_error("unreachable code reached"); /* shouldn't happen, but avoids warnings */
    }
}

void SDL_GL_putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            *p = pixel;
            break;

        case 2:
            *(Uint16 *)p = pixel;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            }
            else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;

        case 4:
            *(Uint32 *)p = pixel;
            break;
        default:
            throw std::runtime_error("unreachable code reached"); /* shouldn't happen, but avoids warnings */
    }
}


std::shared_ptr<SDL_Surface> SDL_GL_convert(std::shared_ptr<SDL_Surface> surface, const Ego::PixelFormatDescriptor& pixelFormatDescriptor)
{
    if (!surface)
    {
        throw std::invalid_argument("nullptr == surface");
    }
    uint32_t Amask, Bmask, Gmask, Rmask;
    int bpp;

    Amask = pixelFormatDescriptor.getAlphaMask();
    Bmask = pixelFormatDescriptor.getBlueMask();
    Gmask = pixelFormatDescriptor.getGreenMask();
    Rmask = pixelFormatDescriptor.getRedMask();
    bpp = pixelFormatDescriptor.getBitsPerPixel();
    
    uint32_t newFormat = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
    if (newFormat == SDL_PIXELFORMAT_UNKNOWN) {
        throw std::invalid_argument("pixelFormatDescriptor doesn't correspond with a SDL_PixelFormat");
    }

    SDL_Surface *newSurface = SDL_ConvertSurfaceFormat(surface.get(), newFormat, 0);
    if (!newSurface)
    {
        throw std::runtime_error("unable to convert surface");
    }

    return std::shared_ptr<SDL_Surface>(newSurface, [ ](SDL_Surface *surface) { SDL_FreeSurface(surface); });
}

Ego::PixelFormatDescriptor SDL_GL_fromSDL(const SDL_PixelFormat& source)
{
    if (source.palette)
    {
        throw std::runtime_error("pixel format not supported");
    }
    const Ego::PixelFormatDescriptor pfds [] =
        { Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>(),
          Ego::PixelFormatDescriptor::get<Ego::PixelFormat::B8G8R8A8>(),
          Ego::PixelFormatDescriptor::get<Ego::PixelFormat::B8G8R8>(),
          Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8>() };
    for (size_t i = 0; i < 4; ++i)
    {
        if (source.BytesPerPixel == pfds[i].getBitsPerPixel()/8 &&
            source.Amask == pfds[i].getAlphaMask() &&
            source.Rmask == pfds[i].getRedMask() &&
            source.Gmask == pfds[i].getGreenMask() &&
            source.Bmask == pfds[i].getBlueMask() &&
            source.Ashift == pfds[i].getAlphaShift() &&
            source.Rshift == pfds[i].getRedShift() &&
            source.Gshift == pfds[i].getGreenShift() &&
            source.Bshift == pfds[i].getBlueShift() &&
            source.BitsPerPixel == pfds[i].getBitsPerPixel())
        {
            return pfds[i];
        }
    }
    throw std::runtime_error("pixel format not supported");
}

std::shared_ptr<SDL_Surface> SDL_GL_pad(std::shared_ptr<SDL_Surface> surface, size_t left, size_t right, size_t top, size_t bottom)
{
    if (!surface || surface->w < 0 || surface->h < 0)
    {
        throw std::invalid_argument("nullptr == surface || surface->w < 0 || surface->h < 0");
    }
    if (left == 0 && right == 0 && top == 0 && bottom == 0)
    {
        return surface;
    }
    // Alias old width and old height.
    size_t oldWidth  = surface->w,
           oldHeight = surface->h;

    // Compute new width and new height.
    size_t newWidth = oldWidth + left + right,
           newHeight = oldHeight + top + bottom;

    // Create a new surface.
    const auto& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>();
    auto temporary = ImageManager::get().createImage(newWidth, newHeight, pixelFormatDescriptor);

    // Fill the surface with transparent black.
    SDL_FillRect(temporary.get(), nullptr, SDL_MapRGBA(temporary->format, 0, 0, 0, 0));
    for (size_t y = 0; y < oldHeight; ++y)
    {
        for (size_t x = 0; x < oldWidth; ++x)
        {
            uint32_t p = SDL_GL_getpixel(surface.get(), x, y);
            uint8_t r, g, b, a;
            SDL_GetRGBA(p, surface->format, &r, &g, &b, &a);
            uint32_t q = SDL_MapRGBA(temporary->format, r, g, b, a);
            SDL_GL_putpixel(temporary.get(), left + x, top + y, q);
        }
    }

    // Return the temporary surface.
    return temporary;
}

std::shared_ptr<SDL_Surface> SDL_GL_convert(std::shared_ptr<SDL_Surface> surface)
{
    const auto& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>();
    // (1) Perform pixel format conversion.
    surface = SDL_GL_convert(surface, pixelFormatDescriptor);
    if (!surface)
    {
        throw std::runtime_error("unable to apply transformation: pixel format conversion");
    }

    // (2) Perform padding to power of two if necessary.

    // Alias old width and old height.
    int oldWidth = surface->w,
        oldHeight = surface->h;

    // Compute new width and new height.
    int newWidth = Ego::Math::powerOfTwo(oldWidth),
        newHeight = Ego::Math::powerOfTwo(oldHeight);

    // Only if the new dimension differ from the old dimensions, perform the scaling.
    if (newWidth != oldWidth || newHeight != oldHeight)
    {
        surface = SDL_GL_pad(surface, 0, newWidth - oldWidth, 0, newHeight - oldHeight);
    }

    // (3) Return the result.
    return surface;
}
