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

SDLX_screen_info_t sdl_scr;

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

bool SDLX_Get_Screen_Info( SDLX_screen_info_t& psi )
{
    Uint32 init_flags = 0;

    init_flags = SDL_WasInit( SDL_INIT_EVERYTHING );
    if ( 0 == init_flags )
    {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "SDL not initialized", Log::EndOfEntry);
        return false;
    }
    else if ( HAS_NO_BITS( init_flags, SDL_INIT_VIDEO ) )
    {
        Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "SDL video not initialized", Log::EndOfEntry);
        return false;
    }
    
    // Grab all the available video modes
    psi.displayModes.clear();
    
    int displayNum = Ego::GraphicsSystem::window->getDisplayIndex();
    int numDisplayModes = SDL_GetNumDisplayModes(displayNum);
    
    for (int i = 0; i < numDisplayModes; i++) {
        SDL_DisplayMode mode;
        SDL_GetDisplayMode(displayNum, i, &mode);
        psi.displayModes.push_back(std::make_shared<Ego::SDL::DisplayMode>(mode));
    }
    
    // log the video driver info
    psi.szDriver = SDL_GetCurrentVideoDriver();

    // grab all SDL_GL_* attributes
    psi.contextProperties.download();

    return true;
}

//--------------------------------------------------------------------------------------------

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
        e << s.contextProperties;
    }
    return e;
}

void SDLX_video_parameters_t::upload() const {
    if (windowProperties.opengl) {
        contextProperties.upload();
    }
}

//--------------------------------------------------------------------------------------------

bool SDLX_CreateWindow( SDLX_video_parameters_t& v )
{  
    if (Ego::GraphicsSystem::window) return false;

    if (!v.windowProperties.opengl) {
        // do our one-and-only video initialization
        Ego::GraphicsSystem::window = Ego::GraphicsSystem::createWindow(v.windowProperties);
        if (!Ego::GraphicsSystem::window) {
            Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to create SDL window: ", SDL_GetError(), Log::EndOfEntry);
        } else {
            Ego::GraphicsSystem::window->setSize(v.resolution);
            Ego::GraphicsSystem::window->center();
        }
    }
    else
    {
        Ego::ContextProperties::validate(v.contextProperties);

        // synch some parameters between OpenGL and SDL
        v.colorBufferDepth = v.contextProperties.buffer_size;
        // try a softer video initialization
        // if it fails, then it tries to get the closest possible valid video mode
        Ego::GraphicsSystem::window = Ego::GraphicsSystem::createWindow(v.windowProperties);
        if (!Ego::GraphicsSystem::window) {
			Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create SDL window: ", SDL_GetError(), Log::EndOfEntry);
        } else {
            Ego::GraphicsSystem::window->setSize(v.resolution);
            Ego::GraphicsSystem::window->center();
            Ego::GraphicsSystem::context = Ego::GraphicsSystem::createContext(Ego::GraphicsSystem::window, v.contextProperties);
            if (!Ego::GraphicsSystem::context) {
				Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create OpenGLGL context: ", SDL_GetError(), Log::EndOfEntry);
                delete Ego::GraphicsSystem::window;
                Ego::GraphicsSystem::window = nullptr;
            }
        }
        
        if (!Ego::GraphicsSystem::window) {
            // if we're using multisamples, try lowering it until we succeed
            if ( v.contextProperties.multisampling.samples > 0 )
            {
                v.contextProperties.multisampling.samples -= 1;
                while ( v.contextProperties.multisampling.samples > 1 && !Ego::GraphicsSystem::window)
                {
                    v.contextProperties.multisampling.buffers = 1;
                    Ego::GraphicsSystem::window = Ego::GraphicsSystem::createWindow(v.windowProperties);
                    if (!Ego::GraphicsSystem::window) {
						Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create SDL window (", v.contextProperties.multisampling.samples, " multisamples): ", SDL_GetError(), Log::EndOfEntry);
                    } else {
                        Ego::GraphicsSystem::window->setSize(v.resolution);
                        Ego::GraphicsSystem::window->center();
                        Ego::GraphicsSystem::context = Ego::GraphicsSystem::createContext(Ego::GraphicsSystem::window, v.contextProperties);
                        if (!Ego::GraphicsSystem::context) {
							Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create OpenGL context (", v.contextProperties.multisampling.samples, " multisamples): ", SDL_GetError(), Log::EndOfEntry);
                            delete Ego::GraphicsSystem::window;
                            Ego::GraphicsSystem::window = nullptr;
                        }
                    }

                    v.contextProperties.multisampling.samples -= 1;
                }
            }
        }

        if (!Ego::GraphicsSystem::window)
        {
            // something is interfering with our ability to generate a screen.
            // assume that it is a complete incompatability with multisampling
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "disabled antialiasing", Log::EndOfEntry);
            v.contextProperties.multisampling.buffers = 0;
            v.contextProperties.multisampling.samples = 0;

            Ego::GraphicsSystem::window = Ego::GraphicsSystem::createWindow(v.windowProperties);
            if (!Ego::GraphicsSystem::window) {
				Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create SDL window (no multisamples): ", SDL_GetError(), Log::EndOfEntry);
            } else {
                Ego::GraphicsSystem::window->setSize(v.resolution);
                Ego::GraphicsSystem::window->center();
                Ego::GraphicsSystem::context = Ego::GraphicsSystem::createContext(Ego::GraphicsSystem::window, v.contextProperties);
                if (!Ego::GraphicsSystem::context) {
					Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "unable to create OpenGL context (no multisamples): ", SDL_GetError(), Log::EndOfEntry);
                    delete Ego::GraphicsSystem::window;
                    Ego::GraphicsSystem::window = nullptr;
                }
            }
        }

        if (Ego::GraphicsSystem::window) {
            // grab the actual status of the multi_buffers and multi_samples
            v.contextProperties.multisampling.buffers = 0;
            v.contextProperties.multisampling.samples = 0;
            v.contextProperties.accelerated_visual = SDL_FALSE;
            v.contextProperties.multisampling.download();
#if !defined(ID_LINUX)
            SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL, &( v.contextProperties.accelerated_visual ) );
