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
#include "egolib/Log/_Include.hpp"
#include "egolib/Graphics/GraphicsSystem.hpp"
#include "egolib/Graphics/GraphicsWindow.hpp"

// SDL 2.0.1 adds high DPI support
#if !SDL_VERSION_ATLEAST(2, 0, 1)
#define SDL_WINDOW_ALLOW_HIGHDPI 0
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

SDLX_screen_info_t sdl_scr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Log::Entry& operator<<(Log::Entry& e, const SDLX_screen_info_t& s)
{
    e << "SDL video driver = " << s.szDriver.c_str() << Log::EndOfLine;
    if (!s.displayModes.empty())
    {
        e << "\t" << "available fullscreen video modes:" << Log::EndOfLine;
        for (const auto &displayMode : s.displayModes)
        {
            e << "\t" << "\t"
              << displayMode->getHorizontalResolution() << " pixels x "
              << displayMode->getVerticalResolution() << " pixels x "
              << displayMode->getRefreshRate() << " Hz"
              << Log::EndOfLine;
        }
    }
    return e;
}

//--------------------------------------------------------------------------------------------
bool SDLX_Get_Screen_Info( SDLX_screen_info_t& psi, bool make_report )
{
    Uint32 init_flags = 0;

    init_flags = SDL_WasInit( SDL_INIT_EVERYTHING );
    if ( 0 == init_flags )
    {
        if ( make_report ) Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "SDL not initialized", Log::EndOfEntry);
        return false;
    }
    else if ( HAS_NO_BITS( init_flags, SDL_INIT_VIDEO ) )
    {
        if ( make_report ) Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "SDL video not initialized", Log::EndOfEntry);
        return false;
    }

    // store the screen info for everyone to use
    auto window = Ego::GraphicsSystem::window;

    
    // Grab all the available video modes
    psi.displayModes.clear();
    
    int displayNum = window->getDisplayIndex();
    int numDisplayModes = SDL_GetNumDisplayModes(displayNum);
    
    for (int i = 0; i < numDisplayModes; i++) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(displayNum, i, &mode);
        psi.displayModes.push_back(std::make_shared<Ego::SDL::DisplayMode>(mode));
    }
    
    // log the video driver info
    psi.szDriver = SDL_GetCurrentVideoDriver();

    // grab all SDL_GL_* attributes
    SDLX_sdl_gl_attrib_t::download(psi.gl_att);

    if (make_report)
    {
        Log::get() << Log::Entry::create(Log::Level::Info, __FILE__, __LINE__, psi);
    }
    return true;
}

//--------------------------------------------------------------------------------------------

SDLX_sdl_gl_multisampling_t::SDLX_sdl_gl_multisampling_t()
    : multibuffers(1),
      multisamples(4) {
}

bool SDLX_sdl_gl_multisampling_t::equalTo(const SDLX_sdl_gl_multisampling_t& other) const {
    return multibuffers == other.multibuffers
        && multisamples == other.multisamples;
}

void SDLX_sdl_gl_multisampling_t::upload() const {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, multibuffers);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisamples);
}

void SDLX_sdl_gl_multisampling_t::download() {
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &multibuffers);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &multisamples);
}

Log::Entry& operator<<(Log::Entry& e, const SDLX_sdl_gl_multisampling_t& s) {
    // Multisampling.
    e << "  " << "multisampling" << Log::EndOfLine;
    e << "  " << "  " << "multisample buffers = " << s.multibuffers << Log::EndOfLine;
    e << "  " << "  " << "multisamples = " << s.multisamples << Log::EndOfLine;
    return e;
}

SDLX_sdl_gl_attrib_t::SDLX_sdl_gl_attrib_t()
    : colourBufferDepth(32, 8, 8, 8, 8),
    accumulationBufferDepth(0, 0, 0, 0, 0),
    doublebuffer(0),
    stencilBufferDepth(0),
    stereo(0),
    swap_control(0),
    multisampling(),
    accelerated_visual(1),
    buffer_size(32),
    depthBufferDepth(8) {
}

