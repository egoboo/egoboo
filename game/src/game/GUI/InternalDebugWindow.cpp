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

VariablesDebugPanel::VariablesDebugPanel() : Container(),
    _variables() {
    setSize(Vector2f(200, 75));
}

void VariablesDebugPanel::addVariable(const std::string& name, std::function<std::string()> value) {
    //Add variable to watch list
    _variables[name] = value;

    //Make the window bigger
    int textWidth, textHeight;
    _gameEngine->getUIManager()->getDebugFont()->getTextSize(name, &textWidth, &textHeight);
    textWidth = std::max(32, textWidth);
    textHeight = std::max(8, textHeight);
    setSize(Vector2f(std::max(getWidth(), textWidth * 2.0f), getHeight() + textHeight));
}

void VariablesDebugPanel::draw(DrawingContext& drawingContext) {
    drawContainer(drawingContext);
    drawAll(drawingContext);
}

void VariablesDebugPanel::drawContainer(DrawingContext& drawingContext) {
    //Draw all monitored variables
    int textWidth, textHeight;
    Vector2f offset = Point2f::toVector(getDerivedPosition());
    for (const auto &element : _variables) {
        _gameEngine->getUIManager()->getDebugFont()->drawText(element.first + ": " + element.second(), offset.x(), offset.y());
        _gameEngine->getUIManager()->getDebugFont()->getTextSize(element.first, &textWidth, &textHeight);
        offset += Vector2f(0.0f, textHeight + 5);
    }
}

InternalDebugWindow::InternalDebugWindow(const std::string &title)
    : InternalWindow(title), _variablesDebugPanel() {
    _variablesDebugPanel = std::make_shared<VariablesDebugPanel>();
    addComponent(_variablesDebugPanel);
    _variablesDebugPanel->setPosition(Point2f(13 + 5, _titleBar->getHeight()));
    setSize(Vector2f(200, 75));
}

void InternalDebugWindow::addWatchVariable(const std::string& name, std::function<std::string()> value) {
    _variablesDebugPanel->addVariable(name, value);
    setSize(Vector2f(_variablesDebugPanel->getSize().x() + 13*2,
                     _variablesDebugPanel->getSize().y() + 8*2));
}

void InternalDebugWindow::drawContainer(DrawingContext& drawingContext) {
    //Draw the window itself
    InternalWindow::drawContainer(drawingContext);
}

} // namespace GUI
} // namespace Ego
