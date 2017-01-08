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
#include "egolib/Extensions/ContextProperties.hpp"

// Forward declaration.
namespace Ego {
struct GraphicsWindow;
}

//--------------------------------------------------------------------------------------------

#include "egolib/Graphics/SDL/DisplayMode.hpp"


/// A representation of a SDL Screen state
    struct SDLX_screen_info_t
    {
        /// A list of shared pointers to display modes.
        std::vector<std::shared_ptr<Ego::DisplayMode>> displayModes;
        std::string szDriver;    ///< graphics driver name;

        /// Context properties.
        Ego::ContextProperties contextProperties;
    };

    Log::Entry& operator<<(Log::Entry& e, const SDLX_screen_info_t& s);

//--------------------------------------------------------------------------------------------

/// Parameters for setting an SDL video state
    struct SDLX_video_parameters_t
    {
        /// horizontal and vertical resolution
        Size2i resolution;
        int colorBufferDepth;

        Ego::WindowProperties windowProperties;
        Ego::ContextProperties contextProperties;

        static void defaults(SDLX_video_parameters_t& self);
        static void download(SDLX_video_parameters_t& self, egoboo_config_t& cfg);
        void upload() const;
    };

    Log::Entry& operator<<(Log::Entry& e, const SDLX_video_parameters_t& s);

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
    bool SDLX_set_mode( SDLX_video_parameters_t& v_new );

/// Dump the info on the given surface to whatever FILE SDL_extensions is using for stdout
    void   SDLX_report_mode( Ego::GraphicsWindow * surface, SDLX_video_parameters_t& v );
