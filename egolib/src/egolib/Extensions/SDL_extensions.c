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
void SDLX_screen_info_t::report(SDLX_screen_info_t& self)
{
	Log::get().message("\nSDL using video driver - %s\n", self.szDriver.c_str());

    if (!self.video_mode_list.empty())
    {
		Log::get().message("\tAvailable full-screen video modes...\n");
        for (const auto &mode : self.video_mode_list)
        {
			Log::get().message("    \tVideo Mode - %d x %d, %d Hz\n", mode.w, mode.h, mode.refresh_rate);
        }
    }
}

//--------------------------------------------------------------------------------------------
bool SDLX_Get_Screen_Info( SDLX_screen_info_t& psi, bool make_report )
{
    Uint32 init_flags = 0;
    Ego::GraphicsWindow *window;

    init_flags = SDL_WasInit( SDL_INIT_EVERYTHING );
    if ( 0 == init_flags )
    {
        if ( make_report ) Log::get().message("ERROR: SDLX_Get_Screen_Info() called before initializing SDL\n");
        return false;
    }
    else if ( HAS_NO_BITS( init_flags, SDL_INIT_VIDEO ) )
    {
        if ( make_report ) Log::get().message("ERROR: SDLX_Get_Screen_Info() called before initializing SDL video driver\n");
        return false;
    }

    // store the screen info for everyone to use
    window = Ego::GraphicsSystem::window;
	psi.window = window;
    psi.size = window->getSize();
    psi.drawableSize = window->getDrawableSize();
    
    // Grab all the available video modes
    psi.video_mode_list.clear();
    
    int displayNum = window->getDisplayIndex();
    int numDisplayModes = SDL_GetNumDisplayModes(displayNum);
    
    for (int i = 0; i < numDisplayModes; i++) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(displayNum, i, &mode);
        psi.video_mode_list.push_back(mode);
    }
    
    // log the video driver info
    psi.szDriver = SDL_GetCurrentVideoDriver();

    // grab all SDL_GL_* attributes
    SDLX_sdl_gl_attrib_t::download(psi.gl_att);

    // translate the surface flags into the bitfield
    psi.flags = window->getFlags();

    if (make_report)
    {
        SDLX_screen_info_t::report(psi);
    }
    return true;
}

//--------------------------------------------------------------------------------------------

SDLX_sdl_gl_multisampling_t::SDLX_sdl_gl_multisampling_t()
    : multibuffers(1),
      multisamples(4) {
}

bool SDLX_sdl_gl_multisampling_t::operator==(const SDLX_sdl_gl_multisampling_t& other) const {
    return multibuffers == other.multibuffers
        && multisamples == other.multisamples;
}

