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

/// @file egolib/game/GameStates/MapEditorSelectModuleState.hpp
/// @author Johan Jansen

#pragma once

#include "egolib/game/GameStates/GameState.hpp"
#include "egolib/Graphics/Font.hpp"

// Forward declarations.
class ModuleProfile;
namespace Ego::GUI {
class Button;
class Label;
} // namespace Ego::GUI

namespace Ego::GameStates
{

class MapEditorSelectModuleState : public GameState
{
public:
    MapEditorSelectModuleState();

    void update() override {};

    void beginState() override {};

    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        drawContainer(drawingContext);
    }

protected:
    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override;

private:
	void setSelectedModule(const std::shared_ptr<ModuleProfile> &profile);

private:
	std::shared_ptr<ModuleProfile> _selectedModule;
	std::shared_ptr<Ego::GUI::Button> _selectedButton;
	std::shared_ptr<Ego::GUI::Label> _moduleName;

    std::shared_ptr<Ego::Font::LaidTextRenderer> _moduleDescription;
};

} // namespace Ego::GameStates
