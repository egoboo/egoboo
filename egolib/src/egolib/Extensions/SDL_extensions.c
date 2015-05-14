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

/// @file egolib/Extensions/SDL_extensions.c
/// @ingroup _sdl_extensions_
/// @brief Implementation of generic extensions to SDL
/// @details

#include "egolib/Extensions/SDL_extensions.h"
#include "egolib/log.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

SDLX_screen_info_t sdl_scr;

// create the color masks
// this will work if both endian systems think they have "RGBA" graphics
// if you need a different pixel format (ARGB or BGRA or whatever) this section
// will have to be changed to reflect that

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
const Uint32 rmask = 0x000000ff;
const Uint32 gmask = 0x0000ff00;
const Uint32 bmask = 0x00ff0000;
const Uint32 amask = 0xff000000;
#else
const Uint32 rmask = 0xff000000;
const Uint32 gmask = 0x00ff0000;
const Uint32 bmask = 0x0000ff00;
const Uint32 amask = 0x000000ff;
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void SDLX_download_sdl_video_flags( Uint32 iflags, SDLX_sdl_video_flags_t * pflags );
static void SDLX_read_sdl_gl_attrib( SDLX_sdl_gl_attrib_t * patt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void SDLX_screen_info_t::report(SDLX_screen_info_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    log_message("\nSDL using video driver - %s\n", self->szDriver);

    if (nullptr != self->video_mode_list)
    {
        log_message("\tAvailable full-screen video modes...\n");
        for (size_t i = 0; nullptr != self->video_mode_list[i]; ++i)
        {
            log_message("    \tVideo Mode - %d x %d\n", self->video_mode_list[i]->w, self->video_mode_list[i]->h);
        }
    }
}

//--------------------------------------------------------------------------------------------
SDL_bool SDLX_Get_Screen_Info( SDLX_screen_info_t * psi, SDL_bool make_report )
{
    Uint32 init_flags = 0;
    SDL_Surface * ps;
    const SDL_VideoInfo * pvi;

    BLANK_STRUCT_PTR( psi )

    init_flags = SDL_WasInit( SDL_INIT_EVERYTHING );
    if ( 0 == init_flags )
    {
        if ( make_report ) log_message("ERROR: SDLX_Get_Screen_Info() called before initializing SDL\n");
        return SDL_FALSE;
    }
    else if ( HAS_NO_BITS( init_flags, SDL_INIT_VIDEO ) )
    {
        if ( make_report ) log_message("ERROR: SDLX_Get_Screen_Info() called before initializing SDL video driver\n");
        return SDL_FALSE;
    }

    ps  = SDL_GetVideoSurface();
    pvi = SDL_GetVideoInfo();

    // store the screen info for everyone to use
    psi->pscreen = ps;
    psi->d = ps->format->BitsPerPixel;
    psi->x = ps->w;
    psi->y = ps->h;

    // Grab all the available video modes
    psi->video_mode_list = SDL_ListModes( ps->format, ps->flags | SDL_FULLSCREEN );

    // log the video driver info
    SDL_VideoDriverName( psi->szDriver, sizeof( psi->szDriver ) );

    // grab all SDL_GL_* attributes
    SDLX_read_sdl_gl_attrib( &( psi->gl_att ) );

    // translate the surface flags into the bitfield
    SDLX_download_sdl_video_flags( ps->flags, &( psi->flags ) );

    psi->hw_available = pvi->hw_available;
    psi->wm_available = pvi->wm_available;
    psi->blit_hw      = pvi->blit_hw;
    psi->blit_hw_CC   = pvi->blit_hw_CC;
    psi->blit_hw_A    = pvi->blit_hw_A;
    psi->blit_sw      = pvi->blit_sw;
    psi->blit_sw_CC   = pvi->blit_sw_CC;
    psi->blit_sw_A    = pvi->blit_sw_A;

    if (make_report)
    {
        SDLX_screen_info_t::report(psi);
    }
    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void SDLX_sdl_gl_attrib_t::report(SDLX_sdl_gl_attrib_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    log_message("\nSDL_GL_Attribtes\n");

#if !defined(ID_LINUX)
    // Under Unix we cannot specify these, we just get whatever format
    // the framebuffer has, specifying depths > the framebuffer one
    // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"

    log_message("\tSDL_GL_RED_SIZE           == %d\n", self->color[0]);
    log_message("\tSDL_GL_GREEN_SIZE         == %d\n", self->color[1]);
    log_message("\tSDL_GL_BLUE_SIZE          == %d\n", self->color[2]);
    log_message("\tSDL_GL_ALPHA_SIZE         == %d\n", self->color[3]);
    log_message("\tSDL_GL_BUFFER_SIZE        == %d\n", self->buffer_size);
    log_message("\tSDL_GL_DEPTH_SIZE         == %d\n", self->depth_size);
#endif

    log_message("\tSDL_GL_DOUBLEBUFFER       == %d\n", self->doublebuffer);
    log_message("\tSDL_GL_STENCIL_SIZE       == %d\n", self->stencil_size);
    log_message("\tSDL_GL_ACCUM_RED_SIZE     == %d\n", self->accum[0]);
    log_message("\tSDL_GL_ACCUM_GREEN_SIZE   == %d\n", self->accum[1]);
    log_message("\tSDL_GL_ACCUM_BLUE_SIZE    == %d\n", self->accum[2]);
    log_message("\tSDL_GL_ACCUM_ALPHA_SIZE   == %d\n", self->accum[3]);
    log_message("\tSDL_GL_STEREO             == %d\n", self->stereo);

#if !defined(ID_LINUX)
    log_message("\tSDL_GL_MULTISAMPLEBUFFERS == %d\n", self->multi_buffers);
    log_message("\tSDL_GL_MULTISAMPLESAMPLES == %d\n", self->multi_samples);
    log_message("\tSDL_GL_ACCELERATED_VISUAL == %d\n", self->accelerated_visual);

    // Fedora 7 doesn't support SDL_GL_SWAP_CONTROL, but we use this nvidia extension instead.
    log_message("\tSDL_GL_SWAP_CONTROL       == %d\n", self->swap_control);
#endif
}

//--------------------------------------------------------------------------------------------
void SDLX_sdl_video_flags_t::report(SDLX_sdl_video_flags_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    log_message("\nSDL flags\n");

    log_message("    %s\n", self->full_screen ? "fullscreen" : "windowed");
    log_message("    %s\n", self->hw_surface ? "SDL hardware surface" : "SDL software surface");
    log_message("    %s\n", self->double_buf ? "SDL double buffer" : "SDL single buffer");

    if (self->opengl)
    {
        log_message("\tOpenGL support\n");
    }

    if (self->opengl_blit)
    {
        log_message("\tOpenGL-compatible blitting\n");
    }

    if (self->async_blit)
    {
        log_message("\tasynchronous blit\n");
    }

    if (self->any_format)
    {
        log_message("\tuse closest format\n");
    }

    if (self->hw_palette)
    {
        log_message("\texclusive palate access\n");
    }

    if (self->resizable)
    {
        log_message("\tresizable window\n");
    }

    if (self->no_frame)
    {
        log_message("\tno external frame\n");
    }
}

//--------------------------------------------------------------------------------------------
Uint32 SDLX_upload_sdl_video_flags( SDLX_sdl_video_flags_t flags )
{
    Uint32 ret = 0;

    ret |= flags.hw_surface  ? SDL_HWSURFACE  : 0;
    ret |= flags.async_blit  ? SDL_ASYNCBLIT  : 0;
    ret |= flags.any_format  ? SDL_ANYFORMAT  : 0;
    ret |= flags.hw_palette  ? SDL_HWPALETTE  : 0;
    ret |= flags.double_buf  ? SDL_DOUBLEBUF  : 0;
    ret |= flags.full_screen ? SDL_FULLSCREEN : 0;
    ret |= flags.opengl      ? SDL_OPENGL     : 0;
    ret |= flags.opengl_blit ? SDL_OPENGLBLIT : 0;
    ret |= flags.resizable   ? SDL_RESIZABLE  : 0;
    ret |= flags.no_frame    ? SDL_NOFRAME    : 0;

    // "read only"
    ret |= flags.use_hwaccel     ? SDL_HWACCEL     : 0;
    ret |= flags.has_srccolorkey ? SDL_SRCCOLORKEY : 0;
    ret |= flags.use_rleaccelok  ? SDL_RLEACCELOK  : 0;
    ret |= flags.use_rleaccel    ? SDL_RLEACCEL    : 0;
    ret |= flags.use_srcalpha    ? SDL_SRCALPHA    : 0;
    ret |= flags.is_prealloc     ? SDL_PREALLOC    : 0;

    return ret;
}

//--------------------------------------------------------------------------------------------
void SDLX_download_sdl_video_flags( Uint32 iflags, SDLX_sdl_video_flags_t * pflags )
{
    if ( NULL != pflags )
    {
        pflags->hw_surface  = HAS_SOME_BITS( iflags, SDL_HWSURFACE );
        pflags->async_blit  = HAS_SOME_BITS( iflags, SDL_ASYNCBLIT );
        pflags->any_format  = HAS_SOME_BITS( iflags, SDL_ANYFORMAT );
        pflags->hw_palette  = HAS_SOME_BITS( iflags, SDL_HWPALETTE );
        pflags->double_buf  = HAS_SOME_BITS( iflags, SDL_DOUBLEBUF );
        pflags->full_screen = HAS_SOME_BITS( iflags, SDL_FULLSCREEN );
        pflags->opengl      = HAS_SOME_BITS( iflags, SDL_OPENGL );
        pflags->opengl_blit = HAS_SOME_BITS( iflags, SDL_OPENGLBLIT );
        pflags->resizable   = HAS_SOME_BITS( iflags, SDL_RESIZABLE );
        pflags->no_frame    = HAS_SOME_BITS( iflags, SDL_NOFRAME );

        // "read only"
        pflags->use_hwaccel     = HAS_SOME_BITS( iflags, SDL_HWACCEL );
        pflags->has_srccolorkey = HAS_SOME_BITS( iflags, SDL_SRCCOLORKEY );
        pflags->use_rleaccelok  = HAS_SOME_BITS( iflags, SDL_RLEACCELOK );
        pflags->use_rleaccel    = HAS_SOME_BITS( iflags, SDL_RLEACCEL );
        pflags->use_srcalpha    = HAS_SOME_BITS( iflags, SDL_SRCALPHA );
        pflags->is_prealloc     = HAS_SOME_BITS( iflags, SDL_PREALLOC );
    }
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::report(SDLX_video_parameters_t *self)
{
    /// @author BB
    /// @details make a report

    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    log_message("\thorizontalResolution == %d, verticalResolution == %d, colorBufferDepth == %d\n", self->horizontalResolution, self->verticalResolution, self->colorBufferDepth);

    SDLX_sdl_video_flags_t::report(&(self->flags));

    if (self->flags.opengl)
    {
        SDLX_sdl_gl_attrib_t::report(&(self->gl_att));
    }
}

//--------------------------------------------------------------------------------------------
void SDLX_read_sdl_gl_attrib( SDLX_sdl_gl_attrib_t * patt )
{
    if ( NULL == patt ) return;

    SDL_GL_GetAttribute( SDL_GL_RED_SIZE,               patt->color + 0 );
    SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE,             patt->color + 1 );
    SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE,              patt->color + 2 );
    SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE,             patt->color + 3 );
    SDL_GL_GetAttribute( SDL_GL_BUFFER_SIZE,          &( patt->buffer_size ) );
    SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER,         &( patt->doublebuffer ) );
    SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE,           &( patt->depth_size ) );
    SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE,         &( patt->stencil_size ) );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_RED_SIZE,         patt->accum + 0 );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_GREEN_SIZE,       patt->accum + 1 );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_BLUE_SIZE,        patt->accum + 2 );
    SDL_GL_GetAttribute( SDL_GL_ACCUM_ALPHA_SIZE,       patt->accum + 3 );
    SDL_GL_GetAttribute( SDL_GL_STEREO,               &( patt->stereo ) );

