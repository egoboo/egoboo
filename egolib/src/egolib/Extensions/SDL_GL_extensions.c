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
#include "egolib/Extensions/ogl_debug.h"
#include "egolib/Extensions/ogl_texture.h"
#include "egolib/Math/_Include.hpp"

//--------------------------------------------------------------------------------------------
// - Global function stolen from Jonathan Fisher
// - who stole it from gl_font.c test program from SDL_ttf ;)
//--------------------------------------------------------------------------------------------
SDL_bool SDL_GL_uploadSurface( SDL_Surface *surface, GLuint tx_id, GLfloat *texCoords )
{
    int tx_w, tx_h;

    GLfloat local_texCoords[4];

    if ( NULL == surface ) return SDL_FALSE;

    // handle the optional parameters
    if ( NULL == texCoords ) texCoords = local_texCoords;

    // Use the surface width & height expanded to the next powers of two
    tx_w = Ego::Math::powerOfTwo( surface->w );
    tx_h = Ego::Math::powerOfTwo( surface->h );

    texCoords[0] = 0.0f;
    texCoords[1] = 0.0f;
    texCoords[2] = ( GLfloat )surface->w / ( GLfloat )tx_w;
    texCoords[3] = ( GLfloat )surface->h / ( GLfloat )tx_h;

    // use the default wrap parameters
    SDL_GL_convert_surface( tx_id, surface, -1, -1 );

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
SDL_bool SDL_GL_set_gl_mode( oglx_video_parameters_t * v )
{
    /// @author BB
    /// @details this function applies OpenGL settings. Must have a valid SDL_Surface to do any good.

    if ( NULL == v || !SDL_WasInit( SDL_INIT_VIDEO ) ) return SDL_FALSE;

    oglx_Get_Screen_Info(&g_ogl_caps);

    if ( v->multisample_arb )
    {
        GL_DEBUG( glDisable )( GL_MULTISAMPLE );
        GL_DEBUG( glEnable )( GL_MULTISAMPLE_ARB );
    }
    else if ( v->multisample )
    {
        GL_DEBUG( glEnable )( GL_MULTISAMPLE );
    }
    else
    {
        GL_DEBUG( glDisable )( GL_MULTISAMPLE );
        GL_DEBUG( glDisable )( GL_MULTISAMPLE_ARB );
    }

    // Enable perspective correction?
    GL_DEBUG( glHint )( GL_PERSPECTIVE_CORRECTION_HINT, v->perspective );

    // Enable dithering?
    if ( v->dither ) GL_DEBUG( glEnable )( GL_DITHER );
    else GL_DEBUG( glDisable )( GL_DITHER );

    // Enable Gouraud shading? (Important!)
    GL_DEBUG( glShadeModel )( v->shading );

    // Enable antialiasing?
    if ( v->antialiasing )
    {
        GL_DEBUG(glEnable)(GL_LINE_SMOOTH);
        GL_DEBUG(glHint)(GL_LINE_SMOOTH_HINT, GL_NICEST);

        GL_DEBUG(glEnable)(GL_POINT_SMOOTH );
        GL_DEBUG(glHint)(GL_POINT_SMOOTH_HINT, GL_NICEST);

        GL_DEBUG(glDisable)(GL_POLYGON_SMOOTH );
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
        GL_DEBUG( glDisable )( GL_POINT_SMOOTH );
        GL_DEBUG( glDisable )( GL_LINE_SMOOTH );
        GL_DEBUG( glDisable )( GL_POLYGON_SMOOTH );
    }

    // anisotropic filtering
    if ( v->userAnisotropy > 1.0f )
    {
        // limit the userAnisotropy top be in a valid range
        if (v->userAnisotropy > g_ogl_caps.maxAnisotropy)
        {
            v->userAnisotropy = g_ogl_caps.maxAnisotropy;
        }

        GL_DEBUG( glTexParameterf )( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, v->userAnisotropy );
    };

    // fill mode
    GL_DEBUG( glPolygonMode )( GL_FRONT, GL_FILL );
    GL_DEBUG( glPolygonMode )( GL_BACK,  GL_FILL );

    /* Disable OpenGL lighting */
    GL_DEBUG( glDisable )( GL_LIGHTING );

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
void SDL_GL_report_mode( SDLX_video_parameters_t * retval )
{
    SDL_Surface * surface = ( NULL == retval ) ? NULL : retval->surface;

    SDLX_report_mode( surface, retval );

    if ( NULL != retval && retval->flags.opengl )
    {
        oglx_report_caps();
    }
}

//--------------------------------------------------------------------------------------------
SDLX_video_parameters_t * SDL_GL_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, oglx_video_parameters_t * gl_new, SDL_bool has_valid_mode )
{
    /// @author BB
    /// @details let SDL_GL try to set a new video mode.

    SDLX_video_parameters_t param_old;
    SDLX_video_parameters_t * retval = NULL;

    // initialize v_old and param_old
    if ( has_valid_mode )
    {
        if ( NULL == v_old )
        {
            SDLX_video_parameters_t::defaults(&param_old);
            v_old = &param_old;
        }
        else
        {
            memcpy( &param_old, v_old, sizeof( SDLX_video_parameters_t ) );
        }
    }
    else
    {
        v_old = NULL;
    }

    // use the sdl extensions to set the SDL video mode
    retval = SDLX_set_mode( v_old, v_new, has_valid_mode, SDL_FALSE );

    if ( NULL != retval )
    {
        // report on the success or failure to set the mode
        SDL_GL_report_mode( retval );

        // set the opengl parameters
        gl_new->multisample     = GL_FALSE;
        gl_new->multisample_arb = GL_FALSE;
        if ( NULL != retval->surface && retval->flags.opengl )
        {
            // correct the multisampling
            gl_new->multisample_arb = retval->gl_att.multi_samples > 1;

            SDL_GL_set_gl_mode( gl_new );
        }
    }

    return retval;
}

SDL_Surface *SDL_GL_convert_surface_2(SDL_Surface *surface)
{
    if (!surface)
    {
        throw std::invalid_argument("nullptr == surface");
    }

    // Aliases old surface.
    SDL_Surface *oldSurface = surface;

    // Alias old width and old height.
    int oldWidth = surface->w,
        oldHeight = surface->h;

    // Compute new width and new height.
    int newWidth = Ego::Math::powerOfTwo(oldWidth);
    int newHeight = Ego::Math::powerOfTwo(oldHeight);

    // Compute new pixel format (R8G8B8A8).
    SDL_PixelFormat newFormat = *(SDL_GetVideoSurface()->format);

    using PixelDescriptor = PixelDescriptor<Ego::PixelFormat::R8G8B8A8>;
    newFormat.Amask = PixelDescriptor::a_mask();
    newFormat.Ashift = PixelDescriptor::a_shift();
    newFormat.Aloss = 0;

    newFormat.Bmask = PixelDescriptor::b_mask();
    newFormat.Bshift = PixelDescriptor::b_shift();
    newFormat.Bloss = 0;

    newFormat.Gmask = PixelDescriptor::g_mask();
    newFormat.Gshift = PixelDescriptor::g_shift();
    newFormat.Gloss = 0;

    newFormat.Rmask = PixelDescriptor::r_mask();
    newFormat.Rshift = PixelDescriptor::r_shift();
    newFormat.Rloss = 0;

    newFormat.BitsPerPixel = 32;
    newFormat.BytesPerPixel = 4;

    // Convert to new format.
    SDL_Surface *tmp = SDL_ConvertSurface(oldSurface, &newFormat, SDL_SWSURFACE);
    if (!tmp)
    {
        throw std::runtime_error("unable to convert surface");
    }
    oldSurface = tmp;

    // If the alpha mask is non-zero (which is the case here), then SDL_ConvertSurface
    // behaves "as if" the addition flag @a SDL_SRCALPHA was supplied. That is, if
    // the surface is blitted onto another surface, then alpha blending is performed:
    // This is not desired by us as we want to set each pixel in the target surface to
    // the value of the corresponding pixel in the source surface i.e. we do not want
    // alpha blending to be done: We turn of alpha blending by a calling
    // <tt>SetAlpha(oldSurface, 0, SDL_ALPHA_OPAQUE)</tt>.
    SDL_SetAlpha(oldSurface, 0, SDL_ALPHA_OPAQUE);
    // We do not want colour-keying to be performed either: Wether it is activated or not,
    // turn it off by the call <tt>SDL_SetColorKey(oldSurface, 0, 0)</tt>.
    SDL_SetColorKey(oldSurface, 0, 0);
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
            SDL_FreeSurface(oldSurface);
            throw std::runtime_error("unable to convert surface");
        }
        SDL_BlitSurface(oldSurface, &(oldSurface->clip_rect), tmp, nullptr);
        /** @todo Handle errors of SDL_BitSurface. */
        oldSurface = tmp;
    }

    // Return the result.
    return oldSurface;
}
//--------------------------------------------------------------------------------------------
GLuint SDL_GL_convert_surface( GLenum binding, SDL_Surface * surface, GLint wrap_s, GLint wrap_t )
{
    int srf_w, srf_h, tx_w, tx_h;
    SDL_Surface *screen,
		        *local_surface;
    SDL_PixelFormat tmpformat;
    bool use_alpha;
    GLenum target;

    // Bind the error texture instead of the old texture
    ErrorImage_bind( GL_TEXTURE_2D, binding );

    if ( NULL == surface ) return INVALID_GL_ID;
    local_surface = surface;

    // handle default parameters
    if ( wrap_s < 0 ) wrap_s = GL_REPEAT;
    if ( wrap_t < 0 ) wrap_t = GL_REPEAT;

    /* Set the original local_surface's size (incase it's not an exact square of a power of two) */
    srf_h = local_surface->h;
    srf_w = local_surface->w;

    // adjust the texture target
    target = (( 1 == local_surface->h ) && ( local_surface->w > 1 ) ) ? GL_TEXTURE_1D : GL_TEXTURE_2D;

    /* Determine the correct power of two greater than or equal to the original local_surface's size */
    tx_h = Ego::Math::powerOfTwo( local_surface->h );
    tx_w = Ego::Math::powerOfTwo( local_surface->w );

    screen  = SDL_GetVideoSurface();
    memcpy( &tmpformat, screen->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format

    //if( ogl_caps.alpha_bits > 0 )
    {
        // convert the local_surface to a 32-bit pixel format
        using pixelDescriptor = PixelDescriptor < Ego::PixelFormat::R8G8B8A8 > ;
        tmpformat.Amask = pixelDescriptor::a_mask();
        tmpformat.Ashift = pixelDescriptor::a_shift();
        tmpformat.Aloss = 0;

        tmpformat.Bmask = pixelDescriptor::b_mask();
        tmpformat.Bshift = pixelDescriptor::b_shift();
        tmpformat.Bloss = 0;

        tmpformat.Gmask = pixelDescriptor::g_mask();
        tmpformat.Gshift = pixelDescriptor::g_shift();
        tmpformat.Gloss = 0;

        tmpformat.Rmask = pixelDescriptor::r_mask();
        tmpformat.Rshift = pixelDescriptor::r_shift();
        tmpformat.Rloss = 0;

        // make the pixel size match the screen format
        tmpformat.BitsPerPixel  = 32;
        tmpformat.BytesPerPixel = 4;
    }
    {
        SDL_Surface * tmp;
        Uint32 convert_flags;

        // convert the local_surface format to the correct format
        convert_flags = SDL_SWSURFACE;
        tmp = SDL_ConvertSurface( local_surface, &tmpformat, convert_flags );
        if ( local_surface != surface ) SDL_FreeSurface( local_surface );
        local_surface = tmp;

        // fix the alpha channel on the new SDL_Surface.  For some reason, SDL wants to create
        // local_surface with local_surface->format->alpha==0x00 which causes a problem if we have to
        // use the SDL_BlitSurface() function below.  With the local_surface alpha set to zero, the
        // local_surface will be converted to BLACK!
        //
        // The following statement tells SDL
        //  1) to set the local_surface to opaque
        //  2) not to alpha blend the local_surface on blit
        SDL_SetAlpha( local_surface, 0, SDL_ALPHA_OPAQUE );
        SDL_SetColorKey( local_surface, 0, 0 );
    }

    // create a ptex that is acceptable to OpenGL (height and width are powers of 2)
    if ( srf_h != tx_h || srf_w != tx_w )
    {
        SDL_Surface * tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, tx_w, tx_h, tmpformat.BitsPerPixel, tmpformat.Rmask, tmpformat.Gmask, tmpformat.Bmask, tmpformat.Amask );

        SDL_BlitSurface( local_surface, &( local_surface->clip_rect ), tmp, NULL );
        if ( local_surface != surface ) SDL_FreeSurface( local_surface );
        local_surface = tmp;
    };

    /* Generate an OpenGL texture ID */
    if ( !VALID_BINDING( binding ) || ERROR_IMAGE_BINDING( binding ) )
    {
        GL_DEBUG( glGenTextures )( 1, &binding );
    }

    /* Set up some parameters for the format of the oglx_texture_t */
    binding = oglx_bind_to_tex_params( binding, target, wrap_s, wrap_t );

    /* actually create the OpenGL textures */
    use_alpha = !( 8 == local_surface->format->Aloss );
    if ( target == GL_TEXTURE_2D )
    {
        if (g_ogl_textureParameters.textureFiltering >= Ego::TextureFilter::MIPMAP)
        {
            oglx_upload_2d_mipmap( use_alpha, local_surface->w, local_surface->h, local_surface->pixels );
        }
        else
        {
            oglx_upload_2d( use_alpha, local_surface->w, local_surface->h, local_surface->pixels );
        }
    }
    else if ( target == GL_TEXTURE_1D )
    {
        oglx_upload_1d( use_alpha, local_surface->w, local_surface->pixels );
    }
    else
    {
        EGOBOO_ASSERT( 0 );
    }

    if ( local_surface != surface )  SDL_FreeSurface( local_surface );

    return binding;
}