void SDLX_sdl_gl_attrib_t::validate(SDLX_sdl_gl_attrib_t& self) {
    int frameBufferSize = self.buffer_size;
    int colorBufferRedDepth = self.colourBufferDepth.getRedDepth(),
        colorBufferGreenDepth = self.colourBufferDepth.getGreenDepth(),
        colorBufferBlueDepth = self.colourBufferDepth.getBlueDepth();
    if (0 == frameBufferSize) frameBufferSize = self.colourBufferDepth.getDepth();
    if (0 == frameBufferSize) frameBufferSize = 32;
    if (frameBufferSize > 32) frameBufferSize = 32;

    // Fix bad colour depths.
    if ((0 == colorBufferRedDepth &&
         0 == colorBufferGreenDepth &&
         0 == colorBufferBlueDepth) ||
        (colorBufferRedDepth + colorBufferGreenDepth + colorBufferBlueDepth  > frameBufferSize)) {
        if (frameBufferSize > 24) {
            colorBufferRedDepth = colorBufferGreenDepth = colorBufferBlueDepth = frameBufferSize / 4;
        } else {
            // Do a kludge in case we have something silly like 16 bit "highcolor" mode.
            colorBufferRedDepth = colorBufferBlueDepth = frameBufferSize / 3;
            colorBufferGreenDepth = frameBufferSize - colorBufferRedDepth - colorBufferBlueDepth;
        }

        colorBufferRedDepth = (colorBufferRedDepth > 8) ? 8 : colorBufferRedDepth;
        colorBufferGreenDepth = (colorBufferGreenDepth > 8) ? 8 : colorBufferGreenDepth;
        colorBufferBlueDepth = (colorBufferBlueDepth > 8) ? 8 : colorBufferBlueDepth;
    }

    // Fix the alpha alpha depth.
    int colorBufferAlphaDepth;// = self.colourBufferDepth.getAlphaDepth();
    colorBufferAlphaDepth = frameBufferSize - colorBufferRedDepth - colorBufferGreenDepth - colorBufferBlueDepth;
    colorBufferAlphaDepth = (colorBufferAlphaDepth < 0) ? 0 : colorBufferAlphaDepth;

    // Fix the frame buffer depth.
    frameBufferSize = colorBufferRedDepth + colorBufferGreenDepth + colorBufferBlueDepth + colorBufferAlphaDepth;
    frameBufferSize &= ~7;

    // Recompute the frame buffer depth and colour buffer depth.
    self.buffer_size = frameBufferSize;
    self.colourBufferDepth = Ego::ColourDepth(colorBufferRedDepth + colorBufferGreenDepth + colorBufferBlueDepth + colorBufferAlphaDepth,
                                              colorBufferRedDepth, colorBufferGreenDepth, colorBufferBlueDepth, colorBufferAlphaDepth);

}

void SDLX_sdl_gl_attrib_t::upload() const {
    // (1) Set the colour buffer depth.
    const int colourBufferDepth_sdl[4]
        = { colourBufferDepth.getRedDepth(),
            colourBufferDepth.getGreenDepth(),
            colourBufferDepth.getBlueDepth(),
            colourBufferDepth.getAlphaDepth() };
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, colourBufferDepth_sdl[0]);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, colourBufferDepth_sdl[1]);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, colourBufferDepth_sdl[2]);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, colourBufferDepth_sdl[3]);

    // (2) Set the frame buffer depth.
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, buffer_size);

    // (3) Enable/disable double buffering.
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, doublebuffer);

    // (4) Set the depth buffer and stencil buffer depths.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBufferDepth);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBufferDepth);

    // (5) Set the accumulation buffer depth.
    const int accumulationBufferDepth_sdl[4]{
        accumulationBufferDepth.getRedDepth(),
        accumulationBufferDepth.getGreenDepth(),
        accumulationBufferDepth.getBlueDepth(),
        accumulationBufferDepth.getAlphaDepth()
    };
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, accumulationBufferDepth_sdl[0]);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, accumulationBufferDepth_sdl[1]);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, accumulationBufferDepth_sdl[2]);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, accumulationBufferDepth_sdl[3]);
    
    // (6) Enable/disable stereoscopic rendering.
    SDL_GL_SetAttribute(SDL_GL_STEREO, stereo);

    // (7) Set multisampling.
    multisampling.upload();

    // (8) Enable/disable hardware acceleration.
#if !defined(ID_LINUX)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, accelerated_visual);
#endif
}

