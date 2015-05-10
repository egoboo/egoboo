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
#include "egolib/Graphics/PixelFormat.hpp"

SDL_Surface *SDL_GL_createSurface(int w, int h)
{
    SDL_Surface *primarySurface = SDL_GetVideoSurface();
    if (!primarySurface)
    {
        return nullptr;
    }

    // Expand the screen format to support alpha:
    // a) Copy the format of the main surface.
    SDL_PixelFormat format;
    memcpy(&format, primarySurface->format, sizeof(SDL_PixelFormat));
    // b) Expand the format.
    SDLX_ExpandFormat(&format);

    return SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, format.BitsPerPixel,
                                format.Rmask, format.Gmask, format.Bmask, format.Amask);
}

//--------------------------------------------------------------------------------------------
SDL_bool SDL_GL_set_gl_mode(oglx_video_parameters_t * v)
{
    /// @author BB
    /// @details this function applies OpenGL settings. Must have a valid SDL_Surface to do any good.

    if (NULL == v || !SDL_WasInit(SDL_INIT_VIDEO)) return SDL_FALSE;

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

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
void SDL_GL_report_mode(SDLX_video_parameters_t * retval)
{
    SDL_Surface * surface = (NULL == retval) ? NULL : retval->surface;

    SDLX_report_mode(surface, retval);

    if (NULL != retval && retval->flags.opengl)
    {
        oglx_report_caps();
    }
}

//--------------------------------------------------------------------------------------------
SDLX_video_parameters_t * SDL_GL_set_mode(SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, oglx_video_parameters_t * gl_new, SDL_bool has_valid_mode)
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
    retval = SDLX_set_mode(v_old, v_new, has_valid_mode, SDL_FALSE);

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

std::shared_ptr<SDL_Surface> SDL_GL_convert_surface(std::shared_ptr<SDL_Surface> surface)
{
    if (!surface)
    {
        throw std::invalid_argument("nullptr == surface");
    }

    // Aliases old surface.
    std::shared_ptr<SDL_Surface> oldSurface = surface;

    // Alias old width and old height.
    int oldWidth = surface->w,
        oldHeight = surface->h;

    // Compute new width and new height.
    int newWidth = Ego::Math::powerOfTwo(oldWidth);
    int newHeight = Ego::Math::powerOfTwo(oldHeight);

    // Compute new pixel format (R8G8B8A8).
    SDL_PixelFormat newFormat = *(SDL_GetVideoSurface()->format);

    const auto& pixelFormatDescriptor = Ego::PixelFormatDescriptor::get<Ego::PixelFormat::R8G8B8A8>();
    newFormat.Amask = pixelFormatDescriptor.getAlphaMask();
    newFormat.Ashift = pixelFormatDescriptor.getAlphaShift();
    newFormat.Aloss = 0;

    newFormat.Bmask = pixelFormatDescriptor.getBlueMask();
    newFormat.Bshift = pixelFormatDescriptor.getBlueShift();
    newFormat.Bloss = 0;

    newFormat.Gmask = pixelFormatDescriptor.getGreenMask();
    newFormat.Gshift = pixelFormatDescriptor.getGreenShift();
    newFormat.Gloss = 0;

    newFormat.Rmask = pixelFormatDescriptor.getRedMask();
    newFormat.Rshift = pixelFormatDescriptor.getRedShift();
    newFormat.Rloss = 0;

    newFormat.BitsPerPixel = pixelFormatDescriptor.getBitsPerPixel();
    newFormat.BytesPerPixel = pixelFormatDescriptor.getBitsPerPixel() / sizeof(char);

    // Convert to new format.
    SDL_Surface *tmp = SDL_ConvertSurface(oldSurface.get(), &newFormat, SDL_SWSURFACE);
    if (!tmp)
    {
        throw std::runtime_error("unable to convert surface");
    }
    oldSurface.reset(tmp);

    // If the alpha mask is non-zero (which is the case here), then SDL_ConvertSurface
    // behaves "as if" the addition flag @a SDL_SRCALPHA was supplied. That is, if
    // the surface is blitted onto another surface, then alpha blending is performed:
    // This is not desired by us as we want to set each pixel in the target surface to
    // the value of the corresponding pixel in the source surface i.e. we do not want
    // alpha blending to be done: We turn of alpha blending by a calling
    // <tt>SetAlpha(oldSurface, 0, SDL_ALPHA_OPAQUE)</tt>.
    SDL_SetAlpha(oldSurface.get(), 0, SDL_ALPHA_OPAQUE);
    // We do not want colour-keying to be performed either: Wether it is activated or not,
    // turn it off by the call <tt>SDL_SetColorKey(oldSurface, 0, 0)</tt>.
    SDL_SetColorKey(oldSurface.get(), 0, 0);
    // For more information, see <a>http://sdl.beuc.net/sdl.wiki/SDL_CreateRGBSurface</a>.

    // Convert to new width and new height each a power of 2 as (old) OpenGL versions required it (we'r conservative).
    if (newWidth != oldWidth || newHeight != oldHeight)
    {
        SDL_Surface *tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, newWidth, newHeight,
                                                newFormat.BitsPerPixel,
                                                newFormat.Rmask,
                                                newFormat.Gmask,
                                                newFormat.Bmask,
                                                newFormat.Amask);
        if (!tmp)
        {
            throw std::runtime_error("unable to convert surface");
        }
        SDL_BlitSurface(oldSurface.get(), &(oldSurface->clip_rect), tmp, nullptr);
        /** @todo Handle errors of SDL_BitSurface. */
        oldSurface.reset(tmp);
    }

    // Return the result.
    return oldSurface;
}
