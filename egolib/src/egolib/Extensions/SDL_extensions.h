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

#include "egolib/egoboo_setup.h"
#include "egolib/Math/_Include.hpp"
#include "egolib/Extensions/WindowProperties.hpp"
#include "egolib/Extensions/ContextProperties.hpp"

//--------------------------------------------------------------------------------------------

/// Parameters for setting an SDL video state
    struct SDLX_video_parameters_t
    {
        /// horizontal and vertical resolution
        Size2i resolution;
        int colorBufferDepth;

        Ego::WindowProperties windowProperties;
        Ego::ContextProperties contextProperties;

        SDLX_video_parameters_t();

        void download(egoboo_config_t& cfg);
        void upload() const;
    };

    Log::Entry& operator<<(Log::Entry& e, const SDLX_video_parameters_t& s);

//--------------------------------------------------------------------------------------------

/// Use a SDLX_video_parameters_t structure to create window
    bool SDLX_CreateWindow( SDLX_video_parameters_t& v );