void SDLX_sdl_gl_attrib_t::download(SDLX_sdl_gl_attrib_t& self) {
    int temporary[4];

    // (1) Get the colour buffer depth.
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &(temporary[0]));
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &(temporary[1]));
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &(temporary[2]));
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &(temporary[3]));
    self.colourBufferDepth = Ego::ColourDepth(temporary[0] + temporary[1] + temporary[2] + temporary[3],
                                              temporary[0], temporary[1], temporary[2], temporary[3]);

    SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &(self.buffer_size));
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &(self.doublebuffer));
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &(self.depthBufferDepth));
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &(self.stencilBufferDepth));

    // (2) Get the accumulation buffer depth.
    SDL_GL_GetAttribute(SDL_GL_ACCUM_RED_SIZE, &(temporary[0]));
    SDL_GL_GetAttribute(SDL_GL_ACCUM_GREEN_SIZE, &(temporary[1]));
    SDL_GL_GetAttribute(SDL_GL_ACCUM_BLUE_SIZE, &(temporary[1]));
    SDL_GL_GetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, &(temporary[2]));
    self.accumulationBufferDepth = Ego::ColourDepth(temporary[0] + temporary[1] + temporary[2] + temporary[3],
                                                    temporary[0], temporary[1], temporary[2], temporary[3]);

    SDL_GL_GetAttribute(SDL_GL_STEREO, &(self.stereo));

    self.swap_control = SDL_GL_GetSwapInterval();

    self.multisampling.download();

#if !defined(ID_LINUX)
    SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &(self.accelerated_visual));
#endif
}

