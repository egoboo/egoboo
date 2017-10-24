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

/// @file egolib/Graphics/FontManager.hpp
/// @brief TTF management
/// @details TrueType font drawing functionality.  Uses the SDL_ttf module
///          to do its business. This depends on SDL_ttf and OpenGL.

#pragma once

#include "egolib/typedef.h"
#include "egolib/Graphics/Font.hpp"

namespace Ego {

class FontManager : public id::singleton<FontManager> {
public:
    std::shared_ptr<Font> loadFont(const std::string &fileName, int pointSize);

protected:
	friend id::default_new_functor<FontManager>;
	friend id::default_delete_functor<FontManager>;

    FontManager();
    ~FontManager();
};

} // namespace Ego
