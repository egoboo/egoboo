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

// SDL 2.0.1 adds high DPI support
#if !SDL_VERSION_ATLEAST(2, 0, 1)
#define SDL_WINDOW_ALLOW_HIGHDPI 0
#endif

void SDLX_GetDrawableSize(SDL_Window *window, int *w, int *h)
{
#if SDL_VERSION_ATLEAST(2, 0, 1)
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_OPENGL)
        return SDL_GL_GetDrawableSize(window, w, h);
#endif
    SDL_GetWindowSize(window, w, h);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

SDLX_screen_info_t sdl_scr;
SDL_Window *_window;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void SDLX_download_sdl_video_flags( Uint32 iflags, SDLX_sdl_video_flags_t * pflags );
static void SDLX_read_sdl_gl_attrib( SDLX_sdl_gl_attrib_t * patt );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void SDLX_screen_info_t::report(SDLX_screen_info_t& self)
{
	Log::message("\nSDL using video driver - %s\n", self.szDriver.c_str());

    if (!self.video_mode_list.empty())
    {
		Log::message("\tAvailable full-screen video modes...\n");
        for (const auto &mode : self.video_mode_list)
        {
			Log::message("    \tVideo Mode - %d x %d, %d Hz\n", mode.w, mode.h, mode.refresh_rate);
        }
    }
}

//--------------------------------------------------------------------------------------------
bool SDLX_Get_Screen_Info( SDLX_screen_info_t& psi, bool make_report )
{
    Uint32 init_flags = 0;
    SDL_Window *window;

    init_flags = SDL_WasInit( SDL_INIT_EVERYTHING );
    if ( 0 == init_flags )
    {
        if ( make_report ) Log::message("ERROR: SDLX_Get_Screen_Info() called before initializing SDL\n");
        return false;
    }
    else if ( HAS_NO_BITS( init_flags, SDL_INIT_VIDEO ) )
    {
        if ( make_report ) Log::message("ERROR: SDLX_Get_Screen_Info() called before initializing SDL video driver\n");
        return false;
    }

    // store the screen info for everyone to use
    window = _window;
	psi.window = window;
    SDL_GetWindowSize(window, &(psi.x), &(psi.y));
    SDLX_GetDrawableSize(window, &(psi.drawWidth), &(psi.drawHeight));

    // Grab all the available video modes
    psi.video_mode_list.clear();
    
    int displayNum = SDL_GetWindowDisplayIndex(window);
    int numDisplayModes = SDL_GetNumDisplayModes(displayNum);
    
    for (int i = 0; i < numDisplayModes; i++) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(displayNum, i, &mode);
        psi.video_mode_list.push_back(mode);
    }
    
    // log the video driver info
    psi.szDriver = SDL_GetCurrentVideoDriver();

    // grab all SDL_GL_* attributes
    SDLX_read_sdl_gl_attrib( &( psi.gl_att ) );

    // translate the surface flags into the bitfield
    SDLX_download_sdl_video_flags( SDL_GetWindowFlags(window), &( psi.flags ) );

    if (make_report)
    {
        SDLX_screen_info_t::report(psi);
    }
    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void SDLX_sdl_gl_attrib_t::report(SDLX_sdl_gl_attrib_t& self)
{
	Log::message("\nSDL_GL_Attribtes\n");

	Log::message("\tSDL_GL_RED_SIZE           == %d\n", self.color[0]);
	Log::message("\tSDL_GL_GREEN_SIZE         == %d\n", self.color[1]);
	Log::message("\tSDL_GL_BLUE_SIZE          == %d\n", self.color[2]);
	Log::message("\tSDL_GL_ALPHA_SIZE         == %d\n", self.color[3]);
	Log::message("\tSDL_GL_BUFFER_SIZE        == %d\n", self.buffer_size);
	Log::message("\tSDL_GL_DEPTH_SIZE         == %d\n", self.depth_size);

	Log::message("\tSDL_GL_DOUBLEBUFFER       == %d\n", self.doublebuffer);
	Log::message("\tSDL_GL_STENCIL_SIZE       == %d\n", self.stencil_size);
	Log::message("\tSDL_GL_ACCUM_RED_SIZE     == %d\n", self.accum[0]);
	Log::message("\tSDL_GL_ACCUM_GREEN_SIZE   == %d\n", self.accum[1]);
	Log::message("\tSDL_GL_ACCUM_BLUE_SIZE    == %d\n", self.accum[2]);
	Log::message("\tSDL_GL_ACCUM_ALPHA_SIZE   == %d\n", self.accum[3]);
	Log::message("\tSDL_GL_STEREO             == %d\n", self.stereo);

	Log::message("\tSDL_GL_MULTISAMPLEBUFFERS == %d\n", self.multi_buffers);
	Log::message("\tSDL_GL_MULTISAMPLESAMPLES == %d\n", self.multi_samples);
#if !defined(ID_LINUX)
	Log::message("\tSDL_GL_ACCELERATED_VISUAL == %d\n", self.accelerated_visual);
#endif

	Log::message("\tSDL_GL_SWAP_CONTROL       == %d\n", self.swap_control);
}