#endif
            // Swap interval needs a current context
            SDL_GL_SetSwapInterval(v.contextProperties.swap_control);
        }
    }


    // Update the video parameters.
    if (Ego::GraphicsSystem::window)
    {
        SDLX_Get_Screen_Info( sdl_scr );
        v.resolution = Ego::GraphicsSystem::window->getSize();
        v.windowProperties = Ego::GraphicsSystem::window->getProperties();
        v.contextProperties.download();
    }
    return true;
}

//--------------------------------------------------------------------------------------------

SDLX_video_parameters_t::SDLX_video_parameters_t() :
    resolution(640, 480), colorBufferDepth(32), windowProperties(), contextProperties()
{}

void SDLX_video_parameters_t::download(egoboo_config_t& cfg)
{
    resolution = Size2i(cfg.graphic_resolution_horizontal.getValue(),
                        cfg.graphic_resolution_vertical.getValue());
    colorBufferDepth = cfg.graphic_colorBuffer_bitDepth.getValue();
    windowProperties.fullscreen = cfg.graphic_fullscreen.getValue();
    contextProperties.buffer_size = cfg.graphic_colorBuffer_bitDepth.getValue();
    contextProperties.depthBufferDepth = cfg.graphic_depthBuffer_bitDepth.getValue();
    contextProperties.multisampling.buffers = (cfg.graphic_antialiasing.getValue() > 1) ? 1 : 0;
    contextProperties.multisampling.samples = cfg.graphic_antialiasing.getValue();
}

//--------------------------------------------------------------------------------------------
void SDLX_report_mode( SDLX_video_parameters_t& v)
{
    auto e = Log::Entry::create(Log::Level::Message, __FILE__, __LINE__);
    e << "==============================================================" << Log::EndOfLine;
    if (!Ego::GraphicsSystem::window)
    {
        e << "SDL video mode not set:" << Log::EndOfLine
            << SDL_GetError() << Log::EndOfLine;
    }
    else
    {
        e << "SDL vide mode set" << Log::EndOfLine;
    }
    e << "SDL window parameters" << Log::EndOfLine;
    e << v;

    if (Ego::GraphicsSystem::window)
    {
        // report the SDL screen info
        SDLX_Get_Screen_Info( sdl_scr );
        e << sdl_scr;
    }
    e << "==============================================================" << Log::EndOfLine;
}
