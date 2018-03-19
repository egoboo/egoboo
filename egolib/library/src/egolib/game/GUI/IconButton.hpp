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

/// @file egolib/game/GUI/IconButton.cpp
/// @details A button with an small icon on the right side
/// @author Johan Jansen

#pragma once

#include "egolib/game/GUI/Button.hpp"

// Forward declarations.
namespace Ego { class DeferredTexture; }

namespace Ego::GUI {

class IconButton : public Button {
public:
    IconButton(const std::string &buttonText, const DeferredTexture& icon, int hotkey = SDLK_UNKNOWN);

    virtual void draw(DrawingContext& drawingContext) override;

    void setIconTint(const Colour4f &tint);

private:
    const DeferredTexture& _icon;
    Colour4f _iconTint;
};

} // namespace Ego::GUI