//--------------------------------------------------------------------------------------------
void SDLX_sdl_video_flags_t::report(SDLX_sdl_video_flags_t& self)
{
	Log::message("\nSDL flags\n");

	Log::message("\t%s\n", self.full_screen ? "fullscreen" : "windowed");

    if (self.opengl)
    {
		Log::message("\tOpenGL support\n");
    }

    if (self.resizable)
    {
		Log::message("\tresizable window\n");
    }

    if (self.borderless)
    {
		Log::message("\tborderless window\n");
    }
    
    if (self.highdpi)
    {
		Log::message("\thigh DPI (Apple 'Retina') support");
    }
}

//--------------------------------------------------------------------------------------------
Uint32 SDLX_upload_sdl_video_flags( SDLX_sdl_video_flags_t flags )
{
    Uint32 ret = 0;

    ret |= flags.full_screen ? SDL_WINDOW_FULLSCREEN    : 0;
    ret |= flags.opengl      ? SDL_WINDOW_OPENGL        : 0;
    ret |= flags.resizable   ? SDL_WINDOW_RESIZABLE     : 0;
    ret |= flags.borderless  ? SDL_WINDOW_BORDERLESS    : 0;
    ret |= flags.highdpi     ? SDL_WINDOW_ALLOW_HIGHDPI : 0;
    if (flags.full_screen && flags.use_desktop_size) ret |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    return ret;
}

//--------------------------------------------------------------------------------------------
void SDLX_download_sdl_video_flags( Uint32 iflags, SDLX_sdl_video_flags_t * pflags )
{
    if ( NULL != pflags )
    {
        pflags->full_screen = HAS_SOME_BITS(iflags, SDL_WINDOW_FULLSCREEN);
        pflags->opengl      = HAS_SOME_BITS(iflags, SDL_WINDOW_OPENGL);
        pflags->resizable   = HAS_SOME_BITS(iflags, SDL_WINDOW_RESIZABLE);
        pflags->borderless  = HAS_SOME_BITS(iflags, SDL_WINDOW_BORDERLESS);
        pflags->highdpi     = HAS_SOME_BITS(iflags, SDL_WINDOW_ALLOW_HIGHDPI);
        if (pflags->full_screen) pflags->use_desktop_size = HAS_SOME_BITS(iflags, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::report(SDLX_video_parameters_t& self)
{
    /// @author BB
    /// @details make a report

	Log::message("\thorizontalResolution == %d, verticalResolution == %d, colorBufferDepth == %d\n", self.horizontalResolution, self.verticalResolution, self.colorBufferDepth);

    SDLX_sdl_video_flags_t::report(self.flags);

    if (self.flags.opengl)
    {
        SDLX_sdl_gl_attrib_t::report(self.gl_att);
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
    
    patt->swap_control = SDL_GL_GetSwapInterval();

    SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS,   &( patt->multi_buffers ) );
    SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES,   &( patt->multi_samples ) );
#if !defined(ID_LINUX)
    SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL,   &( patt->accelerated_visual ) );
#endif
}

//--------------------------------------------------------------------------------------------
void SDLX_synch_video_parameters( SDL_Window * ret, SDLX_video_parameters_t * v )
{
    /// @author BB
    /// @details synch values

    if ( NULL == ret || NULL == v ) return;

    SDL_GetWindowSize(ret, &(v->horizontalResolution), &(v->verticalResolution));

    // translate the surface flags into the bitfield
    SDLX_download_sdl_video_flags( SDL_GetWindowFlags(ret), &( v->flags ) );

    // grab all SDL_GL_* attributes
    SDLX_read_sdl_gl_attrib( &( v->gl_att ) );
}

//--------------------------------------------------------------------------------------------
bool SDLX_set_sdl_gl_attrib( SDLX_video_parameters_t * v )
{
    if ( NULL == v ) return false;

    if ( v->flags.opengl )
    {
        SDLX_sdl_gl_attrib_t * patt = &( v->gl_att );

        SDL_GL_SetAttribute( SDL_GL_RED_SIZE,             patt->color[0] );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE,           patt->color[1] );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,            patt->color[2] );
        SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE,           patt->color[3] );
        SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE,          patt->buffer_size );

        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,         patt->doublebuffer );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE,           patt->depth_size );
        SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE,         patt->stencil_size );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE,       patt->accum[0] );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE,     patt->accum[1] );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE,      patt->accum[2] );
        SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE,     patt->accum[3] );
        SDL_GL_SetAttribute( SDL_GL_STEREO,               patt->stereo );

        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS,   patt->multi_buffers );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES,   patt->multi_samples );