Log::Entry& operator<<(Log::Entry& e, const SDLX_sdl_gl_attrib_t& s) {
    e << "context attributes" << Log::EndOfLine;

    // Framebuffer depth.
    e << "  " << "framebuffer depth = " << s.buffer_size << Log::EndOfLine;

    // Colour buffer depth.
    e << "  " << "colour buffer" << Log::EndOfLine
        << "  " << "  " << "depth = " << s.colourBufferDepth.getDepth() << Log::EndOfLine
        << "  " << "  " << "red depth = " << s.colourBufferDepth.getRedDepth() << Log::EndOfLine
        << "  " << "  " << "green depth = " << s.colourBufferDepth.getGreenDepth() << Log::EndOfLine
        << "  " << "  " << "blue depth = " << s.colourBufferDepth.getBlueDepth() << Log::EndOfLine
        << "  " << "  " << "alpha depth = " << s.colourBufferDepth.getAlphaDepth() << Log::EndOfLine;

    // Depth buffer depth.
    e << "  " << "depth buffer depth = " << s.depthBufferDepth << Log::EndOfLine;

    // Stencil buffer depth.
    e << "  " << "stencil buffer depth = " << s.stencilBufferDepth << Log::EndOfLine;

    // Accumulation buffer depth.
    e << "  " << "accumulation buffer" << Log::EndOfLine
        << "  " << "  " << "depth = " << s.accumulationBufferDepth.getDepth() << Log::EndOfLine
        << "  " << "  " << "red depth = " << s.accumulationBufferDepth.getRedDepth() << Log::EndOfLine
        << "  " << "  " << "green depth = " << s.accumulationBufferDepth.getGreenDepth() << Log::EndOfLine
        << "  " << "  " << "blue depth = " << s.accumulationBufferDepth.getBlueDepth() << Log::EndOfLine
        << "  " << "  " << "alpha depth = " << s.accumulationBufferDepth.getAlphaDepth() << Log::EndOfLine;

    // Double buffering, stereoscopic rendering.
    e << "  " << "double buffer = " << s.doublebuffer << Log::EndOfLine;
    e << "  " << "stereoscopic rendering = " << s.stereo << Log::EndOfLine;

    // Multisampling.
    e << s.multisampling;

#if !defined(ID_LINUX)
    e << "  " << "accelerated visual = " << s.accelerated_visual << Log::EndOfLine;
#endif
    e << "  " << "swap control = " << s.swap_control << Log::EndOfLine;
    return e;
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::report(SDLX_video_parameters_t& self)
{
    Log::Entry e(Log::Level::Info, __FILE__, __LINE__);
    e << self << Log::EndOfEntry;
    Log::get() << e;
}

Log::Entry& operator<<(Log::Entry& e, const SDLX_video_parameters_t& s)
{
    // Write the horizontal and vertical resolution and the color depth.
    e << "resolution = " << s.resolution.width() << " x " << s.resolution.height() << Log::EndOfLine
      << "color depth = " << s.colorBufferDepth << Log::EndOfLine;
    // Write the window properties.
    e << s.windowProperties;

    // Write the context properties.
    if (s.windowProperties.opengl)
    {
        e << s.gl_att;
    }
    return e;
}

//--------------------------------------------------------------------------------------------
void SDLX_synch_video_parameters( Ego::GraphicsWindow *window, SDLX_video_parameters_t& v )
{
    /// @brief Read from SDL/GL window into the video parameters.

    if ( NULL == window ) return;

    /// @todo Shouldn't this be getDrawableSize?
    v.resolution = window->getSize();

    // Download the video flags from SDL.
    v.windowProperties = window->getProperties();

    // Download the OpenGL attributes from SDL.
    // @todo Call GraphicsContext::getFlags().
    SDLX_sdl_gl_attrib_t::download(v.gl_att);
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::upload() const {
    if (windowProperties.opengl) {
        gl_att.upload();
    }
}

//--------------------------------------------------------------------------------------------
SDL_GLContext SDLX_CreateContext(SDL_Window *window, const SDLX_sdl_gl_attrib_t& flags) {
    flags.upload();
    return SDL_GL_CreateContext(window);
}
Ego::GraphicsWindow *SDLX_CreateWindow(const Ego::WindowProperties& windowProperties) {
    try {
        return new Ego::GraphicsWindow(windowProperties);
    } catch(...) {
        return nullptr;
    }
}
Ego::GraphicsWindow *SDLX_CreateWindow( SDLX_video_parameters_t& v, bool make_report )
{
    Ego::GraphicsWindow *ret = nullptr;
    
    if (Ego::GraphicsSystem::window) return nullptr;

    if (!v.windowProperties.opengl) {
        // do our one-and-only video initialization
        ret = SDLX_CreateWindow(v.windowProperties);
        if (!ret) {
            Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to create SDL window: ", SDL_GetError(), Log::EndOfEntry);
        } else {
            ret->setSize(v.resolution);
            ret->center();
        }
    }
    else
    {
        SDLX_sdl_gl_attrib_t::validate(v.gl_att);

        // synch some parameters between OpenGL and SDL
        v.colorBufferDepth = v.gl_att.buffer_size;
        // try a softer video initialization
        // if it fails, then it tries to get the closest possible valid video mode
        ret = SDLX_CreateWindow(v.windowProperties);
        if ( nullptr == ret ) {
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create SDL window: ", SDL_GetError(), Log::EndOfEntry);
        } else {
            ret->setSize(v.resolution);
            ret->center();
            SDL_GLContext context = SDLX_CreateContext(ret->get(), v.gl_att);
            if (!context) {
				Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create OpenGLGL context: ", SDL_GetError(), Log::EndOfEntry);
                delete ret;
                ret = nullptr;
            }
        }
        
        if (!ret) {
            // if we're using multisamples, try lowering it until we succeed
            if ( v.gl_att.multisampling.multisamples > 0 )
            {
                v.gl_att.multisampling.multisamples -= 1;
                while ( v.gl_att.multisampling.multisamples > 1 && !ret )
                {
                    v.gl_att.multisampling.multibuffers = 1;                 
                    ret = SDLX_CreateWindow(v.windowProperties);
                    if ( nullptr == ret ) {
						Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create SDL window (", v.gl_att.multisampling.multisamples, " multisamples): ", SDL_GetError(), Log::EndOfEntry);
                    } else {
                        ret->setSize(v.resolution);
                        ret->center();
                        SDL_GLContext context = SDLX_CreateContext(ret->get(), v.gl_att);
                        if (!context) {
							Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create OpenGL context (", v.gl_att.multisampling.multisamples, " multisamples): ", SDL_GetError(), Log::EndOfEntry);
                            delete ret;
                            ret = nullptr;
                        }
                    }

                    v.gl_att.multisampling.multisamples -= 1;
                }
            }
        }

        if (nullptr == ret)
        {
            // something is interfering with our ability to generate a screen.
            // assume that it is a complete incompatability with multisampling
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "disabled antialiasing", Log::EndOfEntry);
            v.gl_att.multisampling.multibuffers = 0;
            v.gl_att.multisampling.multisamples = 0;

            ret = SDLX_CreateWindow(v.windowProperties);
            if ( nullptr == ret ) {
				Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create SDL window (no multisamples): ", SDL_GetError(), Log::EndOfEntry);
            } else {
                ret->setSize(v.resolution);
                ret->center();
                SDL_GLContext context = SDLX_CreateContext(ret->get(), v.gl_att);
                if (!context) {
					Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create OpenGL context (no multisamples): ", SDL_GetError(), Log::EndOfEntry);
                    delete ret;
                    ret = nullptr;
                }
            }
        }

        if (ret) {
            // grab the actual status of the multi_buffers and multi_samples
            v.gl_att.multisampling.multibuffers = 0;
            v.gl_att.multisampling.multisamples = 0;
            v.gl_att.accelerated_visual = SDL_FALSE;
            v.gl_att.multisampling.download();
#if !defined(ID_LINUX)
            SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL, &( v.gl_att.accelerated_visual ) );
#endif
            // Swap interval needs a current context
            SDL_GL_SetSwapInterval(v.gl_att.swap_control);
        }
    }


    // Update the video parameters.
    Ego::GraphicsSystem::window = ret;
    if ( NULL != ret )
    {
        SDLX_Get_Screen_Info( sdl_scr, make_report );
        SDLX_synch_video_parameters( ret, v );
    }
    return ret;
}

