//********************************************************************************************
//*
//*    This file is part of Egoboo.
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

/// @file egolib/Graphics/FontManager.cpp
/// @brief TTF management
/// @details TrueType font drawing functionality.  Uses the SDL_ttf module
///          to do its business. This depends on SDL_ttf and OpenGL.

#include "egolib/Graphics/FontManager.hpp"

#include "egolib/log.h"

namespace Ego
{
    void FontManager::initialize()
    {
        if (isInitialized()) return;
        log_info( "Initializing the SDL_ttf font handler version %i.%i.%i... ", SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION, SDL_TTF_PATCHLEVEL );
        if ( TTF_Init() < 0 )
        {
            log_message( "Failed!\n" );
        }
        else
        {
            log_message( "Success!\n" );
        }
    }
    
    void FontManager::uninitialize()
    {
        if (!isInitialized()) throw std::runtime_error("font manager not initialized!");
        TTF_Quit();
    }
    
    bool FontManager::isInitialized()
    {
        return TTF_WasInit();
    }
    
    std::shared_ptr<Font> FontManager::loadFont(const std::string &fileName, int pointSize)
    {
        if (!isInitialized()) throw std::runtime_error("font manager not initialized!");
        return std::make_shared<Font>(fileName, pointSize);
    }    
}