#if !defined(ID_LINUX)
        SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL,   patt->accelerated_visual );
#endif
    }

    return true;
}

//--------------------------------------------------------------------------------------------
SDL_Window * SDLX_CreateWindow( SDLX_video_parameters_t * v, bool make_report )
{
    Uint32 flags;
    SDL_Window * ret = nullptr;
    int windowPos = SDL_WINDOWPOS_CENTERED;
    
    if ( NULL == v ) return ret;
    if (_window) return nullptr;

    if ( !v->flags.opengl )
    {
        // set the
        flags = SDLX_upload_sdl_video_flags( v->flags );

        // do our one-and-only video initialization
        ret = SDL_CreateWindow("SDL App", windowPos, windowPos, v->horizontalResolution, v->verticalResolution, flags);
        if (!ret)
        {
			Log::message("SDL WARN: Unable to create SDL window: %s\n", SDL_GetError());
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

        // the GL_ATTRIB_* parameters must be set before opening the video mode
        SDLX_set_sdl_gl_attrib( v );

        // set the flags
        flags = SDLX_upload_sdl_video_flags( v->flags );

        // try a softer video initialization
        // if it fails, then it tries to get the closest possible valid video mode
        ret = SDL_CreateWindow("", windowPos, windowPos, v->horizontalResolution, v->verticalResolution, flags);
        if ( nullptr == ret ) {
			Log::message("SDL WARN: Unable to create SDL window: %s\n", SDL_GetError());
        } else {
            SDL_GLContext context = SDL_GL_CreateContext(ret);
            if (!context) {
				Log::message("SDL WARN: Unable to create GL context: %s\n", SDL_GetError());
                SDL_DestroyWindow(ret);
                ret = nullptr;
            }
        }
        
        if (!ret) {
            // if we're using multisamples, try lowering it until we succeed
            if ( v->gl_att.multi_samples > 0 )
            {
                v->gl_att.multi_samples -= 1;
                while ( v->gl_att.multi_samples > 1 && !ret )
                {
                    v->gl_att.multi_buffers = 1;

                    SDLX_set_sdl_gl_attrib( v );
                    
                    ret = SDL_CreateWindow("", windowPos, windowPos, v->horizontalResolution, v->verticalResolution, flags);
                    if ( nullptr == ret ) {
						Log::message("SDL WARN: Unable to create SDL window (%d multisamples): %s\n", v->gl_att.multi_samples, SDL_GetError());
                    } else {
                        SDL_GLContext context = SDL_GL_CreateContext(ret);
                        if (!context) {
							Log::message("SDL WARN: Unable to create GL context (%d multisamples): %s\n", v->gl_att.multi_samples, SDL_GetError());
                            SDL_DestroyWindow(ret);
                            ret = nullptr;
                        }
                    }

                    v->gl_att.multi_samples -= 1;
                }
            }
        }

        if ( NULL == ret )
        {
            // something is interfering with our ability to generate a screen.
            // assume that it is a complete incompatability with multisampling

            SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 0 );
            SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 0 );

            ret = SDL_CreateWindow("", windowPos, windowPos, v->horizontalResolution, v->verticalResolution, flags);
            if ( nullptr == ret ) {
				Log::message("SDL WARN: Unable to create SDL window (no multisamples): %s\n", SDL_GetError());
            } else {
                SDL_GLContext context = SDL_GL_CreateContext(ret);
                if (!context) {
					Log::message("SDL WARN: Unable to create GL context (no multisamples): %s\n", SDL_GetError());
                    SDL_DestroyWindow(ret);
                    ret = nullptr;
                }
            }
        }

        if (ret) {
        // grab the actual status of the multi_buffers and multi_samples
            v->gl_att.multi_buffers = 0;
            v->gl_att.multi_samples = 0;
            v->gl_att.accelerated_visual = SDL_FALSE;
            SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS, &( v->gl_att.multi_buffers ) );
            SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES, &( v->gl_att.multi_samples ) );
#if !defined(ID_LINUX)
            SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL, &( v->gl_att.accelerated_visual ) );