#if !defined(__unix)
    SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS,   &( patt->multi_buffers ) );
    SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES,   &( patt->multi_samples ) );
    SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL,   &( patt->accelerated_visual ) );
    SDL_GL_GetAttribute( SDL_GL_SWAP_CONTROL,         &( patt->swap_control ) );
#endif
}

//--------------------------------------------------------------------------------------------
void SDLX_synch_video_parameters( SDL_Surface * ret, SDLX_video_parameters_t * v )
{
    /// @author BB
    /// @details synch values

    if ( NULL == ret || NULL == v ) return;

    v->horizontalResolution  = ret->w;
    v->verticalResolution = ret->h;
    v->colorBufferDepth  = ret->format->BitsPerPixel;

    // translate the surface flags into the bitfield
    SDLX_download_sdl_video_flags( ret->flags, &( v->flags ) );

    // grab all SDL_GL_* attributes
    SDLX_read_sdl_gl_attrib( &( v->gl_att ) );
}

//--------------------------------------------------------------------------------------------
SDL_bool SDLX_set_sdl_gl_attrib( SDLX_video_parameters_t * v )
{
    if ( NULL == v ) return SDL_FALSE;

    if ( v->flags.opengl )
    {
        SDLX_sdl_gl_attrib_t * patt = &( v->gl_att );

#if !defined(ID_LINUX)
        // Under Unix we cannot specify these, we just get whatever format
        // the framebuffer has, specifying depths > the framebuffer one
        // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"
        SDL_GL_SetAttribute( SDL_GL_RED_SIZE,             patt->color[0] );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE,           patt->color[1] );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,            patt->color[2] );
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE,           patt->color[3] );
        SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE,          patt->buffer_size );
