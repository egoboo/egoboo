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

/// @file game/GameStates/InGameMenuState.hpp
/// @details Menu while PlayingState is running in background
/// @author Johan Jansen

#pragma once

#include "game/GameStates/GameState.hpp"

// Forward declarations.
namespace Ego {
namespace GUI {
class Button;
class PlayingState;
} // namespace GUI
} // namespace Ego

class InGameMenuState : public GameState
{

public:

    InGameMenuState(GameState &backgroundState);

    void update() override;

    void beginState() override;

    void draw(Ego::GUI::DrawingContext& drawingContext) override {
        drawContainer(drawingContext);
    }
protected:

    void drawContainer(Ego::GUI::DrawingContext& drawingContext) override;

private:

    std::forward_list<std::shared_ptr<Ego::GUI::Button>> _slidyButtons;

    GameState &_backgroundState;
};