#endif
            // Swap interval needs a current context
            SDL_GL_SetSwapInterval(v->gl_att.swap_control);
        }
    }

    // update the video parameters
    if ( NULL != ret )
    {
        SDLX_Get_Screen_Info( sdl_scr, make_report );
        SDLX_synch_video_parameters( ret, v );
    }
    
    _window = ret;

    return ret;
}

//--------------------------------------------------------------------------------------------
void SDLX_sdl_video_flags_t::defaults(SDLX_sdl_video_flags_t& self)
{
	self.resizable = 0;
	self.borderless = 0;
	self.use_desktop_size = 0;
	self.highdpi = 0;

    self.full_screen = 1;
    self.opengl      = 1;
}

//--------------------------------------------------------------------------------------------
void SDLX_sdl_gl_attrib_t::defaults(SDLX_sdl_gl_attrib_t& self)
{
	self.doublebuffer = 0;
	self.stencil_size = 0;
	self.accum[0] = 0;
	self.accum[1] = 0;
	self.accum[2] = 0;
	self.accum[3] = 0;
	self.stereo = 0;
	self.swap_control = 0;

    self.multi_buffers = 1;
    self.multi_samples = 2;
    self.accelerated_visual = 1;

    self.color[0] = 8;
    self.color[1] = 8;
    self.color[2] = 8;
    self.color[2] = 8;
    self.buffer_size = 32;

    self.depth_size = 8;
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::defaults(SDLX_video_parameters_t& self)
{
    self.surface = nullptr;
    self.horizontalResolution = 640;
    self.verticalResolution = 480;
    self.colorBufferDepth = 32;

    SDLX_sdl_video_flags_t::defaults(self.flags);
    SDLX_sdl_gl_attrib_t::defaults(self.gl_att);
}

void SDLX_video_parameters_t::download(SDLX_video_parameters_t& self, egoboo_config_t& cfg)
{
    self.horizontalResolution = cfg.graphic_resolution_horizontal.getValue();
    self.verticalResolution = cfg.graphic_resolution_vertical.getValue();
    self.colorBufferDepth = cfg.graphic_colorBuffer_bitDepth.getValue();
    self.flags.full_screen = cfg.graphic_fullscreen.getValue();
    self.gl_att.buffer_size = cfg.graphic_colorBuffer_bitDepth.getValue();
    self.gl_att.depth_size = cfg.graphic_depthBuffer_bitDepth.getValue();
    self.gl_att.multi_buffers = (cfg.graphic_antialiasing.getValue() > 1) ? 1 : 0;
    self.gl_att.multi_samples = cfg.graphic_antialiasing.getValue();
}

//--------------------------------------------------------------------------------------------
void SDLX_report_mode( SDL_Window * surface, SDLX_video_parameters_t& v)
{

    if ( NULL == surface )
    {
		Log::message("\n==============================================================\n");
		Log::message("!!!! SDL unable to set video mode with current parameters !!!! - \n    \"%s\"\n", SDL_GetError());
        SDLX_video_parameters_t::report( v );
		Log::message("==============================================================\n");
    }
    else
    {
		Log::message("\n==============================================================\n");
		Log::message("SDL set video mode to the current parameters\n");
		Log::message("\nSDL window parameters\n");

        // report the SDL screen info
        SDLX_Get_Screen_Info( sdl_scr, SDL_FALSE );
        SDLX_video_parameters_t::report( v );
        SDLX_screen_info_t::report( sdl_scr );

		Log::message("==============================================================\n");
    }
}

//--------------------------------------------------------------------------------------------
SDLX_video_parameters_t * SDLX_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, bool has_valid_mode, bool make_report )
{
    /// @author BB
    /// @details let SDL try to set a new video mode.

    SDLX_video_parameters_t   param_old, param_new;
    SDLX_video_parameters_t * retval = NULL;
    SDL_Window              * surface;

    // initialize v_old and param_old
    if ( has_valid_mode )
    {
        if ( NULL == v_old )
        {
            SDLX_video_parameters_t::defaults( param_old );
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
    
    if (v_old) throw std::invalid_argument("v_old is not null! unsupported");

    // initialize v_new and param_new
    if ( NULL == v_new )
    {
        SDLX_video_parameters_t::defaults( param_new );
        v_new = &param_new;
    }
    else
    {
        memcpy( &param_new, v_new, sizeof( SDLX_video_parameters_t ) );
    }

    // assume any problem with setting the graphics mode is with the multisampling
    surface = SDLX_CreateWindow( &param_new, make_report );

    if ( make_report )
    {
        // report on the success or failure to set the mode
        SDLX_report_mode( surface, param_new );
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
        surface = SDLX_CreateWindow( v_old, make_report );

        if ( NULL == surface )
        {
			Log::message("Could not restore the old video mode. Terminating.\n");
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