#endif

        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,         patt->doublebuffer );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE,           patt->depth_size );
        SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE,         patt->stencil_size );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE,       patt->accum[0] );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE,     patt->accum[1] );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE,      patt->accum[2] );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE,     patt->accum[3] );
        SDL_GL_SetAttribute( SDL_GL_STEREO,               patt->stereo );

#if defined(ID_LINUX)

        // Fedora 7 doesn't suuport SDL_GL_SWAP_CONTROL, but we use this nvidia extension instead.
        if ( patt->swap_control )
        {
            SDL_putenv( "__GL_SYNC_TO_VBLANK=1" );
        }

#else

        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS,   patt->multi_buffers );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES,   patt->multi_samples );
        SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,   patt->accelerated_visual );
        SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL,         patt->swap_control );

#endif
    }

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
SDL_Surface * SDLX_RequestVideoMode( SDLX_video_parameters_t * v, SDL_bool make_report )
{
    Uint32 flags;
    int sdl_nearset_bpp = -1;
    SDL_Surface * ret = NULL;

    if ( NULL == v ) return ret;

    if ( !v->flags.opengl )
    {
        // set the
        flags = SDLX_upload_sdl_video_flags( v->flags );

        // do our one-and-only video initialization
        ret = NULL;
        sdl_nearset_bpp = SDL_VideoModeOK(v->horizontalResolution, v->verticalResolution, v->colorBufferDepth, flags);
        if ( 0 != sdl_nearset_bpp )
        {
            ret = SDL_SetVideoMode(v->horizontalResolution, v->verticalResolution, sdl_nearset_bpp, flags);

            if ( NULL == ret )
            {
                log_message("SDL WARN: Unable to set SDL video mode: %s\n", SDL_GetError());
            }
        }
    }
    else
    {
        int buffer_size = v->gl_att.buffer_size;

        if ( 0 == buffer_size ) buffer_size = v->colorBufferDepth;
        if ( 0 == buffer_size ) buffer_size = 32;
        if ( buffer_size > 32 ) buffer_size = 32;

        // fix bad colordepth
        if (( 0 == v->gl_att.color[0] && 0 == v->gl_att.color[1] && 0 == v->gl_att.color[2] ) ||
            ( v->gl_att.color[0] + v->gl_att.color[1] + v->gl_att.color[2] > buffer_size ) )
        {
            if ( buffer_size > 24 )
            {
                v->gl_att.color[0] = v->gl_att.color[1] = v->gl_att.color[2] = buffer_size / 4;
            }
            else
            {
                // do a kludge in case we have something silly like 16 bit "highcolor" mode
                v->gl_att.color[0] = v->gl_att.color[2] = buffer_size / 3;
                v->gl_att.color[1] = buffer_size - v->gl_att.color[0] - v->gl_att.color[2];
            }

            v->gl_att.color[0] = ( v->gl_att.color[0] > 8 ) ? 8 : v->gl_att.color[0];
            v->gl_att.color[1] = ( v->gl_att.color[1] > 8 ) ? 8 : v->gl_att.color[1];
            v->gl_att.color[2] = ( v->gl_att.color[2] > 8 ) ? 8 : v->gl_att.color[2];
        }

        // fix the alpha value
        v->gl_att.color[3] = buffer_size - v->gl_att.color[0] - v->gl_att.color[1] - v->gl_att.color[2];
        v->gl_att.color[3] = ( v->gl_att.color[3] < 0 ) ? 0 : v->gl_att.color[3];

        // get the proper buffer size
        buffer_size = v->gl_att.color[0] + v->gl_att.color[1] + v->gl_att.color[2] + v->gl_att.color[3];
        buffer_size &= ~7;

        // synch some parameters between OpenGL and SDL
        v->colorBufferDepth    = buffer_size;
        v->gl_att.buffer_size  = buffer_size;
        v->gl_att.doublebuffer = v->flags.double_buf ? SDL_TRUE : SDL_FALSE;

        // the GL_ATTRIB_* parameters must be set before opening the video mode
        SDLX_set_sdl_gl_attrib( v );

        // set the flags
        flags = SDLX_upload_sdl_video_flags( v->flags );

        // try a softer video initialization
        // if it fails, then it tries to get the closest possible valid video mode
        ret = NULL;
        sdl_nearset_bpp = SDL_VideoModeOK(v->horizontalResolution, v->verticalResolution, buffer_size, flags);
        if ( 0 != sdl_nearset_bpp )
        {
            ret = SDL_SetVideoMode(v->horizontalResolution, v->verticalResolution, sdl_nearset_bpp, flags);

            if ( NULL == ret )
            {
                log_message("SDL WARN: Unable to set SDL video mode: %s\n", SDL_GetError());
            }
        }

#if !defined(ID_LINUX)
        {
            int actual_multi_buffers = 0;                      // ignored in linux

            // attempt to see if our antialiasing setting is valid

            SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS, &actual_multi_buffers );

            if ( v->gl_att.multi_samples > 0 && 0 == actual_multi_buffers )
            {
                // could not create the multi-buffer with this pixel format
                // i.e. cross-platform equivalent of the vectors wglChoosePixelFormatARB and
                // wglGetPixelFormatAttribivARB could not be found
                //
                // This is the only feedback we have that the initialization failed
                //
                // we will try to reduce the amount of super sampling and try again

                v->gl_att.multi_samples -= 1;
                while ( v->gl_att.multi_samples > 1 && 0 == actual_multi_buffers )
                {
                    v->gl_att.multi_buffers = 1;

                    SDLX_set_sdl_gl_attrib( v );

                    sdl_nearset_bpp = SDL_VideoModeOK(v->horizontalResolution, v->verticalResolution, buffer_size, flags);
                    if ( 0 != sdl_nearset_bpp )
                    {
                        ret = SDL_SetVideoMode(v->horizontalResolution, v->verticalResolution, sdl_nearset_bpp, flags);
                        if ( NULL == ret )
                        {
                            log_message("SDL WARN: Unable to set SDL video mode: %s\n", SDL_GetError());
                        }
                    }

                    actual_multi_buffers = 0;
                    SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS, &actual_multi_buffers );

                    v->gl_att.multi_samples -= 1;
                }
            }
        }
