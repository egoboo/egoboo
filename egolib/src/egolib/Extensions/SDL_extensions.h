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

/// @defgroup _sdl_extensions_ Extensions to SDL

/// @file egolib/Extensions/SDL_extensions.h
/// @ingroup _sdl_extensions_
/// @brief Definitions for generic extensions to SDL
/// @details

#pragma once

#include "egolib/file_common.h"
#include "egolib/egoboo_setup.h"
#include "egolib/Graphics/ColorDepth.hpp"

//--------------------------------------------------------------------------------------------

/// A structure holding information about a window.
/// A structure holding some of SDL's video data
/// @todo Rename to Ego::Graphics::SDL::WindowProperties.
struct SDLX_sdl_video_flags_t {
    unsigned full_screen : 1;      ///< SDL_WINDOW_FULLSCREEN    - Window is a full screen display
    unsigned opengl : 1;           ///< SDL_WINDOW_OPENGL        - Create an OpenGL rendering context
    unsigned resizable : 1;        ///< SDL_WINDOW_RESIZABLE     - Window may be resized
    unsigned borderless : 1;       ///< SDL_WINDOW_BORDERLESS    - No window caption or edge frame
    unsigned use_desktop_size : 1; ///< SDL_WINDOW_FULLSCREEN_DESKTOP - Window uses desktop size in fullscreen, requires full_screen to be set
    unsigned highdpi : 1;          ///< SDL_WINDOW_ALLOW_HIGHDPI - Supports High-DPI mode (Apple 'Retina')

    static void defaults(SDLX_sdl_video_flags_t& self);
    static uint32_t upload(const SDLX_sdl_video_flags_t& self);
    static void download(SDLX_sdl_video_flags_t& self, uint32_t bits);
};

Log::Entry& operator<<(Log::Entry& e, const SDLX_sdl_video_flags_t& s);

//--------------------------------------------------------------------------------------------

/// @brief A structure holding all of the OpenGL data that can be queried through SDL.
/// @remark SDL_GL_SetAttribute and SDL_GL_GetAttribute are used to write and read those values.
/// @todo Rename to Ego::Graphics::SDL::ContextProperties.
struct SDLX_sdl_gl_attrib_t {
    /// The depth of the colour buffer.
    /// SDL_GL_RED_SIZE, SDL_GL_GREEN_SIZE, SDL_GL_BLUE_SIZE, SDL_GL_ALPHA_SIZE
    Ego::ColorDepth colourBufferDepth;
    /// The depth of the accumulation buffer.
    /// SDL_GL_ACCUM_RED_SIZE, SDL_GL_ACCUM_GREEN_SIZE, SDL_GL_ACCUM_BLUE_SIZE, SDL_GL_ACCUM_ALPHA_SIZE
    Ego::ColorDepth accumulationBufferDepth;
    int buffer_size;        ///< SDL_GL_BUFFER_SIZE
    int doublebuffer;       ///< SDL_GL_DOUBLEBUFFER
    int depth_size;         ///< SDL_GL_DEPTH_SIZE
    int stencil_size;       ///< SDL_GL_STENCIL_SIZE
    int stereo;             ///< SDL_GL_STEREO
    int multi_buffers;      ///< SDL_GL_MULTISAMPLEBUFFERS
    int multi_samples;      ///< SDL_GL_MULTISAMPLESAMPLES
    int accelerated_visual; ///< SDL_GL_ACCELERATED_VISUAL
    int swap_control;       ///< SDL_GL_SWAP_CONTROL

    SDLX_sdl_gl_attrib_t();

    static void validate(SDLX_sdl_gl_attrib_t& self);
    static void defaults(SDLX_sdl_gl_attrib_t& self);
    // Upload the attributes to SDL.
    static void upload(SDLX_sdl_gl_attrib_t& self);
    // Download the attributes from SDL.
    static void download(SDLX_sdl_gl_attrib_t& self);
};

Log::Entry& operator<<(Log::Entry& e, const SDLX_sdl_gl_attrib_t& s);

//--------------------------------------------------------------------------------------------

/// A representation of a SDL Screen state
    struct SDLX_screen_info_t
    {
        SDL_Window *window;

        std::vector<SDL_DisplayMode> video_mode_list;

        std::string szDriver;    ///< graphics driver name;

        int x;                ///< Screen X size @todo rename to width
        int y;                ///< Screen Y size @todo rename to height
        int drawWidth;        ///< Framebuffer width (may be different with high DPI on)
        int drawHeight;       ///< Framebuffer height (may be different with high DPI on)

        /// Context properties.
        /// @todo Rename gl_att to contextProperties.
        SDLX_sdl_gl_attrib_t gl_att;

        /// Window properties.
        /// @todo Rename flags to windowProperties.
        SDLX_sdl_video_flags_t flags;

        static void report(SDLX_screen_info_t& self);
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Parameters for setting an SDL video state
    struct SDLX_video_parameters_t
    {
        int horizontalResolution;
        int verticalResolution;
        int colorBufferDepth;

        SDLX_sdl_video_flags_t flags;
        SDLX_sdl_gl_attrib_t gl_att;

        SDL_Window *surface;

        static void report(SDLX_video_parameters_t& self);
        static void defaults(SDLX_video_parameters_t& self);
        static void download(SDLX_video_parameters_t& self, egoboo_config_t& cfg);
        static bool upload(SDLX_video_parameters_t * v);
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern SDLX_screen_info_t sdl_scr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Grab the current SDL screen information
    bool      SDLX_Get_Screen_Info( SDLX_screen_info_t& psi, bool display );

/// Use a SDLX_video_parameters_t structure to create window
    SDL_Window * SDLX_CreateWindow( SDLX_video_parameters_t * v, bool make_report );

/// Use a SDLX_video_parameters_t structure to try to set a SDL video mode directly
/// on success, it returns a pointer to the actual data used to set the mode. On failure,
/// it resets the mode to v_old (if possible), and returns a pointer to the restored parameters
    SDLX_video_parameters_t * SDLX_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, bool has_valid_mode, bool make_report );

/// Dump the info on the given surface to whatever FILE SDL_extensions is using for stdout
    void   SDLX_report_mode( SDL_Window * surface, SDLX_video_parameters_t& v );
