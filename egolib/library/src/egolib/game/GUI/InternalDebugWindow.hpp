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

/// @file egolib/game/GUI/InternalDebugWindow.hpp
/// @details InternalDebugWindow
/// @author Johan Jansen
#pragma once

#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include "egolib/game/GUI/InternalWindow.hpp"

namespace Ego {
namespace GUI {

class Label;

class VariablesDebugPanel : public Container {
public:
    VariablesDebugPanel();

    void addVariable(const std::string& name, std::function<std::string()> value);
    void update();
protected:
    void draw(DrawingContext& drawingContext) override;
    void drawContainer(DrawingContext& drawingContext) override;

private:
    std::unordered_map<std::string, std::function<std::string()>> _variables;
    std::unordered_map<std::string, std::shared_ptr<Label>> _labels;
};

class InternalDebugWindow : public InternalWindow {
public:
    InternalDebugWindow(const std::string &title);

    void addWatchVariable(const std::string &variableName, std::function<std::string()> lambda);

protected:
    void drawContainer(DrawingContext& drawingContext) override;

private:
    std::shared_ptr<VariablesDebugPanel> _variablesDebugPanel;
    std::unordered_map<std::string, std::function<std::string()>> _watchedVariables;
};

} // namespace GUI
} // namespace Ego