#endif

        if ( NULL == ret )
        {
            // something is interfering with our ability to generate a screen.
            // assume that it is a complete incompatability with multisampling

            SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
            SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );

            sdl_nearset_bpp = SDL_VideoModeOK( v->horizontalResolution, v->verticalResolution, buffer_size, flags );
            if ( 0 != sdl_nearset_bpp )
            {
                ret = SDL_SetVideoMode( v->horizontalResolution, v->verticalResolution, sdl_nearset_bpp, flags );
                if ( NULL == ret )
                {
                    log_message("SDL WARN: Unable to set SDL video mode: %s\n", SDL_GetError());
                }
            }
        }

#if !defined(ID_LINUX)
        // grab the actual status of the multi_buffers and multi_samples
        v->gl_att.multi_buffers = 0;
        v->gl_att.multi_samples = 0;
        v->gl_att.accelerated_visual = SDL_FALSE;
        SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS, &( v->gl_att.multi_buffers ) );
        SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES, &( v->gl_att.multi_samples ) );
        SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL, &( v->gl_att.accelerated_visual ) );
#endif
    }

    // update the video parameters
    if ( NULL != ret )
    {
        SDLX_Get_Screen_Info( &sdl_scr, make_report );
        SDLX_synch_video_parameters( ret, v );
    }

    return ret;
}

