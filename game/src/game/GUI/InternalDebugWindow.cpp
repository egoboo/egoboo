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

/// @file game/GUI/InternalDebugWindow.cpp
/// @details InternalDebugWindow
/// @author Johan Jansen

#include "game/GUI/InternalDebugWindow.hpp"

namespace Ego {
namespace GUI {

InternalDebugWindow::InternalDebugWindow(const std::string &title) : InternalWindow(title),
_watchedVariables() {
    setSize(Vector2f(200, 75));
}

void InternalDebugWindow::addWatchVariable(const std::string &variableName, std::function<std::string()> lambda) {
    //Add variable to watch list
    _watchedVariables[variableName] = lambda;

    //Make the window bigger
    int textWidth, textHeight;
    _gameEngine->getUIManager()->getDebugFont()->getTextSize(variableName, &textWidth, &textHeight);
    textWidth = std::max(32, textWidth);
    textHeight = std::max(8, textHeight);
    setSize(Vector2f(std::max(getWidth(), 5.0f + textWidth * 2.0f), getHeight() + textHeight + 5.0f));
}

void InternalDebugWindow::drawContainer(DrawingContext& drawingContext) {
    //Draw the window itself
    InternalWindow::drawContainer(drawingContext);

    //Rendering variables
    int textWidth, textHeight;
    int xOffset = getX() + 5;
    int yOffset = getY() + 32;

    //Draw all monitored variables
    for (const auto &element : _watchedVariables) {
        _gameEngine->getUIManager()->getDebugFont()->drawText(element.first + ": " + element.second(), xOffset, yOffset);

        _gameEngine->getUIManager()->getDebugFont()->getTextSize(element.first, &textWidth, &textHeight);
        yOffset += textHeight + 5;
    }
}

} // namespace GUI
} // namespace Ego
