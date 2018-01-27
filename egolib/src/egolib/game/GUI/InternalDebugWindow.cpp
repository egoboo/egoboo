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

/// @file egolib/game/GUI/InternalDebugWindow.cpp
/// @details InternalDebugWindow
/// @author Johan Jansen

#include "egolib/game/GUI/InternalDebugWindow.hpp"
#include "egolib/game/GUI/Label.hpp"
#include "egolib/game/GUI/JoinBounds.hpp"

namespace Ego {
namespace GUI {

VariablesDebugPanel::VariablesDebugPanel() : Container(),
    _variables(), _labels() {
    setSize(Vector2f(200, 75));
}

void VariablesDebugPanel::addVariable(const std::string& name, std::function<std::string()> value) {
    //Add variable to watch list
    _variables[name] = value;
}

void VariablesDebugPanel::draw(DrawingContext& drawingContext) {
    drawContainer(drawingContext);
    drawAll(drawingContext);
}

void VariablesDebugPanel::update() {
    for (const auto& variable : _variables) {
        // Check if we have a label for the specified variable already.
        auto it = _labels.find(variable.first);
        std::shared_ptr<Label> label = nullptr;
        // If we have no label ...
        if (_labels.cend() == it) {
            // .. create one.
            label = std::make_shared<Label>();
            addComponent(label);
            _labels[variable.first] = label;
            //_gameEngine->getUIManager()->getDefaultFont()
            label->setFont(_gameEngine->getUIManager()->getDefaultFont());
        } else {
            label = it->second;
        }
        label->setText(variable.first + ": " + variable.second());
    }
    float width = 0.0f, height = 0.0f;
    for (auto& label : _labels) {
        label.second->setPosition(Point2f(0.0f, height));
        width = std::max(width, label.second->getSize().x());
        height += label.second->getSize().y();
    }
    setWidth(width);
    setHeight(height);
}

void VariablesDebugPanel::drawContainer(DrawingContext& drawingContext) {
    update();
}

InternalDebugWindow::InternalDebugWindow(const std::string &title)
    : InternalWindow(title), _variablesDebugPanel() {
    _variablesDebugPanel = std::make_shared<VariablesDebugPanel>();
    addComponent(_variablesDebugPanel);
    _variablesDebugPanel->setPosition(Point2f(13 + 5, _titleBar->getHeight()));
    setSize(Vector2f(64, 64));
}

void InternalDebugWindow::addWatchVariable(const std::string& name, std::function<std::string()> value) {
    _variablesDebugPanel->addVariable(name, value);
}

void InternalDebugWindow::drawContainer(DrawingContext& drawingContext) {
    setSize(Vector2f(_variablesDebugPanel->getSize().x() + 13 * 2,
                     _variablesDebugPanel->getPosition().y() + _variablesDebugPanel->getSize().y() + 8 * 2));
    //Draw the window itself
    InternalWindow::drawContainer(drawingContext);
}

} // namespace GUI
} // namespace Ego