//--------------------------------------------------------------------------------------------
void SDLX_sdl_video_flags_t::defaults(SDLX_sdl_video_flags_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    BLANK_STRUCT_PTR(self);

    self->double_buf  = 1;
    self->full_screen = 1;
    self->opengl      = 1;
}

//--------------------------------------------------------------------------------------------
void SDLX_sdl_gl_attrib_t::defaults(SDLX_sdl_gl_attrib_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    BLANK_STRUCT_PTR(self);

    self->multi_buffers = 1;
    self->multi_samples = 2;
    self->accelerated_visual = 1;

    self->color[0] = 8;
    self->color[1] = 8;
    self->color[2] = 8;
    self->color[2] = 8;
    self->buffer_size = 32;

    self->depth_size = 8;
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::defaults(SDLX_video_parameters_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    self->surface = nullptr;
    self->horizontalResolution = 640;
    self->verticalResolution = 480;
    self->colorBufferDepth = 32;

    SDLX_sdl_video_flags_t::defaults(&(self->flags));
    SDLX_sdl_gl_attrib_t::defaults(&(self->gl_att));
}

void SDLX_video_parameters_t::download(SDLX_video_parameters_t *self, egoboo_config_t *cfg)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!cfg)
    {
        throw std::invalid_argument("nullptr == cfg");
    }
    self->horizontalResolution = cfg->graphic_resolution_horizontal.getValue();
    self->verticalResolution = cfg->graphic_resolution_vertical.getValue();
    self->colorBufferDepth = cfg->graphic_colorBuffer_bitDepth.getValue();
    self->flags.full_screen = cfg->graphic_fullscreen.getValue();
    self->gl_att.buffer_size = cfg->graphic_colorBuffer_bitDepth.getValue();
    self->gl_att.depth_size = cfg->graphic_depthBuffer_bitDepth.getValue();
    self->gl_att.multi_buffers = (cfg->graphic_antialiasing.getValue() > 1) ? 1 : 0;
    self->gl_att.multi_samples = cfg->graphic_antialiasing.getValue();
}

