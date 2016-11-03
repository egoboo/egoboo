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
#include "egolib/Math/_Include.hpp"
#include "egolib/Graphics/ColourDepth.hpp"
#include "egolib/Extensions/WindowProperties.hpp"

// Forward declaration.
namespace Ego {
struct GraphicsWindow;
}

//--------------------------------------------------------------------------------------------

/// Information about the multisampling capabilities of an OpenGL context.
/// @todo Originally there was only multisample antialiasing (MSAA). Samples
/// was designated for the number of MSAA samples per pixel to allocate for
/// that system framebuffer. Clean, simple. Then the GPU vendors added support
/// for supersample antialiasing(SSAA), coverage sample antialiasing(CSAA), and
/// combinations of the three.
struct SDLX_sdl_gl_multisampling_t : public Id::EqualToExpr<SDLX_sdl_gl_multisampling_t> {
    int multibuffers; /// SDL_GL_MULTISAMPLEBUFFERS
    int multisamples; /// SDL_GL_MULTISAMPLESAMPLES
    SDLX_sdl_gl_multisampling_t();
    void upload() const;
    void download();
    // CRTP
    bool equalTo(const SDLX_sdl_gl_multisampling_t& other) const;
};

Log::Entry& operator<<(Log::Entry& e, const SDLX_sdl_gl_multisampling_t& s);

/// @brief A structure holding all of the OpenGL data that can be queried through SDL.
/// @remark SDL_GL_SetAttribute and SDL_GL_GetAttribute are used to write and read those values.
/// @todo Rename to Ego::Graphics::SDL::ContextProperties.
struct SDLX_sdl_gl_attrib_t {
    /// The depth of the colour buffer.
    /// SDL_GL_RED_SIZE, SDL_GL_GREEN_SIZE, SDL_GL_BLUE_SIZE, SDL_GL_ALPHA_SIZE
    Ego::ColourDepth colourBufferDepth;
    /// The depth of the accumulation buffer.
    /// SDL_GL_ACCUM_RED_SIZE, SDL_GL_ACCUM_GREEN_SIZE, SDL_GL_ACCUM_BLUE_SIZE, SDL_GL_ACCUM_ALPHA_SIZE
    Ego::ColourDepth accumulationBufferDepth;
    int buffer_size;        ///< SDL_GL_BUFFER_SIZE
    int doublebuffer;       ///< SDL_GL_DOUBLEBUFFER
    int depthBufferDepth;         ///< SDL_GL_DEPTH_SIZE
    int stencilBufferDepth;       ///< SDL_GL_STENCIL_SIZE
    int stereo;             ///< SDL_GL_STEREO
    SDLX_sdl_gl_multisampling_t multisampling;
    int accelerated_visual; ///< SDL_GL_ACCELERATED_VISUAL
    int swap_control;       ///< SDL_GL_SWAP_CONTROL

    SDLX_sdl_gl_attrib_t();

    static void validate(SDLX_sdl_gl_attrib_t& self);
    // Upload the attributes to SDL.
    void upload() const;
    // Download the attributes from SDL.
    static void download(SDLX_sdl_gl_attrib_t& self);
};

Log::Entry& operator<<(Log::Entry& e, const SDLX_sdl_gl_attrib_t& s);

//--------------------------------------------------------------------------------------------

#include "egolib/Graphics/SDL/DisplayMode.hpp"


/// A representation of a SDL Screen state
    struct SDLX_screen_info_t
    {
        /// A list of shared pointers to display modes.
        std::vector<std::shared_ptr<Ego::DisplayMode>> displayModes;
        std::string szDriver;    ///< graphics driver name;

        /// Context properties.
        /// @todo Rename gl_att to contextProperties.
        SDLX_sdl_gl_attrib_t gl_att;

        static void report(SDLX_screen_info_t& self);
    };

//--------------------------------------------------------------------------------------------

/// Parameters for setting an SDL video state
    struct SDLX_video_parameters_t
    {
        /// horizontal and vertical resolution
        Size2i resolution;
        int colorBufferDepth;

        Ego::WindowProperties windowProperties;
        SDLX_sdl_gl_attrib_t gl_att;

        Ego::GraphicsWindow *surface;

        static void report(SDLX_video_parameters_t& self);
        static void defaults(SDLX_video_parameters_t& self);
        static void download(SDLX_video_parameters_t& self, egoboo_config_t& cfg);
        void upload() const;
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    extern SDLX_screen_info_t sdl_scr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Grab the current SDL screen information
    bool      SDLX_Get_Screen_Info( SDLX_screen_info_t& psi, bool display );

/// Use a SDLX_video_parameters_t structure to create window
    Ego::GraphicsWindow *SDLX_CreateWindow( SDLX_video_parameters_t& v, bool make_report );

/// Use a SDLX_video_parameters_t structure to try to set a SDL video mode directly
/// on success, it returns a pointer to the actual data used to set the mode. On failure,
/// it resets the mode to v_old (if possible), and returns a pointer to the restored parameters
    SDLX_video_parameters_t * SDLX_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new, bool has_valid_mode, bool make_report );

/// Dump the info on the given surface to whatever FILE SDL_extensions is using for stdout
    void   SDLX_report_mode( Ego::GraphicsWindow * surface, SDLX_video_parameters_t& v );
