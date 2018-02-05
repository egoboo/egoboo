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

/// @file egolib/game/GameStates/MainMenuState.hpp
/// @details The Main Menu of the game, the first screen presented to the players
/// @author Johan Jansen

#pragma once

#include "egolib/game/GameStates/GameState.hpp"

// Forward declarations.
namespace Ego {
namespace GUI{
class Button;
} // namespace GUI
} // namespace Ego

class MainMenuState : public GameState
{
public:
    MainMenuState();
    ~MainMenuState();

    void update() override;

    void beginState() override;

    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        drawContainer(drawingContext);
    }
protected:
    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override;

private:
    idlib::connection _onExitButtonClicked;
    idlib::connection _onOptionsButtonClicked;
    idlib::connection _onNewGameButtonClicked;
    idlib::connection _onLoadGameButtonClicked;
    idlib::connection _onDebugButtonClicked;
    idlib::connection _onMapEditorButtonClicked;
    std::forward_list<std::shared_ptr<Ego::GUI::Button>> _slidyButtons;
};