//--------------------------------------------------------------------------------------------
void SDLX_report_mode( SDL_Surface * surface, SDLX_video_parameters_t * v )
{

    if ( NULL == surface )
    {
        log_message("\n==============================================================\n");
        log_message("!!!! SDL unable to set video mode with current parameters !!!! - \n    \"%s\"\n", SDL_GetError());
        SDLX_video_parameters_t::report( v );
        log_message("==============================================================\n");
    }
    else
    {
        log_message("\n==============================================================\n");
        log_message("SDL set video mode to the current parameters\n");
        log_message("\nSDL window parameters\n");

        // report the SDL screen info
        SDLX_Get_Screen_Info( &sdl_scr, SDL_FALSE );
        SDLX_video_parameters_t::report( v );
        SDLX_screen_info_t::report( &sdl_scr );

        log_message("==============================================================\n");
    }
}

//--------------------------------------------------------------------------------------------
SDLX_video_parameters_t * SDLX_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, SDL_bool has_valid_mode, SDL_bool make_report )
{
    /// @author BB
    /// @details let SDL try to set a new video mode.

    SDLX_video_parameters_t   param_old, param_new;
    SDLX_video_parameters_t * retval = NULL;
    SDL_Surface             * surface;

    // initialize v_old and param_old
    if ( has_valid_mode )
    {
        if ( NULL == v_old )
        {
            SDLX_video_parameters_t::defaults( &param_old );
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

    // initialize v_new and param_new
    if ( NULL == v_new )
    {
        SDLX_video_parameters_t::defaults( &param_new );
        v_new = &param_new;
    }
    else
    {
        memcpy( &param_new, v_new, sizeof( SDLX_video_parameters_t ) );
    }

    // assume any problem with setting the graphics mode is with the multisampling
    surface = SDLX_RequestVideoMode( &param_new, make_report );

    if ( make_report )
    {
        // report on the success or failure to set the mode
        SDLX_report_mode( surface, &param_new );
    };

    if ( NULL != surface )
    {
        param_new.surface = surface;
        if ( NULL != v_new )
        {
            memcpy( v_new, &param_new, sizeof( SDLX_video_parameters_t ) );
        }
        retval = v_new;
    }
    else if ( NULL != v_old )
    {
        surface = SDLX_RequestVideoMode( v_old, make_report );

        if ( NULL == surface )
        {
            log_message("Could not restore the old video mode. Terminating.\n");
            exit( -1 );
        }
        else
        {
            param_old.surface = surface;
            if ( v_old != &param_old )
            {
                memcpy( v_old, &param_old, sizeof( SDLX_video_parameters_t ) );
            }
        }

        retval = v_old;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
SDL_bool SDLX_ExpandFormat( SDL_PixelFormat * pformat )
{
    // use the basic screen format to create a surface with proper alpha support

    Uint8 i;

    if ( NULL == pformat ) return SDL_FALSE;

    if ( pformat->BitsPerPixel > 24 )
    {
        pformat->Amask = amask;
        pformat->Bmask = bmask;
        pformat->Gmask = gmask;
        pformat->Rmask = rmask;

        for ( i = 0; i < sdl_scr.d && HAS_NO_BITS( pformat->Amask, 1 << i ); i++ );

        if ( HAS_NO_BITS( pformat->Amask, 1 << i ) )
        {
            // no alpha bits available
            pformat->Ashift = 0;
            pformat->Aloss  = 8;
        }
        else
        {
            // normal alpha channel
            pformat->Ashift = i;
            pformat->Aloss  = 0;
        }
    }

    return SDL_TRUE;
}