bool SDLX_sdl_gl_multisampling_t::operator!=(const SDLX_sdl_gl_multisampling_t& other) const {
    return !((*this) == other);
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

void SDLX_sdl_gl_attrib_t::defaults(SDLX_sdl_gl_attrib_t& self) {
    self = SDLX_sdl_gl_attrib_t();
}

void SDLX_sdl_gl_attrib_t::validate(SDLX_sdl_gl_attrib_t& self) {
    int frameBufferSize = self.buffer_size;
    int colorBufferRedDepth = self.colourBufferDepth.getRedDepth(),
        colorBufferGreenDepth = self.colourBufferDepth.getGreenDepth(),
        colorBufferBlueDepth = self.colourBufferDepth.getBlueDepth(),
        colorBufferAlphaDepth = self.colourBufferDepth.getAlphaDepth();
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

uint32_t SDLX_sdl_video_flags_t::upload(const SDLX_sdl_video_flags_t& self) {
    uint32_t bits = 0;
    bits |= self.full_screen ? SDL_WINDOW_FULLSCREEN : 0;
    bits |= self.opengl ? SDL_WINDOW_OPENGL : 0;
    bits |= self.resizable ? SDL_WINDOW_RESIZABLE : 0;
    bits |= self.borderless ? SDL_WINDOW_BORDERLESS : 0;
    bits |= self.highdpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0;
    if (self.full_screen && self.use_desktop_size) {
        bits |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    return bits;
}

void SDLX_sdl_video_flags_t::download(SDLX_sdl_video_flags_t& self, uint32_t bits) {
    self.full_screen = HAS_SOME_BITS(bits, SDL_WINDOW_FULLSCREEN);
    self.opengl = HAS_SOME_BITS(bits, SDL_WINDOW_OPENGL);
    self.resizable = HAS_SOME_BITS(bits, SDL_WINDOW_RESIZABLE);
    self.borderless = HAS_SOME_BITS(bits, SDL_WINDOW_BORDERLESS);
    self.highdpi = HAS_SOME_BITS(bits, SDL_WINDOW_ALLOW_HIGHDPI);
    if (self.full_screen) {
        self.use_desktop_size = HAS_SOME_BITS(bits, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

Log::Entry& operator<<(Log::Entry& e, const SDLX_sdl_video_flags_t& s) {
    e << "window properties" << Log::EndOfLine;
    e << " fullscreen = " << (s.full_screen ? "yes" : "no") << Log::EndOfLine
      << " OpenGL = " << (s.opengl ? "yes" : "no") << Log::EndOfLine
      << " resizable = " << (s.resizable ? "yes" : "no") << Log::EndOfLine
      << " borderless = " << (s.borderless ? "yes" : "no") << Log::EndOfLine
      << " high DPI (Apple 'Retina') = " << (s.highdpi ? "yes" : "no") << Log::EndOfLine;
    return e;
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::report(SDLX_video_parameters_t& self)
{
    /// @author BB
    /// @details make a report
    Log::Entry e(Log::Level::Info, __FILE__, __LINE__);

    // Write the horizontal and vertical resolution and the color depth.
    e << "resolution = " << self.resolution.getWidth() << " x " << self.resolution.getHeight() << Log::EndOfLine
      << "color depth = " << self.colorBufferDepth << Log::EndOfLine;
    // Write the window properties.
    e << self.flags;

    // Write the context properties.
    if (self.flags.opengl)
    {
        e << self.gl_att;
    }

    e << Log::EndOfEntry;
    Log::get() << e;
}

//--------------------------------------------------------------------------------------------
void SDLX_synch_video_parameters( Ego::GraphicsWindow *window, SDLX_video_parameters_t& v )
{
    /// @brief Read from SDL/GL window into the video parameters.

    if ( NULL == window ) return;

    /// @todo Shouldn't this be getDrawableSize?
    v.resolution = window->getSize();

    // Download the video flags from SDL.
    v.flags = window->getFlags();

    // Download the OpenGL attributes from SDL.
    // @todo Call GraphicsContext::getFlags().
    SDLX_sdl_gl_attrib_t::download(v.gl_att);
}

//--------------------------------------------------------------------------------------------
void SDLX_video_parameters_t::upload() const {
    if (flags.opengl) {
        gl_att.upload();
    }
}

//--------------------------------------------------------------------------------------------
SDL_GLContext SDLX_CreateContext(SDL_Window *window, const SDLX_sdl_gl_attrib_t& flags) {
    flags.upload();
    return SDL_GL_CreateContext(window);
}
Ego::GraphicsWindow *SDLX_CreateWindow(const SDLX_sdl_video_flags_t& flags) {
    try {
        return new Ego::GraphicsWindow(flags);
    } catch(...) {
        return nullptr;
    }
}
Ego::GraphicsWindow *SDLX_CreateWindow( SDLX_video_parameters_t& v, bool make_report )
{
    Ego::GraphicsWindow *ret = nullptr;
    static const Point2i windowPosition = Point2i();
    
    if (Ego::GraphicsSystem::window) return nullptr;

    if (!v.flags.opengl) {
        // do our one-and-only video initialization
        ret = SDLX_CreateWindow(v.flags);
        if (!ret) {
            Log::get().message("SDL WARN: Unable to create SDL window: %s\n", SDL_GetError());
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
        ret = SDLX_CreateWindow(v.flags);
        if ( nullptr == ret ) {
			Log::get().warn("unable to create SDL window: %s\n", SDL_GetError());
        } else {
            ret->setSize(v.resolution);
            ret->center();
            SDL_GLContext context = SDLX_CreateContext(ret->get(), v.gl_att);
            if (!context) {
				Log::get().warn("unable to create GL context: %s\n", SDL_GetError());
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
                    ret = SDLX_CreateWindow(v.flags);
                    if ( nullptr == ret ) {
						Log::get().warn("unable to create SDL window (%d multisamples): %s\n", v.gl_att.multisampling.multisamples, SDL_GetError());
                    } else {
                        ret->setSize(v.resolution);
                        ret->center();
                        SDL_GLContext context = SDLX_CreateContext(ret->get(), v.gl_att);
                        if (!context) {
							Log::get().warn("unable to create GL context (%d multisamples): %s\n", v.gl_att.multisampling.multisamples, SDL_GetError());
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
            Log::get().warn("Disabled antialiasing\n");
            v.gl_att.multisampling.multibuffers = 0;
            v.gl_att.multisampling.multisamples = 0;

            ret = SDLX_CreateWindow(v.flags);
            if ( nullptr == ret ) {
				Log::get().warn("unable to create SDL window (no multisamples): %s\n", SDL_GetError());
            } else {
                ret->setSize(v.resolution);
                ret->center();
                SDL_GLContext context = SDLX_CreateContext(ret->get(), v.gl_att);
                if (!context) {
					Log::get().warn("unable to create GL context (no multisamples): %s\n", SDL_GetError());
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

SDLX_sdl_video_flags_t::SDLX_sdl_video_flags_t() :
    resizable(0),
    borderless(0),
    use_desktop_size(0), 
    highdpi(0), 
    full_screen(1), 
    opengl(1) {
}

SDLX_sdl_video_flags_t::SDLX_sdl_video_flags_t(const SDLX_sdl_video_flags_t& other) :
    resizable(other.resizable),
    borderless(other.borderless),
    use_desktop_size(other.use_desktop_size),
    highdpi(other.highdpi),
    full_screen(1),
    opengl(1) {
}

const SDLX_sdl_video_flags_t& SDLX_sdl_video_flags_t::operator=(const SDLX_sdl_video_flags_t& other) {
    resizable = other.resizable;
    borderless = other.borderless;
    use_desktop_size = other.use_desktop_size;
    highdpi = other.highdpi;
    full_screen = other.full_screen;
    opengl = other.opengl;
    return *this;
}

void SDLX_sdl_video_flags_t::defaults(SDLX_sdl_video_flags_t& self)
{
    self = SDLX_sdl_video_flags_t();
}

//--------------------------------------------------------------------------------------------

void SDLX_video_parameters_t::defaults(SDLX_video_parameters_t& self)
{
    self.surface = nullptr;
    self.resolution = Size2i(640, 480);
    self.colorBufferDepth = 32;

    SDLX_sdl_video_flags_t::defaults(self.flags);
    SDLX_sdl_gl_attrib_t::defaults(self.gl_att);
}

void SDLX_video_parameters_t::download(SDLX_video_parameters_t& self, egoboo_config_t& cfg)
{
    self.resolution = Size2i(cfg.graphic_resolution_horizontal.getValue(),
                             cfg.graphic_resolution_vertical.getValue());
    self.colorBufferDepth = cfg.graphic_colorBuffer_bitDepth.getValue();
    self.flags.full_screen = cfg.graphic_fullscreen.getValue();
    self.gl_att.buffer_size = cfg.graphic_colorBuffer_bitDepth.getValue();
    self.gl_att.depthBufferDepth = cfg.graphic_depthBuffer_bitDepth.getValue();
    self.gl_att.multisampling.multibuffers = (cfg.graphic_antialiasing.getValue() > 1) ? 1 : 0;
    self.gl_att.multisampling.multisamples = cfg.graphic_antialiasing.getValue();
}

//--------------------------------------------------------------------------------------------
void SDLX_report_mode( Ego::GraphicsWindow *surface, SDLX_video_parameters_t& v)
{

    if ( NULL == surface )
    {
		Log::get().message("\n==============================================================\n");
		Log::get().message("!!!! SDL unable to set video mode with current parameters !!!! - \n    \"%s\"\n", SDL_GetError());
        SDLX_video_parameters_t::report( v );
		Log::get().message("==============================================================\n");
    }
    else
    {
		Log::get().message("\n==============================================================\n");
		Log::get().message("SDL set video mode to the current parameters\n");
		Log::get().message("\nSDL window parameters\n");

        // report the SDL screen info
        SDLX_Get_Screen_Info( sdl_scr, SDL_FALSE );
        SDLX_video_parameters_t::report( v );
        SDLX_screen_info_t::report( sdl_scr );

		Log::get().message("==============================================================\n");
    }
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
			Log::get().error("could not restore the old video mode. Terminating.\n");
			throw std::runtime_error("unable to restore the old video mode\n");
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
