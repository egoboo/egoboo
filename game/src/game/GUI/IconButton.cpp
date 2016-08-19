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

/// @file game/GUI/IconButton.cpp
/// @details A button with an small icon on the right side
/// @author Johan Jansen

#include "game/GUI/IconButton.hpp"
#include "game/GUI/Material.hpp"
#include "egolib/Renderer/DeferredTexture.hpp"

namespace Ego {
namespace GUI {

IconButton::IconButton(const std::string &buttonText, const DeferredTexture& icon, int hotkey) : Button(buttonText, hotkey),
_icon(icon),
_iconTint(Math::Colour4f::white()) {
    //ctor
}

void IconButton::draw(DrawingContext& drawingContext) {
    //Update slidy button effect
    updateSlidyButtonEffect();

    // Draw the button
    std::shared_ptr<const Material> material = nullptr;

    // Determine button color
    if (!isEnabled()) {
        material = std::make_shared<const Material>(nullptr, DISABLED_BUTTON_COLOUR, true);
    } else if (_mouseOver) {
        material = std::make_shared<const Material>(nullptr, HOVER_BUTTON_COLOUR, true);
    } else {
        material = std::make_shared<const Material>(nullptr, DEFAULT_BUTTON_COLOUR, true);
    }
    material->apply();
    _gameEngine->getUIManager()->drawQuad2d(getDerivedBounds());

    // Draw icon
    int iconSize = getHeight() - 4;
    material = std::make_shared<const Material>(_icon.get_ptr(), _iconTint, true);
    _gameEngine->getUIManager()->drawImage(Point2f(getDerivedPosition().x() + getWidth() - getHeight() - 2,
                                                   getDerivedPosition().y() + 2), Vector2f(iconSize, iconSize), material);

    // Draw text on left side in button
    if (_buttonTextRenderer) {
        _buttonTextRenderer->render(getDerivedPosition().x() + 5, getDerivedPosition().y() + (getHeight() - _buttonTextHeight) / 2);
    }
}

void IconButton::setIconTint(const Math::Colour4f &tint) {
    _iconTint = tint;
}

} // namespace GUI
} // namespace Ego