//--------------------------------------------------------------------------------------------

void SDLX_video_parameters_t::defaults(SDLX_video_parameters_t& self)
{
    self.surface = nullptr;
    self.resolution = Size2i(640, 480);
    self.colorBufferDepth = 32;

    self.windowProperties = Ego::WindowProperties();
    self.gl_att = SDLX_sdl_gl_attrib_t();
}

void SDLX_video_parameters_t::download(SDLX_video_parameters_t& self, egoboo_config_t& cfg)
{
    self.resolution = Size2i(cfg.graphic_resolution_horizontal.getValue(),
                             cfg.graphic_resolution_vertical.getValue());
    self.colorBufferDepth = cfg.graphic_colorBuffer_bitDepth.getValue();
    self.windowProperties.fullscreen = cfg.graphic_fullscreen.getValue();
    self.gl_att.buffer_size = cfg.graphic_colorBuffer_bitDepth.getValue();
    self.gl_att.depthBufferDepth = cfg.graphic_depthBuffer_bitDepth.getValue();
    self.gl_att.multisampling.multibuffers = (cfg.graphic_antialiasing.getValue() > 1) ? 1 : 0;
    self.gl_att.multisampling.multisamples = cfg.graphic_antialiasing.getValue();
}

//--------------------------------------------------------------------------------------------
void SDLX_report_mode( Ego::GraphicsWindow *surface, SDLX_video_parameters_t& v)
{
    auto e = Log::Entry::create(Log::Level::Message, __FILE__, __LINE__);
    e << "==============================================================" << Log::EndOfLine;
    if (!surface)
    {
        e << "SDL video mode not set:" << Log::EndOfLine
            << SDL_GetError() << Log::EndOfLine;
    }
    else
    {
        e << "SDL vide mode set" << Log::EndOfLine;
    }
    e << "SDL window parameters" << Log::EndOfLine;
    SDLX_video_parameters_t::report(v);

    if (surface)
    {
        // report the SDL screen info
        SDLX_Get_Screen_Info( sdl_scr, SDL_FALSE );
        e << sdl_scr;
    }
    e << "==============================================================" << Log::EndOfLine;
}

//--------------------------------------------------------------------------------------------
SDLX_video_parameters_t * SDLX_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, bool has_valid_mode, bool make_report )
{
    /// @author BB
    /// @details let SDL try to set a new video mode.

    SDLX_video_parameters_t   param_old, param_new;
    SDLX_video_parameters_t * retval = NULL;
    Ego::GraphicsWindow *surface;

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
            param_old = *v_old;
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
        param_new = *v_new;
    }

    // assume any problem with setting the graphics mode is with the multisampling
    surface = SDLX_CreateWindow( param_new, make_report );

    if ( make_report )
    {
        // report on the success or failure to set the mode
        SDLX_report_mode( surface, param_new );
    }

    if ( NULL != surface )
    {
        param_new.surface = surface;
        if ( NULL != v_new )
        {
            *v_new = param_new;
        }
        retval = v_new;
    }
    else if ( NULL != v_old )
    {
        surface = SDLX_CreateWindow( *v_old, make_report );

        if ( NULL == surface )
        {
            auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to restore the old video mode", Log::EndOfEntry);
            Log::get() << e;
			throw std::runtime_error(e.getText());
        }
        else
        {
            param_old.surface = surface;
            if ( v_old != &param_old )
            {
                *v_old = param_old;
            }
        }

        retval = v_old;
    }

    return retval;
}
