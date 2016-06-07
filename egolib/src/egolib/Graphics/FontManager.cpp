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

#include "egolib/Log/_Include.hpp"

namespace Ego {

FontManager::FontManager() {
    Log::get().info("[font manager]: SDL_ttf %i.%i.%i\n", SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION, SDL_TTF_PATCHLEVEL);
    if (TTF_Init() < 0) {
        Log::Entry e(Log::Level::Warning, __FILE__, __LINE__);
        e << "[font manager]: unable to initialized SDL_ttf: " << SDL_GetError() << Log::EndOfLine;
        Log::get() << e;
        throw Id::EnvironmentErrorException(__FILE__, __LINE__, "font manager",
                                            std::string("unable to initialized SDL_ttf: ") + SDL_GetError());
    }
}

FontManager::~FontManager() {
    TTF_Quit();
}

std::shared_ptr<Font> FontManager::loadFont(const std::string &fileName, int pointSize) {
    auto *font = new Font(fileName, pointSize);
    try {
        return std::shared_ptr<Font>(font);
    } catch (...) {
        delete font;
        std::rethrow_exception(std::current_exception());
    }
}
} // namespace Ego